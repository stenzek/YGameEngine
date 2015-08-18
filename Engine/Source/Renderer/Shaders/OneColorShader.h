#pragma once
#include "Renderer/ShaderComponent.h"

class ShaderProgram;

class OneColorShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(OneColorShader, ShaderComponent);

public:
    OneColorShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static void SetColor(GPUContext *pContext, ShaderProgram *pShaderProgram, const float4 &col);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

