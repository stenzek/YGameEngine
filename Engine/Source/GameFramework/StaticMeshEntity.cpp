#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/StaticMeshEntity.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Physics/StaticObject.h"
#include "Engine/Physics/KinematicObject.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/ResourceManager.h"
#include "Engine/Physics/BulletHeaders.h"
#include "Renderer/RenderProxies/StaticMeshRenderProxy.h"
#include "Renderer/RenderWorld.h"
Log_SetChannel(StaticMeshEntity);

DEFINE_ENTITY_TYPEINFO(StaticMeshEntity, 0);
DEFINE_ENTITY_GENERIC_FACTORY(StaticMeshEntity);
BEGIN_ENTITY_PROPERTIES(StaticMeshEntity)
    PROPERTY_TABLE_MEMBER("StaticMeshName", PROPERTY_TYPE_STRING, 0, PropertyCallbackGetStaticMeshName, NULL, PropertyCallbackSetStaticMeshName, NULL, PropertyCallbackStaticMeshChanged, NULL)
    PROPERTY_TABLE_MEMBER_BOOL("Visible", 0, offsetof(StaticMeshEntity, m_visible), NULL, NULL)
    PROPERTY_TABLE_MEMBER_BOOL("Collidable", 0, offsetof(StaticMeshEntity, m_collidable), NULL, NULL)
    PROPERTY_TABLE_MEMBER_UINT("ShadowFlags", 0, offsetof(StaticMeshEntity, m_shadowFlags), NULL, NULL)
END_ENTITY_PROPERTIES()
BEGIN_ENTITY_SCRIPT_FUNCTIONS(StaticMeshEntity)
END_ENTITY_SCRIPT_FUNCTIONS()

StaticMeshEntity::StaticMeshEntity(const EntityTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_pStaticMesh(nullptr),
      m_visible(true),
      m_collidable(false),
      m_shadowFlags(0),
      m_pRenderProxy(nullptr),
      m_pCollisionObject(nullptr)
{

}

StaticMeshEntity::~StaticMeshEntity()
{
    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->Release();

    if (m_pCollisionObject != nullptr)
        m_pCollisionObject->Release();

    if (m_pStaticMesh != nullptr)
        m_pStaticMesh->Release();
}

void StaticMeshEntity::SetStaticMesh(const StaticMesh *pStaticMesh)
{
    DebugAssert(pStaticMesh != nullptr);
    if (m_pStaticMesh == pStaticMesh)
        return;

    if (m_pStaticMesh != nullptr)
        m_pStaticMesh->Release();

    m_pStaticMesh = pStaticMesh;
    m_pStaticMesh->AddRef();

    PropertyCallbackStaticMeshChanged(this);
}

void StaticMeshEntity::SetVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;
    PropertyCallbackVisibleChanged(this);
}

void StaticMeshEntity::SetShadowFlags(uint32 shadowFlags)
{
    if (m_shadowFlags == shadowFlags)
        return;

    m_shadowFlags = shadowFlags;

    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->SetShadowFlags(shadowFlags);
}

void StaticMeshEntity::SetCollidable(bool collidable)
{
    if (m_collidable == collidable)
        return;

    m_collidable = collidable;
    PropertyCallbackCollidableChanged(this);
}

void StaticMeshEntity::Create(uint32 entityID, ENTITY_MOBILITY mobility /* = ENTITY_MOBILITY_MOVABLE */, const float3 &position /* = float3::Zero */, const Quaternion &rotation /* = Quaternion::Identity */, const float3 &scale /* = float3::One */, const StaticMesh *pStaticMesh /* = nullptr */, bool visible /* = true */, bool collidable /* = true */, uint32 shadowFlags /* = ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS */)
{
    m_mobility = mobility;
    m_transform.Set(position, rotation, scale);

    if (pStaticMesh != nullptr)
    {
        m_pStaticMesh = pStaticMesh;
        m_pStaticMesh->AddRef();
    }
    else
    {
        m_pStaticMesh = g_pResourceManager->GetDefaultStaticMesh();
    }

    m_visible = visible;
    m_collidable = collidable;
    m_shadowFlags = shadowFlags;

    Initialize(entityID, EmptyString);
}

bool StaticMeshEntity::Initialize(uint32 entityID, const String &entityName)
{
    if (!BaseClass::Initialize(entityID, entityName))
        return false;

    //DebugAssert(m_pStaticMesh != nullptr);
    if (m_pStaticMesh == nullptr)
    {
        Log_ErrorPrintf("StaticMeshEntity::OnCreate: Entity %u missing static mesh", m_entityID);
        return false;
    }

    UpdateRenderProxy();
    UpdateCollisionObject();

    // determine bounds
    m_boundingBox = m_transform.TransformBoundingBox(m_pStaticMesh->GetBoundingBox());
    m_boundingSphere = m_transform.TransformBoundingSphere(m_pStaticMesh->GetBoundingSphere());
    return true;
}

void StaticMeshEntity::UpdateRenderProxy()
{
    if (m_visible)
    {
        if (m_pRenderProxy == nullptr)
        {
            m_pRenderProxy = new StaticMeshRenderProxy(m_entityID, m_pStaticMesh, m_transform, m_shadowFlags);

            if (IsInWorld())
                m_pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);
        }
        else
        {
            m_pRenderProxy->SetStaticMesh(m_pStaticMesh);
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

void StaticMeshEntity::UpdateCollisionObject()
{
    if (m_collidable && m_pStaticMesh->GetCollisionShape() != nullptr)
    {
        if (m_pCollisionObject == nullptr)
        {
            // create collision object based on mobility
            if (m_mobility == ENTITY_MOBILITY_STATIC)
                m_pCollisionObject = new Physics::StaticObject(m_entityID, m_pStaticMesh->GetCollisionShape(), m_transform);
            else
                m_pCollisionObject = new Physics::KinematicObject(m_entityID, m_pStaticMesh->GetCollisionShape(), m_transform);

            if (IsInWorld())
                m_pWorld->GetPhysicsWorld()->AddObject(m_pCollisionObject);
        }
        else
        {
            m_pCollisionObject->SetCollisionShape(m_pStaticMesh->GetCollisionShape());
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

void StaticMeshEntity::UpdateBounds()
{
    // call entity set bounds with the transformed bounds, this will merge with components (if any)
    Entity::SetBounds(m_transform.TransformBoundingBox(m_pStaticMesh->GetBoundingBox()), m_transform.TransformBoundingSphere(m_pStaticMesh->GetBoundingSphere()), true);
}

void StaticMeshEntity::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    if (m_pCollisionObject != nullptr)
        pWorld->GetPhysicsWorld()->AddObject(m_pCollisionObject);

    if (m_pRenderProxy != nullptr)
        pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);
}

void StaticMeshEntity::OnRemoveFromWorld(World *pWorld)
{
    if (m_pRenderProxy != nullptr)
        pWorld->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);

    if (m_pCollisionObject != nullptr)
        pWorld->GetPhysicsWorld()->RemoveObject(m_pCollisionObject);

    BaseClass::OnRemoveFromWorld(pWorld);
}

void StaticMeshEntity::OnTransformChange()
{
    BaseClass::OnTransformChange();

    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->SetTransform(m_transform);

    if (m_pCollisionObject != nullptr)
        m_pCollisionObject->SetTransform(m_transform);

    UpdateBounds();
}

void StaticMeshEntity::OnComponentBoundsChange()
{
    UpdateBounds();
}

bool StaticMeshEntity::PropertyCallbackGetStaticMeshName(ThisClass *pEntity, const void *pUserData, String *pValue)
{
    pValue->Assign(pEntity->m_pStaticMesh->GetName());
    return true;
}

bool StaticMeshEntity::PropertyCallbackSetStaticMeshName(ThisClass *pEntity, const void *pUserData, const String *pValue)
{
    const StaticMesh *pStaticMesh = g_pResourceManager->GetStaticMesh(*pValue);
    if (pStaticMesh == NULL)
        return false;

    if (pEntity->m_pStaticMesh != NULL)
        pEntity->m_pStaticMesh->Release();

    pEntity->m_pStaticMesh = pStaticMesh;
    return true;
}

void StaticMeshEntity::PropertyCallbackStaticMeshChanged(ThisClass *pEntity, const void *pUserData /*= nullptr*/)
{
    pEntity->UpdateRenderProxy();
    pEntity->UpdateCollisionObject();
    pEntity->UpdateBounds();
}

void StaticMeshEntity::PropertyCallbackVisibleChanged(ThisClass *pEntity, const void *pUserData /*= nullptr*/)
{
    pEntity->UpdateRenderProxy();
}

void StaticMeshEntity::PropertyCallbackCollidableChanged(ThisClass *pEntity, const void *pUserData /*= nullptr*/)
{
    pEntity->UpdateCollisionObject();
}
