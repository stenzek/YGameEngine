#include "Renderer/PrecompiledHeader.h"
#include "Renderer/VertexFactories/BlockMeshVertexFactory.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/Renderer.h"
#include "Renderer/VertexBufferBindingArray.h"

DEFINE_VERTEX_FACTORY_TYPE_INFO(BlockMeshVertexFactory);
BEGIN_SHADER_COMPONENT_PARAMETERS(BlockMeshVertexFactory)
END_SHADER_COMPONENT_PARAMETERS()

uint32 BlockMeshVertexFactory::GetVertexSize(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags)
{
    // calculate vertices buffer size
    // base vertex size - position + tangent + binormal + normal + color
    uint32 vertexSize = sizeof(float3);

    // add texcoords
    if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXCOORD)
    {
        if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXTURE_ARRAY)
            vertexSize += sizeof(float3);
        else
            vertexSize += sizeof(float2);

        // add atlas texcoords
        if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_ATLAS_TEXCOORDS)
            vertexSize += sizeof(float4);
    }

    // add tangents
    if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_TANGENT_VECTORS)
        vertexSize += sizeof(float3) * 3;

    // add colour
    if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS)
        vertexSize += sizeof(uint32);

    // add face index
    if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_FACE_INDEX)
        vertexSize += sizeof(byte) * 4;

    // set it
    return vertexSize;
}

void BlockMeshVertexFactory::FillVertices(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const BlockMeshBuilder::Vertex *pVertices, uint32 nVertices, void *pOutputVertices, uint32 cbOutputVertices)
{
    DebugAssert((GetVertexSize(platform, featureLevel, flags) * nVertices) <= cbOutputVertices);

    // fill buffer
    {
        uint32 i;
        const BlockMeshBuilder::Vertex *pVertex = pVertices;
        byte *pOutVertexPtr = reinterpret_cast<byte *>(pOutputVertices);
        for (i = 0; i < nVertices; i++, pVertex++)
        {
            // position
            Y_memcpy(pOutVertexPtr, &pVertex->Position, sizeof(float3)); 
            pOutVertexPtr += sizeof(float3);

            if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXCOORD)
            {
                // texcoord
                if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXTURE_ARRAY)
                {
                    Y_memcpy(pOutVertexPtr, &pVertex->TexCoord, sizeof(float3));
                    pOutVertexPtr += sizeof(float3);
                }
                else
                {
                    Y_memcpy(pOutVertexPtr, &pVertex->TexCoord, sizeof(float2));
                    pOutVertexPtr += sizeof(float2);
                }

                // atlas texcoord
                if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_ATLAS_TEXCOORDS)
                {
                    Y_memcpy(pOutVertexPtr, &pVertex->AtlasTexCoord, sizeof(float4));
                    pOutVertexPtr += sizeof(float4);
                }
            }

            if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_TANGENT_VECTORS)
            {
                // todo: hardcoded tangent vectors
                pOutVertexPtr += sizeof(float3);
                pOutVertexPtr += sizeof(float3);
                pOutVertexPtr += sizeof(float3);
            }

            if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS)
            {
                // color
                *(uint32 *)pOutVertexPtr = pVertex->Color;
                pOutVertexPtr += sizeof(uint32);
            }

            if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_FACE_INDEX)
            {
                // data
                *(uint32 *)pOutVertexPtr = (uint32)pVertex->FaceIndex | (uint32)pVertex->FaceIndex << 8 | (uint32)pVertex->FaceIndex << 16 | (uint32)pVertex->FaceIndex << 24;
                pOutVertexPtr += sizeof(uint32);
            }
        }
    }
}

bool BlockMeshVertexFactory::CreateVerticesBuffer(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, const BlockMeshBuilder::Vertex *pVertices, uint32 nVertices, VertexBufferBindingArray *pVertexBufferBindingArray)
{
    uint32 vertexSize = GetVertexSize(platform, featureLevel, flags);
    uint32 bufferSize = vertexSize * nVertices;
    byte *pBuffer = new byte[bufferSize];
    FillVertices(platform, featureLevel, flags, pVertices, nVertices, pBuffer, bufferSize);    

    GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER, bufferSize);
    GPUBuffer *pVertexBuffer = g_pRenderer->CreateBuffer(&bufferDesc, pBuffer);
    delete[] pBuffer;

    if (pVertexBuffer == NULL)
        return false;

    pVertexBufferBindingArray->SetBuffer(0, pVertexBuffer, 0, vertexSize);
    pVertexBuffer->Release();
    return true;
}

bool BlockMeshVertexFactory::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    return true;
}

bool BlockMeshVertexFactory::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetVertexFactoryFileName("shaders/base/BlockMeshVertexFactory.hlsl");

    if (vertexFactoryFlags & BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXCOORD)
    {
        pParameters->AddPreprocessorMacro("BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXCOORD", "1");
        if (vertexFactoryFlags & BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXTURE_ARRAY)
            pParameters->AddPreprocessorMacro("BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXTURE_ARRAY", "1");
        if (vertexFactoryFlags & BLOCK_MESH_VERTEX_FACTORY_FLAG_ATLAS_TEXCOORDS)
            pParameters->AddPreprocessorMacro("BLOCK_MESH_VERTEX_FACTORY_FLAG_ATLAS_TEXCOORDS", "1");
    }
    if (vertexFactoryFlags & BLOCK_MESH_VERTEX_FACTORY_FLAG_TANGENT_VECTORS)
        pParameters->AddPreprocessorMacro("BLOCK_MESH_VERTEX_FACTORY_FLAG_TANGENT_VECTORS", "1");
    if (vertexFactoryFlags & BLOCK_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS)
        pParameters->AddPreprocessorMacro("BLOCK_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS", "1");
    if (vertexFactoryFlags & BLOCK_MESH_VERTEX_FACTORY_FLAG_FACE_INDEX)
        pParameters->AddPreprocessorMacro("BLOCK_MESH_VERTEX_FACTORY_FLAG_FACE_INDEX", "1");


    return true;
}

uint32 BlockMeshVertexFactory::GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS])
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
        
        if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXCOORD)
        {        
            // texcoord
            if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXTURE_ARRAY)
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
            else
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

            // atlas texcoord
            if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_ATLAS_TEXCOORDS)
            {
                pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD;
                pElementDesc->SemanticIndex = 1;
                pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT4;
                pElementDesc->StreamIndex = nStreams;
                pElementDesc->StreamOffset = streamOffset;
                pElementDesc->InstanceStepRate = 0;
                streamOffset += sizeof(float4);
                pElementDesc++;
                nElements++;
            }
        }

        if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_TANGENT_VECTORS)
        {
            pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_TANGENT;
            pElementDesc->SemanticIndex = 0;
            pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT3;
            pElementDesc->StreamIndex = nStreams;
            pElementDesc->StreamOffset = streamOffset;
            pElementDesc->InstanceStepRate = 0;
            streamOffset += sizeof(float3);
            pElementDesc++;
            nElements++;

            pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_BINORMAL;
            pElementDesc->SemanticIndex = 0;
            pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT3;
            pElementDesc->StreamIndex = nStreams;
            pElementDesc->StreamOffset = streamOffset;
            pElementDesc->InstanceStepRate = 0;
            streamOffset += sizeof(float3);
            pElementDesc++;
            nElements++;

            pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_NORMAL;
            pElementDesc->SemanticIndex = 0;
            pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_FLOAT3;
            pElementDesc->StreamIndex = nStreams;
            pElementDesc->StreamOffset = streamOffset;
            pElementDesc->InstanceStepRate = 0;
            streamOffset += sizeof(float3);
            pElementDesc++;
            nElements++;
        }

        // color
        if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS)
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

        // data
        if (flags & BLOCK_MESH_VERTEX_FACTORY_FLAG_FACE_INDEX)
        {
            pElementDesc->Semantic = GPU_VERTEX_ELEMENT_SEMANTIC_BLENDINDICES;
            pElementDesc->SemanticIndex = 0;
            pElementDesc->Type = GPU_VERTEX_ELEMENT_TYPE_UBYTE4;
            pElementDesc->StreamIndex = nStreams;
            pElementDesc->StreamOffset = streamOffset;
            pElementDesc->InstanceStepRate = 0;
            streamOffset += sizeof(byte) * 4;
            pElementDesc++;
            nElements++;
        }

        // end of stream
        nStreams++;
    }

    // done
    return nElements;
}

uint32 BlockMeshVertexFactory::GetVertexFlagsForCurrentRenderer()
{
    uint32 outFlags = BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXCOORD | BLOCK_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS;

    if (g_pRenderer->GetCapabilities().SupportsTextureArrays)
    {
        outFlags |= BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXTURE_ARRAY;
    }
    else
    {
        outFlags |= BLOCK_MESH_VERTEX_FACTORY_FLAG_ATLAS_TEXCOORDS;
    }

    outFlags |= BLOCK_MESH_VERTEX_FACTORY_FLAG_FACE_INDEX;

    return outFlags;
}
