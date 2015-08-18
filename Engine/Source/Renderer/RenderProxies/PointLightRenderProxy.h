#pragma once
#include "Renderer/RenderProxy.h"

class PointLightRenderProxy : public RenderProxy
{
public:
    PointLightRenderProxy(uint32 entityId,
                          const float3 &position = float3::Zero,
                          const float range = 64.0f,
                          const float3 &color = float3::One,
                          uint32 shadowFlags = 0,
                          const float falloffExponent = 2.0f);

    ~PointLightRenderProxy();

    // can be called at any time.
    void SetEnabled(bool newEnabled);
    void SetPosition(const float3 &newPosition);
    void SetRange(const float newRange);
    void SetShadowFlags(uint32 newShadowFlags);
    void SetColor(const float3 &newColor);
    void SetFalloffExponent(const float newFalloffExponent);

    // virtual methods
    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override;

    // helper functions to calculate bounding volumes
    static AABox CalculatePointLightBoundingBox(const float3 &position, float range);
    static Sphere CalculatePointLightBoundingSphere(const float3 &position, float range);

protected:
    // update bounds
    void UpdateBounds();

    // read and written from render thread, no game thread access
    bool m_enabled;
    float3 m_position;
    float m_range;
    float3 m_color;
    uint32 m_shadowFlags;
    float m_falloffExponent;
};
