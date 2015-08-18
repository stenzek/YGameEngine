#include "Engine/PrecompiledHeader.h"
#include "Engine/Physics/TriangleMeshCollisionShape.h"
#include "Engine/Physics/ScaledTriangleMeshCollisionShape.h"
#include "Engine/Physics/BulletHeaders.h"
Log_SetChannel(TriangleMeshCollisionShape);

#include "btBulletWorldImporter.h"

namespace Physics {

TriangleMeshCollisionShape::TriangleMeshCollisionShape()
    : CollisionShape(),
      m_localBoundingBox(AABox::Zero),
      m_pTriangleMeshShape(NULL)
{

}

TriangleMeshCollisionShape::~TriangleMeshCollisionShape()
{
    btOptimizedBvh *pBvh = m_pTriangleMeshShape->getOptimizedBvh();
    btStridingMeshInterface *pMeshInterface = m_pTriangleMeshShape->getMeshInterface();
    delete m_pTriangleMeshShape;
    delete pBvh;
    delete pMeshInterface;
}

const COLLISION_SHAPE_TYPE TriangleMeshCollisionShape::GetType() const
{
    return COLLISION_SHAPE_TYPE_TRIANGLE_MESH;
}

const AABox &TriangleMeshCollisionShape::GetLocalBoundingBox() const
{
    return m_localBoundingBox;
}

bool TriangleMeshCollisionShape::LoadFromStream(ByteStream *pStream, uint32 dataSize)
{
    byte *pBuffer = new byte[dataSize];
    if (!pStream->Read2(pBuffer, dataSize))
        return false;

    bool result = LoadFromData(pBuffer, dataSize);
    delete[] pBuffer;
    return result;
}

bool TriangleMeshCollisionShape::LoadFromData(const void *pData, uint32 dataSize)
{
    btBulletWorldImporter worldImporter;
    if (!worldImporter.loadFileFromMemory(reinterpret_cast<char *>(const_cast<void *>(pData)), dataSize))
    {
        Log_ErrorPrintf("TriangleMeshCollisionShape::LoadFromData: Failed to parse data");
        worldImporter.deleteAllData();
        return false;
    }
  
    if (worldImporter.getNumCollisionShapes() != 1)
    {
        Log_ErrorPrintf("TriangleMeshCollisionShape::LoadFromData: Bullet file does not contain correct number of collision shapes (%u)", (uint32)worldImporter.getNumCollisionShapes());
        worldImporter.deleteAllData();
        return false;
    }

    btCollisionShape *pCollisionShape = worldImporter.getCollisionShapeByIndex(0);
    if (pCollisionShape->getShapeType() != TRIANGLE_MESH_SHAPE_PROXYTYPE)
    {
        Log_ErrorPrintf("TriangleMeshCollisionShape::LoadFromData: Collision shape is of incorrect type (%i)", pCollisionShape->getShapeType());
        worldImporter.deleteAllData();
        return false;
    }

    m_pTriangleMeshShape = static_cast<btBvhTriangleMeshShape *>(pCollisionShape);

    // get bounds
    btVector3 minBounds, maxBounds;
    m_pTriangleMeshShape->getAabb(btTransform::getIdentity(), minBounds, maxBounds);
    m_localBoundingBox.SetBounds(BulletVector3ToFloat3(minBounds), BulletVector3ToFloat3(maxBounds));
    return true;
}

CollisionShape *TriangleMeshCollisionShape::CreateScaledShape(const float3 &scale) const
{
    return new ScaledTriangleMeshCollisionShape(this, scale);
}

void TriangleMeshCollisionShape::ApplyShapeTransform(btTransform &worldTransform) const
{

}

void TriangleMeshCollisionShape::ApplyInverseShapeTransform(btTransform &worldTransform) const
{

}

btCollisionShape *TriangleMeshCollisionShape::GetBulletShape() const
{
    return m_pTriangleMeshShape;
}


}       // namespace Physics
