#include "Engine/PrecompiledHeader.h"
#include "Engine/Brush.h"
#include "Engine/World.h"
#include "Core/PropertyTable.h"
//Log_SetChannel(WorldObject);

DEFINE_OBJECT_TYPE_INFO(Brush);
BEGIN_OBJECT_PROPERTY_MAP(Brush)
    PROPERTY_TABLE_MEMBER("Position", PROPERTY_TYPE_FLOAT3, PROPERTY_FLAG_READ_ONLY, PropertyCallbackGetPosition, nullptr, PropertyCallbackSetPosition, nullptr, nullptr, nullptr)
    PROPERTY_TABLE_MEMBER("Rotation", PROPERTY_TYPE_QUATERNION, PROPERTY_FLAG_READ_ONLY, PropertyCallbackGetRotation, nullptr, PropertyCallbackSetRotation, nullptr, nullptr, nullptr)
    PROPERTY_TABLE_MEMBER("Scale", PROPERTY_TYPE_FLOAT3, PROPERTY_FLAG_READ_ONLY, PropertyCallbackGetScale, nullptr, PropertyCallbackSetScale, nullptr, nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

Brush::Brush(const ObjectTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_pWorld(nullptr),
      m_transform(Transform::Identity),
      m_boundingBox(AABox::Zero),
      m_boundingSphere(Sphere::Zero)
{

}

Brush::~Brush()
{
    DebugAssert(m_pWorld == nullptr);
}

void Brush::OnAddToWorld(World *pWorld)
{
    DebugAssert(m_pWorld == nullptr);

    // set world pointer
    m_pWorld = pWorld;
}

void Brush::OnRemoveFromWorld(World *pWorld)
{
    DebugAssert(m_pWorld == pWorld);

    // null world pointer
    m_pWorld = nullptr;
}

bool Brush::PropertyCallbackGetPosition(Brush *pObjectPtr, const void *pUserData, float3 *pValuePtr)
{
    *pValuePtr = pObjectPtr->GetPosition();
    return true;
}

bool Brush::PropertyCallbackSetPosition(Brush *pObjectPtr, const void *pUserData, const float3 *pValuePtr)
{
    pObjectPtr->m_transform.SetPosition(*pValuePtr);
    return true;
}

bool Brush::PropertyCallbackGetRotation(Brush *pObjectPtr, const void *pUserData, Quaternion *pValuePtr)
{
    *pValuePtr = pObjectPtr->GetRotation();
    return true;
}

bool Brush::PropertyCallbackSetRotation(Brush *pObjectPtr, const void *pUserData, const Quaternion *pValuePtr)
{
    pObjectPtr->m_transform.SetRotation(*pValuePtr);
    return true;
}

bool Brush::PropertyCallbackGetScale(Brush *pObjectPtr, const void *pUserData, float3 *pValuePtr)
{
    *pValuePtr = pObjectPtr->GetScale();
    return true;
}

bool Brush::PropertyCallbackSetScale(Brush *pObjectPtr, const void *pUserData, const float3 *pValuePtr)
{
    pObjectPtr->m_transform.SetScale(*pValuePtr);
    return true;
}

