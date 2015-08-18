#pragma once
#include "Engine/Physics/CollisionObject.h"

class btRigidBody;

namespace Physics {

class KinematicObject : public CollisionObject
{
    class MotionState;

public:
    KinematicObject(uint32 entityID, const CollisionShape *pCollisionShape, const Transform &transform);
    virtual ~KinematicObject();

    // overrides
    virtual void OnAddedToPhysicsWorld(PhysicsWorld *pPhysicsWorld) override;
    virtual void OnRemovedFromPhysicsWorld(PhysicsWorld *pPhysicsWorld) override;

protected:
    // overrides
    virtual btCollisionObject *GetBulletCollisionObject() const override;
    virtual void OnCollisionShapeChanged() override;
    virtual void OnTransformChanged() override;

    MotionState *m_pMotionState;
    btRigidBody *m_pBulletRigidBody;
};

}   // namespace Physics
