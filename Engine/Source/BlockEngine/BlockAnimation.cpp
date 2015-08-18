#include "BlockEngine/PrecompiledHeader.h"
#include "BlockEngine/BlockAnimation.h"
#include "Engine/Physics/BoxCollisionShape.h"
#include "Engine/Physics/RigidBody.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Camera.h"
#include "Engine/World.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/Renderer.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Renderer/RenderWorld.h"
Log_SetChannel(BlockAnimation);

static const float BLOCK_START_FADEOUT_TIME = 1.0f;

class BlockAnimationBlockRenderProxy : public RenderProxy
{
public:
    struct Batch
    {
        Batch() {}
        Batch(const void *pSourcePtr_, const Material *pMaterial_, uint32 baseVertex, uint32 baseIndex, uint32 firstIndex, uint32 indexCount, const float4x4 &transformMatrix, const AABox &localBoundingBox, const AABox &worldBoundingBox) 
            : pSourcePtr(pSourcePtr_), pMaterial(pMaterial_),
              BaseVertex(baseVertex), BaseIndex(baseIndex), 
              FirstIndex(firstIndex), IndexCount(indexCount), 
              TransformMatrix(transformMatrix),
              LocalBoundingBox(localBoundingBox),
              WorldBoundingBox(worldBoundingBox),
              TintColor(0xFFFFFFFF) {}

        const void *pSourcePtr;
        const Material *pMaterial;
        uint32 BaseVertex;
        uint32 BaseIndex;
        uint32 FirstIndex;
        uint32 IndexCount;
        float4x4 TransformMatrix;
        AABox LocalBoundingBox;
        AABox WorldBoundingBox;
        uint32 TintColor;
    };

public:
    BlockAnimationBlockRenderProxy(uint32 entityID) 
        : RenderProxy(entityID),
          m_pIndexBuffer(nullptr)
    {

    }

    ~BlockAnimationBlockRenderProxy()
    {
        SAFE_RELEASE(m_pIndexBuffer);
        m_vertexBuffers.Clear();
    }

    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override
    {
        // add batches
        uint32 wantedRenderPasses = RENDER_PASSES_DEFAULT | RENDER_PASS_TINT;
        wantedRenderPasses &= pRenderQueue->GetAcceptingRenderPassMask();
        if (wantedRenderPasses == 0)
            return;

        // get batches
        for (uint32 batchIndex = 0; batchIndex < m_batches.GetSize(); batchIndex++)
        {
            const Batch *pBatch = &m_batches[batchIndex];

            // get view distance
            float3 boundingBoxCenter(pBatch->WorldBoundingBox.GetCenter());
            float viewDistance = pCamera->CalculateDepthToPoint(boundingBoxCenter);
            if (viewDistance > pCamera->GetObjectCullDistance())
                continue;

            // get pass mask
            uint32 renderPassMask = pBatch->pMaterial->GetShader()->SelectRenderPassMask(wantedRenderPasses);
            if (pBatch->TintColor == 0xFFFFFFFF)
                renderPassMask &= ~(RENDER_PASS_TINT);
            else
                renderPassMask &= ~(RENDER_PASS_SHADOW_MAP);

            if (renderPassMask != 0)
            {
                // create queue entry for base layer
                RENDER_QUEUE_RENDERABLE_ENTRY queueEntry;
                queueEntry.pRenderProxy = this;
                queueEntry.pVertexFactoryTypeInfo = VERTEX_FACTORY_TYPE_INFO(BlockWorldVertexFactory);
                queueEntry.pMaterial = pBatch->pMaterial;
                queueEntry.BoundingBox = pBatch->WorldBoundingBox;
                queueEntry.RenderPassMask = renderPassMask;
                queueEntry.VertexFactoryFlags = 0;
                queueEntry.ViewDistance = viewDistance;
                queueEntry.TintColor = pBatch->TintColor;
                queueEntry.UserData[0] = batchIndex;
                queueEntry.Layer = queueEntry.pMaterial->GetShader()->SelectRenderQueueLayer();
                pRenderQueue->AddRenderable(&queueEntry);
            }
        }
    }

    virtual void SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const override
    {
        const Batch *pBatch = &m_batches[pQueueEntry->UserData[0]];

        pGPUContext->GetConstants()->SetLocalToWorldMatrix(pBatch->TransformMatrix, true);
        m_vertexBuffers.BindBuffers(pGPUContext);
        pGPUContext->SetIndexBuffer(m_pIndexBuffer, GPU_INDEX_FORMAT_UINT16, 0);
        pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);
    }

    virtual void DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const override
    {
        const Batch *pBatch = &m_batches[pQueueEntry->UserData[0]];

        pGPUContext->DrawIndexed(pBatch->BaseIndex + pBatch->FirstIndex, pBatch->IndexCount, pBatch->BaseVertex);
    }

    void UpdateRenderData(MemArray<BlockWorldVertexFactory::Vertex> *pVertices, MemArray<uint16> *pIndices, MemArray<BlockAnimationBlockRenderProxy::Batch> *pBatches, const AABox &boundingBox)
    {
        // forward through to render thread
        QUEUE_RENDERER_LAMBDA_COMMAND([this, pVertices, pIndices, pBatches, boundingBox]() 
        {
            // create vertex buffer
            GPU_BUFFER_DESC vertexBufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER, pVertices->GetStorageSizeInBytes());
            AutoReleasePtr<GPUBuffer> pVertexBuffer = g_pRenderer->CreateBuffer(&vertexBufferDesc, pVertices->GetBasePointer());

            // create index buffer
            GPU_BUFFER_DESC indexBufferDesc(GPU_BUFFER_FLAG_BIND_INDEX_BUFFER, pIndices->GetStorageSizeInBytes());
            AutoReleasePtr<GPUBuffer> pIndexBuffer = g_pRenderer->CreateBuffer(&indexBufferDesc, pIndices->GetBasePointer());

            // store everything
            if (pVertexBuffer != nullptr && pIndexBuffer != nullptr)
            {
                m_vertexBuffers.SetBuffer(0, pVertexBuffer, 0, sizeof(BlockWorldVertexFactory::Vertex));
                SAFE_RELEASE(m_pIndexBuffer);
                m_pIndexBuffer = pIndexBuffer;
                m_pIndexBuffer->AddRef();
                m_batches.Swap(*pBatches);
                SetBounds(boundingBox, Sphere::FromAABox(boundingBox));
            }

            // release temporary buffers
            delete pBatches;
            delete pIndices;
            delete pVertices;
        });
    }

    void UpdateTransforms(const void *pSourcePtr, const float4x4 &newTransform, uint32 tintColor)
    {
        float4x4 transformCopy(newTransform);
        QUEUE_RENDERER_LAMBDA_COMMAND([this, pSourcePtr, transformCopy, tintColor]()
        {
            for (Batch &blockBatch : m_batches)
            {
                if (blockBatch.pSourcePtr == pSourcePtr)
                {
                    blockBatch.TransformMatrix = transformCopy;
                    blockBatch.WorldBoundingBox = blockBatch.LocalBoundingBox.GetTransformed(transformCopy);
                    blockBatch.TintColor = tintColor;
                }
            }                

            // update overall bounds
            AABox overallBounds(AABox::Zero);
            for (uint32 batchIndex = 0; batchIndex < m_batches.GetSize(); batchIndex++)
            {
                if (batchIndex == 0)
                    overallBounds = m_batches[batchIndex].WorldBoundingBox;
                else
                    overallBounds.Merge(m_batches[batchIndex].WorldBoundingBox);
            }
            SetBounds(overallBounds, Sphere::FromAABox(overallBounds));
        });
    }

private:
    VertexBufferBindingArray m_vertexBuffers;
    GPUBuffer *m_pIndexBuffer;
    MemArray<Batch> m_batches;
};

DEFINE_ENTITY_TYPEINFO(BlockAnimation, 0);
BEGIN_ENTITY_PROPERTIES(BlockAnimation)
END_ENTITY_PROPERTIES()
BEGIN_ENTITY_SCRIPT_FUNCTIONS(BlockAnimation)
END_ENTITY_SCRIPT_FUNCTIONS()

BlockAnimation::BlockAnimation(const EntityTypeInfo *pTypeInfo /*= &s_typeInfo*/)
    : Entity(pTypeInfo),
      m_pBlockPalette(nullptr),
      m_pBlockWorld(nullptr),
      m_autoDespawn(false),
      m_pBlockRenderProxy(nullptr),
      m_renderDataUpdatePending(false)
{

}

BlockAnimation::~BlockAnimation()
{
    DebugAssert(m_physicsBlocks.IsEmpty());
    SAFE_RELEASE(m_pBlockRenderProxy);
}

BlockAnimation *BlockAnimation::Create(BlockWorld *pBlockWorld, bool autoDespawn /*= true*/)
{
    BlockAnimation *pAnimation = new BlockAnimation();
    if (!pAnimation->Initialize(pBlockWorld->AllocateEntityID(), EmptyString))
    {
        pAnimation->Release();
        return nullptr;
    }
    
    // automatically put us in world, and register for updates.
    pAnimation->m_pBlockPalette = pBlockWorld->GetPalette();
    pAnimation->m_pBlockWorld = pBlockWorld;
    pAnimation->m_autoDespawn = autoDespawn;
    pAnimation->m_pBlockRenderProxy = new BlockAnimationBlockRenderProxy(pAnimation->GetEntityID());
    pBlockWorld->AddEntity(pAnimation);
    pAnimation->RegisterForUpdates();
    return pAnimation;
}

bool BlockAnimation::CreatePhysicsBlock(const float3 &basePosition, BlockWorldBlockType blockValue, const float3 &forceVector, float despawnTime /*= 5.0f*/)
{
    // lookup block info
    const BlockPalette::BlockType *pBlockType = m_pBlockPalette->GetBlockType(blockValue);
    if (pBlockType->ShapeType != BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
        return false;

    // calculate the physics block size
    float3 physicsBlockExtents(1.0f);

    // create physics block
    PhysicsBlock *pPhysicsBlock = new PhysicsBlock();
    pPhysicsBlock->SourceValue = blockValue;
    pPhysicsBlock->LastTransform.Set(basePosition, Quaternion::Identity, float3::One);
    pPhysicsBlock->TimeRemaining = despawnTime;

    // create the collision shape
    AutoReleasePtr<Physics::BoxCollisionShape> pCollisionShape = new Physics::BoxCollisionShape(physicsBlockExtents * 0.5f, physicsBlockExtents * 0.5f);
    pPhysicsBlock->pCollisionObject = new Physics::RigidBody(m_entityID, pCollisionShape, pPhysicsBlock->LastTransform);
    m_pWorld->GetPhysicsWorld()->AddObject(pPhysicsBlock->pCollisionObject);

    // apply the impulse
    if (forceVector.SquaredLength() > 0.0f)
        static_cast<Physics::RigidBody *>(pPhysicsBlock->pCollisionObject)->ApplyCentralImpulse(forceVector);

    // add to physics block list
    m_physicsBlocks.Add(pPhysicsBlock);
    m_renderDataUpdatePending = true;
    return true;
}

bool BlockAnimation::CreatePhysicsBlock(int32 x, int32 y, int32 z, const float3 &forceVector, bool removeBlock /*= true*/, float despawnTime /*= 5.0f*/)
{
    // retrieve the block value
    BlockWorldBlockType blockValue = m_pBlockWorld->GetBlockValue(x, y, z);
    if (blockValue == 0)
        return false;

    // calculate base position for this block
    float3 basePosition((float)x, (float)y, (float)z);

    // create physics block
    bool result = CreatePhysicsBlock(basePosition, blockValue, forceVector, despawnTime);

    // remove the block from the world?
    if (removeBlock)
        m_pBlockWorld->SetBlockType(x, y, z, 0);

    // done
    return result;
}

void BlockAnimation::OnAddToWorld(World *pWorld)
{
    BaseClass::OnAddToWorld(pWorld);

    DebugAssert(pWorld == m_pBlockWorld);
    pWorld->GetRenderWorld()->AddRenderable(m_pBlockRenderProxy);
}

void BlockAnimation::OnRemoveFromWorld(World *pWorld)
{
    DebugAssert(pWorld == m_pBlockWorld);
    
    // kill any remaining objects
    for (PhysicsBlock *pPhysicsBlock : m_physicsBlocks)
    {
        m_pWorld->GetPhysicsWorld()->RemoveObject(pPhysicsBlock->pCollisionObject);
        delete pPhysicsBlock;
    }
    m_physicsBlocks.Clear();

    pWorld->GetRenderWorld()->RemoveRenderable(m_pBlockRenderProxy);
    m_pBlockWorld = nullptr;

    BaseClass::OnRemoveFromWorld(pWorld);
}

void BlockAnimation::Update(float timeSinceLastUpdate)
{
    BaseClass::Update(timeSinceLastUpdate);

    // do updates...
    {
        // update physics blocks
        bool recalculateBounds = false;
        for (uint32 physicsBlockIndex = 0; physicsBlockIndex < m_physicsBlocks.GetSize();)
        {
            PhysicsBlock *pPhysicsBlock = m_physicsBlocks[physicsBlockIndex];

            // timeout blocks
            pPhysicsBlock->TimeRemaining -= timeSinceLastUpdate;
            if (pPhysicsBlock->TimeRemaining <= 0.0f)
            {
                m_pWorld->GetPhysicsWorld()->RemoveObject(pPhysicsBlock->pCollisionObject);

                delete pPhysicsBlock;
                m_physicsBlocks.FastRemove(physicsBlockIndex);
                m_renderDataUpdatePending = true;
                continue;
            }

            // pull collision data from the physics objects, if they've changed, update the render proxy
            if (pPhysicsBlock->pCollisionObject->GetTransform() != pPhysicsBlock->LastTransform || pPhysicsBlock->TimeRemaining < BLOCK_START_FADEOUT_TIME)
            {
                uint8 tintColor = 255;
                if (pPhysicsBlock->TimeRemaining < BLOCK_START_FADEOUT_TIME)
                {
                    float opacity = Math::Saturate(pPhysicsBlock->TimeRemaining / BLOCK_START_FADEOUT_TIME);
                    tintColor = (uint8)Math::Clamp(Math::Truncate(opacity * 255.0f), 0, 255);
                }

                pPhysicsBlock->LastTransform = pPhysicsBlock->pCollisionObject->GetTransform();
                m_pBlockRenderProxy->UpdateTransforms(pPhysicsBlock, pPhysicsBlock->LastTransform.GetTransformMatrix4x4(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, tintColor));
                recalculateBounds = true;
            }

            // next
            physicsBlockIndex++;
            continue;
        }        

        if (recalculateBounds)
        {
            AABox newBoundingBox(AABox::Zero);
            for (uint32 physicsBlockIndex = 0; physicsBlockIndex < m_physicsBlocks.GetSize(); physicsBlockIndex++)
            {
                PhysicsBlock *pPhysicsBlock = m_physicsBlocks[physicsBlockIndex];
                if (physicsBlockIndex == 0)
                    newBoundingBox = pPhysicsBlock->BoundingBox.GetTransformed(pPhysicsBlock->LastTransform);
                else
                    newBoundingBox.Merge(pPhysicsBlock->BoundingBox.GetTransformed(pPhysicsBlock->LastTransform));
            }

            SetBounds(newBoundingBox, Sphere::FromAABox(newBoundingBox));
        }
    }

    // despawn if there's nothing left
    if (m_physicsBlocks.IsEmpty())
    {
        m_pBlockWorld->QueueRemoveEntity(this);
        return;
    }

    // update render data
    if (m_renderDataUpdatePending)
    {
        m_renderDataUpdatePending = false;
        RebuildRenderData();
    }
}

void BlockAnimation::RebuildRenderData()
{
#if 0
    // create arrays
    MemArray<BlockWorldVertexFactory::Vertex> *pVertices = new MemArray<BlockWorldVertexFactory::Vertex>();
    MemArray<uint16> *pIndices = new MemArray<uint16>();
    MemArray<BlockAnimationBlockRenderProxy::Batch> *pBatches = new MemArray<BlockAnimationBlockRenderProxy::Batch>();
    MemArray<BlockWorldMesher::Triangle> blockTriangles;

    // fill with data
    AABox animationBoundingBox(AABox::Zero);
    for (uint32 physicsBlockIndex = 0; physicsBlockIndex < m_physicsBlocks.GetSize(); physicsBlockIndex++)
    {
        PhysicsBlock *pPhysicsBlock = m_physicsBlocks[physicsBlockIndex];
        uint32 baseVertex = pVertices->GetSize();
        uint32 baseIndex = pIndices->GetSize();

        // mesh the block
        blockTriangles.Clear();
        if (!BlockWorldMesher::MeshSingleBlock(m_pBlockPalette, pPhysicsBlock->SourceValue, *pVertices, blockTriangles))
            continue;

        // add triangles
        if (blockTriangles.GetSize() > 0)
        {
            // optimize triangle order
            MeshUtilites::OptimizeIndicesForBatching(&blockTriangles[0].Indices[0], sizeof(BlockWorldMesher::Triangle), &blockTriangles[0].MaterialIndex, sizeof(BlockWorldMesher::Triangle), blockTriangles.GetSize() * 3);

            // calculate bounding box
            AABox localBoundingBox(AABox::Zero);
            for (const BlockWorldMesher::Vertex &vertex : *pVertices)
                localBoundingBox.Merge(vertex.Position);

            // move bbox to world space
            AABox worldBoundingBox(localBoundingBox.GetTransformed(pPhysicsBlock->LastTransform));

            // create batches, append indices
            BlockAnimationBlockRenderProxy::Batch batch(pPhysicsBlock, m_pBlockPalette->GetMaterial(blockTriangles[0].MaterialIndex), baseVertex, baseIndex, 0, 0, pPhysicsBlock->LastTransform.GetTransformMatrix4x4(), localBoundingBox, worldBoundingBox);
            uint32 lastMaterialIndex = blockTriangles[0].MaterialIndex;
            for (uint32 triangleIndex = 0; triangleIndex < blockTriangles.GetSize(); triangleIndex++)
            {
                const BlockWorldMesher::Triangle &inTriangle = blockTriangles[triangleIndex];
                if (inTriangle.MaterialIndex == lastMaterialIndex)
                {
                    batch.IndexCount += 3;
                }
                else
                {
                    pBatches->Add(batch);
                    lastMaterialIndex = inTriangle.MaterialIndex;
                    batch.pMaterial = m_pBlockPalette->GetMaterial(lastMaterialIndex);
                    batch.FirstIndex = triangleIndex * 3;
                    batch.IndexCount = 3;
                }

                uint16 indices[3] = { (uint16)inTriangle.Indices[0], (uint16)inTriangle.Indices[1], (uint16)inTriangle.Indices[2] };
                pIndices->AddRange(indices, countof(indices));
            }

            // add last batch
            pBatches->Add(batch);

            // update bounds
            pPhysicsBlock->BoundingBox = localBoundingBox;

            // update overall bounding box
            if (physicsBlockIndex != 0)
                animationBoundingBox = worldBoundingBox;
            else
                animationBoundingBox.Merge(worldBoundingBox);
        }
    }

    // pass through to render proxy
    //Log_DevPrintf("pass to RP %u verts, %u inds, %u batches, bbox {%s}-{%s}", pVertices->GetSize(), pIndices->GetSize(), pBatches->GetSize(), StringConverter::Float3ToString(animationBoundingBox.GetMinBounds()).GetCharArray(), StringConverter::Float3ToString(animationBoundingBox.GetMaxBounds()).GetCharArray());
    m_pBlockRenderProxy->UpdateRenderData(pVertices, pIndices, pBatches, animationBoundingBox);
    SetBounds(animationBoundingBox, Sphere::FromAABox(animationBoundingBox));
#endif
}

void BlockAnimation::CreateSpawnBlock(const float3 &sourcePosition, int32 x, int32 y, int32 z, BlockWorldBlockType blockValue, float spawnTime /*= 1.0f*/, bool setAfterSpawn /*= true*/)
{

}
