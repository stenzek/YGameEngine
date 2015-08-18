#pragma once
#include "D3D11Renderer/D3D11Common.h"
#include "D3D11Renderer/D3D11GPUTexture.h"

class D3D11RendererOutputBuffer;
class D3D11RendererOutputWindow;
union SDL_Event;

class D3D11RendererOutputBuffer : public RendererOutputBuffer
{
    friend class D3D11RendererOutputWindow;

public:
    virtual ~D3D11RendererOutputBuffer();

    // virtual methods
    virtual uint32 GetWidth() const override { return m_width; }
    virtual uint32 GetHeight() const override { return m_height; }
    virtual bool ResizeBuffers(uint32 width = 0, uint32 height = 0) override;
    virtual void SetVSyncType(RENDERER_VSYNC_TYPE vsyncType) override;
    virtual void SwapBuffers() override;

    // creation
    static D3D11RendererOutputBuffer *Create(IDXGIFactory *pDXGIFactory, ID3D11Device *pD3DDevice, HWND hWnd, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType);

    // views
    ID3D11Device *GetD3DDevice() const { return m_pD3DDevice; }
    IDXGISwapChain *GetDXGISwapChain() const { return m_pDXGISwapChain; }
    ID3D11Texture2D *GetBackBufferTexture() const { return m_pBackBufferTexture; }
    ID3D11RenderTargetView *GetRenderTargetView() const { return m_pRenderTargetView; }
    ID3D11Texture2D *GetDepthStencilBuffer() const { return m_pDepthStencilBuffer; }
    ID3D11DepthStencilView *GetDepthStencilView() const { return m_pDepthStencilView; }
    DXGI_FORMAT GetBackBufferFormat() const { return m_backBufferFormat; }
    DXGI_FORMAT GetDepthStencilBufferFormat() const { return m_depthStencilBufferFormat; }

private:
    D3D11RendererOutputBuffer(ID3D11Device *pD3DDevice, IDXGISwapChain *pDXGISwapChain, HWND hWnd, uint32 width, uint32 height, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType);
    void InternalResizeBuffers(uint32 width, uint32 height, RENDERER_VSYNC_TYPE vsyncType);
    bool InternalCreateBuffers();
    void InternalReleaseBuffers();

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

class D3D11RendererOutputWindow : public RendererOutputWindow
{
public:
    D3D11RendererOutputWindow(SDL_Window *pSDLWindow, D3D11RendererOutputBuffer *pBuffer, RENDERER_FULLSCREEN_STATE fullscreenState);
    virtual ~D3D11RendererOutputWindow();

    // Accessors
    D3D11RendererOutputBuffer *GetD3DOutputBuffer() const { return m_pD3DOutputBuffer; }

    // Overrides
    virtual bool SetFullScreen(RENDERER_FULLSCREEN_STATE type, uint32 width, uint32 height) override;

    // creation
    static D3D11RendererOutputWindow *Create(IDXGIFactory *pDXGIFactory, IDXGIAdapter *pDXGIAdapter, ID3D11Device *pD3DDevice, const char *windowCaption, uint32 windowWidth, uint32 windowHeight, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType, bool visible);

    // pending event handler
    static int PendingSDLEventFilter(void *pUserData, SDL_Event *pEvent);

private:
    D3D11RendererOutputBuffer *m_pD3DOutputBuffer;
};

