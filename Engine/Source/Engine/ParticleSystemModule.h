#pragma once
#include "Engine/ParticleSystemCommon.h"

class RandomNumberGenerator;

// Base affector/module type
class ParticleSystemModule : public Object
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemModule, Object);
    DECLARE_OBJECT_NO_FACTORY(ParticleSystemModule);
    DECLARE_OBJECT_NO_PROPERTIES(ParticleSystemModule);

public:
    ParticleSystemModule(const ObjectTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~ParticleSystemModule();

    // Set properties on a new particle
    virtual bool CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const;

    // Update a list of particles
    virtual void UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const;

    // Type registration
    static void RegisterBuiltinModules();
};
