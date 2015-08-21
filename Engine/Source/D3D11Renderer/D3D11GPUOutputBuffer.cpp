#include "D3D11Renderer/PrecompiledHeader.h"
#include "D3D11Renderer/D3D11GPUOutputBuffer.h"
#include "D3D11Renderer/D3D11GPUDevice.h"
#include "Engine/SDLHeaders.h"
Log_SetChannel(D3D11GPUOutputBuffer);

static const char *SDL_D3D11_RENDERER_OUTPUT_WINDOW_POINTER_STRING = "D3D11RendererOutputBufferPtr";

static uint32 CalculateDXGISwapChainBufferCount(bool exclusiveFullscreen, RENDERER_VSYNC_TYPE vsyncType)
{
    return 1 + 
           ((IsCompositionActive() == FALSE || exclusiveFullscreen) ? 1 : 0) + 
           ((vsyncType == RENDERER_VSYNC_TYPE_TRIPLE_BUFFERING) ? 1 : 0);
}

D3D11GPUOutputBuffer::D3D11GPUOutputBuffer(ID3D11Device *pD3DDevice, IDXGISwapChain *pDXGISwapChain, HWND hWnd, uint32 width, uint32 height, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType)
    : GPUOutputBuffer(vsyncType),
      m_pD3DDevice(pD3DDevice),
      m_pDXGISwapChain(pDXGISwapChain),
      m_hWnd(hWnd),
      m_width(width),
      m_height(height),
      m_backBufferFormat(backBufferFormat),
      m_depthStencilBufferFormat(depthStencilBufferFormat),
      m_pBackBufferTexture(NULL),
      m_pRenderTargetView(NULL),
      m_pDepthStencilBuffer(NULL),
      m_pDepthStencilView(NULL)
{

}

D3D11GPUOutputBuffer::~D3D11GPUOutputBuffer()
{
    if (m_pDepthStencilView != NULL)
        m_pDepthStencilView->Release();

    if (m_pDepthStencilBuffer != NULL)
        m_pDepthStencilBuffer->Release();

    if (m_pRenderTargetView != NULL)
        m_pRenderTargetView->Release();

    if (m_pBackBufferTexture != NULL)
        m_pBackBufferTexture->Release();

    m_pDXGISwapChain->Release();
    m_pD3DDevice->Release();
}

D3D11GPUOutputBuffer *D3D11GPUOutputBuffer::Create(IDXGIFactory *pDXGIFactory, ID3D11Device *pD3DDevice, HWND hWnd, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthStencilBufferFormat, RENDERER_VSYNC_TYPE vsyncType)
{
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
        Log_ErrorPrintf("D3D11GPUOutputBuffer::Create: CreateSwapChain failed with hResult %08X", hResult);
        return false;
    }

    // create object
    pD3DDevice->AddRef();
    D3D11GPUOutputBuffer *pSwapChain = new D3D11GPUOutputBuffer(pD3DDevice, pDXGISwapChain, hWnd, width, height, backBufferFormat, depthStencilBufferFormat, vsyncType);
    if (!pSwapChain->InternalCreateBuffers())
    {
        Log_ErrorPrintf("D3D11GPUOutputBuffer::Create: CreateRTVAndDepthStencilBuffer failed");
        pSwapChain->Release();
        return NULL;
    }

    return pSwapChain;
}

void D3D11GPUOutputBuffer::InternalResizeBuffers(uint32 width, uint32 height, RENDERER_VSYNC_TYPE vsyncType)
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
    Log_DevPrintf("D3D11GPUOutputBuffer::InternalResizeBuffers: New buffer count = %u", bufferCount);

    // invoke resize
    hResult = m_pDXGISwapChain->ResizeBuffers(bufferCount, width, height, m_backBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
    if (FAILED(hResult))
        Panic("D3D11GPUOutputBuffer::ResizeBuffers: IDXGISwapChain::ResizeBuffers failed.");

    // update attributes
    m_width = width;
    m_height = height;
    m_vsyncType = vsyncType;

    // recreate textures
    if (!InternalCreateBuffers())
        Panic("D3D11GPUOutputBuffer::ResizeBuffers: Failed to recreate texture objects on resized swap chain.");
}


bool D3D11GPUOutputBuffer::InternalCreateBuffers()
{
    DebugAssert(m_pRenderTargetView == NULL && m_pDepthStencilBuffer == NULL && m_pDepthStencilView == NULL);

    // access the backbuffer texture
    ID3D11Texture2D *pBackBufferTexture;
    HRESULT hResult = m_pDXGISwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&pBackBufferTexture));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUOutputBuffer::CreateRTVAndDepthStencilBuffer: IDXGISwapChain::GetBuffer failed with hResult %08X", hResult);
        return false;
    }

    // get d3d texture desc
    D3D11_TEXTURE2D_DESC backBufferDesc;
    pBackBufferTexture->GetDesc(&backBufferDesc);

    // fixup dimensions
    m_width = backBufferDesc.Width;
    m_height = backBufferDesc.Height;

    // create RTV
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    rtvDesc.Format = backBufferDesc.Format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;

    ID3D11RenderTargetView *pRenderTargetView;
    hResult = m_pD3DDevice->CreateRenderTargetView(pBackBufferTexture, &rtvDesc, &pRenderTargetView);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUOutputBuffer::CreateRTVAndDepthStencilBuffer: CreateRenderTargetView failed with hResult %08X", hResult);
        pBackBufferTexture->Release();
        return false;
    }

    // create depth stencil
    ID3D11Texture2D *pDepthStencilBuffer = NULL;
    ID3D11DepthStencilView *pDepthStencilView = NULL;
    if (m_depthStencilBufferFormat != DXGI_FORMAT_UNKNOWN)
    {
        D3D11_TEXTURE2D_DESC depthStencilBufferDesc;
        Y_memcpy(&depthStencilBufferDesc, &backBufferDesc, sizeof(depthStencilBufferDesc));
        depthStencilBufferDesc.Format = m_depthStencilBufferFormat;
        depthStencilBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        depthStencilBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthStencilBufferDesc.CPUAccessFlags = 0;
        depthStencilBufferDesc.MiscFlags = 0;

        hResult = m_pD3DDevice->CreateTexture2D(&depthStencilBufferDesc, NULL, &pDepthStencilBuffer);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUOutputBuffer::CreateRTVAndDepthStencilBuffer: CreateTexture2D for DS failed with hResult %08X", hResult);
            pRenderTargetView->Release();
            pBackBufferTexture->Release();
            return false;
        }

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
        dsvDesc.Format = m_depthStencilBufferFormat;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = 0;
        dsvDesc.Texture2D.MipSlice = 0;

        hResult = m_pD3DDevice->CreateDepthStencilView(pDepthStencilBuffer, &dsvDesc, &pDepthStencilView);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUOutputBuffer::CreateRTVAndDepthStencilBuffer: CreateDepthStencilView failed with hResult %08X", hResult);
            pDepthStencilBuffer->Release();
            pRenderTargetView->Release();
            pBackBufferTexture->Release();
            return false;
        }
    }

#ifdef Y_BUILD_CONFIG_DEBUG
    D3D11Helpers::SetD3D11DeviceChildDebugName(pBackBufferTexture, String::FromFormat("SwapChain backbuffer for hWnd %08X", (uint32)m_hWnd));
    D3D11Helpers::SetD3D11DeviceChildDebugName(pRenderTargetView, String::FromFormat("SwapChain RTV for backbuffer for hWnd %08X", (uint32)m_hWnd));
    if (m_pDepthStencilBuffer != NULL)
    {
        D3D11Helpers::SetD3D11DeviceChildDebugName(pBackBufferTexture, String::FromFormat("SwapChain depthstencil for hWnd %08X", (uint32)m_hWnd));
        D3D11Helpers::SetD3D11DeviceChildDebugName(pRenderTargetView, String::FromFormat("SwapChainRTV for depthstencil for hWnd %08X", (uint32)m_hWnd));
    }
#endif

    m_pBackBufferTexture = pBackBufferTexture;
    m_pRenderTargetView = pRenderTargetView;
    m_pDepthStencilBuffer = pDepthStencilBuffer;
    m_pDepthStencilView = pDepthStencilView;
    return true;
}

void D3D11GPUOutputBuffer::InternalReleaseBuffers()
{
    SAFE_RELEASE(m_pDepthStencilView);
    SAFE_RELEASE(m_pDepthStencilBuffer);
    SAFE_RELEASE(m_pRenderTargetView);
    SAFE_RELEASE(m_pBackBufferTexture);
}

void D3D11GPUOutputBuffer::SetVSyncType(RENDERER_VSYNC_TYPE vsyncType)
{
    if (m_vsyncType == vsyncType)
        return;

    if (vsyncType == RENDERER_VSYNC_TYPE_TRIPLE_BUFFERING || m_vsyncType == RENDERER_VSYNC_TYPE_TRIPLE_BUFFERING)
        InternalResizeBuffers(m_width, m_height, vsyncType);
    else
        m_vsyncType = vsyncType;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GPUOutputBuffer *D3D11GPUDevice::CreateOutputBuffer(RenderSystemWindowHandle hWnd, RENDERER_VSYNC_TYPE vsyncType)
{
    return D3D11GPUOutputBuffer::Create(m_pDXGIFactory, m_pD3DDevice, hWnd, m_swapChainBackBufferFormat, m_swapChainDepthStencilBufferFormat, vsyncType);
}
