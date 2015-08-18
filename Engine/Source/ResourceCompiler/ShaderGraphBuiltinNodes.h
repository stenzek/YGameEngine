#pragma once
#include "ResourceCompiler/ShaderGraphNode.h"
#include "ResourceCompiler/ShaderGraphSchema.h"

class ResourceTypeInfo;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ShaderGraphNode_Global : public ShaderGraphNode
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Global, ShaderGraphNode);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Global);

public:
    ShaderGraphNode_Global(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}
    virtual ~ShaderGraphNode_Global() {}

    const String &GetGlobalName() const { return m_globalName; }
    void SetGlobalName(const char *globalName);

    virtual bool LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader) override;
    virtual void SaveToXML(XMLWriter &xmlWriter) const override;

    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override;

protected:
    String m_globalName;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ShaderGraphNode_ShaderInputs : public ShaderGraphNode
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_ShaderInputs, ShaderGraphNode);
    DECLARE_SHADER_GRAPH_NODE_NO_FACTORY(ShaderGraphNode_ShaderInputs);

public:
    ShaderGraphNode_ShaderInputs(const ShaderGraphSchema::InputDeclaration *const *ppInputDeclarations, uint32 nInputDeclarations, const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo);

    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override;
};

class ShaderGraphNode_ShaderOutputs : public ShaderGraphNode
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_ShaderOutputs, ShaderGraphNode);
    DECLARE_SHADER_GRAPH_NODE_NO_FACTORY(ShaderGraphNode_ShaderOutputs);

public:
    ShaderGraphNode_ShaderOutputs(const ShaderGraphSchema::OutputDeclaration *const *ppOutputDeclarations, uint32 nOutputDeclarations, const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ShaderGraphNode_Parameter : public ShaderGraphNode
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Parameter, ShaderGraphNode);
    DECLARE_SHADER_GRAPH_NODE_NO_FACTORY(ShaderGraphNode_Parameter);

public:
    ShaderGraphNode_Parameter(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    const String &GetParameterName() const { return m_strParameterName; }
    void SetParameterName(const char *ParameterName);

    virtual bool LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader);
    virtual void SaveToXML(XMLWriter &xmlWriter) const;

protected:
    String m_strParameterName;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ShaderGraphNode_UniformParameter : public ShaderGraphNode_Parameter
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_UniformParameter, ShaderGraphNode_Parameter);
    DECLARE_SHADER_GRAPH_NODE_NO_FACTORY(ShaderGraphNode_UniformParameter);

public:
    ShaderGraphNode_UniformParameter(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    virtual bool LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader) override final;
    virtual void SaveToXML(XMLWriter &xmlWriter) const override final;
    
    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;
};

class ShaderGraphNode_TextureParameter : public ShaderGraphNode_Parameter
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_TextureParameter, ShaderGraphNode_Parameter);
    DECLARE_SHADER_GRAPH_NODE_NO_FACTORY(ShaderGraphNode_TextureParameter);

public:
    ShaderGraphNode_TextureParameter(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    virtual const ResourceTypeInfo *GetTextureTypeInfo() const = 0;

    virtual bool LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader) override final;
    virtual void SaveToXML(XMLWriter &xmlWriter) const override final;
    
    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;
};

class ShaderGraphNode_StaticSwitchParameter : public ShaderGraphNode_Parameter
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_StaticSwitchParameter, ShaderGraphNode_Parameter);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_StaticSwitchParameter);

public:
    ShaderGraphNode_StaticSwitchParameter(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    //virtual bool LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader) override;
    //virtual void SaveToXML(XMLWriter &xmlWriter) const;

    virtual bool OnInputConnectionChange(uint32 InputIndex) override final;
    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ShaderGraphNode_FloatParameter : public ShaderGraphNode_UniformParameter
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_FloatParameter, ShaderGraphNode_UniformParameter);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_FloatParameter);

public:
    ShaderGraphNode_FloatParameter(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}
};

class ShaderGraphNode_Float2Parameter : public ShaderGraphNode_UniformParameter
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float2Parameter, ShaderGraphNode_UniformParameter);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float2Parameter);

public:
    ShaderGraphNode_Float2Parameter(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}
};

class ShaderGraphNode_Float3Parameter : public ShaderGraphNode_UniformParameter
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float3Parameter, ShaderGraphNode_UniformParameter);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float3Parameter);

public:
    ShaderGraphNode_Float3Parameter(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}
};

class ShaderGraphNode_Float4Parameter : public ShaderGraphNode_UniformParameter
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float4Parameter, ShaderGraphNode_UniformParameter);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float4Parameter);

public:
    ShaderGraphNode_Float4Parameter(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ShaderGraphNode_Texture2DParameter : public ShaderGraphNode_TextureParameter
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Texture2DParameter, ShaderGraphNode_TextureParameter);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Texture2DParameter);

public:
    ShaderGraphNode_Texture2DParameter(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    const ResourceTypeInfo *GetTextureTypeInfo() const;
};

class ShaderGraphNode_Texture2DArrayParameter : public ShaderGraphNode_TextureParameter
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Texture2DArrayParameter, ShaderGraphNode_TextureParameter);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Texture2DArrayParameter);

public:
    ShaderGraphNode_Texture2DArrayParameter(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    const ResourceTypeInfo *GetTextureTypeInfo() const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ShaderGraphNode_Constant : public ShaderGraphNode
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Constant, ShaderGraphNode);
    DECLARE_SHADER_GRAPH_NODE_NO_FACTORY(ShaderGraphNode_Constant);

public:
    ShaderGraphNode_Constant(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    virtual void GetValueString(String &Destination) const = 0;
    virtual bool SetValueString(const char *NewValue) = 0;

    virtual bool LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader) override final;
    virtual void SaveToXML(XMLWriter &xmlWriter) const override final;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ShaderGraphNode_FloatConstant : public ShaderGraphNode_Constant
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_FloatConstant, ShaderGraphNode_Constant);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_FloatConstant);

public:
    ShaderGraphNode_FloatConstant(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo);

    const float &GetValue() const { return m_Value; }
    void SetValue(const float &v) { m_Value = v; }

    virtual void GetValueString(String &Destination) const override final { StringConverter::FloatToString(Destination, m_Value); }
    virtual bool SetValueString(const char *NewValue) override final { m_Value = StringConverter::StringToFloat(NewValue); return true; }

    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;

protected:
    float m_Value;
};

class ShaderGraphNode_Float2Constant : public ShaderGraphNode_Constant
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float2Constant, ShaderGraphNode_Constant);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float2Constant);

public:
    ShaderGraphNode_Float2Constant(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo);

    const float2 &GetValue() const { return m_Value; }
    void SetValue(const float2 &v) { m_Value = v; }

    virtual void GetValueString(String &Destination) const override final { StringConverter::Float2ToString(Destination, m_Value); }
    virtual bool SetValueString(const char *NewValue) override final { m_Value = StringConverter::StringToFloat2(NewValue); return true; }

    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;

protected:
    float2 m_Value;
};

class ShaderGraphNode_Float3Constant : public ShaderGraphNode_Constant
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float3Constant, ShaderGraphNode_Constant);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float3Constant);

public:
    ShaderGraphNode_Float3Constant(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo);

    const float3 &GetValue() const { return m_Value; }
    void SetValue(const float3 &v) { m_Value = v; }

    virtual void GetValueString(String &Destination) const override final { StringConverter::Float3ToString(Destination, m_Value); }
    virtual bool SetValueString(const char *NewValue) override final { m_Value = StringConverter::StringToFloat3(NewValue); return true; }

    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;

protected:
    float3 m_Value;
};

class ShaderGraphNode_Float4Constant : public ShaderGraphNode_Constant
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float4Constant, ShaderGraphNode_Constant);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float4Constant);

public:
    ShaderGraphNode_Float4Constant(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo);

    const float4 &GetValue() const { return m_Value; }
    void SetValue(const float4 &v) { m_Value = v; }

    virtual void GetValueString(String &Destination) const override final { StringConverter::Float4ToString(Destination, m_Value); }
    virtual bool SetValueString(const char *NewValue) override final { m_Value = StringConverter::StringToFloat4(NewValue); return true; }

    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;

protected:
    float4 m_Value;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ShaderGraphNode_Variable : public ShaderGraphNode
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Variable, ShaderGraphNode);
    DECLARE_SHADER_GRAPH_NODE_NO_FACTORY(ShaderGraphNode_Variable);

public:
    ShaderGraphNode_Variable(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}
};

class ShaderGraphNode_Float2Variable : public ShaderGraphNode
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float2Variable, ShaderGraphNode);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float2Variable);

public:
    ShaderGraphNode_Float2Variable(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;
};

class ShaderGraphNode_Float3Variable : public ShaderGraphNode
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float3Variable, ShaderGraphNode);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float3Variable);

public:
    ShaderGraphNode_Float3Variable(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;
};

class ShaderGraphNode_Float4Variable : public ShaderGraphNode
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Float4Variable, ShaderGraphNode);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Float4Variable);

public:
    ShaderGraphNode_Float4Variable(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ShaderGraphNode_Operator : public ShaderGraphNode
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Operator, ShaderGraphNode);
    DECLARE_SHADER_GRAPH_NODE_NO_FACTORY(ShaderGraphNode_Operator);

public:
    ShaderGraphNode_Operator(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}
};

class ShaderGraphNode_Add : public ShaderGraphNode_Operator
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Add, ShaderGraphNode_Operator);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Add);

public:
    ShaderGraphNode_Add(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    virtual bool OnInputConnectionChange(uint32 InputIndex) override final;
    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;
};

class ShaderGraphNode_Subtract : public ShaderGraphNode_Operator
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Subtract, ShaderGraphNode_Operator);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Subtract);

public:
    ShaderGraphNode_Subtract(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    virtual bool OnInputConnectionChange(uint32 InputIndex) override final;
    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;
};

class ShaderGraphNode_Multiply : public ShaderGraphNode_Operator
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Multiply, ShaderGraphNode_Operator);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Multiply);

public:
    ShaderGraphNode_Multiply(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    virtual bool OnInputConnectionChange(uint32 InputIndex) override final;
    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;
};

class ShaderGraphNode_Divide : public ShaderGraphNode_Operator
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Divide, ShaderGraphNode_Operator);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Divide);

public:
    ShaderGraphNode_Divide(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    virtual bool OnInputConnectionChange(uint32 InputIndex) override final;
    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;
};

class ShaderGraphNode_Dot : public ShaderGraphNode_Operator
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Dot, ShaderGraphNode_Operator);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Dot);

public:
    ShaderGraphNode_Dot(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    virtual bool OnInputConnectionChange(uint32 InputIndex) override final;
    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ShaderGraphNode_Normalize : public ShaderGraphNode
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Normalize, ShaderGraphNode);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Normalize);

public:
    ShaderGraphNode_Normalize(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    virtual bool OnInputConnectionChange(uint32 InputIndex) override final;
    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;
};

class ShaderGraphNode_Negate : public ShaderGraphNode
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_Negate, ShaderGraphNode);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_Negate);

public:
    ShaderGraphNode_Negate(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    virtual bool OnInputConnectionChange(uint32 InputIndex) override final;
    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;
};

class ShaderGraphNode_FractionalPart : public ShaderGraphNode
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_FractionalPart, ShaderGraphNode);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_FractionalPart);

public:
    ShaderGraphNode_FractionalPart(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo) {}

    virtual bool OnInputConnectionChange(uint32 InputIndex) override final;
    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ShaderGraphNode_TextureSample : public ShaderGraphNode
{
    DECLARE_SHADER_GRAPH_NODE_TYPE_INFO(ShaderGraphNode_TextureSample, ShaderGraphNode);
    DECLARE_SHADER_GRAPH_NODE_GENERIC_FACTORY(ShaderGraphNode_TextureSample);

public:
    ShaderGraphNode_TextureSample(const ShaderGraphNodeTypeInfo *pTypeInfo = &s_typeInfo) : BaseClass(pTypeInfo), m_eUnpackOperation(SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION_NONE) {}

    const SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION GetUnpackOperation() const { return (SHADER_GRAPH_TEXTURE_SAMPLE_UNPACK_OPERATION)m_eUnpackOperation; }

    virtual bool OnInputConnectionChange(uint32 InputIndex) override final;
    virtual bool EmitOutput(ShaderGraphCompiler *pCompiler, uint32 OutputIndex) const override final;

    virtual bool LoadFromXML(const ShaderGraph *pShaderGraph, XMLReader &xmlReader) override final;
    virtual void SaveToXML(XMLWriter &xmlWriter) const override final;

private:
    uint32 m_eUnpackOperation;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

