#pragma once
#include "OpenGLRenderer/OpenGLCommon.h"
#include "OpenGLRenderer/OpenGLGPUTexture.h"

class OpenGLRendererOutputBuffer;
class OpenGLRendererOutputWindow;
struct SDL_Window;
union SDL_Event;

class OpenGLRendererOutputBuffer : public RendererOutputBuffer
{
    friend class OpenGLRendererOutputWindow;

public:
    virtual ~OpenGLRendererOutputBuffer();

    // virtual methods
    virtual uint32 GetWidth() const override { return m_width; }
    virtual uint32 GetHeight() const override { return m_height; }
    virtual bool ResizeBuffers(uint32 width = 0, uint32 height = 0) override;
    virtual void SetVSyncType(RENDERER_VSYNC_TYPE vsyncType) override;
    virtual void SwapBuffers() override;

    // creation
    static OpenGLRendererOutputBuffer *Create(RenderSystemWindowHandle windowHandle, PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType);

    // views
    SDL_Window *GetSDLWindow() const { return m_pSDLWindow; }
    PIXEL_FORMAT GetBackBufferFormat() const { return m_backBufferFormat; }
    PIXEL_FORMAT GetDepthStencilBufferFormat() const { return m_depthStencilBufferFormat; }

private:
    OpenGLRendererOutputBuffer(SDL_Window *pSDLWindow, bool ownsWindow, uint32 width, uint32 height, PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType);

    SDL_Window *m_pSDLWindow;
    bool m_ownsWindow;

    uint32 m_width;
    uint32 m_height;

    PIXEL_FORMAT m_backBufferFormat;
    PIXEL_FORMAT m_depthStencilBufferFormat;
};

class OpenGLRendererOutputWindow : public RendererOutputWindow
{
public:
    OpenGLRendererOutputWindow(SDL_Window *pSDLWindow, OpenGLRendererOutputBuffer *pBuffer, RENDERER_FULLSCREEN_STATE fullscreenState);
    virtual ~OpenGLRendererOutputWindow();

    // accessors
    OpenGLRendererOutputBuffer *GetOpenGLOutputBuffer() { return m_pOpenGLOutputBuffer; }

    // Overrides
    virtual bool SetFullScreen(RENDERER_FULLSCREEN_STATE type, uint32 width, uint32 height) override;

    // creation
    static OpenGLRendererOutputWindow *Create(const char *windowCaption, uint32 windowWidth, uint32 windowHeight, PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType, bool visible);

    // pending event handler
    static int PendingSDLEventFilter(void *pUserData, SDL_Event *pEvent);

private:
    OpenGLRendererOutputBuffer *m_pOpenGLOutputBuffer;
};

