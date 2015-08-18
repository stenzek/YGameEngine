#pragma once
#include "ResourceCompiler/Common.h"
#include "ResourceCompiler/ShaderGraphNode.h"

struct ResourceCompilerCallbacks;

class ShaderGraphSchema : public ReferenceCounted
{
public:
    // keep in mind: these declarations are in the opposite order. inputs to the shader have outputs,
    // and outputs from the shader have inputs from the point of the graph.

    // shader global declaration
    struct GlobalDeclaration
    {
        String Name;
        String DisplayName;
        SHADER_PARAMETER_TYPE Type;
        String VariableName;

        SHADER_GRAPH_NODE_OUTPUT OutputDesc;
    };

    // shader input declaration
    struct InputDeclaration
    {
        String Name;
        String DisplayName;
        SHADER_PARAMETER_TYPE Type;

        struct AccessInfo
        {
            String DefineName;
            String VariableName;
        };
        AccessInfo Access[SHADER_PROGRAM_STAGE_COUNT];

        SHADER_GRAPH_NODE_OUTPUT OutputDesc;
    };

    // shader output declaration
    struct OutputDeclaration
    {
        String Name;
        String DisplayName;
        SHADER_PROGRAM_STAGE Stage;
        SHADER_PARAMETER_TYPE Type;
        String FunctionSignature;
        String DefineName;
        String DefaultValue;

        SHADER_GRAPH_NODE_INPUT InputDesc;
    };

public:
    ShaderGraphSchema();
    ~ShaderGraphSchema();

    bool LoadFromXML(const char *FileName, ByteStream *pStream);

    const GlobalDeclaration *const *GetGlobalDeclarations() const { return m_globalDeclarations.GetBasePointer(); }
    const InputDeclaration *const *GetInputDeclarations() const { return m_inputDeclarations.GetBasePointer(); }
    const OutputDeclaration *const *GetOutputDeclarations() const { return m_outputDeclarations.GetBasePointer(); }
    const GlobalDeclaration *GetGlobalDeclaration(uint32 i) const { DebugAssert(i < m_globalDeclarations.GetSize()); return m_globalDeclarations[i]; }
    const InputDeclaration *GetInputDeclaration(uint32 i) const { DebugAssert(i < m_inputDeclarations.GetSize()); return m_inputDeclarations[i]; }
    const OutputDeclaration *GetOutputDeclaration(uint32 i) const { DebugAssert(i < m_outputDeclarations.GetSize()); return m_outputDeclarations[i]; }
    uint32 GetGlobalDeclarationCount() const { return m_globalDeclarations.GetSize(); }
    uint32 GetInputDeclarationCount() const { return m_inputDeclarations.GetSize(); }
    uint32 GetOutputDeclarationCount() const { return m_outputDeclarations.GetSize(); }    

    static ShaderGraphSchema *GetSchemaForFeatureLevel(ResourceCompilerCallbacks *pCallbacks, RENDERER_FEATURE_LEVEL featureLevel);

private:
    typedef PODArray<GlobalDeclaration *> GlobalDeclarationArray;
    typedef PODArray<InputDeclaration *> InputDeclarationArray;
    typedef PODArray<OutputDeclaration *> OutputDeclarationArray;

    GlobalDeclarationArray m_globalDeclarations;
    InputDeclarationArray m_inputDeclarations;
    OutputDeclarationArray m_outputDeclarations;
};
