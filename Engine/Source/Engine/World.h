#pragma once
#include "Engine/Common.h"

namespace Physics { class PhysicsWorld; }
class RenderWorld;

class Brush;
class Entity;
class ParticleSystem;
class ParticleSystemRenderProxy;

// Base world class doesn't implement entity storage
class World
{
public:
    World();
    virtual ~World();

    // Bounds
    const AABox &GetWorldBoundingBox() const { return m_worldBoundingBox; }
    const Sphere &GetWorldBoundingSphere() const { return m_worldBoundingSphere; }
    
    // Game time
    const float GetGameTime() const { return m_gameTime; }

    // Physics World
    const Physics::PhysicsWorld *GetPhysicsWorld() const { return m_pPhysicsWorld; }
    Physics::PhysicsWorld *GetPhysicsWorld() { return m_pPhysicsWorld; }

    // Render World
    const RenderWorld *GetRenderWorld() const { return m_pRenderWorld; }
    RenderWorld *GetRenderWorld() { return m_pRenderWorld; }

    // Entity ID allocation
    uint32 GetNextEntityID() const { return m_nextEntityID; }
    uint32 AllocateEntityID() { return m_nextEntityID++; }
    void SetNextEntityID(uint32 NewNextEntityId) { DebugAssert(NewNextEntityId >= m_nextEntityID); m_nextEntityID = NewNextEntityId; }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Implementation-specific methods
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // immediately adds an object to world
    virtual void AddBrush(Brush *pObject) = 0;

    // immediately removes an object from world, use with care
    virtual void RemoveBrush(Brush *pObject) = 0;

    // Searches for an entity by id.
    // Pointers returned by this method are guaranteed for the lifetime of the current frame.
    virtual const Entity *GetEntityByID(uint32 EntityId) const = 0;
    virtual Entity *GetEntityByID(uint32 EntityId) = 0;

    // immediately adds the entity to world
    virtual void AddEntity(Entity *pEntity) = 0;

    // updates spatial structure for new entity bounds
    virtual void MoveEntity(Entity *pEntity) = 0;

    // updates any needed internal information when an entity property is modified
    virtual void UpdateEntity(Entity *pEntity) = 0;

    // immediately removes the entity from world
    virtual void RemoveEntity(Entity *pEntity) = 0;

    // removes the entity from the world at the end of the frame, safer for game use
    void QueueRemoveEntity(Entity *pEntity);

    // observers
    uint32 GetObserverCount() const { return m_observers.GetSize(); }
    const float3 &GetObserverLocation(uint32 observerIndex) const { return m_observers[observerIndex].Value; }
    void AddObserver(const void *identifier, const float3 &location = float3::Zero);
    void UpdateObserver(const void *identifier, const float3 &location);
    void RemoveObserver(const void *identifier);

    // cast a ray into the world, returning the entity it hit (if any)
    bool RayCast(const Ray &ray, Entity **ppHitEntity, float3 *pContactNormal, float3 *pContactPoint);
    bool RayCast(const float3 &origin, const float3 &direction, float maxDistance, Entity **ppHitEntity, float3 *pContactNormal, float3 *pContactPoint) { return RayCast(Ray(origin, direction, maxDistance), ppHitEntity, pContactNormal, pContactPoint); }

    // temporary particle effects, these are allocated and managed by the world,
    // and automatically destroyed at the end of their lifetime. todo move to own class?
    void SpawnParticleEmitter(const ParticleSystem *pParticleSystem, float lifeSpan, const float3 &location, const Quaternion &rotation = Quaternion::Identity, const float3 &scale = float3::One, const float3 &initialVelocity = float3::Zero, float mass = 0.0f);
        
    // active entities
    void RegisterEntityForUpdate(Entity *pEntity, float interval);
    void RegisterEntityForAsyncUpdate(Entity *pEntity, float interval);
    void UnregisterEntityForUpdate(Entity *pEntity);
    void UnregisterEntityForAsyncUpdate(Entity *pEntity);

    // Executed on the start of the frame
    virtual void BeginFrame(float deltaTime);

    // Update entities, async part
    virtual void UpdateAsync(float deltaTime);

    // Update entities
    virtual void Update(float deltaTime);

    // Execute on the end of the frame
    virtual void EndFrame();

protected:
    AABox m_worldBoundingBox;
    Sphere m_worldBoundingSphere;
    uint32 m_nextEntityID;
    float m_gameTime;

    // Bullet world
    Physics::PhysicsWorld *m_pPhysicsWorld;

    // Render world
    RenderWorld *m_pRenderWorld;

    // entity updates
    struct EntityUpdateData
    {
        Entity *pEntity;
        float UpdateInterval;
        float TimeSinceLastUpdate;
    };
    typedef MemArray<EntityUpdateData> EntityUpdateDataArray;
    EntityUpdateDataArray m_activeEntities;
    EntityUpdateDataArray m_activeAsyncEntities;

    // sort the active entity list, entities with more frequent updates come first
    void SortActiveEntities();
    void SortActiveAsyncEntities();

    // observers
    typedef KeyValuePair<const void *, float3> ObserverEntry;
    typedef MemArray<ObserverEntry> ObserverArray;
    ObserverArray m_observers;

    // removal queue for end of frame
    PODArray<Entity *> m_removeQueue;

    // temporary particle effects
    void UpdateTemporaryParticleEffects(float deltaTime);
    struct TemporaryParticleEffect
    {
        ParticleSystemRenderProxy *pRenderProxy;
        Transform BaseTransform;
        float TimeRemaining;

        bool HasVelocity;
        float3 Velocity;
        float Mass;
    };
    MemArray<TemporaryParticleEffect> m_temporaryParticleEffects;
};
