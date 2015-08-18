#pragma once
#include "Engine/Physics/CollisionShape.h"

class btSphereShape;

namespace Physics {

class SphereCollisionShape : public CollisionShape
{
public:
    SphereCollisionShape();
    SphereCollisionShape(const float3 &center, const float radius);
    ~SphereCollisionShape();

    const float3 &GetCenter() const { return m_center; }
    const float GetRadius() const { return m_radius; }

    // Virtual methods
    virtual const COLLISION_SHAPE_TYPE GetType() const override;
    virtual const AABox &GetLocalBoundingBox() const override;
    virtual bool LoadFromStream(ByteStream *pStream, uint32 dataSize) override;
    virtual bool LoadFromData(const void *pData, uint32 dataSize) override;
    virtual CollisionShape *CreateScaledShape(const float3 &scale) const override;
    virtual void ApplyShapeTransform(btTransform &worldTransform) const override;
    virtual void ApplyInverseShapeTransform(btTransform &worldTransform) const override;
    virtual btCollisionShape *GetBulletShape() const override;

private:
    AABox m_bounds;
    float3 m_center;
    float m_radius;
    btSphereShape *m_pBulletShape;
};

}       // namespace Physics
