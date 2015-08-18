#pragma once
#include "Engine/Common.h"
#include "ResourceCompiler/ShaderGraphNode.h"
#include "ResourceCompiler/ShaderGraphSchema.h"

class XMLReader;
class XMLWriter;

class ShaderGraph
{
public:
    ShaderGraph();
    ~ShaderGraph();

    const ShaderGraphSchema *GetSchema() const { return m_pSchema; }
    const uint32 GetNextNodeId() const { return m_iNextNodeId; }
    
    const ShaderGraphNode *GetShaderInputsNode() const { return m_pShaderInputsNode; }
    const ShaderGraphNode *GetShaderOutputsNode() const { return m_pShaderOutputsNode; }

    const uint32 GetNodeCount() const { return m_Nodes.GetSize(); }
    
    const ShaderGraphNode *GetNodeByID(uint32 NodeID) const;
    const ShaderGraphNode *GetNodeByArrayIndex(uint32 ArrayIndex) const { DebugAssert(ArrayIndex < m_Nodes.GetSize()); return m_Nodes[ArrayIndex]; }
    const ShaderGraphNode *GetNodeByName(const char *Name) const;

    ShaderGraphNode *GetNodeByID(uint32 NodeID);
    ShaderGraphNode *GetNodeByArrayIndex(uint32 ArrayIndex) { DebugAssert(ArrayIndex < m_Nodes.GetSize()); return m_Nodes[ArrayIndex]; }
    ShaderGraphNode *GetNodeByName(const char *Name);

    bool AddNode(ShaderGraphNode *pNode);
    bool RemoveNode(ShaderGraphNode *pNode);

    bool AddLink(ShaderGraphNode *pTargetNode, uint32 InputIndex, ShaderGraphNode *pSourceNode, uint32 OutputIndex, SHADER_GRAPH_VALUE_SWIZZLE Swizzle);
    bool RemoveLink(ShaderGraphNode *pTargetNode, uint32 InputIndex);
    bool RemoveLink(ShaderGraphNodeInput *pInput);

    bool Create(const ShaderGraphSchema *pSchema);
    bool LoadFromXML(const ShaderGraphSchema *pSchema, XMLReader &xmlReader);
    bool SaveToXML(XMLWriter &xmlWriter) const;

    // Type registration.
    static void RegisterBuiltinTypes();

    // helper functions
    static bool GetValueSwizzleFromString(SHADER_GRAPH_VALUE_SWIZZLE *pSwizzle, const char *str);
    static bool GetStringForValueSwizzle(char outString[5], SHADER_GRAPH_VALUE_SWIZZLE swizzle);
    static uint32 GetValueSwizzleComponentCount(SHADER_GRAPH_VALUE_SWIZZLE swizzle);
    static uint32 GetValueSwizzleComponent(SHADER_GRAPH_VALUE_SWIZZLE swizzle, uint32 index) { return (swizzle >> ((3 - index) * 8)) & 0xFF; }
    
protected:
    bool ParseXMLNodes(XMLReader &xmlReader);
    bool ParseXMLLinks(XMLReader &xmlReader);

    const ShaderGraphSchema *m_pSchema;

    typedef PODArray<ShaderGraphNode *> NodeArray;
    NodeArray m_Nodes;
    uint32 m_iNextNodeId;

    typedef PODArray<ShaderGraphNodeInput *> LinkArray;
    LinkArray m_Links;

    // schema nodes
    ShaderGraphNode *m_pShaderInputsNode;
    ShaderGraphNode *m_pShaderOutputsNode;

private:
    ShaderGraph(const ShaderGraph &);
    ShaderGraph &operator=(const ShaderGraph &);
};

