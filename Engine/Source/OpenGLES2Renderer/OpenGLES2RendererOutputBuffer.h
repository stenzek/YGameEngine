#pragma once
#include "OpenGLES2Renderer/OpenGLES2Common.h"
#include "OpenGLES2Renderer/OpenGLES2GPUTexture.h"

class OpenGLES2RendererOutputBuffer;
class OpenGLES2RendererOutputWindow;
struct SDL_Window;
union SDL_Event;

class OpenGLES2RendererOutputBuffer : public GPUOutputBuffer
{
    friend class OpenGLES2RendererOutputWindow;

public:
    virtual ~OpenGLES2RendererOutputBuffer();

    // virtual methods
    virtual uint32 GetWidth() const override { return m_width; }
    virtual uint32 GetHeight() const override { return m_height; }
    virtual void SetVSyncType(RENDERER_VSYNC_TYPE vsyncType) override;

    // creation
    static OpenGLES2RendererOutputBuffer *Create(RenderSystemWindowHandle windowHandle, PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType);

    // views
    SDL_Window *GetSDLWindow() const { return m_pSDLWindow; }
    PIXEL_FORMAT GetBackBufferFormat() const { return m_backBufferFormat; }
    PIXEL_FORMAT GetDepthStencilBufferFormat() const { return m_depthStencilBufferFormat; }
    void Resize(uint32 width, uint32 height);

private:
    OpenGLES2RendererOutputBuffer(SDL_Window *pSDLWindow, bool ownsWindow, uint32 width, uint32 height, PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType);

    SDL_Window *m_pSDLWindow;
    bool m_ownsWindow;

    uint32 m_width;
    uint32 m_height;

    PIXEL_FORMAT m_backBufferFormat;
    PIXEL_FORMAT m_depthStencilBufferFormat;
};

class OpenGLES2RendererOutputWindow : public RendererOutputWindow
{
public:
    OpenGLES2RendererOutputWindow(SDL_Window *pSDLWindow, OpenGLES2RendererOutputBuffer *pBuffer, RENDERER_FULLSCREEN_STATE fullscreenState);
    virtual ~OpenGLES2RendererOutputWindow();

    // accessors
    OpenGLES2RendererOutputBuffer *GetOpenGLOutputBuffer() { return m_pOpenGLOutputBuffer; }

    // Overrides
    virtual bool SetFullScreen(RENDERER_FULLSCREEN_STATE type, uint32 width, uint32 height) override;

    // creation
    static OpenGLES2RendererOutputWindow *Create(const char *windowCaption, uint32 windowWidth, uint32 windowHeight, PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType, bool visible);

    // pending event handler
    static int PendingSDLEventFilter(void *pUserData, SDL_Event *pEvent);

private:
    OpenGLES2RendererOutputBuffer *m_pOpenGLOutputBuffer;
};

