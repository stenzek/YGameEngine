#pragma once
#include "Renderer/ShaderComponent.h"

class GPUTexture;
class Texture;
class ShaderProgram;

class PlainShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(PlainShader, ShaderComponent);

public:
    enum FLAGS
    {
        WITH_TEXTURE                        = (1 << 0),
        WITH_VERTEX_COLOR                   = (1 << 1),
        WITH_MATERIAL                       = (1 << 2),
        BLEND_TEXTURE_AND_VERTEX_COLOR      = (1 << 3),
    };

public:
    PlainShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) {}

    static void SetTexture(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture *pTexture);
    static void SetTexture(GPUContext *pContext, ShaderProgram *pShaderProgram, Texture *pTexture);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};
