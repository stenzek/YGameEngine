#include "Renderer/PrecompiledHeader.h"
#include "Renderer/Shaders/OneColorShader.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/ShaderProgram.h"
#include "Engine/MaterialShader.h"

DEFINE_SHADER_COMPONENT_INFO(OneColorShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(OneColorShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("DrawColor", SHADER_PARAMETER_TYPE_FLOAT4)
END_SHADER_COMPONENT_PARAMETERS()

void OneColorShader::SetColor(GPUContext *pContext, ShaderProgram *pShaderProgram, const float4 &col)
{
    pShaderProgram->SetBaseShaderParameterValue(pContext, 0, SHADER_PARAMETER_TYPE_FLOAT4, &col);
}

bool OneColorShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo == NULL)
        return false;

    return true;
}

bool OneColorShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/OneColorShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/OneColorShader.hlsl", "PSMain");
    return true;
}
