#pragma once
#include "ResourceCompiler/Common.h"
#include "Engine/Material.h"

struct ResourceCompilerCallbacks;

class MaterialGenerator
{
public:
    typedef KeyValuePair<String, String> ShaderUniformParameter;
    typedef KeyValuePair<String, String> ShaderTextureParameter;
    typedef KeyValuePair<String, bool> ShaderStaticSwitchParameter;

public:
    MaterialGenerator();
    ~MaterialGenerator();
    
    void Create(const char *ShaderName);
    void CreateFromMaterial(const Material *pMaterial);
    bool LoadFromXML(const char *FileName, ByteStream *pStream);
    bool SaveToXML(ByteStream *pStream);
    bool Compile(ResourceCompilerCallbacks *pCallbacks, ByteStream *pOutputStream);

    const String &GetShaderName() const { return m_strShaderName; }
    void SetShaderName(const String &ShaderName) { m_strShaderName = ShaderName; }
    void SetShaderName(const char *ShaderName) { m_strShaderName = ShaderName; }

    const uint32 GetShaderUniformParameterCount() const { return m_ShaderUniformParameters.GetSize(); }
    const uint32 GetShaderTextureParameterCount() const { return m_ShaderTextureParameters.GetSize(); }
    const uint32 GetShaderStaticSwitchParameterCount() const { return m_ShaderStaticSwitchParameters.GetSize(); }

    const ShaderUniformParameter *GetShaderUniformParameter(uint32 Index) const { DebugAssert(Index < m_ShaderUniformParameters.GetSize()); return &m_ShaderUniformParameters[Index]; }
    const ShaderTextureParameter *GetShaderTextureParameter(uint32 Index) const { DebugAssert(Index < m_ShaderTextureParameters.GetSize()); return &m_ShaderTextureParameters[Index]; }
    const ShaderStaticSwitchParameter *GetShaderStaticSwitchParameter(uint32 Index) const { DebugAssert(Index < m_ShaderStaticSwitchParameters.GetSize()); return &m_ShaderStaticSwitchParameters[Index]; }

    // find parameters
    int32 FindShaderUniformParameter(const char *Name);
    int32 FindShaderTextureParameter(const char *Name);
    int32 FindShaderStaticSwitchParameter(const char *Name);

    // remove parameters (i.e. use default value)
    void RemoveShaderUniformParameter(uint32 Index);
    void RemoveShaderTextureParameter(uint32 Index);
    void RemoveShaderStaticSwitchParameter(uint32 Index);
    bool RemoveShaderUniformParameterByName(const char *Name);
    bool RemoveShaderTextureParameterByName(const char *Name);
    bool RemoveShaderStaticSwitchParameterByName(const char *Name);

    // shader parameters
    void SetShaderUniformParameter(uint32 Index, SHADER_PARAMETER_TYPE Type, const void *pNewValue);
    void SetShaderUniformParameterString(uint32 Index, const char *NewValue);
    void SetShaderTextureParameter(uint32 Index, const Texture *pTexture);
    void SetShaderTextureParameterString(uint32 Index, const char *NewTextureName);
    void SetShaderStaticSwitchParameter(uint32 Index, bool On);

    // shader parameters by string
    void SetShaderUniformParameterByName(const char *Name, SHADER_PARAMETER_TYPE Type, const void *pNewValue);
    void SetShaderUniformParameterStringByName(const char *Name, const char *NewValue);
    void SetShaderTextureParameterByName(const char *Name, const Texture *pNewTexture);
    void SetShaderTextureParameterStringByName(const char *Name, const char *NewTextureName);
    void SetShaderStaticSwitchParameterByName(const char *Name, bool On);

    // assignment operator
    MaterialGenerator &operator=(const MaterialGenerator &copyFrom);

private:
    String m_strShaderName;

    Array<ShaderUniformParameter> m_ShaderUniformParameters;
    Array<ShaderTextureParameter> m_ShaderTextureParameters;
    Array<ShaderStaticSwitchParameter> m_ShaderStaticSwitchParameters;
};

