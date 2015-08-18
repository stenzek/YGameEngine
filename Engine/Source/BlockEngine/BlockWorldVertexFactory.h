#pragma once
#include "Renderer/VertexFactory.h"

class BlockWorldVertexFactory : public VertexFactory
{
    DECLARE_VERTEX_FACTORY_TYPE_INFO(BlockWorldVertexFactory, VertexFactory);

public:
#pragma pack(push, 1)
    struct Vertex
    {
        float3 Position;
        float3 TexCoord;
        uint32 Color;
        //float3 Tangent;
        //float3 Binormal;
        //float3 Normal;
        uint32 TangentAndSign;
        uint32 Normal;

        Vertex() {}
        Vertex(const float3 &position, const float3 &texcoord, uint32 color, const float3 &tangent, const float3 &binormal, const float3 &normal);
        Vertex(const float3 &position, const float3 &texcoord, uint32 color, uint32 packedTangentAndSign, uint32 packedNormal);
        Vertex(const Vertex &v) { Y_memcpy(this, &v, sizeof(*this)); }
        void Set(const float3 &position, const float3 &texcoord, uint32 color, const float3 &tangent, const float3 &binormal, const float3 &normal);
        void Set(const float3 &position, const float3 &texcoord, uint32 color, uint32 packedTangentAndSign, uint32 packedNormal);
    };
#pragma pack(pop)

public:
    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);

    static uint32 GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS]);
};
