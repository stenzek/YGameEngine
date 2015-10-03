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

D3D12GraphicsCommandQueue::D3D12GraphicsCommandQueue(D3D12RenderBackend *pBackend, ID3D12Device *pDevice, uint32 linearBufferHeapSize, uint32 linearViewHeapSize, uint32 linearSamplerHeapSize)
    : m_pBackend(pBackend)
    , m_pD3DDevice(pDevice)
    , m_linearBufferHeapSize(linearBufferHeapSize)
    , m_linearViewHeapSize(linearViewHeapSize)
    , m_linearSamplerHeapSize(linearSamplerHeapSize)
    , m_maxCommandAllocatorPoolSize(pBackend->GetFrameLatency() * 1)
    , m_maxLinearBufferHeapPoolSize(256)
    , m_maxLinearViewHeapPoolSize(1024)
    , m_maxLinearSamplerHeapPoolSize(1024)
    , m_pD3DFence(nullptr)
    , m_fenceEvent(NULL)
    , m_nextFenceValue(1)
    , m_lastCompletedFenceValue(0)
    , m_outstandingCommandAllocators(0)
    , m_outstandingLinearBufferHeaps(0)
    , m_outstandingLinearViewHeaps(0)
    , m_outstandingLinearSamplerHeaps(0)
{

}

D3D12GraphicsCommandQueue::~D3D12GraphicsCommandQueue()
{
    // sync, then destroy everything
    if (m_pD3DCommandQueue != nullptr)
    {
        uint64 fenceValue = CreateSynchronizationPoint();
        WaitForFence(fenceValue);

        // nuke descriptors
        for (uint32 i = 0; i < m_pendingDeletionDescriptors.GetSize(); i++)
            m_pBackend->GetCPUDescriptorHeap(m_pendingDeletionDescriptors[i].Handle.Type)->Free(m_pendingDeletionDescriptors[i].Handle);

        // nuke resources
        for (uint32 i = 0; i < m_pendingDeletionResources.GetSize(); i++)
            m_pendingDeletionResources[i].pResource->Release();
    }

    if (m_fenceEvent != NULL)
        CloseHandle(m_fenceEvent);
    SAFE_RELEASE(m_pD3DFence);
    SAFE_RELEASE(m_pD3DCommandQueue);

}

bool D3D12GraphicsCommandQueue::Initialize()
{
    HRESULT hResult;

    // allocate command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE, 1 };
    hResult = m_pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pD3DCommandQueue));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommandQueue failed with hResult %08X", hResult);
        return false;
    }

    // allocate fence
    hResult = m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pD3DFence));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("ID3D12Device::CreateFence failed with hResult %08X", hResult);
        return false;
    }

    // allocate reached event
    m_fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    if (m_fenceEvent == NULL)
    {
        Log_ErrorPrintf("CreateEvent failed (%u).", GetLastError());
        return false;
    }

    return true;
}

void D3D12GraphicsCommandQueue::ReleaseStaleResources()
{
    // clean up as much as we can
    UpdateLastCompletedFenceValue();
    DeletePendingResources(m_lastCompletedFenceValue);
}

uint64 D3D12GraphicsCommandQueue::CreateSynchronizationPoint()
{
    DebugAssert(Renderer::IsOnRenderThread());

    uint64 returnValue = m_nextFenceValue++;
    HRESULT hResult = m_pD3DCommandQueue->Signal(m_pD3DFence, returnValue);
    if (FAILED(hResult))
        Log_WarningPrintf("ID3D12CommandQueue::Signal failed with hResult %08X", hResult);

    return returnValue;
}

uint64 D3D12GraphicsCommandQueue::ExecuteCommandList(ID3D12CommandList *pCommandList)
{
    DebugAssert(Renderer::IsOnRenderThread());

    m_pD3DCommandQueue->ExecuteCommandLists(1, &pCommandList);
    return CreateSynchronizationPoint();
}

template<class T>
T *D3D12GraphicsCommandQueue::SearchPool(MemArray<KeyValuePair<T *, uint64>> &pool)
{
    if (pool.IsEmpty())
        return nullptr;

    if (m_lastCompletedFenceValue < pool[0].Value)
        UpdateLastCompletedFenceValue();

    if (m_lastCompletedFenceValue < pool[0].Value)
        return nullptr;

    T *pReturn = pool[0].Key;
    pool.RemoveFront();
    return pReturn;
}

template<class T>
T *D3D12GraphicsCommandQueue::SearchPoolAndWait(MemArray<KeyValuePair<T *, uint64>> &pool)
{
    DebugAssert(!pool.IsEmpty());
    
    WaitForFence(pool[0].Value);

    T *pReturn = pool[0].Key;
    pool.RemoveFront();
    return pReturn;
}

template<class T>
void D3D12GraphicsCommandQueue::InsertIntoPool(MemArray<KeyValuePair<T *, uint64>> &pool, T *pItem, uint64 fenceValue)
{
    uint32 pos = 0;
    for (; pos < pool.GetSize(); pos++)
    {
        if (pool[pos].Value > fenceValue)
            break;
    }

    pool.Insert(KeyValuePair<T *, uint64>(pItem, fenceValue), pos);
}

ID3D12CommandAllocator *D3D12GraphicsCommandQueue::RequestCommandAllocator()
{
    ID3D12CommandAllocator *pReturnAllocator;
    DebugAssert(Renderer::IsOnRenderThread());

    // search the pool first
    pReturnAllocator = SearchPool<ID3D12CommandAllocator>(m_commandAllocatorPool);
    if (pReturnAllocator != nullptr)
    {
        // reset the allocator, ready for use
        HRESULT hResult = pReturnAllocator->Reset();
        if (FAILED(hResult))
            Panic("ID3D12CommandAllocator::Reset failed.");

        // have one ready to go, so release it
        m_outstandingCommandAllocators++;
        return pReturnAllocator;
    }

    // can we allocate a new one?
    if ((m_outstandingCommandAllocators + m_commandAllocatorPool.GetSize()) < m_maxCommandAllocatorPoolSize)
    {
        Log_PerfPrintf("Allocating new command allocator.");

        // create allocator
        HRESULT hResult = m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pReturnAllocator));
        if (SUCCEEDED(hResult))
        {
            // return the new allocator
            m_outstandingCommandAllocators++;
            return pReturnAllocator;
        }
        else
        {
            Log_ErrorPrintf("ID3D12Device::CreateCommandAllocator failed with hResult %08X", hResult);
            return false;
        }
    }

    // wait for one to free up (assuming pool is in ascending order, which it should be)
    pReturnAllocator = SearchPoolAndWait<ID3D12CommandAllocator>(m_commandAllocatorPool);
    if (pReturnAllocator != nullptr)
    {
        // reset the allocator, ready for use
        HRESULT hResult = pReturnAllocator->Reset();
        if (FAILED(hResult))
            Panic("ID3D12CommandAllocator::Reset failed.");

        // have one ready to go, so release it
        m_outstandingCommandAllocators++;
        return pReturnAllocator;
    }

    // shouldn't hit here ever. [unless pool allocation failed and had never succeeded]
    UnreachableCode();
    return nullptr;
}

void D3D12GraphicsCommandQueue::ReleaseCommandAllocator(ID3D12CommandAllocator *pAllocator, uint64 availableFenceValue)
{
    DebugAssert(m_outstandingCommandAllocators > 0);
    InsertIntoPool<ID3D12CommandAllocator>(m_commandAllocatorPool, pAllocator, availableFenceValue);
    m_outstandingCommandAllocators--;
}

D3D12LinearBufferHeap *D3D12GraphicsCommandQueue::RequestLinearBufferHeap()
{
    D3D12LinearBufferHeap *pReturnHeap;
    DebugAssert(Renderer::IsOnRenderThread());

    // search the pool first
    pReturnHeap = SearchPool<D3D12LinearBufferHeap>(m_linearBufferHeapPool);
    if (pReturnHeap != nullptr)
    {
        // have one ready to go, so release it
        pReturnHeap->Reset(true);
        m_outstandingLinearBufferHeaps++;
        return pReturnHeap;
    }

    // can we allocate a new one?
    if ((m_outstandingLinearBufferHeaps + m_linearBufferHeapPool.GetSize()) < m_maxLinearBufferHeapPoolSize)
    {
        Log_PerfPrintf("Allocating new linear buffer heap.");
        pReturnHeap = D3D12LinearBufferHeap::Create(m_pD3DDevice, m_linearBufferHeapSize);
        if (pReturnHeap != nullptr)
        {
            m_outstandingLinearBufferHeaps++;
            return pReturnHeap;
        }
    }

    // wait for one to free up (assuming pool is in ascending order, which it should be)
    pReturnHeap = SearchPoolAndWait<D3D12LinearBufferHeap>(m_linearBufferHeapPool);
    if (pReturnHeap != nullptr)
    {
        // have one ready to go, so release it
        pReturnHeap->Reset(true);
        m_outstandingLinearBufferHeaps++;
        return pReturnHeap;
    }

    // shouldn't hit here ever. [unless pool allocation failed and had never succeeded]
    UnreachableCode();
    return nullptr;
}

void D3D12GraphicsCommandQueue::ReleaseLinearBufferHeap(D3D12LinearBufferHeap *pHeap, uint64 availableFenceValue)
{
    DebugAssert(m_outstandingLinearBufferHeaps > 0);
    InsertIntoPool<D3D12LinearBufferHeap>(m_linearBufferHeapPool, pHeap, availableFenceValue);
    m_outstandingLinearBufferHeaps--;
}

D3D12LinearDescriptorHeap *D3D12GraphicsCommandQueue::RequestLinearViewHeap()
{
    D3D12LinearDescriptorHeap *pReturnHeap;
    DebugAssert(Renderer::IsOnRenderThread());

    // search the pool first
    pReturnHeap = SearchPool<D3D12LinearDescriptorHeap>(m_linearViewHeapPool);
    if (pReturnHeap != nullptr)
    {
        // have one ready to go, so release it
        pReturnHeap->Reset();
        m_outstandingLinearViewHeaps++;
        return pReturnHeap;
    }

    // can we allocate a new one?
    if ((m_outstandingLinearViewHeaps + m_linearViewHeapPool.GetSize()) < m_maxLinearViewHeapPoolSize)
    {
        Log_PerfPrintf("Allocating new linear view heap.");
        pReturnHeap = D3D12LinearDescriptorHeap::Create(m_pD3DDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_linearViewHeapSize);
        if (pReturnHeap != nullptr)
        {
            m_outstandingLinearViewHeaps++;
            return pReturnHeap;
        }
    }

    // wait for one to free up (assuming pool is in ascending order, which it should be)
    pReturnHeap = SearchPoolAndWait<D3D12LinearDescriptorHeap>(m_linearViewHeapPool);
    if (pReturnHeap != nullptr)
    {
        // have one ready to go, so release it
        pReturnHeap->Reset();
        m_outstandingLinearViewHeaps++;
        return pReturnHeap;
    }

    // shouldn't hit here ever. [unless pool allocation failed and had never succeeded]
    UnreachableCode();
    return nullptr;
}

void D3D12GraphicsCommandQueue::ReleaseLinearViewHeap(D3D12LinearDescriptorHeap *pHeap, uint64 availableFenceValue)
{
    DebugAssert(m_outstandingLinearViewHeaps > 0);
    InsertIntoPool<D3D12LinearDescriptorHeap>(m_linearViewHeapPool, pHeap, availableFenceValue);
    m_outstandingLinearViewHeaps--;
}

D3D12LinearDescriptorHeap *D3D12GraphicsCommandQueue::RequestLinearSamplerHeap()
{
    D3D12LinearDescriptorHeap *pReturnHeap;
    DebugAssert(Renderer::IsOnRenderThread());

    // search the pool first
    pReturnHeap = SearchPool<D3D12LinearDescriptorHeap>(m_linearSamplerHeapPool);
    if (pReturnHeap != nullptr)
    {
        // have one ready to go, so release it
        pReturnHeap->Reset();
        m_outstandingLinearSamplerHeaps++;
        return pReturnHeap;
    }

    // can we allocate a new one?
    if ((m_outstandingLinearSamplerHeaps + m_linearSamplerHeapPool.GetSize()) < m_maxLinearSamplerHeapPoolSize)
    {
        Log_PerfPrintf("Allocating new linear sampler heap.");
        pReturnHeap = D3D12LinearDescriptorHeap::Create(m_pD3DDevice, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, m_linearViewHeapSize);
        if (pReturnHeap != nullptr)
        {
            m_outstandingLinearSamplerHeaps++;
            return pReturnHeap;
        }
    }

    // wait for one to free up (assuming pool is in ascending order, which it should be)
    pReturnHeap = SearchPoolAndWait<D3D12LinearDescriptorHeap>(m_linearSamplerHeapPool);
    if (pReturnHeap != nullptr)
    {
        // have one ready to go, so release it
        pReturnHeap->Reset();
        m_outstandingLinearSamplerHeaps++;
        return pReturnHeap;
    }

    // shouldn't hit here ever. [unless pool allocation failed and had never succeeded]
    UnreachableCode();
    return nullptr;
}

void D3D12GraphicsCommandQueue::ReleaseLinearSamplerHeap(D3D12LinearDescriptorHeap *pHeap, uint64 availableFenceValue)
{
    DebugAssert(m_outstandingLinearSamplerHeaps > 0);
    InsertIntoPool<D3D12LinearDescriptorHeap>(m_linearSamplerHeapPool, pHeap, availableFenceValue);
    m_outstandingLinearSamplerHeaps--;
}

void D3D12GraphicsCommandQueue::WaitForFence(uint64 fence)
{
    UpdateLastCompletedFenceValue();

    if (m_lastCompletedFenceValue < fence)
    {
#if defined(Y_BUILD_CONFIG_DEBUG) && 0
        Timer waitTimer;
        m_pD3DFence->SetEventOnCompletion(fence, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
        Log_ProfilePrintf("GPU took %.4f ms to catch up to fence", waitTimer.GetTimeMilliseconds());
#else
        m_pD3DFence->SetEventOnCompletion(fence, m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
#endif

        UpdateLastCompletedFenceValue();
    }
}

void D3D12GraphicsCommandQueue::UpdateLastCompletedFenceValue()
{
    m_lastCompletedFenceValue = m_pD3DFence->GetCompletedValue();
}

void D3D12GraphicsCommandQueue::ScheduleResourceForDeletion(ID3D12Pageable *pResource)
{
    ScheduleResourceForDeletion(pResource, m_nextFenceValue);
}

void D3D12GraphicsCommandQueue::ScheduleResourceForDeletion(ID3D12Pageable *pResource, uint64 fenceValue /* = GetCurrentCleanupFenceValue() */)
{
    PendingDeletionResource pdr;
    pdr.pResource = pResource;
    pdr.FenceValue = fenceValue;

    m_pendingResourceLock.Lock();
    m_pendingDeletionResources.Add(pdr);
    m_pendingResourceLock.Unlock();
}

void D3D12GraphicsCommandQueue::ScheduleDescriptorForDeletion(const D3D12DescriptorHandle &handle)
{
    ScheduleDescriptorForDeletion(handle, m_nextFenceValue);
}

void D3D12GraphicsCommandQueue::ScheduleDescriptorForDeletion(const D3D12DescriptorHandle &handle, uint64 fenceValue /* = GetCurrentCleanupFenceValue() */)
{
    PendingDeletionDescriptor pdr;
    pdr.Handle = handle;
    pdr.FenceValue = fenceValue;

    m_pendingResourceLock.Lock();
    m_pendingDeletionDescriptors.Add(pdr);
    m_pendingResourceLock.Unlock();
}

//thread_local D3D12_RESOURCE_DESC rsz;

void D3D12GraphicsCommandQueue::DeletePendingResources(uint64 fenceValue)
{
    m_pendingResourceLock.Lock();

    for (uint32 i = 0; i < m_pendingDeletionResources.GetSize(); )
    {
        if (m_pendingDeletionResources[i].FenceValue > fenceValue)
        {
            i++;
            continue;
        }

        ID3D12Pageable *pResource = m_pendingDeletionResources[i].pResource;

//         ID3D12Resource *pr;
//         D3D12_RESOURCE_DESC desc;
//         if (pResource->QueryInterface(IID_PPV_ARGS(&pr)) == S_OK)
//         {
//             desc = pr->GetDesc();
//             rsz = desc;
//             pr->Release();
//         }

        SAFE_RELEASE_LAST(pResource);
        m_pendingDeletionResources.FastRemove(i);
    }

    for (uint32 i = 0; i < m_pendingDeletionDescriptors.GetSize(); )
    {
        PendingDeletionDescriptor &desc = m_pendingDeletionDescriptors[i];
        if (desc.FenceValue > fenceValue)
        {
            i++;
            continue;
        }

        DebugAssert(desc.Handle.Type < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);
        m_pBackend->GetCPUDescriptorHeap(desc.Handle.Type)->Free(desc.Handle);
        m_pendingDeletionDescriptors.FastRemove(i);
    }

    m_pendingResourceLock.Unlock();
}
