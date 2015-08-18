#include "Renderer/PrecompiledHeader.h"
#include "Renderer/VertexFactoryTypeInfo.h"

VertexFactoryTypeInfo::VertexFactoryTypeInfo(const char *TypeName, const ObjectTypeInfo *pParentTypeInfo, 
                                             IsValidPermutationFunction fpIsValidPermutation, FillShaderCompilerParametersFunction fpFillShaderCompilerParameters, 
                                             const SHADER_COMPONENT_PARAMETER_BINDING *pParameterBindings,
                                             GetVertexElementsDescFunction fpGetVertexElementsDesc)
    : ShaderComponentTypeInfo(TypeName, pParentTypeInfo, 
                              fpIsValidPermutation, fpFillShaderCompilerParameters,
                              pParameterBindings),
      m_fpGetVertexElementsDesc(fpGetVertexElementsDesc)
{

}

VertexFactoryTypeInfo::~VertexFactoryTypeInfo()
{

}
