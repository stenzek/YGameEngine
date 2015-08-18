#include "Engine/PrecompiledHeader.h"
#include "Engine/DynamicWorld.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Brush.h"
#include "Engine/Entity.h"
#include "Renderer/RenderWorld.h"

DynamicWorld::DynamicWorld()
    : World()
{

}

DynamicWorld::~DynamicWorld()
{
    // clean up remaining entites
    while (m_entities.GetSize() > 0)
    {
        EntityData &data = m_entities[m_entities.GetSize() - 1];
        Entity *pEntity = data.pEntity;

        for (uint32 i = 0; i < m_activeEntities.GetSize(); i++)
        {
            if (m_activeEntities[i].pEntity == pEntity)
            {
                m_activeEntities.FastRemove(i);
                break;
            }
        }
        for (uint32 i = 0; i < m_activeAsyncEntities.GetSize(); i++)
        {
            if (m_activeAsyncEntities[i].pEntity == pEntity)
            {
                m_activeAsyncEntities.FastRemove(i);
                break;
            }
        }

        // as OnRemoveFromWorld could invoke an entity lookup/search, we have to remove the entity from the list *now*
        m_entities.RemoveBack();

        // invoke handler and cleanup
        pEntity->OnRemoveFromWorld(this);
        pEntity->Release();
    }

    // and static objects
    while (m_brushes.GetSize() > 0)
    {
        Brush *pObject = m_brushes.PopBack();

        pObject->OnRemoveFromWorld(this);
        pObject->Release();
    }
}

DynamicWorld::EntityData *DynamicWorld::GetEntityData(const Entity *pEntity)
{
    for (uint32 i = 0; i < m_entities.GetSize(); i++)
    {
        if (m_entities[i].pEntity == pEntity)
            return &m_entities[i];
    }

    return nullptr;
}

void DynamicWorld::AddBrush(Brush *pObject)
{
    DebugAssert(!m_brushes.Contains(pObject));

    // add reference
    pObject->AddRef();
    m_brushes.Add(pObject);

    // invoke added function
    pObject->OnAddToWorld(this);

    // update bounds
    m_worldBoundingBox.Merge(pObject->GetBoundingBox());
    m_worldBoundingSphere.Merge(pObject->GetBoundingSphere());
}

void DynamicWorld::RemoveBrush(Brush *pObject)
{
    // lookup
    for (uint32 i = 0; i < m_brushes.GetSize(); i++)
    {
        if (m_brushes[i] == pObject)
        {
            pObject->OnRemoveFromWorld(this);
            pObject->Release();
            m_brushes.OrderedRemove(i);
            return;
        }
    }

    // should not get reached
    Panic("attempted to remove brush from world where it does not exist");
}

const Entity *DynamicWorld::GetEntityByID(uint32 EntityId) const
{
    DebugAssert(EntityId != 0);

    for (uint32 i = 0; i < m_entities.GetSize(); i++)
    {
        if (m_entities[i].EntityID == EntityId)
            return m_entities[i].pEntity->Cast<Entity>();
    }

    return nullptr;
}

Entity *DynamicWorld::GetEntityByID(uint32 EntityId)
{
    DebugAssert(EntityId != 0);

    for (uint32 i = 0; i < m_entities.GetSize(); i++)
    {
        if (m_entities[i].EntityID == EntityId)
            return m_entities[i].pEntity->Cast<Entity>();
    }

    return nullptr;
}

void DynamicWorld::AddEntity(Entity *pEntity)
{
    DebugAssert(pEntity->GetEntityID() != 0);
    DebugAssert(GetEntityByID(pEntity->GetEntityID()) == nullptr);

    // add reference
    pEntity->AddRef();

    // add to lookup list
    EntityData data;
    data.pEntity = pEntity;
    data.EntityID = pEntity->GetEntityID();
    data.BoundingBox = pEntity->GetBoundingBox();
    data.BoundingSphere = pEntity->GetBoundingSphere();
    m_entities.Add(data);

    // invoke added function
    pEntity->OnAddToWorld(this);

    // update bounds
    m_worldBoundingBox.Merge(pEntity->GetBoundingBox());
    m_worldBoundingSphere.Merge(pEntity->GetBoundingSphere());
}

void DynamicWorld::MoveEntity(Entity *pEntity)
{
    for (uint32 i = 0; i < m_entities.GetSize(); i++)
    {
        EntityData &data = m_entities[i];
        if (data.pEntity == pEntity)
        {
            data.BoundingBox = pEntity->GetBoundingBox();
            data.BoundingSphere = pEntity->GetBoundingSphere();

            m_worldBoundingBox.Merge(pEntity->GetBoundingBox());
            m_worldBoundingSphere.Merge(pEntity->GetBoundingSphere());
            return;
        }
    }

    Panic("attempted to move entity in world where it does not exist");
}

void DynamicWorld::UpdateEntity(Entity *pEntity)
{

}

void DynamicWorld::RemoveEntity(Entity *pEntity)
{
    for (uint32 i = 0; i < m_entities.GetSize(); i++)
    {
        EntityData &data = m_entities[i];
        if (data.pEntity == pEntity)
        {
            // ensure it isn't active
            for (uint32 j = 0; j < m_activeEntities.GetSize(); j++)
            {
                if (m_activeEntities[j].pEntity == pEntity)
                {
                    m_activeEntities.FastRemove(j);
                    SortActiveEntities();
                    break;
                }
            }

            for (uint32 j = 0; j < m_activeAsyncEntities.GetSize(); j++)
            {
                if (m_activeAsyncEntities[j].pEntity == pEntity)
                {
                    m_activeAsyncEntities.FastRemove(j);
                    SortActiveAsyncEntities();
                    break;
                }
            }

            // ensure it isn't queued for removal
            for (uint32 j = 0; j < m_removeQueue.GetSize(); j++)
            {
                if (m_removeQueue[j] == pEntity)
                {
                    m_removeQueue.FastRemove(j);
                    break;
                }
            }

            // remove from list
            m_entities.FastRemove(i);

            // invoke removed function
            pEntity->OnRemoveFromWorld(this);

            // remove object
            pEntity->Release();
            return;
        }
    }

    // should not get reached
    Panic("attempted to remove entity from world where it does not exist");
}

void DynamicWorld::BeginFrame(float deltaTime)
{
    World::BeginFrame(deltaTime);
}

void DynamicWorld::UpdateAsync(float deltaTime)
{
    World::UpdateAsync(deltaTime);
}

void DynamicWorld::Update(float deltaTime)
{
    World::Update(deltaTime);
}

void DynamicWorld::EndFrame()
{
    World::EndFrame();
}
