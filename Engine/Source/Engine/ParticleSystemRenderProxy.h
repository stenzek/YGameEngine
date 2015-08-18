#pragma once
#include "Engine/ParticleSystem.h"
#include "Renderer/RenderProxy.h"

// ParticleSystemRenderProxy does not hold a reference on the instance data, it is assumed the
// holding class will maintain this reference, and keep a copy of the associated data.
class ParticleSystemRenderProxy : public RenderProxy
{
public:
    ParticleSystemRenderProxy(const ParticleSystem *pParticleSystem, uint32 entityID);
    virtual ~ParticleSystemRenderProxy();

    // get the current bounding box of the particle system (game thread)
    const AABox &GetParticleSystemBoundingBox() const { return m_pEmitterInstanceData->BoundingBox; }

    // particle system
    const ParticleSystem *GetParticleSystem() const { return m_pParticleSystem; }
    void SetParticleSystem(const ParticleSystem *pParticleSystem);

    // reset the particle system back to initial state
    void Reset();

    // update the particle system. call from the game thread.
    void Update(const Transform *pBaseTransform, float deltaTime);

    // Draw methods
    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override final;
    virtual void SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const override final;
    virtual void DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const override final;

private:
    // Create instance data
    void CreateInstanceData();

    // Cleanup instance data
    void DeleteInstanceData();

    const ParticleSystem *m_pParticleSystem;

    ParticleSystemEmitter::InstanceData *m_pEmitterInstanceData;
    ParticleSystemEmitter::InstanceRenderData *m_pEmitterInstanceRenderData;
    uint32 m_emitterCount;

    float m_timeSinceLastUpdate;
};

