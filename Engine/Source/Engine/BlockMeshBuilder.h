#pragma once
#include "Engine/Common.h"
#include "Engine/BlockPalette.h"
#include "Engine/BlockMeshVolume.h"

class VertexBufferBindingArray;
class GPUBuffer;

class BlockMeshBuilder
{
public:
    enum NEIGHBOUR_VOLUME
    {
        NEIGHBOUR_VOLUME_LEFT,
        NEIGHBOUR_VOLUME_RIGHT,
        NEIGHBOUR_VOLUME_BACK,
        NEIGHBOUR_VOLUME_FRONT,
        NEIGHBOUR_VOLUME_BOTTOM,
        NEIGHBOUR_VOLUME_TOP,
        NEIGHBOUR_VOLUME_COUNT,
    };

    struct Vertex
    {
        float3 Position;
        float3 TexCoord;
        float4 AtlasTexCoord;
        uint32 Color;
        uint8 FaceIndex;

        Vertex()
        {

        }

        Vertex(const float3 &position, const float3 &texcoord, const float4 &atlasTexCoord, uint32 color, uint8 faceIndex)
        {
            Position = position;
            TexCoord = texcoord;
            AtlasTexCoord = atlasTexCoord;
            Color = color;
            FaceIndex = faceIndex;
        }

        void Set(const float3 &position, const float3 &texcoord, const float4 &atlasTexCoord, uint32 color, uint8 faceIndex)
        {
            Position = position;
            TexCoord = texcoord;
            AtlasTexCoord = atlasTexCoord;
            Color = color;
            FaceIndex = faceIndex;
        }
    };

    struct Triangle
    {
        uint32 MaterialIndex;
        uint32 FaceIndex;
        uint32 Indices[3];

        Triangle() 
        {

        }

        Triangle(uint32 materialIndex, uint32 faceIndex, uint32 i0, uint32 i1, uint32 i2)
        {
            MaterialIndex = materialIndex;
            FaceIndex = faceIndex;
            Indices[0] = i0;
            Indices[1] = i1;
            Indices[2] = i2;
        }

        void Set(uint32 materialIndex, uint32 faceIndex, uint32 i0, uint32 i1, uint32 i2)
        {
            MaterialIndex = materialIndex;
            FaceIndex = faceIndex;
            Indices[0] = i0;
            Indices[1] = i1;
            Indices[2] = i2;
        }
    };

    struct Batch
    {
        uint32 MaterialIndex;
        uint32 StartIndex;
        uint32 NumIndices;
        bool DrawShadows;
    };

    typedef MemArray<Vertex> VertexArray;
    typedef MemArray<Triangle> TriangleArray;
    typedef MemArray<Batch> BatchArray;

public:
    BlockMeshBuilder();
    ~BlockMeshBuilder();

    // input data accessors
    const BlockPalette *GetPalette() const { return m_pPalette; }
    const uint32 GetWidth() const { return m_width; }
    const uint32 GetLength() const { return m_length; }
    const uint32 GetHeight() const { return m_height; }
    const BlockVolumeBlockType *GetBlockData() const { return m_pBlockData; }
    const BlockVolumeBlockType *GetNeighbourBlockData(NEIGHBOUR_VOLUME neighbour) const { DebugAssert(neighbour < NEIGHBOUR_VOLUME_COUNT); return m_pNeighbourBlockData[neighbour]; }
    const float3 &GetTranslation() const { return m_translation; }
    const float GetBlockScale() const { return m_scale; }

    // input data mutators
    void SetPalette(const BlockPalette *pBlockList) { m_pPalette = pBlockList;}
    void SetSize(uint32 meshWidth, uint32 meshLength, uint32 meshHeight) { m_width = meshWidth; m_length = meshLength; m_height = meshHeight; }
    void SetBlockData(const BlockVolumeBlockType *pBlockData) { m_pBlockData = pBlockData; }
    void SetNeighbourBlockData(NEIGHBOUR_VOLUME neighbour, const BlockVolumeBlockType *pBlockData) { DebugAssert(neighbour < NEIGHBOUR_VOLUME_COUNT); m_pNeighbourBlockData[neighbour] = pBlockData; }
    void SetTranslation(const float3 &basePosition) { m_translation = basePosition; }
    void SetScale(const float scale) { m_scale = scale; }
    void SetAmbientOcclusionEnabled(bool on) { m_ambientOcclusion = on; }
    void SetFromVolume(const BlockMeshVolume *pVolume);

    // output data
    const AABox &GetOutputBoundingBox() const { return m_outputBoundingBox; }
    const Sphere &GetOutputBoundingSphere() const { return m_outputBoundingSphere; }
    const VertexArray &GetOutputVertices() const { return m_outputVertices; }
    const uint32 GetOutputVertexCount() const { return m_outputVertices.GetSize(); }
    const TriangleArray &GetOutputTriangles() const { return m_outputTriangles; }
    const uint32 GetOutputTriangleCount() const { return m_outputTriangles.GetSize(); }
    const BatchArray &GetOutputBatches() const { return m_outputBatches; }
    const uint32 GetOutputBatchCount() const { return m_outputBatches.GetSize(); }

    // generate a render view of the chunk
    void GenerateMesh();

    // generate a render view of the chunk, at the specified lod level
    void GenerateLODMesh(uint32 lodLevel);

    // generate collision mesh
    void GenerateCollisionMesh();

    // generate a silhouette view of the chunk, used for shadow mapping
    void GenerateSilhouetteMesh();

    // create gpu buffers for the generated data
    bool CreateGPUBuffers(VertexBufferBindingArray *pVertexBuffers, uint32 *pVertexFactoryFlags, GPUBuffer **ppIndexBuffer, GPU_INDEX_FORMAT *pIndexFormat, uint32 inVertexFactoryFlags);

private:
    const BlockVolumeBlockType GetBlockValueAt(uint32 x, uint32 y, uint32 z) const;
    const BlockVolumeBlockType GetNeighbourBlockValueAt(NEIGHBOUR_VOLUME neighbour, uint32 x, uint32 y, uint32 z) const;
    const bool IsVisibleBlockAt(uint32 x, uint32 y, uint32 z) const;
    const bool IsVisibleNonTransparentBlockAt(uint32 x, uint32 y, uint32 z) const;

    const bool HasCubeLightBlockingBlockAt(uint32 x, uint32 y, uint32 z) const;
    const bool HasCubeVisibilityBlockingBlockAt(const BlockPalette::BlockType *pVolumeBlockType, int32 x, int32 y, int32 z) const;

    const uint32 CalculateCubeVertexColor(const BlockPalette::BlockType *pBlockType, const uint32 vertexIndex, const uint32 blockData, uint32 faceColor);
    void GenerateBlocks(float3 &outMinBounds, float3 &outMaxBounds);
    void GenerateCollisionBlocks(float3 &outMinBounds, float3 &outMaxBounds);
    void GenerateSilhouetteBlocks(float3 &outMinBounds, float3 &outMaxBounds);

    void OptimizeTriangleOrder();
    void GenerateBatches();

    // input data
    const BlockPalette *m_pPalette;
    uint32 m_width;
    uint32 m_length;
    uint32 m_height;
    const BlockVolumeBlockType *m_pBlockData;
    const BlockVolumeBlockType *m_pNeighbourBlockData[NEIGHBOUR_VOLUME_COUNT];
    float3 m_translation;
    float m_scale;
    bool m_ambientOcclusion;

    // output data
    uint32 m_outputVertexFactoryFlags;
    AABox m_outputBoundingBox;
    Sphere m_outputBoundingSphere;
    VertexArray m_outputVertices;
    TriangleArray m_outputTriangles;
    BatchArray m_outputBatches;
};

