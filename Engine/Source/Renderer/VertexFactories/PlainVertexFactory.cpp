#include "Renderer/PrecompiledHeader.h"
#include "Renderer/VertexFactories/PlainVertexFactory.h"
#include "Renderer/ShaderCompilerFrontend.h"

void PlainVertexFactory::Vertex::SetUV(float x_, float y_, float z_, float u_, float v_)
{
    this->Position.Set(x_, y_, z_);
    this->TexCoord.Set(u_, v_);
    this->Color = 0x00000000;
}

void PlainVertexFactory::Vertex::SetUVColor(float x_, float y_, float z_, float u_, float v_, uint32 color)
{
    this->Position.Set(x_, y_, z_);
    this->TexCoord.Set(u_, v_);
    this->Color = color;
}

void PlainVertexFactory::Vertex::SetUVColorFloat(float x_, float y_, float z_, float u_, float v_, float r_, float g_, float b_, float a_)
{
    this->Position.Set(x_, y_, z_);
    this->TexCoord.Set(u_, v_);
    this->Color = static_cast<uint32>(Y_fclamp(r_ * 255.0f, 0.0f, 255.0f)) |
                  static_cast<uint32>(Y_fclamp(g_ * 255.0f, 0.0f, 255.0f)) << 8 |
                  static_cast<uint32>(Y_fclamp(b_ * 255.0f, 0.0f, 255.0f)) << 16 |
                  static_cast<uint32>(Y_fclamp(a_ * 255.0f, 0.0f, 255.0f)) << 24;
}

void PlainVertexFactory::Vertex::SetUVColorByte(float x_, float y_, float z_, float u_, float v_, uint8 r_, uint8 g_, uint8 b_, uint8 a_)
{
    this->Position.Set(x_, y_, z_);
    this->TexCoord.Set(u_, v_);
    this->Color = static_cast<uint32>(r_) |
                  static_cast<uint32>(g_) << 8 |
                  static_cast<uint32>(b_) << 16 |
                  static_cast<uint32>(a_) << 24;
}

void PlainVertexFactory::Vertex::SetColor(float x_, float y_, float z_, uint32 color)
{
    this->Position.Set(x_, y_, z_);
    this->TexCoord.SetZero();
    this->Color = color;
}

void PlainVertexFactory::Vertex::SetColorFloat(float x_, float y_, float z_, float r_, float g_, float b_, float a_)
{
    this->Position.Set(x_, y_, z_);
    this->TexCoord.SetZero();
    this->Color = static_cast<uint32>(Y_fclamp(r_ * 255.0f, 0.0f, 255.0f)) |
                  static_cast<uint32>(Y_fclamp(g_ * 255.0f, 0.0f, 255.0f)) << 8 |
                  static_cast<uint32>(Y_fclamp(b_ * 255.0f, 0.0f, 255.0f)) << 16 |
                  static_cast<uint32>(Y_fclamp(a_ * 255.0f, 0.0f, 255.0f)) << 24;
}

void PlainVertexFactory::Vertex::SetColorByte(float x_, float y_, float z_, uint8 r_, uint8 g_, uint8 b_, uint8 a_)
{
    this->Position.Set(x_, y_, z_);
    this->TexCoord.SetZero();
    this->Color = static_cast<uint32>(r_) |
                  static_cast<uint32>(g_) << 8 |
                  static_cast<uint32>(b_) << 16 |
                  static_cast<uint32>(a_) << 24;
}

void PlainVertexFactory::Vertex::Set(const float3 &Position_, const float2 &TexCoord_, const float4 &Color_)
{
    this->Position = Position_;
    this->TexCoord = TexCoord_;
    this->Color = static_cast<uint32>(Y_fclamp(Color_.r * 255.0f, 0.0f, 255.0f)) |
                  static_cast<uint32>(Y_fclamp(Color_.g * 255.0f, 0.0f, 255.0f)) << 8 |
                  static_cast<uint32>(Y_fclamp(Color_.b * 255.0f, 0.0f, 255.0f)) << 16 |
                  static_cast<uint32>(Y_fclamp(Color_.a * 255.0f, 0.0f, 255.0f)) << 24;
}

void PlainVertexFactory::Vertex::Set(const float3 &Position_, const float2 &TexCoord_, const uint32 Color_)
{
    this->Position = Position_;
    this->TexCoord = TexCoord_;
    this->Color = Color_;
}

bool operator==(const PlainVertexFactory::Vertex &v1, const PlainVertexFactory::Vertex &v2)
{
    return (v1.Position == v2.Position &&
        v1.TexCoord == v2.TexCoord &&
        v1.Color == v2.Color);
}

bool operator!=(const PlainVertexFactory::Vertex &v1, const PlainVertexFactory::Vertex &v2)
{
    return (v1.Position != v2.Position ||
        v1.TexCoord != v2.TexCoord ||
        v1.Color != v2.Color);
}

DEFINE_VERTEX_FACTORY_TYPE_INFO(PlainVertexFactory);
BEGIN_SHADER_COMPONENT_PARAMETERS(PlainVertexFactory)
END_SHADER_COMPONENT_PARAMETERS()

uint32 PlainVertexFactory::GetVertexSize(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags)
{
    uint32 vertexSize = sizeof(float3);

    if (flags & PLAIN_VERTEX_FACTORY_FLAG_TEXCOORD)
        vertexSize += sizeof(float2);

    if (flags & PLAIN_VERTEX_FACTORY_FLAG_COLOR)
        vertexSize += sizeof(uint32);

    return vertexSize;
}

bool PlainVertexFactory::FillBuffer(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const Vertex *pVertices, uint32 nVertices, void *pBuffer, uint32 cbBuffer)
{
    uint32 vertexSize = GetVertexSize(platform, featureLevel, flags);
    if (cbBuffer < (nVertices * vertexSize))
        return false;

    uint32 i;
    const Vertex *pInVertexPtr = pVertices;
    byte *pOutVertexPtr = reinterpret_cast<byte *>(pBuffer);

    for (i = 0; i < nVertices; i++)
    {
        Y_memcpy(pOutVertexPtr, &pInVertexPtr->Position, sizeof(float3));
        pOutVertexPtr += sizeof(float3);

        if (flags & PLAIN_VERTEX_FACTORY_FLAG_TEXCOORD)
        {
            Y_memcpy(pOutVertexPtr, &pInVertexPtr->TexCoord, sizeof(float2));
            pOutVertexPtr += sizeof(float2);
        }

        if (flags & PLAIN_VERTEX_FACTORY_FLAG_COLOR)
        {
            // todo: d3d9: reverse byte order
            *(uint32 *)pOutVertexPtr = pInVertexPtr->Color;
            pOutVertexPtr += sizeof(uint32);
        }

        pInVertexPtr++;
    }

    return true;
}

bool PlainVertexFactory::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    return true;
}

bool PlainVertexFactory::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetVertexFactoryFileName("shaders/base/PlainVertexFactory.hlsl");
    
    if (vertexFactoryFlags & PLAIN_VERTEX_FACTORY_FLAG_TEXCOORD)
        pParameters->AddPreprocessorMacro("PLAIN_VERTEX_FACTORY_FLAG_TEXCOORD", "1");

    if (vertexFactoryFlags & PLAIN_VERTEX_FACTORY_FLAG_COLOR)
        pParameters->AddPreprocessorMacro("PLAIN_VERTEX_FACTORY_FLAG_COLOR", "1");

    return true;
}

uint32 PlainVertexFactory::GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS])
{
    GPU_VERTEX_ELEMENT_DESC *pElementDesc = &pElementsDesc[0];
    uint32 nElements = 0;
    uint32 streamOffset = 0;

    // position
    pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_POSITION;
    pElementDesc->SemanticIndex = 0;
    pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT3;
    pElementDesc->StreamIndex = 0;
    pElementDesc->StreamOffset = streamOffset;
    pElementDesc->InstanceStepRate = 0;
    streamOffset += sizeof(float3);
    pElementDesc++;
    nElements++;

    // texcoord
    if (flags & PLAIN_VERTEX_FACTORY_FLAG_TEXCOORD)
    {
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT2;
        pElementDesc->StreamIndex = 0;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(float2);
        pElementDesc++;
        nElements++;
    }

    // color
    if (flags & PLAIN_VERTEX_FACTORY_FLAG_COLOR)
    {
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_COLOR;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_UNORM4;
        pElementDesc->StreamIndex = 0;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(uint32);
        pElementDesc++;
        nElements++;
    }

    return nElements;
}

