#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/PointLightEntity.h"
#include "Renderer/RenderProxies/PointLightRenderProxy.h"
#include "Renderer/RenderWorld.h"
#include "Engine/World.h"

DEFINE_ENTITY_TYPEINFO(PointLightEntity, 0);
DEFINE_ENTITY_GENERIC_FACTORY(PointLightEntity);
BEGIN_ENTITY_PROPERTIES(PointLightEntity)
    PROPERTY_TABLE_MEMBER_BOOL("Enabled", 0, offsetof(PointLightEntity, m_bEnabled), OnEnabledPropertyChange, NULL)
    PROPERTY_TABLE_MEMBER_FLOAT("Range", 0, offsetof(PointLightEntity, m_fRange), OnRangePropertyChange, NULL)
    PROPERTY_TABLE_MEMBER_FLOAT3("Color", 0, offsetof(PointLightEntity, m_Color), OnColorOrBrightnessPropertyChange, NULL)
    PROPERTY_TABLE_MEMBER_FLOAT("Brightness", 0, offsetof(PointLightEntity, m_fBrightness), OnColorOrBrightnessPropertyChange, NULL)
    PROPERTY_TABLE_MEMBER_UINT("LightShadowFlags", 0, offsetof(PointLightEntity, m_iLightShadowFlags), OnShadowPropertyChange, NULL)
    PROPERTY_TABLE_MEMBER_FLOAT("FalloffExponent", 0, offsetof(PointLightEntity, m_fFalloffExponent), OnFalloffExponentChange, NULL)
END_ENTITY_PROPERTIES()
BEGIN_ENTITY_SCRIPT_FUNCTIONS(PointLightEntity)
END_ENTITY_SCRIPT_FUNCTIONS()

PointLightEntity::PointLightEntity(const EntityTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_bEnabled(true),
      m_fRange(8.0f),
      m_Color(float3::One),
      m_fBrightness(1.0f),
      m_iLightShadowFlags(0),
      m_fFalloffExponent(1.0f),
      m_pRenderProxy(NULL)
{

}

PointLightEntity::~PointLightEntity()
{
    m_pRenderProxy->Release();
}

void PointLightEntity::SetEnabled(bool enabled)
{
    m_bEnabled = enabled;
    OnEnabledPropertyChange(this);
}

void PointLightEntity::SetRange(float range)
{
    m_fRange = range;
    OnRangePropertyChange(this);
}

void PointLightEntity::SetColor(const float3 &color)
{
    m_Color = color;
    OnColorOrBrightnessPropertyChange(this);
}

void PointLightEntity::SetBrightness(float brightness)
{
    m_fBrightness = brightness;
    OnColorOrBrightnessPropertyChange(this);
}

void PointLightEntity::SetLightShadowFlags(uint32 lightShadowFlags)
{
    m_iLightShadowFlags = lightShadowFlags;
    OnShadowPropertyChange(this);
}

void PointLightEntity::SetFalloffExponent(float falloffExponent)
{
    m_fFalloffExponent = falloffExponent;
    OnFalloffExponentChange(this);
}

bool PointLightEntity::Initialize(uint32 entityID, const String &entityName)
{
    if (!BaseClass::Initialize(entityID, entityName))
        return false;

    // create the render proxy
    m_pRenderProxy = new PointLightRenderProxy(GetEntityID(), GetPosition(), GetRange(), CalculateLightColor(), m_iLightShadowFlags, GetFalloffExponent());
    UpdateBounds();
    return true;
}

void PointLightEntity::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);
    pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);
}

void PointLightEntity::OnRemoveFromWorld(World *pWorld)
{
    pWorld->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);
    BaseClass::OnRemoveFromWorld(pWorld);
}

void PointLightEntity::OnTransformChange()
{
    BaseClass::OnTransformChange();
    m_pRenderProxy->SetPosition(m_transform.GetPosition());
    UpdateBounds();
}

void PointLightEntity::OnEnabledPropertyChange(ThisClass *pEntity, const void *pUserData)
{
    pEntity->m_pRenderProxy->SetEnabled(pEntity->m_bEnabled);
}

void PointLightEntity::OnRangePropertyChange(ThisClass *pEntity, const void *pUserData)
{
    pEntity->m_pRenderProxy->SetRange(pEntity->GetRange());
    pEntity->UpdateBounds();
}

void PointLightEntity::OnColorOrBrightnessPropertyChange(ThisClass *pEntity, const void *pUserData)
{
    pEntity->m_pRenderProxy->SetColor(pEntity->CalculateLightColor());
}

void PointLightEntity::OnShadowPropertyChange(ThisClass *pEntity, const void *pUserData)
{
    pEntity->m_pRenderProxy->SetShadowFlags(pEntity->m_iLightShadowFlags);
}

void PointLightEntity::OnFalloffExponentChange(ThisClass *pEntity, const void *pUserData)
{
    pEntity->m_pRenderProxy->SetFalloffExponent(pEntity->GetFalloffExponent());
}

float3 PointLightEntity::CalculateLightColor()
{
    float3 lightColor(m_Color * m_fBrightness);
    return lightColor;
}

void PointLightEntity::UpdateBounds()
{
    SIMDVector3f lightPosition(GetPosition());
    SIMDVector3f minBounds(lightPosition - m_fRange);
    SIMDVector3f maxBounds(lightPosition + m_fRange);

    SetBounds(AABox(minBounds, maxBounds), Sphere(lightPosition, m_fRange));
}
