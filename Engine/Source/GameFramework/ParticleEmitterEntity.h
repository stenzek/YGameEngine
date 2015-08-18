#pragma once
#include "Engine/Entity.h"
#include "Engine/ParticleSystem.h"

class ParticleSystemRenderProxy;

class ParticleEmitterEntity : public Entity
{
    DECLARE_ENTITY_TYPEINFO(ParticleEmitterEntity, Entity);
    DECLARE_ENTITY_GENERIC_FACTORY(ParticleEmitterEntity);

public:
    ParticleEmitterEntity(const EntityTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~ParticleEmitterEntity();

    // accessors
    bool IsActive() const { return m_active; }

    // particle system
    const ParticleSystem *GetParticleSystem() const { return m_pParticleSystem; }
    void SetParticleSystem(const ParticleSystem *pParticleSystem);

    // create new instance
    void Create(uint32 entityID, const ParticleSystem *pParticleSystem, const float3 &position = float3::Zero, const Quaternion &rotation = Quaternion::Identity, const float3 &scale = float3::One);

    // reset to initial state
    void Reset();

    // activate/deactivate
    void Activate();
    void Deactivate();

    // Events
    virtual bool Initialize(uint32 entityID, const String &entityName) override;
    virtual void OnAddToWorld(World *pWorld) override;
    virtual void OnRemoveFromWorld(World *pWorld) override;
    virtual void OnTransformChange() override;
    virtual void OnComponentBoundsChange() override;
    virtual void Update(float timeSinceLastUpdate) override;

private:
    const ParticleSystem *m_pParticleSystem;
    bool m_autoStart;

    ParticleSystemRenderProxy *m_pRenderProxy;
    bool m_active;
};
