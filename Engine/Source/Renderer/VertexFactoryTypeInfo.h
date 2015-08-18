#pragma once
#include "Renderer/ShaderComponentTypeInfo.h"
#include "Renderer/RendererTypes.h"

class VertexFactory;

class VertexFactoryTypeInfo : public ShaderComponentTypeInfo
{
public:
    typedef uint32(*GetVertexElementsDescFunction)(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS]);

public:
    VertexFactoryTypeInfo(const char *TypeName, const ObjectTypeInfo *pParentTypeInfo,
                          IsValidPermutationFunction fpIsValidPermutation, FillShaderCompilerParametersFunction fpFillShaderCompilerParameters, 
                          const SHADER_COMPONENT_PARAMETER_BINDING *pParameterBindings,
                          GetVertexElementsDescFunction fpGetVertexElementsDesc);

    ~VertexFactoryTypeInfo();

    uint32 GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS]) const { return m_fpGetVertexElementsDesc(platform, featureLevel, flags, pElementsDesc); }

protected:
    GetVertexElementsDescFunction m_fpGetVertexElementsDesc;
};

#define DECLARE_VERTEX_FACTORY_TYPE_INFO(Type, ParentType) \
            private: \
            static VertexFactoryTypeInfo s_TypeInfo; \
            static const SHADER_COMPONENT_PARAMETER_BINDING s_parameterBindings[]; \
            public: \
            typedef Type ThisClass; \
            typedef ParentType BaseClass; \
            static const VertexFactoryTypeInfo *StaticTypeInfo() { return &s_TypeInfo; }  \
            static VertexFactoryTypeInfo *StaticMutableTypeInfo() { return &s_TypeInfo; }

#define DECLARE_VERTEX_FACTORY_FACTORY(Type) \
            public: \
            static VertexFactory *CreateInstance(uint32 Flags, GPUVertexArray *pVertexArray) { return new Type(Flags, pVertexArray); }

#define DEFINE_VERTEX_FACTORY_TYPE_INFO(Type) \
            VertexFactoryTypeInfo Type::s_TypeInfo = VertexFactoryTypeInfo( #Type , OBJECT_TYPEINFO(BaseClass), \
                                                                                        &Type::IsValidPermutation, &Type::FillShaderCompilerParameters, \
                                                                                        Type::s_parameterBindings, \
                                                                                        &Type::GetVertexElementsDesc);

#define VERTEX_FACTORY_TYPE_INFO(Type) Type::StaticTypeInfo()
#define VERTEX_FACTORY_TYPE_INFO_PTR(Ptr) Ptr->StaticTypeInfo()

#define VERTEX_FACTORY_MUTABLE_TYPE_INFO(Type) Type::StaticMutableTypeInfo()
#define VERTEX_FACTORY_MUTABLE_TYPE_INFO_PTR(Type) Type->StaticMutableTypeInfo()
