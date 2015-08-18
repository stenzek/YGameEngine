#pragma once
#include "Engine/Entity.h"

class DirectionalLightRenderProxy;

class DirectionalLightEntity : public Entity
{
    DECLARE_ENTITY_TYPEINFO(DirectionalLightEntity, Entity);
    DECLARE_ENTITY_GENERIC_FACTORY(DirectionalLightEntity);

public:
    DirectionalLightEntity(const EntityTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~DirectionalLightEntity();

    // property accessors
    const bool GetEnabled() const { return m_bEnabled; }
    const float3 &GetColor() const { return m_Color; }
    const float GetBrightness() const { return m_fBrightness; }
    const uint32 GetLightShadowFlags() const { return m_iLightShadowFlags; }
    const float GetAmbientFactor() const { return m_fAmbientFactor; }

    // property setters
    void SetEnabled(bool enabled);
    void SetColor(const float3 &color);
    void SetBrightness(float brightness);
    void SetLightShadowFlags(uint32 lightShadowFlags);
    void SetAmbientFactor(float ambientContribution);

    // creation method
    void Create(uint32 entityID, ENTITY_MOBILITY mobility = ENTITY_MOBILITY_MOVABLE, const Quaternion &rotation = Quaternion::Identity, const bool enabled = true, const float3 &color = float3::One, float brightness = 1.0f, uint32 shadowFlags = LIGHT_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS, float ambientFactor = 0.2f);

    // inherited methods
    virtual bool Initialize(uint32 entityID, const String &entityName) override;
    virtual void OnAddToWorld(World *pWorld) override;
    virtual void OnRemoveFromWorld(World *pWorld) override;
    virtual void OnTransformChange() override;

private:
    // updates light properties
    void UpdateColors();

    // determine light direction
    float3 CalculateLightDirection();

    // property callbacks
    static void OnEnabledPropertyChange(ThisClass *pEntity, const void *pUserData = NULL);
    static void OnColorOrBrightnessPropertyChange(ThisClass *pEntity, const void *pUserData = NULL);
    static void OnShadowPropertyChange(ThisClass *pEntity, const void *pUserData = NULL);

    // local copy of properties
    bool m_bEnabled;
    float3 m_Color;
    float m_fBrightness;
    float m_fAmbientFactor;
    uint32 m_iLightShadowFlags;

    // instance of render proxy
    DirectionalLightRenderProxy *m_pRenderProxy;
};

