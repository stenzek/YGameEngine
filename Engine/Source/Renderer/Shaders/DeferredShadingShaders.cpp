#include "Renderer/PrecompiledHeader.h"
#include "Renderer/Shaders/DeferredShadingShaders.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/ShaderConstantBuffer.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/Renderer.h"
#include "Engine/MaterialShader.h"

DEFINE_SHADER_COMPONENT_INFO(DeferredGBufferShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(DeferredGBufferShader)
END_SHADER_COMPONENT_PARAMETERS()

bool DeferredGBufferShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo == nullptr || pMaterialShader == nullptr)
        return false;

    if (pMaterialShader->GetLightingType() == MATERIAL_LIGHTING_TYPE_REFLECTIVE)
        return false;

    return true;
}

bool DeferredGBufferShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Requires feature level SM4
    if (pParameters->FeatureLevel < RENDERER_FEATURE_LEVEL_SM4)
        return false;

    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/DeferredGBufferShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/DeferredGBufferShader.hlsl", "PSMain");

    if (baseShaderFlags & WITH_LIGHTMAP)
        pParameters->AddPreprocessorMacro("WITH_LIGHTMAP", "1");

    if (baseShaderFlags & WITH_LIGHTBUFFER)
        pParameters->AddPreprocessorMacro("WITH_LIGHTBUFFER", "1");

    if (baseShaderFlags & NO_ALBEDO)
        pParameters->AddPreprocessorMacro("NO_ALBEDO", "1");

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_COMPONENT_INFO(DeferredDirectionalLightShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(DeferredDirectionalLightShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("DepthBuffer", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("GBuffer0", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("GBuffer1", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("GBuffer2", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("ShadowMapTexture", SHADER_PARAMETER_TYPE_TEXTURE2DARRAY)
END_SHADER_COMPONENT_PARAMETERS()

BEGIN_SHADER_CONSTANT_BUFFER(cbDeferredDirectionalLightParameters, "DeferredDirectionalLightParameters", "cbDeferredDirectionalLightParameters", RENDERER_PLATFORM_COUNT, RENDERER_FEATURE_LEVEL_SM4, SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY_PER_PROGRAM)
    SHADER_CONSTANT_BUFFER_FIELD("LightVector", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightColor", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("AmbientColor", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("ShadowMapSize", SHADER_PARAMETER_TYPE_FLOAT4, 1)
    SHADER_CONSTANT_BUFFER_FIELD("CascadeViewProjectionMatrix", SHADER_PARAMETER_TYPE_FLOAT4X4, CSMShadowMapRenderer::MaxCascadeCount)
    SHADER_CONSTANT_BUFFER_FIELD("CascadeSplitDepths", SHADER_PARAMETER_TYPE_FLOAT, CSMShadowMapRenderer::MaxCascadeCount)
END_SHADER_CONSTANT_BUFFER(cbDeferredDirectionalLightParameters)

uint32 DeferredDirectionalLightShader::CalculateShadowFlags(bool enableShadows, bool useHardwareShadowFiltering, RENDERER_SHADOW_FILTER shadowFilter, bool showCascades)
{
    uint32 flags = 0;

    if (enableShadows)
    {
        flags |= WITH_SHADOW_MAP;
        if (useHardwareShadowFiltering)
            flags |= USE_HARDWARE_PCF;
        
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

        if (showCascades)
            flags |= SHOW_CASCADES;
    }

    return flags;
}

void DeferredDirectionalLightShader::SetLightParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, const WorldRenderer::ViewParameters *pViewParameters, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight)
{
    // TODO convert direction to viewspace
    //cbDeferredDirectionalLightParameters.SetFieldFloat3(pCommandList, 0, pLight->Direction, false);
    cbDeferredDirectionalLightParameters.SetFieldFloat3(pCommandList, 0, pViewParameters->ViewCamera.GetViewMatrix().TransformNormal(pLight->Direction), false);
    cbDeferredDirectionalLightParameters.SetFieldFloat3(pCommandList, 1, pLight->LightColor, false);
    cbDeferredDirectionalLightParameters.SetFieldFloat3(pCommandList, 2, pLight->AmbientColor, false);
}

void DeferredDirectionalLightShader::SetShadowParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, const CSMShadowMapRenderer::ShadowMapData *pShadowMapData)
{
    uint32 shadowMapWidth = pShadowMapData->pShadowMapTexture->GetDesc()->Width;
    uint32 shadowMapHeight = pShadowMapData->pShadowMapTexture->GetDesc()->Height;
    float4 shadowMapSize((float)shadowMapWidth, (float)shadowMapHeight, 1.0f / (float)shadowMapWidth, 1.0f / (float)shadowMapHeight);

    cbDeferredDirectionalLightParameters.SetFieldFloat4(pCommandList, 3, shadowMapSize, false);
    cbDeferredDirectionalLightParameters.SetFieldFloat4x4Array(pCommandList, 4, 0, pShadowMapData->CascadeCount, pShadowMapData->ViewProjectionMatrices, false);
    cbDeferredDirectionalLightParameters.SetFieldFloatArray(pCommandList, 5, 0, pShadowMapData->CascadeCount, pShadowMapData->CascadeFrustumEyeSpaceDepths, false);

    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 4, pShadowMapData->pShadowMapTexture, nullptr);
}

void DeferredDirectionalLightShader::SetBufferParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, GPUTexture2D *pDepthBuffer, GPUTexture2D *pGBuffer0, GPUTexture2D *pGBuffer1, GPUTexture2D *pGBuffer2)
{
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 0, pDepthBuffer, nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 1, pGBuffer0, nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 2, pGBuffer1, nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 3, pGBuffer2, nullptr);
}

void DeferredDirectionalLightShader::CommitParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram)
{
    cbDeferredDirectionalLightParameters.CommitChanges(pCommandList);
}

bool DeferredDirectionalLightShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr || pMaterialShader != nullptr)
        return false;

    return true;
}

bool DeferredDirectionalLightShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Requires feature level SM4
    if (pParameters->FeatureLevel < RENDERER_FEATURE_LEVEL_SM4)
        return false;

    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/ScreenQuadVertexShader.hlsl", "Main");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/DeferredDirectionalLightShader.hlsl", "PSMain");

    // need view ray from VS
    /*GPU_VERTEX_ELEMENT_DESC screenQuadVertexElementDesc(GPU_VERTEX_ELEMENT_SEMANTIC_POSITION, 0, GPU_VERTEX_ELEMENT_TYPE_FLOAT3, 0, 0, 0);
    pParameters->AddVertexAttribute(&screenQuadVertexElementDesc);*/
    pParameters->AddPreprocessorMacro("GENERATE_VIEW_RAY", "1");

    if (baseShaderFlags & WITH_SHADOW_MAP)
    {
        pParameters->AddPreprocessorMacro("WITH_SHADOW_MAP", "1");

        if (baseShaderFlags & USE_HARDWARE_PCF)
            pParameters->AddPreprocessorMacro("USE_HARDWARE_PCF", "1");

        // filter
        if (baseShaderFlags & SHADOW_FILTER_3X3)
            pParameters->AddPreprocessorMacro("SHADOW_FILTER_3X3", "1");
        else if (baseShaderFlags & SHADOW_FILTER_5X5)
            pParameters->AddPreprocessorMacro("SHADOW_FILTER_5X5", "1");
        else //if (baseShaderFlags & SHADOW_FILTER_1X1)
            pParameters->AddPreprocessorMacro("SHADOW_FILTER_1X1", "1");

        if (baseShaderFlags & SHOW_CASCADES)
            pParameters->AddPreprocessorMacro("SHOW_CASCADES", "1");
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_COMPONENT_INFO(DeferredPointLightShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(DeferredPointLightShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("DepthBuffer", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("GBuffer0", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("GBuffer1", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("GBuffer2", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("ShadowMapTexture", SHADER_PARAMETER_TYPE_TEXTURECUBE)
END_SHADER_COMPONENT_PARAMETERS()

BEGIN_SHADER_CONSTANT_BUFFER(cbDeferredPointLightParameters, "DeferredPointLightParameters", "cbDeferredPointLightParameters", RENDERER_PLATFORM_COUNT, RENDERER_FEATURE_LEVEL_COUNT, SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY_PER_PROGRAM)
    SHADER_CONSTANT_BUFFER_FIELD("LightColor", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightPosition", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightPositionVS", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightRange", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightInverseRange", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightFalloffExponent", SHADER_PARAMETER_TYPE_FLOAT, 1)
END_SHADER_CONSTANT_BUFFER(cbDeferredPointLightParameters)

uint32 DeferredPointLightShader::CalculateShadowFlags(bool enableShadows, bool useHardwareShadowFiltering)
{
    uint32 flags = 0;

    if (enableShadows)
    {
        flags |= WITH_SHADOW_MAP;
        if (useHardwareShadowFiltering)
            flags |= USE_HARDWARE_PCF;
    }

    return flags;
}

void DeferredPointLightShader::SetLightParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, const WorldRenderer::ViewParameters *pViewParameters, const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight)
{
    cbDeferredPointLightParameters.SetFieldFloat3(pCommandList, 0, pLight->LightColor, false);
    cbDeferredPointLightParameters.SetFieldFloat3(pCommandList, 1, pLight->Position, false);
    cbDeferredPointLightParameters.SetFieldFloat3(pCommandList, 2, pViewParameters->ViewCamera.GetViewMatrix().TransformPoint(pLight->Position), false);
    cbDeferredPointLightParameters.SetFieldFloat(pCommandList, 3, pLight->Range, false);
    cbDeferredPointLightParameters.SetFieldFloat(pCommandList, 4, pLight->InverseRange, false);
    cbDeferredPointLightParameters.SetFieldFloat(pCommandList, 5, pLight->FalloffExponent, false);

    cbDeferredPointLightParameters.CommitChanges(pCommandList);
}

void DeferredPointLightShader::SetShadowParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, const CubeMapShadowMapRenderer::ShadowMapData *pShadowMapData)
{
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 4, pShadowMapData->pShadowMapTexture, nullptr);
}

void DeferredPointLightShader::SetBufferParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, GPUTexture2D *pDepthBuffer, GPUTexture2D *pGBuffer0, GPUTexture2D *pGBuffer1, GPUTexture2D *pGBuffer2)
{
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 0, pDepthBuffer, nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 1, pGBuffer0, nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 2, pGBuffer1, nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 3, pGBuffer2, nullptr);
}

bool DeferredPointLightShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr || pMaterialShader != nullptr)
        return false;

    return true;
}

bool DeferredPointLightShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Requires feature level SM4
    if (pParameters->FeatureLevel < RENDERER_FEATURE_LEVEL_SM4)
        return false;

    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/DeferredPointLightShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/DeferredPointLightShader.hlsl", "PSMain");

    if (baseShaderFlags & WITH_SHADOW_MAP)
    {
        pParameters->AddPreprocessorMacro("WITH_SHADOW_MAP", "1");

        if (baseShaderFlags & USE_HARDWARE_PCF)
            pParameters->AddPreprocessorMacro("USE_HARDWARE_PCF", "1");
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_COMPONENT_INFO(DeferredPointLightListShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(DeferredPointLightListShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("DepthBuffer", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("GBuffer0", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("GBuffer1", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("GBuffer2", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("LightParameters", SHADER_PARAMETER_TYPE_STRUCT)
    END_SHADER_COMPONENT_PARAMETERS()

struct _ShaderPointLight
{
    _ShaderPointLight(const float3 &position, float range, const float3 &color, float falloffExponent) : Position(position), Range(range), Color(color), FalloffExponent(falloffExponent) {}

    float3 Position;
    float Range;
    float3 Color;
    float FalloffExponent;
};

void DeferredPointLightListShader::SetLightParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, const WorldRenderer::ViewParameters *pViewParameters, uint32 lightIndex, const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight)
{
    DebugAssert(lightIndex < MAX_LIGHTS);

    _ShaderPointLight lightParams(pLight->Position, pLight->Range, pLight->LightColor, pLight->FalloffExponent);
    pShaderProgram->SetBaseShaderParameterStructArray(pCommandList, 4, &lightParams, sizeof(lightParams), lightIndex, 1);
}

void DeferredPointLightListShader::SetBufferParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, GPUTexture2D *pDepthBuffer, GPUTexture2D *pGBuffer0, GPUTexture2D *pGBuffer1, GPUTexture2D *pGBuffer2)
{
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 0, pDepthBuffer, nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 1, pGBuffer0, nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 2, pGBuffer1, nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 3, pGBuffer2, nullptr);
}

bool DeferredPointLightListShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr || pMaterialShader != nullptr)
        return false;

    return true;
}

bool DeferredPointLightListShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Requires feature level SM4
    if (pParameters->FeatureLevel < RENDERER_FEATURE_LEVEL_SM4)
        return false;

    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/DeferredPointLightListShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/DeferredPointLightListShader.hlsl", "PSMain");
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_COMPONENT_INFO(DeferredTiledPointLightShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(DeferredTiledPointLightShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("ActiveLightCount", SHADER_PARAMETER_TYPE_UINT)
    DEFINE_SHADER_COMPONENT_PARAMETER("TileCountX", SHADER_PARAMETER_TYPE_UINT)
    DEFINE_SHADER_COMPONENT_PARAMETER("TileCountY", SHADER_PARAMETER_TYPE_UINT)
    DEFINE_SHADER_COMPONENT_PARAMETER("DepthBuffer", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("GBuffer0", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("GBuffer1", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("GBuffer2", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("LightBuffer", SHADER_PARAMETER_TYPE_TEXTURE2D)
END_SHADER_COMPONENT_PARAMETERS()

DEFINE_RAW_SHADER_CONSTANT_BUFFER(cbDeferredTiledPointLightShaderLightBuffer, "DeferredTiledPointLightShaderLightBuffer", "", sizeof(DeferredTiledPointLightShader::Light) * DeferredTiledPointLightShader::MAX_LIGHTS_PER_DISPATCH, RENDERER_PLATFORM_COUNT, RENDERER_FEATURE_LEVEL_SM5, SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY_PER_PROGRAM)
    
void DeferredTiledPointLightShader::SetLights(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, const Light *pLights, uint32 nLights)
{
    cbDeferredTiledPointLightShaderLightBuffer.SetRawData(pCommandList, 0, sizeof(Light) * nLights, pLights, true);
    pShaderProgram->SetBaseShaderParameterValue(pCommandList, 0, SHADER_PARAMETER_TYPE_UINT, &nLights);
}

void DeferredTiledPointLightShader::SetProgramParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, uint32 tileCountX, uint32 tileCountY)
{
    pShaderProgram->SetBaseShaderParameterValue(pCommandList, 1, SHADER_PARAMETER_TYPE_UINT, &tileCountX);
    pShaderProgram->SetBaseShaderParameterValue(pCommandList, 2, SHADER_PARAMETER_TYPE_UINT, &tileCountY);
}

void DeferredTiledPointLightShader::SetBufferParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, GPUTexture2D *pDepthBuffer, GPUTexture2D *pGBuffer0, GPUTexture2D *pGBuffer1, GPUTexture2D *pGBuffer2, GPUTexture2D *pLightBuffer)
{
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 3, pDepthBuffer, nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 4, pGBuffer0, nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 5, pGBuffer1, nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 6, pGBuffer2, nullptr);
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 7, pLightBuffer, nullptr);
}

void DeferredTiledPointLightShader::CommitParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram)
{
    cbDeferredTiledPointLightShaderLightBuffer.CommitChanges(pCommandList);
}

bool DeferredTiledPointLightShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr || pMaterialShader != nullptr)
        return false;

    return true;
}

bool DeferredTiledPointLightShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Requires feature level SM5
    if (pParameters->FeatureLevel < RENDERER_FEATURE_LEVEL_SM5)
        return false;

    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_COMPUTE_SHADER, "shaders/base/d3d11/DeferredTiledPointLightShader.hlsl", "Main");
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_COMPONENT_INFO(DeferredFogShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(DeferredFogShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("FogStartDistance", SHADER_PARAMETER_TYPE_FLOAT)
    DEFINE_SHADER_COMPONENT_PARAMETER("FogEndDistance", SHADER_PARAMETER_TYPE_FLOAT)
    DEFINE_SHADER_COMPONENT_PARAMETER("FogDistance", SHADER_PARAMETER_TYPE_FLOAT)
    DEFINE_SHADER_COMPONENT_PARAMETER("FogDensity", SHADER_PARAMETER_TYPE_FLOAT)
    DEFINE_SHADER_COMPONENT_PARAMETER("FogColor", SHADER_PARAMETER_TYPE_FLOAT3)
    DEFINE_SHADER_COMPONENT_PARAMETER("DepthBuffer", SHADER_PARAMETER_TYPE_TEXTURE2D)
END_SHADER_COMPONENT_PARAMETERS()

uint32 DeferredFogShader::GetFlagsForMode(RENDERER_FOG_MODE mode)
{
    switch (mode)
    {
    case RENDERER_FOG_MODE_LINEAR:  return MODE_LINEAR;
    case RENDERER_FOG_MODE_EXP:     return MODE_EXP;
    case RENDERER_FOG_MODE_EXP2:    return MODE_EXP2;
    }

    return 0;
}

void DeferredFogShader::SetProgramParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, const WorldRenderer::ViewParameters *pViewParameters, GPUTexture2D *pDepthBuffer)
{
    float fogDistance = pViewParameters->FogEndDistance - pViewParameters->FogStartDistance;
    float3 fogColor(PixelFormatHelpers::ConvertSRGBToLinear(pViewParameters->FogColor));
    float3 f2(PixelFormatHelpers::ConvertLinearToSRGB(fogColor));

    // set parameters
    pShaderProgram->SetBaseShaderParameterValue(pCommandList, 0, SHADER_PARAMETER_TYPE_FLOAT, &pViewParameters->FogStartDistance);
    pShaderProgram->SetBaseShaderParameterValue(pCommandList, 1, SHADER_PARAMETER_TYPE_FLOAT, &pViewParameters->FogEndDistance);
    pShaderProgram->SetBaseShaderParameterValue(pCommandList, 2, SHADER_PARAMETER_TYPE_FLOAT, &fogDistance);
    pShaderProgram->SetBaseShaderParameterValue(pCommandList, 3, SHADER_PARAMETER_TYPE_FLOAT, &pViewParameters->FogDensity);
    pShaderProgram->SetBaseShaderParameterValue(pCommandList, 4, SHADER_PARAMETER_TYPE_FLOAT3, &fogColor);
    pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 5, pDepthBuffer, nullptr);
}

bool DeferredFogShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr || pMaterialShader != nullptr)
        return false;

    return true;
}

bool DeferredFogShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Requires feature level SM4
    if (pParameters->FeatureLevel < RENDERER_FEATURE_LEVEL_SM4)
        return false;

    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/ScreenQuadVertexShader.hlsl", "Main");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/FogShader.hlsl", "PSMain");

    // need view ray from VS
    pParameters->AddPreprocessorMacro("GENERATE_VIEW_RAY", "1");

    // mode flags
    if (baseShaderFlags & MODE_LINEAR)
        pParameters->AddPreprocessorMacro("FOG_MODE_LINEAR", "1");
    if (baseShaderFlags & MODE_EXP)
        pParameters->AddPreprocessorMacro("FOG_MODE_EXP", "1");
    if (baseShaderFlags & MODE_EXP2)
        pParameters->AddPreprocessorMacro("FOG_MODE_EXP2", "1");

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
