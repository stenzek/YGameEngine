#pragma once
#include "Engine/Component.h"
#include "Engine/BlockMesh.h"

namespace Physics { class StaticObject; }
class BlockMeshRenderProxy;

class BlockMeshComponent : public Component
{
    DECLARE_COMPONENT_TYPEINFO(BlockMeshComponent, Component);
    DECLARE_COMPONENT_GENERIC_FACTORY(BlockMeshComponent);

public:
    BlockMeshComponent(const ComponentTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~BlockMeshComponent();

    // property accessors
    const bool IsVisible() const { return m_visible; }
    const bool IsCollidable() const { return m_collidable; }
    const BlockMesh *GetBlockMesh() const { return m_pBlockMesh; }
    const uint32 GetShadowFlags() const { return m_shadowFlags; }

    // property setters
    void SetVisible(bool visible);
    void SetCollidable(bool enabled);
    void SetBlockMesh(const BlockMesh *pBlockMesh);
    void SetShadowFlags(uint32 shadowFlags);

    // External creation method
    void Create(const float3 &localPosition = float3::Zero, const Quaternion &localRotation = Quaternion::Identity, const float3 &localScale = float3::One, const BlockMesh *pBlockMesh = nullptr, bool visible = true, bool collidable = true, uint32 shadowFlags = ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS);

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
    static void PropertyCallbackBlockMeshChanged(ThisClass *pEntity, const void *pUserData = nullptr);
    static void PropertyCallbackVisibleChanged(ThisClass *pEntity, const void *pUserData = nullptr);
    static void PropertyCallbackCollidableChanged(ThisClass *pEntity, const void *pUserData = nullptr);

    // vars
    const BlockMesh *m_pBlockMesh;
    bool m_visible;
    bool m_collidable;
    uint32 m_shadowFlags;

    // render proxy
    BlockMeshRenderProxy *m_pRenderProxy;

    // physics proxy
    Physics::StaticObject *m_pCollisionObject;
};
