#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/BlockMeshComponent.h"
#include "Engine/ResourceManager.h"
#include "Engine/Entity.h"
#include "Engine/World.h"
#include "Engine/Physics/StaticObject.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Renderer/RenderProxies/BlockMeshRenderProxy.h"
#include "Renderer/RenderWorld.h"

DEFINE_COMPONENT_TYPEINFO(BlockMeshComponent);
DEFINE_COMPONENT_GENERIC_FACTORY(BlockMeshComponent);
BEGIN_COMPONENT_PROPERTIES(BlockMeshComponent)
    PROPERTY_TABLE_MEMBER("BlockMeshName", PROPERTY_TYPE_STRING, 0, PropertyCallbackGetMeshName, NULL, PropertyCallbackSetMeshName, NULL, PropertyCallbackBlockMeshChanged, NULL)
    PROPERTY_TABLE_MEMBER_BOOL("Visible", 0, offsetof(BlockMeshComponent, m_visible), NULL, NULL)
    PROPERTY_TABLE_MEMBER_BOOL("Collidable", 0, offsetof(BlockMeshComponent, m_collidable), NULL, NULL)
    PROPERTY_TABLE_MEMBER_UINT("ShadowFlags", 0, offsetof(BlockMeshComponent, m_shadowFlags), NULL, NULL)
END_COMPONENT_PROPERTIES()

BlockMeshComponent::BlockMeshComponent(const ComponentTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_pBlockMesh(nullptr),
      m_visible(true),
      m_collidable(false),
      m_shadowFlags(0),
      m_pRenderProxy(nullptr),
      m_pCollisionObject(nullptr)
{

}

BlockMeshComponent::~BlockMeshComponent()
{
    if (m_pCollisionObject != nullptr)
        m_pCollisionObject->Release();

    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->Release();

    if (m_pBlockMesh != nullptr)
        m_pBlockMesh->Release();
}

void BlockMeshComponent::SetBlockMesh(const BlockMesh *pBlockMesh)
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

void BlockMeshComponent::SetVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;
    PropertyCallbackVisibleChanged(this);
}

void BlockMeshComponent::SetShadowFlags(uint32 shadowFlags)
{
    if (m_shadowFlags == shadowFlags)
        return;

    m_shadowFlags = shadowFlags;

    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->SetShadowFlags(shadowFlags);
}

void BlockMeshComponent::SetCollidable(bool collidable)
{
    if (m_collidable == collidable)
        return;

    m_collidable = collidable;
    PropertyCallbackCollidableChanged(this);
}

void BlockMeshComponent::Create(const float3 &localPosition /* = float3::Zero */, const Quaternion &localRotation /* = Quaternion::Identity */, const float3 &localScale /* = float3::One */, const BlockMesh *pBlockMesh /* = nullptr */, bool visible /* = true */, bool collidable /* = true */, uint32 shadowFlags /* = ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS */)
{
    m_localPosition = localPosition;
    m_localRotation = localRotation;
    m_localScale = localScale;

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

    Initialize();
}

bool BlockMeshComponent::Initialize()
{
    if (!BaseClass::Initialize())
        return false;

    DebugAssert(m_pBlockMesh != nullptr);

    if (m_visible)
        m_pRenderProxy = new BlockMeshRenderProxy(0, m_pBlockMesh, Transform::Identity, m_shadowFlags);

    if (m_collidable && m_pBlockMesh->GetCollisionShape() != nullptr)
        m_pCollisionObject = new Physics::StaticObject(0, m_pBlockMesh->GetCollisionShape(), Transform::Identity);

    return true;
}

void BlockMeshComponent::OnAddToEntity(Entity *pEntity)
{
    BaseClass::OnAddToEntity(pEntity);

    // calculate the transform
    Transform worldTransform(CalculateWorldTransform());

    // update entity ids
    if (m_pRenderProxy != nullptr)
    {
        m_pRenderProxy->SetEntityID(pEntity->GetEntityID());
        m_pRenderProxy->SetTransform(worldTransform);
    }
    if (m_pCollisionObject != nullptr)
    {
        m_pCollisionObject->SetEntityID(pEntity->GetEntityID());
        m_pCollisionObject->SetTransform(worldTransform);
    }

    // add to world
    if (pEntity->IsInWorld())
    {
        // add to physics + render world
        if (m_pCollisionObject != nullptr)
            pEntity->GetWorld()->GetPhysicsWorld()->AddObject(m_pCollisionObject);

        if (m_pRenderProxy != nullptr)
            pEntity->GetWorld()->GetRenderWorld()->AddRenderable(m_pRenderProxy);
    }

    // update bounds, no need to notify the entity as it'll be recalculated when the add is complete
    SetBounds(worldTransform.TransformBoundingBox(m_pBlockMesh->GetBoundingBox()), worldTransform.TransformBoundingSphere(m_pBlockMesh->GetBoundingSphere()), false);
}

void BlockMeshComponent::OnRemoveFromEntity(Entity *pEntity)
{
    if (pEntity->IsInWorld())
    {
        if (m_pCollisionObject != nullptr)
            pEntity->GetWorld()->GetPhysicsWorld()->RemoveObject(m_pCollisionObject);

        if (m_pRenderProxy != nullptr)
            pEntity->GetWorld()->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);
    }

    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->SetEntityID(0);
    if (m_pCollisionObject != nullptr)
        m_pCollisionObject->SetEntityID(0);    

    BaseClass::OnRemoveFromEntity(pEntity);
}

void BlockMeshComponent::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    if (m_pCollisionObject != nullptr)
        pWorld->GetPhysicsWorld()->AddObject(m_pCollisionObject);

    if (m_pRenderProxy != nullptr)
        pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);
}

void BlockMeshComponent::OnRemoveFromWorld(World *pWorld)
{
    if (m_pRenderProxy != nullptr)
        pWorld->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);
    
    if (m_pCollisionObject != nullptr)
        pWorld->GetPhysicsWorld()->RemoveObject(m_pCollisionObject);
    
    BaseClass::OnRemoveFromWorld(pWorld);
}

void BlockMeshComponent::OnLocalTransformChange()
{
    BaseClass::OnLocalTransformChange();

    // recalculate the world transform
    Transform worldTransform(CalculateWorldTransform());

    // update proxies
    if (m_pCollisionObject != nullptr)
        m_pCollisionObject->SetTransform(worldTransform);
    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->SetTransform(worldTransform);

    // change bounding box
    SetBounds(worldTransform.TransformBoundingBox(m_pBlockMesh->GetBoundingBox()), worldTransform.TransformBoundingSphere(m_pBlockMesh->GetBoundingSphere()), true);
}

void BlockMeshComponent::OnEntityTransformChange()
{
    BaseClass::OnEntityTransformChange();

    // recalculate the world transform
    Transform worldTransform(CalculateWorldTransform());

    // update proxies
    if (m_pCollisionObject != nullptr)
        m_pCollisionObject->SetTransform(worldTransform);
    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->SetTransform(worldTransform);

    // change bounding box
    SetBounds(worldTransform.TransformBoundingBox(m_pBlockMesh->GetBoundingBox()), worldTransform.TransformBoundingSphere(m_pBlockMesh->GetBoundingSphere()), true);
}

bool BlockMeshComponent::PropertyCallbackGetMeshName(ThisClass *pComponent, const void *pUserData, String *pValue)
{
    pValue->Assign(pComponent->m_pBlockMesh->GetName());
    return true;
}

bool BlockMeshComponent::PropertyCallbackSetMeshName(ThisClass *pComponent, const void *pUserData, const String *pValue)
{
    const BlockMesh *pBlockMesh = g_pResourceManager->GetBlockMesh(*pValue);
    if (pBlockMesh == NULL)
        return false;

    if (pComponent->m_pBlockMesh != nullptr)
        pComponent->m_pBlockMesh->Release();

    pComponent->m_pBlockMesh = pBlockMesh;
    return true;
}

void BlockMeshComponent::PropertyCallbackBlockMeshChanged(ThisClass *pComponent, const void *pUserData /*= nullptr*/)
{
    if (pComponent->m_visible)
    {
        DebugAssert(pComponent->m_pRenderProxy != nullptr);
        pComponent->m_pRenderProxy->SetBlockMesh(pComponent->m_pBlockMesh);
    }

    if (pComponent->m_collidable)
    {
        if (pComponent->m_pBlockMesh->GetCollisionShape() != nullptr)
        {
            if (pComponent->m_pCollisionObject != nullptr)
            {
                pComponent->m_pCollisionObject->SetCollisionShape(pComponent->m_pBlockMesh->GetCollisionShape());
            }
            else
            {
                pComponent->m_pCollisionObject = new Physics::StaticObject((pComponent->IsAttachedToEntity()) ? pComponent->GetEntity()->GetEntityID() : 0, pComponent->m_pBlockMesh->GetCollisionShape(), pComponent->CalculateWorldTransform());

                if (pComponent->IsAttachedToEntity() && pComponent->GetEntity()->IsInWorld())
                    pComponent->GetEntity()->GetWorld()->GetPhysicsWorld()->AddObject(pComponent->m_pCollisionObject);
            }
        }
        else
        {
            if (pComponent->m_pCollisionObject != nullptr)
            {
                if (pComponent->IsAttachedToEntity() && pComponent->GetEntity()->IsInWorld())
                    pComponent->GetEntity()->GetWorld()->GetPhysicsWorld()->RemoveObject(pComponent->m_pCollisionObject);

                pComponent->m_pCollisionObject->Release();
                pComponent->m_pCollisionObject = nullptr;
            }
        }
    }
}

void BlockMeshComponent::PropertyCallbackVisibleChanged(ThisClass *pComponent, const void *pUserData /*= nullptr*/)
{
    if (pComponent->m_visible)
    {
        if (pComponent->m_pRenderProxy == nullptr)
        {
            pComponent->m_pRenderProxy = new BlockMeshRenderProxy((pComponent->IsAttachedToEntity()) ? pComponent->GetEntity()->GetEntityID() : 0, pComponent->m_pBlockMesh, pComponent->CalculateWorldTransform(), pComponent->m_shadowFlags);

            if (pComponent->IsAttachedToEntity() && pComponent->GetEntity()->IsInWorld())
                pComponent->GetEntity()->GetWorld()->GetRenderWorld()->AddRenderable(pComponent->m_pRenderProxy);
        }
    }
    else
    {
        if (pComponent->m_pRenderProxy != nullptr)
        {
            if (pComponent->IsAttachedToEntity() && pComponent->GetEntity()->IsInWorld())
                pComponent->GetEntity()->GetWorld()->GetRenderWorld()->RemoveRenderable(pComponent->m_pRenderProxy);

            pComponent->m_pRenderProxy->Release();
            pComponent->m_pRenderProxy = nullptr;
        }
    }
}

void BlockMeshComponent::PropertyCallbackCollidableChanged(ThisClass *pComponent, const void *pUserData /*= nullptr*/)
{
    if (pComponent->m_collidable)
    {
        if (pComponent->m_pCollisionObject == nullptr && pComponent->m_pBlockMesh->GetCollisionShape() != nullptr)
        {
            pComponent->m_pCollisionObject = new Physics::StaticObject((pComponent->IsAttachedToEntity()) ? pComponent->GetEntity()->GetEntityID() : 0, pComponent->m_pBlockMesh->GetCollisionShape(), pComponent->CalculateWorldTransform());

            if (pComponent->IsAttachedToEntity() && pComponent->GetEntity()->IsInWorld())
                pComponent->GetEntity()->GetWorld()->GetPhysicsWorld()->AddObject(pComponent->m_pCollisionObject);
        }
    }
    else
    {
        if (pComponent->m_pCollisionObject != nullptr)
        {
            if (pComponent->IsAttachedToEntity() && pComponent->GetEntity()->IsInWorld())
                pComponent->GetEntity()->GetWorld()->GetPhysicsWorld()->RemoveObject(pComponent->m_pCollisionObject);

            pComponent->m_pCollisionObject->Release();
            pComponent->m_pCollisionObject = nullptr;
        }
    }
}

