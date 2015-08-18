#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/StaticMeshComponent.h"
#include "Engine/ResourceManager.h"
#include "Engine/Entity.h"
#include "Engine/World.h"
#include "Engine/Physics/KinematicObject.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Renderer/RenderProxies/StaticMeshRenderProxy.h"
#include "Renderer/RenderWorld.h"

DEFINE_COMPONENT_TYPEINFO(StaticMeshComponent);
DEFINE_COMPONENT_GENERIC_FACTORY(StaticMeshComponent);
BEGIN_COMPONENT_PROPERTIES(StaticMeshComponent)
    PROPERTY_TABLE_MEMBER("StaticMeshName", PROPERTY_TYPE_STRING, 0, PropertyCallbackGetMeshName, NULL, PropertyCallbackSetMeshName, NULL, PropertyCallbackStaticMeshChanged, NULL)
    PROPERTY_TABLE_MEMBER_BOOL("Visible", 0, offsetof(StaticMeshComponent, m_visible), NULL, NULL)
    PROPERTY_TABLE_MEMBER_BOOL("Collidable", 0, offsetof(StaticMeshComponent, m_collidable), NULL, NULL)
    PROPERTY_TABLE_MEMBER_UINT("ShadowFlags", 0, offsetof(StaticMeshComponent, m_shadowFlags), NULL, NULL)
END_COMPONENT_PROPERTIES()

StaticMeshComponent::StaticMeshComponent(const ComponentTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_pStaticMesh(nullptr),
      m_visible(true),
      m_collidable(false),
      m_shadowFlags(0),
      m_pRenderProxy(nullptr),
      m_pCollisionObject(nullptr)
{

}

StaticMeshComponent::~StaticMeshComponent()
{
    if (m_pCollisionObject != nullptr)
        m_pCollisionObject->Release();

    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->Release();

    if (m_pStaticMesh != nullptr)
        m_pStaticMesh->Release();
}

void StaticMeshComponent::SetStaticMesh(const StaticMesh *pStaticMesh)
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

void StaticMeshComponent::SetVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;
    PropertyCallbackVisibleChanged(this);
}

void StaticMeshComponent::SetShadowFlags(uint32 shadowFlags)
{
    if (m_shadowFlags == shadowFlags)
        return;

    m_shadowFlags = shadowFlags;

    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->SetShadowFlags(shadowFlags);
}

void StaticMeshComponent::SetCollidable(bool collidable)
{
    if (m_collidable == collidable)
        return;

    m_collidable = collidable;
    PropertyCallbackCollidableChanged(this);
}

void StaticMeshComponent::Create(const float3 &localPosition /* = float3::Zero */, const Quaternion &localRotation /* = Quaternion::Identity */, const float3 &localScale /* = float3::One */, const StaticMesh *pStaticMesh /* = nullptr */, bool visible /* = true */, bool collidable /* = true */, uint32 shadowFlags /* = ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS */)
{
    m_localPosition = localPosition;
    m_localRotation = localRotation;
    m_localScale = localScale;

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

    Initialize();
}

bool StaticMeshComponent::Initialize()
{
    if (!BaseClass::Initialize())
        return false;

    DebugAssert(m_pStaticMesh != nullptr);

    if (m_visible)
        m_pRenderProxy = new StaticMeshRenderProxy(0, m_pStaticMesh, Transform::Identity, m_shadowFlags);

    if (m_collidable && m_pStaticMesh->GetCollisionShape() != nullptr)
        m_pCollisionObject = new Physics::KinematicObject(0, m_pStaticMesh->GetCollisionShape(), Transform::Identity);

    return true;
}

void StaticMeshComponent::OnAddToEntity(Entity *pEntity)
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

    // update bounds, no need to notify the entity as it'll be recalculated when the add is complete
    SetBounds(worldTransform.TransformBoundingBox(m_pStaticMesh->GetBoundingBox()), worldTransform.TransformBoundingSphere(m_pStaticMesh->GetBoundingSphere()), false);
}

void StaticMeshComponent::OnRemoveFromEntity(Entity *pEntity)
{
    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->SetEntityID(0);
    if (m_pCollisionObject != nullptr)
        m_pCollisionObject->SetEntityID(0);    

    BaseClass::OnRemoveFromEntity(pEntity);
}

void StaticMeshComponent::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    if (m_pCollisionObject != nullptr)
        pWorld->GetPhysicsWorld()->AddObject(m_pCollisionObject);

    if (m_pRenderProxy != nullptr)
        pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);
}

void StaticMeshComponent::OnRemoveFromWorld(World *pWorld)
{
    if (m_pRenderProxy != nullptr)
        pWorld->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);
    
    if (m_pCollisionObject != nullptr)
        pWorld->GetPhysicsWorld()->RemoveObject(m_pCollisionObject);
    
    BaseClass::OnRemoveFromWorld(pWorld);
}

void StaticMeshComponent::OnLocalTransformChange()
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
    SetBounds(worldTransform.TransformBoundingBox(m_pStaticMesh->GetBoundingBox()), worldTransform.TransformBoundingSphere(m_pStaticMesh->GetBoundingSphere()), true);
}

void StaticMeshComponent::OnEntityTransformChange()
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
    SetBounds(worldTransform.TransformBoundingBox(m_pStaticMesh->GetBoundingBox()), worldTransform.TransformBoundingSphere(m_pStaticMesh->GetBoundingSphere()), true);
}

bool StaticMeshComponent::PropertyCallbackGetMeshName(ThisClass *pComponent, const void *pUserData, String *pValue)
{
    pValue->Assign(pComponent->m_pStaticMesh->GetName());
    return true;
}

bool StaticMeshComponent::PropertyCallbackSetMeshName(ThisClass *pComponent, const void *pUserData, const String *pValue)
{
    const StaticMesh *pStaticMesh = g_pResourceManager->GetStaticMesh(*pValue);
    if (pStaticMesh == NULL)
        return false;

    if (pComponent->m_pStaticMesh != nullptr)
        pComponent->m_pStaticMesh->Release();

    pComponent->m_pStaticMesh = pStaticMesh;
    return true;
}

void StaticMeshComponent::PropertyCallbackStaticMeshChanged(ThisClass *pComponent, const void *pUserData /*= nullptr*/)
{
    if (pComponent->m_visible)
    {
        DebugAssert(pComponent->m_pRenderProxy != nullptr);
        pComponent->m_pRenderProxy->SetStaticMesh(pComponent->m_pStaticMesh);
    }

    if (pComponent->m_collidable)
    {
        if (pComponent->m_pStaticMesh->GetCollisionShape() != nullptr)
        {
            if (pComponent->m_pCollisionObject != nullptr)
            {
                pComponent->m_pCollisionObject->SetCollisionShape(pComponent->m_pStaticMesh->GetCollisionShape());
            }
            else
            {
                pComponent->m_pCollisionObject = new Physics::KinematicObject((pComponent->IsAttachedToEntity()) ? pComponent->GetEntity()->GetEntityID() : 0, pComponent->m_pStaticMesh->GetCollisionShape(), pComponent->CalculateWorldTransform());

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

void StaticMeshComponent::PropertyCallbackVisibleChanged(ThisClass *pComponent, const void *pUserData /*= nullptr*/)
{
    if (pComponent->m_visible)
    {
        if (pComponent->m_pRenderProxy == nullptr)
        {
            pComponent->m_pRenderProxy = new StaticMeshRenderProxy((pComponent->IsAttachedToEntity()) ? pComponent->GetEntity()->GetEntityID() : 0, pComponent->m_pStaticMesh, pComponent->CalculateWorldTransform(), pComponent->m_shadowFlags);

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

void StaticMeshComponent::PropertyCallbackCollidableChanged(ThisClass *pComponent, const void *pUserData /*= nullptr*/)
{
    if (pComponent->m_collidable)
    {
        if (pComponent->m_pCollisionObject == nullptr && pComponent->m_pStaticMesh->GetCollisionShape() != nullptr)
        {
            pComponent->m_pCollisionObject = new Physics::KinematicObject((pComponent->IsAttachedToEntity()) ? pComponent->GetEntity()->GetEntityID() : 0, pComponent->m_pStaticMesh->GetCollisionShape(), pComponent->CalculateWorldTransform());

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

