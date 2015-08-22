#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12ScratchBuffer.h"
#include "D3D12Renderer/D3D12RenderBackend.h"
Log_SetChannel(D3D12RenderBackend);

D3D12ScratchBuffer::D3D12ScratchBuffer(ID3D12Resource *pResource, byte *pMappedPointer, uint32 size)
    : m_gpuAddress(pResource->GetGPUVirtualAddress())
    , m_pResource(pResource)
    , m_pMappedPointer(pMappedPointer)
    , m_size(size)
    , m_position(0)
{

}

D3D12ScratchBuffer *D3D12ScratchBuffer::Create(ID3D12Device *pDevice, uint32 size)
{
    HRESULT hResult;
    ID3D12Resource *pResource;

    // create resource
    D3D12_HEAP_PROPERTIES heapProperties = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
    D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 0, 0 }, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE };
    hResult = pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, &resourceDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, __uuidof(ID3D12Resource), (void **)&pResource);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12ScratchBuffer::Create: CreateCommittedResource failed with hResult %08X", hResult);
        return nullptr;
    }

    // map it
    D3D12_RANGE mapRange = { 0, size - 1 };
    byte *pMappedPointer;
    hResult = pResource->Map(0, &mapRange, (void**)&pMappedPointer);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12ScratchBuffer::Create: Mapping new buffer failed with hResult %08X", hResult);
        pResource->Release();
        return nullptr;
    }

    return new D3D12ScratchBuffer(pResource, pMappedPointer, size);
}

D3D12ScratchBuffer::~D3D12ScratchBuffer()
{
    D3D12_RANGE unmapRange = { 0, m_size - 1 };
    m_pResource->Unmap(0, &unmapRange);
    D3D12RenderBackend::GetInstance()->ScheduleResourceForDeletion(m_pResource);
}

void *D3D12ScratchBuffer::GetPointer(uint32 offset) const
{
    DebugAssert(offset < m_size);
    return m_pMappedPointer + offset;
}

D3D12_GPU_VIRTUAL_ADDRESS D3D12ScratchBuffer::GetGPUAddress(uint32 offset) const
{
    DebugAssert(offset < m_size);
    return m_gpuAddress + offset;
}

bool D3D12ScratchBuffer::Allocate(uint32 size, uint32 *pOutOffset)
{
    if ((m_position + size) >= m_size)
        return false;

    *pOutOffset = m_position;
    m_position += size;
    return true;
}

void D3D12ScratchBuffer::Reset()
{
    m_position = 0;
}
