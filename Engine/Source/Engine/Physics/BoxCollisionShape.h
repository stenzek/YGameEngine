#pragma once
#include "Engine/Physics/CollisionShape.h"

class btBoxShape;

namespace Physics {

class BoxCollisionShape : public CollisionShape
{
public:
    BoxCollisionShape();
    BoxCollisionShape(const float3 &boxCenter, const float3 &halfExtents);
    ~BoxCollisionShape();

    const float3 &GetBoxCenter() const { return m_center; }
    const float3 &GetHalfExtents() const { return m_halfExtents; }

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
    float3 m_halfExtents;
    btBoxShape *m_pBulletShape;
};

}       // namespace Physics
