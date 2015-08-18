#pragma once
#include "Renderer/RenderProxy.h"

class DirectionalLightRenderProxy : public RenderProxy
{
public:
    DirectionalLightRenderProxy(uint32 entityId,
                                bool enabled = true,
                                const float3 &lightColor = float3::One,
                                const float3 &ambientColor = float3::Zero,
                                uint32 shadowFlags = 0,
                                const float3 &direction = float3::NegativeUnitZ);

    ~DirectionalLightRenderProxy();

    // can be called at any time.
    void SetEnabled(bool newEnabled);
    void SetLightColor(const float3 &newColor);
    void SetAmbientColor(const float3 &newColor);
    void SetShadowFlags(uint32 newShadowFlags);
    void SetDirection(const float3 &newDirection);

    // virtual methods
    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override;

protected:
    // read and write from render thread, no game thread access
    bool m_enabled;
    float3 m_lightColor;
    float3 m_ambientColor;
    uint32 m_shadowFlags;
    float3 m_direction;
};

