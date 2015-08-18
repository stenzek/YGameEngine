#pragma once
#include "Editor/Common.h"

class RenderWorld;
class SpriteRenderProxy;
class DirectionalLightRenderProxy;
class PointLightRenderProxy;

class EditorLightSimulator
{
    enum LightType
    {
        LightType_Directional,
        LightType_Point,
        LightType_Spot,
        LightType_Count,
    };

public:
    EditorLightSimulator(uint32 entityID, RenderWorld *pRenderWorld);
    ~EditorLightSimulator();

    // light type
    LightType GetLightType() const { return m_lightType; }
    void SetLightType(LightType type) { m_lightType = type; UpdateProxies(); UpdateIndicator(); }

    // indicator options
    const bool GetShowIndicator() const { return m_showIndicator; }
    void SetShowIndicator(bool enabled) { m_showIndicator = enabled; UpdateIndicator(); }

    // all light parameters
    const float3 &GetLightColor() const { return m_color; }
    const float GetLightBrightness() const { return m_brightness; }
    const bool GetCastShadows() const { return m_castShadows; }
    void SetLightColor(const float3 &color) { m_color = color; UpdateProxies(); }
    void SetLightBrightness(float brightness) { m_brightness = brightness; UpdateProxies(); }
    void SetCastShadows(bool castShadows) { m_castShadows = castShadows; UpdateProxies(); }

    // directional light parameters
    const float3 &GetDirectionalLightDirection() const { return m_directionalLightDirection; }
    const Quaternion &GetDirectionalLightRotation() const { return m_directionalLightRotation; }
    void SetDirectionalLightDirection(const float3 &direction) { m_directionalLightDirection = direction; m_directionalLightRotation = Quaternion::FromTwoVectors(float3::NegativeUnitZ, direction); UpdateProxies(); UpdateIndicator(); }
    void SetDirectionalLightRotation(const Quaternion &rotation) { m_directionalLightRotation = rotation; m_directionalLightDirection = rotation * float3::NegativeUnitZ; UpdateProxies(); UpdateIndicator(); }

    // point light parameters
    const float3 &GetPointLightLocation() const { return m_pointLightLocation; }
    const float GetPointLightRange() const { return m_pointLightRange; }
    const float GetPointLightFalloff() const { return m_pointLightFalloff; }
    void SetPointLightLocation(const float3 &location) { m_pointLightLocation = location; UpdateProxies(); UpdateIndicator(); }
    void SetPointLightRange(float range) { m_pointLightRange = range; UpdateProxies(); UpdateIndicator(); }
    void SetPointLightFalloff(float falloff) { m_pointLightFalloff = falloff; UpdateProxies(); }

    // manipulate from mouse movement
    void ManipulateFromMouseMovement(const int2 &mousePositionDelta);

private:
    // rebuild the indicator parameters
    void UpdateIndicator();

    // rebuild the light parameters
    void UpdateProxies();

    // current type
    LightType m_lightType;

    // world we're attached to
    RenderWorld *m_pRenderWorld;

    // sprite for rendering indicator
    bool m_showIndicator;
    SpriteRenderProxy *m_pIndicatorRenderProxy;

    // for all
    float3 m_color;
    float m_brightness;
    bool m_castShadows;

    // for directional + sun
    float3 m_directionalLightDirection;
    Quaternion m_directionalLightRotation;
    float m_directionalLightAmbientFactor;
    DirectionalLightRenderProxy *m_pDirectionalLightRenderProxy;

    // for point
    float3 m_pointLightLocation;
    float m_pointLightRange;
    float m_pointLightFalloff;
    PointLightRenderProxy *m_pPointLightRenderProxy;
};

