#include "Engine/PrecompiledHeader.h"
#include "Engine/ParticleSystemModule.h"
#include "Engine/ParticleSystemEmitter.h"

DEFINE_OBJECT_TYPE_INFO(ParticleSystemModule);

ParticleSystemModule::ParticleSystemModule(const ObjectTypeInfo *pTypeInfo /* = &s_typeInfo */)
    : BaseClass(pTypeInfo)
{

}

ParticleSystemModule::~ParticleSystemModule()
{

}

bool ParticleSystemModule::CreateParticle(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticle) const
{
    return true;
}

void ParticleSystemModule::UpdateParticles(const ParticleSystemEmitter *pEmitter, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticles, uint32 nParticles, float deltaTime) const
{

}

