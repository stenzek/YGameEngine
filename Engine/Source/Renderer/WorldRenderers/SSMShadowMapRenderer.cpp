#include "Renderer/PrecompiledHeader.h"
#include "Renderer/WorldRenderers/SSMShadowMapRenderer.h"
#include "Renderer/Shaders/ShadowMapShader.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/RenderQueue.h"
#include "Renderer/RenderProfiler.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderProgramSelector.h"
#include "Engine/Material.h"
#include "Engine/Profiling.h"
Log_SetChannel(SSMShadowMapRenderer);

SSMShadowMapRenderer::SSMShadowMapRenderer(uint32 shadowMapResolution /* = 256 */, PIXEL_FORMAT shadowMapFormat /* = PIXEL_FORMAT_D16_UNORM */)
    : m_shadowMapResolution(shadowMapResolution),
      m_shadowMapFormat(shadowMapFormat)
{
    m_renderQueue.SetAcceptingLights(false);
    m_renderQueue.SetAcceptingRenderPassMask(RENDER_PASS_SHADOW_MAP);
    m_renderQueue.SetAcceptingOccluders(false);
    m_renderQueue.SetAcceptingDebugObjects(false);
}

SSMShadowMapRenderer::~SSMShadowMapRenderer()
{

}

bool SSMShadowMapRenderer::AllocateShadowMap(ShadowMapData *pShadowMapData)
{
    // store vars
    pShadowMapData->IsActive = false;

    // allocate texture
    GPU_TEXTURE2D_DESC textureDesc(m_shadowMapResolution, m_shadowMapResolution, m_shadowMapFormat, GPU_TEXTURE_FLAG_SHADER_BINDABLE | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER, 1);
    GPU_SAMPLER_STATE_DESC samplerStateDesc(TEXTURE_FILTER_MIN_MAG_MIP_POINT, TEXTURE_ADDRESS_MODE_BORDER, TEXTURE_ADDRESS_MODE_BORDER, TEXTURE_ADDRESS_MODE_CLAMP, float4::One, 0, 0, 0, 0, GPU_COMPARISON_FUNC_NEVER);

    //samplerStateDesc.Filter = TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
    //samplerStateDesc.ComparisonFunc = RENDERER_COMPARISON_FUNC_LESS_EQUAL;

    Log_PerfPrintf("SSMShadowMapRenderer::AllocateShadowMap: Creating new %u x %u %s texture", m_shadowMapResolution, m_shadowMapResolution, NameTable_GetNameString(NameTables::PixelFormat, m_shadowMapFormat));
    pShadowMapData->pShadowMapTexture = g_pRenderer->CreateTexture2D(&textureDesc, &samplerStateDesc);
    if (pShadowMapData->pShadowMapTexture == nullptr)
        return false;

    GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC dsvDesc(pShadowMapData->pShadowMapTexture, 0);
    pShadowMapData->pShadowMapDSV = g_pRenderer->CreateDepthStencilBufferView(pShadowMapData->pShadowMapTexture, &dsvDesc);
    if (pShadowMapData->pShadowMapDSV == nullptr)
    {
        pShadowMapData->pShadowMapTexture->Release();
        pShadowMapData->pShadowMapTexture = nullptr;
        return false;
    }

    // ok
    return true;
}

void SSMShadowMapRenderer::FreeShadowMap(ShadowMapData *pShadowMapData)
{
    pShadowMapData->pShadowMapDSV->Release();
    pShadowMapData->pShadowMapTexture->Release();
}

static void BuildDirectionalLightCamera(Camera *pShadowCamera, const Camera *pViewCamera, const float3 &lightDirection, float shadowDistance)
{
    const float extraBackup = 20.0f;
    const float nearClip = 1.0f;

//     // calc camera up vector
//     Vector3 lightCameraUpVector;
//     float lightDirectionDotWorldUp = lightDirection.Dot(float3::UnitZ);
//     if (lightDirectionDotWorldUp > 0.5f)
//         lightCameraUpVector = float3::NegativeUnitY;
//     else if (lightDirectionDotWorldUp < -0.5f)
//         lightCameraUpVector = float3::UnitY;
//     else
//         lightCameraUpVector = float3::UnitZ;

    //Log_DevPrintf("dir = %s, dotz = %s", StringConverter::Float3ToString(lightDirection).GetCharArray(), StringConverter::FloatToString(lightDirection.Dot(float3::UnitZ)).GetCharArray());

    // convert the direction to a rotation from the default direction
    Quaternion cameraRotation(Quaternion::FromTwoUnitVectors(float3::NegativeUnitZ, lightDirection));

    // since our camera normally points down the y axis, rotate it so the default is pointing downwards
    cameraRotation *= Quaternion::FromEulerAngles(-90.0f, 0.0f, 0.0f);
    cameraRotation.NormalizeInPlace();

#if 0
    // create light aabb in view-space
    AABox lightViewAABB(float3(-shadowDistance, -shadowDistance, -shadowDistance), float3(shadowDistance, shadowDistance, shadowDistance));
    lightViewAABB.ApplyTransform(pViewCamera->GetViewMatrix());

    // rotate into light space
    lightViewAABB.ApplyTransform(cameraRotation.GetFloat3x3());

    // calculate the light position
    float lightDepth = lightViewAABB.GetMaxBounds().z - lightViewAABB.GetMinBounds().z;
    float3 lightPositionWS(Camera::TransformFromCameraToWorldCoordinateSystem(lightViewAABB.GetCenter()));
    lightPositionWS += lightDirection * (-lightDepth / 2.0f);

    // set the window
    pShadowCamera->SetPosition(lightPositionWS);
    pShadowCamera->SetRotation(cameraRotation);
    pShadowCamera->SetProjectionType(CAMERA_PROJECTION_TYPE_ORTHOGRAPHIC);
    pShadowCamera->SetOrthographicWindow(lightViewAABB.GetMaxBounds().x - lightViewAABB.GetMinBounds().x, lightViewAABB.GetMaxBounds().y - lightViewAABB.GetMinBounds().y);
    pShadowCamera->SetNearFarPlaneDistances(1.0f, lightDepth);

#else

    // create camera
    Camera tempCamera(*pViewCamera);
    tempCamera.SetNearFarPlaneDistances(1.0f, shadowDistance);
    Sphere frustumBoundingSphere(tempCamera.GetFrustum().GetBoundingSphere());
    float backupDist = extraBackup + nearClip + frustumBoundingSphere.GetRadius();
    //float windowSize = frustumBoundingSphere.GetRadius() * 2.0f;
    float windowSize = frustumBoundingSphere.GetRadius();
    pShadowCamera->SetPosition(float3(frustumBoundingSphere.GetCenter()) - (lightDirection * backupDist));
    //pShadowCamera->LookDirection(lightDirection, lightCameraUpVector);
    pShadowCamera->SetRotation(cameraRotation);
    pShadowCamera->SetProjectionType(CAMERA_PROJECTION_TYPE_ORTHOGRAPHIC);
    pShadowCamera->SetOrthographicWindow(windowSize, windowSize);
    pShadowCamera->SetNearPlaneDistance(nearClip);
    pShadowCamera->SetFarPlaneDistance((backupDist + (frustumBoundingSphere.GetRadius() * 2.0f)));
#endif

    pShadowCamera->SetObjectCullDistance(shadowDistance);

    //pShadowCamera->SetPosition(float3(250, 250, 500.0f));
    //pShadowCamera->SetOrthographicWindow(1000, 1000);
    //lightCamera.SetPosition(float3(0, 0, 500.0f));
    //lightCamera.SetWindowSize(1000, 1000);
    //pShadowCamera->SetFarPlaneDistance(1.0f);
    //pShadowCamera->SetFarPlaneDistance(500.0f);
    //pShadowCamera->SetFarPlaneDistance(1000.0f);
    //pShadowCamera->SetFarPlaneDistance(8000.0f);
}

void SSMShadowMapRenderer::DrawDirectionalShadowMap(GPUContext *pGPUContext, ShadowMapData *pShadowMapData, const RenderWorld *pRenderWorld, const Camera *pViewCamera, float shadowDistance, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight, RenderProfiler *pRenderProfiler)
{
    // fix shadow distance
    shadowDistance = Min(shadowDistance, pViewCamera->GetFarPlaneDistance() - pViewCamera->GetNearPlaneDistance());

    // build base view parameters
    Camera lightCamera;
    BuildDirectionalLightCamera(&lightCamera, pViewCamera, pLight->Direction, shadowDistance);

    // build viewport
    RENDERER_VIEWPORT shadowMapViewport(0, 0, m_shadowMapResolution, m_shadowMapResolution, 0.0f, 1.0f);

    // set and clear the shadow map
    pGPUContext->SetRenderTargets(0, nullptr, pShadowMapData->pShadowMapDSV);
    pGPUContext->SetViewport(&shadowMapViewport);
    pGPUContext->ClearTargets(false, true, false, float4::Zero, 1.0f);

    // align to texels
    {
        float halfShadowMapSize = (float)m_shadowMapResolution * 0.5f;
        float3 shadowOrigin(lightCamera.GetViewProjectionMatrix().TransformPoint(float3::Zero));
        shadowOrigin *= halfShadowMapSize;
        float2 roundedOrigin(Math::Round(shadowOrigin.x), Math::Round(shadowOrigin.y));
        float2 rounding = roundedOrigin - shadowOrigin.xy();
        rounding /= halfShadowMapSize;
        float4x4 roundingMatrix = float4x4::MakeTranslationMatrix(rounding.x, rounding.y, 0.0f);

        // apply after projection
        lightCamera.SetProjectionMatrix(roundingMatrix * lightCamera.GetProjectionMatrix());
    }

    // store matrix to shadow map data
    pShadowMapData->IsActive = true;
    pShadowMapData->ViewProjectionMatrix = lightCamera.GetViewProjectionMatrix();
    g_pRenderer->CorrectProjectionMatrix(pShadowMapData->ViewProjectionMatrix);

    // clear render queue
    m_renderQueue.Clear();

    // find renderables
    RENDER_PROFILER_BEGIN_SECTION(pRenderProfiler, "DiscoverRenderables", false);
    {
        MICROPROFILE_SCOPEI("SSMShadowMapRenderer", "EnumerateRenderables", MAKE_COLOR_R8G8B8_UNORM(0, 200, 0));

        // enumerate everything in frustum
        pRenderWorld->EnumerateRenderablesInFrustum(lightCamera.GetFrustum(), [this, &lightCamera](const RenderProxy *pRenderProxy)
        {
            // add to render queue
            pRenderProxy->QueueForRender(&lightCamera, &m_renderQueue);
        });

        // sort renderables
        m_renderQueue.Sort();
    }
    RENDER_PROFILER_END_SECTION(pRenderProfiler);

    // no renderables?
    if (m_renderQueue.GetQueueSize() == 0)
        return;

    // set render targets, for pipelining we do this before sorting
    pGPUContext->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK));
    pGPUContext->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(true, true, GPU_COMPARISON_FUNC_LESS), 0);

    // set up view-dependent constants
    pGPUContext->GetConstants()->SetFromCamera(lightCamera, true);

    // init selector @TODO global flags
    ShaderProgramSelector programSelector(0);
    programSelector.SetBaseShader(OBJECT_TYPEINFO(ShadowMapShader), 0);

    // opaque
    // TODO: Use ShaderProgramSelector
    RENDER_PROFILER_BEGIN_SECTION(pRenderProfiler, "DrawOpaqueObjects", true);
    {
        RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_renderQueue.GetOpaqueRenderables().GetBasePointer();
        RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_renderQueue.GetOpaqueRenderables().GetBasePointer() + m_renderQueue.GetOpaqueRenderables().GetSize();
        MICROPROFILE_SCOPEI("SSMShadowMapRenderer", "DrawOpaqueObjects", MAKE_COLOR_R8G8B8_UNORM(255, 100, 0));
        MICROPROFILE_SCOPEGPUI("DrawOpaqueObjects", MAKE_COLOR_R8G8B8_UNORM(255, 100, 0));

        for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
        {
            // Select appropriate shader
            // For now, only masked materials are drawn with clipping
            if (pQueueEntry->pMaterial->GetShader()->GetBlendMode() == MATERIAL_BLENDING_MODE_MASKED)
            {
                programSelector.SetVertexFactory(pQueueEntry->pVertexFactoryTypeInfo, pQueueEntry->VertexFactoryFlags);
                programSelector.SetMaterial(pQueueEntry->pMaterial);

                ShaderProgram *pShaderProgram = programSelector.MakeActive(pGPUContext);
                if (pShaderProgram != nullptr)
                {
                    pQueueEntry->pRenderProxy->SetupForDraw(&lightCamera, pQueueEntry, pGPUContext, pShaderProgram);
                    pQueueEntry->pRenderProxy->DrawQueueEntry(&lightCamera, pQueueEntry, pGPUContext);
                }
            }
            else
            {
                // Otherwise, use shadowmap shader without material
                programSelector.SetVertexFactory(pQueueEntry->pVertexFactoryTypeInfo, pQueueEntry->VertexFactoryFlags);
                programSelector.SetMaterial(nullptr);

                ShaderProgram *pShaderProgram = programSelector.MakeActive(pGPUContext);
                if (pShaderProgram != nullptr)
                {
                    pQueueEntry->pRenderProxy->SetupForDraw(&lightCamera, pQueueEntry, pGPUContext, pShaderProgram);
                    pQueueEntry->pRenderProxy->DrawQueueEntry(&lightCamera, pQueueEntry, pGPUContext);
                }
            }
        }
    }
    RENDER_PROFILER_END_SECTION(pRenderProfiler);

    // translucent
    RENDER_PROFILER_BEGIN_SECTION(pRenderProfiler, "DrawTranslucentObjects", true);
    {
        RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_renderQueue.GetTranslucentRenderables().GetBasePointer();
        RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_renderQueue.GetTranslucentRenderables().GetBasePointer() + m_renderQueue.GetTranslucentRenderables().GetSize();
        MICROPROFILE_SCOPEI("SSMShadowMapRenderer", "DrawTranslucentObjects", MAKE_COLOR_R8G8B8_UNORM(255, 0, 0));
        MICROPROFILE_SCOPEGPUI("DrawTranslucentObjects", MAKE_COLOR_R8G8B8_UNORM(255, 0, 0));

        for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
        {
            if (pQueueEntry->RenderPassMask & RENDER_PASS_SHADOW_MAP)
            {
                // For now, only masked materials are drawn with clipping, so just use the regular shader here
                programSelector.SetVertexFactory(pQueueEntry->pVertexFactoryTypeInfo, pQueueEntry->VertexFactoryFlags);
                programSelector.SetMaterial((pQueueEntry->pMaterial->GetShader()->GetBlendMode() != MATERIAL_BLENDING_MODE_NONE) ? pQueueEntry->pMaterial : nullptr);

                ShaderProgram *pShaderProgram = programSelector.MakeActive(pGPUContext);
                if (pShaderProgram != nullptr)
                {
                    pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());
                    pQueueEntry->pRenderProxy->SetupForDraw(&lightCamera, pQueueEntry, pGPUContext, pShaderProgram);
                    pQueueEntry->pRenderProxy->DrawQueueEntry(&lightCamera, pQueueEntry, pGPUContext);
                }
            }
        }
    }
    RENDER_PROFILER_END_SECTION(pRenderProfiler);
}

void SSMShadowMapRenderer::DrawSpotShadowMap(GPUContext *pGPUContext, ShadowMapData *pShadowMapData, const RenderWorld *pRenderWorld, const Camera *pViewCamera, float shadowDistance, const RENDER_QUEUE_SPOT_LIGHT_ENTRY *pLight, RenderProfiler *pRenderProfiler)
{

}
