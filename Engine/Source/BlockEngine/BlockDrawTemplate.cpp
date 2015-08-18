#include "BlockEngine/PrecompiledHeader.h"
#include "BlockEngine/BlockDrawTemplate.h"
#include "BlockEngine/BlockWorldMesher.h"
#include "Engine/Camera.h"
Log_SetChannel(BlockDrawTemplate);

BlockDrawTemplate::BlockDrawTemplate(const BlockPalette *pPalette)
    : m_pBlockPalette(pPalette),
      m_pVertexBuffer(nullptr),
      m_pIndexBuffer(nullptr)
{
    m_pBlockPalette->AddRef();
}

BlockDrawTemplate::~BlockDrawTemplate()
{
    SAFE_RELEASE(m_pIndexBuffer);
    SAFE_RELEASE(m_pVertexBuffer);
    SAFE_RELEASE(m_pBlockPalette);
}

bool BlockDrawTemplate::Initialize()
{
    // allocate block draw info
    m_blockDrawInfo.Resize(BLOCK_MESH_MAX_BLOCK_TYPES);
    m_blockDrawInfo.ZeroContents();

    // temporary buffers
    BlockWorldMesher::Output mesherOutput;
    MemArray<BlockWorldVertexFactory::Vertex> vertexArray;
    MemArray<uint16> indexArray;
    MemArray<float3x4> instanceArray;

    // for each block type, mesh it, and store to buffers
    for (uint32 blockTypeIndex = 0; blockTypeIndex < BLOCK_MESH_MAX_BLOCK_TYPES; blockTypeIndex++)
    {
        const BlockPalette::BlockType *pBlockType = m_pBlockPalette->GetBlockType(blockTypeIndex);
        if (!pBlockType->IsAllocated || !(pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE))
            continue;

        // stuff we've output
        uint32 baseBatch = m_batches.GetSize();
        uint32 batchCount = 0;

        // handle triangle meshes
        if (pBlockType->ShapeType != BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_MESH)
        {
            // save old state
            uint32 baseVertex = vertexArray.GetSize();
            uint32 baseIndex = indexArray.GetSize();

            // clear mesher output
            mesherOutput.Vertices.Clear();
            mesherOutput.Triangles.Clear();
            mesherOutput.Batches.Clear();
            mesherOutput.Instances.Clear();

            // mesh this block type
            if (!BlockWorldMesher::MeshSingleBlock(m_pBlockPalette, (BlockWorldBlockType)blockTypeIndex, mesherOutput))
                continue;

            // add triangle batches
            if (!mesherOutput.Batches.IsEmpty())
            {
                // append vertices
                vertexArray.AddArray(mesherOutput.Vertices);

                // append indices
                for (const BlockWorldMesher::Triangle &inTriangle : mesherOutput.Triangles)
                {
                    uint16 indices[3] = { (uint16)inTriangle.Indices[0], (uint16)inTriangle.Indices[1], (uint16)inTriangle.Indices[2] };
                    indexArray.AddRange(indices, countof(indices));
                }

                // append batches
                for (const BlockWorldMesher::Batch &inBatch : mesherOutput.Batches)
                {
                    Batch outBatch(pBlockType, nullptr, m_pBlockPalette->GetMaterial(inBatch.MaterialIndex), baseVertex, baseIndex, inBatch.StartIndex, inBatch.NumIndices);
                    m_batches.Add(outBatch);
                    batchCount++;
                }
            }
        }
        else
        {
            // handle mesh instances. each block type only has a single mesh
            const StaticMesh *pMesh = m_pBlockPalette->GetMesh(pBlockType->MeshShapeSettings.MeshIndex);
            const StaticMesh::LOD *pMeshLOD = pMesh->GetLOD(0);

            // create batches for each of the mesh's batches
            for (uint32 meshBatchIndex = 0; meshBatchIndex < pMeshLOD->GetBatchCount(); meshBatchIndex++)
            {
                const StaticMesh::Batch *pMeshBatch = pMeshLOD->GetBatch(meshBatchIndex);

                // copy stuff in
                Batch outBatch(pBlockType, pMesh, pMesh->GetMaterial(pMeshBatch->MaterialIndex), 0, 0, pMeshBatch->StartIndex, pMeshBatch->NumIndices);
                m_batches.Add(outBatch);
                batchCount++;
            }
        }

        // store information
        m_blockDrawInfo[blockTypeIndex].StartBatch = baseBatch;
        m_blockDrawInfo[blockTypeIndex].BatchCount = batchCount;
    }

    // create buffers
    if (!vertexArray.IsEmpty())
    {
        QUEUE_BLOCKING_RENDERER_LAMBA_COMMAND([this, &vertexArray, &indexArray]()
        {
            GPU_BUFFER_DESC vertexBufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER, vertexArray.GetStorageSizeInBytes());
            m_pVertexBuffer = g_pRenderer->CreateBuffer(&vertexBufferDesc, vertexArray.GetBasePointer());

            GPU_BUFFER_DESC indexBufferDesc(GPU_BUFFER_FLAG_BIND_INDEX_BUFFER, indexArray.GetStorageSizeInBytes());
            m_pIndexBuffer = g_pRenderer->CreateBuffer(&indexBufferDesc, indexArray.GetBasePointer());
        });

        if (m_pVertexBuffer == nullptr || m_pIndexBuffer == nullptr)
        {
            Log_ErrorPrintf("BlockPaletteDrawTemplate::Initialize: Failed to allocate vertex or index buffers.");
            return false;
        }
    }

    Log_InfoPrintf("BlockPaletteDrawTemplate::Initialize: %u vertices, %u indices, %u batches", vertexArray.GetSize(), indexArray.GetSize(), m_batches.GetSize());
    return true;
}

bool BlockDrawTemplate::GetBatchRangeForBlock(BlockWorldBlockType blockType, uint32 *startBatch, uint32 *batchCount) const
{
    DebugAssert(blockType < m_blockDrawInfo.GetSize());
    if (m_blockDrawInfo[blockType].BatchCount == 0)
        return false;

    *startBatch = m_blockDrawInfo[blockType].StartBatch;
    *batchCount = m_blockDrawInfo[blockType].BatchCount;
    return true;
}

const Material *BlockDrawTemplate::GetBatchMaterial(uint32 batchIndex) const
{
    DebugAssert(batchIndex < m_batches.GetSize());

    const Batch &batchInfo = m_batches[batchIndex];
    return batchInfo.pMaterial;
}

const VertexFactoryTypeInfo *BlockDrawTemplate::GetBatchVertexFactoryType(uint32 batchIndex) const
{
    DebugAssert(batchIndex < m_batches.GetSize());

    const Batch &batchInfo = m_batches[batchIndex];
    if (batchInfo.pMesh != nullptr)
        return VERTEX_FACTORY_TYPE_INFO(LocalVertexFactory);
    else
        return VERTEX_FACTORY_TYPE_INFO(BlockWorldVertexFactory);
}

const uint32 BlockDrawTemplate::GetBatchVertexFactoryFlags(uint32 batchIndex) const
{
    DebugAssert(batchIndex < m_batches.GetSize());

    const Batch &batchInfo = m_batches[batchIndex];
    if (batchInfo.pMesh != nullptr)
        return batchInfo.pMesh->GetVertexFactoryFlags();
    else
        return 0;
}

void BlockDrawTemplate::DrawBatch(GPUContext *pContext, ShaderProgram *pShaderProgram, uint32 batchIndex) const
{
    const Batch &batchInfo = m_batches[batchIndex];
    if (batchInfo.pMesh != nullptr)
    {
        const StaticMesh::LOD *pMeshLOD = batchInfo.pMesh->GetLOD(0);
        if (!pMeshLOD->CreateGPUResources())
            return;
        
        // bind the mesh buffers
        pMeshLOD->GetVertexBuffers()->BindBuffers(pContext);
        pContext->SetIndexBuffer(pMeshLOD->GetIndexBuffer(), pMeshLOD->GetIndexFormat(), 0);
        pContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);

        // bit of a hack, we just fudge the world matrix to push the mesh to the right location..
        float scale = batchInfo.pBlockType->MeshShapeSettings.Scale;
        float4x4 worldMatrixOffset(scale, 0.0f, 0.0f, 0.5f,
                                   0.0f, scale, 0.0f, 0.5f,
                                   0.0f, 0.0f, scale, 0.0f,
                                   0.0f, 0.0f, 0.0f, 1.0f);
        float4x4 oldWorldMatrix(pContext->GetConstants()->GetLocalToWorldMatrix());
        float4x4 newWorldMatrix(oldWorldMatrix * worldMatrixOffset);
        pContext->GetConstants()->SetLocalToWorldMatrix(newWorldMatrix, false);
        pContext->GetConstants()->CommitChanges();

        // draw the mesh batch
        pContext->DrawIndexed(batchInfo.FirstIndex, batchInfo.IndexCount, 0);

        // restore the old world matrix
        pContext->GetConstants()->SetLocalToWorldMatrix(newWorldMatrix, false);
        pContext->GetConstants()->CommitChanges();
    }
    else
    {
        // draw normally
        pContext->SetVertexBuffer(0, m_pVertexBuffer, 0, sizeof(BlockWorldVertexFactory::Vertex));
        pContext->SetIndexBuffer(m_pIndexBuffer, GPU_INDEX_FORMAT_UINT16, 0);
        pContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);
        pContext->DrawIndexed(batchInfo.BaseIndex + batchInfo.FirstIndex, batchInfo.IndexCount, batchInfo.BaseVertex);
    }
}

BlockDrawTemplate::BlockRenderProxy::BlockRenderProxy(uint32 entityID, const BlockDrawTemplate *pDrawTemplate, uint32 startBatch, uint32 batchCount, const float4x4 &transformMatrix, uint32 tintColor)
    : RenderProxy(entityID),
      m_pDrawTemplate(pDrawTemplate),
      m_startBatch(startBatch),
      m_batchCount(batchCount),
      m_transformMatrix(transformMatrix),
      m_tintColor(tintColor),
      m_visible(true)
{
    m_pDrawTemplate->AddRef();
    UpdateBounds();
}

BlockDrawTemplate::BlockRenderProxy::BlockRenderProxy(uint32 entityID, const BlockDrawTemplate *pDrawTemplate, BlockWorldBlockType blockType, const float4x4 &transformMatrix, uint32 tintColor)
    : RenderProxy(entityID),
      m_pDrawTemplate(pDrawTemplate),
      m_transformMatrix(transformMatrix),
      m_tintColor(tintColor),
      m_visible(true)
{
    if (!pDrawTemplate->GetBatchRangeForBlock(blockType, &m_startBatch, &m_batchCount))
        m_startBatch = m_batchCount = 0;

    m_pDrawTemplate->AddRef();
    UpdateBounds();
}

BlockDrawTemplate::BlockRenderProxy::~BlockRenderProxy()
{
    m_pDrawTemplate->Release();
}

void BlockDrawTemplate::BlockRenderProxy::SetBlockType(BlockWorldBlockType blockType)
{
    uint32 startBatch, batchCount;
    if (!m_pDrawTemplate->GetBatchRangeForBlock(blockType, &startBatch, &batchCount))
        startBatch = batchCount = 0;

    ReferenceCountedHolder<BlockRenderProxy> pThis(this);
    QUEUE_RENDERER_LAMBDA_COMMAND([pThis, startBatch, batchCount]()
    {
        pThis->m_startBatch = startBatch;
        pThis->m_batchCount = batchCount;
        pThis->UpdateBounds();
    });
}

void BlockDrawTemplate::BlockRenderProxy::SetTransform(const float4x4 &transformMatrix)
{
    ReferenceCountedHolder<BlockRenderProxy> pThis(this);
    QUEUE_RENDERER_LAMBDA_COMMAND([pThis, transformMatrix]()
    {
        pThis->m_transformMatrix = transformMatrix;
        pThis->UpdateBounds();
    });
}

void BlockDrawTemplate::BlockRenderProxy::SetTintColor(uint32 tintColor)
{
    ReferenceCountedHolder<BlockRenderProxy> pThis(this);
    QUEUE_RENDERER_LAMBDA_COMMAND([pThis, tintColor]() {
        pThis->m_tintColor = tintColor;
    });
}

void BlockDrawTemplate::BlockRenderProxy::SetVisible(bool visible)
{
    ReferenceCountedHolder<BlockRenderProxy> pThis(this);
    QUEUE_RENDERER_LAMBDA_COMMAND([pThis, visible]() {
        pThis->m_visible = visible;
    });
}

void BlockDrawTemplate::BlockRenderProxy::QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    // not visible?
    if (!m_visible)
        return;

    // add batches
    uint32 wantedRenderPasses = RENDER_PASSES_DEFAULT;
    if (m_tintColor != 0)
    {
        // tinted can't cast shadows.. it looks weird :S
        wantedRenderPasses |= RENDER_PASS_TINT;
        wantedRenderPasses &= ~(RENDER_PASS_SHADOW_MAP);
    }

    // fix up passes
    wantedRenderPasses &= pRenderQueue->GetAcceptingRenderPassMask();
    if (wantedRenderPasses == 0)
        return;

    // get batches
    for (uint32 batchAdd = 0; batchAdd < m_batchCount; batchAdd++)
    {
        uint32 batchIndex = m_startBatch + batchAdd;
        const Batch &batchInfo = m_pDrawTemplate->m_batches[batchIndex];
        
        // get view distance
        float viewDistance = pCamera->CalculateDepthToBox(GetBoundingBox());
        if (viewDistance > pCamera->GetObjectCullDistance())
            continue;

        // get pass mask
        uint32 renderPassMask = batchInfo.pMaterial->GetShader()->SelectRenderPassMask(wantedRenderPasses);
        if (renderPassMask != 0)
        {
            // create queue entry for base layer
            RENDER_QUEUE_RENDERABLE_ENTRY queueEntry;
            queueEntry.pRenderProxy = this;

            // add local or mesh vertex factory
            if (batchInfo.pMesh != nullptr)
            {
                queueEntry.pVertexFactoryTypeInfo = VERTEX_FACTORY_TYPE_INFO(LocalVertexFactory);
                queueEntry.VertexFactoryFlags = batchInfo.pMesh->GetVertexFactoryFlags();
            }
            else
            {
                queueEntry.pVertexFactoryTypeInfo = VERTEX_FACTORY_TYPE_INFO(BlockWorldVertexFactory);
                queueEntry.VertexFactoryFlags = 0;
            }

            // remaining fields
            queueEntry.pMaterial = batchInfo.pMaterial;
            queueEntry.BoundingBox = GetBoundingBox();
            queueEntry.RenderPassMask = renderPassMask;
            queueEntry.ViewDistance = viewDistance;
            queueEntry.TintColor = m_tintColor;
            queueEntry.UserData[0] = batchIndex;
            queueEntry.Layer = queueEntry.pMaterial->GetShader()->SelectRenderQueueLayer();
            pRenderQueue->AddRenderable(&queueEntry);
        }
    }
}

void BlockDrawTemplate::BlockRenderProxy::SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const
{
    const Batch &batchInfo = m_pDrawTemplate->m_batches[pQueueEntry->UserData[0]];

    if (batchInfo.pMesh != nullptr)
    {
        const StaticMesh::LOD *pMeshLOD = batchInfo.pMesh->GetLOD(0);
        if (!pMeshLOD->CreateGPUResources())
            return;

        // bind the mesh buffers
        pMeshLOD->GetVertexBuffers()->BindBuffers(pGPUContext);
        pGPUContext->SetIndexBuffer(pMeshLOD->GetIndexBuffer(), pMeshLOD->GetIndexFormat(), 0);
        pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);

        // bit of a hack, we just fudge the world matrix to push the mesh to the right location..
        float scale = batchInfo.pBlockType->MeshShapeSettings.Scale;
        float4x4 worldMatrixOffset(scale, 0.0f, 0.0f, 0.5f,
                                   0.0f, scale, 0.0f, 0.5f,
                                   0.0f, 0.0f, scale, 0.0f,
                                   0.0f, 0.0f, 0.0f, 1.0f);

        pGPUContext->GetConstants()->SetLocalToWorldMatrix(m_transformMatrix * worldMatrixOffset, true);
    }
    else
    {
        pGPUContext->SetVertexBuffer(0, m_pDrawTemplate->m_pVertexBuffer, 0, sizeof(BlockWorldVertexFactory::Vertex));
        pGPUContext->SetIndexBuffer(m_pDrawTemplate->m_pIndexBuffer, GPU_INDEX_FORMAT_UINT16, 0);
        pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);
        pGPUContext->GetConstants()->SetLocalToWorldMatrix(m_transformMatrix, true);
    }
}

void BlockDrawTemplate::BlockRenderProxy::DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const
{
    const Batch &batchInfo = m_pDrawTemplate->m_batches[pQueueEntry->UserData[0]];

    pGPUContext->DrawIndexed(batchInfo.BaseIndex + batchInfo.FirstIndex, batchInfo.IndexCount, batchInfo.BaseVertex);
}

void BlockDrawTemplate::BlockRenderProxy::UpdateBounds()
{
    AABox cubeBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    AABox transformedBox(cubeBox.GetTransformed(m_transformMatrix));
    SetBounds(transformedBox, Sphere::FromAABox(transformedBox));
}

BlockDrawTemplate::BlockRenderProxy *BlockDrawTemplate::CreateBlockRenderProxy(BlockWorldBlockType blockType, uint32 entityID, const float4x4 &transformMatrix, uint32 tintColor /* = 0 */) const
{
    uint32 startBatch, batchCount;
    if (!GetBatchRangeForBlock(blockType, &startBatch, &batchCount))
        return nullptr;

    return new BlockRenderProxy(entityID, this, startBatch, batchCount, transformMatrix, tintColor);
}

