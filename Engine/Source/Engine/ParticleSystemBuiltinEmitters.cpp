#include "Engine/PrecompiledHeader.h"
#include "Engine/ParticleSystemBuiltinEmitters.h"
#include "Engine/ParticleSystemVertexFactory.h"
#include "Engine/Camera.h"
#include "Engine/Material.h"
#include "Engine/EngineCVars.h"
#include "Engine/ResourceManager.h"
#include "Renderer/Renderer.h"
Log_SetChannel(ParticleSystemBuiltinEmitters);

DEFINE_OBJECT_TYPE_INFO(ParticleSystemEmitter_Sprite);
DEFINE_OBJECT_GENERIC_FACTORY(ParticleSystemEmitter_Sprite);
BEGIN_OBJECT_PROPERTY_MAP(ParticleSystemEmitter_Sprite)
    PROPERTY_TABLE_MEMBER("Material", PROPERTY_TYPE_STRING, 0, PropertyCallbackGetMaterial, nullptr, PropertyCallbackSetMaterial, nullptr, nullptr, nullptr)
END_OBJECT_PROPERTY_MAP()

ParticleSystemEmitter_Sprite::ParticleSystemEmitter_Sprite()
    : ParticleSystemEmitter()
{
    m_pMaterial = g_pResourceManager->GetDefaultMaterial();
}

ParticleSystemEmitter_Sprite::~ParticleSystemEmitter_Sprite()
{
    m_pMaterial->Release();
}

void ParticleSystemEmitter_Sprite::SetMaterial(const Material *pMaterial)
{
    if (m_pMaterial == pMaterial)
        return;

    m_pMaterial->Release();
    m_pMaterial = pMaterial;
    m_pMaterial->AddRef();
}

void ParticleSystemEmitter_Sprite::InitializeInstance(InstanceData *pEmitterData) const
{
    ParticleSystemEmitter::InitializeInstance(pEmitterData);    
}

void ParticleSystemEmitter_Sprite::CleanupInstance(InstanceData *pEmitterData) const
{
    ParticleSystemEmitter::CleanupInstance(pEmitterData);
}

void ParticleSystemEmitter_Sprite::UpdateInstance(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, float deltaTime) const
{
    // Do normal system update
    ParticleSystemEmitter::UpdateInstance(pEmitterData, pBaseTransform, pRNG, deltaTime);
}

void ParticleSystemEmitter_Sprite::QueueForRender(const RenderProxy *pRenderProxy, uint32 userData, const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    // Check particle count, don't render anything if there aren't any.
    if (pEmitterRenderData->ParticleCount > 0)
    {
        // Determine render pass mask, remove shadowing passes
        uint32 renderPassMask = RENDER_PASSES_DEFAULT & ~(RENDER_PASS_SHADOW_MAP | RENDER_PASS_SHADOWED_LIGHTING);
        renderPassMask = m_pMaterial->GetShader()->SelectRenderPassMask(renderPassMask & pRenderQueue->GetAcceptingRenderPassMask());
        if (renderPassMask == 0)
            return;

        // Create a single queue entry for this material
        RENDER_QUEUE_RENDERABLE_ENTRY queueEntry;
        queueEntry.pRenderProxy = pRenderProxy;
        queueEntry.pVertexFactoryTypeInfo = VERTEX_FACTORY_TYPE_INFO(ParticleSystemSpriteVertexFactory);
        queueEntry.VertexFactoryFlags = pEmitterRenderData->VertexFactoryFlags;
        queueEntry.pMaterial = m_pMaterial;
        queueEntry.BoundingBox = pEmitterRenderData->BoundingBox;
        queueEntry.RenderPassMask = renderPassMask;
        queueEntry.ViewDistance = pCamera->CalculateDepthToBox(pEmitterRenderData->BoundingBox);
        queueEntry.Layer = m_pMaterial->GetShader()->SelectRenderQueueLayer();
        pRenderQueue->AddRenderable(&queueEntry);
    }
}

void ParticleSystemEmitter_Sprite::SetupForDraw(const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const
{
    pGPUContext->SetDrawTopology(ParticleSystemSpriteVertexFactory::GetDrawTopology(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), pEmitterRenderData->VertexFactoryFlags));
    pGPUContext->GetConstants()->SetLocalToWorldMatrix(float4x4::Identity, true);

    // set up vertex buffer for instanced quads
    if (pEmitterRenderData->VertexFactoryFlags & ParticleSystemSpriteVertexFactory::Flag_RenderInstancedQuads)
    {
        uint32 vertexBufferOffset = 0;
        uint32 vertexBufferStride = ParticleSystemSpriteVertexFactory::GetVertexSize(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), pEmitterRenderData->VertexFactoryFlags);
        pGPUContext->SetVertexBuffers(0, 1, &pEmitterRenderData->pGPUBuffer, &vertexBufferOffset, &vertexBufferStride);
    }
}

void ParticleSystemEmitter_Sprite::DrawQueueEntry(const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const
{
    // branch out
    if (pEmitterRenderData->VertexFactoryFlags & ParticleSystemSpriteVertexFactory::Flag_RenderBasic)
    {
        uint32 nVertices = pEmitterRenderData->ParticleCount * ParticleSystemSpriteVertexFactory::GetVerticesPerSprite(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), pEmitterRenderData->VertexFactoryFlags);
        uint32 vertexSize = ParticleSystemSpriteVertexFactory::GetVertexSize(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), pEmitterRenderData->VertexFactoryFlags);
        uint32 bufferSize = nVertices * vertexSize;
        DebugAssert(bufferSize > 0);

        // ehh whatever fix me later please
        byte *pBuffer = new byte[bufferSize];
        ParticleSystemSpriteVertexFactory::FillVertexBuffer(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), pEmitterRenderData->VertexFactoryFlags, pCamera, reinterpret_cast<const ParticleData *>(pEmitterRenderData->pGPUStagingBuffer), pEmitterRenderData->ParticleCount, pBuffer, bufferSize);
        pGPUContext->DrawUserPointer(pBuffer, vertexSize, nVertices);
        delete[] pBuffer;
    }
    else if (pEmitterRenderData->VertexFactoryFlags & ParticleSystemSpriteVertexFactory::Flag_RenderInstancedQuads)
    {
        // invoke instanced draw
        DebugAssert(pEmitterRenderData->ParticleCount > 0);
        pGPUContext->DrawInstanced(0, 4, pEmitterRenderData->ParticleCount);
    }
}

void ParticleSystemEmitter_Sprite::InitializeRenderData(InstanceRenderData *pEmitterRenderData) const
{
    ParticleSystemEmitter::InitializeRenderData(pEmitterRenderData);

    // determine vertex factory flags
    uint32 vertexFactoryFlags = 0; // ParticleSystemSpriteVertexFactory::Flag_UseWorldTransform
    if (CVars::r_sprite_draw_instanced_quads.GetBool())
        vertexFactoryFlags |= ParticleSystemSpriteVertexFactory::Flag_RenderInstancedQuads;
    else
        vertexFactoryFlags |= ParticleSystemSpriteVertexFactory::Flag_RenderBasic;

    // set flags
    pEmitterRenderData->VertexFactoryFlags = vertexFactoryFlags;

    // initialize stuff based on flags
    if (vertexFactoryFlags & ParticleSystemSpriteVertexFactory::Flag_RenderBasic)
    {
        // create 'staging' buffer
        pEmitterRenderData->GPUStagingBufferSize = sizeof(ParticleData) * m_maxActiveParticles;
        pEmitterRenderData->pGPUStagingBuffer = reinterpret_cast<byte *>(Y_malloc(pEmitterRenderData->GPUStagingBufferSize));

    }
    else if (vertexFactoryFlags & ParticleSystemSpriteVertexFactory::Flag_RenderInstancedQuads)
    {
        // Create dynamic buffer for streaming
        uint32 vertexSize = ParticleSystemSpriteVertexFactory::GetVertexSize(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), pEmitterRenderData->VertexFactoryFlags);
        GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER | GPU_BUFFER_FLAG_MAPPABLE, vertexSize * m_maxActiveParticles);
        pEmitterRenderData->pGPUBuffer = g_pRenderer->CreateBuffer(&bufferDesc, nullptr);
    }
}

void ParticleSystemEmitter_Sprite::CleanupRenderData(InstanceRenderData *pEmitterRenderData) const
{
    ParticleSystemEmitter::CleanupRenderData(pEmitterRenderData);
}

void ParticleSystemEmitter_Sprite::UpdateRenderData(const InstanceData *pEmitterData, InstanceRenderData *pEmitterRenderData) const
{
    // parent update
    ParticleSystemEmitter::UpdateRenderData(pEmitterData, pEmitterRenderData);

    // count active particles
    uint32 nActiveParticles = pEmitterData->ParticlesState[0].GetSize();
    pEmitterRenderData->ParticleCount = nActiveParticles;
    if (nActiveParticles == 0)
    {
        pEmitterRenderData->ParticleCount = 0;
        return;
    }

    // render type specific
    if (pEmitterRenderData->VertexFactoryFlags & ParticleSystemSpriteVertexFactory::Flag_RenderBasic)
    {
        // Write to GPU staging buffer
        DebugAssert(pEmitterRenderData->GPUStagingBufferSize >= (sizeof(ParticleData) * nActiveParticles));
        Y_memcpy(pEmitterRenderData->pGPUStagingBuffer, pEmitterData->ParticlesState[0].GetBasePointer(), sizeof(ParticleData) * nActiveParticles);
        pEmitterRenderData->GPUStagingBufferUsage = sizeof(ParticleData) * nActiveParticles;
    }
    else if (pEmitterRenderData->VertexFactoryFlags & ParticleSystemSpriteVertexFactory::Flag_RenderInstancedQuads)
    {
        GPUContext *pGPUContext = g_pRenderer->GetGPUContext();
        DebugAssert(pEmitterRenderData->pGPUBuffer != nullptr);

        // map the gpu buffer
        void *pMappedPointer;
        if (!pGPUContext->MapBuffer(pEmitterRenderData->pGPUBuffer, GPU_MAP_TYPE_WRITE_DISCARD, &pMappedPointer))
        {
            Log_ErrorPrintf("ParticleSystemEmitter_Sprite::UpdateRenderData: Failed to map GPU buffer");
            pEmitterRenderData->ParticleCount = 0;
            return;
        }

        // write to it
        if (!ParticleSystemSpriteVertexFactory::FillVertexBuffer(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), 
                                                                 pEmitterRenderData->VertexFactoryFlags, nullptr, 
                                                                 pEmitterData->ParticlesState[0].GetBasePointer(), nActiveParticles,
                                                                 pMappedPointer, pEmitterRenderData->pGPUBuffer->GetDesc()->Size))
        {
            Log_ErrorPrintf("ParticleSystemEmitter_Sprite::UpdateRenderData: Failed to write to GPU buffer");
            pEmitterRenderData->ParticleCount = 0;
            return;
        }

        // unmap buffer again
        pGPUContext->Unmapbuffer(pEmitterRenderData->pGPUBuffer, pMappedPointer);        
    }
}

bool ParticleSystemEmitter_Sprite::PropertyCallbackGetMaterial(ThisClass *pParticleSystem, const void *pUserData, String *pValue)
{
    *pValue = pParticleSystem->m_pMaterial->GetName();
    return true;
}

bool ParticleSystemEmitter_Sprite::PropertyCallbackSetMaterial(ThisClass *pParticleSystem, const void *pUserData, const String *pValue)
{
    const Material *pMaterial = g_pResourceManager->GetMaterial(pValue->GetCharArray());
    if (pMaterial == nullptr)
        return false;

    pParticleSystem->m_pMaterial->Release();
    pParticleSystem->m_pMaterial = pMaterial;
    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ParticleSystemEmitter_StaticMesh::ParticleSystemEmitter_StaticMesh()
{

}

ParticleSystemEmitter_StaticMesh::~ParticleSystemEmitter_StaticMesh()
{

}

void ParticleSystemEmitter_StaticMesh::SetMesh(uint32 index, const StaticMesh *pMesh)
{

}

void ParticleSystemEmitter_StaticMesh::AddMesh(const StaticMesh *pMesh)
{

}

void ParticleSystemEmitter_StaticMesh::RemoveMesh(uint32 index)
{

}

void ParticleSystemEmitter_StaticMesh::InitializeInstance(InstanceData *pEmitterData) const
{

}

void ParticleSystemEmitter_StaticMesh::CleanupInstance(InstanceData *pEmitterData) const
{

}

void ParticleSystemEmitter_StaticMesh::UpdateInstance(InstanceData *pEmitterData, const Transform *pBaseTransform, RandomNumberGenerator *pRNG, float deltaTime) const
{

}

void ParticleSystemEmitter_StaticMesh::InitializeRenderData(InstanceRenderData *pEmitterRenderData) const
{

}

void ParticleSystemEmitter_StaticMesh::CleanupRenderData(InstanceRenderData *pEmitterRenderData) const
{

}

void ParticleSystemEmitter_StaticMesh::UpdateRenderData(const InstanceData *pEmitterData, InstanceRenderData *pEmitterRenderData) const
{

}

void ParticleSystemEmitter_StaticMesh::QueueForRender(const RenderProxy *pRenderProxy, uint32 userData, const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, RenderQueue *pRenderQueue) const
{

}

void ParticleSystemEmitter_StaticMesh::SetupForDraw(const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const
{

}

void ParticleSystemEmitter_StaticMesh::DrawQueueEntry(const InstanceRenderData *pEmitterRenderData, const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const
{

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ParticleSystemEmitter::RegisterBuiltinEmitters()
{
    static bool called = false;
    DebugAssert(!called);
    called = true;

#define REGISTER_TYPE(Type) Type::StaticMutableTypeInfo()->RegisterType()

    REGISTER_TYPE(ParticleSystemEmitter_Sprite);

#undef REGISTER_TYPE
}
