#pragma once
#include "ResourceCompiler/Common.h"
#include "Core/PropertyTable.h"
#include "Engine/StaticMesh.h"

namespace Physics { class CollisionShapeGenerator; }

class StaticMeshGenerator
{
public:
    struct Vertex
    {
        float3 Position;
        float3 Tangent;
        float3 Binormal;
        float3 Normal;
        float3 TexCoord;
        uint32 Color;
    };

    struct Triangle
    {
        uint32 Indices[3];
    };

    struct Batch
    {
        String MaterialName;
        MemArray<Triangle> Triangles;
    };

    struct LOD
    {
        MemArray<Vertex> Vertices;
        PODArray<Batch *> Batches;
    };

    enum CenterOrigin
    {
        CenterOrigin_Center,
        CenterOrigin_CenterBottom,
        CenterOrigin_Count,
    };

    enum CollisionShapeType
    {
        CollisionShapeType_None,
        CollisionShapeType_Box,
        CollisionShapeType_Sphere,
        CollisionShapeType_TriangleMesh,
        CollisionShapeType_ConvexHull,
        CollisionShapeType_Count,
    };

public:
    StaticMeshGenerator();
    ~StaticMeshGenerator();

    // Accessors
    const AABox &GetBoundingBox() const { return m_boundingBox; }
    const Sphere &GetBoundingSphere() const { return m_boundingSphere; }

    const Vertex *GetVertex(uint32 lod, uint32 i) const { return &m_lods[lod]->Vertices[i]; }
    const uint32 GetVertexCount(uint32 lod) const { return m_lods[lod]->Vertices.GetSize(); }
    const Triangle *GetTriangle(uint32 lod, uint32 batchIndex, uint32 i) const { return &m_lods[lod]->Batches[batchIndex]->Triangles[i]; }
    const uint32 GetTriangleCount(uint32 lod, uint32 batchIndex) const { return m_lods[lod]->Batches[batchIndex]->Triangles.GetSize(); }
    const Batch *GetBatch(uint32 lod, uint32 i) const { return m_lods[lod]->Batches[i]; }
    const uint32 GetBatchCount(uint32 lod) const { return m_lods[lod]->Batches.GetSize(); }
    const LOD *GetLOD(uint32 i) const { return m_lods[i]; }
    const uint32 GetLODCount() const { return m_lods.GetSize(); }
    const Physics::CollisionShapeGenerator *GetCollisionShape() const { return m_pCollisionShapeGenerator; }

    // properties
    const PropertyTable *GetPropertyTable() const { return &m_properties; }
    bool GetVertexTextureCoordinatesEnabled() const;
    bool GetVertexColorsEnabled() const;
    void SetVertexTextureCoordinatesEnabled(bool enabled);
    void SetVertexColorsEnabled(bool enabled);
    uint32 GetVertexTextureCoordinateComponentCount() const;
    void SetVertexTextureCoordinateComponentCount(uint32 components);

    // Creation interface
    bool Create(bool enableVertexTextureCoordinates = true, bool enableVertexColors = false, uint32 textureCoordinateComponents = 2);

    // GPU data adding interface
    uint32 AddLOD();
    uint32 AddVertex(uint32 lod, const float3 &position, const float3 &tangent = float3::UnitX, const float3 &binormal = float3::UnitY, const float3 &normal = float3::UnitZ, const float3 &texCoord = float3::Zero, const uint32 color = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));
    uint32 AddBatch(uint32 lod, const char *materialName);
    uint32 AddTriangle(uint32 lod, uint32 batchIndex, const uint32 i0, const uint32 i1, const uint32 i2);

    // Loading interface (from XML)
    bool LoadFromXML(const char *FileName, ByteStream *pStream);

    // Output interface
    bool IsCompleteMesh() const;
    bool SaveToXML(ByteStream *pStream) const;
    bool Compile(ByteStream *pStream) const;

    // Copy another mesh
    void Copy(const StaticMeshGenerator *pGenerator);

    // Mesh Operations
    void CalculateBounds();
    void GenerateTangents(uint32 LODIndex);
    void JoinBatches();
    //void RemoveDuplicateVertices();
    //void RemoveUnusedTriangles();
    void CenterMesh(CenterOrigin origin = CenterOrigin_Center, float3 *pOffset = nullptr);
    void FlipTriangleWinding();

    // Collision Shape Generators
    bool BuildCollisionShape(CollisionShapeType type);
    bool BuildBoxCollisionShape();
    bool BuildSphereCollisionShape();
    bool BuildTriangleMeshCollisionShape(uint32 buildFromLOD = 0);
    bool BuildConvexHullCollisionShape(uint32 buildFromLOD = 0);
    void SetCollisionShape(Physics::CollisionShapeGenerator *pGenerator);
    void RemoveCollisionShape();

private:
    void InternalBuildTriangleMeshCollisionShape(uint32 buildFromLOD);

    AABox m_boundingBox;
    Sphere m_boundingSphere;
    PropertyTable m_properties;

    Physics::CollisionShapeGenerator *m_pCollisionShapeGenerator;

    PODArray<LOD *> m_lods;
};
