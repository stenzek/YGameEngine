#include "Renderer/PrecompiledHeader.h"
#include "Renderer/ShaderComponent.h"

DEFINE_SHADER_COMPONENT_INFO(ShaderComponent);
BEGIN_SHADER_COMPONENT_PARAMETERS(ShaderComponent)
END_SHADER_COMPONENT_PARAMETERS()

ShaderComponent::ShaderComponent(const ShaderComponentTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo)
{

}

bool ShaderComponent::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    return false;
}

bool ShaderComponent::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    return false;
}
