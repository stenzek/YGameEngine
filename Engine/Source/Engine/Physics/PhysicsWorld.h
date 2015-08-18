#pragma once
#include "Engine/Common.h"
#include "Engine/Physics/PhysicsProxy.h"

class btCollisionObject;
class btDiscreteDynamicsWorld;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
struct btDbvtBroadphase;
class btSequentialImpulseConstraintSolver;
class btGhostPairCallback;

namespace Physics {

class CollisionObject;

class PhysicsWorld
{
public:
    PhysicsWorld();
    ~PhysicsWorld();

    // Gravity
    const float3 &GetGravity() const { return m_gravity; }
    void SetGravity(const float3 &gravity);

    // Bullet accessor
    const btDiscreteDynamicsWorld *GetBulletWorld() const { return m_pBulletWorld; }
    btDiscreteDynamicsWorld *GetBulletWorld() { return m_pBulletWorld; }

    // Object manipulation.
    void AddObject(PhysicsProxy *pPhysicsProxy);
    void RemoveObject(PhysicsProxy *pPhysicsProxy);
    void QueueObjectSynchronization(PhysicsProxy *pPhysicsProxy);

    // Runs a frame (allowed to be called asynchronously)
    // todo: split to async and sync parts
    void UpdateAsync(float timeSinceLastUpdate);
    void Update(float timeSinceLastUpdate);

    // Ray casting
    bool RayCast(const Ray &ray) const;
    bool RayCast(const Ray &ray, const PhysicsProxy **ppContactObject, float3 *pContactNormal, float3 *pContactPoint) const;

    // Reverse intersection functions
    // These test for a reverse (i.e. the object being tested colliding with us)
    bool TestAABoxIntersection(const AABox &box) const;
    bool TestAABoxIntersection(const AABox &box, const PhysicsProxy **ppContactObject, float3 *pContactNormal, float3 *pContactPoint) const;
    bool TestSphereIntersection(const Sphere &sphere) const;
    bool TestSphereIntersection(const Sphere &sphere, const PhysicsProxy **ppContactObject, float3 *pContactNormal, float3 *pContactPoint) const;
    //bool TestMovingSphereIntersection(const Sphere &)

    // sweep tests
    bool SweepBox(const float3 &boxHalfExtents, const float3 &from, const float3 &to, const PhysicsProxy **ppContactObject, float3 *pContactNormal, float3 *pContactPoint, float *pHitFraction) const;
    bool SweepSphere(const float radius, const float3 &from, const float3 &to, const PhysicsProxy **ppContactObject, float3 *pContactNormal, float3 *pContactPoint, float *pHitFraction) const;

    // Query functions
    template<typename T>
    void EnumerateCollisionObjects(T &Callback)
    {
        for (PhysicsProxy *pPhysicsProxy : m_collisionObjects)
            Callback(pPhysicsProxy);
    }
    template<typename T>
    void EnumerateCollisionObjects(T &Callback) const
    {
        for (const PhysicsProxy *pPhysicsProxy : m_collisionObjects)
            Callback(pPhysicsProxy);
    }

    // Finds objects inside the specified axis-aligned box.
    template<typename T>
    void EnumerateCollisionObjectsInAABox(const AABox &box, T &Callback)
    {
        for (PhysicsProxy *pPhysicsProxy : m_collisionObjects)
        {
            if (box.AABoxIntersection(pPhysicsProxy->GetBoundingBox()))
                Callback(pPhysicsProxy);
        }
    }
    template<typename T>
    void EnumerateCollisionObjectsInAABox(const AABox &box, T &Callback) const
    {
        for (const PhysicsProxy *pPhysicsProxy : m_collisionObjects)
        {
            if (box.AABoxIntersection(pPhysicsProxy->GetBoundingBox()))
                Callback(pPhysicsProxy);
        }
    }

    // apply a radial force
    void ApplyRadialForce(const float3 &center, float radius, float amount, float falloffRate);

    // change the aabb of a single object
    void UpdateSingleObject(CollisionObject *pCollisionObject);

    // wake objects when a static object changes within a bounding box
    void WakeObjectsInBox(const AABox &box);

private:
    typedef LinkedList<PhysicsProxy *> CollisionObjectList;
    typedef PODArray<PhysicsProxy *> CollisionObjectArray;

    // gravity vector
    float3 m_gravity;

    // arrays
    CollisionObjectArray m_collisionObjects;
    //CollisionObjectArray m_addQueue;
    //CollisionObjectArray m_removeQueue;
    CollisionObjectArray m_synchronizationQueue;

    // Bullet world
    btDiscreteDynamicsWorld *m_pBulletWorld;
    btDefaultCollisionConfiguration *m_pBulletCollisionConfiguration;
    btCollisionDispatcher *m_pBulletCollisionDispatcher;
    btDbvtBroadphase *m_pBulletBroadphase;
    btSequentialImpulseConstraintSolver *m_pBulletSolver;
    btGhostPairCallback *m_pBulletGhostPairCallback;
};

}       // namespace Physics
