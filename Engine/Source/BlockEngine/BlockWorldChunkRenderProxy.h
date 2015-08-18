#pragma once
#include "BlockEngine/BlockWorldTypes.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Renderer/RenderQueue.h"

class BlockWorldMesher;

class BlockWorldChunkRenderProxy : public RenderProxy
{
public:
    // batch info
    struct RenderBatch
    {
        uint32 MaterialIndex;
        uint32 StartIndex;
        uint32 IndexCount;

        RenderBatch(uint32 materialIndex, uint32 startIndex, uint32 indexCount) : MaterialIndex(materialIndex), StartIndex(startIndex), IndexCount(indexCount) {}
    };

    // mesh info
    struct RenderMeshInstance
    {
        uint32 MeshIndex;
        uint32 InstanceBufferOffset;
        uint32 InstanceCount;

        RenderMeshInstance(uint32 meshIndex, uint32 instanceBufferOffset, uint32 instanceCount) : MeshIndex(meshIndex), InstanceBufferOffset(instanceBufferOffset), InstanceCount(instanceCount) {}
    };

public:
    BlockWorldChunkRenderProxy(uint32 entityID, const BlockPalette *pPalette, const float4x4 &transformMatrix, int32 lodLevel, BlockWorldMesher *pBuilder);
    virtual ~BlockWorldChunkRenderProxy();

    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override;
    virtual void SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const override;
    virtual void DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const override;
    virtual void DrawDebugInfo(const Camera *pCamera, GPUContext *pGPUContext, MiniGUIContext *pGUIContext) const override;
    virtual bool CreateDeviceResources() const override;
    virtual void ReleaseDeviceResources() const override;

    static BlockWorldMesher *CreateMesher(const BlockWorld *pWorld, const BlockWorldSection *pSection, const BlockWorldChunk *pChunk, int32 lodLevel);
    static BlockWorldChunkRenderProxy *CreateForChunk(uint32 entityID, const BlockWorld *pWorld, const BlockWorldSection *pSection, const BlockWorldChunk *pChunk, int32 lodLevel, BlockWorldMesher *pBuilder);
    bool RebuildForChunk(const BlockWorld *pWorld, const BlockWorldSection *pSection, const BlockWorldChunk *pChunk, int32 lodLevel, BlockWorldMesher *pBuilder);

    // render resources
    const RenderBatch *GetRenderBatch(uint32 i) const { return &m_renderBatches[i]; }
    const uint32 GetRenderBatchCount() const { return m_renderBatches.GetSize(); }

private:
    void QueueMeshUpload(BlockWorldMesher *pBuilder);
    bool UploadMeshToGPU(BlockWorldMesher *pBuilder);

    // data
    const BlockPalette *m_pPalette;
    float4x4 m_transformMatrix;
    int32 m_lodLevel;

    // gpu resources
    mutable bool m_renderResourcesCreated;
    mutable VertexBufferBindingArray m_vertexBuffers;
    mutable GPUBuffer *m_pIndexBuffer;
    mutable GPU_INDEX_FORMAT m_indexFormat;
    mutable MemArray<RenderBatch> m_renderBatches;
    mutable GPUBuffer *m_pMeshInstanceBuffer;
    mutable MemArray<RenderMeshInstance> m_renderMeshInstances;

    // render resources
    mutable MemArray<RENDER_QUEUE_POINT_LIGHT_ENTRY> m_lights;
};
