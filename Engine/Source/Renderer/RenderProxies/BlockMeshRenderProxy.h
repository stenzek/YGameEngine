#pragma once
#include "Renderer/Common.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Engine/BlockMesh.h"

class Material;

class BlockMeshRenderProxy : public RenderProxy
{
public:
    BlockMeshRenderProxy(uint32 entityId, const BlockMesh *pStaticBlockMesh, const Transform &transform, uint32 shadowFlags);
    ~BlockMeshRenderProxy();

    // update functions, can be called from game thread safely after adding to render world.
    void SetBlockMesh(const BlockMesh *pStaticBlockMesh);
    void SetTransform(const Transform &transform);
    void SetTintColor(bool enabled, uint32 color = 0);
    void SetShadowFlags(uint32 shadowFlags);
    void SetVisibility(bool visible);

    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override;
    virtual void SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const override;
    virtual void DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const override;
    virtual bool CreateDeviceResources() const override;
    virtual void ReleaseDeviceResources() const override;

private:
    void RealSetBlockMesh(const BlockMesh *pStaticBlockMesh);
    void RealSetTransform(const Transform &transform);
    void RealSetTintColor(bool enabled, uint32 color = 0);
    void RealSetShadowFlags(uint32 shadowFlags);
    void RealSetVisibility(bool visible);

    // read/write from render thread. no access from game thread.
    bool m_visibility;
    const BlockMesh *m_pBlockMesh;
    Transform m_transform;
    float4x4 m_localToWorldMatrix;
    uint32 m_shadowFlags;
    bool m_tintEnabled;
    uint32 m_tintColor;

    // gpu resources, also owned by render thread.
    mutable bool m_bGPUResourcesCreated;
    //mutable VertexBufferBindingArray m_VertexBuffers;
};

