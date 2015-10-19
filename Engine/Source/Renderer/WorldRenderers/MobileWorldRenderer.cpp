#include "Renderer/PrecompiledHeader.h"
#include "Renderer/WorldRenderers/MobileWorldRenderer.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/RenderProfiler.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/ShaderProgramSelector.h"
#include "Renderer/Shaders/DepthOnlyShader.h"
#include "Renderer/Shaders/MobileShaders.h"
#include "Engine/Material.h"
#include "Engine/EngineCVars.h"
#include "Engine/ResourceManager.h"
#include "Engine/Profiling.h"
#include "MathLib/CollisionDetection.h"
Log_SetChannel(MobileWorldRenderer);



MobileWorldRenderer::MobileWorldRenderer(GPUContext *pGPUContext, const Options *pOptions)
    : WorldRenderer(pGPUContext, pOptions)
    //, m_pSceneColorBuffer(nullptr)
    //, m_pSceneDepthBuffer(nullptr)
    , m_pDirectionalShadowMapRenderer(nullptr)
    , m_pSpotShadowMapRenderer(nullptr)
    , m_lastDirectionalLightIndex(0xFFFFFFFF)
    , m_lastPointLightIndex(0xFFFFFFFF)
    , m_lastSpotLightIndex(0xFFFFFFFF)
    , m_lastVolumetricLightIndex(0xFFFFFFFF)
    , m_directionalLightShaderFlags(MobileDirectionalLightShader::CalculateFlags(pOptions->EnableShadows, pOptions->ShadowMapFiltering))
    , m_pointLightShaderFlags(0)//PointLightShader::CalculateFlags(pOptions->EnableShadows, pOptions->EnableHardwareShadowFiltering))
    , m_spotLightShaderFlags(0)
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

MobileWorldRenderer::~MobileWorldRenderer()
{
    // release postprocess buffers
    //ReleaseIntermediateBuffer(m_pSceneDepthBuffer);
    //ReleaseIntermediateBuffer(m_pSceneColorBuffer);

    // delete cached shadow maps
    for (uint32 i = 0; i < m_directionalShadowMaps.GetSize(); i++)
        m_pDirectionalShadowMapRenderer->FreeShadowMap(&m_directionalShadowMaps[i]);
    m_directionalShadowMaps.Clear();
    for (uint32 i = 0; i < m_spotShadowMaps.GetSize(); i++)
        m_pSpotShadowMapRenderer->FreeShadowMap(&m_spotShadowMaps[i]);
    m_spotShadowMaps.Clear();

    // delete shadow map renderers
    delete m_pDirectionalShadowMapRenderer;
    delete m_pSpotShadowMapRenderer;
}

bool MobileWorldRenderer::Initialize()
{
    if (!WorldRenderer::Initialize())
        return false;

    // load shaders
    if ((m_pFinalCompositeShader = GetShaderProgram(OBJECT_TYPEINFO(MobileFinalCompositeShader), 0)) == nullptr)
        return false;

    // create buffers
    //m_pSceneColorBuffer = RequestIntermediateBuffer(m_options.RenderWidth, m_options.RenderHeight, SCENE_COLOR_PIXEL_FORMAT, 1);
    //m_pSceneDepthBuffer = RequestIntermediateBuffer(m_options.RenderWidth, m_options.RenderHeight, SCENE_DEPTH_PIXEL_FORMAT, 1);
    //if (m_pSceneColorBuffer == nullptr || m_pSceneDepthBuffer == nullptr)
        //return false;

    // create shadow map renderers
    if (m_options.EnableShadows)
    {
        m_pDirectionalShadowMapRenderer = new DirectionalShadowMapRenderer(m_options.DirectionalShadowMapResolution, m_options.ShadowMapPixelFormat);
        m_pSpotShadowMapRenderer = new SpotShadowMapRenderer(m_options.SpotShadowMapResolution, m_options.ShadowMapPixelFormat);
    }

    // all done
    return true;
}

void MobileWorldRenderer::DrawWorld(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, GPURenderTargetView *pRenderTargetView, GPUDepthStencilBufferView *pDepthStencilBufferView)
{
    MICROPROFILE_SCOPEI("MobileWorldRenderer", "DrawWorld", MICROPROFILE_COLOR(200, 100, 0));

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
        MICROPROFILE_SCOPEI("MobileWorldRenderer", "ClearTargets", MICROPROFILE_COLOR(200, 100, 20));

        // need a seperate viewport
        RENDERER_VIEWPORT bufferViewport(0, 0, m_options.RenderWidth, m_options.RenderHeight, 0.0f, 1.0f);
        m_pGPUContext->SetRenderTargets(1, &pRenderTargetView, pDepthStencilBufferView);
        m_pGPUContext->SetViewport(&bufferViewport);
        m_pGPUContext->ClearTargets(true, true, true, pViewParameters->FogColor);
    }

    // set constants
    {
        MICROPROFILE_SCOPEI("MobileWorldRenderer", "SetConstants", MICROPROFILE_COLOR(200, 100, 20));

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

    // tonemap to present buffer
    //MICROPROFILE_SCOPEI("MobileWorldRenderer", "DrawFinalPass", MAKE_COLOR_R8G8B8_UNORM(200, 100, 20));
    //DrawFinalPass(pViewParameters, pRenderTargetView);

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

void MobileWorldRenderer::OnFrameComplete()
{
    WorldRenderer::OnFrameComplete();

    // release all shadow maps
    for (uint32 i = 0; i < m_directionalShadowMaps.GetSize(); i++)
        m_directionalShadowMaps[i].IsActive = false;
    for (uint32 i = 0; i < m_spotShadowMaps.GetSize(); i++)
        m_spotShadowMaps[i].IsActive = false;

    // clear last indices
    m_lastDirectionalLightIndex = 0xFFFFFFFF;
    m_lastPointLightIndex = 0xFFFFFFFF;
    m_lastSpotLightIndex = 0xFFFFFFFF;
    m_lastVolumetricLightIndex = 0xFFFFFFFF;
}

void MobileWorldRenderer::DrawShadowMaps(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("MobileWorldRenderer", "DrawShadowMaps", MICROPROFILE_COLOR(200, 200, 200));

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

    // clear shaders/targets since the targets will be bound to inputs
    m_pGPUContext->ClearState(true, false, false, true);
}

bool MobileWorldRenderer::DrawDirectionalShadowMap(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight)
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
    m_pDirectionalShadowMapRenderer->DrawDirectionalShadowMap(m_pGPUContext, pShadowMapData, pRenderWorld, &pViewParameters->ViewCamera, pViewParameters->MaximumShadowViewDistance, pLight);

    // add to buffer list
    AddDebugBufferView(pShadowMapData->pShadowMapTexture, "DirectionalShadowMap");

    // ok
    return true;
}

bool MobileWorldRenderer::DrawSpotShadowMap(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, RENDER_QUEUE_SPOT_LIGHT_ENTRY *pLight)
{
    return false;
}

void MobileWorldRenderer::SetCommonShaderProgramParameters(const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, ShaderProgram *pShaderProgram)
{
    pQueueEntry->pMaterial->BindDeviceResources(m_pGPUContext, pShaderProgram);
    pQueueEntry->pRenderProxy->SetupForDraw(&pViewParameters->ViewCamera, pQueueEntry, m_pGPUContext, pShaderProgram);
}

uint32 MobileWorldRenderer::DrawBasePassForObject(const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, bool depthWrites, GPU_COMPARISON_FUNC depthFunc)
{
    const MaterialShader *pMaterialShader = pQueueEntry->pMaterial->GetShader();
    uint32 renderPassMask = pQueueEntry->RenderPassMask;

    // base pass flags
    uint32 basePassFlags = 0;

    // emissive
    if (renderPassMask & RENDER_PASS_EMISSIVE)
        basePassFlags |= MobileBasePassShader::WITH_EMISSIVE;

    // lightmap
    else if (renderPassMask & RENDER_PASS_LIGHTMAP)
        basePassFlags |= MobileBasePassShader::WITH_LIGHTMAP;

    // nothing for base pass
    else
        return 0;

    // should have flags
    DebugAssert(basePassFlags != 0);

    // draw base pass
    ShaderProgram *pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(MobileBasePassShader), basePassFlags, pQueueEntry);
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

void MobileWorldRenderer::DrawEmptyPassForObject(const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry)
{
    const MaterialShader *pMaterialShader = pQueueEntry->pMaterial->GetShader();

    // get shader
    ShaderProgram *pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(MobileBasePassShader), 0, pQueueEntry);
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

uint32 MobileWorldRenderer::DrawForwardLightPassesForObject(const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, bool useAdditiveBlending, bool depthWrites, GPU_COMPARISON_FUNC depthFunc)
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
            directionalLightShaderFlags &= ~MobileDirectionalLightShader::SHADOW_BITS_MASK;

        // get shader
        if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(MobileDirectionalLightShader), directionalLightShaderFlags, pQueueEntry)) == nullptr)
            continue;

        // bind shader and set parameters
        m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());

        // using shadow map? note: we need the shadow map data to set the constant buffer up, even if this object does not use it
        const DirectionalShadowMapRenderer::ShadowMapData *pShadowMapData = (pLight->ShadowMapIndex >= 0) ? &m_directionalShadowMaps[pLight->ShadowMapIndex] : nullptr;

        // set constant buffer parameters
        if (m_lastDirectionalLightIndex != lightIndex)
        {
            MobileDirectionalLightShader::SetLightParameters(m_pGPUContext, pLight, pShadowMapData);
            m_lastDirectionalLightIndex = lightIndex;
        }

        // and the shadow map textures
        if (pShadowMapData)
            MobileDirectionalLightShader::SetProgramParameters(m_pGPUContext, pShaderProgram, pLight, pShadowMapData);

        // draw light
        DRAW_LIGHT();
    }

#if 0
    // draw point lights
    RenderQueue::PointLightArray &pointLights = m_renderQueue.GetPointLightArray();
    const RENDER_QUEUE_POINT_LIGHT_ENTRY *queuedLights[MobilePointLightListShader::MAX_LIGHTS];
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
            if (queuedLightCount == MobilePointLightListShader::MAX_LIGHTS)
            {
                if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(PointLightListShader), 0, pQueueEntry)) != nullptr)
                {
                    // bind shader
                    m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());

                    // initialize light pointers
                    for (uint32 queuedLightIndex = 0; queuedLightIndex < queuedLightCount; queuedLightIndex++)
                        MobilePointLightListShader::SetLightParameters(m_pGPUContext, queuedLightIndex, queuedLights[queuedLightIndex]);

                    // commit parameters, and draw
                    MobilePointLightListShader::SetActiveLightCount(m_pGPUContext, queuedLightCount);
                    MobilePointLightListShader::CommitParameters(m_pGPUContext);
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
            pointLightShaderFlags &= ~MobilePointLightShader::SHADOW_BITS_MASK;

        // get shader
        if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(PointLightShader), pointLightShaderFlags, pQueueEntry)) == nullptr)
            continue;

        // bind shader
        m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());

        // using shadow map? note: we need the shadow map data to set the constant buffer up, even if this object does not use it
        const MobilePointShadowMapRenderer::ShadowMapData *pShadowMapData = (pLight->ShadowMapIndex >= 0) ? &m_pointShadowMaps[pLight->ShadowMapIndex] : nullptr;

        // set constant buffer parameters
        if (m_lastPointLightIndex != lightIndex)
        {
            MobilePointLightShader::SetLightParameters(m_pGPUContext, pLight, pShadowMapData);
            m_lastPointLightIndex = lightIndex;
        }
        
        // set program parameters
        MobilePointLightShader::SetProgramParameters(m_pGPUContext, pShaderProgram, pLight, pShadowMapData);

        // draw light
        DRAW_LIGHT();
    }

    // if there's only one remaining light, we can use the fast shader, otherwise use the multi-light shader
    if (queuedLightCount == 1)
    {
        if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(MobilePointLightShader), 0, pQueueEntry)) != nullptr)
        {
            m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());
            PointLightShader::SetLightParameters(m_pGPUContext, queuedLights[0], nullptr);
            DRAW_LIGHT();
        }
    }
    else if (queuedLightCount > 0)
    {
        if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(MobilePointLightListShader), 0, pQueueEntry)) != nullptr)
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

#endif      // 0

#undef DRAW_LIGHT
    
    return drawnLights;
}

void MobileWorldRenderer::DrawDepthPrepass(const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("MobileWorldRenderer", "DrawDepthPrepass", MICROPROFILE_COLOR(255, 100, 255));
    MICROPROFILE_SCOPEGPUI("DrawDepthPrepass", MICROPROFILE_COLOR(255, 100, 255));

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
            pQueueEntry->pRenderProxy->SetupForDraw(&pViewParameters->ViewCamera, pQueueEntry, m_pGPUContext, pShaderProgram);
            pQueueEntry->pRenderProxy->DrawQueueEntry(&pViewParameters->ViewCamera, pQueueEntry, m_pGPUContext);
        }
    }
}

void MobileWorldRenderer::DrawOpaqueObjects(const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("MobileWorldRenderer", "DrawOpaqueObjects", MICROPROFILE_COLOR(0, 200, 0));
    MICROPROFILE_SCOPEGPUI("DrawOpaqueObjects", MICROPROFILE_COLOR(0, 200, 0));

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

void MobileWorldRenderer::DrawTranslucentObjects(const ViewParameters *pViewParameters)
{
    MICROPROFILE_SCOPEI("MobileWorldRenderer", "DrawTranslucentObjects", MICROPROFILE_COLOR(0, 0, 200));
    MICROPROFILE_SCOPEGPUI("DrawTranslucentObjects", MICROPROFILE_COLOR(0, 0, 200));

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

void MobileWorldRenderer::DrawFinalPass(const ViewParameters *pViewParameters, GPURenderTargetView *pOutputRTV)
{
    MICROPROFILE_SCOPEGPUI("DrawFinalPass", MICROPROFILE_COLOR(0, 150, 0));

//     // @fixme
//     m_pGPUContext->SetRenderTargets(1, &pOutputRTV, nullptr);
//     m_pGPUContext->SetViewport(&pViewParameters->Viewport);
//     m_pGPUContext->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState());
//     m_pGPUContext->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false), 0);
//     m_pGPUContext->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateNoBlending());
// 
//     m_pGPUContext->SetShaderProgram(m_pFinalCompositeShader->GetGPUProgram());
//     MobileFinalCompositeShader::SetInputParameters(m_pGPUContext, m_pFinalCompositeShader, m_pSceneColorBuffer->pTexture);
//     g_pRenderer->DrawFullScreenQuad(m_pGPUContext);
}

