#pragma once
#include "Editor/Common.h"
#include "Engine/BlockMeshVolume.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Renderer/RendererTypes.h"

class BlockPalette;
class BlockMeshVertexFactory;
class EditorTemporaryBlockMeshEntity;

class EditorBlockVolumeRenderProxy : public RenderProxy
{
public:
    EditorBlockVolumeRenderProxy(uint32 entityId);
    ~EditorBlockVolumeRenderProxy();

    void BuildMesh(const BlockMeshVolume *pVolume, bool useAmbientOcclusion);
    void ClearMesh();

    void SetTransform(const float4x4 &newLocalToWorldMatrix);
    void SetShadowFlags(uint32 shadowFlags);
    void SetTintColor(bool enabled, uint32 color = 0);
    void SetVisibility(bool visible);

    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override;
    virtual void SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList, ShaderProgram *pShaderProgram) const override;
    virtual void DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList) const override;
    virtual bool CreateDeviceResources() const override;

protected:
    // batch info
    struct RenderBatch
    {
        uint32 MaterialIndex;
        uint32 StartIndex;
        uint32 IndexCount;
        bool DrawShadows;

        RenderBatch(uint32 materialIndex, uint32 startIndex, uint32 indexCount, bool drawShadows) : MaterialIndex(materialIndex), StartIndex(startIndex), IndexCount(indexCount), DrawShadows(drawShadows) {}
    };

    // maintain a reference to the block list
    mutable const BlockPalette *m_pPalette;
    bool m_visibility;
    float4x4 m_localToWorldMatrix;
    uint32 m_shadowFlags;
    bool m_tintEnabled;
    uint32 m_tintColor;

    // bounding box in local space
    AABox m_localBoundingBox;
    Sphere m_localBoundingSphere;

    // we don't keep a copy of the triangles in memory, we store them only on the gpu
    VertexBufferBindingArray m_vertexBuffers;
    uint32 m_vertexFactoryFlags;
    GPUBuffer *m_pIndexBuffer;
    GPU_INDEX_FORMAT m_indexFormat;
    MemArray<RenderBatch> m_batches;
};

