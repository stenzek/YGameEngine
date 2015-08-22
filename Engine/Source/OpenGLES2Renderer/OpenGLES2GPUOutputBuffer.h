#pragma once
#include "OpenGLES2Renderer/OpenGLES2Common.h"
#include "OpenGLES2Renderer/OpenGLES2GPUTexture.h"

class OpenGLES2GPUOutputBuffer : public GPUOutputBuffer
{
public:
    OpenGLES2GPUOutputBuffer(SDL_Window *pSDLWindow, PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType, bool externalWindow);
    virtual ~OpenGLES2GPUOutputBuffer();

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
