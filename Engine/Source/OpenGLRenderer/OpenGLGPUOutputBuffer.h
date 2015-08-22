#pragma once
#include "OpenGLRenderer/OpenGLCommon.h"
#include "OpenGLRenderer/OpenGLGPUTexture.h"

class OpenGLGPUOutputBuffer : public GPUOutputBuffer
{
public:
    OpenGLGPUOutputBuffer(SDL_Window *pSDLWindow, PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType, bool externalWindow);
    virtual ~OpenGLGPUOutputBuffer();

    // virtual methods
    virtual uint32 GetWidth() const override { return m_width; }
    virtual uint32 GetHeight() const override { return m_height; }
    virtual void SetVSyncType(RENDERER_VSYNC_TYPE vsyncType) override;

    // views
    SDL_Window *GetSDLWindow() const { return m_pSDLWindow; }
    PIXEL_FORMAT GetBackBufferFormat() const { return m_backBufferFormat; }
    PIXEL_FORMAT GetDepthStencilBufferFormat() const { return m_depthStencilBufferFormat; }
    void Resize(uint32 width, uint32 height);

private:
    SDL_Window *m_pSDLWindow;

    uint32 m_width;
    uint32 m_height;

    PIXEL_FORMAT m_backBufferFormat;
    PIXEL_FORMAT m_depthStencilBufferFormat;

    RENDERER_VSYNC_TYPE m_vsyncType;
    bool m_externalWindow;
};
