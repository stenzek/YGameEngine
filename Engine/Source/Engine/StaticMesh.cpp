#include "Engine/PrecompiledHeader.h"
#include "Engine/StaticMesh.h"
#include "Engine/DataFormats.h"
#include "Engine/Material.h"
#include "Engine/ResourceManager.h"
#include "Engine/Physics/CollisionShape.h"
#include "Renderer/Renderer.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Core/ChunkFileReader.h"
#include "Core/MeshUtilties.h"
Log_SetChannel(StaticMesh);

DEFINE_RESOURCE_TYPE_INFO(StaticMesh);
DEFINE_RESOURCE_GENERIC_FACTORY(StaticMesh);

StaticMesh::StaticMesh(const ResourceTypeInfo *pResourceTypeInfo /* = &s_TypeInfo */) 
    : BaseClass(pResourceTypeInfo),
      m_boundingBox(AABox::Zero),
      m_boundingSphere(Sphere::Zero),
      m_vertexFactoryFlags(0),
      m_pCollisionShape(nullptr),
      m_pLODs(nullptr),
      m_LODCount(0),
      m_GPUResourcesCreated(false)
{

}

StaticMesh::LOD::LOD()
    : m_vertexFactoryFlags(0),
      m_pIndices(nullptr),
      m_indexCount(0),
      m_indexFormat(GPU_INDEX_FORMAT_COUNT),
      m_pIndexBuffer(nullptr),
      m_loaded(false)
{

}

StaticMesh::~StaticMesh()
{
    ReleaseGPUResources();

    delete[] m_pLODs;

    for (uint32 materialIndex = 0; materialIndex < m_materials.GetSize(); materialIndex++)
    {
        if (m_materials[materialIndex] != nullptr)
            m_materials[materialIndex]->Release();
    }

    if (m_pCollisionShape != nullptr)
        m_pCollisionShape->Release();
}

StaticMesh::LOD::~LOD()
{
    ReleaseGPUResources();
    Y_free(m_pIndices);
}

bool StaticMesh::Load(const char *FileName, ByteStream *pStream)
{
    BinaryReader binaryReader(pStream);

#define ABORTREASON(Reason) Log_ErrorPrintf("Could not load static mesh '%s': %s", FileName, Reason)

    // fix name
    m_strName = FileName;

    // read header
    DF_STATICMESH_HEADER meshHeader;
    if (!binaryReader.SafeReadBytes(&meshHeader, sizeof(meshHeader)) || 
        meshHeader.Magic != DF_STATICMESH_HEADER_MAGIC ||
        meshHeader.Size != sizeof(DF_STATICMESH_HEADER) ||
        meshHeader.MaterialCount == 0 ||
        meshHeader.LODCount == 0)
    {
        ABORTREASON("invalid file header.");
        return false;
    }

    // set data
    m_boundingBox = AABox(meshHeader.BoundingBoxMin, meshHeader.BoundingBoxMax);
    m_boundingSphere = Sphere(meshHeader.BoundingSphereCenter, meshHeader.BoundingSphereRadius);
    m_materials.Resize(meshHeader.MaterialCount);
    Y_memzero(m_materials.GetBasePointer(), sizeof(const Material *) * m_materials.GetSize());
    m_pLODs = new LOD[meshHeader.LODCount];
    m_LODCount = meshHeader.LODCount;
    m_LODOffsetTable.Resize(m_LODCount);
    m_LODOffsetTable.ZeroContents();

    // figure out VF flags
    m_vertexFactoryFlags = LOCAL_VERTEX_FACTORY_FLAG_TANGENT_VECTORS;
    if (meshHeader.VertexFlags & DF_STATICMESH_VERTEX_FLAG_COLOR)
        m_vertexFactoryFlags |= LOCAL_VERTEX_FACTORY_FLAG_VERTEX_COLORS;

    if (meshHeader.VertexFlags & DF_STATICMESH_VERTEX_FLAG_TEXCOORD_FLOAT2)
        m_vertexFactoryFlags |= LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT2_TEXCOORDS;
    else if (meshHeader.VertexFlags & DF_STATICMESH_VERTEX_FLAG_TEXCOORD_FLOAT3)
        m_vertexFactoryFlags |= LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT3_TEXCOORDS;

    // load collision shape
    if (meshHeader.CollisionShapeType != Physics::COLLISION_SHAPE_TYPE_NONE)
    {
        if (!binaryReader.SafeSeekAbsolute(meshHeader.CollisionShapeOffset))
            return false;
        
        m_pCollisionShape = Physics::CollisionShape::CreateFromStream(pStream, meshHeader.CollisionShapeSize);
        if (m_pCollisionShape == nullptr)
        {
            ABORTREASON("could not load collision shape");
            return false;
        }
    }

    // load materials
    {
        if (!binaryReader.SafeSeekAbsolute(meshHeader.MaterialNamesOffset))
            return false;

        SmallString materialName;
        for (uint32 materialIndex = 0; materialIndex < m_materials.GetSize(); materialIndex++)
        {
            if (!binaryReader.SafeReadCString(&materialName))
                return false;

            if ((m_materials[materialIndex] = g_pResourceManager->GetMaterial(materialName)) == nullptr)
            {
                m_materials[materialIndex] = g_pResourceManager->GetDefaultMaterial();
                DebugAssert(m_materials[materialIndex] != nullptr);
            }
        }
    }

    // load lods
    {
        if (!binaryReader.SafeSeekAbsolute(meshHeader.LODOffsetsOffset))
            return false;

        // load the offsets
        for (uint32 lodIndex = 0; lodIndex < m_LODCount; lodIndex++)
        {
            if (!binaryReader.SafeReadUInt32(&m_LODOffsetTable[lodIndex]))
                return false;
        }

        // load the actual lods
        for (uint32 lodIndex = 0; lodIndex < m_LODCount; lodIndex++)
        {
            if (!binaryReader.SafeSeekAbsolute(m_LODOffsetTable[lodIndex]) || !m_pLODs[lodIndex].LoadFromStream(pStream, m_vertexFactoryFlags))
                return false;
        }
    }

    // create on gpu
    if (g_pRenderer != nullptr && !CreateGPUResources())
    {
        ABORTREASON("failed to create GPU resources");
        return false;
    }

    return true;

#undef ABORTREASON
}

bool StaticMesh::LOD::LoadFromStream(ByteStream *pStream, uint32 vertexFactoryFlags)
{
    BinaryReader binaryReader(pStream);

    DF_STATICMESH_LOD_HEADER lodHeader;
    if (!binaryReader.SafeReadBytes(&lodHeader, sizeof(lodHeader)))
        return false;

    // allocate everything
    m_vertexFactoryFlags = vertexFactoryFlags;
    m_vertices.Resize(lodHeader.VertexCount);
    m_indexCount = lodHeader.IndexCount;
    m_indexFormat = (GPU_INDEX_FORMAT)lodHeader.IndexFormat;
    uint32 indicesSize = m_indexCount * ((m_indexFormat == GPU_INDEX_FORMAT_UINT32) ? sizeof(uint32) : sizeof(uint16));
    m_pIndices = Y_malloc(indicesSize);
    m_batches.Resize(lodHeader.BatchCount);

    // load vertices
    {
        if (!binaryReader.SafeSeekAbsolute(lodHeader.VerticesOffset))
            return false;

        // read them into a temporary memory block
        DF_STATICMESH_VERTEX *fileVertices = new DF_STATICMESH_VERTEX[m_vertices.GetSize()];
        if (!binaryReader.SafeReadBytes(fileVertices, sizeof(DF_STATICMESH_VERTEX) * m_vertices.GetSize()))
        {
            delete[] fileVertices;
            return false;
        }

        // parse vertices
        const DF_STATICMESH_VERTEX *pSourceVertex = fileVertices;
        LocalVertexFactory::Vertex *pDestinationVertex = m_vertices.GetBasePointer();
        for (uint32 vertexIndex = 0; vertexIndex < m_vertices.GetSize(); vertexIndex++)
        {
            pDestinationVertex->Position.Load(pSourceVertex->Position);
            pDestinationVertex->Tangent.Load(pSourceVertex->Tangent);
            pDestinationVertex->Binormal.Load(pSourceVertex->Binormal);
            pDestinationVertex->Normal.Load(pSourceVertex->Normal);
            pDestinationVertex->TexCoord.Load(pSourceVertex->TexCoord);
            pDestinationVertex->Color = pSourceVertex->Color;

            pSourceVertex++;
            pDestinationVertex++;
        }

        delete[] fileVertices;
    }

    // load indices
    {
        if (!binaryReader.SafeSeekAbsolute(lodHeader.IndicesOffset))
            return false;

        // one read
        if (!binaryReader.SafeReadBytes(m_pIndices, indicesSize))
            return false;
    }

    // load batches
    {
        if (!binaryReader.SafeSeekAbsolute(lodHeader.BatchesOffset))
            return false;

        // read them into a temporary memory block
        DF_STATICMESH_BATCH *fileBatches = new DF_STATICMESH_BATCH[m_batches.GetSize()];
        if (!binaryReader.SafeReadBytes(fileBatches, sizeof(DF_STATICMESH_BATCH) * m_batches.GetSize()))
        {
            delete[] fileBatches;
            return false;
        }

        // parse vertices
        const DF_STATICMESH_BATCH *pSourceBatch = fileBatches;
        Batch *pDestinationBatch = m_batches.GetBasePointer();
        for (uint32 batchIndex = 0; batchIndex < m_batches.GetSize(); batchIndex++)
        {
            pDestinationBatch->MaterialIndex = pSourceBatch->MaterialIndex;
            pDestinationBatch->StartIndex = pSourceBatch->StartIndex;
            pDestinationBatch->NumIndices = pSourceBatch->NumIndices;

            pSourceBatch++;
            pDestinationBatch++;
        }

        delete[] fileBatches;
    }

    // create on gpu
    if (!CreateGPUResources())
    {
        Log_ErrorPrintf("GPU upload failed.");
        return false;
    }

    m_loaded = true;
    return true;
}

void StaticMesh::LOD::Unload()
{
    ReleaseGPUResources();

    m_loaded = false;
    m_batches.Obliterate();
    Y_free(m_pIndices);
    m_pIndices = nullptr;
    m_indexCount = 0;
    m_indexFormat = GPU_INDEX_FORMAT_COUNT;
    m_vertices.Obliterate();
}

bool StaticMesh::CreateGPUResources() const
{
    if (m_GPUResourcesCreated)
        return true;

    for (uint32 lodIndex = 0; lodIndex < m_LODCount; lodIndex++)
    {
        LOD *pLOD = &m_pLODs[lodIndex];
        if (pLOD->IsLoaded() && !pLOD->CreateGPUResources())
            return false;
    }

    m_GPUResourcesCreated = true;
    return true;
}

bool StaticMesh::LOD::CreateGPUResources() const
{
    if (m_vertexBuffers.GetActiveBufferCount() == 0)
    {
        if (!LocalVertexFactory::CreateVerticesBuffer(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), m_vertexFactoryFlags, m_vertices.GetBasePointer(), m_vertices.GetSize(), &m_vertexBuffers))
            return false;
    }

    if (m_pIndexBuffer == nullptr)
    {
        GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_INDEX_BUFFER, m_indexCount * ((m_indexFormat == GPU_INDEX_FORMAT_UINT32) ? sizeof(uint32) : sizeof(uint16)));
        if ((m_pIndexBuffer = g_pRenderer->CreateBuffer(&bufferDesc, m_pIndices)) == NULL)
            return false;
    }

    return true;
}

void StaticMesh::ReleaseGPUResources() const
{
    for (uint32 lodIndex = 0; lodIndex < m_LODCount; lodIndex++)
    {
        LOD *pLOD = &m_pLODs[lodIndex];
        if (pLOD->IsLoaded())
            pLOD->ReleaseGPUResources();
    }

    m_GPUResourcesCreated = false;
}

void StaticMesh::LOD::ReleaseGPUResources() const
{
    m_vertexBuffers.Clear();
    SAFE_RELEASE(m_pIndexBuffer);
}

bool StaticMesh::CheckGPUResources() const
{
    if (!m_GPUResourcesCreated && !CreateGPUResources())
        return false;

    return true;
}

