#pragma once
#include "OpenGLES2Renderer/OpenGLES2Common.h"
#include "OpenGLES2Renderer/OpenGLES2GPUOutputBuffer.h"
#include "OpenGLES2Renderer/OpenGLES2GPUContext.h"
#include "OpenGLES2Renderer/OpenGLES2GPUTexture.h"
#include "OpenGLES2Renderer/OpenGLES2GPUBuffer.h"

class OpenGLES2RenderBackend : public RenderBackend
{
public:
    OpenGLES2RenderBackend();
    ~OpenGLES2RenderBackend();

    // instance accessor
    static OpenGLES2RenderBackend *GetInstance() { return m_pInstance; }

    // internal methods
    OpenGLES2GPUDevice *GetGPUDevice() const { return m_pGPUDevice; }
    OpenGLES2GPUContext *GetGPUContext() const { return m_pGPUContext; }
    OpenGLES2ConstantLibrary *GetConstantLibrary() const { return m_pConstantLibrary; }

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

    OpenGLES2GPUDevice *m_pGPUDevice;
    OpenGLES2GPUContext *m_pGPUContext;
    OpenGLES2ConstantLibrary *m_pConstantLibrary;

    // instance
    static OpenGLES2RenderBackend *m_pInstance;
};
