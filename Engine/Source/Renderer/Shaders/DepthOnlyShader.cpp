#include "Renderer/PrecompiledHeader.h"
#include "Renderer/Shaders/DepthOnlyShader.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Engine/MaterialShader.h"

DEFINE_SHADER_COMPONENT_INFO(DepthOnlyShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(DepthOnlyShader)
END_SHADER_COMPONENT_PARAMETERS()

bool DepthOnlyShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo == NULL)
        return false;

    return true;
}

bool DepthOnlyShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/DepthOnlyShader.hlsl", "VSMain");

    if (!pParameters->MaterialShaderName.IsEmpty())
    {
        pParameters->AddPreprocessorMacro(StaticString("WITH_MATERIAL"), StaticString("1"));
        pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/DepthOnlyShader.hlsl", "PSMain");
    }

    return true;
}

