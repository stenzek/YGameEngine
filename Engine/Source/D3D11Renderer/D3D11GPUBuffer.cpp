#include "D3D11Renderer/PrecompiledHeader.h"
#include "D3D11Renderer/D3D11GPUBuffer.h"
#include "D3D11Renderer/D3D11Renderer.h"
#include "D3D11Renderer/D3D11GPUContext.h"
Log_SetChannel(Renderer);

D3D11GPUBuffer::D3D11GPUBuffer(const GPU_BUFFER_DESC *pBufferDesc, ID3D11Buffer *pD3DBuffer, ID3D11Buffer *pD3DStagingBuffer)
    : GPUBuffer(pBufferDesc),
      m_pD3DBuffer(pD3DBuffer),
      m_pD3DStagingBuffer(pD3DStagingBuffer),
      m_pMappedContext(NULL),
      m_pMappedPointer(NULL)
{
    static_cast<D3D11Renderer *>(g_pRenderer)->OnResourceCreated(this);
}

D3D11GPUBuffer::~D3D11GPUBuffer()
{
    static_cast<D3D11Renderer *>(g_pRenderer)->OnResourceReleased(this);

    DebugAssert(m_pMappedContext == NULL);
    SAFE_RELEASE(m_pD3DBuffer);
}

void D3D11GPUBuffer::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this) + sizeof(ID3D11Buffer);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = m_desc.Size;
}

void D3D11GPUBuffer::SetDebugName(const char *debugName)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DBuffer, debugName);
}

GPUBuffer *D3D11Renderer::CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData /* = NULL */)
{
    D3D11_BUFFER_DESC D3DBufferDesc;
    D3DBufferDesc.ByteWidth = pDesc->Size;
    D3DBufferDesc.BindFlags = 0;
    D3DBufferDesc.MiscFlags = 0;
    D3DBufferDesc.StructureByteStride = 0;

    if (pDesc->Flags & GPU_BUFFER_FLAG_MAPPABLE)
    {
        D3DBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        D3DBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    else if (pDesc->Flags & (GPU_BUFFER_FLAG_READABLE | GPU_BUFFER_FLAG_WRITABLE))
    {
        D3DBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        D3DBufferDesc.CPUAccessFlags = 0;
    }
    else
    {
        DebugAssert(pInitialData != NULL);
        D3DBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        D3DBufferDesc.CPUAccessFlags = 0;
    }

    if (pDesc->Flags & GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER)
        D3DBufferDesc.BindFlags |= D3D11_BIND_VERTEX_BUFFER;
    if (pDesc->Flags & GPU_BUFFER_FLAG_BIND_INDEX_BUFFER)
        D3DBufferDesc.BindFlags |= D3D11_BIND_INDEX_BUFFER;
    if (pDesc->Flags & GPU_BUFFER_FLAG_BIND_CONSTANT_BUFFER)
        D3DBufferDesc.BindFlags |= D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA subResourceData;
    subResourceData.SysMemPitch = 0;
    subResourceData.SysMemSlicePitch = 0;
    subResourceData.pSysMem = pInitialData;

    HRESULT hResult;

    // create buffer
    ID3D11Buffer *pBuffer;
    hResult = m_pD3DDevice->CreateBuffer(&D3DBufferDesc, (pInitialData != NULL) ? &subResourceData : NULL, &pBuffer);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11Renderer::CreateBuffer: Could not create buffer (%u bytes): %08X", pDesc->Size, hResult);
        return NULL;
    }

    // create staging buffer if needed
    ID3D11Buffer *pStagingBuffer;
    if (pDesc->Flags & GPU_BUFFER_FLAG_READABLE)
    {
        D3D11_BUFFER_DESC D3DStagingBufferDesc;
        D3DBufferDesc.ByteWidth = pDesc->Size;
        D3DBufferDesc.Usage = D3D11_USAGE_STAGING;
        D3DBufferDesc.BindFlags = 0;
        D3DBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;// | D3D11_CPU_ACCESS_WRITE;
        D3DBufferDesc.MiscFlags = 0;
        D3DBufferDesc.StructureByteStride = 0;

        hResult = m_pD3DDevice->CreateBuffer(&D3DStagingBufferDesc, NULL, &pStagingBuffer);
        if (FAILED(hResult))
        {
            pBuffer->Release();

            Log_ErrorPrintf("D3D11Renderer::CreateBuffer: Could not create staging buffer (%u bytes): %08X", pDesc->Size, hResult);
            return NULL;
        }
    }
    else
    {
        pStagingBuffer = NULL;
    }

    return new D3D11GPUBuffer(pDesc, pBuffer, pStagingBuffer);
}

bool D3D11GPUContext::ReadBuffer(GPUBuffer *pBuffer, void *pDestination, uint32 start, uint32 count)
{
    D3D11GPUBuffer *pD3D11Buffer = static_cast<D3D11GPUBuffer *>(pBuffer);
    DebugAssert((start + count) <= pD3D11Buffer->GetDesc()->Size);
    DebugAssert(pD3D11Buffer->GetDesc()->Flags & GPU_BUFFER_FLAG_READABLE);

    // Copy from the GPU buffer to the staging buffer
    bool fullCopy = (start == 0 && count == pBuffer->GetDesc()->Size);
    if (fullCopy)
    {
        // as there isn't multiple mip levels we can copy the whole resource
        m_pD3DContext->CopyResource(pD3D11Buffer->GetD3DStagingBuffer(), pD3D11Buffer->GetD3DBuffer());
    }
    else
    {
        // calculate box
        D3D11_BOX box = { start, 0, 0, start + count, 1, 1 };
        m_pD3DContext->CopySubresourceRegion(pD3D11Buffer->GetD3DStagingBuffer(), 0, 0, 0, 0, pD3D11Buffer->GetD3DBuffer(), 0, &box);
    }

    // map the staging buffer into memory
    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    HRESULT hResult = m_pD3DContext->Map(pD3D11Buffer->GetD3DStagingBuffer(), 0, D3D11_MAP_READ, 0, &mappedSubresource);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUContext::ReadBuffer: Map of staging buffer failed with hResult %08X", hResult);
        return false;
    }

    // copy into our memory
    Y_memcpy(pDestination, mappedSubresource.pData, count);
    m_pD3DContext->Unmap(pD3D11Buffer->GetD3DStagingBuffer(), 0);
    return true;
}

bool D3D11GPUContext::WriteBuffer(GPUBuffer *pBuffer, const void *pSource, uint32 start, uint32 count)
{
    D3D11GPUBuffer *pD3D11Buffer = static_cast<D3D11GPUBuffer *>(pBuffer);
    DebugAssert(pD3D11Buffer->GetDesc()->Flags & GPU_BUFFER_FLAG_WRITABLE);
    DebugAssert((start + count) <= pD3D11Buffer->GetDesc()->Size);

    bool fullCopy = (start == 0 && count == pBuffer->GetDesc()->Size);
    if (pD3D11Buffer->GetDesc()->Flags & GPU_BUFFER_FLAG_MAPPABLE)
    {
        // Have to use map
        D3D11_MAPPED_SUBRESOURCE mappedSubresource;
        HRESULT hResult = m_pD3DContext->Map(pD3D11Buffer->GetD3DBuffer(), 0, (fullCopy) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE, 0, &mappedSubresource);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUContext::WriteBuffer: Map failed with HResult %08X", hResult);
            return false;
        }

        // write and unmap
        Y_memcpy(reinterpret_cast<byte *>(mappedSubresource.pData) + start, pSource, count);
        m_pD3DContext->Unmap(pD3D11Buffer->GetD3DBuffer(), 0);
    }
    else
    {
        // Use UpdateSubresource
        if (fullCopy)
        {
            // whole buffer
            m_pD3DContext->UpdateSubresource(pD3D11Buffer->GetD3DBuffer(), 0, NULL, pSource, 0, 0);
        }
        else
        {
            // partial buffer, use box
            D3D11_BOX box = { start, 0, 0, start + count, 1, 1 };
            m_pD3DContext->UpdateSubresource(pD3D11Buffer->GetD3DBuffer(), 0, &box, pSource, 0, 0);
        }
    }

    return true;
}

bool D3D11GPUContext::MapBuffer(GPUBuffer *pBuffer, GPU_MAP_TYPE mapType, void **ppPointer)
{
    D3D11GPUBuffer *pD3D11Buffer = static_cast<D3D11GPUBuffer *>(pBuffer);
    DebugAssert(pD3D11Buffer->GetDesc()->Flags & GPU_BUFFER_FLAG_MAPPABLE);
    DebugAssert(pD3D11Buffer->GetMappedContext() == NULL && pD3D11Buffer->GetMappedPointer() == NULL);

    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    HRESULT hResult = m_pD3DContext->Map(pD3D11Buffer->GetD3DBuffer(), 0, D3D11TypeConversion::MapTypetoD3D11MapType(mapType), 0, &mappedSubresource);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUContext::MapBuffer: Failed with HResult %08X", hResult);
        return false;
    }

    pD3D11Buffer->SetMappedContextPointer(this, mappedSubresource.pData);
    *ppPointer = mappedSubresource.pData;
    return true;
}

void D3D11GPUContext::Unmapbuffer(GPUBuffer *pBuffer, void *pPointer)
{
    D3D11GPUBuffer *pD3D11Buffer = static_cast<D3D11GPUBuffer *>(pBuffer);
    DebugAssert(pD3D11Buffer->GetDesc()->Flags & GPU_BUFFER_FLAG_MAPPABLE);
    DebugAssert(pD3D11Buffer->GetMappedContext() == this && pD3D11Buffer->GetMappedPointer() == pPointer);

    m_pD3DContext->Unmap(pD3D11Buffer->GetD3DBuffer(), 0);
    pD3D11Buffer->SetMappedContextPointer(NULL, NULL);
}
