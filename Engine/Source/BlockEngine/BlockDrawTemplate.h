#pragma once
#include "BlockEngine/BlockWorldTypes.h"
#include "Engine/BlockPalette.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/Renderer.h"

class ShaderProgram;

class BlockDrawTemplate : public ReferenceCounted
{
public:
    class BlockRenderProxy : public RenderProxy
    {
    public:
        BlockRenderProxy(uint32 entityID, const BlockDrawTemplate *pDrawTemplate, uint32 startBatch, uint32 batchCount, const float4x4 &transformMatrix, uint32 tintColor);
        BlockRenderProxy(uint32 entityID, const BlockDrawTemplate *pDrawTemplate, BlockWorldBlockType blockType, const float4x4 &transformMatrix, uint32 tintColor);
        virtual ~BlockRenderProxy();

        void SetBlockType(BlockWorldBlockType blockType);
        void SetTransform(const float4x4 &transformMatrix);
        void SetTintColor(uint32 tintColor);
        void SetVisible(bool visible);

        virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override;
        virtual void SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const override;
        virtual void DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const override;

    private:
        // recalculate bounding box
        void UpdateBounds();

        // vars
        const BlockDrawTemplate *m_pDrawTemplate;
        uint32 m_startBatch;
        uint32 m_batchCount;
        float4x4 m_transformMatrix;
        uint32 m_tintColor;
        bool m_visible;
    };

public:
    BlockDrawTemplate(const BlockPalette *pPalette);
    ~BlockDrawTemplate();

    // Create meshes, buffers, batches.
    bool Initialize();

    // Get the starting batch and batch count for a block type.
    bool GetBatchRangeForBlock(BlockWorldBlockType blockType, uint32 *startBatch, uint32 *batchCount) const;

    // Retrieves the material for a specified batch.
    const Material *GetBatchMaterial(uint32 batchIndex) const;

    // Retrieves the vertex factory and flags for a specified batch.
    const VertexFactoryTypeInfo *GetBatchVertexFactoryType(uint32 batchIndex) const;
    const uint32 GetBatchVertexFactoryFlags(uint32 batchIndex) const;

    // Draws a batch using the specified shader.
    void DrawBatch(GPUContext *pContext, ShaderProgram *pShaderProgram, uint32 batchIndex) const;

    // Create a renderproxy instance to draw a block at the specified position. 
    // The origin is located at the bottom-front-left corner of the box.
    BlockRenderProxy *CreateBlockRenderProxy(BlockWorldBlockType blockType, uint32 entityID, const float4x4 &transformMatrix, uint32 tintColor = 0) const;
    
private:
    // properties
    const BlockPalette *m_pBlockPalette;

    // gpu buffers
    GPUBuffer *m_pVertexBuffer;
    GPUBuffer *m_pIndexBuffer;
    GPUBuffer *m_pInstanceTransformBuffer;

    // block information
    struct BlockDrawInfo
    {
        uint32 StartBatch;
        uint32 BatchCount;
    };

    // batch information
    struct Batch
    {
        Batch() {}
        Batch(const BlockPalette::BlockType *pBlockType_, const StaticMesh *pMesh_, const Material *pMaterial_, uint32 baseVertex, uint32 baseIndex, uint32 firstIndex, uint32 indexCount) : 
            pBlockType(pBlockType_), pMesh(pMesh_), pMaterial(pMaterial_), BaseVertex(baseVertex), 
            BaseIndex(baseIndex), FirstIndex(firstIndex), IndexCount(indexCount) {}

        const BlockPalette::BlockType *pBlockType;
        const StaticMesh *pMesh;
        const Material *pMaterial;
        uint32 BaseVertex;
        uint32 BaseIndex;
        uint32 FirstIndex;
        uint32 IndexCount;
    };

    // arrays
    MemArray<BlockDrawInfo> m_blockDrawInfo;
    MemArray<Batch> m_batches;
};

