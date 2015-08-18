#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/MaterialShaderGenerator.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "ResourceCompiler/ShaderGraph.h"
#include "Engine/Engine.h"
#include "Engine/DataFormats.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
#include "YBaseLib/CRC32.h"
Log_SetChannel(MaterialShaderGenerator);

MaterialShaderGenerator::MaterialShaderGenerator()
{
    m_blendingMode = MATERIAL_BLENDING_MODE_NONE;
    m_lightingType = MATERIAL_LIGHTING_TYPE_REFLECTIVE;
    m_lightingModel = MATERIAL_LIGHTING_MODEL_PHONG;
    m_lightingNormalSpace = MATERIAL_LIGHTING_NORMAL_SPACE_WORLD_SPACE;
    m_renderMode = MATERIAL_RENDER_MODE_NORMAL;
    m_renderLayer = MATERIAL_RENDER_LAYER_NORMAL;
    m_twoSided = false;
    m_depthClamping = false;
    m_depthTests = true;
    m_depthWrites = true;
    m_castShadows = true;
    m_receiveShadows = true;

    Y_memzero(m_pShaderCodes, sizeof(m_pShaderCodes));
    Y_memzero(m_pShaderGraphs, sizeof(m_pShaderGraphs));
}

MaterialShaderGenerator::~MaterialShaderGenerator()
{
    uint32 i;
    for (i = 0; i < RENDERER_PLATFORM_COUNT; i++)
        delete m_pShaderCodes[i];
    for (i = 0; i < RENDERER_FEATURE_LEVEL_COUNT; i++)
        delete m_pShaderGraphs[i];
}

void MaterialShaderGenerator::Create(ResourceCompilerCallbacks *pCallbacks)
{
    m_blendingMode = MATERIAL_BLENDING_MODE_NONE;
    m_lightingType = MATERIAL_LIGHTING_TYPE_REFLECTIVE;
    m_lightingModel = MATERIAL_LIGHTING_MODEL_PHONG;
    m_lightingNormalSpace = MATERIAL_LIGHTING_NORMAL_SPACE_WORLD_SPACE;
    m_renderMode = MATERIAL_RENDER_MODE_NORMAL;
    m_renderLayer = MATERIAL_RENDER_LAYER_NORMAL;
    m_twoSided = false;
    m_depthClamping = false;
    m_depthTests = true;
    m_depthWrites = true;
    m_castShadows = true;
    m_receiveShadows = true;

    CreateShaderGraph(pCallbacks, RENDERER_FEATURE_LEVEL_ES2);
}

bool MaterialShaderGenerator::LoadFromXML(ResourceCompilerCallbacks *pCallbacks, const char *FileName, ByteStream *pStream)
{
    const char *pAttributeStr;
    bool loadResult = false;

    // create xml reader
    XMLReader xmlReader;
    if (!xmlReader.Create(pStream, FileName))
    {
        Log_ErrorPrintf("Could not load material shader '%s': Failed to create XML reader.", FileName);
        goto CLEANUP;
    }

    // skip to correct node
    if (!xmlReader.SkipToElement("shader"))
    {
        Log_ErrorPrintf("Could not load material shader '%s': Failed to skip to shader element.", FileName);
        goto CLEANUP;
    }

    // start looping
    for (;;)
    {
        if (!xmlReader.NextToken())
            goto CLEANUP;

        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
        {
            int32 shaderSelection = xmlReader.Select("blending|lighting|render|parameters|sources");
            if (shaderSelection < 0)
                goto CLEANUP;

            switch (shaderSelection)
            {
                // blending
            case 0:
                {
                    // blend mode
                    pAttributeStr = xmlReader.FetchAttributeDefault("mode", "none");
                    if (!NameTable_TranslateType(NameTables::MaterialBlendingMode, pAttributeStr, &m_blendingMode, true))
                    {
                        xmlReader.PrintError("unknown blending mode '%s'", pAttributeStr);
                        goto CLEANUP;
                    }

                    if (!xmlReader.SkipCurrentElement())
                        goto CLEANUP;
                }
                break;

                // lighting
            case 1:
                {
                    // lighting type
                    pAttributeStr = xmlReader.FetchAttributeDefault("type", "Reflective");
                    if (!NameTable_TranslateType(NameTables::MaterialLightingType, pAttributeStr, &m_lightingType, true))
                    {
                        xmlReader.PrintError("unknown lighting type '%s'", pAttributeStr);
                        goto CLEANUP;
                    }

                    // lighting model
                    pAttributeStr = xmlReader.FetchAttributeDefault("model", "LambertBlinnPhong");
                    if (!NameTable_TranslateType(NameTables::MaterialLightingModel, pAttributeStr, &m_lightingModel, true))
                    {
                        xmlReader.PrintError("unknown lighting model '%s'", pAttributeStr);
                        goto CLEANUP;
                    }

                    // normal space
                    pAttributeStr = xmlReader.FetchAttributeDefault("normal-space", "World");
                    if (!NameTable_TranslateType(NameTables::MaterialLightingNormalSpace, pAttributeStr, &m_lightingNormalSpace, true))
                    {
                        xmlReader.PrintError("unknown lighting normal space '%s'", pAttributeStr);
                        goto CLEANUP;
                    }

                    // two-sided
                    pAttributeStr = xmlReader.FetchAttributeDefault("two-sided", "true");
                    m_twoSided = StringConverter::StringToBool(pAttributeStr);

                    // cast-shadows
                    pAttributeStr = xmlReader.FetchAttributeDefault("cast-shadows", "true");
                    m_castShadows = StringConverter::StringToBool(pAttributeStr);

                    // recieve-shadows
                    pAttributeStr = xmlReader.FetchAttributeDefault("receive-shadows", "true");
                    m_receiveShadows = StringConverter::StringToBool(pAttributeStr);

                    if (!xmlReader.SkipCurrentElement())
                        goto CLEANUP;
                }
                break;

                // render
            case 2:
                {
                    pAttributeStr = xmlReader.FetchAttributeDefault("mode", "Normal");
                    if (!NameTable_TranslateType(NameTables::MaterialRenderMode, pAttributeStr, &m_renderMode, true))
                    {
                        xmlReader.PrintError("unknown render mode '%s'", pAttributeStr);
                        goto CLEANUP;
                    }

                    pAttributeStr = xmlReader.FetchAttributeDefault("layer", "Normal");
                    if (!NameTable_TranslateType(NameTables::MaterialRenderLayer, pAttributeStr, &m_renderLayer, true))
                    {
                        xmlReader.PrintError("unknown render layer '%s'", pAttributeStr);
                        goto CLEANUP;
                    }

                    m_depthClamping = StringConverter::StringToBool(xmlReader.FetchAttributeDefault("depth-clamping", "false"));
                    m_depthTests = StringConverter::StringToBool(xmlReader.FetchAttributeDefault("depth-tests", "true"));
                    m_depthWrites = StringConverter::StringToBool(xmlReader.FetchAttributeDefault("depth-writes", "true"));

                    if (!xmlReader.SkipCurrentElement())
                        goto CLEANUP;
                }
                break;

                // parameters
            case 3:
                {
                    if (!xmlReader.IsEmptyElement())
                    {
                        for (;;)
                        {
                            if (!xmlReader.NextToken())
                                goto CLEANUP;

                            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                            {
                                int32 parameterSelection = xmlReader.Select("uniform|texture|staticswitch");
                                if (parameterSelection < 0)
                                    goto CLEANUP;

                                switch (parameterSelection)
                                {
                                    // uniform
                                case 0:
                                    {
                                        UniformParameter uniformParameter;

                                        // get name
                                        if ((pAttributeStr = xmlReader.FetchAttribute("name")) == NULL)
                                        {
                                            xmlReader.PrintError("missing name attribute");
                                            goto CLEANUP;
                                        }
                                        uniformParameter.Name = pAttributeStr;

                                        // get type
                                        if (!NameTable_TranslateType(NameTables::ShaderParameterType, xmlReader.FetchAttributeString("type"), &uniformParameter.Type, true))
                                        {
                                            xmlReader.PrintError("unknown uniform type '%s'", xmlReader.FetchAttributeString("type"));
                                            goto CLEANUP;
                                        }

                                        // get default value
                                        Y_memzero(&uniformParameter.DefaultValue, sizeof(uniformParameter.DefaultValue));
                                        if ((pAttributeStr = xmlReader.FetchAttribute("default")) != NULL)
                                            ShaderParameterTypeFromString(uniformParameter.Type, &uniformParameter.DefaultValue, pAttributeStr);

                                        // add to list
                                        m_UniformParameters.Add(uniformParameter);
                                    }
                                    break;

                                    // texture
                                case 1:
                                    {
                                        TextureParameter textureParameter;

                                        // get name
                                        if ((pAttributeStr = xmlReader.FetchAttribute("name")) == NULL)
                                        {
                                            xmlReader.PrintError("missing name attribute");
                                            goto CLEANUP;
                                        }
                                        textureParameter.Name = pAttributeStr;

                                        // get type
                                        if (!NameTable_TranslateType(NameTables::TextureClassNames, xmlReader.FetchAttributeString("type"), &textureParameter.Type, true))
                                        {
                                            xmlReader.PrintError("unknown texture type '%s'", xmlReader.FetchAttributeString("type"));
                                            goto CLEANUP;
                                        }

                                        // get default value
                                        pAttributeStr = xmlReader.FetchAttribute("default");
                                        if (pAttributeStr != NULL)
                                            textureParameter.DefaultValue = pAttributeStr;

                                        // add to list
                                        m_TextureParameters.Add(textureParameter);
                                    }
                                    break;

                                    // staticswitch
                                case 2:
                                    {
                                        StaticSwitchParameter staticSwitchParameter;

                                        // check count
                                        if (m_StaticSwitchParameters.GetSize() == MATERIAL_MAX_STATIC_SWITCH_COUNT)
                                        {
                                            xmlReader.PrintError("too many static switches defined, maximum is %u", (uint32)MATERIAL_MAX_STATIC_SWITCH_COUNT);
                                            goto CLEANUP;
                                        }

                                        // get name
                                        if ((pAttributeStr = xmlReader.FetchAttribute("name")) == NULL)
                                        {
                                            xmlReader.PrintError("missing name attribute");
                                            goto CLEANUP;
                                        }
                                        staticSwitchParameter.Name = pAttributeStr;

                                        // get default value
                                        staticSwitchParameter.DefaultValue = StringConverter::StringToBool(xmlReader.FetchAttributeString("default"));

                                        // add to list
                                        m_StaticSwitchParameters.Add(staticSwitchParameter);
                                    }
                                    break;

                                default:
                                    UnreachableCode();
                                    break;
                                }
                            }
                            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                            {
                                DebugAssert(!Y_stricmp(xmlReader.GetNodeName(), "parameters"));
                                break;
                            }
                        }
                    }
                }
                break;

                // sources
            case 4:
                {
                    if (!xmlReader.IsEmptyElement())
                    {
                        for (;;)
                        {
                            if (!xmlReader.NextToken())
                                goto CLEANUP;

                            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                            {
                                int32 sourceSelection = xmlReader.Select("code|graph");
                                if (sourceSelection < 0)
                                    goto CLEANUP;

                                switch (sourceSelection)
                                {
                                    // code
                                case 0:
                                    {
                                        RENDERER_FEATURE_LEVEL featureLevel;
                                        const char *featureLevelStr = xmlReader.FetchAttributeString("featurelevel");

                                        if (!NameTable_TranslateType(NameTables::RendererFeatureLevel, featureLevelStr, &featureLevel, true))
                                        {
                                            xmlReader.PrintError("invalid feature level '%s'", featureLevelStr);
                                            goto CLEANUP;
                                        }

                                        if (m_pShaderCodes[featureLevel] != NULL)
                                        {
                                            xmlReader.PrintError("code for '%s' already defined", featureLevel);
                                            goto CLEANUP;
                                        }

                                        m_pShaderCodes[featureLevel] = new String(xmlReader.GetElementText());
                                        if (!xmlReader.SkipCurrentElement())
                                            goto CLEANUP;
                                    }
                                    break;

                                    // graph
                                case 1:
                                    {
                                        RENDERER_FEATURE_LEVEL featureLevel;
                                        const char *featureLevelStr = xmlReader.FetchAttributeString("featurelevel");

                                        if (!NameTable_TranslateType(NameTables::RendererFeatureLevel, featureLevelStr, &featureLevel, true))
                                        {
                                            xmlReader.PrintError("invalid feature level '%s'", featureLevelStr);
                                            goto CLEANUP;
                                        }

                                        if (m_pShaderGraphs[featureLevel] != NULL)
                                        {
                                            xmlReader.PrintError("graph for feature level '%s' already defined", featureLevelStr);
                                            goto CLEANUP;
                                        }

                                        // get schema
                                        const ShaderGraphSchema *pSchema = ShaderGraphSchema::GetSchemaForFeatureLevel(pCallbacks, featureLevel);
                                        if (pSchema == NULL)
                                        {
                                            xmlReader.PrintError("could not load shader graph schema.");
                                            goto CLEANUP;
                                        }

                                        // create graph
                                        ShaderGraph *pShaderGraph = new ShaderGraph();
                                        if (!pShaderGraph->LoadFromXML(pSchema, xmlReader))
                                        {
                                            xmlReader.PrintError("failed to load shader graph.");
                                            pSchema->Release();
                                            delete pShaderGraph;
                                            goto CLEANUP;
                                        }

                                        // set and release
                                        m_pShaderGraphs[featureLevel] = pShaderGraph;
                                        pSchema->Release();
                                    }
                                    break;

                                default:
                                    UnreachableCode();
                                    break;
                                }
                            }
                            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                            {
                                DebugAssert(!Y_stricmp(xmlReader.GetNodeName(), "sources"));
                                break;
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
            DebugAssert(!Y_stricmp(xmlReader.GetNodeName(), "shader"));
            break;
        }
    }

    // load ok
    loadResult = true;

CLEANUP:
    return loadResult;
}

bool MaterialShaderGenerator::SaveToXML(ByteStream *pStream)
{
    uint32 i;
    SmallString tempString;
    XMLWriter xmlWriter;

    if (!xmlWriter.Create(pStream))
        return false;

    // shader
    xmlWriter.StartElement("shader");
    {
        // blending
        xmlWriter.StartElement("blending");
        {
            // mode
            xmlWriter.WriteAttribute("mode", NameTable_GetNameString(NameTables::MaterialBlendingMode, m_blendingMode));
        }
        xmlWriter.EndElement();

        // lighting
        xmlWriter.StartElement("lighting");
        {
            // type
            xmlWriter.WriteAttribute("type", NameTable_GetNameString(NameTables::MaterialLightingType, m_lightingType));

            // model
            xmlWriter.WriteAttribute("model", NameTable_GetNameString(NameTables::MaterialLightingModel, m_lightingModel));

            // two-sided
            StringConverter::BoolToString(tempString, m_twoSided);
            xmlWriter.WriteAttribute("two-sided", tempString);
        }
        xmlWriter.EndElement();

        // render
        xmlWriter.StartElement("render");
        {
            xmlWriter.WriteAttribute("mode", NameTable_GetNameString(NameTables::MaterialRenderMode, m_renderMode));
        }
        xmlWriter.EndElement();

        // parameters
        xmlWriter.StartElement("parameters");
        {
            for (i = 0; i < m_UniformParameters.GetSize(); i++)
            {
                const UniformParameter &uniformParameter = m_UniformParameters[i];
                xmlWriter.StartElement("uniform");
                {
                    xmlWriter.WriteAttribute("name", uniformParameter.Name);
                    xmlWriter.WriteAttribute("type", NameTable_GetNameString(NameTables::ShaderParameterType, uniformParameter.Type));
                    ShaderParameterTypeToString(tempString, uniformParameter.Type, &uniformParameter.DefaultValue);
                    xmlWriter.WriteAttribute("default", tempString);
                }
                xmlWriter.EndElement();
            }

            for (i = 0; i < m_TextureParameters.GetSize(); i++)
            {
                const TextureParameter &textureParameter = m_TextureParameters[i];
                xmlWriter.StartElement("texture");
                {
                    xmlWriter.WriteAttribute("name", textureParameter.Name);
                    xmlWriter.WriteAttribute("type", NameTable_GetNameString(NameTables::TextureClassNames, textureParameter.Type));
                    xmlWriter.WriteAttribute("default", textureParameter.DefaultValue);
                }
                xmlWriter.EndElement();
            }

            for (i = 0; i < m_StaticSwitchParameters.GetSize(); i++)
            {
                const StaticSwitchParameter &staticSwitchParameter = m_StaticSwitchParameters[i];
                xmlWriter.StartElement("staticswitch");
                {
                    xmlWriter.WriteAttribute("name", staticSwitchParameter.Name);

                    StringConverter::BoolToString(tempString, staticSwitchParameter.DefaultValue);
                    xmlWriter.WriteAttribute("default", tempString);
                }
                xmlWriter.EndElement();
            }
        }
        xmlWriter.EndElement();

        // sources
        xmlWriter.StartElement("sources");
        {
            for (i = 0; i < RENDERER_FEATURE_LEVEL_COUNT; i++)
            {
                if (m_pShaderCodes[i] != NULL)
                {
                    xmlWriter.StartElement("code");
                    xmlWriter.WriteAttribute("featurelevel", NameTable_GetNameString(NameTables::RendererFeatureLevel, i));
                    xmlWriter.WriteCDATA(m_pShaderCodes[i]->GetCharArray());
                    xmlWriter.EndElement();
                }
            }

            for (i = 0; i < RENDERER_FEATURE_LEVEL_COUNT; i++)
            {
                if (m_pShaderGraphs[i] != NULL)
                {
                    xmlWriter.StartElement("graph");
                    xmlWriter.WriteAttribute("featurelevel", NameTable_GetNameString(NameTables::RendererFeatureLevel, i));

                    if (!m_pShaderGraphs[i]->SaveToXML(xmlWriter))
                        return false;

                    xmlWriter.EndElement();
                }
            }
        }
        xmlWriter.EndElement();
    }
    xmlWriter.EndElement();

    return !pStream->InErrorState() && !xmlWriter.InErrorState();
}

bool MaterialShaderGenerator::CreateUniformParameter(const char *Name, SHADER_PARAMETER_TYPE Type)
{
    if (FindUniformParameter(Name) >= 0)
        return false;

    UniformParameter uniformParameter;
    uniformParameter.Name = Name;
    uniformParameter.Type = Type;
    Y_memzero(&uniformParameter.DefaultValue, sizeof(uniformParameter.DefaultValue));

    m_UniformParameters.Add(uniformParameter);
    return true;
}

bool MaterialShaderGenerator::CreateTextureParameter(const char *Name, TEXTURE_TYPE Type)
{
    if (FindTextureParameter(Name) >= 0)
        return false;

    TextureParameter textureParameter;
    textureParameter.Name = Name;
    textureParameter.Type = Type;
    
    m_TextureParameters.Add(textureParameter);
    return true;
}

bool MaterialShaderGenerator::CreateStaticSwitchParameter(const char *Name, const char *DefineName)
{
    if (FindStaticSwitchParameter(Name) >= 0)
        return false;

    StaticSwitchParameter staticSwitchParameter;
    staticSwitchParameter.Name = Name;
    staticSwitchParameter.DefaultValue = true;
    
    m_StaticSwitchParameters.Add(staticSwitchParameter);
    return true;
}

int32 MaterialShaderGenerator::FindUniformParameter(const char *Name)
{
    uint32 i;
    for (i = 0; i < m_UniformParameters.GetSize(); i++)
    {
        if (m_UniformParameters[i].Name.CompareInsensitive(Name))
            return (int32)i;
    }

    return -1;
}

int32 MaterialShaderGenerator::FindTextureParameter(const char *Name)
{
    uint32 i;
    for (i = 0; i < m_TextureParameters.GetSize(); i++)
    {
        if (m_TextureParameters[i].Name.CompareInsensitive(Name))
            return (int32)i;
    }

    return -1;
}

int32 MaterialShaderGenerator::FindStaticSwitchParameter(const char *Name)
{
    uint32 i;
    for (i = 0; i < m_StaticSwitchParameters.GetSize(); i++)
    {
        if (m_StaticSwitchParameters[i].Name.CompareInsensitive(Name))
            return (int32)i;
    }

    return -1;
}

void MaterialShaderGenerator::RemoveUniformParameter(uint32 Index)
{
    DebugAssert(Index < m_UniformParameters.GetSize());

    m_UniformParameters.OrderedRemove(Index);
}

void MaterialShaderGenerator::RemoveTextureParameter(uint32 Index)
{
    DebugAssert(Index < m_UniformParameters.GetSize());

    m_TextureParameters.OrderedRemove(Index);
}

void MaterialShaderGenerator::RemoveStaticSwitchParameter(uint32 Index)
{
    DebugAssert(Index < m_UniformParameters.GetSize());

    m_StaticSwitchParameters.OrderedRemove(Index);
}

// MaterialShaderGenerator &MaterialShaderGenerator::operator=(const MaterialShaderGenerator &copyFrom)
// {
//     m_eBlendingMode = copyFrom.m_eBlendingMode;
//     m_eLightingModel = copyFrom.m_eLightingModel;
//     m_bTwoSided = copyFrom.m_bTwoSided;
//     m_UniformParameters = copyFrom.m_UniformParameters;
//     m_TextureParameters = copyFrom.m_TextureParameters;
//     m_StaticSwitchParameters = copyFrom.m_StaticSwitchParameters;
// 
//     uint32 i;
//     for (i = 0; i < RENDERER_PLATFORM_COUNT; i++)
//     {
//         delete m_pShaderCodes[i];
//         m_pShaderCodes[i] = (copyFrom.m_pShaderCodes[i] != NULL) ? new String(*copyFrom.m_pShaderCodes[i]) : NULL;
//     }
//     for (i = 0; i < SHADER_GRAPH_FEATURE_LEVEL_COUNT; i++)
//     {
//         delete m_pShaderGraphs[i];
//         //m_pShaderGraphs[i] = (copyFrom.m_pShaderGraphs[i] != NULL) ? new ShaderGraph(*copyFrom.m_pShaderGraphs[i]) : NULL;
//     }
// 
//     return *this;
// }

String *MaterialShaderGenerator::CreateShaderCode(RENDERER_FEATURE_LEVEL featureLevel)
{
    DebugAssert(featureLevel < RENDERER_FEATURE_LEVEL_COUNT);
    Assert(m_pShaderCodes[featureLevel] == NULL);

    m_pShaderCodes[featureLevel] = new String();
    return m_pShaderCodes[featureLevel];
}

void MaterialShaderGenerator::DeleteShaderCode(RENDERER_FEATURE_LEVEL featureLevel)
{
    DebugAssert(featureLevel < RENDERER_FEATURE_LEVEL_COUNT);
    Assert(m_pShaderCodes[featureLevel] != NULL);

    delete m_pShaderCodes[featureLevel];
    m_pShaderCodes[featureLevel] = NULL;
}

ShaderGraph *MaterialShaderGenerator::CreateShaderGraph(ResourceCompilerCallbacks *pCallbacks, RENDERER_FEATURE_LEVEL featureLevel)
{
    DebugAssert(featureLevel < RENDERER_FEATURE_LEVEL_COUNT);
    Assert(m_pShaderGraphs[featureLevel] == NULL);

    const ShaderGraphSchema *pSchema = ShaderGraphSchema::GetSchemaForFeatureLevel(pCallbacks, featureLevel);
    if (pSchema == NULL)
        return NULL;

    ShaderGraph *pShaderGraph = new ShaderGraph();
    if (!pShaderGraph->Create(pSchema))
    {
        delete pShaderGraph;
        pSchema->Release();
        return NULL;
    }

    m_pShaderGraphs[featureLevel] = pShaderGraph;
    pSchema->Release();
    return pShaderGraph;
}

void MaterialShaderGenerator::DeleteShaderGraph(RENDERER_FEATURE_LEVEL featureLevel)
{
    DebugAssert(featureLevel < RENDERER_FEATURE_LEVEL_COUNT);
    Assert(m_pShaderGraphs[featureLevel] == NULL);

    delete m_pShaderGraphs[featureLevel];
    m_pShaderGraphs[featureLevel] = NULL;
}

// Compiler
bool MaterialShaderGenerator::Compile(ByteStream *pOutputStream, uint32 sourceCRC)
{
    BinaryWriter binaryWriter(pOutputStream);

    // Create header
    DF_MATERIAL_SHADER_HEADER header;
    header.Magic = DF_MATERIAL_SHADER_HEADER_MAGIC;
    header.HeaderSize = sizeof(header);
    header.BlendingMode = (uint32)m_blendingMode;
    header.LightingType = (uint32)m_lightingType;
    header.LightingModel = (uint32)m_lightingModel;
    header.LightingNormalSpace = (uint32)m_lightingNormalSpace;
    header.RenderMode = (uint32)m_renderMode;
    header.RenderLayer = (uint32)m_renderLayer;
    header.TwoSided = m_twoSided;
    header.DepthClamping = m_depthClamping;
    header.DepthTests = m_depthTests;
    header.DepthWrites = m_depthWrites;
    header.CastShadows = m_castShadows;
    header.ReceiveShadows = m_receiveShadows;
    header.SourceCRC = sourceCRC;
    header.UniformParameterCount = (uint32)m_UniformParameters.GetSize();
    header.TextureParameterCount = (uint32)m_TextureParameters.GetSize();
    header.StaticSwitchParameterCount = (uint32)m_StaticSwitchParameters.GetSize();

    // Perform overrides: translucent materials can't cast or receive shadows, nor write to the depth buffer
    if (m_blendingMode == MATERIAL_BLENDING_MODE_STRAIGHT || m_blendingMode == MATERIAL_BLENDING_MODE_PREMULTIPLIED)
        header.CastShadows = header.ReceiveShadows = header.DepthWrites = false;

    // Write header
    binaryWriter.WriteBytes(&header, sizeof(header));

    // Write uniforms
    for (uint32 i = 0; i < m_UniformParameters.GetSize(); i++)
    {
        const UniformParameter &inParameter = m_UniformParameters[i];
        DF_MATERIAL_SHADER_UNIFORM_PARAMETER outParameter;
        outParameter.NameLength = inParameter.Name.GetLength();
        outParameter.Type = (uint32)inParameter.Type;
        Y_memcpy(&outParameter.DefaultValue, &inParameter.DefaultValue, sizeof(outParameter.DefaultValue));
        binaryWriter.WriteBytes(&outParameter, sizeof(outParameter));
        binaryWriter.WriteFixedString(inParameter.Name, outParameter.NameLength);
    }

    // Write textures
    for (uint32 i = 0; i < m_TextureParameters.GetSize(); i++)
    {
        const TextureParameter &inParameter = m_TextureParameters[i];
        DF_MATERIAL_SHADER_TEXTURE_PARAMETER outParameter;
        outParameter.NameLength = inParameter.Name.GetLength();
        outParameter.Type = (uint32)inParameter.Type;
        outParameter.DefaultValueLength = inParameter.DefaultValue.GetLength();
        binaryWriter.WriteBytes(&outParameter, sizeof(outParameter));
        binaryWriter.WriteFixedString(inParameter.Name, outParameter.NameLength);
        binaryWriter.WriteFixedString(inParameter.DefaultValue, outParameter.DefaultValueLength);
    }

    // Write static switches
    for (uint32 i = 0; i < m_StaticSwitchParameters.GetSize(); i++)
    {
        const StaticSwitchParameter &inParameter = m_StaticSwitchParameters[i];
        DF_MATERIAL_SHADER_STATIC_SWITCH_PARAMETER outParameter;
        outParameter.NameLength = inParameter.Name.GetLength();
        outParameter.Mask = (uint32)1 << i;
        outParameter.DefaultValue = inParameter.DefaultValue;
        binaryWriter.WriteBytes(&outParameter, sizeof(outParameter));
        binaryWriter.WriteFixedString(inParameter.Name, outParameter.NameLength);
    }

    // Done
    return !binaryWriter.InErrorState();
}

// Interface
BinaryBlob *ResourceCompiler::CompileMaterialShader(ResourceCompilerCallbacks *pCallbacks, const char *name)
{
    SmallString sourceFileName;
    sourceFileName.Format("%s.msh.xml", name);

    BinaryBlob *pSourceData = pCallbacks->GetFileContents(sourceFileName);
    if (pSourceData == nullptr)
    {
        Log_ErrorPrintf("ResourceCompiler::CompileMaterialShader: Failed to read '%s'", sourceFileName.GetCharArray());
        return nullptr;
    }

    ByteStream *pStream = ByteStream_CreateReadOnlyMemoryStream(pSourceData->GetDataPointer(), pSourceData->GetDataSize());
    MaterialShaderGenerator *pGenerator = new MaterialShaderGenerator();
    if (!pGenerator->LoadFromXML(pCallbacks, sourceFileName, pStream))
    {
        delete pGenerator;
        pStream->Release();
        pSourceData->Release();
        return nullptr;
    }

    // calculate CRC32 of source before we nuke it
    CRC32 sourceCRC;
    sourceCRC.Update(pSourceData->GetDataPointer(), pSourceData->GetDataSize());

    // release memory
    pStream->Release();
    pSourceData->Release();

    // compile shader
    ByteStream *pOutputStream = ByteStream_CreateGrowableMemoryStream();
    if (!pGenerator->Compile(pOutputStream, sourceCRC.GetCRC()))
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
