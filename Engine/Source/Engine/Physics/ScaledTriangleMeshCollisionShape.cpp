#include "Engine/PrecompiledHeader.h"
#include "Engine/Physics/ScaledTriangleMeshCollisionShape.h"
#include "Engine/Physics/BulletHeaders.h"
Log_SetChannel(TriangleMeshCollisionShape);

namespace Physics {

ScaledTriangleMeshCollisionShape::ScaledTriangleMeshCollisionShape(const TriangleMeshCollisionShape *pParentShape, const float3 &scale)
    : CollisionShape(),
      m_Bounds(AABox::Zero),
      m_pParentShape(NULL),
      m_scale(scale),
      m_pTriangleMeshShape(NULL),
      m_pScaledTriangleMeshShape(NULL)
{
    // set
    m_pParentShape = pParentShape;
    m_pParentShape->AddRef();

    // create scaled shape
    m_pTriangleMeshShape = m_pParentShape->m_pTriangleMeshShape;
    m_pScaledTriangleMeshShape = new btScaledBvhTriangleMeshShape(m_pTriangleMeshShape, Float3ToBulletVector(scale));

    // determine bounds
    btVector3 minBounds, maxBounds;
    m_pTriangleMeshShape->getAabb(btTransform::getIdentity(), minBounds, maxBounds);
    m_Bounds.SetBounds(BulletVector3ToFloat3(minBounds), BulletVector3ToFloat3(maxBounds));
}

ScaledTriangleMeshCollisionShape::~ScaledTriangleMeshCollisionShape()
{
    delete m_pScaledTriangleMeshShape;

    DebugAssert(m_pParentShape != NULL);
    m_pParentShape->Release();
}

const COLLISION_SHAPE_TYPE ScaledTriangleMeshCollisionShape::GetType() const
{
    return COLLISION_SHAPE_TYPE_SCALED_TRIANGLE_MESH;
}

const AABox &ScaledTriangleMeshCollisionShape::GetLocalBoundingBox() const
{
    return m_Bounds;
}

bool ScaledTriangleMeshCollisionShape::LoadFromData(const void *pData, uint32 dataSize)
{
    return false;
}

bool ScaledTriangleMeshCollisionShape::LoadFromStream(ByteStream *pStream, uint32 dataSize)
{
    return false;
}

CollisionShape *ScaledTriangleMeshCollisionShape::CreateScaledShape(const float3 &scale) const
{
    return new ScaledTriangleMeshCollisionShape(m_pParentShape, scale);
}

void ScaledTriangleMeshCollisionShape::ApplyShapeTransform(btTransform &worldTransform) const
{

}

void ScaledTriangleMeshCollisionShape::ApplyInverseShapeTransform(btTransform &worldTransform) const
{

}

btCollisionShape *ScaledTriangleMeshCollisionShape::GetBulletShape() const
{
    return static_cast<btCollisionShape *>(m_pScaledTriangleMeshShape);
}


}       // namespace Physics
