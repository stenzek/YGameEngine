#pragma once
#include "D3D12Renderer/D3D12Common.h"

class D3D12GPUSamplerState : public GPUSamplerState
{
public:
    D3D12GPUSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const D3D12_SAMPLER_DESC *pD3DSamplerDesc);
    virtual ~D3D12GPUSamplerState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const D3D12_SAMPLER_DESC *GetD3DSamplerStateDesc() { return &m_D3DSamplerStateDesc; }

private:
    D3D12_SAMPLER_DESC m_D3DSamplerStateDesc;
};

class D3D12GPURasterizerState : public GPURasterizerState
{
public:
    D3D12GPURasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc, const D3D12_RASTERIZER_DESC *pD3DRasterizerDesc);
    virtual ~D3D12GPURasterizerState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const D3D12_RASTERIZER_DESC *GetD3DRasterizerStateDesc() { return &m_D3DRasterizerDesc; }

private:
    D3D12_RASTERIZER_DESC m_D3DRasterizerDesc;
};

class D3D12GPUDepthStencilState : public GPUDepthStencilState
{
public:
    D3D12GPUDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc, const D3D12_DEPTH_STENCIL_DESC *pD3DDepthStencilDesc);
    virtual ~D3D12GPUDepthStencilState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const D3D12_DEPTH_STENCIL_DESC *GetD3DDepthStencilDesc() { return &m_D3DDepthStencilDesc; }

private:
    D3D12_DEPTH_STENCIL_DESC m_D3DDepthStencilDesc;
};

class D3D12GPUBlendState : public GPUBlendState
{
public:
    D3D12GPUBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc, const D3D12_BLEND_DESC *pD3DBlendDesc);
    virtual ~D3D12GPUBlendState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const D3D12_BLEND_DESC *GetD3DBlendDesc() { return &m_D3DBlendDesc; }

private:
    D3D12_BLEND_DESC m_D3DBlendDesc;
};

class D3D12GPUDevice : public GPUDevice
{
public:
    D3D12GPUDevice(D3D12RenderBackend *pBackend, IDXGIFactory3 *pDXGIFactory, IDXGIAdapter3 *pDXGIAdapter, ID3D12Device *pD3DDevice, DXGI_FORMAT outputBackBufferFormat, DXGI_FORMAT outputDepthStencilFormat);
    virtual ~D3D12GPUDevice();

    // private methods
    IDXGIFactory3 *GetDXGIFactory() const { return m_pDXGIFactory; }
    IDXGIAdapter3 *GetDXGIAdapter() const { return m_pDXGIAdapter; }
    ID3D12Device *GetD3DDevice() const { return m_pD3DDevice; }
    D3D12RenderBackend *GetBackend() const { return m_pBackend; }

    // Creates a swap chain on an existing window.
    virtual GPUOutputBuffer *CreateOutputBuffer(RenderSystemWindowHandle hWnd, RENDERER_VSYNC_TYPE vsyncType) override final;
    virtual GPUOutputBuffer *CreateOutputBuffer(SDL_Window *pSDLWindow, RENDERER_VSYNC_TYPE vsyncType) override final;

    // Resource creation
    virtual GPUQuery *CreateQuery(GPU_QUERY_TYPE type) override final;
    virtual GPUBuffer *CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData = nullptr) override final;
    virtual GPUTexture1D *CreateTexture1D(const GPU_TEXTURE1D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = nullptr, const uint32 *pInitialDataPitch = nullptr) override final;
    virtual GPUTexture1DArray *CreateTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = nullptr, const uint32 *pInitialDataPitch = nullptr) override final;
    virtual GPUTexture2D *CreateTexture2D(const GPU_TEXTURE2D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = nullptr, const uint32 *pInitialDataPitch = nullptr) override final;
    virtual GPUTexture2DArray *CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = nullptr, const uint32 *pInitialDataPitch = nullptr) override final;
    virtual GPUTexture3D *CreateTexture3D(const GPU_TEXTURE3D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = nullptr, const uint32 *pInitialDataPitch = nullptr, const uint32 *pInitialDataSlicePitch = nullptr) override final;
    virtual GPUTextureCube *CreateTextureCube(const GPU_TEXTURECUBE_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = nullptr, const uint32 *pInitialDataPitch = nullptr) override final;
    virtual GPUTextureCubeArray *CreateTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = nullptr, const uint32 *pInitialDataPitch = nullptr) override final;
    virtual GPUDepthTexture *CreateDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pTextureDesc) override final;
    virtual GPUSamplerState *CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc) override final;
    virtual GPURenderTargetView *CreateRenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc) override final;
    virtual GPUDepthStencilBufferView *CreateDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc) override final;
    virtual GPUComputeView *CreateComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc) override final;
    virtual GPUDepthStencilState *CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc) override final;
    virtual GPURasterizerState *CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc) override final;
    virtual GPUBlendState *CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc) override final;
    virtual GPUShaderProgram *CreateGraphicsProgram(const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream) override final;
    virtual GPUShaderProgram *CreateComputeProgram(ByteStream *pByteCodeStream) override final;

    // off-thread resource creation
    virtual void BeginResourceBatchUpload() override final;
    virtual void EndResourceBatchUpload() override final;

    // helper methods
    D3D12GPUContext *GetGPUContext() const { return m_pGPUContext; }
    void SetGPUContext(D3D12GPUContext *pContext) { m_pGPUContext = pContext; }
    ID3D12GraphicsCommandList *GetCommandList();
    bool CreateOffThreadResources();
    void FlushCopyQueue();

private:
    D3D12RenderBackend *m_pBackend;
    IDXGIFactory3 *m_pDXGIFactory;
    IDXGIAdapter3 *m_pDXGIAdapter;
    ID3D12Device *m_pD3DDevice;
    D3D12GPUContext *m_pGPUContext;

    ID3D12CommandAllocator *m_pOffThreadCommandAllocator;
    ID3D12CommandQueue *m_pOffThreadCommandQueue;
    ID3D12GraphicsCommandList *m_pOffThreadCommandList;
    ID3D12Fence *m_pOffThreadFence;
    HANDLE m_hFenceReachedEvent;
    uint64 m_fenceValue;

    DXGI_FORMAT m_outputBackBufferFormat;
    DXGI_FORMAT m_outputDepthStencilFormat;

    uint32 m_copyQueueLength;
    uint32 m_copyQueueEnabled;
};
