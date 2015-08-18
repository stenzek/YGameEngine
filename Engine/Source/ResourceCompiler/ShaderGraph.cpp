#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/ShaderGraph.h"
#include "ResourceCompiler/ShaderGraphBuiltinNodes.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
Log_SetChannel(ShaderGraph);

static bool typesRegistered = false;

ShaderGraph::ShaderGraph()
    : m_pSchema(NULL),
      m_iNextNodeId(1),
      m_pShaderInputsNode(NULL),
      m_pShaderOutputsNode(NULL)
{
    // ensure types are registered
    if (!typesRegistered)
    {
        RegisterBuiltinTypes();
        typesRegistered = true;
    }
}

ShaderGraph::~ShaderGraph()
{
    int32 i;

    // clear the links in reverse order
    for (i = (int32)m_Links.GetSize() - 1; i >= 0; i--)
        RemoveLink(m_Links[i]);

    m_Links.Obliterate();

    // clear the nodes in reverse order
    for (i = (int32)m_Nodes.GetSize() - 1; i >= 0; i--)
        delete m_Nodes[i];

    m_Nodes.Obliterate();
    
    SAFE_RELEASE(m_pSchema);
}

const ShaderGraphNode *ShaderGraph::GetNodeByID(uint32 NodeID) const
{
    uint32 i;

    for (i = 0; i < m_Nodes.GetSize(); i++)
    {
        const ShaderGraphNode *pNode = m_Nodes[i];
        if (pNode->GetID() == NodeID)
            return pNode;
    }

    return NULL;
}

const ShaderGraphNode *ShaderGraph::GetNodeByName(const char *Name) const
{
    uint32 i;

    for (i = 0; i < m_Nodes.GetSize(); i++)
    {
        const ShaderGraphNode *pNode = m_Nodes[i];
        if (pNode->GetName().CompareInsensitive(Name))
            return pNode;
    }

    return NULL;
}

ShaderGraphNode *ShaderGraph::GetNodeByID(uint32 NodeID)
{
    uint32 i;

    for (i = 0; i < m_Nodes.GetSize(); i++)
    {
        ShaderGraphNode *pNode = m_Nodes[i];
        if (pNode->GetID() == NodeID)
            return pNode;
    }

    return NULL;
}

ShaderGraphNode *ShaderGraph::GetNodeByName(const char *Name)
{
    uint32 i;

    for (i = 0; i < m_Nodes.GetSize(); i++)
    {
        ShaderGraphNode *pNode = m_Nodes[i];
        if (pNode->GetName().CompareInsensitive(Name))
            return pNode;
    }

    return NULL;
}

bool ShaderGraph::AddNode(ShaderGraphNode *pNode)
{
    uint32 i;
    for (i = 0; i < m_Nodes.GetSize(); i++)
    {
        ShaderGraphNode *pCurNode = m_Nodes[i];
        if (pCurNode->GetName().CompareInsensitive(pNode->GetName()))
            return false;
    }

    pNode->SetID(m_iNextNodeId++);
    m_Nodes.Add(pNode);

    //Log_DevPrintf("ShaderGraph: Add node '%s' (%s)", pNode->GetName().GetCharArray(), pNode->GetTypeInfo()->GetName());
    return true;
}

bool ShaderGraph::RemoveNode(ShaderGraphNode *pNode)
{
    uint32 i;

    for (i = 0; i < pNode->GetOutputCount(); i++)
    {
        if (pNode->GetOutput(i)->GetLinkCount() > 0)
            return false;
    }

    for (i = 0; i < m_Nodes.GetSize(); i++)
    {
        if (m_Nodes[i] == pNode)
        {
            pNode->SetID(0xFFFFFFFF);
            m_Nodes.OrderedRemove(i);
            delete pNode;
            return true;
        }
    }
    
    Panic("Attempting to remove node from shader graph that is not tracked.");
    return false;
}

bool ShaderGraph::AddLink(ShaderGraphNode *pTargetNode, uint32 InputIndex, ShaderGraphNode *pSourceNode, uint32 OutputIndex, SHADER_GRAPH_VALUE_SWIZZLE Swizzle)
{
    uint32 i;
    if (OutputIndex >= pSourceNode->GetOutputCount() || InputIndex >= pTargetNode->GetInputCount())
        return false;

    ShaderGraphNodeInput *pInput = pTargetNode->GetInput(InputIndex);
    if (pInput->IsLinked())
        return false;

    if (!pInput->SetLink(pSourceNode, OutputIndex, Swizzle))
        return false;

    for (i = 0; i < m_Links.GetSize(); i++)
    {
        if (m_Links[i] == pInput)
            Panic("Double inserting input node.");
    }

    m_Links.Add(pInput);
    return true;    
}

bool ShaderGraph::RemoveLink(ShaderGraphNode *pTargetNode, uint32 InputIndex)
{
    if (InputIndex >= pTargetNode->GetInputCount())
        return false;

    return RemoveLink(pTargetNode->GetInput(InputIndex));
}

bool ShaderGraph::RemoveLink(ShaderGraphNodeInput *pInput)
{
    uint32 i;

    pInput->ClearLink();
    for (i = 0; i < m_Links.GetSize(); i++)
    {
        if (m_Links[i] == pInput)
        {
            m_Links.OrderedRemove(i);
            return true;
        }
    }

    Panic("Attempting to remove a non-tracked link.");
    return false;
}

bool ShaderGraph::Create(const ShaderGraphSchema *pSchema)
{
    // set schema
    m_pSchema = pSchema;
    m_pSchema->AddRef();

    // create the inputs node
    ShaderGraphNode_ShaderInputs *pInputsNode = new ShaderGraphNode_ShaderInputs(m_pSchema->GetInputDeclarations(), m_pSchema->GetInputDeclarationCount());
    pInputsNode->SetName("SHADER_INPUTS");
    if (!AddNode(pInputsNode))
    {
        Log_ErrorPrintf("ShaderGraph: Unable to add shader inputs node to graph.");
        delete pInputsNode;
        return false;
    }

    // create the outputs node
    ShaderGraphNode_ShaderOutputs *pOutputsNode = new ShaderGraphNode_ShaderOutputs(m_pSchema->GetOutputDeclarations(), m_pSchema->GetOutputDeclarationCount());
    pOutputsNode->SetName("SHADER_OUTPUTS");
    if (!AddNode(pOutputsNode))
    {
        Log_ErrorPrintf("ShaderGraph: Unable to add shader outputs node to graph.");
        delete pOutputsNode;
        return false;
    }

    m_pShaderInputsNode = pInputsNode;
    m_pShaderOutputsNode = pOutputsNode;
    return true;
}

bool ShaderGraph::LoadFromXML(const ShaderGraphSchema *pSchema, XMLReader &xmlReader)
{
    // set schema and create inout nodes
    if (!Create(pSchema))
        return false;

    // expected to be in the appropriate element
    if (!xmlReader.IsEmptyElement())
    {
        // load nodes
        for (;;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT && !xmlReader.IsEmptyElement())
            {
                int32 shaderGraphSelection = xmlReader.Select("nodes|links");
                if (shaderGraphSelection < 0)
                    return false;

                switch (shaderGraphSelection)
                {
                    // nodes
                case 0:
                    {
                        if (!ParseXMLNodes(xmlReader))
                            return false;
                    }
                    break;

                    // links
                case 1:
                    {
                        if (!ParseXMLLinks(xmlReader))
                            return false;
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                break;
            }
        }
    }

    return true;
}

//DebugAssert(!Y_stricmp(xmlReader.GetNodeName(), "shadergraph"));

bool ShaderGraph::ParseXMLNodes(XMLReader &xmlReader)
{
    if (!xmlReader.IsEmptyElement())
    {
        for (;;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                if (xmlReader.Select("node") != 0)
                    return false;

                const char *nodeType = xmlReader.FetchAttributeString("type");
                if (nodeType == NULL)
                {
                    xmlReader.PrintError("Incomplete node definition.");
                    return false;
                }

                // lookup node type
                //const ShaderGraphNodeTypeInfo *pTypeInfo = ShaderGraphNode::FindTypeInfoForShortName(nodeType);
                //if (pTypeInfo == nullptr || pTypeInfo->GetFactory() == nullptr)
                const ObjectTypeInfo *pTypeInfo = ObjectTypeInfo::GetRegistry().GetTypeInfoByName(nodeType);
                if (pTypeInfo == nullptr || !pTypeInfo->IsDerived(OBJECT_TYPEINFO(ShaderGraphNode)) || pTypeInfo->GetFactory() == nullptr)
                {
                    xmlReader.PrintError("Unknown node type '%s', or the type is uncreateable.", nodeType);
                    return false;
                }

                // create it
                Object *pNodeObject = pTypeInfo->GetFactory()->CreateObject();
                DebugAssert(pNodeObject != nullptr);

                // cast it
                ShaderGraphNode *pNode = pNodeObject->Cast<ShaderGraphNode>();

                // load it
                if (!pNode->LoadFromXML(this, xmlReader))
                {
                    delete pNode;
                    return false;
                }

                // add to graph
                if (!AddNode(pNode))
                {
                    xmlReader.PrintError("Failed to add node to graph.");
                    delete pNode;
                    return false;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(!Y_stricmp(xmlReader.GetNodeName(), "nodes"));
                break;
            }
            else
            {
                UnreachableCode();
            }
        }
    }

    return true;
}

bool ShaderGraph::ParseXMLLinks(XMLReader &xmlReader)
{
    if (!xmlReader.IsEmptyElement())
    {
        for (;;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                if (xmlReader.Select("link") != 0)
                    return false;

                const char *linkTarget = xmlReader.FetchAttribute("target");
                const char *linkInputName = xmlReader.FetchAttribute("input");
                const char *linkSourceName = xmlReader.FetchAttribute("source");
                const char *linkOutputName = xmlReader.FetchAttribute("output");
                const char *linkSwizzleStr = xmlReader.FetchAttribute("swizzle");
                if (linkSourceName == NULL || linkOutputName == NULL || linkTarget == NULL || linkInputName == NULL)
                {
                    xmlReader.PrintError("Incomplete link definition.");
                    return false;
                }

                // Parse swizzle.
                SHADER_GRAPH_VALUE_SWIZZLE linkSwizzle = SHADER_GRAPH_VALUE_SWIZZLE_NONE;
                if (linkSwizzleStr != NULL && !GetValueSwizzleFromString(&linkSwizzle, linkSwizzleStr))
                {
                    xmlReader.PrintError("Unknown swizzle: '%s'", linkSwizzleStr);
                    return false;
                }

                // Find source node.
                ShaderGraphNode *pSourceNode = GetNodeByName(linkSourceName);
                if (pSourceNode == NULL)
                {
                    xmlReader.PrintError("Could not find source node '%s' in graph.", linkSourceName);
                    return false;
                }

                // Find target node.
                ShaderGraphNode *pTargetNode = GetNodeByName(linkTarget);
                if (pTargetNode == NULL)
                {
                    xmlReader.PrintError("Could not find target node '%s' in graph.", linkTarget);
                    return false;
                }

                // Search for the output index based on name.
                uint32 linkOutputIndex;
                for (linkOutputIndex = 0; linkOutputIndex < pSourceNode->GetOutputCount(); linkOutputIndex++)
                {
                    if (Y_stricmp(linkOutputName, pSourceNode->GetOutput(linkOutputIndex)->GetOutputDesc()->Name) == 0)
                        break;
                }
                if (linkOutputIndex == pSourceNode->GetOutputCount())
                {
                    xmlReader.PrintError("Could not find an output named '%s' in '%s' (%s)", linkOutputName, pSourceNode->GetName().GetCharArray(), pSourceNode->GetTypeInfo()->GetTypeName());
                    return false;
                }

                // repeat for input.
                uint32 linkInputIndex;
                for (linkInputIndex = 0; linkInputIndex < pTargetNode->GetInputCount(); linkInputIndex++)
                {
                    if (Y_stricmp(linkInputName, pTargetNode->GetInput(linkInputIndex)->GetInputDesc()->Name) == 0)
                        break;
                }
                if (linkInputIndex == pTargetNode->GetInputCount())
                {
                    xmlReader.PrintError("Could not find an input named '%s' in '%s' (%s)", linkInputName, pTargetNode->GetName().GetCharArray(), pTargetNode->GetTypeInfo()->GetTypeName());
                    return false;
                }

                // Create the link.
                //if (!pTargetNode->GetInput(linkTargetIndex)->SetLink(pSourceNode, linkSourceIndex, linkSwizzle))
                if (!AddLink(pTargetNode, linkInputIndex, pSourceNode, linkOutputIndex, linkSwizzle))
                {
                    xmlReader.PrintError("Could not link node '%s' output '%s' to node '%s' input '%s' (swizzle: %s)",
                                         pSourceNode->GetName().GetCharArray(), pSourceNode->GetOutput(linkOutputIndex)->GetOutputDesc()->Name,
                                         pTargetNode->GetName().GetCharArray(), pTargetNode->GetInput(linkInputIndex)->GetInputDesc()->Name,
                                         (linkSwizzleStr != NULL) ? linkSwizzleStr : "none");

                    return false;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(!Y_stricmp(xmlReader.GetNodeName(), "links"));
                break;
            }
            else
            {
                UnreachableCode();
            }
        }
    }

    return true;
}

static uint32 GetNodeInputDepth(const ShaderGraphNodeInput *pInput, uint32 currentDepth)
{
    const ShaderGraphNode *pSourceNode = pInput->GetSourceNode();
    if (pSourceNode == NULL)
        return currentDepth;

    uint32 i;
    uint32 passDepth = currentDepth + 1;
    uint32 maxDepth = 0;
    for (i = 0; i < pSourceNode->GetInputCount(); i++)
    {
        const ShaderGraphNodeInput *pNodeInput = pSourceNode->GetInput(i);
        maxDepth = Max(maxDepth, GetNodeInputDepth(pNodeInput, passDepth));
    }

    return maxDepth;
}

bool ShaderGraph::SaveToXML(XMLWriter &xmlWriter) const
{
    // save nodes
    xmlWriter.StartElement("nodes");
    if (m_Nodes.GetSize() > 0)
    {
        uint32 i;
        for (i = 0; i < m_Nodes.GetSize(); i++)
        {
            const ShaderGraphNode *pNode = m_Nodes[i];

            // don't write input/output nodes
            if (m_pShaderInputsNode == pNode || m_pShaderOutputsNode == pNode)
                continue;

            // create the node element
            xmlWriter.StartElement("node");
            xmlWriter.WriteAttribute("type", pNode->GetTypeInfo()->GetShortName());

            // save the node
            pNode->SaveToXML(xmlWriter);

            // end node element
            xmlWriter.EndElement();
        }
    }
    xmlWriter.EndElement();

    // save links
    xmlWriter.StartElement("links");
    if (m_Nodes.GetSize() > 0)
    {
        // algo:
        //   determine max depth
        //   for i = max depth, i >= 0, i--
        //      save all links for nodes at this depth

        uint32 i, j, k;
        uint32 *pNodeMaxLinkDepths = new uint32[m_Nodes.GetSize()];
        uint32 maxOverallInputDepth = 0;
        
        // determine max link depth of each node
        for (i = 0; i < m_Nodes.GetSize(); i++)
        {
            const ShaderGraphNode *pNode = m_Nodes[i];
            uint32 nodeMaxInputDepth = 0;
            for (j = 0; j < pNode->GetInputCount(); j++)
            {
                uint32 nodeInputDepth = GetNodeInputDepth(pNode->GetInput(j), 0);
                nodeMaxInputDepth = Max(nodeMaxInputDepth, nodeInputDepth);
            }

            // store
            pNodeMaxLinkDepths[i] = nodeMaxInputDepth;
        }

        // save links at each node at each link depth level
        for (i = 0; i <= maxOverallInputDepth; i++)
        {
            for (j = 0; j < m_Nodes.GetSize(); j++)
            {
                if (pNodeMaxLinkDepths[j] != i)
                    continue;

                const ShaderGraphNode *pNode = m_Nodes[j];
                for (k = 0; k < pNode->GetInputCount(); k++)
                {
                    const ShaderGraphNodeInput *pInput = pNode->GetInput(k);
                    const ShaderGraphNode *pSourceNode = pInput->GetSourceNode();
                    if (pSourceNode != NULL)
                    {
                        char swizzleStr[5];
                        if (!GetStringForValueSwizzle(swizzleStr, pInput->GetSwizzle()))
                        {
                            Log_ErrorPrintf("Corrupted swizzle value in shader graph");
                            delete[] pNodeMaxLinkDepths;
                            return false;
                        }

                        xmlWriter.StartElement("link");
                        xmlWriter.WriteAttribute("target", pNode->GetName());
                        xmlWriter.WriteAttribute("input", pInput->GetInputDesc()->Name);
                        xmlWriter.WriteAttribute("source", pSourceNode->GetName());
                        xmlWriter.WriteAttribute("output", pSourceNode->GetOutput(pInput->GetSourceOutputIndex())->GetOutputDesc()->Name);
                        xmlWriter.WriteAttribute("swizzle", swizzleStr);
                        xmlWriter.EndElement();
                    }
                }                    
            }
        }

        delete[] pNodeMaxLinkDepths;
    }

    return true;
}

bool ShaderGraph::GetValueSwizzleFromString(SHADER_GRAPH_VALUE_SWIZZLE *pSwizzle, const char *str)
{
    uint8 components[4] = { 255, 255, 255, 255 };
    for (uint32 i = 0; i < 4; i++)
    {
        if (str[i] == '\0')
            break;

        char ch = str[i];
        if (ch == 'R' || ch == 'r')
            components[i] = 0;
        else if (ch == 'G' || ch == 'g')
            components[i] = 1;
        else if (ch == 'B' || ch == 'b')
            components[i] = 2;
        else if (ch == 'A' || ch == 'a')
            components[i] = 3;
        else
            return false;
    }

    *pSwizzle = ((uint32)components[0] << 24) | ((uint32)components[1] << 16) | ((uint32)components[2] << 8) | ((uint32)components[3]);
    return true;
}

bool ShaderGraph::GetStringForValueSwizzle(char outString[5], SHADER_GRAPH_VALUE_SWIZZLE swizzle)
{
    uint32 i;
    for (i = 0; i < 4; i++)
    {
        uint32 componentIndex = (swizzle >> ((3 - i) * 8)) & 0xFF;
        if (componentIndex == 255)
            break;

        if (componentIndex >= 4)
            return false;

        static const char componentNames[4] = { 'r', 'g', 'b', 'a' };
        outString[i] = componentNames[componentIndex];
    }

    outString[i] = '\0';
    return true;
}

uint32 ShaderGraph::GetValueSwizzleComponentCount(SHADER_GRAPH_VALUE_SWIZZLE swizzle)
{
    if (((swizzle >> 24) & 0xFF) < 4)
    {
        if (((swizzle >> 16) & 0xFF) < 4)
        {
            if (((swizzle >> 8) & 0xFF) < 4)
            {
                if (((swizzle) & 0xFF) < 4)
                {
                    return 4;
                }
                else
                {
                    return 3;
                }
            }
            else
            {
                return 2;
            }
        }
        else
        {
            return 1;
        }
    }
    else
    {
        return 0;
    }
}
