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