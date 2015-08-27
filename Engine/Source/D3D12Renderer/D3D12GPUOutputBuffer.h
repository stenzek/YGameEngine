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
    static D3D12GPUOutputBuffer *Create(D3D12RenderBackend *pBackend, IDXGIFactory3 *pDXGIFactory, ID3D12Device *pD3DDevice, HWND hWnd, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthStencilFormat, RENDERER_VSYNC_TYPE vsyncType);

    // views
    HWND GetHWND() const { return m_hWnd; }
    ID3D12Device *GetD3DDevice() const { return m_pD3DDevice; }
    IDXGISwapChain3 *GetDXGISwapChain() const { return m_pDXGISwapChain; }
    DXGI_FORMAT GetBackBufferFormat() const { return m_backBufferFormat; }
    DXGI_FORMAT GetDepthStencilBufferFormat() const { return m_depthStencilFormat; }

    // current backbuffer access
    ID3D12Resource *GetCurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferViewDescriptor() const;
    void UpdateCurrentBackBuffer();

    // DS access
    ID3D12Resource *GetDepthStencilBuffer() const { return m_pDepthStencilBuffer; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilBufferViewDescriptor() const { return m_depthStencilViewDescriptor.CPUHandle; }

    // resizing
    void InternalResizeBuffers(uint32 width, uint32 height, RENDERER_VSYNC_TYPE vsyncType);
    bool InternalCreateBuffers();
    void InternalReleaseBuffers();

private:
    D3D12GPUOutputBuffer(D3D12RenderBackend *pBackend, ID3D12Device *pD3DDevice, IDXGISwapChain3 *pDXGISwapChain, HWND hWnd, uint32 width, uint32 height, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthStencilFormat, RENDERER_VSYNC_TYPE vsyncType);

    D3D12RenderBackend *m_pBackend;
    ID3D12Device *m_pD3DDevice;
    IDXGISwapChain3 *m_pDXGISwapChain;

    HWND m_hWnd;

    uint32 m_width;
    uint32 m_height;

    DXGI_FORMAT m_backBufferFormat;
    DXGI_FORMAT m_depthStencilFormat;

    uint32 m_currentBackBufferIndex;
    PODArray<ID3D12Resource *> m_backBuffers;
    ID3D12Resource *m_pDepthStencilBuffer;

    D3D12DescriptorHandle m_renderTargetViewsDescriptorStart;
    D3D12DescriptorHandle m_depthStencilViewDescriptor;
};
