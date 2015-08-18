#pragma once
#include "Engine/Component.h"
#include "Engine/StaticMesh.h"

namespace Physics { class CollisionObject; }
class StaticMeshRenderProxy;

class StaticMeshComponent : public Component
{
    DECLARE_COMPONENT_TYPEINFO(StaticMeshComponent, Component);
    DECLARE_COMPONENT_GENERIC_FACTORY(StaticMeshComponent);

public:
    StaticMeshComponent(const ComponentTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~StaticMeshComponent();

    // property accessors
    const bool IsVisible() const { return m_visible; }
    const bool IsCollidable() const { return m_collidable; }
    const StaticMesh *GetStaticMesh() const { return m_pStaticMesh; }
    const uint32 GetShadowFlags() const { return m_shadowFlags; }

    // property setters
    void SetVisible(bool visible);
    void SetCollidable(bool enabled);
    void SetStaticMesh(const StaticMesh *pStaticMesh);
    void SetShadowFlags(uint32 shadowFlags);

    // External creation method
    void Create(const float3 &localPosition = float3::Zero, const Quaternion &localRotation = Quaternion::Identity, const float3 &localScale = float3::One, const StaticMesh *pStaticMesh = nullptr, bool visible = true, bool collidable = true, uint32 shadowFlags = ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS);

    // Component events
    virtual bool Initialize() override;
    virtual void OnAddToEntity(Entity *pEntity) override;
    virtual void OnRemoveFromEntity(Entity *pEntity) override;
    virtual void OnAddToWorld(World *pWorld) override;
    virtual void OnRemoveFromWorld(World *pWorld) override;
    virtual void OnLocalTransformChange() override;
    virtual void OnEntityTransformChange() override;

private:
    // property callbacks
    static bool PropertyCallbackGetMeshName(ThisClass *pComponent, const void *pUserData, String *pValue);
    static bool PropertyCallbackSetMeshName(ThisClass *pComponent, const void *pUserData, const String *pValue);
    static void PropertyCallbackStaticMeshChanged(ThisClass *pEntity, const void *pUserData = nullptr);
    static void PropertyCallbackVisibleChanged(ThisClass *pEntity, const void *pUserData = nullptr);
    static void PropertyCallbackCollidableChanged(ThisClass *pEntity, const void *pUserData = nullptr);

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
