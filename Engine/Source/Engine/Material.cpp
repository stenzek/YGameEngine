#include "Engine/PrecompiledHeader.h"
#include "Engine/Material.h"
#include "Engine/Texture.h"
#include "Engine/ResourceManager.h"
#include "Engine/DataFormats.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderProgram.h"
Log_SetChannel(Material);

DEFINE_RESOURCE_TYPE_INFO(Material);
DEFINE_RESOURCE_GENERIC_FACTORY(Material);

Material::Material(const ResourceTypeInfo *pResourceTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pResourceTypeInfo)
{
    m_pShader = NULL;
    m_iShaderStaticSwitchMask = 0;
    m_bDeviceResourcesCreated = false;
}

Material::~Material()
{
    uint32 i;

    for (i = 0; i < m_ShaderTextureParameters.GetSize(); i++)
    {
        MaterialShader::TextureParameter::Value &value = m_ShaderTextureParameters[i];
        if (value.pTexture != NULL)
            value.pTexture->Release();
        else if (value.pGPUTexture != NULL)
            value.pGPUTexture->Release();
    }

    SAFE_RELEASE(m_pShader);
}

void Material::Create(const char *resourceName, const MaterialShader *pShader)
{
    // set name
    m_strName = resourceName;

    // set shader
    m_pShader = pShader;
    m_pShader->AddRef();

    // create parameter arrays
    if (m_pShader->GetUniformParameterCount() > 0)
        m_ShaderUniformParameters.Resize(m_pShader->GetUniformParameterCount());
    if (m_pShader->GetTextureParameterCount() > 0)
        m_ShaderTextureParameters.Resize(m_pShader->GetTextureParameterCount());

    // set default uniforms
    for (uint32 i = 0; i < m_pShader->GetUniformParameterCount(); i++)
        Y_memcpy(&m_ShaderUniformParameters[i], &m_pShader->GetUniformParameter(i)->DefaultValue, sizeof(MaterialShader::UniformParameter::Value));

    // set default textures
    for (uint32 i = 0; i < m_pShader->GetTextureParameterCount(); i++)
    {
        const MaterialShader::TextureParameter *pTextureParameter = m_pShader->GetTextureParameter(i);
        const Texture *pTexture = NULL;
        if (pTextureParameter->DefaultValue.GetLength() > 0)
        {
            pTexture = g_pResourceManager->GetTexture(pTextureParameter->DefaultValue);
            if (pTexture != NULL && pTexture->GetTextureType() != pTextureParameter->Type)
            {
                pTexture->Release();
                pTexture = NULL;
            }
        }

        if (pTexture != NULL)
            m_ShaderTextureParameters[i].pTexture = pTexture;
        else
            m_ShaderTextureParameters[i].pTexture = g_pResourceManager->GetDefaultTexture(pTextureParameter->Type);

        m_ShaderTextureParameters[i].pGPUTexture = NULL;
    }

    // set default staticswitch mask
    for (uint32 i = 0; i < m_pShader->GetStaticSwitchParameterCount(); i++)
    {
        const MaterialShader::StaticSwitchParameter *pStaticSwitchParameter = m_pShader->GetStaticSwitchParameter(i);
        if (pStaticSwitchParameter->DefaultValue)
            m_iShaderStaticSwitchMask |= pStaticSwitchParameter->Mask;
    }
}

bool Material::Load(const char *resourceName, ByteStream *pStream)
{
    BinaryReader binaryReader(pStream);

    // set name
    m_strName = resourceName;

    // read header
    DF_MATERIAL_HEADER header;
    if (!binaryReader.SafeReadBytes(&header, sizeof(header)) || header.Magic != DF_MATERIAL_HEADER_MAGIC || header.HeaderSize != sizeof(header))
        return false;

    // read shader name
    SmallString materialShaderName;
    if (!binaryReader.SafeReadFixedString(header.ShaderNameLength, &materialShaderName))
        return false;

    // get shader
    if ((m_pShader = g_pResourceManager->GetMaterialShader(materialShaderName)) == nullptr)
    {
        Log_ErrorPrintf("Could not load material '%s': shader '%s' not found", resourceName, materialShaderName.GetCharArray());
        return false;
    }

    // compare our sizes against the shader, it should match, if not, we're outdated
    if (header.UniformParameterCount != m_pShader->GetUniformParameterCount() || header.TextureParameterCount != m_pShader->GetTextureParameterCount())
    {
        Log_ErrorPrintf("Could not load material '%s': parameter mismatch with shader '%s'", resourceName, materialShaderName.GetCharArray());
        return false;
    }

    // load uniform parameters
    m_ShaderUniformParameters.Resize(header.UniformParameterCount);
    m_ShaderUniformParameters.ZeroContents();
    for (uint32 i = 0; i < m_ShaderUniformParameters.GetSize(); i++)
    {
        DF_MATERIAL_UNIFORM_PARAMETER uniformParameter;
        if (!binaryReader.SafeReadBytes(&uniformParameter, sizeof(uniformParameter)))
            return false;

        // validate against shader
        if (uniformParameter.UniformType != (uint32)m_pShader->GetUniformParameter(i)->Type)
        {
            Log_ErrorPrintf("Could not load material '%s': uniform parameter %u type mismatch with shader '%s'", resourceName, i, materialShaderName.GetCharArray());
            return false;
        }

        // store value
        Y_memcpy(&m_ShaderUniformParameters[i], uniformParameter.UniformValue, sizeof(MaterialShader::UniformParameter::Value));
    }

    // load texture parameters
    m_ShaderTextureParameters.Resize(header.TextureParameterCount);
    m_ShaderTextureParameters.ZeroContents();
    for (uint32 i = 0; i < m_ShaderTextureParameters.GetSize(); i++)
    {
        DF_MATERIAL_TEXTURE_PARAMETER textureParameter;
        if (!binaryReader.SafeReadBytes(&textureParameter, sizeof(textureParameter)))
            return false;

        // validate against shader
        if (textureParameter.TextureType != (uint32)m_pShader->GetTextureParameter(i)->Type)
        {
            Log_ErrorPrintf("Could not load material '%s': texture parameter %u type mismatch with shader '%s'", resourceName, i, materialShaderName.GetCharArray());
            return false;
        }

        // read texture name
        SmallString textureName;
        if (!binaryReader.SafeReadFixedString(textureParameter.TextureNameLength, &textureName))
            return false;

        // load texture
        const Texture *pTexture = g_pResourceManager->GetTexture(textureName);
        if (pTexture == nullptr)
        {
            Log_WarningPrintf("In material '%s': texture parameter %u could not be loaded '%s'", resourceName, i, textureName.GetCharArray());
            pTexture = g_pResourceManager->GetDefaultTexture((TEXTURE_TYPE)textureParameter.TextureType);
        }
        else if (pTexture->GetTextureType() != (TEXTURE_TYPE)textureParameter.TextureType)
        {
            Log_WarningPrintf("In material '%s': texture parameter %u is bad type '%s'", resourceName, i, textureName.GetCharArray());
            pTexture->Release();
            pTexture = g_pResourceManager->GetDefaultTexture((TEXTURE_TYPE)textureParameter.TextureType);
        }

        // set pointer
        m_ShaderTextureParameters[i].pTexture = pTexture;
        m_ShaderTextureParameters[i].pGPUTexture = nullptr;
    }

    // store static switch mask
    m_iShaderStaticSwitchMask = header.StaticSwitchMask;
    return true;
}

bool Material::CreateDeviceResources() const
{
    uint32 i;
    if (m_bDeviceResourcesCreated)
        return true;

    for (i = 0; i < m_ShaderTextureParameters.GetSize(); i++)
    {
        const MaterialShader::TextureParameter::Value &value = m_ShaderTextureParameters[i];
        if (value.pTexture != NULL && !value.pTexture->CreateDeviceResources())
            return false;
    }
    
    m_bDeviceResourcesCreated = true;
    return true;
}

bool Material::BindDeviceResources(GPUContext *pContext, ShaderProgram *pProgram) const
{
    uint32 i;

    if (!m_bDeviceResourcesCreated && !CreateDeviceResources())
        return false;

    for (i = 0; i < m_ShaderUniformParameters.GetSize(); i++)
        pProgram->SetMaterialParameterValue(pContext, i, m_pShader->GetUniformParameter(i)->Type, &m_ShaderUniformParameters[i]);

    for (i = 0; i < m_ShaderTextureParameters.GetSize(); i++)
    {
        const MaterialShader::TextureParameter::Value &value = m_ShaderTextureParameters[i];
        pProgram->SetMaterialParameterResource(pContext, i, (value.pGPUTexture != NULL) ? value.pGPUTexture : value.pTexture->GetGPUTexture());
    }
    
    return true;
}

void Material::ReleaseDeviceResources() const
{
    m_bDeviceResourcesCreated = false;

    // kill any references to direct gpu textures
    for (uint32 i = 0; i < m_ShaderTextureParameters.GetSize(); i++)
    {
        MaterialShader::TextureParameter::Value *pValue = const_cast<MaterialShader::TextureParameter::Value *>(&m_ShaderTextureParameters[i]);
        SAFE_RELEASE(pValue->pGPUTexture);
    }
}

void Material::SetShaderUniformParameterString(uint32 Index, const char *NewValue)
{
    const MaterialShader::UniformParameter *pUniformParameter = m_pShader->GetUniformParameter(Index);
    ShaderParameterTypeFromString(pUniformParameter->Type, &m_ShaderUniformParameters[Index], NewValue);
}

bool Material::SetShaderTextureParameterString(uint32 Index, const char *NewValue)
{
    const MaterialShader::TextureParameter *pTextureParameter = m_pShader->GetTextureParameter(Index);
    
    const Texture *pTexture = g_pResourceManager->GetTexture(NewValue);
    if (pTexture != NULL && pTexture->GetTextureType() == pTextureParameter->Type)
    {
        pTexture->AddRef();

        MaterialShader::TextureParameter::Value &value = m_ShaderTextureParameters[Index];
        if (value.pGPUTexture != NULL)
        {
            DebugAssert(value.pTexture == NULL);
            value.pGPUTexture->Release();
            value.pGPUTexture = NULL;
            value.pTexture = pTexture;
        }
        else if (value.pTexture != NULL)
        {
            DebugAssert(value.pGPUTexture == NULL);
            value.pTexture->Release();
            value.pTexture = pTexture;
        }

        if (m_bDeviceResourcesCreated)
        {
            // todo: log warning if fail
            pTexture->CreateDeviceResources();
        }

        return true;
    }
    else if (pTexture != NULL)
    {
        pTexture->Release();
    }

    return false;
}

void Material::SetShaderUniformParameter(uint32 Index, SHADER_PARAMETER_TYPE Type, const void *pNewValue)
{
    const MaterialShader::UniformParameter *pUniformParameter = m_pShader->GetUniformParameter(Index);

    uint32 typeSize = ShaderParameterValueTypeSize(pUniformParameter->Type);
    if (typeSize > 0)
        Y_memcpy(&m_ShaderUniformParameters[Index], pNewValue, typeSize);
}

bool Material::SetShaderTextureParameter(uint32 Index, const Texture *pNewTexture)
{
    const MaterialShader::TextureParameter *pTextureParameter = m_pShader->GetTextureParameter(Index);

    if (pNewTexture == NULL || pNewTexture->GetTextureType() != pTextureParameter->Type)
        return false;

    pNewTexture->AddRef();

    MaterialShader::TextureParameter::Value &value = m_ShaderTextureParameters[Index];
    if (value.pGPUTexture != NULL)
    {
        DebugAssert(value.pTexture == NULL);
        value.pGPUTexture->Release();
        value.pGPUTexture = NULL;
        value.pTexture = pNewTexture;
    }
    else if (value.pTexture != NULL)
    {
        DebugAssert(value.pGPUTexture == NULL);
        value.pTexture->Release();
        value.pTexture = pNewTexture;
    }

    if (m_bDeviceResourcesCreated)
        pNewTexture->CreateDeviceResources();

    return true;
}

bool Material::SetShaderTextureParameter(uint32 Index, GPUTexture *pNewTexture)
{
    const MaterialShader::TextureParameter *pTextureParameter = m_pShader->GetTextureParameter(Index);
    MaterialShader::TextureParameter::Value &value = m_ShaderTextureParameters[Index];

    if (pNewTexture == NULL)
    {
        // reset back to the default value if a gpu texture is currently bound
        if (value.pGPUTexture == NULL)
            return false;

        // find default value
        const Texture *pTexture = NULL;
        if (pTextureParameter->DefaultValue.GetLength() > 0)
        {
            pTexture = g_pResourceManager->GetTexture(pTextureParameter->DefaultValue);
            if (pTexture != NULL && pTexture->GetTextureType() != pTextureParameter->Type)
            {
                pTexture->Release();
                pTexture = NULL;
            }
        }

        // kill old value
        DebugAssert(value.pTexture == NULL);
        value.pGPUTexture->Release();
        value.pGPUTexture = NULL;

        // set
        if (pTexture != NULL)
            m_ShaderTextureParameters[Index].pTexture = pTexture;
        else
            m_ShaderTextureParameters[Index].pTexture = g_pResourceManager->GetDefaultTexture(pTextureParameter->Type);

        // ok
        return true;
    }

    if (pNewTexture->GetTextureType() != pTextureParameter->Type)
        return false;

    pNewTexture->AddRef();

    if (value.pGPUTexture != NULL)
    {
        DebugAssert(value.pTexture == NULL);
        value.pGPUTexture->Release();
        value.pGPUTexture = pNewTexture;
    }
    else if (value.pTexture != NULL)
    {
        DebugAssert(value.pGPUTexture == NULL);
        value.pTexture->Release();
        value.pTexture = NULL;
        value.pGPUTexture = pNewTexture;
    }

    return true;
}

void Material::SetShaderStaticSwitchParameter(uint32 Index, bool On)
{
    DebugAssert(Index < m_pShader->GetStaticSwitchParameterCount());

    const MaterialShader::StaticSwitchParameter *pParameter = m_pShader->GetStaticSwitchParameter(Index);
    if (On)
        m_iShaderStaticSwitchMask |= pParameter->Mask;
    else
        m_iShaderStaticSwitchMask &= ~pParameter->Mask;
}

bool Material::SetShaderUniformParameterStringByName(const char *Name, const char *NewValue)
{
    int32 Index = m_pShader->FindUniformParameter(Name);
    if (Index < 0)
        return false;

    SetShaderUniformParameterString(Index, NewValue);
    return true;
}

bool Material::SetShaderTextureParameterStringByName(const char *Name, const char *NewValue)
{
    int32 Index = m_pShader->FindTextureParameter(Name);
    return (Index >= 0) ? SetShaderTextureParameterString(Index, NewValue) : false;
}

bool Material::SetShaderUniformParameterByName(const char *Name, SHADER_PARAMETER_TYPE Type, const void *pNewValue)
{
    int32 Index = m_pShader->FindUniformParameter(Name);
    if (Index < 0)
        return false;

    SetShaderUniformParameter(Index, Type, pNewValue);
    return true;
}

bool Material::SetShaderTextureParameterByName(const char *Name, const Texture *pNewTexture)
{
    int32 Index = m_pShader->FindTextureParameter(Name);
    if (Index < 0)
        return false;

    return SetShaderTextureParameter(Index, pNewTexture);
}

bool Material::SetShaderTextureParameterByName(const char *Name, GPUTexture *pNewTexture)
{
    int32 Index = m_pShader->FindTextureParameter(Name);
    if (Index < 0)
        return false;

    return SetShaderTextureParameter(Index, pNewTexture);
}

bool Material::SetShaderStaticSwitchParameterByName(const char *Name, bool On)
{
    int32 Index = m_pShader->FindStaticSwitchParameter(Name);
    if (Index < 0)
        return false;

    SetShaderStaticSwitchParameter(Index, On);
    return true;
}

//////////////////////////////////////////////////////////////////////////
