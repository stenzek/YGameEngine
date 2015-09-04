#pragma once
#include "D3D12Renderer/D3D12Common.h"

struct D3D12DescriptorHandle
{
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
    D3D12_DESCRIPTOR_HEAP_TYPE Type;
    uint32 StartIndex;
    uint32 DescriptorCount;
    uint32 IncrementSize;

    // constructors
    D3D12DescriptorHandle() { Y_memzero(this, sizeof(*this)); }
    D3D12DescriptorHandle(const D3D12DescriptorHandle &handle) { Y_memcpy(this, &handle, sizeof(*this)); }
    D3D12DescriptorHandle &operator=(const D3D12DescriptorHandle &handle) { Y_memcpy(this, &handle, sizeof(*this)); return *this; }
    void Clear() { Y_memzero(this, sizeof(*this)); }

    // Helper functions
    D3D12_CPU_DESCRIPTOR_HANDLE GetOffsetCPUHandle(uint32 index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetOffsetGPUHandle(uint32 index) const;
    bool IsNull() const { return (DescriptorCount == 0); }

    // Get this handle
    operator D3D12_CPU_DESCRIPTOR_HANDLE () const { return CPUHandle; }
    operator D3D12_GPU_DESCRIPTOR_HANDLE () const { return GPUHandle; }

    // Comparisons
    bool operator==(const D3D12DescriptorHandle &handle) const { return (CPUHandle.ptr == handle.CPUHandle.ptr && Type == handle.Type); }
    bool operator|=(const D3D12DescriptorHandle &handle) const { return (CPUHandle.ptr != handle.CPUHandle.ptr || Type != handle.Type); }
};

class D3D12DescriptorHeap
{
public:
    ~D3D12DescriptorHeap();

    static D3D12DescriptorHeap *Create(ID3D12Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorCount, bool cpuOnly);

    ID3D12DescriptorHeap *GetD3DHeap() const { return m_pD3DDescriptorHeap; }

    bool Allocate(D3D12DescriptorHandle *handle);
    bool AllocateRange(uint32 count, D3D12DescriptorHandle *handle);
    void Free(D3D12DescriptorHandle &handle);

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
