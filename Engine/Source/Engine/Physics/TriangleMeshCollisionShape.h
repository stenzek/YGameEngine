#pragma once
#include "Engine/Physics/CollisionShape.h"

class btBvhTriangleMeshShape;

namespace Physics {

class TriangleMeshCollisionShape : public CollisionShape
{
    friend class ScaledTriangleMeshCollisionShape;

public:
    TriangleMeshCollisionShape();
    virtual ~TriangleMeshCollisionShape();

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
    AABox m_localBoundingBox;

    btBvhTriangleMeshShape *m_pTriangleMeshShape;
};

}       // namespace Physics
