#include "Renderer/PrecompiledHeader.h"
#include "Renderer/RenderProxies/BlockMeshRenderProxy.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/Renderer.h"
#include "Engine/Camera.h"
#include "Engine/Material.h"

BlockMeshRenderProxy::BlockMeshRenderProxy(uint32 entityId, const BlockMesh *pStaticBlockMesh, const Transform &transform, uint32 shadowFlags)
    : RenderProxy(entityId),
      m_visibility(true),
      m_pBlockMesh(NULL),
      m_shadowFlags(shadowFlags),
      m_tintEnabled(false),
      m_tintColor(0xFFFFFFFF),
      m_bGPUResourcesCreated(false)
{
    DebugAssert(pStaticBlockMesh != NULL);

    m_pBlockMesh = pStaticBlockMesh;
    m_pBlockMesh->AddRef();

    SetTransform(transform);
}

BlockMeshRenderProxy::~BlockMeshRenderProxy()
{
    ReleaseDeviceResources();

    m_pBlockMesh->Release();
}

void BlockMeshRenderProxy::SetBlockMesh(const BlockMesh *pBlockMesh)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetBlockMesh(pBlockMesh);
    }
    else
    {
        ReferenceCountedHolder<BlockMeshRenderProxy> pThisHolder(this);
        ReferenceCountedHolder<const BlockMesh> pBlockMeshHolder(pBlockMesh);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, pBlockMeshHolder]() {
            pThisHolder->RealSetBlockMesh(pBlockMeshHolder);
        });
    }
}

void BlockMeshRenderProxy::RealSetBlockMesh(const BlockMesh *pBlockMesh)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());
    DebugAssert(pBlockMesh != nullptr);
    if (m_pBlockMesh == pBlockMesh)
        return;

    // not yet owned by render thread, so no need to do async modifications
    ReleaseDeviceResources();
    
    // release mesh
    m_pBlockMesh->Release();

    // set new mesh
    m_pBlockMesh = pBlockMesh;
    m_pBlockMesh->AddRef();
    
    // fix up bounds
    SetBounds(m_transform.TransformBoundingBox(m_pBlockMesh->GetBoundingBox()), m_transform.TransformBoundingSphere(m_pBlockMesh->GetBoundingSphere()));
}

void BlockMeshRenderProxy::SetTransform(const Transform &transform)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetTransform(transform);
    }
    else
    {
        ReferenceCountedHolder<BlockMeshRenderProxy> pThisHolder(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, transform]() {
            pThisHolder->RealSetTransform(transform);
        });
    }
}

void BlockMeshRenderProxy::RealSetTransform(const Transform &transform)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_transform = transform;
    m_localToWorldMatrix = m_transform.GetTransformMatrix4x4();
    SetBounds(m_transform.TransformBoundingBox(m_pBlockMesh->GetBoundingBox()), m_transform.TransformBoundingSphere(m_pBlockMesh->GetBoundingSphere()));
}

void BlockMeshRenderProxy::SetTintColor(bool enabled, uint32 color /* = 0 */)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetTintColor(enabled, color);
    }
    else
    {
        ReferenceCountedHolder<BlockMeshRenderProxy> pThisHolder(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, enabled, color]() {
            pThisHolder->RealSetTintColor(enabled, color);
        });
    }
}

void BlockMeshRenderProxy::RealSetTintColor(bool enabled, uint32 color /*= 0*/)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_tintEnabled = enabled;
    m_tintColor = (enabled) ? color : 0xFFFFFFFF;
}

void BlockMeshRenderProxy::SetVisibility(bool visible)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetVisibility(visible);
    }
    else
    {
        ReferenceCountedHolder<BlockMeshRenderProxy> pThisHolder(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, visible]() {
            pThisHolder->RealSetVisibility(visible);
        });
    }
}

void BlockMeshRenderProxy::RealSetVisibility(bool visible)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_visibility = visible;
}

void BlockMeshRenderProxy::SetShadowFlags(uint32 shadowFlags)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetShadowFlags(shadowFlags);
    }
    else
    {
        ReferenceCountedHolder<BlockMeshRenderProxy> pThisHolder(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, shadowFlags]() {
            pThisHolder->RealSetShadowFlags(shadowFlags);
        });
    }
}

void BlockMeshRenderProxy::RealSetShadowFlags(uint32 shadowFlags)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_shadowFlags = shadowFlags;
}

void BlockMeshRenderProxy::QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    uint32 i;
    if (!m_visibility)
        return;

    // check gpu resources
    if (!m_bGPUResourcesCreated && !CreateDeviceResources())
        return;

    // Store the requested render passes.
    uint32 wantedRenderPasses = RENDER_PASSES_DEFAULT;
    if (m_shadowFlags & ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS)
        wantedRenderPasses |= RENDER_PASS_SHADOW_MAP;
    if (!(m_shadowFlags & ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS))
        wantedRenderPasses &= ~RENDER_PASS_SHADOWED_LIGHTING;
//     if (hasLightMaps && (availableRenderPassMask & RENDER_PASS_LIGHTMAP))
//         wantedRenderPasses = (wantedRenderPasses & ~(RENDER_PASS_STATIC_OPAQUE_LIGHTING | RENDER_PASS_STATIC_TRANSPARENT_LIGHTING)) | RENDER_PASS_LIGHTMAP;

    // Calculate view distance
    float viewDistance = pCamera->CalculateDepthToPoint(GetBoundingSphere().GetCenter());
    uint32 selectedLOD = 0;

    // Draw this LOD.
    DebugAssert(selectedLOD < m_pBlockMesh->GetLODCount());
    for (i = 0; i < m_pBlockMesh->GetBatchCount(selectedLOD); i++)
    {
        // Get batch info.
        const BlockMesh::Batch *pBatch = m_pBlockMesh->GetBatch(selectedLOD, i);
        
        // Get material.
        const Material *pMaterial = m_pBlockMesh->GetBlockList()->GetMaterial(pBatch->MaterialIndex);

        // Determine render passes. There isn't really any passes that a static mesh can't handle, so we only remove the material ones.
        uint32 renderPassMask = pMaterial->GetShader()->SelectRenderPassMask(wantedRenderPasses);

        // Add to render queue if we have any render passes.
        if (renderPassMask != 0)
        {
            RENDER_QUEUE_RENDERABLE_ENTRY queueEntry;
            queueEntry.RenderPassMask = renderPassMask;
            queueEntry.pRenderProxy = this;
            queueEntry.BoundingBox = GetBoundingBox();
            queueEntry.pVertexFactoryTypeInfo = VERTEX_FACTORY_TYPE_INFO(BlockMeshVertexFactory);
            queueEntry.VertexFactoryFlags = m_pBlockMesh->GetVertexFactoryFlags();
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

void BlockMeshRenderProxy::SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const
{
    uint32 selectedLOD = pQueueEntry->UserData[0];

    pGPUContext->GetConstants()->SetLocalToWorldMatrix(m_localToWorldMatrix, true);
    pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);
    m_pBlockMesh->GetVertexBuffers(selectedLOD)->BindBuffers(pGPUContext);
    pGPUContext->SetIndexBuffer(m_pBlockMesh->GetIndexBuffer(selectedLOD), m_pBlockMesh->GetIndexFormat(selectedLOD), 0);
}

void BlockMeshRenderProxy::DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const
{
    uint32 selectedLOD = pQueueEntry->UserData[0];
    uint32 batchIndex = pQueueEntry->UserData[1];
    const BlockMesh::Batch *pBatch = m_pBlockMesh->GetBatch(selectedLOD, batchIndex);
    pGPUContext->DrawIndexed(pBatch->StartIndex, pBatch->IndexCount, 0);
}

bool BlockMeshRenderProxy::CreateDeviceResources() const
{
    if (m_bGPUResourcesCreated)
        return true;

    if (!m_pBlockMesh->CreateGPUResources())
        return false;

    //if (m_VertexBuffers.GetBuffer(0) == NULL)
        //BlockMeshVertexFactory::ShareVerticesBuffer(&m_VertexBuffers, m_pStaticBlockMesh->GetVertexBuffers());

    m_bGPUResourcesCreated = true;
    return true;
}

void BlockMeshRenderProxy::ReleaseDeviceResources() const
{
    //m_VertexBuffers.ClearBuffers();
    m_bGPUResourcesCreated = false;
}
