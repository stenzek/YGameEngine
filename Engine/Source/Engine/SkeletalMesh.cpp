#include "Engine/PrecompiledHeader.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/DataFormats.h"
#include "Engine/ResourceManager.h"
#include "Engine/Material.h"
#include "Engine/Physics/CollisionShape.h"
#include "Renderer/VertexFactories/SkeletalMeshVertexFactory.h"
#include "Renderer/Renderer.h"
#include "Core/ChunkFileReader.h"
Log_SetChannel(SkeletalMesh);

DEFINE_RESOURCE_TYPE_INFO(SkeletalMesh);
DEFINE_RESOURCE_GENERIC_FACTORY(SkeletalMesh);

SkeletalMesh::SkeletalMesh(const ResourceTypeInfo *pResourceTypeInfo /*= &s_TypeInfo*/)
    : BaseClass(pResourceTypeInfo),
      m_pSkeleton(nullptr),
      m_boundingBox(AABox::Zero),
      m_boundingSphere(Sphere::Zero),
      m_baseVertexFactoryFlags(0),
      m_pCollisionShape(nullptr),
      m_bufferVertexFactoryFlags(0),
      m_pIndexBuffer(nullptr),
      m_GPUResourcesCreated(false)
{

}

SkeletalMesh::~SkeletalMesh()
{
    SkeletalMesh::ReleaseGPUResources();

    // release material references
    for (uint32 i = 0; i < m_materials.GetSize(); i++)
    {
        const Material *pMaterial = m_materials[i];
        if (pMaterial != nullptr)
            pMaterial->Release();
    }

    if (m_pSkeleton != nullptr)
        m_pSkeleton->Release();

    if (m_pCollisionShape != nullptr)
        m_pCollisionShape->Release();
}

bool SkeletalMesh::LoadFromStream(const char *name, ByteStream *pStream)
{
    DF_SKELETALMESH_HEADER fileHeader;
    if (!pStream->Read2(&fileHeader, sizeof(fileHeader)))
        return false;

    if (fileHeader.Magic != DF_SKELETALMESH_HEADER_MAGIC || fileHeader.HeaderSize != sizeof(fileHeader))
        return false;

    ChunkFileReader chunkReader;
    if (!chunkReader.Initialize(pStream))
        return false;

    // get skeleton
    {
        if (!chunkReader.LoadChunk(DF_SKELETALMESH_CHUNK_SKELETON) || chunkReader.GetCurrentChunkTypeCount<DF_SKELETALMESH_SKELETON>() != 1)
            return false;

        const DF_SKELETALMESH_SKELETON *pSkeletonChunk = chunkReader.GetCurrentChunkTypePointer<DF_SKELETALMESH_SKELETON>();
        m_pSkeleton = g_pResourceManager->GetSkeleton(chunkReader.GetStringByIndex(pSkeletonChunk->SkeletonNameStringIndex));
        if (m_pSkeleton == nullptr)
        {
            Log_ErrorPrintf("SkeletalMesh::LoadFromStream: Failed to load skeleton '%s'", chunkReader.GetStringByIndex(pSkeletonChunk->SkeletonNameStringIndex));
            return false;
        }

        if (m_pSkeleton->GetBoneCount() != fileHeader.SkeletonBoneCount)
        {
            Log_ErrorPrintf("SkeletalMesh::LoadFromStream: Mismatched skeleton '%s' (%u vs %u bones)", chunkReader.GetStringByIndex(pSkeletonChunk->SkeletonNameStringIndex), fileHeader.BoneCount, m_pSkeleton->GetBoneCount());
            return false;
        }
    }

    // allocate everything
    m_boundingBox.SetBounds(float3(fileHeader.BoundingBoxMin), float3(fileHeader.BoundingBoxMax));
    m_boundingSphere.SetCenterAndRadius(float3(fileHeader.BoundingSphereCenter), fileHeader.BoundingSphereRadius);
    m_materials.Resize(fileHeader.MaterialCount);
    m_materials.ZeroContents();
    m_bones.Resize(fileHeader.BoneCount);
    m_boneRefs.Resize(fileHeader.BoneRefCount);
    m_vertices.Resize(fileHeader.VertexCount);
    m_indices.Resize(fileHeader.IndexCount);
    m_batches.Resize(fileHeader.BatchCount);

    // bones
    {
        if (!chunkReader.LoadChunk(DF_SKELETALMESH_CHUNK_BONES) || chunkReader.GetCurrentChunkTypeCount<DF_SKELETALMESH_BONE>() != fileHeader.BoneCount)
            return false;

        const DF_SKELETALMESH_BONE *pSourceBone = chunkReader.GetCurrentChunkTypePointer<DF_SKELETALMESH_BONE>();
        Bone *pDestinationBone = m_bones.GetBasePointer();
        for (uint32 i = 0; i < m_bones.GetSize(); i++, pSourceBone++, pDestinationBone++)
        {
            if (pSourceBone->SkeletonBoneIndex >= m_pSkeleton->GetBoneCount())
                return false;

            pDestinationBone->SkeletonBoneIndex = pSourceBone->SkeletonBoneIndex;
            pDestinationBone->LocalToBoneTransform = Transform(float3(pSourceBone->LocalToBonePosition), Quaternion(pSourceBone->LocalToBoneRotation), float3(pSourceBone->LocalToBoneScale));
        }
    }

    // bone refs
    {
        if (!chunkReader.LoadChunk(DF_SKELETALMESH_CHUNK_BONE_REFS) || chunkReader.GetCurrentChunkTypeCount<uint16>() != fileHeader.BoneRefCount)
            return false;

        Y_memcpy(m_boneRefs.GetBasePointer(), chunkReader.GetCurrentChunkPointer(), sizeof(uint16) * fileHeader.BoneRefCount);
    }

    // read materials
    {
        if (!chunkReader.LoadChunk(DF_SKELETALMESH_CHUNK_MATERIALS) || chunkReader.GetCurrentChunkTypeCount<DF_SKELETALMESH_MATERIAL>() != m_materials.GetSize())
            return false;

        const DF_SKELETALMESH_MATERIAL *pSourceMaterials = chunkReader.GetCurrentChunkTypePointer<DF_SKELETALMESH_MATERIAL>();
        for (uint32 i = 0; i < m_materials.GetSize(); i++)
        {
            const char *materialName = chunkReader.GetStringByIndex(pSourceMaterials[i].MaterialNameStringIndex);
            const Material *pMaterial = g_pResourceManager->GetMaterial(materialName);
            if (pMaterial == nullptr)
            {
                Log_WarningPrintf("SkeletalMesh::LoadFromStream: Could not find material '%s', using default.", materialName);
                pMaterial = g_pResourceManager->GetDefaultMaterial();
            }

            m_materials[i] = pMaterial;
        }
    }

    // read vertices
    {
        if (!chunkReader.LoadChunk(DF_SKELETALMESH_CHUNK_VERTICES) || chunkReader.GetCurrentChunkTypeCount<DF_SKELETALMESH_VERTEX>() != m_vertices.GetSize())
            return false;

        const DF_SKELETALMESH_VERTEX *pSourceVertices = chunkReader.GetCurrentChunkTypePointer<DF_SKELETALMESH_VERTEX>();
        for (uint32 i = 0; i < m_vertices.GetSize(); i++)
        {
            const DF_SKELETALMESH_VERTEX *pSourceVertex = &pSourceVertices[i];
            SkeletalMeshVertexFactory::Vertex *pDestinationVertex = &m_vertices[i];

            pDestinationVertex->Position.Load(pSourceVertex->Position);
            pDestinationVertex->TangentX.Load(pSourceVertex->Tangent);
            pDestinationVertex->TangentY.Load(pSourceVertex->Binormal);
            pDestinationVertex->TangentZ.Load(pSourceVertex->Normal);
            pDestinationVertex->TexCoord.Load(pSourceVertex->TextureCoordinates);
            pDestinationVertex->Color = pSourceVertex->Color;
            Y_memcpy(pDestinationVertex->BoneIndices, pSourceVertex->BoneIndices, sizeof(pDestinationVertex->BoneIndices));
            Y_memcpy(pDestinationVertex->BoneWeights, pSourceVertex->BoneWeights, sizeof(pDestinationVertex->BoneWeights));
        }
    }

    // read indices
    {
        if (!chunkReader.LoadChunk(DF_SKELETALMESH_CHUNK_INDICES) || chunkReader.GetCurrentChunkTypeCount<uint16>() != m_indices.GetSize())
            return false;

        Y_memcpy(m_indices.GetBasePointer(), chunkReader.GetCurrentChunkPointer(), sizeof(uint16) * m_indices.GetSize());
    }

    // read batches
    {
        if (!chunkReader.LoadChunk(DF_SKELETALMESH_CHUNK_BATCHES) || chunkReader.GetCurrentChunkTypeCount<DF_SKELETALMESH_BATCH>() != m_batches.GetSize())
            return false;

        // determine base vertex factory flags
        m_baseVertexFactoryFlags = 0;
        if (fileHeader.Flags & DF_SKELETALMESH_FLAG_USE_TEXTURE_COORDINATES)
            m_baseVertexFactoryFlags |= SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_TEXCOORDS;
        if (fileHeader.Flags & DF_SKELETALMESH_FLAG_USE_VERTEX_COLORS)
            m_baseVertexFactoryFlags |= SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS;

        // add batches
        const DF_SKELETALMESH_BATCH *pSourceBatches = chunkReader.GetCurrentChunkTypePointer<DF_SKELETALMESH_BATCH>();
        for (uint32 i = 0; i < m_batches.GetSize(); i++)
        {
            const DF_SKELETALMESH_BATCH *pSourceBatch = &pSourceBatches[i];
            Batch *pDestinationBatch = &m_batches[i];

            // set fields
            pDestinationBatch->MaterialIndex = pSourceBatch->MaterialIndex;
            pDestinationBatch->WeightCount = pSourceBatch->WeightCount;
            pDestinationBatch->BaseBoneRef = pSourceBatch->BaseBoneRef;
            pDestinationBatch->BoneRefCount = pSourceBatch->BoneRefCount;
            pDestinationBatch->BaseVertex = pSourceBatch->BaseVertex;
            pDestinationBatch->VertexCount = pSourceBatch->VertexCount;
            pDestinationBatch->FirstIndex = pSourceBatch->StartIndex;
            pDestinationBatch->IndexCount = pSourceBatch->IndexCount;
        }
    }

    // read collision shape
    if (fileHeader.CollisionShapeType != Physics::COLLISION_SHAPE_TYPE_NONE)
    {
        if (!chunkReader.LoadChunk(DF_SKELETALMESH_CHUNK_COLLISION_SHAPE))
            return false;

        m_pCollisionShape = Physics::CollisionShape::CreateFromData(chunkReader.GetCurrentChunkPointer(), chunkReader.GetCurrentChunkSize());
        if (m_pCollisionShape == nullptr)
            return false;
    }

    // done
    m_strName = name;

    // create on gpu
    if (!CreateGPUResources())
    {
        Log_ErrorPrintf("GPU upload failed.");
        return false;
    }

    return true;
}

bool SkeletalMesh::CreateGPUResources() const
{
    if (m_GPUResourcesCreated)
        return true;

    // todo: check constant buffer limits
    m_bufferVertexFactoryFlags = m_baseVertexFactoryFlags | SKELETAL_MESH_VERTEX_FACTORY_FLAG_GPU_SKINNING;

    if (m_vertexBuffers.GetBuffer(0) == nullptr)
    {
        if (!SkeletalMeshVertexFactory::CreateVerticesBuffer(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), m_bufferVertexFactoryFlags, m_vertices.GetBasePointer(), m_vertices.GetSize(), &m_vertexBuffers))
            return false;
    }

    if (m_pIndexBuffer == NULL)
    {
        GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_INDEX_BUFFER, m_indices.GetStorageSizeInBytes());
        if ((m_pIndexBuffer = g_pRenderer->CreateBuffer(&bufferDesc, m_indices.GetBasePointer())) == nullptr)
            return false;
    }

    m_GPUResourcesCreated = true;
    return true;
}

void SkeletalMesh::ReleaseGPUResources() const
{
    m_vertexBuffers.Clear();
    SAFE_RELEASE(m_pIndexBuffer);
    m_GPUResourcesCreated = false;
}

bool SkeletalMesh::CheckGPUResources() const
{
    if (m_vertexBuffers.GetBuffer(0) == nullptr)
        return false;

    if (m_pIndexBuffer == nullptr)
        return false;

    return true;
}

/*
SkeletalMeshInstanceBoneData::SkeletalMeshInstanceBoneData(const SkeletalMesh *pSkeletalMesh)
{
    InitializeForMesh(pSkeletalMesh);
}

SkeletalMeshInstanceBoneData::~SkeletalMeshInstanceBoneData()
{

}

void SkeletalMeshInstanceBoneData::InitializeForMesh(const SkeletalMesh *pSkeletalMesh)
{
    const Skeleton *pSkeleton = pSkeletalMesh->GetSkeleton();

    if (pSkeleton->GetBoneCount() != m_boneTransforms.GetSize())
    {
        m_boneTransforms.Obliterate();
        m_boneTransforms.Resize(pSkeleton->GetBoneCount());
    }

    ResetToBaseFrame(pSkeletalMesh);
}

void SkeletalMeshInstanceBoneData::ResetToBaseFrame(const SkeletalMesh *pSkeletalMesh)
{
    const Skeleton *pSkeleton = pSkeletalMesh->GetSkeleton();
    DebugAssert(pSkeleton->GetBoneCount() == m_boneTransforms.GetSize());

    for (uint32 i = 0; i < m_boneTransforms.GetSize(); i++)
        m_boneTransforms[i] = pSkeleton->GetBoneByIndex(i)->GetAbsoluteBaseFrameTransform();
}
*/
