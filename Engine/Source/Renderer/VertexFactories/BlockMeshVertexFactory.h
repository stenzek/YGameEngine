#pragma once
#include "Renderer/VertexFactory.h"
#include "Engine/BlockMeshBuilder.h"

enum BLOCK_MESH_VERTEX_FACTORY_FLAGS
{
    BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXCOORD                         = (1 << 0),
    BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXTURE_ARRAY                    = (1 << 1),
    BLOCK_MESH_VERTEX_FACTORY_FLAG_ATLAS_TEXCOORDS                  = (1 << 2),
    BLOCK_MESH_VERTEX_FACTORY_FLAG_TANGENT_VECTORS                  = (1 << 3),
    BLOCK_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS                    = (1 << 4),
    BLOCK_MESH_VERTEX_FACTORY_FLAG_FACE_INDEX                       = (1 << 5),
};

class BlockMeshVertexFactory : public VertexFactory
{
    DECLARE_VERTEX_FACTORY_TYPE_INFO(BlockMeshVertexFactory, VertexFactory);

public:
    static uint32 GetVertexSize(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags);

    static void FillVertices(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const BlockMeshBuilder::Vertex *pVertices, uint32 nVertices, void *pOutputVertices, uint32 cbOutputVertices);
    static bool CreateVerticesBuffer(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const BlockMeshBuilder::Vertex *pVertices, uint32 nVertices, VertexBufferBindingArray *pVertexBufferBindingArray);
    static void ShareVerticesBuffer(VertexBufferBindingArray *pDestinationVertexArray, VertexBufferBindingArray *pSourceVertexArray) { ShareBuffers(pDestinationVertexArray, pSourceVertexArray, 0); }

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);

    static uint32 GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS]);

    // determine flags for current renderer
    static uint32 GetVertexFlagsForCurrentRenderer();
};
