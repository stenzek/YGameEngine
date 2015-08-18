#include "Engine/PrecompiledHeader.h"
#include "Engine/Physics/KinematicObject.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/BulletHeaders.h"
//Log_SetChannel(KinematicObject);

namespace Physics {

class KinematicObject::MotionState : public btMotionState
{
public:
    BT_DECLARE_ALIGNED_ALLOCATOR();

    MotionState(const btTransform &transform) : m_transform(transform) {}

    virtual void getWorldTransform(btTransform& worldTrans) const override
    {
        worldTrans = m_transform;
    }

    virtual void setWorldTransform(const btTransform& worldTrans) override
    {
        m_transform = worldTrans;
    }

private:
    btTransform m_transform;
};


KinematicObject::KinematicObject(uint32 entityID, const CollisionShape *pCollisionShape, const Transform &transform)
    : CollisionObject(entityID, pCollisionShape, transform),
      m_pBulletRigidBody(nullptr)
{
    // fix up transform
    btTransform bulletTransform;
    ConvertWorldTransformToBulletTransform(m_transform, &bulletTransform);

    // create motion state
    m_pMotionState = new MotionState(bulletTransform);

    // and RB construction info
    btRigidBody::btRigidBodyConstructionInfo rbConstructionInfo(0.0f, m_pMotionState, m_pCollisionShape->GetBulletShape());

    // create bullet object
    m_pBulletRigidBody = new btRigidBody(rbConstructionInfo);
    m_pBulletRigidBody->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
    m_pBulletRigidBody->forceActivationState(DISABLE_SIMULATION);
    m_pBulletRigidBody->setUserPointer(reinterpret_cast<void *>(this));

    // update our aabb from bullet
    UpdateBoundingBox();
}

KinematicObject::~KinematicObject()
{
    delete m_pBulletRigidBody;
    delete m_pMotionState;
}

btCollisionObject *Physics::KinematicObject::GetBulletCollisionObject() const
{
    return static_cast<btCollisionObject *>(m_pBulletRigidBody);
}

void Physics::KinematicObject::OnCollisionShapeChanged()
{
    // fix up transform
    btTransform bulletTransform;
    ConvertWorldTransformToBulletTransform(m_transform, &bulletTransform);

    // update bullet
    m_pBulletRigidBody->setCollisionShape(m_pCollisionShape->GetBulletShape());
    m_pBulletRigidBody->setWorldTransform(bulletTransform);
    m_pMotionState->setWorldTransform(bulletTransform);

    // update aabb
    UpdateBoundingBox();
}

void Physics::KinematicObject::OnTransformChanged()
{
    // fix up transform
    btTransform bulletTransform;
    ConvertWorldTransformToBulletTransform(m_transform, &bulletTransform);

    // update bullet, motion state only, since it will get pulled back next iteration
    m_pMotionState->setWorldTransform(bulletTransform);

    // fix up our copy of the aabb manually, since the collision object still contains the old transform
    btVector3 aabbMin, aabbMax;
    m_pCollisionShape->GetBulletShape()->getAabb(bulletTransform, aabbMin, aabbMax);
    m_boundingBox.SetBounds(BulletVector3ToFloat3(aabbMin), BulletVector3ToFloat3(aabbMax));
}

void KinematicObject::OnAddedToPhysicsWorld(PhysicsWorld *pPhysicsWorld)
{
    PhysicsProxy::OnAddedToPhysicsWorld(pPhysicsWorld);
    pPhysicsWorld->GetBulletWorld()->addRigidBody(m_pBulletRigidBody);
}

void KinematicObject::OnRemovedFromPhysicsWorld(PhysicsWorld *pPhysicsWorld)
{
    pPhysicsWorld->GetBulletWorld()->removeRigidBody(m_pBulletRigidBody);
    PhysicsProxy::OnRemovedFromPhysicsWorld(pPhysicsWorld);
}

}   // namespace Physics
