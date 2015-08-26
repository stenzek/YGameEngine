#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12GPUBuffer.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUContext.h"
Log_SetChannel(D3D12RenderBackend);

D3D12GPUBuffer::D3D12GPUBuffer(const GPU_BUFFER_DESC *pBufferDesc, ID3D12Resource *pD3DResource, ID3D12Resource *pD3DReadBackResource, ID3D12Resource *pD3DStagingResource)
    : GPUBuffer(pBufferDesc)
    , m_pD3DResource(pD3DResource)
    , m_pD3DReadBackResource(pD3DReadBackResource)
    , m_pD3DStagingResource(pD3DStagingResource)
    , m_pMappedContext(nullptr)
    , m_pMappedPointer(nullptr)
{
    // @TODO virtual call bad here, should just adjust the counter directly.
    g_pRenderer->GetStats()->OnResourceCreated(this);
}

D3D12GPUBuffer::~D3D12GPUBuffer()
{
    g_pRenderer->GetStats()->OnResourceDeleted(this);

    DebugAssert(m_pMappedContext == nullptr);
    SAFE_RELEASE(m_pD3DReadBackResource);
    SAFE_RELEASE(m_pD3DStagingResource);
    SAFE_RELEASE(m_pD3DResource);
}

void D3D12GPUBuffer::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = m_desc.Size;
}

void D3D12GPUBuffer::SetDebugName(const char *debugName)
{
    D3D12Helpers::SetD3D12DeviceChildDebugName(m_pD3DResource, debugName);

    if (m_pD3DReadBackResource != nullptr)
        D3D12Helpers::SetD3D12DeviceChildDebugName(m_pD3DReadBackResource, SmallString::FromFormat("%s_STAGING", debugName));

    if (m_pD3DStagingResource != nullptr)
        D3D12Helpers::SetD3D12DeviceChildDebugName(m_pD3DStagingResource, SmallString::FromFormat("%s_STAGING", debugName));
}

GPUBuffer *D3D12GPUDevice::CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData /* = NULL */)
{
    // create main resource
    ID3D12Resource *pResource;
    D3D12_HEAP_PROPERTIES heapProperties = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
    D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, pDesc->Size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    HRESULT hResult = m_pD3DDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pResource));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommitedResource for main resource failed with hResult %08X", hResult);
        return false;
    }

    // create upload resource
    if (pInitialData != nullptr)
    {
        ID3D12Resource *pUploadResource;
        D3D12_HEAP_PROPERTIES uploadHeapProperties = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
        D3D12_RESOURCE_DESC uploadResourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, pDesc->Size, 1, 1, 1, DXGI_FORMAT_UNKNOWN,{ 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE };
        hResult = m_pD3DDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS, &resourceDesc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&pUploadResource));
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("CreateCommitedResource for upload resource failed with hResult %08X", hResult);
            pResource->Release();
            return false;
        }

        // map the upload resource
        D3D12_RANGE mapRange = { 0, pDesc->Size - 1 };
        void *pMappedPointer;
        hResult = pUploadResource->Map(0, &mapRange, &pMappedPointer);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("Map for upload resource failed with hResult %08X", hResult);
            pUploadResource->Release();
            pResource->Release();
            return false;
        }

        // fill it
        Y_memcpy(pMappedPointer, pInitialData, pDesc->Size);
        pUploadResource->Unmap(0, &mapRange);

        // add to copy command queue
        GetCommandList()->CopyBufferRegion(pResource, 0, pUploadResource, 0, pDesc->Size);
        FlushCopyQueue();
        ScheduleUploadResourceDeletion(pUploadResource);
    }

    // create readback resource
    ID3D12Resource *pReadBackResource = nullptr;
    if (pDesc->Flags & GPU_BUFFER_FLAG_READABLE)
    {
        // @TODO
    }

    // create staging resource (for mapping)
    ID3D12Resource *pMappingResource = nullptr;
    if (pDesc->Flags & GPU_BUFFER_FLAG_MAPPABLE)
    {
        // @TODO
    }

    return new D3D12GPUBuffer(pDesc, pResource, pReadBackResource, pMappingResource);
}

bool D3D12GPUContext::ReadBuffer(GPUBuffer *pBuffer, void *pDestination, uint32 start, uint32 count)
{
#if 0
    D3D12GPUBuffer *pD3D12Buffer = static_cast<D3D12GPUBuffer *>(pBuffer);
    DebugAssert((start + count) <= pD3D12Buffer->GetDesc()->Size);
    DebugAssert(pD3D12Buffer->GetDesc()->Flags & GPU_BUFFER_FLAG_READABLE);

    // Copy from the GPU buffer to the staging buffer
    bool fullCopy = (start == 0 && count == pBuffer->GetDesc()->Size);
    if (fullCopy)
    {
        // as there isn't multiple mip levels we can copy the whole resource
        m_pD3DContext->CopyResource(pD3D12Buffer->GetD3DStagingBuffer(), pD3D12Buffer->GetD3DBuffer());
    }
    else
    {
        // calculate box
        D3D12_BOX box = { start, 0, 0, start + count, 1, 1 };
        m_pD3DContext->CopySubresourceRegion(pD3D12Buffer->GetD3DStagingBuffer(), 0, 0, 0, 0, pD3D12Buffer->GetD3DBuffer(), 0, &box);
    }

    // map the staging buffer into memory
    D3D12_MAPPED_SUBRESOURCE mappedSubresource;
    HRESULT hResult = m_pD3DContext->Map(pD3D12Buffer->GetD3DStagingBuffer(), 0, D3D12_MAP_READ, 0, &mappedSubresource);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12GPUContext::ReadBuffer: Map of staging buffer failed with hResult %08X", hResult);
        return false;
    }

    // copy into our memory
    Y_memcpy(pDestination, mappedSubresource.pData, count);
    m_pD3DContext->Unmap(pD3D12Buffer->GetD3DStagingBuffer(), 0);
    return true;
#endif

    // @TODO slow path, as the command list has to be flushed.. but this will break our bindings. need to work out how to do this.
    return false;
}

bool D3D12GPUContext::WriteBuffer(GPUBuffer *pBuffer, const void *pSource, uint32 start, uint32 count)
{
#if 0
    D3D12GPUBuffer *pD3D12Buffer = static_cast<D3D12GPUBuffer *>(pBuffer);
    DebugAssert(pD3D12Buffer->GetDesc()->Flags & GPU_BUFFER_FLAG_WRITABLE);
    DebugAssert((start + count) <= pD3D12Buffer->GetDesc()->Size);

    bool fullCopy = (start == 0 && count == pBuffer->GetDesc()->Size);
    if (pD3D12Buffer->GetDesc()->Flags & GPU_BUFFER_FLAG_MAPPABLE)
    {
        // Have to use map
        D3D12_MAPPED_SUBRESOURCE mappedSubresource;
        HRESULT hResult = m_pD3DContext->Map(pD3D12Buffer->GetD3DBuffer(), 0, (fullCopy) ? D3D12_MAP_WRITE_DISCARD : D3D12_MAP_WRITE, 0, &mappedSubresource);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D12GPUContext::WriteBuffer: Map failed with HResult %08X", hResult);
            return false;
        }

        // write and unmap
        Y_memcpy(reinterpret_cast<byte *>(mappedSubresource.pData) + start, pSource, count);
        m_pD3DContext->Unmap(pD3D12Buffer->GetD3DBuffer(), 0);
    }
    else
    {
        // Use UpdateSubresource
        if (fullCopy)
        {
            // whole buffer
            m_pD3DContext->UpdateSubresource(pD3D12Buffer->GetD3DBuffer(), 0, NULL, pSource, 0, 0);
        }
        else
        {
            // partial buffer, use box
            D3D12_BOX box = { start, 0, 0, start + count, 1, 1 };
            m_pD3DContext->UpdateSubresource(pD3D12Buffer->GetD3DBuffer(), 0, &box, pSource, 0, 0);
        }
    }
    return true;
#endif
    
    // @TODO if buffer size is smaller than a threshold, use the scratch buffer, otherwise the mapping buffer??
    return false;
}

bool D3D12GPUContext::MapBuffer(GPUBuffer *pBuffer, GPU_MAP_TYPE mapType, void **ppPointer)
{
#if 0
    D3D12GPUBuffer *pD3D12Buffer = static_cast<D3D12GPUBuffer *>(pBuffer);
    DebugAssert(pD3D12Buffer->GetDesc()->Flags & GPU_BUFFER_FLAG_MAPPABLE);
    DebugAssert(pD3D12Buffer->GetMappedContext() == NULL && pD3D12Buffer->GetMappedPointer() == NULL);

    D3D12_MAPPED_SUBRESOURCE mappedSubresource;
    HRESULT hResult = m_pD3DContext->Map(pD3D12Buffer->GetD3DBuffer(), 0, D3D12TypeConversion::MapTypetoD3D12MapType(mapType), 0, &mappedSubresource);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D12GPUContext::MapBuffer: Failed with HResult %08X", hResult);
        return false;
    }

    pD3D12Buffer->SetMappedContextPointer(this, mappedSubresource.pData);
    *ppPointer = mappedSubresource.pData;
    return true;
#endif
    // @TODO use mapping buffer
    // obviously sync issue here if the buffer is mapped twice before the buffer is done with
    // maybe the best solution is to just create buffers on-demand? they're on the cpu, so
    // allocation can't be *that* slow
    return false;
}

void D3D12GPUContext::Unmapbuffer(GPUBuffer *pBuffer, void *pPointer)
{
#if 0
    D3D12GPUBuffer *pD3D12Buffer = static_cast<D3D12GPUBuffer *>(pBuffer);
    DebugAssert(pD3D12Buffer->GetDesc()->Flags & GPU_BUFFER_FLAG_MAPPABLE);
    DebugAssert(pD3D12Buffer->GetMappedContext() == this && pD3D12Buffer->GetMappedPointer() == pPointer);

    m_pD3DContext->Unmap(pD3D12Buffer->GetD3DBuffer(), 0);
    pD3D12Buffer->SetMappedContextPointer(NULL, NULL);
#endif
}

