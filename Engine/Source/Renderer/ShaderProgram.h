#pragma once
#include "Renderer/RendererTypes.h"

class GPUContext;
class GPUResource;
class GPUSamplerState;
class ShaderComponentTypeInfo;
class VertexFactoryTypeInfo;
class GPUShaderProgram;
class MaterialShader;

class ShaderProgram
{
public:
    ShaderProgram(GPUShaderProgram *pGPUProgram, uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    ~ShaderProgram();

    const uint32 GetGlobalShaderFlags() const { return m_globalShaderFlags; }
    const ShaderComponentTypeInfo *GetBaseShaderTypeInfo() const { return m_pBaseShaderTypeInfo; }
    const uint32 GetBaseShaderFlags() const { return m_baseShaderFlags; }
    const VertexFactoryTypeInfo *GetVertexFactoryTypeInfo() const { return m_pVertexFactoryTypeInfo; }
    const uint32 GetVertexFactoryFlags() const { return m_vertexFactoryFlags; }
    const MaterialShader *GetMaterialShader() const { return m_pMaterialShader; }
    const uint32 GetMaterialShaderFlags() const { return m_materialShaderFlags; }

    GPUShaderProgram *GetGPUProgram() const { return m_pGPUProgram; }

    // --- parameter tables ---
    void SetBaseShaderParameterValue(GPUCommandList *pCommandList, uint32 index, SHADER_PARAMETER_TYPE type, const void *pValue) const;
    void SetBaseShaderParameterValueArray(GPUCommandList *pCommandList, uint32 index, SHADER_PARAMETER_TYPE type, const void *pValues, uint32 firstElement, uint32 numElements) const;
    void SetBaseShaderParameterStruct(GPUCommandList *pCommandList, uint32 index, const void *pValue, uint32 valueSize) const;
    void SetBaseShaderParameterStructArray(GPUCommandList *pCommandList, uint32 index, const void *pValue, uint32 valueSize, uint32 firstElement, uint32 numElements) const;
    void SetBaseShaderParameterResource(GPUCommandList *pCommandList, uint32 index, GPUResource *pResource) const;
    void SetBaseShaderParameterTexture(GPUCommandList *pCommandList, uint32 index, GPUTexture *pTexture, GPUSamplerState *pSamplerState) const;
    void SetVertexFactoryParameterValue(GPUCommandList *pCommandList, uint32 index, SHADER_PARAMETER_TYPE type, const void *pValue) const;
    void SetVertexFactoryParameterValueArray(GPUCommandList *pCommandList, uint32 index, SHADER_PARAMETER_TYPE type, const void *pValues, uint32 firstElement, uint32 numElements) const;
    void SetVertexFactoryParameterStruct(GPUCommandList *pCommandList, uint32 index, const void *pValue, uint32 valueSize) const;
    void SetVertexFactoryParameterStructArray(GPUCommandList *pCommandList, uint32 index, const void *pValue, uint32 valueSize, uint32 firstElement, uint32 numElements) const;
    void SetVertexFactoryParameterResource(GPUCommandList *pCommandList, uint32 index, GPUResource *pResource) const;
    void SetVertexFactoryParameterTexture(GPUCommandList *pCommandList, uint32 index, GPUTexture *pTexture, GPUSamplerState *pSamplerState) const;
    void SetMaterialParameterValue(GPUCommandList *pCommandList, uint32 index, SHADER_PARAMETER_TYPE type, const void *pValue) const;
    void SetMaterialParameterValueArray(GPUCommandList *pCommandList, uint32 index, SHADER_PARAMETER_TYPE type, const void *pValues, uint32 firstElement, uint32 numElements) const;
    void SetMaterialParameterResource(GPUCommandList *pCommandList, uint32 index, GPUResource *pResource) const;
    void SetMaterialParameterTexture(GPUCommandList *pCommandList, uint32 index, GPUTexture *pTexture, GPUSamplerState *pSamplerState) const;

    // comparitor
    static int32 Compare(const ShaderProgram *a, const ShaderProgram *b);

    // ordering operators
    bool operator<(const ShaderProgram &other) const { return (Compare(this, &other) < 0); }

private:
    // parameter map generators
    void GenerateBaseShaderParameterMap();
    void GenerateVertexFactoryParameterMap();
    void GenerateMaterialShaderUniformParameterMap();
    void GenerateMaterialShaderTextureParameterMap();

    // ordered by size, access frequency for lookup, then use
    const ShaderComponentTypeInfo *m_pBaseShaderTypeInfo;
    const VertexFactoryTypeInfo *m_pVertexFactoryTypeInfo;
    const MaterialShader *m_pMaterialShader;
    uint32 m_globalShaderFlags;
    uint32 m_baseShaderFlags;
    uint32 m_vertexFactoryFlags;
    uint32 m_materialShaderFlags;
    GPUShaderProgram *m_pGPUProgram;
    int32 *m_pBaseShaderParameterMap;
    int32 *m_pVertexFactoryParameterMap;
    int32 *m_pMaterialShaderUniformParameterMap;
    int32 *m_pMaterialShaderTextureParameterMap;
};

// D3D12 version
class ShaderPipeline : public ShaderProgram
{
public:
    struct CreationParameters
    {
        // TODO: order in the order of most frequently changing -> least, so that searching is faster

        const GPURasterizerState *pRasterizerState;
        const GPUDepthStencilState *pDepthStencilState;
        const GPUBlendState *pBlendState;

        const ShaderComponentTypeInfo *pBaseShaderTypeInfo;
        const VertexFactoryTypeInfo *pVertexFactoryTypeInfo;
        const MaterialShader *pMaterialShader;

        uint32 GlobalShaderFlags;
        uint32 BaseShaderFlags;
        uint32 VertexFactoryFlags;
        uint32 MaterialShaderFlags;

        PIXEL_FORMAT OutputColorFormat;
        PIXEL_FORMAT OutputDepthStencilFormat;
    };

public:
    ShaderPipeline(GPUShaderPipeline *pGPUPipeline, const CreationParameters *pCreationParameters);
    ~ShaderPipeline();

    // comparitor
    static int32 Compare(const ShaderPipeline *a, const ShaderPipeline *b);

    // ordering operators
    bool operator<(const ShaderPipeline &other) const { return (Compare(this, &other) < 0); }
};