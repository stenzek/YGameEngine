#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/PositionInterpolatorComponent.h"
#include "Engine/Entity.h"

DEFINE_COMPONENT_TYPEINFO(PositionInterpolatorComponent);
DEFINE_COMPONENT_GENERIC_FACTORY(PositionInterpolatorComponent);
BEGIN_COMPONENT_PROPERTIES(PositionInterpolatorComponent)
END_COMPONENT_PROPERTIES()

PositionInterpolatorComponent::PositionInterpolatorComponent(const ComponentTypeInfo *pTypeInfo /*= &s_typeInfo*/)
    : BaseClass(pTypeInfo),
      m_moveDuration(1.0f),
      m_pauseDuration(0.0f),
      m_reverseCycle(true),
      m_repeatCount(0),
      m_easingFunction(EasingFunction::Linear),
      m_autoActivate(true),
      m_active(false),
      m_interpolator(float3::Zero, float3::Zero, m_moveDuration, m_pauseDuration, m_reverseCycle, m_repeatCount, m_easingFunction)
{

}

PositionInterpolatorComponent::~PositionInterpolatorComponent()
{

}

void PositionInterpolatorComponent::SetMoveDuration(float moveDuration)
{
    if (m_moveDuration == moveDuration)
        return;

    m_moveDuration = moveDuration;
    m_interpolator.SetMoveDuration(moveDuration);
    Reset();
}

void PositionInterpolatorComponent::SetPauseDuration(float pauseDuration)
{
    if (m_pauseDuration == pauseDuration)
        return;

    m_pauseDuration = pauseDuration;
    m_interpolator.SetPauseDuration(pauseDuration);
    Reset();
}

void PositionInterpolatorComponent::SetReverseCycle(bool autoReverse)
{
    if (m_reverseCycle == autoReverse)
        return;

    m_reverseCycle = autoReverse;
    m_interpolator.SetReverseCycle(autoReverse);
    Reset();
}

void PositionInterpolatorComponent::SetRepeatCount(uint32 repeatCount)
{
    if (m_repeatCount == repeatCount)
        return;

    m_repeatCount = repeatCount;
    m_interpolator.SetRepeatCount(repeatCount);
    Reset();
}

void PositionInterpolatorComponent::SetEasingFunction(EasingFunction::Type easingFunction)
{
    if (m_easingFunction == easingFunction)
        return;

    m_easingFunction = easingFunction;
    m_interpolator.SetFunction(easingFunction);
    Reset();
}

void PositionInterpolatorComponent::SetAutoActivate(bool autoActivate)
{
    if (m_autoActivate == autoActivate)
        return;

    m_autoActivate = autoActivate;
    if (autoActivate && !m_active && IsAttachedToEntity())
        m_pEntity->RegisterForUpdates(0.0f);
}

void PositionInterpolatorComponent::Create(const float3 &moveVector /* = float3::UnitZ */, float moveDuration /* = 1.0f */, float pauseDuration /* = 0.0f */, bool reverseCycle /* = true */, uint32 repeatCount /* = 0 */, EasingFunction::Type easingFunction /* = EasingFunction::Linear */, bool autoActivate /* = true */)
{
    DebugAssert(moveVector.SquaredLength() > 0.0f && moveDuration > 0.0f);

    m_localPosition = moveVector;
    m_localRotation = Quaternion::Identity;
    m_localScale = float3::One;
    m_moveDuration = moveDuration;
    m_pauseDuration = pauseDuration;
    m_reverseCycle = reverseCycle;
    m_repeatCount = repeatCount;
    m_easingFunction = easingFunction;
    m_autoActivate = autoActivate;

    Initialize();
}

void PositionInterpolatorComponent::Reset()
{
    m_interpolator.Reset();

    if (m_pEntity != nullptr)
        m_pEntity->SetPosition(m_interpolator.GetCurrentValue());
}

void PositionInterpolatorComponent::Activate()
{
    if (!m_active)
    {
        m_active = true;
        if (IsAttachedToEntity())
            m_pEntity->RegisterForUpdates(0.0f);
    }
}

void PositionInterpolatorComponent::Deactivate()
{
    m_active = false;
}

bool PositionInterpolatorComponent::Initialize()
{
    if (!BaseClass::Initialize())
        return false;

    // initialize the interpolator
    m_interpolator.SetValues(float3::Zero, m_localPosition);
    m_interpolator.SetMoveDuration(m_moveDuration);
    m_interpolator.SetPauseDuration(m_pauseDuration);
    m_interpolator.SetReverseCycle(m_reverseCycle);
    m_interpolator.SetRepeatCount(m_repeatCount);
    m_interpolator.SetFunction(m_easingFunction);
    m_interpolator.Reset();
    return true;
}

void PositionInterpolatorComponent::OnAddToEntity(Entity *pEntity)
{
    BaseClass::OnAddToEntity(pEntity);

    // extract the starting position from the entity's current position
    m_interpolator.SetValues(pEntity->GetPosition(), pEntity->GetPosition() + m_localPosition);
    m_interpolator.Reset();

    // if autostarting, ensure the entity is registered for ticks every frame
    if (m_autoActivate && pEntity->IsInWorld())
        pEntity->RegisterForUpdates(0.0f);
}

void PositionInterpolatorComponent::OnRemoveFromEntity(Entity *pEntity)
{
    // ensure we're deactivated
    m_active = false;

    // nuke the starting position
    m_interpolator.SetValues(float3::Zero, m_localPosition);
    m_interpolator.Reset();

    BaseClass::OnRemoveFromEntity(pEntity);
}

void PositionInterpolatorComponent::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    // if autostarting, ensure the entity is registered for ticks every frame
    if (m_autoActivate || m_active)
        m_pEntity->RegisterForUpdates(0.0f);
}

void PositionInterpolatorComponent::OnRemoveFromWorld(World *pWorld)
{
    BaseClass::OnRemoveFromWorld(pWorld);
}

void PositionInterpolatorComponent::OnLocalTransformChange()
{
    BaseClass::OnLocalTransformChange();

    // update the ending position only, the starting position will still be valid
    m_interpolator.SetEndValue(m_interpolator.GetStartValue() + m_localPosition);
}

void PositionInterpolatorComponent::OnEntityTransformChange()
{
    BaseClass::OnEntityTransformChange();

    // is this an actual change of entity position?
    if (m_pEntity->GetPosition() != m_interpolator.GetCurrentValue())
    {
        // update the starting position, and ending position
        m_interpolator.SetValues(m_pEntity->GetPosition(), m_pEntity->GetPosition() + m_localPosition);
    }
}

void PositionInterpolatorComponent::Update(float timeSinceLastUpdate)
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
        if (m_pEntity->GetPosition() != m_interpolator.GetCurrentValue())
            m_pEntity->SetPosition(m_interpolator.GetCurrentValue());

        // if all cycles are complete, deactivate ourselves
        if (!m_interpolator.IsActive())
            m_active = false;
    }
}

