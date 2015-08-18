#include "Renderer/PrecompiledHeader.h"
#include "Renderer/Shaders/TextureBlitShader.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/ShaderProgram.h"
#include "Engine/MaterialShader.h"

DEFINE_SHADER_COMPONENT_INFO(TextureBlitShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(TextureBlitShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("SourceTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("SourceLevel", SHADER_PARAMETER_TYPE_FLOAT)
END_SHADER_COMPONENT_PARAMETERS()

void TextureBlitShader::SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture *pSourceTexture, GPUSamplerState *pSamplerState, uint32 sourceLevel)
{
    float fSourceLevel = (float)sourceLevel;
    pShaderProgram->SetBaseShaderParameterTexture(pContext, 0, pSourceTexture, pSamplerState);
    pShaderProgram->SetBaseShaderParameterValue(pContext, 1, SHADER_PARAMETER_TYPE_FLOAT, &fSourceLevel);
}

bool TextureBlitShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr || pMaterialShader != nullptr)
        return false;

    return true;
}

bool TextureBlitShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/TextureBlitShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/TextureBlitShader.hlsl", "PSMain");

    if (baseShaderFlags & USE_TEXTURE_LOD)
        pParameters->AddPreprocessorMacro(StaticString("USE_TEXTURE_LOD"), StaticString("1"));

    return true;
}
