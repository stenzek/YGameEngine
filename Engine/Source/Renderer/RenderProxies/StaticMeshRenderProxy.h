#pragma once
#include "Renderer/Common.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Engine/StaticMesh.h"

class Material;

class StaticMeshRenderProxy : public RenderProxy
{
public:
    StaticMeshRenderProxy(uint32 entityId, const StaticMesh *pStaticMesh, const Transform &transform, uint32 shadowFlags);
    ~StaticMeshRenderProxy();

    // update functions
    void SetStaticMesh(const StaticMesh *pStaticMesh);
    void SetMaterial(uint32 i, const Material *pMaterialOverride);
    void SetTransform(const Transform &transform);
    void SetTintColor(bool enabled, uint32 color = 0);
    void SetShadowFlags(uint32 shadowFlags);
    void SetVisibility(bool visible);

    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override;
    virtual void SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList, ShaderProgram *pShaderProgram) const override;
    virtual void DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList) const override;
    virtual bool CreateDeviceResources() const override;
    virtual void ReleaseDeviceResources() const override;

private:
    // real methods
    void RealSetStaticMesh(const StaticMesh *pStaticMesh);
    void RealSetMaterial(uint32 i, const Material *pMaterialOverride);
    void RealSetTransform(const Transform &transform);
    void RealSetTintColor(bool enabled, uint32 color = 0);
    void RealSetShadowFlags(uint32 shadowFlags);
    void RealSetVisibility(bool visible);

    // read from render thread at async time, write from game thread at sync time
    bool m_visibility;
    const StaticMesh *m_pStaticMesh;
    PODArray<const Material *> m_materials;
    Transform m_transform;
    float4x4 m_localToWorldMatrix;
    uint32 m_shadowFlags;
    bool m_tintEnabled;
    uint32 m_tintColor;

    // gpu resources, also owned by render thread.
    mutable bool m_bGPUResourcesCreated;
    //mutable VertexBufferBindingArray m_VertexBuffers;
};

