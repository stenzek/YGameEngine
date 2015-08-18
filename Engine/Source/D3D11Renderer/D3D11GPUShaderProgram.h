#pragma once
#include "D3D11Renderer/D3D11Common.h"
#include "D3D11Renderer/D3D11ShaderCacheEntry.h"

class D3D11GPUShaderProgram : public GPUShaderProgram
{
public:
    struct ShaderLocalConstantBuffer
    {
        char Name[D3D11_SHADER_CACHE_ENTRY_MAX_NAME_LENGTH];
        uint32 ParameterIndex;
        uint32 Size;
        uint32 EngineConstantBufferIndex;

        D3D11GPUBuffer *pLocalGPUBuffer;
        byte *pLocalBuffer;
        int32 LocalBufferDirtyLowerBounds;
        int32 LocalBufferDirtyUpperBounds;
    };

    struct ShaderParameter
    {
        char Name[D3D11_SHADER_CACHE_ENTRY_MAX_NAME_LENGTH];
        SHADER_PARAMETER_TYPE Type;
        int32 ConstantBufferIndex;
        uint32 ConstantBufferOffset;
        uint32 ArraySize;
        uint32 ArrayStride;
        D3D11_SHADER_BIND_TARGET BindTarget;
        int32 BindPoint[SHADER_PROGRAM_STAGE_COUNT];
        int32 LinkedSamplerIndex;
    };

public:
    D3D11GPUShaderProgram();
    virtual ~D3D11GPUShaderProgram();

    // resource virtuals
    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    // create
    bool Create(D3D11Renderer *pRenderer, const GPU_VERTEX_ELEMENT_DESC *pVertexAttributes, uint32 nVertexAttributes, ByteStream *pByteCodeStream);

    // bind to a context that has no current shader
    void Bind(D3D11GPUContext *pContext);

    // switch context to a new shader
    void Switch(D3D11GPUContext *pContext, D3D11GPUShaderProgram *pCurrentProgram);

    // update the local constant buffers for this shader with pending values
    void CommitLocalConstantBuffers(D3D11GPUContext *pContext);

    // unbind from a context, resetting all shader states
    void Unbind(D3D11GPUContext *pContext);

    // --- internal query api ---
    uint32 InternalGetConstantBufferCount() const { return m_constantBuffers.GetSize(); }
    uint32 InternalGetParameterCount() const { return m_parameters.GetSize(); }
    const ShaderLocalConstantBuffer *GetConstantBuffer(uint32 Index) const { DebugAssert(Index < m_constantBuffers.GetSize()); return &m_constantBuffers[Index]; }
    const ShaderParameter *GetParameter(uint32 Index) const { DebugAssert(Index < m_parameters.GetSize()); return &m_parameters[Index]; }

    // --- internal parameter api ---
    void InternalSetParameterValue(D3D11GPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue);
    void InternalSetParameterValueArray(D3D11GPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue, uint32 firstElement, uint32 numElements);
    void InternalSetParameterStruct(GPUContext *pContext, uint32 parameterIndex, const void *pValue, uint32 valueSize);
    void InternalSetParameterStructArray(GPUContext *pContext, uint32 parameterIndex, const void *pValue, uint32 valueSize, uint32 firstElement, uint32 numElements);
    void InternalSetParameterResource(D3D11GPUContext *pContext, uint32 parameterIndex, GPUResource *pResource, GPUSamplerState *pLinkedSamplerState);
    void InternalBindAutomaticParameters(D3D11GPUContext *pContext);

    // --- public parameter api ---
    virtual uint32 GetParameterCount() const override;
    virtual void GetParameterInformation(uint32 index, const char **name, SHADER_PARAMETER_TYPE *type, uint32 *arraySize) override;
    virtual void SetParameterValue(GPUContext *pContext, uint32 index, SHADER_PARAMETER_TYPE valueType, const void *pValue) override;
    virtual void SetParameterValueArray(GPUContext *pContext, uint32 index, SHADER_PARAMETER_TYPE valueType, const void *pValue, uint32 firstElement, uint32 numElements) override;
    virtual void SetParameterStruct(GPUContext *pContext, uint32 index, const void *pValue, uint32 valueSize) override;
    virtual void SetParameterStructArray(GPUContext *pContext, uint32 index, const void *pValue, uint32 valueSize, uint32 firstElement, uint32 numElements) override;
    virtual void SetParameterResource(GPUContext *pContext, uint32 index, GPUResource *pResource) override;
    virtual void SetParameterTexture(GPUContext *pContext, uint32 index, GPUTexture *pTexture, GPUSamplerState *pSamplerState) override;

protected:
    // arrays
    typedef MemArray<ShaderLocalConstantBuffer> ConstantBufferArray;
    typedef MemArray<ShaderParameter> ParameterArray;

    // shader objects
    ID3D11InputLayout *m_pD3DInputLayout;
    ID3D11VertexShader *m_pD3DVertexShader;
    ID3D11HullShader *m_pD3DHullShader;
    ID3D11DomainShader *m_pD3DDomainShader;
    ID3D11GeometryShader *m_pD3DGeometryShader;
    ID3D11PixelShader *m_pD3DPixelShader;
    ID3D11ComputeShader *m_pD3DComputeShader;

    // arrays of above
    ConstantBufferArray m_constantBuffers;
    ParameterArray m_parameters;

    // context we are bound to
    D3D11GPUContext *m_pBoundContext;
};
