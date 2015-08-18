#include "Engine/PrecompiledHeader.h"
#include "Engine/Physics/BoxCollisionShape.h"
#include "Engine/Physics/BulletHeaders.h"
Log_SetChannel(BoxCollisionShape);

namespace Physics {

BoxCollisionShape::BoxCollisionShape()
    : m_bounds(AABox::Zero),
      m_center(float3::Zero),
      m_halfExtents(float3::Zero),
      m_pBulletShape(nullptr)
{

}

BoxCollisionShape::BoxCollisionShape(const float3 &boxCenter, const float3 &halfExtents)
{
    m_bounds.SetBounds(boxCenter - halfExtents, boxCenter + halfExtents);
    m_center = boxCenter;
    m_halfExtents = halfExtents;
    m_pBulletShape = new btBoxShape(Float3ToBulletVector(halfExtents));
}

BoxCollisionShape::~BoxCollisionShape()
{
    delete m_pBulletShape;
}

const COLLISION_SHAPE_TYPE BoxCollisionShape::GetType() const
{
    return COLLISION_SHAPE_TYPE_BOX;
}

const AABox & BoxCollisionShape::GetLocalBoundingBox() const
{
    return m_bounds;
}

bool BoxCollisionShape::LoadFromData(const void *pData, uint32 dataSize)
{
    AutoReleasePtr<ByteStream> pStream = ByteStream_CreateReadOnlyMemoryStream(pData, dataSize);
    return LoadFromStream(pStream, dataSize);
}

bool BoxCollisionShape::LoadFromStream(ByteStream *pStream, uint32 dataSize)
{
    // check size
    if (dataSize != (sizeof(float3) + sizeof(float3)))
        return false;

    BinaryReader binaryReader(pStream);
    binaryReader >> m_center >> m_halfExtents;
    if (binaryReader.GetErrorState())
        return false;

    m_bounds.SetBounds(m_center - m_halfExtents, m_center + m_halfExtents);

    delete m_pBulletShape;
    m_pBulletShape = new btBoxShape(SIMDVector3fToBulletVector(m_halfExtents));
    return true;
}

CollisionShape *BoxCollisionShape::CreateScaledShape(const float3 &scale) const
{
    // straight up create a new shape, memory's gonna be the same anyway
    return new BoxCollisionShape(m_center, SIMDVector3f(m_halfExtents) * SIMDVector3f(scale));
}

void BoxCollisionShape::ApplyShapeTransform(btTransform &worldTransform) const
{
    // if we have a nonzero center, this has to be transformed prior to the world transform
    if (m_center.SquaredLength() > 0.0f)
        //worldTransform = worldTransform * btTransform(btMatrix3x3::getIdentity(), Float3ToBulletVector(m_center));
        worldTransform.setOrigin(worldTransform.getOrigin() + Float3ToBulletVector(m_center));
}

void BoxCollisionShape::ApplyInverseShapeTransform(btTransform &worldTransform) const
{
    // if we have a nonzero center, this has to be transformed prior to the world transform
    if (m_center.SquaredLength() > 0.0f)
        //worldTransform = worldTransform * btTransform(btMatrix3x3::getIdentity(), Float3ToBulletVector(m_center));
        worldTransform.setOrigin(worldTransform.getOrigin() - Float3ToBulletVector(m_center));
}

btCollisionShape *BoxCollisionShape::GetBulletShape() const
{
    DebugAssert(m_pBulletShape != nullptr);
    return m_pBulletShape;
}


}       // namespace Physics
