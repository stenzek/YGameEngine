#include "Renderer/PrecompiledHeader.h"
#include "Renderer/WorldRenderers/CSMShadowMapRenderer.h"
#include "Renderer/Shaders/ShadowMapShader.h"
#include "Renderer/ShaderProgramSelector.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/RenderQueue.h"
#include "Renderer/RenderProfiler.h"
#include "Renderer/Renderer.h"
#include "Engine/Material.h"
#include "Engine/EngineCVars.h"
Log_SetChannel(CSMShadowMapRenderer);

CSMShadowMapRenderer::CSMShadowMapRenderer(uint32 shadowMapResolution /* = 256 */, PIXEL_FORMAT shadowMapFormat /* = PIXEL_FORMAT_D16_UNORM */, uint32 cascadeCount /* = 3 */, float splitLambda /* = 0.95f */)
    : m_shadowMapResolution(shadowMapResolution),
      m_shadowMapFormat(shadowMapFormat),
      m_cascadeCount(cascadeCount),
      m_splitLambda(splitLambda)
{
    Y_memzero(m_splitDepths, sizeof(m_splitDepths));
    m_renderQueue.SetAcceptingLights(false);
    m_renderQueue.SetAcceptingRenderPassMask(RENDER_PASS_SHADOW_MAP);
    m_renderQueue.SetAcceptingOccluders(false);
    m_renderQueue.SetAcceptingDebugObjects(false);
}

CSMShadowMapRenderer::~CSMShadowMapRenderer()
{

}

bool CSMShadowMapRenderer::AllocateShadowMap(ShadowMapData *pShadowMapData)
{
    // store vars
    pShadowMapData->IsActive = false;
    pShadowMapData->CascadeCount = m_cascadeCount;
    Y_memzero(pShadowMapData->ViewProjectionMatrices, sizeof(pShadowMapData->ViewProjectionMatrices));
    Y_memzero(pShadowMapData->CascadeFrustumEyeSpaceDepths, sizeof(pShadowMapData->CascadeFrustumEyeSpaceDepths));

    // create descriptor
    GPU_TEXTURE2DARRAY_DESC textureDesc(m_shadowMapResolution, m_shadowMapResolution, m_shadowMapFormat, GPU_TEXTURE_FLAG_SHADER_BINDABLE | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER, 1, m_cascadeCount);
    GPU_SAMPLER_STATE_DESC samplerStateDesc(TEXTURE_FILTER_MIN_MAG_MIP_POINT, TEXTURE_ADDRESS_MODE_BORDER, TEXTURE_ADDRESS_MODE_BORDER, TEXTURE_ADDRESS_MODE_CLAMP, float4::One, 0, 0, 0, 0, GPU_COMPARISON_FUNC_NEVER);

    // hardware pcf?
    if (CVars::r_shadow_use_hardware_pcf.GetBool())
    {
        samplerStateDesc.Filter = TEXTURE_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
        samplerStateDesc.ComparisonFunc = GPU_COMPARISON_FUNC_LESS;
    }

    // create it
    Log_PerfPrintf("CSMShadowMapRenderer::AllocateShadowMap: Allocating new %u x %u x %u %s texture", textureDesc.Width, textureDesc.Height, textureDesc.ArraySize, NameTable_GetNameString(NameTables::PixelFormat, m_shadowMapFormat));
    pShadowMapData->pShadowMapTexture = g_pRenderer->CreateTexture2DArray(&textureDesc, &samplerStateDesc);
    if (pShadowMapData->pShadowMapTexture == nullptr)
        return false;

    // create views
    Y_memzero(pShadowMapData->pShadowMapDSV, sizeof(pShadowMapData->pShadowMapDSV));
    for (uint32 arrayIndex = 0; arrayIndex < m_cascadeCount; arrayIndex++)
    {
        GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC dbvDesc(pShadowMapData->pShadowMapTexture, 0, arrayIndex);
        pShadowMapData->pShadowMapDSV[arrayIndex] = g_pRenderer->CreateDepthStencilBufferView(pShadowMapData->pShadowMapTexture, &dbvDesc);
        if (pShadowMapData->pShadowMapDSV[arrayIndex] == nullptr)
        {
            for (uint32 i = 0; i < arrayIndex; i++)
            {
                pShadowMapData->pShadowMapDSV[i]->Release();
                pShadowMapData->pShadowMapDSV[i] = nullptr;
            }
            pShadowMapData->pShadowMapTexture->Release();
            pShadowMapData->pShadowMapTexture = nullptr;
            return false;
        }
    }

    // ok
    return true;
}

void CSMShadowMapRenderer::FreeShadowMap(ShadowMapData *pShadowMapData)
{
    for (uint32 i = 0; i < pShadowMapData->CascadeCount; i++)
        pShadowMapData->pShadowMapDSV[i]->Release();

    pShadowMapData->pShadowMapTexture->Release();
}

void CSMShadowMapRenderer::CalculateSplitDepths(const Camera *pViewCamera, float shadowDistance)
{
    // far distance is max of camera far and shadow distance
    float nearDistance = pViewCamera->GetNearPlaneDistance();
    float farDistance = shadowDistance;

    // calculate each split depth
    for (uint32 splitIndex = 0; splitIndex <= m_cascadeCount; splitIndex++)
    {
        float fLog = nearDistance * Y_powf((farDistance / nearDistance), (float)splitIndex / (float)m_cascadeCount);
        float fLinear = nearDistance + (farDistance - nearDistance) * ((float)splitIndex / (float)m_cascadeCount);
        m_splitDepths[splitIndex] = fLog * m_splitLambda + fLinear * (1.0f - m_splitLambda);
    }

//     // calculate for far
//     float farDistance = viewFarDistance;
//     float fLog = viewNearDistance * Y_powf((farDistance / viewNearDistance), (float)(splitIndex + 1) / (float)splitCount);
//     float fLinear = viewNearDistance + (farDistance - viewNearDistance) * ((float)(splitIndex + 1) / (float)splitCount);
//     *pFarDistance = fLog * lambda + fLinear * (1.0f - lambda);
}

static void GetViewCameraFrustumCorners(float3 *pOutPoints, const Camera *pViewCamera, float overrideNearPlaneDistance, float overrideFarPlaneDistance)
{
    if (pViewCamera->GetProjectionType() == CAMERA_PROJECTION_TYPE_PERSPECTIVE)
    {
        // get forward vector
        float3 forwardVector((-pViewCamera->GetViewMatrix().GetRow(2).xyz()).Normalize());
        float3 upVector(pViewCamera->GetViewMatrix().GetRow(1).xyz().Normalize());
        float3 leftVector((-pViewCamera->GetViewMatrix().GetRow(0).xyz()).Normalize());

        // get near/far plane centers
        float3 nearPlaneCenter(pViewCamera->GetPosition() + forwardVector * overrideNearPlaneDistance);
        float3 farPlaneCenter(pViewCamera->GetPosition() + forwardVector * overrideFarPlaneDistance);

        // get v/h extent locations from center
        float viewAngle = Math::DegreesToRadians(pViewCamera->GetPerspectiveFieldOfView());
        float nearExtentDistance = Y_tanf(viewAngle / 2.0f) * overrideNearPlaneDistance;
        float3 nearExtentY = upVector * nearExtentDistance;
        float3 nearExtentX = leftVector * (pViewCamera->GetPerspectiveAspect() * nearExtentDistance);

        float farExtentDistance = Y_tanf(viewAngle / 2.0f) * overrideFarPlaneDistance;
        float3 farExtentY = upVector * farExtentDistance;
        float3 farExtentX = leftVector * (pViewCamera->GetPerspectiveAspect() * farExtentDistance);

        // find corners by adding/subtracting extents
        pOutPoints[0] = nearPlaneCenter + nearExtentY - nearExtentX;
        pOutPoints[1] = nearPlaneCenter + nearExtentY + nearExtentX;
        pOutPoints[2] = nearPlaneCenter - nearExtentY + nearExtentX;
        pOutPoints[3] = nearPlaneCenter - nearExtentY - nearExtentX;
        pOutPoints[4] = farPlaneCenter + farExtentY - farExtentX;
        pOutPoints[5] = farPlaneCenter + farExtentY + farExtentX;
        pOutPoints[6] = farPlaneCenter - farExtentY + farExtentX;
        pOutPoints[7] = farPlaneCenter - farExtentY - farExtentX;
    }
    else
    {
        Camera cloneCamera(*pViewCamera);
        cloneCamera.SetNearFarPlaneDistances(overrideNearPlaneDistance, overrideFarPlaneDistance);
        cloneCamera.GetFrustum().GetCornerVertices(pOutPoints);
    }
}

void CSMShadowMapRenderer::CalculateViewDependantVariables(const Camera *pViewCamera)
{
    // get world-space corners of view camera
    //float3 frustumCornersWS[8];
    float3 *frustumCornersWS = m_frustumCornersVS;
    GetViewCameraFrustumCorners(frustumCornersWS, pViewCamera, pViewCamera->GetNearPlaneDistance(), pViewCamera->GetFarPlaneDistance());

    // transform world-space corners of view camera to view space
    for (uint32 i = 0; i < 8; i++)
        m_frustumCornersVS[i] = pViewCamera->GetViewMatrix().TransformPoint(frustumCornersWS[i]);
}

void CSMShadowMapRenderer::BuildCascadeCamera(Camera *pOutCascadeCamera, const Camera *pViewCamera, const float3 &lightDirection, uint32 splitIndex, uint32 splitCount, float lambda, float shadowDrawDistance, const RenderWorld *pRenderWorld)
{
    // bring the nearZ back to 80%, for better results at split point
    float nearZ = (splitIndex > 0) ? m_splitDepths[splitIndex] * 0.8f : m_splitDepths[splitIndex];
    float farZ = m_splitDepths[splitIndex + 1];

    // shorten view frustum according to shadow view distance
    float3 splitFrustumCornersVS[8];
    for (uint32 i = 0; i < 4; i++)
        splitFrustumCornersVS[i] = m_frustumCornersVS[i + 4] * (nearZ / pViewCamera->GetFarPlaneDistance());
    for (uint32 i = 4; i < 8; i++)
        splitFrustumCornersVS[i] = m_frustumCornersVS[i] * (farZ / pViewCamera->GetFarPlaneDistance());

    // transform back to world-space
    float3 frustumCornersWS[8];
    for (uint32 i = 0; i < 8; i++)
        frustumCornersWS[i] = pViewCamera->GetInverseViewMatrix().TransformPoint(splitFrustumCornersVS[i]);

    // find the centroid
    float3 frustumCentroid(float3::Zero);
    for (uint32 i = 0; i < 8; i++)
        frustumCentroid += frustumCornersWS[i];
    frustumCentroid /= 8.0f;

    // position shadow-caster camera so that it's looking at the centroid, backed up direction of light
    float distFromCentroid = Max((farZ - nearZ), splitFrustumCornersVS[4].Distance(splitFrustumCornersVS[5])) + 100.0f;
    pOutCascadeCamera->SetPosition(frustumCentroid - (lightDirection * distFromCentroid));
    pOutCascadeCamera->SetRotation((Quaternion::FromTwoUnitVectors(float3::NegativeUnitZ, lightDirection) * Quaternion::FromEulerAngles(-90.0f, 0.0f, 0.0f)).Normalize());

    // determine position of frustum corners in light space
    float3 frustumCornersLS[8];
    for (uint32 i = 0; i < 8; i++)
        frustumCornersLS[i] = pOutCascadeCamera->GetViewMatrix().TransformPoint(frustumCornersWS[i]);

    // create orthographic projection by sizing bounding box to frustum coordinates in light space
    float3 minBounds(frustumCornersLS[0]);
    float3 maxBounds(frustumCornersLS[0]);
    for (uint32 i = 1; i < 8; i++)
    {
        minBounds = minBounds.Min(frustumCornersLS[i]);
        maxBounds = maxBounds.Max(frustumCornersLS[i]);
    }

    // snap camera in one texel increments
    float diagonalLength = (frustumCornersWS[0] - frustumCornersWS[6]).Length() + 2.0f;
    float worldUnitsPerTexel = diagonalLength / (float)m_shadowMapResolution;
    float3 borderOffset((float3(diagonalLength) - (maxBounds - minBounds)) * 0.5f);
    maxBounds += borderOffset;
    minBounds -= borderOffset;

    minBounds /= worldUnitsPerTexel;
    minBounds = minBounds.Floor();
    minBounds *= worldUnitsPerTexel;

    maxBounds /= worldUnitsPerTexel;
    maxBounds = maxBounds.Floor();
    maxBounds *= worldUnitsPerTexel;

    // create orthographic camera
    const float nearClipOffset = 100.0f;
    pOutCascadeCamera->SetProjectionType(CAMERA_PROJECTION_TYPE_ORTHOGRAPHIC);
    pOutCascadeCamera->SetOrthographicWindow(minBounds.x, maxBounds.x, minBounds.y, maxBounds.y);
    pOutCascadeCamera->SetNearFarPlaneDistances(-maxBounds.z - nearClipOffset, -minBounds.z);  
    pOutCascadeCamera->SetObjectCullDistance(shadowDrawDistance);

    // align to texels
    {
        float halfShadowMapSize = (float)m_shadowMapResolution * 0.5f;
        float2 shadowOrigin(pOutCascadeCamera->GetViewProjectionMatrix().TransformPoint(float3::Zero).xy() * halfShadowMapSize);
        float2 roundedOrigin(shadowOrigin.Round());
        float2 rounding = (roundedOrigin - shadowOrigin) / halfShadowMapSize;
        float4x4 roundingMatrix = float4x4::MakeTranslationMatrix(rounding.x, rounding.y, 0.0f);

        // apply after projection
        pOutCascadeCamera->SetProjectionMatrix(roundingMatrix * pOutCascadeCamera->GetProjectionMatrix());
    }
}

void CSMShadowMapRenderer::DrawShadowMap(GPUContext *pGPUContext, ShadowMapData *pShadowMapData, const Camera *pViewCamera, float shadowDistance, const RenderWorld *pRenderWorld, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight, RenderProfiler *pRenderProfiler)
{
    // draw using multipass technique
    DrawMultiPass(pGPUContext, pShadowMapData, pViewCamera, shadowDistance, pRenderWorld, pLight, pRenderProfiler);
}

void CSMShadowMapRenderer::DrawMultiPass(GPUContext *pGPUContext, ShadowMapData *pShadowMapData, const Camera *pViewCamera, float shadowDistance, const RenderWorld *pRenderWorld, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight, RenderProfiler *pRenderProfiler)
{
    // work out shadow draw distance
    float shadowDrawDistance = Min(shadowDistance, pViewCamera->GetFarPlaneDistance() - pViewCamera->GetNearPlaneDistance());

    // set common states
    RENDERER_VIEWPORT shadowMapViewport(0, 0, m_shadowMapResolution, m_shadowMapResolution, 0.0f, 1.0f);
    pGPUContext->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK));
    pGPUContext->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(true, true, GPU_COMPARISON_FUNC_LESS), 0);
    pGPUContext->SetViewport(&shadowMapViewport);

    // calculate split depths
    CalculateSplitDepths(pViewCamera, shadowDrawDistance);

    // calculate scene-dependant variables
    CalculateViewDependantVariables(pViewCamera);

    // draw each cascade individually
    for (uint32 i = 0; i < m_cascadeCount; i++)
    {
        // invoke the clear first of this layer
        pGPUContext->SetRenderTargets(0, nullptr, pShadowMapData->pShadowMapDSV[i]);
        pGPUContext->ClearTargets(false, true, false, float4::Zero, 1.0f);

        // get camera
        Camera lightCamera;
        BuildCascadeCamera(&lightCamera, pViewCamera, pLight->Direction, i, m_cascadeCount, m_splitLambda, shadowDrawDistance, pRenderWorld);

        // add cascade camera
        RENDER_PROFILER_ADD_CAMERA(pRenderProfiler, &lightCamera, String::FromFormat("PSSM cascade %u camera", i));

        // store vp matrix
        pShadowMapData->CascadeFrustumEyeSpaceDepths[i] = m_splitDepths[i + 1];
        pShadowMapData->ViewProjectionMatrices[i] = lightCamera.GetViewProjectionMatrix();
        g_pRenderer->CorrectProjectionMatrix(pShadowMapData->ViewProjectionMatrices[i]);

        // follow the normal process... clear queue
        m_renderQueue.Clear();

        // find renderables
        RENDER_PROFILER_BEGIN_SECTION(pRenderProfiler, "DiscoverRenderables", false);
        {
            // find everything in this cascade's frustum
            pRenderWorld->EnumerateRenderablesInFrustum(lightCamera.GetFrustum(), [this, &lightCamera](const RenderProxy *pRenderProxy)
            {
                // add to render queue
                pRenderProxy->QueueForRender(&lightCamera, &m_renderQueue);
            });
        }
        RENDER_PROFILER_END_SECTION(pRenderProfiler);

        // got any?
        if (!m_renderQueue.GetQueueSize())
            continue;

        // set constants
        pGPUContext->GetConstants()->SetFromCamera(lightCamera, true);

        // sort queue
        m_renderQueue.Sort();

        // draw opaque objects
        RENDER_PROFILER_BEGIN_SECTION(pRenderProfiler, "DrawOpaqueObjects", true);
        {           
            // initialize selector -- fixme for global flags?
            ShaderProgramSelector shaderSelector(0);
            shaderSelector.SetBaseShader(OBJECT_TYPEINFO(ShadowMapShader), 0);

            // loop renderables
            RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_renderQueue.GetOpaqueRenderables().GetBasePointer();
            RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_renderQueue.GetOpaqueRenderables().GetBasePointer() + m_renderQueue.GetOpaqueRenderables().GetSize();
            for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
            {
                DebugAssert(pQueueEntry->RenderPassMask & RENDER_PASS_SHADOW_MAP);

                // For now, only masked materials are drawn with clipping
                shaderSelector.SetVertexFactory(pQueueEntry->pVertexFactoryTypeInfo, pQueueEntry->VertexFactoryFlags);
                shaderSelector.SetMaterial((pQueueEntry->pMaterial->GetShader()->GetBlendMode() == MATERIAL_BLENDING_MODE_MASKED) ? pQueueEntry->pMaterial : nullptr);

                // only continue with shader
                ShaderProgram *pShaderProgram = shaderSelector.MakeActive(pGPUContext);
                if (pShaderProgram != nullptr)
                {
                    pQueueEntry->pRenderProxy->SetupForDraw(&lightCamera, pQueueEntry, pGPUContext, pShaderProgram);
                    pQueueEntry->pRenderProxy->DrawQueueEntry(&lightCamera, pQueueEntry, pGPUContext);
                }
            }
        }
        RENDER_PROFILER_END_SECTION(pRenderProfiler);

        // draw transparent objects
        RENDER_PROFILER_BEGIN_SECTION(pRenderProfiler, "DrawTranslucentObjects", true);
        {
            // initialize selector
            ShaderProgramSelector shaderSelector(0);
            shaderSelector.SetBaseShader(OBJECT_TYPEINFO(ShadowMapShader), 0);

            // loop renderables
            RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_renderQueue.GetTranslucentRenderables().GetBasePointer();
            RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_renderQueue.GetTranslucentRenderables().GetBasePointer() + m_renderQueue.GetTranslucentRenderables().GetSize();
            for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
            {
                DebugAssert(pQueueEntry->RenderPassMask & RENDER_PASS_SHADOW_MAP);

                // For now, only masked materials are drawn with clipping
                shaderSelector.SetVertexFactory(pQueueEntry->pVertexFactoryTypeInfo, pQueueEntry->VertexFactoryFlags);
                shaderSelector.SetMaterial((pQueueEntry->pMaterial->GetShader()->GetBlendMode() != MATERIAL_BLENDING_MODE_NONE) ? pQueueEntry->pMaterial : nullptr);

                // only continue with shader
                ShaderProgram *pShaderProgram = shaderSelector.MakeActive(pGPUContext);
                if (pShaderProgram != nullptr)
                {
                    pQueueEntry->pRenderProxy->SetupForDraw(&lightCamera, pQueueEntry, pGPUContext, pShaderProgram);
                    pQueueEntry->pRenderProxy->DrawQueueEntry(&lightCamera, pQueueEntry, pGPUContext);
                }
            }
        }
        RENDER_PROFILER_END_SECTION(pRenderProfiler);
    }

    // set everything else to infinte as not to break it
    for (uint32 i = m_cascadeCount; i < MaxCascadeCount; i++)
        pShadowMapData->CascadeFrustumEyeSpaceDepths[i] = Y_FLT_INFINITE;
}
