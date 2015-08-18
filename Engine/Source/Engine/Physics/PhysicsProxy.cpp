#include "Engine/PrecompiledHeader.h"
#include "Engine/Physics/PhysicsProxy.h"

namespace Physics {

PhysicsProxy::PhysicsProxy(uint32 entityID)
    : m_entityID(entityID),
      m_boundingBox(AABox::Zero),
      m_pPhysicsWorld(nullptr)
{

}

PhysicsProxy::~PhysicsProxy()
{
    DebugAssert(m_pPhysicsWorld == nullptr);
}

void PhysicsProxy::OnAddedToPhysicsWorld(PhysicsWorld *pPhysicsWorld)
{
    DebugAssert(m_pPhysicsWorld == nullptr);
    m_pPhysicsWorld = pPhysicsWorld;
}

void PhysicsProxy::OnRemovedFromPhysicsWorld(PhysicsWorld *pPhysicsWorld)
{
    DebugAssert(m_pPhysicsWorld == pPhysicsWorld);
    m_pPhysicsWorld = nullptr;
}

void PhysicsProxy::OnSynchronize()
{

}

}   // namespace Physics
