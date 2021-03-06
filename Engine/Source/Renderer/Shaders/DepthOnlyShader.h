#pragma once
#include "Renderer/ShaderComponent.h"

class DepthOnlyShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(DepthOnlyShader, ShaderComponent);
    
public:
    DepthOnlyShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { };

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

