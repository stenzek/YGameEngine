#pragma once
#include "ResourceCompiler/Common.h"
#include "Engine/MaterialShader.h"

struct ResourceCompilerCallbacks;
class ShaderGraph;

class MaterialShaderGenerator
{
public:
    struct UniformParameter
    {
        String Name;
        SHADER_PARAMETER_TYPE Type;
        MaterialShader::UniformParameter::Value DefaultValue;
    };

    struct TextureParameter
    {
        String Name;
        TEXTURE_TYPE Type;
        String DefaultValue;
    };

    struct StaticSwitchParameter
    {
        String Name;
        bool DefaultValue;
    };

public:
    MaterialShaderGenerator();
    ~MaterialShaderGenerator();
    
    void Create(ResourceCompilerCallbacks *pCallbacks);
    bool LoadFromXML(ResourceCompilerCallbacks *pCallbacks, const char *FileName, ByteStream *pStream);
    bool SaveToXML(ByteStream *pStream);
    bool Compile(ByteStream *pOutputStream, uint32 sourceCRC);

    // getters
    MATERIAL_BLENDING_MODE GetBlendMode() const { return m_blendingMode; }
    MATERIAL_LIGHTING_TYPE GetLightingType() const { return m_lightingType; }
    MATERIAL_LIGHTING_MODEL GetLightingModel() const { return m_lightingModel; }
    MATERIAL_LIGHTING_NORMAL_SPACE GetLightingNormalSpace() const { return m_lightingNormalSpace; }
    MATERIAL_RENDER_MODE GetRenderMode() const { return m_renderMode; }
    MATERIAL_RENDER_LAYER GetRenderLayer() const { return m_renderLayer; }
    bool GetTwoSided() const { return m_twoSided; }
    bool GetDepthClamping() const { return m_depthClamping; }
    bool GetDepthTests() const { return m_depthTests; }
    bool GetDepthWrites() const { return m_depthWrites; }
    bool GetCastShadows() const { return m_castShadows; }
    bool GetReceiveShadows() const { return m_receiveShadows; }
    
    // setters
    void SetBlendMode(MATERIAL_BLENDING_MODE BlendMode) { m_blendingMode = BlendMode; }
    void SetLightingModel(MATERIAL_LIGHTING_MODEL LightingModel) { m_lightingModel = LightingModel; }
    void SetRenderMode(MATERIAL_RENDER_MODE renderMode) { m_renderMode = renderMode; }
    void SetRenderLayer(MATERIAL_RENDER_LAYER renderLayer) { m_renderLayer = renderLayer; }
    void SetTwoSided(bool Enabled) { m_twoSided = Enabled; }
    void SetDepthClamping(bool enabled) { m_depthClamping = enabled; }
    void SetDepthTests(bool enabled) { m_depthTests = enabled; }
    void SetDepthWrites(bool enabled) { m_depthWrites = enabled; }
    void SetCastShadows(bool enabled) { m_castShadows = enabled; }

    // ---- parameters ----

    // query parameters
    const uint32 GetUniformParameterCount() const { return m_UniformParameters.GetSize(); }
    const uint32 GetTextureParameterCount() const { return m_TextureParameters.GetSize(); }
    const uint32 GetStaticSwitchParameterCount() const { return m_StaticSwitchParameters.GetSize(); }
    const UniformParameter *GetUniformParameter(uint32 Index) const { return &m_UniformParameters.GetElement(Index); }
    const TextureParameter *GetTextureParameter(uint32 Index) const { return &m_TextureParameters.GetElement(Index); }
    const StaticSwitchParameter *GetStaticSwitchParameter(uint32 Index) const { return &m_StaticSwitchParameters.GetElement(Index); }

    // create parameters
    bool CreateUniformParameter(const char *Name, SHADER_PARAMETER_TYPE Type);
    bool CreateTextureParameter(const char *Name, TEXTURE_TYPE Type);
    bool CreateStaticSwitchParameter(const char *Name, const char *DefineName);

    // find parameters
    int32 FindUniformParameter(const char *Name);
    int32 FindTextureParameter(const char *Name);
    int32 FindStaticSwitchParameter(const char *Name);

    // modify parameters
    UniformParameter *GetUniformParameter(uint32 Index) { return &m_UniformParameters.GetElement(Index); }
    TextureParameter *GetTextureParameter(uint32 Index) { return &m_TextureParameters.GetElement(Index); }
    StaticSwitchParameter *GetStaticSwitchParameter(uint32 Index) { return &m_StaticSwitchParameters.GetElement(Index); }

    // remove parameters
    void RemoveUniformParameter(uint32 Index);
    void RemoveTextureParameter(uint32 Index);
    void RemoveStaticSwitchParameter(uint32 Index);

    // ---- sources ----
    const bool GetShaderCodeAvailability(RENDERER_FEATURE_LEVEL featureLevel) const { DebugAssert(featureLevel < RENDERER_FEATURE_LEVEL_COUNT); return (m_pShaderCodes[featureLevel] != nullptr); }
    const String *GetShaderCode(RENDERER_FEATURE_LEVEL featureLevel) const { DebugAssert(featureLevel < RENDERER_FEATURE_LEVEL_COUNT); return m_pShaderCodes[featureLevel]; }
    String *GetShaderCode(RENDERER_FEATURE_LEVEL featureLevel) { DebugAssert(featureLevel < RENDERER_FEATURE_LEVEL_COUNT); return m_pShaderCodes[featureLevel]; }
    String *CreateShaderCode(RENDERER_FEATURE_LEVEL featureLevel);
    void DeleteShaderCode(RENDERER_FEATURE_LEVEL featureLevel);

    const bool GetShaderGraphAvailability(RENDERER_FEATURE_LEVEL featureLevel) const { DebugAssert(featureLevel < RENDERER_FEATURE_LEVEL_COUNT); return (m_pShaderGraphs[featureLevel] != nullptr); }
    const ShaderGraph *GetShaderGraph(RENDERER_FEATURE_LEVEL featureLevel) const { DebugAssert(featureLevel < RENDERER_FEATURE_LEVEL_COUNT); return m_pShaderGraphs[featureLevel]; }
    ShaderGraph *GetShaderGraph(RENDERER_FEATURE_LEVEL featureLevel) { DebugAssert(featureLevel < RENDERER_FEATURE_LEVEL_COUNT); return m_pShaderGraphs[featureLevel]; }
    ShaderGraph *CreateShaderGraph(ResourceCompilerCallbacks *pCallbacks, RENDERER_FEATURE_LEVEL featureLevel);
    void DeleteShaderGraph(RENDERER_FEATURE_LEVEL featureLevel);

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

    // parameters
    Array<UniformParameter> m_UniformParameters;
    Array<TextureParameter> m_TextureParameters;
    Array<StaticSwitchParameter> m_StaticSwitchParameters;

    // present shaders
    String *m_pShaderCodes[RENDERER_FEATURE_LEVEL_COUNT];
    ShaderGraph *m_pShaderGraphs[RENDERER_FEATURE_LEVEL_COUNT];

private:
    MaterialShaderGenerator(const MaterialShaderGenerator &);
    MaterialShaderGenerator &operator=(const MaterialShaderGenerator &);
};

