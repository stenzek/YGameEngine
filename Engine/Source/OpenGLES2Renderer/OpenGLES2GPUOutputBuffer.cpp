#include "OpenGLES2Renderer/PrecompiledHeader.h"
#include "OpenGLES2Renderer/OpenGLES2GPUOutputBuffer.h"
#include "OpenGLES2Renderer/OpenGLES2GPUDevice.h"
#include "Engine/SDLHeaders.h"
Log_SetChannel(RenderBackend);

OpenGLES2GPUOutputBuffer::OpenGLES2GPUOutputBuffer(SDL_Window *pSDLWindow, PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType, bool externalWindow)
    : GPUOutputBuffer(vsyncType)
    , m_pSDLWindow(pSDLWindow)
    , m_width(0)
    , m_height(0)
    , m_backBufferFormat(backBufferFormat)
    , m_depthStencilBufferFormat(depthStencilBufferFormat)
    , m_vsyncType(vsyncType)
    , m_externalWindow(externalWindow)
{
    // get window dimensions
    SDL_GetWindowSize(pSDLWindow, (int *)&m_width, (int *)&m_height);
}

OpenGLES2GPUOutputBuffer::~OpenGLES2GPUOutputBuffer()
{
    // we don't destroy the window (except for external outputs), it's the owner's job to do that.
    if (m_externalWindow)
        SDL_DestroyWindow(m_pSDLWindow);
}

void OpenGLES2GPUOutputBuffer::Resize(uint32 width, uint32 height)
{
    if (width == 0 || height == 0)
    {
        // get window dimensions
        int windowWidth, windowHeight;
        SDL_GetWindowSize(m_pSDLWindow, &windowWidth, &windowHeight);
        width = windowWidth;
        height = windowHeight;
    }

    m_width = width;
    m_height = height;
}

void OpenGLES2GPUOutputBuffer::SetVSyncType(RENDERER_VSYNC_TYPE vsyncType)
{
    if (m_vsyncType == vsyncType)
        return;

    // fixme!
    m_vsyncType = vsyncType;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GPUOutputBuffer *OpenGLES2GPUDevice::CreateOutputBuffer(RenderSystemWindowHandle hWnd, RENDERER_VSYNC_TYPE vsyncType)
{
    // create an external sdl window
    SDL_Window *pSDLWindow = SDL_CreateWindowFrom(reinterpret_cast<const void *>(hWnd));
    if (pSDLWindow == nullptr)
    {
        Log_ErrorPrintf("OpenGLGPUOutputBuffer::Create: SDL_CreateWindowFrom failed: %s", SDL_GetError());
        return nullptr;
    }

    // create wrapper
    return new OpenGLES2GPUOutputBuffer(pSDLWindow, m_outputBackBufferFormat, m_outputDepthStencilFormat, vsyncType, true);
}

GPUOutputBuffer *OpenGLES2GPUDevice::CreateOutputBuffer(SDL_Window *pSDLWindow, RENDERER_VSYNC_TYPE vsyncType)
{
    // create wrapper
    return new OpenGLES2GPUOutputBuffer(pSDLWindow, m_outputBackBufferFormat, m_outputDepthStencilFormat, vsyncType, false);
}

