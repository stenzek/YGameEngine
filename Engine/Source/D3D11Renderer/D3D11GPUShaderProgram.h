#pragma once
#include "D3D11Renderer/D3D11Common.h"
#include "D3D11Renderer/D3DShaderCacheEntry.h"

class D3D11GPUShaderProgram : public GPUShaderProgram
{
public:
    struct ConstantBuffer
    {
        // @TODO this class could potentially be removed to save a level of indirection
        String Name;
        uint32 ParameterIndex;
        uint32 MinimumSize;
        uint32 EngineConstantBufferIndex;
    };

    struct Parameter
    {
        String Name;
        SHADER_PARAMETER_TYPE Type;
        int32 ConstantBufferIndex;
        uint32 ConstantBufferOffset;
        uint32 ArraySize;
        uint32 ArrayStride;
        D3D_SHADER_BIND_TARGET BindTarget;
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
    bool Create(D3D11GPUDevice *pDevice, const GPU_VERTEX_ELEMENT_DESC *pVertexAttributes, uint32 nVertexAttributes, ByteStream *pByteCodeStream);

    // bind to a context that has no current shader
    void Bind(D3D11GPUContext *pContext);

    // switch context to a new shader
    void Switch(D3D11GPUContext *pContext, D3D11GPUShaderProgram *pCurrentProgram);

    // unbind from a context, resetting all shader states
    void Unbind(D3D11GPUContext *pContext);

    // --- internal query api ---
    uint32 InternalGetConstantBufferCount() const { return m_constantBuffers.GetSize(); }
    uint32 InternalGetParameterCount() const { return m_parameters.GetSize(); }
    const ConstantBuffer *GetConstantBuffer(uint32 Index) const { DebugAssert(Index < m_constantBuffers.GetSize()); return &m_constantBuffers[Index]; }
    const Parameter *GetParameter(uint32 Index) const { DebugAssert(Index < m_parameters.GetSize()); return &m_parameters[Index]; }

    // --- internal parameter api ---
    void InternalSetParameterValue(D3D11GPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue);
    void InternalSetParameterValueArray(D3D11GPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue, uint32 firstElement, uint32 numElements);
    void InternalSetParameterStruct(D3D11GPUContext *pContext, uint32 parameterIndex, const void *pValue, uint32 valueSize);
    void InternalSetParameterStructArray(D3D11GPUContext *pContext, uint32 parameterIndex, const void *pValue, uint32 valueSize, uint32 firstElement, uint32 numElements);
    void InternalSetParameterResource(D3D11GPUContext *pContext, uint32 parameterIndex, GPUResource *pResource, GPUSamplerState *pLinkedSamplerState);
    void InternalBindAutomaticParameters(D3D11GPUContext *pContext);

    // --- public parameter api ---
    virtual uint32 GetParameterCount() const override;
    virtual void GetParameterInformation(uint32 index, const char **name, SHADER_PARAMETER_TYPE *type, uint32 *arraySize) override;

protected:
    // arrays
    typedef Array<ConstantBuffer> ConstantBufferArray;
    typedef Array<Parameter> ParameterArray;

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
