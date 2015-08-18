#pragma once
#include "Renderer/ShaderComponent.h"

class ShaderProgram;

class SSAOShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(SSAOShader, ShaderComponent);

public:
    SSAOShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static void SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pDepthBuffer, GPUTexture2D *pNormalsTexture);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

class SSAOApplyShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(SSAOApplyShader, ShaderComponent);

public:
    SSAOApplyShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static void SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pAOTexture);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

