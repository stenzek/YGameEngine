#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/RotationInterpolatorComponent.h"
#include "Engine/Entity.h"
Log_SetChannel(RotationInterpolatorComponent);

DEFINE_COMPONENT_TYPEINFO(RotationInterpolatorComponent);
DEFINE_COMPONENT_GENERIC_FACTORY(RotationInterpolatorComponent);
BEGIN_COMPONENT_PROPERTIES(RotationInterpolatorComponent)
    PROPERTY_TABLE_MEMBER_FLOAT("MoveDuration", 0, offsetof(RotationInterpolatorComponent, m_moveDuration), PropertyCallbackOnMoveDurationChange, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("PauseDuration", 0, offsetof(RotationInterpolatorComponent, m_pauseDuration),PropertyCallbackOnMoveDurationChange, nullptr)
    PROPERTY_TABLE_MEMBER_BOOL("ReverseCycle", 0, offsetof(RotationInterpolatorComponent, m_reverseCycle), PropertyCallbackOnReverseCycleChange, nullptr)
    PROPERTY_TABLE_MEMBER_UINT("RepeatCount", 0, offsetof(RotationInterpolatorComponent, m_repeatCount), PropertyCallbackOnRepeatCountChange, nullptr)
    PROPERTY_TABLE_MEMBER_UINT("EasingFunction", 0, offsetof(RotationInterpolatorComponent, m_easingFunction), PropertyCallbackOnEasingFunctionChange, nullptr)
    PROPERTY_TABLE_MEMBER_BOOL("AutoActivate", 0, offsetof(RotationInterpolatorComponent, m_autoActivate), PropertyCallbackOnAutoActivateChange, nullptr)
END_COMPONENT_PROPERTIES()

RotationInterpolatorComponent::RotationInterpolatorComponent(const ComponentTypeInfo *pTypeInfo /*= &s_typeInfo*/)
    : BaseClass(pTypeInfo),
      m_moveDuration(1.0f),
      m_pauseDuration(0.0f),
      m_reverseCycle(true),
      m_repeatCount(0),
      m_easingFunction(EasingFunction::Linear),
      m_autoActivate(true),
      m_active(false),
      m_interpolator(Quaternion::Identity, Quaternion::Identity, m_moveDuration, m_pauseDuration, m_reverseCycle, m_repeatCount, (EasingFunction::Type)m_easingFunction)
{

}

RotationInterpolatorComponent::~RotationInterpolatorComponent()
{

}

void RotationInterpolatorComponent::SetMoveDuration(float moveDuration)
{
    if (m_moveDuration == moveDuration)
        return;

    m_moveDuration = moveDuration;
    PropertyCallbackOnMoveDurationChange(this);
}

void RotationInterpolatorComponent::SetPauseDuration(float pauseDuration)
{
    if (m_pauseDuration == pauseDuration)
        return;

    m_pauseDuration = pauseDuration;
    PropertyCallbackOnPauseDurationChange(this);
}

void RotationInterpolatorComponent::SetReverseCycle(bool autoReverse)
{
    if (m_reverseCycle == autoReverse)
        return;

    m_reverseCycle = autoReverse;
    PropertyCallbackOnReverseCycleChange(this);
}

void RotationInterpolatorComponent::SetRepeatCount(uint32 repeatCount)
{
    if (m_repeatCount == repeatCount)
        return;

    m_repeatCount = repeatCount;
    PropertyCallbackOnRepeatCountChange(this);
}

void RotationInterpolatorComponent::SetEasingFunction(EasingFunction::Type easingFunction)
{
    if (m_easingFunction == (uint32)easingFunction)
        return;

    m_easingFunction = (uint32)easingFunction;
    PropertyCallbackOnEasingFunctionChange(this);
}

void RotationInterpolatorComponent::SetAutoActivate(bool autoActivate)
{
    if (m_autoActivate == autoActivate)
        return;

    m_autoActivate = autoActivate;
    PropertyCallbackOnAutoActivateChange(this);
}

void RotationInterpolatorComponent::Create(const Quaternion &rotationAmount /* = Quaternion::Identity */, float moveDuration /* = 1.0f */, float pauseDuration /* = 0.0f */, bool reverseCycle /* = true */, uint32 repeatCount /* = 0 */, EasingFunction::Type easingFunction /* = EasingFunction::Linear */, bool autoActivate /* = true */)
{
    DebugAssert(moveDuration > 0.0f);

    m_localPosition = float3::Zero;
    m_localRotation = rotationAmount;
    m_localScale = float3::One;
    m_moveDuration = moveDuration;
    m_pauseDuration = pauseDuration;
    m_reverseCycle = reverseCycle;
    m_repeatCount = repeatCount;
    m_easingFunction = (uint32)easingFunction;
    m_autoActivate = autoActivate;

    Initialize();
}

void RotationInterpolatorComponent::Reset()
{
    m_interpolator.Reset();

    if (m_pEntity != nullptr)
        m_pEntity->SetRotation(m_interpolator.GetCurrentValue());
}

void RotationInterpolatorComponent::Activate()
{
    if (!m_active)
    {
        m_active = true;
        if (IsAttachedToEntity())
            m_pEntity->RegisterForUpdates(0.0f);
    }
}

void RotationInterpolatorComponent::Deactivate()
{
    m_active = false;
}

bool RotationInterpolatorComponent::Initialize()
{
    if (!BaseClass::Initialize())
        return false;

    // initialize the interpolator
    DebugAssert(m_easingFunction < EasingFunction::TypeCount);
    m_interpolator.SetValues(Quaternion::Identity, m_localRotation);
    m_interpolator.SetMoveDuration(m_moveDuration);
    m_interpolator.SetPauseDuration(m_pauseDuration);
    m_interpolator.SetReverseCycle(m_reverseCycle);
    m_interpolator.SetRepeatCount(m_repeatCount);
    m_interpolator.SetFunction((EasingFunction::Type)m_easingFunction);
    m_interpolator.Reset();
    return true;
}

void RotationInterpolatorComponent::OnAddToEntity(Entity *pEntity)
{
    BaseClass::OnAddToEntity(pEntity);

    // extract the starting position from the entity's current position
    m_interpolator.SetValues(m_pEntity->GetRotation(), m_pEntity->GetRotation() * m_localRotation);
    m_interpolator.Reset();

    // if autostarting, ensure the entity is registered for ticks every frame
    if (m_autoActivate && pEntity->IsInWorld())
        pEntity->RegisterForUpdates(0.0f);
}

void RotationInterpolatorComponent::OnRemoveFromEntity(Entity *pEntity)
{
    // ensure we're deactivated
    m_active = false;

    // nuke the starting position
    m_interpolator.SetValues(m_pEntity->GetRotation(), m_localRotation);
    m_interpolator.Reset();

    BaseClass::OnRemoveFromEntity(pEntity);
}

void RotationInterpolatorComponent::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    // if autostarting, ensure the entity is registered for ticks every frame
    if (m_autoActivate || m_active)
        m_pEntity->RegisterForUpdates(0.0f);
}

void RotationInterpolatorComponent::OnRemoveFromWorld(World *pWorld)
{
    BaseClass::OnRemoveFromWorld(pWorld);
}

void RotationInterpolatorComponent::OnLocalTransformChange()
{
    BaseClass::OnLocalTransformChange();

    // update the ending position only, the starting position will still be valid
    m_interpolator.SetEndValue(m_interpolator.GetStartValue() * m_localRotation);
}

void RotationInterpolatorComponent::OnEntityTransformChange()
{
    BaseClass::OnEntityTransformChange();

    // is this an actual change of entity position?
    if (m_pEntity->GetRotation() != m_interpolator.GetCurrentValue())
    {
        // update the starting position, and ending position
        m_interpolator.SetValues(m_pEntity->GetRotation(), m_pEntity->GetRotation() * m_localRotation);
    }
}

void RotationInterpolatorComponent::Update(float timeSinceLastUpdate)
{
    // this is where the magic happens
    if (!m_active && m_autoActivate)
        Activate();

    // active?
    if (m_active)
    {
        // update interpolator
        m_interpolator.Update(timeSinceLastUpdate);

        // and the position if it should be updated
        if (m_pEntity->GetRotation() != m_interpolator.GetCurrentValue())
            m_pEntity->SetRotation(m_interpolator.GetCurrentValue());

        //Log_DevPrintf("value = %s", StringConverter::Vector3fToString(m_interpolator.GetCurrentValue().GetEulerAngles()).GetCharArray());

        // if all cycles are complete, deactivate ourselves
        if (!m_interpolator.IsActive())
            m_active = false;
    }
}

void RotationInterpolatorComponent::PropertyCallbackOnMoveDurationChange(RotationInterpolatorComponent *pComponent, const void *pUserData /*= nullptr*/)
{
    pComponent->m_interpolator.SetMoveDuration(pComponent->m_moveDuration);
    pComponent->Reset();
}

void RotationInterpolatorComponent::PropertyCallbackOnPauseDurationChange(RotationInterpolatorComponent *pComponent, const void *pUserData /*= nullptr*/)
{
    pComponent->m_interpolator.SetPauseDuration(pComponent->m_pauseDuration);
    pComponent->Reset();
}

void RotationInterpolatorComponent::PropertyCallbackOnReverseCycleChange(RotationInterpolatorComponent *pComponent, const void *pUserData /*= nullptr*/)
{
    pComponent->m_interpolator.SetReverseCycle(pComponent->m_reverseCycle);
    pComponent->Reset();
}

void RotationInterpolatorComponent::PropertyCallbackOnRepeatCountChange(RotationInterpolatorComponent *pComponent, const void *pUserData /*= nullptr*/)
{
    pComponent->m_interpolator.SetRepeatCount(pComponent->m_repeatCount);
    pComponent->Reset();
}

void RotationInterpolatorComponent::PropertyCallbackOnEasingFunctionChange(RotationInterpolatorComponent *pComponent, const void *pUserData /*= nullptr*/)
{
    DebugAssert(pComponent->m_easingFunction < EasingFunction::TypeCount);
    pComponent->m_interpolator.SetFunction((EasingFunction::Type)pComponent->m_easingFunction);
    pComponent->Reset();
}

void RotationInterpolatorComponent::PropertyCallbackOnAutoActivateChange(RotationInterpolatorComponent *pComponent, const void *pUserData /*= nullptr*/)
{
    if (pComponent->m_autoActivate && !pComponent->m_active && pComponent->IsAttachedToEntity())
        pComponent->m_pEntity->RegisterForUpdates(0.0f);
}

