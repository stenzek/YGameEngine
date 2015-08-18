#include "Renderer/PrecompiledHeader.h"
#include "Renderer/VertexFactories/LocalVertexFactory.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/VertexBufferBindingArray.h"

DEFINE_VERTEX_FACTORY_TYPE_INFO(LocalVertexFactory);
BEGIN_SHADER_COMPONENT_PARAMETERS(LocalVertexFactory)
END_SHADER_COMPONENT_PARAMETERS()

uint32 LocalVertexFactory::GetVertexSize(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags)
{
    // calculate vertices buffer size
    // base vertex size - position + normal
    uint32 vertexSize = sizeof(float3) + sizeof(float3);
    
    // add tangent space
    if (flags & LOCAL_VERTEX_FACTORY_FLAG_TANGENT_VECTORS)
        vertexSize += sizeof(float3) + sizeof(float3);

    // add texcoords
    if (flags & LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT2_TEXCOORDS)
        vertexSize += sizeof(float2);
    else if (flags & LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT3_TEXCOORDS)
        vertexSize += sizeof(float3);

    // add colors
    if (flags & LOCAL_VERTEX_FACTORY_FLAG_VERTEX_COLORS)
        vertexSize += sizeof(uint32);

    // set it
    return vertexSize;
}

bool LocalVertexFactory::CreateVerticesBuffer(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const Vertex *pVertices, uint32 nVertices, VertexBufferBindingArray *pVertexBufferBindingArray)
{
    uint32 vertexSize = GetVertexSize(platform, featureLevel, flags);
    uint32 bufferSize = vertexSize * nVertices;
    byte *pBuffer = new byte[bufferSize];
    FillVerticesBuffer(platform, featureLevel, flags, pVertices, nVertices, pBuffer, bufferSize);

    GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER, bufferSize);
    GPUBuffer *pVertexBuffer = g_pRenderer->CreateBuffer(&bufferDesc, pBuffer);
    delete[] pBuffer;

    if (pVertexBuffer == NULL)
        return false;

    pVertexBufferBindingArray->SetBuffer(0, pVertexBuffer, 0, vertexSize);
    pVertexBuffer->Release();
    return true;
}

bool LocalVertexFactory::FillVerticesBuffer(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const Vertex *pVertices, uint32 nVertices, void *pBuffer, uint32 cbBuffer)
{
    uint32 vertexSize = GetVertexSize(platform, featureLevel, flags);
    if (cbBuffer < (vertexSize * nVertices))
        return false;

    uint32 i;
    byte *pOutVertexPtr = reinterpret_cast<byte *>(pBuffer);
    for (i = 0; i < nVertices; i++)
    {
        const Vertex *pVertex = &pVertices[i];

        // position
        Y_memcpy(pOutVertexPtr, &pVertex->Position, sizeof(float3)); 
        pOutVertexPtr += sizeof(float3);
        
        // normal
        Y_memcpy(pOutVertexPtr, &pVertex->Normal, sizeof(float3));
        pOutVertexPtr += sizeof(float3);

        if (flags & LOCAL_VERTEX_FACTORY_FLAG_TANGENT_VECTORS)
        {
            // tangent
            Y_memcpy(pOutVertexPtr, &pVertex->Tangent, sizeof(float3));
            pOutVertexPtr += sizeof(float3);

            // binormal
            Y_memcpy(pOutVertexPtr, &pVertex->Binormal, sizeof(float3));
            pOutVertexPtr += sizeof(float3);
        }

        // texcoord
        if (flags & LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT3_TEXCOORDS)
        {
            Y_memcpy(pOutVertexPtr, &pVertex->TexCoord, sizeof(float3));
            pOutVertexPtr += sizeof(float3);
        }
        else if (flags & LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT2_TEXCOORDS)
        {
            Y_memcpy(pOutVertexPtr, &pVertex->TexCoord, sizeof(float2));
            pOutVertexPtr += sizeof(float2);
        }

        // color
        if (flags & LOCAL_VERTEX_FACTORY_FLAG_VERTEX_COLORS)
        {
            Y_memcpy(pOutVertexPtr, &pVertex->Color, sizeof(uint32));
            pOutVertexPtr += sizeof(uint32);
        }
    }

    return true;
}

bool LocalVertexFactory::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    return true;
}

bool LocalVertexFactory::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetVertexFactoryFileName("shaders/base/LocalVertexFactory.hlsl");

    if (vertexFactoryFlags & LOCAL_VERTEX_FACTORY_FLAG_TANGENT_VECTORS)
        pParameters->AddPreprocessorMacro("LOCAL_VERTEX_FACTORY_FLAG_TANGENT_VECTORS", "1");

    if (vertexFactoryFlags & LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT2_TEXCOORDS)
        pParameters->AddPreprocessorMacro("LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT2_TEXCOORDS", "1");

    if (vertexFactoryFlags & LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT3_TEXCOORDS)
        pParameters->AddPreprocessorMacro("LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT3_TEXCOORDS", "1");

    if (vertexFactoryFlags & LOCAL_VERTEX_FACTORY_FLAG_VERTEX_COLORS)
        pParameters->AddPreprocessorMacro("LOCAL_VERTEX_FACTORY_FLAG_VERTEX_COLORS", "1");

    if (vertexFactoryFlags & LOCAL_VERTEX_FACTORY_FLAG_INSTANCING_BY_MATRIX)
        pParameters->AddPreprocessorMacro("LOCAL_VERTEX_FACTORY_FLAG_INSTANCING_BY_MATRIX", "1");

    return true;
}

uint32 LocalVertexFactory::GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS])
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

        // normal
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_NORMAL;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT3;
        pElementDesc->StreamIndex = nStreams;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(float3);
        pElementDesc++;
        nElements++;

        // tangent space
        if (flags & LOCAL_VERTEX_FACTORY_FLAG_TANGENT_VECTORS)
        {
            // tangent
            pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_TANGENT;
            pElementDesc->SemanticIndex = 0;
            pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT3;
            pElementDesc->StreamIndex = nStreams;
            pElementDesc->StreamOffset = streamOffset;
            pElementDesc->InstanceStepRate = 0;
            streamOffset += sizeof(float3);
            pElementDesc++;
            nElements++;

            // binormal
            pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_BINORMAL;
            pElementDesc->SemanticIndex = 0;
            pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT3;
            pElementDesc->StreamIndex = nStreams;
            pElementDesc->StreamOffset = streamOffset;
            pElementDesc->InstanceStepRate = 0;
            streamOffset += sizeof(float3);
            pElementDesc++;
            nElements++;
        }

        // texcoord
        if (flags & LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT3_TEXCOORDS)
        {
            pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD;
            pElementDesc->SemanticIndex = 0;
            pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT3;
            pElementDesc->StreamIndex = nStreams;
            pElementDesc->StreamOffset = streamOffset;
            pElementDesc->InstanceStepRate = 0;
            streamOffset += sizeof(float3);
            pElementDesc++;
            nElements++;
        }
        else if (flags & LOCAL_VERTEX_FACTORY_FLAG_VERTEX_FLOAT2_TEXCOORDS)
        {
            pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD;
            pElementDesc->SemanticIndex = 0;
            pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT2;
            pElementDesc->StreamIndex = nStreams;
            pElementDesc->StreamOffset = streamOffset;
            pElementDesc->InstanceStepRate = 0;
            streamOffset += sizeof(float2);
            pElementDesc++;
            nElements++;
        }

        // color
        if (flags & LOCAL_VERTEX_FACTORY_FLAG_VERTEX_COLORS)
        {
            pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_COLOR;
            pElementDesc->SemanticIndex = 0;
            pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_UNORM4;
            pElementDesc->StreamIndex = nStreams;
            pElementDesc->StreamOffset = streamOffset;
            pElementDesc->InstanceStepRate = 0;
            streamOffset += sizeof(uint32);
            pElementDesc++;
            nElements++;
        }

        // end of stream
        nStreams++;
    }

    // build stream 1
    // lightmap coords
    if (flags & LOCAL_VERTEX_FACTORY_FLAG_LIGHTMAP_TEXCOORD_STREAM)
    {
        streamOffset = 0;

        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD;
        pElementDesc->SemanticIndex = 1;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT2;
        pElementDesc->StreamIndex = nStreams;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(float2);
        pElementDesc++;
        nElements++;

        nStreams++;
    }

    // build stream 2
    // instance transforms
    if (flags & LOCAL_VERTEX_FACTORY_FLAG_INSTANCING_BY_MATRIX)
    {
        streamOffset = 0;

        // row 1 of transform
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_POSITION;
        pElementDesc->SemanticIndex = 1;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT4;
        pElementDesc->StreamIndex = nStreams;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 1;
        streamOffset += sizeof(float4);
        pElementDesc++;
        nElements++;

        // row 2 of transform
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_POSITION;
        pElementDesc->SemanticIndex = 2;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT4;
        pElementDesc->StreamIndex = nStreams;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 1;
        streamOffset += sizeof(float4);
        pElementDesc++;
        nElements++;

        // row 3 of transform
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_POSITION;
        pElementDesc->SemanticIndex = 3;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT4;
        pElementDesc->StreamIndex = nStreams;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 1;
        streamOffset += sizeof(float4);
        pElementDesc++;
        nElements++;

        nStreams++;
    }

    // done
    return nElements;
}
