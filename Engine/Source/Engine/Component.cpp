#include "Engine/PrecompiledHeader.h"
#include "Engine/Component.h"
#include "Engine/Entity.h"
#include "Core/PropertyTable.h"
Log_SetChannel(Component);

DEFINE_COMPONENT_TYPEINFO(Component);
BEGIN_COMPONENT_PROPERTIES(Component)
    PROPERTY_TABLE_MEMBER_FLOAT3("LocalPosition", 0, offsetof(Component, m_localPosition), ProperyCallbackOnLocalTransformChange, nullptr)
    PROPERTY_TABLE_MEMBER_QUATERNION("LocalRotation", 0, offsetof(Component, m_localRotation), ProperyCallbackOnLocalTransformChange, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT3("LocalScale", 0, offsetof(Component, m_localScale), ProperyCallbackOnLocalTransformChange, nullptr)
END_COMPONENT_PROPERTIES()

Component::Component(const ComponentTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_pEntity(nullptr),
      m_localPosition(float3::Zero),
      m_localRotation(Quaternion::Identity),
      m_localScale(float3::Zero),
      m_boundingBox(AABox::Zero),
      m_boundingSphere(Sphere::Zero)
{

}

Component::~Component()
{
    DebugAssert(m_pEntity == NULL);
}

bool Component::Initialize()
{
    return true;
}

void Component::OnLocalTransformChange()
{

}

void Component::OnAddToEntity(Entity *pEntity)
{
    m_pEntity = pEntity;
}

void Component::OnRemoveFromEntity(Entity *pEntity)
{
    m_pEntity = nullptr;
}

void Component::OnEntityTransformChange()
{

}

void Component::OnAddToWorld(World *pWorld)
{

}

void Component::OnRemoveFromWorld(World *pWorld)
{

}

void Component::Update(float timeSinceLastUpdate)
{

}

void Component::UpdateAsync(float timeSinceLastUpdate)
{

}

void Component::SetLocalPosition(const float3 &localPosition)
{
    m_localPosition = localPosition;
    OnLocalTransformChange();
}

void Component::SetLocalRotation(const Quaternion &localRotation)
{
    m_localRotation = localRotation;
    OnLocalTransformChange();
}

void Component::SetLocalScale(const float3 &localScale)
{
    m_localScale = localScale;
    OnLocalTransformChange();
}

Transform Component::CalculateWorldTransform() const
{
    if (m_pEntity != nullptr)
    {
        // transform local space first, then into world space
        return Transform::ConcatenateTransforms(Transform(m_localPosition, m_localRotation, m_localScale), Transform(m_pEntity->GetPosition(), Quaternion::Identity, m_pEntity->GetScale()));
    }
    else
    {
        // just use local
        return Transform(m_localPosition, m_localRotation, m_localScale);
    }
}

void Component::SetBounds(const AABox &boundingBox, const Sphere &boundingSphere, bool notifyEntity /* = true */)
{
    if (boundingBox != m_boundingBox || boundingSphere != m_boundingSphere)
    {
        m_boundingBox = boundingBox;
        m_boundingSphere = boundingSphere;

        if (notifyEntity && m_pEntity != nullptr)
            m_pEntity->OnComponentBoundsChange();
    }
}

bool Component::InitializePropertiesFromTable(Component *pComponent, const PropertyTable *pPropertyTable)
{
    // for each property in our type
    const ComponentTypeInfo *pComponentTypeInfo = pComponent->GetComponentTypeInfo();
    for (uint32 propertyIndex = 0; propertyIndex < pComponentTypeInfo->GetPropertyCount(); propertyIndex++)
    {
        const PROPERTY_DECLARATION *pPropertyDeclaration = pComponentTypeInfo->GetPropertyDeclarationByIndex(propertyIndex);

        // attempt to find a value in the property table
        const String *propertyValue = pPropertyTable->GetPropertyValuePointer(pPropertyDeclaration->Name);
        if (propertyValue == nullptr)
            continue;

        // set the value
        if (!SetPropertyValueFromString(pComponent, pPropertyDeclaration, *propertyValue))
        {
            Log_WarningPrintf("Failed to initialize component '%s' property '%s' to value '%s'", pComponentTypeInfo->GetTypeName(), pPropertyDeclaration->Name, propertyValue);
            continue;
        }
    }

    return true;
}

void Component::ProperyCallbackOnLocalTransformChange(Component *pComponent, void *pUserData /*= nullptr*/)
{
    pComponent->OnLocalTransformChange();
}

