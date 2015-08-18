#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/ShaderGraphNode.h"
#include "ResourceCompiler/ShaderGraph.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
//Log_SetChannel(ShaderGraphNode);

DEFINE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode, Object, "ShaderGraphNode", "Description");
BEGIN_SHADER_GRAPH_NODE_PROPERTIES(ShaderGraphNode)
END_SHADER_GRAPH_NODE_PROPERTIES()
DEFINE_SHADER_GRAPH_NODE_INPUTS(ShaderGraphNode)
END_SHADER_GRAPH_NODE_INPUTS()
DEFINE_SHADER_GRAPH_NODE_OUTPUTS(ShaderGraphNode)
END_SHADER_GRAPH_NODE_OUTPUTS()

Y_Define_NameTable(NameTables::ShaderGraphTextureSampleUnpackOperation)
    Y_NameTable_Entry("None",               SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION_NONE)
    Y_NameTable_Entry("NormalMap",          SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION_NORMAL_MAP)
Y_NameTable_End()

ShaderGraphNodeInput::ShaderGraphNodeInput()
{
    m_pNode = NULL;
    m_iInputIndex = 0;
    m_pInputDesc = NULL;
    m_pSourceNode = NULL;
    m_iSourceOutputIndex = 0;
    m_eSwizzle = SHADER_GRAPH_VALUE_SWIZZLE_NONE;
    m_eFixupSwizzle = SHADER_GRAPH_VALUE_SWIZZLE_NONE;
    m_eValueType = SHADER_PARAMETER_TYPE_COUNT;
}

ShaderGraphNodeInput::~ShaderGraphNodeInput()
{
    //if (m_pSourceNode != NULL)
        //m_pSourceNode->Release();
}

void ShaderGraphNodeInput::Init(ShaderGraphNode *pNode, uint32 InputIndex, const SHADER_GRAPH_NODE_INPUT *pInputDesc)
{
    m_pNode = pNode;
    m_iInputIndex = InputIndex;
    m_pInputDesc = pInputDesc;
    m_eValueType = pInputDesc->Type;
}

void ShaderGraphNodeInput::SetExpectedType(SHADER_PARAMETER_TYPE ExpectedType)
{
    DebugAssert(m_pSourceNode == NULL);
    m_eValueType = ExpectedType;
}

bool ShaderGraphNodeInput::SetLink(const ShaderGraphNode *pSourceNode, uint32 OutputIndex, SHADER_GRAPH_VALUE_SWIZZLE Swizzle)
{
    const ShaderGraphNodeOutput *pOutput = pSourceNode->GetOutput(OutputIndex);
    DebugAssert(pOutput != NULL);

    SHADER_PARAMETER_TYPE newValueType = GetTypeAfterSwizzle(pOutput->GetValueType(), Swizzle);
    if (newValueType == SHADER_PARAMETER_TYPE_COUNT)
        return false;

    // Check the types are compatible.
    SHADER_GRAPH_VALUE_SWIZZLE fixupSwizzle = SHADER_GRAPH_VALUE_SWIZZLE_NONE;
    if (m_eValueType != SHADER_PARAMETER_TYPE_COUNT && m_eValueType != newValueType)
    {
        // Can we truncate it with a swizzle to be the correct type?
        if (!CanAutoTruncateType(&fixupSwizzle, m_eValueType, newValueType))
            return false;

        // set fixup swizzle
        newValueType = GetTypeAfterSwizzle(m_eValueType, m_eFixupSwizzle);
        DebugAssert(newValueType == m_eValueType);
    }

    // unlink current node if present
    if (m_pSourceNode != NULL)
        ClearLink();

    // link new node
    DebugAssert(pSourceNode != NULL);
    m_pSourceNode = pSourceNode;
    m_iSourceOutputIndex = OutputIndex;
    m_eSwizzle = Swizzle;
    m_eFixupSwizzle = fixupSwizzle;
    m_eValueType = newValueType;
    //m_pSourceNode->AddRef();

    // fire input event
    if (!m_pNode->OnInputConnectionChange(m_iInputIndex))
    {
        //m_pSourceNode->Release();
        m_pSourceNode = NULL;
        m_iSourceOutputIndex = 0;
        m_eSwizzle = SHADER_GRAPH_VALUE_SWIZZLE_NONE;
        m_eFixupSwizzle = SHADER_GRAPH_VALUE_SWIZZLE_NONE;
        m_eValueType = m_pInputDesc->Type;
        return false;
    }

    // add output reference
    m_pSourceNode->GetOutput(OutputIndex)->AddLinkReference();

    // link added
    //Log_DevPrintf("ShaderGraph: Link added '%s':%u (%s) -> '%s':%u (%s)", m_pSourceNode->GetName().GetCharArray(), OutputIndex, m_pSourceNode->GetTypeInfo()->GetName(),
                                                                          //m_pNode->GetName().GetCharArray(), m_iInputIndex, m_pNode->GetTypeInfo()->GetName());

    return true;
}

void ShaderGraphNodeInput::ClearLink()
{
    if (m_pSourceNode != NULL)
    {
        m_pSourceNode->GetOutput(m_iSourceOutputIndex)->RemoveLinkReference();
        //m_pSourceNode->Release();

        m_pSourceNode = NULL;
        m_iSourceOutputIndex = 0;
        m_eSwizzle = SHADER_GRAPH_VALUE_SWIZZLE_NONE;
        m_eFixupSwizzle = SHADER_GRAPH_VALUE_SWIZZLE_NONE;
    }
}

bool ShaderGraphNodeInput::CanSwizzleType(SHADER_PARAMETER_TYPE ValueType, SHADER_GRAPH_VALUE_SWIZZLE Swizzle)
{
    if (Swizzle == SHADER_GRAPH_VALUE_SWIZZLE_NONE)
        return true;

    return (GetTypeAfterSwizzle(ValueType, Swizzle) != SHADER_PARAMETER_TYPE_COUNT);
}

SHADER_PARAMETER_TYPE ShaderGraphNodeInput::GetTypeAfterSwizzle(SHADER_PARAMETER_TYPE ValueType, SHADER_GRAPH_VALUE_SWIZZLE Swizzle)
{
    if (Swizzle == SHADER_GRAPH_VALUE_SWIZZLE_NONE)
        return ValueType;
    
    static const SHADER_PARAMETER_TYPE swizzleTypesInt[4] = { SHADER_PARAMETER_TYPE_INT, SHADER_PARAMETER_TYPE_INT2, SHADER_PARAMETER_TYPE_INT3, SHADER_PARAMETER_TYPE_INT4 };
    //static const SHADER_PARAMETER_TYPE swizzleTypesUInt[4] = { SHADER_PARAMETER_TYPE_INT, SHADER_PARAMETER_TYPE_INT2, SHADER_PARAMETER_TYPE_INT3, SHADER_PARAMETER_TYPE_INT4 };
    static const SHADER_PARAMETER_TYPE swizzleTypesFloat[4] = { SHADER_PARAMETER_TYPE_FLOAT, SHADER_PARAMETER_TYPE_FLOAT2, SHADER_PARAMETER_TYPE_FLOAT3, SHADER_PARAMETER_TYPE_FLOAT4 };

    uint32 valueComponents;
    uint32 valueComponentType;
    switch (ValueType)
    {
    case SHADER_PARAMETER_TYPE_INT:
        valueComponents = 1;
        valueComponentType = SHADER_PARAMETER_TYPE_INT;
        break;

    case SHADER_PARAMETER_TYPE_INT2:
        valueComponents = 2;
        valueComponentType = SHADER_PARAMETER_TYPE_INT;
        break;

    case SHADER_PARAMETER_TYPE_INT3:
        valueComponents = 3;
        valueComponentType = SHADER_PARAMETER_TYPE_INT;
        break;

    case SHADER_PARAMETER_TYPE_INT4:
        valueComponents = 4;
        valueComponentType = SHADER_PARAMETER_TYPE_INT;
        break;

    case SHADER_PARAMETER_TYPE_FLOAT:
        valueComponents = 1;
        valueComponentType = SHADER_PARAMETER_TYPE_FLOAT;
        break;

    case SHADER_PARAMETER_TYPE_FLOAT2:
        valueComponents = 2;
        valueComponentType = SHADER_PARAMETER_TYPE_FLOAT;
        break;

    case SHADER_PARAMETER_TYPE_FLOAT3:
        valueComponents = 3;
        valueComponentType = SHADER_PARAMETER_TYPE_FLOAT;
        break;

    case SHADER_PARAMETER_TYPE_FLOAT4:
        valueComponents = 4;
        valueComponentType = SHADER_PARAMETER_TYPE_FLOAT;
        break;

    default:
        return SHADER_PARAMETER_TYPE_COUNT;
    }

    // get components in swizzle
    uint32 swizzleComponents = ShaderGraph::GetValueSwizzleComponentCount(Swizzle);
    if (swizzleComponents == 0)
    {
        // no swizzle, or an internal issue
        return ValueType;
    }

    // make sure all the components are in range
    for (uint32 i = 0; i < swizzleComponents; i++)
    {
        if (ShaderGraph::GetValueSwizzleComponent(Swizzle, i) >= valueComponents)
            return SHADER_PARAMETER_TYPE_COUNT;
    }

    // return the corresponding type
    switch (valueComponentType)
    {
    case SHADER_PARAMETER_TYPE_INT:
        return swizzleTypesInt[swizzleComponents - 1];

    case SHADER_PARAMETER_TYPE_FLOAT:
        return swizzleTypesFloat[swizzleComponents - 1];
    }

    // should not be reached
    return SHADER_PARAMETER_TYPE_COUNT;
}

bool ShaderGraphNodeInput::CanAutoTruncateType(SHADER_GRAPH_VALUE_SWIZZLE *pSwizzle, SHADER_PARAMETER_TYPE expectedType, SHADER_PARAMETER_TYPE valueType)
{
    // we can go from a longer vector to a shorter vector, nothing else.
    switch (expectedType)
    {
    case SHADER_PARAMETER_TYPE_FLOAT:
        {
            switch (valueType)
            {
            case SHADER_PARAMETER_TYPE_FLOAT:
            case SHADER_PARAMETER_TYPE_FLOAT2:
            case SHADER_PARAMETER_TYPE_FLOAT3:
            case SHADER_PARAMETER_TYPE_FLOAT4:
                *pSwizzle = SHADER_GRAPH_VALUE_SWIZZLE_R;
                return true;
            }
        }
        break;

    case SHADER_PARAMETER_TYPE_FLOAT2:
        {
            switch (valueType)
            {
            case SHADER_PARAMETER_TYPE_FLOAT2:
            case SHADER_PARAMETER_TYPE_FLOAT3:
            case SHADER_PARAMETER_TYPE_FLOAT4:
                *pSwizzle = SHADER_GRAPH_VALUE_SWIZZLE_RG;
                return true;
            }
        }
        break;

    case SHADER_PARAMETER_TYPE_FLOAT3:
        {
            switch (valueType)
            {
            case SHADER_PARAMETER_TYPE_FLOAT3:
            case SHADER_PARAMETER_TYPE_FLOAT4:
                *pSwizzle = SHADER_GRAPH_VALUE_SWIZZLE_RGB;
                return true;
            }
        }
        break;

    case SHADER_PARAMETER_TYPE_FLOAT4:
        {
            switch (valueType)
            {
            case SHADER_PARAMETER_TYPE_FLOAT2:
            case SHADER_PARAMETER_TYPE_FLOAT3:
            case SHADER_PARAMETER_TYPE_FLOAT4:
                *pSwizzle = SHADER_GRAPH_VALUE_SWIZZLE_RGBA;
                return true;
            }
        }
        break;
    }

    return false;
}

bool ShaderGraphNodeInput::SetFixupSwizzle(SHADER_PARAMETER_TYPE expectedOutputType)
{
    DebugAssert(m_pSourceNode != NULL && m_pSourceNode->GetOutput(m_iSourceOutputIndex) != NULL);

    // get the original type, without any fixup swizzle
    SHADER_PARAMETER_TYPE originalType = m_pSourceNode->GetOutput(m_iSourceOutputIndex)->GetValueType();
    SHADER_PARAMETER_TYPE valueType = GetTypeAfterSwizzle(originalType, m_eSwizzle);

    // ok?
    if (valueType != expectedOutputType)
    {
        SHADER_GRAPH_VALUE_SWIZZLE truncateSwizzle;
        if (CanAutoTruncateType(&truncateSwizzle, expectedOutputType, valueType))
        {
            m_eFixupSwizzle = truncateSwizzle;
            m_eValueType = GetTypeAfterSwizzle(valueType, m_eFixupSwizzle);
            DebugAssert(m_eValueType == expectedOutputType);
            return true;
        }
    }

    return false;
}

void ShaderGraphNodeInput::ClearFixupSwizzle()
{
    DebugAssert(m_pSourceNode != NULL && m_pSourceNode->GetOutput(m_iSourceOutputIndex) != NULL);

    // get the original type, without any fixup swizzle
    SHADER_PARAMETER_TYPE originalType = m_pSourceNode->GetOutput(m_iSourceOutputIndex)->GetValueType();
    SHADER_PARAMETER_TYPE valueType = GetTypeAfterSwizzle(originalType, m_eSwizzle);

    // set back
    m_eFixupSwizzle = SHADER_GRAPH_VALUE_SWIZZLE_NONE;
    m_eValueType = valueType;
}

ShaderGraphNodeOutput::ShaderGraphNodeOutput()
{
    m_pNode = NULL;
    m_iOutputIndex = 0;
    m_pOutputDesc = NULL;
    m_eValueType = SHADER_PARAMETER_TYPE_COUNT;
    m_iLinkCount = 0;
}

ShaderGraphNodeOutput::~ShaderGraphNodeOutput()
{
    DebugAssert(m_iLinkCount == 0);
}

void ShaderGraphNodeOutput::Init(ShaderGraphNode *pNode, uint32 OutputIndex, const SHADER_GRAPH_NODE_OUTPUT *pOutputDesc)
{
    m_pNode = pNode;
    m_iOutputIndex = OutputIndex;
    m_pOutputDesc = pOutputDesc;
    m_eValueType = pOutputDesc->Type;
}

void ShaderGraphNodeOutput::SetValueType(SHADER_PARAMETER_TYPE NewValueType)
{
    m_eValueType = NewValueType;
}

ShaderGraphNode::ShaderGraphNode(const ShaderGraphNodeTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo),
      m_pTypeInfo(pTypeInfo),
      m_iID(0xFFFFFFFF),
      m_pInputs(NULL),
      m_nInputs(pTypeInfo->GetInputCount()),
      m_pOutputs(NULL),
      m_nOutputs(pTypeInfo->GetOutputCount())
{
    uint32 i;

    // allocate linked nodes
    if (m_nInputs > 0)
    {
        m_pInputs = new ShaderGraphNodeInput[m_nInputs];
        for (i = 0; i < m_nInputs; i++)
            m_pInputs[i].Init(this, i, pTypeInfo->GetInput(i));
    }
    if (m_nOutputs > 0)
    {
        m_pOutputs = new ShaderGraphNodeOutput[m_nOutputs];
        for (i = 0; i < m_nOutputs; i++)
            m_pOutputs[i].Init(this, i, pTypeInfo->GetOutput(i));
    }

    // Graph editor position.
    m_GraphEditorPosition.SetZero();
}

ShaderGraphNode::~ShaderGraphNode()
{
    if (m_pInputs != NULL)
        delete[] m_pInputs;
    if (m_pOutputs != NULL)
        delete[] m_pOutputs;
}

bool ShaderGraphNode::OnInputConnectionChange(uint32 InputIndex)
{
    return true;
}

bool ShaderGraphNode::LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader)
{
    const char *nodeName = xmlReader.FetchAttribute("name");
    if (nodeName == NULL)
    {
        xmlReader.PrintError("Could not find name attribute.");
        return false;
    }

    const char *position = xmlReader.FetchAttribute("position");
    if (position != NULL)
        m_GraphEditorPosition = StringConverter::StringToInt2(position);

    m_strName = nodeName;
    return true;
}

void ShaderGraphNode::SaveToXML(XMLWriter &xmlWriter) const
{
    xmlWriter.WriteAttribute("name", m_strName);

    SmallString tempStr;
    StringConverter::Int2ToString(tempStr, m_GraphEditorPosition);
    xmlWriter.WriteAttribute("position", tempStr);
}

bool ShaderGraphNode::EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const
{
    return true;
}

const ShaderGraphNodeTypeInfo *ShaderGraphNode::FindTypeInfoForShortName(const char *shortName)
{
    const ObjectTypeInfo::RegistryType &typeRegistry = ObjectTypeInfo::GetRegistry();
    for (uint32 typeIndex = 0; typeIndex < typeRegistry.GetNumTypes(); typeIndex++)
    {
        const ObjectTypeInfo *pTypeInfo = typeRegistry.GetTypeInfoByIndex(typeIndex);
        if (pTypeInfo != nullptr && pTypeInfo->IsDerived(OBJECT_TYPEINFO(ShaderGraphNode)))
        {
            const ShaderGraphNodeTypeInfo *pShaderGraphNodeTypeInfo = static_cast<const ShaderGraphNodeTypeInfo *>(pTypeInfo);
            if (Y_stricmp(pShaderGraphNodeTypeInfo->GetShortName(), shortName) == 0)
                return pShaderGraphNodeTypeInfo;
        }
    }

    return nullptr;
}


