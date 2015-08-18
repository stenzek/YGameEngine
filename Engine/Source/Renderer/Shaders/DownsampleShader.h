#pragma once
#include "Renderer/ShaderComponent.h"

class ShaderProgram;

class DownsampleShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(DownsampleShader, ShaderComponent);

public:
    DownsampleShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static void SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pInputTexture, uint32 mipLevel);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};
