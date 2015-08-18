#pragma once
#include "Engine/World.h"
#include "Engine/Entity.h"

class DynamicWorld : public World
{
public:
    DynamicWorld();
    virtual ~DynamicWorld();

    virtual void AddBrush(Brush *pObject) override;
    virtual void RemoveBrush(Brush *pObject) override;

    virtual const Entity *GetEntityByID(uint32 EntityId) const override;
    virtual Entity *GetEntityByID(uint32 EntityId) override;

    virtual void AddEntity(Entity *pEntity) override;
    virtual void MoveEntity(Entity *pEntity) override;
    virtual void UpdateEntity(Entity *pEntity) override;
    virtual void RemoveEntity(Entity *pEntity) override;

    virtual void BeginFrame(float deltaTime) override;
    virtual void UpdateAsync(float deltaTime) override;
    virtual void Update(float deltaTime) override;
    virtual void EndFrame() override;

private:
    // data we track for entiites
    struct EntityData
    {
        Entity *pEntity;
        uint32 EntityID;
        AABox BoundingBox;
        Sphere BoundingSphere;
    };

    // array types
    typedef PODArray<Brush *> StaticObjectArray;
    typedef MemArray<EntityData> EntityDataArray;
    typedef PODArray<Entity *> EntityArray;

    // get object data for a specified entity
    EntityData *GetEntityData(const Entity *pEntity);

    // we could probably binary sort these with the uint32 id
    StaticObjectArray m_brushes;
    EntityDataArray m_entities;

public:
    // Finds all entity in the world.
    template<typename T>
    void EnumerateEntities(T Callback)
    {
        for (uint32 i = 0; i < m_entities.GetSize(); i++)
        {
            if (m_entities[i].EntityID != 0)
                Callback(m_entities[i].pEntity->Cast<Entity>());
        }
    }
    template<typename T>
    void EnumerateEntities(T Callback) const
    {
        for (uint32 i = 0; i < m_entities.GetSize(); i++)
        {
            if (m_entities[i].EntityID != 0)
                Callback(m_entities[i].pEntity->Cast<Entity>());
        }
    }

    // Finds objects inside the specified frustum.
    template<typename T>
    void EnumerateEntitiesInFrustum(const Frustum &frustum, T Callback)
    {
        for (uint32 i = 0; i < m_entities.GetSize(); i++)
        {
            if (m_entities[i].EntityID != 0 && frustum.AABoxIntersection(m_entities[i].BoundingBox))
                Callback(m_entities[i].pEntity->Cast<Entity>());
        }
    }
    template<typename T>
    void EnumerateEntitiesInFrustum(const Frustum &frustum, T Callback) const
    {
        for (uint32 i = 0; i < m_entities.GetSize(); i++)
        {
            if (m_entities[i].EntityID != 0 && frustum.AABoxIntersection(m_entities[i].BoundingBox))
                Callback(m_entities[i].pEntity->Cast<Entity>());
        }
    }

    // Finds objects inside the specified axis-aligned box.
    template<typename T>
    void EnumerateEntitiesInAABox(const AABox &box, T Callback)
    {
        for (uint32 i = 0; i < m_entities.GetSize(); i++)
        {
            if (m_entities[i].EntityID != 0 && box.AABoxIntersection(m_entities[i].BoundingBox))
                Callback(m_entities[i].pEntity->Cast<Entity>());
        }
    }
    template<typename T>
    void EnumerateEntitiesInAABox(const AABox &box, T Callback) const
    {
        for (uint32 i = 0; i < m_entities.GetSize(); i++)
        {
            if (m_entities[i].EntityID != 0 && box.AABoxIntersection(m_entities[i].BoundingBox))
                Callback(m_entities[i].pEntity->Cast<Entity>());
        }
    }

    // Finds objects inside the specified sphere.
    template<typename T>
    void EnumerateEntitiesInSphere(const Sphere &sphere, T Callback)
    {
        for (uint32 i = 0; i < m_entities.GetSize(); i++)
        {
            if (m_entities[i].EntityID != 0 && sphere.SphereIntersection(m_entities[i].BoundingSphere))
                Callback(m_entities[i].pEntity->Cast<Entity>());
        }
    }
    template<typename T>
    void EnumerateEntitiesInSphere(const Sphere &sphere, T Callback) const
    {
        for (uint32 i = 0; i < m_entities.GetSize(); i++)
        {
            if (m_entities[i].EntityID != 0 && sphere.SphereIntersection(m_entities[i].BoundingSphere))
                Callback(m_entities[i].pEntity->Cast<Entity>());
        }
    }
};
