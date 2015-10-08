#include "Renderer/PrecompiledHeader.h"
#include "Renderer/VertexFactories/SkeletalMeshVertexFactory.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Renderer/ShaderProgram.h"

DEFINE_VERTEX_FACTORY_TYPE_INFO(SkeletalMeshVertexFactory);
BEGIN_SHADER_COMPONENT_PARAMETERS(SkeletalMeshVertexFactory)
    DEFINE_SHADER_COMPONENT_PARAMETER("BoneMatrices", SHADER_PARAMETER_TYPE_FLOAT3X4)
    DEFINE_SHADER_COMPONENT_PARAMETER("BoneMatrices4x4", SHADER_PARAMETER_TYPE_FLOAT4X4)
END_SHADER_COMPONENT_PARAMETERS()

void SkeletalMeshVertexFactory::SetBoneMatrices(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, uint32 firstBoneIndex, uint32 boneCount, const float3x4 *pBoneMatrices)
{
    pShaderProgram->SetVertexFactoryParameterValueArray(pCommandList, 0, SHADER_PARAMETER_TYPE_FLOAT3X4, pBoneMatrices, firstBoneIndex, boneCount);
    if (g_pRenderer->GetFeatureLevel() == RENDERER_FEATURE_LEVEL_ES2)
    {
        // Fixme, please, this is disgusting.
        for (uint32 i = 0; i < boneCount; i++)
        {
            float4x4 bm44(pBoneMatrices[i]);
            pShaderProgram->SetVertexFactoryParameterValueArray(pCommandList, 1, SHADER_PARAMETER_TYPE_FLOAT4X4, &bm44, firstBoneIndex + i, 1);
        }
    }
}

uint32 SkeletalMeshVertexFactory::GetVertexSize(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags)
{
    // calculate vertices buffer size
    // base vertex size - position + tangents
    uint32 vertexSize = sizeof(float3) + sizeof(float3) + sizeof(float3) + sizeof(float3);

    // add texcoords
    if (flags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_TEXCOORDS)
        vertexSize += sizeof(float2);

    // add colors
    if (flags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS)
        vertexSize += sizeof(uint32);

    // add gpu skinning
    if (flags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_GPU_SKINNING)
        vertexSize += sizeof(uint8) * 4 + sizeof(float) * 4;

    // set it
    return vertexSize;
}

bool SkeletalMeshVertexFactory::CreateVerticesBuffer(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const Vertex *pVertices, uint32 nVertices, VertexBufferBindingArray *pVertexBufferBindingArray)
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

bool SkeletalMeshVertexFactory::FillVerticesBuffer(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const Vertex *pVertices, uint32 nVertices, void *pBuffer, uint32 cbBuffer)
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

        // tangentx
        Y_memcpy(pOutVertexPtr, &pVertex->TangentX, sizeof(float3));
        pOutVertexPtr += sizeof(float3);

        // tangenty
        Y_memcpy(pOutVertexPtr, &pVertex->TangentY, sizeof(float3));
        pOutVertexPtr += sizeof(float3);

        // tangentz
        Y_memcpy(pOutVertexPtr, &pVertex->TangentZ, sizeof(float3));
        pOutVertexPtr += sizeof(float3);

        // texcoord
        if (flags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_TEXCOORDS)
        {
            Y_memcpy(pOutVertexPtr, &pVertex->TexCoord, sizeof(float2));
            pOutVertexPtr += sizeof(float2);
        }

        // color
        if (flags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS)
        {
            Y_memcpy(pOutVertexPtr, &pVertex->Color, sizeof(uint32));
            pOutVertexPtr += sizeof(uint32);
        }

        if (flags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_GPU_SKINNING)
        {
            Y_memcpy(pOutVertexPtr, pVertex->BoneIndices, sizeof(uint8) * 4);
            pOutVertexPtr += sizeof(uint8) * 4;
            Y_memcpy(pOutVertexPtr, pVertex->BoneWeights, sizeof(float) * 4);
            pOutVertexPtr += sizeof(float) * 4;
        }
    }

    return true;
}

bool SkeletalMeshVertexFactory::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    return true;
}

bool SkeletalMeshVertexFactory::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetVertexFactoryFileName("shaders/base/SkeletalMeshVertexFactory.hlsl");

    if (vertexFactoryFlags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_TEXCOORDS)
        pParameters->AddPreprocessorMacro("SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_TEXCOORDS", "1");

    if (vertexFactoryFlags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS)
        pParameters->AddPreprocessorMacro("SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS", "1");

    if (vertexFactoryFlags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_GPU_SKINNING)
        pParameters->AddPreprocessorMacro("SKELETAL_MESH_VERTEX_FACTORY_FLAG_GPU_SKINNING", "1");

    if (vertexFactoryFlags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_0_ENABLED)
        pParameters->AddPreprocessorMacro("SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_0_ENABLED", "1");

    if (vertexFactoryFlags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_1_ENABLED)
        pParameters->AddPreprocessorMacro("SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_1_ENABLED", "1");

    if (vertexFactoryFlags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_2_ENABLED)
        pParameters->AddPreprocessorMacro("SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_2_ENABLED", "1");

    if (vertexFactoryFlags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_3_ENABLED)
        pParameters->AddPreprocessorMacro("SKELETAL_MESH_VERTEX_FACTORY_FLAG_WEIGHT_3_ENABLED", "1");

    return true;
}

uint32 SkeletalMeshVertexFactory::GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS])
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

        // tangentx
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_TANGENT;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT3;
        pElementDesc->StreamIndex = nStreams;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(float3);
        pElementDesc++;
        nElements++;

        // tangenty
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_BINORMAL;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT3;
        pElementDesc->StreamIndex = nStreams;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(float3);
        pElementDesc++;
        nElements++;

        // tangentz
        pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_NORMAL;
        pElementDesc->SemanticIndex = 0;
        pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT3;
        pElementDesc->StreamIndex = nStreams;
        pElementDesc->StreamOffset = streamOffset;
        pElementDesc->InstanceStepRate = 0;
        streamOffset += sizeof(float3);
        pElementDesc++;
        nElements++;

        // texcoord
        if (flags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_TEXCOORDS)
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
        if (flags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS)
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

        // color
        if (flags & SKELETAL_MESH_VERTEX_FACTORY_FLAG_GPU_SKINNING)
        {
            pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_BLENDINDICES;
            pElementDesc->SemanticIndex = 0;
            pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_UBYTE4;
            pElementDesc->StreamIndex = nStreams;
            pElementDesc->StreamOffset = streamOffset;
            pElementDesc->InstanceStepRate = 0;
            streamOffset += sizeof(uint8) * 4;
            pElementDesc++;
            nElements++;

            pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_BLENDWEIGHTS;
            pElementDesc->SemanticIndex = 0;
            pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT4;
            pElementDesc->StreamIndex = nStreams;
            pElementDesc->StreamOffset = streamOffset;
            pElementDesc->InstanceStepRate = 0;
            streamOffset += sizeof(float) * 4;
            pElementDesc++;
            nElements++;
        }

        // end of stream
        nStreams++;
    }

    // done
    return nElements;
}
