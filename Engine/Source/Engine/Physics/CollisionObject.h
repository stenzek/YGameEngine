#pragma once
#include "Engine/Physics/PhysicsProxy.h"
#include "Engine/Physics/CollisionShape.h"

class PhysicsWorld;
class btTransform;

namespace Physics {

class CollisionObject : public PhysicsProxy
{
    friend PhysicsWorld;

public:
    CollisionObject(uint32 entityID, const CollisionShape *pCollisionShape, const Transform &transform);
    virtual ~CollisionObject();

    // getters
    const CollisionShape *GetCollisionShape() const { return m_pCollisionShape; }
    const Transform &GetTransform() const { return m_transform; }
    const float GetFriction() const;
    const float GetRollingFriction() const;
    const float3 GetAnisotropicFriction() const;
    const float GetRestitution() const;

    // setters
    void SetCollisionShape(const CollisionShape *pCollisionShape);
    void SetTransform(const Transform &transform);
    void SetFriction(float friction);
    void SetRollingFriction(float rollingFriction);
    void SetAnisotropicFriction(const float3 &anisotropicFriction);
    void SetRestitution(float restitution);

    // helper methods
    void ConvertWorldTransformToBulletTransform(const Transform &worldTransform, btTransform *pBulletTransform);
    void ConvertBulletTransformToWorldTransform(const btTransform &bulletTransform, Transform *pWorldTransform);

protected:
    virtual btCollisionObject *GetBulletCollisionObject() const = 0;
    virtual void OnCollisionShapeChanged() = 0;
    virtual void OnTransformChanged() = 0;
    void UpdateBoundingBox();

    const CollisionShape *m_pCollisionShape;
    const CollisionShape *m_pOriginalCollisionShape;
    Transform m_transform;
};

}   // namespace Physics
