#pragma once
#include "D3D11Renderer/D3D11Common.h"
#include "D3D11Renderer/D3D11GPUTexture.h"

class D3D11GPUOutputBuffer;
class D3D11RendererOutputWindow;
union SDL_Event;

class D3D11GPUOutputBuffer : public GPUOutputBuffer
{
    friend class D3D11RendererOutputWindow;

public:
    virtual ~D3D11GPUOutputBuffer();

    // virtual methods
    virtual uint32 GetWidth() const override { return m_width; }
    virtual uint32 GetHeight() const override { return m_height; }
    virtual void SetVSyncType(RENDERER_VSYNC_TYPE vsyncType) override;

    // creation
    static D3D11GPUOutputBuffer *Create(IDXGIFactory *pDXGIFactory, ID3D11Device *pD3DDevice, HWND hWnd, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType);

    // views
    HWND GetHWND() const { return m_hWnd; }
    ID3D11Device *GetD3DDevice() const { return m_pD3DDevice; }
    IDXGISwapChain *GetDXGISwapChain() const { return m_pDXGISwapChain; }
    ID3D11Texture2D *GetBackBufferTexture() const { return m_pBackBufferTexture; }
    ID3D11RenderTargetView *GetRenderTargetView() const { return m_pRenderTargetView; }
    ID3D11Texture2D *GetDepthStencilBuffer() const { return m_pDepthStencilBuffer; }
    ID3D11DepthStencilView *GetDepthStencilView() const { return m_pDepthStencilView; }
    DXGI_FORMAT GetBackBufferFormat() const { return m_backBufferFormat; }
    DXGI_FORMAT GetDepthStencilBufferFormat() const { return m_depthStencilBufferFormat; }
    void InternalResizeBuffers(uint32 width, uint32 height, RENDERER_VSYNC_TYPE vsyncType);
    bool InternalCreateBuffers();
    void InternalReleaseBuffers();

private:
    D3D11GPUOutputBuffer(ID3D11Device *pD3DDevice, IDXGISwapChain *pDXGISwapChain, HWND hWnd, uint32 width, uint32 height, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType);

    ID3D11Device *m_pD3DDevice;
    IDXGISwapChain *m_pDXGISwapChain;

    HWND m_hWnd;

    uint32 m_width;
    uint32 m_height;

    DXGI_FORMAT m_backBufferFormat;
    DXGI_FORMAT m_depthStencilBufferFormat;

    ID3D11Texture2D *m_pBackBufferTexture;
    ID3D11RenderTargetView *m_pRenderTargetView;

    ID3D11Texture2D *m_pDepthStencilBuffer;
    ID3D11DepthStencilView *m_pDepthStencilView;
};
