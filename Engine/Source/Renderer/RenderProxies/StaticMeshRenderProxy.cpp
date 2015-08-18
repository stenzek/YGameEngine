#include "Renderer/PrecompiledHeader.h"
#include "Renderer/RenderProxies/StaticMeshRenderProxy.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/Renderer.h"
#include "Engine/Camera.h"
#include "Engine/Material.h"

StaticMeshRenderProxy::StaticMeshRenderProxy(uint32 entityId, const StaticMesh *pStaticMesh, const Transform &transform, uint32 shadowFlags)
    : RenderProxy(entityId),
      m_visibility(true),
      m_pStaticMesh(nullptr),
      m_shadowFlags(shadowFlags),
      m_tintEnabled(false),
      m_tintColor(0xFFFFFFFF),
      m_bGPUResourcesCreated(false)
{
    uint32 i;
    DebugAssert(pStaticMesh != nullptr);

    m_pStaticMesh = pStaticMesh;
    m_pStaticMesh->AddRef();

    // allocate materials
    m_materials.Resize(pStaticMesh->GetMaterialCount());
    for (i = 0; i < pStaticMesh->GetMaterialCount(); i++)
    {
        m_materials[i] = pStaticMesh->GetMaterial(i);
        DebugAssert(m_materials[i] != nullptr);
        m_materials[i]->AddRef();
    }

    // update bounds
    SetTransform(transform);
}

StaticMeshRenderProxy::~StaticMeshRenderProxy()
{
    uint32 i;
    ReleaseDeviceResources();

    m_pStaticMesh->Release();
    for (i = 0; i < m_materials.GetSize(); i++)
        m_materials[i]->Release();
}

void StaticMeshRenderProxy::SetStaticMesh(const StaticMesh *pStaticMesh)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetStaticMesh(pStaticMesh);
    }
    else
    {
        ReferenceCountedHolder<StaticMeshRenderProxy> pThisHolder(this);
        ReferenceCountedHolder<const StaticMesh> pStaticMeshHolder(pStaticMesh);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, pStaticMeshHolder]() {
            pThisHolder->RealSetStaticMesh(pStaticMeshHolder);
        });
    }
}

void StaticMeshRenderProxy::RealSetStaticMesh(const StaticMesh *pStaticMesh)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());
    DebugAssert(pStaticMesh != nullptr);
    if (m_pStaticMesh == pStaticMesh)
        return;

    // not yet owned by render thread, so no need to do async modifications
    ReleaseDeviceResources();

    // release materials
    for (uint32 i = 0; i < m_materials.GetSize(); i++)
    {
        m_materials[i]->Release();
        m_materials[i] = NULL;
    }

    // release mesh
    m_pStaticMesh->Release();

    // set new mesh
    m_pStaticMesh = pStaticMesh;
    m_pStaticMesh->AddRef();

    // reallocate materials
    m_materials.Resize(m_pStaticMesh->GetMaterialCount());
    m_materials.Shrink();
    for (uint32 i = 0; i < m_pStaticMesh->GetMaterialCount(); i++)
    {
        m_materials[i] = m_pStaticMesh->GetMaterial(i);
        DebugAssert(m_materials[i] != nullptr);
        m_materials[i]->AddRef();
    }

    // fix up bounds
    SetBounds(m_transform.TransformBoundingBox(m_pStaticMesh->GetBoundingBox()), m_transform.TransformBoundingSphere(m_pStaticMesh->GetBoundingSphere()));
}

void StaticMeshRenderProxy::SetMaterial(uint32 i, const Material *pMaterialOverride)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetMaterial(i, pMaterialOverride);
    }
    else
    {
        ReferenceCountedHolder<StaticMeshRenderProxy> pThisHolder(this);
        ReferenceCountedHolder<const Material> pMaterialHolder(pMaterialOverride);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, i, pMaterialHolder]() {
            pThisHolder->RealSetMaterial(i, pMaterialHolder);
        });
    }
}

void StaticMeshRenderProxy::RealSetMaterial(uint32 i, const Material *pMaterialOverride)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());
    DebugAssert(pMaterialOverride != nullptr);

    // not yet owned by render thread, so no need to do async modifications
    DebugAssert(i < m_materials.GetSize());
    m_materials[i]->Release();
    m_materials[i] = pMaterialOverride;
    m_materials[i]->AddRef();
}

void StaticMeshRenderProxy::SetTransform(const Transform &transform)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetTransform(transform);
    }
    else
    {
        ReferenceCountedHolder<StaticMeshRenderProxy> pThisHolder(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, transform]() {
            pThisHolder->RealSetTransform(transform);
        });
    }
}

void StaticMeshRenderProxy::RealSetTransform(const Transform &transform)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_transform = transform;
    m_localToWorldMatrix = m_transform.GetTransformMatrix4x4();
    SetBounds(m_transform.TransformBoundingBox(m_pStaticMesh->GetBoundingBox()), m_transform.TransformBoundingSphere(m_pStaticMesh->GetBoundingSphere()));
}

void StaticMeshRenderProxy::SetTintColor(bool enabled, uint32 color /* = 0 */)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetTintColor(enabled, color);
    }
    else
    {
        ReferenceCountedHolder<StaticMeshRenderProxy> pThisHolder(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, enabled, color]() {
            pThisHolder->RealSetTintColor(enabled, color);
        });
    }
}

void StaticMeshRenderProxy::RealSetTintColor(bool enabled, uint32 color /*= 0*/)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_tintEnabled = enabled;
    m_tintColor = (enabled) ? color : 0xFFFFFFFF;
}

void StaticMeshRenderProxy::SetVisibility(bool visible)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetVisibility(visible);
    }
    else
    {
        ReferenceCountedHolder<StaticMeshRenderProxy> pThisHolder(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, visible]() {
            pThisHolder->RealSetVisibility(visible);
        });
    }
}

void StaticMeshRenderProxy::RealSetVisibility(bool visible)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_visibility = visible;
}

void StaticMeshRenderProxy::SetShadowFlags(uint32 shadowFlags)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetShadowFlags(shadowFlags);
    }
    else
    {
        ReferenceCountedHolder<StaticMeshRenderProxy> pThisHolder(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, shadowFlags]() {
            pThisHolder->RealSetShadowFlags(shadowFlags);
        });
    }
}

void StaticMeshRenderProxy::RealSetShadowFlags(uint32 shadowFlags)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_shadowFlags = shadowFlags;
}

void StaticMeshRenderProxy::QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    if (!m_visibility)
        return;

    // check gpu resources
    if (!m_bGPUResourcesCreated && !CreateDeviceResources())
        return;

    // Select the LOD index we are drawing for.
    uint32 selectedLOD = 0;
    DebugAssert(selectedLOD < m_pStaticMesh->GetLODCount());

    // Store the requested render passes.
    uint32 wantedRenderPasses = RENDER_PASSES_DEFAULT;
    if (!(m_shadowFlags & ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS))
        wantedRenderPasses &= ~RENDER_PASS_SHADOW_MAP;
    if (!(m_shadowFlags & ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS))
        wantedRenderPasses &= ~RENDER_PASS_SHADOWED_LIGHTING;
//     if (hasLightMaps && (availableRenderPassMask & RENDER_PASS_LIGHTMAP))
//         wantedRenderPasses = (wantedRenderPasses & ~(RENDER_PASS_STATIC_OPAQUE_LIGHTING | RENDER_PASS_STATIC_TRANSPARENT_LIGHTING)) | RENDER_PASS_LIGHTMAP;
    if (m_tintEnabled)
        wantedRenderPasses |= RENDER_PASS_TINT;

    // skip for no passes
    if ((pRenderQueue->GetAcceptingRenderPassMask() & wantedRenderPasses) == 0)
        return;

    // Calculate view distance
    float viewDistance = pCamera->CalculateDepthToPoint(GetBoundingSphere().GetCenter());

    // Draw this LOD.
    const StaticMesh::LOD *pLOD = m_pStaticMesh->GetLOD(selectedLOD);
    for (uint32 i = 0; i < pLOD->GetBatchCount(); i++)
    {
        // Get batch info.
        const StaticMesh::Batch *pBatch = pLOD->GetBatch(i);
        
        // Get material.
        const Material *pMaterial = m_materials[pBatch->MaterialIndex];

        // Determine render passes. There isn't really any passes that a static mesh can't handle, so we only remove the material ones.
        uint32 renderPassMask = pMaterial->GetShader()->SelectRenderPassMask(wantedRenderPasses);

        // Add to render queue if we have any render passes.
        if (renderPassMask != 0)
        {
            RENDER_QUEUE_RENDERABLE_ENTRY queueEntry;
            queueEntry.RenderPassMask = renderPassMask;
            queueEntry.pRenderProxy = this;
            queueEntry.BoundingBox = GetBoundingBox();
            queueEntry.pVertexFactoryTypeInfo = VERTEX_FACTORY_TYPE_INFO(LocalVertexFactory);
            queueEntry.VertexFactoryFlags = m_pStaticMesh->GetVertexFactoryFlags();
            queueEntry.pMaterial = pMaterial;
            queueEntry.ViewDistance = viewDistance;
            queueEntry.UserData[0] = selectedLOD;
            queueEntry.UserData[1] = i;
            queueEntry.TintColor = m_tintColor;
            queueEntry.Layer = pMaterial->GetShader()->SelectRenderQueueLayer();
            pRenderQueue->AddRenderable(&queueEntry);
        }
    }
}

void StaticMeshRenderProxy::SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const
{
    uint32 lodIndex = pQueueEntry->UserData[0];
    const StaticMesh::LOD *pLOD = m_pStaticMesh->GetLOD(lodIndex);

    pGPUContext->GetConstants()->SetLocalToWorldMatrix(m_localToWorldMatrix, true);
    pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);

    pLOD->GetVertexBuffers()->BindBuffers(pGPUContext);
    pGPUContext->SetIndexBuffer(pLOD->GetIndexBuffer(), pLOD->GetIndexFormat(), 0);
}

void StaticMeshRenderProxy::DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const
{
    uint32 lodIndex = pQueueEntry->UserData[0];
    uint32 batchIndex = pQueueEntry->UserData[1];
    const StaticMesh::Batch *pBatch = m_pStaticMesh->GetLOD(lodIndex)->GetBatch(batchIndex);

    pGPUContext->DrawIndexed(pBatch->StartIndex, pBatch->NumIndices, 0);
}

bool StaticMeshRenderProxy::CreateDeviceResources() const
{
    uint32 i;
    if (m_bGPUResourcesCreated)
        return true;

    if (!m_pStaticMesh->CreateGPUResources())
        return false;

    for (i = 0; i < m_materials.GetSize(); i++)
    {
        if (!m_materials[i]->CreateDeviceResources())
            return false;
    }

    //if (m_VertexBuffers.GetInputLayout() == nullptr)
        //m_VertexBuffers.SetInputLayout(m_pStaticMesh->GetVertexBuffers()->GetInputLayout());

    //if (m_VertexBuffers.GetBuffer(0) == nullptr)
        //LocalVertexFactory::ShareVerticesBuffer(&m_VertexBuffers, m_pStaticMesh->GetVertexBuffers());

    m_bGPUResourcesCreated = true;
    return true;
}

void StaticMeshRenderProxy::ReleaseDeviceResources() const
{
    //m_VertexBuffers.Clear();
    m_bGPUResourcesCreated = false;
}

