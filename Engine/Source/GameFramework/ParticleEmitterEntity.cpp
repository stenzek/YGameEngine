#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/ParticleEmitterEntity.h"
#include "Engine/ParticleSystemRenderProxy.h"
#include "Engine/Entity.h"
#include "Engine/World.h"
#include "Renderer/RenderWorld.h"

DEFINE_ENTITY_TYPEINFO(ParticleEmitterEntity, 0);
DEFINE_ENTITY_GENERIC_FACTORY(ParticleEmitterEntity);
BEGIN_ENTITY_PROPERTIES(ParticleEmitterEntity)
END_ENTITY_PROPERTIES()
BEGIN_ENTITY_SCRIPT_FUNCTIONS(ParticleEmitterEntity)
END_ENTITY_SCRIPT_FUNCTIONS()

ParticleEmitterEntity::ParticleEmitterEntity(const EntityTypeInfo *pTypeInfo /*= &s_typeInfo*/)
    : BaseClass(pTypeInfo),
      m_pParticleSystem(nullptr),
      m_autoStart(true),
      m_pRenderProxy(nullptr),
      m_active(false)
{

}

ParticleEmitterEntity::~ParticleEmitterEntity()
{
    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->Release();

    if (m_pParticleSystem != nullptr)
        m_pParticleSystem->Release();
}

void ParticleEmitterEntity::SetParticleSystem(const ParticleSystem *pParticleSystem)
{
    if (m_pParticleSystem != pParticleSystem)
    {
        m_pParticleSystem->Release();
        m_pParticleSystem = pParticleSystem;
        m_pParticleSystem->AddRef();
        m_pRenderProxy->SetParticleSystem(pParticleSystem);
        
        if (IsInWorld())
            RegisterForUpdates(m_pParticleSystem->GetUpdateInterval());
    }
    else
    {
        m_pRenderProxy->Reset();
    }
}

void ParticleEmitterEntity::Create(uint32 entityID, const ParticleSystem *pParticleSystem, const float3 &position /* = float3::Zero */, const Quaternion &rotation /* = Quaternion::Identity */, const float3 &scale /* = float3::One */)
{
    DebugAssert(m_pParticleSystem == nullptr && pParticleSystem != nullptr);

    m_transform.Set(position, rotation, scale);

    m_pParticleSystem = pParticleSystem;
    m_pParticleSystem->AddRef();

    m_active = false;

    Initialize(entityID, EmptyString);
}

void ParticleEmitterEntity::Activate()
{
    if (m_active)
        return;

    m_active = true;

    if (IsInWorld())
        RegisterForUpdates(m_pParticleSystem->GetUpdateInterval());
}

void ParticleEmitterEntity::Deactivate()
{
    if (!m_active)
        return;

    m_active = false;

    if (IsInWorld())
        UnregisterForUpdates();
}

bool ParticleEmitterEntity::Initialize(uint32 entityID, const String &entityName)
{
    if (!BaseClass::Initialize(entityID, entityName))
        return false;

    // create render proxy
    DebugAssert(m_pRenderProxy == nullptr);
    m_pRenderProxy = new ParticleSystemRenderProxy(m_pParticleSystem, 0);
    return true;
}

void ParticleEmitterEntity::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    // add render proxy to world
    pWorld->GetRenderWorld()->AddRenderable(m_pRenderProxy);

    // if autostarting, ensure the entity is registered for ticks every frame
    if (m_autoStart)
        m_active = true;
    
    if (m_active)
        RegisterForUpdates(0.0f);
}

void ParticleEmitterEntity::OnRemoveFromWorld(World *pWorld)
{
    BaseClass::OnRemoveFromWorld(pWorld);

    // remove render proxy from world
    pWorld->GetRenderWorld()->RemoveRenderable(m_pRenderProxy);
}

void ParticleEmitterEntity::OnTransformChange()
{
    // pass through
    BaseClass::OnTransformChange();
}

void ParticleEmitterEntity::OnComponentBoundsChange()
{
    // fix up bounds
    BaseClass::SetBounds(m_pRenderProxy->GetParticleSystemBoundingBox(), Sphere::FromAABox(m_pRenderProxy->GetParticleSystemBoundingBox()), true);
}

void ParticleEmitterEntity::Update(float timeSinceLastUpdate)
{
    // update the particle system
    m_pRenderProxy->Update(&m_transform, timeSinceLastUpdate);

    // update bounding box
    AABox systemBoundingBox(m_pRenderProxy->GetParticleSystemBoundingBox());
    if (systemBoundingBox != m_boundingBox)
        BaseClass::SetBounds(systemBoundingBox, Sphere::FromAABox(systemBoundingBox));
}
