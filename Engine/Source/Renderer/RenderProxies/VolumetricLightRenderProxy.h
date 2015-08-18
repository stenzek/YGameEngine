#pragma once
#include "Renderer/RenderProxy.h"

class VolumetricLightRenderProxy : public RenderProxy
{
public:
    VolumetricLightRenderProxy(uint32 entityId,
                         bool enabled = true,
                         const float3 &color = float3::One,
                         const float3 &position = float3::Zero,
                         float falloffRate = 0.5f);

    ~VolumetricLightRenderProxy();

    // can be called at any time.
    void SetEnabled(bool newEnabled);
    void SetColor(const float3 &newColor);
    void SetPosition(const float3 &position);
    void SetFalloffRate(float falloffRate);
    void SetPrimitive(VOLUMETRIC_LIGHT_PRIMITIVE primitive);
    void SetBoxPrimitiveExtents(float3 boxExtents);
    void SetSpherePrimitiveRadius(float sphereRadius);

    // virtual methods
    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override;

protected:
    void UpdateBounds();

    bool m_enabled;
    float3 m_color;
    float3 m_position;
    float m_falloffRate;
    float m_renderRange;

    VOLUMETRIC_LIGHT_PRIMITIVE m_primitive;
    float3 m_boxPrimitiveExtents;
    float m_spherePrimitiveRadius;    
};

