#include "Renderer/PrecompiledHeader.h"
#include "Renderer/Shaders/FullBrightShader.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Engine/MaterialShader.h"

DEFINE_SHADER_COMPONENT_INFO(FullBrightShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(FullBrightShader)
END_SHADER_COMPONENT_PARAMETERS()

bool FullBrightShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo == NULL || pMaterialShader == NULL)
        return false;

    return true;
}

bool FullBrightShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/FullBrightShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/FullBrightShader.hlsl", "PSMain");
    return true;
}
