#pragma once
#include "ResourceCompiler/ShaderGraphCompiler.h"

class ShaderGraphNode_ShaderOutputs;

class ShaderGraphCompilerHLSL : public ShaderGraphCompiler
{
public:
    ShaderGraphCompilerHLSL(const ShaderGraph *pShaderGraph);
    ~ShaderGraphCompilerHLSL();

    virtual bool Compile(ByteStream *pOutputStream) override;

    // emitters
    virtual bool EmitAdd(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB) override;
    virtual bool EmitSubtract(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB) override;
    virtual bool EmitMultiply(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB) override;
    virtual bool EmitDivide(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB) override;
    virtual bool EmitDot(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB) override;

    virtual bool EmitNormalize(const ShaderGraphNodeInput *pNode) override;
    virtual bool EmitNegate(const ShaderGraphNodeInput *pNode) override;
    virtual bool EmitFractionalPart(const ShaderGraphNodeInput *pNode) override;

    virtual void EmitConstant(SHADER_PARAMETER_TYPE ValueType, const void *pValue) override;
    virtual void EmitConstantBool(bool Value) override;
    virtual void EmitConstantInt(int32 Value) override;
    virtual void EmitConstantInt2(const int2 &Value) override;
    virtual void EmitConstantInt3(const int3 &Value) override;
    virtual void EmitConstantInt4(const int4 &Value) override;
    virtual void EmitConstantFloat(const float &Value) override;
    virtual void EmitConstantFloat2(const float2 &Value) override;
    virtual void EmitConstantFloat3(const float3 &Value) override;
    virtual void EmitConstantFloat4(const float4 &Value) override;
    virtual bool EmitAccessShaderGlobal(const char *GlobalName) override;
    virtual bool EmitAccessShaderInput(const char *InputName) override;
    virtual bool EmitAccessExternalParameter(const ExternalParameter *pExternalParameter) override;
    virtual bool EmitTextureSample(const ShaderGraphNodeInput *pTextureNodeLink, const ShaderGraphNodeInput *pTexCoordNodeLink, SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION unpackOperation) override;
    virtual bool EmitConstructPrimitive(SHADER_PARAMETER_TYPE primitiveType, const ShaderGraphNodeInput **ppInputNodes, uint32 nInputNodes) override;
    virtual void EmitSwizzle(SHADER_GRAPH_VALUE_SWIZZLE Swizzle) override;

    virtual bool EvaluateNode(const ShaderGraphNode *pSourceNode, uint32 OutputIndex, SHADER_GRAPH_VALUE_SWIZZLE Swizzle, SHADER_GRAPH_VALUE_SWIZZLE FixupSwizzle) override;
    virtual bool EvaluateNode(const ShaderGraphNodeInput *pNodeLink) override;

protected:
    typedef KeyValuePair<const ShaderGraphNode *, String> NodeStringPair;
    typedef KeyValuePair<String, String> DefinePair;
    typedef LinkedList<NodeStringPair> GlobalsMap;
    typedef LinkedList<DefinePair> CompileDefineMap;

    static void CleanString(String &str);
    static const char *GetTypeNameString(SHADER_PARAMETER_TYPE ValueType);

    bool AddCompileDefine(const char *Name, const char *Value);
    bool AddBufferedCompileDefine(const char *Name, const char *Value);

    bool InternalCompile();
    bool InternalCompileDefaultValue(SHADER_PARAMETER_TYPE Type, const char *DefaultValue);
    bool InternalCompileOutput(uint32 outputIndex);

    SHADER_PROGRAM_STAGE m_currentStage;
    GlobalsMap m_globals;
    CompileDefineMap m_defines;
    String m_externsCode;
    String m_globalsCode;
    String m_methodsCode;
    String m_currentScope;
    String m_buffer;
    CompileDefineMap m_bufferedDefines;
};

