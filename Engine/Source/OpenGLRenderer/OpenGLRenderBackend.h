#pragma once
#include "OpenGLRenderer/OpenGLCommon.h"
#include "OpenGLRenderer/OpenGLGPUDevice.h"
#include "OpenGLRenderer/OpenGLGPUContext.h"
#include "OpenGLRenderer/OpenGLGPUOutputBuffer.h"

class OpenGLRenderBackend : public RenderBackend
{
public:
    OpenGLRenderBackend();
    ~OpenGLRenderBackend();

    // instance accessor
    static OpenGLRenderBackend *GetInstance() { return m_pInstance; }

    // internal methods
    OpenGLGPUDevice *GetGPUDevice() const { return m_pGPUDevice; }
    OpenGLGPUContext *GetGPUContext() const { return m_pGPUContext; }

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
    RENDERER_FEATURE_LEVEL m_featureLevel;
    TEXTURE_PLATFORM m_texturePlatform;

    PIXEL_FORMAT m_outputBackBufferFormat;
    PIXEL_FORMAT m_outputDepthStencilFormat;

    OpenGLGPUDevice *m_pGPUDevice;
    OpenGLGPUContext *m_pGPUContext;

    // instance
    static OpenGLRenderBackend *m_pInstance;
};
