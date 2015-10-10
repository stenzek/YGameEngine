#include "Renderer/PrecompiledHeader.h"
#include "Renderer/Shaders/ForwardShadingShaders.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderConstantBuffer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/ShaderProgram.h"
#include "Engine/MaterialShader.h"

DEFINE_SHADER_COMPONENT_INFO(BasePassShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(BasePassShader)
END_SHADER_COMPONENT_PARAMETERS()

bool BasePassShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo == NULL || pMaterialShader == NULL)
        return false;

    if (baseShaderFlags & WITH_EMISSIVE && pMaterialShader->GetLightingType() != MATERIAL_LIGHTING_TYPE_EMISSIVE)
        return false;

    return true;
}

bool BasePassShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/BasePassShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/BasePassShader.hlsl", "PSMain");

    if (baseShaderFlags & WITH_EMISSIVE)
        pParameters->AddPreprocessorMacro("WITH_EMISSIVE", "1");

    if (baseShaderFlags & WITH_LIGHTMAP)
        pParameters->AddPreprocessorMacro("WITH_LIGHTMAP", "1");

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_COMPONENT_INFO(DirectionalLightShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(DirectionalLightShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("ShadowMapTexture", SHADER_PARAMETER_TYPE_TEXTURE2DARRAY)
END_SHADER_COMPONENT_PARAMETERS()

BEGIN_SHADER_CONSTANT_BUFFER(cbDirectionalLightParameters, "DirectionalLightParameters", "cbDirectionalLightParameters", RENDERER_PLATFORM_COUNT, RENDERER_FEATURE_LEVEL_COUNT, SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY_PER_PROGRAM)
    SHADER_CONSTANT_BUFFER_FIELD("LightVector", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightColor", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("AmbientColor", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("ShadowMapSize", SHADER_PARAMETER_TYPE_FLOAT4, 1)
    SHADER_CONSTANT_BUFFER_FIELD("CascadeViewProjectionMatrix", SHADER_PARAMETER_TYPE_FLOAT4X4, CSMShadowMapRenderer::MaxCascadeCount)
    SHADER_CONSTANT_BUFFER_FIELD("CascadeSplitDepths", SHADER_PARAMETER_TYPE_FLOAT, CSMShadowMapRenderer::MaxCascadeCount)
END_SHADER_CONSTANT_BUFFER(cbDirectionalLightParameters)

uint32 DirectionalLightShader::CalculateFlags(bool enableShadows, bool useHardwareShadowFiltering, RENDERER_SHADOW_FILTER shadowFilter, bool showCascades)
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

void DirectionalLightShader::SetLightParameters(GPUCommandList *pCommandList, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight, const CSMShadowMapRenderer::ShadowMapData *pShadowMapData)
{
    cbDirectionalLightParameters.SetFieldFloat3(pCommandList, 0, pLight->Direction, false);
    cbDirectionalLightParameters.SetFieldFloat3(pCommandList, 1, pLight->LightColor, false);
    cbDirectionalLightParameters.SetFieldFloat3(pCommandList, 2, pLight->AmbientColor, false);

    if (pShadowMapData != nullptr)
    {
        uint32 shadowMapWidth = pShadowMapData->pShadowMapTexture->GetDesc()->Width;
        uint32 shadowMapHeight = pShadowMapData->pShadowMapTexture->GetDesc()->Height;
        float4 shadowMapSize((float)shadowMapWidth, (float)shadowMapHeight, 1.0f / (float)shadowMapWidth, 1.0f / (float)shadowMapHeight);

        cbDirectionalLightParameters.SetFieldFloat4(pCommandList, 3, shadowMapSize, false);
        cbDirectionalLightParameters.SetFieldFloat4x4Array(pCommandList, 4, 0, pShadowMapData->CascadeCount, pShadowMapData->ViewProjectionMatrices, false);
        cbDirectionalLightParameters.SetFieldFloatArray(pCommandList, 5, 0, pShadowMapData->CascadeCount, pShadowMapData->CascadeFrustumEyeSpaceDepths, false);
    }

    cbDirectionalLightParameters.CommitChanges(pCommandList);
}

void DirectionalLightShader::SetProgramParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight, const CSMShadowMapRenderer::ShadowMapData *pShadowMapData)
{
    if (pShadowMapData != nullptr)
        pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 0, pShadowMapData->pShadowMapTexture, nullptr);
}

bool DirectionalLightShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo == NULL || pMaterialShader == NULL)
        return false;

    if (pMaterialShader->GetLightingType() == MATERIAL_LIGHTING_TYPE_EMISSIVE)
        return false;

    return true;
}

bool DirectionalLightShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/DirectionalLightShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/DirectionalLightShader.hlsl", "PSMain");

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

DEFINE_SHADER_COMPONENT_INFO(EmissiveShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(EmissiveShader)
END_SHADER_COMPONENT_PARAMETERS()

bool EmissiveShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo == NULL || pMaterialShader == NULL)
        return false;

    if (pMaterialShader->GetLightingType() != MATERIAL_LIGHTING_TYPE_EMISSIVE)
        return false;

    return true;
}

bool EmissiveShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/EmissiveShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/EmissiveShader.hlsl", "PSMain");
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_COMPONENT_INFO(PointLightShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(PointLightShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("ShadowMapTexture", SHADER_PARAMETER_TYPE_TEXTURECUBE)
END_SHADER_COMPONENT_PARAMETERS()

BEGIN_SHADER_CONSTANT_BUFFER(cbPointLightParameters, "PointLightParameters", "cbPointLightParameters", RENDERER_PLATFORM_COUNT, RENDERER_FEATURE_LEVEL_COUNT, SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY_PER_PROGRAM)
    SHADER_CONSTANT_BUFFER_FIELD("LightColor", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightPosition", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightRange", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightInverseRange", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightFalloffExponent", SHADER_PARAMETER_TYPE_FLOAT, 1)
END_SHADER_CONSTANT_BUFFER(cbPointLightParameters)

uint32 PointLightShader::CalculateFlags(bool enableShadows, bool useHardwareShadowFiltering)
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

void PointLightShader::SetLightParameters(GPUCommandList *pCommandList, const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight, const CubeMapShadowMapRenderer::ShadowMapData *pShadowMapData)
{
    cbPointLightParameters.SetFieldFloat3(pCommandList, 0, pLight->LightColor, false);
    cbPointLightParameters.SetFieldFloat3(pCommandList, 1, pLight->Position, false);
    cbPointLightParameters.SetFieldFloat(pCommandList, 2, pLight->Range, false);
    cbPointLightParameters.SetFieldFloat(pCommandList, 3, pLight->InverseRange, false);
    cbPointLightParameters.SetFieldFloat(pCommandList, 4, pLight->FalloffExponent, false);

    cbPointLightParameters.CommitChanges(pCommandList);
}

void PointLightShader::SetProgramParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight, const CubeMapShadowMapRenderer::ShadowMapData *pShadowMapData)
{
    if (pShadowMapData != nullptr)
        pShaderProgram->SetBaseShaderParameterTexture(pCommandList, 0, pShadowMapData->pShadowMapTexture, nullptr);
}

bool PointLightShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo == NULL || pMaterialShader == NULL)
        return false;

    if (pMaterialShader->GetLightingType() == MATERIAL_LIGHTING_TYPE_EMISSIVE)
        return false;

    return true;
}

bool PointLightShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/PointLightShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/PointLightShader.hlsl", "PSMain");

    if (baseShaderFlags & WITH_SHADOW_MAP)
    {
        pParameters->AddPreprocessorMacro("WITH_SHADOW_MAP", "1");

        if (baseShaderFlags & USE_HARDWARE_PCF)
            pParameters->AddPreprocessorMacro("USE_HARDWARE_PCF", "1");
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_COMPONENT_INFO(PointLightListShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(PointLightListShader)
END_SHADER_COMPONENT_PARAMETERS()

BEGIN_SHADER_CONSTANT_BUFFER(cbPointLightListParameters, "PointLightListParameters", "cbPointLightListParameters", RENDERER_PLATFORM_COUNT, RENDERER_FEATURE_LEVEL_COUNT, SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY_PER_PROGRAM)
    SHADER_CONSTANT_BUFFER_FIELD("LightPosition0", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightInverseRange0", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightColor0", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightFalloffExponent0", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightPosition1", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightInverseRange1", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightColor1", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightFalloffExponent1", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightPosition2", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightInverseRange2", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightColor2", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightFalloffExponent2", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightPosition3", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightInverseRange3", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightColor3", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightFalloffExponent3", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightPosition4", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightInverseRange4", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightColor4", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightFalloffExponent4", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightPosition5", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightInverseRange5", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightColor5", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightFalloffExponent5", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightPosition6", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightInverseRange6", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightColor6", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightFalloffExponent6", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightPosition7", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightInverseRange7", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightColor7", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightFalloffExponent7", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("ActiveLightCount", SHADER_PARAMETER_TYPE_UINT, 1)
END_SHADER_CONSTANT_BUFFER(cbPointLightListParameters)

void PointLightListShader::SetLightParameters(GPUCommandList *pCommandList, uint32 lightIndex, const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight)
{
    uint32 baseIndex = lightIndex * 4;
    DebugAssert(lightIndex < MAX_LIGHTS);

    cbPointLightListParameters.SetFieldFloat3(pCommandList, baseIndex + 0, pLight->Position, false);
    cbPointLightListParameters.SetFieldFloat(pCommandList, baseIndex + 1, pLight->InverseRange, false);
    cbPointLightListParameters.SetFieldFloat3(pCommandList, baseIndex + 2, pLight->LightColor, false);
    cbPointLightListParameters.SetFieldFloat(pCommandList, baseIndex + 3, pLight->FalloffExponent, false);
}

void PointLightListShader::SetActiveLightCount(GPUCommandList *pCommandList, uint32 activeLightCount)
{
    cbPointLightListParameters.SetFieldUInt(pCommandList, 32, activeLightCount, false);
}

void PointLightListShader::CommitParameters(GPUCommandList *pCommandList)
{
    cbPointLightListParameters.CommitChanges(pCommandList);
}

bool PointLightListShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo == NULL || pMaterialShader == NULL)
        return false;

    if (pMaterialShader->GetLightingType() == MATERIAL_LIGHTING_TYPE_EMISSIVE)
        return false;

    return true;
}

bool PointLightListShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/PointLightListShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/PointLightListShader.hlsl", "PSMain");
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


DEFINE_SHADER_COMPONENT_INFO(VolumetricLightShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(VolumetricLightShader)
END_SHADER_COMPONENT_PARAMETERS()

BEGIN_SHADER_CONSTANT_BUFFER(cbVolumetricLightShader, "VolumetricLightShader", "cbVolumetricLightParameters", RENDERER_PLATFORM_COUNT, RENDERER_FEATURE_LEVEL_COUNT, SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY_PER_PROGRAM)
    SHADER_CONSTANT_BUFFER_FIELD("LightColor", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightPosition", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightFalloff", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightBoxExtents", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("LightSphereRadius", SHADER_PARAMETER_TYPE_FLOAT, 1)
END_SHADER_CONSTANT_BUFFER(cbVolumetricLightShader)

uint32 VolumetricLightShader::GetTypeFlagsForPrimitive(VOLUMETRIC_LIGHT_PRIMITIVE primitive)
{
    switch (primitive)
    {
    case VOLUMETRIC_LIGHT_PRIMITIVE_BOX:
        return Flag_BoxPrimitive;

    case VOLUMETRIC_LIGHT_PRIMITIVE_SPHERE:
        return Flag_SpherePrimitive;
    }

    UnreachableCode();
    return 0;
}

void VolumetricLightShader::SetLightParameters(GPUCommandList *pCommandList, const RENDER_QUEUE_VOLUMETRIC_LIGHT_ENTRY *pLight)
{
    cbVolumetricLightShader.SetFieldFloat3(pCommandList, 0, pLight->LightColor, false);
    cbVolumetricLightShader.SetFieldFloat3(pCommandList, 1, pLight->Position, false);
    cbVolumetricLightShader.SetFieldFloat(pCommandList, 2, pLight->FalloffRate, false);
    cbVolumetricLightShader.SetFieldFloat3(pCommandList, 3, pLight->BoxExtents, false);
    cbVolumetricLightShader.SetFieldFloat(pCommandList, 4, pLight->SphereRadius, false);

    cbVolumetricLightShader.CommitChanges(pCommandList);
}

void VolumetricLightShader::SetProgramParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, const RENDER_QUEUE_VOLUMETRIC_LIGHT_ENTRY *pLight)
{

}

bool VolumetricLightShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo == NULL || pMaterialShader == NULL)
        return false;

    if (pMaterialShader->GetLightingType() == MATERIAL_LIGHTING_TYPE_EMISSIVE)
        return false;

    return true;
}

bool VolumetricLightShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/VolumetricLightShader.hlsl", "VSMain");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/VolumetricLightShader.hlsl", "PSMain");

    if (baseShaderFlags & Flag_BoxPrimitive)
        pParameters->AddPreprocessorMacro("VOLUMETRIC_LIGHT_BOX_PRIMITIVE", "1");

    if (baseShaderFlags & Flag_SpherePrimitive)
        pParameters->AddPreprocessorMacro("VOLUMETRIC_LIGHT_SPHERE_PRIMITIVE", "1");

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
