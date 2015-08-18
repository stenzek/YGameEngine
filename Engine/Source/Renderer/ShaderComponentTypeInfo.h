#pragma once
#include "Renderer/Common.h"
#include "Renderer/RendererTypes.h"
#include "Core/ObjectTypeInfo.h"

class VertexFactoryTypeInfo;
class MaterialShader;
struct ShaderCompilerParameters;

struct SHADER_COMPONENT_PARAMETER_BINDING
{
    const char *ParameterName;
    SHADER_PARAMETER_TYPE ExpectedType;
};

class ShaderComponentTypeInfo : public ObjectTypeInfo
{
public:
    typedef bool(*IsValidPermutationFunction)(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    typedef bool(*FillShaderCompilerParametersFunction)(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
    
public:
    ShaderComponentTypeInfo(const char *TypeName, const ObjectTypeInfo *pParentTypeInfo, 
                             IsValidPermutationFunction fpIsValidPermutation, FillShaderCompilerParametersFunction fpFillShaderCompilerParameters, 
                             const SHADER_COMPONENT_PARAMETER_BINDING *pParameterBindings);

    ~ShaderComponentTypeInfo();

    const SHADER_COMPONENT_PARAMETER_BINDING *GetParameterBindings() const { return m_pParameterBindings; }
    uint32 GetParameterBindingCount() const { return m_nParameterBindings; }
    uint32 GetParameterCRC() const { return m_parameterCRC; }

    // wrappers
    bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags) const;
    bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters) const;

    // parameter crc
    void SetParameterCRC(uint32 parameterCRC);

protected:
    const SHADER_COMPONENT_PARAMETER_BINDING *m_pParameterBindings;
    uint32 m_nParameterBindings;
    uint32 m_parameterCRC;

    IsValidPermutationFunction m_fpIsValidPermutation;
    FillShaderCompilerParametersFunction m_fpFillShaderCompilerParameters;
};

#define DECLARE_SHADER_COMPONENT_INFO(Type, ParentType) \
            private: \
            static ShaderComponentTypeInfo s_TypeInfo; \
            static const SHADER_COMPONENT_PARAMETER_BINDING s_parameterBindings[]; \
            public: \
            typedef Type ThisClass; \
            typedef ParentType BaseClass; \
            static const ShaderComponentTypeInfo *StaticTypeInfo() { return &s_TypeInfo; }  \
            static ShaderComponentTypeInfo *StaticMutableTypeInfo() { return &s_TypeInfo; }

#define DEFINE_SHADER_COMPONENT_INFO(Type) \
            ShaderComponentTypeInfo Type::s_TypeInfo = ShaderComponentTypeInfo( #Type , OBJECT_TYPEINFO(BaseClass), \
                                                                                        &Type::IsValidPermutation, &Type::FillShaderCompilerParameters, \
                                                                                        Type::s_parameterBindings);

#define BEGIN_SHADER_COMPONENT_PARAMETERS(Type) const SHADER_COMPONENT_PARAMETER_BINDING Type::s_parameterBindings[] = {
#define DEFINE_SHADER_COMPONENT_PARAMETER(Name, ExpectedType) { Name, ExpectedType },
#define END_SHADER_COMPONENT_PARAMETERS() { NULL, SHADER_PARAMETER_TYPE_COUNT } };

#define SHADER_COMPONENT_INFO(Type) Type::StaticTypeInfo()
#define SHADER_COMPONENT_INFO_PTR(Ptr) Ptr->StaticTypeInfo()

#define MUTABLE_SHADER_COMPONENT_INFO(Type) Type::StaticMutableTypeInfo()
#define MUTABLE_SHADER_COMPONENT_INFO_PTR(Type) Type->StaticMutableTypeInfo()

