#include "Renderer/PrecompiledHeader.h"
#include "Renderer/Shaders/ShadowMapShader.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Engine/MaterialShader.h"

DEFINE_SHADER_COMPONENT_INFO(ShadowMapShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(ShadowMapShader)
END_SHADER_COMPONENT_PARAMETERS()

bool ShadowMapShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo == NULL)
        return false;

    return true;
}

bool ShadowMapShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/ShadowMapShader.hlsl", "VSMain");

    // Disable pixel shader on materials that don't contain any clipping.
    if (!pParameters->MaterialShaderName.IsEmpty())
    {
        pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/ShadowMapShader.hlsl", "PSMain");
        pParameters->AddPreprocessorMacro("WITH_MATERIAL", "1");
    }
    // ES2/3 require a fragment shader
    else if (pParameters->FeatureLevel <= RENDERER_FEATURE_LEVEL_ES3)
        pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/ShadowMapShader.hlsl", "PSMain");

    return true;
}
