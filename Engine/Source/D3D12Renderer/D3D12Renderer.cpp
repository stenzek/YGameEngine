#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12Renderer.h"
//#include "D3D12Renderer/D3D12GPUContext.h"
//#include "D3D12Renderer/D3D12GPUBuffer.h"
#include "D3D12Renderer/D3D12RendererOutputBuffer.h"
#include "Engine/EngineCVars.h"
Log_SetChannel(D3D12Renderer);

static HMODULE s_hD3D12DLL = nullptr;

D3D12SamplerState::D3D12SamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const D3D12_SAMPLER_DESC *pD3DSamplerDesc)
    : GPUSamplerState(pSamplerStateDesc)
{
    Y_memcpy(&m_D3DSamplerStateDesc, pD3DSamplerDesc, sizeof(m_D3DSamplerStateDesc));
}

D3D12SamplerState::~D3D12SamplerState()
{
    
}

void D3D12SamplerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12SamplerState::SetDebugName(const char *name)
{

}

D3D12RasterizerState::D3D12RasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc, const D3D12_RASTERIZER_DESC *pD3DRasterizerDesc)
    : GPURasterizerState(pRasterizerStateDesc)
{
    Y_memcpy(&m_D3DRasterizerDesc, pD3DRasterizerDesc, sizeof(m_D3DRasterizerDesc));
}

D3D12RasterizerState::~D3D12RasterizerState()
{
    
}

void D3D12RasterizerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12RasterizerState::SetDebugName(const char *name)
{

}

D3D12DepthStencilState::D3D12DepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc, const D3D12_DEPTH_STENCIL_DESC *pD3DDepthStencilDesc)
    : GPUDepthStencilState(pDepthStencilStateDesc)
{
    Y_memcpy(&m_D3DDepthStencilDesc, pD3DDepthStencilDesc, sizeof(m_D3DDepthStencilDesc));
}

D3D12DepthStencilState::~D3D12DepthStencilState()
{
    
}

void D3D12DepthStencilState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12DepthStencilState::SetDebugName(const char *name)
{

}

D3D12BlendState::D3D12BlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc, const D3D12_BLEND_DESC *pD3DBlendDesc)
    : GPUBlendState(pBlendStateDesc)
{
    Y_memcpy(&m_D3DBlendDesc, pD3DBlendDesc, sizeof(m_D3DBlendDesc));
}

D3D12BlendState::~D3D12BlendState()
{
    
}

void D3D12BlendState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12BlendState::SetDebugName(const char *name)
{

}


D3D12Renderer::D3D12Renderer()
    : Renderer()
{
    m_eRendererPlatform = RENDERER_PLATFORM_D3D12;
    m_fTexelOffset = 0.0f;

    m_pDXGIFactory = NULL;
    m_pDXGIAdapter = NULL;
    
    m_pD3DDevice = NULL;
    m_eD3DFeatureLevel = D3D_FEATURE_LEVEL_10_0;

    m_swapChainBackBufferFormat = DXGI_FORMAT_UNKNOWN;
    m_swapChainDepthStencilBufferFormat = DXGI_FORMAT_UNKNOWN;

    m_pMainContext = NULL;

    m_pImplicitRenderWindow = NULL;

    Y_memzero(&m_gpuMemoryUsage, sizeof(m_gpuMemoryUsage));
}

D3D12Renderer::~D3D12Renderer()
{
    // clear device state
    //if (m_pMainContext != NULL)
        //m_pMainContext->ClearState(true, true, true, true);

    //SAFE_RELEASE(m_pMainContext);
    SAFE_RELEASE(m_pImplicitRenderWindow);

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
}

bool D3D12Renderer::Create(const RendererInitializationParameters *pInitializationParameters)
{
    HRESULT hResult;

    // load d3d12 library
    DebugAssert(s_hD3D12DLL == nullptr);
    s_hD3D12DLL = LoadLibraryA("d3d12.dll");
    if (s_hD3D12DLL == nullptr)
    {
        Log_ErrorPrintf("D3D12Renderer::Create: Failed to load d3d12.dll library.");
        return false;
    }

    // get function entry points
    PFN_D3D12_CREATE_DEVICE fnD3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(s_hD3D12DLL, "D3D12CreateDevice");
    PFN_D3D12_GET_DEBUG_INTERFACE fnD3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(s_hD3D12DLL, "D3D12GetDebugInterface");
    if (fnD3D12CreateDevice == nullptr || fnD3D12GetDebugInterface == nullptr)
    {
        Log_ErrorPrintf("D3D12Renderer::Create: Missing D3D12 device entry points.");
        return false;
    }

    // select formats
    m_swapChainBackBufferFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(pInitializationParameters->BackBufferFormat);
    m_swapChainDepthStencilBufferFormat = (pInitializationParameters->DepthStencilBufferFormat != PIXEL_FORMAT_UNKNOWN) ? D3D12TypeConversion::PixelFormatToDXGIFormat(pInitializationParameters->DepthStencilBufferFormat) : DXGI_FORMAT_UNKNOWN;
    if (m_swapChainBackBufferFormat == DXGI_FORMAT_UNKNOWN || (pInitializationParameters->DepthStencilBufferFormat != PIXEL_FORMAT_UNKNOWN && m_swapChainDepthStencilBufferFormat == DXGI_FORMAT_UNKNOWN))
    {
        Log_ErrorPrintf("D3D12Renderer::Create: Invalid swap chain format (%s / %s)", NameTable_GetNameString(NameTables::PixelFormat, pInitializationParameters->BackBufferFormat), NameTable_GetNameString(NameTables::PixelFormat, pInitializationParameters->DepthStencilBufferFormat));
        return false;
    }

    // determine driver type
    D3D_DRIVER_TYPE driverType;
    if (CVars::r_d3d12_force_ref.GetBool())
        driverType = D3D_DRIVER_TYPE_REFERENCE;
    else if (CVars::r_d3d12_force_warp.GetBool())
        driverType = D3D_DRIVER_TYPE_WARP;
    else
        driverType = D3D_DRIVER_TYPE_HARDWARE;

    // todo: get adapter based on driver type
    hResult = CreateDXGIFactory2((CVars::r_use_debug_device.GetBool()) ? DXGI_CREATE_FACTORY_DEBUG : 0, __uuidof(IDXGIFactory3), (void **)&m_pDXGIFactory);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12Renderer::Create: Failed to create DXGI factory with hResult %08X", hResult);
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
                Log_ErrorPrintf("D3D12Renderer::Create: IDXGIAdapter::QueryInterface(IDXGIAdapter3) failed.");
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
        Log_ErrorPrintf("D3D12Renderer::Create: Failed to find an acceptable adapter.", hResult);
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
        Log_InfoPrintf("D3D12Renderer using DXGI Adapter: %s.", deviceName);
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
        Log_ErrorPrintf("D3D12Renderer::Create: Could not create D3D12 device: %08X.", hResult);
        return false;
    }

//     // feature levels
//     D3D_FEATURE_LEVEL acquiredFeatureLevel;
//     static const D3D_FEATURE_LEVEL requestedFeatureLevels[] =
//     {
//         D3D_FEATURE_LEVEL_11_0,
//         D3D_FEATURE_LEVEL_10_1,
//         D3D_FEATURE_LEVEL_10_0,
//     };

//     // device flags
//     UINT deviceFlags = 0;
//     if (CVars::r_use_debug_device.GetBool())
//     {
//         Log_PerfPrintf("Creating a debug Direct3D 12 device, performance will suffer as a result.");
//         deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
//     }

    // logging
    //Log_DevPrintf("D3D12Renderer::Create: Returned a device with feature level %s.", D3D12TypeConversion::D3DFeatureLevelToString(acquiredFeatureLevel));

//     // test feature levels
//     if (acquiredFeatureLevel == D3D_FEATURE_LEVEL_10_0 || acquiredFeatureLevel == D3D_FEATURE_LEVEL_10_1)
//     {
//         m_eRendererFeatureLevel = RENDERER_FEATURE_LEVEL_SM4;
//         m_eTexturePlatform = TEXTURE_PLATFORM_DXTC;
//     }
//     else if (acquiredFeatureLevel == D3D_FEATURE_LEVEL_11_0)
//     {
//         m_eRendererFeatureLevel = RENDERER_FEATURE_LEVEL_SM5;
//         m_eTexturePlatform = TEXTURE_PLATFORM_DXTC;
//     }
//     else
//     {
//         Log_ErrorPrintf("D3D12Renderer::Create: Returned a device with an unusable feature level: %s (%08X)", D3D12TypeConversion::D3DFeatureLevelToString(acquiredFeatureLevel), acquiredFeatureLevel);
//         pD3DImmediateContext->Release();
//         return false;
//     }

    // find texture platform
    {
        m_eTexturePlatform = TEXTURE_PLATFORM_DXTC;
        Log_InfoPrintf("Texture Platform: %s", NameTable_GetNameString(NameTables::TexturePlatform, m_eTexturePlatform));
    }

//     // check for threading support
//     D3D11_FEATURE_DATA_THREADING threadingFeatureData;
//     if (FAILED(hResult = m_pD3DDevice->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threadingFeatureData, sizeof(threadingFeatureData))))
//     {
//         Log_ErrorPrintf("D3D12Renderer::Create: CheckFeatureSupport(D3D11_FEATURE_THREADING) failed with hResult %08X", hResult);
//         pD3DImmediateContext->Release();
//         return false;
//     }

//     // cap warnings
//     if (threadingFeatureData.DriverConcurrentCreates != TRUE)
//         Log_WarningPrint("Direct3D device driver does not support concurrent resource creation. This may cause some stuttering during streaming.");

    // fill in caps
    m_RendererCapabilities.MaxTextureAnisotropy = D3D12_MAX_MAXANISOTROPY;
    m_RendererCapabilities.MaximumVertexBuffers = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    m_RendererCapabilities.MaximumConstantBuffers = D3D12_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
    m_RendererCapabilities.MaximumTextureUnits = D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT;
    m_RendererCapabilities.MaximumSamplers = D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT;
    m_RendererCapabilities.MaximumRenderTargets = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
    m_RendererCapabilities.SupportsMultithreadedResourceCreation = true;
    m_RendererCapabilities.SupportsDrawBaseVertex = true;
    m_RendererCapabilities.SupportsDepthTextures = true;
    //m_RendererCapabilities.SupportsTextureArrays = (acquiredFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
    //m_RendererCapabilities.SupportsCubeMapTextureArrays = (acquiredFeatureLevel >= D3D_FEATURE_LEVEL_10_1);
    //m_RendererCapabilities.SupportsGeometryShaders = (acquiredFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
    //m_RendererCapabilities.SupportsSinglePassCubeMaps = (acquiredFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
    //m_RendererCapabilities.SupportsInstancing = (acquiredFeatureLevel >= D3D_FEATURE_LEVEL_10_0);

    // create device wrapper class
    //m_pMainContext = new D3D11GPUContext(this);
    //if (!m_pMainContext->Create(pD3DImmediateContext))
        //return false;

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
        m_pImplicitRenderWindow = D3D12RendererOutputWindow::Create(m_pDXGIFactory, m_pDXGIAdapter, m_pD3DDevice,
                                                                pInitializationParameters->ImplicitSwapChainCaption,
                                                                createWidth, createHeight,
                                                                m_swapChainBackBufferFormat, m_swapChainDepthStencilBufferFormat,
                                                                pInitializationParameters->ImplicitSwapChainVSyncType,
                                                                !pInitializationParameters->HideImplicitSwapChain);

        if (m_pImplicitRenderWindow == NULL)
            return false;

        // bind to the main context
        //m_pMainContext->SetOutputBuffer(m_pImplicitRenderWindow->GetOutputBuffer());
    }

    Log_InfoPrint("D3D12Renderer::Create: Creation successful.");

    // switch to fullscreen
    if (!pInitializationParameters->HideImplicitSwapChain && pInitializationParameters->ImplicitSwapChainFullScreen)
    {
        Log_InfoPrint("D3D12Renderer::Create: Trying to switch to fullscreen...");
        if (!m_pImplicitRenderWindow->SetFullScreen(pInitializationParameters->ImplicitSwapChainFullScreen, pInitializationParameters->ImplicitSwapChainWidth, pInitializationParameters->ImplicitSwapChainHeight))
            Log_WarningPrint("D3D12Renderer::Create: Switching to fullscreen failed.");
    }

    return true;
}

void D3D12Renderer::CorrectProjectionMatrix(float4x4 &projectionMatrix) const
{
    //D3D11Helpers::CorrectProjectionMatrix(projectionMatrix);
}

bool D3D12Renderer::CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat /*= NULL*/) const
{
    DXGI_FORMAT DXGIFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(PixelFormat);
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

GPUContext *D3D12Renderer::CreateUploadContext()
{
    Panic("Fixme");
    return NULL;
}

GPUQuery *D3D12Renderer::CreateQuery(GPU_QUERY_TYPE type)
{
    return nullptr;
}

GPUBuffer *D3D12Renderer::CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData /*= NULL*/)
{
    return nullptr;
}

GPUTexture1D *D3D12Renderer::CreateTexture1D(const GPU_TEXTURE1D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= NULL*/, const uint32 *pInitialDataPitch /*= NULL*/)
{
    return nullptr;
}

GPUTexture1DArray *D3D12Renderer::CreateTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= NULL*/, const uint32 *pInitialDataPitch /*= NULL*/)
{
    return nullptr;
}

GPUTexture2D *D3D12Renderer::CreateTexture2D(const GPU_TEXTURE2D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= NULL*/, const uint32 *pInitialDataPitch /*= NULL*/)
{
    return nullptr;
}

GPUTexture2DArray *D3D12Renderer::CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= NULL*/, const uint32 *pInitialDataPitch /*= NULL*/)
{
    return nullptr;
}

GPUTexture3D *D3D12Renderer::CreateTexture3D(const GPU_TEXTURE3D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= NULL*/, const uint32 *pInitialDataPitch /*= NULL*/, const uint32 *pInitialDataSlicePitch /*= NULL*/)
{
    return nullptr;
}

GPUTextureCube *D3D12Renderer::CreateTextureCube(const GPU_TEXTURECUBE_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= NULL*/, const uint32 *pInitialDataPitch /*= NULL*/)
{
    return nullptr;
}

GPUTextureCubeArray *D3D12Renderer::CreateTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= NULL*/, const uint32 *pInitialDataPitch /*= NULL*/)
{
    return nullptr;
}

GPUDepthTexture *D3D12Renderer::CreateDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pTextureDesc)
{
    return nullptr;
}

GPUSamplerState *D3D12Renderer::CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc)
{
    return nullptr;
}

GPURenderTargetView *D3D12Renderer::CreateRenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc)
{
    return nullptr;
}

GPUDepthStencilBufferView *D3D12Renderer::CreateDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc)
{
    return nullptr;
}

GPUComputeView *D3D12Renderer::CreateComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc)
{
    return nullptr;
}

GPUDepthStencilState *D3D12Renderer::CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc)
{
    return nullptr;
}

GPURasterizerState *D3D12Renderer::CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc)
{
    return nullptr;
}

GPUBlendState *D3D12Renderer::CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc)
{
    return nullptr;
}

GPUShaderProgram *D3D12Renderer::CreateGraphicsProgram(const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream)
{
    return nullptr;
}

GPUShaderProgram *D3D12Renderer::CreateComputeProgram(ByteStream *pByteCodeStream)
{
    return nullptr;
}

void D3D12Renderer::OnResourceCreated(GPUResource *pResource)
{
    uint32 cpuMemoryUsage, gpuMemoryUsage;
    pResource->GetMemoryUsage(&cpuMemoryUsage, &gpuMemoryUsage);

    if (pResource->GetResourceType() == GPU_RESOURCE_TYPE_BUFFER)
        Y_AtomicAdd(m_gpuMemoryUsage.BufferMemory, (size_t)gpuMemoryUsage);
}

void D3D12Renderer::OnResourceReleased(GPUResource *pResource)
{

}

void D3D12Renderer::GetGPUMemoryUsage(GPUMemoryUsage *pMemoryUsage) const
{
    Y_memcpy(pMemoryUsage, &m_gpuMemoryUsage, sizeof(GPUMemoryUsage));
}

Renderer *D3D12Renderer_CreateRenderer(const RendererInitializationParameters *pCreateParameters)
{
    DebugAssert(g_pRenderer == nullptr);

    D3D12Renderer *pRenderer = new D3D12Renderer();
    g_pRenderer = pRenderer;        // HACK, fixme! move to static counters

    if (!pRenderer->Create(pCreateParameters))
    {
        g_pRenderer = nullptr;
        delete pRenderer;
        return nullptr;
    }

    return pRenderer;
}

D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorCount, ID3D12DescriptorHeap *pD3DDescriptorHeap, uint32 incrementSize)
    : m_type(type)
    , m_descriptorCount(descriptorCount)
    , m_pD3DDescriptorHeap(pD3DDescriptorHeap)
    , m_allocationMap(descriptorCount)
    , m_incrementSize(incrementSize)
{
    m_allocationMap.Clear();
}

D3D12DescriptorHeap::~D3D12DescriptorHeap()
{
    m_pD3DDescriptorHeap->Release();
}

D3D12DescriptorHeap *D3D12DescriptorHeap::Create(ID3D12Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorCount)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.Type = type;
    desc.NumDescriptors = descriptorCount;
    desc.Flags = (type < D3D12_DESCRIPTOR_HEAP_TYPE_RTV) ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    ID3D12DescriptorHeap *pD3DDescriptorHeap;
    HRESULT hResult = pDevice->CreateDescriptorHeap(&desc, __uuidof(pD3DDescriptorHeap), (void **)&pD3DDescriptorHeap);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12DescriptorHeap::Create: CreateDescriptorHeap failed with hResult %08X", hResult);
        return nullptr;
    }

    uint32 incrementSize = pDevice->GetDescriptorHandleIncrementSize(type);
    return new D3D12DescriptorHeap(type, descriptorCount, pD3DDescriptorHeap, incrementSize);
}

bool D3D12DescriptorHeap::Allocate(Handle *handle)
{
    // find a free index
    size_t index;
    if (!m_allocationMap.FindFirstClearBit(&index))
        return false;

    // create handle
    handle->StartIndex = (uint32)index;
    handle->DescriptorCount = 1;
    handle->CPUHandle = m_pD3DDescriptorHeap->GetCPUDescriptorHandleForHeapStart();     // @TODO cache this
    handle->CPUHandle.ptr += index * m_incrementSize;
    handle->GPUHandle = m_pD3DDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    handle->GPUHandle.ptr += index * m_incrementSize;
    m_allocationMap.SetBit(index);
    return true;
}

bool D3D12DescriptorHeap::AllocateRange(uint32 count, Handle *handle)
{
    size_t startIndex;
    if (!m_allocationMap.FindContiguousClearBits(count, &startIndex))
        return false;

    // create handle
    handle->StartIndex = (uint32)startIndex;
    handle->DescriptorCount = count;
    handle->CPUHandle = m_pD3DDescriptorHeap->GetCPUDescriptorHandleForHeapStart();     // @TODO cache this
    handle->CPUHandle.ptr += startIndex * m_incrementSize;
    handle->GPUHandle = m_pD3DDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    handle->GPUHandle.ptr += startIndex * m_incrementSize;
    for (uint32 i = 0; i < count; i++)
        m_allocationMap.SetBit(startIndex + i);

    return true;
}

void D3D12DescriptorHeap::Free(const Handle *handle)
{
    // @TODO sanity check handle is correct offset and hasn't been corrupted
    uint32 startIndex = handle->StartIndex;
    DebugAssert(startIndex < m_descriptorCount);

    for (uint32 i = 0; i < handle->DescriptorCount; i++)
    {
        DebugAssert(m_allocationMap.TestBit(handle->StartIndex + i));
        m_allocationMap.UnsetBit(handle->StartIndex + i);
    }
}
