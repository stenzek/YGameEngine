#pragma once
#include "D3D12Renderer/D3D12Common.h"

class D3D12ScratchBuffer
{
    // @TODO maybe use something like a circular buffer here?
public:
    static D3D12ScratchBuffer *Create(ID3D12Device *pDevice, uint32 size);
    ~D3D12ScratchBuffer();

    ID3D12Resource *GetResource() const { return m_pResource; }
    uint32 GetSize() const { return m_size; }

    void *GetPointer(uint32 offset) const;
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress(uint32 offset) const;

    bool Allocate(uint32 size, uint32 *pOutOffset);
    void Reset();
    
private:
    D3D12ScratchBuffer(ID3D12Resource *pResource, byte *pMappedPointer, uint32 size);

    D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddress;

    ID3D12Resource *m_pResource;
    byte *m_pMappedPointer;

    uint32 m_size;
    uint32 m_position;
};

class D3D12ScratchDescriptorHeap
{
public:
    static D3D12ScratchDescriptorHeap *Create(ID3D12Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 count);
    ~D3D12ScratchDescriptorHeap();

    ID3D12DescriptorHeap *GetD3DHeap() const { return m_pD3DHeap; }
    uint32 GetSize() const { return m_size; }

    bool Allocate(uint32 count, D3D12_CPU_DESCRIPTOR_HANDLE *pOutCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE *pOutGPUHandle);

    void Reset();

private:
    D3D12ScratchDescriptorHeap(ID3D12DescriptorHeap *pHeap, uint32 count, uint32 incrementSize);

    ID3D12DescriptorHeap *m_pD3DHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandleStart;
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandleStart;
    uint32 m_size;
    uint32 m_position;
    uint32 m_incrementSize;
};
