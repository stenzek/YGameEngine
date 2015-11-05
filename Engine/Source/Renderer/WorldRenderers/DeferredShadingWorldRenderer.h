#pragma once
#include "Renderer/WorldRenderers/CompositingWorldRenderer.h"
#include "Renderer/WorldRenderers/CSMShadowMapRenderer.h"
#include "Renderer/WorldRenderers/CubeMapShadowMapRenderer.h"
#include "Renderer/WorldRenderers/SSMShadowMapRenderer.h"
#include "Renderer/RenderQueue.h"
#include "Renderer/MiniGUIContext.h"
#include "Renderer/Shaders/DeferredShadingShaders.h"

class DeferredShadingWorldRenderer : public CompositingWorldRenderer
{
public:
    // aliased renderer types
    typedef CSMShadowMapRenderer DirectionalShadowMapRenderer;
    typedef CubeMapShadowMapRenderer PointShadowMapRenderer;
    typedef SSMShadowMapRenderer SpotShadowMapRenderer;

    // buffer formats
    const PIXEL_FORMAT DEPTHBUFFER_FORMAT = PIXEL_FORMAT_D24_UNORM_S8_UINT;     // Depth
    //const PIXEL_FORMAT DEPTHBUFFER_FORMAT = PIXEL_FORMAT_D32_FLOAT;             // Depth
    //const PIXEL_FORMAT GBUFFER0_FORMAT = PIXEL_FORMAT_R8G8B8A8_UNORM;           // Diffuse RGB, (Spec Factor, Spec Power / Metallic, Specular)
    //const PIXEL_FORMAT GBUFFER1_FORMAT = PIXEL_FORMAT_R8G8B8A8_UNORM;           // XY Normal, Normal Sign + Roughness, (Shadow Mask, Shading Model)
    //const PIXEL_FORMAT GBUFFER2_FORMAT = PIXEL_FORMAT_R8G8B8A8_UNORM;           // 
    const PIXEL_FORMAT GBUFFER0_FORMAT = PIXEL_FORMAT_R8G8B8A8_UNORM;           // Diffuse RGB, Shadow Mask
    const PIXEL_FORMAT GBUFFER1_FORMAT = PIXEL_FORMAT_R8G8B8A8_UNORM;           // XYZ Normal, Roughness
    const PIXEL_FORMAT GBUFFER2_FORMAT = PIXEL_FORMAT_R8G8B8A8_UNORM;           // Spec Factor / Metallic, Spec Power / Specular, Shading Model, TwoSided 
    const PIXEL_FORMAT AOBUFFER_FORMAT = PIXEL_FORMAT_R8_UNORM;                 // AO buffer
    const PIXEL_FORMAT LIGHTBUFFER_FORMAT = PIXEL_FORMAT_R11G11B10_FLOAT;      // Light accumulation buffer
    const PIXEL_FORMAT AUXBUFFER_FORMAT = PIXEL_FORMAT_R8G8B8A8_UNORM;
    const PIXEL_FORMAT LUMINANCEBUFFER_FORMAT = PIXEL_FORMAT_R32_FLOAT;

public:
    DeferredShadingWorldRenderer(GPUContext *pGPUContext, const Options *pOptions);
    virtual ~DeferredShadingWorldRenderer();

    virtual bool Initialize() override;

    virtual void DrawWorld(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, GPURenderTargetView *pRenderTargetView, GPUDepthStencilBufferView *pDepthStencilBufferView) override;
    virtual void GetRenderStats(RenderStats *pRenderStats) const override;
    virtual void OnFrameComplete() override;

private:
    // draw shadow maps from needed lights
    void DrawShadowMaps(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters);
    bool DrawDirectionalShadowMap(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight);
    bool DrawPointShadowMap(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight);
    bool DrawSpotShadowMap(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, RENDER_QUEUE_SPOT_LIGHT_ENTRY *pLight);

    // set shader program parameters for queue entry
    void SetCommonShaderProgramParameters(GPUCommandList *pCommandList, const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, ShaderProgram *pShaderProgram);

    // draw base pass for an object (emissive/lightmap/ambient light/main directional light)
    uint32 DrawBasePassForObject(GPUCommandList *pCommandList, const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, bool depthWrites, GPU_COMPARISON_FUNC depthFunc);

    // draw forward light passes for an object
    uint32 DrawForwardLightPassesForObject(GPUCommandList *pCommandList, const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, bool useAdditiveBlending, bool depthWrites, GPU_COMPARISON_FUNC depthFunc);

    // draw a clear pass to set depth for an object
    void DrawEmptyPassForObject(GPUCommandList *pCommandList, const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry);

    // draw z prepass (Overwrites render targets)
    void DrawDepthPrepass(GPUCommandList *pCommandList, const ViewParameters *pViewParameters);

    // draw emissive objects (Overwrites render targets)
    void DrawUnlitObjects(GPUCommandList *pCommandList, const ViewParameters *pViewParameters);

    // draw opaque objects (Overwrites render targets)
    void DrawGBuffers(GPUCommandList *pCommandList, const ViewParameters *pViewParameters);

    // draw lights (Overwrites render targets)
    void DrawLights(GPUCommandList *pCommandList, const ViewParameters *pViewParameters);
    void DrawLights_DirectionalLights(GPUCommandList *pCommandList, const ViewParameters *pViewParameters);
    void DrawLights_PointLights_ByLightVolumes(GPUCommandList *pCommandList, const ViewParameters *pViewParameters);
    void DrawLights_PointLights_Tiled(GPUCommandList *pCommandList, const ViewParameters *pViewParameters);

    // build ambient occlusion terms (Overwrites render targets)
    void ApplyAmbientOcclusion(GPUCommandList *pCommandList, const ViewParameters *pViewParameters);

    // apply fog post-process
    void ApplyFog(GPUCommandList *pCommandList, const ViewParameters *pViewParameters);

    // draw post process passes (Overwrites render targets)
    void DrawPostProcessAndTranslucentObjects(GPUCommandList *pCommandList, const ViewParameters *pViewParameters);

    // draw any object debug info
    void DrawDebugInfo(const Camera *pCamera);

    // shadow map renderers
    DirectionalShadowMapRenderer *m_pDirectionalShadowMapRenderer;
    PointShadowMapRenderer *m_pPointShadowMapRenderer;
    SpotShadowMapRenderer *m_pSpotShadowMapRenderer;

    // shadow map cache
    MemArray<DirectionalShadowMapRenderer::ShadowMapData *> m_directionalShadowMaps;
    MemArray<PointShadowMapRenderer::ShadowMapData *> m_pointShadowMaps;
    MemArray<SpotShadowMapRenderer::ShadowMapData *> m_spotShadowMaps;

    // last rendered light indices, used to avoid buffer updates when not needed
    uint32 m_lastDirectionalLightIndex;
    uint32 m_lastPointLightIndex;
    uint32 m_lastSpotLightIndex;
    uint32 m_lastVolumetricLightIndex;

    // shader flags
    uint32 m_directionalLightShaderFlags;
    uint32 m_pointLightShaderFlags;
    uint32 m_spotLightShaderFlags;

    // buffers
    IntermediateBuffer *m_pSceneColorBuffer;
    IntermediateBuffer *m_pSceneColorBufferCopy;
    IntermediateBuffer *m_pSceneDepthBuffer;
    IntermediateBuffer *m_pSceneDepthBufferCopy;
    IntermediateBuffer *m_pGBuffer0;
    IntermediateBuffer *m_pGBuffer1;
    IntermediateBuffer *m_pGBuffer2;

    // light volume vertex buffers
    GPUBuffer *m_pPointLightVolumeVertexBuffer;
    GPUBuffer *m_pSpotLightVolumeVertexBuffer;
    uint32 m_pointLightVolumeVertexCount;

    // tiled point light info
    MemArray<DeferredTiledPointLightShader::Light> m_tiledPointLights;

    // programs
    ShaderProgram *m_pSSAOProgram;
    ShaderProgram *m_pSSAOApplyProgram;
};

