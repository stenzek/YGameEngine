#pragma once
#include "Renderer/ShaderComponent.h"

class GPUResource;
class ShaderProgram;

class TextureBlitShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(TextureBlitShader, ShaderComponent);

public:
    enum FLAGS
    {
        USE_TEXTURE_LOD         = (1 << 0),
    };

public:
    TextureBlitShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static void SetProgramParameters(GPUCommandList *pCommandList, ShaderProgram *pShaderProgram, GPUTexture *pSourceTexture, GPUSamplerState *pSamplerState, uint32 sourceLevel);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

