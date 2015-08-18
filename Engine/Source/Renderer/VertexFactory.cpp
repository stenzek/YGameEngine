#include "Renderer/PrecompiledHeader.h"
#include "Renderer/VertexFactory.h"
#include "Renderer/VertexBufferBindingArray.h"

DEFINE_VERTEX_FACTORY_TYPE_INFO(VertexFactory);
BEGIN_SHADER_COMPONENT_PARAMETERS(VertexFactory)
END_SHADER_COMPONENT_PARAMETERS()

bool VertexFactory::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    return false;
}

bool VertexFactory::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    return false;
}

uint32 VertexFactory::GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS])
{
    return 0;
}

void VertexFactory::ShareBuffers(VertexBufferBindingArray *pDestinationVertexArray, const VertexBufferBindingArray *pSourceVertexArray, int32 BufferIndex /* = -1 */)
{
    if (BufferIndex < 0)
    {
        uint32 i;
        uint32 nBuffersToSet = pSourceVertexArray->GetActiveBufferCount();
        for (i = 0; i < nBuffersToSet; i++)
            pDestinationVertexArray->SetBuffer(i, pSourceVertexArray->GetBuffer(i), pSourceVertexArray->GetBufferOffset(i), pSourceVertexArray->GetBufferStride(i));
    }
    else
    {
        DebugAssert(static_cast<uint32>(BufferIndex) < pSourceVertexArray->GetActiveBufferCount());
        pDestinationVertexArray->SetBuffer(BufferIndex, pSourceVertexArray->GetBuffer(BufferIndex), pSourceVertexArray->GetBufferOffset(BufferIndex), pSourceVertexArray->GetBufferStride(BufferIndex));
    }
}

