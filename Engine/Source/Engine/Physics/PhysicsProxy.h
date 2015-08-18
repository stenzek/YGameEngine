#pragma once
#include "Engine/Common.h"

class btCollisionObject;

namespace Physics {

class PhysicsWorld;

class PhysicsProxy : public ReferenceCounted
{
public:
    PhysicsProxy(uint32 entityID);
    virtual ~PhysicsProxy();

    const uint32 GetEntityID() const { return m_entityID; }
    const AABox &GetBoundingBox() const { return m_boundingBox; }
    PhysicsWorld *GetPhysicsWorld() const { return m_pPhysicsWorld; }
    const bool IsInWorld() const { return (m_pPhysicsWorld != nullptr); }
    void SetEntityID(uint32 entityID) { m_entityID = entityID; }

    virtual void OnAddedToPhysicsWorld(PhysicsWorld *pPhysicsWorld);
    virtual void OnRemovedFromPhysicsWorld(PhysicsWorld *pPhysicsWorld);
    virtual void OnSynchronize();

    // todo: test collision against another object

protected:
    uint32 m_entityID;
    AABox m_boundingBox;
    PhysicsWorld *m_pPhysicsWorld;
};

}   // namespace Physics