#pragma once
#include "ResourceCompiler/ShaderGraphNodeType.h"

class XMLReader;
class XMLWriter;
class ShaderGraph;
class ShaderGraphNode;
class ShaderGraphCompiler;

enum SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION
{
    SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION_NONE,
    SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION_NORMAL_MAP,
    SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION_COUNT,
};

namespace NameTables {
    Y_Declare_NameTable(ShaderGraphTextureSampleUnpackOperation);
}

class ShaderGraphNodeInput
{
public:
    ShaderGraphNodeInput();
    ~ShaderGraphNodeInput();
    
    const ShaderGraphNode *GetNode() const { return m_pNode; }
    const SHADER_GRAPH_NODE_INPUT *GetInputDesc() const { return m_pInputDesc; }
    const uint32 GetInputIndex() const { return m_iInputIndex; }
    const ShaderGraphNode *GetSourceNode() const { return m_pSourceNode; }
    uint32 GetSourceOutputIndex() const { return m_iSourceOutputIndex; }
    SHADER_GRAPH_VALUE_SWIZZLE GetSwizzle() const { return m_eSwizzle; }
    SHADER_GRAPH_VALUE_SWIZZLE GetFixupSwizzle() const { return m_eFixupSwizzle; }
    SHADER_PARAMETER_TYPE GetValueType() const { return m_eValueType; }
    bool IsLinked() const { return (m_pSourceNode != NULL); }

    void Init(ShaderGraphNode *pNode, uint32 InputIndex, const SHADER_GRAPH_NODE_INPUT *pInputDesc);
    void SetExpectedType(SHADER_PARAMETER_TYPE ExpectedType);
    bool SetLink(const ShaderGraphNode *pSourceNode, uint32 OutputIndex, SHADER_GRAPH_VALUE_SWIZZLE Swizzle);
    void ClearLink();

    bool SetFixupSwizzle(SHADER_PARAMETER_TYPE expectedOutputType);
    void ClearFixupSwizzle();

    static bool CanSwizzleType(SHADER_PARAMETER_TYPE ValueType, SHADER_GRAPH_VALUE_SWIZZLE Swizzle);
    static SHADER_PARAMETER_TYPE GetTypeAfterSwizzle(SHADER_PARAMETER_TYPE ValueType, SHADER_GRAPH_VALUE_SWIZZLE Swizzle);
    static bool CanAutoTruncateType(SHADER_GRAPH_VALUE_SWIZZLE *pSwizzle, SHADER_PARAMETER_TYPE expectedType, SHADER_PARAMETER_TYPE valueType);

protected:
    ShaderGraphNode *m_pNode;
    uint32 m_iInputIndex;
    const SHADER_GRAPH_NODE_INPUT *m_pInputDesc;
    const ShaderGraphNode *m_pSourceNode;
    uint32 m_iSourceOutputIndex;
    SHADER_GRAPH_VALUE_SWIZZLE m_eSwizzle;
    SHADER_GRAPH_VALUE_SWIZZLE m_eFixupSwizzle;
    SHADER_PARAMETER_TYPE m_eValueType;
};

class ShaderGraphNodeOutput
{
public:
    ShaderGraphNodeOutput();
    ~ShaderGraphNodeOutput();
    
    const SHADER_GRAPH_NODE_OUTPUT *GetOutputDesc() const { return m_pOutputDesc; }
    const uint32 GetOutputIndex() const { return m_iOutputIndex; }
    SHADER_PARAMETER_TYPE GetValueType() const { return m_eValueType; }
    uint32 GetLinkCount() const { return m_iLinkCount; }

    void Init(ShaderGraphNode *pNode, uint32 OutputIndex, const SHADER_GRAPH_NODE_OUTPUT *pOutputDesc);
    void SetValueType(SHADER_PARAMETER_TYPE NewValueType);
    
    void AddLinkReference() const { m_iLinkCount++; }
    void RemoveLinkReference() const { DebugAssert(m_iLinkCount > 0); m_iLinkCount--; }

protected:
    ShaderGraphNode *m_pNode;
    const SHADER_GRAPH_NODE_OUTPUT *m_pOutputDesc;
    uint32 m_iOutputIndex;
    SHADER_PARAMETER_TYPE m_eValueType;
    mutable uint32 m_iLinkCount;
};

class ShaderGraphNode : public Object
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode, Object);
    DECLARE_SHADER_GRAPH_NODE_NO_FACTORY(ShaderGraphNode);

public:
    ShaderGraphNode(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~ShaderGraphNode();

    // Retrieves the type information for this object.
    const ShaderGraphNodeTypeInfo *GetTypeInfo() const { return m_pTypeInfo; }

    // Node ID.
    const uint32 GetID() const { return m_iID; }
    void SetID(uint32 ID) { m_iID = ID; }

    // Node name.
    const String &GetName() const { return m_strName; }
    void SetName(const char *Name) { m_strName = Name; }

    // Shader graph editor position.
    const int2 &GetGraphEditorPosition() const { return m_GraphEditorPosition; }
    void SetGraphEditorPosition(const int2 &NewPosition) { m_GraphEditorPosition = NewPosition; }

    // Input nodes.
    const uint32 GetInputCount() const { return m_nInputs; }
    const ShaderGraphNodeInput *GetInput(uint32 i) const { DebugAssert(i < m_nInputs); return &m_pInputs[i]; }
    ShaderGraphNodeInput *GetInput(uint32 i) { DebugAssert(i < m_nInputs); return &m_pInputs[i]; }

    // Number of nodes that have this node linked to their input.
    const uint32 GetOutputCount() const { return m_nOutputs; }
    const ShaderGraphNodeOutput *GetOutput(uint32 i) const { DebugAssert(i < m_nOutputs); return &m_pOutputs[i]; }
    ShaderGraphNodeOutput *GetOutput(uint32 i) { DebugAssert(i < m_nOutputs); return &m_pOutputs[i]; }

    // Connection events.
    virtual bool OnInputConnectionChange(uint32 InputIndex);

    // Loading/saving to XML.
    virtual bool LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader);
    virtual void SaveToXML(XMLWriter &xmlWriter) const;

    // For compiler.
    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const;

    // Helper function for looking up type info based on short names
    // Optimize me at some point
    static const ShaderGraphNodeTypeInfo *FindTypeInfoForShortName(const char *shortName);

protected:
    const ShaderGraphNodeTypeInfo *m_pTypeInfo;
    uint32 m_iID;
    String m_strName;

    // Inputs.
    ShaderGraphNodeInput *m_pInputs;
    uint32 m_nInputs;

    // Outputs.
    ShaderGraphNodeOutput *m_pOutputs;
    uint32 m_nOutputs;

    // Position in graph editor.
    int2 m_GraphEditorPosition;
};

