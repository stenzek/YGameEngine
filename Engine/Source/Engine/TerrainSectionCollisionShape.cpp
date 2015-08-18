#include "Engine/PrecompiledHeader.h"
#include "Engine/TerrainSectionCollisionShape.h"
#include "Engine/TerrainSection.h"
#include "Engine/Physics/BulletHeaders.h"
#include "BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"
Log_SetChannel(TerrainCollisionShape);

TerrainSectionCollisionShape::TerrainSectionCollisionShape(const TerrainParameters *pParameters, const TerrainSection *pSectionData)
    : m_boundingBox(pSectionData->GetBoundingBox())
{
    m_pSectionData = pSectionData;
    m_pSectionData->AddRef();

    // various lookup tables
    PHY_ScalarType heightStorageScalarTypes[TERRAIN_HEIGHT_STORAGE_FORMAT_COUNT] =
    {
        PHY_UCHAR,      // TERRAIN_HEIGHT_STORAGE_FORMAT_UINT8
        PHY_SHORT,      // TERRAIN_HEIGHT_STORAGE_FORMAT_UINT16
        PHY_FLOAT       // TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32
    };

    // store some vars
    uint32 pointCount = pSectionData->GetPointCount();
    float heightScale = (pParameters->HeightStorageFormat == TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32) ? 1.0f : static_cast<float>(pParameters->MaxHeight - pParameters->MinHeight);
    btVector3 localScaling((btScalar)pParameters->Scale, (btScalar)pParameters->Scale, (btScalar)1);

    // get height map
    const void *pHeightMapValues;
    uint32 heightMapDataSize;
    uint32 heightMapRowPitch;
    pSectionData->GetRawHeightMapData(&pHeightMapValues, &heightMapDataSize, &heightMapRowPitch);

    // create the bullet shape
    btHeightfieldTerrainShape *pBulletShape = new btHeightfieldTerrainShape(pointCount, pointCount, pHeightMapValues,
                                                                            heightScale, (btScalar)pParameters->MinHeight, (btScalar)pParameters->MaxHeight,
                                                                            2, heightStorageScalarTypes[pParameters->HeightStorageFormat], false);

    // set scale
    pBulletShape->setLocalScaling(localScaling);

    // set bullet shape
    m_pBulletShape = pBulletShape;
}

TerrainSectionCollisionShape::~TerrainSectionCollisionShape()
{
    delete m_pBulletShape;
    m_pSectionData->Release();
}

const Physics::COLLISION_SHAPE_TYPE TerrainSectionCollisionShape::GetType() const
{
    return Physics::COLLISION_SHAPE_TYPE_TERRAIN;
}

const AABox &TerrainSectionCollisionShape::GetLocalBoundingBox() const
{
    return m_boundingBox;
}

bool TerrainSectionCollisionShape::LoadFromData(const void *pData, uint32 dataSize)
{
    return false;
}

bool TerrainSectionCollisionShape::LoadFromStream(ByteStream *pStream, uint32 dataSize)
{
    return false;
}

Physics::CollisionShape *TerrainSectionCollisionShape::CreateScaledShape(const float3 &scale) const
{
    Panic("Should never be called.");
    return nullptr;
}

void TerrainSectionCollisionShape::ApplyShapeTransform(btTransform &worldTransform) const
{

}

void TerrainSectionCollisionShape::ApplyInverseShapeTransform(btTransform &worldTransform) const
{

}

btCollisionShape *TerrainSectionCollisionShape::GetBulletShape() const
{
    return m_pBulletShape;
}

Transform TerrainSectionCollisionShape::GetSectionTransform(const TerrainParameters *pParameters, const TerrainSection *pSectionData)
{
    return Transform(float3(pSectionData->GetBoundingBox().GetCenter().xy(), 0.0f), Quaternion::Identity, float3::One);
}
