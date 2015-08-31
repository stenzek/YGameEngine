#pragma once
#include "D3D12Renderer/D3D12Common.h"
#include "D3D11Renderer/D3DShaderCacheEntry.h"

class D3D12GPUShaderProgram : public GPUShaderProgram
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

    struct ShaderParameter
    {
        String Name;
        SHADER_PARAMETER_TYPE Type;
        uint32 ConstantBufferIndex;
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

    // switch context to a new shader
    bool Switch(D3D12GPUContext *pContext, ID3D12GraphicsCommandList *pCommandList, const PipelineStateKey *pKey);

    // --- internal query api ---
    uint32 InternalGetParameterCount() const { return m_parameters.GetSize(); }
    const ShaderParameter *GetParameter(uint32 Index) const { DebugAssert(Index < m_parameters.GetSize()); return &m_parameters[Index]; }

    // --- internal parameter api ---
    void InternalSetParameterValue(D3D12GPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue);
    void InternalSetParameterValueArray(D3D12GPUContext *pContext, uint32 parameterIndex, SHADER_PARAMETER_TYPE valueType, const void *pValue, uint32 firstElement, uint32 numElements);
    void InternalSetParameterStruct(GPUContext *pContext, uint32 parameterIndex, const void *pValue, uint32 valueSize);
    void InternalSetParameterStructArray(GPUContext *pContext, uint32 parameterIndex, const void *pValue, uint32 valueSize, uint32 firstElement, uint32 numElements);
    void InternalSetParameterResource(D3D12GPUContext *pContext, uint32 parameterIndex, GPUResource *pResource, GPUSamplerState *pLinkedSamplerState);

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
    typedef Array<ConstantBuffer> ConstantBufferArray;
    typedef Array<ShaderParameter> ParameterArray;

    // compiled pipeline object
    struct PipelineState
    {
        ID3D12PipelineState *pPipelineState;
        uint32 Key1;
        uint32 Key2[4];
    };
    MemArray<PipelineState> m_pipelineStates;

    // vertex attributes
    MemArray<D3D12_INPUT_ELEMENT_DESC> m_vertexAttributes;

    // arrays of above
    ConstantBufferArray m_constantBuffers;
    ParameterArray m_parameters;

    // copies of bytecode for all shader stages
    BinaryBlob *m_pStageByteCode[SHADER_PROGRAM_STAGE_COUNT];

    // debug name
    String m_debugName;
};

