#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUContext.h"
#include "D3D12Renderer/D3D12GPUOutputBuffer.h"
#include "D3D12Renderer/D3D12Helpers.h"
#include "D3D12Renderer/D3D12CVars.h"
#include "Engine/EngineCVars.h"
#include "Renderer/ShaderConstantBuffer.h"
Log_SetChannel(D3D12GPUDevice);

D3D12CommandQueue::D3D12CommandQueue(D3D12GPUDevice *pGPUDevice, ID3D12Device *pD3DDevice, D3D12_COMMAND_LIST_TYPE type, uint32 linearBufferHeapSize, uint32 linearViewHeapSize, uint32 linearSamplerHeapSize)
    : m_pGPUDevice(pGPUDevice)
    , m_pD3DDevice(pD3DDevice)
    , m_type(type)
    , m_linearBufferHeapSize(linearBufferHeapSize)
    , m_linearViewHeapSize(linearViewHeapSize)
    , m_linearSamplerHeapSize(linearSamplerHeapSize)
    , m_maxCommandAllocatorPoolSize(16)
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

D3D12CommandQueue::~D3D12CommandQueue()
{
    // sync, then destroy everything
    if (m_pD3DCommandQueue != nullptr)
    {
        uint64 fenceValue = CreateSynchronizationPoint();
        WaitForFence(fenceValue);
    }

    // shouldn't have anything outstanding
    Assert(m_outstandingCommandLists == 0);
    Assert(m_outstandingCommandAllocators == 0);
    Assert(m_outstandingLinearBufferHeaps == 0);
    Assert(m_outstandingLinearViewHeaps == 0);
    Assert(m_outstandingLinearSamplerHeaps == 0);

    // nuke command lists
    while (!m_commandListPool.IsEmpty())
        m_commandListPool.PopBack()->Release();
    
    // nuke command allocators
    while (!m_commandAllocatorPool.IsEmpty())
    {
        CommandAllocatorEntry entry;
        m_commandAllocatorPool.PopBack(&entry);
        entry.Key->Release();
    }

    // nuke linear buffer heaps
    while (!m_linearBufferHeapPool.IsEmpty())
    {
        LinearBufferHeapEntry entry;
        m_linearBufferHeapPool.PopBack(&entry);
        delete entry.Key;
    }

    // nuke linear view heaps
    while (!m_linearViewHeapPool.IsEmpty())
    {
        LinearResourceDescriptorHeapEntry entry;
        m_linearViewHeapPool.PopBack(&entry);
        delete entry.Key;
    }

    // nuke linear view heaps
    while (!m_linearSamplerHeapPool.IsEmpty())
    {
        LinearSamplerDescriptorHeapEntry entry;
        m_linearSamplerHeapPool.PopBack(&entry);
        delete entry.Key;
    }

    // nuke descriptors
    for (uint32 i = 0; i < m_pendingDeletionDescriptors.GetSize(); i++)
        m_pGPUDevice->GetCPUDescriptorHeap(m_pendingDeletionDescriptors[i].Handle.Type)->Free(m_pendingDeletionDescriptors[i].Handle);

    // nuke resources
    for (uint32 i = 0; i < m_pendingDeletionResources.GetSize(); i++)
        m_pendingDeletionResources[i].pResource->Release();

    if (m_fenceEvent != NULL)
        CloseHandle(m_fenceEvent);

    SAFE_RELEASE(m_pD3DFence);
    SAFE_RELEASE(m_pD3DCommandQueue);
}

bool D3D12CommandQueue::Initialize()
{
    HRESULT hResult;

    // allocate command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = { m_type, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE, 1 };
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

uint64 D3D12CommandQueue::CreateSynchronizationPoint()
{
    uint64 returnValue = m_nextFenceValue++;
    HRESULT hResult = m_pD3DCommandQueue->Signal(m_pD3DFence, returnValue);
    if (FAILED(hResult))
        Log_WarningPrintf("ID3D12CommandQueue::Signal failed with hResult %08X", hResult);

    return returnValue;
}

void D3D12CommandQueue::ExecuteCommandList(ID3D12CommandList *pCommandList)
{
    m_pD3DCommandQueue->ExecuteCommandLists(1, &pCommandList);
}

ID3D12GraphicsCommandList *D3D12CommandQueue::RequestCommandList()
{
    RecursiveMutexLock lock(m_allocatorLock);

    // free one
    if (!m_commandListPool.IsEmpty())
    {
        m_outstandingCommandLists++;
        return m_commandListPool.PopFront();
    }

    // create one, we need a temporary allocator
    ID3D12CommandAllocator *pTemporaryAllocator = RequestCommandAllocator();
    ID3D12GraphicsCommandList *pCommandList;
    HRESULT hResult = m_pD3DDevice->CreateCommandList(1, m_type, pTemporaryAllocator, nullptr, IID_PPV_ARGS(&pCommandList));
    if (FAILED(hResult))
        Panic("Failed to create command list.");

    // close the command list
    hResult = pCommandList->Close();
    if (FAILED(hResult))
        Log_WarningPrintf("Closing new command list failed with HRESULT %08X", hResult);

    // and release allocator back
    ReleaseCommandAllocator(pTemporaryAllocator, m_lastCompletedFenceValue);

    // return new list
    m_outstandingCommandLists++;
    return pCommandList;
}

ID3D12GraphicsCommandList *D3D12CommandQueue::RequestAndOpenCommandList(ID3D12CommandAllocator *pCommandAllocator)
{
    RecursiveMutexLock lock(m_allocatorLock);

    // free one
    if (!m_commandListPool.IsEmpty())
    {
        ID3D12GraphicsCommandList *pCommandList = m_commandListPool.PopFront();
        HRESULT hResult = pCommandList->Reset(pCommandAllocator, nullptr);
        if (FAILED(hResult))
            Panic("Failed to reset pooled command list.");

        m_outstandingCommandLists++;
        return pCommandList;
    }

    // create a new list on the specified allocator
    ID3D12GraphicsCommandList *pCommandList;
    HRESULT hResult = m_pD3DDevice->CreateCommandList(1, m_type, pCommandAllocator, nullptr, IID_PPV_ARGS(&pCommandList));
    if (FAILED(hResult))
        Panic("Failed to create command list.");

    // return new list
    m_outstandingCommandLists++;
    return pCommandList;
}

void D3D12CommandQueue::ReleaseCommandList(ID3D12GraphicsCommandList *pCommandList)
{
    DebugAssert(m_outstandingCommandLists > 0);
    m_commandListPool.Add(pCommandList);
    m_outstandingCommandLists--;
}

void D3D12CommandQueue::ReleaseFailedCommandList(ID3D12GraphicsCommandList *pCommandList)
{
    // don't put it back in the pool
    DebugAssert(m_outstandingCommandLists > 0);
    m_outstandingCommandLists--;
    pCommandList->Release();
}

template<class T>
T *D3D12CommandQueue::SearchPool(MemArray<KeyValuePair<T *, uint64>> &pool)
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
T *D3D12CommandQueue::SearchPoolAndWait(MemArray<KeyValuePair<T *, uint64>> &pool)
{
    DebugAssert(!pool.IsEmpty());
    
    WaitForFence(pool[0].Value);

    T *pReturn = pool[0].Key;
    pool.RemoveFront();
    return pReturn;
}

template<class T>
void D3D12CommandQueue::InsertIntoPool(MemArray<KeyValuePair<T *, uint64>> &pool, T *pItem, uint64 fenceValue)
{
    uint32 pos = 0;
    for (; pos < pool.GetSize(); pos++)
    {
        if (pool[pos].Value > fenceValue)
            break;
    }

    pool.Insert(KeyValuePair<T *, uint64>(pItem, fenceValue), pos);
}

ID3D12CommandAllocator *D3D12CommandQueue::RequestCommandAllocator()
{
    ID3D12CommandAllocator *pReturnAllocator;
    RecursiveMutexLock lock(m_allocatorLock);

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
        HRESULT hResult = m_pD3DDevice->CreateCommandAllocator(m_type, IID_PPV_ARGS(&pReturnAllocator));
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

void D3D12CommandQueue::ReleaseCommandAllocator(ID3D12CommandAllocator *pAllocator, uint64 availableFenceValue)
{
    RecursiveMutexLock lock(m_allocatorLock);

    DebugAssert(m_outstandingCommandAllocators > 0);
    InsertIntoPool<ID3D12CommandAllocator>(m_commandAllocatorPool, pAllocator, availableFenceValue);
    m_outstandingCommandAllocators--;
}

D3D12LinearBufferHeap *D3D12CommandQueue::RequestLinearBufferHeap()
{
    D3D12LinearBufferHeap *pReturnHeap;
    RecursiveMutexLock lock(m_allocatorLock);

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

void D3D12CommandQueue::ReleaseLinearBufferHeap(D3D12LinearBufferHeap *pHeap, uint64 availableFenceValue)
{
    RecursiveMutexLock lock(m_allocatorLock);
    
    DebugAssert(m_outstandingLinearBufferHeaps > 0);
    InsertIntoPool<D3D12LinearBufferHeap>(m_linearBufferHeapPool, pHeap, availableFenceValue);
    m_outstandingLinearBufferHeaps--;
}

D3D12LinearDescriptorHeap *D3D12CommandQueue::RequestLinearViewHeap()
{
    D3D12LinearDescriptorHeap *pReturnHeap;
    RecursiveMutexLock lock(m_allocatorLock);

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

void D3D12CommandQueue::ReleaseLinearViewHeap(D3D12LinearDescriptorHeap *pHeap, uint64 availableFenceValue)
{
    RecursiveMutexLock lock(m_allocatorLock);
    
    DebugAssert(m_outstandingLinearViewHeaps > 0);
    InsertIntoPool<D3D12LinearDescriptorHeap>(m_linearViewHeapPool, pHeap, availableFenceValue);
    m_outstandingLinearViewHeaps--;
}

D3D12LinearDescriptorHeap *D3D12CommandQueue::RequestLinearSamplerHeap()
{
    D3D12LinearDescriptorHeap *pReturnHeap;
    RecursiveMutexLock lock(m_allocatorLock);

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

void D3D12CommandQueue::ReleaseLinearSamplerHeap(D3D12LinearDescriptorHeap *pHeap, uint64 availableFenceValue)
{
    RecursiveMutexLock lock(m_allocatorLock);

    DebugAssert(m_outstandingLinearSamplerHeaps > 0);
    InsertIntoPool<D3D12LinearDescriptorHeap>(m_linearSamplerHeapPool, pHeap, availableFenceValue);
    m_outstandingLinearSamplerHeaps--;
}

void D3D12CommandQueue::WaitForFence(uint64 fence)
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

void D3D12CommandQueue::UpdateLastCompletedFenceValue()
{
    m_lastCompletedFenceValue = m_pD3DFence->GetCompletedValue();
}

void D3D12CommandQueue::ReleaseStaleResources()
{
    // clean up as much as we can
    UpdateLastCompletedFenceValue();
    DeletePendingResources(m_lastCompletedFenceValue);
}

void D3D12CommandQueue::ScheduleResourceForDeletion(ID3D12Pageable *pResource)
{
    ScheduleResourceForDeletion(pResource, m_nextFenceValue);
}

void D3D12CommandQueue::ScheduleResourceForDeletion(ID3D12Pageable *pResource, uint64 fenceValue /* = GetCurrentCleanupFenceValue() */)
{
    PendingDeletionResource pdr;
    pdr.pResource = pResource;
    pdr.FenceValue = fenceValue;

    m_pendingResourceLock.Lock();
    m_pendingDeletionResources.Add(pdr);
    m_pendingResourceLock.Unlock();
}

void D3D12CommandQueue::ScheduleDescriptorForDeletion(const D3D12DescriptorHandle &handle)
{
    ScheduleDescriptorForDeletion(handle, m_nextFenceValue);
}

void D3D12CommandQueue::ScheduleDescriptorForDeletion(const D3D12DescriptorHandle &handle, uint64 fenceValue /* = GetCurrentCleanupFenceValue() */)
{
    PendingDeletionDescriptor pdr;
    pdr.Handle = handle;
    pdr.FenceValue = fenceValue;

    m_pendingResourceLock.Lock();
    m_pendingDeletionDescriptors.Add(pdr);
    m_pendingResourceLock.Unlock();
}

//thread_local D3D12_RESOURCE_DESC rsz;

void D3D12CommandQueue::DeletePendingResources(uint64 fenceValue)
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
        m_pGPUDevice->GetCPUDescriptorHeap(desc.Handle.Type)->Free(desc.Handle);
        m_pendingDeletionDescriptors.FastRemove(i);
    }

    m_pendingResourceLock.Unlock();
}
