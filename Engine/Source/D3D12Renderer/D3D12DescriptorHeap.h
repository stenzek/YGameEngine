#pragma once
#include "D3D12Renderer/D3D12Common.h"

class D3D12DescriptorHeap
{
public:
    struct Handle
    {
        D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
        D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
        uint32 StartIndex;
        uint32 DescriptorCount;
        uint32 IncrementSize;

        // constructors
        Handle() { Y_memzero(this, sizeof(*this)); }
        Handle(const Handle &handle) { Y_memcpy(this, &handle, sizeof(*this)); }
        Handle &operator=(const Handle &handle) { Y_memcpy(this, &handle, sizeof(*this)); }
        void Clear() { Y_memzero(this, sizeof(*this)); }

        // Helper functions
        D3D12_CPU_DESCRIPTOR_HANDLE GetOffsetCPUHandle(uint32 index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetOffsetGPUHandle(uint32 index) const;

        // Get this handle
        operator D3D12_CPU_DESCRIPTOR_HANDLE () const { return CPUHandle; }
        operator D3D12_GPU_DESCRIPTOR_HANDLE () const { return GPUHandle; }
    };

public:
    ~D3D12DescriptorHeap();

    static D3D12DescriptorHeap *Create(ID3D12Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorCount);

    bool Allocate(Handle *handle);
    bool AllocateRange(uint32 count, Handle *handle);
    void Free(Handle *handle);

private:
    D3D12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorCount, ID3D12DescriptorHeap *pD3DDescriptorHeap, uint32 incrementSize);

    D3D12_DESCRIPTOR_HEAP_TYPE m_type;
    uint32 m_descriptorCount;

    ID3D12DescriptorHeap *m_pD3DDescriptorHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandleStart;
    D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandleStart;
    BitSet32 m_allocationMap;
    uint32 m_incrementSize;

    Mutex m_mutex;
};
