#pragma once
#include "Engine/Physics/CollisionShape.h"
#include "Engine/TerrainSection.h"

class TerrainSectionCollisionShape : public Physics::CollisionShape
{
public:
    TerrainSectionCollisionShape(const TerrainParameters *pParameters, const TerrainSection *pSectionData);
    ~TerrainSectionCollisionShape();

    virtual const Physics::COLLISION_SHAPE_TYPE GetType() const override;
    virtual const AABox &GetLocalBoundingBox() const override;
    virtual bool LoadFromData(const void *pData, uint32 dataSize) override;
    virtual bool LoadFromStream(ByteStream *pStream, uint32 dataSize) override;
    virtual Physics::CollisionShape *CreateScaledShape(const float3 &scale) const override;
    virtual void ApplyShapeTransform(btTransform &worldTransform) const override;
    virtual void ApplyInverseShapeTransform(btTransform &worldTransform) const override;
    virtual btCollisionShape *GetBulletShape() const override;

    // get the transform for the specified parameters and section
    static Transform GetSectionTransform(const TerrainParameters *pParameters, const TerrainSection *pSectionData);

private:
    AABox m_boundingBox;
    const TerrainSection *m_pSectionData;
    btCollisionShape *m_pBulletShape;
};

