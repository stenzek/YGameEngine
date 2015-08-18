#pragma once
#include "Engine/Physics/CollisionObject.h"
#include "Engine/Physics/CollisionShape.h"

class btRigidBody;

namespace Physics {

class RigidBody : public CollisionObject
{
    class MotionState;
    friend class MotionState;

public:
    typedef void(*UpdateTransformCallbackFunction)(void *pUserData, const Transform &newTransform);

public:
    RigidBody(uint32 entityID, const CollisionShape *pCollisionShape, const Transform &transform);
    ~RigidBody();

    // getters
    const bool IsMovable() const { return m_isMovable; }
    const float GetMass() const { return m_mass; }

    // setters
    void SetMass(float mass);

    // apply a force
    void ApplyCentralImpulse(const float3 &direction);

    // clear any forces
    void ResetForces();

    // clear any velocity
    void ResetVelocity();

    // callback from when the rigid body is moved
    void SetCallbackFunction(UpdateTransformCallbackFunction callback, void *pUserData);

    // overrides
    virtual void OnAddedToPhysicsWorld(PhysicsWorld *pPhysicsWorld) override;
    virtual void OnRemovedFromPhysicsWorld(PhysicsWorld *pPhysicsWorld) override;
    virtual void OnSynchronize() override;

private:
    virtual btCollisionObject *GetBulletCollisionObject() const override;
    virtual void OnCollisionShapeChanged() override;
    virtual void OnTransformChanged() override;

    // reset the rigid body, i.e. call when the transform is changed or shape changes
    void ResetRigidBodyState();

    bool m_isMovable;
    float m_mass;

    MotionState *m_pMotionState;
    btRigidBody *m_pBulletRigidBody;

    UpdateTransformCallbackFunction m_updateTransformCallbackFunction;
    void *m_pUpdateTransformCallbackUserData;
};

}       // namespace Physics
