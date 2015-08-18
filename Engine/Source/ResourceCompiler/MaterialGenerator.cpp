#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/MaterialGenerator.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "Engine/Engine.h"
#include "Engine/Texture.h"
#include "Engine/DataFormats.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
Log_SetChannel(MaterialShaderGenerator);

MaterialGenerator::MaterialGenerator()
{
    
}

MaterialGenerator::~MaterialGenerator()
{
    
}

void MaterialGenerator::Create(const char *ShaderName)
{
    m_strShaderName = ShaderName;
    m_ShaderUniformParameters.Clear();
    m_ShaderTextureParameters.Clear();
    m_ShaderStaticSwitchParameters.Clear();
}

void MaterialGenerator::CreateFromMaterial(const Material *pMaterial)
{
    uint32 i;
    SmallString tempString;
    const MaterialShader *pMaterialShader = pMaterial->GetShader();

    m_strShaderName = pMaterialShader->GetName();
    m_ShaderUniformParameters.Clear();
    m_ShaderTextureParameters.Clear();
    m_ShaderStaticSwitchParameters.Clear();

    for (i = 0; i < pMaterialShader->GetUniformParameterCount(); i++)
    {
        const MaterialShader::UniformParameter *pUniformParameter = pMaterialShader->GetUniformParameter(i);
        const MaterialShader::UniformParameter::Value *pValue = pMaterial->GetShaderUniformParameter(i);

        // check if default value
        if (Y_memcmp(pValue, &pUniformParameter->DefaultValue, sizeof(MaterialShader::UniformParameter::Value)) != 0)
        {
            // if not, add it
            ShaderParameterTypeToString(tempString, pUniformParameter->Type, reinterpret_cast<const void *>(pValue));
            m_ShaderUniformParameters.Add(ShaderUniformParameter(pUniformParameter->Name, tempString));
        }
    }

    for (i = 0; i < pMaterialShader->GetTextureParameterCount(); i++)
    {
        const MaterialShader::TextureParameter *pTextureParameter = pMaterialShader->GetTextureParameter(i);
        const MaterialShader::TextureParameter::Value *pValue = pMaterial->GetShaderTextureParameter(i);

        // check if default value
        if (pValue->pTexture != NULL)
        {
            bool isDefaultValue;
            if (pTextureParameter->DefaultValue.GetLength() == 0)
                isDefaultValue = (pValue->pTexture->GetName() != g_pEngine->GetDefaultTextureName(pTextureParameter->Type));
            else
                isDefaultValue = (pValue->pTexture->GetName() != pTextureParameter->DefaultValue);

            if (!isDefaultValue)
                m_ShaderTextureParameters.Add(ShaderTextureParameter(pTextureParameter->Name, pValue->pTexture->GetName()));
        }
    }

    for (i = 0; i < pMaterialShader->GetStaticSwitchParameterCount(); i++)
    {
        const MaterialShader::StaticSwitchParameter *pStaticSwitchParameter = pMaterialShader->GetStaticSwitchParameter(i);
        const bool Value = pMaterial->GetShaderStaticSwitchParameter(i);

        if (Value != pStaticSwitchParameter->DefaultValue)
            m_ShaderStaticSwitchParameters.Add(ShaderStaticSwitchParameter(pStaticSwitchParameter->Name, Value));
    }
}

bool MaterialGenerator::LoadFromXML(const char *FileName, ByteStream *pStream)
{
    // create xml reader
    XMLReader xmlReader;
    if (!xmlReader.Create(pStream, FileName))
    {
        xmlReader.PrintError("failed to create XML reader");
        return false;
    }

    // skip to correct node
    if (!xmlReader.SkipToElement("material"))
    {
        xmlReader.PrintError("failed to skip to material element");
        return false;
    }

    // start parsing xml
    if (!xmlReader.IsEmptyElement())
    {
        for (;;)
        {
            if (!xmlReader.NextToken())
                break;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 materialSelection = xmlReader.Select("shader");
                if (materialSelection < 0)
                    return false;

                switch (materialSelection)
                {
                    // shader
                case 0:
                    {
                        // read the name element
                        const char *materialShaderName = xmlReader.FetchAttribute("name");
                        if (materialShaderName == NULL)
                        {
                            xmlReader.PrintError("material shader name not found");
                            return false;
                        }

                        // set name
                        m_strShaderName = materialShaderName;

                        // parse parameters
                        if (!xmlReader.IsEmptyElement())
                        {
                            for (;;)
                            {
                                if (!xmlReader.NextToken())
                                    return false;

                                if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                {
                                    int32 shaderSelection = xmlReader.Select("parameters");
                                    if (shaderSelection < 0)
                                        return false;

                                    switch (shaderSelection)
                                    {
                                        // parameters
                                    case 0:
                                        {
                                            for (;;)
                                            {
                                                if (!xmlReader.NextToken())
                                                    return false;

                                                if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                                {
                                                    int32 parametersSelection = xmlReader.Select("uniform|texture|staticswitch");
                                                    if (parametersSelection < 0)
                                                        return false;

                                                    switch (parametersSelection)
                                                    {
                                                        // uniform
                                                    case 0:
                                                        {
                                                            const char *uniformName = xmlReader.FetchAttribute("name");
                                                            const char *uniformValue = xmlReader.FetchAttribute("value");
                                                            if (uniformName == NULL || uniformValue == NULL)
                                                            {
                                                                xmlReader.PrintError("incomplete uniform declaration");
                                                                return false;
                                                            }

                                                            int32 uniformIndex = FindShaderUniformParameter(uniformName);
                                                            if (uniformIndex >= 0)
                                                            {
                                                                xmlReader.PrintWarning("duplicate declared uniform '%s'", uniformName);
                                                                m_ShaderUniformParameters[uniformIndex].Value = uniformValue;
                                                            }
                                                            else
                                                            {
                                                                ShaderUniformParameter uniformParameter(uniformName, uniformValue);
                                                                m_ShaderUniformParameters.Add(uniformParameter);
                                                            }
                                                        }
                                                        break;

                                                        // texture
                                                    case 1:
                                                        {
                                                            const char *textureName = xmlReader.FetchAttribute("name");
                                                            const char *textureValue = xmlReader.FetchAttribute("value");
                                                            if (textureName == NULL || textureValue == NULL)
                                                            {
                                                                xmlReader.PrintError("incomplete texture declaration");
                                                                return false;
                                                            }

                                                            int32 textureIndex = FindShaderTextureParameter(textureName);
                                                            if (textureIndex >= 0)
                                                            {
                                                                xmlReader.PrintWarning("duplicate declared texture '%s'", textureName);
                                                                m_ShaderTextureParameters[textureIndex].Value = textureValue;
                                                            }
                                                            else
                                                            {
                                                                ShaderTextureParameter textureParameter(textureName, textureValue);
                                                                m_ShaderTextureParameters.Add(textureParameter);
                                                            }
                                                        }
                                                        break;

                                                        // staticswitch
                                                    case 2:
                                                        {
                                                            const char *staticSwitchName = xmlReader.FetchAttribute("name");
                                                            const char *staticSwitchValue = xmlReader.FetchAttribute("value");
                                                            if (staticSwitchName == NULL || staticSwitchValue == NULL)
                                                            {
                                                                xmlReader.PrintError("incomplete static switch declaration");
                                                                return false;
                                                            }

                                                            int32 staticSwitchIndex = FindShaderStaticSwitchParameter(staticSwitchName);
                                                            if (staticSwitchIndex >= 0)
                                                            {
                                                                xmlReader.PrintWarning("duplicate declared static switch '%s'", staticSwitchName);
                                                                m_ShaderStaticSwitchParameters[staticSwitchIndex].Value = StringConverter::StringToBool(staticSwitchValue);
                                                            }
                                                            else
                                                            {
                                                                ShaderStaticSwitchParameter staticSwitchParameter(staticSwitchName, StringConverter::StringToBool(staticSwitchValue));
                                                                m_ShaderStaticSwitchParameters.Add(staticSwitchParameter);
                                                            }
                                                        }
                                                        break;

                                                    default:
                                                        UnreachableCode();
                                                        break;
                                                    }
                                                }
                                                else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                                {
                                                    DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "parameters") == 0);
                                                    break;
                                                }
                                                else
                                                {
                                                    UnreachableCode();
                                                }
                                            }
                                        }
                                        break;

                                    default:
                                        UnreachableCode();
                                        break;
                                    }
                                }
                                else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                {
                                    DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "shader") == 0);
                                    break;
                                }
                                else
                                {
                                    UnreachableCode();
                                }
                            }
                        }
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "material") == 0);
                break;
            }
        }
    }

    if (m_strShaderName.GetLength() == 0)
    {
        Log_ErrorPrintf("Failed to load material '%s': No material shader defined.", FileName);
        return false;
    }

    return true;
}

bool MaterialGenerator::SaveToXML(ByteStream *pStream)
{
    uint32 i;
    SmallString tempString;

    XMLWriter xmlWriter;
    if (!xmlWriter.Create(pStream))
        return false;

    xmlWriter.StartElement("material");
    {
        xmlWriter.StartElement("shader");
        xmlWriter.WriteAttribute("name", m_strShaderName.GetCharArray());
        {
            xmlWriter.StartElement("parameters");
            {
                for (i = 0; i < m_ShaderUniformParameters.GetSize(); i++)
                {
                    const ShaderUniformParameter &uniformParameter = m_ShaderUniformParameters[i];

                    xmlWriter.StartElement("uniform");
                    xmlWriter.WriteAttribute("name", uniformParameter.Key);
                    xmlWriter.WriteAttribute("value", uniformParameter.Value);
                    xmlWriter.EndElement();
                }

                for (i = 0; i < m_ShaderTextureParameters.GetSize(); i++)
                {
                    const ShaderTextureParameter &textureParameter = m_ShaderTextureParameters[i];

                    xmlWriter.StartElement("texture");
                    xmlWriter.WriteAttribute("name", textureParameter.Key);
                    xmlWriter.WriteAttribute("value", textureParameter.Value);
                    xmlWriter.EndElement();
                }

                for (i = 0; i < m_ShaderStaticSwitchParameters.GetSize(); i++)
                {
                    const ShaderStaticSwitchParameter &staticSwitchParameter = m_ShaderStaticSwitchParameters[i];
                    
                    xmlWriter.StartElement("staticswitch");
                    xmlWriter.WriteAttribute("name", staticSwitchParameter.Key);
                    StringConverter::BoolToString(tempString, staticSwitchParameter.Value);
                    xmlWriter.WriteAttribute("value", tempString);
                    xmlWriter.EndElement();
                }

            }
            xmlWriter.EndElement();
        }
        xmlWriter.EndElement();
    }
    xmlWriter.EndElement();
    bool errorState = xmlWriter.InErrorState();
    xmlWriter.Close();

    return !errorState;
}

int32 MaterialGenerator::FindShaderUniformParameter(const char *Name)
{
    uint32 i;
    
    for (i = 0; i < m_ShaderUniformParameters.GetSize(); i++)
    {
        if (m_ShaderUniformParameters[i].Key.CompareInsensitive(Name))
            return static_cast<int32>(i);
    }

    return -1;
}

int32 MaterialGenerator::FindShaderTextureParameter(const char *Name)
{
    uint32 i;

    for (i = 0; i < m_ShaderTextureParameters.GetSize(); i++)
    {
        if (m_ShaderTextureParameters[i].Key.CompareInsensitive(Name))
            return static_cast<int32>(i);
    }

    return -1;
}

int32 MaterialGenerator::FindShaderStaticSwitchParameter(const char *Name)
{
    uint32 i;

    for (i = 0; i < m_ShaderStaticSwitchParameters.GetSize(); i++)
    {
        if (m_ShaderStaticSwitchParameters[i].Key.CompareInsensitive(Name))
            return static_cast<int32>(i);
    }

    return -1;
}

void MaterialGenerator::RemoveShaderUniformParameter(uint32 Index)
{
    DebugAssert(Index < m_ShaderUniformParameters.GetSize());
    m_ShaderUniformParameters.OrderedRemove(Index);
}

void MaterialGenerator::RemoveShaderTextureParameter(uint32 Index)
{
    DebugAssert(Index < m_ShaderTextureParameters.GetSize());
    m_ShaderTextureParameters.OrderedRemove(Index);
}

void MaterialGenerator::RemoveShaderStaticSwitchParameter(uint32 Index)
{
    DebugAssert(Index < m_ShaderStaticSwitchParameters.GetSize());
    m_ShaderStaticSwitchParameters.OrderedRemove(Index);
}

bool MaterialGenerator::RemoveShaderUniformParameterByName(const char *Name)
{
    int32 Index = FindShaderUniformParameter(Name);
    if (Index < 0)
        return false;

    m_ShaderUniformParameters.OrderedRemove(static_cast<uint32>(Index));
    return true;
}

bool MaterialGenerator::RemoveShaderTextureParameterByName(const char *Name)
{
    int32 Index = FindShaderTextureParameter(Name);
    if (Index < 0)
        return false;

    m_ShaderTextureParameters.OrderedRemove(static_cast<uint32>(Index));
    return true;
}

bool MaterialGenerator::RemoveShaderStaticSwitchParameterByName(const char *Name)
{
    int32 Index = FindShaderStaticSwitchParameter(Name);
    if (Index < 0)
        return false;

    m_ShaderStaticSwitchParameters.OrderedRemove(static_cast<uint32>(Index));
    return true;
}

void MaterialGenerator::SetShaderUniformParameter(uint32 Index, SHADER_PARAMETER_TYPE Type, const void *pNewValue)
{
    DebugAssert(Index < m_ShaderUniformParameters.GetSize());
    ShaderParameterTypeToString(m_ShaderUniformParameters[Index].Value, Type, pNewValue);
}

void MaterialGenerator::SetShaderUniformParameterString(uint32 Index, const char *NewValue)
{
    DebugAssert(Index < m_ShaderUniformParameters.GetSize());
    m_ShaderUniformParameters[Index].Value = NewValue;
}

void MaterialGenerator::SetShaderTextureParameter(uint32 Index, const Texture *pTexture)
{
    DebugAssert(Index < m_ShaderTextureParameters.GetSize());
    m_ShaderTextureParameters[Index].Value = pTexture->GetName();
}

void MaterialGenerator::SetShaderTextureParameterString(uint32 Index, const char *NewTextureName)
{
    DebugAssert(Index < m_ShaderTextureParameters.GetSize());
    m_ShaderTextureParameters[Index].Value = NewTextureName;
}

void MaterialGenerator::SetShaderStaticSwitchParameter(uint32 Index, bool On)
{
    DebugAssert(Index < m_ShaderStaticSwitchParameters.GetSize());
    m_ShaderStaticSwitchParameters[Index].Value = On;
}

void MaterialGenerator::SetShaderUniformParameterByName(const char *Name, SHADER_PARAMETER_TYPE Type, const void *pNewValue)
{
    int32 Index = FindShaderUniformParameter(Name);
    if (Index >= 0)
    {
        ShaderParameterTypeToString(m_ShaderUniformParameters[Index].Value, Type, pNewValue);
    }
    else
    {
        ShaderUniformParameter uniformParameter;
        uniformParameter.Key = Name;
        ShaderParameterTypeToString(uniformParameter.Value, Type, pNewValue);
        m_ShaderUniformParameters.Add(uniformParameter);
    }
}

void MaterialGenerator::SetShaderUniformParameterStringByName(const char *Name, const char *NewValue)
{
    int32 Index = FindShaderUniformParameter(Name);
    if (Index >= 0)
        m_ShaderUniformParameters[Index].Value = NewValue;
    else
        m_ShaderUniformParameters.Add(ShaderUniformParameter(Name, NewValue));
}

void MaterialGenerator::SetShaderTextureParameterByName(const char *Name, const Texture *pNewTexture)
{
    int32 Index = FindShaderTextureParameter(Name);
    if (Index >= 0)
        m_ShaderTextureParameters[Index].Value = pNewTexture->GetName();
    else
        m_ShaderTextureParameters.Add(ShaderTextureParameter(Name, pNewTexture->GetName()));
}

void MaterialGenerator::SetShaderTextureParameterStringByName(const char *Name, const char *NewTextureName)
{
    int32 Index = FindShaderTextureParameter(Name);
    if (Index >= 0)
        m_ShaderTextureParameters[Index].Value = NewTextureName;
    else
        m_ShaderTextureParameters.Add(ShaderTextureParameter(Name, NewTextureName));
}

void MaterialGenerator::SetShaderStaticSwitchParameterByName(const char *Name, bool On)
{
    int32 Index = FindShaderStaticSwitchParameter(Name);
    if (Index >= 0)
        m_ShaderStaticSwitchParameters[Index].Value = On;
    else
        m_ShaderStaticSwitchParameters.Add(ShaderStaticSwitchParameter(Name, On));
}

MaterialGenerator &MaterialGenerator::operator=(const MaterialGenerator &copyFrom)
{
    m_strShaderName = copyFrom.m_strShaderName;

    m_ShaderUniformParameters = copyFrom.m_ShaderUniformParameters;
    m_ShaderTextureParameters = copyFrom.m_ShaderTextureParameters;
    m_ShaderStaticSwitchParameters = copyFrom.m_ShaderStaticSwitchParameters;

    return *this;
}

// Compiler
bool MaterialGenerator::Compile(ResourceCompilerCallbacks *pCallbacks, ByteStream *pOutputStream)
{
    // Load the material we're using
    AutoReleasePtr<const MaterialShader> pMaterialShader = pCallbacks->GetCompiledMaterialShader(m_strShaderName);
    if (pMaterialShader == nullptr)
    {
        Log_ErrorPrintf("MaterialGenerator::Compile: Could not load MaterialShader '%s'", m_strShaderName.GetCharArray());
        return false;
    }

    // Writes in binary
    BinaryWriter binaryWriter(pOutputStream);

    // Generate static switch mask. We should really compare against the material for this, due to ordering.. :S
    // But that would mean having a GetCompiledMaterialShader method.. or reloading the source.. ugh.
    uint32 staticSwitchMask = 0;
    for (uint32 i = 0; i < m_ShaderStaticSwitchParameters.GetSize(); i++)
    {
        if (m_ShaderStaticSwitchParameters[i].Value)
        {
            int32 index = pMaterialShader->FindStaticSwitchParameter(m_ShaderStaticSwitchParameters[i].Key);
            if (index >= 0)
                staticSwitchMask |= pMaterialShader->GetStaticSwitchParameter((uint32)index)->Mask;
        }
    }

    // Write header
    DF_MATERIAL_HEADER header;
    header.Magic = DF_MATERIAL_HEADER_MAGIC;
    header.HeaderSize = sizeof(header);
    header.ShaderNameLength = m_strShaderName.GetLength();
    header.UniformParameterCount = pMaterialShader->GetUniformParameterCount();
    header.TextureParameterCount = pMaterialShader->GetTextureParameterCount();
    header.StaticSwitchMask = staticSwitchMask;
    binaryWriter.WriteBytes(&header, sizeof(header));
    binaryWriter.WriteFixedString(m_strShaderName, header.ShaderNameLength);

    // Write uniforms - must follow shader ordering
    for (uint32 i = 0; i < pMaterialShader->GetUniformParameterCount(); i++)
    {
        const MaterialShader::UniformParameter *pShaderParameter = pMaterialShader->GetUniformParameter(i);

        // start with default value
        MaterialShader::UniformParameter::Value value;
        Y_memcpy(&value, &pShaderParameter->DefaultValue, sizeof(value));

        // Find our version of it
        for (uint32 j = 0; j < m_ShaderUniformParameters.GetSize(); j++)
        {
            const ShaderUniformParameter &ourParameter = m_ShaderUniformParameters[j];
            if (ourParameter.Key.CompareInsensitive(pShaderParameter->Name))
            {
                // use this value instead
                ShaderParameterTypeFromString(pShaderParameter->Type, &value, ourParameter.Value);
                break;
            }
        }

        // Write it
        DF_MATERIAL_UNIFORM_PARAMETER outParameter;
        outParameter.UniformType = (uint32)pShaderParameter->Type;
        Y_memcpy(&outParameter.UniformValue, &value, sizeof(outParameter.UniformValue));
        binaryWriter.WriteBytes(&outParameter, sizeof(outParameter));
    }

    // Write textures
    for (uint32 i = 0; i < pMaterialShader->GetTextureParameterCount(); i++)
    {
        const MaterialShader::TextureParameter *pShaderParameter = pMaterialShader->GetTextureParameter(i);

        // start with default value
        String textureName = pShaderParameter->DefaultValue;

        // Find our version of it
        for (uint32 j = 0; j < m_ShaderTextureParameters.GetSize(); j++)
        {
            const ShaderTextureParameter &ourParameter = m_ShaderTextureParameters[j];
            if (ourParameter.Key.CompareInsensitive(pShaderParameter->Name))
            {
                // use this value instead
                textureName = ourParameter.Value;
                break;
            }
        }

        // Write it out
        DF_MATERIAL_TEXTURE_PARAMETER outParameter;
        outParameter.TextureNameLength = textureName.GetLength();
        outParameter.TextureType = (uint32)pShaderParameter->Type;
        binaryWriter.WriteBytes(&outParameter, sizeof(outParameter));
        binaryWriter.WriteFixedString(textureName, outParameter.TextureNameLength);
    }

    // Done
    return !binaryWriter.InErrorState();
}

// Interface
BinaryBlob *ResourceCompiler::CompileMaterial(ResourceCompilerCallbacks *pCallbacks, const char *name)
{
    SmallString sourceFileName;
    sourceFileName.Format("%s.mtl.xml", name);

    BinaryBlob *pSourceData = pCallbacks->GetFileContents(sourceFileName);
    if (pSourceData == nullptr)
    {
        Log_ErrorPrintf("ResourceCompiler::CompileMaterial: Failed to read '%s'", sourceFileName.GetCharArray());
        return nullptr;
    }

    ByteStream *pStream = ByteStream_CreateReadOnlyMemoryStream(pSourceData->GetDataPointer(), pSourceData->GetDataSize());
    MaterialGenerator *pGenerator = new MaterialGenerator();
    if (!pGenerator->LoadFromXML(sourceFileName, pStream))
    {
        delete pGenerator;
        pStream->Release();
        pSourceData->Release();
        return nullptr;
    }

    pStream->Release();
    pSourceData->Release();

    ByteStream *pOutputStream = ByteStream_CreateGrowableMemoryStream();
    if (!pGenerator->Compile(pCallbacks, pOutputStream))
    {
        pOutputStream->Release();
        delete pGenerator;
        return nullptr;
    }

    BinaryBlob *pReturnBlob = BinaryBlob::CreateFromStream(pOutputStream);
    pOutputStream->Release();
    delete pGenerator;
    return pReturnBlob;
}
