#pragma once
#include "Engine/Entity.h"

class DirectionalLightRenderProxy;

class SunLightEntity : public Entity
{
    DECLARE_ENTITY_TYPEINFO(SunLightEntity, Entity);
    DECLARE_ENTITY_GENERIC_FACTORY(SunLightEntity);

public:
    struct Step
    {
        float Hours;
        float Minutes;
        float3 Direction;
        uint32 Color;
        float Brightness;
        float AmbientTerm;
        float StartTime;
    };

public:
    SunLightEntity(const EntityTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~SunLightEntity();

    // steps
    void AddStep(float hours, float minutes, const float3 &direction, uint32 color, float brightness, float ambientTerm);
    void AddDefaultSteps();

    // time of day
    const float GetTimeOfDay() const { return m_timeOfDay; }
    const float GetTimeOfDayHours() const { return Math::Floor(m_timeOfDay / 3600.0f); }
    const float GetTimeOfDayMinutes() const { return Math::Floor(Y_fmodf(m_timeOfDay / 60.0f, 60.0f)); }
    const float GetTimeOfDaySeconds() const { return Y_fmodf(m_timeOfDay, 60.0f); }
    void SetTimeOfDay(float hours, float minutes, float seconds);
    void SetTimeOfDay(float secondInDay);

    // time scale
    const float GetTimeScale() const { return m_timeScale; }
    void SetTimeScale(float timeScale) { m_timeScale = timeScale; }

    // other
    void SetEnabled(bool enabled);
    void SetLightShadowFlags(uint32 lightShadowFlags);

    // creation method
    void Create(uint32 entityID, const bool enabled = true, uint32 shadowFlags = LIGHT_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS);

    // inherited methods
    virtual bool Initialize(uint32 entityID, const String &entityName) override;
    virtual void OnAddToWorld(World *pWorld) override;
    virtual void OnRemoveFromWorld(World *pWorld) override;
    virtual void OnTransformChange() override;
    virtual void Update(float timeSinceLastUpdate) override;

private:
    void SortSteps();

    // updates light properties
    void UpdateLight();

    // property callbacks
    static void OnEnabledPropertyChange(ThisClass *pEntity, const void *pUserData = NULL);
    static void OnShadowPropertyChange(ThisClass *pEntity, const void *pUserData = NULL);

    // local copy of properties
    bool m_bEnabled;
    uint32 m_iLightShadowFlags;

    // time of day
    float m_timeOfDay;
    float m_timeScale;

    // steps
    MemArray<Step> m_steps;

    // instance of render proxy
    DirectionalLightRenderProxy *m_pRenderProxy;
};

