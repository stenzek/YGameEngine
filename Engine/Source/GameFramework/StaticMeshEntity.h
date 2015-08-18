#pragma once
#include "Engine/Entity.h"

class StaticMesh;
namespace Physics { class CollisionObject; }
class StaticMeshRenderProxy;

class StaticMeshEntity : public Entity
{
    DECLARE_ENTITY_TYPEINFO(StaticMeshEntity, Entity);
    DECLARE_ENTITY_GENERIC_FACTORY(StaticMeshEntity);

public:
    StaticMeshEntity(const EntityTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~StaticMeshEntity();

    // property accessors
    const StaticMesh *GetStaticMesh() const { return m_pStaticMesh; }
    const bool IsVisible() const { return m_visible; }
    const uint32 GetShadowFlags() const { return m_shadowFlags; }
    const bool IsCollidable() const { return m_collidable; }

    // property setters
    void SetStaticMesh(const StaticMesh *pStaticMesh);
    void SetVisible(bool visible);
    void SetCollidable(bool collidable);
    void SetShadowFlags(uint32 shadowFlags);

    // External creation method
    void Create(uint32 entityID, ENTITY_MOBILITY mobility = ENTITY_MOBILITY_MOVABLE, const float3 &position = float3::Zero, const Quaternion &rotation = Quaternion::Identity, const float3 &scale = float3::One, const StaticMesh *pStaticMesh = nullptr, bool visible = true, bool collidable = true, uint32 shadowFlags = ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS);

    // implemented methods
    virtual bool Initialize(uint32 entityID, const String &entityName) override;
    virtual void OnAddToWorld(World *pWorld) override;
    virtual void OnRemoveFromWorld(World *pWorld) override;
    virtual void OnTransformChange() override;
    virtual void OnComponentBoundsChange() override;

private:
    // property callbacks
    static bool PropertyCallbackGetStaticMeshName(ThisClass *pEntity, const void *pUserData, String *pValue);
    static bool PropertyCallbackSetStaticMeshName(ThisClass *pEntity, const void *pUserData, const String *pValue);
    static void PropertyCallbackStaticMeshChanged(ThisClass *pEntity, const void *pUserData = nullptr);
    static void PropertyCallbackVisibleChanged(ThisClass *pEntity, const void *pUserData = nullptr);
    static void PropertyCallbackCollidableChanged(ThisClass *pEntity, const void *pUserData = nullptr);

    // update methods
    void UpdateRenderProxy();
    void UpdateCollisionObject();
    void UpdateBounds();

    // vars
    const StaticMesh *m_pStaticMesh;
    bool m_visible;
    bool m_collidable;
    uint32 m_shadowFlags;

    // render proxy
    StaticMeshRenderProxy *m_pRenderProxy;

    // physics proxy
    Physics::CollisionObject *m_pCollisionObject;
};

