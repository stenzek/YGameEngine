#pragma once
#include "Engine/Entity.h"

class StaticMesh;
namespace Physics { class RigidBody; }
class StaticMeshRenderProxy;

class StaticMeshRigidBodyEntity : public Entity
{
    DECLARE_ENTITY_TYPEINFO(StaticMeshRigidBodyEntity, Entity);
    DECLARE_ENTITY_GENERIC_FACTORY(StaticMeshRigidBodyEntity);

public:
    StaticMeshRigidBodyEntity(const EntityTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~StaticMeshRigidBodyEntity();

    // property accessors
    const bool IsVisible() const { return m_visible; }
    const StaticMesh *GetStaticMesh() const { return m_pStaticMesh; }
    const uint32 GetShadowFlags() const { return m_shadowFlags; }

    // property setters
    void SetVisibility(bool visible);
    void SetStaticMesh(const StaticMesh *pStaticMesh);
    void SetShadowFlags(uint32 shadowFlags);

    // physics
    void SetMass(float mass);
    void SetFriction(float friction);
    void SetRollingFriction(float rollingFriction);
    void ApplyCentralImpulse(const float3 &direction);
    void ResetForces();
    void ResetVelocity();
    void ResetForcesAndVelocity();

    // External create interface
    void Create(uint32 entityID, const float3 &position = float3::Zero, const Quaternion &rotation = Quaternion::Identity, const float3 &scale = float3::One, const StaticMesh *pStaticMesh = nullptr, bool visible = true, uint32 shadowFlags = ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS, float mass = 1.0f, float friction = 0.5f, float rollingFriction = 1.0f);

    // Events
    virtual bool Initialize(uint32 entityID, const String &entityName) override;
    virtual void OnAddToWorld(World *pWorld) override;
    virtual void OnRemoveFromWorld(World *pWorld) override;
    virtual void OnTransformChange() override;
    virtual void OnComponentBoundsChange() override;
    
private:
    // updates the collision object
    void UpdateCollisionObject();
    void UpdateRenderProxy();

    // property callbacks
    static bool PropertyCallbackGetStaticMesh(ThisClass *pEntity, const void *pUserData, String *pValue);
    static bool PropertyCallbackSetStaticMesh(ThisClass *pEntity, const void *pUserData, const String *pValue);
    static void PropertyCallbackStaticMeshChanged(ThisClass *pEntity, const void *pUserData);
    static void PropertyCallbackEnableCollisionChanged(ThisClass *pEntity, const void *pUserData);
    static void PropertyCallbackMassChanged(ThisClass *pEntity, const void *pUserData);
    static void PropertyCallbackFrictionChanged(ThisClass *pEntity, const void *pUserData);
    static void PropertyCallbackRollingFrictionChanged(ThisClass *pEntity, const void *pUserData);

    // physics callbacks
    static void PhysicsCallbackTransformChanged(ThisClass *pEntity, const Transform &physicsTransform);

    // vars
    bool m_visible;
    const StaticMesh *m_pStaticMesh;
    uint32 m_shadowFlags;
    float m_mass;
    float m_friction;
    float m_rollingFriction;

    // physics proxy
    Physics::RigidBody *m_pRigidBody;

    // render proxy
    StaticMeshRenderProxy *m_pRenderProxy;
};
