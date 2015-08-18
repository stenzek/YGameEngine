#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/ShaderGraphSchema.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "YBaseLib/XMLReader.h"
Log_SetChannel(ShaderGraphSchema);

ShaderGraphSchema *ShaderGraphSchema::GetSchemaForFeatureLevel(ResourceCompilerCallbacks *pCallbacks, RENDERER_FEATURE_LEVEL featureLevel)
{
    static const char *schemaFileNames[RENDERER_FEATURE_LEVEL_COUNT] = {
        "resources/engine/shadergraph/material_schema_es2.xml",
        "resources/engine/shadergraph/material_schema_es3.xml",
        "resources/engine/shadergraph/material_schema_sm4.xml",
        "resources/engine/shadergraph/material_schema_sm5.xml"
    };

    DebugAssert(featureLevel < RENDERER_FEATURE_LEVEL_COUNT);
    BinaryBlob *pBlob = pCallbacks->GetFileContents(schemaFileNames[featureLevel]);
    if (pBlob == nullptr)
    {
        Log_ErrorPrintf("ShaderGraphSchema::GetSchemaForFeatureLevel: Failed to get file '%s'", schemaFileNames[featureLevel]);
        return nullptr;
    }

    ByteStream *pStream = pBlob->CreateReadOnlyStream();
    ShaderGraphSchema *pSchema = new ShaderGraphSchema();
    if (!pSchema->LoadFromXML(schemaFileNames[featureLevel], pStream))
    {
        Log_ErrorPrintf("ShaderGraphSchema::GetSchemaForFeatureLevel: Failed to load file '%s'", schemaFileNames[featureLevel]);
        pSchema->Release();
        pStream->Release();
        pBlob->Release();
        return nullptr;
    }

    pStream->Release();
    pBlob->Release();
    return pSchema;
}

ShaderGraphSchema::ShaderGraphSchema()
{

}

ShaderGraphSchema::~ShaderGraphSchema()
{
    for (GlobalDeclaration *pDeclaration : m_globalDeclarations)
        delete pDeclaration;
    for (InputDeclaration *pDeclaration : m_inputDeclarations)
        delete pDeclaration;
    for (OutputDeclaration *pDeclaration : m_outputDeclarations)
        delete pDeclaration;
}

bool ShaderGraphSchema::LoadFromXML(const char *FileName, ByteStream *pStream)
{
    uint32 i;

    // set name
    //m_strName = FileName;
    //m_strName.Erase(m_strName.RFind('.'));

    // parse the xml
    XMLReader xmlReader;
    if (!xmlReader.Create(pStream, FileName) || !xmlReader.SkipToElement("schema"))
    {
        Log_ErrorPrintf("ShaderGraphSchemaManager::GetSchema: Could not initialize XML reader for schema file '%s'.", FileName);
        return false;
    }
   
    // get version attribute
    uint32 version = StringConverter::StringToUInt32(xmlReader.FetchAttributeString("version"));
    if (version < 3 || version > 3)
    {
        xmlReader.PrintError("invalid schema version: %u", version);
        return false;
    }

    // iterate nodes
    for (;;)
    {
        if (!xmlReader.NextToken())
            return false;

        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT && !xmlReader.IsEmptyElement())
        {
            int32 schemaSelection = xmlReader.Select("globals|inputs|outputs");
            if (schemaSelection < 0)
                return false;

            switch (schemaSelection)
            {
                // globals
            case 0:
                {
                    if (!xmlReader.IsEmptyElement())
                    {
                        for (;;)
                        {
                            if (!xmlReader.NextToken())
                                return false;

                            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                            {
                                int32 inputsSelection = xmlReader.Select("global");
                                if (inputsSelection < 0)
                                    return false;

                                const char *nameStr = xmlReader.FetchAttribute("name");
                                const char *displayNameStr = xmlReader.FetchAttribute("displayname");
                                const char *typeStr = xmlReader.FetchAttribute("type");
                                const char *variableStr = xmlReader.FetchAttribute("variable");
                                if (nameStr == nullptr || typeStr == nullptr || variableStr == nullptr)
                                {
                                    xmlReader.PrintError("Incomplete input declaration.");
                                    return false;
                                }

                                SHADER_PARAMETER_TYPE valueType;
                                if (!NameTable_TranslateType(NameTables::ShaderParameterType, typeStr, &valueType, true))
                                {
                                    xmlReader.PrintError("Unknown value type '%s'", typeStr);
                                    return false;
                                }

                                // make sure it doesn't already exist
                                for (i = 0; i < m_inputDeclarations.GetSize(); i++)
                                {
                                    if (Y_stricmp(m_inputDeclarations[i]->Name, nameStr) == 0)
                                    {
                                        xmlReader.PrintError("duplicate input declared: '%s'", nameStr);
                                        return false;
                                    }
                                }

                                // allocate+fill it
                                GlobalDeclaration *pGlobalDeclaration = new GlobalDeclaration();
                                pGlobalDeclaration->Name = nameStr;
                                if (displayNameStr != nullptr)
                                    pGlobalDeclaration->DisplayName = displayNameStr;
                                pGlobalDeclaration->Type = valueType;
                                pGlobalDeclaration->VariableName = variableStr;
                                pGlobalDeclaration->OutputDesc.Name = pGlobalDeclaration->Name;
                                pGlobalDeclaration->OutputDesc.Type = pGlobalDeclaration->Type;

                                // add to list
                                m_globalDeclarations.Add(pGlobalDeclaration);

                                // skip element
                                if (!xmlReader.IsEmptyElement() && !xmlReader.SkipCurrentElement())
                                    return false;
                            }
                            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                            {
                                if (!xmlReader.ExpectEndOfElementName("globals"))
                                    return false;

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

                // inputs
            case 1:
                {
                    if (!xmlReader.IsEmptyElement())
                    {
                        for (;;)
                        {
                            if (!xmlReader.NextToken())
                                return false;

                            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                            {
                                int32 inputsSelection = xmlReader.Select("input");
                                if (inputsSelection < 0)
                                    return false;

                                const char *nameStr = xmlReader.FetchAttribute("name");
                                const char *typeStr = xmlReader.FetchAttribute("type");
                                if (nameStr == nullptr || typeStr == nullptr)
                                {
                                    xmlReader.PrintError("Incomplete input declaration.");
                                    return false;
                                }

                                SHADER_PARAMETER_TYPE valueType;
                                if (!NameTable_TranslateType(NameTables::ShaderParameterType, typeStr, &valueType, true))
                                {
                                    xmlReader.PrintError("Unknown value type '%s'", typeStr);
                                    return false;
                                }

                                // make sure it doesn't already exist
                                for (i = 0; i < m_inputDeclarations.GetSize(); i++)
                                {
                                    if (Y_stricmp(m_inputDeclarations[i]->Name, nameStr) == 0)
                                    {
                                        xmlReader.PrintError("duplicate input declared: '%s'", nameStr);
                                        return false;
                                    }
                                }

                                // allocate+fill it
                                InputDeclaration *pInputDeclaration = new InputDeclaration();
                                pInputDeclaration->Name = nameStr;
                                pInputDeclaration->Type = valueType;
                                pInputDeclaration->OutputDesc.Name = pInputDeclaration->Name;
                                pInputDeclaration->OutputDesc.Type = pInputDeclaration->Type;

                                // add to list
                                m_inputDeclarations.Add(pInputDeclaration);

                                // handle access
                                if (!xmlReader.IsEmptyElement())
                                {
                                    for (;;)
                                    {
                                        if (!xmlReader.NextToken())
                                            return false;

                                        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                        {
                                            if (xmlReader.Select("access") < 0)
                                                break;

                                            const char *stageStr = xmlReader.FetchAttribute("stage");
                                            const char *defineStr = xmlReader.FetchAttribute("define");
                                            const char *variableStr = xmlReader.FetchAttribute("variable");
                                            if (stageStr == nullptr || variableStr == nullptr)
                                            {
                                                xmlReader.PrintError("Incomplete access declaration.");
                                                return false;
                                            }

                                            SHADER_PROGRAM_STAGE stageIndex;
                                            if (!NameTable_TranslateType(NameTables::ShaderProgramStage, stageStr, &stageIndex, true))
                                            {
                                                xmlReader.PrintError("Unknown stage: '%s'", stageStr);
                                                return false;
                                            }

                                            if (defineStr != nullptr)
                                                pInputDeclaration->Access[stageIndex].DefineName = defineStr;
                                            pInputDeclaration->Access[stageIndex].VariableName = variableStr;
                                        }
                                        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                        {
                                            if (!xmlReader.ExpectEndOfElementName("input"))
                                                return false;

                                            break;
                                        }
                                        else
                                        {
                                            UnreachableCode();
                                        }
                                    }
                                }
                            }
                            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                            {
                                if (!xmlReader.ExpectEndOfElementName("inputs"))
                                    return false;

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

                // outputs
            case 2:
                {
                    for (;;)
                    {
                        if (!xmlReader.NextToken())
                            return false;

                        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                        {
                            int32 outputsSelection = xmlReader.Select("output");
                            if (outputsSelection < 0)
                                return false;

                            const char *stageStr = xmlReader.FetchAttribute("stage");
                            const char *nameStr = xmlReader.FetchAttribute("name");
                            const char *displayNameStr = xmlReader.FetchAttribute("displayname");
                            const char *defineStr = xmlReader.FetchAttribute("define");
                            const char *typeStr = xmlReader.FetchAttribute("type");
                            const char *signatureStr = xmlReader.FetchAttribute("signature");
                            const char *defaultStr = xmlReader.FetchAttribute("default");
                            if (stageStr == nullptr || nameStr == nullptr || typeStr == nullptr || signatureStr == nullptr || defaultStr == nullptr)
                            {
                                xmlReader.PrintError("Incomplete output declaration.");
                                return false;
                            }

                            SHADER_PARAMETER_TYPE valueType;
                            if (!NameTable_TranslateType(NameTables::ShaderParameterType, typeStr, &valueType, true))
                            {
                                xmlReader.PrintError("Unknown value type '%s'", typeStr);
                                return false;
                            }

                            SHADER_PROGRAM_STAGE stageIndex;
                            if (!NameTable_TranslateType(NameTables::ShaderProgramStage, stageStr, &stageIndex, true))
                            {
                                xmlReader.PrintError("Unknown stage: '%s'", stageStr);
                                return false;
                            }

                            // make sure it doesn't already exist
                            for (i = 0; i < m_outputDeclarations.GetSize(); i++)
                            {
                                if (Y_stricmp(m_outputDeclarations[i]->Name, nameStr) == 0)
                                {
                                    xmlReader.PrintError("duplicate output declared: '%s'", nameStr);
                                    return false;
                                }
                            }

                            // allocate+fill it
                            OutputDeclaration *pOutputDeclaration = new OutputDeclaration();
                            pOutputDeclaration->Name = nameStr;
                            pOutputDeclaration->DisplayName = displayNameStr;
                            if (defineStr != nullptr)
                                pOutputDeclaration->DefineName = defineStr;
                            pOutputDeclaration->Stage = stageIndex;
                            pOutputDeclaration->Type = valueType;
                            pOutputDeclaration->FunctionSignature = signatureStr;
                            pOutputDeclaration->DefaultValue = defaultStr;
                            pOutputDeclaration->InputDesc.Name = pOutputDeclaration->Name;
                            pOutputDeclaration->InputDesc.Type = pOutputDeclaration->Type;
                            pOutputDeclaration->InputDesc.DefaultValue = pOutputDeclaration->DefaultValue;
                            pOutputDeclaration->InputDesc.Optional = false;

                            // add to list
                            m_outputDeclarations.Add(pOutputDeclaration);

                            // skip element
                            if (!xmlReader.IsEmptyElement() && !xmlReader.SkipCurrentElement())
                                return false;
                        }
                        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                        {
                            if (!xmlReader.ExpectEndOfElementName("outputs"))
                                return false;

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
            if (!xmlReader.ExpectEndOfElementName("schema"))
                return false;

            break;
        }
        else
        {
            UnreachableCode();
        }
    }

    return true;
}
