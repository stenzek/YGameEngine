#pragma once
#include "Renderer/ShaderComponent.h"
#include "Renderer/WorldRenderer.h"
#include "Renderer/WorldRenderers/CSMShadowMapRenderer.h"
#include "Renderer/WorldRenderers/CubeMapShadowMapRenderer.h"

class DeferredGBufferShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(DeferredGBufferShader, ShaderComponent);

    enum
    {
        WITH_LIGHTMAP = (1 << 0),
        WITH_LIGHTBUFFER = (1 << 1),
        NO_ALBEDO = (1 << 2)
    };

public:
    DeferredGBufferShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class DeferredDirectionalLightShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(DeferredDirectionalLightShader, ShaderComponent);

public:
    enum FLAGS
    {
        WITH_SHADOW_MAP         = (1 << 0),
        SHOW_CASCADES           = (1 << 1),
        USE_HARDWARE_PCF        = (1 << 2),
        SHADOW_FILTER_1X1       = (1 << 3),
        SHADOW_FILTER_3X3       = (1 << 4),
        SHADOW_FILTER_5X5       = (1 << 5),
        
        SHADOW_BITS_MASK        = WITH_SHADOW_MAP | SHOW_CASCADES | USE_HARDWARE_PCF | SHADOW_FILTER_1X1 | SHADOW_FILTER_3X3 | SHADOW_FILTER_5X5
    };

public:
    DeferredDirectionalLightShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    // get flags based on cvar values
    static uint32 CalculateShadowFlags(bool enableShadows, bool useHardwareShadowFiltering, RENDERER_SHADOW_FILTER shadowFilter, bool showCascades);

    // light params
    static void SetLightParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, const WorldRenderer::ViewParameters *pViewParameters, const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLight);
    static void SetShadowParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, const CSMShadowMapRenderer::ShadowMapData *pShadowMapData);
    static void SetBufferParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pDepthBuffer, GPUTexture2D *pGBuffer0, GPUTexture2D *pGBuffer1, GPUTexture2D *pGBuffer2);
    static void CommitParameters(GPUContext *pContext, ShaderProgram *pShaderProgram);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class DeferredPointLightShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(DeferredPointLightShader, ShaderComponent);

public:
    enum FLAGS
    {
        WITH_SHADOW_MAP         = (1 << 0),
        USE_HARDWARE_PCF        = (1 << 1),
        
        SHADOW_BITS_MASK        = WITH_SHADOW_MAP | USE_HARDWARE_PCF
    };

public:
    DeferredPointLightShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    // get flags based on cvar values
    static uint32 CalculateShadowFlags(bool enableShadows, bool useHardwareShadowFiltering);

    // light params
    static void SetLightParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, const WorldRenderer::ViewParameters *pViewParameters, const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight);
    static void SetShadowParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, const CubeMapShadowMapRenderer::ShadowMapData *pShadowMapData);
    static void SetBufferParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pDepthBuffer, GPUTexture2D *pGBuffer0, GPUTexture2D *pGBuffer1, GPUTexture2D *pGBuffer2);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class DeferredPointLightListShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(DeferredPointLightListShader, ShaderComponent);

public:
    static const uint32 MAX_LIGHTS = 256;

public:
    DeferredPointLightListShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static void SetLightParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, const WorldRenderer::ViewParameters *pViewParameters, uint32 lightIndex, const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLight);
    static void SetBufferParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pDepthBuffer, GPUTexture2D *pGBuffer0, GPUTexture2D *pGBuffer1, GPUTexture2D *pGBuffer2);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class DeferredTiledPointLightShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(DeferredTiledPointLightShader, ShaderComponent);

public:
    struct Light
    {
        Light(const float3 &position, float inverseRange, const float3 &color, float falloffExponent) : Position(position), InverseRange(inverseRange), Color(color), FalloffExponent(falloffExponent) {}

        float3 Position;
        float InverseRange;
        float3 Color;
        float FalloffExponent;
    };

    static const uint32 MAX_LIGHTS_PER_DISPATCH = 1024;
    static const uint32 TILE_SIZE = 32;

public:
    DeferredTiledPointLightShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    // light params
    static void SetLights(GPUContext *pContext, ShaderProgram *pShaderProgram, const Light *pLights, uint32 nLights);
    static void SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, uint32 tileCountX, uint32 tileCountY);
    static void SetBufferParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pDepthBuffer, GPUTexture2D *pGBuffer0, GPUTexture2D *pGBuffer1, GPUTexture2D *pGBuffer2, GPUTexture2D *pLightBuffer);
    static void CommitParameters(GPUContext *pContext, ShaderProgram *pShaderProgram);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class DeferredFogShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(DeferredFogShader, ShaderComponent);

    enum FLAGS
    {
        MODE_LINEAR         = (1 << 0),
        MODE_EXP            = (1 << 1),
        MODE_EXP2           = (1 << 2),
    };

public:
    DeferredFogShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static uint32 GetFlagsForMode(RENDERER_FOG_MODE mode);

    static void SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, const WorldRenderer::ViewParameters *pViewParameters, GPUTexture2D *pDepthBuffer);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

