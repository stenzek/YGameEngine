#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/ShaderGraphBuiltinNodes.h"
#include "ResourceCompiler/ShaderGraphCompiler.h"
#include "Engine/Texture.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
Log_SetChannel(ShaderGraphNode);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Global, ShaderGraphNode, "Global", "Global");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Global);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Global)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Global)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Global)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_COUNT)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_Global::LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader)
{
    if (!BaseClass::LoadFromXML(pShaderGraph, xmlReader))
        return false;

    const char *nameString = xmlReader.FetchAttribute("global");
    if (nameString == NULL)
    {
        xmlReader.PrintError("Could not read name attribute.");
        return false;
    }

    // look up global in schema
    const ShaderGraphSchema::GlobalDeclaration *pGlobalDefinition = nullptr;
    for (uint32 i = 0; i < pShaderGraph->GetSchema()->GetGlobalDeclarationCount(); i++)
    {
        const ShaderGraphSchema::GlobalDeclaration *pCurrent = pShaderGraph->GetSchema()->GetGlobalDeclaration(i);
        if (pCurrent->Name.Compare(nameString))
        {
            pGlobalDefinition = pCurrent;
            break;
        }
    }
    if (pGlobalDefinition == nullptr)
    {
        xmlReader.PrintError("Unknown global '%s' not found in schema.", nameString);
        return false;
    }

    // update name and output
    m_globalName = pGlobalDefinition->Name;
    m_pOutputs[0].SetValueType(pGlobalDefinition->Type);
    return true;
}

void ShaderGraphNode_Global::SaveToXML(XMLWriter &xmlWriter) const
{
    BaseClass::SaveToXML(xmlWriter);

    xmlWriter.WriteAttribute("global", m_globalName);
}

bool ShaderGraphNode_Global::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex < m_nOutputs);
    return pCompiler->EmitAccessShaderGlobal(m_globalName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_ShaderInputs, ShaderGraphNode, "ShaderInputs", "ShaderInputs");
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_ShaderInputs)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_ShaderInputs)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_ShaderInputs)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_ShaderInputs::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex < m_nOutputs);
    return pCompiler->EmitAccessShaderInput(m_pOutputs[OutputIndex].GetOutputDesc()->Name);
}

ShaderGraphNode_ShaderInputs::ShaderGraphNode_ShaderInputs(const ShaderGraphSchema::InputDeclaration *const *ppInputDeclarations, uint32 nInputDeclarations, const ShaderGraphNodeTypeInfo *pTypeInfo /*= &s_TypeInfo*/)
    : BaseClass(pTypeInfo)
{
    uint32 i;

    // we shouldn't have any inputs or outputs. this is ok, it is what we want.
    DebugAssert(m_nInputs == 0 && m_nOutputs == 0);

    // create outputs based on the shader input declarations
    m_nOutputs = nInputDeclarations;
    if (m_nOutputs > 0)
    {
        m_pOutputs = new ShaderGraphNodeOutput[m_nOutputs];
        for (i = 0; i < m_nOutputs; i++)
            m_pOutputs[i].Init(this, i, &ppInputDeclarations[i]->OutputDesc);
    }
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_ShaderOutputs, ShaderGraphNode, "ShaderOutputs", "ShaderOutputs");
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_ShaderOutputs)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_ShaderOutputs)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_ShaderOutputs)
END_SHADER_GRAPH_NODE_OUTPUTS()

ShaderGraphNode_ShaderOutputs::ShaderGraphNode_ShaderOutputs(const ShaderGraphSchema::OutputDeclaration *const *ppOutputDeclarations, uint32 nOutputDeclarations, const ShaderGraphNodeTypeInfo *pTypeInfo /*= &s_TypeInfo*/)
    : BaseClass(pTypeInfo)
{
    uint32 i;

    // we shouldn't have any inputs or outputs. this is ok, it is what we want.
    DebugAssert(m_nInputs == 0 && m_nOutputs == 0);

    // create inputs based on the shader output declarations
    m_nInputs = nOutputDeclarations;
    if (m_nInputs > 0)
    {
        m_pInputs = new ShaderGraphNodeInput[m_nInputs];
        for (i = 0; i < m_nInputs; i++)
            m_pInputs[i].Init(this, i, &ppOutputDeclarations[i]->InputDesc);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Parameter, ShaderGraphNode, "Parameter", "Parameter");
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Parameter)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Parameter)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Parameter)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_Parameter::LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader)
{
    if (!BaseClass::LoadFromXML(pShaderGraph, xmlReader))
        return false;

    const char *parameterNameStr = xmlReader.FetchAttribute("parametername");
    if (parameterNameStr == NULL)
    {
        xmlReader.PrintError("Could not read parameter name attribute.");
        return false;
    }

    m_strParameterName = parameterNameStr;
    return true;
}

void ShaderGraphNode_Parameter::SaveToXML(XMLWriter &xmlWriter) const
{
    BaseClass::SaveToXML(xmlWriter);

    xmlWriter.WriteAttribute("parametername", m_strParameterName);
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_UniformParameter, ShaderGraphNode_Parameter, "UniformParameter", "UniformParameter");
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_UniformParameter)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_UniformParameter)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_UniformParameter)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_UniformParameter::LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader)
{
    if (!BaseClass::LoadFromXML(pShaderGraph, xmlReader))
        return false;

//     const char *valueString = xmlReader.FetchAttribute("value");
//     if (valueString == NULL)
//     {
//         xmlReader.PrintError("Could not read value attribute.");
//         return false;
//     }
// 
//     if (!SetValueString(valueString))
//     {
//         xmlReader.PrintError("Could not set value to '%s'", valueString);
//         return false;
//     }

    return true;
}

void ShaderGraphNode_UniformParameter::SaveToXML(XMLWriter &xmlWriter) const
{
    BaseClass::SaveToXML(xmlWriter);

//     SmallString valueString;
//     GetValueString(valueString);
// 
//     xmlWriter.WriteAttribute("value", valueString);
}

bool ShaderGraphNode_UniformParameter::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);

    const ShaderGraphCompiler::ExternalParameter *pExternalParameter = pCompiler->GetExternalUniformParameter(m_strParameterName);
    if (pExternalParameter == NULL)
    {
        Log_ErrorPrintf("Shader graph node '%s' referencing nonexistant uniform parameter '%s'", m_strName.GetCharArray(), m_strParameterName.GetCharArray());
        return false;
    }

    if (pExternalParameter->Type != m_pOutputs[0].GetOutputDesc()->Type)
    {
        Log_ErrorPrintf("Shader graph node '%s' referencing uniform parameter '%s' has a type mismatch (%s vs %s)", m_strName.GetCharArray(), m_strParameterName.GetCharArray(),
                                                                                                                    NameTable_GetNameString(NameTables::ShaderParameterType, pExternalParameter->Type),
                                                                                                                    NameTable_GetNameString(NameTables::ShaderParameterType, m_pOutputs[0].GetOutputDesc()->Type));

        return false;
    }


    return pCompiler->EmitAccessExternalParameter(pExternalParameter);
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_TextureParameter, ShaderGraphNode_Parameter, "TextureParameter", "TextureParameter");
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_TextureParameter)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_TextureParameter)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_TextureParameter)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_TextureParameter::LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader)
{
    if (!BaseClass::LoadFromXML(pShaderGraph, xmlReader))
        return false;

//     const char *textureName = xmlReader.FetchAttribute("texture");
//     if (textureName == NULL)
//     {
//         xmlReader.PrintError("Could not read texture attribute.");
//         return false;
//     }
// 
//     m_strTextureName = textureName;
    return true;
}

void ShaderGraphNode_TextureParameter::SaveToXML(XMLWriter &xmlWriter) const
{
    BaseClass::SaveToXML(xmlWriter);
}

bool ShaderGraphNode_TextureParameter::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);

    const ShaderGraphCompiler::ExternalParameter *pExternalParameter = pCompiler->GetExternalTextureParameter(m_strParameterName);
    if (pExternalParameter == NULL)
    {
        Log_ErrorPrintf("Shader graph node '%s' referencing nonexistant texture parameter '%s'", m_strName.GetCharArray(), m_strParameterName.GetCharArray());
        return false;
    }

    if (pExternalParameter->Type != m_pOutputs[0].GetOutputDesc()->Type)
    {
        Log_ErrorPrintf("Shader graph node '%s' referencing texture parameter '%s' has a type mismatch (%s vs %s)", m_strName.GetCharArray(), m_strParameterName.GetCharArray(),
                                                                                                                    NameTable_GetNameString(NameTables::ShaderParameterType, pExternalParameter->Type),
                                                                                                                    NameTable_GetNameString(NameTables::ShaderParameterType, m_pOutputs[0].GetOutputDesc()->Type));

        return false;
    }


    return pCompiler->EmitAccessExternalParameter(pExternalParameter);
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_StaticSwitchParameter, ShaderGraphNode, "StaticSwitchParameter", "StaticSwitchParameter");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_StaticSwitchParameter);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_StaticSwitchParameter)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_StaticSwitchParameter)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("IfEnabled", SHADER_PARAMETER_TYPE_COUNT, "", false)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("IfDisabled", SHADER_PARAMETER_TYPE_COUNT, "", false)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_StaticSwitchParameter)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Result", SHADER_PARAMETER_TYPE_COUNT)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_StaticSwitchParameter::OnInputConnectionChange(uint32 InputIndex)
{
    // check it matches the other type
    uint32 otherInput = (InputIndex == 0) ? 1 : 0;
    if (m_pInputs[otherInput].IsLinked() && m_pInputs[InputIndex].GetValueType() != m_pInputs[otherInput].GetValueType())
        return false;

    // prevent unlinking if linked
    if (m_pOutputs[0].GetLinkCount() > 0 && !m_pInputs[InputIndex].IsLinked())
        return false;

    // set combined type
    if (m_pInputs[0].IsLinked() && m_pInputs[1].IsLinked())
    {
        DebugAssert(m_pInputs[0].GetValueType() == m_pInputs[1].GetValueType());
        m_pOutputs[0].SetValueType(m_pInputs[0].GetValueType());    
    }
    else
    {
        m_pOutputs[0].SetValueType(SHADER_PARAMETER_TYPE_COUNT);
    }

    return true;
}

bool ShaderGraphNode_StaticSwitchParameter::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);
    DebugAssert(m_pInputs[0].IsLinked());
    DebugAssert(m_pInputs[1].IsLinked());

    if (pCompiler->GetExternalStaticSwitchParameter(m_strParameterName))
        return pCompiler->EvaluateNode(&m_pInputs[0]);
    else
        return pCompiler->EvaluateNode(&m_pInputs[1]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_FloatParameter, ShaderGraphNode_UniformParameter, "FloatParameter", "FloatParameter");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_FloatParameter);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_FloatParameter)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_FloatParameter)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_FloatParameter)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_FLOAT)
END_SHADER_GRAPH_NODE_OUTPUTS()

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float2Parameter, ShaderGraphNode_UniformParameter, "Float2Parameter", "Float2Parameter");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float2Parameter);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Float2Parameter)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Float2Parameter)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Float2Parameter)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_FLOAT2)
END_SHADER_GRAPH_NODE_OUTPUTS()

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float3Parameter, ShaderGraphNode_UniformParameter, "Float3Parameter", "Float3Parameter");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float3Parameter);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Float3Parameter)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Float3Parameter)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Float3Parameter)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_FLOAT3)
END_SHADER_GRAPH_NODE_OUTPUTS()

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float4Parameter, ShaderGraphNode_UniformParameter, "Float4Parameter", "Float4Parameter");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float4Parameter);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Float4Parameter)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Float4Parameter)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Float4Parameter)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_FLOAT4)
END_SHADER_GRAPH_NODE_OUTPUTS()

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Texture2DParameter, ShaderGraphNode_TextureParameter, "Texture2DParameter", "Texture2DParameter");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Texture2DParameter);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Texture2DParameter)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Texture2DParameter)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Texture2DParameter)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Texture", SHADER_PARAMETER_TYPE_TEXTURE2D)
END_SHADER_GRAPH_NODE_OUTPUTS()

const ResourceTypeInfo *ShaderGraphNode_Texture2DParameter::GetTextureTypeInfo() const
{
    return Texture2D::StaticTypeInfo();
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Texture2DArrayParameter, ShaderGraphNode_TextureParameter, "Texture2DArrayParameter", "Texture2DArrayParameter");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Texture2DArrayParameter);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Texture2DArrayParameter)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Texture2DArrayParameter)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Texture2DArrayParameter)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Texture", SHADER_PARAMETER_TYPE_TEXTURE2DARRAY)
END_SHADER_GRAPH_NODE_OUTPUTS()

const ResourceTypeInfo *ShaderGraphNode_Texture2DArrayParameter::GetTextureTypeInfo() const
{
    return Texture2DArray::StaticTypeInfo();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Constant, ShaderGraphNode, "Constant", "Constant");
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Constant)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Constant)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Constant)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_Constant::LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader)
{
    if (!BaseClass::LoadFromXML(pShaderGraph, xmlReader))
        return false;

    const char *valueString = xmlReader.FetchAttribute("value");
    if (valueString == NULL)
    {
        xmlReader.PrintError("Could not read value attribute.");
        return false;
    }

    if (!SetValueString(valueString))
    {
        xmlReader.PrintError("Could not set value to '%s'", valueString);
        return false;
    }

    return true;
}

void ShaderGraphNode_Constant::SaveToXML(XMLWriter &xmlWriter) const
{
    BaseClass::SaveToXML(xmlWriter);

    SmallString valueString;
    GetValueString(valueString);

    xmlWriter.WriteAttribute("value", valueString);
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_FloatConstant, ShaderGraphNode_Constant, "FloatConstant", "FloatConstant");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_FloatConstant);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_FloatConstant)
    PROPERTY_TABLE_MEMBER_FLOAT("Value", 0, offsetof(ShaderGraphNode_FloatConstant, m_Value), NULL, NULL)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_FloatConstant)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_FloatConstant)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_FLOAT)
END_SHADER_GRAPH_NODE_OUTPUTS()

ShaderGraphNode_FloatConstant::ShaderGraphNode_FloatConstant(const ShaderGraphNodeTypeInfo *pTypeInfo /*= &s_TypeInfo*/)
    : BaseClass(pTypeInfo),
      m_Value(0)
{

}

bool ShaderGraphNode_FloatConstant::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);
    pCompiler->EmitConstantFloat(m_Value);
    return true;
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float2Constant, ShaderGraphNode_Constant, "Float2Constant", "Float2Constant");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float2Constant);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Float2Constant)
    PROPERTY_TABLE_MEMBER_FLOAT2("Value", 0, offsetof(ShaderGraphNode_Float2Constant, m_Value), NULL, NULL)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Float2Constant)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Float2Constant)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_FLOAT2)
END_SHADER_GRAPH_NODE_OUTPUTS()

ShaderGraphNode_Float2Constant::ShaderGraphNode_Float2Constant(const ShaderGraphNodeTypeInfo *pTypeInfo /*= &s_TypeInfo*/)
    : BaseClass(pTypeInfo),
      m_Value(float2::Zero)
{

}

bool ShaderGraphNode_Float2Constant::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);
    pCompiler->EmitConstantFloat2(m_Value);
    return true;
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float3Constant, ShaderGraphNode_Constant, "Float3Constant", "Float3Constant");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float3Constant);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Float3Constant)
    PROPERTY_TABLE_MEMBER_FLOAT3("Value", 0, offsetof(ShaderGraphNode_Float3Constant, m_Value), NULL, NULL)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Float3Constant)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Float3Constant)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_FLOAT3)
END_SHADER_GRAPH_NODE_OUTPUTS()

ShaderGraphNode_Float3Constant::ShaderGraphNode_Float3Constant(const ShaderGraphNodeTypeInfo *pTypeInfo /*= &s_TypeInfo*/)
    : BaseClass(pTypeInfo),
      m_Value(float3::Zero)
{

}

bool ShaderGraphNode_Float3Constant::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);
    pCompiler->EmitConstantFloat3(m_Value);
    return true;
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float4Constant, ShaderGraphNode_Constant, "Float4Constant", "Float4Constant");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float4Constant);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Float4Constant)
    PROPERTY_TABLE_MEMBER_FLOAT4("Value", 0, offsetof(ShaderGraphNode_Float4Constant, m_Value), NULL, NULL)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Float4Constant)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Float4Constant)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_FLOAT4)
END_SHADER_GRAPH_NODE_OUTPUTS()

ShaderGraphNode_Float4Constant::ShaderGraphNode_Float4Constant(const ShaderGraphNodeTypeInfo *pTypeInfo /*= &s_TypeInfo*/)
    : BaseClass(pTypeInfo),
      m_Value(float4::Zero)
{

}

bool ShaderGraphNode_Float4Constant::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);
    pCompiler->EmitConstantFloat4(m_Value);
    return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_TextureSample, ShaderGraphNode, "TextureSample", "TextureSample");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_TextureSample);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_TextureSample)
    //PROPERTY_TABLE_MEMBER_ENUM("UnpackOperation", NameTables::ShaderGraphTextureSampleUnpackOperation, 0, offsetof(ShaderGraphNode_TextureSample, m_eUnpackOperation), NULL, NULL)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_TextureSample)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("Texture", SHADER_PARAMETER_TYPE_COUNT, "", false)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("TexCoord", SHADER_PARAMETER_TYPE_COUNT, "", false)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_TextureSample)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Result", SHADER_PARAMETER_TYPE_FLOAT4)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_TextureSample::OnInputConnectionChange(uint32 InputIndex)
{
    SHADER_PARAMETER_TYPE expectedValueType;

    // determine expected texcoords type
    if (m_pInputs[0].IsLinked())
    {
        switch (m_pInputs[0].GetValueType())
        {
        case SHADER_PARAMETER_TYPE_TEXTURE1D:
            expectedValueType = SHADER_PARAMETER_TYPE_FLOAT;
            break;

        case SHADER_PARAMETER_TYPE_TEXTURE1DARRAY:
            expectedValueType = SHADER_PARAMETER_TYPE_FLOAT2;
            break;

        case SHADER_PARAMETER_TYPE_TEXTURE2D:
            expectedValueType = SHADER_PARAMETER_TYPE_FLOAT2;
            break;

        case SHADER_PARAMETER_TYPE_TEXTURE2DARRAY:
            expectedValueType = SHADER_PARAMETER_TYPE_FLOAT3;
            break;

        case SHADER_PARAMETER_TYPE_TEXTURE3D:
            expectedValueType = SHADER_PARAMETER_TYPE_FLOAT3;
            break;

        case SHADER_PARAMETER_TYPE_TEXTURECUBE:
            expectedValueType = SHADER_PARAMETER_TYPE_FLOAT3;
            break;

        case SHADER_PARAMETER_TYPE_TEXTURECUBEARRAY:
            expectedValueType = SHADER_PARAMETER_TYPE_FLOAT4;
            break;

        default:
            return false;
        }

        // check texcoords are valid
        if (m_pInputs[1].IsLinked())
        {
            // try a fixup swizzle (truncate)
            if (m_pInputs[1].GetValueType() != expectedValueType && !m_pInputs[1].SetFixupSwizzle(expectedValueType))
                return false;
        }
        else
        {
            // update the expected type
            m_pInputs[1].SetExpectedType(expectedValueType);
        }
    }
    else
    {
        // clear the fixup swizzle on the input node if present
        if (m_pInputs[1].IsLinked())
            m_pInputs[1].ClearFixupSwizzle();
        else
            m_pInputs[1].SetExpectedType(SHADER_PARAMETER_TYPE_COUNT);
    }

    return true;
}

bool ShaderGraphNode_TextureSample::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);
    return pCompiler->EmitTextureSample(&m_pInputs[0], &m_pInputs[1], (SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION)m_eUnpackOperation);
}

bool ShaderGraphNode_TextureSample::LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader)
{
    if (!BaseClass::LoadFromXML(pShaderGraph, xmlReader))
        return false;

    const char *unpackOperation = xmlReader.FetchAttribute("unpack-operation");
    if (unpackOperation == NULL || !NameTable_TranslateType(NameTables::ShaderGraphTextureSampleUnpackOperation, unpackOperation, &m_eUnpackOperation, true))
    {
        xmlReader.PrintError("could not read unpack operation attribute ('%s')", (unpackOperation != NULL) ? unpackOperation : "NULL");
        return false;
    }

    return true;
}

void ShaderGraphNode_TextureSample::SaveToXML(XMLWriter &xmlWriter) const
{
    BaseClass::SaveToXML(xmlWriter);

    xmlWriter.WriteAttribute("unpackoperation", NameTable_GetNameString(NameTables::ShaderGraphTextureSampleUnpackOperation, m_eUnpackOperation));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Variable, ShaderGraphNode, "Variable", "Variable");
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Variable)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Variable)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Variable)
END_SHADER_GRAPH_NODE_OUTPUTS()

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float2Variable, ShaderGraphNode, "Float2Variable", "Float2Variable");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float2Variable);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Float2Variable)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Float2Variable)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("X", SHADER_PARAMETER_TYPE_FLOAT, "0", true)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("Y", SHADER_PARAMETER_TYPE_FLOAT, "0", true)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Float2Variable)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_FLOAT2)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_Float2Variable::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);

    const ShaderGraphNodeInput *pComponentNodes[2] = { &m_pInputs[0], &m_pInputs[1] };

    return pCompiler->EmitConstructPrimitive(SHADER_PARAMETER_TYPE_FLOAT2, pComponentNodes, countof(pComponentNodes));
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float3Variable, ShaderGraphNode, "Float3Variable", "Float3Variable");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float3Variable);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Float3Variable)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Float3Variable)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("X", SHADER_PARAMETER_TYPE_FLOAT, "0", true)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("Y", SHADER_PARAMETER_TYPE_FLOAT, "0", true)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("Z", SHADER_PARAMETER_TYPE_FLOAT, "0", true)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Float3Variable)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_FLOAT3)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_Float3Variable::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);

    const ShaderGraphNodeInput *pComponentNodes[3] = { &m_pInputs[0], &m_pInputs[1], &m_pInputs[2] };

    return pCompiler->EmitConstructPrimitive(SHADER_PARAMETER_TYPE_FLOAT3, pComponentNodes, countof(pComponentNodes));
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float4Variable, ShaderGraphNode, "Float4Variable", "Float4Variable");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float4Variable);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Float4Variable)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Float4Variable)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("X", SHADER_PARAMETER_TYPE_FLOAT, "0", true)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("Y", SHADER_PARAMETER_TYPE_FLOAT, "0", true)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("Z", SHADER_PARAMETER_TYPE_FLOAT, "0", true)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("W", SHADER_PARAMETER_TYPE_FLOAT, "0", true)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Float4Variable)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_FLOAT4)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_Float4Variable::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);

    const ShaderGraphNodeInput *pComponentNodes[4] = { &m_pInputs[0], &m_pInputs[1], &m_pInputs[2], &m_pInputs[3] };

    return pCompiler->EmitConstructPrimitive(SHADER_PARAMETER_TYPE_FLOAT4, pComponentNodes, countof(pComponentNodes));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Operator, ShaderGraphNode, "Operator", "Operator");
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Operator)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Operator)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Operator)
END_SHADER_GRAPH_NODE_OUTPUTS()

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Add, ShaderGraphNode, "Add", "Add");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Add);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Add)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Add)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("A", SHADER_PARAMETER_TYPE_COUNT, "", false)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("B", SHADER_PARAMETER_TYPE_COUNT, "", false)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Add)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Result", SHADER_PARAMETER_TYPE_COUNT)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_Add::OnInputConnectionChange(uint32 InputIndex)
{
    // check it matches the other type
    uint32 otherInput = (InputIndex == 0) ? 1 : 0;
    if (m_pInputs[otherInput].IsLinked() && m_pInputs[InputIndex].GetValueType() != m_pInputs[otherInput].GetValueType())
        return false;

    // prevent unlinking if linked
    if (m_pOutputs[0].GetLinkCount() > 0 && !m_pInputs[InputIndex].IsLinked())
        return false;

    // set combined type
    if (m_pInputs[0].IsLinked() && m_pInputs[1].IsLinked())
    {
        DebugAssert(m_pInputs[0].GetValueType() == m_pInputs[1].GetValueType());
        m_pOutputs[0].SetValueType(m_pInputs[0].GetValueType());    
    }
    else
    {
        m_pOutputs[0].SetValueType(SHADER_PARAMETER_TYPE_COUNT);
    }

    return true;
}

bool ShaderGraphNode_Add::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);
    return pCompiler->EmitAdd(&m_pInputs[0], &m_pInputs[1]);
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Subtract, ShaderGraphNode, "Subtract", "Subtract");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Subtract);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Subtract)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Subtract)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("A", SHADER_PARAMETER_TYPE_COUNT, "", false)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("B", SHADER_PARAMETER_TYPE_COUNT, "", false)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Subtract)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Result", SHADER_PARAMETER_TYPE_COUNT)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_Subtract::OnInputConnectionChange(uint32 InputIndex)
{
    // check it matches the other type
    uint32 otherInput = (InputIndex == 0) ? 1 : 0;
    if (m_pInputs[otherInput].IsLinked() && m_pInputs[InputIndex].GetValueType() != m_pInputs[otherInput].GetValueType())
        return false;

    // prevent unlinking if linked
    if (m_pOutputs[0].GetLinkCount() > 0 && !m_pInputs[InputIndex].IsLinked())
        return false;

    // set combined type
    if (m_pInputs[0].IsLinked() && m_pInputs[1].IsLinked())
    {
        DebugAssert(m_pInputs[0].GetValueType() == m_pInputs[1].GetValueType());
        m_pOutputs[0].SetValueType(m_pInputs[0].GetValueType());    
    }
    else
    {
        m_pOutputs[0].SetValueType(SHADER_PARAMETER_TYPE_COUNT);
    }

    return true;
}

bool ShaderGraphNode_Subtract::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);
    return pCompiler->EmitSubtract(&m_pInputs[0], &m_pInputs[1]);
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Multiply, ShaderGraphNode, "Multiply", "Multiply");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Multiply);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Multiply)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Multiply)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("A", SHADER_PARAMETER_TYPE_COUNT, "", false)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("B", SHADER_PARAMETER_TYPE_COUNT, "", false)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Multiply)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Result", SHADER_PARAMETER_TYPE_COUNT)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_Multiply::OnInputConnectionChange(uint32 InputIndex)
{
    // check it matches the other type
    uint32 otherInput = (InputIndex == 0) ? 1 : 0;
    if (m_pInputs[otherInput].IsLinked() && m_pInputs[InputIndex].GetValueType() != m_pInputs[otherInput].GetValueType())
        return false;

    // prevent unlinking if linked
    if (m_pOutputs[0].GetLinkCount() > 0 && !m_pInputs[InputIndex].IsLinked())
        return false;

    // set combined type
    if (m_pInputs[0].IsLinked() && m_pInputs[1].IsLinked())
    {
        DebugAssert(m_pInputs[0].GetValueType() == m_pInputs[1].GetValueType());
        m_pOutputs[0].SetValueType(m_pInputs[0].GetValueType());    
    }
    else
    {
        m_pOutputs[0].SetValueType(SHADER_PARAMETER_TYPE_COUNT);
    }

    return true;
}

bool ShaderGraphNode_Multiply::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);
    return pCompiler->EmitMultiply(&m_pInputs[0], &m_pInputs[1]);
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Divide, ShaderGraphNode, "Divide", "Divide");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Divide);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Divide)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Divide)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("A", SHADER_PARAMETER_TYPE_COUNT, "", false)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("B", SHADER_PARAMETER_TYPE_COUNT, "", false)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Divide)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Result", SHADER_PARAMETER_TYPE_COUNT)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_Divide::OnInputConnectionChange(uint32 InputIndex)
{
    // check it matches the other type
    uint32 otherInput = (InputIndex == 0) ? 1 : 0;
    if (m_pInputs[otherInput].IsLinked() && m_pInputs[InputIndex].GetValueType() != m_pInputs[otherInput].GetValueType())
        return false;

    // prevent unlinking if linked
    if (m_pOutputs[0].GetLinkCount() > 0 && !m_pInputs[InputIndex].IsLinked())
        return false;

    // set combined type
    if (m_pInputs[0].IsLinked() && m_pInputs[1].IsLinked())
    {
        DebugAssert(m_pInputs[0].GetValueType() == m_pInputs[1].GetValueType());
        m_pOutputs[0].SetValueType(m_pInputs[0].GetValueType());    
    }
    else
    {
        m_pOutputs[0].SetValueType(SHADER_PARAMETER_TYPE_COUNT);
    }

    return true;
}

bool ShaderGraphNode_Divide::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);
    return pCompiler->EmitDivide(&m_pInputs[0], &m_pInputs[1]);
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Dot, ShaderGraphNode, "Dot", "Dot");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Dot);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Dot)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Dot)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("A", SHADER_PARAMETER_TYPE_COUNT, "", false)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("B", SHADER_PARAMETER_TYPE_COUNT, "", false)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Dot)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Result", SHADER_PARAMETER_TYPE_FLOAT)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_Dot::OnInputConnectionChange(uint32 InputIndex)
{
    // check it matches the other type
    uint32 otherInput = (InputIndex == 0) ? 1 : 0;
    if (m_pInputs[otherInput].IsLinked() && m_pInputs[InputIndex].GetValueType() != m_pInputs[otherInput].GetValueType())
        return false;

    // prevent unlinking if linked
    if (m_pOutputs[0].GetLinkCount() > 0 && !m_pInputs[InputIndex].IsLinked())
        return false;

    return true;
}

bool ShaderGraphNode_Dot::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);
    return pCompiler->EmitDot(&m_pInputs[0], &m_pInputs[1]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Normalize, ShaderGraphNode, "Normalize", "Normalize");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Normalize);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Normalize)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Normalize)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_COUNT, "", false)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Normalize)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_COUNT)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_Normalize::OnInputConnectionChange(uint32 InputIndex)
{
    SHADER_PARAMETER_TYPE inputValueType = m_pInputs[InputIndex].GetValueType();
    if (inputValueType != SHADER_PARAMETER_TYPE_FLOAT && inputValueType != SHADER_PARAMETER_TYPE_FLOAT2 &&
        inputValueType != SHADER_PARAMETER_TYPE_FLOAT3 && inputValueType != SHADER_PARAMETER_TYPE_FLOAT4)
    {
        return false;
    }

    m_pOutputs[0].SetValueType(inputValueType);    
    return true;
}

bool ShaderGraphNode_Normalize::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);
    
    return pCompiler->EmitNormalize(&m_pInputs[OutputIndex]);
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Negate, ShaderGraphNode, "Negate", "Negate");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Negate);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_Negate)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_Negate)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_COUNT, "", false)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_Negate)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_COUNT)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_Negate::OnInputConnectionChange(uint32 InputIndex)
{
    SHADER_PARAMETER_TYPE inputValueType = m_pInputs[InputIndex].GetValueType();
    if (inputValueType != SHADER_PARAMETER_TYPE_FLOAT && inputValueType != SHADER_PARAMETER_TYPE_FLOAT2 &&
        inputValueType != SHADER_PARAMETER_TYPE_FLOAT3 && inputValueType != SHADER_PARAMETER_TYPE_FLOAT4 &&
        inputValueType != SHADER_PARAMETER_TYPE_INT && inputValueType != SHADER_PARAMETER_TYPE_INT2 &&
        inputValueType != SHADER_PARAMETER_TYPE_INT3 && inputValueType != SHADER_PARAMETER_TYPE_INT4)
    {
        return false;
    }

    m_pOutputs[0].SetValueType(inputValueType);    
    return true;
}

bool ShaderGraphNode_Negate::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);
    
    return pCompiler->EmitNegate(&m_pInputs[OutputIndex]);
}

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_FractionalPart, ShaderGraphNode, "FractionalPart", "Fractional part");
DEFINE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_FractionalPart);
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode_FractionalPart)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode_FractionalPart)
    DEFINE_SHADER_GRAPH_NODE_INPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_COUNT, "", false)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode_FractionalPart)
    DEFINE_SHADER_GRAPH_NODE_OUTPUT_ENTRY("Value", SHADER_PARAMETER_TYPE_COUNT)
END_SHADER_GRAPH_NODE_OUTPUTS()

bool ShaderGraphNode_FractionalPart::OnInputConnectionChange(uint32 InputIndex)
{
    SHADER_PARAMETER_TYPE inputValueType = m_pInputs[InputIndex].GetValueType();
    if (inputValueType != SHADER_PARAMETER_TYPE_FLOAT && inputValueType != SHADER_PARAMETER_TYPE_FLOAT2 &&
        inputValueType != SHADER_PARAMETER_TYPE_FLOAT3 && inputValueType != SHADER_PARAMETER_TYPE_FLOAT4)
    {
        return false;
    }

    m_pOutputs[0].SetValueType(inputValueType);    
    return true;
}

bool ShaderGraphNode_FractionalPart::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    DebugAssert(OutputIndex == 0);
    
    return pCompiler->EmitFractionalPart(&m_pInputs[OutputIndex]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ShaderGraph::RegisterBuiltinTypes()
{
#define REGISTER_TYPE(Type) Type::StaticMutableTypeInfo()->RegisterType()

    REGISTER_TYPE(ShaderGraphNode);

    REGISTER_TYPE(ShaderGraphNode_Global);
    REGISTER_TYPE(ShaderGraphNode_ShaderInputs);
    REGISTER_TYPE(ShaderGraphNode_ShaderOutputs);

    REGISTER_TYPE(ShaderGraphNode_Parameter);
    REGISTER_TYPE(ShaderGraphNode_UniformParameter);
    REGISTER_TYPE(ShaderGraphNode_TextureParameter);
    REGISTER_TYPE(ShaderGraphNode_StaticSwitchParameter);

    REGISTER_TYPE(ShaderGraphNode_FloatParameter);
    REGISTER_TYPE(ShaderGraphNode_Float2Parameter);
    REGISTER_TYPE(ShaderGraphNode_Float3Parameter);
    REGISTER_TYPE(ShaderGraphNode_Float4Parameter);

    REGISTER_TYPE(ShaderGraphNode_Texture2DParameter);
    REGISTER_TYPE(ShaderGraphNode_Texture2DArrayParameter);

    REGISTER_TYPE(ShaderGraphNode_Constant);

    REGISTER_TYPE(ShaderGraphNode_FloatConstant);
    REGISTER_TYPE(ShaderGraphNode_Float2Constant);
    REGISTER_TYPE(ShaderGraphNode_Float3Constant);
    REGISTER_TYPE(ShaderGraphNode_Float4Constant);

    REGISTER_TYPE(ShaderGraphNode_Variable);
    REGISTER_TYPE(ShaderGraphNode_Float2Variable);
    REGISTER_TYPE(ShaderGraphNode_Float3Variable);
    REGISTER_TYPE(ShaderGraphNode_Float4Variable);

    REGISTER_TYPE(ShaderGraphNode_TextureSample);

    REGISTER_TYPE(ShaderGraphNode_Add);
    REGISTER_TYPE(ShaderGraphNode_Subtract);
    REGISTER_TYPE(ShaderGraphNode_Multiply);
    REGISTER_TYPE(ShaderGraphNode_Divide);
    REGISTER_TYPE(ShaderGraphNode_Dot);

    REGISTER_TYPE(ShaderGraphNode_Normalize);
    REGISTER_TYPE(ShaderGraphNode_Negate);
    REGISTER_TYPE(ShaderGraphNode_FractionalPart);

#undef REGISTER_TYPE
}

