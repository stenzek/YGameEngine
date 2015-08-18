#pragma once
#include "Engine/EntityTypeInfo.h"
#include "Engine/ScriptObject.h"

class Component;
class World;

enum ENTITY_MOBILITY
{
    ENTITY_MOBILITY_STATIC,         // Entity is static, cannot move, and is allocated and removed with the sector.
    ENTITY_MOBILITY_MOVABLE,        // Entity is movable, and allocated/removed with the sector.
    ENTITY_MOBILITY_DEFERRED,       // Entity is movable, allocated with the sector, but not removed if/when the sector is unloaded.
    ENTITY_MOBILITY_GLOBAL,         // Entity is global, allocated on map load, and never removed.
    ENTITY_MOBILITY_COUNT,
};

enum ENTITY_ACTION
{
    ENTITY_ACTION_FIRST_USER = 100,
};

class Entity : public ScriptObject
{
    DECLARE_ENTITY_TYPEINFO(Entity, ScriptObject);
    DECLARE_ENTITY_NO_FACTORY(Entity);

public:
    Entity(const EntityTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~Entity();

    // Retrieves the type information for this object.
    const uint32 GetEntityID() const { return m_entityID; }
    const String &GetEntityName() const { return m_entityName; }
    const EntityTypeInfo *GetEntityTypeInfo() const { return static_cast<const EntityTypeInfo *>(m_pObjectTypeInfo); }

    // Transform accessors.
    const ENTITY_MOBILITY GetMobility() const { return static_cast<ENTITY_MOBILITY>(m_mobility); }
    const Transform &GetTransform() const { return m_transform; }
    const float3 &GetPosition() const { return m_transform.GetPosition(); }
    const Quaternion &GetRotation() const { return m_transform.GetRotation(); }
    const float3 &GetScale() const { return m_transform.GetScale(); }

    // Bounding volumes.
    const AABox &GetBoundingBox() const { return m_boundingBox; }
    const Sphere &GetBoundingSphere() const { return m_boundingSphere; }

    // World.
    World *GetWorld() const { return m_pWorld; }
    bool IsInWorld() const { return (m_pWorld != nullptr); }

    // World data, do not modify from anywhere except world class.
    void *GetWorldData() const { return m_pWorldData; }
    void SetWorldData(void *pWorldData) { m_pWorldData = pWorldData; }

    // Events -- todo look at making these protected instead?
    // Called when the entity is finishing creation, after all properties have been set.
    virtual bool Initialize(uint32 entityID, const String &entityName);

    // Called when the entity is added to the world.
    virtual void OnAddToWorld(World *pWorld);

    // Called prior to the entity being removed from the world.
    virtual void OnRemoveFromWorld(World *pWorld);

    // Called when the transform changes.
    virtual void OnTransformChange();

    // Can be called by components when their bounds changes.
    virtual void OnComponentBoundsChange();

    // Called when an action is performed by another entity.
    virtual void OnAction(Entity *pInitiator, uint32 action);

    // To prevent a transform change from occurring, override this and return false.
    virtual bool SetTransform(const Transform &transform);
    
    // Invoked events
    virtual void Update(float timeSinceLastUpdate);
    virtual void UpdateAsync(float timeSinceLastUpdate);

    // register/unregister for updates
    void RegisterForUpdates(float interval = 0.0f);
    void RegisterForAsyncUpdates(float interval = 0.0f);
    void UnregisterForUpdates();
    void UnregisterForAsyncUpdates();

    // Position setters.
    bool SetPosition(const float3 &position);
    bool SetRotation(const Quaternion &rotation);
    bool SetScale(const float3 &scale);

    // Component accessors.
    uint32 GetComponentCount() const { return m_components.GetSize(); }
    const Component *GetComponent(uint32 i) const { return m_components[i]; }
    Component *GetComponent(uint32 i) { return m_components[i]; }

    // Component add/remove.
    void AddComponent(Component *pComponent);
    void RemoveComponent(Component *pComponent);

protected:
    // gets the bounds of all components
    bool GetComponentBounds(AABox &boundingBox, Sphere &boundingSphere);

    // update the bounding box of the entity, optionally taking into account components
    void SetBounds(const AABox &boundingBox, const Sphere &boundingSphere, bool mergeWithComponents = true);

    // callbacks for property system
    static bool PropertyCallbackGetPosition(Entity *pObjectPtr, const void *pUserData, float3 *pValuePtr);
    static bool PropertyCallbackSetPosition(Entity *pObjectPtr, const void *pUserData, const float3 *pValuePtr);
    static bool PropertyCallbackGetRotation(Entity *pObjectPtr, const void *pUserData, Quaternion *pValuePtr);
    static bool PropertyCallbackSetRotation(Entity *pObjectPtr, const void *pUserData, const Quaternion *pValuePtr);
    static bool PropertyCallbackGetScale(Entity *pObjectPtr, const void *pUserData, float3 *pValuePtr);
    static bool PropertyCallbackSetScale(Entity *pObjectPtr, const void *pUserData, const float3 *pValuePtr);

protected:
    // ID of entity. Unique to world it lives inside.
    uint32 m_entityID;
    String m_entityName;

    // Base object transform.
    uint32 m_mobility;
    Transform m_transform;

    // World-space bounds of the object.
    AABox m_boundingBox;
    Sphere m_boundingSphere;

    // World this object is located inside.
    World *m_pWorld;
    void *m_pWorldData;

    // Component array.
    typedef PODArray<Component *> ComponentArray;
    ComponentArray m_components;

private:
    // Properties/update state
    uint32 m_registeredUpdateCount;
    float m_registeredUpdateInterval;
    uint32 m_registeredAsyncUpdateCount;
    float m_registeredAsyncUpdateInterval;
};

// helper functions
SIMDVector4f EncodeUInt32ToColor(uint32 EntityId);
uint32 DecodeUInt32FromColorR8G8B8A8(const void *pValue);
uint32 DecodeUInt32IdFromColor(const float4 &EncodedColor);
