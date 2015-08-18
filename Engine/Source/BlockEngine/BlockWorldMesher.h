#pragma once
#include "BlockEngine/BlockWorldTypes.h"
#include "BlockEngine/BlockWorldVertexFactory.h"
#include "Engine/BlockPalette.h"
#include "Renderer/RenderQueue.h"

class VertexBufferBindingArray;
class GPUBuffer;

class BlockWorldMesher
{
public:
    typedef BlockWorldVertexFactory::Vertex Vertex;

    struct FullVertex
    {
        float3 Position;
        float3 TexCoord;
        uint32 Color;
        float3 Tangent;
        float3 Binormal;
        float3 Normal;

        FullVertex() {}
        FullVertex(const float3 &position, const float3 &texcoord, uint32 color, const float3 &tangent, const float3 &binormal, const float3 &normal) : Position(position), TexCoord(texcoord), Color(color), Tangent(tangent), Binormal(binormal), Normal(normal) {}
        FullVertex(const FullVertex &v) { Y_memcpy(this, &v, sizeof(*this)); }
        void Set(const float3 &position, const float3 &texcoord, uint32 color, const float3 &tangent, const float3 &binormal, const float3 &normal) { Position = position; TexCoord = texcoord; Color = color; Tangent = tangent; Binormal = binormal; Normal = normal; }
    };

    struct Triangle
    {
        uint32 MaterialIndex;
        uint32 Indices[3];

        Triangle() 
        {

        }

        Triangle(uint32 materialIndex, uint32 i0, uint32 i1, uint32 i2)
        {
            MaterialIndex = materialIndex;
            Indices[0] = i0;
            Indices[1] = i1;
            Indices[2] = i2;
        }

        void Set(uint32 materialIndex, uint32 i0, uint32 i1, uint32 i2)
        {
            MaterialIndex = materialIndex;
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
    };

    struct MeshInstances
    {
        uint32 MeshIndex;
        MemArray<float3x4> Transforms;
        uint32 BufferOffset;
    };

    typedef MemArray<BlockWorldVertexFactory::Vertex> VertexArray;
    typedef MemArray<Triangle> TriangleArray;
    typedef MemArray<Batch> BatchArray;
    typedef PODArray<MeshInstances *> MeshInstancesArray;
    typedef MemArray<RENDER_QUEUE_POINT_LIGHT_ENTRY> LightArray;

    struct Output
    {
        AABox BoundingBox;
        Sphere BoundingSphere;
        VertexArray Vertices;
        TriangleArray Triangles;
        BatchArray Batches;
        MeshInstancesArray Instances;
        LightArray Lights;
    };

public:
    BlockWorldMesher(const BlockPalette *pPalette, uint32 chunkSize, uint32 lodLevel, const float3 &basePosition, bool generateLightMaps);
    ~BlockWorldMesher();

    // input data accessors
    const BlockPalette *GetPalette() const { return m_pPalette; }
    const int32 GetChunkSize() const { return m_chunkSize; }
    const int32 GetLODLevel() const { return m_lodLevel; }
    const float3 &GetBasePosition() const { return m_basePosition; }

    // input data mutators
    BlockWorldBlockType *GetBlockValues() { return m_pBlockValues; }
    BlockWorldBlockDataType *GetBlockData() { return m_pBlockData; }

    // output data
    const AABox &GetOutputBoundingBox() const { return m_output.BoundingBox; }
    const Sphere &GetOutputBoundingSphere() const { return m_output.BoundingSphere; }
    const VertexArray &GetOutputVertices() const { return m_output.Vertices; }
    const uint32 GetOutputVertexCount() const { return m_output.Vertices.GetSize(); }
    const TriangleArray &GetOutputTriangles() const { return m_output.Triangles; }
    const uint32 GetOutputTriangleCount() const { return m_output.Triangles.GetSize(); }
    const BatchArray &GetOutputBatches() const { return m_output.Batches; }
    const uint32 GetOutputBatchCount() const { return m_output.Batches.GetSize(); }
    const LightArray &GetOutputLights() const { return m_output.Lights; }
    const uint32 GetOutputLightCount() const { return m_output.Lights.GetSize(); }
    const MeshInstancesArray &GetOutputMeshInstances() const { return m_output.Instances; }
    const uint32 GetOutputMeshInstancesCount() const { return m_output.Instances.GetSize(); }

    // generate a render view of the chunk
    void GenerateMesh();

    // create gpu buffers for the generated data
    bool CreateGPUBuffers(VertexBufferBindingArray *pVertexBuffers, GPUBuffer **ppIndexBuffer, GPU_INDEX_FORMAT *pIndexFormat, GPUBuffer **ppInstanceTransformBuffer) { return CreateGPUBuffers(m_output, pVertexBuffers, ppIndexBuffer, pIndexFormat, ppInstanceTransformBuffer); }

    // create gpu buffers for the generated data
    static bool CreateGPUBuffers(const Output &output, VertexBufferBindingArray *pVertexBuffers, GPUBuffer **ppIndexBuffer, GPU_INDEX_FORMAT *pIndexFormat, GPUBuffer **ppInstanceTransformBuffer);

    // mesh a single block
    static bool MeshSingleBlock(const BlockPalette *pPalette, BlockWorldBlockType blockValue, Output &output);

private:
    const BlockWorldBlockType GetBlockValueAt(uint32 x, uint32 y, uint32 z) const;
    const bool IsVisibleBlockAt(uint32 x, uint32 y, uint32 z) const;
    const bool IsVisibleNonTransparentBlockAt(uint32 x, uint32 y, uint32 z) const;

    const bool HasCubeLightBlockingBlockAt(uint32 x, uint32 y, uint32 z) const;
    const bool CalculateBlockFaceVisibility(uint32 x, uint32 y, uint32 z, BlockWorldBlockType blockValue, CUBE_FACE face) const;

    static const uint32 CalculateCubeVertexColor(const BlockPalette::BlockType *pBlockType, CUBE_FACE faceIndex, const uint32 blockValue, const uint32 blockLighting);
    static const uint32 CalculatePlaneVertexColor(const BlockPalette::BlockType *pBlockType, const uint32 blockValue, const uint32 blockLighting);
    const uint32 CalculateCubeFaceLighting(uint32 x, uint32 y, uint32 z, CUBE_FACE faceIndex);

    static void AddCubeBlockFace(const BlockPalette::BlockType *pBlockType, BlockWorldBlockType blockValue, uint32 blockLighting, uint8 blockRotation, const float3 &basePosition, uint32 lodLevel, uint32 startX, uint32 startY, uint32 startZ, uint32 endX, uint32 endY, uint32 endZ, CUBE_FACE face, Output &output);
    static void AddSlabBlockFace(const BlockPalette::BlockType *pBlockType, BlockWorldBlockType blockValue, uint32 blockLighting, uint8 blockRotation, const float3 &basePosition, uint32 lodLevel, uint32 startX, uint32 startY, uint32 startZ, uint32 endX, uint32 endY, uint32 endZ, CUBE_FACE face, bool isTopSlab, Output &output);
    static void AddStairBlockFace(const BlockPalette::BlockType *pBlockType, BlockWorldBlockType blockValue, uint32 blockLighting, uint8 blockRotation, const float3 &basePosition, uint32 lodLevel, uint32 startX, uint32 startY, uint32 startZ, uint32 endX, uint32 endY, uint32 endZ, CUBE_FACE face, Output &output);
    static void AddPlaneBlock(const BlockPalette::BlockType *pBlockType, BlockWorldBlockType blockValue, uint32 blockLighting, uint8 blockRotation, const float3 &basePosition, uint32 lodLevel, uint32 x, uint32 y, uint32 z, Output &output);
    static void AddMeshBlock(const BlockPalette::BlockType *pBlockType, BlockWorldBlockType blockValue, uint32 blockLighting, uint8 blockRotation, const float3 &basePosition, uint32 lodLevel, uint32 x, uint32 y, uint32 z, Output &output);
    static void AddLightBlock(const BlockPalette::BlockType *pBlockType, const float3 &basePosition, uint32 lodLevel, uint32 x, uint32 y, uint32 z, Output &output);

    static void OptimizeTriangleOrder(Output &output);
    static void GenerateBatches(Output &output);

    void GenerateBlocks(uint3 &minBlockCoordinates, uint3 &maxBlockCoordinates);

    // input data
    const BlockPalette *m_pPalette;
    uint32 m_chunkSize;
    uint32 m_lodLevel;
    float3 m_basePosition;
    BlockWorldBlockType *m_pBlockValues;
    BlockWorldBlockDataType *m_pBlockData;
    uint8 *m_pBlockFaceMasks;
    bool m_generateLightMaps;

    // output data
    Output m_output;
};

