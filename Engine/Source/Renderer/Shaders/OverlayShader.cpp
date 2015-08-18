#include "Renderer/PrecompiledHeader.h"
#include "Renderer/Shaders/OverlayShader.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/ShaderProgram.h"
#include "Engine/MaterialShader.h"
#include "Engine/Texture.h"

DEFINE_SHADER_COMPONENT_INFO(OverlayShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(OverlayShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("DrawTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
END_SHADER_COMPONENT_PARAMETERS()

void OverlayShader::SetTexture(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture *pTexture)
{
    pShaderProgram->SetBaseShaderParameterResource(pContext, 0, pTexture);
}

void OverlayShader::SetTexture(GPUContext *pContext, ShaderProgram *pShaderProgram, Texture *pTexture)
{
    pShaderProgram->SetBaseShaderParameterResource(pContext, 0, (pTexture != nullptr) ? pTexture->GetGPUTexture() : nullptr);
}

bool OverlayShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr)
        return false;

    if (pMaterialShader != nullptr)
        return false;

    return true;
}

bool OverlayShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    if (baseShaderFlags & WITH_3D_VERTEX)
        pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/OverlayShader.hlsl", "WorldVS");
    else
        pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/OverlayShader.hlsl", "ScreenVS");

    if (baseShaderFlags & WITH_TEXTURE)
        pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/OverlayShader.hlsl", "TexturedPS");
    else
        pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/OverlayShader.hlsl", "ColoredPS");

    return true;
}
