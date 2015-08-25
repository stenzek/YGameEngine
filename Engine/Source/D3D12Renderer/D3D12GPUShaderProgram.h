#pragma once
#include "D3D12Renderer/D3D12Common.h"
#include "D3D11Renderer/D3DShaderCacheEntry.h"

class D3D12GPUShaderProgram : public GPUShaderProgram
{
public:
    struct ShaderLocalConstantBuffer
    {
        String Name;
        uint32 ParameterIndex;
        uint32 Size;
        uint32 EngineConstantBufferIndex;

        D3D12GPUBuffer *pLocalGPUBuffer;
        byte *pLocalBuffer;
        int32 LocalBufferDirtyLowerBounds;
        int32 LocalBufferDirtyUpperBounds;
    };

    struct ShaderParameter
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

    // pipeline object key
    struct PipelineStateKey
    {
        D3D12_RASTERIZER_DESC RasterizerState;
        D3D12_DEPTH_STENCIL_DESC DepthStencilState;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE PrimitiveTopologyType;
        D3D12_BLEND_DESC BlendState;
        uint32 RenderTargetCount;
        PIXEL_FORMAT RTVFormats[8];
        PIXEL_FORMAT DSVFormat;
    };

public:
    D3D12GPUShaderProgram();
    virtual ~D3D12GPUShaderProgram();

    // resource virtuals
    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    // create
    bool Create(D3D12GPUDevice *pDevice, const GPU_VERTEX_ELEMENT_DESC *pVertexAttributes, uint32 nVertexAttributes, ByteStream *pByteCodeStream);

    // bind to a context that has no current shader
    void Bind(D3D12GPUContext *pContext);

    // switch context to a new shader
    void Switch(D3D12GPUContext *pContext, D3D12GPUShaderProgram *pCurrentProgram);

    // update the local constant buffers for this shader with pending values
    void CommitLocalConstantBuffers(D3D12GPUContext *pContext);

    // unbind from a context, resetting all shader states
    void Unbind(D3D12GPUContext *pContext);

    // --- internal query api ---
    uint32 InternalGetConstantBufferCount() const { return m_constantBuffers.GetSize(); }
    uint32 InternalGetParameterCount() const { return m_parameters.GetSize(); }
    const ShaderLocalConstantBuffer *GetConstantBuffer(uint32 Index) const { DebugAssert(Index < m_constantBuffers.GetSize()); return &m_constantBuffers[Index]; }
    const ShaderParameter *GetParameter(uint32 Index) const { DebugAssert(Index < m_parameters.GetSize()); return &m_parameters[Index]; }

    // --- internal parameter api ---
    void InternalSetParameterValue(D3D12GPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue);
    void InternalSetParameterValueArray(D3D12GPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue, uint32 firstElement, uint32 numElements);
    void InternalSetParameterStruct(GPUContext *pContext, uint32 parameterIndex, const void *pValue, uint32 valueSize);
    void InternalSetParameterStructArray(GPUContext *pContext, uint32 parameterIndex, const void *pValue, uint32 valueSize, uint32 firstElement, uint32 numElements);
    void InternalSetParameterResource(D3D12GPUContext *pContext, uint32 parameterIndex, GPUResource *pResource, GPUSamplerState *pLinkedSamplerState);
    void InternalBindAutomaticParameters(D3D12GPUContext *pContext);

    // --- pipeline state accessor ---
    ID3D12PipelineState *GetPipelineState(const PipelineStateKey *pKey);

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

    // compiled pipeline object
    struct PipelineState
    {
        ID3D12PipelineState *pPipelineState;
        uint32 Key1;
        uint32 Key2[4];
    };
    MemArray<PipelineState> m_pipelineStates;

    // arrays of above
    ConstantBufferArray m_constantBuffers;
    ParameterArray m_parameters;

    // copies of bytecode for all shader stages
    BinaryBlob *m_pStageByteCode[SHADER_PROGRAM_STAGE_COUNT];
};

