#pragma once
#include "OpenGLRenderer/OpenGLCommon.h"
#include "OpenGLRenderer/OpenGLGPUDevice.h"
#include "OpenGLRenderer/OpenGLGPUContext.h"
#include "OpenGLRenderer/OpenGLGPUOutputBuffer.h"

#define GL_CHECKED_SECTION_BEGIN() (OpenGLRenderBackend::ClearLastGLError())
#define GL_CHECK_ERROR_STATE() (OpenGLRenderBackend::CheckForGLError() != GL_NO_ERROR)
#define GL_PRINT_ERROR(...) OpenGLRenderBackend::PrintLastGLError(__VA_ARGS__)

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

    // GL error handling
    static void ClearLastGLError();
    static GLenum CheckForGLError();
    static GLenum GetLastGLError();
    static void PrintLastGLError(const char *format, ...);

private:
    RENDERER_FEATURE_LEVEL m_featureLevel;
    TEXTURE_PLATFORM m_texturePlatform;

    PIXEL_FORMAT m_outputBackBufferFormat;
    PIXEL_FORMAT m_outputDepthStencilFormat;

    OpenGLGPUDevice *m_pGPUDevice;
    OpenGLGPUContext *m_pGPUContext;
    OpenGLGPUOutputBuffer *m_pImplicitOutputBuffer;

    // instance
    static OpenGLRenderBackend *m_pInstance;
};
