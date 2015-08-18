#include "Engine/PrecompiledHeader.h"
#include "Engine/ParticleSystemEmitter.h"
#include "Engine/ParticleSystemModule.h"
#include "Renderer/Renderer.h"
Log_SetChannel(ParticleSystemEmitter);

DEFINE_OBJECT_TYPE_INFO(ParticleSystemEmitter);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemEmitter)
    PROPERTY_TABLE_MEMBER_UINT("MaxActiveParticles", 0, offsetof(ParticleSystemEmitter, m_maxActiveParticles), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_UINT("UpdateInterval", 0, offsetof(ParticleSystemEmitter, m_updateInterval), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_UINT("SpawnRate", 0, offsetof(ParticleSystemEmitter, m_spawnRate), nullptr, nullptr)
    PROPERTY_TABLE_MEMBER_UINT("SpawnCount", 0, offsetof(ParticleSystemEmitter, m_spawnCount), nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemEmitter::ParticleSystemEmitter(const ObjectTypeInfo *pTypeInfo /* = &s_typeInfo */)
    : BaseClass(pTypeInfo)
{
    m_maxActiveParticles = 16;
    m_updateInterval = 1.0f / 60.0f;
    m_spawnRate = 1.0f;
    m_spawnCount = 1;
}

ParticleSystemEmitter::~ParticleSystemEmitter()
{
    for (uint32 i = 0; i < m_modules.GetSize(); i++)
        delete m_modules[i];
}

void ParticleSystemEmitter::AddModule(const ParticleSystemModule *pAffector)
{
    DebugAssert(m_modules.IndexOf(pAffector) < 0);
    m_modules.Add(pAffector);
}

void ParticleSystemEmitter::RemoveModule(const ParticleSystemModule *pAffector)
{
    int32 index = m_modules.IndexOf(pAffector);
    if (index >= 0)
        m_modules.OrderedRemove(index);
}

void ParticleSystemEmitter::InitializeInstance(InstanceData *pEmitterData) const
{
    // Allocate particle arrays
    pEmitterData->ParticlesState[0].Reserve(m_maxActiveParticles);
    pEmitterData->ParticlesState[1].Reserve(m_maxActiveParticles);
    pEmitterData->BoundingBox.SetZero();
    pEmitterData->TimeUntilNextSpawn = 0.0f;
}

void ParticleSystemEmitter::CleanupInstance(InstanceData *pEmitterData) const
{
    // Cleanup everything
    pEmitterData->ParticlesState[0].Obliterate();
    pEmitterData->ParticlesState[1].Obliterate();
}

void ParticleSystemEmitter::UpdateInstance(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, float deltaTime) const
{
    // Call affector update on everything in array[1]
    if (pEmitterData->ParticlesState[0].GetSize() > 0)
    {
        for (uint32 affectorIndex = 0; affectorIndex < m_modules.GetSize(); affectorIndex++)
        {
            const ParticleSystemModule *pAffector = m_modules[affectorIndex];
            pAffector->UpdateParticles(this, pBaseTransform, pRNG, pEmitterData->ParticlesState[0].GetBasePointer(), pEmitterData->ParticlesState[0].GetSize(), deltaTime);
        }
    }
    
    // Clear array[2], Update position, lifetime on existing particles, store in array[2]
    // Calculate new bounding box along the way
    AABox particlesBoundingBox;
    uint32 nActiveParticles = 0;
    pEmitterData->ParticlesState[1].Clear();
    for (uint32 particleIndex = 0; particleIndex < pEmitterData->ParticlesState[0].GetSize(); particleIndex++)
    {
        ParticleData *pParticleData = &pEmitterData->ParticlesState[0][particleIndex];
        pParticleData->LifeRemaining -= deltaTime;
        if (pParticleData->LifeRemaining <= 0.0f)
            continue;

        pParticleData->Position += pParticleData->Velocity * deltaTime;
        
        float maxDimension = Max(pParticleData->Width, pParticleData->Height);
        if ((nActiveParticles++) == 0)
            particlesBoundingBox.SetBounds(pParticleData->Position - maxDimension, pParticleData->Position + maxDimension);
        else
            particlesBoundingBox.Merge(AABox(pParticleData->Position - maxDimension, pParticleData->Position + maxDimension));

        pEmitterData->ParticlesState[1].Add(*pParticleData);
    }

    // Spawn new particles, add to array[2]
    float remainingDeltaTime = deltaTime;
    while (remainingDeltaTime > 0.0f)
    {
        // spawn one?
        if (remainingDeltaTime >= pEmitterData->TimeUntilNextSpawn)
        {
            remainingDeltaTime -= pEmitterData->TimeUntilNextSpawn;

            // room for another particle?
            if (pEmitterData->ParticlesState[1].GetSize() < m_maxActiveParticles)
            {
                // spawn particles
                for (uint32 spawnCount = 0; spawnCount < m_spawnCount && pEmitterData->ParticlesState[1].GetSize() < m_maxActiveParticles; spawnCount++)
                {
                    ParticleData particleData;
                    if (InternalCreateParticle(pEmitterData, pBaseTransform, pRNG, &particleData))
                    {
                        pEmitterData->ParticlesState[1].Add(particleData);

                        // update bounding box
                        float maxDimension = Max(particleData.Width, particleData.Height);
                        if ((nActiveParticles++) == 0)
                            particlesBoundingBox.SetBounds(particleData.Position - maxDimension, particleData.Position + maxDimension);
                        else
                            particlesBoundingBox.Merge(AABox(particleData.Position - maxDimension, particleData.Position + maxDimension));
                    }
                }

                // reset time remaining
                pEmitterData->TimeUntilNextSpawn = m_spawnRate;
            }
            else
            {
                // keep the time remaining to zero so we spawn a particle as soon as possible
                pEmitterData->TimeUntilNextSpawn = 0.0f;
                break;
            }
        }
        else
        {
            pEmitterData->TimeUntilNextSpawn -= remainingDeltaTime;
            break;
        }        
    }

    // Swap array[2] with array[1]
    pEmitterData->ParticlesState[0].Swap(pEmitterData->ParticlesState[1]);

    // Update bounding box
    if (nActiveParticles > 0)
        pEmitterData->BoundingBox = particlesBoundingBox;

    //Log_DevPrintf("num particles %u", nActiveParticles);
}

bool ParticleSystemEmitter::InternalCreateParticle(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, ParticleData *pParticleData) const
{
    pParticleData->Position = pBaseTransform->TransformPoint(float3::Zero);
    pParticleData->Velocity.SetZero();
    pParticleData->Width = 1.0f;
    pParticleData->Height = 1.0f;
    pParticleData->Rotation = 0.0f;
    pParticleData->Color = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255);
    pParticleData->MinTextureCoordinates = float2(0.0f, 0.0f);
    pParticleData->MaxTextureCoordinates = float2(1.0f, 1.0f);
    pParticleData->LifeSpan = Y_FLT_MAX;
    pParticleData->LifeRemaining = Y_FLT_MAX;

    // invoke modules on each particle
    for (uint32 moduleIndex = 0; moduleIndex < m_modules.GetSize(); moduleIndex++)
    {
        if (!m_modules[moduleIndex]->CreateParticle(this, pBaseTransform, pRNG, pParticleData))
            return false;
    }

    // okay to spawn
    return true;
}

ParticleData *ParticleSystemEmitter::InternalSpawnParticle(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG) const
{
    if (pEmitterData->ParticlesState[0].GetSize() > m_maxActiveParticles)
        return nullptr;

    ParticleData particleData;
    if (!InternalCreateParticle(pEmitterData, pBaseTransform, pRNG, &particleData))
        return nullptr;

    // update bounding box
    float maxDimension = Max(particleData.Width, particleData.Height);
    if (pEmitterData->ParticlesState[0].IsEmpty())
        pEmitterData->BoundingBox.SetBounds(particleData.Position - maxDimension, particleData.Position + maxDimension);
    else
        pEmitterData->BoundingBox.Merge(AABox(particleData.Position - maxDimension, particleData.Position + maxDimension));

    // add particle
    pEmitterData->ParticlesState[0].Add(particleData);
    return &pEmitterData->ParticlesState[0].LastElement();
}

bool ParticleSystemEmitter::SpawnParticle(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG) const
{
    ParticleData *pParticleData = InternalSpawnParticle(pEmitterData, pBaseTransform, pRNG);
    if (pEmitterData == nullptr)
        return false;

    return true;
}

bool ParticleSystemEmitter::SpawnParticle(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, uint32 emitterSpecificData) const
{
    ParticleData *pParticleData = InternalSpawnParticle(pEmitterData, pBaseTransform, pRNG);
    if (pEmitterData == nullptr)
        return false;

    return true;
}

bool ParticleSystemEmitter::SpawnParticle(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, uint32 emitterSpecificData, const float3 &localPosition, const float3 &spawnDirection) const
{
    ParticleData *pParticleData = InternalSpawnParticle(pEmitterData, pBaseTransform, pRNG);
    if (pEmitterData == nullptr)
        return false;

    pParticleData->Position = pBaseTransform->TransformPoint(localPosition);
    pParticleData->Velocity = spawnDirection;
    return true;
}

void ParticleSystemEmitter::InitializeRenderData(InstanceRenderData *pEmitterRenderData) const
{
    // Start with no GPU staging area, the emitter types can fill these in
    Y_memzero(pEmitterRenderData, sizeof(InstanceRenderData));
}

void ParticleSystemEmitter::CleanupRenderData(InstanceRenderData *pEmitterRenderData) const
{
    Y_free(pEmitterRenderData->pGPUStagingBuffer);
    pEmitterRenderData->pGPUStagingBuffer = nullptr;
    pEmitterRenderData->GPUStagingBufferSize = 0;
    pEmitterRenderData->GPUStagingBufferUsage = 0;
    if (pEmitterRenderData->pGPUBuffer != nullptr)
    {
        pEmitterRenderData->pGPUBuffer->Release();
        pEmitterRenderData->pGPUBuffer = nullptr;
    }
}

void ParticleSystemEmitter::UpdateRenderData(const InstanceData *pEmitterData, InstanceRenderData *pEmitterRenderData) const
{
    // Update the render thread's copy of the bounding box.
    pEmitterRenderData->BoundingBox = pEmitterData->BoundingBox;
}

void ParticleSystemEmitter::QueueForRender(const RenderProxy *pRenderProxy, uint32 userData, const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, RenderQueue *pRenderQueue) const
{

}

void ParticleSystemEmitter::SetupForDraw(const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const
{

}

void ParticleSystemEmitter::DrawQueueEntry(const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const
{

}
