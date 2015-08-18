#include "Engine/PrecompiledHeader.h"
#include "Engine/Physics/SphereCollisionShape.h"
#include "Engine/Physics/BulletHeaders.h"
Log_SetChannel(BoxCollisionShape);

namespace Physics {
    
SphereCollisionShape::SphereCollisionShape()
    : m_bounds(AABox::Zero),
      m_center(float3::Zero),
      m_radius(0.0f),      
      m_pBulletShape(nullptr)
{

}

SphereCollisionShape::SphereCollisionShape(const float3 &center, const float radius)
{
    m_center = center;
    m_radius = radius;
    m_bounds.SetBounds(float3(-radius, -radius, -radius) + center, float3(radius, radius, radius) + center);
    m_pBulletShape = new btSphereShape(m_radius);
}

SphereCollisionShape::~SphereCollisionShape()
{
    delete m_pBulletShape;
}

const COLLISION_SHAPE_TYPE SphereCollisionShape::GetType() const
{
    return COLLISION_SHAPE_TYPE_SPHERE;
}

const AABox &SphereCollisionShape::GetLocalBoundingBox() const
{
    return m_bounds;
}

bool SphereCollisionShape::LoadFromData(const void *pData, uint32 dataSize)
{
    AutoReleasePtr<ByteStream> pStream = ByteStream_CreateReadOnlyMemoryStream(pData, dataSize);
    return LoadFromStream(pStream, dataSize);
}

bool SphereCollisionShape::LoadFromStream(ByteStream *pStream, uint32 dataSize)
{
    // check size
    if (dataSize != (sizeof(float3) + sizeof(float)))
        return false;

    BinaryReader binaryReader(pStream);
    binaryReader >> m_center >> m_radius;
    if (binaryReader.GetErrorState())
        return false;

    // calculate bounds
    m_bounds.SetBounds(float3(-m_radius, -m_radius, -m_radius) + m_center, float3(m_radius, m_radius, m_radius) + m_center);

    delete m_pBulletShape;
    m_pBulletShape = new btSphereShape(m_radius);
    return true;
}

CollisionShape *SphereCollisionShape::CreateScaledShape(const float3 &scale) const
{
    // straight up create a new shape, memory's gonna be the same anyway
    return new SphereCollisionShape(m_center, m_radius * Max(scale.x, Max(scale.y, scale.z)));
}

void SphereCollisionShape::ApplyShapeTransform(btTransform &worldTransform) const
{
    // if we have a nonzero center, this has to be transformed prior to the world transform
    if (m_center.SquaredLength() > 0.0f)
        //worldTransform = worldTransform * btTransform(btMatrix3x3::getIdentity(), Float3ToBulletVector(m_center));
        worldTransform.setOrigin(worldTransform.getOrigin() + Float3ToBulletVector(m_center));
}

void SphereCollisionShape::ApplyInverseShapeTransform(btTransform &worldTransform) const
{
    // if we have a nonzero center, this has to be transformed prior to the world transform
    if (m_center.SquaredLength() > 0.0f)
        //worldTransform = worldTransform * btTransform(btMatrix3x3::getIdentity(), Float3ToBulletVector(m_center));
        worldTransform.setOrigin(worldTransform.getOrigin() - Float3ToBulletVector(m_center));
}

btCollisionShape *SphereCollisionShape::GetBulletShape() const
{
    DebugAssert(m_pBulletShape != nullptr);
    return m_pBulletShape;
}

}       // namespace Physics


