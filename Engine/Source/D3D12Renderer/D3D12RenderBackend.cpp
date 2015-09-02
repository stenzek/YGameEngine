#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12RenderBackend.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUContext.h"
#include "D3D12Renderer/D3D12GPUOutputBuffer.h"
#include "D3D12Renderer/D3D12Helpers.h"
#include "D3D12Renderer/D3D12CVars.h"
#include "Engine/EngineCVars.h"
#include "Renderer/ShaderConstantBuffer.h"
Log_SetChannel(D3D12GPUDevice);

D3D12RenderBackend *D3D12RenderBackend::s_pInstance = nullptr;
static HMODULE s_hD3D12DLL = nullptr;

static void DumpActiveObjects()
{
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

//     // real hackery, lets us paste a pointer from the above in and view it
//     ID3D12Resource *x = nullptr;
//     if (x != nullptr)
//     {
//         D3D12_RESOURCE_DESC d = x->GetDesc();
//         Log_DevPrint("");
//     }
}
#endif
}

static void foo()
{
    HMODULE hDXGIDebugModule = GetModuleHandleA("dxgidebug.dll");
    if (hDXGIDebugModule != NULL)
    {
        HRESULT(WINAPI *pDXGIGetDebugInterface)(REFIID riid, void **ppDebug);
        pDXGIGetDebugInterface = (HRESULT(WINAPI *)(REFIID, void **))GetProcAddress(hDXGIDebugModule, "DXGIGetDebugInterface");
        if (pDXGIGetDebugInterface != NULL)
        {
            IDXGIInfoQueue *pDXGIInfoQueue;
            if (SUCCEEDED(pDXGIGetDebugInterface(__uuidof(pDXGIInfoQueue), reinterpret_cast<void **>(&pDXGIInfoQueue))))
            {
                pDXGIInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                pDXGIInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
                pDXGIInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, TRUE);
                //pDXGIInfoQueue->Release();
            }
        }
    }
}

D3D12RenderBackend::D3D12RenderBackend()
    : m_pDXGIFactory(nullptr)
    , m_pDXGIAdapter(nullptr)
    , m_pD3DDevice(nullptr)
    , m_pD3DInfoQueue(nullptr)
    , m_D3DFeatureLevel(D3D_FEATURE_LEVEL_11_0)
    , m_featureLevel(RENDERER_FEATURE_LEVEL_COUNT)
    , m_texturePlatform(NUM_TEXTURE_PLATFORMS)
    , m_outputBackBufferFormat(PIXEL_FORMAT_UNKNOWN)
    , m_outputDepthStencilFormat(PIXEL_FORMAT_UNKNOWN)
    , m_frameLatency(0)
    , m_pGPUDevice(nullptr)
    , m_pGPUContext(nullptr)
    , m_pLegacyGraphicsRootSignature(nullptr)
    , m_pLegacyComputeRootSignature(nullptr)
    , m_fnD3D12SerializeRootSignature(nullptr)
    , m_pConstantBufferStorageHeap(nullptr)
    , m_currentCleanupFenceValue(0)
{
    Y_memzero(m_pDescriptorHeaps, sizeof(m_pDescriptorHeaps));

    DebugAssert(s_pInstance == nullptr);
    s_pInstance = this;
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
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE fnD3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(GetModuleHandleA("d3d12.dll"), "D3D12SerializeRootSignature");
    if (fnD3D12CreateDevice == nullptr || fnD3D12GetDebugInterface == nullptr || fnD3D12SerializeRootSignature == nullptr)
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Missing D3D12 device entry points.");
        return false;
    }

    // set pointer
    m_fnD3D12SerializeRootSignature = fnD3D12SerializeRootSignature;

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
    hResult = CreateDXGIFactory2((CVars::r_use_debug_device.GetBool()) ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&m_pDXGIFactory));
    //hResult = CreateDXGIFactory1(IID_PPV_ARGS(&m_pDXGIFactory));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Failed to create DXGI factory with hResult %08X", hResult);
        return false;
    }

    // find adapter
    if (CVars::r_d3d12_force_warp.GetBool())
    {
        hResult = m_pDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&m_pDXGIAdapter));
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("IDXGIFactory::EnumWarpAdapter failed with hResult %08X", hResult);
            return false;
        }
    }
#if 0
    else
    {
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

            Log_WarningPrintf("IDXGIFactory::EnumAdapters(%u) failed with hResult %08X", adapterIndex, hResult);
        }
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
#endif

    // create the device
    hResult = fnD3D12CreateDevice(m_pDXGIAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pD3DDevice));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12RenderBackend::Create: Could not create D3D12 device: %08X.", hResult);
        return false;
    }

    // debug device?
    if (CVars::r_use_debug_device.GetBool())
    {
        hResult = m_pD3DDevice->QueryInterface(IID_PPV_ARGS(&m_pD3DInfoQueue));
        if (SUCCEEDED(hResult))
        {
            // enable break on error
            if (CVars::r_d3d12_break_on_error.GetBool() && IsDebuggerPresent())
            {
                m_pD3DInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                m_pD3DInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
                m_pD3DInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
                foo();
            }
        }
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
        //m_texturePlatform = TEXTURE_PLATFORM_DXTC;
        m_texturePlatform = TEXTURE_PLATFORM_ES2_NOTC;
        Log_InfoPrintf("Texture Platform: %s", NameTable_GetNameString(NameTables::TexturePlatform, m_texturePlatform));
    }

    // other vars
    m_frameLatency = pCreateParameters->GPUFrameLatency;
    Log_DevPrintf("Frame latency: %u", m_frameLatency);

    // set default backbuffer formats if unspecified
    m_outputBackBufferFormat = pCreateParameters->BackBufferFormat;
    m_outputDepthStencilFormat = pCreateParameters->DepthStencilBufferFormat;
    if (m_outputBackBufferFormat == PIXEL_FORMAT_UNKNOWN)
        m_outputBackBufferFormat = PIXEL_FORMAT_R8G8B8A8_UNORM;

    // create legacy root descriptors
    if (!CreateLegacyRootSignatures())
        return false;

    // create descriptor heaps
    if (!CreateDescriptorHeaps())
        return false;

    // create constant buffers
    if (!CreateConstantStorage())
        return false;

    // create device wrapper class
    m_pGPUDevice = new D3D12GPUDevice(this, m_pDXGIFactory, m_pDXGIAdapter, m_pD3DDevice, m_outputBackBufferFormat, m_outputDepthStencilFormat);
    if (!m_pGPUDevice->CreateOffThreadResources())
        return false;

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

bool D3D12RenderBackend::CreateLegacyRootSignatures()
{
    HRESULT hResult;

    D3D12_DESCRIPTOR_RANGE descriptorRanges[] =
    {
        { D3D12_DESCRIPTOR_RANGE_TYPE_CBV, D3D12_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
        { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
        { D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_COMMONSHADER_SAMPLER_SLOT_COUNT, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
        { D3D12_DESCRIPTOR_RANGE_TYPE_UAV, D3D12_PS_CS_UAV_REGISTER_COUNT, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }
    };

    D3D12_ROOT_PARAMETER graphicsRootParameters[] =
    {
        // VS
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[0] }, D3D12_SHADER_VISIBILITY_VERTEX },
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[1] }, D3D12_SHADER_VISIBILITY_VERTEX },
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[2] }, D3D12_SHADER_VISIBILITY_VERTEX },

        // HS
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[0] }, D3D12_SHADER_VISIBILITY_HULL },
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[1] }, D3D12_SHADER_VISIBILITY_HULL },
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[2] }, D3D12_SHADER_VISIBILITY_HULL },

        // DS
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[0] }, D3D12_SHADER_VISIBILITY_DOMAIN },
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[1] }, D3D12_SHADER_VISIBILITY_DOMAIN },
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[2] }, D3D12_SHADER_VISIBILITY_DOMAIN },

        // GS
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[0] }, D3D12_SHADER_VISIBILITY_GEOMETRY },
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[1] }, D3D12_SHADER_VISIBILITY_GEOMETRY },
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[2] }, D3D12_SHADER_VISIBILITY_GEOMETRY },

        // PS
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[0] }, D3D12_SHADER_VISIBILITY_PIXEL },
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[1] }, D3D12_SHADER_VISIBILITY_PIXEL },
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[2] }, D3D12_SHADER_VISIBILITY_PIXEL },

        // UAVs
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[3] }, D3D12_SHADER_VISIBILITY_PIXEL }
    };

    D3D12_ROOT_PARAMETER computeRootParameters[] = 
    {
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[0] }, D3D12_SHADER_VISIBILITY_ALL },
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[1] }, D3D12_SHADER_VISIBILITY_ALL },
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[2] }, D3D12_SHADER_VISIBILITY_ALL },
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[3] }, D3D12_SHADER_VISIBILITY_ALL }
    };
    
    D3D12_ROOT_SIGNATURE_DESC graphicsRootSignatureDesc = { countof(graphicsRootParameters), graphicsRootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };
    D3D12_ROOT_SIGNATURE_DESC computeRootSignatureDesc = { countof(computeRootParameters), computeRootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS };

    // serialize it
    ID3DBlob *pRootSignatureBlob;
    ID3DBlob *pErrorBlob;

    // for graphics
    hResult = m_fnD3D12SerializeRootSignature(&graphicsRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pRootSignatureBlob, &pErrorBlob);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12SerializeRootSignature for graphics failed with hResult %08X", hResult);
        if (pErrorBlob != nullptr)
        {
            Log_ErrorPrint((const char *)pErrorBlob->GetBufferPointer());
            pErrorBlob->Release();
        }
        return false;
    }
    SAFE_RELEASE(pErrorBlob);
    hResult = m_pD3DDevice->CreateRootSignature(0, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_pLegacyGraphicsRootSignature));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateRootSignature for graphics failed with hResult %08X", hResult);
        pRootSignatureBlob->Release();
        return false;
    }
    pRootSignatureBlob->Release();

    // for compute
    hResult = m_fnD3D12SerializeRootSignature(&computeRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pRootSignatureBlob, &pErrorBlob);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12SerializeRootSignature for compute failed with hResult %08X", hResult);
        if (pErrorBlob != nullptr)
        {
            Log_ErrorPrint((const char *)pErrorBlob->GetBufferPointer());
            pErrorBlob->Release();
        }
        return false;
    }
    SAFE_RELEASE(pErrorBlob);
    hResult = m_pD3DDevice->CreateRootSignature(0, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_pLegacyComputeRootSignature));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateRootSignature for compute failed with hResult %08X", hResult);
        pRootSignatureBlob->Release();
        return false;
    }
    pRootSignatureBlob->Release();
    return true;
}

bool D3D12RenderBackend::CreateDescriptorHeaps()
{
    static const uint32 initialDescriptorHeapSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
    {
        16384,          // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        1024,           // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
        1024,           // D3D12_DESCRIPTOR_HEAP_TYPE_RTV
        1024            // D3D12_DESCRIPTOR_HEAP_TYPE_DSV
    };

    static_assert(countof(initialDescriptorHeapSize) == countof(m_pDescriptorHeaps), "descriptor heap type count");
    for (uint32 i = 0; i < countof(initialDescriptorHeapSize); i++)
    {
        m_pDescriptorHeaps[i] = D3D12DescriptorHeap::Create(m_pD3DDevice, (D3D12_DESCRIPTOR_HEAP_TYPE)i, initialDescriptorHeapSize[i]);
        if (m_pDescriptorHeaps[i] == nullptr)
        {
            Log_ErrorPrintf("Failed to allocate descriptor heap type %u", i);
            return false;
        }
    }

    return true;
}

bool D3D12RenderBackend::CreateConstantStorage()
{
    HRESULT hResult;
    uint32 memoryUsage = 0;
    uint32 activeCount = 0;

    // allocate constant buffer storage
    const ShaderConstantBuffer::RegistryType *registry = ShaderConstantBuffer::GetRegistry();
    m_constantBufferStorage.Resize(registry->GetNumTypes());
    m_constantBufferStorage.ZeroContents();
    for (uint32 i = 0; i < m_constantBufferStorage.GetSize(); i++)
    {
        // applicable to us?
        const ShaderConstantBuffer *declaration = registry->GetTypeInfoByIndex(i);
        if (declaration == nullptr)
            continue;
        if (declaration->GetPlatformRequirement() != RENDERER_PLATFORM_COUNT && declaration->GetPlatformRequirement() != RENDERER_PLATFORM_D3D12)
            continue;
        if (declaration->GetMinimumFeatureLevel() != RENDERER_FEATURE_LEVEL_COUNT && declaration->GetMinimumFeatureLevel() > D3D12RenderBackend::GetInstance()->GetFeatureLevel())
            continue;

        // set size so we know to allocate it later or on demand
        ConstantBufferStorage *pConstantBufferStorage = &m_constantBufferStorage[i];
        pConstantBufferStorage->Size = ALIGNED_SIZE(declaration->GetBufferSize(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        // align next buffer to 64kb
        memoryUsage = (memoryUsage > 0) ? ALIGNED_SIZE(memoryUsage, D3D12_CONSTANT_BUFFER_ALIGNMENT) : 0;
        memoryUsage += pConstantBufferStorage->Size;
        activeCount++;
    }

    // create a heap for the constant buffers
    D3D12_HEAP_DESC heapDesc =
    {
        memoryUsage,
        { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 },
        D3D12_CONSTANT_BUFFER_ALIGNMENT,
        D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS
    };
    hResult = m_pD3DDevice->CreateHeap(&heapDesc, __uuidof(ID3D12Heap), (void **)&m_pConstantBufferStorageHeap);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("Failed to create constant buffer heap: CreateHeap failed with hResult %08X", hResult);
        return false;
    }

    // preallocate constant buffers
    Log_DevPrintf("Preallocating %u constant buffers, total VRAM usage: %u bytes", activeCount, memoryUsage);
    uint32 currentOffset = 0;
    for (uint32 i = 0; i < m_constantBufferStorage.GetSize(); i++)
    {
        ConstantBufferStorage *pConstantBufferStorage = &m_constantBufferStorage[i];
        if (pConstantBufferStorage->Size == 0)
            continue;

        // align buffer to 64kb
        currentOffset = (currentOffset > 0) ? ALIGNED_SIZE(currentOffset, D3D12_CONSTANT_BUFFER_ALIGNMENT) : 0;

        // allocate resource in heap
        D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_CONSTANT_BUFFER_ALIGNMENT, pConstantBufferStorage->Size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
        hResult = m_pD3DDevice->CreatePlacedResource(m_pConstantBufferStorageHeap, currentOffset, &resourceDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, IID_PPV_ARGS(&pConstantBufferStorage->pResource));
        if (FAILED(hResult))
        {
            const ShaderConstantBuffer *declaration = ShaderConstantBuffer::GetRegistry()->GetTypeInfoByIndex(i);
            Log_ErrorPrintf("Failed to allocate constant buffer %u (%s)", i, declaration->GetBufferName());
            return false;
        }

        // allocate a handle for the view
        if (!GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate(&pConstantBufferStorage->DescriptorHandle))
        {
            const ShaderConstantBuffer *declaration = ShaderConstantBuffer::GetRegistry()->GetTypeInfoByIndex(i);
            Log_ErrorPrintf("Failed to allocate constant buffer %u (%s)", i, declaration->GetBufferName());
            SAFE_RELEASE(pConstantBufferStorage->pResource);
            return false;
        }

        // create a constant buffer view
        D3D12_CONSTANT_BUFFER_VIEW_DESC constantBufferViewDesc = { pConstantBufferStorage->pResource->GetGPUVirtualAddress(), pConstantBufferStorage->Size };
        m_pD3DDevice->CreateConstantBufferView(&constantBufferViewDesc, pConstantBufferStorage->DescriptorHandle);

        // move buffer forward
        currentOffset += pConstantBufferStorage->Size;
    }

    return true;
}

void D3D12RenderBackend::Shutdown()
{
    // cleanup our objects
    SAFE_RELEASE_LAST(m_pGPUContext);
    SAFE_RELEASE_LAST(m_pGPUDevice);

    // remove any scheduled resources (descriptors will be dropped anyways)
    for (uint32 i = 0; i < m_pendingDeletionResources.GetSize(); i++)
        m_pendingDeletionResources[i].pResource->Release();

    // cleanup constant buffers
    for (ConstantBufferStorage &constantBufferStorage : m_constantBufferStorage)
    {
        GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Free(constantBufferStorage.DescriptorHandle);
        if (constantBufferStorage.pResource != nullptr)
            constantBufferStorage.pResource->Release();
    }
    SAFE_RELEASE(m_pConstantBufferStorageHeap);

    // remove descriptor heaps
    for (uint32 i = 0; i < countof(m_pDescriptorHeaps); i++)
    {
        if (m_pDescriptorHeaps[i] != nullptr)
            delete m_pDescriptorHeaps[i];
    }

    // signatures
    SAFE_RELEASE(m_pLegacyComputeRootSignature);
    SAFE_RELEASE(m_pLegacyGraphicsRootSignature);

//     // disable break on error, since each leaked object is a message
//     if (m_pD3DInfoQueue != nullptr && CVars::r_d3d12_break_on_error.GetBool() && IsDebuggerPresent())
//     {
//         m_pD3DInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);
//         m_pD3DInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, FALSE);
//         m_pD3DInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, FALSE);
//     }

    // dump objects before teardown
    //Log_DevPrintf("Dumping active objects before teardown");
    //DumpActiveObjects();

    // cleanup
    SAFE_RELEASE(m_pD3DInfoQueue);
    SAFE_RELEASE(m_pDXGIAdapter);
    SAFE_RELEASE(m_pDXGIFactory);
    SAFE_RELEASE(m_pD3DDevice);

    // dump objects before teardown
    Log_DevPrintf("Dumping active objects after teardown");
    DumpActiveObjects();

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
    DXGI_FORMAT DXGIFormat = D3D12Helpers::PixelFormatToDXGIFormat(PixelFormat);
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

ID3D12Resource *D3D12RenderBackend::GetConstantBufferResource(uint32 index)
{
    DebugAssert(index < m_constantBufferStorage.GetSize());
    return m_constantBufferStorage[index].pResource;
}

const D3D12DescriptorHandle *D3D12RenderBackend::GetConstantBufferDescriptor(uint32 index) const
{
    DebugAssert(index < m_constantBufferStorage.GetSize());
    return (m_constantBufferStorage[index].pResource != nullptr) ? &m_constantBufferStorage[index].DescriptorHandle : nullptr;
}

void D3D12RenderBackend::ScheduleResourceForDeletion(ID3D12Pageable *pResource)
{
    ScheduleResourceForDeletion(pResource, m_currentCleanupFenceValue);
}

void D3D12RenderBackend::ScheduleResourceForDeletion(ID3D12Pageable *pResource, uint64 fenceValue /* = GetCurrentCleanupFenceValue() */)
{
    PendingDeletionResource pdr;
    pdr.pResource = pResource;
    pdr.FenceValue = fenceValue;
    
    m_pendingDeletionLock.Lock();
    m_pendingDeletionResources.Add(pdr);
    m_pendingDeletionLock.Unlock();
}

void D3D12RenderBackend::ScheduleDescriptorForDeletion(const D3D12DescriptorHandle &handle)
{
    ScheduleDescriptorForDeletion(handle, m_currentCleanupFenceValue);
}

void D3D12RenderBackend::ScheduleDescriptorForDeletion(const D3D12DescriptorHandle &handle, uint64 fenceValue /* = GetCurrentCleanupFenceValue() */)
{
    PendingDeletionDescriptor pdr;
    pdr.Handle = handle;
    pdr.FenceValue = fenceValue;

    m_pendingDeletionLock.Lock();
    m_pendingDeletionDescriptors.Add(pdr);
    m_pendingDeletionLock.Unlock();
}

void D3D12RenderBackend::DeletePendingResources(uint64 fenceValue)
{
    m_pendingDeletionLock.Lock();

    for (uint32 i = 0; i < m_pendingDeletionResources.GetSize(); )
    {
        if (m_pendingDeletionResources[i].FenceValue > fenceValue)
        {
            i++;
            continue;
        }

        ID3D12Pageable *pResource = m_pendingDeletionResources[i].pResource;
        m_pendingDeletionResources.FastRemove(i);
        SAFE_RELEASE_LAST(pResource);
    }

    for (uint32 i = 0; i < m_pendingDeletionDescriptors.GetSize(); )
    {
        PendingDeletionDescriptor &desc = m_pendingDeletionDescriptors[i];
        if (desc.FenceValue > fenceValue)
        {
            i++;
            continue;
        }

        DebugAssert(desc.Handle.Type < countof(m_pDescriptorHeaps));
        m_pDescriptorHeaps[desc.Handle.Type]->Free(desc.Handle);
        m_pendingDeletionDescriptors.FastRemove(i);
    }

    m_pendingDeletionLock.Unlock();
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
