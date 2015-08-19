#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12RendererOutputBuffer.h"
#include "D3D12Renderer/D3D12Renderer.h"
#include "Engine/SDLHeaders.h"
Log_SetChannel(D3D12RendererOutputBuffer);

// fix up a warning
#ifdef SDL_VIDEO_DRIVER_WINDOWS
    #undef WIN32_LEAN_AND_MEAN
#endif
#include <SDL/SDL_syswm.h>

static const char *SDL_D3D11_RENDERER_OUTPUT_WINDOW_POINTER_STRING = "D3D12RendererOutputBufferPtr";

static uint32 CalculateDXGISwapChainBufferCount(bool exclusiveFullscreen, RENDERER_VSYNC_TYPE vsyncType)
{
    return 1 + 
           ((IsCompositionActive() == FALSE || exclusiveFullscreen) ? 1 : 0) + 
           ((vsyncType == RENDERER_VSYNC_TYPE_TRIPLE_BUFFERING) ? 1 : 0);
}

D3D12RendererOutputBuffer::D3D12RendererOutputBuffer(ID3D12Device *pD3DDevice, IDXGISwapChain3 *pDXGISwapChain, HWND hWnd, uint32 width, uint32 height, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType)
    : RendererOutputBuffer(vsyncType)
    , m_pD3DDevice(pD3DDevice)
    , m_pDXGISwapChain(pDXGISwapChain)
    , m_hWnd(hWnd)
    , m_width(width)
    , m_height(height)
    , m_backBufferFormat(backBufferFormat)
    , m_depthStencilBufferFormat(depthStencilBufferFormat)
    //, m_pBackBufferTexture(NULL)
    //, m_pRenderTargetView(NULL)
    //, m_pDepthStencilBuffer(NULL)
    //, m_pDepthStencilView(NULL)
{

}

D3D12RendererOutputBuffer::~D3D12RendererOutputBuffer()
{
//     if (m_pDepthStencilView != NULL)
//         m_pDepthStencilView->Release();
// 
//     if (m_pDepthStencilBuffer != NULL)
//         m_pDepthStencilBuffer->Release();
// 
//     if (m_pRenderTargetView != NULL)
//         m_pRenderTargetView->Release();
// 
//     if (m_pBackBufferTexture != NULL)
//         m_pBackBufferTexture->Release();

    m_pDXGISwapChain->Release();
    m_pD3DDevice->Release();
}

D3D12RendererOutputBuffer *D3D12RendererOutputBuffer::Create(IDXGIFactory3 *pDXGIFactory, ID3D12Device *pD3DDevice, HWND hWnd, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType)
{
#if 0
    // get client rect of the window
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    uint32 width = Max(clientRect.right - clientRect.left, (LONG)1);
    uint32 height = Max(clientRect.bottom - clientRect.top, (LONG)1);

    // create swap chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    Y_memzero(&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = backBufferFormat;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = CalculateDXGISwapChainBufferCount(false, vsyncType);
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = 0;

    // create it
    IDXGISwapChain *pDXGISwapChain;
    HRESULT hResult = pDXGIFactory->CreateSwapChain(pD3DDevice, &swapChainDesc, &pDXGISwapChain);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12RendererOutputBuffer::Create: CreateSwapChain failed with hResult %08X", hResult);
        return false;
    }

    // create object
    pD3DDevice->AddRef();
    D3D12RendererOutputBuffer *pSwapChain = new D3D12RendererOutputBuffer(pD3DDevice, pDXGISwapChain, hWnd, width, height, backBufferFormat, depthStencilBufferFormat, vsyncType);
    if (!pSwapChain->InternalCreateBuffers())
    {
        Log_ErrorPrintf("D3D12RendererOutputBuffer::Create: CreateRTVAndDepthStencilBuffer failed");
        pSwapChain->Release();
        return NULL;
    }

    return pSwapChain;
#else
    return nullptr;
#endif
}

void D3D12RendererOutputBuffer::InternalResizeBuffers(uint32 width, uint32 height, RENDERER_VSYNC_TYPE vsyncType)
{
    HRESULT hResult;

    // check if fullscreen state was lost
    BOOL newFullscreenState;
    hResult = m_pDXGISwapChain->GetFullscreenState(&newFullscreenState, nullptr);
    if (FAILED(hResult))
        newFullscreenState = FALSE;

    // release resources
    InternalReleaseBuffers();

    // calculate buffer count
    uint32 bufferCount = CalculateDXGISwapChainBufferCount((newFullscreenState == TRUE), m_vsyncType);
    Log_DevPrintf("D3D12RendererOutputBuffer::InternalResizeBuffers: New buffer count = %u", bufferCount);

    // invoke resize
    hResult = m_pDXGISwapChain->ResizeBuffers(bufferCount, width, height, m_backBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
    if (FAILED(hResult))
        Panic("D3D12RendererOutputBuffer::ResizeBuffers: IDXGISwapChain::ResizeBuffers failed.");

    // update attributes
    m_width = width;
    m_height = height;
    m_vsyncType = vsyncType;

    // recreate textures
    if (!InternalCreateBuffers())
        Panic("D3D12RendererOutputBuffer::ResizeBuffers: Failed to recreate texture objects on resized swap chain.");
}


bool D3D12RendererOutputBuffer::InternalCreateBuffers()
{
//     DebugAssert(m_pRenderTargetView == NULL && m_pDepthStencilBuffer == NULL && m_pDepthStencilView == NULL);
// 
//     // access the backbuffer texture
//     ID3D11Texture2D *pBackBufferTexture;
//     HRESULT hResult = m_pDXGISwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&pBackBufferTexture));
//     if (FAILED(hResult))
//     {
//         Log_ErrorPrintf("D3D12RendererOutputBuffer::CreateRTVAndDepthStencilBuffer: IDXGISwapChain::GetBuffer failed with hResult %08X", hResult);
//         return false;
//     }
// 
//     // get d3d texture desc
//     D3D11_TEXTURE2D_DESC backBufferDesc;
//     pBackBufferTexture->GetDesc(&backBufferDesc);
// 
//     // fixup dimensions
//     m_width = backBufferDesc.Width;
//     m_height = backBufferDesc.Height;
// 
//     // create RTV
//     D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
//     rtvDesc.Format = backBufferDesc.Format;
//     rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
//     rtvDesc.Texture2D.MipSlice = 0;
// 
//     ID3D11RenderTargetView *pRenderTargetView;
//     hResult = m_pD3DDevice->CreateRenderTargetView(pBackBufferTexture, &rtvDesc, &pRenderTargetView);
//     if (FAILED(hResult))
//     {
//         Log_ErrorPrintf("D3D12RendererOutputBuffer::CreateRTVAndDepthStencilBuffer: CreateRenderTargetView failed with hResult %08X", hResult);
//         pBackBufferTexture->Release();
//         return false;
//     }
// 
//     // create depth stencil
//     ID3D11Texture2D *pDepthStencilBuffer = NULL;
//     ID3D11DepthStencilView *pDepthStencilView = NULL;
//     if (m_depthStencilBufferFormat != DXGI_FORMAT_UNKNOWN)
//     {
//         D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
//         Y_memcpy(&depthStencilBufferDesc, &backBufferDesc, sizeof(depthStencilBufferDesc));
//         depthStencilBufferDesc.Format = m_depthStencilBufferFormat;
//         depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
//         depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
//         depthStencilBufferDesc.CPUAccessFlags = 0;
//         depthStencilBufferDesc.MiscFlags = 0;
// 
//         hResult = m_pD3DDevice->CreateTexture2D(&depthStencilBufferDesc, NULL, &pDepthStencilBuffer);
//         if (FAILED(hResult))
//         {
//             Log_ErrorPrintf("D3D12RendererOutputBuffer::CreateRTVAndDepthStencilBuffer: CreateTexture2D for DS failed with hResult %08X", hResult);
//             pRenderTargetView->Release();
//             pBackBufferTexture->Release();
//             return false;
//         }
// 
//         D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
//         dsvDesc.Format = m_depthStencilBufferFormat;
//         dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
//         dsvDesc.Flags = 0;
//         dsvDesc.Texture2D.MipSlice = 0;
// 
//         hResult = m_pD3DDevice->CreateDepthStencilView(pDepthStencilBuffer, &dsvDesc, &pDepthStencilView);
//         if (FAILED(hResult))
//         {
//             Log_ErrorPrintf("D3D12RendererOutputBuffer::CreateRTVAndDepthStencilBuffer: CreateDepthStencilView failed with hResult %08X", hResult);
//             pDepthStencilBuffer->Release();
//             pRenderTargetView->Release();
//             pBackBufferTexture->Release();
//             return false;
//         }
//     }
// 
// #ifdef Y_BUILD_CONFIG_DEBUG
//     D3D11Helpers::SetD3D11DeviceChildDebugName(pBackBufferTexture, String::FromFormat("SwapChain backbuffer for hWnd %08X", (uint32)m_hWnd));
//     D3D11Helpers::SetD3D11DeviceChildDebugName(pRenderTargetView, String::FromFormat("SwapChain RTV for backbuffer for hWnd %08X", (uint32)m_hWnd));
//     if (m_pDepthStencilBuffer != NULL)
//     {
//         D3D11Helpers::SetD3D11DeviceChildDebugName(pBackBufferTexture, String::FromFormat("SwapChain depthstencil for hWnd %08X", (uint32)m_hWnd));
//         D3D11Helpers::SetD3D11DeviceChildDebugName(pRenderTargetView, String::FromFormat("SwapChainRTV for depthstencil for hWnd %08X", (uint32)m_hWnd));
//     }
// #endif
// 
//     m_pBackBufferTexture = pBackBufferTexture;
//     m_pRenderTargetView = pRenderTargetView;
//     m_pDepthStencilBuffer = pDepthStencilBuffer;
//     m_pDepthStencilView = pDepthStencilView;
//     return true;
    return false;
}

void D3D12RendererOutputBuffer::InternalReleaseBuffers()
{
//     SAFE_RELEASE(m_pDepthStencilView);
//     SAFE_RELEASE(m_pDepthStencilBuffer);
//     SAFE_RELEASE(m_pRenderTargetView);
//     SAFE_RELEASE(m_pBackBufferTexture);
}

bool D3D12RendererOutputBuffer::ResizeBuffers(uint32 width /* = 0 */, uint32 height /* = 0 */)
{
    if (width == 0 || height == 0)
    {
        // get the new size of the window
        RECT clientRect;
        GetClientRect(m_hWnd, &clientRect);

        // changed?
        width = Max(clientRect.right - clientRect.left, (LONG)1);
        height = Max(clientRect.bottom - clientRect.top, (LONG)1);
    }

    if (width == m_width && height == m_height)
        return true;

//     // ensure we are not referenced by the context
//     // FIXME!
//     D3D11GPUContext *pOwnerContext = static_cast<D3D11GPUContext *>(g_pRenderer->GetMainContext());
//     bool wasBoundByContext = false;
//     if (pOwnerContext != nullptr)
//     {
//         GPURenderTargetView *pRenderTarget;
//         GPUDepthStencilBufferView *pDepthStencilBuffer;
//         uint32 nTargets = pOwnerContext->GetRenderTargets(1, &pRenderTarget, &pDepthStencilBuffer);
//         if (nTargets == 0 && pDepthStencilBuffer == nullptr)
//         {
//             pOwnerContext->GetD3DContext()->OMSetRenderTargets(0, nullptr, nullptr);
//             wasBoundByContext = true;
//         }
//     }

    InternalResizeBuffers(width, height, m_vsyncType);

//     // rebind again
//     if (wasBoundByContext)
//         pOwnerContext->GetD3DContext()->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

    return true;
}

void D3D12RendererOutputBuffer::SetVSyncType(RENDERER_VSYNC_TYPE vsyncType)
{
    if (m_vsyncType == vsyncType)
        return;

    if (vsyncType == RENDERER_VSYNC_TYPE_TRIPLE_BUFFERING || m_vsyncType == RENDERER_VSYNC_TYPE_TRIPLE_BUFFERING)
        InternalResizeBuffers(m_width, m_height, vsyncType);
    else
        m_vsyncType = vsyncType;
}

void D3D12RendererOutputBuffer::SwapBuffers()
{
    m_pDXGISwapChain->Present((m_vsyncType == RENDERER_VSYNC_TYPE_NONE) ? 0 : 1, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

D3D12RendererOutputWindow::D3D12RendererOutputWindow(SDL_Window *pSDLWindow, D3D12RendererOutputBuffer *pBuffer, RENDERER_FULLSCREEN_STATE fullscreenState)
    : RendererOutputWindow(pSDLWindow, pBuffer, fullscreenState),
      m_pD3DOutputBuffer(pBuffer)
{

}

D3D12RendererOutputWindow::~D3D12RendererOutputWindow()
{
    // remove the window binding
    SDL_SetWindowData(m_pSDLWindow, SDL_D3D11_RENDERER_OUTPUT_WINDOW_POINTER_STRING, nullptr);

    // if we are in fullscreen state, dxgi requires we switch back to windowed before releasing the swap chain
    if (m_fullscreenState == RENDERER_FULLSCREEN_STATE_FULLSCREEN)
    {
        Log_DevPrint("D3D12RendererOutputWindow::~D3D12RendererOutputWindow: Switching back to windowed mode before destroying swap chain.");

        // change fullscreen state back
        HRESULT hResult = m_pD3DOutputBuffer->GetDXGISwapChain()->SetFullscreenState(FALSE, NULL);
        if (FAILED(hResult))
            Log_WarningPrintf("D3D12RendererOutputWindow::~D3D12RendererOutputWindow: Windowed switch at release failed with hResult %08X", hResult);
    }
}

D3D12RendererOutputWindow *D3D12RendererOutputWindow::Create(IDXGIFactory3 *pDXGIFactory, IDXGIAdapter3 *pDXGIAdapter, ID3D12Device *pD3DDevice, const char *windowCaption, uint32 windowWidth, uint32 windowHeight, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType, bool visible)
{
    HRESULT hResult;

    // Determine SDL window flags
    uint32 sdlWindowFlags = SDL_WINDOW_RESIZABLE;
    if (visible)
        sdlWindowFlags |= SDL_WINDOW_SHOWN;
    else
        sdlWindowFlags |= SDL_WINDOW_HIDDEN;

    // Create the SDL window
    SDL_Window *pSDLWindow = SDL_CreateWindow(windowCaption, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, sdlWindowFlags);

    // Created?
    if (pSDLWindow == nullptr)
    {
        Log_ErrorPrintf("D3D12RendererOutputWindow::Create: SDL_CreateWindow failed: %s", SDL_GetError());
        return nullptr;
    }

    // retreive the hwnd from the sdl window
    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    if (!SDL_GetWindowWMInfo(pSDLWindow, &info))
    {
        Log_ErrorPrintf("D3D12RendererOutputWindow::Create: SDL_GetWindowWMInfo failed: %s", SDL_GetError());
        SDL_DestroyWindow(pSDLWindow);
        return nullptr;
    }

    // create swap chain depending on window type
    IDXGISwapChain1 *pDXGISwapChain1;
    if (info.subsystem == SDL_SYSWM_WINDOWS)
    {
        // setup swap chain desc
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
        Y_memzero(&swapChainDesc, sizeof(swapChainDesc));
        swapChainDesc.Width = windowWidth;
        swapChainDesc.Height = windowHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = CalculateDXGISwapChainBufferCount(false, vsyncType);
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = 0;

        // create the swapchain
        hResult = pDXGIFactory->CreateSwapChainForHwnd(pD3DDevice, info.info.win.window, &swapChainDesc, nullptr, nullptr, &pDXGISwapChain1);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D12RendererOutputWindow::Create: CreateSwapChainForHwnd failed with hResult %08X.", hResult);
            SDL_DestroyWindow(pSDLWindow);
            return nullptr;
        }

        // disable alt+enter, we handle it elsewhere
        hResult = pDXGIFactory->MakeWindowAssociation(info.info.win.window, DXGI_MWA_NO_ALT_ENTER);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D12RendererOutputWindow::Create: MakeWindowAssociation failed with hResult %08X.", hResult);
            pDXGISwapChain1->Release();
            SDL_DestroyWindow(pSDLWindow);
            return nullptr;
        }
    }
    else
    {
        Log_ErrorPrintf("D3D12RendererOutputWindow::Create: Unhandled syswm: %u", (uint32)info.subsystem);
        SDL_DestroyWindow(pSDLWindow);
        return nullptr;
    }

    // query swapchain3 interface
    IDXGISwapChain3 *pDXGISwapChain;
    if (FAILED((hResult = pDXGISwapChain1->QueryInterface(__uuidof(IDXGISwapChain3), (void **)&pDXGISwapChain))))
    {
        Log_ErrorPrintf("D3D12RendererOutputWindow::Create: QueryInterface(IDXGISwapChain3) failed with hResult %08X.", hResult);
        return false;
    }

    // create the buffer
    D3D12RendererOutputBuffer *pOutputBuffer;
    {
        pD3DDevice->AddRef();
        pOutputBuffer = new D3D12RendererOutputBuffer(pD3DDevice, pDXGISwapChain, info.info.win.window, windowWidth, windowHeight, backBufferFormat, depthStencilBufferFormat, vsyncType);
        if (!pOutputBuffer->InternalCreateBuffers())
        {
            Log_ErrorPrintf("D3D11GPUManagedSwapChain::Create: CreateRTVAndDepthStencilBuffer failed");
            SDL_DestroyWindow(pSDLWindow);
            return nullptr;
        }
    }

    // create the output window
    D3D12RendererOutputWindow *pOutputWindow = new D3D12RendererOutputWindow(pSDLWindow, pOutputBuffer, RENDERER_FULLSCREEN_STATE_WINDOWED);

    // bind the output window to the sdl window
    SDL_SetWindowData(pSDLWindow, SDL_D3D11_RENDERER_OUTPUT_WINDOW_POINTER_STRING, reinterpret_cast<void *>(pOutputWindow));

    // done
    return pOutputWindow;
}

bool D3D12RendererOutputWindow::SetFullScreen(RENDERER_FULLSCREEN_STATE state, uint32 width /* = 0 */, uint32 height /* = 0 */)
{
    D3D12Renderer *pRenderer = static_cast<D3D12Renderer *>(g_pRenderer);
    Log_DevPrintf("D3D11GPUManagedSwapChain::SetFullScreen: State = %u, width = %u, height = %u", (uint32)state, width, height);

    // are we bound to the pipeline?
    bool switchResult = true;
//     bool wasBoundToPipeline = false;
//     D3D11GPUContext *pGPUContext = static_cast<D3D11GPUContext *>(GPUContext::GetContextForCurrentThread());
//     {
//         GPURenderTargetView *pRenderTarget;
//         GPUDepthStencilBufferView *pDepthStencilBuffer;
//         uint32 nTargets = pGPUContext->GetRenderTargets(1, &pRenderTarget, &pDepthStencilBuffer);
//         if (nTargets == 0 && pDepthStencilBuffer == nullptr)
//         {
//             pGPUContext->GetD3DContext()->OMSetRenderTargets(0, nullptr, nullptr);
//             wasBoundToPipeline = true;
//         }
//     }

    // release the buffers
    m_pD3DOutputBuffer->InternalReleaseBuffers();

    // new width/height
    uint32 newWidth = (width == 0) ? m_pD3DOutputBuffer->GetWidth() : width;
    uint32 newHeight = (height == 0) ? m_pD3DOutputBuffer->GetHeight() : height;

    // branch out depending on state
    if (state == RENDERER_FULLSCREEN_STATE_FULLSCREEN)
    {
        HRESULT hResult;

        // get output
        IDXGIOutput *pOutput;
        hResult = pRenderer->GetDXGIAdapter()->EnumOutputs(0, &pOutput);
        if (SUCCEEDED(hResult))
        {
            // fill in mode details with requested width/height
            DXGI_MODE_DESC modeDesc;
            Y_memzero(&modeDesc, sizeof(modeDesc));
            modeDesc.Width = width;
            modeDesc.Height = height;
            modeDesc.Format = m_pD3DOutputBuffer->GetBackBufferFormat();

            // find the best mode match
            DXGI_MODE_DESC closestMatch;
            hResult = pOutput->FindClosestMatchingMode(&modeDesc, &closestMatch, pRenderer->GetD3DDevice());
            if (SUCCEEDED(hResult))
            {
                // update dimensions with closest match
                Log_InfoPrintf("D3D12RendererOutputWindow::SetFullScreen: Switching to closest mode match %ux%u RefreshRate %u/%u", closestMatch.Width, closestMatch.Height, closestMatch.RefreshRate.Numerator, closestMatch.RefreshRate.Denominator);
                newWidth = closestMatch.Width;
                newHeight = closestMatch.Height;

                // switch to fullscreen state
                if (m_fullscreenState != RENDERER_FULLSCREEN_STATE_FULLSCREEN)
                {
                    hResult = m_pD3DOutputBuffer->GetDXGISwapChain()->SetFullscreenState(TRUE, pOutput);
                    if (FAILED(hResult))
                    {
                        Log_ErrorPrintf("D3D12RendererOutputWindow::SetFullScreen: IDXGISwapChain::SetFullscreenState failed with hResult %08X.", hResult);
                        switchResult = false;
                    }
                }
                else if (m_fullscreenState == RENDERER_FULLSCREEN_STATE_WINDOWED_FULLSCREEN)
                {
                    // if currently windowed fullscreen, remove that flag
                    SDL_SetWindowFullscreen(m_pSDLWindow, 0);
                }

                // resize the target
                if (switchResult)
                {
                    // call ResizeTarget to fix up the window size
                    hResult = m_pD3DOutputBuffer->GetDXGISwapChain()->ResizeTarget(&closestMatch);
                    if (FAILED(hResult))
                    {
                        Log_ErrorPrintf("D3D12RendererOutputWindow::SetFullScreen: IDXGISwapChain::ResizeTarget failed with hResult %08X.", hResult);
                        switchResult = false;
                    }
                }
            }
            else
            {
                Log_ErrorPrintf("D3D12RendererOutputWindow::SetFullScreen: IDXGIOutput::FindClosestMatchingMode failed with hResult %08X.", hResult);
                switchResult = false;
            }

            pOutput->Release();
        }
        else
        {
            Log_ErrorPrintf("D3D12RendererOutputWindow::SetFullScreen: IDXGIAdapter::EnumOutputs failed with hResult %08X.", hResult);
            switchResult = false;
        }
    }
    else
    {
        // if the current state is fullscreen, and we are switching to anything else, we have to lose the d3d exclusivity
        if (m_fullscreenState == RENDERER_FULLSCREEN_STATE_FULLSCREEN)
        {
            // switch out
            HRESULT hResult = m_pD3DOutputBuffer->GetDXGISwapChain()->SetFullscreenState(FALSE, NULL);
            if (FAILED(hResult))
            {
                Log_ErrorPrintf("D3D12RendererOutputWindow::SetFullScreen: IDXGISwapChain::SetFullscreenState failed with hResult %08X.", hResult);
                switchResult = false;
            }
        }

        if (switchResult)
        {
            if (state == RENDERER_FULLSCREEN_STATE_WINDOWED_FULLSCREEN)
            {
                if (m_fullscreenState != RENDERER_FULLSCREEN_STATE_WINDOWED_FULLSCREEN)
                {
                    // set sdl window state
                    SDL_SetWindowFullscreen(m_pSDLWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);

                    // get the new dimensions
                    SDL_GetWindowSize(m_pSDLWindow, reinterpret_cast<int *>(&newWidth), reinterpret_cast<int *>(&newHeight));

                    // change state
                    m_fullscreenState = RENDERER_FULLSCREEN_STATE_WINDOWED_FULLSCREEN;
                }
            }
            else
            {
                // disable this in sdl
                if (m_fullscreenState == RENDERER_FULLSCREEN_STATE_WINDOWED_FULLSCREEN)
                {
                    SDL_SetWindowFullscreen(m_pSDLWindow, 0);
                    m_fullscreenState = RENDERER_FULLSCREEN_STATE_WINDOWED;
                }

                // change the window dimensions
                SDL_SetWindowSize(m_pSDLWindow, newWidth, newHeight);
                SDL_GetWindowSize(m_pSDLWindow, reinterpret_cast<int *>(&newWidth), reinterpret_cast<int *>(&newHeight));
            }
        }
    }

    // fix up dimensions
    if (switchResult)
    {
        m_width = newWidth;
        m_height = newHeight;
    }

    // update internal backbuffer
    m_pD3DOutputBuffer->InternalResizeBuffers(m_width, m_height, m_pD3DOutputBuffer->GetVSyncType());

//     // were we bound to the pipeline?
//     if (wasBoundToPipeline)
//         pGPUContext->GetD3DContext()->OMSetRenderTargets(1, &m_pD3DOutputBuffer->m_pRenderTargetView, m_pD3DOutputBuffer->m_pDepthStencilView);

    return switchResult;
}

int D3D12RendererOutputWindow::PendingSDLEventFilter(void *pUserData, SDL_Event *pEvent)
{
    switch (pEvent->type)
    {
    case SDL_WINDOWEVENT:
        {
            // Is it an event we care about?
            switch (pEvent->window.event)
            {
            case SDL_WINDOWEVENT_RESIZED:
                {
                    // Get the window pointer
                    SDL_Window *pWindow = SDL_GetWindowFromID(pEvent->window.windowID);
                    D3D12RendererOutputWindow *pOutputWindow;
                    if (pWindow != nullptr && (pOutputWindow = reinterpret_cast<D3D12RendererOutputWindow *>(SDL_GetWindowData(pWindow, SDL_D3D11_RENDERER_OUTPUT_WINDOW_POINTER_STRING))) != NULL)
                    {
                        // Handle the event
                        switch (pEvent->window.event)
                        {
                        case SDL_WINDOWEVENT_RESIZED:
                            {
                                // Check the fullscreen state of the swap chain
                                if (pOutputWindow->GetFullscreenState() == RENDERER_FULLSCREEN_STATE_FULLSCREEN)
                                {
                                    BOOL newFullscreenState;
                                    HRESULT hResult = pOutputWindow->GetD3DOutputBuffer()->GetDXGISwapChain()->GetFullscreenState(&newFullscreenState, nullptr);
                                    if (SUCCEEDED(hResult) && !newFullscreenState)
                                    {
                                        Log_WarningPrintf("D3D12RendererOutputWindow::PendingSDLEventFilter: Fullscreen state lost outside our control. Updating internal state.");
                                        pOutputWindow->m_fullscreenState = RENDERER_FULLSCREEN_STATE_WINDOWED;
                                    }
                                    else
                                    {
                                        Log_WarningPrintf("D3D12RendererOutputWindow::PendingSDLEventFilter: IDXGISwapChain::GetFullscreenState failed with hResult %08X.", hResult);
                                    }
                                }

                                // update window dimensions
                                pOutputWindow->m_width = pEvent->window.data1;
                                pOutputWindow->m_height = pEvent->window.data2;

                                // Issue buffer resize
                                pOutputWindow->GetD3DOutputBuffer()->ResizeBuffers(pEvent->window.data1, pEvent->window.data2);
                            }
                            break;
                        }
                    }
                }
                break;
            }
        }
    }

    // don't delete any events
    return 1;
}

void D3D12Renderer::HandlePendingSDLEvents()
{
    SDL_FilterEvents(&D3D12RendererOutputWindow::PendingSDLEventFilter, nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RendererOutputBuffer *D3D12Renderer::CreateOutputBuffer(RenderSystemWindowHandle hWnd, RENDERER_VSYNC_TYPE vsyncType)
{
    return D3D12RendererOutputBuffer::Create(m_pDXGIFactory, m_pD3DDevice, hWnd, m_swapChainBackBufferFormat, m_swapChainDepthStencilBufferFormat, vsyncType);
}

RendererOutputWindow *D3D12Renderer::CreateOutputWindow(const char *windowTitle, uint32 windowWidth, uint32 windowHeight, RENDERER_VSYNC_TYPE vsyncType)
{
    //return D3D12RendererOutputWindow::Create(m_pDXGIFactory, m_pDXGIAdapter, m_pD3DDevice, windowTitle, windowWidth, windowHeight, m_swapChainBackBufferFormat, m_swapChainDepthStencilBufferFormat, vsyncType, true);
    return nullptr;
}

