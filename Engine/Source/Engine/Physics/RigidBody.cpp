#include "Engine/PrecompiledHeader.h"
#include "Engine/Physics/RigidBody.h"
#include "Engine/Physics/CollisionShape.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/BulletHeaders.h"

namespace Physics {

class RigidBody::MotionState : public btMotionState
{
public:
    BT_DECLARE_ALIGNED_ALLOCATOR();

    MotionState(const btTransform &transform, RigidBody *pRigidBody)
        : m_transform(transform),
          m_pRigidBody(pRigidBody)
    {

    }

    virtual void getWorldTransform(btTransform& worldTrans) const override
    {
        worldTrans = m_transform;
    }

    virtual void setWorldTransform(const btTransform& worldTrans)
    {
        m_transform = worldTrans;

        DebugAssert(m_pRigidBody->IsInWorld());
        m_pRigidBody->GetPhysicsWorld()->QueueObjectSynchronization(m_pRigidBody);
    }

    void setWorldTransformNoCallback(const btTransform& worldTrans)
    {
        m_transform = worldTrans;
    }

protected:
    btTransform m_transform;
    RigidBody *m_pRigidBody;
};

RigidBody::RigidBody(uint32 entityID, const CollisionShape *pCollisionShape, const Transform &transform)
    : CollisionObject(entityID, pCollisionShape, transform),
      m_isMovable(false),
      m_mass(1.0f),
      m_pMotionState(nullptr),
      m_pBulletRigidBody(nullptr),
      m_updateTransformCallbackFunction(nullptr),
      m_pUpdateTransformCallbackUserData(nullptr)
{
    // only convex objects can be movable rigid bodies
    btCollisionShape *pBulletCollisionShape = m_pCollisionShape->GetBulletShape();
    btVector3 localInertia(0.0f, 0.0f, 0.0f);
    m_isMovable = pBulletCollisionShape->isConvex();
    if (m_isMovable)
        pBulletCollisionShape->calculateLocalInertia(m_mass, localInertia);

    // fix up it's transform
    btTransform bulletTransform;
    ConvertWorldTransformToBulletTransform(m_transform, &bulletTransform);

    // create motion state
    m_pMotionState = new MotionState(bulletTransform, this);

    // create construction info
    btRigidBody::btRigidBodyConstructionInfo rbConstructionInfo(m_mass, m_pMotionState, pBulletCollisionShape, localInertia);
    rbConstructionInfo.m_friction = 0.5f;
    rbConstructionInfo.m_rollingFriction = 0.0f;

    // create the rigid body
    m_pBulletRigidBody = new btRigidBody(rbConstructionInfo);
    m_pBulletRigidBody->setUserPointer(reinterpret_cast<void *>(this));

    // update bounding box from object
    UpdateBoundingBox();
}

RigidBody::~RigidBody()
{
    delete m_pBulletRigidBody;
    delete m_pMotionState;
}

btCollisionObject *RigidBody::GetBulletCollisionObject() const
{
    return static_cast<btCollisionObject *>(m_pBulletRigidBody);
}

void RigidBody::OnCollisionShapeChanged()
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

    // clear forces and stuff
    ResetRigidBodyState();
}

void RigidBody::OnTransformChanged()
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

    // clear forces and stuff
    ResetRigidBodyState();
}

void RigidBody::OnAddedToPhysicsWorld(PhysicsWorld *pPhysicsWorld)
{
    PhysicsProxy::OnAddedToPhysicsWorld(pPhysicsWorld);
    pPhysicsWorld->GetBulletWorld()->addRigidBody(m_pBulletRigidBody);
}

void RigidBody::OnRemovedFromPhysicsWorld(PhysicsWorld *pPhysicsWorld)
{
    pPhysicsWorld->GetBulletWorld()->removeRigidBody(m_pBulletRigidBody);
    PhysicsProxy::OnRemovedFromPhysicsWorld(pPhysicsWorld);
}

void RigidBody::OnSynchronize()
{
    // retrieve bullet transform
    btTransform bulletTransform;
    m_pMotionState->getWorldTransform(bulletTransform);

    // reverse the object's transform and update our transform copy
    ConvertBulletTransformToWorldTransform(bulletTransform, &m_transform);

    // update our aabb
    btVector3 aabbMin, aabbMax;
    m_pCollisionShape->GetBulletShape()->getAabb(bulletTransform, aabbMin, aabbMax);
    m_boundingBox.SetBounds(BulletVector3ToFloat3(aabbMin), BulletVector3ToFloat3(aabbMax));

    // invoke game callback
    if (m_updateTransformCallbackFunction != nullptr)
        m_updateTransformCallbackFunction(m_pUpdateTransformCallbackUserData, m_transform);
}

void RigidBody::ResetRigidBodyState()
{
    btCollisionShape *pBulletCollisionShape = m_pCollisionShape->GetBulletShape();

    btVector3 localInertia(0.0f, 0.0f, 0.0f);
    m_isMovable = pBulletCollisionShape->isConvex();
    if (m_isMovable)
        pBulletCollisionShape->calculateLocalInertia(m_mass, localInertia);

    // before setting it, for non-movable objects, kill all velocity
    if (!m_isMovable)
        m_pBulletRigidBody->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));

    m_pBulletRigidBody->setMassProps(m_mass, localInertia);

    // force it active, that way it'll settle in its new position
    m_pBulletRigidBody->activate();
}

void RigidBody::SetMass(float mass)
{
    m_mass = mass;
    ResetRigidBodyState();
}

void RigidBody::SetCallbackFunction(UpdateTransformCallbackFunction callback, void *pUserData)
{
    m_updateTransformCallbackFunction = callback;
    m_pUpdateTransformCallbackUserData = pUserData;
}

void RigidBody::ApplyCentralImpulse(const float3 &direction)
{
    if (!IsInWorld())
        return;

    m_pBulletRigidBody->applyCentralImpulse(Float3ToBulletVector(direction));
    m_pBulletRigidBody->activate();
}

void RigidBody::ResetForces()
{
    if (!IsInWorld())
        return;

    m_pBulletRigidBody->clearForces();
}

void RigidBody::ResetVelocity()
{
    m_pBulletRigidBody->setLinearVelocity(btVector3(0.0f, 0.0f, 0.0f));
    m_pBulletRigidBody->setAngularVelocity(btVector3(0.0f, 0.0f, 0.0f));
}

}       // namespace Physics
