#pragma once
#include "D3D11Renderer/D3D11Common.h"

class D3D11RasterizerState;
class D3D11DepthStencilState;
class D3D11BlendState;
class D3D11GPURenderTargetView;
class D3D11GPUDepthStencilBufferView;
class D3D11GPUComputeView;
class D3D11GPUInputLayout;
class D3D11GPUBuffer;
class D3D11GPUShaderProgram;

class D3D11GPUContext : public GPUContext
{
public:
    D3D11GPUContext(D3D11GPUDevice *pDevice, ID3D11Device *pD3DDevice, ID3D11Device1 *pD3DDevice1, ID3D11DeviceContext *pImmediateContext);
    ~D3D11GPUContext();

    // State clearing
    virtual void ClearState(bool clearShaders = true, bool clearBuffers = true, bool clearStates = true, bool clearRenderTargets = true) override final;

    // Retrieve RendererVariables interface.
    virtual GPUContextConstants *GetConstants() override final { return m_pConstants; }

    // State Management (D3D11RenderSystem.cpp)
    virtual GPURasterizerState *GetRasterizerState() override final;
    virtual void SetRasterizerState(GPURasterizerState *pRasterizerState) override final;
    virtual GPUDepthStencilState *GetDepthStencilState() override final;
    virtual uint8 GetDepthStencilStateStencilRef() override final;
    virtual void SetDepthStencilState(GPUDepthStencilState *pDepthStencilState, uint8 stencilRef) override final;
    virtual GPUBlendState *GetBlendState() override final;
    virtual const float4 &GetBlendStateBlendFactor() override final;
    virtual void SetBlendState(GPUBlendState *pBlendState, const float4 &blendFactor = float4::One) override final;

    // Viewport Management (D3D11RenderSystem.cpp)
    virtual const RENDERER_VIEWPORT *GetViewport() override final;
    virtual void SetViewport(const RENDERER_VIEWPORT *pNewViewport) override final;
    virtual void SetFullViewport(GPUTexture *pForRenderTarget = NULL) override final;

    // Scissor Rect Management (D3D11RenderSystem.cpp)
    virtual const RENDERER_SCISSOR_RECT *GetScissorRect() override final;
    virtual void SetScissorRect(const RENDERER_SCISSOR_RECT *pScissorRect) override final;

    // Buffer mapping/reading/writing
    virtual bool ReadBuffer(GPUBuffer *pBuffer, void *pDestination, uint32 start, uint32 count) override final;
    virtual bool WriteBuffer(GPUBuffer *pBuffer, const void *pSource, uint32 start, uint32 count) override final;
    virtual bool MapBuffer(GPUBuffer *pBuffer, GPU_MAP_TYPE mapType, void **ppPointer) override final;
    virtual void Unmapbuffer(GPUBuffer *pBuffer, void *pPointer) override final;

    // Texture reading/writing
    virtual bool ReadTexture(GPUTexture1D *pTexture, void *pDestination, uint32 cbDestination, uint32 mipIndex, uint32 start, uint32 count) override final;
    virtual bool ReadTexture(GPUTexture1DArray *pTexture, void *pDestination, uint32 cbDestination, uint32 arrayIndex, uint32 mipIndex, uint32 start, uint32 count) override final;
    virtual bool ReadTexture(GPUTexture2D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool ReadTexture(GPUTexture2DArray *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 arrayIndex, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool ReadTexture(GPUTexture3D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 destinationSlicePitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 startZ, uint32 countX, uint32 countY, uint32 countZ) override final;
    virtual bool ReadTexture(GPUTextureCube *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool ReadTexture(GPUTextureCubeArray *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 arrayIndex, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool ReadTexture(GPUDepthTexture *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool WriteTexture(GPUTexture1D *pTexture, const void *pSource, uint32 cbSource, uint32 mipIndex, uint32 start, uint32 count) override final;
    virtual bool WriteTexture(GPUTexture1DArray *pTexture, const void *pSource, uint32 cbSource, uint32 arrayIndex, uint32 mipIndex, uint32 start, uint32 count) override final;
    virtual bool WriteTexture(GPUTexture2D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool WriteTexture(GPUTexture2DArray *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 arrayIndex, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool WriteTexture(GPUTexture3D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 sourceSlicePitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 startZ, uint32 countX, uint32 countY, uint32 countZ) override final;
    virtual bool WriteTexture(GPUTextureCube *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool WriteTexture(GPUTextureCubeArray *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 arrayIndex, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool WriteTexture(GPUDepthTexture *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;

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
    virtual GPU_QUERY_GETDATA_RESULT GetQueryData(GPUQuery *pQuery, void *pData, uint32 cbData, uint32 flags) override final;

    // Predicated drawing
    virtual void SetPredication(GPUQuery *pQuery) override final;

    // RT Clearing
    virtual void ClearTargets(bool clearColor = true, bool clearDepth = true, bool clearStencil = true, const float4 &clearColorValue = float4::Zero, float clearDepthValue = 1.0f, uint8 clearStencilValue = 0) override final;
    virtual void DiscardTargets(bool discardColor = true, bool discardDepth = true, bool discardStencil = true) override final;

    // Swap chain
    virtual GPUOutputBuffer *GetOutputBuffer() override final;
    virtual void SetOutputBuffer(GPUOutputBuffer *pSwapChain) override final;
    virtual bool GetExclusiveFullScreen() override final;
    virtual bool SetExclusiveFullScreen(bool enabled, uint32 width, uint32 height, uint32 refreshRate) override final;
    virtual bool ResizeOutputBuffer(uint32 width = 0, uint32 height = 0) override final;
    virtual void PresentOutputBuffer(GPU_PRESENT_BEHAVIOUR presentBehaviour) override final;

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
    ID3D11DeviceContext *GetD3DContext() const { return m_pD3DContext; }
    ID3D11DeviceContext1 *GetD3DContext1() const { return m_pD3DContext1; }
    D3D11GPUShaderProgram *GetD3D11ShaderProgram() const { return m_pCurrentShaderProgram; }

    // create device
    bool Create();

    // constant resource management
    D3D11GPUBuffer *GetConstantBuffer(uint32 index);

    // access to shader states for the shader mutators to modify
    void SetShaderConstantBuffers(SHADER_PROGRAM_STAGE stage, uint32 index, ID3D11Buffer *pBuffer);
    void SetShaderResources(SHADER_PROGRAM_STAGE stage, uint32 index, ID3D11ShaderResourceView *pResource);
    void SetShaderSamplers(SHADER_PROGRAM_STAGE stage, uint32 index, ID3D11SamplerState *pSampler);
    void SetShaderUAVs(SHADER_PROGRAM_STAGE stage, uint32 index, ID3D11UnorderedAccessView *pResource);

    // synchronize the states with the d3d context
    void SynchronizeRenderTargetsAndUAVs();
    void SynchronizeShaderStates();

    // temporarily disable predication to force a call to go through
    void BypassPredication();
    void RestorePredication();

private:
    // preallocate constant buffers
    bool CreateConstantBuffers();

    D3D11GPUDevice *m_pDevice;
    ID3D11Device *m_pD3DDevice;
    ID3D11Device1 *m_pD3DDevice1;
    ID3D11DeviceContext *m_pD3DContext;
    ID3D11DeviceContext1 *m_pD3DContext1;

    GPUContextConstants *m_pConstants;

    RENDERER_VIEWPORT m_currentViewport;
    RENDERER_SCISSOR_RECT m_scissorRect;
    DRAW_TOPOLOGY m_currentTopology;

    D3D11GPUBuffer *m_pCurrentVertexBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    uint32 m_currentVertexBufferOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    uint32 m_currentVertexBufferStrides[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    uint32 m_currentVertexBufferBindCount;

    D3D11GPUBuffer *m_pCurrentIndexBuffer;
    GPU_INDEX_FORMAT m_currentIndexFormat;
    uint32 m_currentIndexBufferOffset;

    D3D11GPUShaderProgram *m_pCurrentShaderProgram;

    // constant buffers
    struct ConstantBuffer
    {
        D3D11GPUBuffer *pGPUBuffer;
        uint32 Size;

        byte *pLocalMemory;
        int32 DirtyLowerBounds;
        int32 DirtyUpperBounds;
    };
    MemArray<ConstantBuffer> m_constantBuffers;

    // shader stage state
    // we can cheat this in d3d by storing pointers to the d3d objects themselves,
    // since they're reference counted by the runtime. even if our wrapper object is
    // deleted by release, the runtime will hold its own reference.
    struct ShaderStageState
    {
        ID3D11Buffer *ConstantBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
        uint32 ConstantBufferBindCount;
        int32 ConstantBufferDirtyLowerBounds;
        int32 ConstantBufferDirtyUpperBounds;

        ID3D11ShaderResourceView *Resources[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
        uint32 ResourceBindCount;
        int32 ResourceDirtyLowerBounds;
        int32 ResourceDirtyUpperBounds;

        ID3D11SamplerState *Samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
        uint32 SamplerBindCount;
        int32 SamplerDirtyLowerBounds;
        int32 SamplerDirtyUpperBounds;

        ID3D11UnorderedAccessView *UAVs[D3D11_PS_CS_UAV_REGISTER_COUNT];
        uint32 UAVBindCount;
        int32 UAVDirtyLowerBounds;
        int32 UAVDirtyUpperBounds;
    };
    ShaderStageState m_shaderStates[SHADER_PROGRAM_STAGE_COUNT];

    D3D11RasterizerState *m_pCurrentRasterizerState;
    D3D11DepthStencilState *m_pCurrentDepthStencilState;
    uint8 m_currentDepthStencilRef;
    D3D11BlendState *m_pCurrentBlendState;
    float4 m_currentBlendStateBlendFactors;

    D3D11GPUOutputBuffer *m_pCurrentSwapChain;

    D3D11GPURenderTargetView *m_pCurrentRenderTargetViews[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    D3D11GPUDepthStencilBufferView *m_pCurrentDepthBufferView;
    uint32 m_nCurrentRenderTargets;

    // predication
    GPUQuery *m_pCurrentPredicate;
    ID3D11Predicate *m_pCurrentPredicateD3D;
    uint32 m_predicateBypassCount;

    ID3D11Buffer *m_pUserVertexBuffer;
    uint32 m_userVertexBufferSize;
    uint32 m_userVertexBufferPosition;
};



