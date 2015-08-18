#pragma once
#include "Renderer/ShaderComponent.h"
#include "Renderer/WorldRenderers/SSMShadowMapRenderer.h"

class MobileBasePassShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(MobileBasePassShader, ShaderComponent);

public:
    enum FLAGS
    {
        WITH_EMISSIVE                               = (1 << 0),
        WITH_LIGHTMAP                               = (1 << 1),
    };

public:
    MobileBasePassShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class MobileDirectionalLightShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(MobileDirectionalLightShader, ShaderComponent);

public:
    enum FLAGS
    {
        WITH_SHADOW_MAP = (1 << 0),
        SHADOW_FILTER_1X1 = (1 << 3),
        SHADOW_FILTER_3X3 = (1 << 4),
        SHADOW_FILTER_5X5 = (1 << 5),

        SHADOW_BITS_MASK = WITH_SHADOW_MAP | SHADOW_FILTER_1X1 | SHADOW_FILTER_3X3 | SHADOW_FILTER_5X5
    };

public:
    MobileDirectionalLightShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    // get flags based on cvar values
    static uint32 CalculateFlags(bool enableShadows, RENDERER_SHADOW_FILTER shadowFilter);

    // light params
    static void SetLightParameters(GPUContext *pContext, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight, const SSMShadowMapRenderer::ShadowMapData *pShadowMapData);
    static void SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight, const SSMShadowMapRenderer::ShadowMapData *pShadowMapData);

    // common functions
    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class MobileFinalCompositeShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(MobileFinalCompositeShader, ShaderComponent);

public:
    MobileFinalCompositeShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static void SetInputParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pSceneTexture);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

