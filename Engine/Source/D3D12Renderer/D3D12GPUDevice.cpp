#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUContext.h"
#include "D3D12Renderer/D3D12GPUOutputBuffer.h"
#include "D3D12Renderer/D3D12Helpers.h"
#include "D3D12Renderer/D3D12CVars.h"
#include "Renderer/ShaderConstantBuffer.h"
#include "Engine/EngineCVars.h"
Log_SetChannel(D3D12RenderBackend);

Y_DECLARE_THREAD_LOCAL(ID3D12GraphicsCommandList *) s_pCurrentThreadCopyCommandList = nullptr;
Y_DECLARE_THREAD_LOCAL(D3D12GPUDevice::CopyCommandQueue *) s_pCurrentThreadCopyCommandQueue = nullptr;
Y_DECLARE_THREAD_LOCAL(uint32) s_currentThreadCopyBatchCount = 0;

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

D3D12GPUDevice::D3D12GPUDevice(HMODULE hD3D12Module, IDXGIFactory4 *pDXGIFactory, IDXGIAdapter3 *pDXGIAdapter, ID3D12Device *pD3DDevice, D3D_FEATURE_LEVEL D3DFeatureLevel, RENDERER_FEATURE_LEVEL featureLevel, TEXTURE_PLATFORM texturePlatform, PIXEL_FORMAT outputBackBufferFormat, PIXEL_FORMAT outputDepthStencilFormat, uint32 frameLatency)
    : m_hD3D12Module(hD3D12Module)
    , m_pDXGIFactory(pDXGIFactory)
    , m_pDXGIAdapter(pDXGIAdapter)
    , m_pD3DDevice(pD3DDevice)
    , m_pD3DInfoQueue(nullptr)
    , m_D3DFeatureLevel(D3DFeatureLevel)
    , m_featureLevel(featureLevel)
    , m_texturePlatform(texturePlatform)
    , m_outputBackBufferFormat(outputBackBufferFormat)
    , m_outputDepthStencilFormat(outputDepthStencilFormat)
    , m_frameLatency(frameLatency)
    , m_pGraphicsCommandQueue(nullptr)
    , m_pLegacyGraphicsRootSignature(nullptr)
    , m_pLegacyComputeRootSignature(nullptr)
    , m_fnD3D12SerializeRootSignature(nullptr)
    , m_pConstantBufferStorageHeap(nullptr)
{
    Y_memzero(m_pCPUDescriptorHeaps, sizeof(m_pCPUDescriptorHeaps));
}

D3D12GPUDevice::~D3D12GPUDevice()
{
    // remove any scheduled resources (descriptors will be dropped anyways)
    delete m_pGraphicsCommandQueue;

    // cleanup constant buffers
    for (ConstantBufferStorage &constantBufferStorage : m_constantBufferStorage)
    {
        GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Free(constantBufferStorage.DescriptorHandle);
        if (constantBufferStorage.pResource != nullptr)
            constantBufferStorage.pResource->Release();
    }
    SAFE_RELEASE(m_pConstantBufferStorageHeap);

    // remove descriptor heaps
    m_pCPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Free(m_nullCBVDescriptorHandle);
    m_pCPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Free(m_nullSRVDescriptorHandle);
    m_pCPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]->Free(m_nullSamplerHandle);
    for (uint32 i = 0; i < countof(m_pCPUDescriptorHeaps); i++)
    {
        if (m_pCPUDescriptorHeaps[i] != nullptr)
            delete m_pCPUDescriptorHeaps[i];
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
    Log_DevPrintf("Dumping active objects before teardown");
    DumpActiveObjects();

    // cleanup
    SAFE_RELEASE(m_pD3DInfoQueue);
    SAFE_RELEASE(m_pDXGIAdapter);
    SAFE_RELEASE(m_pDXGIFactory);
    SAFE_RELEASE(m_pD3DDevice);

    // dump objects before teardown
    Log_DevPrintf("Dumping active objects after teardown");
    DumpActiveObjects();

    // release handle to d3d12
    FreeLibrary(m_hD3D12Module);
}

RENDERER_PLATFORM D3D12GPUDevice::GetPlatform() const
{
    return RENDERER_PLATFORM_D3D12;
}

RENDERER_FEATURE_LEVEL D3D12GPUDevice::GetFeatureLevel() const
{
    return m_featureLevel;
}

TEXTURE_PLATFORM D3D12GPUDevice::GetTexturePlatform() const
{
    return m_texturePlatform;
}

void D3D12GPUDevice::GetCapabilities(RendererCapabilities *pCapabilities) const
{
    pCapabilities->MaxTextureAnisotropy = D3D12_MAX_MAXANISOTROPY;
    pCapabilities->MaximumVertexBuffers = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
    pCapabilities->MaximumConstantBuffers = D3D12_LEGACY_GRAPHICS_ROOT_CONSTANT_BUFFER_SLOTS;
    pCapabilities->MaximumTextureUnits = D3D12_LEGACY_GRAPHICS_ROOT_SHADER_RESOURCE_SLOTS;
    pCapabilities->MaximumSamplers = D3D12_LEGACY_GRAPHICS_ROOT_SHADER_SAMPLER_SLOTS;
    pCapabilities->MaximumRenderTargets = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
    pCapabilities->SupportsCommandLists = true;
    pCapabilities->SupportsMultithreadedResourceCreation = true;
    pCapabilities->SupportsDrawBaseVertex = true;
    pCapabilities->SupportsDepthTextures = true;
    pCapabilities->SupportsTextureArrays = (m_D3DFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
    pCapabilities->SupportsCubeMapTextureArrays = (m_D3DFeatureLevel >= D3D_FEATURE_LEVEL_10_1);
    pCapabilities->SupportsGeometryShaders = (m_D3DFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
    pCapabilities->SupportsSinglePassCubeMaps = (m_D3DFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
    pCapabilities->SupportsInstancing = (m_D3DFeatureLevel >= D3D_FEATURE_LEVEL_10_0);
}

bool D3D12GPUDevice::CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat /*= NULL*/) const
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

bool D3D12GPUDevice::CreateCommandQueues(uint32 copyCommandQueueCount)
{
    // create graphics command queue
    m_pGraphicsCommandQueue = new D3D12CommandQueue(this, m_pD3DDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, 2 * 1024 * 1024, 1024, 1024);
    if (!m_pGraphicsCommandQueue->Initialize())
        return false;

    // create copy command queues
    m_copyCommandQueues.Resize(copyCommandQueueCount);
    for (uint32 i = 0; i < copyCommandQueueCount; i++)
    {
        D3D12CommandQueue *pCommandQueue = new D3D12CommandQueue(this, m_pD3DDevice, D3D12_COMMAND_LIST_TYPE_COPY, 0, 0, 0);
        if (!pCommandQueue->Initialize())
        {
            delete pCommandQueue;
            return false;
        }

        m_copyCommandQueues[i].pCommandQueue = pCommandQueue;
    }

    // okay
    return true;
}

D3D12GPUDevice::CopyQueueReference::CopyQueueReference(D3D12GPUDevice *pDevice)
    : pDevice(pDevice)
{
    if (s_pCurrentThreadCopyCommandList == nullptr)
    {
        pDevice->GetFreeCopyCommandQueue();
        ContextNeedsRelease = true;
    }
    else
    {
        ContextNeedsRelease = false;
    }
}

D3D12GPUDevice::CopyQueueReference::~CopyQueueReference()
{
    if (ContextNeedsRelease)
        pDevice->ReleaseCopyCommandQueue();
}

bool D3D12GPUDevice::CopyQueueReference::HasContext() const
{
    return true;
}

void D3D12GPUDevice::BeginResourceBatchUpload()
{
    if ((s_currentThreadCopyBatchCount++) == 0)
    {
        // Have we got a copy queue?
        if (s_pCurrentThreadCopyCommandList == nullptr)
            GetFreeCopyCommandQueue();
    }
}

void D3D12GPUDevice::EndResourceBatchUpload()
{
    DebugAssert(s_currentThreadCopyBatchCount > 0);

    // Last one?
    if ((--s_currentThreadCopyBatchCount) == 0)
    {
        // Release if it's a copy queue.
        if (s_pCurrentThreadCopyCommandQueue != nullptr)
            ReleaseCopyCommandQueue();
    }
}

void D3D12GPUDevice::ResourceBarrier(ID3D12Resource *pResource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    // On graphics thread?
    if (s_pCurrentThreadCopyCommandQueue == nullptr)
    {
        // Issue as normal.
        D3D12_RESOURCE_BARRIER resourceBarrier =
        {
            D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,
            { pResource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, beforeState, afterState }
        };

        DebugAssert(s_pCurrentThreadCopyCommandList != nullptr);
        s_pCurrentThreadCopyCommandList->ResourceBarrier(1, &resourceBarrier);
    }
    else
    {
        // Queue transition.
        // @TODO: Handle cases where the resources is released better. Destruction *should* be queued until
        // the next fence, which will have passed by the time the transition has happened, anyway.
        PendingResourceTransition transition = { pResource, beforeState, afterState };
        m_pendingResourceTransitionLock.Lock();
        m_pendingResourceTransitions.Add(transition);
        m_pendingResourceTransitionLock.Unlock();
    }
}

void D3D12GPUDevice::ScheduleResourceForDeletion(ID3D12Pageable *pResource)
{
    m_pGraphicsCommandQueue->ScheduleResourceForDeletion(pResource);
}

void D3D12GPUDevice::ScheduleDescriptorForDeletion(const D3D12DescriptorHandle &handle)
{
    m_pGraphicsCommandQueue->ScheduleDescriptorForDeletion(handle);
}

void D3D12GPUDevice::SetCopyCommandList(ID3D12GraphicsCommandList *pCommandList)
{
    // should only be done on render thread with the graphics context command list
    DebugAssert(Renderer::IsOnRenderThread() && s_pCurrentThreadCopyCommandQueue == nullptr);
    s_pCurrentThreadCopyCommandList = pCommandList;
}

void D3D12GPUDevice::ScheduleCopyResourceForDeletion(ID3D12Pageable *pResource)
{
    // are we on the main thread?
    if (s_pCurrentThreadCopyCommandQueue == nullptr)
    {
        // release to graphics queue
        DebugAssert(s_pCurrentThreadCopyCommandList != nullptr);
        m_pGraphicsCommandQueue->ScheduleResourceForDeletion(pResource);
    }
    else
    {
        // release to copy queue
        s_pCurrentThreadCopyCommandQueue->pCommandQueue->ScheduleResourceForDeletion(pResource);
    }
}

void D3D12GPUDevice::GetFreeCopyCommandQueue()
{
    DebugAssert(s_pCurrentThreadCopyCommandQueue == nullptr);
    DebugAssert(s_pCurrentThreadCopyCommandList == nullptr);

    // search for an unlocked queue first
    CopyCommandQueue *pCommandQueue = nullptr;
    for (CopyCommandQueue &queue : m_copyCommandQueues)
    {
        if (queue.Lock.TryLock())
        {
            pCommandQueue = &queue;
            break;
        }
    }

    // failed, so just use the first one
    if (pCommandQueue == nullptr)
    {
        pCommandQueue = &m_copyCommandQueues[0];
        pCommandQueue->Lock.Lock();
    }

    // create a command list
    DebugAssert(pCommandQueue->pCommandAllocator == nullptr && pCommandQueue->pCommandList == nullptr);
    pCommandQueue->pCommandAllocator = pCommandQueue->pCommandQueue->RequestCommandAllocator();
    pCommandQueue->pCommandList = pCommandQueue->pCommandQueue->RequestAndOpenCommandList(pCommandQueue->pCommandAllocator);

    // set thread-local pointers
    s_pCurrentThreadCopyCommandQueue = pCommandQueue;
    s_pCurrentThreadCopyCommandList = pCommandQueue->pCommandList;
}

void D3D12GPUDevice::ReleaseCopyCommandQueue()
{
    DebugAssert(s_pCurrentThreadCopyCommandQueue != nullptr);
    DebugAssert(s_pCurrentThreadCopyCommandQueue->pCommandList == s_pCurrentThreadCopyCommandList);

    // close the command list
    CopyCommandQueue *pCopyQueue = s_pCurrentThreadCopyCommandQueue;
    HRESULT hResult = pCopyQueue->pCommandList->Close();
    if (SUCCEEDED(hResult))
    {
        // execute the commands
        pCopyQueue->pCommandQueue->ExecuteCommandList(pCopyQueue->pCommandList);
        pCopyQueue->pCommandQueue->ReleaseCommandList(pCopyQueue->pCommandList);
    }
    else
    {
        Log_WarningPrintf("Copy command list failed close with HRESULT %08X", hResult);
        pCopyQueue->pCommandQueue->ReleaseFailedCommandList(pCopyQueue->pCommandList);
    }

    // release allocator
    pCopyQueue->pCommandQueue->ReleaseCommandAllocator(pCopyQueue->pCommandAllocator);

    // wait for the queue to finish
    uint64 fenceValue = pCopyQueue->pCommandQueue->CreateSynchronizationPoint();
    pCopyQueue->pCommandQueue->WaitForFence(fenceValue);
    pCopyQueue->pCommandQueue->ReleaseStaleResources();

    // clear pointers
    s_pCurrentThreadCopyCommandList = nullptr;
    s_pCurrentThreadCopyCommandQueue = nullptr;
    pCopyQueue->pCommandAllocator = nullptr;
    pCopyQueue->pCommandList = nullptr;
    pCopyQueue->Lock.Unlock();
}

void D3D12GPUDevice::TransitionPendingResources(D3D12CommandQueue *pCommandQueue)
{
    // get out as quickly as possible if there's no pending transitions.
    m_pendingResourceTransitionLock.Lock();
    if (m_pendingResourceTransitions.IsEmpty())
    {
        m_pendingResourceTransitionLock.Unlock();
        return;
    }

    // get a command allocator and list
    ID3D12CommandAllocator *pCommandAllocator = pCommandQueue->RequestCommandAllocator();
    ID3D12GraphicsCommandList *pCommandList = pCommandQueue->RequestAndOpenCommandList(pCommandAllocator);
    Log_DevPrintf("D3D12RenderBackend: Transitioning %u pending resources.", m_pendingResourceTransitions.GetSize());

    // pass through to d3d. unfortunately this will be a period of contention, when 
    // resources have to be transitioned, command list execution halts on all threads.
    D3D12_RESOURCE_BARRIER *pResourceBarriers = (D3D12_RESOURCE_BARRIER *)alloca(sizeof(D3D12_RESOURCE_BARRIER) * Min(m_pendingResourceTransitions.GetSize(), (uint32)256));
    for (uint32 startIndex = 0; startIndex < m_pendingResourceTransitions.GetSize(); )
    {
        uint32 nResourceBarriers = Min(m_pendingResourceTransitions.GetSize() - startIndex, (uint32)256);
        for (uint32 i = 0; i < nResourceBarriers; i++)
        {
            const PendingResourceTransition *pPendingTransition = &m_pendingResourceTransitions[startIndex + i];
            D3D12_RESOURCE_BARRIER *pResourceBarrier = &pResourceBarriers[i];
            pResourceBarrier->Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            pResourceBarrier->Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            pResourceBarrier->Transition.pResource = pPendingTransition->pResource;
            pResourceBarrier->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            pResourceBarrier->Transition.StateBefore = pPendingTransition->BeforeState;
            pResourceBarrier->Transition.StateAfter = pPendingTransition->AfterState;
        }
        pCommandList->ResourceBarrier(nResourceBarriers, pResourceBarriers);
        startIndex += nResourceBarriers;
    }

    // allow re-queuing again. this will become an issue later on with multiple compute issuer threads though.
    m_pendingResourceTransitions.Clear();
    m_pendingResourceTransitionLock.Unlock();

    // execute commands.
    HRESULT hResult = pCommandList->Close();
    if (SUCCEEDED(hResult))
    {
        pCommandQueue->ExecuteCommandList(pCommandList);
        pCommandQueue->ReleaseCommandList(pCommandList);
    }
    else
    {
        Log_ErrorPrintf("Failed to close/execute command list. Resources will be left in an undefined state.");
        pCommandQueue->ReleaseFailedCommandList(pCommandList);
    }

    // release command allocator back.
    pCommandQueue->ReleaseCommandAllocator(pCommandAllocator);
}

ID3D12GraphicsCommandList *D3D12GPUDevice::GetCurrentCopyCommandList()
{
    return s_pCurrentThreadCopyCommandList;
}

D3D12GPUContext *D3D12GPUDevice::InitializeAndCreateContext()
{
    // function pointers
    m_fnD3D12SerializeRootSignature = (PFN_D3D12_SERIALIZE_ROOT_SIGNATURE)GetProcAddress(m_hD3D12Module, "D3D12SerializeRootSignature");
    DebugAssert(m_fnD3D12SerializeRootSignature != nullptr);

    // debug device?
    if (CVars::r_use_debug_device.GetBool() && CVars::r_d3d12_break_on_error.GetBool() && IsDebuggerPresent())
    {
        // enable d3d12 break-on-error
        HRESULT hResult = m_pD3DDevice->QueryInterface(IID_PPV_ARGS(&m_pD3DInfoQueue));
        if (SUCCEEDED(hResult))
        {
            // enable break on error
            m_pD3DInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            m_pD3DInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            m_pD3DInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

            // stop these annoying warnings
            // D3D12 WARNING : ID3D12CommandList::ClearRenderTargetView : The clear values do not match those passed to resource creation.The clear operation is typically slower as a result; but will still clear to the desired value.[EXECUTION WARNING #820: CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE]
            //m_pD3DInfoQueue->SetBreakOnID(D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE, FALSE);
        }

        // enable dxgi break-on-error
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
                    pDXGIInfoQueue->Release();
                }
            }
        }
    }

    // create legacy root descriptors
    if (!CreateLegacyRootSignatures())
        return nullptr;

    // create descriptor heaps
    if (!CreateCPUDescriptorHeaps())
        return nullptr;

    // create constant buffers
    if (!CreateConstantStorage())
        return nullptr;

    // create queues
    if (!CreateCommandQueues(4))
        return nullptr;

    // create context
    D3D12GPUContext *pContext = new D3D12GPUContext(this, m_pD3DDevice, m_pGraphicsCommandQueue);
    if (!pContext->Create())
    {
        pContext->Release();
        return false;
    }

    return pContext;
}

bool D3D12GPUDevice::CreateLegacyRootSignatures()
{
    HRESULT hResult;

    static const D3D12_DESCRIPTOR_RANGE descriptorRanges[] =
    {
        { D3D12_DESCRIPTOR_RANGE_TYPE_CBV, D3D12_LEGACY_GRAPHICS_ROOT_CONSTANT_BUFFER_SLOTS, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
        { D3D12_DESCRIPTOR_RANGE_TYPE_SRV, D3D12_LEGACY_GRAPHICS_ROOT_SHADER_RESOURCE_SLOTS, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
        { D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_LEGACY_GRAPHICS_ROOT_SHADER_SAMPLER_SLOTS, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND },
        { D3D12_DESCRIPTOR_RANGE_TYPE_UAV, D3D12_PS_CS_UAV_REGISTER_COUNT, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND }
    };

    static const D3D12_ROOT_PARAMETER graphicsRootParameters[] =
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
        { D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE, { 1, &descriptorRanges[3] }, D3D12_SHADER_VISIBILITY_PIXEL },

        // Per-draw constant buffers
        { D3D12_ROOT_PARAMETER_TYPE_CBV, { D3D12_LEGACY_GRAPHICS_ROOT_CONSTANT_BUFFER_SLOTS, 0 }, D3D12_SHADER_VISIBILITY_ALL }
    };

    static const D3D12_ROOT_PARAMETER computeRootParameters[] = 
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

bool D3D12GPUDevice::CreateCPUDescriptorHeaps()
{
    static const uint32 initialDescriptorHeapSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] =
    {
        16384,          // D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        1024,           // D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
        1024,           // D3D12_DESCRIPTOR_HEAP_TYPE_RTV
        1024            // D3D12_DESCRIPTOR_HEAP_TYPE_DSV
    };

    static_assert(countof(initialDescriptorHeapSize) == countof(m_pCPUDescriptorHeaps), "descriptor heap type count");
    for (uint32 i = 0; i < countof(initialDescriptorHeapSize); i++)
    {
        m_pCPUDescriptorHeaps[i] = D3D12DescriptorHeap::Create(m_pD3DDevice, (D3D12_DESCRIPTOR_HEAP_TYPE)i, initialDescriptorHeapSize[i], true);
        if (m_pCPUDescriptorHeaps[i] == nullptr)
        {
            Log_ErrorPrintf("Failed to allocate descriptor heap type %u", i);
            return false;
        }
    }

    // allocate null cbv
    m_pCPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate(&m_nullCBVDescriptorHandle);
    m_pD3DDevice->CreateConstantBufferView(nullptr, m_nullCBVDescriptorHandle);

    // allocate null srv
    D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = { DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING };
    nullSrvDesc.Texture2D = { 0, 1, 0, 0.0f };
    m_pCPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate(&m_nullSRVDescriptorHandle);
    m_pD3DDevice->CreateShaderResourceView(nullptr, &nullSrvDesc, m_nullSRVDescriptorHandle);

    // allocate null sampler
    D3D12_SAMPLER_DESC samplerDesc = { D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
                                       0.0f, 1, D3D12_COMPARISON_FUNC_NEVER, { 0.0f, 0.0f, 0.0f, 0.0f }, 0.0f, Y_FLT_MAX };
    m_pCPUDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]->Allocate(&m_nullSamplerHandle);
    m_pD3DDevice->CreateSampler(&samplerDesc, m_nullSamplerHandle);

    // done
    return true;
}

bool D3D12GPUDevice::CreateConstantStorage()
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
        if (declaration->GetMinimumFeatureLevel() != RENDERER_FEATURE_LEVEL_COUNT && declaration->GetMinimumFeatureLevel() > m_featureLevel)
            continue;

        // set size so we know to allocate it later or on demand
        ConstantBufferStorage *pConstantBufferStorage = &m_constantBufferStorage[i];
        pConstantBufferStorage->Size = ALIGNED_SIZE(declaration->GetBufferSize(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

        // align next buffer to 64kb
        memoryUsage = (memoryUsage > 0) ? ALIGNED_SIZE(memoryUsage, D3D12_PLACED_CONSTANT_BUFFER_ALIGNMENT) : 0;
        memoryUsage += pConstantBufferStorage->Size;
        activeCount++;
    }

    // create a heap for the constant buffers
    D3D12_HEAP_DESC heapDesc =
    {
        memoryUsage,
        { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 },
        D3D12_PLACED_CONSTANT_BUFFER_ALIGNMENT,
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
        currentOffset = (currentOffset > 0) ? ALIGNED_SIZE(currentOffset, D3D12_PLACED_CONSTANT_BUFFER_ALIGNMENT) : 0;

        // allocate resource in heap
        D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_PLACED_CONSTANT_BUFFER_ALIGNMENT, pConstantBufferStorage->Size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
        hResult = m_pD3DDevice->CreatePlacedResource(m_pConstantBufferStorageHeap, currentOffset, &resourceDesc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, IID_PPV_ARGS(&pConstantBufferStorage->pResource));
        if (FAILED(hResult))
        {
            const ShaderConstantBuffer *declaration = ShaderConstantBuffer::GetRegistry()->GetTypeInfoByIndex(i);
            Log_ErrorPrintf("Failed to allocate constant buffer %u (%s)", i, declaration->GetBufferName());
            return false;
        }

        // allocate a handle for the view
        if (!GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate(&pConstantBufferStorage->DescriptorHandle))
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

ID3D12Resource *D3D12GPUDevice::GetConstantBufferResource(uint32 index)
{
    DebugAssert(index < m_constantBufferStorage.GetSize());
    return m_constantBufferStorage[index].pResource;
}

const D3D12DescriptorHandle *D3D12GPUDevice::GetConstantBufferDescriptor(uint32 index) const
{
    DebugAssert(index < m_constantBufferStorage.GetSize());
    return (m_constantBufferStorage[index].pResource != nullptr) ? &m_constantBufferStorage[index].DescriptorHandle : nullptr;
}

static const D3D12_COMPARISON_FUNC s_D3D12ComparisonFuncs[GPU_COMPARISON_FUNC_COUNT] =
{
    D3D12_COMPARISON_FUNC_NEVER,             // RENDERER_COMPARISON_FUNC_NEVER
    D3D12_COMPARISON_FUNC_LESS,              // RENDERER_COMPARISON_FUNC_LESS
    D3D12_COMPARISON_FUNC_EQUAL,             // RENDERER_COMPARISON_FUNC_EQUAL
    D3D12_COMPARISON_FUNC_LESS_EQUAL,        // RENDERER_COMPARISON_FUNC_LESS_EQUAL
    D3D12_COMPARISON_FUNC_GREATER,           // RENDERER_COMPARISON_FUNC_GREATER
    D3D12_COMPARISON_FUNC_NOT_EQUAL,         // RENDERER_COMPARISON_FUNC_NOT_EQUAL
    D3D12_COMPARISON_FUNC_GREATER_EQUAL,     // RENDERER_COMPARISON_FUNC_GREATER_EQUAL
    D3D12_COMPARISON_FUNC_ALWAYS,            // RENDERER_COMPARISON_FUNC_ALWAYS
};

D3D12GPUSamplerState::D3D12GPUSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, D3D12GPUDevice *pDevice, const D3D12DescriptorHandle &samplerHandle)
    : GPUSamplerState(pSamplerStateDesc)
    , m_pDevice(pDevice)
    , m_samplerHandle(samplerHandle)
{
    m_pDevice->AddRef();
}

D3D12GPUSamplerState::~D3D12GPUSamplerState()
{
    m_pDevice->ScheduleDescriptorForDeletion(m_samplerHandle);
    m_pDevice->Release();
}

void D3D12GPUSamplerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12GPUSamplerState::SetDebugName(const char *name)
{

}

GPUSamplerState *D3D12GPUDevice::CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc)
{
    D3D12_SAMPLER_DESC D3DSamplerDesc;
    if (!D3D12Helpers::FillD3D12SamplerStateDesc(pSamplerStateDesc, &D3DSamplerDesc))
    {
        Log_ErrorPrintf("Invalid sampler state description");
        return nullptr;
    }

    // allocate a descriptor
    D3D12DescriptorHandle descriptorHandle;
    if (!GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)->Allocate(&descriptorHandle))
    {
        Log_ErrorPrintf("Failed to allocate descriptor handle");
        return false;
    }

    // fill descriptor
    m_pD3DDevice->CreateSampler(&D3DSamplerDesc, descriptorHandle);
    return new D3D12GPUSamplerState(pSamplerStateDesc, this, descriptorHandle);
}

D3D12GPURasterizerState::D3D12GPURasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc, const D3D12_RASTERIZER_DESC *pD3DRasterizerDesc)
    : GPURasterizerState(pRasterizerStateDesc)
{
    Y_memcpy(&m_D3DRasterizerDesc, pD3DRasterizerDesc, sizeof(m_D3DRasterizerDesc));
}

D3D12GPURasterizerState::~D3D12GPURasterizerState()
{

}

void D3D12GPURasterizerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12GPURasterizerState::SetDebugName(const char *name)
{

}

GPURasterizerState *D3D12GPUDevice::CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc)
{
    D3D12_RASTERIZER_DESC D3DRasterizerDesc;
    if (!D3D12Helpers::FillD3D12RasterizerStateDesc(pRasterizerStateDesc, &D3DRasterizerDesc))
    {
        Log_ErrorPrintf("Invalid rasterizer state description");
        return nullptr;
    }

    return new D3D12GPURasterizerState(pRasterizerStateDesc, &D3DRasterizerDesc);
}

D3D12GPUDepthStencilState::D3D12GPUDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc, const D3D12_DEPTH_STENCIL_DESC *pD3DDepthStencilDesc)
    : GPUDepthStencilState(pDepthStencilStateDesc)
{
    Y_memcpy(&m_D3DDepthStencilDesc, pD3DDepthStencilDesc, sizeof(m_D3DDepthStencilDesc));
}

D3D12GPUDepthStencilState::~D3D12GPUDepthStencilState()
{

}

void D3D12GPUDepthStencilState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12GPUDepthStencilState::SetDebugName(const char *name)
{

}

GPUDepthStencilState *D3D12GPUDevice::CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc)
{
    D3D12_DEPTH_STENCIL_DESC D3DDepthStencilDesc;
    if (!D3D12Helpers::FillD3D12DepthStencilStateDesc(pDepthStencilStateDesc, &D3DDepthStencilDesc))
    {
        Log_ErrorPrintf("Invalid depth-stencil state description");
        return nullptr;
    }

    return new D3D12GPUDepthStencilState(pDepthStencilStateDesc, &D3DDepthStencilDesc);
}

D3D12GPUBlendState::D3D12GPUBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc, const D3D12_BLEND_DESC *pD3DBlendDesc)
    : GPUBlendState(pBlendStateDesc)
{
    Y_memcpy(&m_D3DBlendDesc, pD3DBlendDesc, sizeof(m_D3DBlendDesc));
}

D3D12GPUBlendState::~D3D12GPUBlendState()
{
    
}

void D3D12GPUBlendState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12GPUBlendState::SetDebugName(const char *name)
{

}

GPUBlendState *D3D12GPUDevice::CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc)
{
    D3D12_BLEND_DESC D3DBlendDesc;
    if (!D3D12Helpers::FillD3D12BlendStateDesc(pBlendStateDesc, &D3DBlendDesc))
    {
        Log_ErrorPrintf("Invalid blend state description");
        return nullptr;
    }

    return new D3D12GPUBlendState(pBlendStateDesc, &D3DBlendDesc);
}
