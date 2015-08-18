#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/InterpolatorComponent.h"
#include "Engine/Entity.h"
Log_SetChannel(InterpolatorComponent);

DEFINE_COMPONENT_TYPEINFO(InterpolatorComponent);
DEFINE_COMPONENT_GENERIC_FACTORY(InterpolatorComponent);
BEGIN_COMPONENT_PROPERTIES(InterpolatorComponent)
    PROPERTY_TABLE_MEMBER_FLOAT("MoveDuration", 0, offsetof(InterpolatorComponent, m_moveDuration), PropertyCallbackOnMoveDurationChange, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("PauseDuration", 0, offsetof(InterpolatorComponent, m_pauseDuration), PropertyCallbackOnMoveDurationChange, nullptr)
    PROPERTY_TABLE_MEMBER_BOOL("ReverseCycle", 0, offsetof(InterpolatorComponent, m_reverseCycle), PropertyCallbackOnReverseCycleChange, nullptr)
    PROPERTY_TABLE_MEMBER_UINT("RepeatCount", 0, offsetof(InterpolatorComponent, m_repeatCount), PropertyCallbackOnRepeatCountChange, nullptr)
    PROPERTY_TABLE_MEMBER_UINT("EasingFunction", 0, offsetof(InterpolatorComponent, m_easingFunction), PropertyCallbackOnEasingFunctionChange, nullptr)
    PROPERTY_TABLE_MEMBER_BOOL("AutoActivate", 0, offsetof(InterpolatorComponent, m_autoActivate), PropertyCallbackOnAutoActivateChange, nullptr)
END_COMPONENT_PROPERTIES()

InterpolatorComponent::InterpolatorComponent(const ComponentTypeInfo *pTypeInfo /*= &s_typeInfo*/)
    : BaseClass(pTypeInfo),
      m_moveDuration(1.0f),
      m_pauseDuration(0.0f),
      m_reverseCycle(true),
      m_repeatCount(0),
      m_easingFunction(EasingFunction::Linear),
      m_autoActivate(true),
      m_active(false),
      m_positionInterpolator(float3::Zero, float3::Zero, m_moveDuration, m_pauseDuration, m_reverseCycle, m_repeatCount, (EasingFunction::Type)m_easingFunction),
      m_rotationInterpolator(Quaternion::Identity, Quaternion::Identity, m_moveDuration, m_pauseDuration, m_reverseCycle, m_repeatCount, (EasingFunction::Type)m_easingFunction),
      m_scaleInterpolator(float3::One, float3::One, m_moveDuration, m_pauseDuration, m_reverseCycle, m_repeatCount, (EasingFunction::Type)m_easingFunction),
      m_activeInterpolatorMask(0)
{

}

InterpolatorComponent::~InterpolatorComponent()
{

}

void InterpolatorComponent::SetMoveDuration(float moveDuration)
{
    if (m_moveDuration == moveDuration)
        return;

    m_moveDuration = moveDuration;
    PropertyCallbackOnMoveDurationChange(this);
}

void InterpolatorComponent::SetPauseDuration(float pauseDuration)
{
    if (m_pauseDuration == pauseDuration)
        return;

    m_pauseDuration = pauseDuration;
    PropertyCallbackOnPauseDurationChange(this);
}

void InterpolatorComponent::SetReverseCycle(bool autoReverse)
{
    if (m_reverseCycle == autoReverse)
        return;

    m_reverseCycle = autoReverse;
    PropertyCallbackOnReverseCycleChange(this);
}

void InterpolatorComponent::SetRepeatCount(uint32 repeatCount)
{
    if (m_repeatCount == repeatCount)
        return;

    m_repeatCount = repeatCount;
    PropertyCallbackOnRepeatCountChange(this);
}

void InterpolatorComponent::SetEasingFunction(EasingFunction::Type easingFunction)
{
    if (m_easingFunction == (uint32)easingFunction)
        return;

    m_easingFunction = (uint32)easingFunction;
    PropertyCallbackOnEasingFunctionChange(this);
}

void InterpolatorComponent::SetAutoActivate(bool autoActivate)
{
    if (m_autoActivate == autoActivate)
        return;

    m_autoActivate = autoActivate;
    PropertyCallbackOnAutoActivateChange(this);
}

void InterpolatorComponent::Create(const float3 &moveAmount /* = float3::Zero */, const Quaternion &rotationAmount /* = Quaternion::Identity */, const float3 &scaleAmount /* = float3::One */, float moveDuration /* = 1.0f */, float pauseDuration /* = 0.0f */, bool reverseCycle /* = true */, uint32 repeatCount /* = 0 */, EasingFunction::Type easingFunction /* = EasingFunction::Linear */, bool autoActivate /* = true */)
{
    DebugAssert(moveDuration > 0.0f);

    m_localPosition = (!moveAmount.NearEqual(float3::Zero, Y_FLT_EPSILON)) ? moveAmount : float3::Zero;
    m_localRotation = (rotationAmount != Quaternion::Identity) ? rotationAmount : Quaternion::Identity;
    m_localScale = (!scaleAmount.NearEqual(float3::One, Y_FLT_EPSILON)) ? scaleAmount : float3::One;
    m_moveDuration = moveDuration;
    m_pauseDuration = pauseDuration;
    m_reverseCycle = reverseCycle;
    m_repeatCount = repeatCount;
    m_easingFunction = (uint32)easingFunction;
    m_autoActivate = autoActivate;

    Initialize();
}

void InterpolatorComponent::UpdateActiveInterpolators()
{
    m_activeInterpolatorMask = 0;

    if (!m_localPosition.NearEqual(float3::Zero, Y_FLT_EPSILON))
    {
        m_activeInterpolatorMask |= ActiveInterpolator_Position;
        m_positionInterpolator.SetEndValue(m_positionInterpolator.GetStartValue() + m_localPosition);
        m_positionInterpolator.Reset();
    }

    if (m_localRotation != Quaternion::Identity)
    {
        m_activeInterpolatorMask |= ActiveInterpolator_Rotation;
        m_rotationInterpolator.SetEndValue(m_rotationInterpolator.GetStartValue() * m_localRotation);
        m_rotationInterpolator.Reset();
    }

    if (!m_localScale.NearEqual(float3::One, Y_FLT_EPSILON))
    {
        m_activeInterpolatorMask |= ActiveInterpolator_Scale;
        m_scaleInterpolator.SetEndValue(m_scaleInterpolator.GetStartValue() * m_localScale);
        m_scaleInterpolator.Reset();
    }
}

void InterpolatorComponent::Reset()
{
    if (m_activeInterpolatorMask & ActiveInterpolator_Position)
        m_positionInterpolator.Reset();
    if (m_activeInterpolatorMask & ActiveInterpolator_Rotation)
        m_rotationInterpolator.Reset();
    if (m_activeInterpolatorMask & ActiveInterpolator_Scale)
        m_scaleInterpolator.Reset();

    if (m_pEntity != nullptr)
    {
        if (m_activeInterpolatorMask & ActiveInterpolator_Position)
            m_pEntity->SetPosition(m_positionInterpolator.GetCurrentValue());
        if (m_activeInterpolatorMask & ActiveInterpolator_Rotation)
            m_pEntity->SetRotation(m_rotationInterpolator.GetCurrentValue());
        if (m_activeInterpolatorMask & ActiveInterpolator_Scale)
            m_pEntity->SetScale(m_scaleInterpolator.GetCurrentValue());
    }
}

void InterpolatorComponent::Activate()
{
    if (!m_active)
    {
        m_active = true;
        if (IsAttachedToEntity())
            m_pEntity->RegisterForUpdates(0.0f);
    }
}

void InterpolatorComponent::Deactivate()
{
    m_active = false;
}

bool InterpolatorComponent::Initialize()
{
    if (!BaseClass::Initialize())
        return false;

    // initialize the interpolators
    DebugAssert(m_easingFunction < EasingFunction::TypeCount);
    m_positionInterpolator.SetMoveDuration(m_moveDuration);
    m_positionInterpolator.SetPauseDuration(m_pauseDuration);
    m_positionInterpolator.SetReverseCycle(m_reverseCycle);
    m_positionInterpolator.SetRepeatCount(m_repeatCount);
    m_positionInterpolator.SetFunction((EasingFunction::Type)m_easingFunction);
    m_rotationInterpolator.SetMoveDuration(m_moveDuration);
    m_rotationInterpolator.SetPauseDuration(m_pauseDuration);
    m_rotationInterpolator.SetReverseCycle(m_reverseCycle);
    m_rotationInterpolator.SetRepeatCount(m_repeatCount);
    m_rotationInterpolator.SetFunction((EasingFunction::Type)m_easingFunction);
    m_scaleInterpolator.SetMoveDuration(m_moveDuration);
    m_scaleInterpolator.SetPauseDuration(m_pauseDuration);
    m_scaleInterpolator.SetReverseCycle(m_reverseCycle);
    m_scaleInterpolator.SetRepeatCount(m_repeatCount);
    m_scaleInterpolator.SetFunction((EasingFunction::Type)m_easingFunction);

    // update which ones are active and set values
    UpdateActiveInterpolators();
    return true;
}

void InterpolatorComponent::OnAddToEntity(Entity *pEntity)
{
    BaseClass::OnAddToEntity(pEntity);

    // extract the starting position from the entity's current position
    m_positionInterpolator.SetValues(m_pEntity->GetPosition(), m_pEntity->GetPosition() + m_localPosition);
    m_rotationInterpolator.SetValues(m_pEntity->GetRotation(), m_pEntity->GetRotation() * m_localRotation);
    m_scaleInterpolator.SetValues(m_pEntity->GetScale(), m_pEntity->GetScale() * m_localScale);

    // if autostarting, ensure the entity is registered for ticks every frame
    if (m_autoActivate && pEntity->IsInWorld())
        pEntity->RegisterForUpdates(0.0f);
}

void InterpolatorComponent::OnRemoveFromEntity(Entity *pEntity)
{
    // ensure we're deactivated
    m_active = false;

    BaseClass::OnRemoveFromEntity(pEntity);
}

void InterpolatorComponent::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    // if autostarting, ensure the entity is registered for ticks every frame
    if (m_autoActivate || m_active)
        m_pEntity->RegisterForUpdates(0.0f);
}

void InterpolatorComponent::OnRemoveFromWorld(World *pWorld)
{
    BaseClass::OnRemoveFromWorld(pWorld);
}

void InterpolatorComponent::OnLocalTransformChange()
{
    BaseClass::OnLocalTransformChange();

    // update the ending position only, the starting position will still be valid
    UpdateActiveInterpolators();
}

void InterpolatorComponent::OnEntityTransformChange()
{
    BaseClass::OnEntityTransformChange();

    // update the starting position of the interpolators if the value differs from the current value
    if (m_activeInterpolatorMask & ActiveInterpolator_Position && m_pEntity->GetPosition() != m_positionInterpolator.GetCurrentValue())
        m_positionInterpolator.SetValues(m_pEntity->GetPosition(), m_pEntity->GetPosition() + m_localPosition);
    if (m_activeInterpolatorMask & ActiveInterpolator_Rotation && m_pEntity->GetRotation() != m_rotationInterpolator.GetCurrentValue())
        m_rotationInterpolator.SetValues(m_pEntity->GetRotation(), m_pEntity->GetRotation() * m_localRotation);
    if (m_activeInterpolatorMask & ActiveInterpolator_Scale && m_pEntity->GetScale() != m_scaleInterpolator.GetCurrentValue())
        m_scaleInterpolator.SetValues(m_pEntity->GetScale(), m_pEntity->GetScale() * m_localScale);
}

void InterpolatorComponent::Update(float timeSinceLastUpdate)
{
    // this is where the magic happens
    if (!m_active && m_autoActivate)
        Activate();

    // active?
    if (m_active)
    {
        // update interpolator, and value if it has changed
        if (m_activeInterpolatorMask & ActiveInterpolator_Position)
        {
            m_positionInterpolator.Update(timeSinceLastUpdate);
            if (m_pEntity->GetPosition() != m_positionInterpolator.GetCurrentValue())
                m_pEntity->SetPosition(m_positionInterpolator.GetCurrentValue());
        }
        if (m_activeInterpolatorMask & ActiveInterpolator_Rotation)
        {
            m_rotationInterpolator.Update(timeSinceLastUpdate);
            if (m_pEntity->GetRotation() != m_rotationInterpolator.GetCurrentValue())
                m_pEntity->SetRotation(m_rotationInterpolator.GetCurrentValue());
        }
        if (m_activeInterpolatorMask & ActiveInterpolator_Scale)
        {
            m_scaleInterpolator.Update(timeSinceLastUpdate);
            if (m_pEntity->GetScale() != m_scaleInterpolator.GetCurrentValue())
                m_pEntity->SetScale(m_scaleInterpolator.GetCurrentValue());
        }

        // if all cycles are complete, deactivate ourselves
        if ((m_positionInterpolator.IsActive() | m_rotationInterpolator.IsActive() | m_scaleInterpolator.IsActive()))
            m_active = false;
    }
}

void InterpolatorComponent::PropertyCallbackOnMoveDurationChange(InterpolatorComponent *pComponent, const void *pUserData /*= nullptr*/)
{
    pComponent->m_positionInterpolator.SetMoveDuration(pComponent->m_moveDuration);
    pComponent->m_rotationInterpolator.SetMoveDuration(pComponent->m_moveDuration);
    pComponent->m_scaleInterpolator.SetMoveDuration(pComponent->m_moveDuration);

    pComponent->Reset();
}

void InterpolatorComponent::PropertyCallbackOnPauseDurationChange(InterpolatorComponent *pComponent, const void *pUserData /*= nullptr*/)
{
    pComponent->m_positionInterpolator.SetPauseDuration(pComponent->m_pauseDuration);
    pComponent->m_rotationInterpolator.SetPauseDuration(pComponent->m_pauseDuration);
    pComponent->m_scaleInterpolator.SetPauseDuration(pComponent->m_pauseDuration);
    pComponent->Reset();
}

void InterpolatorComponent::PropertyCallbackOnReverseCycleChange(InterpolatorComponent *pComponent, const void *pUserData /*= nullptr*/)
{
    pComponent->m_positionInterpolator.SetReverseCycle(pComponent->m_reverseCycle);
    pComponent->m_rotationInterpolator.SetReverseCycle(pComponent->m_reverseCycle);
    pComponent->m_scaleInterpolator.SetReverseCycle(pComponent->m_reverseCycle);
    pComponent->Reset();
}

void InterpolatorComponent::PropertyCallbackOnRepeatCountChange(InterpolatorComponent *pComponent, const void *pUserData /*= nullptr*/)
{
    pComponent->m_positionInterpolator.SetRepeatCount(pComponent->m_repeatCount);
    pComponent->m_rotationInterpolator.SetRepeatCount(pComponent->m_repeatCount);
    pComponent->m_scaleInterpolator.SetRepeatCount(pComponent->m_repeatCount);
    pComponent->Reset();
}

void InterpolatorComponent::PropertyCallbackOnEasingFunctionChange(InterpolatorComponent *pComponent, const void *pUserData /*= nullptr*/)
{
    DebugAssert(pComponent->m_easingFunction < EasingFunction::TypeCount);
    pComponent->m_positionInterpolator.SetFunction((EasingFunction::Type)pComponent->m_easingFunction);
    pComponent->m_rotationInterpolator.SetFunction((EasingFunction::Type)pComponent->m_easingFunction);
    pComponent->m_scaleInterpolator.SetFunction((EasingFunction::Type)pComponent->m_easingFunction);
    pComponent->Reset();
}

void InterpolatorComponent::PropertyCallbackOnAutoActivateChange(InterpolatorComponent *pComponent, const void *pUserData /*= nullptr*/)
{
    if (pComponent->m_autoActivate && !pComponent->m_active && pComponent->IsAttachedToEntity())
        pComponent->m_pEntity->RegisterForUpdates(0.0f);
}

