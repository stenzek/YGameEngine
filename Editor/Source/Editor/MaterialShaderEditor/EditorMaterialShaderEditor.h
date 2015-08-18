#pragma once
#include "Editor/Common.h"
#include "Engine/MaterialShader.h"
#include "ResourceCompiler/MaterialShaderGenerator.h"

class Material;
struct INPUT_EVENT_KEYBOARD;
struct INPUT_EVENT_MOUSE;

class EditorMaterialShaderEditorCallbacks
{
public:
    virtual void OnMaterialPropertiesChanged() = 0;
    virtual void OnMaterialParametersChanged() = 0;
};

class EditorMaterialShaderEditor
{
public:
    EditorMaterialShaderEditor(EditorMaterialShaderEditorCallbacks *pCallbacks);
    ~EditorMaterialShaderEditor();

    // accessors
    const bool GetShaderCodeAvailability(RENDERER_FEATURE_LEVEL featureLevel) const { return m_pShaderGenerator->GetShaderCodeAvailability(featureLevel); }
    String *CreateShaderCode(RENDERER_FEATURE_LEVEL featureLevel) { return m_pShaderGenerator->CreateShaderCode(featureLevel); }
    const String *GetShaderCode(RENDERER_FEATURE_LEVEL featureLevel) const { return m_pShaderGenerator->GetShaderCode(featureLevel); }
    String *GetShaderCode(RENDERER_FEATURE_LEVEL featureLevel) { return m_pShaderGenerator->GetShaderCode(featureLevel); }
    void DeleteShaderCode(RENDERER_FEATURE_LEVEL featureLevel) { return m_pShaderGenerator->DeleteShaderCode(featureLevel); }
    const bool GetShaderGraphAvailability(RENDERER_FEATURE_LEVEL featureLevel) const { return m_pShaderGenerator->GetShaderGraphAvailability(featureLevel); }
    ShaderGraph *CreateShaderGraph(RENDERER_FEATURE_LEVEL featureLevel) { return m_pShaderGenerator->CreateShaderGraph(nullptr, featureLevel); }
    const ShaderGraph *GetShaderGraph(RENDERER_FEATURE_LEVEL featureLevel) const { return m_pShaderGenerator->GetShaderGraph(featureLevel); }
    ShaderGraph *GetShaderGraph(RENDERER_FEATURE_LEVEL featureLevel) { return m_pShaderGenerator->GetShaderGraph(featureLevel); }
    void DeleteShaderGraph(RENDERER_FEATURE_LEVEL featureLevel) { return m_pShaderGenerator->DeleteShaderGraph(featureLevel); }

    // previews
    const MaterialShader *GetPreviewMaterialShader() const;
    const Material *GetPreviewMaterial() const;
    bool RegeneratePreviewMaterials() const;

    // shader loaders
    void CreateShader();
    bool LoadShader(const char *fileName, ByteStream *pStream);
    bool SaveShader(ByteStream *pStream);

    // material property getters
    const MATERIAL_BLENDING_MODE GetBlendingMode() const { return m_pShaderGenerator->GetBlendMode(); }
    const MATERIAL_LIGHTING_MODEL GetLightingModel() const { return m_pShaderGenerator->GetLightingModel(); }
    const bool GetDoubleSidedLighting() const { return m_pShaderGenerator->GetTwoSided(); }

    // material parameter getters
    const uint32 GetUniformParameterCount() const { return m_pShaderGenerator->GetUniformParameterCount(); }
    const MaterialShaderGenerator::UniformParameter *GetUniformParameter(uint32 i) const { return m_pShaderGenerator->GetUniformParameter(i); }
    const uint32 GetTextureParameterCount() const { return m_pShaderGenerator->GetTextureParameterCount(); }
    const MaterialShaderGenerator::TextureParameter *GetTextureParameter(uint32 i) const { return m_pShaderGenerator->GetTextureParameter(i); }
    const uint32 GetStaticSwitchParameterCount() const { return m_pShaderGenerator->GetStaticSwitchParameterCount(); }
    const MaterialShaderGenerator::StaticSwitchParameter *GetStaticSwitchParameter(uint32 i) const { return m_pShaderGenerator->GetStaticSwitchParameter(i); }

    // material property setters
    void SetBlendingMode(MATERIAL_BLENDING_MODE BlendingMode, bool SupressCallbacks = false);
    void SetLightingMode(MATERIAL_LIGHTING_MODEL LightingMode, bool SupressCallbacks = false);
    void SetDoubleSidedLighting(bool Enabled, bool SupressCallbacks = false);

    // material parameter manipulators
    // uniform
    bool AddUniformParameter(const char *ParameterName, const char *ParameterDefaultValue, bool SupressCallbacks = false);
    bool RenameUniformParameter(const char *ParameterName, const char *NewParameterName, bool SupressCallbacks = false);
    bool ChangeUniformParameterDefaultValue(const char *ParameterName, const char *NewDefaultValue, bool SupressCallbacks = false);
    bool RemoveUniformParameter(const char *ParameterName, bool SupressCallbacks = false);

    // texture
    bool AddTextureParameter(const char *ParameterName, TEXTURE_TYPE textureType, const char *ParameterDefaultValue, bool SupressCallbacks = false);
    bool RenameTextureParameter(const char *ParameterName, const char *NewParameterName, bool SupressCallbacks = false);
    bool ChangeTextureParameterDefaultValue(const char *ParameterName, const char *NewDefaultValue, bool SupressCallbacks = false);
    bool RemoveTextureParameter(const char *ParameterName, bool SupressCallbacks = false);

    // static switch
    bool AddStaticSwitchParameter(const char *ParameterName, const char *ParameterDefaultValue, bool SupressCallbacks = false);
    bool RenameStaticSwitchParameter(const char *ParameterName, const char *NewParameterName, bool SupressCallbacks = false);
    bool ChangeStaticSwitchParameterDefaultValue(const char *ParameterName, const char *NewDefaultValue, bool SupressCallbacks = false);
    bool RemoveStaticSwitchParameter(const char *ParameterName, bool SupressCallbacks = false);

private:
    // callback interface
    EditorMaterialShaderEditorCallbacks *m_pCallbacks;

    // shader generator, and generated shader
    MaterialShaderGenerator *m_pShaderGenerator;

    // we can't render a shader directly, it needs an actual material attached to it. so, we generate one on the fly.
    mutable MaterialShader *m_pPreviewMaterialShader;
    mutable Material *m_pPreviewMaterial;
    mutable bool m_bRegeneratePreviewMaterial;
};
