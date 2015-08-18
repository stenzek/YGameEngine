#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/BlockMeshBrush.h"
#include "Engine/BlockMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/ResourceManager.h"
#include "Engine/Physics/StaticObject.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Renderer/RenderProxies/BlockMeshRenderProxy.h"
#include "Renderer/RenderWorld.h"
Log_SetChannel(BlockMeshBrush);

DEFINE_OBJECT_TYPE_INFO(BlockMeshBrush);
DEFINE_OBJECT_GENERIC_FACTORY(BlockMeshBrush);
BEGIN_OBJECT_PROPERTY_MAP(BlockMeshBrush)
    PROPERTY_TABLE_MEMBER("BlockMeshName", PROPERTY_TYPE_STRING, 0, PropertyCallbackGetBlockMeshName, NULL, PropertyCallbackSetBlockMeshName, NULL, NULL, NULL)
    PROPERTY_TABLE_MEMBER_BOOL("Visible", 0, offsetof(BlockMeshBrush, m_visible), NULL, NULL)
    PROPERTY_TABLE_MEMBER_BOOL("Collidable", 0, offsetof(BlockMeshBrush, m_collidable), NULL, NULL)
    PROPERTY_TABLE_MEMBER_UINT("ShadowFlags", 0, offsetof(BlockMeshBrush, m_shadowFlags), NULL, NULL)
END_OBJECT_PROPERTY_MAP()

BlockMeshBrush::BlockMeshBrush(const ObjectTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_pBlockMesh(nullptr),
      m_visible(true),
      m_shadowFlags(0),
      m_collidable(false),
      m_pRenderProxy(nullptr),
      m_pCollisionObject(nullptr)
{

}

BlockMeshBrush::~BlockMeshBrush()
{
    if (m_pRenderProxy != NULL)
        m_pRenderProxy->Release();

    if (m_pCollisionObject != NULL)
        m_pCollisionObject->Release();

    if (m_pBlockMesh != NULL)
        m_pBlockMesh->Release();
}

bool BlockMeshBrush::Create(const float3 &position /* = float3::Zero */, const Quaternion &rotation /* = Quaternion::Identity */, const float3 &scale /* = float3::One */, const BlockMesh *pBlockMesh /* = nullptr */, bool visible /* = true */, bool collidable /* = true */, uint32 shadowFlags /* = ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS */)
{
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

    // done
    return Initialize();
}

bool BlockMeshBrush::Initialize()
{
    // handle load errors
    if (m_pBlockMesh == nullptr)
        m_pBlockMesh = g_pResourceManager->GetDefaultBlockMesh();

    // create render proxy
    if (m_visible)
        m_pRenderProxy = new BlockMeshRenderProxy(0, m_pBlockMesh, m_transform, m_shadowFlags);

    // create collision object
    if (m_collidable && m_pBlockMesh->GetCollisionShape() != nullptr)
        m_pCollisionObject = new Physics::StaticObject(0, m_pBlockMesh->GetCollisionShape(), m_transform);

    // calculate bounding box
    m_boundingBox = m_transform.TransformBoundingBox(m_pBlockMesh->GetBoundingBox());
    m_boundingSphere = m_transform.TransformBoundingSphere(m_pBlockMesh->GetBoundingSphere());
    return true;
}

void BlockMeshBrush::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    if (m_pCollisionObject != nullptr)
        pWorld->GetPhysicsWorld()->AddObject(m_pCollisionObject);

    if (m_pRenderProxy != nullptr)
        pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);
}

void BlockMeshBrush::OnRemoveFromWorld(World *pWorld)
{
    if (m_pRenderProxy != nullptr)
        pWorld->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);

    if (m_pCollisionObject != nullptr)
        pWorld->GetPhysicsWorld()->RemoveObject(m_pCollisionObject);

    BaseClass::OnRemoveFromWorld(pWorld);
}

bool BlockMeshBrush::PropertyCallbackGetBlockMeshName(ThisClass *pEntity, const void *pUserData, String *pValue)
{
    pValue->Assign(pEntity->m_pBlockMesh->GetName());
    return true;
}

bool BlockMeshBrush::PropertyCallbackSetBlockMeshName(ThisClass *pEntity, const void *pUserData, const String *pValue)
{
    const BlockMesh *pBlockMesh = g_pResourceManager->GetBlockMesh(*pValue);
    if (pBlockMesh == NULL)
        return false;

    if (pEntity->m_pBlockMesh != NULL)
        pEntity->m_pBlockMesh->Release();

    pEntity->m_pBlockMesh = pBlockMesh;
    return true;
}
