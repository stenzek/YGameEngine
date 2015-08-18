#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/BlockMeshEntity.h"
#include "Engine/BlockMesh.h"
#include "Renderer/RenderProxies/BlockMeshRenderProxy.h"
#include "Engine/World.h"
#include "Engine/Physics/StaticObject.h"
#include "Engine/Physics/KinematicObject.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Renderer/RenderWorld.h"
#include "Engine/ResourceManager.h"
Log_SetChannel(BlockMeshEntity);

DEFINE_ENTITY_TYPEINFO(BlockMeshEntity, 0);
DEFINE_ENTITY_GENERIC_FACTORY(BlockMeshEntity);
BEGIN_ENTITY_PROPERTIES(BlockMeshEntity)
    PROPERTY_TABLE_MEMBER("BlockMeshName", PROPERTY_TYPE_STRING, 0, PropertyCallbackGetBlockMeshName, NULL, PropertyCallbackSetBlockMeshName, NULL, PropertyCallbackBlockMeshChanged, NULL)
    PROPERTY_TABLE_MEMBER_BOOL("Visible", 0, offsetof(BlockMeshEntity, m_visible), NULL, NULL)
    PROPERTY_TABLE_MEMBER_BOOL("Collidable", 0, offsetof(BlockMeshEntity, m_collidable), NULL, NULL)
    PROPERTY_TABLE_MEMBER_UINT("ShadowFlags", 0, offsetof(BlockMeshEntity, m_shadowFlags), NULL, NULL)
END_ENTITY_PROPERTIES()
BEGIN_ENTITY_SCRIPT_FUNCTIONS(BlockMeshEntity)
END_ENTITY_SCRIPT_FUNCTIONS()

BlockMeshEntity::BlockMeshEntity(const EntityTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_pBlockMesh(nullptr),
      m_visible(true),
      m_collidable(false),
      m_shadowFlags(0),
      m_pRenderProxy(nullptr),
      m_pCollisionObject(nullptr)
{

}

BlockMeshEntity::~BlockMeshEntity()
{
    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->Release();

    if (m_pCollisionObject != nullptr)
        m_pCollisionObject->Release();

    if (m_pBlockMesh != nullptr)
        m_pBlockMesh->Release();
}

void BlockMeshEntity::SetBlockMesh(const BlockMesh *pBlockMesh)
{
    DebugAssert(pBlockMesh != nullptr);
    if (m_pBlockMesh == pBlockMesh)
        return;

    if (m_pBlockMesh != nullptr)
        m_pBlockMesh->Release();

    m_pBlockMesh = pBlockMesh;
    m_pBlockMesh->AddRef();

    PropertyCallbackBlockMeshChanged(this);
}

void BlockMeshEntity::SetVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;
    PropertyCallbackVisibleChanged(this);
}

void BlockMeshEntity::SetShadowFlags(uint32 shadowFlags)
{
    if (m_shadowFlags == shadowFlags)
        return;

    m_shadowFlags = shadowFlags;

    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->SetShadowFlags(shadowFlags);
}

void BlockMeshEntity::SetCollidable(bool collidable)
{
    if (m_collidable == collidable)
        return;

    m_collidable = collidable;
    PropertyCallbackCollidableChanged(this);
}

void BlockMeshEntity::Create(uint32 entityID, ENTITY_MOBILITY mobility /* = ENTITY_MOBILITY_MOVABLE */, const float3 &position /* = float3::Zero */, const Quaternion &rotation /* = Quaternion::Identity */, const float3 &scale /* = float3::One */, const BlockMesh *pBlockMesh /* = nullptr */, bool visible /* = true */, bool collidable /* = true */, uint32 shadowFlags /* = ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS */)
{
    m_mobility = mobility;
    m_transform.Set(position, rotation, scale);

    if (pBlockMesh != nullptr)
    {
        m_pBlockMesh = pBlockMesh;
        m_pBlockMesh->AddRef();
    }
    else
    {
        m_pBlockMesh = g_pResourceManager->GetDefaultBlockMesh();
    }

    m_visible = visible;
    m_collidable = collidable;
    m_shadowFlags = shadowFlags;

    Initialize(entityID, EmptyString);
}

bool BlockMeshEntity::Initialize(uint32 entityID, const String &entityName)
{
    if (!BaseClass::Initialize(entityID, entityName))
        return false;

    DebugAssert(m_pBlockMesh != nullptr);

    UpdateRenderProxy();
    UpdateCollisionObject();

    // determine bounds
    m_boundingBox = m_transform.TransformBoundingBox(m_pBlockMesh->GetBoundingBox());
    m_boundingSphere = m_transform.TransformBoundingSphere(m_pBlockMesh->GetBoundingSphere());
    return true;
}

void BlockMeshEntity::UpdateRenderProxy()
{
    if (m_visible)
    {
        if (m_pRenderProxy == nullptr)
        {
            m_pRenderProxy = new BlockMeshRenderProxy(m_entityID, m_pBlockMesh, m_transform, m_shadowFlags);

            if (IsInWorld())
                m_pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);
        }
        else
        {
            m_pRenderProxy->SetBlockMesh(m_pBlockMesh);
        }
    }
    else
    {
        if (m_pRenderProxy != nullptr)
        {
            if (IsInWorld())
                m_pWorld->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);

            m_pRenderProxy->Release();
            m_pRenderProxy = nullptr;
        }
    }
}

void BlockMeshEntity::UpdateCollisionObject()
{
    if (m_collidable && m_pBlockMesh->GetCollisionShape() != nullptr)
    {
        if (m_pCollisionObject == nullptr)
        {
            // create collision object based on mobility
            if (m_mobility == ENTITY_MOBILITY_STATIC)
                m_pCollisionObject = new Physics::StaticObject(m_entityID, m_pBlockMesh->GetCollisionShape(), m_transform);
            else
                m_pCollisionObject = new Physics::KinematicObject(m_entityID, m_pBlockMesh->GetCollisionShape(), m_transform);

            if (IsInWorld())
                m_pWorld->GetPhysicsWorld()->AddObject(m_pCollisionObject);
        }
        else
        {
            m_pCollisionObject->SetCollisionShape(m_pBlockMesh->GetCollisionShape());
        }
    }
    else
    {
        if (m_pCollisionObject != nullptr)
        {
            if (IsInWorld())
                m_pWorld->GetPhysicsWorld()->RemoveObject(m_pCollisionObject);

            m_pCollisionObject->Release();
            m_pCollisionObject = nullptr;
        }
    }
}

void BlockMeshEntity::UpdateBounds()
{
    // call entity set bounds with the transformed bounds, this will merge with components (if any)
    Entity::SetBounds(m_transform.TransformBoundingBox(m_pBlockMesh->GetBoundingBox()), m_transform.TransformBoundingSphere(m_pBlockMesh->GetBoundingSphere()), true);
}

void BlockMeshEntity::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    if (m_pCollisionObject != NULL)
        pWorld->GetPhysicsWorld()->AddObject(m_pCollisionObject);

    if (m_pRenderProxy != NULL)
        pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);
}

void BlockMeshEntity::OnRemoveFromWorld(World *pWorld)
{
    if (m_pRenderProxy != NULL)
        pWorld->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);

    if (m_pCollisionObject != NULL)
        pWorld->GetPhysicsWorld()->RemoveObject(m_pCollisionObject);

    BaseClass::OnRemoveFromWorld(pWorld);
}

void BlockMeshEntity::OnTransformChange()
{
    BaseClass::OnTransformChange();

    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->SetTransform(m_transform);

    if (m_pCollisionObject != nullptr)
        m_pCollisionObject->SetTransform(m_transform);

    UpdateBounds();
}

void BlockMeshEntity::OnComponentBoundsChange()
{
    UpdateBounds();
}

bool BlockMeshEntity::PropertyCallbackGetBlockMeshName(ThisClass *pEntity, const void *pUserData, String *pValue)
{
    pValue->Assign(pEntity->m_pBlockMesh->GetName());
    return true;
}

bool BlockMeshEntity::PropertyCallbackSetBlockMeshName(ThisClass *pEntity, const void *pUserData, const String *pValue)
{
    const BlockMesh *pBlockMesh = g_pResourceManager->GetBlockMesh(*pValue);
    if (pBlockMesh == NULL)
        return false;

    if (pEntity->m_pBlockMesh != NULL)
        pEntity->m_pBlockMesh->Release();

    pEntity->m_pBlockMesh = pBlockMesh;
    return true;
}

void BlockMeshEntity::PropertyCallbackBlockMeshChanged(ThisClass *pEntity, const void *pUserData /*= nullptr*/)
{
    pEntity->UpdateRenderProxy();
    pEntity->UpdateCollisionObject();
    pEntity->UpdateBounds();
}

void BlockMeshEntity::PropertyCallbackVisibleChanged(ThisClass *pEntity, const void *pUserData /*= nullptr*/)
{
    pEntity->UpdateRenderProxy();
}

void BlockMeshEntity::PropertyCallbackCollidableChanged(ThisClass *pEntity, const void *pUserData /*= nullptr*/)
{
    pEntity->UpdateCollisionObject();
}
