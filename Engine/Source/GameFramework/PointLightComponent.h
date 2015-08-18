#include "Engine/Component.h"

class PointLightRenderProxy;

class PointLightComponent : public Component
{
    DECLARE_COMPONENT_TYPEINFO(PointLightComponent, Component);
    DECLARE_COMPONENT_GENERIC_FACTORY(PointLightComponent);

public:
    PointLightComponent(const ComponentTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~PointLightComponent();

    // property accessors
    const bool GetEnabled() const { return m_enabled; }
    const float GetRange() const { return m_range; }
    const float3 &GetColor() const { return m_color; }
    const float GetBrightness() const { return m_brightness; }
    const uint32 GetShadowFlags() const { return m_shadowFlags; }
    const float GetFalloffExponent() const { return m_falloffExponent; }

    // property setters
    void SetEnabled(bool enabled);
    void SetRange(float range);
    void SetColor(const float3 &color);
    void SetBrightness(float brightness);
    void SetShadowFlags(uint32 lightShadowFlags);
    void SetFalloffExponent(float falloffExponent);

    // External creation method
    void Create(const float3 &localPosition = float3::Zero, bool enabled = true, float range = 8.0f, const float3 &color = float3::One, float brightness = 1.0f, uint32 shadowFlags = 0, float falloffExponent = 1.0f);

    // inherited methods
    virtual bool Initialize() override;
    virtual void OnAddToEntity(Entity *pEntity) override;
    virtual void OnRemoveFromEntity(Entity *pEntity) override;
    virtual void OnAddToWorld(World *pWorld) override;
    virtual void OnRemoveFromWorld(World *pWorld) override;
    virtual void OnLocalTransformChange() override;
    virtual void OnEntityTransformChange() override;

private:
    // helper functions
    float3 CalculateLightPosition() const;
    float3 CalculateLightColor() const;

    // property callbacks
    static void OnEnabledPropertyChange(ThisClass *pEntity, const void *pUserData = nullptr);
    static void OnRangePropertyChange(ThisClass *pEntity, const void *pUserData = nullptr);
    static void OnColorOrBrightnessPropertyChange(ThisClass *pEntity, const void *pUserData = nullptr);
    static void OnShadowPropertyChange(ThisClass *pEntity, const void *pUserData = nullptr);
    static void OnFalloffExponentChange(ThisClass *pEntity, const void *pUserData = nullptr);

    // local copy of properties
    bool m_enabled;
    float m_range;
    float3 m_color;
    float m_brightness;
    uint32 m_shadowFlags;
    float m_falloffExponent;

    // instance of render proxy
    PointLightRenderProxy *m_pRenderProxy;
};

