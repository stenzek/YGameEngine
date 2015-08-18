#pragma once
#include "Renderer/ShaderComponent.h"
#include "Renderer/WorldRenderers/CSMShadowMapRenderer.h"
#include "Renderer/WorldRenderers/CubeMapShadowMapRenderer.h"

class BasePassShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(BasePassShader, ShaderComponent);

public:
    enum FLAGS
    {
        WITH_EMISSIVE                               = (1 << 0),
        WITH_LIGHTMAP                               = (1 << 1),
    };

public:
    BasePassShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class DirectionalLightShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(DirectionalLightShader, ShaderComponent);

public:
    enum FLAGS
    {
        WITH_SHADOW_MAP = (1 << 0),
        SHOW_CASCADES = (1 << 1),
        USE_HARDWARE_PCF = (1 << 2),
        SHADOW_FILTER_1X1 = (1 << 3),
        SHADOW_FILTER_3X3 = (1 << 4),
        SHADOW_FILTER_5X5 = (1 << 5),

        SHADOW_BITS_MASK = WITH_SHADOW_MAP | SHOW_CASCADES | USE_HARDWARE_PCF | SHADOW_FILTER_1X1 | SHADOW_FILTER_3X3 | SHADOW_FILTER_5X5
    };

public:
    DirectionalLightShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    // get flags based on cvar values
    static uint32 CalculateFlags(bool enableShadows, bool useHardwareShadowFiltering, RENDERER_SHADOW_FILTER shadowFilter, bool showCascades);

    // light params
    static void SetLightParameters(GPUContext *pContext, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight, const CSMShadowMapRenderer::ShadowMapData *pShadowMapData);
    static void SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight, const CSMShadowMapRenderer::ShadowMapData *pShadowMapData);

    // common functions
    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class EmissiveShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(EmissiveShader, ShaderComponent);

public:
    EmissiveShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PointLightShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(PointLightShader, ShaderComponent);

public:
    enum FLAGS
    {
        WITH_SHADOW_MAP = (1 << 0),
        USE_HARDWARE_PCF = (1 << 1),

        SHADOW_BITS_MASK = WITH_SHADOW_MAP | USE_HARDWARE_PCF
    };

public:
    PointLightShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static uint32 CalculateFlags(bool enableShadows, bool useHardwareShadowFiltering);

    static void SetLightParameters(GPUContext *pContext, const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight, const CubeMapShadowMapRenderer::ShadowMapData *pShadowMapData);
    static void SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight, const CubeMapShadowMapRenderer::ShadowMapData *pShadowMapData);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PointLightListShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(PointLightListShader, ShaderComponent);

public:
    static const uint32 MAX_LIGHTS = 8;

public:
    PointLightListShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static void SetLightParameters(GPUContext *pContext, uint32 lightIndex, const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight);
    static void SetActiveLightCount(GPUContext *pContext, uint32 activeLightCount);
    static void CommitParameters(GPUContext *pContext);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class VolumetricLightShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(VolumetricLightShader, ShaderComponent);

public:
    enum Flags
    {
        Flag_BoxPrimitive = (1 << 0),
        Flag_SpherePrimitive = (1 << 1),
    };

public:
    VolumetricLightShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static uint32 GetTypeFlagsForPrimitive(VOLUMETRIC_LIGHT_PRIMITIVE primitive);

    static void SetLightParameters(GPUContext *pContext, const RENDER_QUEUE_VOLUMETRIC_LIGHT_ENTRY *pLight);
    static void SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, const RENDER_QUEUE_VOLUMETRIC_LIGHT_ENTRY *pLight);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
