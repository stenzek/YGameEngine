#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12GPUTexture.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUContext.h"
#include "D3D12Renderer/D3D12RenderBackend.h"
#include "D3D12Renderer/D3D12Helpers.h"
Log_SetChannel(D3D12RenderBackend);

static void MapTextureFormatToViewFormat(DXGI_FORMAT *pCreationFormat, DXGI_FORMAT *pSRVFormat, DXGI_FORMAT *pRTVFormat, DXGI_FORMAT *pDSVFormat)
{
    DXGI_FORMAT creationFormat = *pCreationFormat;

    // handle depth type mapping
    switch (creationFormat)
    {
    case DXGI_FORMAT_D16_UNORM:
        *pCreationFormat = DXGI_FORMAT_R16_TYPELESS;
        *pSRVFormat = DXGI_FORMAT_R16_UNORM;
        *pRTVFormat = DXGI_FORMAT_UNKNOWN;
        *pDSVFormat = DXGI_FORMAT_D16_UNORM;
        break;

    case DXGI_FORMAT_D24_UNORM_S8_UINT:    
        *pCreationFormat = DXGI_FORMAT_R24G8_TYPELESS;
        *pSRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        *pRTVFormat = DXGI_FORMAT_UNKNOWN;
        *pDSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        break;

    case DXGI_FORMAT_D32_FLOAT:
        *pCreationFormat = DXGI_FORMAT_R32_TYPELESS;
        *pSRVFormat = DXGI_FORMAT_R32_FLOAT;
        *pRTVFormat = DXGI_FORMAT_UNKNOWN;
        *pDSVFormat = DXGI_FORMAT_D32_FLOAT;
        break;

    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        *pCreationFormat = DXGI_FORMAT_R32G8X24_TYPELESS;
        *pSRVFormat = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        *pRTVFormat = DXGI_FORMAT_UNKNOWN;
        *pDSVFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        break;

    default:
        *pCreationFormat = creationFormat;
        *pSRVFormat = creationFormat;
        *pRTVFormat = creationFormat;
        *pDSVFormat = DXGI_FORMAT_UNKNOWN;
        break;
    }
}

static D3D12_RESOURCE_FLAGS GetD3DResourceFlagsFromTextureFlags(uint32 textureFlags)
{
    // get d3d resource flags
    D3D12_RESOURCE_FLAGS resourceFlags = D3D12_RESOURCE_FLAG_NONE;
    if (!(textureFlags & GPU_TEXTURE_FLAG_SHADER_BINDABLE))
        resourceFlags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    if (textureFlags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (textureFlags & GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER)
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    if (textureFlags & GPU_TEXTURE_FLAG_BIND_COMPUTE_WRITABLE)
        resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    return resourceFlags;
}

static D3D12_RESOURCE_STATES GetD3DResourceStates(uint32 textureFlags)
{
    // find generic state -- default to RTV/DSV only if not SRV
    D3D12_RESOURCE_STATES defaultResourceState = D3D12_RESOURCE_STATE_COMMON;
    if (textureFlags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        defaultResourceState |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    else if (textureFlags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
        defaultResourceState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    else if (textureFlags & GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER)
        defaultResourceState |= D3D12_RESOURCE_STATE_DEPTH_WRITE;

    return defaultResourceState;
}

ID3D12Resource *UploadTextureData(ID3D12Device *pD3DDevice, ID3D12GraphicsCommandList *pCommandList, ID3D12Resource *pD3DResource, const D3D12_RESOURCE_DESC *pResourceDesc, const void **ppInitialData, const uint32 *pInitialDataPitch, const uint32 *pInitialDataSlicePitch)
{
    HRESULT hResult;
    DebugAssert(pResourceDesc->MipLevels > 0);

    // get the total number of subresource to upload
    uint32 subResourceCount = (pResourceDesc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? pResourceDesc->MipLevels : (pResourceDesc->MipLevels * pResourceDesc->DepthOrArraySize);
    DebugAssert(subResourceCount > 0);

    // get the footprints of each subresource
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pSubResourceFootprints = new D3D12_PLACED_SUBRESOURCE_FOOTPRINT[subResourceCount];
    UINT *pRowCounts = new UINT[subResourceCount];
    UINT64 *pRowSizes = new UINT64[subResourceCount];
    UINT64 intermediateSize;
    pD3DDevice->GetCopyableFootprints(pResourceDesc, 0, subResourceCount, 0, pSubResourceFootprints, pRowCounts, pRowSizes, &intermediateSize);

    // create upload buffer
    ID3D12Resource *pUploadResource;
    D3D12_HEAP_PROPERTIES uploadHeapProperties = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
    D3D12_RESOURCE_DESC uploadResourceDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, intermediateSize, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    hResult = pD3DDevice->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &uploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pUploadResource));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommittedResource for upload resource failed with hResult %08X", hResult);
        return nullptr;
    }

    // map the upload resource
    byte *pMappedPointer;
    D3D12_RANGE readRange = { 0, 0 };
    hResult = pUploadResource->Map(0, D3D12_MAP_RANGE_PARAM(&readRange), (void **)&pMappedPointer);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("Map upload subresource for upload resource failed with hResult %08X", hResult);
        pUploadResource->Release();
        return nullptr;
    }

    // uploading a non-3D texture?
    if (pResourceDesc->Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        // loop through subresources
        for (uint32 subResourceIndex = 0; subResourceIndex < subResourceCount; subResourceIndex++)
        {
            // map this subresource
            uint32 rowPitch = pSubResourceFootprints[subResourceIndex].Footprint.RowPitch;
            DebugAssert(pRowSizes[subResourceIndex] <= rowPitch && pRowSizes[subResourceIndex] <= pInitialDataPitch[subResourceIndex]);

            // copy in data. this could result in a buffer overflow, except for the fact that we're only copying as much data as we can,
            // so if the gpu is aligning each row, we won't copy past the actual data
            if (rowPitch == pInitialDataPitch[subResourceIndex])
                Y_memcpy(pMappedPointer + (size_t)pSubResourceFootprints[subResourceIndex].Offset, ppInitialData[subResourceIndex], rowPitch * pRowCounts[subResourceIndex]);
            else
                Y_memcpy_stride(pMappedPointer + (size_t)pSubResourceFootprints[subResourceIndex].Offset, rowPitch, ppInitialData[subResourceIndex], pInitialDataPitch[subResourceIndex], (size_t)pRowSizes[subResourceIndex], pRowCounts[subResourceIndex]);

            // invoke a copy of this subresource
            D3D12_TEXTURE_COPY_LOCATION sourceCopyLocation = { pUploadResource, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, pSubResourceFootprints[subResourceIndex] };
            D3D12_TEXTURE_COPY_LOCATION destCopyLocation = { pD3DResource, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, subResourceIndex };
            pCommandList->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &sourceCopyLocation, nullptr);
        }
    }
    else
    {
        // upload process for 3D Texture is different.
        for (uint32 mipLevel = 0; mipLevel < subResourceCount; mipLevel++)
        {
            // map this subresource
            uint32 inRowPitch = pInitialDataPitch[mipLevel];
            uint32 outRowPitch = pSubResourceFootprints[mipLevel].Footprint.RowPitch;
            uint32 rowSize = Min(pInitialDataPitch[mipLevel], (uint32)pRowSizes[mipLevel]);
            DebugAssert(pRowSizes[mipLevel] <= outRowPitch && pRowSizes[mipLevel] <= pInitialDataPitch[mipLevel]);
            DebugAssert(pInitialDataSlicePitch[mipLevel] <= pSubResourceFootprints[mipLevel].Footprint.RowPitch * pRowCounts[mipLevel]);

            // copy each slice individually
            const byte *pInPointer = (const byte *)ppInitialData[mipLevel];
            byte *pOutPointer = pMappedPointer + (size_t)pSubResourceFootprints[mipLevel].Offset;
            for (uint32 z = 0; z < pResourceDesc->DepthOrArraySize; z++)
            {
                if (inRowPitch == outRowPitch)
                    Y_memcpy(pOutPointer, pInPointer, inRowPitch * pRowCounts[mipLevel]);
                else
                    Y_memcpy_stride(pOutPointer, outRowPitch, pInPointer, inRowPitch, rowSize, pRowCounts[mipLevel]);

                pInPointer += inRowPitch;
                pOutPointer += outRowPitch;
            }

            // copy the subresource
            D3D12_TEXTURE_COPY_LOCATION sourceCopyLocation = { pUploadResource, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, pSubResourceFootprints[mipLevel] };
            D3D12_TEXTURE_COPY_LOCATION destCopyLocation = { pD3DResource, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, mipLevel };
            pCommandList->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &sourceCopyLocation, nullptr);
        }
    }

    // release the mapping - wrote the whole range
    D3D12_RANGE writeRange = { 0, (size_t)intermediateSize };
    pUploadResource->Unmap(0, D3D12_MAP_RANGE_PARAM(&writeRange));

    // free memory
    delete[] pRowSizes;
    delete[] pRowCounts;
    delete[] pSubResourceFootprints;
    return pUploadResource;
}


GPUTexture1D *D3D12GPUDevice::CreateTexture1D(const GPU_TEXTURE1D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/)
{
    return nullptr;
}

bool D3D12GPUContext::ReadTexture(GPUTexture1D *pTexture, void *pDestination, uint32 cbDestination, uint32 mipIndex, uint32 start, uint32 count)
{
    return false;
}

bool D3D12GPUContext::WriteTexture(GPUTexture1D *pTexture, const void *pSource, uint32 cbSource, uint32 mipIndex, uint32 start, uint32 count)
{
    return false;
}

GPUTexture1DArray *D3D12GPUDevice::CreateTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/)
{
    return nullptr;
}

bool D3D12GPUContext::ReadTexture(GPUTexture1DArray *pTexture, void *pDestination, uint32 cbDestination, uint32 arrayIndex, uint32 mipIndex, uint32 start, uint32 count)
{
    return false;
}

bool D3D12GPUContext::WriteTexture(GPUTexture1DArray *pTexture, const void *pSource, uint32 cbSource, uint32 arrayIndex, uint32 mipIndex, uint32 start, uint32 count)
{
    return false;
}

D3D12GPUTexture2D::D3D12GPUTexture2D(const GPU_TEXTURE2D_DESC *pDesc, ID3D12Resource *pD3DResource, const D3D12DescriptorHandle &srvHandle, const D3D12DescriptorHandle &samplerHandle, D3D12_RESOURCE_STATES defaultResourceState)
    : GPUTexture2D(pDesc)
    , m_pD3DResource(pD3DResource)
    , m_srvHandle(srvHandle)
    , m_samplerHandle(samplerHandle)
    , m_defaultResourceState(defaultResourceState)
{   

}

D3D12GPUTexture2D::~D3D12GPUTexture2D()
{
    if (!m_samplerHandle.IsNull())
        D3D12RenderBackend::GetInstance()->GetGraphicsCommandQueue()->ScheduleDescriptorForDeletion(m_samplerHandle);

    if (!m_srvHandle.IsNull())
        D3D12RenderBackend::GetInstance()->GetGraphicsCommandQueue()->ScheduleDescriptorForDeletion(m_srvHandle);

    D3D12RenderBackend::GetInstance()->GetGraphicsCommandQueue()->ScheduleResourceForDeletion(m_pD3DResource);
}

void D3D12GPUTexture2D::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), Max(m_desc.Height >> j, (uint32)1), 1);

        *gpuMemoryUsage = memoryUsage;
    }
}

void D3D12GPUTexture2D::SetDebugName(const char *name)
{
    D3D12Helpers::SetD3D12ObjectName(m_pD3DResource, name);
}

GPUTexture2D *D3D12GPUDevice::CreateTexture2D(const GPU_TEXTURE2D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/)
{
    HRESULT hResult;

    // validate pixel formats.
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DXGI_FORMAT creationFormat = D3D12Helpers::PixelFormatToDXGIFormat(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr && creationFormat != DXGI_FORMAT_UNKNOWN);
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT);

    // determine view formats
    DXGI_FORMAT srvFormat, rtvFormat, dsvFormat;
    MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);

    // get d3d resource flags
    D3D12_RESOURCE_FLAGS resourceFlags = GetD3DResourceFlagsFromTextureFlags(pTextureDesc->Flags);
    D3D12_RESOURCE_STATES defaultResourceState = GetD3DResourceStates(pTextureDesc->Flags);

    // find the initial state
    D3D12_RESOURCE_STATES initialResourceState = (ppInitialData != nullptr) ? D3D12_RESOURCE_STATE_COPY_DEST : defaultResourceState;

    // clear value
    D3D12_CLEAR_VALUE optimizedClearValue;
    D3D12_CLEAR_VALUE *pOptimizedClearValue;
    if (pTextureDesc->Flags & (GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER) && D3D12Helpers::GetOptimizedClearValue(pTextureDesc->Format, &optimizedClearValue))
        pOptimizedClearValue = &optimizedClearValue;
    else
        pOptimizedClearValue = nullptr;

    // create the texture resource
    ID3D12Resource *pD3DResource;
    D3D12_HEAP_PROPERTIES heapProperties = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
    D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, pTextureDesc->Width, pTextureDesc->Height, 1, (UINT16)pTextureDesc->MipLevels, creationFormat, { 1, 0 }, D3D12_TEXTURE_LAYOUT_UNKNOWN, resourceFlags };
    hResult = m_pD3DDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialResourceState, pOptimizedClearValue, IID_PPV_ARGS(&pD3DResource));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommittedResource failed with hResult %08X", hResult);
        return false;
    }

    // create SRV
    D3D12DescriptorHandle srvHandle;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        // allocate a descriptor
        if (!m_pBackend->GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate(&srvHandle))
        {
            pD3DResource->Release();
            return false;
        }

        // fill the descriptor
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = srvFormat;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = pTextureDesc->MipLevels;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0;
        m_pD3DDevice->CreateShaderResourceView(pD3DResource, &srvDesc, srvHandle);
    }

    // create sampler state
    D3D12DescriptorHandle samplerHandle;
    if (pSamplerStateDesc != nullptr)
    {
        // fill the descriptor
        D3D12_SAMPLER_DESC samplerDesc;
        if (!D3D12Helpers::FillD3D12SamplerStateDesc(pSamplerStateDesc, &samplerDesc))
        {
            Log_ErrorPrintf("Failed to convert sampler state description.");
            m_pBackend->GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Free(srvHandle);
            pD3DResource->Release();
            return false;
        }

        // allocate a descriptor
        if (!m_pBackend->GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)->Allocate(&samplerHandle))
        {
            Log_ErrorPrintf("Failed to allocator sampler descriptor.");
            m_pBackend->GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Free(srvHandle);
            pD3DResource->Release();
            return false;
        }

        // fill the descriptor @TODO use a pool here, since we'll have duplicates for sure.. also 2048 sampler per heap limit..
        m_pD3DDevice->CreateSampler(&samplerDesc, samplerHandle);
    }

    // create the upload resource, and do the upload
    if (ppInitialData != nullptr)
    {
        // we need to batch the uploads, even on the render thread
        BeginResourceBatchUpload();

        // upload the texture
        ID3D12Resource *pUploadResource = UploadTextureData(m_pD3DDevice, GetCommandList(), pD3DResource, &resourceDesc, ppInitialData, pInitialDataPitch, nullptr);
        if (pUploadResource == nullptr)
        {
            Log_ErrorPrintf("Failed to upload texture data", hResult);
            m_pBackend->GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Free(srvHandle);
            m_pBackend->GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)->Free(samplerHandle);
            pD3DResource->Release();
            return false;
        }
            
        // transition to its real resource state
        ResourceBarrier(pD3DResource, D3D12_RESOURCE_STATE_COPY_DEST, defaultResourceState);

        // done with the upload, and free the buffer
        FlushCopyQueue();
        ScheduleUploadResourceDeletion(pUploadResource);
        EndResourceBatchUpload();
    }

    // create class
    return new D3D12GPUTexture2D(pTextureDesc, pD3DResource, srvHandle, samplerHandle, defaultResourceState);
}

bool D3D12GPUContext::ReadTexture(GPUTexture2D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    // don't support block compression reads atm
    DXGI_FORMAT textureDXGIFormat = D3D12Helpers::PixelFormatToDXGIFormat(pTexture->GetDesc()->Format);
    if (!PixelFormat_GetPixelFormatInfo(pTexture->GetDesc()->Format)->IsBlockCompressed)
        return false;

    // fill descriptor information with the size we want to capture
    D3D12_RESOURCE_DESC regionDesc = { D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, countX, countY, 1, 1, textureDXGIFormat, { 1, 0 }, D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAG_NONE };
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT regionTextureFootprint;
    uint64 regionTextureRowSize;
    uint32 regionTextureRowCount;
    uint64 regionTextureSize;
    m_pD3DDevice->GetCopyableFootprints(&regionDesc, 0, 1, 0, &regionTextureFootprint, &regionTextureRowCount, &regionTextureRowSize, &regionTextureSize);

    // allocate a readback texture, with a descriptor matching the texture we're reading from
    D3D12_HEAP_PROPERTIES readbackHeapProperties = { D3D12_HEAP_TYPE_READBACK, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
    D3D12_RESOURCE_DESC readbackTextureDesc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, regionTextureSize, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    ID3D12Resource *pReadbackResource;
    HRESULT hResult = m_pD3DDevice->CreateCommittedResource(&readbackHeapProperties, D3D12_HEAP_FLAG_NONE, &readbackTextureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&pReadbackResource));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommittedResource failed with HRESULT %08X", hResult);
        return false;
    }

    // issue the copy to our readback resource
    D3D12_TEXTURE_COPY_LOCATION copySource = { static_cast<D3D12GPUTexture2D *>(pTexture)->GetD3DResource(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, mipIndex };
    D3D12_TEXTURE_COPY_LOCATION copyDestination = { pReadbackResource, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, regionTextureFootprint };
    D3D12_BOX copySourceBox = { startX, startY, 0, startX + countX, startY + countY, 1 };
    m_pCommandList->CopyTextureRegion(&copyDestination, 0, 0, 0, &copySource, &copySourceBox);

    // flush the queue, and wait for all operations to finish
    FlushCommandList(true, true, false);
    RestoreCommandListDependantState();

    // map the readback texture
    void *pMappedPointer;
    hResult = pReadbackResource->Map(0, nullptr, &pMappedPointer);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("Map failed with HRESULT %08X", hResult);
        return false;
    }

    // copy from readback resource to the user-provided buffer
    DebugAssert(cbDestination >= destinationRowPitch * regionTextureRowCount);
    if (destinationRowPitch == regionTextureFootprint.Footprint.RowPitch)
        Y_memcpy(pDestination, pMappedPointer, (size_t)regionTextureRowSize * regionTextureRowCount);
    else
        Y_memcpy_stride(pDestination, destinationRowPitch, pMappedPointer, regionTextureFootprint.Footprint.RowPitch, Min((uint32)regionTextureRowSize, destinationRowPitch), regionTextureRowCount);

    // release readback resource
    pReadbackResource->Unmap(0, nullptr);
    pReadbackResource->Release();
    return true;
}

bool D3D12GPUContext::WriteTexture(GPUTexture2D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    return false;
}

D3D12GPUTexture2DArray::D3D12GPUTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pDesc, ID3D12Resource *pD3DResource, const D3D12DescriptorHandle &srvHandle, const D3D12DescriptorHandle &samplerHandle, D3D12_RESOURCE_STATES defaultResourceState)
    : GPUTexture2DArray(pDesc)
    , m_pD3DResource(pD3DResource)
    , m_srvHandle(srvHandle)
    , m_samplerHandle(samplerHandle)
    , m_defaultResourceState(defaultResourceState)
{   

}

D3D12GPUTexture2DArray::~D3D12GPUTexture2DArray()
{
    if (!m_samplerHandle.IsNull())
        D3D12RenderBackend::GetInstance()->GetGraphicsCommandQueue()->ScheduleDescriptorForDeletion(m_samplerHandle);

    if (!m_srvHandle.IsNull())
        D3D12RenderBackend::GetInstance()->GetGraphicsCommandQueue()->ScheduleDescriptorForDeletion(m_srvHandle);

    D3D12RenderBackend::GetInstance()->GetGraphicsCommandQueue()->ScheduleResourceForDeletion(m_pD3DResource);
}

void D3D12GPUTexture2DArray::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), Max(m_desc.Height >> j, (uint32)1), 1);

        *gpuMemoryUsage = memoryUsage;
    }
}

void D3D12GPUTexture2DArray::SetDebugName(const char *name)
{
    D3D12Helpers::SetD3D12ObjectName(m_pD3DResource, name);
}

GPUTexture2DArray *D3D12GPUDevice::CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = nullptr */, const uint32 *pInitialDataPitch /* = nullptr */)
{
    HRESULT hResult;

    // validate pixel formats.
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DXGI_FORMAT creationFormat = D3D12Helpers::PixelFormatToDXGIFormat(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr && creationFormat != DXGI_FORMAT_UNKNOWN);
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->ArraySize > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT);

    // determine view formats
    DXGI_FORMAT srvFormat, rtvFormat, dsvFormat;
    MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);

    // get d3d resource flags
    D3D12_RESOURCE_FLAGS resourceFlags = GetD3DResourceFlagsFromTextureFlags(pTextureDesc->Flags);
    D3D12_RESOURCE_STATES defaultResourceState = GetD3DResourceStates(pTextureDesc->Flags);

    // find the initial state
    D3D12_RESOURCE_STATES initialResourceState = (ppInitialData != nullptr) ? D3D12_RESOURCE_STATE_COPY_DEST : defaultResourceState;

    // clear value
    D3D12_CLEAR_VALUE optimizedClearValue;
    D3D12_CLEAR_VALUE *pOptimizedClearValue;
    if (pTextureDesc->Flags & (GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER) && D3D12Helpers::GetOptimizedClearValue(pTextureDesc->Format, &optimizedClearValue))
        pOptimizedClearValue = &optimizedClearValue;
    else
        pOptimizedClearValue = nullptr;

    // create the texture resource
    ID3D12Resource *pD3DResource;
    D3D12_HEAP_PROPERTIES heapProperties = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
    D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, pTextureDesc->Width, pTextureDesc->Height, (UINT16)pTextureDesc->ArraySize, (UINT16)pTextureDesc->MipLevels, creationFormat, { 1, 0 }, D3D12_TEXTURE_LAYOUT_UNKNOWN, resourceFlags };
    hResult = m_pD3DDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, initialResourceState, pOptimizedClearValue, IID_PPV_ARGS(&pD3DResource));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommittedResource failed with hResult %08X", hResult);
        return false;
    }

    // create SRV
    D3D12DescriptorHandle srvHandle;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        // allocate a descriptor
        if (!m_pBackend->GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate(&srvHandle))
        {
            pD3DResource->Release();
            return false;
        }

        // fill the descriptor
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = srvFormat;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2DArray.MostDetailedMip = 0;
        srvDesc.Texture2DArray.MipLevels = pTextureDesc->MipLevels;
        srvDesc.Texture2DArray.FirstArraySlice = 0;
        srvDesc.Texture2DArray.ArraySize = pTextureDesc->ArraySize;
        srvDesc.Texture2DArray.PlaneSlice = 0;
        srvDesc.Texture2DArray.ResourceMinLODClamp = 0;
        m_pD3DDevice->CreateShaderResourceView(pD3DResource, &srvDesc, srvHandle);
    }

    // create sampler state
    D3D12DescriptorHandle samplerHandle;
    if (pSamplerStateDesc != nullptr)
    {
        // fill the descriptor
        D3D12_SAMPLER_DESC samplerDesc;
        if (!D3D12Helpers::FillD3D12SamplerStateDesc(pSamplerStateDesc, &samplerDesc))
        {
            Log_ErrorPrintf("Failed to convert sampler state description.");
            m_pBackend->GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Free(srvHandle);
            pD3DResource->Release();
            return false;
        }

        // allocate a descriptor
        if (!m_pBackend->GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)->Allocate(&samplerHandle))
        {
            Log_ErrorPrintf("Failed to allocator sampler descriptor.");
            m_pBackend->GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Free(srvHandle);
            pD3DResource->Release();
            return false;
        }

        // fill the descriptor @TODO use a pool here, since we'll have duplicates for sure.. also 2048 sampler per heap limit..
        m_pD3DDevice->CreateSampler(&samplerDesc, samplerHandle);
    }

    // create the upload resource, and do the upload
    if (ppInitialData != nullptr)
    {
        // we need to batch the uploads, even on the render thread
        BeginResourceBatchUpload();

        // upload the texture
        ID3D12Resource *pUploadResource = UploadTextureData(m_pD3DDevice, GetCommandList(), pD3DResource, &resourceDesc, ppInitialData, pInitialDataPitch, nullptr);
        if (pUploadResource == nullptr)
        {
            Log_ErrorPrintf("Failed to upload texture data", hResult);
            m_pBackend->GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Free(srvHandle);
            m_pBackend->GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)->Free(samplerHandle);
            pD3DResource->Release();
            return false;
        }
            
        // transition to its real resource state
        ResourceBarrier(pD3DResource, D3D12_RESOURCE_STATE_COPY_DEST, defaultResourceState);

        // done with the upload, and free the buffer
        FlushCopyQueue();
        ScheduleUploadResourceDeletion(pUploadResource);
        EndResourceBatchUpload();
    }

    // create class
    return new D3D12GPUTexture2DArray(pTextureDesc, pD3DResource, srvHandle, samplerHandle, defaultResourceState);
}

bool D3D12GPUContext::ReadTexture(GPUTexture2DArray *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 arrayIndex, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    return false;
}

bool D3D12GPUContext::WriteTexture(GPUTexture2DArray *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 arrayIndex, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    return false;
}

GPUTexture3D *D3D12GPUDevice::CreateTexture3D(const GPU_TEXTURE3D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/, const uint32 *pInitialDataSlicePitch /*= nullptr*/)
{
    return nullptr;
}

bool D3D12GPUContext::ReadTexture(GPUTexture3D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 destinationSlicePitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 startZ, uint32 countX, uint32 countY, uint32 countZ)
{
    return false;
}

bool D3D12GPUContext::WriteTexture(GPUTexture3D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 sourceSlicePitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 startZ, uint32 countX, uint32 countY, uint32 countZ)
{
    return false;
}

GPUTextureCube *D3D12GPUDevice::CreateTextureCube(const GPU_TEXTURECUBE_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/)
{
    return nullptr;
}

bool D3D12GPUContext::ReadTexture(GPUTextureCube *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    return false;
}

bool D3D12GPUContext::WriteTexture(GPUTextureCube *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    return false;
}

GPUTextureCubeArray *D3D12GPUDevice::CreateTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/)
{
    return nullptr;
}

bool D3D12GPUContext::ReadTexture(GPUTextureCubeArray *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 arrayIndex, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    return false;
}

bool D3D12GPUContext::WriteTexture(GPUTextureCubeArray *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 arrayIndex, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    return false;
}

GPUDepthTexture *D3D12GPUDevice::CreateDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pTextureDesc)
{
    return nullptr;
}

bool D3D12GPUContext::ReadTexture(GPUDepthTexture *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    return false;
}

bool D3D12GPUContext::WriteTexture(GPUDepthTexture *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    return false;
}

D3D12GPURenderTargetView::D3D12GPURenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc, const D3D12DescriptorHandle &descriptorHandle, ID3D12Resource *pD3DResource)
    : GPURenderTargetView(pTexture, pDesc)
    , m_descriptorHandle(descriptorHandle)
    , m_pD3DResource(pD3DResource)
{

}

D3D12GPURenderTargetView::~D3D12GPURenderTargetView()
{
    D3D12RenderBackend::GetInstance()->GetGraphicsCommandQueue()->ScheduleDescriptorForDeletion(m_descriptorHandle);
}

void D3D12GPURenderTargetView::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
    if (gpuMemoryUsage != nullptr)
        gpuMemoryUsage = 0;
}

void D3D12GPURenderTargetView::SetDebugName(const char *name)
{
    //D3D12Helpers::SetD3D12DeviceChildDebugName(m_pD3DRTV, name);
}

GPURenderTargetView *D3D12GPUDevice::CreateRenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc)
{
    DebugAssert(pTexture != nullptr);

    // we need to convert to d3d formats
    ID3D12Resource *pD3DResource;
    DXGI_FORMAT creationFormat, srvFormat, rtvFormat, dsvFormat;

    // fill in DSV structure
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
    switch (pTexture->GetResourceType())
    {
#if 0
    case GPU_RESOURCE_TYPE_TEXTURE1D:
        pD3DResource = static_cast<D3D12GPUTexture1D *>(pTexture)->GetD3DTexture();
        creationFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUTexture1D *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
        rtvDesc.Texture1D.MipSlice = pDesc->MipLevel;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
        pD3DResource = static_cast<D3D12GPUTexture1DArray *>(pTexture)->GetD3DTexture();
        creationFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUTexture1DArray *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
        rtvDesc.Texture1DArray.MipSlice = pDesc->MipLevel;
        rtvDesc.Texture1DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        rtvDesc.Texture1DArray.ArraySize = pDesc->NumLayers;
        break;
#endif

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        pD3DResource = static_cast<D3D12GPUTexture2D *>(pTexture)->GetD3DResource();
        creationFormat = D3D12Helpers::PixelFormatToDXGIFormat(static_cast<D3D12GPUTexture2D *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = pDesc->MipLevel;
        rtvDesc.Texture2D.PlaneSlice = 0;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        pD3DResource = static_cast<D3D12GPUTexture2DArray *>(pTexture)->GetD3DResource();
        creationFormat = D3D12Helpers::PixelFormatToDXGIFormat(static_cast<D3D12GPUTexture2DArray *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice = pDesc->MipLevel;
        rtvDesc.Texture2DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        rtvDesc.Texture2DArray.ArraySize = pDesc->NumLayers;
        rtvDesc.Texture2DArray.PlaneSlice = 0;
        break;

#if 0
    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        pD3DResource = static_cast<D3D12GPUTextureCube *>(pTexture)->GetD3DTexture();
        creationFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUTextureCube *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice = pDesc->MipLevel;
        rtvDesc.Texture2DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        rtvDesc.Texture2DArray.ArraySize = pDesc->NumLayers;
        rtvDesc.Texture2DArray.PlaneSlice = 0;
        break;

    case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
        pD3DResource = static_cast<D3D12GPUTextureCubeArray *>(pTexture)->GetD3DTexture();
        creationFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUTextureCubeArray *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice = pDesc->MipLevel;
        rtvDesc.Texture2DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        rtvDesc.Texture2DArray.ArraySize = pDesc->NumLayers;
        rtvDesc.Texture2DArray.PlaneSlice = 0;
        break;

    case GPU_RESOURCE_TYPE_DEPTH_TEXTURE:
        //pD3DResource = static_cast<D3D12GPUDepthTexture *>(pTexture)->GetD3DTexture();
        //creationFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUDepthTexture *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;
        rtvDesc.Texture2D.PlaneSlice = 0;
        break;
#endif

    default:
        Log_ErrorPrintf("D3D12GPUDevice::CreateRenderTargetView: Invalid resource type %s", NameTable_GetNameString(NameTables::GPUResourceType, pTexture->GetResourceType()));
        return nullptr;
    }

    // allocate handle
    D3D12DescriptorHandle handle;
    if (!m_pBackend->GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->Allocate(&handle))
    {
        Log_ErrorPrintf("D3D12GPUDevice::CreateRenderTargetView: Failed to allocate descriptor.");
        return nullptr;
    }

    // create RTV
    m_pD3DDevice->CreateRenderTargetView(pD3DResource, &rtvDesc, handle);
    return new D3D12GPURenderTargetView(pTexture, pDesc, handle, pD3DResource);
}

D3D12GPUDepthStencilBufferView::D3D12GPUDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc, const D3D12DescriptorHandle &descriptorHandle, ID3D12Resource *pD3DResource)
    : GPUDepthStencilBufferView(pTexture, pDesc)
    , m_descriptorHandle(descriptorHandle)
    , m_pD3DResource(pD3DResource)
{

}

D3D12GPUDepthStencilBufferView::~D3D12GPUDepthStencilBufferView()
{
    D3D12RenderBackend::GetInstance()->GetGraphicsCommandQueue()->ScheduleDescriptorForDeletion(m_descriptorHandle);
}

void D3D12GPUDepthStencilBufferView::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
    if (gpuMemoryUsage != nullptr)
        gpuMemoryUsage = 0;
}

void D3D12GPUDepthStencilBufferView::SetDebugName(const char *name)
{
    //D3D12Helpers::SetD3D12DeviceChildDebugName(m_pD3DDSV, name);
}

GPUDepthStencilBufferView *D3D12GPUDevice::CreateDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc)
{
    DebugAssert(pTexture != nullptr);

    // we need to convert to d3d formats
    ID3D12Resource *pD3DResource;
    DXGI_FORMAT creationFormat, srvFormat, rtvFormat, dsvFormat;

    // fill in DSV structure
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    switch (pTexture->GetResourceType())
    {
#if 0
    case GPU_RESOURCE_TYPE_TEXTURE1D:
        pD3DResource = static_cast<D3D12GPUTexture1D *>(pTexture)->GetD3DTexture();
        creationFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUTexture1D *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
        dsvDesc.Flags = 0;
        dsvDesc.Texture1D.MipSlice = pDesc->MipLevel;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
        pD3DResource = static_cast<D3D12GPUTexture1DArray *>(pTexture)->GetD3DTexture();
        creationFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUTexture1DArray *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
        dsvDesc.Flags = 0;
        dsvDesc.Texture1DArray.MipSlice = pDesc->MipLevel;
        dsvDesc.Texture1DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        dsvDesc.Texture1DArray.ArraySize = pDesc->NumLayers;
        break;
#endif

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        pD3DResource = static_cast<D3D12GPUTexture2D *>(pTexture)->GetD3DResource();
        creationFormat = D3D12Helpers::PixelFormatToDXGIFormat(static_cast<D3D12GPUTexture2D *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.Texture2D.MipSlice = pDesc->MipLevel;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        pD3DResource = static_cast<D3D12GPUTexture2DArray *>(pTexture)->GetD3DResource();
        creationFormat = D3D12Helpers::PixelFormatToDXGIFormat(static_cast<D3D12GPUTexture2DArray *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.Texture2DArray.MipSlice = pDesc->MipLevel;
        dsvDesc.Texture2DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        dsvDesc.Texture2DArray.ArraySize = pDesc->NumLayers;
        break;

#if 0
    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        pD3DResource = static_cast<D3D12GPUTextureCube *>(pTexture)->GetD3DTexture();
        creationFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUTextureCube *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Flags = 0;
        dsvDesc.Texture2DArray.MipSlice = pDesc->MipLevel;
        dsvDesc.Texture2DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        dsvDesc.Texture2DArray.ArraySize = pDesc->NumLayers;
        break;

    case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
        pD3DResource = static_cast<D3D12GPUTextureCubeArray *>(pTexture)->GetD3DTexture();
        creationFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUTextureCubeArray *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Flags = 0;
        dsvDesc.Texture2DArray.MipSlice = pDesc->MipLevel;
        dsvDesc.Texture2DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        dsvDesc.Texture2DArray.ArraySize = pDesc->NumLayers;
        break;

    case GPU_RESOURCE_TYPE_DEPTH_TEXTURE:
        pD3DResource = static_cast<D3D12GPUDepthTexture *>(pTexture)->GetD3DTexture();
        creationFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUDepthTexture *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = 0;
        dsvDesc.Texture2D.MipSlice = 0;
        break;
#endif

    default:
        Log_ErrorPrintf("D3D12GPUDevice::CreateDepthBufferView: Invalid resource type %s", NameTable_GetNameString(NameTables::GPUResourceType, pTexture->GetResourceType()));
        return nullptr;
    }

    // allocate handle
    D3D12DescriptorHandle handle;
    if (!m_pBackend->GetCPUDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)->Allocate(&handle))
    {
        Log_ErrorPrintf("D3D12GPUDevice::CreateRenderTargetView: Failed to allocate descriptor.");
        return nullptr;
    }

    // create RTV
    m_pD3DDevice->CreateDepthStencilView(pD3DResource, &dsvDesc, handle);
    return new D3D12GPUDepthStencilBufferView(pTexture, pDesc, handle, pD3DResource);
}

D3D12GPUComputeView::D3D12GPUComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc, const D3D12DescriptorHandle &descriptorHandle, ID3D12Resource *pD3DResource)
    : GPUComputeView(pResource, pDesc)
    , m_descriptorHandle(descriptorHandle)
    , m_pD3DResource(pD3DResource)
{

}

D3D12GPUComputeView::~D3D12GPUComputeView()
{
    D3D12RenderBackend::GetInstance()->GetGraphicsCommandQueue()->ScheduleDescriptorForDeletion(m_descriptorHandle);
}

void D3D12GPUComputeView::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
    if (gpuMemoryUsage != nullptr)
        gpuMemoryUsage = 0;
}

void D3D12GPUComputeView::SetDebugName(const char *name)
{
    //D3D12Helpers::SetD3D12DeviceChildDebugName(m_pD3DUAV, name);
}

GPUComputeView *D3D12GPUDevice::CreateComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc)
{
    Panic("Unimplemented");
    return nullptr;
}
