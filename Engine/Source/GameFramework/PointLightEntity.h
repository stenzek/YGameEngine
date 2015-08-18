#include "Engine/Entity.h"

class PointLightRenderProxy;

class PointLightEntity : public Entity
{
    DECLARE_ENTITY_TYPEINFO(PointLightEntity, Entity);
    DECLARE_ENTITY_GENERIC_FACTORY(PointLightEntity);

public:
    PointLightEntity(const EntityTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~PointLightEntity();

    // property accessors
    const bool GetEnabled() const { return m_bEnabled; }
    const float &GetRange() const { return m_fRange; }
    const float3 &GetColor() const { return m_Color; }
    const float &GetBrightness() const { return m_fBrightness; }
    const uint32 GetLightShadowFlags() const { return m_iLightShadowFlags; }
    const float GetFalloffExponent() const { return m_fFalloffExponent; }

    // property setters
    void SetEnabled(bool enabled);
    void SetRange(float range);
    void SetColor(const float3 &color);
    void SetBrightness(float brightness);
    void SetLightShadowFlags(uint32 lightShadowFlags);
    void SetFalloffExponent(float falloffExponent);

    // inherited methods
    virtual bool Initialize(uint32 entityID, const String &entityName) override;
    virtual void OnAddToWorld(World *pWorld) override;
    virtual void OnRemoveFromWorld(World *pWorld) override;
    virtual void OnTransformChange() override;

private:
    // helper functions
    float3 CalculateLightColor();
    void UpdateBounds();

    // property callbacks
    static void OnEnabledPropertyChange(ThisClass *pEntity, const void *pUserData = NULL);
    static void OnRangePropertyChange(ThisClass *pEntity, const void *pUserData = NULL);
    static void OnColorOrBrightnessPropertyChange(ThisClass *pEntity, const void *pUserData = NULL);
    static void OnShadowPropertyChange(ThisClass *pEntity, const void *pUserData = NULL);
    static void OnFalloffExponentChange(ThisClass *pEntity, const void *pUserData = NULL);

    // local copy of properties
    bool m_bEnabled;
    float m_fRange;
    float3 m_Color;
    float m_fBrightness;
    uint32 m_iLightShadowFlags;
    float m_fFalloffExponent;

    // instance of render proxy
    PointLightRenderProxy *m_pRenderProxy;
};

