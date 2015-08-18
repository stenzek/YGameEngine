#pragma once
#include "Engine/Common.h"
#include "Engine/ParticleSystemCommon.h"
#include "Engine/ParticleSystemEmitter.h"
#include "Engine/ParticleSystemModule.h"

class ParticleSystemGenerator;

class ParticleSystem : public Resource
{
    DECLARE_RESOURCE_TYPE_INFO(ParticleSystem, Resource);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystem);

// public:
//     struct InstanceData
//     {
//         ParticleSystemEmitter::InstanceData *pEmitterData;
//         ParticleSystemEmitter::InstanceRenderData *pEmitterRenderData;
//         uint32 EmitterCount;
//     };

public:
    ParticleSystem(const ResourceTypeInfo *pTypeInfo = &s_TypeInfo);
    virtual ~ParticleSystem();

    // Emitter management
    const ParticleSystemEmitter *GetEmitter(uint32 emitterIndex) const { return m_emitters[emitterIndex]; }
    const uint32 GetEmitterCount() const { return m_emitters.GetSize(); }
    void AddEmitter(const ParticleSystemEmitter *pEmitter);
    void RemoveEmitter(const ParticleSystemEmitter *pEmitter);

    // Update interval
    const float GetUpdateInterval() const { return m_updateInterval; }

//     // Instance initialization
//     bool InitializeInstance(InstanceData *pInstanceData) const;
// 
//     // Instance cleanup
//     void CleanupInstance(InstanceData *pInstanceData) const;
// 
//     // Instance update
//     void UpdateInstance(InstanceData *pInstanceData, const Transform *pBaseTransform, float deltaTime) const;
// 
//     // Render-thread instance update
//     void UpdateInstanceRenderData(InstanceData *pInstanceData) const;

    // load from serialized
    bool LoadFromStream(const char *name, ByteStream *pStream);

private:
    PODArray<const ParticleSystemEmitter *> m_emitters;
    float m_updateInterval;
};

