#include "BlockEngine/PrecompiledHeader.h"
#include "BlockEngine/BlockWorldVertexFactory.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/Renderer.h"
#include "Core/MeshUtilties.h"

DEFINE_VERTEX_FACTORY_TYPE_INFO(BlockWorldVertexFactory);
BEGIN_SHADER_COMPONENT_PARAMETERS(BlockWorldVertexFactory)
END_SHADER_COMPONENT_PARAMETERS()

bool BlockWorldVertexFactory::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    return true;
}

bool BlockWorldVertexFactory::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetVertexFactoryFileName("shaders/base/BlockWorldVertexFactory.hlsl");
    return true;
}

uint32 BlockWorldVertexFactory::GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS])
{
    // create format declaration
    GPU_VERTEX_ELEMENT_DESC *pElementDesc = pElementsDesc;
    uint32 streamOffset;
    uint32 nElements = 0;
    uint32 nStreams = 0;

    // build stream 0
    {
        streamOffset = 0;

        // position
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_POSITION;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT3;
        pElementDesc->StreamIndex = nStreams;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(float3);
        pElementDesc++;
        nElements++;

        // texcoord
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT3;
        pElementDesc->StreamIndex = nStreams;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(float3);
        pElementDesc++;
        nElements++;

        // color
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_COLOR;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_UNORM4;
        pElementDesc->StreamIndex = nStreams;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(uint32);
        pElementDesc++;
        nElements++;

        // tangent
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_TANGENT;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_SNORM4;
        pElementDesc->StreamIndex = nStreams;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(uint32);
        pElementDesc++;
        nElements++;

        // normal
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_NORMAL;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_SNORM4;
        pElementDesc->StreamIndex = nStreams;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(uint32);
        pElementDesc++;
        nElements++;

        // end of stream
        nStreams++;
    }

    // done
    return nElements;
}

BlockWorldVertexFactory::Vertex::Vertex(const float3 &position, const float3 &texcoord, uint32 color, const float3 &tangent, const float3 &binormal, const float3 &normal)
{
    Set(position, texcoord, color, tangent, binormal, normal);
}

BlockWorldVertexFactory::Vertex::Vertex(const float3 &position, const float3 &texcoord, uint32 color, uint32 packedTangentAndSign, uint32 packedNormal)
{
    Set(position, texcoord, color, packedTangentAndSign, packedNormal);
}

void BlockWorldVertexFactory::Vertex::Set(const float3 &position, const float3 &texcoord, uint32 color, const float3 &tangent, const float3 &binormal, const float3 &normal)
{
    Position = position;
    TexCoord = texcoord;
    Color = color;
    //Tangent = tangent;
    //Binormal = binormal;
    //Normal = normal;

    //float3x3 temp;
    //temp.SetRow(0, tangent);
    //temp.SetRow(1, binormal);
    //temp.SetRow(2, normal);
    //float det = temp.Determinant();

    float3 orthonormalizedTangent;
    float binormalSign;
    MeshUtilites::OrthogonalizeTangent(tangent, binormal, normal, orthonormalizedTangent, binormalSign);
    
    //TangentAndSign = PixelFormatHelpers::ConvertFloat4ToRGBA(float4(tangent * 0.5f + 0.5f, Math::Sign(det) * 0.5f + 0.5f));
    //TangentAndSign = PixelFormatHelpers::ConvertFloat4ToRGBA(float4(orthonormalizedTangent, Math::Sign(binormalSign)) * 0.5f + 0.5f);
    //Normal = PixelFormatHelpers::ConvertFloat4ToRGBA(float4(normal * 0.5f + 0.5f, 0.0f));

    union
    {
        uint32 asUInt32;
        int8 asInt8[4];
    } converter;
    converter.asInt8[0] = (int8)Math::Clamp(tangent.x * 127.0f, -127.0f, 127.0f);
    converter.asInt8[1] = (int8)Math::Clamp(tangent.y * 127.0f, -127.0f, 127.0f);
    converter.asInt8[2] = (int8)Math::Clamp(tangent.z * 127.0f, -127.0f, 127.0f);
    converter.asInt8[3] = (int8)Math::Clamp(binormalSign * 127.0f, -127.0f, 127.0f);
    TangentAndSign = converter.asUInt32;

    converter.asInt8[0] = (int8)Math::Clamp(normal.x * 127.0f, -127.0f, 127.0f);
    converter.asInt8[1] = (int8)Math::Clamp(normal.y * 127.0f, -127.0f, 127.0f);
    converter.asInt8[2] = (int8)Math::Clamp(normal.z * 127.0f, -127.0f, 127.0f);
    converter.asInt8[3] = (int8)0;
    Normal = converter.asUInt32;
}

void BlockWorldVertexFactory::Vertex::Set(const float3 &position, const float3 &texcoord, uint32 color, uint32 packedTangentAndSign, uint32 packedNormal)
{
    Position = position;
    TexCoord = texcoord;
    Color = color;
    TangentAndSign = packedTangentAndSign;
    Normal = packedNormal;
}
