#pragma once
#include "Engine/Physics/TriangleMeshCollisionShape.h"

class btBvhTriangleMeshShape;
class btScaledBvhTriangleMeshShape;

namespace Physics {

class ScaledTriangleMeshCollisionShape : public CollisionShape
{
public:
    ScaledTriangleMeshCollisionShape(const TriangleMeshCollisionShape *pParentShape, const float3 &scale);
    virtual ~ScaledTriangleMeshCollisionShape();

    // Virtual methods
    virtual const COLLISION_SHAPE_TYPE GetType() const override;
    virtual const AABox &GetLocalBoundingBox() const override;
    virtual bool LoadFromData(const void *pData, uint32 dataSize) override;
    virtual bool LoadFromStream(ByteStream *pStream, uint32 dataSize) override;
    virtual CollisionShape *CreateScaledShape(const float3 &scale) const override;
    virtual void ApplyShapeTransform(btTransform &worldTransform) const override;
    virtual void ApplyInverseShapeTransform(btTransform &worldTransform) const override;
    virtual btCollisionShape *GetBulletShape() const override;

private:
    AABox m_Bounds;
    const TriangleMeshCollisionShape *m_pParentShape;
    float3 m_scale;

    btBvhTriangleMeshShape *m_pTriangleMeshShape;
    btScaledBvhTriangleMeshShape *m_pScaledTriangleMeshShape;
};

}       // namespace Physics
