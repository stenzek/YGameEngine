#include "Renderer/PrecompiledHeader.h"
#include "Renderer/WorldRenderers/CubeMapShadowMapRenderer.h"
#include "Renderer/Shaders/ShadowMapShader.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/RenderQueue.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderProgram.h"
#include "Engine/Camera.h"
#include "Engine/Material.h"
#include "Engine/EngineCVars.h"
#include "Engine/Profiling.h"

Log_SetChannel(CubeMapShadowMapRenderer);

CubeMapShadowMapRenderer::CubeMapShadowMapRenderer(uint32 shadowMapResolution /* = 256 */, PIXEL_FORMAT shadowMapFormat /* = PIXEL_FORMAT_D16_UNORM */)
    : m_shadowMapResolution(shadowMapResolution),
      m_shadowMapFormat(shadowMapFormat)
{
    m_renderQueue.SetAcceptingLights(false);
    m_renderQueue.SetAcceptingRenderPassMask(RENDER_PASS_SHADOW_MAP);
    m_renderQueue.SetAcceptingOccluders(false);
    m_renderQueue.SetAcceptingDebugObjects(false);
}

CubeMapShadowMapRenderer::~CubeMapShadowMapRenderer()
{

}

void CubeMapShadowMapRenderer::BuildCubeMapCamera(Camera *pCamera, const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight, CUBE_FACE face)
{
    // cube faces and where they should point:
    // Positive-X = Right
    // Negative-X = Left
    // Positive-Y = Up
    // Negative-Y = Down
    // Positive-Z = Forward
    // Negative-Z = Backwards

    // remember normal camera faces down the y axis
    static const float cameraRotations[CUBE_FACE_COUNT][3] =
    {
        { 0.0f, 0.0f, -90.0f }, // looking to the right
        { 0.0f, 0.0f, 90.0f },  // looking to the left
        { 90.0f, 0.0f, 0.0f },  // looking up
        { -90.0f, 0.0f, 0.0f }, // looking down
        { 0.0f, 0.0f, 0.0f },   // looking forwards
        { 0.0f, 0.0f, 180.0f }, // looking backwards
    };

    // create a look at orthographic camera
    DebugAssert(face < CUBE_FACE_COUNT);
    pCamera->SetPosition(pLight->Position);
    pCamera->SetRotation(Quaternion::FromEulerAngles(cameraRotations[face][0], cameraRotations[face][1], cameraRotations[face][2]));
    pCamera->SetProjectionType(CAMERA_PROJECTION_TYPE_PERSPECTIVE);
    pCamera->SetPerspectiveFieldOfView(90.0f);
    pCamera->SetNearFarPlaneDistances(0.1f, pLight->Range);
    //pCamera->SetNearFarPlaneDistances(0.1f, 100.0f);
}

bool CubeMapShadowMapRenderer::AllocateShadowMap(ShadowMapData *pShadowMapData)
{
    // store vars
    pShadowMapData->IsActive = false;

    // allocate texture
    GPU_TEXTURECUBE_DESC textureDesc(m_shadowMapResolution, m_shadowMapResolution, m_shadowMapFormat, GPU_TEXTURE_FLAG_SHADER_BINDABLE | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER, 1);
    GPU_SAMPLER_STATE_DESC samplerStateDesc(TEXTURE_FILTER_MIN_MAG_MIP_POINT, TEXTURE_ADDRESS_MODE_BORDER, TEXTURE_ADDRESS_MODE_BORDER, TEXTURE_ADDRESS_MODE_CLAMP, float4::One, 0, 0, 0, 0, GPU_COMPARISON_FUNC_NEVER);

    // hardware pcf?
    if (CVars::r_shadow_use_hardware_pcf.GetBool())
    {
        samplerStateDesc.Filter = TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        samplerStateDesc.ComparisonFunc = GPU_COMPARISON_FUNC_LESS;
    }

    Log_PerfPrintf("CubeMapShadowMapRenderer::AllocateShadowMap: Creating new %u x %u %s texture", m_shadowMapResolution, m_shadowMapResolution, NameTable_GetNameString(NameTables::PixelFormat, m_shadowMapFormat));
    pShadowMapData->pShadowMapTexture = g_pRenderer->CreateTextureCube(&textureDesc, &samplerStateDesc);
    if (pShadowMapData->pShadowMapTexture == nullptr)
        return false;

    // create dsv
    for (uint32 cubeFace = 0; cubeFace < CUBE_FACE_COUNT; cubeFace++)
    {
        GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC dsvDesc(pShadowMapData->pShadowMapTexture, 0, (CUBE_FACE)cubeFace);
        pShadowMapData->pShadowMapDSV[cubeFace] = g_pRenderer->CreateDepthStencilBufferView(pShadowMapData->pShadowMapTexture, &dsvDesc);
        if (pShadowMapData->pShadowMapDSV[cubeFace] == nullptr)
        {
            for (uint32 i = 0; i < cubeFace; i++)
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

void CubeMapShadowMapRenderer::FreeShadowMap(ShadowMapData *pShadowMapData)
{
    for (uint32 i = 0; i < CUBE_FACE_COUNT; i++)
        pShadowMapData->pShadowMapDSV[i]->Release();

    pShadowMapData->pShadowMapTexture->Release();
}

void CubeMapShadowMapRenderer::DrawShadowMap(GPUCommandList *pCommandList, ShadowMapData *pShadowMapData, const Camera *pViewCamera, float shadowDistance, const RenderWorld *pRenderWorld, const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight)
{
    // get shadow distance
    shadowDistance = Min(shadowDistance, pViewCamera->GetFarPlaneDistance() - pViewCamera->GetNearPlaneDistance());

    // common device parameters
    RENDERER_VIEWPORT shadowMapViewport(0, 0, m_shadowMapResolution, m_shadowMapResolution, 0.0f, 1.0f);
    pCommandList->SetViewport(&shadowMapViewport);

    // for each cube face
    for (uint32 cubeFaceIndex = 0; cubeFaceIndex < CUBE_FACE_COUNT; cubeFaceIndex++)
    {
        CUBE_FACE cubeFace = (CUBE_FACE)cubeFaceIndex;

        // build camera
        Camera lightCamera;
        BuildCubeMapCamera(&lightCamera, pLight, cubeFace);

        // add camera
        //RENDER_PROFILER_ADD_CAMERA(pRenderProfiler, &lightCamera, String::FromFormat("Point Shadow Camera Face %u", cubeFaceIndex));

        // set+clear the shadow map
        pCommandList->SetRenderTargets(0, nullptr, pShadowMapData->pShadowMapDSV[cubeFaceIndex]);
        pCommandList->ClearTargets(false, true, false, float4::Zero, 1.0f);

        // clear render queue
        m_renderQueue.Clear();

        // find renderables
        {
            MICROPROFILE_SCOPEI("CubeMapShadowMapRenderer", "EnumerateRenerables", MAKE_COLOR_R8G8B8_UNORM(50, 150, 100));

            // enumerate everything in frustum
            pRenderWorld->EnumerateRenderablesInFrustum(lightCamera.GetFrustum(), [this, &lightCamera](const RenderProxy *pRenderProxy)
            {
                // add to render queue
                pRenderProxy->QueueForRender(&lightCamera, &m_renderQueue);
            });
        }

        // no renderables?
        if (m_renderQueue.GetQueueSize() == 0)
            continue;

        // set render targets, for pipelining we do this before sorting
        pCommandList->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK));
        pCommandList->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(true, true, GPU_COMPARISON_FUNC_LESS), 0);

        // set up view-dependent constants
        pCommandList->GetConstants()->SetFromCamera(lightCamera, true);

        // sort renderables
        m_renderQueue.Sort();

        // opaque
        {
            RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_renderQueue.GetOpaqueRenderables().GetBasePointer();
            RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_renderQueue.GetOpaqueRenderables().GetBasePointer() + m_renderQueue.GetOpaqueRenderables().GetSize();
            MICROPROFILE_SCOPEI("CubeMapShadowMapRenderer", "DrawOpaqueObjects", MAKE_COLOR_R8G8B8_UNORM(150, 50, 100));

            for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
            {
                // Select appropriate shader
                // For now, only masked materials are drawn with clipping
                if (pQueueEntry->pMaterial->GetShader()->GetBlendMode() == MATERIAL_BLENDING_MODE_MASKED)
                {
                    ShaderProgram *pShaderProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(ShadowMapShader), 0, pQueueEntry->pVertexFactoryTypeInfo, pQueueEntry->VertexFactoryFlags, pQueueEntry->pMaterial->GetShader(), pQueueEntry->pMaterial->GetShaderStaticSwitchMask());
                    if (pShaderProgram != nullptr)
                    {
                        pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());
                        pQueueEntry->pMaterial->BindDeviceResources(pCommandList, pShaderProgram);
                        pQueueEntry->pRenderProxy->SetupForDraw(&lightCamera, pQueueEntry, pCommandList, pShaderProgram);
                        pQueueEntry->pRenderProxy->DrawQueueEntry(&lightCamera, pQueueEntry, pCommandList);
                    }
                }
                else
                {
                    // Otherwise, use shadowmap shader without material
                    ShaderProgram *pShaderProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(ShadowMapShader), 0, pQueueEntry->pVertexFactoryTypeInfo, pQueueEntry->VertexFactoryFlags, NULL, 0);
                    if (pShaderProgram != nullptr)
                    {
                        pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());
                        pQueueEntry->pRenderProxy->SetupForDraw(&lightCamera, pQueueEntry, pCommandList, pShaderProgram);
                        pQueueEntry->pRenderProxy->DrawQueueEntry(&lightCamera, pQueueEntry, pCommandList);
                    }
                }
            }
        }

        // translucent
        {
            RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_renderQueue.GetTranslucentRenderables().GetBasePointer();
            RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_renderQueue.GetTranslucentRenderables().GetBasePointer() + m_renderQueue.GetTranslucentRenderables().GetSize();
            MICROPROFILE_SCOPEI("CubeMapShadowMapRenderer", "DrawOpaqueObjects", MAKE_COLOR_R8G8B8_UNORM(150, 100, 50));

            for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
            {
                if (pQueueEntry->RenderPassMask & RENDER_PASS_SHADOW_MAP)
                {
                    // For now, only masked materials are drawn with clipping, so just use the regular shader here
                    ShaderProgram *pShaderProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(ShadowMapShader), 0, pQueueEntry->pVertexFactoryTypeInfo, pQueueEntry->VertexFactoryFlags, NULL, 0);
                    if (pShaderProgram != nullptr)
                    {
                        pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());
                        pQueueEntry->pRenderProxy->SetupForDraw(&lightCamera, pQueueEntry, pCommandList, pShaderProgram);
                        pQueueEntry->pRenderProxy->DrawQueueEntry(&lightCamera, pQueueEntry, pCommandList);
                    }
                }
            }
        }
    }
}
