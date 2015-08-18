#pragma once
#include "Engine/Brush.h"

class StaticMesh;
namespace Physics { class StaticObject; }
class StaticMeshRenderProxy;

class StaticMeshBrush : public Brush
{
    DECLARE_OBJECT_TYPE_INFO(StaticMeshBrush, Brush);
    DECLARE_OBJECT_PROPERTY_MAP(StaticMeshBrush);
    DECLARE_OBJECT_GENERIC_FACTORY(StaticMeshBrush);

public:
    StaticMeshBrush(const ObjectTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~StaticMeshBrush();

    // External creation method
    bool Create(const float3 &position = float3::Zero, const Quaternion &rotation = Quaternion::Identity, const float3 &scale = float3::One, const StaticMesh *pStaticMesh = nullptr, bool visible = true, bool collidable = true, uint32 shadowFlags = ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS);
    
    // Virtual creation method
    virtual bool Initialize() override;

    // Events
    virtual void OnAddToWorld(World *pWorld) override;
    virtual void OnRemoveFromWorld(World *pWorld) override;

    // property accessors
    const StaticMesh *GetStaticMesh() const { return m_pStaticMesh; }
    const bool IsVisible() const { return m_visible; }
    const uint32 GetShadowFlags() const { return m_shadowFlags; }
    const bool IsCollidable() const { return m_collidable; }

private:
    // property callbacks
    static bool PropertyCallbackGetStaticMeshName(ThisClass *pEntity, const void *pUserData, String *pValue);
    static bool PropertyCallbackSetStaticMeshName(ThisClass *pEntity, const void *pUserData, const String *pValue);

    // vars
    const StaticMesh *m_pStaticMesh;
    bool m_visible;
    uint32 m_shadowFlags;
    bool m_collidable;

    // render proxy
    StaticMeshRenderProxy *m_pRenderProxy;

    // physics proxy
    Physics::StaticObject *m_pCollisionObject;
};
