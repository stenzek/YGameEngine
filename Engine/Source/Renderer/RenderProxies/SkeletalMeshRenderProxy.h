#pragma once
#include "Renderer/RenderProxy.h"
#include "Engine/SkeletalMesh.h"

class SkeletalMeshRenderProxy : public RenderProxy
{
public:
    SkeletalMeshRenderProxy(uint32 entityID, const SkeletalMesh *pSkeletalMesh, const Transform &transform, uint32 shadowFlags);
    ~SkeletalMeshRenderProxy();

    const SkeletalMesh *GetSkeletalMesh() const { return m_pSkeletalMesh; }

    void SetSkeletalMesh(const SkeletalMesh *pSkeletalMesh);
    void SetMaterial(uint32 i, const Material *pMaterialOverride);
    void SetTransform(const Transform &transform);
    void SetTintColor(bool enabled, uint32 color = 0);
    void SetShadowFlags(uint32 shadowFlags);
    void SetVisibility(bool visible);

    // animation
    void SetBoneTransforms(uint32 firstTransform, uint32 transformCount, const float3x4 *pTransforms);
    void SetBoneTransforms(uint32 firstTransform, uint32 transformCount, const Transform *pTransforms);
    void ResetToBaseFrameTransform();

    // render proxy stuff
    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override;
    virtual void SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const override;
    virtual void DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const override;
    virtual void DrawDebugInfo(const Camera *pCamera, GPUContext *pGPUContext, MiniGUIContext *pGUIContext) const override;
    virtual bool CreateDeviceResources() const override;
    virtual void ReleaseDeviceResources() const override;    

private:
    // real methods
    void RealSetSkeletalMesh(const SkeletalMesh *pSkeletalMesh);
    void RealSetMaterial(uint32 i, const Material *pMaterialOverride);
    void RealSetTransform(const Transform &transform);
    void RealSetTintColor(bool enabled, uint32 color = 0);
    void RealSetShadowFlags(uint32 shadowFlags);
    void RealSetVisibility(bool visible);

    // initialize the bone transform array
    void InitializeBoneTransformArray();

    // cpu skinning
    void TransformVerticesOnCPU();

    bool m_visibility;
    const SkeletalMesh *m_pSkeletalMesh;
    PODArray<const Material *> m_materials;
    Transform m_transform;
    float4x4 m_localToWorldMatrix;
    uint32 m_shadowFlags;
    bool m_tintEnabled;
    uint32 m_tintColor;

    // gpu resources, also owned by render thread.
    mutable bool m_bGPUResourcesCreated;
    mutable VertexBufferBindingArray m_VertexBuffers;
    MemArray<float3x4> m_boneTransforms;

    // cpu skinning
    mutable MemArray<SkeletalMeshVertexFactory::Vertex> m_cpuSkinnedVertices;
    mutable GPUBuffer *m_pCPUSkinningVertexBuffer;
    mutable bool m_useGPUSkinning;
};
