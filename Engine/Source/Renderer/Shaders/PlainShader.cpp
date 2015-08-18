#include "Renderer/PrecompiledHeader.h"
#include "Renderer/Shaders/PlainShader.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/ShaderProgram.h"
#include "Engine/MaterialShader.h"
#include "Engine/Texture.h"

DEFINE_SHADER_COMPONENT_INFO(PlainShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(PlainShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("PlainTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
END_SHADER_COMPONENT_PARAMETERS()

void PlainShader::SetTexture(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture *pTexture)
{
    DebugAssert(pShaderProgram->GetBaseShaderFlags() & WITH_TEXTURE);
    pShaderProgram->SetBaseShaderParameterResource(pContext, 0, pTexture);
}

void PlainShader::SetTexture(GPUContext *pContext, ShaderProgram *pShaderProgram, Texture *pTexture)
{
    DebugAssert(pShaderProgram->GetBaseShaderFlags() & WITH_TEXTURE);
    pShaderProgram->SetBaseShaderParameterResource(pContext, 0, (pTexture != NULL) ? pTexture->GetGPUTexture() : NULL);
}

bool PlainShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != VERTEX_FACTORY_TYPE_INFO(PlainVertexFactory))
        return false;

    if (((baseShaderFlags & (WITH_TEXTURE | WITH_VERTEX_COLOR)) == (WITH_TEXTURE | WITH_VERTEX_COLOR)) && !(baseShaderFlags & BLEND_TEXTURE_AND_VERTEX_COLOR))
        return false;

    if (baseShaderFlags & WITH_MATERIAL && pMaterialShader == NULL)
        return false;

    return true;
}

bool PlainShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/PlainShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/PlainShader.hlsl", "PSMain");
        
    if (baseShaderFlags & WITH_TEXTURE)
        pParameters->AddPreprocessorMacro("WITH_TEXTURE", "1");
    if (baseShaderFlags & WITH_VERTEX_COLOR)
        pParameters->AddPreprocessorMacro("WITH_VERTEX_COLOR", "1");
    if (baseShaderFlags & WITH_MATERIAL)
        pParameters->AddPreprocessorMacro("WITH_MATERIAL", "1");
    if (baseShaderFlags & BLEND_TEXTURE_AND_VERTEX_COLOR)
        pParameters->AddPreprocessorMacro("BLEND_TEXTURE_AND_VERTEX_COLOR", "1");

    return true;
}
