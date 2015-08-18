#include "BlockEngine/PrecompiledHeader.h"
#include "BlockEngine/BlockWorldChunkRenderProxy.h"
#include "BlockEngine/BlockWorldChunk.h"
#include "BlockEngine/BlockWorldSection.h"
#include "BlockEngine/BlockWorldMesher.h"
#include "BlockEngine/BlockWorld.h"
#include "BlockEngine/BlockEngineCVars.h"
#include "BlockEngine/BlockWorldVertexFactory.h"
#include "Engine/Camera.h"
#include "Engine/Engine.h"
#include "Renderer/Renderer.h"

//#define USE_LOCAL_TO_WORLD_TRANSFORM

BlockWorldChunkRenderProxy::BlockWorldChunkRenderProxy(uint32 entityID, const BlockPalette *pPalette, const float4x4 &transformMatrix, int32 lodLevel, BlockWorldMesher *pBuilder)
    : RenderProxy(entityID),
      m_pPalette(pPalette),
      m_transformMatrix(transformMatrix),
      m_lodLevel(lodLevel),
      m_renderResourcesCreated(false),
      m_pIndexBuffer(nullptr),
      m_indexFormat(GPU_INDEX_FORMAT_COUNT),
      m_pMeshInstanceBuffer(nullptr)
{
    // we maintain a reference to the section and palette because they may be deleted before it is
    // removed from world if the render thread is behind
    m_pPalette->AddRef();
}

BlockWorldChunkRenderProxy::~BlockWorldChunkRenderProxy()
{
    if (m_pMeshInstanceBuffer != nullptr)
        m_pMeshInstanceBuffer->Release();

    m_vertexBuffers.Clear();
    if (m_pIndexBuffer != nullptr)
        m_pIndexBuffer->Release();

    m_pPalette->Release();
}

BlockWorldMesher *BlockWorldChunkRenderProxy::CreateMesher(const BlockWorld *pWorld, const BlockWorldSection *pSection, const BlockWorldChunk *pChunk, int32 lodLevel)
{
    // find the neighbouring chunks
    const BlockWorldChunk *pNeighbourChunks[CUBE_FACE_COUNT];
    {
        int32 sectionSize = pWorld->GetSectionSize();
        int32 sectionSizeMinusOne = sectionSize - 1;
        int32 relativeX = pChunk->GetRelativeChunkX();
        int32 relativeY = pChunk->GetRelativeChunkY();
        int32 relativeZ = pChunk->GetRelativeChunkZ();
        int32 globalX = pChunk->GetGlobalChunkX();
        int32 globalY = pChunk->GetGlobalChunkY();
        int32 globalZ = pChunk->GetGlobalChunkZ();
        pNeighbourChunks[CUBE_FACE_LEFT] = (relativeX == 0) ? pWorld->GetChunk(globalX - 1, globalY, globalZ) : pSection->GetChunk(relativeX - 1, relativeY, relativeZ);
        pNeighbourChunks[CUBE_FACE_RIGHT] = (relativeX == sectionSizeMinusOne) ? pWorld->GetChunk(globalX + 1, globalY, globalZ) : pSection->GetChunk(relativeX + 1, relativeY, relativeZ);
        pNeighbourChunks[CUBE_FACE_BACK] = (relativeY == 0) ? pWorld->GetChunk(globalX, globalY - 1, globalZ) : pSection->GetChunk(relativeX, relativeY - 1, relativeZ);
        pNeighbourChunks[CUBE_FACE_FRONT] = (relativeY == sectionSizeMinusOne) ? pWorld->GetChunk(globalX, globalY + 1, globalZ) : pSection->GetChunk(relativeX, relativeY + 1, relativeZ);
        pNeighbourChunks[CUBE_FACE_BOTTOM] = pSection->SafeGetChunk(relativeX, relativeY, relativeZ - 1);
        pNeighbourChunks[CUBE_FACE_TOP] = pSection->SafeGetChunk(relativeX, relativeY, relativeZ + 1);

        // if anything doesn't have the lod level required, skip it
        for (uint32 i = 0; i < countof(pNeighbourChunks); i++)
        {
            if (pNeighbourChunks[i] != nullptr && pNeighbourChunks[i]->GetLoadedLODLevel() > lodLevel)
                pNeighbourChunks[i] = nullptr;
        }
    }

    // allocate the volume
    int32 chunkSize = (pChunk->GetChunkSize() >> lodLevel);
    int32 chunkSizeMinusOne = chunkSize - 1;
    int32 volumeSize = chunkSize + 2;
    int32 volumeSizeMinusOne = volumeSize - 1;
#ifdef USE_LOCAL_TO_WORLD_TRANSFORM
    BlockWorldMesher *pBuilder = new BlockWorldMesher(pWorld->GetPalette(), volumeSize, lodLevel, float3::Zero, CVars::r_block_world_use_lightmaps.GetBool());
#else
    BlockWorldMesher *pBuilder = new BlockWorldMesher(pWorld->GetPalette(), volumeSize, lodLevel, pChunk->GetBasePosition(), CVars::r_block_world_use_lightmaps.GetBool());
#endif
    BlockWorldBlockType *pBlockValues = pBuilder->GetBlockValues();
    BlockWorldBlockDataType *pBlockData = pBuilder->GetBlockData();
    int32 srcZStride = (chunkSize * chunkSize);
    int32 srcYStride = (chunkSize);
    int32 zStride = (volumeSize * volumeSize);
    int32 yStride = (volumeSize);

    // initialize the volume to zero
    Y_memzero(pBuilder->GetBlockValues(), sizeof(BlockWorldBlockType) * volumeSize * volumeSize * volumeSize);
    Y_memzero(pBuilder->GetBlockData(), sizeof(BlockWorldBlockDataType) * volumeSize * volumeSize * volumeSize);

    // extract the volume information. start at the bottom, and work up in height
    for (int32 z = 0; z < volumeSize; z++)
    {
        BlockWorldBlockType *pLayerStart = pBlockValues + (zStride * z);
        BlockWorldBlockDataType *pLayerDataStart = pBlockData + (zStride * z);
#define BLOCK(x_, y_) pLayerStart[(y_) * yStride + (x_)]
#define BLOCKDATA(x_, y_) pLayerDataStart[(y_) * yStride + (x_)]

        // the corners should always be zero 
        //BLOCK(0, 0) = 0;
        //BLOCK(0, volumeSizeMinusOne) = 0;
        //BLOCK(volumeSizeMinusOne, 0) = 0;
        //BLOCK(volumeSizeMinusOne, volumeSizeMinusOne) = 0;

        // bottom slab?
        if (z == 0)
        {
            // copy the bottom slab's top layer to our bottom slab
            if (pNeighbourChunks[CUBE_FACE_BOTTOM] != nullptr)
            {
                Y_memcpy_stride(&BLOCK(1, 1), sizeof(BlockWorldBlockType) * yStride, pNeighbourChunks[CUBE_FACE_BOTTOM]->GetBlockValues(lodLevel) + (srcZStride * chunkSizeMinusOne), sizeof(BlockWorldBlockType) * srcYStride, sizeof(BlockWorldBlockType) * chunkSize, chunkSize);
                Y_memcpy_stride(&BLOCKDATA(1, 1), sizeof(BlockWorldBlockDataType) * yStride, pNeighbourChunks[CUBE_FACE_BOTTOM]->GetBlockData(lodLevel) + (srcZStride * chunkSizeMinusOne), sizeof(BlockWorldBlockDataType) * srcYStride, sizeof(BlockWorldBlockDataType) * chunkSize, chunkSize);
            }
        }
        // top slab?
        else if (z == volumeSizeMinusOne)
        {
            // copy the bottom slab's bottom layer to our top slab
            if (pNeighbourChunks[CUBE_FACE_TOP] != nullptr)
            {
                Y_memcpy_stride(&BLOCK(1, 1), sizeof(BlockWorldBlockType) * yStride, pNeighbourChunks[CUBE_FACE_TOP]->GetBlockValues(lodLevel), sizeof(BlockWorldBlockType) * srcYStride, sizeof(BlockWorldBlockType) * chunkSize, chunkSize);
                Y_memcpy_stride(&BLOCKDATA(1, 1), sizeof(BlockWorldBlockDataType) * yStride, pNeighbourChunks[CUBE_FACE_TOP]->GetBlockData(lodLevel), sizeof(BlockWorldBlockDataType) * srcYStride, sizeof(BlockWorldBlockDataType) * chunkSize, chunkSize);
            }
        }
        // normal slabs
        else
        {
            // set the left-most column to what's on the left chunk
            if (pNeighbourChunks[CUBE_FACE_LEFT] != nullptr)
            {
                for (int32 y = 0; y < chunkSize; y++)
                {
                    BLOCK(0, y + 1) = pNeighbourChunks[CUBE_FACE_LEFT]->GetBlock(lodLevel, chunkSizeMinusOne, y, z - 1);
                    BLOCKDATA(0, y + 1) = pNeighbourChunks[CUBE_FACE_LEFT]->GetBlockData(lodLevel, chunkSizeMinusOne, y, z - 1);
                }
            }

            // set the right-most column to what's on the right chunk
            if (pNeighbourChunks[CUBE_FACE_RIGHT] != nullptr)
            {
                for (int32 y = 0; y < chunkSize; y++)
                {
                    BLOCK(volumeSizeMinusOne, y + 1) = pNeighbourChunks[CUBE_FACE_RIGHT]->GetBlock(lodLevel, 0, y, z - 1);
                    BLOCKDATA(volumeSizeMinusOne, y + 1) = pNeighbourChunks[CUBE_FACE_RIGHT]->GetBlockData(lodLevel, 0, y, z - 1);
                }
            }

            // set the first row to what's on the back chunk
            if (pNeighbourChunks[CUBE_FACE_BACK] != nullptr)
            {
                for (int32 x = 0; x < chunkSize; x++)
                {
                    BLOCK(x + 1, 0) = pNeighbourChunks[CUBE_FACE_BACK]->GetBlock(lodLevel, x, chunkSizeMinusOne, z - 1);
                    BLOCKDATA(x + 1, 0) = pNeighbourChunks[CUBE_FACE_BACK]->GetBlockData(lodLevel, x, chunkSizeMinusOne, z - 1);
                }
            }

            // set the last row to what's on the front chunk
            if (pNeighbourChunks[CUBE_FACE_FRONT] != nullptr)
            {
                for (int32 x = 0; x < chunkSize; x++)
                {
                    BLOCK(x + 1, volumeSizeMinusOne) = pNeighbourChunks[CUBE_FACE_FRONT]->GetBlock(lodLevel, x, 0, z - 1);
                    BLOCKDATA(x + 1, volumeSizeMinusOne) = pNeighbourChunks[CUBE_FACE_FRONT]->GetBlockData(lodLevel, x, 0, z - 1);
                }
            }

            // set everything else to the actual chunk data
            //Y_memcpy_stride(&BLOCK(1, 1), yStride, pChunk->GetBlockValues(lodLevel) + (srcZStride * (z - 1)), sizeof(BlockWorldBlockType) * srcYStride, sizeof(BlockWorldBlockType) * chunkSize, chunkSize);

            for (int32 y = 0; y < chunkSize; y++)
            {
                for (int32 x = 0; x < chunkSize; x++)
                {
                    BLOCK(x + 1, y + 1) = pChunk->GetBlock(lodLevel, x, y, z - 1);
                    BLOCKDATA(x + 1, y + 1) = pChunk->GetBlockData(lodLevel, x, y, z - 1);
                }
            }
        }
#undef BLOCKDATA
#undef BLOCK
    }

//     // invoke builder
//     pBuilder->GenerateMesh();
// 
//     // if nothing was output, bail out
//     if (pBuilder->GetOutputBatchCount() == 0 && pBuilder->GetOutputLightCount() == 0 && pBuilder->GetOutputMeshInstancesCount() == 0)
//     {
//         delete pBuilder;
//         return nullptr;
//     }

    return pBuilder;
}

BlockWorldChunkRenderProxy *BlockWorldChunkRenderProxy::CreateForChunk(uint32 entityID, const BlockWorld *pWorld, const BlockWorldSection *pSection, const BlockWorldChunk *pChunk, int32 lodLevel, BlockWorldMesher *pBuilder)
{
    // create transform matrix
    float3 basePosition(pChunk->GetBasePosition());
#ifdef USE_LOCAL_TO_WORLD_TRANSFORM
    float4x4 transformMatrix(float4x4::MakeTranslationMatrix(pChunk->GetBasePosition()));
#else
    float4x4 transformMatrix(float4x4::Identity);
#endif

    // create chunk
    BlockWorldChunkRenderProxy *pRenderProxy = new BlockWorldChunkRenderProxy(entityID, pWorld->GetPalette(), transformMatrix, lodLevel, pBuilder);

    // set initial bounds
#ifdef USE_LOCAL_TO_WORLD_TRANSFORM
    pRenderProxy->SetBounds(AABox(pBuilder->GetOutputBoundingBox().GetMinBounds() + basePosition, pBuilder->GetOutputBoundingBox().GetMaxBounds() + basePosition), Sphere(pBuilder->GetOutputBoundingSphere().GetCenter() + basePosition, pBuilder->GetOutputBoundingSphere().GetRadius()));
#else
    pRenderProxy->SetBounds(pBuilder->GetOutputBoundingBox(), pBuilder->GetOutputBoundingSphere());
#endif

    // upload to gpu, either deferred or immediate
    if (g_pRenderer->GetCapabilities().SupportsMultithreadedResourceCreation)
    {
        // if deferred upload fails, queue it for next frame
        if (!pRenderProxy->UploadMeshToGPU(pBuilder))
            pRenderProxy->QueueMeshUpload(pBuilder);
    }
    else
    {
        pRenderProxy->QueueMeshUpload(pBuilder);
    }

    // done
    return pRenderProxy;
}

bool BlockWorldChunkRenderProxy::RebuildForChunk(const BlockWorld *pWorld, const BlockWorldSection *pSection, const BlockWorldChunk *pChunk, int32 lodLevel, BlockWorldMesher *pBuilder)
{
    // queue upload always
    QueueMeshUpload(pBuilder);
    return true;
}

void BlockWorldChunkRenderProxy::QueueMeshUpload(BlockWorldMesher *pBuilder)
{
    DebugAssert(pBuilder != nullptr);

    // upload from render thread
    ReferenceCountedHolder<BlockWorldChunkRenderProxy> pThis(this);
    QUEUE_RENDERER_LAMBDA_COMMAND([pThis, pBuilder]()
    {
        pThis->ReleaseDeviceResources();
        if (!pThis->UploadMeshToGPU(pBuilder))
        {
            // upload failed :/ try again next frame..
            // this is basically a hack due to if we're not using threaded rendering,
            // the command will execute immediately, if it fails, again, causing a
            // stack overflow, so we ping-pong it back to the main thread
            QUEUE_MAIN_THREAD_LAMBDA_COMMAND([pThis, pBuilder]() {
                pThis->QueueMeshUpload(pBuilder);
            });
        }
    });
}

bool BlockWorldChunkRenderProxy::UploadMeshToGPU(BlockWorldMesher *pBuilder)
{
    // create the gpu buffers
    if (!pBuilder->CreateGPUBuffers(&m_vertexBuffers, &m_pIndexBuffer, &m_indexFormat, &m_pMeshInstanceBuffer))
        return false;

    // copy batches
    for (uint32 i = 0; i < pBuilder->GetOutputBatchCount(); i++)
    {
        const BlockWorldMesher::Batch &inBatch = pBuilder->GetOutputBatches().GetElement(i);
        m_renderBatches.Emplace(inBatch.MaterialIndex, inBatch.StartIndex, inBatch.NumIndices);
    }

    // copy mesh instances
    for (uint32 i = 0; i < pBuilder->GetOutputMeshInstancesCount(); i++)
    {
        const BlockWorldMesher::MeshInstances *inInstances = pBuilder->GetOutputMeshInstances().GetElement(i);
        m_renderMeshInstances.Emplace(inInstances->MeshIndex, inInstances->BufferOffset, inInstances->Transforms.GetSize());
    }

    // fixup bounds
#ifdef USE_LOCAL_TO_WORLD_TRANSFORM
    float3 basePosition(m_transformMatrix.GetColumn(3).xyz()); // hackity hack
    SetBounds(AABox(pBuilder->GetOutputBoundingBox().GetMinBounds() + basePosition, pBuilder->GetOutputBoundingBox().GetMaxBounds() + basePosition), Sphere(pBuilder->GetOutputBoundingSphere().GetCenter() + basePosition, pBuilder->GetOutputBoundingSphere().GetRadius()));
#else
    SetBounds(pBuilder->GetOutputBoundingBox(), pBuilder->GetOutputBoundingSphere());
#endif

    // copy lights
    m_lights.Clear();
    m_lights.AddArray(pBuilder->GetOutputLights());

    // update lod level
    m_lodLevel = pBuilder->GetLODLevel();

    // clean up
    delete pBuilder;
    m_renderResourcesCreated = true;
    return true;
}

bool BlockWorldChunkRenderProxy::CreateDeviceResources() const
{
    if (m_renderResourcesCreated)
        return true;

    // can't create from this method
    return false;
}

void BlockWorldChunkRenderProxy::ReleaseDeviceResources() const
{
    m_renderResourcesCreated = false;
    SAFE_RELEASE(m_pMeshInstanceBuffer);
    SAFE_RELEASE(m_pIndexBuffer);
    m_vertexBuffers.Clear();
    m_indexFormat = GPU_INDEX_FORMAT_COUNT;
    m_renderMeshInstances.Clear();
    m_renderBatches.Clear();
    m_lights.Clear();
}

void BlockWorldChunkRenderProxy::QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    // ensure upload has been done
    if (!m_renderResourcesCreated && !CreateDeviceResources())
        return;

    // add lights
    if (pRenderQueue->IsAcceptingLights())
    {
        for (const RENDER_QUEUE_POINT_LIGHT_ENTRY &light : m_lights)
        {
            if (pCamera->GetFrustum().SphereIntersection(Sphere(light.Position, light.Range)))
                pRenderQueue->AddLight(&light);
        }
    }

    // Store the requested render passes.
    uint32 wantedRenderPasses;
    if (CVars::r_block_world_use_lightmaps.GetBool())
        wantedRenderPasses = RENDER_PASSES_DEFAULT | RENDER_PASS_LIGHTMAP;
    else
        wantedRenderPasses = RENDER_PASSES_DEFAULT;

    // cvar r_block_terrain_shadows ?
    //if (m_shadowFlags & ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS)
        wantedRenderPasses |= RENDER_PASS_SHADOW_MAP;
    //if (!(m_shadowFlags & ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS))
        //wantedRenderPasses &= ~RENDER_PASS_SHADOWED_LIGHTING;

    // early exit?
    wantedRenderPasses &= pRenderQueue->GetAcceptingRenderPassMask();
    if (wantedRenderPasses == 0)
        return;

    // get view distance
    float3 boundingBoxCenter(GetBoundingBox().GetCenter());
    float viewDistance = pCamera->CalculateDepthToPoint(boundingBoxCenter);

    // cull based on xy distance
    if (viewDistance > pCamera->GetObjectCullDistance())
        return;

    // get batches
    for (uint32 batchIndex = 0; batchIndex < m_renderBatches.GetSize(); batchIndex++)
    {
        const RenderBatch &renderBatch = m_renderBatches[batchIndex];
        const Material *pMaterial = m_pPalette->GetMaterial(renderBatch.MaterialIndex);

        // get pass mask
        uint32 renderPassMask = pMaterial->GetShader()->SelectRenderPassMask(wantedRenderPasses);
        if (renderPassMask != 0)
        {
            // create queue entry for base layer
            RENDER_QUEUE_RENDERABLE_ENTRY queueEntry;
            queueEntry.pRenderProxy = this;
            queueEntry.pVertexFactoryTypeInfo = VERTEX_FACTORY_TYPE_INFO(BlockWorldVertexFactory);
            queueEntry.pMaterial = pMaterial;
            queueEntry.BoundingBox = GetBoundingBox();
            queueEntry.RenderPassMask = renderPassMask;
            queueEntry.VertexFactoryFlags = 0;
            queueEntry.ViewDistance = viewDistance;
            queueEntry.TintColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255);
            queueEntry.UserData[0] = 0;
            queueEntry.UserData[1] = batchIndex;
            queueEntry.Layer = pMaterial->GetShader()->SelectRenderQueueLayer();
            pRenderQueue->AddRenderable(&queueEntry);

            // create occluder if enabled
            //if (pRenderQueue->IsAcceptingOccluders() && CVars::r_block_world_occlusion.GetBool())
                //pRenderQueue->AddOccluder(this, queueEntry.BoundingBox, queueEntry.UserData, queueEntry.UserDataPointer);
        }
    }

    // occluder
    if (pRenderQueue->IsAcceptingOccluders() && CVars::r_block_world_occlusion.GetBool() && m_lodLevel == 0)
        pRenderQueue->AddOccluder(this, GetBoundingBox());

    // add instances
    for (uint32 meshIndex = 0; meshIndex < m_renderMeshInstances.GetSize(); meshIndex++)
    {
        const RenderMeshInstance &renderMeshInstance = m_renderMeshInstances[meshIndex];
        const StaticMesh *pMesh = m_pPalette->GetMesh(renderMeshInstance.MeshIndex);
        uint32 meshLODLevel = 0; 

        for (uint32 meshBatchIndex = 0; meshBatchIndex < pMesh->GetLOD(meshLODLevel)->GetBatchCount(); meshBatchIndex++)
        {
            const StaticMesh::Batch *pBatch = pMesh->GetLOD(meshLODLevel)->GetBatch(meshBatchIndex);
            const Material *pMaterial = pMesh->GetMaterial(pBatch->MaterialIndex);

            uint32 renderPassMask = pMaterial->GetShader()->SelectRenderPassMask(RENDER_PASSES_DEFAULT & pRenderQueue->GetAcceptingRenderPassMask());
            if (renderPassMask != 0)
            {
                RENDER_QUEUE_RENDERABLE_ENTRY queueEntry;
                queueEntry.pRenderProxy = this;
                queueEntry.pVertexFactoryTypeInfo = VERTEX_FACTORY_TYPE_INFO(LocalVertexFactory);
                queueEntry.pMaterial = pMaterial;
                queueEntry.BoundingBox = GetBoundingBox();
                queueEntry.RenderPassMask = renderPassMask;
                queueEntry.VertexFactoryFlags = pMesh->GetVertexFactoryFlags() | LOCAL_VERTEX_FACTORY_FLAG_INSTANCING_BY_MATRIX;
                queueEntry.ViewDistance = viewDistance;
                queueEntry.TintColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255);
                queueEntry.UserData[0] = meshIndex + 1;
                queueEntry.UserData[1] = meshLODLevel;
                queueEntry.UserData[2] = meshBatchIndex;
                queueEntry.Layer = pMaterial->GetShader()->SelectRenderQueueLayer();
                pRenderQueue->AddRenderable(&queueEntry);
            }
        }
    }

    // debug stuff
    if (pRenderQueue->IsAcceptingDebugObjects() &&
        CVars::r_block_world_show_lods.GetBool())
    {
        pRenderQueue->AddDebugInfoObject(this);
    }
}

void BlockWorldChunkRenderProxy::SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const
{
    if (pQueueEntry->UserData[0] == 0)
    {
        pGPUContext->GetConstants()->SetLocalToWorldMatrix(m_transformMatrix, true);
        pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);
        m_vertexBuffers.BindBuffers(pGPUContext);
        pGPUContext->SetIndexBuffer(m_pIndexBuffer, m_indexFormat, 0);
    }
    else
    {
        const RenderMeshInstance &renderMeshInstance = m_renderMeshInstances[pQueueEntry->UserData[0] - 1];
        const StaticMesh *pMesh = m_pPalette->GetMesh(renderMeshInstance.MeshIndex);
        const StaticMesh::LOD *pLOD = pMesh->GetLOD(pQueueEntry->UserData[1]);
        
        pGPUContext->GetConstants()->SetLocalToWorldMatrix(m_transformMatrix, true);
        pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);
        pGPUContext->SetVertexBuffer(0, pLOD->GetVertexBuffers()->GetBuffer(0), pLOD->GetVertexBuffers()->GetBufferOffset(0), pLOD->GetVertexBuffers()->GetBufferStride(0));
        pGPUContext->SetVertexBuffer(1, m_pMeshInstanceBuffer, renderMeshInstance.InstanceBufferOffset, sizeof(float3x4));
        pGPUContext->SetIndexBuffer(pLOD->GetIndexBuffer(), pLOD->GetIndexFormat(), 0);
    }
}

void BlockWorldChunkRenderProxy::DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const
{
    if (pQueueEntry->UserData[0] == 0)
    {
        uint32 batchIndex = pQueueEntry->UserData[1];
        DebugAssert(batchIndex < m_renderBatches.GetSize());

        const RenderBatch &renderBatch = m_renderBatches[batchIndex];
        pGPUContext->DrawIndexed(renderBatch.StartIndex, renderBatch.IndexCount, 0);
    }
    else
    {
        const RenderMeshInstance &renderMeshInstance = m_renderMeshInstances[pQueueEntry->UserData[0] - 1];
        const StaticMesh *pMesh = m_pPalette->GetMesh(renderMeshInstance.MeshIndex);
        const StaticMesh::LOD *pLOD = pMesh->GetLOD(pQueueEntry->UserData[1]);
        const StaticMesh::Batch *pBatch = pLOD->GetBatch(pQueueEntry->UserData[2]);

        pGPUContext->DrawIndexedInstanced(pBatch->StartIndex, pBatch->NumIndices, 0, renderMeshInstance.InstanceCount);
    }
}

void BlockWorldChunkRenderProxy::DrawDebugInfo(const Camera *pCamera, GPUContext *pGPUContext, MiniGUIContext *pGUIContext) const
{
    if (CVars::r_block_world_show_lods.GetBool())
    {
        static const uint32 lodColors[] = {
            MAKE_COLOR_R8G8B8A8_UNORM(0, 255, 0, 255),
            MAKE_COLOR_R8G8B8A8_UNORM(0, 255, 255, 255),
            MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 255),
            MAKE_COLOR_R8G8B8A8_UNORM(128, 255, 128, 255)
        };

        uint32 color = (m_lodLevel >= countof(lodColors)) ? lodColors[countof(lodColors) - 1] : lodColors[m_lodLevel];
        pGUIContext->SetDepthTestingEnabled(false);
        pGUIContext->SetAlphaBlendingEnabled(false);
        pGUIContext->Draw3DWireBox(GetBoundingBox(), color);
    }
}
