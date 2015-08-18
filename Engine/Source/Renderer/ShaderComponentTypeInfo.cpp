#include "Renderer/PrecompiledHeader.h"
#include "Renderer/ShaderComponentTypeInfo.h"

ShaderComponentTypeInfo::ShaderComponentTypeInfo(const char *TypeName, const ObjectTypeInfo *pParentTypeInfo, 
                                                   IsValidPermutationFunction fpIsValidPermutation, FillShaderCompilerParametersFunction fpFillShaderCompilerParameters, 
                                                   const SHADER_COMPONENT_PARAMETER_BINDING *pParameterBindings)
    : ObjectTypeInfo(TypeName, pParentTypeInfo, nullptr, nullptr),
      m_pParameterBindings(pParameterBindings), m_nParameterBindings(0), m_parameterCRC(0),
      m_fpIsValidPermutation(fpIsValidPermutation),
      m_fpFillShaderCompilerParameters(fpFillShaderCompilerParameters)
{
    for (; m_pParameterBindings[m_nParameterBindings].ParameterName != nullptr; m_nParameterBindings++);
}

ShaderComponentTypeInfo::~ShaderComponentTypeInfo()
{

}

bool ShaderComponentTypeInfo::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags) const
{
    return m_fpIsValidPermutation(globalShaderFlags, pBaseShaderTypeInfo, baseShaderFlags, pVertexFactoryTypeInfo, vertexFactoryFlags, pMaterialShader, materialShaderFlags);
}

bool ShaderComponentTypeInfo::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters) const
{
    return m_fpFillShaderCompilerParameters(globalShaderFlags, baseShaderFlags, vertexFactoryFlags, pParameters);
}

void ShaderComponentTypeInfo::SetParameterCRC(uint32 parameterCRC)
{
    m_parameterCRC = parameterCRC;
}

