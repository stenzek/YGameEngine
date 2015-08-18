#include "Engine/PrecompiledHeader.h"
#include "Engine/Physics/CollisionObject.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/BulletHeaders.h"
//Log_SetChannel(CollisionObject);

namespace Physics {

CollisionObject::CollisionObject(uint32 entityID, const CollisionShape *pCollisionShape, const Transform &transform)
    : PhysicsProxy(entityID),
      m_pCollisionShape(nullptr),
      m_pOriginalCollisionShape(nullptr),
      m_transform(transform)
{
    m_pOriginalCollisionShape = pCollisionShape;
    m_pOriginalCollisionShape->AddRef();

    if (m_transform.GetScale() != float3::One)
    {
        m_pCollisionShape = m_pOriginalCollisionShape->CreateScaledShape(m_transform.GetScale());
    }
    else
    {
        m_pCollisionShape = m_pOriginalCollisionShape;
        m_pCollisionShape->AddRef();
    }
}

CollisionObject::~CollisionObject()
{
    m_pCollisionShape->Release();
    m_pOriginalCollisionShape->Release();
}

void CollisionObject::SetCollisionShape(const CollisionShape *pCollisionShape)
{
    if (pCollisionShape == m_pOriginalCollisionShape)
        return;

    // this has to be kept in order to maintain references for bullet
    const CollisionShape *pOldOriginalCollisionShape = m_pOriginalCollisionShape;
    const CollisionShape *pOldCollisionShape = m_pCollisionShape;

    m_pOriginalCollisionShape = pCollisionShape;
    m_pOriginalCollisionShape->AddRef();

    if (m_transform.GetScale() != float3::One)
    {
        m_pCollisionShape = pCollisionShape->CreateScaledShape(m_transform.GetScale());
    }
    else
    {
        m_pCollisionShape = pCollisionShape;
        m_pCollisionShape->AddRef();
    }

    OnCollisionShapeChanged();

    // cleanup references
    pOldCollisionShape->Release();
    pOldOriginalCollisionShape->Release();
}

void CollisionObject::SetTransform(const Transform &transform)
{
    if (transform == m_transform)
        return;

    if (transform.GetScale() != m_transform.GetScale())
    {
        // scaling from != 1.0 to 1.0
        if (transform.GetScale() == float3::One)
        {
            if (m_transform.GetScale() != float3::One)
            {
                DebugAssert(m_pCollisionShape != m_pOriginalCollisionShape);

                // save current shape
                const CollisionShape *pDeleteCollisionShape = m_pCollisionShape;

                // set original shape
                m_pCollisionShape = m_pOriginalCollisionShape;
                m_pCollisionShape->AddRef();
                OnCollisionShapeChanged();

                // delete old shape
                pDeleteCollisionShape->Release();
            }
        }
        else
        {
            // create new scaled collision shape
            CollisionShape *pNewScaledShape = m_pOriginalCollisionShape->CreateScaledShape(transform.GetScale());
            const CollisionShape *pDeleteCollisionShape = m_pCollisionShape;

            // set new shape
            m_pCollisionShape = pNewScaledShape;
            OnCollisionShapeChanged();

            // delete old shape
            pDeleteCollisionShape->Release();
        }
    }

    m_transform = transform;
    OnTransformChanged();
}

void CollisionObject::UpdateBoundingBox()
{
    btCollisionObject *bulletCollisionObject = GetBulletCollisionObject();

    // if in world, trigger an aabb update for this object
    if (m_pPhysicsWorld != nullptr)
        m_pPhysicsWorld->GetBulletWorld()->updateSingleAabb(bulletCollisionObject);

    // fix up our copy of the aabb
    btVector3 aabbMin, aabbMax;
    m_pCollisionShape->GetBulletShape()->getAabb(bulletCollisionObject->getWorldTransform(), aabbMin, aabbMax);
    m_boundingBox.SetBounds(BulletVector3ToFloat3(aabbMin), BulletVector3ToFloat3(aabbMax));
}

const float CollisionObject::GetFriction() const
{
    return GetBulletCollisionObject()->getFriction();
}

const float CollisionObject::GetRollingFriction() const
{
    return GetBulletCollisionObject()->getRollingFriction();
}

const float3 CollisionObject::GetAnisotropicFriction() const
{
    return BulletVector3ToFloat3(GetBulletCollisionObject()->getAnisotropicFriction());
}

const float CollisionObject::GetRestitution() const
{
    return GetBulletCollisionObject()->getRestitution();
}

void CollisionObject::SetFriction(float friction)
{
    GetBulletCollisionObject()->setFriction(friction);
}

void CollisionObject::SetRollingFriction(float rollingFriction)
{
    GetBulletCollisionObject()->setRollingFriction(rollingFriction);
}

void CollisionObject::SetAnisotropicFriction(const float3 &anisotropicFriction)
{
    GetBulletCollisionObject()->setAnisotropicFriction(Float3ToBulletVector(anisotropicFriction));
}

void CollisionObject::SetRestitution(float restitution)
{
    GetBulletCollisionObject()->setRestitution(restitution);
}

void CollisionObject::ConvertWorldTransformToBulletTransform(const Transform &worldTransform, btTransform *pBulletTransform)
{
    // add shape transform
    pBulletTransform->setOrigin(Float3ToBulletVector(worldTransform.GetPosition()));
    pBulletTransform->setRotation(QuaternionToBulletQuaternion(worldTransform.GetRotation()));
    m_pCollisionShape->ApplyShapeTransform(*pBulletTransform);        
}

void CollisionObject::ConvertBulletTransformToWorldTransform(const btTransform &bulletTransform, Transform *pWorldTransform)
{
    // remove shape transform
    btTransform worldTransform(bulletTransform);
    m_pCollisionShape->ApplyInverseShapeTransform(worldTransform);

    // convert back
    pWorldTransform->SetPosition(BulletVector3ToFloat3(worldTransform.getOrigin()));
    pWorldTransform->SetRotation(BulletQuaternionToQuaternion(worldTransform.getRotation()));
}

}   // namespace Physics
