#pragma once
#include "Engine/Component.h"
#include "Engine/ParticleSystem.h"

class ParticleSystemRenderProxy;

class ParticleEmitterComponent : public Component
{
    DECLARE_COMPONENT_TYPEINFO(ParticleEmitterComponent, Component);
    DECLARE_COMPONENT_GENERIC_FACTORY(ParticleEmitterComponent);

public:
    ParticleEmitterComponent(const ComponentTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~ParticleEmitterComponent();

    // particle system
    const ParticleSystem *GetParticleSystem() const { return m_pParticleSystem; }
    void SetParticleSystem(const ParticleSystem *pParticleSystem);

    // create new instance
    bool Create(const ParticleSystem *pParticleSystem, const float3 &offsetPosition = float3::Zero, const Quaternion &offsetRotation = Quaternion::Identity);

    // reset to initial state
    void Reset();

    // activate/deactivate
    void Activate();
    void Deactivate();

    // Events
    virtual bool Initialize() override;
    virtual void OnAddToEntity(Entity *pEntity) override;
    virtual void OnRemoveFromEntity(Entity *pEntity) override;
    virtual void OnAddToWorld(World *pWorld) override;
    virtual void OnRemoveFromWorld(World *pWorld) override;
    virtual void OnLocalTransformChange() override;
    virtual void OnEntityTransformChange() override;
    virtual void Update(float timeSinceLastUpdate) override;

private:
    const ParticleSystem *m_pParticleSystem;
    bool m_autoStart;

    ParticleSystemRenderProxy *m_pRenderProxy;
    Transform m_cachedTransform;
    bool m_active;
};
