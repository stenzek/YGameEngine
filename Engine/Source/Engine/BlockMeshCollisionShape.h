#pragma once
#include "Engine/Physics/CollisionShape.h"

class BlockMesh;
class BlockMeshVolume;

class BlockMeshCollisionShape : public Physics::CollisionShape
{
public:
    BlockMeshCollisionShape(const BlockMesh *pBlockMesh);
    virtual ~BlockMeshCollisionShape();

    // Virtual methods
    virtual const Physics::COLLISION_SHAPE_TYPE GetType() const  override;
    virtual const AABox &GetLocalBoundingBox() const override;
    virtual bool LoadFromData(const void *pData, uint32 dataSize) override;
    virtual bool LoadFromStream(ByteStream *pStream, uint32 dataSize) override;
    virtual Physics::CollisionShape *CreateScaledShape(const float3 &scale) const override;
    virtual void ApplyShapeTransform(btTransform &worldTransform) const override;
    virtual void ApplyInverseShapeTransform(btTransform &worldTransform) const override;
    virtual btCollisionShape *GetBulletShape() const override;
   
private:
    const BlockMesh *m_pBlockMesh;
    const BlockMeshVolume *m_pBlockVolume;
    const BlockMeshCollisionShape *m_pOriginalShape;
    class btBlockMeshCollisionShape *m_pBulletShape;
    float3 m_scale;
    AABox m_boundingBox;
};
