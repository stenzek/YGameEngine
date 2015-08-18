#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/DirectionalLightEntity.h"
#include "Renderer/RenderProxies/DirectionalLightRenderProxy.h"
#include "Renderer/RenderWorld.h"
#include "Engine/World.h"

DEFINE_ENTITY_TYPEINFO(DirectionalLightEntity, 0);
DEFINE_ENTITY_GENERIC_FACTORY(DirectionalLightEntity);
BEGIN_ENTITY_PROPERTIES(DirectionalLightEntity)
    PROPERTY_TABLE_MEMBER_BOOL("Enabled", 0, offsetof(DirectionalLightEntity, m_bEnabled), OnEnabledPropertyChange, NULL)
    PROPERTY_TABLE_MEMBER_FLOAT3("Color", 0, offsetof(DirectionalLightEntity, m_Color), OnColorOrBrightnessPropertyChange, NULL)
    PROPERTY_TABLE_MEMBER_FLOAT("Brightness", 0, offsetof(DirectionalLightEntity, m_fBrightness), OnColorOrBrightnessPropertyChange, NULL)
    PROPERTY_TABLE_MEMBER_UINT("LightShadowFlags", 0, offsetof(DirectionalLightEntity, m_iLightShadowFlags), OnShadowPropertyChange, NULL)
    PROPERTY_TABLE_MEMBER_FLOAT("AmbientFactor", 0, offsetof(DirectionalLightEntity, m_fAmbientFactor), OnColorOrBrightnessPropertyChange, NULL)
END_ENTITY_PROPERTIES()
BEGIN_ENTITY_SCRIPT_FUNCTIONS(DirectionalLightEntity)
END_ENTITY_SCRIPT_FUNCTIONS()

DirectionalLightEntity::DirectionalLightEntity(const EntityTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_bEnabled(true),
      m_Color(float3::One),
      m_fBrightness(1.0f),
      m_fAmbientFactor(0.2f),
      m_iLightShadowFlags(LIGHT_SHADOW_FLAG_CAST_STATIC_SHADOWS | LIGHT_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS),
      m_pRenderProxy(nullptr)
{
    // create the render proxies
    m_pRenderProxy = new DirectionalLightRenderProxy(GetEntityID(), m_bEnabled, float3::One, float3::Zero, m_iLightShadowFlags, CalculateLightDirection());

    // setup the object bounds
    SetBounds(AABox::MaxSize, Sphere::MaxSize);

    // update colors
    UpdateColors();
}

DirectionalLightEntity::~DirectionalLightEntity()
{
    m_pRenderProxy->Release();
}

void DirectionalLightEntity::SetEnabled(bool enabled)
{
    m_bEnabled = enabled;
    OnEnabledPropertyChange(this);
}

void DirectionalLightEntity::SetColor(const float3 &color)
{
    m_Color = color;
    OnColorOrBrightnessPropertyChange(this);
}

void DirectionalLightEntity::SetBrightness(float brightness)
{
    m_fBrightness = brightness;
    OnColorOrBrightnessPropertyChange(this);
}

void DirectionalLightEntity::SetAmbientFactor(float ambientContribution)
{
    m_fAmbientFactor = ambientContribution;
    UpdateColors();
}

void DirectionalLightEntity::SetLightShadowFlags(uint32 lightShadowFlags)
{
    m_iLightShadowFlags = lightShadowFlags;
    OnShadowPropertyChange(this);
}

void DirectionalLightEntity::Create(uint32 entityID, ENTITY_MOBILITY mobility /*= ENTITY_MOBILITY_MOVABLE*/, const Quaternion &rotation /*= Quaternion::Identity*/, const bool enabled /*= true*/, const float3 &color /*= float3::One*/, float brightness /*= 1.0f*/, uint32 shadowFlags /*= LIGHT_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS*/, float ambientFactor /*= 0.2f*/)
{
    m_mobility = mobility;
    m_transform.SetRotation(rotation);
    m_bEnabled = enabled;
    m_Color = color;
    m_fBrightness = brightness;
    m_iLightShadowFlags = shadowFlags;
    m_fAmbientFactor = ambientFactor;

    Initialize(entityID, EmptyString);
}

bool DirectionalLightEntity::Initialize(uint32 entityID, const String &entityName)
{
    if (!BaseClass::Initialize(entityID, entityName))
        return false;

    UpdateColors();
    m_pRenderProxy->SetDirection(CalculateLightDirection());
    return true;
}

void DirectionalLightEntity::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    // add render proxy to world
    pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);
}

void DirectionalLightEntity::OnRemoveFromWorld(World *pWorld)
{
    // remove render proxy from world
    pWorld->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);

    BaseClass::OnRemoveFromWorld(pWorld);
}

void DirectionalLightEntity::OnTransformChange()
{
    BaseClass::OnTransformChange();

    m_pRenderProxy->SetDirection(CalculateLightDirection());
}

float3 DirectionalLightEntity::CalculateLightDirection()
{
    // default direction of a directional light is pointing directly down.
    // rotate this by the specified rotation.
    return m_transform.GetRotation() * float3::NegativeUnitZ;
}

void DirectionalLightEntity::OnEnabledPropertyChange(ThisClass *pEntity, const void *pUserData)
{
    pEntity->m_pRenderProxy->SetEnabled(pEntity->m_bEnabled);
}

void DirectionalLightEntity::OnColorOrBrightnessPropertyChange(ThisClass *pEntity, const void *pUserData)
{
    pEntity->UpdateColors();
}

void DirectionalLightEntity::OnShadowPropertyChange(ThisClass *pEntity, const void *pUserData)
{
    pEntity->m_pRenderProxy->SetShadowFlags(pEntity->m_iLightShadowFlags);
}

void DirectionalLightEntity::UpdateColors()
{
    float3 lightColor(m_Color);
    float3 directionalLightColor(lightColor * (m_fBrightness * (1.0f - m_fAmbientFactor)));
    float3 ambientLightColor(lightColor * (m_fBrightness * m_fAmbientFactor));

    m_pRenderProxy->SetLightColor(directionalLightColor);
    m_pRenderProxy->SetAmbientColor(ambientLightColor);
}
