#pragma once
#include "Renderer/RendererTypes.h"
#include "Renderer/RenderQueue.h"

class Camera;
class RenderProfiler;
class RenderWorld;

class SSMShadowMapRenderer
{
public:
    struct ShadowMapData
    {
        bool IsActive;
        GPUTexture2D *pShadowMapTexture;
        GPUDepthStencilBufferView *pShadowMapDSV;
        float4x4 ViewProjectionMatrix;
    };

public:
    SSMShadowMapRenderer(uint32 shadowMapResolution = 256, PIXEL_FORMAT shadowMapFormat = PIXEL_FORMAT_D16_UNORM);
    virtual ~SSMShadowMapRenderer();

    const uint32 GetShadowMapResolution() const { return m_shadowMapResolution; }
    const PIXEL_FORMAT GetShadowMapFormat() const { return m_shadowMapFormat; }

    bool AllocateShadowMap(ShadowMapData *pShadowMapData);
    void FreeShadowMap(ShadowMapData *pShadowMapData);

    void DrawDirectionalShadowMap(GPUContext *pGPUContext, ShadowMapData *pShadowMapData, const RenderWorld *pRenderWorld, const Camera *pViewCamera, float shadowDistance, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight, RenderProfiler *pRenderProfiler);
    void DrawSpotShadowMap(GPUContext *pGPUContext, ShadowMapData *pShadowMapData, const RenderWorld *pRenderWorld, const Camera *pViewCamera, float shadowDistance, const RENDER_QUEUE_SPOT_LIGHT_ENTRY *pLight, RenderProfiler *pRenderProfiler);

private:
    uint32 m_shadowMapResolution;
    PIXEL_FORMAT m_shadowMapFormat;
    RenderQueue m_renderQueue;
};

