#include "Engine/PrecompiledHeader.h"
#include "Engine/Entity.h"
#include "Engine/Component.h"
#include "Engine/ComponentTypeInfo.h"
#include "Engine/World.h"
Log_SetChannel(Entity);

DEFINE_ENTITY_TYPEINFO(Entity, SCRIPT_OBJECT_FLAG_TABLED);
BEGIN_ENTITY_PROPERTIES(Entity)
    PROPERTY_TABLE_MEMBER_UINT("Mobility", PROPERTY_FLAG_READ_ONLY, offsetof(Entity, m_mobility), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER("Position", PROPERTY_TYPE_FLOAT3, 0, PropertyCallbackGetPosition, nullptr, PropertyCallbackSetPosition, nullptr, nullptr, nullptr)
    PROPERTY_TABLE_MEMBER("Rotation", PROPERTY_TYPE_QUATERNION, 0, PropertyCallbackGetRotation, nullptr, PropertyCallbackSetRotation, nullptr, nullptr, nullptr)
    PROPERTY_TABLE_MEMBER("Scale", PROPERTY_TYPE_FLOAT3, 0, PropertyCallbackGetScale, nullptr, PropertyCallbackSetScale, nullptr, nullptr, nullptr)
END_ENTITY_PROPERTIES()
BEGIN_ENTITY_SCRIPT_FUNCTIONS(Entity)
    DEFINE_ENTITY_SCRIPT_FUNCTION("GetEntityID", MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_RET(uint32, Entity, GetEntityID))
    DEFINE_ENTITY_SCRIPT_FUNCTION("GetEntityName", MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_RET(const char *, Entity, GetEntityName))
    DEFINE_ENTITY_SCRIPT_FUNCTION("GetPosition", MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_RET(float3, Entity, GetPosition))
    DEFINE_ENTITY_SCRIPT_FUNCTION("GetRotation", MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_RET(Quaternion, Entity, GetRotation))
    DEFINE_ENTITY_SCRIPT_FUNCTION("GetScale", MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_RET(float3, Entity, GetScale))
    DEFINE_ENTITY_SCRIPT_FUNCTION("SetPosition", MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_P1(Entity, SetPosition, float3))
    DEFINE_ENTITY_SCRIPT_FUNCTION("SetRotation", MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_P1(Entity, SetRotation, Quaternion))
    DEFINE_ENTITY_SCRIPT_FUNCTION("SetScale", MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_P1(Entity, SetScale, float3))
END_ENTITY_SCRIPT_FUNCTIONS()

Entity::Entity(const EntityTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_entityID(0),
      m_mobility(ENTITY_MOBILITY_STATIC),
      m_transform(Transform::Identity),
      m_boundingBox(AABox::Zero),
      m_boundingSphere(Sphere::Zero),
      m_pWorld(nullptr),
      m_pWorldData(nullptr),
      m_registeredUpdateCount(0),
      m_registeredUpdateInterval(Y_FLT_MAX),
      m_registeredAsyncUpdateCount(0),
      m_registeredAsyncUpdateInterval(Y_FLT_MAX)
{

}

Entity::~Entity()
{
    DebugAssert(m_pWorld == NULL);

    // detach all components
    for (uint32 i = 0; i < m_components.GetSize(); i++)
    {
        Component *pComponent = m_components[i];
        pComponent->OnRemoveFromEntity(this);
        pComponent->Release();
    }
}

bool Entity::Initialize(uint32 entityID, const String &entityName)
{
    DebugAssert(entityID != 0);
    m_entityID = entityID;
    m_entityName = entityName;
    return true;
}

void Entity::OnAddToWorld(World *pWorld)
{
    DebugAssert(m_pWorld == nullptr);

    // set world pointer
    m_pWorld = pWorld;

    // if previously registered for updates, set it up
    if (m_registeredUpdateCount > 0)
        pWorld->RegisterEntityForUpdate(this, m_registeredUpdateInterval);
    if (m_registeredAsyncUpdateCount > 0)
        pWorld->RegisterEntityForAsyncUpdate(this, m_registeredAsyncUpdateInterval);

    // broadcast to script
    CallScriptMethod("OnSpawn");

    // broadcast to components
    for (uint32 i = 0; i < m_components.GetSize(); i++)
        m_components[i]->OnAddToWorld(pWorld);
}

void Entity::OnRemoveFromWorld(World *pWorld)
{
    DebugAssert(m_pWorld == pWorld);

    // broadcast to components
    for (uint32 i = 0; i < m_components.GetSize(); i++)
        m_components[i]->OnRemoveFromWorld(pWorld);

    // broadcast to script
    CallScriptMethod("OnDespawn");

    // null world pointer
    m_pWorld = nullptr;
}

void Entity::OnTransformChange()
{
    // broadcast to components
    for (uint32 i = 0; i < m_components.GetSize(); i++)
    {
        Component *pComponent = m_components[i];
        pComponent->OnEntityTransformChange();
    }
}

void Entity::OnComponentBoundsChange()
{
    // use the current bounding box as a fallback, this should really be overriden by derived classes
    AABox boundingBox;
    Sphere boundingSphere;
    if (GetComponentBounds(boundingBox, boundingSphere))
    {
        boundingBox.Merge(m_boundingBox);
        boundingSphere.Merge(boundingSphere);
        SetBounds(boundingBox, boundingSphere, false);
    }
}

void Entity::OnAction(Entity *pInitiator, uint32 action)
{
    // pass to script
    CallScriptMethod("OnAction", pInitiator, action);
}

bool Entity::SetTransform(const Transform &transform)
{
    // position can be changed if not in world
    if (IsInWorld() && GetMobility() == ENTITY_MOBILITY_STATIC)
        return false;

    // update transform
    m_transform = transform;
    OnTransformChange();
    return true;
}

void Entity::Update(float timeSinceLastUpdate)
{
    // update any components
    for (uint32 i = 0; i < m_components.GetSize(); i++)
        m_components[i]->Update(timeSinceLastUpdate);
}

void Entity::UpdateAsync(float timeSinceLastUpdate)
{
    // update any components
    for (uint32 i = 0; i < m_components.GetSize(); i++)
        m_components[i]->UpdateAsync(timeSinceLastUpdate);
}

void Entity::RegisterForUpdates(float interval /* = 0.0f */)
{
    DebugAssert(interval >= 0.0f);
    if (interval < m_registeredUpdateInterval || !m_registeredUpdateCount)
    {
        m_registeredUpdateInterval = Min(m_registeredUpdateInterval, interval);
        if (m_pWorld != nullptr)
            m_pWorld->RegisterEntityForUpdate(this, interval);
    }

    m_registeredUpdateCount++;
}

void Entity::RegisterForAsyncUpdates(float interval /* = 0.0f */)
{
    DebugAssert(interval >= 0.0f);
    if (interval < m_registeredAsyncUpdateInterval || m_registeredAsyncUpdateCount == 0)
    {
        m_registeredAsyncUpdateInterval = Min(m_registeredAsyncUpdateInterval, interval);
        if (m_pWorld != nullptr)
            m_pWorld->RegisterEntityForAsyncUpdate(this, interval);
    }

    m_registeredAsyncUpdateCount++;
}

void Entity::UnregisterForUpdates()
{
    Assert(m_registeredUpdateCount > 0);

    if ((--m_registeredUpdateCount) == 0)
    {
        if (m_pWorld != nullptr)
            m_pWorld->UnregisterEntityForUpdate(this);
    }
}

void Entity::UnregisterForAsyncUpdates()
{
    Assert(m_registeredAsyncUpdateCount > 0);

    if ((--m_registeredAsyncUpdateCount) == 0)
    {
        if (m_pWorld != nullptr)
            m_pWorld->UnregisterEntityForAsyncUpdate(this);
    }
}

bool Entity::SetPosition(const float3 &position)
{
    // position can be changed if not in world
    if (IsInWorld() && GetMobility() == ENTITY_MOBILITY_STATIC)
        return false;

    Transform transform(m_transform);
    transform.SetPosition(position);

    return SetTransform(transform);
}

bool Entity::SetRotation(const Quaternion &rotation)
{
    // position can be changed if not in world
    if (IsInWorld() && GetMobility() == ENTITY_MOBILITY_STATIC)
        return false;

    Transform transform(m_transform);
    transform.SetRotation(rotation);

    return SetTransform(transform);
}

bool Entity::SetScale(const float3 &scale)
{
    // position can be changed if not in world
    if (IsInWorld() && GetMobility() == ENTITY_MOBILITY_STATIC)
        return false;

    Transform transform(m_transform);
    transform.SetScale(scale);

    return SetTransform(transform);
}

void Entity::AddComponent(Component *pComponent)
{
    DebugAssert(pComponent->GetEntity() == nullptr);

#ifdef Y_BUILD_CONFIG_DEBUG
    uint32 i;
    for (i = 0; i < m_components.GetSize(); i++)
        DebugAssert(m_components[i] != pComponent);
#endif

    m_components.Add(pComponent);
    pComponent->AddRef();
    pComponent->OnAddToEntity(this);

    // in world?
    if (m_pWorld != nullptr)
        pComponent->OnAddToWorld(m_pWorld);

    // changed bounding box?
    AABox mergedBoundingBox(m_boundingBox);
    Sphere mergedBoundingSphere(m_boundingSphere);
    mergedBoundingBox.Merge(pComponent->GetBoundingBox());
    mergedBoundingSphere.Merge(pComponent->GetBoundingSphere());
    if (mergedBoundingBox != m_boundingBox || mergedBoundingSphere != m_boundingSphere)
        SetBounds(mergedBoundingBox, mergedBoundingSphere, false);
}

void Entity::RemoveComponent(Component *pComponent)
{
    for (uint32 i = 0; i < m_components.GetSize(); i++)
    {
        if (m_components[i] == pComponent)
        {
            // removing a component while it is in world is perfectly legal, so cater for this case
            if (m_pWorld != nullptr)
                pComponent->OnRemoveFromWorld(m_pWorld);

            // call removed function
            pComponent->OnRemoveFromEntity(this);
            pComponent->Release();

            // remove it from our array, and destroy the component
            m_components.OrderedRemove(i);

            // merge bounds
            OnComponentBoundsChange();
            return;
        }
    }
    
    Panic("Attempting to remove component from entity that is not in list.");
}

bool Entity::GetComponentBounds(AABox &boundingBox, Sphere &boundingSphere)
{
    if (m_components.GetSize() == 0)
        return false;

    boundingBox = m_components[0]->GetBoundingBox();
    boundingSphere = m_components[0]->GetBoundingSphere();
    for (uint32 i = 1; i < m_components.GetSize(); i++)
    {
        boundingBox.Merge(m_components[i]->GetBoundingBox());
        boundingSphere.Merge(m_components[i]->GetBoundingSphere());
    }

    return true;
}

void Entity::SetBounds(const AABox &boundingBox, const Sphere &boundingSphere, bool mergeWithComponents /* = true */)
{
    AABox mergedBoundingBox;
    Sphere mergedBoundingSphere;
    if (mergeWithComponents && GetComponentBounds(mergedBoundingBox, mergedBoundingSphere))
    {
        mergedBoundingBox.Merge(boundingBox);
        mergedBoundingSphere.Merge(boundingSphere);

        if (mergedBoundingBox != m_boundingBox || mergedBoundingSphere != m_boundingSphere)
        {
            m_boundingBox = mergedBoundingBox;
            m_boundingSphere = mergedBoundingSphere;

            if (m_pWorld != nullptr)
                m_pWorld->MoveEntity(this);
        }
    }
    else
    {
        if (boundingBox != m_boundingBox || boundingSphere != m_boundingSphere)
        {
            m_boundingBox = boundingBox;
            m_boundingSphere = boundingSphere;

            if (m_pWorld != nullptr)
                m_pWorld->MoveEntity(this);
        }
    }
}

bool Entity::PropertyCallbackGetPosition(Entity *pObjectPtr, const void *pUserData, float3 *pValuePtr)
{
    *pValuePtr = pObjectPtr->GetPosition();
    return true;
}

bool Entity::PropertyCallbackSetPosition(Entity *pObjectPtr, const void *pUserData, const float3 *pValuePtr)
{
    if (pObjectPtr->IsInWorld())
    {
        // pass through
        return pObjectPtr->SetPosition(*pValuePtr);
    }
    else
    {
        pObjectPtr->m_transform.SetPosition(*pValuePtr);
        return true;
    }
}

bool Entity::PropertyCallbackGetRotation(Entity *pObjectPtr, const void *pUserData, Quaternion *pValuePtr)
{
    *pValuePtr = pObjectPtr->GetRotation();
    return true;
}

bool Entity::PropertyCallbackSetRotation(Entity *pObjectPtr, const void *pUserData, const Quaternion *pValuePtr)
{
    if (pObjectPtr->IsInWorld())
    {
        // pass through
        return pObjectPtr->SetRotation(*pValuePtr);
    }
    else
    {
        pObjectPtr->m_transform.SetRotation(*pValuePtr);
        return true;
    }
}

bool Entity::PropertyCallbackGetScale(Entity *pObjectPtr, const void *pUserData, float3 *pValuePtr)
{
    *pValuePtr = pObjectPtr->GetScale();
    return true;
}

bool Entity::PropertyCallbackSetScale(Entity *pObjectPtr, const void *pUserData, const float3 *pValuePtr)
{
    if (pObjectPtr->IsInWorld())
    {
        // pass through
        return pObjectPtr->SetScale(*pValuePtr);
    }
    else
    {
        pObjectPtr->m_transform.SetScale(*pValuePtr);
        return true;
    }
}

// bool Entity::InitializeEntityPropertiesFromTable(Entity *pEntity, const PropertyTable *pPropertyTable)
// {
//     // for each property in our type
//     const EntityTypeInfo *pEntityTypeInfo = pEntity->GetEntityTypeInfo();
//     for (uint32 propertyIndex = 0; propertyIndex < pEntityTypeInfo->GetPropertyCount(); propertyIndex++)
//     {
//         const PROPERTY_DECLARATION *pPropertyDeclaration = pEntityTypeInfo->GetPropertyDeclarationByIndex(propertyIndex);
//         
//         // attempt to find a value in the property table
//         const String *propertyValue = pPropertyTable->GetPropertyValuePointer(pPropertyDeclaration->Name);
//         if (propertyValue == nullptr)
//             continue;
//         
//         // set the value
//         if (!SetPropertyValueFromString(pEntity, pPropertyDeclaration, *propertyValue))
//         {
//             Log_WarningPrintf("Failed to initialize entity '%s' property '%s' to value '%s'", pEntityTypeInfo->GetTypeName(), pPropertyDeclaration->Name, propertyValue);
//             continue;
//         }
//     }
// 
//     return true;
// }

SIMDVector4f EncodeUInt32ToColor(uint32 EntityId)
{
    float val[4];
    val[0] = float(EntityId & 0xFF) / 255.0f;
    val[1] = float((EntityId >> 8) & 0xFF) / 255.0f;
    val[2] = float((EntityId >> 16) & 0xFF) / 255.0f;
    val[3] = float(EntityId >> 24) / 255.0f;

    return SIMDVector4f(val);
}

uint32 DecodeUInt32FromColorR8G8B8A8(const void *pValue)
{
    uint32 intValue = *(uint32 *)pValue;
    uint32 ret;

//     ret = ((intValue >> 24)) |
//           (((intValue >> 16) & 0xFF) << 8) |
//           (((intValue >> 8) & 0xFF) << 16) |
//           ((intValue & 0xFF) << 24);

    ret = intValue;
    return ret;
}

uint32 DecodeUInt32IdFromColor(const float4 &EncodedColor)
{
    uint32 ret;
    ret = ((uint32(EncodedColor.x * 255.0f) & 0xFF)) |
          ((uint32(EncodedColor.y * 255.0f) & 0xFF) << 8) |
          ((uint32(EncodedColor.z * 255.0f) & 0xFF) << 16) |
          ((uint32(EncodedColor.w * 255.0f) & 0xFF) << 24);

    return ret;
}
