#pragma once
#include "Engine/Physics/CollisionObject.h"

namespace Physics {

class StaticObject : public CollisionObject
{
public:
    StaticObject(uint32 entityID, const CollisionShape *pCollisionShape, const Transform &transform);
    virtual ~StaticObject();

    // overrides
    virtual void OnAddedToPhysicsWorld(PhysicsWorld *pPhysicsWorld) override;
    virtual void OnRemovedFromPhysicsWorld(PhysicsWorld *pPhysicsWorld) override;

protected:
    // overrides
    virtual btCollisionObject *GetBulletCollisionObject() const override;
    virtual void OnCollisionShapeChanged() override;
    virtual void OnTransformChanged() override;

    btCollisionObject *m_pBulletCollisionObject;
};

}   // namespace Physics
