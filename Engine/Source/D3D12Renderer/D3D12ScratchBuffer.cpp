#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12ScratchBuffer.h"
#include "D3D12Renderer/D3D12RenderBackend.h"
#include "D3D12Renderer/D3D12Helpers.h"
Log_SetChannel(D3D12RenderBackend);

D3D12ScratchBuffer::D3D12ScratchBuffer(ID3D12Resource *pResource, byte *pMappedPointer, uint32 size)
    : m_gpuAddress(pResource->GetGPUVirtualAddress())
    , m_pResource(pResource)
    , m_pMappedPointer(pMappedPointer)
    , m_size(size)
    , m_position(0)
    , m_resetPosition(0)
{

}

D3D12ScratchBuffer *D3D12ScratchBuffer::Create(ID3D12Device *pDevice, uint32 size)
{
    HRESULT hResult;
    ID3D12Resource *pResource;

    // create resource
    D3D12_HEAP_PROPERTIES heapProperties = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
    D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    hResult = pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, __uuidof(ID3D12Resource), (void **)&pResource);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12ScratchBuffer::Create: CreateCommittedResource failed with hResult %08X", hResult);
        return nullptr;
    }

    byte *pMappedPointer = nullptr;
    D3D12_RANGE readRange = { 0, 0 };
    hResult = pResource->Map(0, nullptr/*&readRange*/, (void **)&pMappedPointer);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("Map failed with hResult %08X", hResult);
        return false;
    }

    return new D3D12ScratchBuffer(pResource, pMappedPointer, size);
}

D3D12ScratchBuffer::~D3D12ScratchBuffer()
{
    DebugAssert(m_pMappedPointer == nullptr);
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
    DebugAssert(m_pMappedPointer != nullptr);
    if ((m_position + size) >= m_size)
        return false;

    *pOutOffset = m_position;
    m_position += size;
    return true;
}

bool D3D12ScratchBuffer::Reset(bool resetPosition)
{
    DebugAssert(m_pMappedPointer == nullptr);
    if (resetPosition)
        m_position = 0;

    m_resetPosition = m_position;

//     D3D12_RANGE readRange = { 0, 0 };
//     HRESULT hResult = m_pResource->Map(0, nullptr/*&readRange*/, (void **)&m_pMappedPointer);
//     if (FAILED(hResult))
//     {
//         Log_ErrorPrintf("Map failed with hResult %08X", hResult);
//         return false;
//     }

    return true;
}

void D3D12ScratchBuffer::Commit()
{
//     DebugAssert(m_pMappedPointer != nullptr);
// 
//     D3D12_RANGE writtenRange = { m_resetPosition, m_position };
//     m_pResource->Unmap(0, nullptr/*&writtenRange*/);
//     m_pMappedPointer = nullptr;
}

D3D12ScratchDescriptorHeap::D3D12ScratchDescriptorHeap(ID3D12DescriptorHeap *pHeap, uint32 count, uint32 incrementSize)
    : m_pD3DHeap(pHeap)
    , m_cpuHandleStart(pHeap->GetCPUDescriptorHandleForHeapStart())
    , m_gpuHandleStart(pHeap->GetGPUDescriptorHandleForHeapStart())
    , m_size(count)
    , m_position(0)
    , m_incrementSize(incrementSize)
{

}

D3D12ScratchDescriptorHeap *D3D12ScratchDescriptorHeap::Create(ID3D12Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 count)
{
    ID3D12DescriptorHeap *pDescriptorHeap;
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = { type, count, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0 };
    HRESULT hResult = pDevice->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&pDescriptorHeap));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12ScratchDescriptorHeap::Create: CreateDescriptorHeap failed with hResult %08X", hResult);
        return nullptr;
    }

    D3D12Helpers::SetD3D12DeviceChildDebugName(pDescriptorHeap, "scratch descriptor heap");
    return new D3D12ScratchDescriptorHeap(pDescriptorHeap, count, pDevice->GetDescriptorHandleIncrementSize(type));
}

D3D12ScratchDescriptorHeap::~D3D12ScratchDescriptorHeap()
{
    D3D12RenderBackend::GetInstance()->ScheduleResourceForDeletion(m_pD3DHeap);
}

bool D3D12ScratchDescriptorHeap::Allocate(uint32 count, D3D12_CPU_DESCRIPTOR_HANDLE *pOutCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE *pOutGPUHandle)
{
    DebugAssert(count > 0);
    if ((m_position + count) > m_size)
        return false;

    pOutCPUHandle->ptr = m_cpuHandleStart.ptr + m_incrementSize * m_position;
    pOutGPUHandle->ptr = m_gpuHandleStart.ptr + m_incrementSize * m_position;
    m_position += count;
    return true;
}

void D3D12ScratchDescriptorHeap::Reset()
{
    m_position = 0;
}
