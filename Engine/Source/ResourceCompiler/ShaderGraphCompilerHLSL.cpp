#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/ShaderGraphCompilerHLSL.h"
#include "ResourceCompiler/ShaderGraphBuiltinNodes.h"
Log_SetChannel(ShaderGraphCompilerHLSL);

ShaderGraphCompilerHLSL::ShaderGraphCompilerHLSL(const ShaderGraph *pShaderGraph)
    : ShaderGraphCompiler(pShaderGraph),
      m_currentStage(SHADER_PROGRAM_STAGE_COUNT)
{

}

ShaderGraphCompilerHLSL::~ShaderGraphCompilerHLSL()
{

}

bool ShaderGraphCompilerHLSL::Compile(ByteStream *pStream)
{
    if (!InternalCompile())
        return false;

    // output to sink
    static const char header[] = "// Begin generated shader graph code.\n\n";
    static const char footer[] = "// End generated shader graph code.\n\n";
    bool writeResult = true;
    SmallString tempStr;

    writeResult &= pStream->Write2(header, sizeof(header) - 1);

    for (CompileDefineMap::Iterator itr = m_defines.Begin(); !itr.AtEnd(); itr.Forward())
    {
        tempStr.Format("#define %s %s\n", itr->Key.GetCharArray(), itr->Value.GetCharArray());
        writeResult &= pStream->Write2(tempStr.GetCharArray(), tempStr.GetLength());
    }

    if (m_externsCode.GetLength() > 0)
        writeResult &= pStream->Write2(m_externsCode.GetCharArray(), m_externsCode.GetLength());

    if (m_globalsCode.GetLength() > 0)
        writeResult &= pStream->Write2(m_globalsCode.GetCharArray(), m_globalsCode.GetLength());

    if (m_methodsCode.GetLength() > 0)
        writeResult &= pStream->Write2(m_methodsCode.GetCharArray(), m_methodsCode.GetLength());

    writeResult &= pStream->Write2(footer, sizeof(footer) - 1);
    return writeResult;
}

void ShaderGraphCompilerHLSL::CleanString(String &str)
{
    uint32 i;

    for (i = 0; i < str.GetLength(); i++)
    {
        char ch = str[i];
        if ((ch >= '0' && ch <= '9') ||
            (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            ch == '_')
        {
            continue;
        }

        str[i] = '_';
    }
}

const char *ShaderGraphCompilerHLSL::GetTypeNameString(SHADER_PARAMETER_TYPE ValueType)
{
    switch (ValueType)
    {
    case SHADER_PARAMETER_TYPE_BOOL:
        return "bool";

    case SHADER_PARAMETER_TYPE_INT:
        return "int";

    case SHADER_PARAMETER_TYPE_INT2:
        return "int2";

    case SHADER_PARAMETER_TYPE_INT3:
        return "int3";

    case SHADER_PARAMETER_TYPE_INT4:
        return "int4";

    case SHADER_PARAMETER_TYPE_FLOAT:
        return "float";

    case SHADER_PARAMETER_TYPE_FLOAT2:
        return "float2";

    case SHADER_PARAMETER_TYPE_FLOAT3:
        return "float3";

    case SHADER_PARAMETER_TYPE_FLOAT4:
        return "float4";

    case SHADER_PARAMETER_TYPE_FLOAT2X2:
        return "float2x2";

    case SHADER_PARAMETER_TYPE_FLOAT3X3:
        return "float3x3";

    case SHADER_PARAMETER_TYPE_FLOAT3X4:
        return "float3x4";

    case SHADER_PARAMETER_TYPE_FLOAT4X4:
        return "float4x4";

    case SHADER_PARAMETER_TYPE_TEXTURE1D:
        return "Texture1D";

    case SHADER_PARAMETER_TYPE_TEXTURE2D:
        return "Texture2D";

    case SHADER_PARAMETER_TYPE_TEXTURE3D:
        return "Texture3D";

    case SHADER_PARAMETER_TYPE_TEXTURECUBE:
        return "TextureCube";

    case SHADER_PARAMETER_TYPE_TEXTURE1DARRAY:
        return "Texture1DArray";

    case SHADER_PARAMETER_TYPE_TEXTURE2DARRAY:
        return "Texture2DArray";

    case SHADER_PARAMETER_TYPE_TEXTURECUBEARRAY:
        return "TextureCubeArray";
    }

    UnreachableCode();
    return NULL;
}

bool ShaderGraphCompilerHLSL::AddCompileDefine(const char *Name, const char *Value)
{
    for (CompileDefineMap::Iterator itr = m_defines.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Key == Name)
            return (itr->Value == Value);
    }

    DefinePair definePair(Name, Value);
    m_defines.PushBack(definePair);
    return true;
}

bool ShaderGraphCompilerHLSL::AddBufferedCompileDefine(const char *Name, const char *Value)
{
    for (CompileDefineMap::Iterator itr = m_bufferedDefines.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Key == Name)
            return (itr->Value == Value);
    }

    DefinePair definePair(Name, Value);
    m_bufferedDefines.PushBack(definePair);
    return true;
}

bool ShaderGraphCompilerHLSL::EvaluateNode(const ShaderGraphNode *pSourceNode, uint32 OutputIndex, SHADER_GRAPH_VALUE_SWIZZLE Swizzle, SHADER_GRAPH_VALUE_SWIZZLE FixupSwizzle)
{
    DebugAssert(pSourceNode != NULL && OutputIndex < pSourceNode->GetOutputCount());

    if (!pSourceNode->EmitOutput(this, OutputIndex))
        return false;

    if (Swizzle != SHADER_GRAPH_VALUE_SWIZZLE_NONE)
        EmitSwizzle(Swizzle);

    if (FixupSwizzle != SHADER_GRAPH_VALUE_SWIZZLE_NONE)
        EmitSwizzle(FixupSwizzle);

    return true;
}

bool ShaderGraphCompilerHLSL::EvaluateNode(const ShaderGraphNodeInput *pNodeLink)
{
    if (pNodeLink->IsLinked())
        return EvaluateNode(pNodeLink->GetSourceNode(), pNodeLink->GetSourceOutputIndex(), pNodeLink->GetSwizzle(), pNodeLink->GetFixupSwizzle());

    // if it's not linked, use the default value
    if (!InternalCompileDefaultValue(pNodeLink->GetInputDesc()->Type, pNodeLink->GetInputDesc()->DefaultValue))
        return false;

    // handle swizzles
    if (pNodeLink->GetSwizzle() != SHADER_GRAPH_VALUE_SWIZZLE_NONE)
        EmitSwizzle(pNodeLink->GetSwizzle());
    if (pNodeLink->GetFixupSwizzle() != SHADER_GRAPH_VALUE_SWIZZLE_NONE)
        EmitSwizzle(pNodeLink->GetFixupSwizzle());

    return true;
}

#define EVALUATE_NODE(pNode) { if (!EvaluateNode(pNode)) { return false; } }
#define APPEND_STR(str) { m_buffer.AppendString(str); }

bool ShaderGraphCompilerHLSL::EmitAdd(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB)
{
    APPEND_STR("("); EVALUATE_NODE(pA); APPEND_STR(" + "); EVALUATE_NODE(pB); APPEND_STR(")");
    return true;
}

bool ShaderGraphCompilerHLSL::EmitSubtract(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB)
{
    APPEND_STR("("); EVALUATE_NODE(pA); APPEND_STR(" - "); EVALUATE_NODE(pB); APPEND_STR(")");
    return true;
}

bool ShaderGraphCompilerHLSL::EmitMultiply(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB)
{
    APPEND_STR("("); EVALUATE_NODE(pA); APPEND_STR(" * "); EVALUATE_NODE(pB); APPEND_STR(")");
    return true;
}

bool ShaderGraphCompilerHLSL::EmitDivide(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB)
{
    APPEND_STR("("); EVALUATE_NODE(pA); APPEND_STR(" / "); EVALUATE_NODE(pB); APPEND_STR(")");
    return true;
}

bool ShaderGraphCompilerHLSL::EmitDot(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB)
{
    APPEND_STR("dot("); EVALUATE_NODE(pA); APPEND_STR(", "); EVALUATE_NODE(pB); APPEND_STR(")");
    return true;
}

bool ShaderGraphCompilerHLSL::EmitNormalize(const ShaderGraphNodeInput *pNode)
{
    APPEND_STR("normalize("); EVALUATE_NODE(pNode); APPEND_STR(")");
    return true;
}

bool ShaderGraphCompilerHLSL::EmitNegate(const ShaderGraphNodeInput *pNode)
{
    APPEND_STR("-("); EVALUATE_NODE(pNode); APPEND_STR(")");
    return true;
}

bool ShaderGraphCompilerHLSL::EmitFractionalPart(const ShaderGraphNodeInput *pNode)
{
    APPEND_STR("frac("); EVALUATE_NODE(pNode); APPEND_STR(")");
    return true;
}

void ShaderGraphCompilerHLSL::EmitConstant(SHADER_PARAMETER_TYPE ValueType, const void *pValue)
{
    switch (ValueType)
    {
    case SHADER_PARAMETER_TYPE_BOOL:
        EmitConstantBool(*reinterpret_cast<const bool *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_INT:
        EmitConstantInt(*reinterpret_cast<const int32 *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_INT2:
        EmitConstantInt2(*reinterpret_cast<const int2 *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_INT3:
        EmitConstantInt3(*reinterpret_cast<const int3 *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_INT4:
        EmitConstantInt4(*reinterpret_cast<const int4 *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT:
        EmitConstantFloat(*reinterpret_cast<const float *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT2:
        EmitConstantFloat2(*reinterpret_cast<const float2 *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT3:
        EmitConstantFloat3(*reinterpret_cast<const float3 *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT4:
        EmitConstantFloat4(*reinterpret_cast<const float4 *>(pValue));
        break;

    default:
        UnreachableCode();
        break;
    }
}

void ShaderGraphCompilerHLSL::EmitConstantBool(bool Value)
{
    m_buffer.AppendString(Value ? "true" : "false");
}

void ShaderGraphCompilerHLSL::EmitConstantInt(int32 Value)
{
    m_buffer.AppendFormattedString("int(%d)", Value);
}

void ShaderGraphCompilerHLSL::EmitConstantInt2(const int2 &Value)
{
    m_buffer.AppendFormattedString("int2(%d, %d)", Value.x, Value.y);
}

void ShaderGraphCompilerHLSL::EmitConstantInt3(const int3 &Value)
{
    m_buffer.AppendFormattedString("int3(%d, %d, %d)", Value.x, Value.y, Value.z);
}

void ShaderGraphCompilerHLSL::EmitConstantInt4(const int4 &Value)
{
    m_buffer.AppendFormattedString("int4(%d, %d, %d, %d)", Value.x, Value.y, Value.z, Value.w);
}

void ShaderGraphCompilerHLSL::EmitConstantFloat(const float &Value)
{
    m_buffer.AppendFormattedString("float(%.6f)", Value);
}

void ShaderGraphCompilerHLSL::EmitConstantFloat2(const float2 &Value)
{
    m_buffer.AppendFormattedString("float2(%.6f, %.6f)", Value.x, Value.y);
}

void ShaderGraphCompilerHLSL::EmitConstantFloat3(const float3 &Value)
{
    m_buffer.AppendFormattedString("float3(%.6f, %.6f, %.6f)", Value.x, Value.y, Value.z);
}

void ShaderGraphCompilerHLSL::EmitConstantFloat4(const float4 &Value)
{
    m_buffer.AppendFormattedString("float4(%.6f, %.6f, %.6f, %.6f)", Value.x, Value.y, Value.z, Value.w);
}

bool ShaderGraphCompilerHLSL::EmitAccessShaderGlobal(const char *GlobalName)
{
    const ShaderGraphSchema *pSchema = m_pShaderGraph->GetSchema();

    uint32 i;
    for (i = 0; i < pSchema->GetGlobalDeclarationCount(); i++)
    {
        if (pSchema->GetGlobalDeclaration(i)->Name.Compare(GlobalName))
            break;
    }
    if (i == pSchema->GetInputDeclarationCount())
    {
        Log_ErrorPrintf("Attempting to access unknown shader global '%s'.", GlobalName);
        return false;
    }

    // get input decl, write the variable name
    const ShaderGraphSchema::GlobalDeclaration *pGlobalDeclaration = pSchema->GetGlobalDeclaration(i);
    m_buffer.AppendString(pGlobalDeclaration->VariableName.GetCharArray());
    return true;
}

bool ShaderGraphCompilerHLSL::EmitAccessShaderInput(const char *InputName)
{
    const ShaderGraphSchema *pSchema = m_pShaderGraph->GetSchema();

    uint32 i;
    for (i = 0; i < pSchema->GetInputDeclarationCount(); i++)
    {
        if (pSchema->GetInputDeclaration(i)->Name.Compare(InputName))
            break;
    }
    if (i == pSchema->GetInputDeclarationCount())
    {
        Log_ErrorPrintf("Attempting to access unknown shader input '%s'.", InputName);
        return false;
    }

    // get input decl
    const ShaderGraphSchema::InputDeclaration *pInputDeclaration = pSchema->GetInputDeclaration(i);
    DebugAssert(m_currentStage < SHADER_PROGRAM_STAGE_COUNT);

    // check the input is accessible from this stage
    if (pInputDeclaration->Access[m_currentStage].VariableName.IsEmpty())
    {
        Log_ErrorPrintf("attempting to access input '%s' that is not accessible from this stage (%s)", pInputDeclaration->Name.GetCharArray(), NameTable_GetNameString(NameTables::ShaderProgramStage, m_currentStage));
        return false;
    }

    // add defines
    if (pInputDeclaration->Access[m_currentStage].DefineName.GetLength() > 0)
        AddBufferedCompileDefine(pInputDeclaration->Access[m_currentStage].DefineName, "1");

    // write the string out
    m_buffer.AppendString(pInputDeclaration->Access[m_currentStage].VariableName.GetCharArray());
    return true;
}

bool ShaderGraphCompilerHLSL::EmitAccessExternalParameter(const ExternalParameter *pExternalParameter)
{
    m_buffer.AppendString(pExternalParameter->BindingName);
    return true;
}

bool ShaderGraphCompilerHLSL::EmitTextureSample(const ShaderGraphNodeInput *pTextureNodeLink, const ShaderGraphNodeInput *pTexCoordNodeLink, SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION unpackOperation)
{
    switch (unpackOperation)
    {
    case SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION_NORMAL_MAP:
        APPEND_STR("((");
        break;
    }

    EVALUATE_NODE(pTextureNodeLink);

    APPEND_STR(".Sample(");

    {
        EVALUATE_NODE(pTextureNodeLink);
        APPEND_STR("_SamplerState");
        APPEND_STR(", ");

        EVALUATE_NODE(pTexCoordNodeLink);
    }    

    APPEND_STR(")");

    switch (unpackOperation)
    {
    case SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION_NORMAL_MAP:
        APPEND_STR(" - 0.5) * 2.0)");
        break;
    }

    return true;
}

bool ShaderGraphCompilerHLSL::EmitConstructPrimitive(SHADER_PARAMETER_TYPE primitiveType, const ShaderGraphNodeInput **ppInputNodes, uint32 nInputNodes)
{
    switch (primitiveType)
    {
    case SHADER_PARAMETER_TYPE_FLOAT2:
        {
            if (nInputNodes != 2)
                return false;

            APPEND_STR("float2(");
            EVALUATE_NODE(ppInputNodes[0]);
            APPEND_STR(", ");
            EVALUATE_NODE(ppInputNodes[1]);
            APPEND_STR(")");
            return true;
        }

    case SHADER_PARAMETER_TYPE_FLOAT3:
        {
            if (nInputNodes != 3)
                return false;

            APPEND_STR("float3(");
            EVALUATE_NODE(ppInputNodes[0]);
            APPEND_STR(", ");
            EVALUATE_NODE(ppInputNodes[1]);
            APPEND_STR(", ");
            EVALUATE_NODE(ppInputNodes[2]);
            APPEND_STR(")");
            return true;
        }

    case SHADER_PARAMETER_TYPE_FLOAT4:
        {
            if (nInputNodes != 4)
                return false;

            APPEND_STR("float4(");
            EVALUATE_NODE(ppInputNodes[0]);
            APPEND_STR(", ");
            EVALUATE_NODE(ppInputNodes[1]);
            APPEND_STR(", ");
            EVALUATE_NODE(ppInputNodes[2]);
            APPEND_STR(", ");
            EVALUATE_NODE(ppInputNodes[3]);
            APPEND_STR(")");
            return true;
        }
    }

    return false;
}

void ShaderGraphCompilerHLSL::EmitSwizzle(SHADER_GRAPH_VALUE_SWIZZLE Swizzle)
{
    if (Swizzle == SHADER_GRAPH_VALUE_SWIZZLE_NONE)
        return;

    char swizzleStr[5];
    if (!ShaderGraph::GetStringForValueSwizzle(swizzleStr, Swizzle))
        Panic("Invalid swizzle value encountered");

    m_buffer.AppendCharacter('.');
    m_buffer.AppendString(swizzleStr);
}

bool ShaderGraphCompilerHLSL::InternalCompile()
{
    uint32 i;

    // write external parameters
    for (ExternalParameterList::ConstIterator itr = m_ExternalUniforms.Begin(); !itr.AtEnd(); itr.Forward())
    {
        // get type name
        const char *uniformTypeName = GetTypeNameString(itr->Type);
        m_externsCode.AppendFormattedString("%s %s;\n", uniformTypeName, itr->BindingName.GetCharArray());
    }
    for (ExternalParameterList::ConstIterator itr = m_ExternalTextures.Begin(); !itr.AtEnd(); itr.Forward())
    {
        // get type name
        const char *textureTypeName = GetTypeNameString(itr->Type);
        m_externsCode.AppendFormattedString("%s %s;\n", textureTypeName, itr->BindingName.GetCharArray());
        m_externsCode.AppendFormattedString("SamplerState %s_SamplerState;\n", itr->BindingName.GetCharArray());
    }
        
    // compile outputs dependant on output profile
    const ShaderGraphSchema *pSchema = m_pShaderGraph->GetSchema();
    for (i = 0; i < pSchema->GetOutputDeclarationCount(); i++)
    {
        if (!InternalCompileOutput(i))
            return false;
    }

    return true;
}

bool ShaderGraphCompilerHLSL::InternalCompileDefaultValue(SHADER_PARAMETER_TYPE Type, const char *DefaultValue)
{
    if (Y_strnicmp(DefaultValue, "input:", 6) == 0)
    {
        // find the input node.. this cast should not fail
        const ShaderGraphNode_ShaderInputs *pInputsNode = m_pShaderGraph->GetShaderInputsNode()->Cast<ShaderGraphNode_ShaderInputs>();
        if (pInputsNode == NULL)
            return false;

        // find the output in the list
        uint32 outputIndex;
        for (outputIndex = 0; outputIndex < pInputsNode->GetOutputCount(); outputIndex++)
        {
            if (Y_strcmp(DefaultValue + 6, pInputsNode->GetOutput(outputIndex)->GetOutputDesc()->Name) == 0)
                break;
        }
        if (outputIndex == pInputsNode->GetOutputCount())
        {
            Log_ErrorPrintf("could not compile default value '%s': output not found.", DefaultValue);
            return false;
        }

        // check type
        if (Type != pInputsNode->GetOutput(outputIndex)->GetValueType())
        {
            Log_ErrorPrintf("could not compile default value '%s': value types mismatch (%s, %s).", DefaultValue, 
                NameTable_GetNameString(NameTables::ShaderParameterType, Type),
                NameTable_GetNameString(NameTables::ShaderParameterType, pInputsNode->GetOutput(outputIndex)->GetValueType()));

            return false;
        }

        // evaluate the output
        return EvaluateNode(pInputsNode, outputIndex, SHADER_GRAPH_VALUE_SWIZZLE_NONE, SHADER_GRAPH_VALUE_SWIZZLE_NONE);
    }
    else if (Y_strnicmp(DefaultValue, "link:", 5) == 0)
    {
        const ShaderGraphNode *pLinkNode = m_pShaderGraph->GetNodeByName(DefaultValue + 5);
        if (pLinkNode == NULL || pLinkNode->GetOutputCount() < 1)
            return false;

        // evaluate the default node
        return EvaluateNode(pLinkNode, 0, SHADER_GRAPH_VALUE_SWIZZLE_NONE, SHADER_GRAPH_VALUE_SWIZZLE_NONE);
    }
    else
    {
        // parse the default node, it should be a primitive type.
        switch (Type)
        {
        case SHADER_PARAMETER_TYPE_BOOL:
            EmitConstantBool(StringConverter::StringToBool(DefaultValue));
            return true;

        case SHADER_PARAMETER_TYPE_INT:
            EmitConstantInt(StringConverter::StringToInt32(DefaultValue));
            return true;

        case SHADER_PARAMETER_TYPE_INT2:
            EmitConstantInt2(StringConverter::StringToInt2(DefaultValue));
            return true;

        case SHADER_PARAMETER_TYPE_INT3:
            EmitConstantInt3(StringConverter::StringToInt3(DefaultValue));
            return true;

        case SHADER_PARAMETER_TYPE_INT4:
            EmitConstantInt4(StringConverter::StringToInt4(DefaultValue));
            return true;

        case SHADER_PARAMETER_TYPE_FLOAT:
            EmitConstantFloat(StringConverter::StringToFloat(DefaultValue));
            return true;

        case SHADER_PARAMETER_TYPE_FLOAT2:
            EmitConstantFloat2(StringConverter::StringToFloat2(DefaultValue));
            return true;

        case SHADER_PARAMETER_TYPE_FLOAT3:
            EmitConstantFloat3(StringConverter::StringToFloat3(DefaultValue));
            return true;

        case SHADER_PARAMETER_TYPE_FLOAT4:
            EmitConstantFloat4(StringConverter::StringToFloat4(DefaultValue));
            return true;
        }

        return false;
    }
}

bool ShaderGraphCompilerHLSL::InternalCompileOutput(uint32 outputIndex)
{
    // find and cast the outputs node
    const ShaderGraphNode_ShaderOutputs *pShaderOutputsNode = m_pShaderGraph->GetShaderOutputsNode()->SafeCast<ShaderGraphNode_ShaderOutputs>();
    if (pShaderOutputsNode == nullptr)
        return false;

    // clear everything out
    m_currentScope.Clear();
    m_buffer.Clear();
    m_bufferedDefines.Clear();

    // sanity check: this should always match. otherwise something really bad has happened at some point
    const ShaderGraphSchema *pSchema = m_pShaderGraph->GetSchema();
    const ShaderGraphSchema::OutputDeclaration *pOutputDeclaration = pSchema->GetOutputDeclaration(outputIndex);
    const ShaderGraphNodeInput *pInput = pShaderOutputsNode->GetInput(outputIndex);
    DebugAssert(pInput->GetInputDesc() == &pOutputDeclaration->InputDesc);   

    // update current stage
    //static const char *stageDefines[SHADER_PROGRAM_STAGE_COUNT] = { "VERTEX_SHADER", "HULL_SHADER", "DOMAIN_SHADER", "GEOMETRY_SHADER", "PIXEL_SHADER", "COMPUTE_SHADER" };
    m_currentStage = pOutputDeclaration->Stage;

    // check if it is bound
    if (!pInput->IsLinked())
    {
        // use default value
        if (!InternalCompileDefaultValue(pOutputDeclaration->Type, pOutputDeclaration->DefaultValue))
            return false;
    }
    else
    {
        // compile the value
        if (!EvaluateNode(pInput))
            return false;
    }

    // check for define
    if (pOutputDeclaration->DefineName.GetLength() > 0)
        m_methodsCode.AppendFormattedString("#if %s\n\n", pOutputDeclaration->DefineName.GetCharArray());

    // add any buffered defines
    for (CompileDefineMap::ConstIterator itr = m_bufferedDefines.Begin(); !itr.AtEnd(); itr.Forward())
    {
        const DefinePair &definePair = *itr;
        m_methodsCode.AppendFormattedString("#if !%s\n", definePair.Key.GetCharArray());
        m_methodsCode.AppendFormattedString("    #define %s %s\n", definePair.Key.GetCharArray(), definePair.Value.GetCharArray());
        m_methodsCode.AppendFormattedString("#endif\n");
    }

    // get input type string
    const char *inputTypeString = GetTypeNameString(pInput->GetValueType());

    // add code
    //m_methodsCode.AppendFormattedString("#if %s\n\n", stageDefines[m_currentStage]);
    m_methodsCode.AppendFormattedString("%s %s\n", inputTypeString, pOutputDeclaration->FunctionSignature.GetCharArray());
    m_methodsCode.AppendString("{\n");
    m_methodsCode.AppendString(m_currentScope);
    m_methodsCode.AppendString("    return "); m_methodsCode.AppendString(m_buffer); m_methodsCode.AppendString(";\n");
    m_methodsCode.AppendString("}\n\n");
    //m_methodsCode.AppendFormattedString("#endif    // %s\n\n", stageDefines[m_currentStage]);

    // end define
    if (pOutputDeclaration->DefineName.GetLength() > 0)
        m_methodsCode.AppendFormattedString("#endif    // %s\n\n", pOutputDeclaration->DefineName.GetCharArray());

    // done
    return true;
}

