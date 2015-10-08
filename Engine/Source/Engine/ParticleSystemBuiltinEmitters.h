#pragma once
#include "Engine/ParticleSystemEmitter.h"

class ParticleSystemEmitter_Sprite : public ParticleSystemEmitter
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemEmitter_Sprite, ParticleSystemEmitter);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemEmitter_Sprite);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemEmitter_Sprite);

public:
    ParticleSystemEmitter_Sprite();
    virtual ~ParticleSystemEmitter_Sprite();

    // Material to draw.
    const Material *GetMaterial() const { return m_pMaterial; }
    void SetMaterial(const Material *pMaterial);

    virtual void InitializeInstance(InstanceData *pEmitterData) const override;
    virtual void CleanupInstance(InstanceData *pEmitterData) const override;
    virtual void UpdateInstance(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, float deltaTime) const override;

    virtual void InitializeRenderData(InstanceRenderData *pEmitterRenderData) const override;
    virtual void CleanupRenderData(InstanceRenderData *pEmitterRenderData) const override;
    virtual void UpdateRenderData(const InstanceData *pEmitterData, InstanceRenderData *pEmitterRenderData) const override;

    virtual void QueueForRender(const RenderProxy *pRenderProxy, uint32 userData, const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, RenderQueue *pRenderQueue) const override;
    virtual void SetupForDraw(const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList, ShaderProgram *pShaderProgram) const override;
    virtual void DrawQueueEntry(const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList) const override;

private:
    // Material to render.
    const Material *m_pMaterial;

    // property callbacks
    static bool PropertyCallbackGetMaterial(ThisClass *pParticleSystem, const void *pUserData, String *pValue);
    static bool PropertyCallbackSetMaterial(ThisClass *pParticleSystem, const void *pUserData, const String *pValue);
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class StaticMesh;

class ParticleSystemEmitter_StaticMesh : public ParticleSystemEmitter
{
    DECLARE_OBJECT_TYPE_INFO(ParticleSystemEmitter_Sprite, ParticleSystemEmitter);
    DECLARE_OBJECT_GENERIC_FACTORY(ParticleSystemEmitter_Sprite);
    DECLARE_OBJECT_PROPERTY_MAP(ParticleSystemEmitter_Sprite);

public:
    ParticleSystemEmitter_StaticMesh();
    virtual ~ParticleSystemEmitter_StaticMesh();

    // Static mesh to instance.
    const StaticMesh *GetMesh(uint32 index) const { return m_meshes[index]; }
    void SetMesh(uint32 index, const StaticMesh *pMesh);
    void AddMesh(const StaticMesh *pMesh);
    void RemoveMesh(uint32 index);

    virtual void InitializeInstance(InstanceData *pEmitterData) const override;
    virtual void CleanupInstance(InstanceData *pEmitterData) const override;
    virtual void UpdateInstance(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, float deltaTime) const override;

    virtual void InitializeRenderData(InstanceRenderData *pEmitterRenderData) const override;
    virtual void CleanupRenderData(InstanceRenderData *pEmitterRenderData) const override;
    virtual void UpdateRenderData(const InstanceData *pEmitterData, InstanceRenderData *pEmitterRenderData) const override;

    virtual void QueueForRender(const RenderProxy *pRenderProxy, uint32 userData, const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, RenderQueue *pRenderQueue) const override;
    virtual void SetupForDraw(const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList, ShaderProgram *pShaderProgram) const override;
    virtual void DrawQueueEntry(const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList) const override;

private:
    // Meshes to render.
    PODArray<const StaticMesh *> m_meshes;
};

