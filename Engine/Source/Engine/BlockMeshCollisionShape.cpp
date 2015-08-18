#include "Engine/PrecompiledHeader.h"
#include "Engine/BlockMeshCollisionShape.h"
#include "Engine/BlockMesh.h"
#include "Engine/Physics/BulletHeaders.h"

ATTRIBUTE_ALIGNED16(class) btBlockMeshCollisionShape : public btConcaveShape
{
public:
    BT_DECLARE_ALIGNED_ALLOCATOR();

    btBlockMeshCollisionShape(const BlockMeshVolume *pBlockVolume, const btVector3 &scaling, const AABox &boundingBox)
        : btConcaveShape(),
          m_pBlockVolume(pBlockVolume),
          m_localScaling(scaling),
          m_boundingBox(boundingBox)
    {
        m_shapeType = CUSTOM_CONCAVE_SHAPE_TYPE;
    }

    virtual ~btBlockMeshCollisionShape()
    {

    }

    virtual void processAllTriangles(btTriangleCallback* callback,const btVector3& aabbMin,const btVector3& aabbMax) const;

    virtual void getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const;

    virtual void setLocalScaling(const btVector3& scaling) { m_localScaling = scaling; }
    virtual const btVector3& getLocalScaling() const { return m_localScaling; }

    virtual void calculateLocalInertia(btScalar mass,btVector3& inertia) const;

    virtual const char* getName() const { return "btBlockMeshCollisionShape"; }

private:
    const BlockMeshVolume *m_pBlockVolume;
    btVector3 m_localScaling;
    AABox m_boundingBox;
};

BlockMeshCollisionShape::BlockMeshCollisionShape(const BlockMesh *pBlockMesh)
    : m_pBlockMesh(pBlockMesh),
      m_pBlockVolume(&pBlockMesh->GetMeshVolume(0)),
      m_pOriginalShape(NULL),
      m_pBulletShape(NULL),
      m_scale(float3::One),
      m_boundingBox(pBlockMesh->GetMeshVolume(0).CalculateActiveBoundingBox())
{
    // fixme: shared data!
    m_pBulletShape = new btBlockMeshCollisionShape(&pBlockMesh->GetMeshVolume(0), btVector3(1.0f, 1.0f, 1.0f), m_boundingBox);
}

BlockMeshCollisionShape::~BlockMeshCollisionShape()
{
    delete m_pBulletShape;

    if (m_pOriginalShape != NULL)
        m_pBlockMesh->Release();
}

const Physics::COLLISION_SHAPE_TYPE BlockMeshCollisionShape::GetType() const
{
    return Physics::COLLISION_SHAPE_TYPE_BLOCK_MESH;
}

const AABox &BlockMeshCollisionShape::GetLocalBoundingBox() const
{
    return m_boundingBox;
}

bool BlockMeshCollisionShape::LoadFromData(const void *pData, uint32 dataSize)
{
    return false;
}

bool BlockMeshCollisionShape::LoadFromStream(ByteStream *pStream, uint32 dataSize)
{
    return false;
}

Physics::CollisionShape *BlockMeshCollisionShape::CreateScaledShape(const float3 &scale) const
{
    const BlockMeshCollisionShape *pOriginalShape = (m_pOriginalShape != NULL) ? m_pOriginalShape : this;

    if (scale == float3::One)
    {
        pOriginalShape->AddRef();
        return const_cast<BlockMeshCollisionShape *>(pOriginalShape);
    }

    BlockMeshCollisionShape *pScaledShape = new BlockMeshCollisionShape(m_pBlockMesh);
    pScaledShape->m_pOriginalShape = pOriginalShape;
    pScaledShape->m_pBulletShape->setLocalScaling(Physics::Float3ToBulletVector(scale));
    pScaledShape->m_scale = scale;
    return pScaledShape;
}

void BlockMeshCollisionShape::ApplyShapeTransform(btTransform &worldTransform) const
{

}

void BlockMeshCollisionShape::ApplyInverseShapeTransform(btTransform &worldTransform) const
{

}

btCollisionShape *BlockMeshCollisionShape::GetBulletShape() const
{
    return static_cast<btCollisionShape *>(m_pBulletShape);
}

void btBlockMeshCollisionShape::processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const
{
    btVector3 localMinBounds(m_localScaling * aabbMin);
    btVector3 localMaxBounds(m_localScaling * aabbMax);
    m_pBlockVolume->EnumerateTrianglesIntersectingBox(AABox(Physics::BulletVector3ToFloat3(localMinBounds), Physics::BulletVector3ToFloat3(localMaxBounds)), [callback](const float3 vertices[3])
    {
        btVector3 bulletVertices[3];
        bulletVertices[0] = Physics::Float3ToBulletVector(vertices[0]);
        bulletVertices[1] = Physics::Float3ToBulletVector(vertices[1]);
        bulletVertices[2] = Physics::Float3ToBulletVector(vertices[2]);
        callback->processTriangle(bulletVertices, 0, 0);
    });
}

void btBlockMeshCollisionShape::getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
{
    AABox transformedBoundingBox(Physics::BulletTransformToTransform(t, float3::One).TransformBoundingBox(m_boundingBox));
    aabbMin = Physics::Float3ToBulletVector(transformedBoundingBox.GetMinBounds()) * m_localScaling;
    aabbMax = Physics::Float3ToBulletVector(transformedBoundingBox.GetMaxBounds()) * m_localScaling;
}

void btBlockMeshCollisionShape::calculateLocalInertia(btScalar mass, btVector3& inertia) const
{
    //moving concave objects not supported
    Panic("called");
    inertia.setValue(btScalar(0.), btScalar(0.), btScalar(0.));
}
