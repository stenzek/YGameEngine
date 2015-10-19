#include "Renderer/PrecompiledHeader.h"
#include "Renderer/Shaders/OneColorShader.h"
#include "Renderer/WorldRenderers/ForwardShadingWorldRenderer.h"
#include "Renderer/WorldRenderers/DeferredShadingWorldRenderer.h"
#include "Renderer/WorldRenderers/DebugNormalsWorldRenderer.h"
#include "Renderer/WorldRenderers/FullBrightWorldRenderer.h"
#include "Renderer/WorldRenderers/MobileWorldRenderer.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/ShaderProgramSelector.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/WorldRenderer.h"
#include "Engine/Camera.h"
#include "Engine/EngineCVars.h"
#include "Engine/Material.h"
#include "Engine/Profiling.h"
Log_SetChannel(WorldRenderer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class OcclusionCullingShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(OcclusionCullingShader, ShaderComponent);

public:
    OcclusionCullingShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { };

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

DEFINE_SHADER_COMPONENT_INFO(OcclusionCullingShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(OcclusionCullingShader)
END_SHADER_COMPONENT_PARAMETERS()

bool OcclusionCullingShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr || pMaterialShader != nullptr)
        return false;

    return true;
}

bool OcclusionCullingShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/OcclusionCullingShader.hlsl", "VSMain");
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

WorldRenderer::Options::Options()
    : EnableDepthPrepass(false)
    , EnableShadows(false)
    , EnablePointLightShadows(false)
    , EnableHardwareShadowFiltering(false)
    , EnableOcclusionCulling(false)
    , EnableOcclusionPredication(false)
    , WaitForOcclusionResults(false)
    , EnablePostProcessing(false)
    , EnableSSAO(false)
    , EnableBloom(false)
    , ShowDebugInfo(false)
    , ShowShadowMapCascades(false)
    , ShowIntermediateBuffers(false)
    , ShowWireframeOverlay(false)
    , RenderModeFullbright(false)
    , RenderModeNormals(false)
    , RenderModeLightingOnly(false)
    , EmulateMobile(false)
    , EnableMultithreadedRendering(false)
    , RenderWidth(640)
    , RenderHeight(480)
    , OcclusionCullingObjectsPerBatch(1)
    , ShadowMapPixelFormat(PIXEL_FORMAT_D16_UNORM)
    , DirectionalShadowMapResolution(1)
    , PointShadowMapResolution(1)
    , SpotShadowMapResolution(1)
    , ShadowMapFiltering(RENDERER_SHADOW_FILTER_1X1)
    , ShadowMapCascadeCount(1)
{

}

WorldRenderer::Options::Options(const Options &options)
{
    Y_memcpy(this, &options, sizeof(*this));
}

void WorldRenderer::Options::InitFromCVars()
{
    // depth prepass
    EnableDepthPrepass = CVars::r_depth_prepass.GetBool();

    // occlusion culling
    EnableOcclusionCulling = CVars::r_occlusion_culling.GetBool();
    EnableOcclusionPredication = CVars::r_occlusion_prediction.GetBool();
    WaitForOcclusionResults = CVars::r_occlusion_culling_wait_for_results.GetBool();
    OcclusionCullingObjectsPerBatch = CVars::r_occlusion_culling_objects_per_buffer.GetUInt();

    // shadows enabled
    EnableShadows = (CVars::r_shadows.GetUInt() > 0);
    EnablePointLightShadows = EnableShadows && (CVars::r_shadows.GetUInt() != 2);
    EnableHardwareShadowFiltering = CVars::r_shadow_use_hardware_pcf.GetBool();

    // shadow map resolutions
    DirectionalShadowMapResolution = CVars::r_directional_shadow_map_resolution.GetUInt();
    PointShadowMapResolution = CVars::r_point_shadow_map_resolution.GetUInt();
    SpotShadowMapResolution = CVars::r_spot_shadow_map_resolution.GetUInt();

    // shadow map bits
    uint32 shadowMapBits = CVars::r_shadow_map_bits.GetUInt();
    ShadowMapPixelFormat = PIXEL_FORMAT_D16_UNORM;
    if (shadowMapBits == 24)
        ShadowMapPixelFormat = PIXEL_FORMAT_D24_UNORM_S8_UINT;
    else if (shadowMapBits == 32)
        ShadowMapPixelFormat = PIXEL_FORMAT_D32_FLOAT;
    else if (shadowMapBits != 16)
        Log_WarningPrintf("WorldRenderer::Options::InitFromCVars: Invalid shadow map bits: %u (must be 16, 24, 32)", shadowMapBits);

    // shadow map filtering
    uint32 shadowMapFiltering = CVars::r_shadow_filtering.GetUInt();
    ShadowMapFiltering = RENDERER_SHADOW_FILTER_1X1;
    if (shadowMapFiltering == 1)
        ShadowMapFiltering = RENDERER_SHADOW_FILTER_3X3;
    else if (shadowMapFiltering == 2)
        ShadowMapFiltering = RENDERER_SHADOW_FILTER_5X5;
    else if (shadowMapFiltering != 0)
        Log_WarningPrintf("WorldRenderer::Options::InitFromCVars: Invalid shadow map filtering value: %u (must be 0-2)", shadowMapFiltering);

    // shadow map cascade count for CSM
    ShadowMapCascadeCount = 3;

    // post processing
    EnablePostProcessing = CVars::r_post_processing.GetBool();
    EnableSSAO = CVars::r_ssao.GetBool();
    EnableBloom = true;

    // debug options
    ShowDebugInfo = CVars::r_show_debug_info.GetBool();
    ShowShadowMapCascades = CVars::r_show_cascades.GetBool();
    ShowIntermediateBuffers = CVars::r_show_buffers.GetBool();
    ShowWireframeOverlay = CVars::r_wireframe.GetBool();
    RenderModeFullbright = CVars::r_fullbright.GetBool();
    RenderModeNormals = CVars::r_debug_normals.GetBool();
    EmulateMobile = CVars::r_emulate_mobile.GetBool();
    EnableMultithreadedRendering = CVars::r_multithreaded_rendering.GetBool();
}

void WorldRenderer::Options::SetRenderResolution(uint32 width, uint32 height)
{
    DebugAssert(width > 0 && height > 0);
    RenderWidth = width;
    RenderHeight = height;
}

void WorldRenderer::Options::DisableUnsupportedFeatures()
{
    if (g_pRenderer->GetFeatureLevel() < RENDERER_FEATURE_LEVEL_ES3)
    {
        if (EnableOcclusionCulling || EnableOcclusionPredication)
        {
            Log_WarningPrintf("WorldRenderer::Options::DisableUnsupportedFeatures: Disabling occlusion culling.");
            EnableOcclusionCulling = false;
            EnableOcclusionPredication = false;
        }
    }

    EnableMultithreadedRendering &= g_pRenderer->GetCapabilities().SupportsCommandLists;
}

WorldRenderer::ViewParameters::ViewParameters()
    : ViewCamera()
    , MaximumShadowViewDistance(500.0f)
    , Viewport(0, 0, 1, 1, 0.0f, 1.0f)
    , WorldTime(0.0f)
    , FogMode(RENDERER_FOG_MODE_NONE)
    , FogStartDistance(10.0f)
    , FogEndDistance(20.0f)
    , FogDensity(1.0f)
    , FogColor(float3::Zero)
    , EnableManualExposure(true)
    , ManualExposure(1.0f)
    , MaximumExposure(4.0f)
    , EnableBloom(true)
    , BloomThreshold(0.3f)
    , BloomMagnitude(1.0f)
{

}

WorldRenderer::ViewParameters::ViewParameters(const ViewParameters &parameters)
    : ViewCamera(parameters.ViewCamera)
    , MaximumShadowViewDistance(500.0f)
    , Viewport(parameters.Viewport)
    , WorldTime(parameters.WorldTime)
    , FogMode(parameters.FogMode)
    , FogStartDistance(parameters.FogStartDistance)
    , FogEndDistance(parameters.FogEndDistance)
    , FogDensity(parameters.FogDensity)
    , FogColor(parameters.FogColor)
    , EnableManualExposure(parameters.EnableManualExposure)
    , ManualExposure(parameters.ManualExposure)
    , MaximumExposure(parameters.MaximumExposure)
    , EnableBloom(parameters.EnableBloom)
    , BloomThreshold(parameters.BloomThreshold)
    , BloomMagnitude(parameters.BloomMagnitude)
{

}

void WorldRenderer::ViewParameters::SetCamera(const Camera *pCamera)
{
    ViewCamera = *pCamera;
}

WorldRenderer::WorldRenderer(GPUContext *pGPUContext, const Options *pOptions)
    : m_pGPUContext(pGPUContext)
    , m_pGUIContext(nullptr)
    , m_options(*pOptions)
    , m_globalShaderFlags(0)
    , m_pOcclusionCullingCubeVertexBuffer(nullptr)
    , m_pOcclusionCullingCubeIndexBuffer(nullptr)
    , m_pOcclusionCullingProgram(nullptr)
    , m_hQueueingCommandsSemaphore(CreateSemaphore(nullptr, 0, Y_INT32_MAX, nullptr))
    , m_commandListsPending(0)
{
    // disable unusable features
    m_options.DisableUnsupportedFeatures();

    // work out global shader flags
    m_globalShaderFlags = SHADER_GLOBAL_FLAG_SHADER_QUALITY_HIGH;
}

WorldRenderer::~WorldRenderer()
{
    // free intermedidate buffers, assert that everything has been released
    DebugAssert(m_allIntermediateBuffers.GetSize() == m_freeIntermediateBuffers.GetSize());
    for (IntermediateBuffer *buffer : m_allIntermediateBuffers)
    {
        if (buffer->pDSV != nullptr)
            buffer->pDSV->Release();
        if (buffer->pRTV != nullptr)
            buffer->pRTV->Release();
        buffer->pTexture->Release();
        delete buffer;
    }
    m_freeIntermediateBuffers.Obliterate();
    m_allIntermediateBuffers.Obliterate();

    // free buffers
    SAFE_RELEASE(m_pOcclusionCullingCubeIndexBuffer);
    SAFE_RELEASE(m_pOcclusionCullingCubeVertexBuffer);
    for (uint32 i = 0; i < m_occlusionCullingQueryCacheArray.GetSize(); i++)
        m_occlusionCullingQueryCacheArray[i]->Release();
    m_occlusionCullingQueryCacheArray.Obliterate();
    m_occlusionCullingPendingQueries.Obliterate();
}

bool WorldRenderer::Initialize()
{
    // occlusion culling
    if ((m_options.EnableOcclusionCulling | m_options.EnableOcclusionPredication))
    {
        // render program
        m_pOcclusionCullingProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(OcclusionCullingShader), 0, g_pRenderer->GetFixedResources()->GetPositionOnlyVertexAttributes(), g_pRenderer->GetFixedResources()->GetPositionOnlyVertexAttributeCount(), nullptr, 0);
        if (m_pOcclusionCullingProgram == nullptr)
            return false;

        // vertex buffer
        DebugAssert(m_options.OcclusionCullingObjectsPerBatch > 0);
        GPU_BUFFER_DESC vertexBufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER | GPU_BUFFER_FLAG_MAPPABLE, sizeof(float3) * 8 * m_options.OcclusionCullingObjectsPerBatch);
        if ((m_pOcclusionCullingCubeVertexBuffer = g_pRenderer->CreateBuffer(&vertexBufferDesc)) == NULL)
            return false;

        // Indices for drawing a cube using AABox corner points.
        static const uint16 cubeIndices[36] = {
            0, 1, 2, 2, 1, 3,   // left
            4, 6, 5, 5, 6, 7,   // right
            0, 1, 5, 5, 0, 4,   // front
            3, 7, 2, 2, 7, 6,   // back
            1, 5, 3, 3, 5, 7,   // top
            0, 2, 4, 4, 2, 6    // bottom
        };

        // index buffer
        GPU_BUFFER_DESC indexBufferDesc(GPU_BUFFER_FLAG_BIND_INDEX_BUFFER, sizeof(cubeIndices));
        if ((m_pOcclusionCullingCubeIndexBuffer = g_pRenderer->CreateBuffer(&indexBufferDesc, cubeIndices)) == NULL)
            return false;
    }

    return true;
}

void WorldRenderer::DrawWorld(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, GPURenderTargetView *pRenderTargetView, GPUDepthStencilBufferView *pDepthStencilBufferView)
{

}

void WorldRenderer::GetRenderStats(RenderStats *pRenderStats) const
{
    pRenderStats->ObjectCount = m_renderQueue.GetOpaqueRenderableCount() + m_renderQueue.GetTranslucentRenderableCount() + m_renderQueue.GetPostProcessRenderableCount();
    pRenderStats->LightCount = m_renderQueue.GetDirectionalLightCount() + m_renderQueue.GetPointLightCount() + m_renderQueue.GetSpotLightCount() + m_renderQueue.GetVolumetricLightCount();
    pRenderStats->ShadowMapCount = 0;
    pRenderStats->ObjectsCulledByOcclusion = m_renderQueue.GetNumObjectsInvalidatedByOcclusion();
    pRenderStats->IntermediateBufferCount = m_allIntermediateBuffers.GetSize();
    pRenderStats->IntermediateBufferMemoryUsage = 0;

    for (const IntermediateBuffer *buffer : m_allIntermediateBuffers)
    {
        uint32 cpuMemoryUsage, gpuMemoryUsage;
        buffer->pTexture->GetMemoryUsage(&cpuMemoryUsage, &gpuMemoryUsage);
        pRenderStats->IntermediateBufferMemoryUsage += gpuMemoryUsage;
    }
}

void WorldRenderer::OnFrameComplete()
{
    // release autorelease buffers
    for (const DebugBufferView &dbv : m_debugBufferViews)
    {
        if (dbv.pBuffer != nullptr)
            ReleaseIntermediateBuffer(dbv.pBuffer);
    }
    m_debugBufferViews.Clear();
}

WorldRenderer *WorldRenderer::Create(GPUContext *pGPUContext, const Options *pCreationParameters)
{
    WorldRenderer *pWorldRenderer;

    if (pCreationParameters->RenderModeNormals)
    {
        // create normal renderer
        pWorldRenderer = new DebugNormalsWorldRenderer(pGPUContext, pCreationParameters);
    }
    else if (pCreationParameters->RenderModeFullbright)
    {
        // create fullbright renderer
        pWorldRenderer = new FullBrightWorldRenderer(pGPUContext, pCreationParameters);
    }
    else if (pCreationParameters->EmulateMobile || g_pRenderer->GetFeatureLevel() < RENDERER_FEATURE_LEVEL_SM4)
    {
        // use mobile renderer
        pWorldRenderer = new MobileWorldRenderer(pGPUContext, pCreationParameters);
    }
    else
    {
        // create standard forward shading renderer
        if (CVars::r_deferred_shading.GetBool())
            pWorldRenderer = new DeferredShadingWorldRenderer(pGPUContext, pCreationParameters);
        else
            pWorldRenderer = new ForwardShadingWorldRenderer(pGPUContext, pCreationParameters);
    }

    // create resources
    if (!pWorldRenderer->Initialize())
    {
        Log_ErrorPrint("WorldRenderer::CreateForCurrentRenderer: Initialize() failed");
        delete pWorldRenderer;
        return nullptr;
    }

    // return it
    return pWorldRenderer;
}

ShaderProgram *WorldRenderer::GetShaderProgram(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialStaticSwitchMask)
{
    // set shader quality
    globalShaderFlags |= SHADER_GLOBAL_FLAG_SHADER_QUALITY_HIGH;

    // pass through
    return g_pRenderer->GetShaderProgram(globalShaderFlags, pBaseShaderTypeInfo, baseShaderFlags, pVertexFactoryTypeInfo, vertexFactoryFlags, pMaterialShader, materialStaticSwitchMask);
}

ShaderProgram *WorldRenderer::GetShaderProgram(const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry)
{
    const Material *pMaterial = pQueueEntry->pMaterial;

    // add in global tint
    uint32 globalShaderFlags = 0;
    if (pQueueEntry->RenderPassMask & RENDER_PASS_TINT)
        globalShaderFlags |= SHADER_GLOBAL_FLAG_MATERIAL_TINT;

    return GetShaderProgram(globalShaderFlags, pBaseShaderTypeInfo, baseShaderFlags, pQueueEntry->pVertexFactoryTypeInfo, pQueueEntry->VertexFactoryFlags, pMaterial->GetShader(), pMaterial->GetShaderStaticSwitchMask());
}

ShaderProgram *WorldRenderer::GetShaderProgram(const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags)
{
    // set shader quality
    uint32 globalShaderFlags = SHADER_GLOBAL_FLAG_SHADER_QUALITY_HIGH;

    // pass through
    return g_pRenderer->GetShaderProgram(globalShaderFlags, pBaseShaderTypeInfo, baseShaderFlags, g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributes(), g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributeCount(), nullptr, 0);
}

void WorldRenderer::SetBlendingModeForMaterial(GPUCommandList *pCommandList, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry)
{
    switch (pQueueEntry->pMaterial->GetShader()->GetBlendMode())
    {
    case MATERIAL_BLENDING_MODE_ADDITIVE:
        pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAdditive(), float4::One);
        break;

    case MATERIAL_BLENDING_MODE_STRAIGHT:
        pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAlphaBlending(), float4::One);
        break;

    case MATERIAL_BLENDING_MODE_PREMULTIPLIED:
        pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStatePremultipliedAlpha(), float4::One);
        break;

    case MATERIAL_BLENDING_MODE_SOFTMASKED:
        pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAlphaBlending(), float4::One);
        break;

    default:
        {
            if ((pQueueEntry->RenderPassMask & RENDER_PASS_TINT) && (pQueueEntry->TintColor >> 24) != 0xFF)
                pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAlphaBlending(), float4::One);
            else
                pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateNoBlending(), float4::One);
        }
        break;
    }

    // set blending colour
    if (pQueueEntry->RenderPassMask & RENDER_PASS_TINT)
        pCommandList->GetConstants()->SetMaterialTintColor(PixelFormatHelpers::ConvertRGBAToFloat4(pQueueEntry->TintColor), true);
}

void WorldRenderer::SetAdditiveBlendingModeForMaterial(GPUCommandList *pCommandList, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry)
{
    switch (pQueueEntry->pMaterial->GetShader()->GetBlendMode())
    {
    case MATERIAL_BLENDING_MODE_ADDITIVE:
        pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAdditive(), float4::One);
        break;

    case MATERIAL_BLENDING_MODE_STRAIGHT:
        pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAlphaBlendingAdditive(), float4::One);
        break;

    case MATERIAL_BLENDING_MODE_PREMULTIPLIED:
        pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStatePremultipliedAlphaAdditive(), float4::One);
        break;

    case MATERIAL_BLENDING_MODE_SOFTMASKED:
        pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAlphaBlendingAdditive(), float4::One);
        break;

    default:
        {
            if ((pQueueEntry->RenderPassMask & RENDER_PASS_TINT) && (pQueueEntry->TintColor >> 24) != 0xFF)
                pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAlphaBlendingAdditive(), float4::One);
            else
                pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAdditive(), float4::One);
        }
        break;
    }

    // set blending colour
    if (pQueueEntry->RenderPassMask & RENDER_PASS_TINT)
        pCommandList->GetConstants()->SetMaterialTintColor(PixelFormatHelpers::ConvertRGBAToFloat4(pQueueEntry->TintColor), true);
}

void WorldRenderer::FillRenderQueue(const Camera *pCamera, const RenderWorld *pRenderWorld)
{
    MICROPROFILE_SCOPEI("WorldRenderer", "FillRenderQueue", MICROPROFILE_COLOR(0, 255, 255));

    // clear render queue
    m_renderQueue.Clear();

    // find renderables
    pRenderWorld->EnumerateRenderablesInFrustum(pCamera->GetFrustum(), [this, pCamera](const RenderProxy *pRenderProxy)
    {
        // add to render queue
        pRenderProxy->QueueForRender(pCamera, &m_renderQueue);
    });

    // sort render queue
    m_renderQueue.Sort();
}

void WorldRenderer::DrawDebugInfo(const Camera *pCamera)
{
    MICROPROFILE_SCOPEI("WorldRenderer", "DrawDebugInfo", MICROPROFILE_COLOR(185, 20, 185));
    
    // batch batch batch!
    SmallString tempString;
    m_pGUIContext->PushManualFlush();

    // draw infos
    const RenderQueue::DebugDrawRenderableArray &debugInfoObjects = m_renderQueue.GetDebugObjects();
    for (uint32 i = 0; i < debugInfoObjects.GetSize(); i++)
        debugInfoObjects[i]->DrawDebugInfo(pCamera, m_pGPUContext, m_pGUIContext);

//     // override cameras
//     if (pRenderProfiler != nullptr && pRenderProfiler->GetCameraCount() > 0)
//     {
//         tempString.Format("Total camera count: %u, camera override: %i", pRenderProfiler->GetCameraCount(), pRenderProfiler->GetCameraOverrideIndex());
//         m_pGUIContext->DrawText(g_pRenderer->GetFixedResources()->GetDebugFont(), 16, 16, 4, tempString, MAKE_COLOR_R8G8B8_UNORM(255, 255, 255));
// 
//         for (uint32 i = 0; i < pRenderProfiler->GetCameraCount(); i++)
//         {
//             tempString.Format("Camera %u: '%s' at (%s)", i, pRenderProfiler->GetCameraName(i).GetCharArray(), StringConverter::Vector3fToString(pRenderProfiler->GetCamera(i)->GetPosition()).GetCharArray());
//             m_pGUIContext->DrawText(g_pRenderer->GetFixedResources()->GetDebugFont(), 16, 20, i * 16 + 20, tempString, MAKE_COLOR_R8G8B8_UNORM(255, 255, 255));
//         }
//     }
    
    m_pGUIContext->PopManualFlush();
}

void WorldRenderer::DrawWireframeOverlay(GPUCommandList *pCommandList, const Camera *pCamera, const RenderQueue::RenderableArray *pRenderables)
{
    MICROPROFILE_SCOPEI("WorldRenderer", "DrawWireframeOverlay", MICROPROFILE_COLOR(20, 185, 185));

    // set common state
    pCommandList->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_WIREFRAME, RENDERER_CULL_BACK, true, false, false));
    pCommandList->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(true, false, GPU_COMPARISON_FUNC_LESS_EQUAL), 0);
    pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateNoBlending());

    // init shader selector, we want one color shader
    ShaderProgramSelector shaderSelector(m_globalShaderFlags);
    shaderSelector.SetBaseShader(OBJECT_TYPEINFO(OneColorShader), 0);

    // draw away
    const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = pRenderables->GetBasePointer();
    const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = pQueueEntry + pRenderables->GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        float4 drawColor(float4::Zero);
        if (pQueueEntry->RenderPassMask & RENDER_PASS_EMISSIVE)
            drawColor.r += 0.5f;
        if (pQueueEntry->RenderPassMask & RENDER_PASS_LIGHTMAP)
            drawColor.g += 0.5f;
        if (pQueueEntry->RenderPassMask & RENDER_PASS_STATIC_LIGHTING)
            drawColor.r += 0.5f;
        if (pQueueEntry->RenderPassMask & RENDER_PASS_DYNAMIC_LIGHTING)
            drawColor.b += 0.5f;
        if (pQueueEntry->RenderPassMask & RENDER_PASS_SHADOWED_LIGHTING)
            drawColor.b += 0.5f;
        if (pQueueEntry->RenderPassMask & RENDER_PASS_OCCLUSION_CULLING_PROXY)
            drawColor.g += 0.5f;

        // skip anything that wouldn't get a colour
        if (drawColor == float4::Zero)
            continue;

        // mark translucent materials
        if (pQueueEntry->pMaterial->GetShader()->GetBlendMode() != MATERIAL_BLENDING_MODE_NONE)
            drawColor.r += 0.5f;

        // update selector, we set this manually so that tinting is not applied
        shaderSelector.SetVertexFactory(pQueueEntry->pVertexFactoryTypeInfo, pQueueEntry->VertexFactoryFlags);
        shaderSelector.SetMaterial(pQueueEntry->pMaterial);

        // draw it
        ShaderProgram *pShaderProgram = shaderSelector.MakeActive(pCommandList);
        if (pShaderProgram == nullptr)
            continue;

        OneColorShader::SetColor(pCommandList, pShaderProgram, drawColor);
        pQueueEntry->pRenderProxy->SetupForDraw(pCamera, pQueueEntry, pCommandList, pShaderProgram);
        pQueueEntry->pRenderProxy->DrawQueueEntry(pCamera, pQueueEntry, pCommandList);
    }
}

void WorldRenderer::DrawOcclusionCullingProxies(GPUCommandList *pCommandList, const Camera *pCamera)
{
    MICROPROFILE_SCOPEI("WorldRenderer", "DrawOcclusionCullingProxies", MICROPROFILE_COLOR(20, 20, 185));
    Panic("Fixme for map!");

    // Everything here uses the normal rasterizer state, so set that up here.
    pCommandList->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_NONE));
    pCommandList->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(true, false, GPU_COMPARISON_FUNC_LESS_EQUAL), 0);
    pCommandList->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateNoColorWrites());

    // Set identity world matrix.
    pCommandList->GetConstants()->SetLocalToWorldMatrix(float4x4::Identity, true);

    // bind stuff
    pCommandList->SetShaderProgram(m_pOcclusionCullingProgram->GetGPUProgram());

    // Bind buffers
    uint32 bufferOffset = 0;
    uint32 bufferStride = sizeof(float3);
    pCommandList->SetVertexBuffers(0, 1, &m_pOcclusionCullingCubeVertexBuffer, &bufferOffset, &bufferStride);
    pCommandList->SetIndexBuffer(m_pOcclusionCullingCubeIndexBuffer, GPU_INDEX_FORMAT_UINT16, 0);
    pCommandList->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);

    // Get camera eye position.
    void *pBufferMappedPointer = nullptr;
    float3 *pBufferCurrentPointer = nullptr;
    uint32 nOccludersInBuffer = 0;

    // Get pointers.
    // We don't cull translucent materials.
    RENDER_QUEUE_OCCLUDER_ENTRY *pQueueEntry = m_renderQueue.GetOccluders().GetBasePointer();
    RENDER_QUEUE_OCCLUDER_ENTRY *pQueueEntryEnd = m_renderQueue.GetOccluders().GetBasePointer() + m_renderQueue.GetOccluders().GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        // get aabox. if we are located inside the frustum, skip culling process.
        if (pQueueEntry->BoundingBox.ContainsPoint(pCamera->GetPosition()))
            continue;

        // use cached query?
        GPUQuery *pOcclusionQuery;
        if (m_occlusionCullingPendingQueries.GetSize() < m_occlusionCullingQueryCacheArray.GetSize())
        {
            // use the last cached one
            pOcclusionQuery = m_occlusionCullingQueryCacheArray[m_occlusionCullingPendingQueries.GetSize()];
        }
        else
        {
            // create a new one
            pOcclusionQuery = g_pRenderer->CreateQuery(GPU_QUERY_TYPE_OCCLUSION);
            if (pOcclusionQuery == NULL)
            {
                // chances are it'll fail to create others too, so bail out now
                Log_WarningPrint("WorldRenderer::DrawOcclusionCullingProxies: Could not allocate query object");
                break;
            }

            // store it for tracking and later use
            m_occlusionCullingQueryCacheArray.Add(pOcclusionQuery);
        }

        // map buffer if not mapped
        if (pBufferCurrentPointer == NULL)
        {
            if (!m_pGPUContext->MapBuffer(m_pOcclusionCullingCubeVertexBuffer, GPU_MAP_TYPE_WRITE_DISCARD, &pBufferMappedPointer))
                return;

            pBufferCurrentPointer = reinterpret_cast<float3 *>(pBufferMappedPointer);
        }

        // get vertices, and write them to the vb
        float3 cubeVertices[8];
        pQueueEntry->BoundingBox.GetCornerPoints(cubeVertices);
        Y_memcpy(pBufferCurrentPointer, cubeVertices, sizeof(float3) * 8);
        pBufferCurrentPointer += 8;
        nOccludersInBuffer++;

        // add to the array
        PendingOcclusionCullingQuery pendingQuery;
        pendingQuery.pRenderProxy = pQueueEntry->pRenderProxy;
        pendingQuery.MatchUserData = pQueueEntry->MatchUserData;
        if (pQueueEntry->MatchUserData)
        {
            Y_memcpy(pendingQuery.UserData, pQueueEntry->UserData, sizeof(pendingQuery.UserData));
            Y_memcpy(pendingQuery.UserDataPointer, pQueueEntry->UserDataPointer, sizeof(pendingQuery.UserDataPointer));
        }
        pendingQuery.pQuery = pOcclusionQuery;
        pendingQuery.Visible = true;
        m_occlusionCullingPendingQueries.Add(pendingQuery);

        // requires a flush?
        if (nOccludersInBuffer == m_options.OcclusionCullingObjectsPerBatch)
        {
            // flush buffer
            m_pGPUContext->Unmapbuffer(m_pOcclusionCullingCubeVertexBuffer, pBufferMappedPointer);
            pBufferMappedPointer = nullptr;
            pBufferCurrentPointer = nullptr;

            // issue the queries, and draw the occluders
            uint32 baseIndex = m_occlusionCullingPendingQueries.GetSize() - nOccludersInBuffer;
            for (uint32 i = 0; i < nOccludersInBuffer; i++)
            {
                GPUQuery *pCurrentQuery = m_occlusionCullingPendingQueries[baseIndex + i].pQuery;
                
                // draw it
                pCommandList->BeginQuery(pCurrentQuery);
                pCommandList->DrawIndexed(0, 36, i * 8);
                pCommandList->EndQuery(pCurrentQuery);
            }

            // reset count
            nOccludersInBuffer = 0;
        }
    }

    // remaining occluders?
    if (nOccludersInBuffer > 0)
    {
        // flush buffer
        m_pGPUContext->Unmapbuffer(m_pOcclusionCullingCubeVertexBuffer, pBufferMappedPointer);
        pBufferMappedPointer = nullptr;
        pBufferCurrentPointer = nullptr;

        // issue the queries, and draw the occluders
        uint32 baseIndex = m_occlusionCullingPendingQueries.GetSize() - nOccludersInBuffer;
        for (uint32 i = 0; i < nOccludersInBuffer; i++)
        {
            GPUQuery *pCurrentQuery = m_occlusionCullingPendingQueries[baseIndex + i].pQuery;

            // draw it
            pCommandList->BeginQuery(pCurrentQuery);
            pCommandList->DrawIndexed(0, 36, i * 8);
            pCommandList->EndQuery(pCurrentQuery);
        }
    }
}

void WorldRenderer::CollectOcclusionCullingResults()
{
    static const uint32 keepRenderPasses = RENDER_PASS_OCCLUSION_CULLING_PROXY;
    MICROPROFILE_SCOPEI("WorldRenderer", "CollectOcclusionCullingResults", MICROPROFILE_COLOR(20, 50, 120));

    uint32 queryFlags = 0;
    bool waitForResults = CVars::r_occlusion_culling_wait_for_results.GetBool();
    if (!waitForResults)
        queryFlags |= GPU_QUERY_GETDATA_FLAG_NOFLUSH;

    // get data results
    for (uint32 i = 0; i < m_occlusionCullingPendingQueries.GetSize(); i++)
    {
        PendingOcclusionCullingQuery *pPendingQuery = &m_occlusionCullingPendingQueries[i];

        // get result
        for (;;)
        {
            bool isVisible;
            GPU_QUERY_GETDATA_RESULT result = m_pGPUContext->GetQueryData(pPendingQuery->pQuery, &isVisible, sizeof(isVisible), queryFlags);
            if (result == GPU_QUERY_GETDATA_RESULT_NOT_READY && waitForResults)
                continue;
            
            if (result == GPU_QUERY_GETDATA_RESULT_OK)
                pPendingQuery->Visible = isVisible;
            else
                pPendingQuery->Visible = true;

            break;
        }
    }

    // kill the render passes of any OPAQUE objects that's occluding
    for (uint32 i = 0; i < m_occlusionCullingPendingQueries.GetSize(); i++)
    {
        const PendingOcclusionCullingQuery *pPendingQuery = &m_occlusionCullingPendingQueries[i];
        if (!pPendingQuery->Visible)
        {
            if (!pPendingQuery->MatchUserData)
                m_renderQueue.InvalidateOpaqueRenderProxy(pPendingQuery->pRenderProxy);
            else
                m_renderQueue.InvalidateOpaqueRenderProxy(pPendingQuery->pRenderProxy, pPendingQuery->UserData, pPendingQuery->UserDataPointer);
        }
    }

    // clear the culled list
    m_occlusionCullingPendingQueries.Clear();
}

void WorldRenderer::BindOcclusionQueriesToQueueEntries()
{
    MICROPROFILE_SCOPEI("WorldRenderer", "BindOcclusionQueriesToQueueEntries", MICROPROFILE_COLOR(120, 50, 120));

    // kill the render passes of any OPAQUE objects that's occluding
    for (uint32 i = 0; i < m_occlusionCullingPendingQueries.GetSize(); i++)
    {
        const PendingOcclusionCullingQuery *pPendingQuery = &m_occlusionCullingPendingQueries[i];
        if (!pPendingQuery->MatchUserData)
            m_renderQueue.MarkRenderProxyWithPredicate(pPendingQuery->pRenderProxy, pPendingQuery->pQuery);
        else
            m_renderQueue.MarkRenderProxyWithPredicate(pPendingQuery->pRenderProxy, pPendingQuery->UserData, pPendingQuery->UserDataPointer, pPendingQuery->pQuery);
    }

    // clear the culled list
    m_occlusionCullingPendingQueries.Clear();
}

void WorldRenderer::ScaleTexture(GPUCommandList *pCommandList, GPUTexture2D *pSourceTexture, GPURenderTargetView *pDestinationRTV, bool restoreViewport /* = true */, bool restoreTargets /* = true */)
{
    DebugAssert(pDestinationRTV->GetTargetTexture()->GetResourceType() == GPU_RESOURCE_TYPE_TEXTURE2D);

    // old viewport
    RENDERER_VIEWPORT oldViewport;
    if (restoreViewport)
        Y_memcpy(&oldViewport, pCommandList->GetViewport(), sizeof(oldViewport));

    // old render targets
    GPURenderTargetView *pOldRTVs[8];
    GPUDepthStencilBufferView *pOldDSV;
    uint32 nOldRTVs;
    if (restoreTargets)
    {
        // read from context
        nOldRTVs = pCommandList->GetRenderTargets(countof(pOldRTVs), pOldRTVs, &pOldDSV);
    }
    else
    {
        // not strictly necessary, but spews warnings if not present
        nOldRTVs = 0;
        pOldDSV = nullptr;
    }

    // get source texture info
    uint32 sourceTextureWidth = pSourceTexture->GetDesc()->Width;
    uint32 sourceTextureHeight = pSourceTexture->GetDesc()->Height;

    // get destination texture
    GPUTexture2D *pDestinationTexture = static_cast<GPUTexture2D *>(pDestinationRTV->GetTargetTexture());
    uint32 destTextureWidth = pDestinationTexture->GetDesc()->Width;
    uint32 destTextureHeight = pDestinationTexture->GetDesc()->Height;

    // save the old viewport, and set a new viewport covering the destination texture
    RENDERER_VIEWPORT downsampleViewport(0, 0, destTextureWidth, destTextureHeight, 0.0f, 1.0f);
    pCommandList->SetViewport(&downsampleViewport);

    // use texture blit shader
    pCommandList->SetRenderTargets(1, &pDestinationRTV, nullptr);
    pCommandList->DiscardTargets(true, false, false);
    g_pRenderer->BlitTextureUsingShader(pCommandList, pSourceTexture, 0, 0, sourceTextureWidth, sourceTextureHeight, 0, 0, 0, destTextureWidth, destTextureHeight, RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_LINEAR, RENDERER_FRAMEBUFFER_BLIT_BLEND_MODE_NONE);

    // restore old targets
    if (restoreTargets)
        pCommandList->SetRenderTargets(nOldRTVs, pOldRTVs, pOldDSV);

    // restore old viewport
    if (restoreViewport)
        pCommandList->SetViewport(&oldViewport);
}

WorldRenderer::IntermediateBuffer *WorldRenderer::RequestIntermediateBuffer(uint32 width, uint32 height, PIXEL_FORMAT pixelFormat, uint32 mipLevels)
{
    // todo: binary sort the list
    for (uint32 i = 0; i < m_freeIntermediateBuffers.GetSize(); i++)
    {
        IntermediateBuffer *buffer = m_freeIntermediateBuffers[i];
        if (buffer->Width == width &&
            buffer->Height == height &&
            buffer->PixelFormat == pixelFormat &&
            buffer->MipLevels == mipLevels)
        {
            m_freeIntermediateBuffers.FastRemove(i);
            return buffer;
        }
    }

    // create new buffer
    IntermediateBuffer *buffer = new IntermediateBuffer();
    buffer->Width = width;
    buffer->Height = height;
    buffer->PixelFormat = pixelFormat;
    buffer->MipLevels = mipLevels;

    // figure out texture flags
    uint32 flags = GPU_TEXTURE_FLAG_SHADER_BINDABLE;
    if (PixelFormatHelpers::IsDepthFormat(pixelFormat))
        flags |= GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER;
    else
        flags |= GPU_TEXTURE_FLAG_BIND_RENDER_TARGET;
    if (mipLevels > 1)
        flags |= GPU_TEXTURE_FLAG_GENERATE_MIPS;

    // create texture descriptor
    GPU_TEXTURE2D_DESC textureDesc(width, height, pixelFormat, flags, mipLevels);
    GPU_SAMPLER_STATE_DESC pointSamplerDesc(TEXTURE_FILTER_MIN_MAG_MIP_POINT, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, float4::Zero, 0.0f, 0, 0, 1, GPU_COMPARISON_FUNC_NEVER);
    if ((buffer->pTexture = g_pRenderer->CreateTexture2D(&textureDesc, &pointSamplerDesc)) == nullptr)
    {
        // todo log
        delete buffer;
        return nullptr;
    }

    // create rtv
    if (flags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
    {
        GPU_RENDER_TARGET_VIEW_DESC rtvDesc((uint32)0, 0, 1, pixelFormat);
        if ((buffer->pRTV = g_pRenderer->CreateRenderTargetView(buffer->pTexture, &rtvDesc)) == nullptr)
        {
            buffer->pTexture->Release();
            delete buffer;
            return nullptr;
        }
        buffer->pDSV = nullptr;
    }
    else if (flags & GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER)
    {
        // create dsv
        GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC dsvDesc((uint32)0, 0, 1, pixelFormat);
        if ((buffer->pDSV = g_pRenderer->CreateDepthStencilBufferView(buffer->pTexture, &dsvDesc)) == nullptr)
        {
            buffer->pTexture->Release();
            delete buffer;
            return nullptr;
        }

        buffer->pRTV = nullptr;
    }
    else
    {
        buffer->pRTV = nullptr;
        buffer->pDSV = nullptr;
    }

    // done
    m_allIntermediateBuffers.Add(buffer);
    return buffer;
}

WorldRenderer::IntermediateBuffer *WorldRenderer::RequestIntermediateBufferMatching(const IntermediateBuffer *pIntermediateBuffer)
{
    // forward to main method
    return RequestIntermediateBuffer(pIntermediateBuffer->Width, pIntermediateBuffer->Height, pIntermediateBuffer->PixelFormat, pIntermediateBuffer->MipLevels);
}

void WorldRenderer::ReleaseIntermediateBuffer(IntermediateBuffer *pIntermediateBuffer)
{
    if (pIntermediateBuffer == nullptr)
        return;

    // append to free list
    DebugAssert(m_allIntermediateBuffers.Contains(pIntermediateBuffer));
    DebugAssert(!m_freeIntermediateBuffers.Contains(pIntermediateBuffer));
    m_freeIntermediateBuffers.Add(pIntermediateBuffer);
}

void WorldRenderer::AddDebugBufferView(GPUTexture *pTexture, const char *name)
{
    DebugAssert(pTexture->GetResourceType() == GPU_RESOURCE_TYPE_TEXTURE2D || pTexture->GetResourceType() == GPU_RESOURCE_TYPE_TEXTURE2DARRAY);
    if (!m_options.ShowIntermediateBuffers)
        return;

    DebugBufferView dbv;
    dbv.pTexture = pTexture;
    dbv.pBuffer = nullptr;
    dbv.Name = name;
    m_debugBufferViews.Add(dbv);
}

void WorldRenderer::AddDebugBufferView(IntermediateBuffer *pBuffer, const char *name, bool autoRelease)
{
    if (!m_options.ShowIntermediateBuffers)
    {
        if (autoRelease)
            ReleaseIntermediateBuffer(pBuffer);

        return;
    }

    DebugBufferView dbv;
    dbv.pTexture = pBuffer->pTexture;
    dbv.pBuffer = (autoRelease) ? pBuffer : nullptr;
    dbv.Name = name;
    m_debugBufferViews.Add(dbv);
}


void WorldRenderer::DrawIntermediateBuffers()
{
    const uint32 LEFT_MARGIN = 32;
    const uint32 RIGHT_MARGIN = 32;
    const uint32 TOP_MARGIN = 32;
    const uint32 BOTTOM_MARGIN = 32;
    uint32 PREVIEW_TEXTURE_WIDTH = 128;
    uint32 PREVIEW_TEXTURE_HEIGHT = 128;
    uint32 PREVIEW_TEXTURE_MARGIN = 8;
    MICROPROFILE_SCOPEI("WorldRenderer", "DrawIntermediateBuffers", MICROPROFILE_COLOR(120, 120, 45));

    // can't do much without gui context
    if (m_pGUIContext != nullptr)
    {
        // align to aspect ratio
        float aspect = (float)m_options.RenderWidth / (float)m_options.RenderHeight;
        PREVIEW_TEXTURE_HEIGHT = 128;
        PREVIEW_TEXTURE_WIDTH = Math::Truncate((float)PREVIEW_TEXTURE_WIDTH * aspect);

        // enable batching
        m_pGUIContext->PushManualFlush();

        // find the size of the grid we're going to need to draw
        uint32 freeSpaceX = m_options.RenderWidth - LEFT_MARGIN - RIGHT_MARGIN;
        uint32 freeSpaceY = m_options.RenderHeight - TOP_MARGIN - BOTTOM_MARGIN;
        uint32 gridColumns = freeSpaceX / (PREVIEW_TEXTURE_WIDTH + PREVIEW_TEXTURE_MARGIN);
        uint32 gridRows = freeSpaceY / (PREVIEW_TEXTURE_HEIGHT + PREVIEW_TEXTURE_MARGIN);
        if (gridColumns > 0 && gridRows > 0)
        {
            // allocate used grid items
            bool *usedGridItems = (bool *)alloca(sizeof(bool) * gridColumns * gridRows);
            Y_memzero(usedGridItems, sizeof(bool) * gridColumns * gridRows);

            // for each texture
            for (const DebugBufferView &dbv : m_debugBufferViews)
            {
                uint32 neededColumns = 1;
                if (dbv.pTexture->GetResourceType() == GPU_RESOURCE_TYPE_TEXTURE2DARRAY)
                    //neededColumns = static_cast<GPUTexture2DArray *>(dbv.pTexture)->GetDesc()->ArraySize;
                    continue;

                // find a free spot
                uint32 row = 0;
                uint32 column = 0;
                for (; row < gridRows; row++)
                {
                    for (column = 0; column < gridColumns; column++)
                    {
                        if ((column + neededColumns) > gridColumns)
                        {
                            column = gridColumns;
                            break;
                        }

                        uint32 idx = row * gridColumns + column;
                        if (usedGridItems[idx])
                            continue;

                        for (uint32 i = 0; i < neededColumns; i++)
                            usedGridItems[idx + i] = true;

                        break;
                    }

                    if (column != gridColumns)
                        break;
                }

                // found one?
                if (row != gridRows)
                {
                    uint32 startX = LEFT_MARGIN + (column * (PREVIEW_TEXTURE_WIDTH + PREVIEW_TEXTURE_MARGIN) + PREVIEW_TEXTURE_MARGIN);
                    uint32 endX = startX + ((PREVIEW_TEXTURE_WIDTH * neededColumns) - 1);
                    uint32 endY = m_options.RenderHeight - (row * (PREVIEW_TEXTURE_HEIGHT + PREVIEW_TEXTURE_MARGIN) + PREVIEW_TEXTURE_MARGIN);
                    uint32 startY = endY - (PREVIEW_TEXTURE_HEIGHT - 1);

                    MINIGUI_RECT rect(startX, endX, startY, endY);
                    MINIGUI_UV_RECT uvRect(0.0f, 1.0f, 0.0f, 1.0f);
                    m_pGUIContext->DrawTexturedRect(&rect, &uvRect, dbv.pTexture);
                    m_pGUIContext->DrawTextAt(startX + 4, startY + 4, g_pRenderer->GetFixedResources()->GetDebugFont(), 16, MICROPROFILE_COLOR(255, 255, 255), dbv.Name);
                }
            }
        }

        // end batching
        m_pGUIContext->PopManualFlush();
    }
}

void WorldRenderer::ExecuteRenderPasses()
{
    MICROPROFILE_SCOPEI("WorldRenderer", "ExecuteRenderPasses", MICROPROFILE_COLOR(50, 50, 200));
    if (!m_options.EnableMultithreadedRendering)
        return;

    m_commandListLock.Lock();

    for (;;)
    {
        if (m_commandListsPending == 0)
            break;

        m_commandListLock.Unlock();
        WaitForSingleObject(m_hQueueingCommandsSemaphore, INFINITE);
        m_commandListLock.Lock();

        // could be primary or secondary that woke us, so just execute as many secondaries as possible
        while (!m_readySecondaryCommandLists.IsEmpty())
        {
            GPUCommandList *pCommandList = m_readySecondaryCommandLists.PopFront();
            m_pGPUContext->ExecuteCommandList(pCommandList);
            g_pRenderer->ReleaseCommandList(pCommandList);
        }
    }

    // ensure the semaphore is zero
    while (WaitForSingleObject(m_hQueueingCommandsSemaphore, 0) == WAIT_OBJECT_0);

    // execute primary command lists
    while (!m_readyPrimaryCommandLists.IsEmpty())
    {
        GPUCommandList *pCommandList = m_readyPrimaryCommandLists.PopFront();
        m_pGPUContext->ExecuteCommandList(pCommandList);
        g_pRenderer->ReleaseCommandList(pCommandList);
    }

    m_commandListLock.Unlock();
}
