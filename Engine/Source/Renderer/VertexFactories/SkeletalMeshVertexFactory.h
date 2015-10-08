#pragma once
#include "Renderer/VertexFactory.h"

class ShaderProgram;

enum SKELETAL_MESH_VERTEX_FACTORY_FLAGS
{
    SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_TEXCOORDS      = (1 << 0),
    SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS         = (1 << 1),
    SKELETAL_MESH_VERTEX_FACTORY_FLAG_GPU_SKINNING          = (1 << 2),
    SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_0_ENABLED      = (1 << 3),
    SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_1_ENABLED      = (1 << 4),
    SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_2_ENABLED      = (1 << 5),
    SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_3_ENABLED      = (1 << 6)
};

class SkeletalMeshVertexFactory : public VertexFactory
{
    DECLARE_VERTEX_FACTORY_TYPE_INFO(SkeletalMeshVertexFactory, VertexFactory);

public:
    struct Vertex
    {
        float3 Position;
        float3 TangentX;
        float3 TangentY;
        float3 TangentZ;
        float2 TexCoord;
        uint32 Color;
        uint8 BoneIndices[4];
        float BoneWeights[4];
    };

public:
    static void SetBoneMatrices(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, uint32 firstBoneIndex, uint32 boneCount, const float3x4 *pBoneMatrices);

    static uint32 GetVertexSize(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags);
    static bool CreateVerticesBuffer(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const Vertex *pVertices, uint32 nVertices, VertexBufferBindingArray *pVertexBufferBindingArray);
    static bool FillVerticesBuffer(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const Vertex *pVertices, uint32 nVertices, void *pBuffer, uint32 cbBuffer);
    static void ShareVerticesBuffer(VertexBufferBindingArray *pDestinationVertexArray, const VertexBufferBindingArray *pSourceVertexArray) { ShareBuffers(pDestinationVertexArray, pSourceVertexArray, 0); }

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);

    static uint32 GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS]);
};

