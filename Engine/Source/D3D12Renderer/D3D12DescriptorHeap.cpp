#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12DescriptorHeap.h"
Log_SetChannel(D3D12RenderBackend);

D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorCount, ID3D12DescriptorHeap *pD3DDescriptorHeap, uint32 incrementSize)
    : m_type(type)
    , m_descriptorCount(descriptorCount)
    , m_pD3DDescriptorHeap(pD3DDescriptorHeap)
    , m_CPUHandleStart(pD3DDescriptorHeap->GetCPUDescriptorHandleForHeapStart())
    , m_GPUHandleStart(pD3DDescriptorHeap->GetGPUDescriptorHandleForHeapStart())
    , m_allocationMap(descriptorCount)
    , m_incrementSize(incrementSize)
{
    m_allocationMap.Clear();
}

D3D12DescriptorHeap::~D3D12DescriptorHeap()
{
    m_pD3DDescriptorHeap->Release();
}

D3D12DescriptorHeap *D3D12DescriptorHeap::Create(ID3D12Device *pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 descriptorCount)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.Type = type;
    desc.NumDescriptors = descriptorCount;
    desc.Flags = (type < D3D12_DESCRIPTOR_HEAP_TYPE_RTV) ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    ID3D12DescriptorHeap *pD3DDescriptorHeap;
    HRESULT hResult = pDevice->CreateDescriptorHeap(&desc, __uuidof(pD3DDescriptorHeap), (void **)&pD3DDescriptorHeap);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12DescriptorHeap::Create: CreateDescriptorHeap failed with hResult %08X", hResult);
        return nullptr;
    }

    uint32 incrementSize = pDevice->GetDescriptorHandleIncrementSize(type);
    return new D3D12DescriptorHeap(type, descriptorCount, pD3DDescriptorHeap, incrementSize);
}

bool D3D12DescriptorHeap::Allocate(Handle *handle)
{
    m_mutex.Lock();

    // find a free index
    size_t index;
    if (!m_allocationMap.FindFirstClearBit(&index))
    {
        m_mutex.Unlock();
        return false;
    }

    // flag as allocated
    m_allocationMap.SetBit(index);
    m_mutex.Unlock();

    // create handle
    handle->CPUHandle.ptr = m_CPUHandleStart.ptr + index * m_incrementSize;
    handle->GPUHandle.ptr = m_GPUHandleStart.ptr + index * m_incrementSize;
    handle->Type = m_type;
    handle->StartIndex = (uint32)index;
    handle->DescriptorCount = 1;
    handle->IncrementSize = m_incrementSize;
    return true;
}

bool D3D12DescriptorHeap::AllocateRange(uint32 count, Handle *handle)
{
    m_mutex.Lock();

    // find a free index
    size_t startIndex;
    if (!m_allocationMap.FindContiguousClearBits(count, &startIndex))
    {
        m_mutex.Unlock();
        return false;
    }

    // flag as allocated
    for (uint32 i = 0; i < count; i++)
        m_allocationMap.SetBit(startIndex + i);
    m_mutex.Unlock();

    // create handle
    handle->CPUHandle.ptr = m_CPUHandleStart.ptr + startIndex * m_incrementSize;
    handle->GPUHandle.ptr = m_GPUHandleStart.ptr + startIndex * m_incrementSize;
    handle->Type = m_type;
    handle->StartIndex = (uint32)startIndex;
    handle->DescriptorCount = count;
    handle->IncrementSize = m_incrementSize;
    return true;
}

void D3D12DescriptorHeap::Free(Handle *handle)
{
    // free empty handle?
    if (handle->DescriptorCount == 0)
        return;

    // @TODO sanity check handle is correct offset and hasn't been corrupted
    uint32 startIndex = handle->StartIndex;
    DebugAssert(startIndex < m_descriptorCount);

    m_mutex.Lock();
    for (uint32 i = 0; i < handle->DescriptorCount; i++)
    {
        DebugAssert(m_allocationMap.TestBit(handle->StartIndex + i));
        m_allocationMap.UnsetBit(handle->StartIndex + i);
    }
    m_mutex.Unlock();

    // wipe handle
    handle->Clear();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::Handle::GetOffsetCPUHandle(uint32 index) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = CPUHandle;
    handle.ptr += IncrementSize * index;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::Handle::GetOffsetGPUHandle(uint32 index) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = GPUHandle;
    handle.ptr += IncrementSize * index;
    return handle;
}
