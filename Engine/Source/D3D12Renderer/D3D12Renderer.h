#pragma once
#include "D3D12Renderer/D3D12Common.h"
//#include "D3D12Renderer/D3D12GPUContext.h"
#include "D3D12Renderer/D3D12RendererOutputBuffer.h"

class D3D12SamplerState : public GPUSamplerState
{
public:
    D3D12SamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const D3D12_SAMPLER_DESC *pD3DSamplerDesc);
    virtual ~D3D12SamplerState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const D3D12_SAMPLER_DESC *GetD3DSamplerStateDesc() { return &m_D3DSamplerStateDesc; }

private:
    D3D12_SAMPLER_DESC m_D3DSamplerStateDesc;
};

class D3D12RasterizerState : public GPURasterizerState
{
public:
    D3D12RasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc, const D3D12_RASTERIZER_DESC *pD3DRasterizerDesc);
    virtual ~D3D12RasterizerState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const D3D12_RASTERIZER_DESC *GetD3DRasterizerStateDesc() { return &m_D3DRasterizerDesc; }

private:
    D3D12_RASTERIZER_DESC m_D3DRasterizerDesc;
};

class D3D12DepthStencilState : public GPUDepthStencilState
{
public:
    D3D12DepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc, const D3D12_DEPTH_STENCIL_DESC *pD3DDepthStencilDesc);
    virtual ~D3D12DepthStencilState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const D3D12_DEPTH_STENCIL_DESC *GetD3DDepthStencilDesc() { return &m_D3DDepthStencilDesc; }

private:
    D3D12_DEPTH_STENCIL_DESC m_D3DDepthStencilDesc;
};

class D3D12BlendState : public GPUBlendState
{
public:
    D3D12BlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc, const D3D12_BLEND_DESC *pD3DBlendDesc);
    virtual ~D3D12BlendState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const D3D12_BLEND_DESC *GetD3DBlendDesc() { return &m_D3DBlendDesc; }

private:
    D3D12_BLEND_DESC m_D3DBlendDesc;
};

class D3D12DescriptorHeap
{
public:
    struct Handle
    {
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
        uint32 StartIndex;
        uint32 DescriptorCount;
    };

public:
    ~D3D12DescriptorHeap();

    static D3D12DescriptorHeap *Create(ID3D12Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorCount);

    bool Allocate(Handle *handle);
    bool AllocateRange(uint32 count, Handle *handle);
    void Free(const Handle *handle);

private:
    D3D12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorCount, ID3D12DescriptorHeap *pD3DDescriptorHeap, uint32 incrementSize);

    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
    uint32 m_descriptorCount;

    ID3D12DescriptorHeap *m_pD3DDescriptorHeap;
    BitSet32 m_allocationMap;
    uint32 m_incrementSize;
};

class D3D12Renderer : public Renderer
{
public:
    D3D12Renderer();
    virtual ~D3D12Renderer();

    // --- our methods ---
    static inline D3D12Renderer *Get() { return static_cast<D3D12Renderer *>(g_pRenderer); }
    IDXGIFactory3 *GetDXGIFactory() const { return m_pDXGIFactory; }
    IDXGIAdapter3 *GetDXGIAdapter() const { return m_pDXGIAdapter; }
    ID3D12Device *GetD3DDevice() const { return m_pD3DDevice; }
    D3D12GPUContext *GetD3D11MainContext() const { return m_pMainContext; }
    void OnResourceCreated(GPUResource *pResource);
    void OnResourceReleased(GPUResource *pResource);
    D3D12DescriptorHeap *GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) { return m_pDescriptorHeaps[type]; }

    // creation
    bool Create(const RendererInitializationParameters *pInitializationParameters);

    // --- virtual methods ---
    virtual void CorrectProjectionMatrix(float4x4 &projectionMatrix) const override;
    virtual bool CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat = NULL) const override;

    // Creates a context capable of uploading resources owned by the calling thread.
    virtual GPUContext *CreateUploadContext() override;

    // Creates a swap chain on an existing window.
    virtual GPUOutputBuffer *CreateOutputBuffer(RenderSystemWindowHandle hWnd, RENDERER_VSYNC_TYPE vsyncType) override;

    // Creates a swap chain on a new window.
    virtual RendererOutputWindow *CreateOutputWindow(const char *windowTitle, uint32 windowWidth, uint32 windowHeight, RENDERER_VSYNC_TYPE vsyncType) override;

    // Resource creation
    virtual GPUQuery *CreateQuery(GPU_QUERY_TYPE type) override;
    virtual GPUBuffer *CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData = NULL) override;
    virtual GPUTexture1D *CreateTexture1D(const GPU_TEXTURE1D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTexture1DArray *CreateTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTexture2D *CreateTexture2D(const GPU_TEXTURE2D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTexture2DArray *CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTexture3D *CreateTexture3D(const GPU_TEXTURE3D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL, const uint32 *pInitialDataSlicePitch = NULL) override;
    virtual GPUTextureCube *CreateTextureCube(const GPU_TEXTURECUBE_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTextureCubeArray *CreateTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUDepthTexture *CreateDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pTextureDesc) override;
    virtual GPUSamplerState *CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc) override;
    virtual GPURenderTargetView *CreateRenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc) override;
    virtual GPUDepthStencilBufferView *CreateDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc) override;
    virtual GPUComputeView *CreateComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc) override;
    virtual GPUDepthStencilState *CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc) override;
    virtual GPURasterizerState *CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc) override;
    virtual GPUBlendState *CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc) override;
    virtual GPUShaderProgram *CreateGraphicsProgram(const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream) override;
    virtual GPUShaderProgram *CreateComputeProgram(ByteStream *pByteCodeStream) override;

    virtual RendererOutputWindow *GetImplicitOutputWindow() override { return static_cast<RendererOutputWindow *>(m_pImplicitRenderWindow); }

    virtual GPUContext *GetGPUContext() const override { /*return static_cast<GPUContext *>(m_pMainContext);*/return nullptr; }

    virtual void HandlePendingSDLEvents() override;

    virtual void GetGPUMemoryUsage(GPUMemoryUsage *pMemoryUsage) const override;

private:
    IDXGIFactory3 *m_pDXGIFactory;
    IDXGIAdapter3 *m_pDXGIAdapter;

    ID3D12Device *m_pD3DDevice;
    D3D_FEATURE_LEVEL m_eD3DFeatureLevel;

    DXGI_FORMAT m_swapChainBackBufferFormat;
    DXGI_FORMAT m_swapChainDepthStencilBufferFormat;

    D3D12GPUContext *m_pMainContext;

    D3D12RendererOutputWindow *m_pImplicitRenderWindow;

    GPUMemoryUsage m_gpuMemoryUsage;

    // create descriptor heaps
    bool CreateDescriptorHeaps();

    // descriptor heaps
    D3D12DescriptorHeap *m_pDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
};
