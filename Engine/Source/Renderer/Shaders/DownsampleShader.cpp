#include "Renderer/PrecompiledHeader.h"
#include "Renderer/Shaders/DownsampleShader.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/ShaderProgram.h"

DEFINE_SHADER_COMPONENT_INFO(DownsampleShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(DownsampleShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("InputTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("MipLevel", SHADER_PARAMETER_TYPE_UINT)
END_SHADER_COMPONENT_PARAMETERS()

void DownsampleShader::SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pInputTexture, uint32 mipLevel)
{
    pShaderProgram->SetBaseShaderParameterTexture(pContext, 0, pInputTexture, g_pRenderer->GetFixedResources()->GetPointSamplerState());
    pShaderProgram->SetBaseShaderParameterValue(pContext, 1, SHADER_PARAMETER_TYPE_UINT, &mipLevel);
}

bool DownsampleShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr || pMaterialShader != nullptr)
        return false;

    return true;
}

bool DownsampleShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Requires feature level SM4
    if (pParameters->FeatureLevel < RENDERER_FEATURE_LEVEL_SM4)
        return false;

    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/ScreenQuadVertexShader.hlsl", "Main");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/DownsamplePixelShader.hlsl", "PSMain");

    // vertex elements -- if not generate on gpu
    /*static const GPU_VERTEX_ELEMENT_DESC screenQuadVertexElementDesc[] = {
        { GPU_VERTEX_ELEMENT_SEMANTIC_POSITION, 0, GPU_VERTEX_ELEMENT_TYPE_FLOAT3, 0, 0, 0 },
        { GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD, 0, GPU_VERTEX_ELEMENT_TYPE_FLOAT2, 0, sizeof(float3), 0 }
    };
    for (uint32 i = 0; i < countof(screenQuadVertexElementDesc); i++)
        pShaderCompiler->AddVertexAttribute(&screenQuadVertexElementDesc[i]);*/

    return true;
}
