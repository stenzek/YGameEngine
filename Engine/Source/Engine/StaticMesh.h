#pragma once
#include "Engine/Common.h"
#include "Renderer/VertexFactories/LocalVertexFactory.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/RendererTypes.h"
#include "Renderer/VertexBufferBindingArray.h"

namespace Physics { class CollisionShape; }
class VertexBufferBindingArray;
class Material;
class ChunkFileReader;

class StaticMesh : public Resource
{
    DECLARE_RESOURCE_TYPE_INFO(StaticMesh, Resource);
    DECLARE_RESOURCE_GENERIC_FACTORY(StaticMesh);

public:
    struct Batch
    {
        uint32 MaterialIndex;
        uint32 StartIndex;
        uint32 NumIndices;
    };

    class LOD
    {
    public:
        LOD();
        ~LOD();

        const bool IsLoaded() const { return m_loaded; }

        const LocalVertexFactory::Vertex *GetVertex(uint32 i) const { return &m_vertices[i]; }
        const uint32 GetVertexCount() const { return m_vertices.GetSize(); }
        const uint16 *GetIndices16() const { return reinterpret_cast<const uint16 *>(m_pIndices); }
        const uint32 *GetIndices32() const { return reinterpret_cast<const uint32 *>(m_pIndices); }
        const uint32 GetIndexCount() const { return m_indexCount; }
        const GPU_INDEX_FORMAT GetIndexFormat() const { return m_indexFormat; }

        const Batch *GetBatch(uint32 i) const { return &m_batches[i]; }
        const uint32 GetBatchCount() const { return m_batches.GetSize(); }

        const VertexBufferBindingArray *GetVertexBuffers() const { return &m_vertexBuffers; }
        GPUBuffer *GetIndexBuffer() const { return m_pIndexBuffer; }

        bool LoadFromStream(ByteStream *pStream, uint32 vertexFactoryFlags);
        void Unload();

        bool CreateGPUResources() const;
        void ReleaseGPUResources() const;

    private:
        // GPU Data
        uint32 m_vertexFactoryFlags;
        MemArray<LocalVertexFactory::Vertex> m_vertices;
        void *m_pIndices;
        uint32 m_indexCount;
        GPU_INDEX_FORMAT m_indexFormat;

        // Rendering information
        MemArray<Batch> m_batches;

        // Data on GPU
        mutable VertexBufferBindingArray m_vertexBuffers;
        mutable GPUBuffer *m_pIndexBuffer;

        // loaded flag
        bool m_loaded;
    };

    // constructor
    StaticMesh(const ResourceTypeInfo *pResourceTypeInfo = &s_TypeInfo);
    ~StaticMesh();

    // field accessors
    const AABox &GetBoundingBox() const { return m_boundingBox; }
    const Sphere &GetBoundingSphere() const { return m_boundingSphere; }
    const Physics::CollisionShape *GetCollisionShape() const { return m_pCollisionShape; }
    const uint32 GetVertexFactoryFlags() const { return m_vertexFactoryFlags; }

    // material access
    const Material *GetMaterial(uint32 i) const { return m_materials[i]; }
    const uint32 GetMaterialCount() const { return m_materials.GetSize(); }

    // lod access
    const LOD *GetLOD(uint32 i) const { DebugAssert(i < m_LODCount); return &m_pLODs[i]; }
    uint32 GetLODCount() const { return m_LODCount; }

    // binary serialization
    bool Load(const char *FileName, ByteStream *pStream);

    // resource binding & device resource management
    bool CreateGPUResources() const;
    void ReleaseGPUResources() const;
    bool CheckGPUResources() const;

private:
    AABox m_boundingBox;
    Sphere m_boundingSphere;
    uint32 m_vertexFactoryFlags;

    const Physics::CollisionShape *m_pCollisionShape;

    PODArray<const Material *> m_materials;
    PODArray<uint32> m_LODOffsetTable;

    LOD *m_pLODs;
    uint32 m_LODCount;

    mutable bool m_GPUResourcesCreated;
};

