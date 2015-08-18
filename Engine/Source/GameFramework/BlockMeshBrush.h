#pragma once
#include "Engine/Brush.h"

class BlockMesh;
namespace Physics { class StaticObject; }
class BlockMeshRenderProxy;

class BlockMeshBrush : public Brush
{
    DECLARE_OBJECT_TYPE_INFO(BlockMeshBrush, Brush);
    DECLARE_OBJECT_PROPERTY_MAP(BlockMeshBrush);
    DECLARE_OBJECT_GENERIC_FACTORY(BlockMeshBrush);

public:
    BlockMeshBrush(const ObjectTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~BlockMeshBrush();

    // External creation method
    bool Create(const float3 &position = float3::Zero, const Quaternion &rotation = Quaternion::Identity, const float3 &scale = float3::One, const BlockMesh *pBlockMesh = nullptr, bool visible = true, bool collidable = true, uint32 shadowFlags = ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS);

    // Virtual creation method
    virtual bool Initialize() override;

    // Events
    virtual void OnAddToWorld(World *pWorld) override;
    virtual void OnRemoveFromWorld(World *pWorld) override;

    // property accessors
    const BlockMesh *GetBlockMesh() const { return m_pBlockMesh; }
    const bool IsVisible() const { return m_visible; }
    const uint32 GetShadowFlags() const { return m_shadowFlags; }
    const bool IsCollidable() const { return m_collidable; }
   
private:
    // property callbacks
    static bool PropertyCallbackGetBlockMeshName(ThisClass *pEntity, const void *pUserData, String *pValue);
    static bool PropertyCallbackSetBlockMeshName(ThisClass *pEntity, const void *pUserData, const String *pValue);

    // vars
    const BlockMesh *m_pBlockMesh;
    bool m_visible;
    uint32 m_shadowFlags;
    bool m_collidable;

    // render proxy
    BlockMeshRenderProxy *m_pRenderProxy;

    // physics proxy
    Physics::StaticObject *m_pCollisionObject;
};
