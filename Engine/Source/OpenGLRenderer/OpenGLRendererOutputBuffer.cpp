#include "OpenGLRenderer/PrecompiledHeader.h"
#include "OpenGLRenderer/OpenGLRendererOutputBuffer.h"
#include "OpenGLRenderer/OpenGLRenderer.h"
#include "Engine/SDLHeaders.h"
Log_SetChannel(D3D11GPUOutputBuffer);

// fix up a warning
#ifdef SDL_VIDEO_DRIVER_WINDOWS
    #ifdef WIN32_LEAN_AND_MEAN
        #undef WIN32_LEAN_AND_MEAN
    #endif
    #include <SDL_syswm.h>
#endif

static const char *SDL_OPENGL_RENDERER_OUTPUT_WINDOW_POINTER_STRING = "OpenGLRendererOutputWindowPtr";

OpenGLRendererOutputBuffer::OpenGLRendererOutputBuffer(SDL_Window *pSDLWindow, bool ownsWindow, uint32 width, uint32 height, PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType)
    : GPUOutputBuffer(vsyncType),
      m_pSDLWindow(pSDLWindow),
      m_ownsWindow(ownsWindow),
      m_width(width),
      m_height(height),
      m_backBufferFormat(backBufferFormat),
      m_depthStencilBufferFormat(depthStencilBufferFormat)
{

}

OpenGLRendererOutputBuffer::~OpenGLRendererOutputBuffer()
{
    // we don't destroy the window (except for external outputs), it's the owner's job to do that.
    if (m_ownsWindow)
        SDL_DestroyWindow(m_pSDLWindow);
}

OpenGLRendererOutputBuffer *OpenGLRendererOutputBuffer::Create(RenderSystemWindowHandle windowHandle, PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType)
{
    // create an external sdl window
    SDL_Window *pSDLWindow = SDL_CreateWindowFrom(reinterpret_cast<const void *>(windowHandle));
    if (pSDLWindow == nullptr)
    {
        Log_ErrorPrintf("OpenGLRendererOutputBuffer::Create: SDL_CreateWindowFrom failed: %s", SDL_GetError());
        return nullptr;
    }

    // get window dimensions
    int windowWidth, windowHeight;
    SDL_GetWindowSize(pSDLWindow, &windowWidth, &windowHeight);

    // create the output buffer
    // todo: fix up vsync
    return new OpenGLRendererOutputBuffer(pSDLWindow, true, windowWidth, windowHeight, backBufferFormat, depthStencilBufferFormat, vsyncType);
}

void OpenGLRendererOutputBuffer::Resize(uint32 width, uint32 height)
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

void OpenGLRendererOutputBuffer::SetVSyncType(RENDERER_VSYNC_TYPE vsyncType)
{
    if (m_vsyncType == vsyncType)
        return;

    // fixme!
    m_vsyncType = vsyncType;
}

void OpenGLRendererOutputBuffer::SwapBuffers()
{
    SDL_GL_SwapWindow(m_pSDLWindow);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLRendererOutputWindow::OpenGLRendererOutputWindow(SDL_Window *pSDLWindow, OpenGLRendererOutputBuffer *pBuffer, RENDERER_FULLSCREEN_STATE fullscreenState)
    : RendererOutputWindow(pSDLWindow, pBuffer, fullscreenState),
      m_pOpenGLOutputBuffer(pBuffer)
{

}

OpenGLRendererOutputWindow::~OpenGLRendererOutputWindow()
{
    // remove the window binding
    SDL_SetWindowData(m_pSDLWindow, SDL_OPENGL_RENDERER_OUTPUT_WINDOW_POINTER_STRING, nullptr);

//     // if we are in fullscreen state, dxgi requires we switch back to windowed before releasing the swap chain
//     if (m_fullscreenState == RENDERER_FULLSCREEN_STATE_FULLSCREEN)
//     {
//         Log_DevPrint("D3D11RendererOutputWindow::~D3D11RendererOutputWindow: Switching back to windowed mode before destroying swap chain.");
// 
//         // change fullscreen state back
//         HRESULT hResult = m_pOpenGLOutputBuffer->GetDXGISwapChain()->SetFullscreenState(FALSE, NULL);
//         if (FAILED(hResult))
//             Log_WarningPrintf("D3D11RendererOutputWindow::~D3D11RendererOutputWindow: Windowed switch at release failed with hResult %08X", hResult);
//     }
}

OpenGLRendererOutputWindow *OpenGLRendererOutputWindow::Create(const char *windowCaption, uint32 windowWidth, uint32 windowHeight, PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType, bool visible)
{
    // Determine SDL window flags
    uint32 sdlWindowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL;
    if (visible)
        sdlWindowFlags |= SDL_WINDOW_SHOWN;
    else
        sdlWindowFlags |= SDL_WINDOW_HIDDEN;

    // Create the SDL window
    SDL_Window *pSDLWindow = SDL_CreateWindow(windowCaption, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, sdlWindowFlags);

    // Created?
    if (pSDLWindow == nullptr)
    {
        Log_ErrorPrintf("OpenGLRendererOutputWindow::Create: SDL_CreateWindow failed: %s", SDL_GetError());
        return nullptr;
    }

    // create the buffer
    OpenGLRendererOutputBuffer *pOutputBuffer = new OpenGLRendererOutputBuffer(pSDLWindow, false, windowWidth, windowHeight, backBufferFormat, depthStencilBufferFormat, vsyncType);

    // create the output window
    OpenGLRendererOutputWindow *pOutputWindow = new OpenGLRendererOutputWindow(pSDLWindow, pOutputBuffer, RENDERER_FULLSCREEN_STATE_WINDOWED);

    // bind the output window to the sdl window
    SDL_SetWindowData(pSDLWindow, SDL_OPENGL_RENDERER_OUTPUT_WINDOW_POINTER_STRING, reinterpret_cast<void *>(pOutputWindow));

    // done
    return pOutputWindow;
}

bool OpenGLRendererOutputWindow::SetFullScreen(RENDERER_FULLSCREEN_STATE state, uint32 width /* = 0 */, uint32 height /* = 0 */)
{
    if (state == RENDERER_FULLSCREEN_STATE_FULLSCREEN)
    {
        // find the matching sdl pixel format
        SDL_DisplayMode preferredDisplayMode;
        preferredDisplayMode.w = (width == 0) ? m_pOpenGLOutputBuffer->GetWidth() : width;
        preferredDisplayMode.h = (height == 0) ? m_pOpenGLOutputBuffer->GetHeight() : height;
        preferredDisplayMode.refresh_rate = 0;
        preferredDisplayMode.driverdata = nullptr;

        // we only have a limited number of backbuffer format possibilities
        switch (m_pOpenGLOutputBuffer->GetBackBufferFormat())
        {
        case PIXEL_FORMAT_R8G8B8A8_UNORM:       preferredDisplayMode.format = SDL_PIXELFORMAT_RGBA8888;     break;
        case PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB:  preferredDisplayMode.format = SDL_PIXELFORMAT_RGBA8888;     break;
        case PIXEL_FORMAT_B8G8R8A8_UNORM:       preferredDisplayMode.format = SDL_PIXELFORMAT_BGRA8888;     break;
        case PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:  preferredDisplayMode.format = SDL_PIXELFORMAT_BGRA8888;     break;
        case PIXEL_FORMAT_B8G8R8X8_UNORM:       preferredDisplayMode.format = SDL_PIXELFORMAT_BGRX8888;     break;
        case PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB:  preferredDisplayMode.format = SDL_PIXELFORMAT_BGRX8888;     break;
        default:
            Log_ErrorPrintf("OpenGLRendererOutputWindow::SetFullScreen: Unable to find SDL pixel format for %s", PixelFormat_GetPixelFormatInfo(m_pOpenGLOutputBuffer->GetBackBufferFormat())->Name);
            return false;
        }

        // find the closest match
        SDL_DisplayMode foundDisplayMode;
        if (SDL_GetClosestDisplayMode(0, &preferredDisplayMode, &foundDisplayMode) == nullptr)
        {
            Log_ErrorPrintf("OpenGLRendererOutputWindow::SetFullScreen: Unable to find matching video mode for: %i x %i (%s)", preferredDisplayMode.w, preferredDisplayMode.h, PixelFormat_GetPixelFormatInfo(m_pOpenGLOutputBuffer->GetBackBufferFormat())->Name);
            return false;
        }

        // change to this mode
        if (SDL_SetWindowDisplayMode(m_pSDLWindow, &foundDisplayMode) != 0)
        {
            Log_ErrorPrintf("OpenGLRendererOutputWindow::SetFullScreen: SDL_SetWindowDisplayMode failed: %s", SDL_GetError());
            return false;
        }

        // and switch to fullscreen if not already there
        if (m_fullscreenState != RENDERER_FULLSCREEN_STATE_FULLSCREEN)
        {
            if (SDL_SetWindowFullscreen(m_pSDLWindow, SDL_WINDOW_FULLSCREEN) != 0)
            {
                Log_ErrorPrintf("OpenGLRendererOutputWindow::SetFullScreen: SDL_SetWindowFullscreen failed: %s", SDL_GetError());
                return false;
            }

            m_fullscreenState = RENDERER_FULLSCREEN_STATE_FULLSCREEN;
        }

        // update the window size
        m_pOpenGLOutputBuffer->Resize(foundDisplayMode.w, foundDisplayMode.h);
        m_width = m_pOpenGLOutputBuffer->GetWidth();
        m_height = m_pOpenGLOutputBuffer->GetHeight();
        return true;
    }
    else if (state == RENDERER_FULLSCREEN_STATE_WINDOWED_FULLSCREEN)
    {
        // if we're not fullscreen, switch back
        if (m_fullscreenState != RENDERER_FULLSCREEN_STATE_WINDOWED_FULLSCREEN)
        {
            if (SDL_SetWindowFullscreen(m_pSDLWindow, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0)
            {
                Log_ErrorPrintf("OpenGLRendererOutputWindow::SetFullScreen: SDL_SetWindowFullscreen failed: %s", SDL_GetError());
                return false;
            }

            m_fullscreenState = RENDERER_FULLSCREEN_STATE_WINDOWED_FULLSCREEN;
        }

        // update the window size
        m_pOpenGLOutputBuffer->Resize();
        m_width = m_pOpenGLOutputBuffer->GetWidth();
        m_height = m_pOpenGLOutputBuffer->GetHeight();
        return true;
    }
    else
    {
        // switch to windowed
        if (m_fullscreenState != RENDERER_FULLSCREEN_STATE_WINDOWED)
        {
            if (SDL_SetWindowFullscreen(m_pSDLWindow, 0) != 0)
            {
                Log_ErrorPrintf("OpenGLRendererOutputWindow::SetFullScreen: SDL_SetWindowFullscreen failed: %s", SDL_GetError());
                return false;
            }

            m_fullscreenState = RENDERER_FULLSCREEN_STATE_WINDOWED;
        }

        // resize the window
        if (width != 0 && height != 0)
            SDL_SetWindowSize(m_pSDLWindow, width, height);

        // update the size
        m_pOpenGLOutputBuffer->Resize();
        m_width = m_pOpenGLOutputBuffer->GetWidth();
        m_height = m_pOpenGLOutputBuffer->GetHeight();
        return true;
    }
}

int OpenGLRendererOutputWindow::PendingSDLEventFilter(void *pUserData, SDL_Event *pEvent)
{
//     switch (pEvent->type)
//     {
//     case SDL_WINDOWEVENT:
//         {
//             // Is it an event we care about?
//             switch (pEvent->window.event)
//             {
//             case SDL_WINDOWEVENT_RESIZED:
//                 {
//                     // Get the window pointer
//                     SDL_Window *pWindow = SDL_GetWindowFromID(pEvent->window.windowID);
//                     D3D11RendererOutputWindow *pOutputWindow;
//                     if (pWindow != nullptr && (pOutputWindow = reinterpret_cast<D3D11RendererOutputWindow *>(SDL_GetWindowData(pWindow, SDL_D3D11_RENDERER_OUTPUT_WINDOW_POINTER_STRING))) != NULL)
//                     {
//                         // Handle the event
//                         switch (pEvent->window.event)
//                         {
//                         case SDL_WINDOWEVENT_RESIZED:
//                             {
//                                 // Check the fullscreen state of the swap chain
//                                 if (pOutputWindow->GetFullscreenState() == RENDERER_FULLSCREEN_STATE_FULLSCREEN)
//                                 {
//                                     BOOL newFullscreenState;
//                                     HRESULT hResult = pOutputWindow->GetD3DOutputBuffer()->GetDXGISwapChain()->GetFullscreenState(&newFullscreenState, nullptr);
//                                     if (SUCCEEDED(hResult) && !newFullscreenState)
//                                     {
//                                         Log_WarningPrintf("D3D11RendererOutputWindow::PendingSDLEventFilter: Fullscreen state lost outside our control. Updating internal state.");
//                                         pOutputWindow->m_fullscreenState = RENDERER_FULLSCREEN_STATE_WINDOWED;
//                                     }
//                                     else
//                                     {
//                                         Log_WarningPrintf("D3D11RendererOutputWindow::PendingSDLEventFilter: IDXGISwapChain::GetFullscreenState failed with hResult %08X.", hResult);
//                                     }
//                                 }
// 
//                                 // Issue buffer resize
//                                 pOutputWindow->GetD3DOutputBuffer()->ResizeBuffers(pEvent->window.data1, pEvent->window.data2);
//                             }
//                             break;
//                         }
//                     }
//                 }
//                 break;
//             }
//         }
//     }

    // don't delete any events
    return 1;
}

void OpenGLRenderer::HandlePendingSDLEvents()
{
    SDL_FilterEvents(&OpenGLRendererOutputWindow::PendingSDLEventFilter, nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GPUOutputBuffer *OpenGLRenderer::CreateOutputBuffer(RenderSystemWindowHandle hWnd, RENDERER_VSYNC_TYPE vsyncType)
{
    return OpenGLRendererOutputBuffer::Create(hWnd, m_swapChainBackBufferFormat, m_swapChainDepthStencilBufferFormat, vsyncType);
}

RendererOutputWindow *OpenGLRenderer::CreateOutputWindow(const char *windowTitle, uint32 windowWidth, uint32 windowHeight, RENDERER_VSYNC_TYPE vsyncType)
{
    //return D3D11RendererOutputWindow::Create(m_pDXGIFactory, m_pDXGIAdapter, m_pD3DDevice, windowTitle, windowWidth, windowHeight, m_swapChainBackBufferFormat, m_swapChainDepthStencilBufferFormat, vsyncType, true);
    return nullptr;
}

