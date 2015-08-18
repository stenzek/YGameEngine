#pragma once
#include "Renderer/Common.h"
#include "Renderer/ShaderComponentTypeInfo.h"
#include "Core/Object.h"

class VertexFactoryTypeInfo;
class ShaderCompiler;
class ShaderProgram;

class ShaderComponent : public Object
{
    DECLARE_SHADER_COMPONENT_INFO(ShaderComponent, Object);

public:
    ShaderComponent(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

