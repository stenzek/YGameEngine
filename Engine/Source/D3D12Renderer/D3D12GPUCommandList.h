#pragma once
#include "D3D12Renderer/D3D12Common.h"
#include "D3D12Renderer/D3D12CommandQueue.h"
#include "D3D12Renderer/D3D12DescriptorHeap.h"
#include "D3D12Renderer/D3D12LinearHeaps.h"

class D3D12GPUCommandList : public GPUCommandList
{
public:
    D3D12GPUCommandList(D3D12GPUDevice *pDevice, ID3D12Device *pD3DDevice);
    ~D3D12GPUCommandList();

    // State clearing
    virtual void ClearState(bool clearShaders = true, bool clearBuffers = true, bool clearStates = true, bool clearRenderTargets = true) override final;

    // Retrieve RendererVariables interface.
    virtual GPUContextConstants *GetConstants() override final { return m_pConstants; }

    // State Management (D3D12RenderSystem.cpp)
    virtual GPURasterizerState *GetRasterizerState() override final;
    virtual void SetRasterizerState(GPURasterizerState *pRasterizerState) override final;
    virtual GPUDepthStencilState *GetDepthStencilState() override final;
    virtual uint8 GetDepthStencilStateStencilRef() override final;
    virtual void SetDepthStencilState(GPUDepthStencilState *pDepthStencilState, uint8 stencilRef) override final;
    virtual GPUBlendState *GetBlendState() override final;
    virtual const float4 &GetBlendStateBlendFactor() override final;
    virtual void SetBlendState(GPUBlendState *pBlendState, const float4 &blendFactor = float4::One) override final;

    // Viewport Management (D3D12RenderSystem.cpp)
    virtual const RENDERER_VIEWPORT *GetViewport() override final;
    virtual void SetViewport(const RENDERER_VIEWPORT *pNewViewport) override final;
    virtual void SetFullViewport(GPUTexture *pForRenderTarget = NULL) override final;

    // Scissor Rect Management (D3D12RenderSystem.cpp)
    virtual const RENDERER_SCISSOR_RECT *GetScissorRect() override final;
    virtual void SetScissorRect(const RENDERER_SCISSOR_RECT *pScissorRect) override final;

    // Texture copying
    virtual bool CopyTexture(GPUTexture2D *pSourceTexture, GPUTexture2D *pDestinationTexture) override final;
    virtual bool CopyTextureRegion(GPUTexture2D *pSourceTexture, uint32 sourceX, uint32 sourceY, uint32 width, uint32 height, uint32 sourceMipLevel, GPUTexture2D *pDestinationTexture, uint32 destX, uint32 destY, uint32 destMipLevel) override final;

    // Blit (copy) a texture to the currently bound framebuffer. If this texture is a different size, it'll be resized
    virtual void BlitFrameBuffer(GPUTexture2D *pTexture, uint32 sourceX, uint32 sourceY, uint32 sourceWidth, uint32 sourceHeight, uint32 destX, uint32 destY, uint32 destWidth, uint32 destHeight, RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER resizeFilter = RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_NEAREST) override final;

    // Mip generation
    virtual void GenerateMips(GPUTexture *pTexture) override final;

    // Query accessing
    virtual bool BeginQuery(GPUQuery *pQuery) override final;
    virtual bool EndQuery(GPUQuery *pQuery) override final;

    // Predicated drawing
    virtual void SetPredication(GPUQuery *pQuery) override final;

    // RT Clearing
    virtual void ClearTargets(bool clearColor = true, bool clearDepth = true, bool clearStencil = true, const float4 &clearColorValue = float4::Zero, float clearDepthValue = 1.0f, uint8 clearStencilValue = 0) override final;
    virtual void DiscardTargets(bool discardColor = true, bool discardDepth = true, bool discardStencil = true) override final;

    // Swap chain
    virtual GPUOutputBuffer *GetOutputBuffer() override final;
    virtual void SetOutputBuffer(GPUOutputBuffer *pSwapChain) override final;

    // RT Changing
    virtual uint32 GetRenderTargets(uint32 nRenderTargets, GPURenderTargetView **ppRenderTargetViews, GPUDepthStencilBufferView **ppDepthBufferView) override final;
    virtual void SetRenderTargets(uint32 nRenderTargets, GPURenderTargetView **ppRenderTargets, GPUDepthStencilBufferView *pDepthBufferView) override final;

    // Drawing Setup
    virtual DRAW_TOPOLOGY GetDrawTopology() override final;
    virtual void SetDrawTopology(DRAW_TOPOLOGY topology) override final;

    // Vertex Buffer Setup
    virtual uint32 GetVertexBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer **ppVertexBuffers, uint32 *pVertexBufferOffsets, uint32 *pVertexBufferStrides) override final;
    virtual void SetVertexBuffer(uint32 bufferIndex, GPUBuffer *pVertexBuffer, uint32 offset, uint32 stride) override final;
    virtual void SetVertexBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer *const *ppVertexBuffers, const uint32 *pVertexBufferOffsets, const uint32 *pVertexBufferStrides) override final;
    virtual void GetIndexBuffer(GPUBuffer **ppBuffer, GPU_INDEX_FORMAT *pFormat, uint32 *pOffset) override final;
    virtual void SetIndexBuffer(GPUBuffer *pBuffer, GPU_INDEX_FORMAT format, uint32 offset) override final;

    // Shader Setup
    virtual void SetShaderProgram(GPUShaderProgram *pShaderProgram) override final;
    virtual void SetShaderParameterValue(uint32 index, SHADER_PARAMETER_TYPE valueType, const void *pValue) override final;
    virtual void SetShaderParameterValueArray(uint32 index, SHADER_PARAMETER_TYPE valueType, const void *pValue, uint32 firstElement, uint32 numElements) override final;
    virtual void SetShaderParameterStruct(uint32 index, const void *pValue, uint32 valueSize) override final;
    virtual void SetShaderParameterStructArray(uint32 index, const void *pValue, uint32 valueSize, uint32 firstElement, uint32 numElements) override final;
    virtual void SetShaderParameterResource(uint32 index, GPUResource *pResource) override final;
    virtual void SetShaderParameterTexture(uint32 index, GPUTexture *pTexture, GPUSamplerState *pSamplerState) override final;

    // constant buffer management
    virtual void WriteConstantBuffer(uint32 bufferIndex, uint32 fieldIndex, uint32 offset, uint32 count, const void *pData, bool commit = false) override final;
    virtual void WriteConstantBufferStrided(uint32 bufferIndex, uint32 fieldIndex, uint32 offset, uint32 bufferStride, uint32 copySize, uint32 count, const void *pData, bool commit = false) override final;
    virtual void CommitConstantBuffer(uint32 bufferIndex) override final;

    // Draw calls
    virtual void Draw(uint32 firstVertex, uint32 nVertices) override final;
    virtual void DrawInstanced(uint32 firstVertex, uint32 nVertices, uint32 nInstances) override final;
    virtual void DrawIndexed(uint32 startIndex, uint32 nIndices, uint32 baseVertex) override final;
    virtual void DrawIndexedInstanced(uint32 startIndex, uint32 nIndices, uint32 baseVertex, uint32 nInstances) override final;

    // Draw calls with user-space buffer
    virtual void DrawUserPointer(const void *pVertices, uint32 vertexSize, uint32 nVertices) override final;

    // Compute shaders
    virtual void Dispatch(uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ) override final;

    // --- our methods ---

    // accessors
    bool IsOpen() const { return m_open; }
    ID3D12Device *GetD3DDevice() const { return m_pD3DDevice; }
    ID3D12GraphicsCommandList *GetD3DCommandList() const { return m_pCommandList; }
    D3D12GPUShaderProgram *GetD3D12ShaderProgram() const { return m_pCurrentShaderProgram; }

    // create device
    bool Create(D3D12CommandQueue *pCommandQueue);

    // context interface
    bool Open(D3D12CommandQueue *pCommandQueue, D3D12GPUOutputBuffer *pOutputBuffer);
    bool Close();

    // release allocators back to command queue
    void ReleaseAllocators(uint64 fenceValue);

    // access the gpu virtual address for a constant buffer
    bool GetPerDrawConstantBufferGPUAddress(uint32 index, D3D12_GPU_VIRTUAL_ADDRESS *pAddress);

    // access to shader states for the shader mutators to modify
    void SetShaderConstantBuffers(SHADER_PROGRAM_STAGE stage, uint32 index, const D3D12DescriptorHandle &handle);
    void SetShaderResources(SHADER_PROGRAM_STAGE stage, uint32 index, const D3D12DescriptorHandle &handle);
    void SetShaderSamplers(SHADER_PROGRAM_STAGE stage, uint32 index, const D3D12DescriptorHandle &handle);
    void SetShaderUAVs(SHADER_PROGRAM_STAGE stage, uint32 index, const D3D12DescriptorHandle &handle);
    void SetPerDrawConstantBuffer(uint32 index, D3D12_GPU_VIRTUAL_ADDRESS address);

    // synchronize the states with the d3d context
    void SynchronizeRenderTargetsAndUAVs();

    // temporarily disable predication to force a call to go through
    void BypassPredication();
    void RestorePredication();

    // create a resource barrier on the current command list
    void ResourceBarrier(ID3D12Resource *pResource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
    void ResourceBarrier(ID3D12Resource *pResource, uint32 subResource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

    // allocate from scratch buffer
    bool AllocateScratchBufferMemory(uint32 size, uint32 alignment, ID3D12Resource **ppScratchBufferResource, uint32 *pScratchBufferOffset, void **ppCPUPointer, D3D12_GPU_VIRTUAL_ADDRESS *pGPUAddress);
    bool AllocateScratchView(uint32 count, D3D12_CPU_DESCRIPTOR_HANDLE *pOutCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE *pOutGPUHandle);
    bool AllocateScratchSamplers(uint32 count, D3D12_CPU_DESCRIPTOR_HANDLE *pOutCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE *pOutGPUHandle);

private:
    // preallocate constant buffers
    void CreateConstantBuffers();
    void UpdateShaderDescriptorHeaps();
    void RestoreCommandListDependantState();
    bool UpdatePipelineState(bool force);
    void GetCurrentRenderTargetDimensions(uint32 *width, uint32 *height);
    D3D12_RESOURCE_STATES GetCurrentResourceState(GPUResource *pResource);
    bool IsBoundAsRenderTarget(GPUTexture *pTexture);
    bool IsBoundAsDepthBuffer(GPUTexture *pTexture);
    bool IsBoundAsUnorderedAccess(GPUResource *pResource);
    void UpdateScissorRect();

    D3D12GPUDevice *m_pDevice;
    ID3D12Device *m_pD3DDevice;
    D3D12CommandQueue *m_pGraphicsCommandQueue;

    GPUContextConstants *m_pConstants;

    // Created once
    ID3D12GraphicsCommandList *m_pCommandList;

    // Current allocators
    ID3D12CommandAllocator *m_pCurrentCommandAllocator;
    D3D12LinearBufferHeap *m_pCurrentScratchBuffer;
    D3D12LinearDescriptorHeap *m_pCurrentScratchViewHeap;
    D3D12LinearDescriptorHeap *m_pCurrentScratchSamplerHeap;

    // Unfortunately, as a command list, we can't release the allocators etc back to the command queue, since it may flush 
    // on the main thread, completing a fence, whilst our list is still using it. So we maintain a release list.
    PODArray<D3D12LinearBufferHeap *> m_scratchBufferReleaseList;
    PODArray<D3D12LinearDescriptorHeap *> m_scratchViewHeapReleaseList;
    PODArray<D3D12LinearDescriptorHeap *> m_scratchSamplerReleaseList;

    // state
    bool m_open;
    RENDERER_VIEWPORT m_currentViewport;
    RENDERER_SCISSOR_RECT m_scissorRect;
    DRAW_TOPOLOGY m_currentTopology;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE m_currentD3DTopologyType;

    D3D12GPUBuffer *m_pCurrentVertexBuffers[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    uint32 m_currentVertexBufferOffsets[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    uint32 m_currentVertexBufferStrides[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    uint32 m_currentVertexBufferBindCount;

    D3D12GPUBuffer *m_pCurrentIndexBuffer;
    GPU_INDEX_FORMAT m_currentIndexFormat;
    uint32 m_currentIndexBufferOffset;

    D3D12GPUShaderProgram *m_pCurrentShaderProgram;

    // constant buffers
    struct ConstantBuffer
    {
        byte *pLocalMemory;
        uint32 Size;
        int32 DirtyLowerBounds;
        int32 DirtyUpperBounds;
        D3D12_GPU_VIRTUAL_ADDRESS LastAddress;
        bool PerDraw;
    };
    MemArray<ConstantBuffer> m_constantBuffers;

    // shader stage state
    struct ShaderStageState
    {
        bool ConstantBuffersDirty;
        bool ResourcesDirty;
        bool SamplersDirty;
        bool UAVsDirty;

        D3D12_CPU_DESCRIPTOR_HANDLE CBVTableCPUHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE SRVTableCPUHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE SamplerTableCPUHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE UAVTableCPUHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE CBVTableGPUHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE SRVTableGPUHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE SamplerTableGPUHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE UAVTableGPUHandle;

        D3D12DescriptorHandle ConstantBuffers[D3D12_LEGACY_GRAPHICS_ROOT_CONSTANT_BUFFER_SLOTS];
        uint32 ConstantBufferBindCount;

        D3D12DescriptorHandle Resources[D3D12_LEGACY_GRAPHICS_ROOT_SHADER_RESOURCE_SLOTS];
        uint32 ResourceBindCount;

        D3D12DescriptorHandle Samplers[D3D12_LEGACY_GRAPHICS_ROOT_SHADER_SAMPLER_SLOTS];
        uint32 SamplerBindCount;

        // @TODO move, since it's shared between stages
        D3D12DescriptorHandle UAVs[D3D12_PS_CS_UAV_REGISTER_COUNT];
        uint32 UAVBindCount;
    };
    MemArray<ShaderStageState> m_shaderStates;

    MemArray<D3D12_GPU_VIRTUAL_ADDRESS> m_perDrawConstantBuffers;
    uint32 m_perDrawConstantBufferCount;
    bool m_perDrawConstantBuffersDirty;

    D3D12GPURasterizerState *m_pCurrentRasterizerState;
    D3D12GPUDepthStencilState *m_pCurrentDepthStencilState;
    uint8 m_currentDepthStencilRef;
    D3D12GPUBlendState *m_pCurrentBlendState;
    float4 m_currentBlendStateBlendFactors;
    bool m_pipelineChanged;

    D3D12GPUOutputBuffer *m_pCurrentSwapChain;

    D3D12GPURenderTargetView *m_pCurrentRenderTargetViews[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
    D3D12GPUDepthStencilBufferView *m_pCurrentDepthBufferView;
    uint32 m_nCurrentRenderTargets;

    // predication
    GPUQuery *m_pCurrentPredicate;
    //ID3D12Predicate *m_pCurrentPredicateD3D;
    uint32 m_predicateBypassCount;
};



