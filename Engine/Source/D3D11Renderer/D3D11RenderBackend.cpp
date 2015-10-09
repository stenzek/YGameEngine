#include "D3D11Renderer/PrecompiledHeader.h"
#include "D3D11Renderer/D3D11RenderBackend.h"
#include "D3D11Renderer/D3D11GPUContext.h"
#include "D3D11Renderer/D3D11GPUBuffer.h"
#include "D3D11Renderer/D3D11GPUOutputBuffer.h"
#include "D3D11Renderer/D3D11GPUDevice.h"
#include "Engine/EngineCVars.h"
#include "Engine/SDLHeaders.h"
Log_SetChannel(D3D11Renderer);

// d3d11 libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "uxtheme.lib")

D3D11RenderBackend *D3D11RenderBackend::m_pInstance = nullptr;

D3D11RenderBackend::D3D11RenderBackend()
    : m_pDXGIFactory(nullptr)
    , m_pDXGIAdapter(nullptr)
    , m_pD3DDevice(nullptr)
    , m_pD3DDevice1(nullptr)
    , m_D3DFeatureLevel(D3D_FEATURE_LEVEL_10_0)
    , m_featureLevel(RENDERER_FEATURE_LEVEL_COUNT)
    , m_texturePlatform(NUM_TEXTURE_PLATFORMS)
    , m_swapChainBackBufferFormat(DXGI_FORMAT_UNKNOWN)
    , m_swapChainDepthStencilBufferFormat(DXGI_FORMAT_UNKNOWN)
    , m_pGPUDevice(nullptr)
    , m_pGPUContext(nullptr)
{
    DebugAssert(m_pInstance == nullptr);
    m_pInstance = this;
}

D3D11RenderBackend::~D3D11RenderBackend()
{
    DebugAssert(m_pInstance == this);
    m_pInstance = nullptr;
}

bool D3D11RenderBackend::Create(const RendererInitializationParameters *pCreateParameters, SDL_Window *pSDLWindow, RenderBackend **ppBackend, GPUDevice **ppDevice, GPUContext **ppContext, GPUOutputBuffer **ppOutputBuffer)
{
    HRESULT hResult;

    // select formats
    m_swapChainBackBufferFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(pCreateParameters->BackBufferFormat);
    m_swapChainDepthStencilBufferFormat = (pCreateParameters->DepthStencilBufferFormat != PIXEL_FORMAT_UNKNOWN) ? D3D11TypeConversion::PixelFormatToDXGIFormat(pCreateParameters->DepthStencilBufferFormat) : DXGI_FORMAT_UNKNOWN;
    if (m_swapChainBackBufferFormat == DXGI_FORMAT_UNKNOWN || (pCreateParameters->DepthStencilBufferFormat != PIXEL_FORMAT_UNKNOWN && m_swapChainDepthStencilBufferFormat == DXGI_FORMAT_UNKNOWN))
    {
        Log_ErrorPrintf("D3D11RenderBackend::Create: Invalid swap chain format (%s / %s)", NameTable_GetNameString(NameTables::PixelFormat, pCreateParameters->BackBufferFormat), NameTable_GetNameString(NameTables::PixelFormat, pCreateParameters->DepthStencilBufferFormat));
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
        Log_ErrorPrintf("D3D11RenderBackend::Create: Could not create D3D11 device: %08X.", hResult);
        return false;
    }

    // logging
    Log_DevPrintf("D3D11RenderBackend::Create: Returned a device with feature level %s.", D3D11TypeConversion::D3DFeatureLevelToString(acquiredFeatureLevel));

    // test feature levels
    if (acquiredFeatureLevel == D3D_FEATURE_LEVEL_10_0 || acquiredFeatureLevel == D3D_FEATURE_LEVEL_10_1)
    {
        m_featureLevel = RENDERER_FEATURE_LEVEL_SM4;
        m_texturePlatform = TEXTURE_PLATFORM_DXTC;
    }
    else if (acquiredFeatureLevel == D3D_FEATURE_LEVEL_11_0)
    {
        m_featureLevel = RENDERER_FEATURE_LEVEL_SM5;
        m_texturePlatform = TEXTURE_PLATFORM_DXTC;
    }
    else
    {
        Log_ErrorPrintf("D3D11RenderBackend::Create: Returned a device with an unusable feature level: %s (%08X)", D3D11TypeConversion::D3DFeatureLevelToString(acquiredFeatureLevel), acquiredFeatureLevel);
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
            Log_ErrorPrintf("D3D11RenderBackend::Create: Could not get DXGI device from D3D11 device.", hResult);
            pD3DImmediateContext->Release();
            return false;
        }

        // get the adapter from this
        hResult = pDXGIDevice->GetAdapter(&m_pDXGIAdapter);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11RenderBackend::Create: Could not get DXGI adapter from device.", hResult);
            pDXGIDevice->Release();
            pD3DImmediateContext->Release();
            return false;
        }

        // get the parent of the adapter (factory)
        hResult = m_pDXGIAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void **>(&m_pDXGIFactory));
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11RenderBackend::Create: Could not get DXGI factory from device.", hResult);
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
        Log_InfoPrintf("D3D11RenderBackend using DXGI Adapter: %s.", deviceName);
        Log_InfoPrintf("Texture Platform: %s", NameTable_GetNameString(NameTables::TexturePlatform, m_texturePlatform));
    }

    // check for threading support
    D3D11_FEATURE_DATA_THREADING threadingFeatureData;
    if (FAILED(hResult = m_pD3DDevice->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threadingFeatureData, sizeof(threadingFeatureData))))
    {
        Log_ErrorPrintf("D3D11RenderBackend::Create: CheckFeatureSupport(D3D11_FEATURE_THREADING) failed with hResult %08X", hResult);
        pD3DImmediateContext->Release();
        return false;
    }

    // cap warnings
    if (threadingFeatureData.DriverConcurrentCreates != TRUE)
        Log_WarningPrint("Direct3D device driver does not support concurrent resource creation. This may cause some stuttering during streaming.");

    // create device wrapper class
    m_pGPUDevice = new D3D11GPUDevice(m_pDXGIFactory, m_pDXGIAdapter, m_pD3DDevice, m_pD3DDevice1, m_swapChainBackBufferFormat, m_swapChainDepthStencilBufferFormat);

    // create context wrapper class
    m_pGPUContext = new D3D11GPUContext(m_pGPUDevice, m_pD3DDevice, m_pD3DDevice1, pD3DImmediateContext);
    if (!m_pGPUContext->Create())
        return false;

    // create implicit swap chain
    GPUOutputBuffer *pOutputBuffer = nullptr;
    if (pSDLWindow != nullptr)
    {
        // pass through to normal method
        pOutputBuffer = m_pGPUDevice->CreateOutputBuffer(pSDLWindow, pCreateParameters->ImplicitSwapChainVSyncType);
        if (pOutputBuffer == nullptr)
            return false;

        // bind to context
        m_pGPUContext->SetOutputBuffer(pOutputBuffer);
    }

    // add references for returned pointers
    m_pGPUDevice->AddRef();
    m_pGPUContext->AddRef();

    // set pointers
    *ppBackend = this;
    *ppDevice = m_pGPUDevice;
    *ppContext = m_pGPUContext;
    *ppOutputBuffer = pOutputBuffer;

    Log_InfoPrint("D3D11RenderBackend::Create: Creation successful.");
    return true;
}

void D3D11RenderBackend::Shutdown()
{
    // cleanup our objects
    m_pGPUContext->Release();
    m_pGPUDevice->Release();

    // cleanup
    m_pD3DDevice->Release();
    if (m_pD3DDevice1 != nullptr)
        m_pD3DDevice1->Release();
    m_pDXGIAdapter->Release();
    m_pDXGIFactory->Release();

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

    // done
    delete this;
}

bool D3D11RenderBackend::CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat /*= NULL*/) const
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

RENDERER_PLATFORM D3D11RenderBackend::GetPlatform() const
{
    return RENDERER_PLATFORM_D3D11;
}

RENDERER_FEATURE_LEVEL D3D11RenderBackend::GetFeatureLevel() const
{
    return m_featureLevel;
}

TEXTURE_PLATFORM D3D11RenderBackend::GetTexturePlatform() const
{
    return m_texturePlatform;
}

void D3D11RenderBackend::GetCapabilities(RendererCapabilities *pCapabilities) const
{
    pCapabilities->MaxTextureAnisotropy = D3D11_MAX_MAXANISOTROPY;
    pCapabilities->MaximumVertexBuffers = D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    pCapabilities->MaximumConstantBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
    pCapabilities->MaximumTextureUnits = D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
    pCapabilities->MaximumSamplers = D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
    pCapabilities->MaximumRenderTargets = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
    pCapabilities->SupportsCommandLists = false;
    pCapabilities->SupportsMultithreadedResourceCreation = true;
    pCapabilities->SupportsDrawBaseVertex = true;
    pCapabilities->SupportsDepthTextures = true;
    pCapabilities->SupportsTextureArrays = (m_featureLevel >= D3D_FEATURE_LEVEL_10_0);
    pCapabilities->SupportsCubeMapTextureArrays = (m_featureLevel >= D3D_FEATURE_LEVEL_10_1);
    pCapabilities->SupportsGeometryShaders = (m_featureLevel >= D3D_FEATURE_LEVEL_10_0);
    pCapabilities->SupportsSinglePassCubeMaps = (m_featureLevel >= D3D_FEATURE_LEVEL_10_0);
    pCapabilities->SupportsInstancing = (m_featureLevel >= D3D_FEATURE_LEVEL_10_0);
}

GPUDevice *D3D11RenderBackend::CreateDeviceInterface()
{
    // D3D11 doesn't have to do anything special with multithread resource creation, so just return the same interface
    m_pGPUDevice->AddRef();
    return m_pGPUDevice;
}

bool D3D11RenderBackend_Create(const RendererInitializationParameters *pCreateParameters, SDL_Window *pSDLWindow, RenderBackend **ppBackend, GPUDevice **ppDevice, GPUContext **ppContext, GPUOutputBuffer **ppOutputBuffer)
{
    D3D11RenderBackend *pBackend = new D3D11RenderBackend();
    if (!pBackend->Create(pCreateParameters, pSDLWindow, ppBackend, ppDevice, ppContext, ppOutputBuffer))
    {
        delete pBackend;
        return false;
    }

    return true;
}

