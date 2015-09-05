#pragma once
#include "Renderer/Renderer.h"
#include "Renderer/RenderQueue.h"

class Camera;
class RenderWorld;

class CubeMapShadowMapRenderer
{
public:
    // shadow map data structure
    struct ShadowMapData
    {
        bool IsActive;
        GPUTextureCube *pShadowMapTexture;
        GPUDepthStencilBufferView *pShadowMapDSV[CUBE_FACE_COUNT];
    };

public:
    CubeMapShadowMapRenderer(uint32 shadowMapResolution = 256, PIXEL_FORMAT shadowMapFormat = PIXEL_FORMAT_D16_UNORM);
    virtual ~CubeMapShadowMapRenderer();

    const uint32 GetShadowMapResolution() const { return m_shadowMapResolution; }
    const PIXEL_FORMAT GetShadowMapFormat() const { return m_shadowMapFormat; }

    bool AllocateShadowMap(ShadowMapData *pShadowMapData);
    void FreeShadowMap(ShadowMapData *pShadowMapData);

    void DrawShadowMap(GPUContext *pGPUContext, ShadowMapData *pShadowMapData, const Camera *pViewCamera, float shadowDistance, const RenderWorld *pRenderWorld, const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight);

private:
    static void BuildCubeMapCamera(Camera *pCamera, const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight, CUBE_FACE face);

    uint32 m_shadowMapResolution;
    PIXEL_FORMAT m_shadowMapFormat;
    RenderQueue m_renderQueue;
};

