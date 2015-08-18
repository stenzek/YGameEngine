#pragma once
#include "Renderer/ShaderComponent.h"
#include "Renderer/VertexFactoryTypeInfo.h"

class ShaderComponentTypeInfo;
class MaterialShader;
class VertexBufferBindingArray;
class ShaderCompiler;

class VertexFactory : public ShaderComponent
{
    DECLARE_VERTEX_FACTORY_TYPE_INFO(VertexFactory, ShaderComponent);

public:
    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);

    static uint32 GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS]);

    static void ShareBuffers(VertexBufferBindingArray *pDestinationVertexArray, const VertexBufferBindingArray *pSourceVertexArray, int32 BufferIndex = -1);
};

