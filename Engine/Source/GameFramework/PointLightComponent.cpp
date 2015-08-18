#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/PointLightComponent.h"
#include "Engine/Entity.h"
#include "Engine/World.h"
#include "Renderer/RenderProxies/PointLightRenderProxy.h"
#include "Renderer/RenderWorld.h"

DEFINE_COMPONENT_TYPEINFO(PointLightComponent);
DEFINE_COMPONENT_GENERIC_FACTORY(PointLightComponent);
BEGIN_COMPONENT_PROPERTIES(PointLightComponent)
    PROPERTY_TABLE_MEMBER_BOOL("Enabled", 0, offsetof(PointLightComponent, m_enabled), OnEnabledPropertyChange, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("Range", 0, offsetof(PointLightComponent, m_range), OnRangePropertyChange, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT3("Color", 0, offsetof(PointLightComponent, m_color), OnColorOrBrightnessPropertyChange, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("Brightness", 0, offsetof(PointLightComponent, m_brightness), OnColorOrBrightnessPropertyChange, nullptr)
    PROPERTY_TABLE_MEMBER_UINT("ShadowFlags", 0, offsetof(PointLightComponent, m_shadowFlags), OnShadowPropertyChange, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("FalloffExponent", 0, offsetof(PointLightComponent, m_falloffExponent), OnFalloffExponentChange, nullptr)
END_COMPONENT_PROPERTIES()

PointLightComponent::PointLightComponent(const ComponentTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_enabled(true),
      m_range(8.0f),
      m_color(float3::One),
      m_brightness(1.0f),
      m_shadowFlags(0),
      m_falloffExponent(1.0f),
      m_pRenderProxy(nullptr)
{

}

PointLightComponent::~PointLightComponent()
{
    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->Release();
}

void PointLightComponent::SetEnabled(bool enabled)
{
    m_enabled = enabled;
    OnEnabledPropertyChange(this);
}

void PointLightComponent::SetRange(float range)
{
    m_range = range;
    OnRangePropertyChange(this);
}

void PointLightComponent::SetColor(const float3 &color)
{
    m_color = color;
    OnColorOrBrightnessPropertyChange(this);
}

void PointLightComponent::SetBrightness(float brightness)
{
    m_brightness = brightness;
    OnColorOrBrightnessPropertyChange(this);
}

void PointLightComponent::SetShadowFlags(uint32 lightShadowFlags)
{
    m_shadowFlags = lightShadowFlags;
    OnShadowPropertyChange(this);
}

void PointLightComponent::SetFalloffExponent(float falloffExponent)
{
    m_falloffExponent = falloffExponent;
    OnFalloffExponentChange(this);
}

void PointLightComponent::Create(const float3 &localPosition /*= float3::Zero*/, bool enabled /*= true*/, float range /*= 8.0f*/, const float3 &color /*= float3::One*/, float brightness /*= 1.0f*/, uint32 shadowFlags /*= 0*/, float falloffExponent /*= 1.0f*/)
{
    // easy enough
    m_localPosition = localPosition;
    m_enabled = enabled;
    m_range = range;
    m_color = color;
    m_brightness = brightness;
    m_shadowFlags = shadowFlags;
    m_falloffExponent = falloffExponent;
    Initialize();
}

bool PointLightComponent::Initialize()
{
    if (!BaseClass::Initialize())
        return false;

    // create the render proxy
    m_pRenderProxy = new PointLightRenderProxy(0, m_localPosition, m_range, m_color, m_shadowFlags, m_falloffExponent);
    return true;
}

void PointLightComponent::OnAddToEntity(Entity *pEntity)
{
    BaseClass::OnAddToEntity(pEntity);

    // extract position from entity
    float3 lightPosition(CalculateLightPosition());
    m_pRenderProxy->SetPosition(lightPosition);
    m_pRenderProxy->SetEntityID(pEntity->GetEntityID());

    // update bounds
    SetBounds(PointLightRenderProxy::CalculatePointLightBoundingBox(lightPosition, m_range),
              PointLightRenderProxy::CalculatePointLightBoundingSphere(lightPosition, m_range),
              true);
}

void PointLightComponent::OnRemoveFromEntity(Entity *pEntity)
{
    m_pRenderProxy->SetEntityID(0);

    BaseClass::OnRemoveFromEntity(pEntity);
}

void PointLightComponent::OnLocalTransformChange()
{
    // update position and bounds
    float3 lightPosition(CalculateLightPosition());
    m_pRenderProxy->SetPosition(lightPosition);
    SetBounds(PointLightRenderProxy::CalculatePointLightBoundingBox(lightPosition, m_range),
              PointLightRenderProxy::CalculatePointLightBoundingSphere(lightPosition, m_range),
              true);
}

void PointLightComponent::OnEntityTransformChange()
{
    BaseClass::OnEntityTransformChange();

    // update position and bounds
    float3 lightPosition(CalculateLightPosition());
    m_pRenderProxy->SetPosition(lightPosition);
    SetBounds(PointLightRenderProxy::CalculatePointLightBoundingBox(lightPosition, m_range),
              PointLightRenderProxy::CalculatePointLightBoundingSphere(lightPosition, m_range),
              true);
}

void PointLightComponent::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);
    pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);
}

void PointLightComponent::OnRemoveFromWorld(World *pWorld)
{
    pWorld->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);
    BaseClass::OnRemoveFromWorld(pWorld);
}

float3 PointLightComponent::CalculateLightPosition() const
{
    return (m_pEntity != nullptr) ? (m_pEntity->GetPosition() + m_localPosition) : (m_localPosition);
}

float3 PointLightComponent::CalculateLightColor() const
{
    return m_color * m_brightness;
}

void PointLightComponent::OnEnabledPropertyChange(ThisClass *pEntity, const void *pUserData)
{
    pEntity->m_pRenderProxy->SetEnabled(pEntity->m_enabled);
}

void PointLightComponent::OnRangePropertyChange(ThisClass *pEntity, const void *pUserData)
{
    // update bounds
    float3 lightPosition(pEntity->CalculateLightPosition());
    pEntity->SetBounds(PointLightRenderProxy::CalculatePointLightBoundingBox(lightPosition, pEntity->m_range),
                       PointLightRenderProxy::CalculatePointLightBoundingSphere(lightPosition, pEntity->m_range),
                       true);
}

void PointLightComponent::OnColorOrBrightnessPropertyChange(ThisClass *pEntity, const void *pUserData)
{
    pEntity->m_pRenderProxy->SetColor(pEntity->CalculateLightColor());
}

void PointLightComponent::OnShadowPropertyChange(ThisClass *pEntity, const void *pUserData)
{
    pEntity->m_pRenderProxy->SetShadowFlags(pEntity->m_shadowFlags);
}

void PointLightComponent::OnFalloffExponentChange(ThisClass *pEntity, const void *pUserData)
{
    pEntity->m_pRenderProxy->SetFalloffExponent(pEntity->GetFalloffExponent());
}
