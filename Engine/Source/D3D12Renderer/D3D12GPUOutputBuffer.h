#pragma once
#include "D3D12Renderer/D3D12Common.h"
#include "D3D12Renderer/D3D12DescriptorHeap.h"

class D3D12GPUOutputBuffer : public GPUOutputBuffer
{
public:
    virtual ~D3D12GPUOutputBuffer();

    // virtual methods
    virtual uint32 GetWidth() const override { return m_width; }
    virtual uint32 GetHeight() const override { return m_height; }
    virtual void SetVSyncType(RENDERER_VSYNC_TYPE vsyncType) override;

    // creation
    static D3D12GPUOutputBuffer *Create(D3D12GPUDevice *pDevice, IDXGIFactory4 *pDXGIFactory, ID3D12Device *pD3DDevice, ID3D12CommandQueue *pCommandQueue, HWND hWnd, PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilFormat, RENDERER_VSYNC_TYPE vsyncType);

    // views
    HWND GetHWND() const { return m_hWnd; }
    ID3D12Device *GetD3DDevice() const { return m_pD3DDevice; }
    IDXGISwapChain3 *GetDXGISwapChain() const { return m_pDXGISwapChain; }
    PIXEL_FORMAT GetBackBufferFormat() const { return m_backBufferFormat; }
    PIXEL_FORMAT GetDepthStencilBufferFormat() const { return m_depthStencilFormat; }
    DXGI_FORMAT GetBackBufferDXGIFormat() const { return m_backBufferDXGIFormat; }
    DXGI_FORMAT GetDepthStencilBufferDXGIFormat() const { return m_depthStencilDXGIFormat; }

    // current backbuffer access
    ID3D12Resource *GetCurrentBackBufferResource() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferViewDescriptorCPUHandle() const;
    bool UpdateCurrentBackBuffer();

    // DS access
    ID3D12Resource *GetDepthStencilBufferResource() const { return m_pDepthStencilBuffer; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilBufferViewDescriptorCPUHandle() const { return m_depthStencilViewDescriptor.CPUHandle; }
    bool HasDepthStencilBuffer() const { return (m_pDepthStencilBuffer != nullptr); }

    // resizing
    void InternalResizeBuffers(uint32 width, uint32 height, RENDERER_VSYNC_TYPE vsyncType);
    bool InternalCreateBuffers();
    void InternalReleaseBuffers();

private:
    D3D12GPUOutputBuffer(D3D12GPUDevice *pDevice, ID3D12Device *pD3DDevice, IDXGISwapChain3 *pDXGISwapChain, HWND hWnd, uint32 width, uint32 height, PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilFormat, DXGI_FORMAT backBufferDXGIFormat, DXGI_FORMAT depthStencilDXGIFormat, RENDERER_VSYNC_TYPE vsyncType);

    D3D12GPUDevice *m_pDevice;
    ID3D12Device *m_pD3DDevice;
    IDXGISwapChain3 *m_pDXGISwapChain;

    HWND m_hWnd;

    uint32 m_width;
    uint32 m_height;

    PIXEL_FORMAT m_backBufferFormat;
    PIXEL_FORMAT m_depthStencilFormat;
    DXGI_FORMAT m_backBufferDXGIFormat;
    DXGI_FORMAT m_depthStencilDXGIFormat;

    uint32 m_currentBackBufferIndex;
    PODArray<ID3D12Resource *> m_backBuffers;
    ID3D12Resource *m_pDepthStencilBuffer;

    D3D12DescriptorHandle m_renderTargetViewsDescriptorStart;
    D3D12DescriptorHandle m_depthStencilViewDescriptor;
};
