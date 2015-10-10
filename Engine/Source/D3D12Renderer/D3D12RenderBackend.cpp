#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUContext.h"
#include "D3D12Renderer/D3D12GPUOutputBuffer.h"
#include "D3D12Renderer/D3D12Helpers.h"
#include "D3D12Renderer/D3D12CVars.h"
#include "Engine/EngineCVars.h"
Log_SetChannel(D3D12RenderBackend);

bool D3D12RenderBackend_Create(const RendererInitializationParameters *pCreateParameters, SDL_Window *pSDLWindow, GPUDevice **ppDevice, GPUContext **ppImmediateContext, GPUOutputBuffer **ppOutputBuffer)
{
    HRESULT hResult;

    // load d3d12 library
    HMODULE hD3D12Module = LoadLibraryA("d3d12.dll");
    if (hD3D12Module == nullptr)
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Failed to load d3d12.dll library.");
        return false;
    }

    // get function entry points
    PFN_D3D12_CREATE_DEVICE fnD3D12CreateDevice = (PFN_D3D12_CREATE_DEVICE)GetProcAddress(hD3D12Module, "D3D12CreateDevice");
    PFN_D3D12_GET_DEBUG_INTERFACE fnD3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(hD3D12Module, "D3D12GetDebugInterface");
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE fnD3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(hD3D12Module, "D3D12SerializeRootSignature");
    if (fnD3D12CreateDevice == nullptr || fnD3D12GetDebugInterface == nullptr || fnD3D12SerializeRootSignature == nullptr)
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Missing D3D12 device entry points.");
        FreeLibrary(hD3D12Module);
        return false;
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

    // create dxgi factory
    IDXGIFactory4 *pDXGIFactory;
    hResult = CreateDXGIFactory2((CVars::r_use_debug_device.GetBool()) ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&pDXGIFactory));
    //hResult = CreateDXGIFactory1(IID_PPV_ARGS(&m_pDXGIFactory));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Failed to create DXGI factory with hResult %08X", hResult);
        FreeLibrary(hD3D12Module);
        return false;
    }

    // find adapter
    IDXGIAdapter3 *pDXGIAdapter = nullptr;
    if (CVars::r_d3d12_force_warp.GetBool())
    {
        hResult = pDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&pDXGIAdapter));
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("IDXGIFactory::EnumWarpAdapter failed with hResult %08X", hResult);
            pDXGIFactory->Release();
            FreeLibrary(hD3D12Module);
            return false;
        }
    }
#if 1
    else
    {
        // iterate over adapters
        for (uint32 adapterIndex = 0; ; adapterIndex++)
        {
            IDXGIAdapter *pAdapter;
            hResult = pDXGIFactory->EnumAdapters(adapterIndex, &pAdapter);
            if (hResult == DXGI_ERROR_NOT_FOUND)
                break;

            if (SUCCEEDED(hResult))
            {
                hResult = pAdapter->QueryInterface(__uuidof(IDXGIAdapter3), (void **)&pDXGIAdapter);
                if (FAILED(hResult))
                {
                    Log_ErrorPrintf("D3D12RenderBackend::Create: IDXGIAdapter::QueryInterface(IDXGIAdapter3) failed.");
                    pAdapter->Release();
                    pDXGIFactory->Release();
                    FreeLibrary(hD3D12Module);
                    return false;
                }

                pAdapter->Release();
                break;
            }

            Log_WarningPrintf("IDXGIFactory::EnumAdapters(%u) failed with hResult %08X", adapterIndex, hResult);
        }
    }

    // found an adapter
    if (pDXGIAdapter == nullptr)
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Failed to find an acceptable adapter.", hResult);
        pDXGIAdapter->Release();
        pDXGIFactory->Release();
        FreeLibrary(hD3D12Module);
        return false;
    }

    // print device name
    {
        // get adapter desc
        DXGI_ADAPTER_DESC DXGIAdapterDesc;
        hResult = pDXGIAdapter->GetDesc(&DXGIAdapterDesc);
        DebugAssert(hResult == S_OK);

        char deviceName[128];
        WideCharToMultiByte(CP_ACP, 0, DXGIAdapterDesc.Description, -1, deviceName, countof(deviceName), NULL, NULL);
        Log_InfoPrintf("D3D12 render backend using DXGI Adapter: %s.", deviceName);
    }
#endif

    // create the device
    ID3D12Device *pD3DDevice;
    hResult = fnD3D12CreateDevice(pDXGIAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pD3DDevice));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Could not create D3D12 device: %08X.", hResult);
        pDXGIAdapter->Release();
        pDXGIFactory->Release();
        FreeLibrary(hD3D12Module);
        return false;
    }

    // query feature levels
    static const D3D_FEATURE_LEVEL requestedFeatureLevels[] = { D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelsData = { countof(requestedFeatureLevels), requestedFeatureLevels, D3D_FEATURE_LEVEL_11_0 };
    hResult = pD3DDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelsData, sizeof(featureLevelsData));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS) failed with hResult %08X", hResult);
        pD3DDevice->Release();
        pDXGIAdapter->Release();
        pDXGIFactory->Release();
        FreeLibrary(hD3D12Module);
        return false;
    }

    // print feature level
    D3D_FEATURE_LEVEL D3DFeatureLevel = featureLevelsData.MaxSupportedFeatureLevel;
    Log_DevPrintf("D3D12RenderBackend::Create: Driver supports feature level %s (%X)", NameTable_GetNameString(NameTables::D3DFeatureLevels, D3DFeatureLevel), D3DFeatureLevel);

    // map to our range
    RENDERER_FEATURE_LEVEL featureLevel;
    if (D3DFeatureLevel >= D3D_FEATURE_LEVEL_11_0)
        featureLevel = RENDERER_FEATURE_LEVEL_SM5;
    else
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Feature level is insufficient");
        pD3DDevice->Release();
        pDXGIAdapter->Release();
        pDXGIFactory->Release();
        FreeLibrary(hD3D12Module);
        return false;
    }

    // find texture platform
    TEXTURE_PLATFORM texturePlatform;
    {
        texturePlatform = TEXTURE_PLATFORM_DXTC;
        Log_InfoPrintf("Texture Platform: %s", NameTable_GetNameString(NameTables::TexturePlatform, texturePlatform));
    }

    // other vars
    uint32 frameLatency = pCreateParameters->GPUFrameLatency;
    Log_DevPrintf("Frame latency: %u", frameLatency);

    // set default backbuffer formats if unspecified
    PIXEL_FORMAT outputBackBufferFormat = pCreateParameters->BackBufferFormat;
    PIXEL_FORMAT outputDepthStencilFormat = pCreateParameters->DepthStencilBufferFormat;
    if (outputBackBufferFormat == PIXEL_FORMAT_UNKNOWN)
        outputBackBufferFormat = PIXEL_FORMAT_R8G8B8A8_UNORM;

    // create device and context
    D3D12GPUDevice *pDevice = new D3D12GPUDevice(hD3D12Module, pDXGIFactory, pDXGIAdapter, pD3DDevice, D3DFeatureLevel, featureLevel, texturePlatform, outputBackBufferFormat, outputDepthStencilFormat, frameLatency);
    D3D12GPUContext *pImmediateContext = pDevice->InitializeAndCreateContext();
    if (pImmediateContext == nullptr)
    {
        pDevice->Release();
        pD3DDevice->Release();
        pDXGIAdapter->Release();
        pDXGIFactory->Release();
        FreeLibrary(hD3D12Module);
        return false;
    }

    // release our d3d pointers
    pD3DDevice->Release();
    pDXGIAdapter->Release();
    pDXGIFactory->Release();

    // create implicit swap chain
    GPUOutputBuffer *pOutputBuffer = nullptr;
    if (pSDLWindow != nullptr)
    {
        // pass through to normal method
        pOutputBuffer = pDevice->CreateOutputBuffer(pSDLWindow, pCreateParameters->ImplicitSwapChainVSyncType);
        if (pOutputBuffer == nullptr)
        {
            pImmediateContext->Release();
            pDevice->Release();
            return false;
        }

        // bind to context
        pImmediateContext->SetOutputBuffer(pOutputBuffer);
    }

    // set pointers
    *ppDevice = pDevice;
    *ppImmediateContext = pImmediateContext;
    *ppOutputBuffer = pOutputBuffer;

    Log_InfoPrint("D3D12 render backend creation successful.");
    return true;
}

