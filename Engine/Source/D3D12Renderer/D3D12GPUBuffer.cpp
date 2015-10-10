#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12GPUBuffer.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUContext.h"
#include "D3D12Renderer/D3D12Helpers.h"
Log_SetChannel(D3D12RenderBackend);

D3D12GPUBuffer::D3D12GPUBuffer(const GPU_BUFFER_DESC *pBufferDesc, D3D12GPUDevice *pDevice, ID3D12Resource *pD3DResource, D3D12_RESOURCE_STATES defaultResourceState)
    : GPUBuffer(pBufferDesc)
    , m_pDevice(pDevice)
    , m_pD3DResource(pD3DResource)
    , m_pD3DMapResource(nullptr)
    , m_defaultResourceState(defaultResourceState)
    , m_mapType(GPU_MAP_TYPE_COUNT)
{
    g_pRenderer->GetCounters()->OnResourceCreated(this);
    pDevice->AddRef();
}

D3D12GPUBuffer::~D3D12GPUBuffer()
{
    DebugAssert(m_pD3DMapResource == nullptr);

    g_pRenderer->GetCounters()->OnResourceDeleted(this);    
    m_pDevice->ScheduleResourceForDeletion(m_pD3DResource);
    m_pDevice->Release();
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
    D3D12Helpers::SetD3D12ObjectName(m_pD3DResource, debugName);
}

GPUBuffer *D3D12GPUDevice::CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData /* = NULL */)
{
    // work out the default resource state
    D3D12_RESOURCE_STATES defaultResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
    if (pDesc->Flags & (GPU_BUFFER_FLAG_BIND_CONSTANT_BUFFER | GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER))
        defaultResourceState |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    if (pDesc->Flags & GPU_BUFFER_FLAG_BIND_INDEX_BUFFER)
        defaultResourceState |= D3D12_RESOURCE_STATE_INDEX_BUFFER;

    // initial resource state
    D3D12_RESOURCE_STATES initialResourceState = defaultResourceState;
    if (pInitialData != nullptr)
        initialResourceState = D3D12_RESOURCE_STATE_COPY_DEST;

    // create main resource
    ID3D12Resource *pResource;
    D3D12_HEAP_PROPERTIES heapProperties = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
    D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, pDesc->Size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    HRESULT hResult = m_pD3DDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialResourceState, nullptr, IID_PPV_ARGS(&pResource));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommitedResource for main resource failed with hResult %08X", hResult);
        return false;
    }

    // create instance
    D3D12GPUBuffer *pBuffer = new D3D12GPUBuffer(pDesc, this, pResource, defaultResourceState);

    // handle uploads
    if (pInitialData != nullptr)
    {
        // create an upload resource
        ID3D12Resource *pUploadResource = pBuffer->CreateUploadResource(m_pD3DDevice, pDesc->Size);
        if (pUploadResource == nullptr)
        {
            pBuffer->Release();
            return nullptr;
        }

        // get a copy queue
        CopyQueueReference queueReference(this);
        if (!queueReference.HasContext())
        {
            pBuffer->Release();
            return nullptr;
        }

        // map the upload resource
        D3D12_RANGE readRange = { 0, 0 };
        void *pMappedPointer;
        hResult = pUploadResource->Map(0, D3D12_MAP_RANGE_PARAM(&readRange), &pMappedPointer);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("Map for upload resource failed with hResult %08X", hResult);
            pUploadResource->Release();
            pResource->Release();
            return false;
        }

        // fill it
        D3D12_RANGE writeRange = { 0, pDesc->Size };
        Y_memcpy(pMappedPointer, pInitialData, pDesc->Size);
        pUploadResource->Unmap(0, D3D12_MAP_RANGE_PARAM(&writeRange));

        // copy from the upload buffer to the real buffer
        GetCurrentCopyCommandList()->CopyBufferRegion(pResource, 0, pUploadResource, 0, pDesc->Size);

        // transition to the real resource state
        ResourceBarrier(pResource, D3D12_RESOURCE_STATE_COPY_DEST, defaultResourceState);

        // free upload resource
        ScheduleCopyResourceForDeletion(pUploadResource);
    }

    // done, return the pointer from before
    return pBuffer;
}

ID3D12Resource *D3D12GPUBuffer::CreateUploadResource(ID3D12Device *pD3DDevice, uint32 size) const
{
    ID3D12Resource *pResource;
    D3D12_HEAP_PROPERTIES heapProperties = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
    D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE };
    HRESULT hResult = pD3DDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pResource));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommitedResource for upload resource failed with hResult %08X", hResult);
        return nullptr;
    }

    return pResource;
}

ID3D12Resource *D3D12GPUBuffer::CreateReadbackResource(ID3D12Device *pD3DDevice, uint32 size) const
{
    ID3D12Resource *pResource;
    D3D12_HEAP_PROPERTIES heapProperties = { D3D12_HEAP_TYPE_READBACK, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
    D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE };
    HRESULT hResult = pD3DDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pResource));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommitedResource for readback resource failed with hResult %08X", hResult);
        return nullptr;
    }

    return pResource;
}

bool D3D12GPUContext::ReadBuffer(GPUBuffer *pBuffer, void *pDestination, uint32 start, uint32 count)
{
    D3D12GPUBuffer *pD3D12Buffer = static_cast<D3D12GPUBuffer *>(pBuffer);
    DebugAssert(count > 0 && (start + count) <= pD3D12Buffer->GetDesc()->Size);

    // create a readback buffer
    ID3D12Resource *pReadbackBuffer = pD3D12Buffer->CreateReadbackResource(m_pD3DDevice, count);
    if (pReadbackBuffer == nullptr)
        return false;

    // transition to copy state, and queue a copy to the readback buffer (the transition back is placed on the next command list)
    ResourceBarrier(pD3D12Buffer->GetD3DResource(), pD3D12Buffer->GetDefaultResourceState(), D3D12_RESOURCE_STATE_COPY_SOURCE);
    m_pCommandList->CopyBufferRegion(pReadbackBuffer, 0, pD3D12Buffer->GetD3DResource(), start, count);

    // flush + finish the command queue (slow!)
    D3D12GPUContext::Finish();

    // now we can transition back
    ResourceBarrier(pD3D12Buffer->GetD3DResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, pD3D12Buffer->GetDefaultResourceState());

    // map the readback buffer
    void *pMappedPointer;
    D3D12_RANGE readRange = { start, start + count };
    HRESULT hResult = pReadbackBuffer->Map(0, D3D12_MAP_RANGE_PARAM(&readRange), &pMappedPointer);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("Failed to map readback buffer: %08X", hResult);
        pReadbackBuffer->Release();
        return false;
    }

    // copy the contents over, and unmap the buffer
    D3D12_RANGE writeRange = { 0, 0 };
    Y_memcpy(pDestination, pMappedPointer, count);
    pReadbackBuffer->Unmap(0, D3D12_MAP_RANGE_PARAM(&writeRange));
    pReadbackBuffer->Release();
    return true;
}

bool D3D12GPUContext::WriteBuffer(GPUBuffer *pBuffer, const void *pSource, uint32 start, uint32 count)
{
    D3D12GPUBuffer *pD3D12Buffer = static_cast<D3D12GPUBuffer *>(pBuffer);
    DebugAssert(pD3D12Buffer->GetDesc()->Flags & GPU_BUFFER_FLAG_WRITABLE);
    DebugAssert(count > 0 && (start + count) <= pD3D12Buffer->GetDesc()->Size);

    // can we use the scratch buffer?
    if (count <= D3D12_BUFFER_WRITE_SCRATCH_BUFFER_THRESHOLD)
    {
        // allocate scratch buffer space
        ID3D12Resource *pScratchBufferResource;
        void *pScratchBufferCPUPointer;
        uint32 scratchBufferOffset;
        if (AllocateScratchBufferMemory(count, 0, &pScratchBufferResource, &scratchBufferOffset, &pScratchBufferCPUPointer, nullptr))
        {
            // copy to scratch buffer
            Y_memcpy(pScratchBufferCPUPointer, pSource, count);

            // queue a copy from scratch buffer -> buffer
            ResourceBarrier(pD3D12Buffer->GetD3DResource(), pD3D12Buffer->GetDefaultResourceState(), D3D12_RESOURCE_STATE_COPY_DEST);
            m_pCommandList->CopyBufferRegion(pD3D12Buffer->GetD3DResource(), start, pScratchBufferResource, scratchBufferOffset, count);
            ResourceBarrier(pD3D12Buffer->GetD3DResource(), D3D12_RESOURCE_STATE_COPY_DEST, pD3D12Buffer->GetDefaultResourceState());
            return true;
        }

        // failed to alloc
        Log_WarningPrintf("Failed to allocate scratch buffer storage, falling back to slow path.");
    }

    // allocate an upload buffer
    ID3D12Resource *pUploadBuffer = pD3D12Buffer->CreateUploadResource(m_pD3DDevice, count);
    if (pUploadBuffer == nullptr)
        return false;

    // map the upload buffer
    void *pMappedPointer;
    D3D12_RANGE readRange = { 0, 0 };
    HRESULT hResult = pUploadBuffer->Map(0, D3D12_MAP_RANGE_PARAM(&readRange), &pMappedPointer);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("Failed to map upload buffer: %08X", hResult);
        pUploadBuffer->Release();
        return false;
    }

    // copy the contents over, and unmap the buffer
    D3D12_RANGE writeRange = { start, start + count };
    Y_memcpy(pMappedPointer, pSource, count);
    pUploadBuffer->Unmap(0, D3D12_MAP_RANGE_PARAM(&writeRange));

    // transition to copy state, and queue a copy from the upload buffer
    ResourceBarrier(pD3D12Buffer->GetD3DResource(), pD3D12Buffer->GetDefaultResourceState(), D3D12_RESOURCE_STATE_COPY_DEST);
    m_pCommandList->CopyBufferRegion(pUploadBuffer, 0, pD3D12Buffer->GetD3DResource(), start, count);
    ResourceBarrier(pD3D12Buffer->GetD3DResource(), D3D12_RESOURCE_STATE_COPY_DEST, pD3D12Buffer->GetDefaultResourceState());

    // release the upload buffer later
    m_pDevice->ScheduleResourceForDeletion(pUploadBuffer);
    return true;
}

bool D3D12GPUContext::MapBuffer(GPUBuffer *pBuffer, GPU_MAP_TYPE mapType, void **ppPointer)
{
    D3D12GPUBuffer *pD3D12Buffer = static_cast<D3D12GPUBuffer *>(pBuffer);
    DebugAssert(pD3D12Buffer->GetDesc()->Flags & GPU_BUFFER_FLAG_MAPPABLE);
    DebugAssert(pD3D12Buffer->GetMapResource() == nullptr);

    // get buffer size
    uint32 bufferSize = pD3D12Buffer->GetDesc()->Size;

    // create a mapping buffer based on the initial state (read/write)
    // @TODO handle WRITE_NO_OVERWRITE and optimizations for this case..
    // @TODO use scratch buffer for small maps (this would need a mapbufferrange call though)
    ID3D12Resource *pMapBuffer;
    if (mapType == GPU_MAP_TYPE_READ || mapType == GPU_MAP_TYPE_READ_WRITE)
    {
        // create readback buffer
        pMapBuffer = pD3D12Buffer->CreateReadbackResource(m_pD3DDevice, bufferSize);
        if (pMapBuffer == nullptr)
            return false;

        // copy the contents from the gpu buffer to the readback buffer
        ResourceBarrier(pD3D12Buffer->GetD3DResource(), pD3D12Buffer->GetDefaultResourceState(), D3D12_RESOURCE_STATE_COPY_SOURCE);
        m_pCommandList->CopyBufferRegion(pMapBuffer, 0, pD3D12Buffer->GetD3DResource(), 0, bufferSize);

        // flush + finish the command queue (slow!)
        D3D12GPUContext::Finish();

        // now we can transition back
        ResourceBarrier(pD3D12Buffer->GetD3DResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, pD3D12Buffer->GetDefaultResourceState());

        // read the whole resource
        D3D12_RANGE readRange = { 0, pD3D12Buffer->GetDesc()->Size };
        HRESULT hResult = pMapBuffer->Map(0, D3D12_MAP_RANGE_PARAM(&readRange), ppPointer);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("Failed to map buffer: %08X", hResult);
            pMapBuffer->Release();
            return false;
        }
    }
    else
    {
        // write-only, so create upload buffer
        pMapBuffer = pD3D12Buffer->CreateUploadResource(m_pD3DDevice, bufferSize);
        if (pMapBuffer == nullptr)
            return false;

        // not reading anything
        D3D12_RANGE readRange = { 0, 0 };
        HRESULT hResult = pMapBuffer->Map(0, D3D12_MAP_RANGE_PARAM(&readRange), ppPointer);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("Failed to map buffer: %08X", hResult);
            pMapBuffer->Release();
            return false;
        }
    }

    // wait for the unmap call.
    pD3D12Buffer->SetMapResource(pMapBuffer, mapType);
    return true;
}

void D3D12GPUContext::Unmapbuffer(GPUBuffer *pBuffer, void *pPointer)
{
    D3D12GPUBuffer *pD3D12Buffer = static_cast<D3D12GPUBuffer *>(pBuffer);
    DebugAssert(pD3D12Buffer->GetDesc()->Flags & GPU_BUFFER_FLAG_MAPPABLE);
    DebugAssert(pD3D12Buffer->GetMapResource() != nullptr);

    // if we're in any write mode, we have to copy from the map buffer back to the gpu buffer
    ID3D12Resource *pMapBuffer = pD3D12Buffer->GetMapResource();
    GPU_MAP_TYPE mapMode = pD3D12Buffer->GetMapType();
    if (mapMode == GPU_MAP_TYPE_READ_WRITE || mapMode == GPU_MAP_TYPE_WRITE || mapMode == GPU_MAP_TYPE_WRITE_DISCARD || mapMode == GPU_MAP_TYPE_WRITE_NO_OVERWRITE)
    {
        // unmap, assume the entire range was written
        D3D12_RANGE writtenRange = { 0, pD3D12Buffer->GetDesc()->Size };
        pMapBuffer->Unmap(0, D3D12_MAP_RANGE_PARAM(&writtenRange));

        // read/write has to be transitioned
        if (mapMode == GPU_MAP_TYPE_READ_WRITE)
            ResourceBarrier(pMapBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);

        // copy to the gpu buffer
        ResourceBarrier(pD3D12Buffer->GetD3DResource(), pD3D12Buffer->GetDefaultResourceState(), D3D12_RESOURCE_STATE_COPY_DEST);
        m_pCommandList->CopyBufferRegion(pD3D12Buffer->GetD3DResource(), 0, pMapBuffer, 0, pD3D12Buffer->GetDesc()->Size);
        ResourceBarrier(pD3D12Buffer->GetD3DResource(), D3D12_RESOURCE_STATE_COPY_DEST, pD3D12Buffer->GetDefaultResourceState());

        // have to wait until the gpu is finished with it before releasing the buffer
        m_pDevice->ScheduleResourceForDeletion(pMapBuffer);
    }
    else
    {
        // unmap, nothing was written
        D3D12_RANGE writtenRange = { 0, 0 };
        pMapBuffer->Unmap(0, D3D12_MAP_RANGE_PARAM(&writtenRange));

        // this was only a read mapping, so we can just nuke the resource right now (the gpu won't touch it)
        pMapBuffer->Release();
    }

    // clear pointer
    pD3D12Buffer->SetMapResource(nullptr, GPU_MAP_TYPE_COUNT);
}

