#pragma once
#include "D3D12Renderer/D3D12Common.h"
#include "D3D12Renderer/D3D12DescriptorHeap.h"

class D3D12RenderBackend : public RenderBackend
{
public:
    struct ConstantBufferStorage
    {
        ID3D12Resource *pResource;
        D3D12DescriptorHandle DescriptorHandle;
        uint32 Size;
    };

public:
    D3D12RenderBackend();
    virtual ~D3D12RenderBackend();

    // instance accessor
    static D3D12RenderBackend *GetInstance() { return s_pInstance; }

    // internal methods
    IDXGIFactory3 *GetDXGIFactory() const { return m_pDXGIFactory; }
    IDXGIAdapter3 *GetDXGIAdapter() const { return m_pDXGIAdapter; }
    ID3D12Device *GetD3DDevice() const { return m_pD3DDevice; }
    D3D12GPUDevice *GetGPUDevice() const { return m_pGPUDevice; }
    D3D12GPUContext *GetGPUContext() const { return m_pGPUContext; }
    D3D12DescriptorHeap *GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) { return m_pDescriptorHeaps[type]; }
    uint32 GetFrameLatency() const { return m_frameLatency; }

    // constant buffer management
    ID3D12Resource *GetConstantBufferResource(uint32 index);
    const D3D12DescriptorHandle *GetConstantBufferDescriptor(uint32 index) const;

    // resource cleanup
    uint64 GetCurrentCleanupFenceValue() const { return m_currentCleanupFenceValue; }
    void SetCleanupFenceValue(uint64 fenceValue) { m_currentCleanupFenceValue = fenceValue; }
    void ScheduleResourceForDeletion(ID3D12Pageable *pResource);
    void ScheduleResourceForDeletion(ID3D12Pageable *pResource, uint64 fenceValue);
    void ScheduleDescriptorForDeletion(const D3D12DescriptorHandle &handle);
    void ScheduleDescriptorForDeletion(const D3D12DescriptorHandle &handle, uint64 fenceValue);
    void DeletePendingResources(uint64 fenceValue);

    // creation
    bool Create(const RendererInitializationParameters *pCreateParameters, SDL_Window *pSDLWindow, RenderBackend **ppBackend, GPUDevice **ppDevice, GPUContext **ppContext, GPUOutputBuffer **ppOutputBuffer);

    // public methods
    virtual RENDERER_PLATFORM GetPlatform() const override final;
    virtual RENDERER_FEATURE_LEVEL GetFeatureLevel() const override final;
    virtual TEXTURE_PLATFORM GetTexturePlatform() const override final;
    virtual void GetCapabilities(RendererCapabilities *pCapabilities) const override final;
    virtual bool CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat = nullptr) const override final;
    virtual GPUDevice *CreateDeviceInterface() override final;
    virtual void Shutdown() override final;

private:
    // create descriptor heaps
    bool CreateDescriptorHeaps();

    // create constant buffers
    bool CreateConstantStorage();

    // vars
    IDXGIFactory3 *m_pDXGIFactory;
    IDXGIAdapter3 *m_pDXGIAdapter;

    ID3D12Device *m_pD3DDevice;

    D3D_FEATURE_LEVEL m_D3DFeatureLevel;
    RENDERER_FEATURE_LEVEL m_featureLevel;
    TEXTURE_PLATFORM m_texturePlatform;

    DXGI_FORMAT m_outputBackBufferFormat;
    DXGI_FORMAT m_outputDepthStencilFormat;

    uint32 m_frameLatency;

    // device
    D3D12GPUDevice *m_pGPUDevice;
    D3D12GPUContext *m_pGPUContext;

    // descriptor heaps
    D3D12DescriptorHeap *m_pDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    // constant buffer storage
    MemArray<ConstantBufferStorage> m_constantBufferStorage;
    ID3D12Heap *m_pConstantBufferStorageHeap;

    // object scheduled for deletion
    struct PendingDeletionResource
    {
        ID3D12Pageable *pResource;
        uint64 FenceValue;
    };
    struct PendingDeletionDescriptor
    {
        D3D12_DESCRIPTOR_HEAP_TYPE Type;
        D3D12DescriptorHandle Handle;
        uint64 FenceValue;
    };
    MemArray<PendingDeletionResource> m_pendingDeletionResources;
    MemArray<PendingDeletionDescriptor> m_pendingDeletionDescriptors;
    Mutex m_pendingDeletionLock;
    uint64 m_currentCleanupFenceValue;

    // instance
    static D3D12RenderBackend *s_pInstance;
};
