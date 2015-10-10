#include "Renderer/PrecompiledHeader.h"
#include "Renderer/WorldRenderers/ForwardShadingWorldRenderer.h"
#include "Renderer/WorldRenderers/SSMShadowMapRenderer.h"
#include "Renderer/WorldRenderers/CSMShadowMapRenderer.h"
#include "Renderer/WorldRenderers/CubeMapShadowMapRenderer.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/ShaderProgramSelector.h"
#include "Renderer/Shaders/DepthOnlyShader.h"
#include "Renderer/Shaders/ForwardShadingShaders.h"
#include "Engine/Material.h"
#include "Engine/EngineCVars.h"
#include "Engine/ResourceManager.h"
#include "Engine/Profiling.h"
#include "MathLib/CollisionDetection.h"
Log_SetChannel(ForwardShadingWorldRenderer);

ForwardShadingWorldRenderer::ForwardShadingWorldRenderer(GPUContext *pGPUContext, const Options *pOptions)
    : CompositingWorldRenderer(pGPUContext, pOptions),
      m_pSceneColorBuffer(nullptr),
      m_pSceneColorBufferCopy(nullptr),
      m_pSceneDepthBuffer(nullptr),
      m_pSceneDepthBufferCopy(nullptr),
      m_pDirectionalShadowMapRenderer(nullptr),
      m_pPointShadowMapRenderer(nullptr),
      m_pSpotShadowMapRenderer(nullptr),
      m_lastDirectionalLightIndex(0xFFFFFFFF),
      m_lastPointLightIndex(0xFFFFFFFF),
      m_lastSpotLightIndex(0xFFFFFFFF),
      m_lastVolumetricLightIndex(0xFFFFFFFF),
      m_directionalLightShaderFlags(DirectionalLightShader::CalculateFlags(pOptions->EnableShadows, pOptions->EnableHardwareShadowFiltering, pOptions->ShadowMapFiltering, pOptions->ShowShadowMapCascades)),
      m_pointLightShaderFlags(PointLightShader::CalculateFlags(pOptions->EnableShadows, pOptions->EnableHardwareShadowFiltering)),
      m_spotLightShaderFlags(0)
{
    // render passes that we can draw
    static const uint32 availableRenderPassMask = RENDER_PASS_LIGHTMAP | RENDER_PASS_EMISSIVE |
                                                  RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING |
                                                  RENDER_PASS_OCCLUSION_CULLING_PROXY |
                                                  RENDER_PASS_TINT;

    // set up render queue
    m_renderQueue.SetAcceptingLights(true);
    m_renderQueue.SetAcceptingRenderPassMask(availableRenderPassMask);
    m_renderQueue.SetAcceptingOccluders((pOptions->EnableOcclusionCulling | pOptions->EnableOcclusionPredication) != 0);
    m_renderQueue.SetAcceptingDebugObjects(pOptions->ShowDebugInfo);
}

ForwardShadingWorldRenderer::~ForwardShadingWorldRenderer()
{
    // release postprocess buffers
    DebugAssert(m_pSceneColorBufferCopy == nullptr && m_pSceneDepthBufferCopy == nullptr);
    ReleaseIntermediateBuffer(m_pSceneDepthBuffer);
    ReleaseIntermediateBuffer(m_pSceneColorBuffer);

    // delete cached shadow maps
    for (uint32 i = 0; i < m_directionalShadowMaps.GetSize(); i++)
        m_pDirectionalShadowMapRenderer->FreeShadowMap(&m_directionalShadowMaps[i]);
    m_directionalShadowMaps.Clear();
    for (uint32 i = 0; i < m_pointShadowMaps.GetSize(); i++)
        m_pPointShadowMapRenderer->FreeShadowMap(&m_pointShadowMaps[i]);
    m_pointShadowMaps.Clear();
    for (uint32 i = 0; i < m_spotShadowMaps.GetSize(); i++)
        m_pSpotShadowMapRenderer->FreeShadowMap(&m_spotShadowMaps[i]);
    m_spotShadowMaps.Clear();

    // delete shadow map renderers
    delete m_pDirectionalShadowMapRenderer;
    delete m_pPointShadowMapRenderer;
    delete m_pSpotShadowMapRenderer;
}

bool ForwardShadingWorldRenderer::Initialize()
{
    if (!CompositingWorldRenderer::Initialize())
        return false;

    // create buffers
    m_pSceneColorBuffer = RequestIntermediateBuffer(m_options.RenderWidth, m_options.RenderHeight, SCENE_COLOR_PIXEL_FORMAT, 1);
    m_pSceneDepthBuffer = RequestIntermediateBuffer(m_options.RenderWidth, m_options.RenderHeight, SCENE_DEPTH_PIXEL_FORMAT, 1);
    if (m_pSceneColorBuffer == nullptr || m_pSceneDepthBuffer == nullptr)
        return false;

    // create shadow map renderers
    if (m_options.EnableShadows)
    {
        m_pDirectionalShadowMapRenderer = new DirectionalShadowMapRenderer(m_options.DirectionalShadowMapResolution, m_options.ShadowMapPixelFormat, m_options.ShadowMapCascadeCount, 0.95f);
        m_pPointShadowMapRenderer = new PointShadowMapRenderer(m_options.PointShadowMapResolution, m_options.ShadowMapPixelFormat);
        m_pSpotShadowMapRenderer = new SpotShadowMapRenderer(m_options.SpotShadowMapResolution, m_options.ShadowMapPixelFormat);
    }

    // all done
    return true;
}

void ForwardShadingWorldRenderer::DrawWorld(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, GPURenderTargetView *pRenderTargetView, GPUDepthStencilBufferView *pDepthStencilBufferView)
{
    // add the main camera
    //RENDER_PROFILER_ADD_CAMERA(pRenderProfiler, &pViewParameters->ViewCamera, "World View Camera");

    // culling
    FillRenderQueue(&pViewParameters->ViewCamera, pRenderWorld);

    // draw shadow maps
    DrawShadowMaps(pRenderWorld, pViewParameters);

//     // handle override cameras
//     if (pRenderProfiler != nullptr && pRenderProfiler->HasCameraOverride())
//     {
//         // clone view parameters
//         ViewParameters *viewParametersCopy = (ViewParameters *)alloca(sizeof(ViewParameters));
//         Y_memcpy(viewParametersCopy, pViewParameters, sizeof(ViewParameters));
//         viewParametersCopy->ViewCamera = *pRenderProfiler->GetCameraOverrideCamera();
//         pViewParameters = viewParametersCopy;
// 
//         // reissue renderable query with new camera
//         FillRenderQueue(&viewParametersCopy->ViewCamera, pRenderWorld);
//     }

    // clear render targets
    {
        MICROPROFILE_SCOPEI("ForwardShadingWorldRenderer", "Prepare", MAKE_COLOR_R8G8B8_UNORM(127, 72, 98));

        // need a seperate viewport
        RENDERER_VIEWPORT bufferViewport(0, 0, m_options.RenderWidth, m_options.RenderHeight, 0.0f, 1.0f);
        m_pGPUContext->SetRenderTargets(1, &m_pSceneColorBuffer->pRTV, m_pSceneDepthBuffer->pDSV);
        m_pGPUContext->SetViewport(&bufferViewport);
        m_pGPUContext->ClearTargets(true, true, true, pViewParameters->FogColor);
    
        // set up view-dependent constants
        GPUContextConstants *pConstants = m_pGPUContext->GetConstants();
        pConstants->SetFromCamera(pViewParameters->ViewCamera, false);
        pConstants->SetWorldTime(pViewParameters->WorldTime, false);
        pConstants->CommitChanges();
    }

    // pull occlusion results back from gpu
    if (m_options.EnableOcclusionCulling)
        CollectOcclusionCullingResults();
    else if (m_options.EnableOcclusionPredication)
        BindOcclusionQueriesToQueueEntries();

    // depth prepass
    if (m_options.EnableDepthPrepass)
        DrawDepthPrepass(pViewParameters);

    // opaque objects
    DrawOpaqueObjects(pViewParameters);

    // if occlusion culling is enabled, draw proxies
    if ((m_options.EnableOcclusionCulling | m_options.EnableOcclusionPredication) != 0)
        DrawOcclusionCullingProxies(m_pGPUContext, &pViewParameters->ViewCamera);

    // transparent objects
    DrawTranslucentObjects(pViewParameters);

    // post process objects
    DrawPostProcessObjects(pViewParameters);

    // tonemap to present buffer
    ApplyFinalCompositePostProcess(pViewParameters, m_pSceneColorBuffer->pTexture, pRenderTargetView);

    // debug info - draw to backbuffer
    if (m_pGUIContext != nullptr)
    {
        if (m_options.ShowDebugInfo)
            DrawDebugInfo(&pViewParameters->ViewCamera);

        if (m_options.ShowIntermediateBuffers)
            DrawIntermediateBuffers();
    }

    // frame complete
    m_pGPUContext->ClearState(true, true, true, true);
    OnFrameComplete();
}

void ForwardShadingWorldRenderer::OnFrameComplete()
{
    CompositingWorldRenderer::OnFrameComplete();

    // release all shadow maps
    for (uint32 i = 0; i < m_directionalShadowMaps.GetSize(); i++)
        m_directionalShadowMaps[i].IsActive = false;
    for (uint32 i = 0; i < m_pointShadowMaps.GetSize(); i++)
        m_pointShadowMaps[i].IsActive = false;
    for (uint32 i = 0; i < m_spotShadowMaps.GetSize(); i++)
        m_spotShadowMaps[i].IsActive = false;

    // clear last indices
    m_lastDirectionalLightIndex = 0xFFFFFFFF;
    m_lastPointLightIndex = 0xFFFFFFFF;
    m_lastSpotLightIndex = 0xFFFFFFFF;
    m_lastVolumetricLightIndex = 0xFFFFFFFF;
}

void ForwardShadingWorldRenderer::DrawShadowMaps(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("ForwardShadingWorldRenderer", "DrawShadowMaps", MAKE_COLOR_R8G8B8_UNORM(127, 98, 42));

    // directional lights
    {
        RenderQueue::DirectionalLightArray &directionalLights = m_renderQueue.GetDirectionalLightArray();
        for (uint32 i = 0; i < directionalLights.GetSize(); i++)
        {
            RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight = &directionalLights[i];
            if (pLight->ShadowFlags & LIGHT_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS)
                DrawDirectionalShadowMap(pRenderWorld, pViewParameters, pLight);
        }
    }

    // point lights
    {
        RenderQueue::PointLightArray &pointLights = m_renderQueue.GetPointLightArray();
        for (uint32 i = 0; i < pointLights.GetSize(); i++)
        {
            RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight = &pointLights[i];
            if (pLight->ShadowFlags & LIGHT_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS)
                DrawPointShadowMap(pRenderWorld, pViewParameters, pLight);
        }
    }

    // clear shaders/targets since the targets will be bound to inputs
    m_pGPUContext->ClearState(true, false, false, true);
}

bool ForwardShadingWorldRenderer::DrawDirectionalShadowMap(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight)
{
    if (m_pDirectionalShadowMapRenderer == nullptr)
        return false;

    // find a free directional shadow map
    uint32 shadowMapIndex = 0;
    for (; shadowMapIndex < m_directionalShadowMaps.GetSize(); shadowMapIndex++)
    {
        if (!m_directionalShadowMaps[shadowMapIndex].IsActive)
            break;
    }

    // none found?
    if (shadowMapIndex == m_directionalShadowMaps.GetSize())
    {
        // allocate it
        DirectionalShadowMapRenderer::ShadowMapData shadowMapData;
        if (!m_pDirectionalShadowMapRenderer->AllocateShadowMap(&shadowMapData))
            return false;

        m_directionalShadowMaps.Add(shadowMapData);
    }

    // get shadow map data pointer
    DirectionalShadowMapRenderer::ShadowMapData *pShadowMapData = &m_directionalShadowMaps[shadowMapIndex];
    pShadowMapData->IsActive = true;

    // bind the shadow map to the light
    pLight->ShadowMapIndex = shadowMapIndex;

    // invoke draw
    m_pDirectionalShadowMapRenderer->DrawShadowMap(m_pGPUContext, pShadowMapData, &pViewParameters->ViewCamera, pViewParameters->MaximumShadowViewDistance, pRenderWorld, pLight);

    // ok
    return true;
}

bool ForwardShadingWorldRenderer::DrawPointShadowMap(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight)
{
    if (m_pPointShadowMapRenderer == nullptr)
        return false;

    // find a free directional shadow map
    uint32 shadowMapIndex = 0;
    for (; shadowMapIndex < m_pointShadowMaps.GetSize(); shadowMapIndex++)
    {
        if (!m_pointShadowMaps[shadowMapIndex].IsActive)
            break;
    }

    // none found?
    if (shadowMapIndex == m_pointShadowMaps.GetSize())
    {
        // allocate it
        PointShadowMapRenderer::ShadowMapData shadowMapData;
        if (!m_pPointShadowMapRenderer->AllocateShadowMap(&shadowMapData))
            return false;

        m_pointShadowMaps.Add(shadowMapData);
    }

    // get shadow map data pointer
    PointShadowMapRenderer::ShadowMapData *pShadowMapData = &m_pointShadowMaps[shadowMapIndex];
    pShadowMapData->IsActive = true;

    // bind the shadow map to the light
    pLight->ShadowMapIndex = shadowMapIndex;

    // invoke draw
    m_pPointShadowMapRenderer->DrawShadowMap(m_pGPUContext, pShadowMapData, &pViewParameters->ViewCamera, pViewParameters->MaximumShadowViewDistance, pRenderWorld, pLight);

    // ok
    return true;
}

bool ForwardShadingWorldRenderer::DrawSpotShadowMap(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, RENDER_QUEUE_SPOT_LIGHT_ENTRY *pLight)
{
    return false;
}

void ForwardShadingWorldRenderer::SetCommonShaderProgramParameters(const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, ShaderProgram *pShaderProgram)
{
    pQueueEntry->pMaterial->BindDeviceResources(m_pGPUContext, pShaderProgram);
    pQueueEntry->pRenderProxy->SetupForDraw(&pViewParameters->ViewCamera, pQueueEntry, m_pGPUContext, pShaderProgram);

    // post process material?
    const MaterialShader *pMaterialShader = pQueueEntry->pMaterial->GetShader();
    if (pMaterialShader->GetRenderMode() == MATERIAL_RENDER_MODE_POST_PROCESS)
    {
        DebugAssert(m_pSceneColorBufferCopy != nullptr && m_pSceneDepthBufferCopy != nullptr);

        // set post process textures
        const uint32 BASE_INDEX = pMaterialShader->GetTextureParameterCount();
        pShaderProgram->SetMaterialParameterTexture(m_pGPUContext, BASE_INDEX + 0, m_pSceneColorBufferCopy->pTexture, nullptr);
        pShaderProgram->SetMaterialParameterTexture(m_pGPUContext, BASE_INDEX + 1, m_pSceneDepthBufferCopy->pTexture, nullptr);
    }
}

uint32 ForwardShadingWorldRenderer::DrawBasePassForObject(const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, bool depthWrites, GPU_COMPARISON_FUNC depthFunc)
{
    const MaterialShader *pMaterialShader = pQueueEntry->pMaterial->GetShader();
    uint32 renderPassMask = pQueueEntry->RenderPassMask;

    // base pass flags
    uint32 basePassFlags = 0;

    // emissive
    if (renderPassMask & RENDER_PASS_EMISSIVE)
        basePassFlags |= BasePassShader::WITH_EMISSIVE;

    // lightmap
    else if (renderPassMask & RENDER_PASS_LIGHTMAP)
        basePassFlags |= BasePassShader::WITH_LIGHTMAP;

    // nothing for base pass
    else
        return 0;

    // should have flags
    DebugAssert(basePassFlags != 0);

    // draw base pass
    ShaderProgram *pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(BasePassShader), basePassFlags, pQueueEntry);
    if (pShaderProgram != nullptr)
    {
        // set states
        m_pGPUContext->SetRasterizerState(pMaterialShader->SelectRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false));
        m_pGPUContext->SetDepthStencilState(pMaterialShader->SelectDepthStencilState(true, depthWrites, depthFunc), 0);

        // bind shader
        m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());
        SetBlendingModeForMaterial(m_pGPUContext, pQueueEntry);
        SetCommonShaderProgramParameters(pViewParameters, pQueueEntry, pShaderProgram);

        // draw away
        pQueueEntry->pRenderProxy->DrawQueueEntry(&pViewParameters->ViewCamera, pQueueEntry, m_pGPUContext);
    }

    // done
    return 1;
}

void ForwardShadingWorldRenderer::DrawEmptyPassForObject(const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry)
{
    const MaterialShader *pMaterialShader = pQueueEntry->pMaterial->GetShader();

    // get shader
    ShaderProgram *pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(BasePassShader), 0, pQueueEntry);
    if (pShaderProgram != nullptr)
    {
        // set initial states
        m_pGPUContext->SetRasterizerState(pMaterialShader->SelectRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false));
        m_pGPUContext->SetDepthStencilState(pMaterialShader->SelectDepthStencilState(true, true, GPU_COMPARISON_FUNC_LESS), 0);

        // bind shader
        m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());
        SetBlendingModeForMaterial(m_pGPUContext, pQueueEntry);
        SetCommonShaderProgramParameters(pViewParameters, pQueueEntry, pShaderProgram);

        // draw it
        pQueueEntry->pRenderProxy->DrawQueueEntry(&pViewParameters->ViewCamera, pQueueEntry, m_pGPUContext);
    }
}

uint32 ForwardShadingWorldRenderer::DrawForwardLightPassesForObject(const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, bool useAdditiveBlending, bool depthWrites, GPU_COMPARISON_FUNC depthFunc)
{
    const MaterialShader *pMaterialShader = pQueueEntry->pMaterial->GetShader();
    const bool staticLightMask = ((pQueueEntry->RenderPassMask & RENDER_PASS_STATIC_LIGHTING) != 0);
    const uint32 renderPassMask = pQueueEntry->RenderPassMask;
    ShaderProgram *pShaderProgram;
    uint32 drawnLights = 0;

    // draw lights macro
#define DRAW_LIGHT() MULTI_STATEMENT_MACRO_BEGIN \
        if (drawnLights == 0) \
        { \
            m_pGPUContext->SetRasterizerState(pMaterialShader->SelectRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false)); \
            m_pGPUContext->SetDepthStencilState(pMaterialShader->SelectDepthStencilState(true, depthWrites, depthFunc), 0); \
            if (useAdditiveBlending) \
                SetAdditiveBlendingModeForMaterial(m_pGPUContext, pQueueEntry); \
            else \
                SetBlendingModeForMaterial(m_pGPUContext, pQueueEntry); \
        } \
        else if (drawnLights == 1) \
        { \
            if (!useAdditiveBlending) \
                SetAdditiveBlendingModeForMaterial(m_pGPUContext, pQueueEntry); \
            if (depthWrites) \
                m_pGPUContext->SetDepthStencilState(pMaterialShader->SelectDepthStencilState(true, false, GPU_COMPARISON_FUNC_EQUAL), 0); \
        } \
        SetCommonShaderProgramParameters(pViewParameters, pQueueEntry, pShaderProgram); \
        pQueueEntry->pRenderProxy->DrawQueueEntry(&pViewParameters->ViewCamera, pQueueEntry, m_pGPUContext); \
        drawnLights++; \
        MULTI_STATEMENT_MACRO_END

    // draw directional lights
    RenderQueue::DirectionalLightArray &directionalLights = m_renderQueue.GetDirectionalLightArray();
    for (uint32 lightIndex = 0; lightIndex < directionalLights.GetSize(); lightIndex++)
    {
        const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight = &directionalLights[lightIndex];
        const bool usingShadowMap = (pLight->ShadowMapIndex >= 0 && (renderPassMask & RENDER_PASS_SHADOWED_LIGHTING));

        // mask static lights on static objects away
        if ((pLight->Static & staticLightMask) != pLight->Static)
            continue;

        // get shader flags
        uint32 directionalLightShaderFlags = m_directionalLightShaderFlags;
        if (!usingShadowMap)
            directionalLightShaderFlags &= ~DirectionalLightShader::SHADOW_BITS_MASK;

        // get shader
        if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(DirectionalLightShader), directionalLightShaderFlags, pQueueEntry)) == nullptr)
            continue;

        // bind shader and set parameters
        m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());

        // using shadow map? note: we need the shadow map data to set the constant buffer up, even if this object does not use it
        const DirectionalShadowMapRenderer::ShadowMapData *pShadowMapData = (pLight->ShadowMapIndex >= 0) ? &m_directionalShadowMaps[pLight->ShadowMapIndex] : nullptr;

        // set constant buffer parameters
        if (m_lastDirectionalLightIndex != lightIndex)
        {
            DirectionalLightShader::SetLightParameters(m_pGPUContext, pLight, pShadowMapData);
            m_lastDirectionalLightIndex = lightIndex;
        }

        // and the shadow map textures
        if (pShadowMapData)
            DirectionalLightShader::SetProgramParameters(m_pGPUContext, pShaderProgram, pLight, pShadowMapData);

        // draw light
        DRAW_LIGHT();
    }

    // draw point lights
    RenderQueue::PointLightArray &pointLights = m_renderQueue.GetPointLightArray();
    const RENDER_QUEUE_POINT_LIGHT_ENTRY *queuedLights[PointLightListShader::MAX_LIGHTS];
    uint32 queuedLightCount = 0;
    for (uint32 lightIndex = 0; lightIndex < pointLights.GetSize(); lightIndex++)
    {
        const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight = &pointLights[lightIndex];
        const bool usingShadowMap = (pLight->ShadowMapIndex >= 0 && (renderPassMask & RENDER_PASS_SHADOWED_LIGHTING));

        // mask static lights on static objects away
        if ((pLight->Static & staticLightMask) != pLight->Static)
            continue;

        // test if it intersects the renderable
        if (!CollisionDetection::AABoxIntersectsSphere(pQueueEntry->BoundingBox.GetMinBounds(), pQueueEntry->BoundingBox.GetMaxBounds(),
            pLight->Position, pLight->Range))
        {
            continue;
        }

        // we only draw shadowed lights immediately, otherwise queue
        if (!usingShadowMap)
        {
            if (queuedLightCount == PointLightListShader::MAX_LIGHTS)
            {
                if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(PointLightListShader), 0, pQueueEntry)) != nullptr)
                {
                    // bind shader
                    m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());

                    // initialize light pointers
                    for (uint32 queuedLightIndex = 0; queuedLightIndex < queuedLightCount; queuedLightIndex++)
                        PointLightListShader::SetLightParameters(m_pGPUContext, queuedLightIndex, queuedLights[queuedLightIndex]);

                    // commit parameters, and draw
                    PointLightListShader::SetActiveLightCount(m_pGPUContext, queuedLightCount);
                    PointLightListShader::CommitParameters(m_pGPUContext);
                    DRAW_LIGHT();
                }

                queuedLightCount = 0;
            }

            // add to queued light list
            queuedLights[queuedLightCount++] = pLight;
            continue;
        }

        // get shader flags
        uint32 pointLightShaderFlags = m_pointLightShaderFlags;
        if (!usingShadowMap)
            pointLightShaderFlags &= ~PointLightShader::SHADOW_BITS_MASK;

        // get shader
        if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(PointLightShader), pointLightShaderFlags, pQueueEntry)) == nullptr)
            continue;

        // bind shader
        m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());

        // using shadow map? note: we need the shadow map data to set the constant buffer up, even if this object does not use it
        const PointShadowMapRenderer::ShadowMapData *pShadowMapData = (pLight->ShadowMapIndex >= 0) ? &m_pointShadowMaps[pLight->ShadowMapIndex] : nullptr;

        // set constant buffer parameters
        if (m_lastPointLightIndex != lightIndex)
        {
            PointLightShader::SetLightParameters(m_pGPUContext, pLight, pShadowMapData);
            m_lastPointLightIndex = lightIndex;
        }
        
        // set program parameters
        PointLightShader::SetProgramParameters(m_pGPUContext, pShaderProgram, pLight, pShadowMapData);

        // draw light
        DRAW_LIGHT();
    }

    // if there's only one remaining light, we can use the fast shader, otherwise use the multi-light shader
    if (queuedLightCount == 1)
    {
        if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(PointLightShader), 0, pQueueEntry)) != nullptr)
        {
            m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());
            PointLightShader::SetLightParameters(m_pGPUContext, queuedLights[0], nullptr);
            DRAW_LIGHT();
        }
    }
    else if (queuedLightCount > 0)
    {
        if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(PointLightListShader), 0, pQueueEntry)) != nullptr)
        {
            // bind shader
            m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());

            // initialize light pointers
            for (uint32 queuedLightIndex = 0; queuedLightIndex < queuedLightCount; queuedLightIndex++)
                PointLightListShader::SetLightParameters(m_pGPUContext, queuedLightIndex, queuedLights[queuedLightIndex]);

            // commit parameters, and draw
            PointLightListShader::SetActiveLightCount(m_pGPUContext, queuedLightCount);
            PointLightListShader::CommitParameters(m_pGPUContext);
            DRAW_LIGHT();
        }
    }

    // draw volumetric lights
    RenderQueue::VolumetricLightArray &volumetricLights = m_renderQueue.GetVolumetricLightArray();
    for (uint32 lightIndex = 0; lightIndex < volumetricLights.GetSize(); lightIndex++)
    {
        const RENDER_QUEUE_VOLUMETRIC_LIGHT_ENTRY *pLight = &volumetricLights[lightIndex];

        // test with bounding box, meh.
        if (!CollisionDetection::AABoxIntersectsSphere(pQueueEntry->BoundingBox.GetMinBounds(), pQueueEntry->BoundingBox.GetMaxBounds(),
            pLight->Position, pLight->Range))
        {
            continue;
        }

        // get shader flags
        uint32 volumetricShaderFlags = VolumetricLightShader::GetTypeFlagsForPrimitive(pLight->Primitive);

        // find shader
        if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(VolumetricLightShader), volumetricShaderFlags, pQueueEntry)) == NULL)
            continue;

        // bind shader
        m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());

        // and parameters
        if (m_lastVolumetricLightIndex != lightIndex)
        {
            VolumetricLightShader::SetLightParameters(m_pGPUContext, pLight);
            m_lastVolumetricLightIndex = lightIndex;
        }

        // set program parameters
        VolumetricLightShader::SetProgramParameters(m_pGPUContext, pShaderProgram, pLight);

        // draw light
        DRAW_LIGHT();
    }

#undef DRAW_LIGHT
    
    return drawnLights;
}

void ForwardShadingWorldRenderer::DrawDepthPrepass(const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("ForwardShadingWorldRenderer", "DrawDepthPrepass", MAKE_COLOR_R8G8B8_UNORM(255, 100, 255));

    ShaderProgramSelector shaderSelector(m_globalShaderFlags);
    shaderSelector.SetBaseShader(OBJECT_TYPEINFO(DepthOnlyShader), 0);

    // Everything here uses the normal rasterizer state, so set that up here.
    m_pGPUContext->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateNoColorWrites());

    // Iterate over renderables
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_renderQueue.GetOpaqueRenderables().GetBasePointer();
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_renderQueue.GetOpaqueRenderables().GetBasePointer() + m_renderQueue.GetOpaqueRenderables().GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        if (pQueueEntry->RenderPassMask == 0)
            continue;

        // get material
        const MaterialShader *pMaterialShader = pQueueEntry->pMaterial->GetShader();

        // ignore materials that don't write depth values for the prepass
        if (!pMaterialShader->GetDepthWrites())
            continue;

        // set states
        m_pGPUContext->SetRasterizerState(pMaterialShader->SelectRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false));
        m_pGPUContext->SetDepthStencilState(pMaterialShader->SelectDepthStencilState(true, true, GPU_COMPARISON_FUNC_LESS), 0);

        // For now, only masked materials are drawn with clipping
        shaderSelector.SetVertexFactory(pQueueEntry->pVertexFactoryTypeInfo, pQueueEntry->VertexFactoryFlags);
        shaderSelector.SetMaterial((pQueueEntry->pMaterial->GetShader()->GetBlendMode() == MATERIAL_BLENDING_MODE_MASKED) ? pQueueEntry->pMaterial : nullptr);

        // only continue with shader
        ShaderProgram *pShaderProgram = shaderSelector.MakeActive(m_pGPUContext);
        if (pShaderProgram != nullptr)
        {
            m_pGPUContext->SetPredication(pQueueEntry->pPredicate);
            pQueueEntry->pRenderProxy->SetupForDraw(&pViewParameters->ViewCamera, pQueueEntry, m_pGPUContext, pShaderProgram);
            pQueueEntry->pRenderProxy->DrawQueueEntry(&pViewParameters->ViewCamera, pQueueEntry, m_pGPUContext);
        }
    }

    // clear predication
    m_pGPUContext->SetPredication(nullptr);
}

void ForwardShadingWorldRenderer::DrawOpaqueObjects(const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("ForwardShadingWorldRenderer", "DrawOpaqueObjects", MAKE_COLOR_R8G8B8_UNORM(90, 127, 42));

    // Iterate over renderables.
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_renderQueue.GetOpaqueRenderables().GetBasePointer();
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_renderQueue.GetOpaqueRenderables().GetBasePointer() + m_renderQueue.GetOpaqueRenderables().GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        if (pQueueEntry->RenderPassMask == 0)
            continue;

        // determine whether to write depth values
        bool writeDepthValues = !(CVars::r_depth_prepass.GetBool() && pQueueEntry->pMaterial->GetShader()->GetDepthWrites());

        // draw base pass
        uint32 drawCount = DrawBasePassForObject(pViewParameters, pQueueEntry, writeDepthValues, GPU_COMPARISON_FUNC_LESS);

        // draw lighting passes
        if (pQueueEntry->RenderPassMask & (RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING))
            drawCount += DrawForwardLightPassesForObject(pViewParameters, pQueueEntry, (drawCount > 0), (drawCount > 0) ? false : writeDepthValues, (drawCount > 0) ? GPU_COMPARISON_FUNC_EQUAL : GPU_COMPARISON_FUNC_LESS);

        // if nothing has been drawn for this object [and no depth prepass], draw a blank pass (for depth)
        if (drawCount == 0 && !CVars::r_depth_prepass.GetBool())
            DrawEmptyPassForObject(pViewParameters, pQueueEntry);
    }

    // wireframe overlay
    if (m_options.ShowWireframeOverlay)
        DrawWireframeOverlay(m_pGPUContext, &pViewParameters->ViewCamera, &m_renderQueue.GetOpaqueRenderables());
}

void ForwardShadingWorldRenderer::DrawTranslucentObjects(const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("ForwardShadingWorldRenderer", "DrawTranslucentObjects", MAKE_COLOR_R8G8B8_UNORM(20, 50, 42));

    // Iterate over renderables.
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_renderQueue.GetTranslucentRenderables().GetBasePointer();
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_renderQueue.GetTranslucentRenderables().GetBasePointer() + m_renderQueue.GetTranslucentRenderables().GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        uint32 renderPassMask = pQueueEntry->RenderPassMask;
        uint32 drawCount = 0;

        // skip anything without any passes
        if (renderPassMask == 0)
            continue;

        // draw base pass?
        if (renderPassMask & (RENDER_PASS_EMISSIVE | RENDER_PASS_LIGHTMAP | RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING))
            drawCount = DrawBasePassForObject(pViewParameters, pQueueEntry, false, GPU_COMPARISON_FUNC_LESS_EQUAL);

        // draw light passes?
        if (renderPassMask & (RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING))
            drawCount += DrawForwardLightPassesForObject(pViewParameters, pQueueEntry, (drawCount != 0), false, GPU_COMPARISON_FUNC_LESS_EQUAL);

        // draw blank pass if there is no draw calls for this object
        if (drawCount == 0)
            DrawEmptyPassForObject(pViewParameters, pQueueEntry);
    }

    // wireframe overlay
    if (m_options.ShowWireframeOverlay)
        DrawWireframeOverlay(m_pGPUContext, &pViewParameters->ViewCamera, &m_renderQueue.GetTranslucentRenderables());
}

void ForwardShadingWorldRenderer::DrawPostProcessObjects(const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("ForwardShadingWorldRenderer", "DrawPostProcessObjects", MAKE_COLOR_R8G8B8_UNORM(50, 20, 42));

    // if we don't have any objects, exit early
    if (m_renderQueue.GetPostProcessRenderables().GetSize() == 0)
        return;

    // acquire copy of scene colour + depth
    m_pSceneColorBufferCopy = RequestIntermediateBufferMatching(m_pSceneColorBuffer);
    m_pSceneDepthBufferCopy = RequestIntermediateBufferMatching(m_pSceneDepthBuffer);
    m_pGPUContext->CopyTexture(m_pSceneColorBuffer->pTexture, m_pSceneColorBufferCopy->pTexture);
    m_pGPUContext->CopyTexture(m_pSceneDepthBuffer->pTexture, m_pSceneDepthBufferCopy->pTexture);

    // common rasterizer state
    m_pGPUContext->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState());

    // draw the objects
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_renderQueue.GetPostProcessRenderables().GetBasePointer();
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_renderQueue.GetPostProcessRenderables().GetBasePointer() + m_renderQueue.GetPostProcessRenderables().GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        const MaterialShader *pMaterialShader = pQueueEntry->pMaterial->GetShader();
        MATERIAL_BLENDING_MODE blendingMode = pMaterialShader->GetBlendMode();
        uint32 renderPassMask = pQueueEntry->RenderPassMask;
        uint32 drawCount = 0;

        // skip anything without any passes
        if (renderPassMask == 0)
            continue;

        // draw appropriately
        switch (blendingMode)
        {
        case MATERIAL_BLENDING_MODE_STRAIGHT:
        case MATERIAL_BLENDING_MODE_PREMULTIPLIED:
            {
                // draw base pass?
                if (renderPassMask & (RENDER_PASS_EMISSIVE | RENDER_PASS_LIGHTMAP | RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING))
                    drawCount = DrawBasePassForObject(pViewParameters, pQueueEntry, false, GPU_COMPARISON_FUNC_LESS_EQUAL);

                // draw light passes?
                if (renderPassMask & (RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING))
                    drawCount += DrawForwardLightPassesForObject(pViewParameters, pQueueEntry, (drawCount != 0), false, GPU_COMPARISON_FUNC_LESS_EQUAL);

                // draw blank pass if there is no draw calls for this object
                if (drawCount == 0)
                    DrawEmptyPassForObject(pViewParameters, pQueueEntry);
            }
            break;

        default:
            {
                // draw base pass
                drawCount = DrawBasePassForObject(pViewParameters, pQueueEntry, false, GPU_COMPARISON_FUNC_LESS_EQUAL);

                // draw lighting passes
                if (pQueueEntry->RenderPassMask & (RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING))
                    drawCount += DrawForwardLightPassesForObject(pViewParameters, pQueueEntry, (drawCount > 0), false, GPU_COMPARISON_FUNC_LESS_EQUAL);

                // if nothing has been drawn for this object [and no depth prepass], draw a blank pass (for depth)
                if (drawCount == 0)
                    DrawEmptyPassForObject(pViewParameters, pQueueEntry);
            }
            break;
        }
    }

    // release intermediate buffers
    ReleaseIntermediateBuffer(m_pSceneDepthBufferCopy);
    ReleaseIntermediateBuffer(m_pSceneColorBufferCopy);
    m_pSceneDepthBufferCopy = nullptr;
    m_pSceneColorBufferCopy = nullptr;

    // wireframe overlay
    if (m_options.ShowWireframeOverlay)
        DrawWireframeOverlay(m_pGPUContext, &pViewParameters->ViewCamera, &m_renderQueue.GetPostProcessRenderables());
}

