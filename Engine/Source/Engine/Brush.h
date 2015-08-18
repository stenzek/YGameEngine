#pragma once
#include "Engine/Common.h"

class PropertyTable;
class World;

class Brush : public Object
{
    DECLARE_OBJECT_TYPE_INFO(Brush, Object);
    DECLARE_OBJECT_PROPERTY_MAP(Brush);
    DECLARE_OBJECT_NO_FACTORY(Brush);

public:
    Brush(const ObjectTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~Brush();

    // Bounding volumes.
    const AABox &GetBoundingBox() const { return m_boundingBox; }
    const Sphere &GetBoundingSphere() const { return m_boundingSphere; }

    // World.
    World *GetWorld() const { return m_pWorld; }
    bool IsInWorld() const { return (m_pWorld != nullptr); }

    // Transform accessors.
    const Transform &GetTransform() const { return m_transform; }
    const float3 &GetPosition() const { return m_transform.GetPosition(); }
    const Quaternion &GetRotation() const { return m_transform.GetRotation(); }
    const float3 &GetScale() const { return m_transform.GetScale(); }

    // Virtual creation method for object types only. Not used by entity types.
    virtual bool Initialize() = 0;

    // Events
    virtual void OnAddToWorld(World *pWorld);
    virtual void OnRemoveFromWorld(World *pWorld);

protected:
    // callbacks for property system
    static bool PropertyCallbackGetPosition(Brush *pObjectPtr, const void *pUserData, float3 *pValuePtr);
    static bool PropertyCallbackSetPosition(Brush *pObjectPtr, const void *pUserData, const float3 *pValuePtr);
    static bool PropertyCallbackGetRotation(Brush *pObjectPtr, const void *pUserData, Quaternion *pValuePtr);
    static bool PropertyCallbackSetRotation(Brush *pObjectPtr, const void *pUserData, const Quaternion *pValuePtr);
    static bool PropertyCallbackGetScale(Brush *pObjectPtr, const void *pUserData, float3 *pValuePtr);
    static bool PropertyCallbackSetScale(Brush *pObjectPtr, const void *pUserData, const float3 *pValuePtr);

    // World this object is located inside.
    World *m_pWorld;

    // Base object transform.
    Transform m_transform;

    // World-space bounds of the object.
    AABox m_boundingBox;
    Sphere m_boundingSphere;
};
