#include "Renderer/PrecompiledHeader.h"
#include "Renderer/RenderProxies/CompositeRenderProxy.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/Renderer.h"
#include "Engine/Camera.h"
#include "Engine/Material.h"
Log_SetChannel(CompositeRenderProxy);

CompositeRenderProxy::CompositeRenderProxy(uint32 entityId, const Transform &transform)
    : RenderProxy(entityId),
      m_Visibility(true),
      m_ShadowFlags(0),
      m_nextObjectId(0),
      m_bGPUResourcesCreated(false)
{
    SetTransform(transform);
}

CompositeRenderProxy::~CompositeRenderProxy()
{
    CompositeRenderProxy::ReleaseDeviceResources();

    for (uint32 i = 0; i < m_objectRecords.GetSize(); i++)
    {
        ObjectRecord &obj = m_objectRecords[i];
        for (uint32 j = 0; j < obj.MaterialCount; j++)
        {
            if (obj.ppMaterials[j] != NULL)
                obj.ppMaterials[j]->Release();
        }
        delete[] obj.ppMaterials;
        delete[] obj.pBatches;
    }

    for (uint32 i = 0; i < m_updateObjectRecords.GetSize(); i++)
    {
        UpdateObjectRecord &obj = m_updateObjectRecords[i];
        Y_free(obj.pVertices);
        Y_free(obj.pIndices);
        for (uint32 j = 0; j < obj.MaterialCount; j++)
        {
            if (obj.ppMaterials[j] != NULL)
                obj.ppMaterials[j]->Release();
        }
        delete[] obj.ppMaterials;
        delete[] obj.pBatches;
    }
}

void CompositeRenderProxy::SetTransform(const Transform &transform)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_Transform = transform;
    m_LocalToWorldMatrix = m_Transform.GetTransformMatrix4x4();
    UpdateBounds();
}

void CompositeRenderProxy::SetVisibility(bool visible)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_Visibility = visible;
}

void CompositeRenderProxy::SetShadowFlags(uint32 shadowFlags)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_ShadowFlags = shadowFlags;
}

uint32 CompositeRenderProxy::AddObject()
{
    UpdateObjectRecord obj;
    obj.ObjectId = m_nextObjectId++;
    obj.UpdateFlags = UPDATE_OBJECT_FLAG_CREATE | UPDATE_OBJECT_FLAG_BUFFERS | UPDATE_OBJECT_FLAG_TINT;
    obj.pVertexFactoryTypeInfo = NULL;
    obj.VertexFactoryFlags = 0;
    obj.pVertices = NULL;
    obj.VertexSize = 0;
    obj.VertexCount = 0;
    obj.pIndices = NULL;
    obj.IndexFormat = GPU_INDEX_FORMAT_UINT16;
    obj.IndexCount = 0;
    obj.ppMaterials = NULL;
    obj.MaterialCount = 0;
    obj.pBatches = NULL;
    obj.BatchCount = 0;
    obj.TintEnabled = false;
    obj.TintColor = 0;
    obj.BoundingBox = AABox::Zero;
    obj.BoundingSphere = Sphere::Zero;
    
    m_updateObjectRecords.Add(obj);

    UpdateObjects();

    return obj.ObjectId;
}

void CompositeRenderProxy::UpdateObject(uint32 objectId, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const void *pVertices, uint32 vertexSize, uint32 vertexCount, const void *pIndices, GPU_INDEX_FORMAT indexFormat, uint32 indexCount, const Material **ppMaterials, uint32 materialCount, const Batch *pBatches, uint32 batchCount, const AABox &boundingBox, const Sphere &boundingSphere)
{
    UpdateObjectRecord obj;
    obj.ObjectId = objectId;
    obj.UpdateFlags = UPDATE_OBJECT_FLAG_BUFFERS;
    obj.pVertexFactoryTypeInfo = pVertexFactoryTypeInfo;
    obj.VertexFactoryFlags = vertexFactoryFlags;
    obj.pVertices = Y_malloc(vertexSize * vertexCount);
    Y_memcpy(obj.pVertices, pVertices, vertexSize * vertexCount);
    obj.VertexSize = vertexSize;
    obj.VertexCount = vertexCount;
    obj.pIndices = Y_malloc(((indexFormat == GPU_INDEX_FORMAT_UINT16) ? sizeof(uint16) : sizeof(uint32)) * indexCount);
    Y_memcpy(obj.pIndices, pIndices, ((indexFormat == GPU_INDEX_FORMAT_UINT16) ? sizeof(uint16) : sizeof(uint32)) * indexCount);
    obj.IndexFormat = indexFormat;
    obj.IndexCount = indexCount;
    obj.ppMaterials = new const Material *[materialCount];
    for (uint32 i = 0; i < materialCount; i++)
    {
        if ((obj.ppMaterials[i] = ppMaterials[i]) != NULL)
            obj.ppMaterials[i]->AddRef();
    }
    obj.MaterialCount = materialCount;
    obj.pBatches = new Batch[batchCount];
    Y_memcpy(obj.pBatches, pBatches, sizeof(Batch) * batchCount);
    obj.BatchCount = batchCount;
    obj.TintEnabled = false;
    obj.TintColor = 0;
    obj.BoundingBox = boundingBox;
    obj.BoundingSphere = boundingSphere;
    m_updateObjectRecords.Add(obj);

    UpdateObjects();
}

void CompositeRenderProxy::UpdateObjectTintColor(uint32 objectId, bool tintEnabled, uint32 tintColor)
{
    UpdateObjectRecord obj;
    obj.ObjectId = objectId;
    obj.UpdateFlags = UPDATE_OBJECT_FLAG_TINT;
    obj.pVertexFactoryTypeInfo = NULL;
    obj.VertexFactoryFlags = 0;
    obj.pVertices = NULL;
    obj.pIndices = NULL;
    obj.IndexFormat = GPU_INDEX_FORMAT_UINT16;
    obj.IndexCount = 0;
    obj.IndexFormat = GPU_INDEX_FORMAT_UINT16;
    obj.ppMaterials = NULL;
    obj.MaterialCount = 0;
    obj.pBatches = NULL;
    obj.BatchCount = 0;
    obj.TintEnabled = tintEnabled;
    obj.TintColor = tintColor;
    obj.BoundingBox = AABox::Zero;
    obj.BoundingSphere = Sphere::Zero;
    m_updateObjectRecords.Add(obj);

    UpdateObjects();
}

void CompositeRenderProxy::ClearObject(uint32 objectId)
{
    UpdateObjectRecord obj;
    obj.ObjectId = objectId;
    obj.UpdateFlags = UPDATE_OBJECT_FLAG_BUFFERS;
    obj.pVertexFactoryTypeInfo = NULL;
    obj.VertexFactoryFlags = 0;
    obj.pVertices = NULL;
    obj.VertexSize = 0;
    obj.VertexCount = 0;
    obj.pIndices = NULL;
    obj.IndexFormat = GPU_INDEX_FORMAT_UINT16;
    obj.IndexCount = 0;
    obj.ppMaterials = NULL;
    obj.MaterialCount = 0;
    obj.pBatches = NULL;
    obj.BatchCount = 0;
    obj.TintEnabled = false;
    obj.TintColor = 0;
    obj.BoundingBox = AABox::Zero;
    obj.BoundingSphere = Sphere::Zero;
    m_updateObjectRecords.Add(obj);

    UpdateObjects();
}

void CompositeRenderProxy::DeleteObject(uint32 objectId)
{
    UpdateObjectRecord obj;
    obj.ObjectId = objectId;
    obj.UpdateFlags = UPDATE_OBJECT_FLAG_DELETE;
    obj.pVertexFactoryTypeInfo = NULL;
    obj.VertexFactoryFlags = 0;
    obj.pVertices = NULL;
    obj.VertexSize = 0;
    obj.VertexCount = 0;
    obj.pIndices = NULL;
    obj.IndexFormat = GPU_INDEX_FORMAT_UINT16;
    obj.IndexCount = 0;
    obj.ppMaterials = NULL;
    obj.MaterialCount = 0;
    obj.pBatches = NULL;
    obj.BatchCount = 0;
    obj.TintEnabled = false;
    obj.TintColor = 0;
    obj.BoundingBox = AABox::Zero;
    obj.BoundingSphere = Sphere::Zero;
    m_updateObjectRecords.Add(obj);

    UpdateObjects();
}

void CompositeRenderProxy::DeleteAllObjects()
{
    for (uint32 i = 0; i < m_objectRecords.GetSize(); i++)
        DeleteObject(m_objectRecords[i].ObjectId);
}

void CompositeRenderProxy::UpdateObjects()
{
    bool updateBounds = false;

    for (uint32 i = 0; i < m_updateObjectRecords.GetSize(); i++)
    {
        UpdateObjectRecord &updateObj = m_updateObjectRecords[i];
        if (updateObj.UpdateFlags & UPDATE_OBJECT_FLAG_CREATE)
        {
            DebugAssert((updateObj.UpdateFlags & UPDATE_OBJECT_FLAG_DELETE) == 0);
            
            ObjectRecord obj;
            obj.ObjectId = updateObj.ObjectId;
            obj.pVertexFactoryTypeInfo = NULL;
            obj.VertexFactoryFlags = 0;
            obj.pVertexBuffer = NULL;
            obj.VertexStride = 0;
            obj.pIndexBuffer = NULL;
            obj.IndexFormat = GPU_INDEX_FORMAT_UINT16;
            obj.ppMaterials = NULL;
            obj.MaterialCount = 0;
            obj.pBatches = NULL;
            obj.BatchCount = 0;
            obj.TintEnabled = false;
            obj.TintColor = 0;
            obj.BoundingBox = AABox::Zero;
            obj.BoundingSphere = Sphere::Zero;
            m_objectRecords.Add(obj);
        }

        uint32 objectId = updateObj.ObjectId;
        uint32 objectIndex = 0;
        for (; objectIndex < m_objectRecords.GetSize(); objectIndex++)
        {
            if (m_objectRecords[objectIndex].ObjectId == objectId)
                break;
        }
        Assert(objectIndex < m_objectRecords.GetSize());

        ObjectRecord &obj = m_objectRecords[objectIndex];
        
        if (updateObj.UpdateFlags & UPDATE_OBJECT_FLAG_DELETE)
        {
            if (obj.pVertexBuffer != NULL)
                obj.pVertexBuffer->Release();
            if (obj.pIndexBuffer != NULL)
                obj.pIndexBuffer->Release();
            for (uint32 j = 0; j < obj.MaterialCount; j++)
            {
                if (obj.ppMaterials[j] != NULL)
                    obj.ppMaterials[j]->Release();
            }
            delete[] obj.ppMaterials;
            delete[] obj.pBatches;
            m_objectRecords.OrderedRemove(objectIndex);
            updateBounds = true;
        }
        else
        {
            if (updateObj.UpdateFlags & UPDATE_OBJECT_FLAG_BUFFERS)
            {
                GPUBuffer *pVertexBuffer = NULL;
                GPUBuffer *pIndexBuffer = NULL;
                GPU_BUFFER_DESC vertexBufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER, updateObj.VertexSize * updateObj.VertexCount);
                GPU_BUFFER_DESC indexBufferDesc(GPU_BUFFER_FLAG_BIND_INDEX_BUFFER, ((updateObj.IndexFormat == GPU_INDEX_FORMAT_UINT16) ? sizeof(uint16) : sizeof(uint32)) * updateObj.IndexCount);
                if (updateObj.VertexCount > 0 && updateObj.IndexCount > 0 &&
                    ((pVertexBuffer = g_pRenderer->CreateBuffer(&vertexBufferDesc, updateObj.pVertices)) == NULL ||
                    (pIndexBuffer = g_pRenderer->CreateBuffer(&indexBufferDesc, updateObj.pIndices)) == NULL))
                {
                    Log_ErrorPrintf("Failed to allocate composite render proxy vertex buffers");
                    SAFE_RELEASE(pVertexBuffer);
                    SAFE_RELEASE(pIndexBuffer);
                }
                else
                {
                    if (obj.pVertexBuffer != NULL)
                        obj.pVertexBuffer->Release();
                    if (obj.pIndexBuffer != NULL)
                        obj.pIndexBuffer->Release();
                    for (uint32 j = 0; j < obj.MaterialCount; j++)
                    {
                        if (obj.ppMaterials[j] != NULL)
                            obj.ppMaterials[j]->Release();
                    }
                    delete[] obj.ppMaterials;
                    delete[] obj.pBatches;

                    obj.pVertexFactoryTypeInfo = updateObj.pVertexFactoryTypeInfo;
                    obj.VertexFactoryFlags = updateObj.VertexFactoryFlags;
                    obj.pVertexBuffer = pVertexBuffer;
                    obj.VertexStride = updateObj.VertexSize;
                    obj.pIndexBuffer = pIndexBuffer;
                    obj.IndexFormat = updateObj.IndexFormat;
                    obj.ppMaterials = updateObj.ppMaterials;
                    obj.MaterialCount = updateObj.MaterialCount;
                    obj.pBatches = updateObj.pBatches;
                    obj.BatchCount = updateObj.BatchCount;
                    obj.BoundingBox = updateObj.BoundingBox;
                    obj.BoundingSphere = updateObj.BoundingSphere;

                    Y_free(updateObj.pVertices);
                    Y_free(updateObj.pIndices);

                    updateBounds = true;
                }
            }

            if (updateObj.UpdateFlags & UPDATE_OBJECT_FLAG_TINT)
            {
                obj.TintEnabled = updateObj.TintEnabled;
                obj.TintColor = updateObj.TintColor;
            }
        }
    }
    m_updateObjectRecords.Clear();

    if (updateBounds)
        UpdateBounds();
}

void CompositeRenderProxy::UpdateBounds()
{
    AABox boundingBox(AABox::Zero);
    Sphere boundingSphere(Sphere::Zero);
    bool first = true;

    for (uint32 i = 0; i < m_objectRecords.GetSize(); i++)
    {
        if (m_objectRecords[i].BatchCount == 0)
            continue;

        if (first)
        {
            boundingBox = m_objectRecords[i].BoundingBox;
            boundingSphere = m_objectRecords[i].BoundingSphere;
            first = false;
        }
        else
        {
            boundingBox.Merge(m_objectRecords[i].BoundingBox);
            boundingSphere.Merge(m_objectRecords[i].BoundingSphere);
        }
    }

    SetBounds(m_Transform.TransformBoundingBox(boundingBox), m_Transform.TransformBoundingSphere(boundingSphere));
}

void CompositeRenderProxy::QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    if (!m_Visibility)
        return;

    // Store the requested render passes.
    uint32 wantedRenderPasses = RENDER_PASSES_DEFAULT;
    if (!(m_ShadowFlags & ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS))
        wantedRenderPasses &= ~RENDER_PASS_SHADOW_MAP;
    if (!(m_ShadowFlags & ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS))
        wantedRenderPasses &= ~RENDER_PASS_SHADOWED_LIGHTING;

    // Add objects
    for (uint32 i = 0; i < m_objectRecords.GetSize(); i++)
    {
        const ObjectRecord &obj = m_objectRecords[i];
        if (obj.BatchCount == 0)
            continue;

        float viewDistance = pCamera->CalculateDepthToPoint(obj.BoundingSphere.GetCenter());
        uint32 renderPassMask = wantedRenderPasses;
        if (obj.TintEnabled)
            renderPassMask |= RENDER_PASS_TINT;

        for (uint32 j = 0; j < obj.BatchCount; j++)
        {
            uint32 materialIndex = obj.pBatches[j].MaterialIndex;
            DebugAssert(materialIndex < obj.MaterialCount);

            const Material *pMaterial = obj.ppMaterials[materialIndex];
            
            renderPassMask = pMaterial->GetShader()->SelectRenderPassMask(renderPassMask);
            if (renderPassMask != 0)
            {
                RENDER_QUEUE_RENDERABLE_ENTRY queueEntry;
                queueEntry.RenderPassMask = renderPassMask;
                queueEntry.pRenderProxy = this;
                queueEntry.BoundingBox = obj.BoundingBox;
                queueEntry.pVertexFactoryTypeInfo = obj.pVertexFactoryTypeInfo;
                queueEntry.VertexFactoryFlags = obj.VertexFactoryFlags;
                queueEntry.pMaterial = pMaterial;
                queueEntry.ViewDistance = viewDistance;
                queueEntry.UserData[0] = i;
                queueEntry.UserData[1] = j;
                queueEntry.TintColor = obj.TintColor;
                queueEntry.Layer = pMaterial->GetShader()->SelectRenderQueueLayer();
                pRenderQueue->AddRenderable(&queueEntry);
            }
        }
    }
}

void CompositeRenderProxy::SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const
{
    uint32 objectIndex = pQueueEntry->UserData[0];
    DebugAssert(objectIndex < m_objectRecords.GetSize());
    const ObjectRecord &obj = m_objectRecords[objectIndex];

    GPUBuffer *pVertexBuffer = obj.pVertexBuffer;
    uint32 vertexBufferOffset = 0;
    uint32 vertexBufferStride = obj.VertexStride;
    pGPUContext->SetVertexBuffers(0, 1, &pVertexBuffer, &vertexBufferOffset, &vertexBufferStride);
    pGPUContext->SetIndexBuffer(obj.pIndexBuffer, obj.IndexFormat, 0);

    pGPUContext->GetConstants()->SetLocalToWorldMatrix(m_LocalToWorldMatrix, true);
    pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);
}

void CompositeRenderProxy::DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const
{
    uint32 objectIndex = pQueueEntry->UserData[0];
    DebugAssert(objectIndex < m_objectRecords.GetSize());

    const ObjectRecord &obj = m_objectRecords[objectIndex];
    uint32 batchIndex = pQueueEntry->UserData[1];
    DebugAssert(batchIndex < obj.BatchCount);

    const Batch &batchInfo = obj.pBatches[batchIndex];
    pGPUContext->DrawIndexed(batchInfo.FirstIndex, batchInfo.IndexCount, 0);
}

bool CompositeRenderProxy::CreateDeviceResources() const
{
    if (m_bGPUResourcesCreated)
        return true;

    m_bGPUResourcesCreated = true;
    return true;
}

void CompositeRenderProxy::ReleaseDeviceResources() const
{
    for (uint32 i = 0; i < m_objectRecords.GetSize(); i++)
    {
        ObjectRecord &obj = const_cast<ObjectRecord &>(m_objectRecords[i]);
        SAFE_RELEASE(obj.pVertexBuffer);
        SAFE_RELEASE(obj.pIndexBuffer);
    }

    m_bGPUResourcesCreated = false;
}
