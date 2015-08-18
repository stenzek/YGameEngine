#pragma once
#include "Engine/Common.h"
#include "Engine/BlockPalette.h"
#include "Engine/BlockMeshVolume.h"
#include "Renderer/VertexFactories/BlockMeshVertexFactory.h"
#include "Renderer/VertexBufferBindingArray.h"

namespace Physics { class CollisionShape; }

class BlockMesh : public Resource
{
    DECLARE_RESOURCE_TYPE_INFO(BlockMesh, Resource);
    DECLARE_RESOURCE_GENERIC_FACTORY(BlockMesh);

public:
    static const uint32 MAX_LOD_LEVELS = 10;

public:
    struct Batch
    {
        uint32 MaterialIndex;
        uint32 StartIndex;
        uint32 IndexCount;
    };
    
public:
    BlockMesh(const ResourceTypeInfo *pResourceTypeInfo = &s_TypeInfo);
    ~BlockMesh();

    const BlockPalette *GetBlockList() const { return m_pPalette; }

    const AABox &GetBoundingBox() const { return m_boundingBox; }
    const Sphere &GetBoundingSphere() const { return m_boundingSphere; }

    const uint32 GetLODCount() const { return m_nLODLevels; }

    const BlockMeshVolume &GetMeshVolume(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].Volume; }
    const int3 &GetMeshMinCoordinates(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].Volume.GetMinCoordinates(); }
    const int3 &GetMeshMaxCoordinates(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].Volume.GetMaxCoordinates(); }
    const uint32 GetMeshWidth(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].Volume.GetWidth(); }
    const uint32 GetMeshLength(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].Volume.GetLength(); }
    const uint32 GetMeshHeight(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].Volume.GetHeight(); }
    const uint8 *GetMeshData(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].Volume.GetData(); }

    const Physics::CollisionShape *GetCollisionShape() const { return m_pCollisionShape; }

    const Batch *GetBatch(uint32 LOD, uint32 i) const { DebugAssert(LOD < m_nLODLevels && i < m_pRenderData[LOD].BatchCount); return &m_pRenderData[LOD].pBatches[i]; }
    uint32 GetBatchCount(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pRenderData[LOD].BatchCount; }

    // binary serialization
    bool Load(const char *resourceName, ByteStream *pStream);

    // resource binding & device resource management
    const uint32 GetVertexFactoryFlags() const { return m_vertexFactoryFlags; }
    VertexBufferBindingArray *GetVertexBuffers(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return &m_pRenderData[LOD].VertexBuffers; }
    GPUBuffer *GetIndexBuffer(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pRenderData[LOD].pIndexBuffer; }
    GPU_INDEX_FORMAT GetIndexFormat(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pRenderData[LOD].IndexFormat; }
    bool CreateGPUResources() const;
    void ReleaseGPUResources() const;
    bool CheckGPUResources() const;

private:
    bool CreateRenderData();

    struct LODLevel
    {
        AABox BoundingBox;
        Sphere BoundingSphere;
        BlockMeshVolume Volume;
    };

    struct RenderData
    {
        const BlockMeshBuilder::Vertex *pVertices;
        uint32 VertexCount;

        union
        {
            const void *pIndices;
            const uint16 *pIndices16;
            const uint32 *pIndices32;
        };
        GPU_INDEX_FORMAT IndexFormat;
        uint32 IndexCount;

        const Batch *pBatches;
        uint32 BatchCount;

        VertexBufferBindingArray VertexBuffers;
        GPUBuffer *pIndexBuffer;
    };

    const BlockPalette *m_pPalette;

    float m_scale;
    bool m_useAmbientOcclusion;
    uint32 m_nLODLevels;

    AABox m_boundingBox;
    Sphere m_boundingSphere;

    LODLevel *m_pLODLevels;

    Physics::CollisionShape *m_pCollisionShape;

    mutable RenderData *m_pRenderData;
    mutable uint32 m_vertexFactoryFlags;
    mutable bool m_bGPUResourcesCreated;
};

