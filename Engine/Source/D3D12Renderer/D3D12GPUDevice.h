#pragma once
#include "D3D12Renderer/D3D12Common.h"
#include "D3D12Renderer/D3D12DescriptorHeap.h"
#include "D3D12Renderer/D3D12CommandQueue.h"

class D3D12GPUSamplerState : public GPUSamplerState
{
public:
    D3D12GPUSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, D3D12GPUDevice *pDevice, const D3D12DescriptorHandle &samplerHandle);
    virtual ~D3D12GPUSamplerState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const D3D12DescriptorHandle &GetSamplerHandle() { return m_samplerHandle; }

private:
    D3D12GPUDevice *m_pDevice;
    D3D12DescriptorHandle m_samplerHandle;
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
    // shared command queue structure
    struct CopyCommandQueue
    {
        CopyCommandQueue()
            : pCommandQueue(nullptr), pCommandAllocator(nullptr), pCommandList(nullptr) 
        {
        }

        ~CopyCommandQueue()
        { 
            if (pCommandList != nullptr)
                pCommandQueue->ReleaseCommandList(pCommandList);
            if (pCommandAllocator != nullptr)
                pCommandQueue->ReleaseCommandAllocator(pCommandAllocator);
            delete pCommandQueue;
        }

        D3D12CommandQueue *pCommandQueue;
        ID3D12CommandAllocator *pCommandAllocator;
        ID3D12GraphicsCommandList *pCommandList;
        Mutex Lock;
    };
    typedef Array<CopyCommandQueue> CopyCommandQueueArray;

    // copy queue reference
    struct CopyQueueReference
    {
        CopyQueueReference(D3D12GPUDevice *pDevice);
        ~CopyQueueReference();
        bool HasContext() const;

        D3D12GPUDevice *pDevice;
        D3D12CommandQueue *pQueue;
        ID3D12GraphicsCommandList *pCommandList;
        bool ContextNeedsRelease;
    };

public:
    D3D12GPUDevice(HMODULE hD3D12Module, IDXGIFactory4 *pDXGIFactory, IDXGIAdapter3 *pDXGIAdapter, ID3D12Device *pD3DDevice,
                   D3D_FEATURE_LEVEL D3DFeatureLevel, RENDERER_FEATURE_LEVEL featureLevel, TEXTURE_PLATFORM texturePlatform,
                   PIXEL_FORMAT outputBackBufferFormat, PIXEL_FORMAT outputDepthStencilFormat, uint32 frameLatency);

    virtual ~D3D12GPUDevice();

    // Accessors.
    virtual RENDERER_PLATFORM GetPlatform() const override final;
    virtual RENDERER_FEATURE_LEVEL GetFeatureLevel() const override final;
    virtual TEXTURE_PLATFORM GetTexturePlatform() const override final;
    virtual void GetCapabilities(RendererCapabilities *pCapabilities) const override final;
    virtual bool CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat = nullptr) const override final;

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

    // private methods
    IDXGIFactory3 *GetDXGIFactory() const { return m_pDXGIFactory; }
    IDXGIAdapter3 *GetDXGIAdapter() const { return m_pDXGIAdapter; }
    ID3D12Device *GetD3DDevice() const { return m_pD3DDevice; }
    uint32 GetFrameLatency() const { return m_frameLatency; }

    // creation interface
    D3D12GPUContext *InitializeAndCreateContext();

    // legacy root signatures
    ID3D12RootSignature *GetLegacyGraphicsRootSignature() const { return m_pLegacyGraphicsRootSignature; }
    ID3D12RootSignature *GetLegacyComputeRootSignature() const { return m_pLegacyComputeRootSignature; }

    // cpu descriptor heaps
    D3D12DescriptorHeap *GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) { return m_pCPUDescriptorHeaps[type]; }
    const D3D12DescriptorHandle GetNullCBVDescriptorHandle() { return m_nullCBVDescriptorHandle; }
    const D3D12DescriptorHandle GetNullSRVDescriptorHandle() { return m_nullSRVDescriptorHandle; }
    const D3D12DescriptorHandle GetNullSamplerHandle() { return m_nullSamplerHandle; }

    // constant buffer management
    ID3D12Resource *GetConstantBufferResource(uint32 index);
    const D3D12DescriptorHandle *GetConstantBufferDescriptor(uint32 index) const;

    // create a resource barrier on the current command list. NOTE: doesn't flush the copy command list, assumes batching
    void ResourceBarrier(ID3D12Resource *pResource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

    // resource freeing - done on graphics queue
    void ScheduleResourceForDeletion(ID3D12Pageable *pResource);
    void ScheduleDescriptorForDeletion(const D3D12DescriptorHandle &handle);

    // copy queue accessor
    void SetCopyCommandList(ID3D12GraphicsCommandList *pCommandList);
    void ScheduleCopyResourceForDeletion(ID3D12Pageable *pResource);
    void GetFreeCopyCommandQueue();
    void ReleaseCopyCommandQueue();

    // clean up after resources are created off-thread
    void TransitionPendingResources(D3D12CommandQueue *pCommandQueue);
    
private:
    bool CreateCommandQueues(uint32 copyCommandQueueCount);
    bool CreateLegacyRootSignatures();
    bool CreateCPUDescriptorHeaps();
    bool CreateConstantStorage();
    ID3D12GraphicsCommandList *GetCurrentCopyCommandList();

    HMODULE m_hD3D12Module;
    IDXGIFactory4 *m_pDXGIFactory;
    IDXGIAdapter3 *m_pDXGIAdapter;
    
    ID3D12Device *m_pD3DDevice;
    ID3D12InfoQueue *m_pD3DInfoQueue;

    D3D_FEATURE_LEVEL m_D3DFeatureLevel;
    RENDERER_FEATURE_LEVEL m_featureLevel;
    TEXTURE_PLATFORM m_texturePlatform;

    PIXEL_FORMAT m_outputBackBufferFormat;
    PIXEL_FORMAT m_outputDepthStencilFormat;

    uint32 m_frameLatency;

    // graphics command queue
    D3D12CommandQueue *m_pGraphicsCommandQueue;

    // copy command queues
    CopyCommandQueueArray m_copyCommandQueues;

    // legacy root signature
    ID3D12RootSignature *m_pLegacyGraphicsRootSignature;
    ID3D12RootSignature *m_pLegacyComputeRootSignature;
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE m_fnD3D12SerializeRootSignature;

    // descriptor heaps
    D3D12DescriptorHeap *m_pCPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    D3D12DescriptorHandle m_nullCBVDescriptorHandle;
    D3D12DescriptorHandle m_nullSRVDescriptorHandle;
    D3D12DescriptorHandle m_nullSamplerHandle;

    // constant buffer storage
    struct ConstantBufferStorage
    {
        ID3D12Resource *pResource;
        D3D12DescriptorHandle DescriptorHandle;
        uint32 Size;
    };
    MemArray<ConstantBufferStorage> m_constantBufferStorage;
    ID3D12Heap *m_pConstantBufferStorageHeap;

    // resources pending transition
    struct PendingResourceTransition
    {
        ID3D12Resource *pResource;
        D3D12_RESOURCE_STATES BeforeState;
        D3D12_RESOURCE_STATES AfterState;
    };
    MemArray<PendingResourceTransition> m_pendingResourceTransitions;
    Mutex m_pendingResourceTransitionLock;
};
