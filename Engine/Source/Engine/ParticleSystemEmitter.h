#pragma once
#include "Engine/ParticleSystemCommon.h"
#include "Renderer/RenderProxy.h"

class GPUBuffer;
class RandomNumberGenerator;

// Base emitter type
class ParticleSystemEmitter : public Object
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemEmitter, Object);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemEmitter);
    DECLARE_OBJECT_NO_FACTORY(ParticleSystemEmitter);
    friend class ParticleSystem;

public:
    // Emitter starting location types
    enum SpawnLocationType
    {
        SpawnLocationType_Point,
        SpawnLocationType_Box,
        SpawnLocationType_Sphere,
        SpawnLocationType_Cylinder,
        SpawnLocationType_Triangle,
        SpawnLocationType_Count
    };

    // Emitter starting velocity types
    enum SpawnVelocityType
    {
        SpawnVelocityType_Fixed,
        SpawnVelocityType_Arc,
        SpawnVelocityType_Random,
        SpawnVelocityType_RandomFixedAxis,
        SpawnVelocityType_Count
    };

    // Particle type definitions
    typedef MemArray<ParticleData> ParticleDataArray;

    // Emitter data, created once per emitter.
    struct InstanceData
    {
        // There are two particle arrays, one containing the emitter state at the
        // start of the frame, and another after the emitter update has completed.
        ParticleDataArray ParticlesState[2];

        // Time remaining until next particle is spawned.
        float TimeUntilNextSpawn;

        // Bounding box of the emitter.
        AABox BoundingBox;
    };

    // Emitter render data, created once per emitter, stored in the Render Proxy
    struct InstanceRenderData
    {
        // Copy of the bounding box of the emitter.
        AABox BoundingBox;

        // Copy of the number of particles.
        uint32 ParticleCount;

        // Vertex factory flags.
        uint32 VertexFactoryFlags;

        // Emitter data.
        uint32 EmitterData[4];

        // Buffer containing the data that is uploaded to the GPU, if needed.
        // Guaranteed to be valid and race-free once queued until the frame is complete.
        byte *pGPUStagingBuffer;
        uint32 GPUStagingBufferSize;
        uint32 GPUStagingBufferUsage;

        // Buffer space on GPU.
        GPUBuffer *pGPUBuffer;
        uint32 GPUBufferUsage;
    };

public:
    ParticleSystemEmitter(const ObjectTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~ParticleSystemEmitter();

    // Maximum active particles
    const uint32 GetMaxActiveParticles() const { return m_maxActiveParticles; }
    void SetMaxActiveParticles(uint32 maxActiveParticles) { m_maxActiveParticles = maxActiveParticles; }

    // Update interval in milliseconds
    const float GetUpdateInterval() const { return m_updateInterval; }
    void SetUpdateInterval(float updateInterval) { m_updateInterval = updateInterval; }

    // Spawn rate, number of particles per second
    const float GetSpawnRate() const { return m_spawnRate; }
    void SetSpawnRate(float particlesPerSecond) { m_spawnRate = particlesPerSecond; }

    // Spawn rate, i.e. number of particles to spawn in each spawn
    const uint32 GetSpawnCount() const { return m_spawnCount; }
    void SetSpawnCount(uint32 particlesPerSpawn) { m_spawnCount = particlesPerSpawn; }

    // Affector array access
    const ParticleSystemModule *GetModule(uint32 moduleIndex) const { return m_modules[moduleIndex]; }
    uint32 GetModuleCount() const { return m_modules.GetSize(); }
    void AddModule(const ParticleSystemModule *pModule);
    void RemoveModule(const ParticleSystemModule *pModule);

    // Manually spawn a particle in an instance.
    virtual bool SpawnParticle(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG) const;
    virtual bool SpawnParticle(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, uint32 emitterSpecificData) const;
    virtual bool SpawnParticle(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, uint32 emitterSpecificData, const float3 &localPosition, const float3 &spawnDirection) const;

    // Initialize emitter state.
    virtual void InitializeInstance(InstanceData *pEmitterData) const;

    // Cleanup emitter state.
    virtual void CleanupInstance(InstanceData *pEmitterData) const;

    // Update the emitter state.
    virtual void UpdateInstance(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, float deltaTime) const;

    // Update the render data associated with the emitter state. Call from render thread.
    virtual void InitializeRenderData(InstanceRenderData *pEmitterRenderData) const;
    virtual void CleanupRenderData(InstanceRenderData *pEmitterRenderData) const;
    virtual void UpdateRenderData(const InstanceData *pEmitterData, InstanceRenderData *pEmitterRenderData) const;

    // Draw methods. UserData[0] will always be populated with what is provided here.
    virtual void QueueForRender(const RenderProxy *pRenderProxy, uint32 userData, const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, RenderQueue *pRenderQueue) const;
    virtual void SetupForDraw(const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const;
    virtual void DrawQueueEntry(const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const;

    // Type registration
    static void RegisterBuiltinEmitters();

protected:
    // Internally initialize a new particle.
    virtual bool InternalCreateParticle(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticleData) const;
    ParticleData *InternalSpawnParticle(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG) const;

    // Maximum number of particles this emitter can accommodate at once.
    // Used to determine buffer and array sizes.
    uint32 m_maxActiveParticles;

    // Update interval in milliseconds. To update every frame, set to 0.
    float m_updateInterval;

    // Spawn rate
    float m_spawnRate;

    // Spawn count
    uint32 m_spawnCount;

    // Array of affectors
    PODArray<const ParticleSystemModule *> m_modules;
};

