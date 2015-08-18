#pragma once
#include "Engine/Physics/CollisionShape.h"
#include "BlockEngine/BlockWorldTypes.h"

class btBlockWorldChunkCollisionShape;

class BlockWorldChunkCollisionShape : public Physics::CollisionShape
{
public:
    BlockWorldChunkCollisionShape(const BlockPalette *pPalette, uint32 chunkSize, const BlockWorldChunk *pChunk);
    virtual ~BlockWorldChunkCollisionShape();
    
    // Virtual methods
    virtual const Physics::COLLISION_SHAPE_TYPE GetType() const override;
    virtual const AABox &GetLocalBoundingBox() const override;
    virtual bool LoadFromData(const void *pData, uint32 dataSize) override;
    virtual bool LoadFromStream(ByteStream *pStream, uint32 dataSize) override;
    virtual Physics::CollisionShape *CreateScaledShape(const float3 &scale) const override;
    virtual void ApplyShapeTransform(btTransform &worldTransform) const override;
    virtual void ApplyInverseShapeTransform(btTransform &worldTransform) const override;
    virtual btCollisionShape *GetBulletShape() const override;

private:
    btBlockWorldChunkCollisionShape *m_pBulletShape;
    AABox m_boundingBox;
};

