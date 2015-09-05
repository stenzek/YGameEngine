#pragma once
#include "D3D12Renderer/D3D12Common.h"
#include "D3D12Renderer/D3D12DescriptorHeap.h"
#include "D3D12Renderer/D3D12GraphicsCommandQueue.h"

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
    uint32 GetFrameLatency() const { return m_frameLatency; }

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

    // resource tracker
    D3D12GraphicsCommandQueue *GetGraphicsCommandQueue() { return m_pGraphicsCommandQueue; }

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
    // create legacy root signatures
    bool CreateLegacyRootSignatures();

    // create descriptor heaps
    bool CreateCPUDescriptorHeaps();

    // create constant buffers
    bool CreateConstantStorage();

    // vars
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

    // resource tracker
    D3D12GraphicsCommandQueue *m_pGraphicsCommandQueue;

    // device
    D3D12GPUDevice *m_pGPUDevice;
    D3D12GPUContext *m_pGPUContext;

    // Legacy root signature
    ID3D12RootSignature *m_pLegacyGraphicsRootSignature;
    ID3D12RootSignature *m_pLegacyComputeRootSignature;
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE m_fnD3D12SerializeRootSignature;

    // descriptor heaps
    D3D12DescriptorHeap *m_pCPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    D3D12DescriptorHandle m_nullCBVDescriptorHandle;
    D3D12DescriptorHandle m_nullSRVDescriptorHandle;
    D3D12DescriptorHandle m_nullSamplerHandle;

    // constant buffer storage
    MemArray<ConstantBufferStorage> m_constantBufferStorage;
    ID3D12Heap *m_pConstantBufferStorageHeap;

    // instance
    static D3D12RenderBackend *s_pInstance;
};
