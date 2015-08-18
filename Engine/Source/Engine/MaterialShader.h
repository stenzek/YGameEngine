#pragma once
#include "Engine/Common.h"
#include "Core/Resource.h"
#include "Renderer/ShaderMap.h"
#include "Renderer/RendererTypes.h"

class Material;
class Texture;
class GPUTexture;
class GPUBlendState;
class GPUShaderProgram;

class MaterialShader : public Resource
{
    DECLARE_RESOURCE_TYPE_INFO(MaterialShader, Resource);
    DECLARE_RESOURCE_GENERIC_FACTORY(MaterialShader);

public:
    MaterialShader(const ResourceTypeInfo *pResourceTypeInfo = &s_TypeInfo);
    ~MaterialShader();

public:
    struct UniformParameter
    {
        struct Value
        {
            union
            {
                bool asBool;
                int32 asInt;
                float asFloat;
                float asFloat2[2];
                float asFloat3[3];
                float asFloat4[4];
            };
        };

        uint32 Index;
        String Name;
        SHADER_PARAMETER_TYPE Type;
        Value DefaultValue;
    };

    struct TextureParameter
    {
        struct Value
        {
            const Texture *pTexture;
            GPUTexture *pGPUTexture;
        };

        uint32 Index;
        String Name;
        TEXTURE_TYPE Type;
        String DefaultValue;
    };

    struct StaticSwitchParameter
    {
        uint32 Index;
        uint32 Mask;
        String Name;
        bool DefaultValue;
    };

    // settings
    MATERIAL_BLENDING_MODE GetBlendMode() const { return m_blendingMode; }
    MATERIAL_LIGHTING_TYPE GetLightingType() const { return m_lightingType; }
    MATERIAL_LIGHTING_MODEL GetLightingModel() const { return m_lightingModel; }
    MATERIAL_LIGHTING_NORMAL_SPACE GetLightingNormalSpace() const { return m_lightingNormalSpace; }
    MATERIAL_RENDER_MODE GetRenderMode() const { return m_renderMode; }
    MATERIAL_RENDER_LAYER GetRenderLayer() const { return m_renderLayer; }
    bool IsTwoSided() const { return m_twoSided; }
    bool GetDepthClamping() const { return m_depthClamping; }
    bool GetBlockDepthTests() const { return m_depthTests; }
    bool GetDepthWrites() const { return m_depthWrites; }
    bool CanCastShadows() const { return m_castShadows; }
    bool CanReceiveShadows() const { return m_receiveShadows; }
    uint32 GetSourceCRC() const { return m_sourceCRC; }

    // loading/saving
    bool Load(const char *resourceName, ByteStream *pStream);

    // params
    const UniformParameter *GetUniformParameter(uint32 Index) const { return &m_UniformParameters.GetElement(Index); }
    const TextureParameter *GetTextureParameter(uint32 Index) const { return &m_TextureParameters.GetElement(Index); }
    const StaticSwitchParameter *GetStaticSwitchParameter(uint32 Index) const { return &m_StaticSwitchParameters.GetElement(Index); }
    uint32 GetUniformParameterCount() const { return m_UniformParameters.GetSize(); }
    uint32 GetTextureParameterCount() const { return m_TextureParameters.GetSize(); }
    uint32 GetStaticSwitchParameterCount() const { return m_StaticSwitchParameters.GetSize(); }
    int32 FindUniformParameter(const char *Name) const;
    int32 FindTextureParameter(const char *Name) const;
    int32 FindStaticSwitchParameter(const char *Name) const;

    // device resources
    bool CreateDeviceResources() const;
    void ReleaseDeviceResources() const;

    // shader instances
    ShaderMap &GetShaderMap() const { return m_ShaderMap; }

    // helper for determining render passes to use
    uint32 SelectRenderPassMask(uint32 wantedPassMask) const;
    uint8 SelectRenderQueueLayer() const;
    GPURasterizerState *SelectRasterizerState(RENDERER_FILL_MODE fillMode = RENDERER_FILL_SOLID, RENDERER_CULL_MODE cullMode = RENDERER_CULL_BACK, bool depthBias = false, bool scissorTest = false) const;
    GPUDepthStencilState *SelectDepthStencilState(bool depthTests = true, bool depthWrites = true, GPU_COMPARISON_FUNC comparisonFunc = GPU_COMPARISON_FUNC_LESS) const;
    GPUBlendState *SelectBlendState() const;

private:
    // settings
    MATERIAL_BLENDING_MODE m_blendingMode;
    MATERIAL_LIGHTING_TYPE m_lightingType;
    MATERIAL_LIGHTING_MODEL m_lightingModel;
    MATERIAL_LIGHTING_NORMAL_SPACE m_lightingNormalSpace;
    MATERIAL_RENDER_MODE m_renderMode;
    MATERIAL_RENDER_LAYER m_renderLayer;
    bool m_twoSided;
    bool m_depthClamping;
    bool m_depthTests;
    bool m_depthWrites;
    bool m_castShadows;
    bool m_receiveShadows;
    uint32 m_sourceCRC;

    // parameters
    Array<UniformParameter> m_UniformParameters;
    Array<TextureParameter> m_TextureParameters;
    Array<StaticSwitchParameter> m_StaticSwitchParameters;

    // shader instances
    mutable ShaderMap m_ShaderMap;
};
