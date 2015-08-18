#include "Engine/PrecompiledHeader.h"
#include "Engine/Physics/StaticObject.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/BulletHeaders.h"
//Log_SetChannel(StaticObject);

namespace Physics {

StaticObject::StaticObject(uint32 entityID, const CollisionShape *pCollisionShape, const Transform &transform)
    : CollisionObject(entityID, pCollisionShape, transform),
      m_pBulletCollisionObject(nullptr)
{
    // create bullet object
    m_pBulletCollisionObject = new btCollisionObject();
    m_pBulletCollisionObject->setCollisionShape(m_pCollisionShape->GetBulletShape());
    m_pBulletCollisionObject->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
    m_pBulletCollisionObject->forceActivationState(DISABLE_SIMULATION);
    m_pBulletCollisionObject->setUserPointer(reinterpret_cast<void *>(this));

    // bullet doesn't zero these, so we do
    m_pBulletCollisionObject->setInterpolationLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
    m_pBulletCollisionObject->setInterpolationAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
    m_pBulletCollisionObject->setInterpolationWorldTransform(btTransform::getIdentity());

    // fix up transform
    btTransform bulletTransform;
    ConvertWorldTransformToBulletTransform(m_transform, &bulletTransform);
    m_pBulletCollisionObject->setWorldTransform(bulletTransform);

    // update our aabb from bullet
    UpdateBoundingBox();
}

StaticObject::~StaticObject()
{
    delete m_pBulletCollisionObject;
}

void StaticObject::OnAddedToPhysicsWorld(PhysicsWorld *pPhysicsWorld)
{
    PhysicsProxy::OnAddedToPhysicsWorld(pPhysicsWorld);
    pPhysicsWorld->GetBulletWorld()->addCollisionObject(m_pBulletCollisionObject);
}

void StaticObject::OnRemovedFromPhysicsWorld(PhysicsWorld *pPhysicsWorld)
{
    pPhysicsWorld->GetBulletWorld()->removeCollisionObject(m_pBulletCollisionObject);
    PhysicsProxy::OnRemovedFromPhysicsWorld(pPhysicsWorld);
}

btCollisionObject *Physics::StaticObject::GetBulletCollisionObject() const
{
    return m_pBulletCollisionObject;
}

void Physics::StaticObject::OnCollisionShapeChanged()
{
    // fix up transform
    btTransform bulletTransform;
    ConvertWorldTransformToBulletTransform(m_transform, &bulletTransform);

    // update bullet
    m_pBulletCollisionObject->setCollisionShape(m_pCollisionShape->GetBulletShape());
    m_pBulletCollisionObject->setWorldTransform(bulletTransform);

    // update aabb
    UpdateBoundingBox();
}

void Physics::StaticObject::OnTransformChanged()
{
    // fix up transform
    btTransform bulletTransform;
    ConvertWorldTransformToBulletTransform(m_transform, &bulletTransform);

    // update bullet
    m_pBulletCollisionObject->setWorldTransform(bulletTransform);

    // update aabb
    UpdateBoundingBox();
}

}   // namespace Physics
