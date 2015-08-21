#pragma once
#include "D3D11Renderer/D3D11Common.h"
#include "D3D11Renderer/D3D11GPUContext.h"
#include "D3D11Renderer/D3D11GPUOutputBuffer.h"

class D3D11RenderBackend : public RenderBackend
{
public:
    D3D11RenderBackend();
    ~D3D11RenderBackend();

    // instance accessor
    static D3D11RenderBackend *GetInstance() { return m_pInstance; }

    // internal methods
    IDXGIFactory *GetDXGIFactory() const { return m_pDXGIFactory; }
    IDXGIAdapter *GetDXGIAdapter() const { return m_pDXGIAdapter; }
    ID3D11Device *GetD3DDevice() const { return m_pD3DDevice; }
    ID3D11Device1 *GetD3DDevice1() const { return m_pD3DDevice1; }
    D3D11GPUDevice *GetGPUDevice() const { return m_pGPUDevice; }
    D3D11GPUContext *GetGPUContext() const { return m_pGPUContext; }

    // creation interface
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
    IDXGIFactory *m_pDXGIFactory;
    IDXGIAdapter *m_pDXGIAdapter;

    ID3D11Device *m_pD3DDevice;
    ID3D11Device1 *m_pD3DDevice1;
    D3D_FEATURE_LEVEL m_D3DFeatureLevel;
    RENDERER_FEATURE_LEVEL m_featureLevel;
    TEXTURE_PLATFORM m_texturePlatform;

    DXGI_FORMAT m_swapChainBackBufferFormat;
    DXGI_FORMAT m_swapChainDepthStencilBufferFormat;

    D3D11GPUDevice *m_pGPUDevice;
    D3D11GPUContext *m_pGPUContext;

    // instance
    static D3D11RenderBackend *m_pInstance;
};
