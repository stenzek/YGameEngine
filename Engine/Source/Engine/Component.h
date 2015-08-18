#pragma once
#include "Engine/Common.h"
#include "Engine/ComponentTypeInfo.h"

class PropertyTable;
class Entity;
class World;

class Component : public Object
{
    DECLARE_COMPONENT_TYPEINFO(Component, Object);
    DECLARE_COMPONENT_NO_FACTORY(Component);

public:
    Component(const ComponentTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~Component();

    // Retrieves the type information for this object.
    const ComponentTypeInfo *GetComponentTypeInfo() const { return static_cast<const ComponentTypeInfo *>(m_pObjectTypeInfo); }

    // entity that we are attached to
    const Entity *GetEntity() const { return m_pEntity; }
    Entity *GetEntity() { return m_pEntity; }
    bool IsAttachedToEntity() const { return (m_pEntity != nullptr); }

    // local position/offset
    const float3 &GetLocalPosition() const { return m_localPosition; }
    const Quaternion &GetLocalRotation() const { return m_localRotation; }
    const float3 &GetLocalScale() const { return m_localScale; }

    // bounding volumes
    const AABox &GetBoundingBox() const { return m_boundingBox; }
    const Sphere &GetBoundingSphere() const { return m_boundingSphere; }

    // events
    virtual bool Initialize();
    virtual void OnAddToEntity(Entity *pEntity);
    virtual void OnRemoveFromEntity(Entity *pEntity);    
    virtual void OnAddToWorld(World *pWorld);
    virtual void OnRemoveFromWorld(World *pWorld);
    virtual void OnLocalTransformChange();
    virtual void OnEntityTransformChange();

    // Invoked events
    virtual void Update(float timeSinceLastUpdate);
    virtual void UpdateAsync(float timeSinceLastUpdate);

    // local position modifiers
    void SetLocalPosition(const float3 &localPosition);
    void SetLocalRotation(const Quaternion &localRotation);
    void SetLocalScale(const float3 &localScale);

protected:
    // calculates the transform for the component after transforming into local space
    Transform CalculateWorldTransform() const;

    // change the bounds of the component. will notify the entity if changed.
    void SetBounds(const AABox &boundingBox, const Sphere &boundingSphere, bool notifyEntity = true);

    // helper for initializing properties of an entity based on its type information
    static bool InitializePropertiesFromTable(Component *pComponent, const PropertyTable *pPropertyTable);

protected:
    // entity we are attached to
    Entity *m_pEntity;

    // local offset
    float3 m_localPosition;
    Quaternion m_localRotation;
    float3 m_localScale;

    // these bounds are in world-space
    AABox m_boundingBox;
    Sphere m_boundingSphere;

private:
    // local transform change callback
    static void ProperyCallbackOnLocalTransformChange(Component *pComponent, void *pUserData = nullptr);
};

