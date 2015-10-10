#include "Renderer/PrecompiledHeader.h"
#include "Renderer/WorldRenderers/DeferredShadingWorldRenderer.h"
#include "Renderer/WorldRenderers/SSMShadowMapRenderer.h"
#include "Renderer/WorldRenderers/CSMShadowMapRenderer.h"
#include "Renderer/WorldRenderers/CubeMapShadowMapRenderer.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/ShaderProgramSelector.h"
#include "Renderer/Shaders/DepthOnlyShader.h"
#include "Renderer/Shaders/ForwardShadingShaders.h"
#include "Renderer/Shaders/DeferredShadingShaders.h"
#include "Renderer/Shaders/SSAOShader.h"
#include "Engine/Material.h"
#include "Engine/EngineCVars.h"
#include "Engine/Texture.h"
#include "Engine/ResourceManager.h"
#include "Engine/Profiling.h"
#include "MathLib/CollisionDetection.h"
#include "Core/MeshUtilties.h"
Log_SetChannel(DeferredShadingWorldRenderer);

DeferredShadingWorldRenderer::DeferredShadingWorldRenderer(GPUContext *pGPUContext, const Options *pOptions)
    : CompositingWorldRenderer(pGPUContext, pOptions),
      m_pDirectionalShadowMapRenderer(nullptr),
      m_pPointShadowMapRenderer(nullptr),
      m_pSpotShadowMapRenderer(nullptr),
      m_lastDirectionalLightIndex(0xFFFFFFFF),
      m_lastPointLightIndex(0xFFFFFFFF),
      m_lastSpotLightIndex(0xFFFFFFFF),
      m_lastVolumetricLightIndex(0xFFFFFFFF),
      m_directionalLightShaderFlags(DirectionalLightShader::CalculateFlags(pOptions->EnableShadows, pOptions->EnableHardwareShadowFiltering, pOptions->ShadowMapFiltering, pOptions->ShowShadowMapCascades)),
      m_pointLightShaderFlags(PointLightShader::CalculateFlags(pOptions->EnableShadows, pOptions->EnableHardwareShadowFiltering)),
      m_spotLightShaderFlags(0),
      m_pSceneColorBuffer(nullptr),
      m_pSceneColorBufferCopy(nullptr),
      m_pSceneDepthBuffer(nullptr),
      m_pSceneDepthBufferCopy(nullptr),
      m_pGBuffer0(nullptr),
      m_pGBuffer1(nullptr),
      m_pGBuffer2(nullptr),
      m_pPointLightVolumeVertexBuffer(nullptr),
      m_pSpotLightVolumeVertexBuffer(nullptr),
      m_pointLightVolumeVertexCount(0),
      m_pSSAOProgram(nullptr)
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

DeferredShadingWorldRenderer::~DeferredShadingWorldRenderer()
{
    // release intermediate buffers
    ReleaseIntermediateBuffer(m_pSceneColorBuffer);
    ReleaseIntermediateBuffer(m_pSceneDepthBuffer);
    ReleaseIntermediateBuffer(m_pGBuffer0);
    ReleaseIntermediateBuffer(m_pGBuffer1);
    ReleaseIntermediateBuffer(m_pGBuffer2);

    // delete cached shadow maps
    for (uint32 i = 0; i < m_directionalShadowMaps.GetSize(); i++)
        m_directionalShadowMaps[i].pShadowMapTexture->Release();
    m_directionalShadowMaps.Clear();
    for (uint32 i = 0; i < m_pointShadowMaps.GetSize(); i++)
        m_pointShadowMaps[i].pShadowMapTexture->Release();
    m_pointShadowMaps.Clear();
    for (uint32 i = 0; i < m_spotShadowMaps.GetSize(); i++)
        m_spotShadowMaps[i].pShadowMapTexture->Release();
    m_spotShadowMaps.Clear();

    // delete shadow map renderers
    delete m_pDirectionalShadowMapRenderer;
    delete m_pPointShadowMapRenderer;
    delete m_pSpotShadowMapRenderer;

    // delete light volume vertex buffers
    SAFE_RELEASE(m_pSpotLightVolumeVertexBuffer);
    SAFE_RELEASE(m_pPointLightVolumeVertexBuffer);
}

bool DeferredShadingWorldRenderer::Initialize()
{
    if (!CompositingWorldRenderer::Initialize())
        return false;

    // find programs
    if ((m_pSSAOProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(SSAOShader), 0, g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributes(), g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributeCount(), nullptr, 0)) == nullptr ||
        (m_pSSAOApplyProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(SSAOApplyShader), 0, g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributes(), g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributeCount(), nullptr, 0)) == nullptr)
    {
        return false;
    }

    // allocate intermediate buffers
    if ((m_pSceneColorBuffer = RequestIntermediateBuffer(m_options.RenderWidth, m_options.RenderHeight, LIGHTBUFFER_FORMAT, 1)) == nullptr ||
        (m_pSceneDepthBuffer = RequestIntermediateBuffer(m_options.RenderWidth, m_options.RenderHeight, DEPTHBUFFER_FORMAT, 1)) == nullptr ||
        (m_pGBuffer0 = RequestIntermediateBuffer(m_options.RenderWidth, m_options.RenderHeight, GBUFFER0_FORMAT, 1)) == nullptr ||
        (m_pGBuffer1 = RequestIntermediateBuffer(m_options.RenderWidth, m_options.RenderHeight, GBUFFER1_FORMAT, 1)) == nullptr ||
        (m_pGBuffer2 = RequestIntermediateBuffer(m_options.RenderWidth, m_options.RenderHeight, GBUFFER2_FORMAT, 1)) == nullptr)
    {
        return false;
    }
    
#ifdef Y_BUILD_CONFIG_DEBUG
    m_pSceneColorBuffer->pTexture->SetDebugName("Deferred Light Buffer");
    m_pSceneDepthBuffer->pTexture->SetDebugName("Deferred Depth Buffer");
    m_pGBuffer0->pTexture->SetDebugName("Deferred GBuffer 0");
    m_pGBuffer1->pTexture->SetDebugName("Deferred GBuffer 1");
    m_pGBuffer2->pTexture->SetDebugName("Deferred GBuffer 2");
#endif
    
    // point light volume vertex buffer
    {
        float3 *pVertices;
        uint32 nVertices;
        MeshUtilites::CreateSphere(&pVertices, &nVertices);
        DebugAssert(nVertices > 0);

        // can be passed straight through to the gpu, ehh on float3 alignment
        GPU_BUFFER_DESC vertexBufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER, nVertices * sizeof(float3));
        if ((m_pPointLightVolumeVertexBuffer = g_pRenderer->CreateBuffer(&vertexBufferDesc, pVertices)) == nullptr)
        {
            delete[] pVertices;
            return false;
        }

        // set vertex count
        m_pointLightVolumeVertexCount = nVertices;
        delete[] pVertices;
    }

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

void DeferredShadingWorldRenderer::DrawWorld(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, GPURenderTargetView *pRenderTargetView, GPUDepthStencilBufferView *pDepthStencilBufferView)
{
    MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "DrawWorld", MAKE_COLOR_R8G8B8_UNORM(255, 100, 100));

    // add the main camera
    //RENDER_PROFILER_ADD_CAMERA(pRenderProfiler, &pViewParameters->ViewCamera, "World View Camera");

    // culling
    FillRenderQueue(&pViewParameters->ViewCamera, pRenderWorld);
    
    // draw shadow maps
    DrawShadowMaps(pRenderWorld, pViewParameters);

    // pull occlusion results back from gpu
    if (m_options.EnableOcclusionCulling)
        CollectOcclusionCullingResults();
    else if (m_options.EnableOcclusionPredication)
        BindOcclusionQueriesToQueueEntries();

    // draw main passes
    QueuePrimaryRenderPass([this, pRenderWorld, pViewParameters](GPUCommandList *pCommandList)
    {
        MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "MainRenderPass", MAKE_COLOR_R8G8B8_UNORM(100, 150, 75));

        // clear render targets
        {
            // need a seperate viewport, skip color clear it'll be done when gbuffers are combined
            RENDERER_VIEWPORT bufferViewport(0, 0, m_options.RenderWidth, m_options.RenderHeight, 0.0f, 1.0f);
            pCommandList->SetRenderTargets(0, nullptr, m_pSceneDepthBuffer->pDSV);
            pCommandList->SetViewport(&bufferViewport);
            pCommandList->ClearTargets(false, true, true);

            // set up view-dependent constants
            GPUContextConstants *pConstants = pCommandList->GetConstants();
            pConstants->SetFromCamera(pViewParameters->ViewCamera, false);
            pConstants->SetWorldTime(pViewParameters->WorldTime, false);
            pConstants->CommitChanges();
        }

        // depth prepass
        if (m_options.EnableDepthPrepass)
            DrawDepthPrepass(pCommandList, pViewParameters);

        // gbuffer
        DrawGBuffers(pCommandList, pViewParameters);

        // if occlusion culling is enabled, draw proxies
        if ((m_options.EnableOcclusionCulling | m_options.EnableOcclusionPredication) != 0)
            DrawOcclusionCullingProxies(pCommandList, &pViewParameters->ViewCamera);

        // draw lights
        DrawLights(pCommandList, pViewParameters);

        // unlit objects
        //DrawUnlitObjects(pCommandList, pViewParameters);

        // apply AO
        if (m_options.EnableSSAO)
            ApplyAmbientOcclusion(pCommandList, pViewParameters);

        // apply fog
        if (pViewParameters->FogMode != RENDERER_FOG_MODE_NONE)
            ApplyFog(pCommandList, pViewParameters);

        // transparent objects
        DrawPostProcessAndTranslucentObjects(pCommandList, pViewParameters);
    });

    // complete all passes and queue to device
    ExecuteRenderPasses();

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

void DeferredShadingWorldRenderer::GetRenderStats(RenderStats *pRenderStats) const
{
    CompositingWorldRenderer::GetRenderStats(pRenderStats);
   
    pRenderStats->ShadowMapCount = 0;
    for (uint32 i = 0; i < m_directionalShadowMaps.GetSize(); i++)
    {
        if (m_directionalShadowMaps[i].IsActive)
            pRenderStats->ShadowMapCount++;
    }
    for (uint32 i = 0; i < m_pointShadowMaps.GetSize(); i++)
    {
        if (m_pointShadowMaps[i].IsActive)
            pRenderStats->ShadowMapCount++;
    }
    for (uint32 i = 0; i < m_spotShadowMaps.GetSize(); i++)
    {
        if (m_spotShadowMaps[i].IsActive)
            pRenderStats->ShadowMapCount++;
    }
}

void DeferredShadingWorldRenderer::OnFrameComplete()
{
    CompositingWorldRenderer::OnFrameComplete();

    // clear last indices
    m_lastDirectionalLightIndex = 0xFFFFFFFF;
    m_lastPointLightIndex = 0xFFFFFFFF;
    m_lastSpotLightIndex = 0xFFFFFFFF;
    m_lastVolumetricLightIndex = 0xFFFFFFFF;
}

void DeferredShadingWorldRenderer::DrawShadowMaps(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "DrawShadowMaps", MAKE_COLOR_R8G8B8_UNORM(255, 100, 255));

    // clear all shadow map states
    for (uint32 i = 0; i < m_directionalShadowMaps.GetSize(); i++)
        m_directionalShadowMaps[i].IsActive = false;
    for (uint32 i = 0; i < m_pointShadowMaps.GetSize(); i++)
        m_pointShadowMaps[i].IsActive = false;
    for (uint32 i = 0; i < m_spotShadowMaps.GetSize(); i++)
        m_spotShadowMaps[i].IsActive = false;

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

bool DeferredShadingWorldRenderer::DrawDirectionalShadowMap(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight)
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

    // pass on to the sm renderer
    QueueSecondaryRenderPass([this, pRenderWorld, pViewParameters, pLight, pShadowMapData](GPUCommandList *pCommandList) {
        m_pDirectionalShadowMapRenderer->DrawShadowMap(pCommandList, pShadowMapData, &pViewParameters->ViewCamera, pViewParameters->MaximumShadowViewDistance, pRenderWorld, pLight);
    });

    // ok
    return true;
}

bool DeferredShadingWorldRenderer::DrawPointShadowMap(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight)
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

    // pass on to the sm renderer
    QueueSecondaryRenderPass([this, pRenderWorld, pViewParameters, pLight, pShadowMapData](GPUCommandList *pCommandList) {
        m_pPointShadowMapRenderer->DrawShadowMap(pCommandList, pShadowMapData, &pViewParameters->ViewCamera, pViewParameters->MaximumShadowViewDistance, pRenderWorld, pLight);
    });

    // ok
    return true;
}

bool DeferredShadingWorldRenderer::DrawSpotShadowMap(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, RENDER_QUEUE_SPOT_LIGHT_ENTRY *pLight)
{
    return false;
}

void DeferredShadingWorldRenderer::SetCommonShaderProgramParameters(GPUCommandList *pCommandList, const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, ShaderProgram *pShaderProgram)
{
    pQueueEntry->pMaterial->BindDeviceResources(pCommandList, pShaderProgram);
    pQueueEntry->pRenderProxy->SetupForDraw(&pViewParameters->ViewCamera, pQueueEntry, pCommandList, pShaderProgram);

    // post process material?
    const MaterialShader *pMaterialShader = pQueueEntry->pMaterial->GetShader();
    if (pMaterialShader->GetRenderMode() == MATERIAL_RENDER_MODE_POST_PROCESS)
    {
        // set post process textures
        DebugAssert(m_pSceneColorBufferCopy != nullptr && m_pSceneDepthBufferCopy != nullptr);
        const uint32 BASE_INDEX = pMaterialShader->GetTextureParameterCount();
        pShaderProgram->SetMaterialParameterTexture(pCommandList, BASE_INDEX + 0, m_pSceneColorBufferCopy->pTexture, nullptr);
        pShaderProgram->SetMaterialParameterTexture(pCommandList, BASE_INDEX + 1, m_pSceneDepthBufferCopy->pTexture, nullptr);
    }
}

uint32 DeferredShadingWorldRenderer::DrawBasePassForObject(GPUCommandList *pCommandList, const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, bool depthWrites, GPU_COMPARISON_FUNC depthFunc)
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
        pCommandList->SetRasterizerState(pMaterialShader->SelectRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false));
        pCommandList->SetDepthStencilState(pMaterialShader->SelectDepthStencilState(true, depthWrites, depthFunc), 0);

        // bind shader
        pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());
        SetBlendingModeForMaterial(pCommandList, pQueueEntry);
        SetCommonShaderProgramParameters(pCommandList, pViewParameters, pQueueEntry, pShaderProgram);

        // draw away
        pQueueEntry->pRenderProxy->DrawQueueEntry(&pViewParameters->ViewCamera, pQueueEntry, pCommandList);
    }

    // done
    return 1;
}

void DeferredShadingWorldRenderer::DrawEmptyPassForObject(GPUCommandList *pCommandList, const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry)
{
    const MaterialShader *pMaterialShader = pQueueEntry->pMaterial->GetShader();

    // get shader
    ShaderProgram *pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(BasePassShader), 0, pQueueEntry);
    if (pShaderProgram != nullptr)
    {
        // set initial states
        pCommandList->SetRasterizerState(pMaterialShader->SelectRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false));
        pCommandList->SetDepthStencilState(pMaterialShader->SelectDepthStencilState(true, true, GPU_COMPARISON_FUNC_LESS), 0);

        // bind shader
        pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());
        SetBlendingModeForMaterial(pCommandList, pQueueEntry);
        SetCommonShaderProgramParameters(pCommandList, pViewParameters, pQueueEntry, pShaderProgram);

        // draw it
        pQueueEntry->pRenderProxy->DrawQueueEntry(&pViewParameters->ViewCamera, pQueueEntry, pCommandList);
    }
}

uint32 DeferredShadingWorldRenderer::DrawForwardLightPassesForObject(GPUCommandList *pCommandList, const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, bool useAdditiveBlending, bool depthWrites, GPU_COMPARISON_FUNC depthFunc)
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
            pCommandList->SetRasterizerState(pMaterialShader->SelectRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false)); \
            pCommandList->SetDepthStencilState(pMaterialShader->SelectDepthStencilState(true, depthWrites, depthFunc), 0); \
            if (useAdditiveBlending) \
                SetAdditiveBlendingModeForMaterial(pCommandList, pQueueEntry); \
            else \
                SetBlendingModeForMaterial(pCommandList, pQueueEntry); \
        } \
        else if (drawnLights == 1) \
        { \
            if (!useAdditiveBlending) \
                SetAdditiveBlendingModeForMaterial(pCommandList, pQueueEntry); \
            if (depthWrites) \
                pCommandList->SetDepthStencilState(pMaterialShader->SelectDepthStencilState(true, false, GPU_COMPARISON_FUNC_EQUAL), 0); \
        } \
        SetCommonShaderProgramParameters(pCommandList, pViewParameters, pQueueEntry, pShaderProgram); \
        pQueueEntry->pRenderProxy->DrawQueueEntry(&pViewParameters->ViewCamera, pQueueEntry, pCommandList); \
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
        pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());

        // using shadow map? note: we need the shadow map data to set the constant buffer up, even if this object does not use it
        const DirectionalShadowMapRenderer::ShadowMapData *pShadowMapData = (pLight->ShadowMapIndex >= 0) ? &m_directionalShadowMaps[pLight->ShadowMapIndex] : nullptr;

        // set constant buffer parameters
        if (m_lastDirectionalLightIndex != lightIndex)
        {
            DirectionalLightShader::SetLightParameters(pCommandList, pLight, pShadowMapData);
            m_lastDirectionalLightIndex = lightIndex;
        }

        // and the shadow map textures
        if (pShadowMapData)
            DirectionalLightShader::SetProgramParameters(pCommandList, pShaderProgram, pLight, pShadowMapData);

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
        if (!CollisionDetection::AABoxIntersectsSphere(pQueueEntry->BoundingBox.GetMinBounds(), pQueueEntry->BoundingBox.GetMaxBounds(), pLight->Position, pLight->Range))
            continue;

        // we only draw shadowed lights immediately, otherwise queue
        if (!usingShadowMap)
        {
            if (queuedLightCount == PointLightListShader::MAX_LIGHTS)
            {
                if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(PointLightListShader), 0, pQueueEntry)) != nullptr)
                {
                    // bind shader
                    pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());

                    // initialize light pointers
                    for (uint32 queuedLightIndex = 0; queuedLightIndex < queuedLightCount; queuedLightIndex++)
                        PointLightListShader::SetLightParameters(pCommandList, queuedLightIndex, queuedLights[queuedLightIndex]);

                    // commit parameters, and draw
                    PointLightListShader::SetActiveLightCount(pCommandList, queuedLightCount);
                    PointLightListShader::CommitParameters(pCommandList);
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
        pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());

        // using shadow map? note: we need the shadow map data to set the constant buffer up, even if this object does not use it
        const PointShadowMapRenderer::ShadowMapData *pShadowMapData = (pLight->ShadowMapIndex >= 0) ? &m_pointShadowMaps[pLight->ShadowMapIndex] : nullptr;

        // set constant buffer parameters
        if (m_lastPointLightIndex != lightIndex)
        {
            PointLightShader::SetLightParameters(pCommandList, pLight, pShadowMapData);
            m_lastPointLightIndex = lightIndex;
        }
        
        // set program parameters
        PointLightShader::SetProgramParameters(pCommandList, pShaderProgram, pLight, pShadowMapData);

        // draw light
        DRAW_LIGHT();
    }

    // if there's only one remaining light, we can use the fast shader, otherwise use the multi-light shader
    if (queuedLightCount == 1)
    {
        if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(PointLightShader), 0, pQueueEntry)) != nullptr)
        {
            pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());
            PointLightShader::SetLightParameters(pCommandList, queuedLights[0], nullptr);
            DRAW_LIGHT();
        }
    }
    else if (queuedLightCount > 0)
    {
        if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(PointLightListShader), 0, pQueueEntry)) != nullptr)
        {
            // bind shader
            pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());

            // initialize light pointers
            for (uint32 queuedLightIndex = 0; queuedLightIndex < queuedLightCount; queuedLightIndex++)
                PointLightListShader::SetLightParameters(pCommandList, queuedLightIndex, queuedLights[queuedLightIndex]);

            // commit parameters, and draw
            PointLightListShader::SetActiveLightCount(pCommandList, queuedLightCount);
            PointLightListShader::CommitParameters(pCommandList);
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
        pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());

        // and parameters
        if (m_lastVolumetricLightIndex != lightIndex)
        {
            VolumetricLightShader::SetLightParameters(pCommandList, pLight);
            m_lastVolumetricLightIndex = lightIndex;
        }

        // set program parameters
        VolumetricLightShader::SetProgramParameters(pCommandList, pShaderProgram, pLight);

        // draw light
        DRAW_LIGHT();
    }

#undef DRAW_LIGHT
    
    return drawnLights;
}

void DeferredShadingWorldRenderer::DrawDepthPrepass(GPUCommandList *pCommandList, const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "DrawDepthPrepass", MAKE_COLOR_R8G8B8_UNORM(255, 100, 100));

    ShaderProgramSelector shaderSelector(m_globalShaderFlags);
    shaderSelector.SetBaseShader(OBJECT_TYPEINFO(DepthOnlyShader), 0);

    // Everything here uses the normal rasterizer state, so set that up here.
    pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateNoColorWrites());
    pCommandList->SetRenderTargets(0, nullptr, m_pSceneDepthBuffer->pDSV);

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
        pCommandList->SetRasterizerState(pMaterialShader->SelectRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false));
        pCommandList->SetDepthStencilState(pMaterialShader->SelectDepthStencilState(true, true, GPU_COMPARISON_FUNC_LESS), 0);

        // For now, only masked materials are drawn with clipping
        shaderSelector.SetVertexFactory(pQueueEntry->pVertexFactoryTypeInfo, pQueueEntry->VertexFactoryFlags);
        shaderSelector.SetMaterial((pQueueEntry->pMaterial->GetShader()->GetBlendMode() == MATERIAL_BLENDING_MODE_MASKED) ? pQueueEntry->pMaterial : nullptr);

        // only continue with shader
        ShaderProgram *pShaderProgram = shaderSelector.MakeActive(pCommandList);
        if (pShaderProgram != nullptr)
        {
            pCommandList->SetPredication(pQueueEntry->pPredicate);
            pQueueEntry->pRenderProxy->SetupForDraw(&pViewParameters->ViewCamera, pQueueEntry, pCommandList, pShaderProgram);
            pQueueEntry->pRenderProxy->DrawQueueEntry(&pViewParameters->ViewCamera, pQueueEntry, pCommandList);
        }
    }

    // clear predication
    pCommandList->SetPredication(nullptr);
}

void DeferredShadingWorldRenderer::DrawUnlitObjects(GPUCommandList *pCommandList, const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "DrawUnlitObjects", MAKE_COLOR_R8G8B8_UNORM(255, 20, 255));

    // Bind light buffer and depth buffer.
    pCommandList->SetRenderTargets(1, &m_pSceneColorBuffer->pRTV, m_pSceneDepthBuffer->pDSV);
    
    // Iterate over renderables.
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_renderQueue.GetOpaqueRenderables().GetBasePointer();
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_renderQueue.GetOpaqueRenderables().GetBasePointer() + m_renderQueue.GetOpaqueRenderables().GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        if (pQueueEntry->RenderPassMask & (RENDER_PASS_EMISSIVE | RENDER_PASS_LIGHTMAP))
        {
            pCommandList->SetPredication(pQueueEntry->pPredicate);
            if (DrawBasePassForObject(pCommandList, pViewParameters, pQueueEntry, !m_options.EnableDepthPrepass, (CVars::r_depth_prepass.GetBool()) ? GPU_COMPARISON_FUNC_LESS_EQUAL : GPU_COMPARISON_FUNC_LESS) > 0)
                continue;
        }
    }

    // clear predication
    pCommandList->SetPredication(nullptr);
}

void DeferredShadingWorldRenderer::DrawGBuffers(GPUCommandList *pCommandList, const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "DrawGBuffers", MAKE_COLOR_R8G8B8_UNORM(50, 150, 255));

    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry;
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd;
    ShaderProgramSelector shaderSelector(m_globalShaderFlags);

    // Bind the light buffer and clear it with the fog colour
    pCommandList->SetRenderTargets(1, &m_pSceneColorBuffer->pRTV, nullptr);
    pCommandList->ClearTargets(true, false, false, PixelFormatHelpers::ConvertSRGBToLinear(pViewParameters->FogColor));

    // Switch to only the gbuffers.
    GPURenderTargetView *pGBufferRenderTargets[] = { m_pGBuffer0->pRTV, m_pGBuffer1->pRTV, m_pGBuffer2->pRTV };
    pCommandList->SetRenderTargets(countof(pGBufferRenderTargets), pGBufferRenderTargets, m_pSceneDepthBuffer->pDSV);
    pCommandList->ClearTargets(true, false, false, float4::Zero);

    // Iterate over renderables.
    /*pQueueEntry = m_renderQueue.GetOpaqueRenderables().GetBasePointer();
    pQueueEntryEnd = m_renderQueue.GetOpaqueRenderables().GetBasePointer() + m_renderQueue.GetOpaqueRenderables().GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        // skip occlusion-culled objects, objects that have to be drawn as emissive/lightmap, they are done afterwards
        if (pQueueEntry->RenderPassMask == 0 || (pQueueEntry->RenderPassMask & (RENDER_PASS_EMISSIVE | RENDER_PASS_LIGHTMAP)))
            continue;

        // we only have to write the pixels that have have to be lit to the buffer, since the depth buffer is already "complete" at this point
        if (pQueueEntry->RenderPassMask & (RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING))
        {
            GPUShaderProgram *pShaderProgram = GetShaderPermutation(OBJECT_TYPEINFO(DeferredGBufferShader), 0, pQueueEntry);
            if (pShaderProgram != nullptr)
            {
                const MaterialShader *pMaterialShader = pQueueEntry->pMaterial->GetShader();

                // set states
                pCommandList->SetRasterizerState(pMaterialShader->SelectRasterizerState(RENDERER_FILL_SOLID, false, false));
                pCommandList->SetDepthStencilState(pMaterialShader->SelectDepthStencilState(true, !CVars::r_depth_prepass.GetBool(), (CVars::r_depth_prepass.GetBool()) ? GPU_COMPARISON_FUNC_LESS_EQUAL : GPU_COMPARISON_FUNC_LESS), 0);

                // bind/draw
                pCommandList->SetShaderProgram(pShaderProgram);
                SetBlendingModeForMaterial(pCommandList, pQueueEntry);
                SetCommonShaderProgramParameters(pCommandList, pViewParameters, pQueueEntry, pShaderProgram);
                pQueueEntry->pRenderProxy->DrawQueueEntry(pViewParameters, pQueueEntry, pCommandList);
            }
        }
    }*/

    // Switch render targets to include the light buffer, for drawing emissive and lightmapped objects
    GPURenderTargetView *pLightBufferGBufferRenderTargets[] = { m_pGBuffer0->pRTV, m_pGBuffer1->pRTV, m_pGBuffer2->pRTV, m_pSceneColorBuffer->pRTV };
    pCommandList->SetRenderTargets(countof(pLightBufferGBufferRenderTargets), pLightBufferGBufferRenderTargets, m_pSceneDepthBuffer->pDSV);
    pQueueEntry = m_renderQueue.GetOpaqueRenderables().GetBasePointer();
    pQueueEntryEnd = m_renderQueue.GetOpaqueRenderables().GetBasePointer() + m_renderQueue.GetOpaqueRenderables().GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        uint32 shaderFlags = DeferredGBufferShader::WITH_LIGHTBUFFER;
        if (pQueueEntry->RenderPassMask == 0)
            continue;
        if (pQueueEntry->RenderPassMask & RENDER_PASS_LIGHTMAP)
            shaderFlags |= DeferredGBufferShader::WITH_LIGHTMAP;
        if (!(pQueueEntry->RenderPassMask & (RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING)))
            shaderFlags |= DeferredGBufferShader::NO_ALBEDO;

        // update selector
        shaderSelector.SetBaseShader(OBJECT_TYPEINFO(DeferredGBufferShader), shaderFlags);
        shaderSelector.SetQueueEntry(pQueueEntry);

        // get shader
        ShaderProgram *pShaderProgram = shaderSelector.MakeActive(pCommandList);
        if (pShaderProgram != nullptr)
        {
            const MaterialShader *pMaterialShader = pQueueEntry->pMaterial->GetShader();

            // set states
            pCommandList->SetRasterizerState(pMaterialShader->SelectRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false));
            pCommandList->SetDepthStencilState(pMaterialShader->SelectDepthStencilState(true, !CVars::r_depth_prepass.GetBool(), (CVars::r_depth_prepass.GetBool()) ? GPU_COMPARISON_FUNC_LESS_EQUAL : GPU_COMPARISON_FUNC_LESS), 0);

            // bind
            SetBlendingModeForMaterial(pCommandList, pQueueEntry);
            pQueueEntry->pRenderProxy->SetupForDraw(&pViewParameters->ViewCamera, pQueueEntry, pCommandList, pShaderProgram);

            // set predication as the last step to avoid duplicate calls
            pCommandList->SetPredication(pQueueEntry->pPredicate);

            // draw
            pQueueEntry->pRenderProxy->DrawQueueEntry(&pViewParameters->ViewCamera, pQueueEntry, pCommandList);
        }
    }

    // clear predication
    pCommandList->SetPredication(nullptr);

    // add them for debugging
    AddDebugBufferView(m_pGBuffer0, "GBuffer0", false);
    AddDebugBufferView(m_pGBuffer1, "GBuffer1", false);
    AddDebugBufferView(m_pGBuffer2, "GBuffer2", false);
}

void DeferredShadingWorldRenderer::ApplyAmbientOcclusion(GPUCommandList *pCommandList, const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "SSAO", MAKE_COLOR_R8G8B8_UNORM(75, 20, 150));

    IntermediateBuffer *pSSAOBuffer = RequestIntermediateBuffer(m_options.RenderWidth, m_options.RenderHeight, PIXEL_FORMAT_R8_UNORM, 1);
    if (pSSAOBuffer == nullptr)
        return;

    IntermediateBuffer *pDownscaledSSAOBuffer = RequestIntermediateBuffer(m_options.RenderWidth / 2, m_options.RenderHeight / 2, PIXEL_FORMAT_R8_UNORM, 1);
    if (pDownscaledSSAOBuffer == nullptr)
    {
        ReleaseIntermediateBuffer(pSSAOBuffer);
        return;
    }

    // run ssao shader
    {
        MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "Generate SSAO", MAKE_COLOR_R8G8B8_UNORM(75, 20, 150));

        pCommandList->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK));
        pCommandList->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false), 0);
        pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateNoBlending());
        pCommandList->SetRenderTargets(1, &pSSAOBuffer->pRTV, nullptr);
        pCommandList->SetShaderProgram(m_pSSAOProgram->GetGPUProgram());
        SSAOShader::SetProgramParameters(pCommandList, m_pSSAOProgram, m_pSceneDepthBuffer->pTexture, m_pGBuffer1->pTexture);
        g_pRenderer->DrawFullScreenQuad(pCommandList);
    }

    // downsample the AO buffer
    {
        MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "Downsample SSAO", MAKE_COLOR_R8G8B8_UNORM(20, 75, 150));
        ScaleTexture(pCommandList, pSSAOBuffer->pTexture, pDownscaledSSAOBuffer->pRTV, true, false);
    }

    // unbind source/dest from state
    pCommandList->ClearState(true, false, false, true);

    // draw/blend to light buffer
    {
        MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "Apply SSAO", MAKE_COLOR_R8G8B8_UNORM(20, 150, 75));

        pCommandList->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK));
        pCommandList->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false), 0);
        pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStatePremultipliedAlpha());
        pCommandList->SetRenderTargets(1, &m_pSceneColorBuffer->pRTV, nullptr);
        pCommandList->SetShaderProgram(m_pSSAOApplyProgram->GetGPUProgram());
        SSAOApplyShader::SetProgramParameters(pCommandList, m_pSSAOApplyProgram, pDownscaledSSAOBuffer->pTexture);
        g_pRenderer->DrawFullScreenQuad(pCommandList);
    }

    // clear shader resources out
    pCommandList->ClearState(true, false, false, false);

    // release downsampled buffer
    AddDebugBufferView(pDownscaledSSAOBuffer, "SSAO", true);
    ReleaseIntermediateBuffer(pSSAOBuffer);
}

void DeferredShadingWorldRenderer::ApplyFog(GPUCommandList *pCommandList, const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "ApplyFog", MAKE_COLOR_R8G8B8_UNORM(50, 42, 150));

    // draw/blend to light buffer
    pCommandList->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK));
    pCommandList->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false), 0);
    pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAlphaBlending());
    pCommandList->SetRenderTargets(1, &m_pSceneColorBuffer->pRTV, nullptr);

    // blend ao
    ShaderProgram *pShaderProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(DeferredFogShader), DeferredFogShader::GetFlagsForMode(pViewParameters->FogMode), g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributes(), g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributeCount(), nullptr, 0);
    if (pShaderProgram != nullptr)
    {
        pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());
        DeferredFogShader::SetProgramParameters(pCommandList, pShaderProgram, pViewParameters, m_pSceneDepthBuffer->pTexture);
        g_pRenderer->DrawFullScreenQuad(pCommandList);
    }

    // clear shader resources out
    pCommandList->ClearState(true, false, false, false);
}

void DeferredShadingWorldRenderer::DrawLights(GPUCommandList *pCommandList, const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "DrawLights", MAKE_COLOR_R8G8B8_UNORM(42, 100, 25));

    // bind the light buffer without any depth buffer, and clear it
    pCommandList->SetRenderTargets(1, &m_pSceneColorBuffer->pRTV, nullptr);

    // use light volumes
    DrawLights_DirectionalLights(pCommandList, pViewParameters);
    DrawLights_PointLights_ByLightVolumes(pCommandList, pViewParameters);
    //DrawLights_PointLights_Tiled(pCommandList, pViewParameters);

    // draw wireframe overlay
    if (m_options.ShowWireframeOverlay)
    {
        pCommandList->SetRenderTargets(1, &m_pSceneColorBuffer->pRTV, m_pSceneDepthBuffer->pDSV);
        DrawWireframeOverlay(pCommandList, &pViewParameters->ViewCamera, &m_renderQueue.GetOpaqueRenderables());
    }
}

void DeferredShadingWorldRenderer::DrawLights_DirectionalLights(GPUCommandList *pCommandList, const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "DrawLights_DirectionalLights", MAKE_COLOR_R8G8B8_UNORM(42, 25, 100));

    // setup state for directional lights -- possibly use stencil buffer for pixels that have to be shaded??
    pCommandList->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false, false));
    pCommandList->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false, GPU_COMPARISON_FUNC_ALWAYS), 0);
    pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAdditive());
    pCommandList->SetRenderTargets(1, &m_pSceneColorBuffer->pRTV, nullptr);
    pCommandList->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_STRIP);

    // draw directional lights
    for (const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY &currentLight : m_renderQueue.GetDirectionalLightArray())
    {
        // apply shadow flags
        uint32 shaderFlags = 0;
        if (currentLight.ShadowMapIndex >= 0)
            shaderFlags = DeferredDirectionalLightShader::CalculateShadowFlags(m_options.EnableShadows, m_options.EnableHardwareShadowFiltering, m_options.ShadowMapFiltering, m_options.ShowShadowMapCascades);

        // get shader
        ShaderProgram *pShaderProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(DeferredDirectionalLightShader), shaderFlags, g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributes(), g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributeCount(), nullptr, 0);
        if (pShaderProgram == nullptr)
            continue;

        // apply shader parameters
        pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());
        DeferredDirectionalLightShader::SetBufferParameters(pCommandList, pShaderProgram, m_pSceneDepthBuffer->pTexture, m_pGBuffer0->pTexture, m_pGBuffer1->pTexture, m_pGBuffer2->pTexture);
        DeferredDirectionalLightShader::SetLightParameters(pCommandList, pShaderProgram, pViewParameters, &currentLight);
        if (currentLight.ShadowMapIndex >= 0)
            DeferredDirectionalLightShader::SetShadowParameters(pCommandList, pShaderProgram, &m_directionalShadowMaps[currentLight.ShadowMapIndex]);

        DeferredDirectionalLightShader::CommitParameters(pCommandList, pShaderProgram);

        // draw a fullscreen quad
        //m_pGPUContext->Draw(0, 4);
        g_pRenderer->DrawFullScreenQuad(pCommandList);
    }

    // clear the shader state, ensuring the buffers are unbound from input
    pCommandList->ClearState(true, false, false, false);
}

void DeferredShadingWorldRenderer::DrawLights_PointLights_ByLightVolumes(GPUCommandList *pCommandList, const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "DrawLights_PointLights_ByLightVolumes", MAKE_COLOR_R8G8B8_UNORM(25, 90, 100));

    pCommandList->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false, false));
    pCommandList->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false, GPU_COMPARISON_FUNC_ALWAYS), 0);
    pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAdditive());
    pCommandList->SetRenderTargets(1, &m_pSceneColorBuffer->pRTV, nullptr);
    pCommandList->SetVertexBuffer(0, m_pPointLightVolumeVertexBuffer, 0, sizeof(float3));
    pCommandList->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);

    // light queue
    ShaderProgram *pShaderProgram;
    uint32 queuedLightCount[2] = { 0, 0 };
    const RENDER_QUEUE_POINT_LIGHT_ENTRY *queuedLights[2][DeferredPointLightListShader::MAX_LIGHTS];

    // draw point lights
    for (const RENDER_QUEUE_POINT_LIGHT_ENTRY &currentLight : m_renderQueue.GetPointLightArray())
    {
        // check if we are inside the light volume, if so, precaution needs to be taken
        bool insideLightVolume = pViewParameters->ViewCamera.GetPosition().SquaredDistance(currentLight.Position) < (Math::Square(currentLight.Range) + 1.0f);

        // queue non-shadowed lights
        if (currentLight.ShadowMapIndex < 0)
        {
            // flush queue
            if (queuedLightCount[insideLightVolume] == DeferredPointLightListShader::MAX_LIGHTS)
            {
                // use front-face culling if inside light volume, otherwise back-face culling
                pCommandList->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, (insideLightVolume) ? RENDERER_CULL_FRONT : RENDERER_CULL_BACK, false, false, false));

                // get shader
                pShaderProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(DeferredPointLightListShader), 0, g_pRenderer->GetFixedResources()->GetPositionOnlyVertexAttributes(), g_pRenderer->GetFixedResources()->GetPositionOnlyVertexAttributeCount(), nullptr, 0);
                if (pShaderProgram != nullptr)
                {
                    pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());
                    DeferredPointLightListShader::SetBufferParameters(pCommandList, pShaderProgram, m_pSceneDepthBuffer->pTexture, m_pGBuffer0->pTexture, m_pGBuffer1->pTexture, m_pGBuffer2->pTexture);
                    for (uint32 i = 0; i < DeferredPointLightListShader::MAX_LIGHTS; i++)
                        DeferredPointLightListShader::SetLightParameters(pCommandList, pShaderProgram, pViewParameters, i, queuedLights[insideLightVolume][i]);

                    // draw the light volume
                    pCommandList->DrawInstanced(0, m_pointLightVolumeVertexCount, DeferredPointLightListShader::MAX_LIGHTS);
                }

                // done
                queuedLightCount[insideLightVolume] = 0;
            }

            // add to queue            
            queuedLights[insideLightVolume][queuedLightCount[insideLightVolume]++] = &currentLight;
            continue;
        }


        // use front-face culling if inside light volume, otherwise back-face culling
        pCommandList->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, (insideLightVolume) ? RENDERER_CULL_FRONT : RENDERER_CULL_BACK, false, false, false));

        // apply shadow flags
        uint32 shaderFlags = DeferredPointLightShader::CalculateShadowFlags(m_options.EnableShadows, m_options.EnableHardwareShadowFiltering);

        // get shader
        pShaderProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(DeferredPointLightShader), shaderFlags, g_pRenderer->GetFixedResources()->GetPositionOnlyVertexAttributes(), g_pRenderer->GetFixedResources()->GetPositionOnlyVertexAttributeCount(), nullptr, 0);
        if (pShaderProgram == nullptr)
            continue;

        // set program parameters
        pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());
        DeferredPointLightShader::SetBufferParameters(pCommandList, pShaderProgram, m_pSceneDepthBuffer->pTexture, m_pGBuffer0->pTexture, m_pGBuffer1->pTexture, m_pGBuffer2->pTexture);
        DeferredPointLightShader::SetLightParameters(pCommandList, pShaderProgram, pViewParameters, &currentLight);
        DeferredPointLightShader::SetShadowParameters(pCommandList, pShaderProgram, &m_pointShadowMaps[currentLight.ShadowMapIndex]);

        // draw the light volume
        pCommandList->Draw(0, m_pointLightVolumeVertexCount);
    }

    // flush the queues
    for (uint32 queueIndex = 0; queueIndex < 2; queueIndex++)
    {
        uint32 lightCount = queuedLightCount[queueIndex];
        if (lightCount == 0)
            continue;

        // use front-face culling if inside light volume, otherwise back-face culling
        pCommandList->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, (queueIndex) ? RENDERER_CULL_FRONT : RENDERER_CULL_BACK, false, false, false));

        // get shader
        pShaderProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(DeferredPointLightListShader), 0, g_pRenderer->GetFixedResources()->GetPositionOnlyVertexAttributes(), g_pRenderer->GetFixedResources()->GetPositionOnlyVertexAttributeCount(), nullptr, 0);
        if (pShaderProgram != nullptr)
        {
            pCommandList->SetShaderProgram(pShaderProgram->GetGPUProgram());
            DeferredPointLightListShader::SetBufferParameters(pCommandList, pShaderProgram, m_pSceneDepthBuffer->pTexture, m_pGBuffer0->pTexture, m_pGBuffer1->pTexture, m_pGBuffer2->pTexture);
            for (uint32 i = 0; i < lightCount; i++)
                DeferredPointLightListShader::SetLightParameters(pCommandList, pShaderProgram, pViewParameters, i, queuedLights[queueIndex][i]);

            // draw the light volume
            pCommandList->DrawInstanced(0, m_pointLightVolumeVertexCount, lightCount);
        }
    }

    // clear the shader state, ensuring the buffers are unbound from input
    pCommandList->ClearState(true, false, false, false);
}

void DeferredShadingWorldRenderer::DrawLights_PointLights_Tiled(GPUCommandList *pCommandList, const ViewParameters *pViewParameters)
{
#if 0
    MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "DrawLights_PointLights_Tiled", MAKE_COLOR_R8G8B8_UNORM(25, 90, 100));

    // setup for drawing volumes
    m_pGPUContext->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false, false));
    m_pGPUContext->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false, GPU_COMPARISON_FUNC_ALWAYS), 0);
    m_pGPUContext->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAdditive());
    m_pGPUContext->SetRenderTargets(1, &m_pSceneColorBuffer->pRTV, nullptr);
    m_pGPUContext->SetVertexBuffer(0, m_pPointLightVolumeVertexBuffer, 0, sizeof(float3));
    m_pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);

    // find lights
    m_tiledPointLights.Clear();
    for (const RENDER_QUEUE_POINT_LIGHT_ENTRY &currentLight : m_renderQueue.GetPointLightArray())
    {
        // queue non-shadowed lights
        if (currentLight.ShadowMapIndex < 0)
        {
            m_tiledPointLights.Emplace(currentLight.Position, currentLight.InverseRange, currentLight.LightColor, currentLight.FalloffExponent);
            continue;
        }

        // draw shadowed lights
        // check if we are inside the light volume, if so, precaution needs to be taken
        bool insideLightVolume = pViewParameters->ViewCamera.GetPosition().SquaredDistance(currentLight.Position) < (Math::Square(currentLight.Range) + 1.0f);
        if (insideLightVolume)
        {
            // use front-face culling
            m_pGPUContext->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_FRONT, false, false, false));
        }
        else
        {
            // use back-face culling
            m_pGPUContext->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false, false));
        }

        // apply shadow flags
        uint32 shaderFlags = 0;
        if (currentLight.ShadowMapIndex >= 0)
            shaderFlags = DeferredPointLightShader::CalculateShadowFlags(m_options.EnableShadows, m_options.EnableHardwareShadowFiltering);

        // get shader
        ShaderProgram *pShaderProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(DeferredPointLightShader), shaderFlags, g_pRenderer->GetFixedResources()->GetPositionOnlyVertexAttributes(), g_pRenderer->GetFixedResources()->GetPositionOnlyVertexAttributeCount(), nullptr, 0);
        if (pShaderProgram == nullptr)
            continue;

        // set program parameters
        m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());
        DeferredPointLightShader::SetBufferParameters(m_pGPUContext, pShaderProgram, m_pSceneDepthBuffer->pTexture, m_pGBuffer0->pTexture, m_pGBuffer1->pTexture, m_pGBuffer2->pTexture);
        DeferredPointLightShader::SetLightParameters(m_pGPUContext, pShaderProgram, pViewParameters, &currentLight);
        if (currentLight.ShadowMapIndex >= 0)
            DeferredPointLightShader::SetShadowParameters(m_pGPUContext, pShaderProgram, &m_pointShadowMaps[currentLight.ShadowMapIndex]);

        // draw the light volume
        m_pGPUContext->Draw(0, m_pointLightVolumeVertexCount);
    }

    // if there's no tiled lights to draw, bail out
    if (m_tiledPointLights.IsEmpty())
        return;

    // calculate tile counts
    const uint32 tileSize = DeferredTiledPointLightShader::TILE_SIZE;
    uint32 tileCountX = m_options.RenderWidth / tileSize + (((m_options.RenderWidth % tileSize) != 0) ? 1 : 0);
    uint32 tileCountY = m_options.RenderHeight / tileSize + (((m_options.RenderHeight % tileSize) != 0) ? 1 : 0);;

    // get tiled shader
    ShaderProgram *pShaderProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(DeferredTiledPointLightShader), 0, g_pRenderer->GetFixedResources()->GetPositionOnlyVertexAttributes(), g_pRenderer->GetFixedResources()->GetPositionOnlyVertexAttributeCount(), nullptr, 0);
    if (pShaderProgram != nullptr)
    {
        // drop the render targets, the compute shader accesses them directly
        m_pGPUContext->SetRenderTargets(0, nullptr, nullptr);

        // fill common shader information
        m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());
        DeferredTiledPointLightShader::SetProgramParameters(m_pGPUContext, pShaderProgram, tileCountX, tileCountY);
        DeferredTiledPointLightShader::SetBufferParameters(m_pGPUContext, pShaderProgram, m_pSceneDepthBuffer->pTexture, m_pGBuffer0->pTexture, m_pGBuffer1->pTexture, m_pGBuffer2->pTexture, nullptr /* FIXME SHOULD BE COPY */);

        // dispatch until we consume all lights
        for (uint32 baseLightIndex = 0; baseLightIndex < m_tiledPointLights.GetSize(); baseLightIndex += DeferredTiledPointLightShader::MAX_LIGHTS_PER_DISPATCH)
        {
            // calculate light count
            uint32 passLightCount = Min(m_tiledPointLights.GetSize() - baseLightIndex, DeferredTiledPointLightShader::MAX_LIGHTS_PER_DISPATCH);

            // set light information
            DeferredTiledPointLightShader::SetLights(m_pGPUContext, pShaderProgram, m_tiledPointLights.GetBasePointer() + baseLightIndex, passLightCount);
            DeferredTiledPointLightShader::CommitParameters(m_pGPUContext, pShaderProgram);

            // invoke compute shader
            m_pGPUContext->Dispatch(tileCountX, tileCountY, 1);

            // temporary until refactor: blend back to the main light buffer
            m_pGPUContext->ClearState(true, false, false, false);
            m_pGPUContext->SetRenderTargets(1, &m_pSceneColorBuffer->pRTV, nullptr);
            m_pGPUContext->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAdditive());
            //g_pRenderer->BlitTextureUsingShader(m_pGPUContext, m_pLightBufferCopy, 0, 0, m_targetWidth, m_targetHeight, 0, 0, 0, m_targetWidth, m_targetHeight, RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_NEAREST, RENDERER_FRAMEBUFFER_BLIT_BLEND_MODE_ADDITIVE);
        }
    }

    // clear state, since the output buffer is bound as a uav this'll cause issues when we next bind it as a render target
    m_pGPUContext->ClearState(true, false, false, true);
#endif
}

void DeferredShadingWorldRenderer::DrawPostProcessAndTranslucentObjects(GPUCommandList *pCommandList, const ViewParameters *pViewParameters)
{
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry;
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd;
    MICROPROFILE_SCOPEI("DeferredShadingWorldRenderer", "DrawPostProcessAndTranslucentObjects", MAKE_COLOR_R8G8B8_UNORM(90, 25, 80));

    // set render target to the light buffer
    pCommandList->SetRenderTargets(1, &m_pSceneColorBuffer->pRTV, m_pSceneDepthBuffer->pDSV);

    // Draw post process objects before translucent
    if (m_renderQueue.GetPostProcessRenderables().GetSize() > 0)
    {
        // acquire copy of scene colour + depth
        m_pSceneColorBufferCopy = RequestIntermediateBufferMatching(m_pSceneColorBuffer);
        m_pSceneDepthBufferCopy = RequestIntermediateBufferMatching(m_pSceneDepthBuffer);
        pCommandList->CopyTexture(m_pSceneColorBuffer->pTexture, m_pSceneColorBufferCopy->pTexture);
        pCommandList->CopyTexture(m_pSceneDepthBuffer->pTexture, m_pSceneDepthBufferCopy->pTexture);

        // Draw objects
        pQueueEntry = m_renderQueue.GetPostProcessRenderables().GetBasePointer();
        pQueueEntryEnd = m_renderQueue.GetPostProcessRenderables().GetBasePointer() + m_renderQueue.GetPostProcessRenderables().GetSize();
        for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
        {
            uint32 renderPassMask = pQueueEntry->RenderPassMask;
            uint32 drawCount = 0;

            // skip anything without any passes
            if (renderPassMask == 0)
                continue;

            // draw base pass?
            if (renderPassMask & (RENDER_PASS_EMISSIVE | RENDER_PASS_LIGHTMAP | RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING))
                drawCount = DrawBasePassForObject(pCommandList, pViewParameters, pQueueEntry, false, GPU_COMPARISON_FUNC_LESS_EQUAL);

            // draw light passes?
            if (renderPassMask & (RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING))
                drawCount += DrawForwardLightPassesForObject(pCommandList, pViewParameters, pQueueEntry, (drawCount != 0), false, GPU_COMPARISON_FUNC_LESS_EQUAL);

            // draw blank pass if there is no draw calls for this object
            if (drawCount == 0)
                DrawEmptyPassForObject(pCommandList, pViewParameters, pQueueEntry);
        }

        // release intermediate buffers
        ReleaseIntermediateBuffer(m_pSceneDepthBufferCopy);
        ReleaseIntermediateBuffer(m_pSceneColorBufferCopy);
        m_pSceneDepthBufferCopy = nullptr;
        m_pSceneColorBufferCopy = nullptr;

        // draw wireframe overlay
        if (m_options.ShowWireframeOverlay)
            DrawWireframeOverlay(pCommandList, &pViewParameters->ViewCamera, &m_renderQueue.GetPostProcessRenderables());
    }

    // Draw translucent objects
    pQueueEntry = m_renderQueue.GetTranslucentRenderables().GetBasePointer();
    pQueueEntryEnd = m_renderQueue.GetTranslucentRenderables().GetBasePointer() + m_renderQueue.GetTranslucentRenderables().GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        uint32 renderPassMask = pQueueEntry->RenderPassMask;
        uint32 drawCount = 0;

        // skip anything without any passes
        if (renderPassMask == 0)
            continue;

        // draw base pass?
        if (renderPassMask & (RENDER_PASS_EMISSIVE | RENDER_PASS_LIGHTMAP | RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING))
            drawCount = DrawBasePassForObject(pCommandList, pViewParameters, pQueueEntry, false, GPU_COMPARISON_FUNC_LESS_EQUAL);

        // draw light passes?
        if (renderPassMask & (RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING))
            drawCount += DrawForwardLightPassesForObject(pCommandList, pViewParameters, pQueueEntry, (drawCount != 0), false, GPU_COMPARISON_FUNC_LESS_EQUAL);

        // draw blank pass if there is no draw calls for this object
        if (drawCount == 0)
            DrawEmptyPassForObject(pCommandList, pViewParameters, pQueueEntry);
    }

    // draw wireframe overlay
    if (m_options.ShowWireframeOverlay)
        DrawWireframeOverlay(pCommandList, &pViewParameters->ViewCamera, &m_renderQueue.GetTranslucentRenderables());
}

void DeferredShadingWorldRenderer::DrawDebugInfo(const Camera *pCamera)
{
    CompositingWorldRenderer::DrawDebugInfo(pCamera);
}
