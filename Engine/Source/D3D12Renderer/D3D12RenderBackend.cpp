#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12RenderBackend.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUContext.h"
#include "D3D12Renderer/D3D12GPUOutputBuffer.h"
#include "Engine/EngineCVars.h"
Log_SetChannel(D3D12GPUDevice);

D3D12RenderBackend *D3D12RenderBackend::s_pInstance = nullptr;
static HMODULE s_hD3D12DLL = nullptr;

D3D12RenderBackend::D3D12RenderBackend()
    : m_pDXGIFactory(nullptr)
    , m_pDXGIAdapter(nullptr)
    , m_pD3DDevice(nullptr)
    , m_D3DFeatureLevel(D3D_FEATURE_LEVEL_11_0)
    , m_featureLevel(RENDERER_FEATURE_LEVEL_COUNT)
    , m_texturePlatform(NUM_TEXTURE_PLATFORMS)
    , m_outputBackBufferFormat(DXGI_FORMAT_UNKNOWN)
    , m_outputDepthStencilFormat(DXGI_FORMAT_UNKNOWN)
    , m_pGPUDevice(nullptr)
    , m_pGPUContext(nullptr)
{
    Y_memzero(m_pDescriptorHeaps, sizeof(m_pDescriptorHeaps));
}

D3D12RenderBackend::~D3D12RenderBackend()
{
    DebugAssert(s_pInstance == this);
    s_pInstance = nullptr;
}

bool D3D12RenderBackend::Create(const RendererInitializationParameters *pCreateParameters, SDL_Window *pSDLWindow, RenderBackend **ppBackend, GPUDevice **ppDevice, GPUContext **ppContext, GPUOutputBuffer **ppOutputBuffer)
{
    HRESULT hResult;

    // load d3d12 library
    DebugAssert(s_hD3D12DLL == nullptr);
    s_hD3D12DLL = LoadLibraryA("d3d12.dll");
    if (s_hD3D12DLL == nullptr)
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Failed to load d3d12.dll library.");
        return false;
    }

    // get function entry points
    PFN_D3D12_CREATE_DEVICE fnD3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(s_hD3D12DLL, "D3D12CreateDevice");
    PFN_D3D12_GET_DEBUG_INTERFACE fnD3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(s_hD3D12DLL, "D3D12GetDebugInterface");
    if (fnD3D12CreateDevice == nullptr || fnD3D12GetDebugInterface == nullptr)
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Missing D3D12 device entry points.");
        return false;
    }

    // select formats
    m_outputBackBufferFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(pCreateParameters->BackBufferFormat);
    m_outputDepthStencilFormat = (pCreateParameters->DepthStencilBufferFormat != PIXEL_FORMAT_UNKNOWN) ? D3D11TypeConversion::PixelFormatToDXGIFormat(pCreateParameters->DepthStencilBufferFormat) : DXGI_FORMAT_UNKNOWN;
    if (m_outputBackBufferFormat == DXGI_FORMAT_UNKNOWN || (pCreateParameters->DepthStencilBufferFormat != PIXEL_FORMAT_UNKNOWN && m_outputDepthStencilFormat == DXGI_FORMAT_UNKNOWN))
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Invalid swap chain format (%s / %s)", NameTable_GetNameString(NameTables::PixelFormat, pCreateParameters->BackBufferFormat), NameTable_GetNameString(NameTables::PixelFormat, pCreateParameters->DepthStencilBufferFormat));
        return false;
    }

//     // determine driver type
//     D3D_DRIVER_TYPE driverType;
//     if (CVars::r_d3d12_force_ref.GetBool())
//         driverType = D3D_DRIVER_TYPE_REFERENCE;
//     else if (CVars::r_d3d12_force_warp.GetBool())
//         driverType = D3D_DRIVER_TYPE_WARP;
//     else
//         driverType = D3D_DRIVER_TYPE_HARDWARE;

    // todo: get adapter based on driver type
    hResult = CreateDXGIFactory2((CVars::r_use_debug_device.GetBool()) ? DXGI_CREATE_FACTORY_DEBUG : 0, __uuidof(IDXGIFactory3), (void **)&m_pDXGIFactory);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Failed to create DXGI factory with hResult %08X", hResult);
        return false;
    }

    // iterate over adapters
    for (uint32 adapterIndex = 0; ; adapterIndex++)
    {
        IDXGIAdapter *pAdapter;
        hResult = m_pDXGIFactory->EnumAdapters(adapterIndex, &pAdapter);
        if (hResult == DXGI_ERROR_NOT_FOUND)
            break;

        if (SUCCEEDED(hResult))
        {
            hResult = pAdapter->QueryInterface(__uuidof(IDXGIAdapter3), (void **)&m_pDXGIAdapter);
            if (FAILED(hResult))
            {
                Log_ErrorPrintf("D3D12RenderBackend::Create: IDXGIAdapter::QueryInterface(IDXGIAdapter3) failed.");
                pAdapter->Release();
                return false;
            }

            pAdapter->Release();
            break;
        }

        Log_WarningPrintf("IDXGIFactory::EnumAdapters(%u) failed with hResult %08X", hResult);
    }

    // found an adapter
    if (m_pDXGIAdapter == nullptr)
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Failed to find an acceptable adapter.", hResult);
        return false;
    }

    // print device name
    {
        // get adapter desc
        DXGI_ADAPTER_DESC DXGIAdapterDesc;
        hResult = m_pDXGIAdapter->GetDesc(&DXGIAdapterDesc);
        DebugAssert(hResult == S_OK);

        char deviceName[128];
        WideCharToMultiByte(CP_ACP, 0, DXGIAdapterDesc.Description, -1, deviceName, countof(deviceName), NULL, NULL);
        Log_InfoPrintf("D3D12 render backend using DXGI Adapter: %s.", deviceName);
    }

    // acquire debug interface
    if (CVars::r_use_debug_device.GetBool())
    {
        Log_PerfPrintf("Enabling Direct3D 12 debug layer, performance will suffer as a result.");
        ID3D12Debug *pD3D12Debug;
        hResult = fnD3D12GetDebugInterface(__uuidof(ID3D12Debug), (void **)&pD3D12Debug);
        if (SUCCEEDED(hResult))
        {
            pD3D12Debug->EnableDebugLayer();
            pD3D12Debug->Release();
        }
        else
        {
            Log_WarningPrintf("D3D12GetDebugInterface failed with hResult %08X. Debug layer will not be enabled.", hResult);
        }
    }

    // create the device
    hResult = fnD3D12CreateDevice(m_pDXGIAdapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void **)&m_pD3DDevice);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Could not create D3D12 device: %08X.", hResult);
        return false;
    }

    // query feature levels
    static const D3D_FEATURE_LEVEL requestedFeatureLevels[] = { D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelsData = { countof(requestedFeatureLevels), requestedFeatureLevels, D3D_FEATURE_LEVEL_11_0 };
    hResult = m_pD3DDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelsData, sizeof(featureLevelsData));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS) failed with hResult %08X", hResult);
        return false;
    }

    // print feature level
    m_D3DFeatureLevel = featureLevelsData.MaxSupportedFeatureLevel;
    Log_DevPrintf("D3D12RenderBackend::Create: Driver supports feature level %s (%X)", NameTable_GetNameString(NameTables::D3DFeatureLevels, m_D3DFeatureLevel), m_D3DFeatureLevel);

    // map to our range
    if (m_D3DFeatureLevel >= D3D_FEATURE_LEVEL_11_0)
        m_featureLevel = RENDERER_FEATURE_LEVEL_SM5;
    else
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Feature level is insufficient");
        return false;
    }

    // find texture platform
    {
        m_texturePlatform = TEXTURE_PLATFORM_DXTC;
        Log_InfoPrintf("Texture Platform: %s", NameTable_GetNameString(NameTables::TexturePlatform, m_texturePlatform));
    }

    // other vars
    m_frameLatency = pCreateParameters->GPUFrameLatency;

    // create device wrapper class
    m_pGPUDevice = new D3D12GPUDevice(this, m_pDXGIFactory, m_pDXGIAdapter, m_pD3DDevice, m_outputBackBufferFormat, m_outputDepthStencilFormat);

    // create context wrapper class
    m_pGPUContext = new D3D12GPUContext(this, m_pGPUDevice, m_pD3DDevice);
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

    Log_InfoPrint("D3D12RenderBackend::Create: Creation successful.");
    return true;
}

void D3D12RenderBackend::Shutdown()
{
    // cleanup our objects
    SAFE_RELEASE(m_pGPUContext);
    SAFE_RELEASE(m_pGPUDevice);

    // remove descriptor heaps
    for (uint32 i = 0; i < countof(m_pDescriptorHeaps); i++)
    {
        if (m_pDescriptorHeaps[i] != nullptr)
            delete m_pDescriptorHeaps[i];
    }

    // cleanup
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

    // release handle to d3d12
    if (s_hD3D12DLL != nullptr)
    {
        FreeLibrary(s_hD3D12DLL);
        s_hD3D12DLL = nullptr;
    }

    // done
    delete this;
}

bool D3D12RenderBackend::CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat /*= NULL*/) const
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

RENDERER_PLATFORM D3D12RenderBackend::GetPlatform() const
{
    return RENDERER_PLATFORM_D3D12;
}

RENDERER_FEATURE_LEVEL D3D12RenderBackend::GetFeatureLevel() const
{
    return m_featureLevel;
}

TEXTURE_PLATFORM D3D12RenderBackend::GetTexturePlatform() const
{
    return m_texturePlatform;
}

void D3D12RenderBackend::GetCapabilities(RendererCapabilities *pCapabilities) const
{
    pCapabilities->MaxTextureAnisotropy = D3D12_MAX_MAXANISOTROPY;
    pCapabilities->MaximumVertexBuffers = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    pCapabilities->MaximumConstantBuffers = D3D12_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
    pCapabilities->MaximumTextureUnits = D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
    pCapabilities->MaximumSamplers = D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT;
    pCapabilities->MaximumRenderTargets = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
    pCapabilities->SupportsMultithreadedResourceCreation = true;
    pCapabilities->SupportsDrawBaseVertex = true;
    pCapabilities->SupportsDepthTextures = true;
    pCapabilities->SupportsTextureArrays = (m_D3DFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
    pCapabilities->SupportsCubeMapTextureArrays = (m_D3DFeatureLevel >= D3D_FEATURE_LEVEL_10_1);
    pCapabilities->SupportsGeometryShaders = (m_D3DFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
    pCapabilities->SupportsSinglePassCubeMaps = (m_D3DFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
    pCapabilities->SupportsInstancing = (m_D3DFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
}

GPUDevice *D3D12RenderBackend::CreateDeviceInterface()
{
    // @TODO
    return nullptr;
}

void D3D12RenderBackend::ScheduleResourceForDeletion(ID3D12Resource *pResource, uint32 frameNumber /*= g_pRenderer->GetFrameNumber()*/)
{

}

void D3D12RenderBackend::ScheduleDescriptorForDeletion(const D3D12DescriptorHeap::Handle *pHandle, uint32 frameNumber /*= g_pRenderer->GetFrameNumber()*/)
{

}

void D3D12RenderBackend::DeletePendingResources(uint32 frameNumber)
{

}

bool D3D12RenderBackend_Create(const RendererInitializationParameters *pCreateParameters, SDL_Window *pSDLWindow, RenderBackend **ppBackend, GPUDevice **ppDevice, GPUContext **ppContext, GPUOutputBuffer **ppOutputBuffer)
{
    D3D12RenderBackend *pBackend = new D3D12RenderBackend();
    if (!pBackend->Create(pCreateParameters, pSDLWindow, ppBackend, ppDevice, ppContext, ppOutputBuffer))
    {
        pBackend->Shutdown();
        return false;
    }

    return true;
}
