#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorBlockVolumeRenderProxy.h"
#include "Engine/BlockMeshBuilder.h"
#include "Engine/Camera.h"
#include "Renderer/VertexFactories/BlockMeshVertexFactory.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/Renderer.h"
Log_SetChannel(EditorBlockVolumeRenderProxy);

EditorBlockVolumeRenderProxy::EditorBlockVolumeRenderProxy(uint32 entityId)
    : RenderProxy(entityId),
      m_pPalette(nullptr),
      m_visibility(true),
      m_localToWorldMatrix(float4x4::Identity),
      m_shadowFlags(0),
      m_tintEnabled(false),
      m_tintColor(0xFFFFFFFF),
      m_localBoundingBox(AABox::Zero),
      m_localBoundingSphere(Sphere::Zero),
      m_vertexFactoryFlags(0),
      m_pIndexBuffer(NULL),
      m_indexFormat(GPU_INDEX_FORMAT_COUNT)
{
    SetBounds(AABox::Zero, Sphere::Zero);
}

EditorBlockVolumeRenderProxy::~EditorBlockVolumeRenderProxy()
{
    SAFE_RELEASE(m_pPalette);
    m_vertexBuffers.Clear();
    SAFE_RELEASE(m_pIndexBuffer);
}

void EditorBlockVolumeRenderProxy::QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    if (!m_visibility || m_batches.GetSize() == 0)
        return;

    uint32 wantedRenderPasses = RENDER_PASSES_DEFAULT;
    if (!(m_shadowFlags & ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS))
        wantedRenderPasses &= ~RENDER_PASS_SHADOW_MAP;
    if (!(m_shadowFlags & ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS))
        wantedRenderPasses &= ~RENDER_PASS_SHADOWED_LIGHTING;

    float viewDistance = pCamera->CalculateDepthToPoint(GetBoundingBox().GetCenter());

    for (uint32 i = 0; i < m_batches.GetSize(); i++)
    {
        const RenderBatch &batch = m_batches[i];

        // skip batches without shadows if drawing shadow map
        if (wantedRenderPasses == RENDER_PASS_SHADOW_MAP && !batch.DrawShadows)
            continue;

        const Material *pMaterial = m_pPalette->GetMaterial(batch.MaterialIndex);
        uint32 renderPassMask = pMaterial->GetShader()->SelectRenderPassMask(wantedRenderPasses);

        if (renderPassMask != 0)
        {
            RENDER_QUEUE_RENDERABLE_ENTRY queueEntry;
            queueEntry.RenderPassMask = renderPassMask;
            queueEntry.pRenderProxy = this;
            queueEntry.BoundingBox = GetBoundingBox();
            queueEntry.pVertexFactoryTypeInfo = VERTEX_FACTORY_TYPE_INFO(BlockMeshVertexFactory);
            queueEntry.VertexFactoryFlags = m_vertexFactoryFlags;
            queueEntry.pMaterial = pMaterial;
            queueEntry.ViewDistance = viewDistance;
            queueEntry.UserData[0] = i;
            queueEntry.TintColor = m_tintColor;
            pRenderQueue->AddRenderable(&queueEntry);
        }
    }
}

void EditorBlockVolumeRenderProxy::SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const
{
    pGPUContext->GetConstants()->SetLocalToWorldMatrix(m_localToWorldMatrix, true);
    pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);
    m_vertexBuffers.BindBuffers(pGPUContext);
    pGPUContext->SetIndexBuffer(m_pIndexBuffer, m_indexFormat, 0);
}

void EditorBlockVolumeRenderProxy::DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const
{
    DebugAssert(pQueueEntry->UserData[0] < m_batches.GetSize());
    const RenderBatch &batch = m_batches[pQueueEntry->UserData[0]];
    pGPUContext->DrawIndexed(batch.StartIndex, batch.IndexCount, 0);
}

bool EditorBlockVolumeRenderProxy::CreateDeviceResources() const
{
    if (m_pPalette != nullptr && !m_pPalette->CreateGPUResources())
        return false;

    return true;
}

void EditorBlockVolumeRenderProxy::SetTransform(const float4x4 &newLocalToWorldMatrix)
{
    m_localToWorldMatrix = newLocalToWorldMatrix;
    SetBounds(m_localBoundingBox.GetTransformed(newLocalToWorldMatrix), m_localBoundingSphere.GetTransformed(newLocalToWorldMatrix));
}

void EditorBlockVolumeRenderProxy::SetTintColor(bool enabled, uint32 color /*= 0*/)
{
    m_tintEnabled = enabled;
    m_tintColor = (enabled) ? color : 0xFFFFFFFF;
}

void EditorBlockVolumeRenderProxy::SetVisibility(bool visible)
{
    m_visibility = visible;
}

void EditorBlockVolumeRenderProxy::SetShadowFlags(uint32 shadowFlags)
{
    m_shadowFlags = shadowFlags;
}

void EditorBlockVolumeRenderProxy::BuildMesh(const BlockMeshVolume *pVolume, bool useAmbientOcclusion)
{
    static const uint32 EXTRA_VERTEX_FACTORY_FLAGS = 0;

    // clear current mesh
    ClearMesh();

    // create mesh
    BlockMeshBuilder builder;
    builder.SetPalette(pVolume->GetPalette());
    builder.SetFromVolume(pVolume);
    builder.SetAmbientOcclusionEnabled(useAmbientOcclusion);
    builder.GenerateMesh();

    // no triangles?
    if (builder.GetOutputBatchCount() == 0)
        return;

    // create buffers
    if (!builder.CreateGPUBuffers(&m_vertexBuffers, &m_vertexFactoryFlags, &m_pIndexBuffer, &m_indexFormat, EXTRA_VERTEX_FACTORY_FLAGS))
        return;

    // fix up palette ref
    m_pPalette = pVolume->GetPalette();
    m_pPalette->AddRef();

    // copy batches
    for (uint32 i = 0; i < builder.GetOutputBatchCount(); i++)
    {
        const BlockMeshBuilder::Batch &sourceBatch = builder.GetOutputBatches().GetElement(i);
        m_batches.Add(RenderBatch(sourceBatch.MaterialIndex, sourceBatch.StartIndex, sourceBatch.NumIndices, sourceBatch.DrawShadows));
    }

    // fix up bounds
    m_localBoundingBox = builder.GetOutputBoundingBox();
    m_localBoundingSphere = builder.GetOutputBoundingSphere();
    SetBounds(m_localBoundingBox.GetTransformed(m_localToWorldMatrix), m_localBoundingSphere.GetTransformed(m_localToWorldMatrix));
}

void EditorBlockVolumeRenderProxy::ClearMesh()
{
    // release old buffers
    SAFE_RELEASE(m_pPalette);
    m_vertexBuffers.Clear();
    SAFE_RELEASE(m_pIndexBuffer);
    m_indexFormat = GPU_INDEX_FORMAT_COUNT;
    m_batches.Clear();

    // fix up bounds
    m_localBoundingBox = AABox::Zero;
    m_localBoundingSphere = Sphere::Zero;
    SetBounds(AABox::Zero, Sphere::Zero);
}

