#pragma once
#include "Renderer/WorldRenderer.h"
#include "Renderer/WorldRenderers/SSMShadowMapRenderer.h"

class MobileWorldRenderer : public WorldRenderer
{
public:
    // aliased renderer types
    typedef SSMShadowMapRenderer DirectionalShadowMapRenderer;
    typedef SSMShadowMapRenderer SpotShadowMapRenderer;

    // render target formats
    static const PIXEL_FORMAT SCENE_COLOR_PIXEL_FORMAT = PIXEL_FORMAT_R8G8B8A8_UNORM;
    static const PIXEL_FORMAT SCENE_DEPTH_PIXEL_FORMAT = PIXEL_FORMAT_D16_UNORM;

public:
    MobileWorldRenderer(GPUContext *pGPUContext, const Options *pOptions);
    virtual ~MobileWorldRenderer();

    virtual bool Initialize() override;

    virtual void DrawWorld(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, GPURenderTargetView *pRenderTargetView, GPUDepthStencilBufferView *pDepthStencilBufferView, RenderProfiler *pRenderProfiler) override;
    virtual void OnFrameComplete() override;

private:
    // draw shadow maps from needed lights
    void DrawShadowMaps(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, RenderProfiler *pRenderProfiler);
    bool DrawDirectionalShadowMap(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight, RenderProfiler *pRenderProfiler);
    bool DrawSpotShadowMap(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, RENDER_QUEUE_SPOT_LIGHT_ENTRY *pLight, RenderProfiler *pRenderProfiler);

    // set shader program parameters for queue entry
    void SetCommonShaderProgramParameters(const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, ShaderProgram *pShaderProgram);

    // draw base pass for an object (emissive/lightmap/ambient light/main directional light)
    uint32 DrawBasePassForObject(const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, bool depthWrites, GPU_COMPARISON_FUNC depthFunc);

    // draw forward light passes for an object
    uint32 DrawForwardLightPassesForObject(const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, bool useAdditiveBlending, bool depthWrites, GPU_COMPARISON_FUNC depthFunc);

    // draw a clear pass to set depth for an object
    void DrawEmptyPassForObject(const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry);

    // draw z prepass
    void DrawDepthPrepass(const ViewParameters *pViewParameters);

    // draw opaque objects
    void DrawOpaqueObjects(const ViewParameters *pViewParameters);

    // draw lit/unlit translucent objects
    void DrawTranslucentObjects(const ViewParameters *pViewParameters);

    // finalize pass
    void DrawFinalPass(const ViewParameters *pViewParameters, GPURenderTargetView *pOutputRTV);

    // scene colour/depth textures
    //IntermediateBuffer *m_pSceneColorBuffer;
    //IntermediateBuffer *m_pSceneDepthBuffer;

    // shadow map renderers -- remove from heap
    DirectionalShadowMapRenderer *m_pDirectionalShadowMapRenderer;
    SpotShadowMapRenderer *m_pSpotShadowMapRenderer;

    // shadow map cache
    MemArray<DirectionalShadowMapRenderer::ShadowMapData> m_directionalShadowMaps;
    MemArray<SpotShadowMapRenderer::ShadowMapData> m_spotShadowMaps;

    // last rendered light indices, used to avoid buffer updates when not needed
    uint32 m_lastDirectionalLightIndex;
    uint32 m_lastPointLightIndex;
    uint32 m_lastSpotLightIndex;
    uint32 m_lastVolumetricLightIndex;

    // shader flags
    uint32 m_directionalLightShaderFlags;
    uint32 m_pointLightShaderFlags;
    uint32 m_spotLightShaderFlags;

    // composite shader
    ShaderProgram *m_pFinalCompositeShader;
};

