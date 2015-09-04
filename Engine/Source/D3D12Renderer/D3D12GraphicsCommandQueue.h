#pragma once
#include "D3D12Renderer/D3D12Common.h"
#include "D3D12Renderer/D3D12DescriptorHeap.h"
#include "D3D12Renderer/D3D12LinearHeaps.h"

class D3D12GraphicsCommandQueue
{
public:
    D3D12GraphicsCommandQueue(D3D12RenderBackend *pBackend, ID3D12Device *pDevice, uint32 linearBufferHeapSize, uint32 linearViewHeapSize, uint32 linearSamplerHeapSize);
    ~D3D12GraphicsCommandQueue();

    // accessors
    const uint32 GetLinearBufferHeapSize() const { return m_linearBufferHeapSize; }
    const uint32 GetLinearViewHeapSize() const { return m_linearViewHeapSize; }
    const uint32 GetLinearSamplerHeapSize() const { return m_linearSamplerHeapSize; }
    ID3D12CommandQueue *GetD3DCommandQueue() const { return m_pD3DCommandQueue; }

    // initialization
    bool Initialize();

    // cleanup any resources that have had the fence passed
    void ReleaseStaleResources();

    // increment the fence value without executing anything
    uint64 CreateSynchronizationPoint();

    // execute a command list. returns the fence value once it has finished on the gpu.
    uint64 ExecuteCommandList(ID3D12CommandList *pCommandList);

    // command allocator request, release
    ID3D12CommandAllocator *RequestCommandAllocator();
    void ReleaseCommandAllocator(ID3D12CommandAllocator *pAllocator, uint64 availableFenceValue);

    // linear buffer request, release
    D3D12LinearBufferHeap *RequestLinearBufferHeap();
    void ReleaseLinearBufferHeap(D3D12LinearBufferHeap *pHeap, uint64 availableFenceValue);

    // linear resource descriptor heap request, release
    D3D12LinearDescriptorHeap *RequestLinearViewHeap();
    void ReleaseLinearViewHeap(D3D12LinearDescriptorHeap *pHeap, uint64 availableFenceValue);

    // linear sampler descriptor heap request, release
    D3D12LinearDescriptorHeap *RequestLinearSamplerHeap();
    void ReleaseLinearSamplerHeap(D3D12LinearDescriptorHeap *pHeap, uint64 availableFenceValue);

    // waits until the specified fence value is completed
    void WaitForFence(uint64 fence);

    // resource cleanup
    void ScheduleResourceForDeletion(ID3D12Pageable *pResource);
    void ScheduleResourceForDeletion(ID3D12Pageable *pResource, uint64 fenceValue);
    void ScheduleDescriptorForDeletion(const D3D12DescriptorHandle &handle);
    void ScheduleDescriptorForDeletion(const D3D12DescriptorHandle &handle, uint64 fenceValue);

private:
    void UpdateLastCompletedFenceValue();
    void DeletePendingResources(uint64 fenceValue);
    template<class T> T *SearchPool(MemArray<KeyValuePair<T *, uint64>> &pool);
    template<class T> T *SearchPoolAndWait(MemArray<KeyValuePair<T *, uint64>> &pool);
    template<class T> void InsertIntoPool(MemArray<KeyValuePair<T *, uint64>> &pool, T *pItem, uint64 fenceValue);

    // parents
    D3D12RenderBackend *m_pBackend;
    ID3D12Device *m_pD3DDevice;
    uint32 m_linearBufferHeapSize;
    uint32 m_linearViewHeapSize;
    uint32 m_linearSamplerHeapSize;
    uint32 m_maxCommandAllocatorPoolSize;
    uint32 m_maxLinearBufferHeapPoolSize;
    uint32 m_maxLinearViewHeapPoolSize;
    uint32 m_maxLinearSamplerHeapPoolSize;

    // actual command queue
    ID3D12CommandQueue *m_pD3DCommandQueue;
    ID3D12Fence *m_pD3DFence;
    HANDLE m_fenceEvent;
    uint64 m_nextFenceValue;
    uint64 m_lastCompletedFenceValue;

    // object scheduled for deletion
    struct PendingDeletionResource
    {
        ID3D12Pageable *pResource;
        uint64 FenceValue;
    };
    struct PendingDeletionDescriptor
    {
        D3D12DescriptorHandle Handle;
        uint64 FenceValue;
    };
    MemArray<PendingDeletionResource> m_pendingDeletionResources;
    MemArray<PendingDeletionDescriptor> m_pendingDeletionDescriptors;
    Mutex m_pendingResourceLock;

//     // command list pool
//     typedef KeyValuePair<ID3D12CommandList *, uint64> CommandListEntry;
//     MemArray<CommandListEntry> m_commandListPool;

    // command allocator pool
    typedef KeyValuePair<ID3D12CommandAllocator *, uint64> CommandAllocatorEntry;
    MemArray<CommandAllocatorEntry> m_commandAllocatorPool;
    uint32 m_outstandingCommandAllocators;

    // linear buffer pool
    typedef KeyValuePair<D3D12LinearBufferHeap *, uint64> LinearBufferHeapEntry;
    MemArray<LinearBufferHeapEntry> m_linearBufferHeapPool;
    uint32 m_outstandingLinearBufferHeaps;

    // resource descriptor pool
    typedef KeyValuePair<D3D12LinearDescriptorHeap *, uint64> LinearResourceDescriptorHeapEntry;
    MemArray<LinearResourceDescriptorHeapEntry> m_linearViewHeapPool;
    uint32 m_outstandingLinearViewHeaps;

    // sampler descriptor pool
    typedef KeyValuePair<D3D12LinearDescriptorHeap *, uint64> LinearSamplerDescriptorHeapEntry;
    MemArray<LinearSamplerDescriptorHeapEntry> m_linearSamplerHeapPool;
    uint32 m_outstandingLinearSamplerHeaps;

};
