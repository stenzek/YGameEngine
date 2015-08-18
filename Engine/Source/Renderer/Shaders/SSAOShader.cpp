#include "Renderer/PrecompiledHeader.h"
#include "Renderer/Shaders/SSAOShader.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/ShaderProgram.h"
#include "Engine/Texture.h"

DEFINE_SHADER_COMPONENT_INFO(SSAOShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(SSAOShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("RandomTextureScale", SHADER_PARAMETER_TYPE_FLOAT2)
    DEFINE_SHADER_COMPONENT_PARAMETER("RandomTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("DepthBuffer", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("NormalsTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
END_SHADER_COMPONENT_PARAMETERS()

void SSAOShader::SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pDepthBuffer, GPUTexture2D *pNormalsTexture)
{
    // get random texture, and scale it
    const Texture2D *pRandomTexture = g_pRenderer->GetFixedResources()->GetRandomTexture();
    float2 randomTextureScale((float)pContext->GetViewport()->Width / (float)pRandomTexture->GetWidth(), (float)pContext->GetViewport()->Height / (float)pRandomTexture->GetHeight());

    // set parameters
    pShaderProgram->SetBaseShaderParameterValue(pContext, 0, SHADER_PARAMETER_TYPE_FLOAT2, &randomTextureScale);
    pShaderProgram->SetBaseShaderParameterTexture(pContext, 1, pRandomTexture->GetGPUTexture(), nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pContext, 2, pDepthBuffer, nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pContext, 3, pNormalsTexture, nullptr);
}

bool SSAOShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr || pMaterialShader != nullptr)
        return false;

    return true;
}

bool SSAOShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Requires feature level SM4
    if (pParameters->FeatureLevel < RENDERER_FEATURE_LEVEL_SM4)
        return false;

    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/ScreenQuadVertexShader.hlsl", "Main");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/SSAOShader.hlsl", "PSMain");

    // need view ray from VS
    pParameters->AddPreprocessorMacro("GENERATE_VIEW_RAY", "1");
    return true;
}

DEFINE_SHADER_COMPONENT_INFO(SSAOApplyShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(SSAOApplyShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("AOTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
END_SHADER_COMPONENT_PARAMETERS()

void SSAOApplyShader::SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pAOTexture)
{
    pShaderProgram->SetBaseShaderParameterTexture(pContext, 0, pAOTexture, nullptr);
}

bool SSAOApplyShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr || pMaterialShader != nullptr)
        return false;

    return true;
}

bool SSAOApplyShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Requires feature level SM4
    if (pParameters->FeatureLevel < RENDERER_FEATURE_LEVEL_SM4)
        return false;

    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/ScreenQuadVertexShader.hlsl", "Main");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/SSAOApplyShader.hlsl", "PSMain");
    return true;
}
