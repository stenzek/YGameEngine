#include "Engine/PrecompiledHeader.h"
#include "Engine/BlockMesh.h"
#include "Engine/DataFormats.h"
#include "Engine/ResourceManager.h"
#include "Engine/BlockMeshBuilder.h"
#include "Engine/Physics/CollisionShape.h"
#include "Engine/BlockMeshCollisionShape.h"
#include "Renderer/Renderer.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Core/ChunkFileReader.h"
Log_SetChannel(BlockMesh);

DEFINE_RESOURCE_TYPE_INFO(BlockMesh);
DEFINE_RESOURCE_GENERIC_FACTORY(BlockMesh);

BlockMesh::BlockMesh(const ResourceTypeInfo *pResourceTypeInfo /*= &s_TypeInfo*/)
    : Resource(pResourceTypeInfo),
      m_pPalette(NULL),
      m_scale(0.0f),
      m_useAmbientOcclusion(false),
      m_nLODLevels(0),
      m_boundingBox(AABox::Zero),
      m_boundingSphere(Sphere::Zero),
      m_pLODLevels(NULL),
      m_pCollisionShape(NULL),
      m_pRenderData(NULL),
      m_vertexFactoryFlags(0),
      m_bGPUResourcesCreated(false)
{

}

BlockMesh::~BlockMesh()
{
    ReleaseGPUResources();

    if (m_pPalette != NULL)
        m_pPalette->Release();

    if (m_pCollisionShape != NULL)
        m_pCollisionShape->Release();

    if (m_pRenderData != NULL)
    {
        for (uint32 i = 0; i < m_nLODLevels; i++)
        {
            delete[] m_pRenderData[i].pVertices;
            Y_free((void *)m_pRenderData[i].pIndices);
            delete[] m_pRenderData[i].pBatches;
        }
        delete[] m_pRenderData;
    }

    delete[] m_pLODLevels;
}

bool BlockMesh::Load(const char *resourceName, ByteStream *pStream)
{
    // set name
    m_strName = resourceName;

    BinaryReader binaryReader(pStream);
    if (binaryReader.ReadUInt32() != DF_BLOCK_MESH_HEADER_MAGIC)
    {
        Log_ErrorPrintf("BlockMesh::Load: '%s': Invalid file magic", resourceName);
        return false;
    }

    PathString paletteName;
    binaryReader.ReadSizePrefixedString(paletteName);
    if ((m_pPalette = g_pResourceManager->GetBlockPalette(paletteName)) == NULL)
    {
        Log_ErrorPrintf("BlockMesh::Load: '%s': Could not load palette '%s'", resourceName, paletteName.GetCharArray());
        return false;
    }

    m_scale = binaryReader.ReadFloat();
    m_useAmbientOcclusion = binaryReader.ReadBool();
    m_nLODLevels = binaryReader.ReadUInt32();

    if (m_nLODLevels > MAX_LOD_LEVELS)
    {
        Log_ErrorPrintf("BlockMesh::Load: '%s': Invalid LOD level count", resourceName);
        return false;
    }

    // bounds
    binaryReader >> m_boundingBox;
    binaryReader >> m_boundingSphere;

    // lod levels
    m_pLODLevels = new LODLevel[m_nLODLevels];
    for (uint32 i = 0; i < m_nLODLevels; i++)
    {           
        float levelScale;
        float3 levelBoundingBoxMinBounds;
        float3 levelBoundingBoxMaxBounds;
        float3 levelBoundingSphereCenter;
        float levelBoundingSphereRadius;
        binaryReader >> levelScale;
        binaryReader >> levelBoundingBoxMinBounds;
        binaryReader >> levelBoundingBoxMaxBounds;
        binaryReader >> levelBoundingSphereCenter;
        binaryReader >> levelBoundingSphereRadius;

        int3 levelMinCoordinates;
        int3 levelMaxCoordinates;
        uint32 levelWidth;
        uint32 levelLength;
        uint32 levelHeight;
        binaryReader >> levelMinCoordinates;
        binaryReader >> levelMaxCoordinates;
        binaryReader >> levelWidth;
        binaryReader >> levelLength;
        binaryReader >> levelHeight;

        // some sanity checks
        if (levelMinCoordinates.AnyGreater(levelMaxCoordinates) || levelMaxCoordinates.AnyLess(levelMinCoordinates))
        {
            Log_ErrorPrintf("BlockMesh::Load: '%s': Corrupted data", resourceName);
            return false;
        }

        // init the volume
        LODLevel &lodLevel = m_pLODLevels[i];
        lodLevel.Volume.SetPalette(m_pPalette);
        lodLevel.Volume.SetScale(levelScale);
        lodLevel.Volume.Resize(levelMinCoordinates, levelMaxCoordinates);
        if (lodLevel.Volume.GetWidth() != levelWidth || lodLevel.Volume.GetLength() != levelLength || lodLevel.Volume.GetHeight() != levelHeight)
        {
            Log_ErrorPrintf("BlockMesh::Load: '%s': Corrupted data", resourceName);
            return false;
        }

        // set other parameters to level
        lodLevel.BoundingBox.SetBounds(levelBoundingBoxMinBounds, levelBoundingBoxMaxBounds);
        lodLevel.BoundingSphere.SetCenterAndRadius(levelBoundingSphereCenter, levelBoundingSphereRadius);

        // fill the volume
        uint32 nBlocks = levelWidth * levelLength * levelHeight;
        binaryReader.ReadBytes(lodLevel.Volume.GetData(), sizeof(BlockVolumeBlockType) * nBlocks);
    }

    // collision shape
    uint32 collisionShapeType = binaryReader.ReadUInt32();
    if (collisionShapeType != Physics::COLLISION_SHAPE_TYPE_NONE)
    {
        if (collisionShapeType == Physics::COLLISION_SHAPE_TYPE_BLOCK_MESH)
        {
            // create block collision shape
            m_pCollisionShape = new BlockMeshCollisionShape(this);
        }
        else
        {
            // load the collision shape
            uint32 collisionShapeDataSize = binaryReader.ReadUInt32();
            DebugAssert(collisionShapeDataSize > 0);

            // create collision shape data
            if ((m_pCollisionShape = Physics::CollisionShape::CreateFromStream(pStream, collisionShapeDataSize)) == nullptr)
            {
                Log_ErrorPrintf("BlockMesh::Load: '%s': Could not create collision shape", resourceName);
                return false;
            }
        }
    }

    if (g_pRenderer != NULL && !CreateRenderData())
    {
        Log_ErrorPrintf("BlockMesh::Load: '%s': Could not create render data", resourceName);
        return false;
    }
    
    if (pStream->InErrorState())
    {
        Log_DevPrintf("Stream in error state.");
        return false;
    }

    // create on gpu
    if (!CreateGPUResources())
    {
        Log_ErrorPrintf("GPU upload failed.");
        return false;
    }

    return true;
}

bool BlockMesh::CreateRenderData()
{
    DebugAssert(m_pRenderData == NULL);
    m_pRenderData = new RenderData[m_nLODLevels];

    // keep pointers to null
    for (uint32 i = 0; i < m_nLODLevels; i++)
    {
        RenderData &renderData = m_pRenderData[i];
        renderData.pVertices = NULL;
        renderData.pIndices = NULL;
        renderData.pBatches = NULL;
        renderData.pIndexBuffer = NULL;
    }

    // build data
    for (uint32 i = 0; i < m_nLODLevels; i++)
    {
        const LODLevel &lodLevel = m_pLODLevels[i];
        RenderData &renderData = m_pRenderData[i];

        // setup builder
        BlockMeshBuilder builder;
        builder.SetPalette(m_pPalette);
        builder.SetFromVolume(&lodLevel.Volume);

        // build it
        builder.GenerateMesh();

        // copy results out to render data
        renderData.VertexCount = builder.GetOutputVertexCount();
        renderData.IndexCount = builder.GetOutputTriangleCount() * 3;
        renderData.IndexFormat = (renderData.VertexCount <= 0xFFFF) ? GPU_INDEX_FORMAT_UINT16 : GPU_INDEX_FORMAT_UINT32;
        renderData.BatchCount = builder.GetOutputBatchCount();

        // skip if no tris
        if (renderData.BatchCount > 0)
        {
            // verts
            BlockMeshBuilder::Vertex *pVertices = new BlockMeshBuilder::Vertex[renderData.VertexCount];
            Y_memcpy(pVertices, builder.GetOutputVertices().GetBasePointer(), sizeof(BlockMeshBuilder::Vertex) * renderData.VertexCount);
            renderData.pVertices = pVertices;

            // tris
            if (renderData.IndexFormat == GPU_INDEX_FORMAT_UINT16)
            {
                uint16 *pIndices = Y_mallocT<uint16>(renderData.IndexCount);
                for (uint32 j = 0, o = 0; j < builder.GetOutputTriangleCount(); j++)
                {
                    const BlockMeshBuilder::Triangle &tri = builder.GetOutputTriangles()[j];
                    pIndices[o++] = (uint16)tri.Indices[0];
                    pIndices[o++] = (uint16)tri.Indices[1];
                    pIndices[o++] = (uint16)tri.Indices[2];
                }
                renderData.pIndices = pIndices;
            }
            else
            {
                uint32 *pIndices = Y_mallocT<uint32>(renderData.IndexCount);
                for (uint32 j = 0, o = 0; j < renderData.IndexCount; j++)
                {
                    const BlockMeshBuilder::Triangle &tri = builder.GetOutputTriangles()[j];
                    pIndices[o++] = tri.Indices[0];
                    pIndices[o++] = tri.Indices[1];
                    pIndices[o++] = tri.Indices[2];
                }
                renderData.pIndices = pIndices;
            }

            // batches
            Batch *pBatches = new Batch[renderData.BatchCount];
            for (uint32 j = 0; j < renderData.BatchCount; j++)
            {
                const BlockMeshBuilder::Batch &sourceBatch = builder.GetOutputBatches()[j];
                Batch &destBatch = pBatches[j];

                destBatch.MaterialIndex = sourceBatch.MaterialIndex;
                destBatch.StartIndex = sourceBatch.StartIndex;
                destBatch.IndexCount = sourceBatch.NumIndices;
            }
            renderData.pBatches = pBatches;
        }
    }

    // figure out flags
    m_vertexFactoryFlags = BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXCOORD | BLOCK_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS | BLOCK_MESH_VERTEX_FACTORY_FLAG_FACE_INDEX;
    if (g_pRenderer->GetCapabilities().SupportsTextureArrays)
        m_vertexFactoryFlags |= BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXTURE_ARRAY;
    else
        m_vertexFactoryFlags |= BLOCK_MESH_VERTEX_FACTORY_FLAG_ATLAS_TEXCOORDS;

    return true;
}

bool BlockMesh::CreateGPUResources() const
{
    if (m_bGPUResourcesCreated)
        return true;

    DebugAssert(m_pRenderData != NULL);

    for (uint32 i = 0; i < m_nLODLevels; i++)
    {
        RenderData &renderData = m_pRenderData[i];
        if (renderData.VertexBuffers.GetBuffer(0) == NULL)
        {
            if (!BlockMeshVertexFactory::CreateVerticesBuffer(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), m_vertexFactoryFlags, renderData.pVertices, renderData.VertexCount, &renderData.VertexBuffers))
                return false;
        }

        if (renderData.pIndexBuffer == NULL)
        {
            GPU_BUFFER_DESC indexBufferDesc(GPU_BUFFER_FLAG_BIND_INDEX_BUFFER, renderData.IndexCount * ((renderData.IndexFormat == GPU_INDEX_FORMAT_UINT16) ? sizeof(uint16) : sizeof(uint32)));
            if ((renderData.pIndexBuffer = g_pRenderer->CreateBuffer(&indexBufferDesc, renderData.pIndices)) == NULL)
                return false;
        }
    }


    m_bGPUResourcesCreated = true;
    return true;
}

void BlockMesh::ReleaseGPUResources() const
{
    if (m_pRenderData != NULL)
    {
        for (uint32 i = 0; i < m_nLODLevels; i++)
        {
            RenderData &renderData = m_pRenderData[i];
            renderData.VertexBuffers.Clear();
            SAFE_RELEASE(renderData.pIndexBuffer);
        }
    }

    m_bGPUResourcesCreated = false;
}

bool BlockMesh::CheckGPUResources() const
{
    if (!m_bGPUResourcesCreated && !CreateGPUResources())
        return false;

    return true;
}
