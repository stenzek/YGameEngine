#include "Engine/PrecompiledHeader.h"
#include "Engine/ParticleSystemRenderProxy.h"
#include "Engine/Engine.h"
#include "Renderer/Renderer.h"

ParticleSystemRenderProxy::ParticleSystemRenderProxy(const ParticleSystem *pParticleSystem, uint32 entityID)
    : RenderProxy(entityID),
      m_pParticleSystem(pParticleSystem),
      m_pEmitterInstanceData(nullptr),
      m_pEmitterInstanceRenderData(nullptr),
      m_emitterCount(0),
      m_timeSinceLastUpdate(0.0f)
{
    m_pParticleSystem->AddRef();

    // create instance data
    CreateInstanceData();
}

ParticleSystemRenderProxy::~ParticleSystemRenderProxy()
{
    // cleanup instance data
    DeleteInstanceData();

    m_pParticleSystem->Release();
}

void ParticleSystemRenderProxy::CreateInstanceData()
{
    DebugAssert(m_pParticleSystem->GetEmitterCount() > 0);
    m_emitterCount = m_pParticleSystem->GetEmitterCount();
    m_pEmitterInstanceData = new ParticleSystemEmitter::InstanceData[m_emitterCount];
    m_pEmitterInstanceRenderData = new ParticleSystemEmitter::InstanceRenderData[m_emitterCount];
    for (uint32 emitterIndex = 0; emitterIndex < m_emitterCount; emitterIndex++)
    {
        const ParticleSystemEmitter *pEmitter = m_pParticleSystem->GetEmitter(emitterIndex);
        pEmitter->InitializeInstance(&m_pEmitterInstanceData[emitterIndex]);
        pEmitter->InitializeRenderData(&m_pEmitterInstanceRenderData[emitterIndex]);
    }
}

void ParticleSystemRenderProxy::DeleteInstanceData()
{
    for (uint32 emitterIndex = 0; emitterIndex < m_emitterCount; emitterIndex++)
    {
        const ParticleSystemEmitter *pEmitter = m_pParticleSystem->GetEmitter(emitterIndex);
        pEmitter->CleanupRenderData(&m_pEmitterInstanceRenderData[emitterIndex]);
        pEmitter->CleanupInstance(&m_pEmitterInstanceData[emitterIndex]);
    }
    delete[] m_pEmitterInstanceRenderData;
    m_pEmitterInstanceRenderData = nullptr;
    delete[] m_pEmitterInstanceData;
    m_pEmitterInstanceData = nullptr;
    m_emitterCount = 0;
}


void ParticleSystemRenderProxy::SetParticleSystem(const ParticleSystem *pParticleSystem)
{
    DebugAssert(m_pParticleSystem != nullptr && pParticleSystem != nullptr);

    DeleteInstanceData();

    if (m_pParticleSystem != pParticleSystem)
    {
        m_pParticleSystem->Release();
        m_pParticleSystem = pParticleSystem;
        m_pParticleSystem->AddRef();
    }

    CreateInstanceData();
}

void ParticleSystemRenderProxy::Reset()
{
    // fixme properly
    DeleteInstanceData();
    CreateInstanceData();
}

void ParticleSystemRenderProxy::Update(const Transform *pBaseTransform, float deltaTime)
{
    m_timeSinceLastUpdate += deltaTime;

    if (m_timeSinceLastUpdate >= m_pParticleSystem->GetUpdateInterval())
    {
        // simulate each emitter
        for (uint32 emitterIndex = 0; emitterIndex < m_emitterCount; emitterIndex++)
            m_pParticleSystem->GetEmitter(emitterIndex)->UpdateInstance(&m_pEmitterInstanceData[emitterIndex], pBaseTransform, g_pEngine->GetRandomNumberGenerator(), m_timeSinceLastUpdate);

        // queue update on render thread
        ReferenceCountedHolder<ParticleSystemRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis]()
        {
            // update render thread copy
            AABox systemBoundingBox;
            for (uint32 emitterIndex = 0; emitterIndex < pThis->m_emitterCount; emitterIndex++)
            {
                pThis->m_pParticleSystem->GetEmitter(emitterIndex)->UpdateRenderData(&pThis->m_pEmitterInstanceData[emitterIndex], &pThis->m_pEmitterInstanceRenderData[emitterIndex]);
                if (emitterIndex == 0)
                    systemBoundingBox.SetBounds(pThis->m_pEmitterInstanceRenderData[emitterIndex].BoundingBox);
                else
                    systemBoundingBox.Merge(pThis->m_pEmitterInstanceRenderData[emitterIndex].BoundingBox);
            }

            // update bounding box
            if (systemBoundingBox != pThis->GetBoundingBox())
                pThis->SetBounds(systemBoundingBox, Sphere::FromAABox(systemBoundingBox));
        });

        // schedule next update
        m_timeSinceLastUpdate = 0.0f;
    }
}

void ParticleSystemRenderProxy::QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    // Use the emitter slot index as the user data
    for (uint32 emitterIndex = 0; emitterIndex < m_pParticleSystem->GetEmitterCount(); emitterIndex++)
        m_pParticleSystem->GetEmitter(emitterIndex)->QueueForRender(this, emitterIndex, &m_pEmitterInstanceRenderData[emitterIndex], pCamera, pRenderQueue);
}

void ParticleSystemRenderProxy::SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const
{
    uint32 emitterIndex = pQueueEntry->UserData[0];
    DebugAssert(emitterIndex < m_emitterCount);

    const ParticleSystemEmitter *pEmitter = m_pParticleSystem->GetEmitter(pQueueEntry->UserData[0]);
    pEmitter->SetupForDraw(&m_pEmitterInstanceRenderData[emitterIndex], pCamera, pQueueEntry, pGPUContext, pShaderProgram);
}

void ParticleSystemRenderProxy::DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const
{
    uint32 emitterIndex = pQueueEntry->UserData[0];
    DebugAssert(emitterIndex < m_emitterCount);

    const ParticleSystemEmitter *pEmitter = m_pParticleSystem->GetEmitter(pQueueEntry->UserData[0]);
    pEmitter->DrawQueueEntry(&m_pEmitterInstanceRenderData[emitterIndex], pCamera, pQueueEntry, pGPUContext);
}
