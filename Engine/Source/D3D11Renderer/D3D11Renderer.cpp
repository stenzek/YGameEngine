#include "D3D11Renderer/PrecompiledHeader.h"
#include "D3D11Renderer/D3D11Renderer.h"
#include "D3D11Renderer/D3D11GPUContext.h"
#include "D3D11Renderer/D3D11GPUBuffer.h"
#include "D3D11Renderer/D3D11RendererOutputBuffer.h"
#include "Engine/EngineCVars.h"
Log_SetChannel(D3D11Renderer);

// d3d11 libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "d3dcompiler.lib")     // <-- remove this

D3D11SamplerState::D3D11SamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, ID3D11SamplerState *pD3DSamplerState)
    : GPUSamplerState(pSamplerStateDesc), m_pD3DSamplerState(pD3DSamplerState)
{

}

D3D11SamplerState::~D3D11SamplerState()
{
    m_pD3DSamplerState->Release();
}

void D3D11SamplerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this) + sizeof(ID3D11SamplerState);

    // approximation
    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 128;
}

void D3D11SamplerState::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DSamplerState, name);
}

D3D11RasterizerState::D3D11RasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc, ID3D11RasterizerState *pD3DRasterizerState)
    : GPURasterizerState(pRasterizerStateDesc), m_pD3DRasterizerState(pD3DRasterizerState)
{

}

D3D11RasterizerState::~D3D11RasterizerState()
{
    m_pD3DRasterizerState->Release();
}

void D3D11RasterizerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this) + sizeof(ID3D11RasterizerState);

    // approximation
    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 128;
}

void D3D11RasterizerState::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DRasterizerState, name);
}

D3D11DepthStencilState::D3D11DepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc, ID3D11DepthStencilState *pD3DDepthStencilState)
    : GPUDepthStencilState(pDepthStencilStateDesc), m_pD3DDepthStencilState(pD3DDepthStencilState)
{

}

D3D11DepthStencilState::~D3D11DepthStencilState()
{
    m_pD3DDepthStencilState->Release();
}

void D3D11DepthStencilState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this) + sizeof(ID3D11DepthStencilState);

    // approximation
    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 128;
}

void D3D11DepthStencilState::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DDepthStencilState, name);
}

D3D11BlendState::D3D11BlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc, ID3D11BlendState *pD3DBlendState)
    : GPUBlendState(pBlendStateDesc), m_pD3DBlendState(pD3DBlendState)
{

}

D3D11BlendState::~D3D11BlendState()
{
    m_pD3DBlendState->Release();
}

void D3D11BlendState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this) + sizeof(ID3D11BlendState);

    // approximation
    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 128;
}

void D3D11BlendState::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DBlendState, name);
}


D3D11Renderer::D3D11Renderer()
    : Renderer()
{
    m_eRendererPlatform = RENDERER_PLATFORM_D3D11;
    m_fTexelOffset = 0.0f;

    m_pDXGIFactory = NULL;
    m_pDXGIAdapter = NULL;
    
    m_pD3DDevice = NULL;
    m_pD3DDevice1 = NULL;
    m_eD3DFeatureLevel = D3D_FEATURE_LEVEL_10_0;

    m_swapChainBackBufferFormat = DXGI_FORMAT_UNKNOWN;
    m_swapChainDepthStencilBufferFormat = DXGI_FORMAT_UNKNOWN;

    m_pMainContext = NULL;

    m_pImplicitRenderWindow = NULL;

    Y_memzero(&m_gpuMemoryUsage, sizeof(m_gpuMemoryUsage));
}

D3D11Renderer::~D3D11Renderer()
{
    // clear device state
    if (m_pMainContext != NULL)
        m_pMainContext->ClearState(true, true, true, true);

    SAFE_RELEASE(m_pMainContext);
    SAFE_RELEASE(m_pImplicitRenderWindow);

    SAFE_RELEASE(m_pD3DDevice1);
    SAFE_RELEASE(m_pD3DDevice);

    SAFE_RELEASE(m_pDXGIAdapter);
    SAFE_RELEASE(m_pDXGIFactory);

#ifdef Y_BUILD_CONFIG_DEBUG
    // dump remaining objects
    {
        HMODULE hDXGIDebugModule = GetModuleHandleA("dxgidebug.dll");
        if (hDXGIDebugModule != NULL)
        {
            HRESULT(WINAPI *pDXGIGetDebugInterface)(REFIID riid, void **ppDebug);
            pDXGIGetDebugInterface = (HRESULT(WINAPI *)(REFIID, void **))GetProcAddress(hDXGIDebugModule, "DXGIGetDebugInterface");
            if (pDXGIGetDebugInterface != NULL)
            {
                IDXGIDebug *pDXGIDebug;
                if (SUCCEEDED(pDXGIGetDebugInterface(__uuidof(pDXGIDebug), reinterpret_cast<void **>(&pDXGIDebug))))
                {
                    Log_DevPrint("=== Begin remaining DXGI and D3D object dump ===");
                    pDXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
                    Log_DevPrint("=== End remaining DXGI and D3D object dump ===");
                    pDXGIDebug->Release();
                }
            }
        }
    }
#endif
}

bool D3D11Renderer::Create(const RendererInitializationParameters *pInitializationParameters)
{
    HRESULT hResult;

    // select formats
    m_swapChainBackBufferFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(pInitializationParameters->BackBufferFormat);
    m_swapChainDepthStencilBufferFormat = (pInitializationParameters->DepthStencilBufferFormat != PIXEL_FORMAT_UNKNOWN) ? D3D11TypeConversion::PixelFormatToDXGIFormat(pInitializationParameters->DepthStencilBufferFormat) : DXGI_FORMAT_UNKNOWN;
    if (m_swapChainBackBufferFormat == DXGI_FORMAT_UNKNOWN || (pInitializationParameters->DepthStencilBufferFormat != PIXEL_FORMAT_UNKNOWN && m_swapChainDepthStencilBufferFormat == DXGI_FORMAT_UNKNOWN))
    {
        Log_ErrorPrintf("D3D11Renderer::Create: Invalid swap chain format (%s / %s)", NameTable_GetNameString(NameTables::PixelFormat, pInitializationParameters->BackBufferFormat), NameTable_GetNameString(NameTables::PixelFormat, pInitializationParameters->DepthStencilBufferFormat));
        return false;
    }

    // determine driver type
    D3D_DRIVER_TYPE driverType;
    if (CVars::r_d3d11_force_ref.GetBool())
        driverType = D3D_DRIVER_TYPE_REFERENCE;
    else if (CVars::r_d3d11_force_warp.GetBool())
        driverType = D3D_DRIVER_TYPE_WARP;
    else
        driverType = D3D_DRIVER_TYPE_HARDWARE;

    // feature levels
    D3D_FEATURE_LEVEL acquiredFeatureLevel;
    static const D3D_FEATURE_LEVEL requestedFeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };

    // device flags
    UINT deviceFlags = 0;
    if (CVars::r_use_debug_device.GetBool())
    {
        Log_PerfPrintf("Creating a debug Direct3D 11 device, performance will suffer as a result.");
        deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    // create the device
    ID3D11DeviceContext *pD3DImmediateContext;
    hResult = D3D11CreateDevice(nullptr,
                                driverType,
                                nullptr,
                                deviceFlags,
                                requestedFeatureLevels,
                                countof(requestedFeatureLevels),
                                D3D11_SDK_VERSION,
                                &m_pD3DDevice,
                                &acquiredFeatureLevel,
                                &pD3DImmediateContext);

    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11Renderer::Create: Could not create D3D11 device chain: %08X.", hResult);
        return false;
    }

    // logging
    Log_DevPrintf("D3D11Renderer::Create: Returned a device with feature level %s.", D3D11TypeConversion::D3DFeatureLevelToString(acquiredFeatureLevel));

    // test feature levels
    if (acquiredFeatureLevel == D3D_FEATURE_LEVEL_10_0 || acquiredFeatureLevel == D3D_FEATURE_LEVEL_10_1)
    {
        m_eRendererFeatureLevel = RENDERER_FEATURE_LEVEL_SM4;
        m_eTexturePlatform = TEXTURE_PLATFORM_DXTC;
    }
    else if (acquiredFeatureLevel == D3D_FEATURE_LEVEL_11_0)
    {
        m_eRendererFeatureLevel = RENDERER_FEATURE_LEVEL_SM5;
        m_eTexturePlatform = TEXTURE_PLATFORM_DXTC;
    }
    else
    {
        Log_ErrorPrintf("D3D11Renderer::Create: Returned a device with an unusable feature level: %s (%08X)", D3D11TypeConversion::D3DFeatureLevelToString(acquiredFeatureLevel), acquiredFeatureLevel);
        pD3DImmediateContext->Release();
        return false;
    }

    // get the 11.1 device
    if (CVars::r_d3d11_use_11_1.GetBool())
    {
        if (FAILED(hResult = m_pD3DDevice->QueryInterface(&m_pD3DDevice1)))
            Log_WarningPrintf("Failed to retrieve ID3D11Device1 interface with hResult %08X. 11.1 features will not be used.", hResult);
        else
            Log_InfoPrintf("Using Direct3D 11.1 features.");
    }

    // retrieve handles to DXGI from the created device
    {
        // get a temporary handle to the dxgi device interface
        IDXGIDevice *pDXGIDevice;
        hResult = m_pD3DDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void **>(&pDXGIDevice));
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11Renderer::Create: Could not get DXGI device from D3D11 device.", hResult);
            pD3DImmediateContext->Release();
            return false;
        }

        // get the adapter from this
        hResult = pDXGIDevice->GetAdapter(&m_pDXGIAdapter);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11Renderer::Create: Could not get DXGI adapter from device.", hResult);
            pDXGIDevice->Release();
            pD3DImmediateContext->Release();
            return false;
        }

        // get the parent of the adapter (factory)
        hResult = m_pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void **>(&m_pDXGIFactory));
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11Renderer::Create: Could not get DXGI factory from device.", hResult);
            pDXGIDevice->Release();
            pD3DImmediateContext->Release();
            return false;
        }

        // dxgi device is no longer needed
        pDXGIDevice->Release();
    }

    // print device name
    {
        // get adapter desc
        DXGI_ADAPTER_DESC DXGIAdapterDesc;
        hResult = m_pDXGIAdapter->GetDesc(&DXGIAdapterDesc);
        DebugAssert(hResult == S_OK);

        char deviceName[128];
        WideCharToMultiByte(CP_ACP, 0, DXGIAdapterDesc.Description, -1, deviceName, countof(deviceName), NULL, NULL);
        Log_InfoPrintf("D3D11Renderer using DXGI Adapter: %s.", deviceName);
        Log_InfoPrintf("Texture Platform: %s", NameTable_GetNameString(NameTables::TexturePlatform, m_eTexturePlatform));
    }

    // check for threading support
    D3D11_FEATURE_DATA_THREADING threadingFeatureData;
    if (FAILED(hResult = m_pD3DDevice->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threadingFeatureData, sizeof(threadingFeatureData))))
    {
        Log_ErrorPrintf("D3D11Renderer::Create: CheckFeatureSupport(D3D11_FEATURE_THREADING) failed with hResult %08X", hResult);
        pD3DImmediateContext->Release();
        return false;
    }

    // cap warnings
    if (threadingFeatureData.DriverConcurrentCreates != TRUE)
        Log_WarningPrint("Direct3D device driver does not support concurrent resource creation. This may cause some stuttering during streaming.");

    // fill in caps
    m_RendererCapabilities.MaxTextureAnisotropy = D3D11_MAX_MAXANISOTROPY;
    m_RendererCapabilities.MaximumVertexBuffers = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    m_RendererCapabilities.MaximumConstantBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
    m_RendererCapabilities.MaximumTextureUnits = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
    m_RendererCapabilities.MaximumSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
    m_RendererCapabilities.MaximumRenderTargets = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
    m_RendererCapabilities.SupportsMultithreadedResourceCreation = true;
    m_RendererCapabilities.SupportsDrawBaseVertex = true;
    m_RendererCapabilities.SupportsDepthTextures = true;
    m_RendererCapabilities.SupportsTextureArrays = (acquiredFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
    m_RendererCapabilities.SupportsCubeMapTextureArrays = (acquiredFeatureLevel >= D3D_FEATURE_LEVEL_10_1);
    m_RendererCapabilities.SupportsGeometryShaders = (acquiredFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
    m_RendererCapabilities.SupportsSinglePassCubeMaps = (acquiredFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
    m_RendererCapabilities.SupportsInstancing = (acquiredFeatureLevel >= D3D_FEATURE_LEVEL_10_0);

    // create device wrapper class
    m_pMainContext = new D3D11GPUContext(this);
    if (!m_pMainContext->Create(pD3DImmediateContext))
        return false;

    // create implicit swap chain
    if (!pInitializationParameters->HideImplicitSwapChain)
    {
        uint32 createWidth = pInitializationParameters->ImplicitSwapChainWidth;
        uint32 createHeight = pInitializationParameters->ImplicitSwapChainHeight;
        if (pInitializationParameters->ImplicitSwapChainFullScreen)
        {
            // when creating full-screen, start windowed, then switch
            createWidth = 1;
            createHeight = 1;
        }

        // create swap shain
        m_pImplicitRenderWindow = D3D11RendererOutputWindow::Create(m_pDXGIFactory, m_pDXGIAdapter, m_pD3DDevice,
                                                                pInitializationParameters->ImplicitSwapChainCaption,
                                                                createWidth, createHeight,
                                                                m_swapChainBackBufferFormat, m_swapChainDepthStencilBufferFormat,
                                                                pInitializationParameters->ImplicitSwapChainVSyncType,
                                                                !pInitializationParameters->HideImplicitSwapChain);

        if (m_pImplicitRenderWindow == NULL)
            return false;

        // bind to the main context
        m_pMainContext->SetOutputBuffer(m_pImplicitRenderWindow->GetOutputBuffer());
    }

    Log_InfoPrint("D3D11Renderer::Create: Creation successful.");

    // switch to fullscreen
    if (!pInitializationParameters->HideImplicitSwapChain && pInitializationParameters->ImplicitSwapChainFullScreen)
    {
        Log_InfoPrint("D3D11Renderer::Create: Trying to switch to fullscreen...");
        if (!m_pImplicitRenderWindow->SetFullScreen(pInitializationParameters->ImplicitSwapChainFullScreen, pInitializationParameters->ImplicitSwapChainWidth, pInitializationParameters->ImplicitSwapChainHeight))
            Log_WarningPrint("D3D11Renderer::Create: Switching to fullscreen failed.");
    }

    return true;
}

void D3D11Renderer::CorrectProjectionMatrix(float4x4 &projectionMatrix) const
{
    //D3D11Helpers::CorrectProjectionMatrix(projectionMatrix);
}

bool D3D11Renderer::CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat /*= NULL*/) const
{
    DXGI_FORMAT DXGIFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(PixelFormat);
    if (DXGIFormat == DXGI_FORMAT_UNKNOWN)
    {
        // use r8g8b8a8
        if (CompatibleFormat != NULL)
            *CompatibleFormat = PIXEL_FORMAT_R8G8B8A8_UNORM;

        return false;
    }

    if (CompatibleFormat != NULL)
        *CompatibleFormat = PixelFormat;

    return true;
}

GPUContext *D3D11Renderer::CreateUploadContext()
{
    Panic("Fixme");
    return NULL;
}

GPUSamplerState *D3D11Renderer::CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc)
{
    ID3D11SamplerState *pD3DSamplerState = D3D11Helpers::CreateD3D11SamplerState(m_pD3DDevice, pSamplerStateDesc);
    if (pD3DSamplerState == NULL)
        return NULL;

    D3D11SamplerState *pSamplerState = new D3D11SamplerState(pSamplerStateDesc, pD3DSamplerState);
    return pSamplerState;
}

GPURasterizerState *D3D11Renderer::CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc)
{
    ID3D11RasterizerState *pD3DRasterizerState = D3D11Helpers::CreateD3D11RasterizerState(m_pD3DDevice, pRasterizerStateDesc);
    if (pD3DRasterizerState == NULL)
        return NULL;

    D3D11RasterizerState *pRasterizerState = new D3D11RasterizerState(pRasterizerStateDesc, pD3DRasterizerState);
    return pRasterizerState;
}

GPUDepthStencilState *D3D11Renderer::CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc)
{
    ID3D11DepthStencilState *pD3DDepthStencilState = D3D11Helpers::CreateD3D11DepthStencilState(m_pD3DDevice, pDepthStencilStateDesc);
    if (pD3DDepthStencilState == NULL)
        return NULL;

    D3D11DepthStencilState *pDepthStencilState = new D3D11DepthStencilState(pDepthStencilStateDesc, pD3DDepthStencilState);
    return pDepthStencilState;
}

GPUBlendState *D3D11Renderer::CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc)
{
    ID3D11BlendState *pD3DBlendState = D3D11Helpers::CreateD3D11BlendState(m_pD3DDevice, pBlendStateDesc);
    if (pD3DBlendState == NULL)
        return NULL;

    D3D11BlendState *pBlendState = new D3D11BlendState(pBlendStateDesc, pD3DBlendState);
    return pBlendState;
}

void D3D11Renderer::OnResourceCreated(GPUResource *pResource)
{
    uint32 cpuMemoryUsage, gpuMemoryUsage;
    pResource->GetMemoryUsage(&cpuMemoryUsage, &gpuMemoryUsage);

    if (pResource->GetResourceType() == GPU_RESOURCE_TYPE_BUFFER)
        Y_AtomicAdd(m_gpuMemoryUsage.BufferMemory, (size_t)gpuMemoryUsage);
}

void D3D11Renderer::OnResourceReleased(GPUResource *pResource)
{

}

void D3D11Renderer::GetGPUMemoryUsage(GPUMemoryUsage *pMemoryUsage) const
{
    Y_memcpy(pMemoryUsage, &m_gpuMemoryUsage, sizeof(GPUMemoryUsage));
}

Renderer *D3D11Renderer_CreateRenderer(const RendererInitializationParameters *pCreateParameters)
{
    DebugAssert(g_pRenderer == nullptr);

    D3D11Renderer *pRenderer = new D3D11Renderer();
    g_pRenderer = pRenderer;        // HACK, fixme! move to static counters

    if (!pRenderer->Create(pCreateParameters))
    {
        g_pRenderer = nullptr;
        delete pRenderer;
        return nullptr;
    }

    return pRenderer;
}

