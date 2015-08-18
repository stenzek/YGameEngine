#pragma once
#include "Renderer/VertexFactory.h"

enum PLAIN_VERTEX_FACTORY_FLAGS
{
    PLAIN_VERTEX_FACTORY_FLAG_TEXCOORD          = (1 << 0),
    PLAIN_VERTEX_FACTORY_FLAG_COLOR             = (1 << 1),
};

class PlainVertexFactory : public VertexFactory
{
    DECLARE_VERTEX_FACTORY_TYPE_INFO(PlainVertexFactory, VertexFactory);

public:
    struct Vertex
    {
        float3 Position;
        float2 TexCoord;
        uint32 Color;

        void SetUV(float x_, float y_, float z_, float u_, float v_);
        void SetUVColor(float x_, float y_, float z_, float u_, float v_, uint32 color);
        void SetUVColorFloat(float x_, float y_, float z_, float u_, float v_, float r_, float g_, float b_, float a_);
        void SetUVColorByte(float x_, float y_, float z_, float u_, float v_, uint8 r_, uint8 g_, uint8 b_, uint8 a_);
        void SetColor(float x_, float y_, float z_, uint32 color);
        void SetColorFloat(float x_, float y_, float z_, float r_, float g_, float b_, float a_);
        void SetColorByte(float x_, float y_, float z_, uint8 r_, uint8 g_, uint8 b_, uint8 a_);

        void Set(const float3 &Position_, const float2 &TexCoord_, const float4 &Color_);
        void Set(const float3 &Position_, const float2 &TexCoord_, const uint32 Color_);
    };

public:
    static uint32 GetVertexSize(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags);
    static bool FillBuffer(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const Vertex *pVertices, uint32 nVertices, void *pBuffer, uint32 cbBuffer);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);

    static uint32 GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS]);
};

// operators for vertex array building
bool operator==(const PlainVertexFactory::Vertex &v1, const PlainVertexFactory::Vertex &v2);
bool operator!=(const PlainVertexFactory::Vertex &v1, const PlainVertexFactory::Vertex &v2);
