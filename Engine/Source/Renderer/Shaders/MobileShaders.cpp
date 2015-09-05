#include "Renderer/PrecompiledHeader.h"
#include "Renderer/Shaders/MobileShaders.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderConstantBuffer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/ShaderProgram.h"
#include "Engine/MaterialShader.h"

DEFINE_SHADER_COMPONENT_INFO(MobileBasePassShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(MobileBasePassShader)
END_SHADER_COMPONENT_PARAMETERS()

bool MobileBasePassShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo == NULL || pMaterialShader == NULL)
        return false;

    if (baseShaderFlags & WITH_EMISSIVE && pMaterialShader->GetLightingType() != MATERIAL_LIGHTING_TYPE_EMISSIVE)
        return false;

    return true;
}

bool MobileBasePassShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/MobileBasePassShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/MobileBasePassShader.hlsl", "PSMain");

    if (baseShaderFlags & WITH_EMISSIVE)
        pParameters->AddPreprocessorMacro("WITH_EMISSIVE", "1");

    if (baseShaderFlags & WITH_LIGHTMAP)
        pParameters->AddPreprocessorMacro("WITH_LIGHTMAP", "1");

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_COMPONENT_INFO(MobileDirectionalLightShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(MobileDirectionalLightShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("ShadowMapTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
END_SHADER_COMPONENT_PARAMETERS()

BEGIN_SHADER_CONSTANT_BUFFER(cbMobileDirectionalLightParameters, "MobileDirectionalLightParametersBuffer", "MobileDirectionalLightParameters", RENDERER_PLATFORM_COUNT, RENDERER_FEATURE_LEVEL_COUNT, SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY_PER_PROGRAM)
    SHADER_CONSTANT_BUFFER_FIELD("LightVector", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightColor", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("AmbientColor", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("ShadowMapSize", SHADER_PARAMETER_TYPE_FLOAT4, 1)
    SHADER_CONSTANT_BUFFER_FIELD("ShadowMapViewProjectionMatrix", SHADER_PARAMETER_TYPE_FLOAT4X4, 1)
END_SHADER_CONSTANT_BUFFER(cbMobileDirectionalLightParameters)

uint32 MobileDirectionalLightShader::CalculateFlags(bool enableShadows, RENDERER_SHADOW_FILTER shadowFilter)
{
    uint32 flags = 0;

    if (enableShadows)
    {
        flags |= WITH_SHADOW_MAP;       
        switch (shadowFilter)
        {
        case RENDERER_SHADOW_FILTER_3X3:
            flags |= SHADOW_FILTER_3X3;
            break;

        case RENDERER_SHADOW_FILTER_5X5:
            flags |= SHADOW_FILTER_5X5;
            break;

        //case RENDERER_SHADOW_FILTER_1X1:
        default:
            flags |= SHADOW_FILTER_1X1;
            break;
        }
    }

    return flags;
}

void MobileDirectionalLightShader::SetLightParameters(GPUContext *pContext, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight, const SSMShadowMapRenderer::ShadowMapData *pShadowMapData)
{
    cbMobileDirectionalLightParameters.SetFieldFloat3(pContext, 0, pLight->Direction, false);
    cbMobileDirectionalLightParameters.SetFieldFloat3(pContext, 1, pLight->LightColor, false);
    cbMobileDirectionalLightParameters.SetFieldFloat3(pContext, 2, pLight->AmbientColor, false);

    if (pShadowMapData != nullptr)
    {
        uint32 shadowMapWidth = pShadowMapData->pShadowMapTexture->GetDesc()->Width;
        uint32 shadowMapHeight = pShadowMapData->pShadowMapTexture->GetDesc()->Height;
        float4 shadowMapSize((float)shadowMapWidth, (float)shadowMapHeight, 1.0f / (float)shadowMapWidth, 1.0f / (float)shadowMapHeight);

        cbMobileDirectionalLightParameters.SetFieldFloat4(pContext, 3, shadowMapSize, false);
        cbMobileDirectionalLightParameters.SetFieldFloat4x4(pContext, 4, pShadowMapData->ViewProjectionMatrix, false);
    }

    cbMobileDirectionalLightParameters.CommitChanges(pContext);
}

void MobileDirectionalLightShader::SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight, const SSMShadowMapRenderer::ShadowMapData *pShadowMapData)
{
    if (pShadowMapData != nullptr)
        pShaderProgram->SetBaseShaderParameterTexture(pContext, 0, pShadowMapData->pShadowMapTexture, nullptr);
}

bool MobileDirectionalLightShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo == NULL || pMaterialShader == NULL)
        return false;

    if (pMaterialShader->GetLightingType() == MATERIAL_LIGHTING_TYPE_EMISSIVE)
        return false;

    return true;
}

bool MobileDirectionalLightShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/MobileDirectionalLightShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/MobileDirectionalLightShader.hlsl", "PSMain");

    if (baseShaderFlags & WITH_SHADOW_MAP)
    {
        pParameters->AddPreprocessorMacro("WITH_SHADOW_MAP", "1");

        // filter
        if (baseShaderFlags & SHADOW_FILTER_3X3)
            pParameters->AddPreprocessorMacro("SHADOW_FILTER_3X3", "1");
        else if (baseShaderFlags & SHADOW_FILTER_5X5)
            pParameters->AddPreprocessorMacro("SHADOW_FILTER_5X5", "1");
        else //if (baseShaderFlags & SHADOW_FILTER_1X1)
            pParameters->AddPreprocessorMacro("SHADOW_FILTER_1X1", "1");
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_COMPONENT_INFO(MobileFinalCompositeShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(MobileFinalCompositeShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("SceneTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
END_SHADER_COMPONENT_PARAMETERS()

void MobileFinalCompositeShader::SetInputParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pHDRTexture)
{
    pShaderProgram->SetBaseShaderParameterTexture(pContext, 0, pHDRTexture, g_pRenderer->GetFixedResources()->GetPointSamplerState());
}

bool MobileFinalCompositeShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr || pMaterialShader != nullptr)
        return false;

    return true;
}

bool MobileFinalCompositeShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/ScreenQuadVertexShader.hlsl", "Main");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/MobileFinalCompositeShader.hlsl", "PSMain");
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
