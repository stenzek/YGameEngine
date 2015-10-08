#pragma once
#include "Engine/Common.h"
#include "Engine/MaterialShader.h"
#include "Renderer/RendererTypes.h"

class GPUTexture;
class Texture;
class GPUSamplerState;
class ShaderProgram;

class Material : public Resource
{
    DECLARE_RESOURCE_TYPE_INFO(Material, Resource);
    DECLARE_RESOURCE_GENERIC_FACTORY(Material);

public:
    Material(const ResourceTypeInfo *pResourceTypeInfo = &s_TypeInfo);
    ~Material();

    const MaterialShader *GetShader() const { return m_pShader; }
    uint32 GetShaderStaticSwitchMask() const { return m_iShaderStaticSwitchMask; }

    // new
    void Create(const char *resourceName, const MaterialShader *pShader);

    // serialization
    bool Load(const char *resourceName, ByteStream *pStream);

    // device resource management
    bool CreateDeviceResources() const;
    bool BindDeviceResources(GPUCommandList *pCommandList, ShaderProgram *pProgram) const;
    void ReleaseDeviceResources() const;

    // wrappers to shader
    const MaterialShader::UniformParameter::Value *GetShaderUniformParameter(uint32 i) const { return &m_ShaderUniformParameters[i]; }
    const MaterialShader::TextureParameter::Value *GetShaderTextureParameter(uint32 i) const { return &m_ShaderTextureParameters[i]; }
    const bool GetShaderStaticSwitchParameter(uint32 i) const { return (m_iShaderStaticSwitchMask & m_pShader->GetStaticSwitchParameter(i)->Mask) != 0; }

    // shader parameters
    void SetShaderUniformParameter(uint32 Index, SHADER_PARAMETER_TYPE Type, const void *pNewValue);
    void SetShaderUniformParameterString(uint32 Index, const char *NewValue);
    bool SetShaderTextureParameter(uint32 Index, const Texture *pNewTexture);
    bool SetShaderTextureParameter(uint32 Index, GPUTexture *pNewTexture);
    bool SetShaderTextureParameterString(uint32 Index, const char *NewValue);
    void SetShaderStaticSwitchParameter(uint32 Index, bool On);

    // shader parameters by string
    bool SetShaderUniformParameterByName(const char *Name, SHADER_PARAMETER_TYPE Type, const void *pNewValue);
    bool SetShaderUniformParameterStringByName(const char *Name, const char *NewValue);
    bool SetShaderTextureParameterByName(const char *Name, const Texture *pNewTexture);
    bool SetShaderTextureParameterByName(const char *Name, GPUTexture *pNewTexture);
    bool SetShaderTextureParameterStringByName(const char *Name, const char *NewValue);
    bool SetShaderStaticSwitchParameterByName(const char *Name, bool On);

    // forwarders to material shader
    uint32 GetRenderPassMask(uint32 WantedPassMask) const { return m_pShader->SelectRenderPassMask(WantedPassMask); }
    GPUBlendState *SelectBlendState() const { return m_pShader->SelectBlendState(); }

private:
    const MaterialShader *m_pShader;
    MemArray<MaterialShader::UniformParameter::Value> m_ShaderUniformParameters;
    MemArray<MaterialShader::TextureParameter::Value> m_ShaderTextureParameters;
    uint32 m_iShaderStaticSwitchMask;
    mutable bool m_bDeviceResourcesCreated;
};

