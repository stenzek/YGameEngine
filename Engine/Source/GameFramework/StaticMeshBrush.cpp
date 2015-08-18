#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/StaticMeshBrush.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/ResourceManager.h"
#include "Engine/Physics/StaticObject.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Renderer/RenderProxies/StaticMeshRenderProxy.h"
#include "Renderer/RenderWorld.h"
Log_SetChannel(StaticMeshBrush);

DEFINE_OBJECT_TYPE_INFO(StaticMeshBrush);
DEFINE_OBJECT_GENERIC_FACTORY(StaticMeshBrush);
BEGIN_OBJECT_PROPERTY_MAP(StaticMeshBrush)
    PROPERTY_TABLE_MEMBER("StaticMeshName", PROPERTY_TYPE_STRING, 0, PropertyCallbackGetStaticMeshName, NULL, PropertyCallbackSetStaticMeshName, NULL, NULL, NULL)
    PROPERTY_TABLE_MEMBER_BOOL("Visible", 0, offsetof(StaticMeshBrush, m_visible), NULL, NULL)
    PROPERTY_TABLE_MEMBER_BOOL("Collidable", 0, offsetof(StaticMeshBrush, m_collidable), NULL, NULL)
    PROPERTY_TABLE_MEMBER_UINT("ShadowFlags", 0, offsetof(StaticMeshBrush, m_shadowFlags), NULL, NULL)
END_OBJECT_PROPERTY_MAP()

StaticMeshBrush::StaticMeshBrush(const ObjectTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_pStaticMesh(nullptr),
      m_visible(true),
      m_shadowFlags(0),
      m_collidable(false),
      m_pRenderProxy(nullptr),
      m_pCollisionObject(nullptr)
{

}

StaticMeshBrush::~StaticMeshBrush()
{
    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->Release();

    if (m_pCollisionObject != nullptr)
        m_pCollisionObject->Release();

    if (m_pStaticMesh != nullptr)
        m_pStaticMesh->Release();
}

bool StaticMeshBrush::Create(const float3 &position /* = float3::Zero */, const Quaternion &rotation /* = Quaternion::Identity */, const float3 &scale /* = float3::One */, const StaticMesh *pStaticMesh /* = nullptr */, bool visible /* = true */, bool collidable /* = true */, uint32 shadowFlags /* = ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS */)
{
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

    return Initialize();
}

bool StaticMeshBrush::Initialize()
{
    // handle load errors
    if (m_pStaticMesh == nullptr)
        m_pStaticMesh = g_pResourceManager->GetDefaultStaticMesh();

    // create render proxy
    if (m_visible)
        m_pRenderProxy = new StaticMeshRenderProxy(0, m_pStaticMesh, m_transform, m_shadowFlags);

    // and collision object
    if (m_collidable && m_pStaticMesh->GetCollisionShape() != nullptr)
        m_pCollisionObject = new Physics::StaticObject(0, m_pStaticMesh->GetCollisionShape(), m_transform);

    // calculate bounding box
    m_boundingBox = m_transform.TransformBoundingBox(m_pStaticMesh->GetBoundingBox());
    m_boundingSphere = m_transform.TransformBoundingSphere(m_pStaticMesh->GetBoundingSphere());
    return true;    
}

void StaticMeshBrush::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    if (m_pCollisionObject != nullptr)
        pWorld->GetPhysicsWorld()->AddObject(m_pCollisionObject);

    if (m_pRenderProxy != nullptr)
        pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);
}

void StaticMeshBrush::OnRemoveFromWorld(World *pWorld)
{
    if (m_pRenderProxy != nullptr)
        pWorld->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);

    if (m_pCollisionObject != nullptr)
        pWorld->GetPhysicsWorld()->RemoveObject(m_pCollisionObject);

    BaseClass::OnRemoveFromWorld(pWorld);
}

bool StaticMeshBrush::PropertyCallbackGetStaticMeshName(ThisClass *pEntity, const void *pUserData, String *pValue)
{
    pValue->Assign(pEntity->m_pStaticMesh->GetName());
    return true;
}

bool StaticMeshBrush::PropertyCallbackSetStaticMeshName(ThisClass *pEntity, const void *pUserData, const String *pValue)
{
    const StaticMesh *pStaticMesh = g_pResourceManager->GetStaticMesh(*pValue);
    if (pStaticMesh == NULL)
        return false;

    if (pEntity->m_pStaticMesh != NULL)
        pEntity->m_pStaticMesh->Release();

    pEntity->m_pStaticMesh = pStaticMesh;
    return true;
}
