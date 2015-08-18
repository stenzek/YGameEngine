#pragma once
#include "Renderer/VertexFactory.h"

enum LOCAL_VERTEX_FACTORY_FLAGS
{
    LOCAL_VERTEX_FACTORY_FLAG_TANGENT_VECTORS               = (1 << 0),
    LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT2_TEXCOORDS       = (1 << 1),
    LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT3_TEXCOORDS       = (1 << 2),
    LOCAL_VERTEX_FACTORY_FLAG_VERTEX_COLORS                 = (1 << 3),
    LOCAL_VERTEX_FACTORY_FLAG_LIGHTMAP_TEXCOORD_STREAM      = (1 << 4),
    LOCAL_VERTEX_FACTORY_FLAG_INSTANCING_BY_MATRIX          = (1 << 5),
};

class LocalVertexFactory : public VertexFactory
{
    DECLARE_VERTEX_FACTORY_TYPE_INFO(LocalVertexFactory, VertexFactory);

public:
    struct Vertex
    {
        float3 Position;
        float3 Normal;
        float3 Tangent;
        float3 Binormal;
        float3 TexCoord;
        uint32 Color;
    };

    struct InstanceTransform
    {
        float4 InstanceTransformMatrix0;
        float4 InstanceTransformMatrix1;
        float4 InstanceTransformMatrix2;
    };

public:
    static uint32 GetVertexSize(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags);
    static bool CreateVerticesBuffer(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const Vertex *pVertices, uint32 nVertices, VertexBufferBindingArray *pVertexBufferBindingArray);
    static bool FillVerticesBuffer(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const Vertex *pVertices, uint32 nVertices, void *pBuffer, uint32 cbBuffer);
    static void ShareVerticesBuffer(VertexBufferBindingArray *pDestinationVertexArray, VertexBufferBindingArray *pSourceVertexArray) { ShareBuffers(pDestinationVertexArray, pSourceVertexArray, 0); }

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);

    static uint32 GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS]);
};

