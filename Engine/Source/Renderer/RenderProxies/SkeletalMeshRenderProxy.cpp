#include "Renderer/PrecompiledHeader.h"
#include "Renderer/RenderProxies/SkeletalMeshRenderProxy.h"
#include "Renderer/VertexFactories/SkeletalMeshVertexFactory.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/Renderer.h"
#include "Engine/EngineCVars.h"
#include "Engine/Camera.h"
#include "Engine/Material.h"

SkeletalMeshRenderProxy::SkeletalMeshRenderProxy(uint32 entityId, const SkeletalMesh *pSkeletalMesh, const Transform &transform, uint32 shadowFlags)
    : RenderProxy(entityId),
      m_visibility(true),
      m_pSkeletalMesh(NULL),
      m_shadowFlags(shadowFlags),
      m_tintEnabled(false),
      m_tintColor(0xFFFFFFFF),
      m_bGPUResourcesCreated(false),
      m_pCPUSkinningVertexBuffer(nullptr),
      m_useGPUSkinning(CVars::r_gpu_skinning.GetBool())
{
    DebugAssert(pSkeletalMesh != NULL);

    m_pSkeletalMesh = pSkeletalMesh;
    m_pSkeletalMesh->AddRef();

    // allocate materials
    m_materials.Resize(pSkeletalMesh->GetMaterialCount());
    for (uint32 i = 0; i < pSkeletalMesh->GetMaterialCount(); i++)
    {
        m_materials[i] = pSkeletalMesh->GetMaterial(i);
        DebugAssert(m_materials[i] != NULL);
        m_materials[i]->AddRef();
    }

    // update bounds
    SetTransform(transform);

    // init bone matrices
    InitializeBoneTransformArray();
}

SkeletalMeshRenderProxy::~SkeletalMeshRenderProxy()
{
    ReleaseDeviceResources();

    m_pSkeletalMesh->Release();
    for (uint32 i = 0; i < m_materials.GetSize(); i++)
        m_materials[i]->Release();
}

void SkeletalMeshRenderProxy::SetSkeletalMesh(const SkeletalMesh *pSkeletalMesh)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetSkeletalMesh(pSkeletalMesh);
    }
    else
    {
        ReferenceCountedHolder<SkeletalMeshRenderProxy> pThisHolder(this);
        ReferenceCountedHolder<const SkeletalMesh> pSkeletalMeshHolder(pSkeletalMesh);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, pSkeletalMeshHolder]() {
            pThisHolder->RealSetSkeletalMesh(pSkeletalMeshHolder);
        });
    }
}

void SkeletalMeshRenderProxy::RealSetSkeletalMesh(const SkeletalMesh *pSkeletalMesh)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());
    DebugAssert(pSkeletalMesh != nullptr);
    if (m_pSkeletalMesh == pSkeletalMesh)
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
    m_pSkeletalMesh->Release();

    // set new mesh
    m_pSkeletalMesh = pSkeletalMesh;
    m_pSkeletalMesh->AddRef();

    // reallocate materials
    m_materials.Resize(m_pSkeletalMesh->GetMaterialCount());
    m_materials.Shrink();
    for (uint32 i = 0; i < m_pSkeletalMesh->GetMaterialCount(); i++)
    {
        m_materials[i] = m_pSkeletalMesh->GetMaterial(i);
        DebugAssert(m_materials[i] != NULL);
        m_materials[i]->AddRef();
    }

    // initialize bone transforms
    InitializeBoneTransformArray();

    // fix up bounds
    SetBounds(m_transform.TransformBoundingBox(m_pSkeletalMesh->GetBoundingBox()), m_transform.TransformBoundingSphere(m_pSkeletalMesh->GetBoundingSphere()));
}

void SkeletalMeshRenderProxy::SetMaterial(uint32 i, const Material *pMaterialOverride)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetMaterial(i, pMaterialOverride);
    }
    else
    {
        ReferenceCountedHolder<SkeletalMeshRenderProxy> pThisHolder(this);
        ReferenceCountedHolder<const Material> pMaterialHolder(pMaterialOverride);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, i, pMaterialHolder]() {
            pThisHolder->RealSetMaterial(i, pMaterialHolder);
        });
    }
}

void SkeletalMeshRenderProxy::RealSetMaterial(uint32 i, const Material *pMaterialOverride)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());
    DebugAssert(pMaterialOverride != NULL);

    // not yet owned by render thread, so no need to do async modifications
    DebugAssert(i < m_materials.GetSize());
    m_materials[i]->Release();
    m_materials[i] = pMaterialOverride;
    m_materials[i]->AddRef();
}

void SkeletalMeshRenderProxy::SetTransform(const Transform &transform)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetTransform(transform);
    }
    else
    {
        ReferenceCountedHolder<SkeletalMeshRenderProxy> pThisHolder(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, transform]() {
            pThisHolder->RealSetTransform(transform);
        });
    }
}

void SkeletalMeshRenderProxy::RealSetTransform(const Transform &transform)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_transform = transform;
    m_localToWorldMatrix = m_transform.GetTransformMatrix4x4();
    SetBounds(m_transform.TransformBoundingBox(m_pSkeletalMesh->GetBoundingBox()), m_transform.TransformBoundingSphere(m_pSkeletalMesh->GetBoundingSphere()));
}

void SkeletalMeshRenderProxy::SetTintColor(bool enabled, uint32 color /* = 0 */)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetTintColor(enabled, color);
    }
    else
    {
        ReferenceCountedHolder<SkeletalMeshRenderProxy> pThisHolder(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, enabled, color]() {
            pThisHolder->RealSetTintColor(enabled, color);
        });
    }
}

void SkeletalMeshRenderProxy::RealSetTintColor(bool enabled, uint32 color /*= 0*/)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_tintEnabled = enabled;
    m_tintColor = (enabled) ? color : 0xFFFFFFFF;
}

void SkeletalMeshRenderProxy::SetVisibility(bool visible)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetVisibility(visible);
    }
    else
    {
        ReferenceCountedHolder<SkeletalMeshRenderProxy> pThisHolder(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, visible]() {
            pThisHolder->RealSetVisibility(visible);
        });
    }
}

void SkeletalMeshRenderProxy::RealSetVisibility(bool visible)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_visibility = visible;
}

void SkeletalMeshRenderProxy::SetShadowFlags(uint32 shadowFlags)
{
    if (!IsInWorld())
    {
        // not yet in world so we can do it directly
        RealSetShadowFlags(shadowFlags);
    }
    else
    {
        ReferenceCountedHolder<SkeletalMeshRenderProxy> pThisHolder(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThisHolder, shadowFlags]() {
            pThisHolder->RealSetShadowFlags(shadowFlags);
        });
    }
}

void SkeletalMeshRenderProxy::RealSetShadowFlags(uint32 shadowFlags)
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    m_shadowFlags = shadowFlags;
}

void SkeletalMeshRenderProxy::QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    if (!m_visibility)
        return;

    // check gpu resources
    if (!m_bGPUResourcesCreated && !CreateDeviceResources())
        return;

    // skinning settings
    if (m_useGPUSkinning != CVars::r_gpu_skinning.GetBool())
    {
        m_useGPUSkinning = CVars::r_gpu_skinning.GetBool();
        ReleaseDeviceResources();
        if (!CreateDeviceResources())
            return;
    }

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

    // Calculate view distance
    float viewDistance = pCamera->CalculateDepthToPoint(GetBoundingSphere().GetCenter());

    // Draw this LOD.
    for (uint32 i = 0; i < m_pSkeletalMesh->GetBatchCount(); i++)
    {
        // Get batch info.
        const SkeletalMesh::Batch *pBatch = m_pSkeletalMesh->GetBatch(i);
        //if (pBatch->WeightCount != 1)
            //continue;
        
        // Get material.
        const Material *pMaterial = m_materials[pBatch->MaterialIndex];

        // Determine render passes. There isn't really any passes that a static mesh can't handle, so we only remove the material ones.
        uint32 renderPassMask = pMaterial->GetShader()->SelectRenderPassMask(wantedRenderPasses);

        // Add to render queue if we have any render passes.
        if (renderPassMask != 0)
        {
            // vertex factory flags for weight count
            uint32 vertexFactoryFlags = m_pSkeletalMesh->GetBaseVertexFactoryFlags();
            if (m_useGPUSkinning)
            {
                vertexFactoryFlags |= SKELETAL_MESH_VERTEX_FACTORY_FLAG_GPU_SKINNING;
                switch (pBatch->WeightCount)
                {
                case 4:
                    vertexFactoryFlags |= SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_3_ENABLED;

                case 3:
                    vertexFactoryFlags |= SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_2_ENABLED;

                case 2:
                    vertexFactoryFlags |= SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_1_ENABLED;

                case 1:
                    vertexFactoryFlags |= SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_0_ENABLED;
                }
            }

            RENDER_QUEUE_RENDERABLE_ENTRY queueEntry;
            queueEntry.RenderPassMask = renderPassMask;
            queueEntry.pRenderProxy = this;
            queueEntry.BoundingBox = GetBoundingBox();
            queueEntry.pVertexFactoryTypeInfo = VERTEX_FACTORY_TYPE_INFO(SkeletalMeshVertexFactory);
            queueEntry.VertexFactoryFlags = vertexFactoryFlags;
            queueEntry.pMaterial = pMaterial;
            queueEntry.ViewDistance = viewDistance;
            queueEntry.UserData[0] = i;
            queueEntry.TintColor = m_tintColor;
            queueEntry.Layer = pMaterial->GetShader()->SelectRenderQueueLayer();
            pRenderQueue->AddRenderable(&queueEntry);
        }
    }

    if (pRenderQueue->IsAcceptingDebugObjects())
    {
        if (CVars::r_show_skeletons.GetBool())
        {
            pRenderQueue->AddDebugInfoObject(this);
        }
    }
}

void SkeletalMeshRenderProxy::SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList, ShaderProgram *pShaderProgram) const
{
    const SkeletalMesh::Batch *pBatch = m_pSkeletalMesh->GetBatch(pQueueEntry->UserData[0]);
    SkeletalMeshVertexFactory::SetBoneMatrices(pCommandList, pShaderProgram, 0, pBatch->BoneRefCount, &m_boneTransforms[pBatch->BaseBoneRef]);

    pCommandList->GetConstants()->SetLocalToWorldMatrix(m_localToWorldMatrix, true);
    pCommandList->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);
    m_VertexBuffers.BindBuffers(pCommandList);
    pCommandList->SetIndexBuffer(m_pSkeletalMesh->GetIndexBuffer(), GPU_INDEX_FORMAT_UINT16, 0);
}

void SkeletalMeshRenderProxy::DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList) const
{
    const SkeletalMesh::Batch *pBatch = m_pSkeletalMesh->GetBatch(pQueueEntry->UserData[0]);
    pCommandList->DrawIndexed(pBatch->FirstIndex, pBatch->IndexCount, pBatch->BaseVertex);
}

bool SkeletalMeshRenderProxy::CreateDeviceResources() const
{
    uint32 i;
    if (m_bGPUResourcesCreated)
        return true;

    if (!m_pSkeletalMesh->CreateGPUResources())
        return false;

    for (i = 0; i < m_materials.GetSize(); i++)
    {
        if (!m_materials[i]->CreateDeviceResources())
            return false;
    }

    if (CVars::r_gpu_skinning.GetBool())
    {
        if (m_VertexBuffers.GetBuffer(0) == nullptr)
            SkeletalMeshVertexFactory::ShareVerticesBuffer(&m_VertexBuffers, m_pSkeletalMesh->GetVertexBuffers());
    }
    else
    {
        if (m_cpuSkinnedVertices.GetSize() != m_pSkeletalMesh->GetVertexCount())
        {
            m_cpuSkinnedVertices.Resize(m_pSkeletalMesh->GetVertexCount());
            m_cpuSkinnedVertices.ZeroContents();
        }

        if (m_pCPUSkinningVertexBuffer == nullptr)
        {
            uint32 vertexSize = SkeletalMeshVertexFactory::GetVertexSize(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), m_pSkeletalMesh->GetBaseVertexFactoryFlags());
            GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER | GPU_BUFFER_FLAG_MAPPABLE, vertexSize * m_pSkeletalMesh->GetVertexCount());
            if ((m_pCPUSkinningVertexBuffer = g_pRenderer->CreateBuffer(&bufferDesc, nullptr)) == nullptr)
                return false;            

            m_VertexBuffers.SetBuffer(0, m_pCPUSkinningVertexBuffer, 0, vertexSize);
        }
    }

    m_bGPUResourcesCreated = true;
    return true;
}

void SkeletalMeshRenderProxy::ReleaseDeviceResources() const
{
    m_VertexBuffers.Clear();
    SAFE_RELEASE(m_pCPUSkinningVertexBuffer);
    m_cpuSkinnedVertices.Obliterate();
    m_bGPUResourcesCreated = false;
}

void SkeletalMeshRenderProxy::InitializeBoneTransformArray()
{
    const Skeleton *pSkeleton = m_pSkeletalMesh->GetSkeleton();
    DebugAssert(pSkeleton->GetBoneCount() > 0 && m_pSkeletalMesh->GetBoneRefCount() > 0);

    m_boneTransforms.Resize(m_pSkeletalMesh->GetBoneRefCount());
    for (uint32 i = 0; i < m_boneTransforms.GetSize(); i++)
    {
        const uint32 meshBoneIndex = (uint32)m_pSkeletalMesh->GetBoneRef(i);
        const SkeletalMesh::Bone *pMeshBone = m_pSkeletalMesh->GetBone(meshBoneIndex);

        // calculate base frame transform
        //m_boneTransforms[i] = pSkeleton->GetBoneByIndex(pMeshBone->SkeletonBoneIndex)->GetAbsoluteBaseFrameTransform().GetTransformMatrix3x4() * pMeshBone->LocalToBoneTransform.GetTransformMatrix3x4();
        m_boneTransforms[i] = Transform::ConcatenateTransforms(pMeshBone->LocalToBoneTransform, pSkeleton->GetBoneByIndex(pMeshBone->SkeletonBoneIndex)->GetAbsoluteBaseFrameTransform()).GetTransformMatrix3x4();
    }
}

void SkeletalMeshRenderProxy::SetBoneTransforms(uint32 firstTransform, uint32 transformCount, const float3x4 *pTransforms)
{
    for (uint32 boneRefIndex = 0; boneRefIndex < m_boneTransforms.GetSize(); boneRefIndex++)
    {
        const uint32 meshBoneIndex = (uint32)m_pSkeletalMesh->GetBoneRef(boneRefIndex);
        const SkeletalMesh::Bone *pMeshBone = m_pSkeletalMesh->GetBone(meshBoneIndex);

        if (meshBoneIndex >= firstTransform && meshBoneIndex < (firstTransform + transformCount))
        {
            uint32 transformArrayIndex = meshBoneIndex - firstTransform;
            m_boneTransforms[boneRefIndex] = Transform::ConcatenateTransforms(pMeshBone->LocalToBoneTransform, Transform(pTransforms[transformArrayIndex])).GetTransformMatrix3x4();
            //m_boneTransforms[boneRefIndex] = pTransforms[transformArrayIndex];
        }
    }

    if (!m_useGPUSkinning)
    {
        if (!m_bGPUResourcesCreated && !CreateDeviceResources())
            return;

        TransformVerticesOnCPU();
    }
}

void SkeletalMeshRenderProxy::SetBoneTransforms(uint32 firstTransform, uint32 transformCount, const Transform *pTransforms)
{
    for (uint32 boneRefIndex = 0; boneRefIndex < m_boneTransforms.GetSize(); boneRefIndex++)
    {
        const uint32 meshBoneIndex = (uint32)m_pSkeletalMesh->GetBoneRef(boneRefIndex);
        const SkeletalMesh::Bone *pMeshBone = m_pSkeletalMesh->GetBone(meshBoneIndex);

        if (meshBoneIndex >= firstTransform && meshBoneIndex < (firstTransform + transformCount))
        {
            uint32 transformArrayIndex = meshBoneIndex - firstTransform;
            m_boneTransforms[boneRefIndex] = Transform::ConcatenateTransforms(pMeshBone->LocalToBoneTransform, pTransforms[transformArrayIndex]).GetTransformMatrix3x4();
            //m_boneTransforms[boneRefIndex] = pTransforms[transformArrayIndex].GetTransformMatrix3x4();
        }
    }

    if (!m_useGPUSkinning)
    {
        if (!m_bGPUResourcesCreated && !CreateDeviceResources())
            return;

        TransformVerticesOnCPU();
    }
}

void SkeletalMeshRenderProxy::ResetToBaseFrameTransform()
{
    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

    const Skeleton *pSkeleton = m_pSkeletalMesh->GetSkeleton();
    DebugAssert(pSkeleton->GetBoneCount() > 0 && m_pSkeletalMesh->GetBoneRefCount() > 0);

    for (uint32 i = 0; i < m_boneTransforms.GetSize(); i++)
    {
        const uint32 meshBoneIndex = (uint32)m_pSkeletalMesh->GetBoneRef(i);
        const SkeletalMesh::Bone *pMeshBone = m_pSkeletalMesh->GetBone(meshBoneIndex);

        // calculate base frame transform
        //m_boneTransforms[i] = pSkeleton->GetBoneByIndex(pMeshBone->SkeletonBoneIndex)->GetAbsoluteBaseFrameTransform().GetTransformMatrix3x4() * pMeshBone->LocalToBoneTransform.GetTransformMatrix3x4();
        m_boneTransforms[i] = Transform::ConcatenateTransforms(pMeshBone->LocalToBoneTransform, pSkeleton->GetBoneByIndex(pMeshBone->SkeletonBoneIndex)->GetAbsoluteBaseFrameTransform()).GetTransformMatrix3x4();
    }
}

void SkeletalMeshRenderProxy::DrawDebugInfo(const Camera *pCamera, GPUCommandList *pCommandList, MiniGUIContext *pGUIContext) const
{
    if (CVars::r_show_skeletons.GetBool())
    {
        const Skeleton *pSkeleton = m_pSkeletalMesh->GetSkeleton();

        // for each bone
        for (uint32 i = 0; i < pSkeleton->GetBoneCount(); i++)
        {
            const Skeleton::Bone *pParentBone = pSkeleton->GetBoneByIndex(i);
            const float3x4 &parentBoneTransform = m_boneTransforms[i];
            float3 lineSource(m_transform.TransformPoint(parentBoneTransform.TransformPoint(float3::Zero)));

            // for each bone that has this bone as a parent
            for (uint32 j = 0; j < pSkeleton->GetBoneCount(); j++)
            {
                const Skeleton::Bone *pChildBone = pSkeleton->GetBoneByIndex(j);
                if (pChildBone->GetParentBone() == pParentBone)
                {
                    const float3x4 &childBoneTransform = m_boneTransforms[j];

                    // draw a line from source to destination
                    float3 lineDestination(m_transform.TransformPoint(childBoneTransform.TransformPoint(float3::Zero)));
                    pGUIContext->Draw3DLineWidth(lineSource, lineDestination, MAKE_COLOR_R8G8B8A8_UNORM(240, 240, 240, 255), 2.0f);

                    // and the axes
                    float3 axisOrigin(lineDestination);
                    float3 xAxisPos(m_transform.TransformPoint(childBoneTransform.TransformPoint(float3::UnitX * 0.5f)));
                    float3 yAxisPos(m_transform.TransformPoint(childBoneTransform.TransformPoint(float3::UnitY * 0.5f)));
                    float3 zAxisPos(m_transform.TransformPoint(childBoneTransform.TransformPoint(float3::UnitZ * 0.5f)));
                    pGUIContext->Draw3DLineWidth(axisOrigin, xAxisPos, MAKE_COLOR_R8G8B8A8_UNORM(255, 0, 0, 255), 2.0f);
                    pGUIContext->Draw3DLineWidth(axisOrigin, yAxisPos, MAKE_COLOR_R8G8B8A8_UNORM(0, 255, 0, 255), 2.0f);
                    pGUIContext->Draw3DLineWidth(axisOrigin, zAxisPos, MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 255, 255), 2.0f);

                }
            }
        }
    }
}

void SkeletalMeshRenderProxy::TransformVerticesOnCPU()
{
    DebugAssert(m_cpuSkinnedVertices.GetSize() == m_pSkeletalMesh->GetVertexCount());

    for (uint32 batchIndex = 0; batchIndex < m_pSkeletalMesh->GetBatchCount(); batchIndex++)
    {
        const SkeletalMesh::Batch *pBatch = m_pSkeletalMesh->GetBatch(batchIndex);

        const SkeletalMeshVertexFactory::Vertex *pSourceVertex = m_pSkeletalMesh->GetVertex(pBatch->BaseVertex);
        SkeletalMeshVertexFactory::Vertex *pDestinationVertex = &m_cpuSkinnedVertices[pBatch->BaseVertex];
        const uint32 weightCount = pBatch->WeightCount;
        const uint32 baseBoneRef = pBatch->BaseBoneRef;
        const uint32 vertexCount = pBatch->VertexCount;
        DebugAssert(pBatch->WeightCount > 0);

        for (uint32 vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++, pSourceVertex++, pDestinationVertex++)
        {
            // should always have at least one bone
            const float3x4 *pBoneTransform = &m_boneTransforms[baseBoneRef + (uint32)pSourceVertex->BoneIndices[0]];
            float boneWeight = pSourceVertex->BoneWeights[0];

            // transform the attributes
            pDestinationVertex->Position = pBoneTransform->TransformPoint(pSourceVertex->Position) * boneWeight;
            pDestinationVertex->TangentX = pBoneTransform->TransformNormal(pSourceVertex->TangentX) * boneWeight;
            pDestinationVertex->TangentY = pBoneTransform->TransformNormal(pSourceVertex->TangentY) * boneWeight;
            pDestinationVertex->TangentZ = pBoneTransform->TransformNormal(pSourceVertex->TangentZ) * boneWeight;

            // and any remaining bones
            switch (weightCount)
            {
            case 4:
                {
                    pBoneTransform = &m_boneTransforms[baseBoneRef + (uint32)pSourceVertex->BoneIndices[3]];
                    boneWeight = pSourceVertex->BoneWeights[3];

                    pDestinationVertex->Position += pBoneTransform->TransformPoint(pSourceVertex->Position) * boneWeight;
                    pDestinationVertex->TangentX += pBoneTransform->TransformNormal(pSourceVertex->TangentX) * boneWeight;
                    pDestinationVertex->TangentY += pBoneTransform->TransformNormal(pSourceVertex->TangentY) * boneWeight;
                    pDestinationVertex->TangentZ += pBoneTransform->TransformNormal(pSourceVertex->TangentZ) * boneWeight;
                }

            case 3:
                {
                    pBoneTransform = &m_boneTransforms[baseBoneRef + (uint32)pSourceVertex->BoneIndices[2]];
                    boneWeight = pSourceVertex->BoneWeights[2];

                    pDestinationVertex->Position += pBoneTransform->TransformPoint(pSourceVertex->Position) * boneWeight;
                    pDestinationVertex->TangentX += pBoneTransform->TransformNormal(pSourceVertex->TangentX) * boneWeight;
                    pDestinationVertex->TangentY += pBoneTransform->TransformNormal(pSourceVertex->TangentY) * boneWeight;
                    pDestinationVertex->TangentZ += pBoneTransform->TransformNormal(pSourceVertex->TangentZ) * boneWeight;
                }

            case 2:
                {
                    pBoneTransform = &m_boneTransforms[baseBoneRef + (uint32)pSourceVertex->BoneIndices[1]];
                    boneWeight = pSourceVertex->BoneWeights[1];

                    pDestinationVertex->Position += pBoneTransform->TransformPoint(pSourceVertex->Position) * boneWeight;
                    pDestinationVertex->TangentX += pBoneTransform->TransformNormal(pSourceVertex->TangentX) * boneWeight;
                    pDestinationVertex->TangentY += pBoneTransform->TransformNormal(pSourceVertex->TangentY) * boneWeight;
                    pDestinationVertex->TangentZ += pBoneTransform->TransformNormal(pSourceVertex->TangentZ) * boneWeight;
                }
            }

            // normalize tangents
            pDestinationVertex->TangentX.SafeNormalizeInPlace();
            pDestinationVertex->TangentY.SafeNormalizeInPlace();
            pDestinationVertex->TangentZ.SafeNormalizeInPlace();

            // non-changing attributes
            pDestinationVertex->TexCoord = pSourceVertex->TexCoord;
            pDestinationVertex->Color = pSourceVertex->Color;
        }
    }

    // update the buffer
    void *pMappedBufferPointer;
    if (g_pRenderer->GetGPUContext()->MapBuffer(m_pCPUSkinningVertexBuffer, GPU_MAP_TYPE_WRITE_DISCARD, &pMappedBufferPointer))
    {
        uint32 vertexSize = SkeletalMeshVertexFactory::GetVertexSize(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), m_pSkeletalMesh->GetBaseVertexFactoryFlags());
        SkeletalMeshVertexFactory::FillVerticesBuffer(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), m_pSkeletalMesh->GetBaseVertexFactoryFlags(), m_cpuSkinnedVertices.GetBasePointer(), m_cpuSkinnedVertices.GetSize(), pMappedBufferPointer, vertexSize * m_pSkeletalMesh->GetVertexCount());
        g_pRenderer->GetGPUContext()->Unmapbuffer(m_pCPUSkinningVertexBuffer, pMappedBufferPointer);
    }
}

