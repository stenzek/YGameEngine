#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/StaticMeshRigidBodyEntity.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Physics/RigidBody.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/ResourceManager.h"
#include "Renderer/RenderProxies/StaticMeshRenderProxy.h"
#include "Renderer/RenderWorld.h"
Log_SetChannel(StaticMeshRigidBodyEntity);

DEFINE_ENTITY_TYPEINFO(StaticMeshRigidBodyEntity, 0);
DEFINE_ENTITY_GENERIC_FACTORY(StaticMeshRigidBodyEntity);
BEGIN_ENTITY_PROPERTIES(StaticMeshRigidBodyEntity)
    PROPERTY_TABLE_MEMBER("StaticMeshName", PROPERTY_TYPE_STRING, 0, PropertyCallbackGetStaticMesh, NULL, PropertyCallbackSetStaticMesh, NULL, NULL, NULL)
    PROPERTY_TABLE_MEMBER_BOOL("Visible", 0, offsetof(StaticMeshRigidBodyEntity, m_visible), NULL, NULL)
    PROPERTY_TABLE_MEMBER_UINT("ShadowFlags", 0, offsetof(StaticMeshRigidBodyEntity, m_shadowFlags), NULL, NULL)
    PROPERTY_TABLE_MEMBER_FLOAT("Mass", 0, offsetof(StaticMeshRigidBodyEntity, m_mass), PropertyCallbackMassChanged, NULL)
    PROPERTY_TABLE_MEMBER_FLOAT("Friction", 0, offsetof(StaticMeshRigidBodyEntity, m_friction), PropertyCallbackMassChanged, NULL)
    PROPERTY_TABLE_MEMBER_FLOAT("RollingFriction", 0, offsetof(StaticMeshRigidBodyEntity, m_rollingFriction), PropertyCallbackMassChanged, NULL)
END_ENTITY_PROPERTIES()
BEGIN_ENTITY_SCRIPT_FUNCTIONS(StaticMeshRigidBodyEntity)
END_ENTITY_SCRIPT_FUNCTIONS()

StaticMeshRigidBodyEntity::StaticMeshRigidBodyEntity(const EntityTypeInfo *pTypeInfo /*= &s_TypeInfo*/)
    : BaseClass(pTypeInfo),
      m_visible(true),
      m_pStaticMesh(nullptr),
      m_shadowFlags(0),
      m_mass(1.0f),
      m_friction(0.5f),
      m_rollingFriction(1.0f),
      m_pRigidBody(nullptr),
      m_pRenderProxy(nullptr)
{
    // force mobility to movable
    m_mobility = ENTITY_MOBILITY_MOVABLE;
}

StaticMeshRigidBodyEntity::~StaticMeshRigidBodyEntity()
{
    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->Release();

    if (m_pRigidBody != nullptr)
        m_pRigidBody->Release();

    m_pStaticMesh->Release();
}

void StaticMeshRigidBodyEntity::Create(uint32 entityID, const float3 &position /* = float3::Zero */, const Quaternion &rotation /* = Quaternion::Identity */, const float3 &scale /* = float3::One */, const StaticMesh *pStaticMesh /* = nullptr */, bool visible /* = true */, uint32 shadowFlags /* = ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS */, float mass /* = 1.0f */, float friction /* = 0.5f */, float rollingFriction /* = 1.0f */)
{
    m_transform.Set(position, rotation, scale);

    if ((m_pStaticMesh = pStaticMesh) != nullptr)
        m_pStaticMesh->AddRef();
    else
        m_pStaticMesh = g_pResourceManager->GetDefaultStaticMesh();

    m_shadowFlags = shadowFlags;
    m_mass = mass;
    m_friction = friction;
    m_rollingFriction = rollingFriction;

    Initialize(entityID, EmptyString);
}

bool StaticMeshRigidBodyEntity::Initialize(uint32 entityID, const String &entityName)
{
    if (!BaseClass::Initialize(entityID, entityName))
        return false;

    if (m_pStaticMesh == nullptr)
        m_pStaticMesh = g_pResourceManager->GetDefaultStaticMesh();

    UpdateRenderProxy();
    UpdateCollisionObject();

    m_boundingBox = m_transform.TransformBoundingBox(m_pStaticMesh->GetBoundingBox());
    m_boundingSphere = m_transform.TransformBoundingSphere(m_pStaticMesh->GetBoundingSphere());
    return true;
}

void StaticMeshRigidBodyEntity::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    if (m_pRigidBody != NULL)
        pWorld->GetPhysicsWorld()->AddObject(m_pRigidBody);

    if (m_pRenderProxy != NULL)
        pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);
}

void StaticMeshRigidBodyEntity::OnRemoveFromWorld(World *pWorld)
{
    if (m_pRenderProxy != NULL)
        pWorld->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);

    if (m_pRigidBody != NULL)
        pWorld->GetPhysicsWorld()->RemoveObject(m_pRigidBody);

    BaseClass::OnRemoveFromWorld(pWorld);
}

void StaticMeshRigidBodyEntity::OnTransformChange()
{
    BaseClass::OnTransformChange();

    if (m_pRigidBody != NULL && m_pRigidBody->GetTransform() != GetTransform())
        m_pRigidBody->SetTransform(GetTransform());

    if (m_pRenderProxy != NULL)
        m_pRenderProxy->SetTransform(GetTransform());

    // call entity set bounds with the transformed bounds, this will merge with components (if any)
    Entity::SetBounds(m_transform.TransformBoundingBox(m_pStaticMesh->GetBoundingBox()), m_transform.TransformBoundingSphere(m_pStaticMesh->GetBoundingSphere()), true);
}

void StaticMeshRigidBodyEntity::OnComponentBoundsChange()
{
    // call entity set bounds with the transformed bounds, this will merge with components (if any)
    Entity::SetBounds(m_transform.TransformBoundingBox(m_pStaticMesh->GetBoundingBox()), m_transform.TransformBoundingSphere(m_pStaticMesh->GetBoundingSphere()), true);
}

void StaticMeshRigidBodyEntity::SetStaticMesh(const StaticMesh *pStaticMesh)
{
    if (m_pStaticMesh == pStaticMesh)
        return;

    if (m_pStaticMesh != nullptr)
        m_pStaticMesh->Release();

    m_pStaticMesh = pStaticMesh;
    m_pStaticMesh->AddRef();

    UpdateCollisionObject();
    UpdateRenderProxy();

    SetBounds(m_transform.TransformBoundingBox(m_pStaticMesh->GetBoundingBox()), m_transform.TransformBoundingSphere(m_pStaticMesh->GetBoundingSphere()), true);
}

void StaticMeshRigidBodyEntity::SetShadowFlags(uint32 shadowFlags)
{
    m_shadowFlags = shadowFlags;

    if (m_pRenderProxy != NULL)
        m_pRenderProxy->SetShadowFlags(shadowFlags);
}

void StaticMeshRigidBodyEntity::UpdateCollisionObject()
{
    if (m_pStaticMesh->GetCollisionShape() != NULL)
    {
        // if does not have collision object and new mesh does
        if (m_pRigidBody == NULL)
        {
            m_pRigidBody = new Physics::RigidBody(m_entityID, m_pStaticMesh->GetCollisionShape(), GetTransform());
            m_pRigidBody->SetMass(m_mass);
            m_pRigidBody->SetFriction(m_friction);
            m_pRigidBody->SetRollingFriction(m_rollingFriction);
            m_pRigidBody->SetCallbackFunction(reinterpret_cast<Physics::RigidBody::UpdateTransformCallbackFunction>(&StaticMeshRigidBodyEntity::PhysicsCallbackTransformChanged), reinterpret_cast<void *>(this));

            if (IsInWorld())
                m_pWorld->GetPhysicsWorld()->AddObject(m_pRigidBody);
        }
        // has one already,  just update it
        else
        {
            m_pRigidBody->SetCollisionShape(m_pStaticMesh->GetCollisionShape());
        }
    }
    else
    {
        // if has collision object and new mesh does not
        if (m_pRigidBody != NULL)
        {
            if (IsInWorld())
                m_pWorld->GetPhysicsWorld()->RemoveObject(m_pRigidBody);

            m_pRigidBody->Release();
            m_pRigidBody = NULL;
        }
    }
}

void StaticMeshRigidBodyEntity::UpdateRenderProxy()
{
    if (m_pRenderProxy != NULL)
    {
        m_pRenderProxy->SetStaticMesh(m_pStaticMesh);
    }
    else
    {
        m_pRenderProxy = new StaticMeshRenderProxy(GetEntityID(), m_pStaticMesh, GetTransform(), m_shadowFlags); 
        m_pRenderProxy->SetShadowFlags(m_shadowFlags);
    }
}

bool StaticMeshRigidBodyEntity::PropertyCallbackGetStaticMesh(ThisClass *pEntity, const void *pUserData, String *pValue)
{
    pValue->Assign(pEntity->m_pStaticMesh->GetName());
    return true;
}

bool StaticMeshRigidBodyEntity::PropertyCallbackSetStaticMesh(ThisClass *pEntity, const void *pUserData, const String *pValue)
{
    const StaticMesh *pStaticMesh = g_pResourceManager->GetStaticMesh(*pValue);
    if (pStaticMesh == NULL)
        return false;

    if (pEntity->m_pStaticMesh != nullptr)
        pEntity->m_pStaticMesh->Release();

    pEntity->m_pStaticMesh = pStaticMesh;
    pEntity->m_pStaticMesh->AddRef();
    return true;
}

void StaticMeshRigidBodyEntity::PropertyCallbackStaticMeshChanged(ThisClass *pEntity, const void *pUserData)
{
    pEntity->UpdateCollisionObject();
    pEntity->UpdateRenderProxy();
    pEntity->SetBounds(pEntity->m_transform.TransformBoundingBox(pEntity->m_pStaticMesh->GetBoundingBox()), pEntity->m_transform.TransformBoundingSphere(pEntity->m_pStaticMesh->GetBoundingSphere()), true);
}

void StaticMeshRigidBodyEntity::PropertyCallbackEnableCollisionChanged(ThisClass *pEntity, const void *pUserData)
{
    pEntity->UpdateCollisionObject();
}

void StaticMeshRigidBodyEntity::PropertyCallbackMassChanged(ThisClass *pEntity, const void *pUserData)
{
    if (pEntity->m_pRigidBody != nullptr)
        pEntity->m_pRigidBody->SetMass(pEntity->m_mass);
}

void StaticMeshRigidBodyEntity::PropertyCallbackFrictionChanged(ThisClass *pEntity, const void *pUserData)
{
    if (pEntity->m_pRigidBody != nullptr)
        pEntity->m_pRigidBody->SetMass(pEntity->m_friction);
}

void StaticMeshRigidBodyEntity::PropertyCallbackRollingFrictionChanged(ThisClass *pEntity, const void *pUserData)
{
    if (pEntity->m_pRigidBody != nullptr)
        pEntity->m_pRigidBody->SetMass(pEntity->m_rollingFriction);
}

void StaticMeshRigidBodyEntity::SetMass(float mass)
{
    m_mass = mass;

    if (m_pRigidBody != NULL)
        m_pRigidBody->SetMass(mass);
}

void StaticMeshRigidBodyEntity::SetFriction(float friction)
{
    m_friction = friction;

    if (m_pRigidBody != NULL)
        m_pRigidBody->SetFriction(friction);
}

void StaticMeshRigidBodyEntity::SetRollingFriction(float rollingFriction)
{
    m_rollingFriction = rollingFriction;

    if (m_pRigidBody != NULL)
        m_pRigidBody->SetRollingFriction(rollingFriction);
}

void StaticMeshRigidBodyEntity::ApplyCentralImpulse(const float3 &direction)
{
    if (m_pRigidBody != NULL)
        m_pRigidBody->ApplyCentralImpulse(direction);
}

void StaticMeshRigidBodyEntity::PhysicsCallbackTransformChanged(ThisClass *pEntity, const Transform &physicsTransform)
{
    //Log_DevPrintf("pos update %s", StringConverter::Float3ToString(physicsTransform.GetPosition()).GetCharArray());
    //pEntity->SetTransform(physicsTransform);

    // manually implemented as to not re-call the rigid body
    pEntity->m_transform = physicsTransform;
    pEntity->BaseClass::OnTransformChange();

    if (pEntity->m_pRenderProxy != nullptr)
        pEntity->m_pRenderProxy->SetTransform(physicsTransform);

    pEntity->SetBounds(physicsTransform.TransformBoundingBox(pEntity->m_pStaticMesh->GetBoundingBox()), physicsTransform.TransformBoundingSphere(pEntity->m_pStaticMesh->GetBoundingSphere()), true);
}

void StaticMeshRigidBodyEntity::ResetForces()
{
    if (m_pRigidBody != nullptr)
        m_pRigidBody->ResetForces();
}

void StaticMeshRigidBodyEntity::ResetVelocity()
{
    if (m_pRigidBody != nullptr)
        m_pRigidBody->ResetVelocity();
}

void StaticMeshRigidBodyEntity::ResetForcesAndVelocity()
{
    if (m_pRigidBody != nullptr)
    {
        m_pRigidBody->ResetForces();
        m_pRigidBody->ResetVelocity();
    }
}

