#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/ParticleEmitterComponent.h"
#include "Engine/ParticleSystemRenderProxy.h"
#include "Engine/Entity.h"
#include "Engine/World.h"
#include "Renderer/RenderWorld.h"

DEFINE_COMPONENT_TYPEINFO(ParticleEmitterComponent);
DEFINE_COMPONENT_GENERIC_FACTORY(ParticleEmitterComponent);
BEGIN_COMPONENT_PROPERTIES(ParticleEmitterComponent)
END_COMPONENT_PROPERTIES()

ParticleEmitterComponent::ParticleEmitterComponent(const ComponentTypeInfo *pTypeInfo /*= &s_typeInfo*/)
    : BaseClass(pTypeInfo),
      m_pParticleSystem(nullptr),
      m_autoStart(true),
      m_pRenderProxy(nullptr),
      m_active(false)
{

}

ParticleEmitterComponent::~ParticleEmitterComponent()
{
    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->Release();

    if (m_pParticleSystem != nullptr)
        m_pParticleSystem->Release();
}

void ParticleEmitterComponent::SetParticleSystem(const ParticleSystem *pParticleSystem)
{
    if (m_pParticleSystem != pParticleSystem)
    {
        m_pParticleSystem->Release();
        m_pParticleSystem = pParticleSystem;
        m_pParticleSystem->AddRef();
        m_pRenderProxy->SetParticleSystem(pParticleSystem);
    }
    else
    {
        m_pRenderProxy->Reset();
    }
}

bool ParticleEmitterComponent::Create(const ParticleSystem *pParticleSystem, const float3 &offsetPosition /* = float3::Zero */, const Quaternion &offsetRotation /* = Quaternion::Identity */)
{
    DebugAssert(m_pParticleSystem == nullptr && pParticleSystem != nullptr);

    m_pParticleSystem = pParticleSystem;
    m_pParticleSystem->AddRef();

    m_localPosition = offsetPosition;
    m_localRotation = offsetRotation;
    m_localScale = float3::One;
    m_active = false;

    return Initialize();
}

void ParticleEmitterComponent::Activate()
{
    if (m_active)
        return;

    m_active = true;
    if (IsAttachedToEntity())
        m_pEntity->RegisterForUpdates(0.0f);
}

void ParticleEmitterComponent::Deactivate()
{
    m_active = false;
}

bool ParticleEmitterComponent::Initialize()
{
    if (!BaseClass::Initialize())
        return false;

    // create render proxy
    DebugAssert(m_pRenderProxy == nullptr);
    m_pRenderProxy = new ParticleSystemRenderProxy(m_pParticleSystem, 0);
    return true;
}

void ParticleEmitterComponent::OnAddToEntity(Entity *pEntity)
{
    BaseClass::OnAddToEntity(pEntity);

    // update cached transform
    m_cachedTransform = Transform::ConcatenateTransforms(Transform(m_localPosition, m_localRotation, m_localScale), pEntity->GetTransform());

    // update render proxy's entity id
    m_pRenderProxy->SetEntityID(pEntity->GetEntityID());

    // if autostarting, ensure the entity is registered for ticks every frame
    if (m_autoStart && pEntity->IsInWorld())
    {
        pEntity->RegisterForUpdates(0.0f);
        m_active = true;
    }
}

void ParticleEmitterComponent::OnRemoveFromEntity(Entity *pEntity)
{
    BaseClass::OnRemoveFromEntity(pEntity);

    // ensure we're deactivated
    m_active = false;
}

void ParticleEmitterComponent::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    // add render proxy to world
    pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);

    // if autostarting, ensure the entity is registered for ticks every frame
    if (m_autoStart)
        m_active = true;
    
    if (m_active)
        m_pEntity->RegisterForUpdates(0.0f);
}

void ParticleEmitterComponent::OnRemoveFromWorld(World *pWorld)
{
    BaseClass::OnRemoveFromWorld(pWorld);

    // remove render proxy from world
    pWorld->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);
}

void ParticleEmitterComponent::OnLocalTransformChange()
{
    // update cached transform, if attached to entity
    if (m_pEntity != nullptr)
        m_cachedTransform = Transform::ConcatenateTransforms(Transform(m_localPosition, m_localRotation, m_localScale), m_pEntity->GetTransform());
}

void ParticleEmitterComponent::OnEntityTransformChange()
{
    // update cached transform
    m_cachedTransform = Transform::ConcatenateTransforms(Transform(m_localPosition, m_localRotation, m_localScale), m_pEntity->GetTransform());
}

void ParticleEmitterComponent::Update(float timeSinceLastUpdate)
{
    if (m_active)
    {
        // update the particle system
        m_pRenderProxy->Update(&m_cachedTransform, timeSinceLastUpdate);

        // compare bounds
        AABox boundingBox(m_pRenderProxy->GetParticleSystemBoundingBox());
        if (boundingBox != m_boundingBox)
            SetBounds(boundingBox, Sphere::FromAABox(boundingBox));
    }
}
