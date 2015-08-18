#include "Engine/PrecompiledHeader.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/PhysicsProxy.h"
#include "Engine/Physics/BulletHeaders.h"
#include "Engine/Physics/CollisionObject.h"
#include "Engine/EngineCVars.h"
Log_SetChannel(PhysicsWorld);

namespace Physics {

PhysicsWorld::PhysicsWorld()
    : m_gravity(0.0f, 0.0f, -10.0f),
      m_pBulletWorld(nullptr),
      m_pBulletCollisionConfiguration(nullptr),
      m_pBulletCollisionDispatcher(nullptr),
      m_pBulletBroadphase(nullptr),
      m_pBulletGhostPairCallback(nullptr)
{
    m_pBulletCollisionConfiguration = new btDefaultCollisionConfiguration();
    m_pBulletCollisionDispatcher = new btCollisionDispatcher(m_pBulletCollisionConfiguration);
    m_pBulletBroadphase = new btDbvtBroadphase();
    m_pBulletSolver = new btSequentialImpulseConstraintSolver();
    m_pBulletWorld = new btDiscreteDynamicsWorld(m_pBulletCollisionDispatcher, m_pBulletBroadphase, m_pBulletSolver, m_pBulletCollisionConfiguration);
    m_pBulletWorld->setGravity(Float3ToBulletVector(m_gravity));
    m_pBulletWorld->setForceUpdateAllAabbs(false);      // why the hell is this on by default?

    m_pBulletGhostPairCallback = new btGhostPairCallback();
    m_pBulletBroadphase->getOverlappingPairCache()->setInternalGhostPairCallback(m_pBulletGhostPairCallback);
}

PhysicsWorld::~PhysicsWorld()
{
    /*
    // remove everything in queues
    for (uint32 i = 0; i < m_addQueue.GetSize(); i++)
    {
        CollisionObject *pCollisionObject = m_addQueue[i];
        pCollisionObject->m_pPhysicsWorld = NULL;

        for (uint32 j = 0; j < m_removeQueue.GetSize(); j++)
        {
            if (m_removeQueue[j] == pCollisionObject)
            {
                m_removeQueue.OrderedRemove(j);
                break;
            }
        }

        pCollisionObject->Release();
    }
    m_addQueue.Obliterate();
    for (uint32 i = 0; i < m_removeQueue.GetSize(); i++)
    {
        CollisionObject *pCollisionObject = m_removeQueue[i];

        CollisionObjectList::Iterator itr;
        for (itr = m_collisionObjects.Begin(); !itr.AtEnd(); itr.Forward())
        {
            if (*itr == pCollisionObject)
                break;
        }

        Assert(!itr.AtEnd());
        m_collisionObjects.Erase(itr);

        pCollisionObject->OnRemovedFromPhysicsWorld(this);

        pCollisionObject->m_pPhysicsWorld = NULL;
        pCollisionObject->Release();
    }
    m_removeQueue.Obliterate();
    */

    // remove everything in world
    while (m_collisionObjects.GetSize() > 0)
    {
        PhysicsProxy *pPhysicsProxy = m_collisionObjects.PopBack();

        pPhysicsProxy->OnRemovedFromPhysicsWorld(this);
        pPhysicsProxy->Release();
    }

    // the synch queue can just be wiped
    m_synchronizationQueue.Obliterate();

    // cleanup bullet
    delete m_pBulletWorld;
    delete m_pBulletSolver;
    delete m_pBulletBroadphase;
    delete m_pBulletCollisionDispatcher;
    delete m_pBulletCollisionConfiguration;
    delete m_pBulletGhostPairCallback;
}

void PhysicsWorld::SetGravity(const float3 &gravity)
{
    m_gravity = gravity;

    // update bullet
    m_pBulletWorld->setGravity(Float3ToBulletVector(m_gravity));
}

void PhysicsWorld::AddObject(PhysicsProxy *pPhysicsProxy)
{
    /*
    DebugAssert(pCollisionObject->m_pPhysicsWorld == NULL);

    uint32 i;
    for (i = 0; i < m_addQueue.GetSize(); i++)
    {
        if (m_addQueue[i] == pCollisionObject)
            break;
    }
    if (i == m_addQueue.GetSize())
    {
        pCollisionObject->AddRef();
        m_addQueue.Add(pCollisionObject);
    }*/

    DebugAssert(m_collisionObjects.IndexOf(pPhysicsProxy) < 0);
    m_collisionObjects.Add(pPhysicsProxy);
    pPhysicsProxy->OnAddedToPhysicsWorld(this);
}

void PhysicsWorld::RemoveObject(PhysicsProxy *pPhysicsProxy)
{
    /*
    DebugAssert(pCollisionObject->m_pPhysicsWorld == this);

    uint32 i;
    for (i = 0; i < m_removeQueue.GetSize(); i++)
    {
        if (m_removeQueue[i] == pCollisionObject)
            break;
    }
    if (i == m_removeQueue.GetSize())
        m_removeQueue.Add(pCollisionObject);*/

    DebugAssert(pPhysicsProxy->GetPhysicsWorld() == this);

    int32 arrayIndex = m_collisionObjects.IndexOf(pPhysicsProxy);
    DebugAssert(arrayIndex >= 0);
    m_collisionObjects.OrderedRemove(arrayIndex);

    pPhysicsProxy->OnRemovedFromPhysicsWorld(this);
}

void PhysicsWorld::QueueObjectSynchronization(PhysicsProxy *pPhysicsProxy)
{
    // bullet can have more than one simulation run in a frame, therefore this can actually be called >1 times
    if (m_synchronizationQueue.Contains(pPhysicsProxy))
        return;

    m_synchronizationQueue.Add(pPhysicsProxy);
}

void PhysicsWorld::UpdateAsync(float timeSinceLastUpdate)
{
    const float fixedTimeStep = 1.0f / CVars::physics_fps.GetFloat();
    m_pBulletWorld->stepSimulation(timeSinceLastUpdate, 10, fixedTimeStep);
}

void PhysicsWorld::Update(float timeSinceLastUpdate)
{
    // sync objects
    for (uint32 i = 0; i < m_synchronizationQueue.GetSize(); i++)
        m_synchronizationQueue[i]->OnSynchronize();
    m_synchronizationQueue.Clear();
}

bool PhysicsWorld::RayCast(const Ray &ray) const
{
    const PhysicsProxy *pContactObject;
    float3 contactNormal;
    float3 contactPoint;

    return RayCast(ray, &pContactObject, &contactNormal, &contactPoint);
}

bool PhysicsWorld::RayCast(const Ray &ray, const PhysicsProxy **ppContactObject, float3 *pContactNormal, float3 *pContactPoint) const
{
    btVector3 rayFrom(Float3ToBulletVector(ray.GetOrigin()));
    btVector3 rayTo(Float3ToBulletVector(ray.GetEnd()));

    btCollisionWorld::ClosestRayResultCallback result(rayFrom, rayTo);
    m_pBulletWorld->rayTest(rayFrom, rayTo, result);

    if (!result.hasHit())
        return false;

    *ppContactObject = reinterpret_cast<const PhysicsProxy *>(result.m_collisionObject->getUserPointer());
    *pContactNormal = BulletVector3ToFloat3(result.m_hitNormalWorld);
    *pContactPoint = BulletVector3ToFloat3(result.m_hitPointWorld);
    return true;
}

bool PhysicsWorld::TestAABoxIntersection(const AABox &box) const
{
    const PhysicsProxy *pContactObject;
    float3 contactNormal;
    float3 contactPoint;

    return TestAABoxIntersection(box, &pContactObject, &contactNormal, &contactPoint);
}

bool PhysicsWorld::TestAABoxIntersection(const AABox &box, const PhysicsProxy **ppContactObject, float3 *pContactNormal, float3 *pContactPoint) const
{
    /*
    struct ResultCallback : public btBroadphaseAabbCallback
    {

        ResultCallback(btCollisionWorld *collisionWorld, const AABox &box)
            : pCollisionWorld(collisionWorld),
              bulletBoxShape(Float3ToBulletVector(box.GetExtents()) * 0.5f)
        {
            bulletCollisionObject.setCollisionShape(&bulletBoxShape);
            bulletCollisionObject.setWorldTransform(btTransform(btQuaternion::getIdentity(), Float3ToBulletVector(box.GetCenter())));
        }

        virtual bool process(const btBroadphaseProxy* proxy)
        {
            // find the bullet object
            btCollisionObject *pBulletCollisionObject = reinterpret_cast<btCollisionObject *>(proxy->m_clientObject);
            DebugAssert(pBulletCollisionObject != nullptr);

            // collide the two objects

        }

        btCollisionWorld *pCollisionWorld;
        btBoxShape bulletBoxShape;
        btCollisionObject bulletCollisionObject;
        btCollisionWorld::ClosestConvexResultCallback result;
    };*/

    // create a temporary bullet shape on the stack
    // construct a bullet transform for the box (ie center of it)
    float3 boxExtents(box.GetExtents());
    float3 boxCenter(box.GetCenter());
    btBoxShape bulletBoxShape(Float3ToBulletVector(boxExtents) * 0.5f);
    btTransform bulletTransform(btQuaternion::getIdentity(), Float3ToBulletVector(boxCenter));

    // sweep it against the world
    btCollisionWorld::ClosestConvexResultCallback result(Float3ToBulletVector(boxCenter), Float3ToBulletVector(boxCenter));
    m_pBulletWorld->convexSweepTest(&bulletBoxShape, bulletTransform, bulletTransform, result);

    // got a result?
    if (!result.hasHit())
        return false;

    // store results
    *ppContactObject = reinterpret_cast<const PhysicsProxy *>(result.m_hitCollisionObject->getUserPointer());
    *pContactNormal = BulletVector3ToFloat3(result.m_hitNormalWorld);
    *pContactPoint = BulletVector3ToFloat3(result.m_hitPointWorld);
    return true;
}

bool PhysicsWorld::TestSphereIntersection(const Sphere &sphere) const
{
    const PhysicsProxy *pContactObject;
    float3 contactNormal;
    float3 contactPoint;

    return TestSphereIntersection(sphere, &pContactObject, &contactNormal, &contactPoint);
}

bool PhysicsWorld::TestSphereIntersection(const Sphere &sphere, const PhysicsProxy **ppContactObject, float3 *pContactNormal, float3 *pContactPoint) const
{
    // create a temporary bullet shape on the stack
    // construct a bullet transform for the box (ie center of it)
    btSphereShape bulletBoxShape(sphere.GetRadius());
    btTransform bulletTransform(btQuaternion::getIdentity(), Float3ToBulletVector(sphere.GetCenter()));

    // sweep it against the world
    btCollisionWorld::ClosestConvexResultCallback result(Float3ToBulletVector(sphere.GetCenter()), Float3ToBulletVector(sphere.GetCenter()));
    m_pBulletWorld->convexSweepTest(&bulletBoxShape, bulletTransform, bulletTransform, result);

    // got a result?
    if (!result.hasHit())
        return false;

    // store results
    *ppContactObject = reinterpret_cast<const PhysicsProxy *>(result.m_hitCollisionObject->getUserPointer());
    *pContactNormal = BulletVector3ToFloat3(result.m_hitNormalWorld);
    *pContactPoint = BulletVector3ToFloat3(result.m_hitPointWorld);
    return true;
}

bool PhysicsWorld::SweepBox(const float3 &boxHalfExtents, const float3 &from, const float3 &to, const PhysicsProxy **ppContactObject, float3 *pContactNormal, float3 *pContactPoint, float *pHitFraction) const
{
    // create a temporary bullet shape on the stack
    // construct a bullet transform for the box (ie center of it)
    btBoxShape bulletBoxShape(Float3ToBulletVector(boxHalfExtents));
    btTransform bulletTransformFrom(btQuaternion::getIdentity(), Float3ToBulletVector(from));
    btTransform bulletTransformTo(btQuaternion::getIdentity(), Float3ToBulletVector(to));

    // sweep it against the world
    btCollisionWorld::ClosestConvexResultCallback result(bulletTransformFrom.getOrigin(), bulletTransformTo.getOrigin());
    m_pBulletWorld->convexSweepTest(&bulletBoxShape, bulletTransformFrom, bulletTransformTo, result);

    // got a result?
    if (!result.hasHit())
        return false;

    // store results
    *ppContactObject = reinterpret_cast<const PhysicsProxy *>(result.m_hitCollisionObject->getUserPointer());
    *pContactNormal = BulletVector3ToFloat3(result.m_hitNormalWorld);
    *pContactPoint = BulletVector3ToFloat3(result.m_hitPointWorld);
    *pHitFraction = result.m_closestHitFraction;
    return true;
}

bool PhysicsWorld::SweepSphere(const float radius, const float3 &from, const float3 &to, const PhysicsProxy **ppContactObject, float3 *pContactNormal, float3 *pContactPoint, float *pHitFraction) const
{
    // create a temporary bullet shape on the stack
    // construct a bullet transform for the box (ie center of it)
    btSphereShape bulletSphereShape(radius);
    btTransform bulletTransformFrom(btQuaternion::getIdentity(), Float3ToBulletVector(from));
    btTransform bulletTransformTo(btQuaternion::getIdentity(), Float3ToBulletVector(to));

    // sweep it against the world
    btCollisionWorld::ClosestConvexResultCallback result(bulletTransformFrom.getOrigin(), bulletTransformTo.getOrigin());
    m_pBulletWorld->convexSweepTest(&bulletSphereShape, bulletTransformFrom, bulletTransformTo, result);

    // got a result?
    if (!result.hasHit())
        return false;

    // store results
    *ppContactObject = reinterpret_cast<const PhysicsProxy *>(result.m_hitCollisionObject->getUserPointer());
    *pContactNormal = BulletVector3ToFloat3(result.m_hitNormalWorld);
    *pContactPoint = BulletVector3ToFloat3(result.m_hitPointWorld);
    *pHitFraction = result.m_closestHitFraction;
    return true;
}

void PhysicsWorld::ApplyRadialForce(const float3 &center, float radius, float amount, float falloffRate)
{
    // find aabbs in range
    btVector3 minAABB(Float3ToBulletVector(center) - btVector3(radius, radius, radius));
    btVector3 maxAABB(Float3ToBulletVector(center) + btVector3(radius, radius, radius));

    // callback
    struct OurBroadphaseAABBCallback : public btBroadphaseAabbCallback
    {
        btVector3 forceCenter;
        float forceAmount;
        float radiusSquared;

        OurBroadphaseAABBCallback(const btVector3 &forceCenter_, float forceAmount_, float radiusSquared_)
            : forceCenter(forceCenter_), forceAmount(forceAmount_), radiusSquared(radiusSquared_)
        {

        }

        virtual bool process(const btBroadphaseProxy *proxy) override
        {
            const btCollisionObject *object = reinterpret_cast<const btCollisionObject *>(proxy->m_clientObject);
            if (object != nullptr && object->getInternalType() == btCollisionObject::CO_RIGID_BODY)
            {
                btRigidBody *rigidBody = const_cast<btRigidBody *>(static_cast<const btRigidBody *>(object));

                // skip anything out of range
                if ((rigidBody->getCenterOfMassPosition() - forceCenter).length2() < radiusSquared)
                {
                    // transform to rigid body local space
                    btVector3 forceCenterLocalSpace(rigidBody->getCenterOfMassTransform().invXform(forceCenter));

                    // get force vector
                    btVector3 forceVector(-forceCenterLocalSpace.normalized() * forceAmount);

                    // apply force vector
                    if (forceVector.length2() > Y_FLT_EPSILON)
                    {
                        //Log_DevPrintf("apply %s, rel pos %s", StringConverter::Float3ToString(BulletVector3ToFloat3(forceVector)).GetCharArray(), StringConverter::Float3ToString(BulletVector3ToFloat3(forceCenterLocalSpace)).GetCharArray());
                        rigidBody->applyImpulse(forceVector, forceCenterLocalSpace);
                        rigidBody->activate();
                    }
                }
            }

            return true;
        }
    };

    OurBroadphaseAABBCallback callback(Float3ToBulletVector(center), amount, Math::Square(radius));
    m_pBulletWorld->getBroadphase()->aabbTest(minAABB, maxAABB, callback);
}

void PhysicsWorld::UpdateSingleObject(CollisionObject *pCollisionObject)
{
    m_pBulletWorld->updateSingleAabb(pCollisionObject->GetBulletCollisionObject());
    WakeObjectsInBox(pCollisionObject->GetBoundingBox());
}

void PhysicsWorld::WakeObjectsInBox(const AABox &box)
{
    // find aabbs in range
    btVector3 minAABB(Float3ToBulletVector(box.GetMinBounds()));
    btVector3 maxAABB(Float3ToBulletVector(box.GetMaxBounds()));

    // callback
    struct WakeObjectAABBCallback : public btBroadphaseAabbCallback
    {
        virtual bool process(const btBroadphaseProxy *proxy) override
        {
            const btCollisionObject *object = reinterpret_cast<const btCollisionObject *>(proxy->m_clientObject);
            if (object != nullptr && object->getInternalType() == btCollisionObject::CO_RIGID_BODY)
            {
                btRigidBody *rigidBody = const_cast<btRigidBody *>(static_cast<const btRigidBody *>(object));
                if (rigidBody->getActivationState() == ISLAND_SLEEPING)
                    rigidBody->activate();
            }

            return true;
        }
    };

    // instantiate callback and run it
    WakeObjectAABBCallback callback;
    m_pBulletWorld->getBroadphase()->aabbTest(minAABB, maxAABB, callback);
}

}       // namespace Physics
