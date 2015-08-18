#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/SunLightEntity.h"
#include "Renderer/RenderProxies/DirectionalLightRenderProxy.h"
#include "Renderer/RenderWorld.h"
#include "Engine/World.h"

DEFINE_ENTITY_TYPEINFO(SunLightEntity, 0);
DEFINE_ENTITY_GENERIC_FACTORY(SunLightEntity);
BEGIN_ENTITY_PROPERTIES(SunLightEntity)
    PROPERTY_TABLE_MEMBER_BOOL("Enabled", 0, offsetof(SunLightEntity, m_bEnabled), OnEnabledPropertyChange, NULL)
    PROPERTY_TABLE_MEMBER_UINT("LightShadowFlags", 0, offsetof(SunLightEntity, m_iLightShadowFlags), OnShadowPropertyChange, NULL)
END_ENTITY_PROPERTIES()
BEGIN_ENTITY_SCRIPT_FUNCTIONS(SunLightEntity)
END_ENTITY_SCRIPT_FUNCTIONS()

SunLightEntity::SunLightEntity(const EntityTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_bEnabled(true),
      m_iLightShadowFlags(LIGHT_SHADOW_FLAG_CAST_STATIC_SHADOWS | LIGHT_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS),
      m_timeOfDay(0),
      m_timeScale(1.0f),
      m_pRenderProxy(nullptr)
{
    // create the render proxies
    m_pRenderProxy = new DirectionalLightRenderProxy(GetEntityID(), m_bEnabled, float3::One, float3::Zero, m_iLightShadowFlags, float3::NegativeUnitZ);

    // setup the object bounds
    SetBounds(AABox::MaxSize, Sphere::MaxSize);
}

SunLightEntity::~SunLightEntity()
{
    m_pRenderProxy->Release();
}

void SunLightEntity::SetEnabled(bool enabled)
{
    m_bEnabled = enabled;
    OnEnabledPropertyChange(this);
}

void SunLightEntity::SetLightShadowFlags(uint32 lightShadowFlags)
{
    m_iLightShadowFlags = lightShadowFlags;
    OnShadowPropertyChange(this);
}

void SunLightEntity::Create(uint32 entityID, const bool enabled /* = true */, uint32 shadowFlags /* = LIGHT_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS */)
{
    m_mobility = ENTITY_MOBILITY_MOVABLE;
    m_bEnabled = enabled;
    m_iLightShadowFlags = shadowFlags;

    Initialize(entityID, EmptyString);
    UpdateLight();
}

bool SunLightEntity::Initialize(uint32 entityID, const String &entityName)
{
    if (!BaseClass::Initialize(entityID, entityName))
        return false;

    UpdateLight();
    return true;
}

void SunLightEntity::Update(float timeSinceLastUpdate)
{
    BaseClass::Update(timeSinceLastUpdate);

    m_timeOfDay += timeSinceLastUpdate * m_timeScale;
    while (m_timeOfDay >= 86400.0f)
        m_timeOfDay -= 86400.0f;

    /*m_timeOfDaySeconds += timeSinceLastUpdate * m_timeScale;
    while (m_timeOfDaySeconds >= 60.0f)
    {
        m_timeOfDayMinutes += 1.0f;
        m_timeOfDaySeconds -= 60.0f;

        while (m_timeOfDayMinutes >= 60.0f)
        {
            m_timeOfDayHours += 1.0f;
            m_timeOfDayMinutes -= 60.0f;

            while (m_timeOfDayHours >= 24.0f)
                m_timeOfDayHours -= 24.0f;
        }
    }*/

    UpdateLight();
}

void SunLightEntity::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    pWorld->RegisterEntityForUpdate(this, 0.0f);

    // add render proxy to world
    pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);
}

void SunLightEntity::OnRemoveFromWorld(World *pWorld)
{
    // remove render proxy from world
    pWorld->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);

    m_pWorld->UnregisterEntityForUpdate(this);

    BaseClass::OnRemoveFromWorld(pWorld);
}

void SunLightEntity::OnTransformChange()
{
    BaseClass::OnTransformChange();
}

void SunLightEntity::OnEnabledPropertyChange(ThisClass *pEntity, const void *pUserData)
{
    pEntity->m_pRenderProxy->SetEnabled(pEntity->m_bEnabled);
}

void SunLightEntity::OnShadowPropertyChange(ThisClass *pEntity, const void *pUserData)
{
    pEntity->m_pRenderProxy->SetShadowFlags(pEntity->m_iLightShadowFlags);
}

void SunLightEntity::AddStep(float hours, float minutes, const float3 &direction, uint32 color, float brightness, float ambientTerm)
{
    Step step;
    step.Hours = hours;
    step.Minutes = minutes;
    step.Direction = direction;
    step.Color = color;
    step.Brightness = brightness;
    step.AmbientTerm = ambientTerm;
    step.StartTime = hours * 3600.0f + minutes * 60.0f;
    m_steps.Add(step);
    SortSteps();
}

void SunLightEntity::SortSteps()
{
    m_steps.SortCB([](const Step &left, const Step &right) {
        return (left.StartTime < right.StartTime) ? -1 : ((left.StartTime == right.StartTime) ? 0 : 1);
    });
}

void SunLightEntity::UpdateLight()
{
    if (m_steps.IsEmpty())
        return;

    // find closest step
    uint32 foundStepIndex = 0;
    for (uint32 stepIndex = 0; stepIndex < m_steps.GetSize(); stepIndex++)
    {
        const Step &step = m_steps[stepIndex];
        if (step.StartTime <= m_timeOfDay)
            foundStepIndex = stepIndex;
        else
            break;
    }

    // get this and next
    const Step &thisStep = m_steps[foundStepIndex];
    const Step &nextStep = m_steps[(foundStepIndex + 1) % m_steps.GetSize()];
    float range = (nextStep.StartTime > thisStep.StartTime) ? (nextStep.StartTime - thisStep.StartTime) : (86400.0f - thisStep.StartTime + nextStep.StartTime);
    float factor = (m_timeOfDay - thisStep.StartTime) / range;

    // lerp values
    float3 direction = thisStep.Direction.Lerp(nextStep.Direction, factor);
    float4 color = PixelFormatHelpers::ConvertRGBAToFloat4(thisStep.Color).Lerp(PixelFormatHelpers::ConvertRGBAToFloat4(nextStep.Color), factor);
    float brightness = Math::Lerp(thisStep.Brightness, nextStep.Brightness, factor);
    float ambient = Math::Lerp(thisStep.AmbientTerm, nextStep.AmbientTerm, factor);

    // update light
    m_pRenderProxy->SetDirection(direction);
    m_pRenderProxy->SetLightColor(color.xyz() * brightness);
    m_pRenderProxy->SetAmbientColor(color.xyz() * ambient);
}

void SunLightEntity::AddDefaultSteps()
{
    AddStep(0.0f, 0.0f, float3(0.0f, 1.0f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 0.1f, 0.1f);
    AddStep(1.5f, 0.0f, float3(0.5f, 0.5f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 0.1f, 0.1f);
    AddStep(3.0f, 0.0f, float3(1.0f, 0.0f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 0.1f, 0.1f);
    AddStep(4.5f, 0.0f, float3(0.5f, -0.5f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 0.275f, 0.2f);
    AddStep(6.0f, 0.0f, float3(0.0f, -1.0f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 0.6f, 0.3f);
    AddStep(7.5f, 0.0f, float3(-0.5f, -0.5f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 0.7f, 0.3f);
    AddStep(9.0f, 0.0f, float3(-1.0f, 0.0f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 0.8f, 0.3f);
    AddStep(10.5f, 0.0f, float3(-0.5f, 0.5f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 1.0f, 0.4f);
    AddStep(12.0f, 0.0f, float3(0.0f, 1.0f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 1.2f, 0.5f);
    AddStep(13.5f, 0.0f, float3(0.5f, 0.5f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 1.0f, 0.45f);
    AddStep(15.0f, 0.0f, float3(1.0f, 0.0f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 0.8f, 0.4f);
    AddStep(17.5f, 0.0f, float3(0.5f, -0.5f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 0.7f, 0.35f);
    AddStep(18.0f, 0.0f, float3(0.0f, -1.0f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 0.3f, 0.3f);
    AddStep(19.5f, 0.0f, float3(-0.5f, -0.5f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 0.2f, 0.2f);
    AddStep(21.0f, 0.0f, float3(-1.0f, 0.0f, -0.7f).Normalize(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), 0.1f, 0.1f);
}

void SunLightEntity::SetTimeOfDay(float hours, float minutes, float seconds)
{
    m_timeOfDay = Y_fmodf(hours, 60.0f) * 3600.0f + Y_fmodf(minutes, 60.0f) * 60.0f + seconds;
}

void SunLightEntity::SetTimeOfDay(float secondInDay)
{
    m_timeOfDay = Y_fmodf(secondInDay, 86400.0f);
}
