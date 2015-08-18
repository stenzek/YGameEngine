#pragma once
#include "ResourceCompiler/ShaderGraph.h"
#include "Renderer/RendererTypes.h"

class ShaderGraphNode_MaterialInput;
class ShaderGraphNode_UniformParameter;
class ShaderGraphNode_TextureParameter;

class ShaderGraphCompiler
{
public:
    struct ExternalParameter
    {
        String Name;
        SHADER_PARAMETER_TYPE Type;
        String BindingName;
    };

public:
    ShaderGraphCompiler(const ShaderGraph *pShaderGraph);
    virtual ~ShaderGraphCompiler();

    const ShaderGraph *GetShaderGraph() const { return m_pShaderGraph; }

    const ExternalParameter *GetExternalUniformParameter(const char *uniformName) const;
    const ExternalParameter *GetExternalTextureParameter(const char *textureName) const;
    const bool GetExternalStaticSwitchParameter(const char *staticSwitchName) const;

    void AddExternalUniformParameter(const char *uniformName, SHADER_PARAMETER_TYPE uniformType, const char *bindingName);
    void AddExternalTextureParameter(const char *textureName, TEXTURE_TYPE textureType, const char *bindingName);
    void AddExternalStaticSwitchParameter(const char *staticSwitchName, bool enabled);

    virtual bool Compile(ByteStream *pOutputStream) = 0;

    // emitters
    virtual bool EmitAdd(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB) = 0;
    virtual bool EmitSubtract(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB) = 0;
    virtual bool EmitMultiply(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB) = 0;
    virtual bool EmitDivide(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB) = 0;
    virtual bool EmitDot(const ShaderGraphNodeInput *pA, const ShaderGraphNodeInput *pB) = 0;
    virtual bool EmitNormalize(const ShaderGraphNodeInput *pNode) = 0;
    virtual bool EmitNegate(const ShaderGraphNodeInput *pNode) = 0;
    virtual bool EmitFractionalPart(const ShaderGraphNodeInput *pNode) = 0;
    virtual void EmitConstant(SHADER_PARAMETER_TYPE ValueType, const void *pValue) = 0;
    virtual void EmitConstantBool(bool Value) = 0;
    virtual void EmitConstantInt(int32 Value) = 0;
    virtual void EmitConstantInt2(const int2 &Value) = 0;
    virtual void EmitConstantInt3(const int3 &Value) = 0;
    virtual void EmitConstantInt4(const int4 &Value) = 0;
    virtual void EmitConstantFloat(const float &Value) = 0;
    virtual void EmitConstantFloat2(const float2 &Value) = 0;
    virtual void EmitConstantFloat3(const float3 &Value) = 0;
    virtual void EmitConstantFloat4(const float4 &Value) = 0;
    virtual bool EmitAccessShaderGlobal(const char *GlobalName) = 0;
    virtual bool EmitAccessShaderInput(const char *InputName) = 0;
    virtual bool EmitAccessExternalParameter(const ExternalParameter *pExternalParameter) = 0;
    virtual bool EmitTextureSample(const ShaderGraphNodeInput *pTextureNodeLink, const ShaderGraphNodeInput *pTexCoordNodeLink, SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION unpackOperation) = 0;
    virtual bool EmitConstructPrimitive(SHADER_PARAMETER_TYPE primitiveType, const ShaderGraphNodeInput **ppInputNodes, uint32 nInputNodes) = 0;
    virtual void EmitSwizzle(SHADER_GRAPH_VALUE_SWIZZLE Swizzle) = 0;

    virtual bool EvaluateNode(const ShaderGraphNode *pSourceNode, uint32 OutputIndex, SHADER_GRAPH_VALUE_SWIZZLE Swizzle, SHADER_GRAPH_VALUE_SWIZZLE FixupSwizzle) = 0; 
    virtual bool EvaluateNode(const ShaderGraphNodeInput *pNodeLink) = 0;

protected:
    const ShaderGraph *m_pShaderGraph;

    typedef LinkedList<ExternalParameter> ExternalParameterList;
    ExternalParameterList m_ExternalUniforms;
    ExternalParameterList m_ExternalTextures;

    typedef LinkedList< Pair<String, bool> > StaticSwitchList;
    StaticSwitchList m_ExternalStaticSwitches;
};
