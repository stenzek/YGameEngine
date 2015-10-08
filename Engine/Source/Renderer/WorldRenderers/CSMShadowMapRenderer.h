#pragma once
#include "Renderer/Renderer.h"
#include "Renderer/RenderQueue.h"

class Camera;
class RenderWorld;

class CSMShadowMapRenderer
{
public:
    // maximum number of splits per shadow map
    enum { MaxCascadeCount = 3 };

    // shadow map data structure
    struct ShadowMapData
    {
        bool IsActive;
        GPUTexture2DArray *pShadowMapTexture;
        GPUDepthStencilBufferView *pShadowMapDSV[MaxCascadeCount];

        uint32 CascadeCount;
        float4x4 ViewProjectionMatrices[MaxCascadeCount];
        float CascadeFrustumEyeSpaceDepths[MaxCascadeCount];
    };

public:
    CSMShadowMapRenderer(uint32 shadowMapResolution = 256, PIXEL_FORMAT shadowMapFormat = PIXEL_FORMAT_D16_UNORM, uint32 cascadeCount = 3, float splitLambda = 0.95f);
    virtual ~CSMShadowMapRenderer();

    // getters
    const uint32 GetShadowMapResolution() const { return m_shadowMapResolution; }
    const PIXEL_FORMAT GetShadowMapFormat() const { return m_shadowMapFormat; }
    const uint32 GetCascadeCount() const { return m_cascadeCount; }
    float GetSplitLambda() const { return m_splitLambda; }
    
    // allocator
    bool AllocateShadowMap(ShadowMapData *pShadowMapData);
    void FreeShadowMap(ShadowMapData *pShadowMapData);

    // drawer
    void DrawShadowMap(GPUCommandList *pCommandList, ShadowMapData *pShadowMapData, const Camera *pViewCamera, float shadowDistance, const RenderWorld *pRenderWorld, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight);

private:
    // calculate split depths, store in m_splitDepths
    void CalculateSplitDepths(const Camera *pViewCamera, float shadowDistance);

    // fill temporary variables
    void CalculateViewDependantVariables(const Camera *pViewCamera);

    // build the camera for this split
    void BuildCascadeCamera(Camera *pOutCascadeCamera, const Camera *pViewCamera, const float3 &lightDirection, uint32 splitIndex, uint32 splitCount, float lambda, float shadowDrawDistance, const RenderWorld *pRenderWorld);

    // draw using multipass technique
    void DrawMultiPass(GPUCommandList *pCommandList, ShadowMapData *pShadowMapData, const Camera *pViewCamera, float shadowDistance, const RenderWorld *pRenderWorld, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight);

    // in vars
    uint32 m_shadowMapResolution;
    PIXEL_FORMAT m_shadowMapFormat;
    uint32 m_cascadeCount;
    float m_splitLambda;

    // temp vars
    float m_splitDepths[MaxCascadeCount + 1];
    float3 m_frustumCornersVS[8];

    // render queue
    RenderQueue m_renderQueue;
};

