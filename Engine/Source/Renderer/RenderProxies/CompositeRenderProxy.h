#pragma once
#include "Renderer/Common.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Renderer/RendererTypes.h"
#include "Engine/BlockMesh.h"

class Material;

class CompositeRenderProxy : public RenderProxy
{
public:
    struct Batch
    {
        uint32 MaterialIndex;
        uint32 FirstIndex;
        uint32 IndexCount;
    };

public:
    CompositeRenderProxy(uint32 entityId, const Transform &transform);
    ~CompositeRenderProxy();

    // manage objects
    uint32 AddObject();
    void UpdateObject(uint32 objectId, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const void *pVertices, uint32 vertexSize, uint32 vertexCount, const void *pIndices, GPU_INDEX_FORMAT indexFormat, uint32 indexCount, const Material **ppMaterials, uint32 materialCount, const Batch *pBatches, uint32 batchCount, const AABox &boundingBox, const Sphere &boundingSphere);
    void UpdateObjectTintColor(uint32 objectId, bool tintEnabled, uint32 tintColor);
    void ClearObject(uint32 objectId);
    void DeleteObject(uint32 objectId);
    void DeleteAllObjects();

    // update functions, can be called from game thread safely after adding to render world.
    void SetTransform(const Transform &transform);
    void SetTintColor(bool enabled, uint32 color = 0);
    void SetShadowFlags(uint32 shadowFlags);
    void SetVisibility(bool visible);

    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override;
    virtual void SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const override;
    virtual void DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const override;
    virtual bool CreateDeviceResources() const override;
    virtual void ReleaseDeviceResources() const override;
    
    void UpdateObjects();

private:
    enum UPDATE_OBJECT_FLAGS
    {
        UPDATE_OBJECT_FLAG_CREATE                   = (1 << 0),
        UPDATE_OBJECT_FLAG_BUFFERS                  = (1 << 1),
        UPDATE_OBJECT_FLAG_TINT                     = (1 << 2),
        UPDATE_OBJECT_FLAG_DELETE                   = (1 << 3),
    };

    struct ObjectRecord
    {
        uint32 ObjectId;
        const VertexFactoryTypeInfo *pVertexFactoryTypeInfo;
        uint32 VertexFactoryFlags;
        GPUBuffer *pVertexBuffer;
        uint32 VertexStride;
        GPUBuffer *pIndexBuffer;
        GPU_INDEX_FORMAT IndexFormat;
        const Material **ppMaterials;
        uint32 MaterialCount;
        const Batch *pBatches;
        uint32 BatchCount;
        bool TintEnabled;
        uint32 TintColor;
        AABox BoundingBox;
        Sphere BoundingSphere;
    };

    struct UpdateObjectRecord
    {
        uint32 ObjectId;
        uint32 UpdateFlags;
        const VertexFactoryTypeInfo *pVertexFactoryTypeInfo;
        uint32 VertexFactoryFlags;
        void *pVertices;
        uint32 VertexSize;
        uint32 VertexCount;
        void *pIndices;
        GPU_INDEX_FORMAT IndexFormat;
        uint32 IndexCount;
        const Material **ppMaterials;
        uint32 MaterialCount;
        Batch *pBatches;
        uint32 BatchCount;
        bool TintEnabled;
        uint32 TintColor;
        AABox BoundingBox;
        Sphere BoundingSphere;
    };

    // sets the synchronization bit specified
    void SetSynchronizationBits(uint32 synchronizationBits);

    // updates objects, call from render thread
    void UpdateBounds();

    // read/write from render thread. no access from game thread.
    bool m_Visibility;
    Transform m_Transform;
    float4x4 m_LocalToWorldMatrix;
    uint32 m_ShadowFlags;
    MemArray<ObjectRecord> m_objectRecords;
    uint32 m_nextObjectId;

    // gpu resources, also owned by render thread.
    mutable bool m_bGPUResourcesCreated;

    // read from render thread. write from game thread.
    MemArray<UpdateObjectRecord> m_updateObjectRecords;
};

