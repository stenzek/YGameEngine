#include "D3D11Renderer/PrecompiledHeader.h"
#include "D3D11Renderer/D3D11GPUTexture.h"
#include "D3D11Renderer/D3D11GPUContext.h"
#include "D3D11Renderer/D3D11GPUDevice.h"
Log_SetChannel(D3D11GPUContext);

static DWORD MapTextureFlagsToD3DBindFlags(uint32 textureFlags)
{
    uint32 bindFlags = 0;
    if (textureFlags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
        bindFlags |= D3D11_BIND_SHADER_RESOURCE;
    if (textureFlags & GPU_TEXTURE_FLAG_BIND_RENDER_TARGET)
        bindFlags |= D3D11_BIND_RENDER_TARGET;
    if (textureFlags & GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER)
        bindFlags |= D3D11_BIND_DEPTH_STENCIL;
    if (textureFlags & GPU_TEXTURE_FLAG_BIND_COMPUTE_WRITABLE)
        bindFlags |= D3D11_BIND_UNORDERED_ACCESS;

    return bindFlags;
}

static DWORD MapTextureFlagsToD3DMiscFlags(uint32 textureFlags)
{
    uint32 miscFlags = 0;
    if (textureFlags & GPU_TEXTURE_FLAG_GENERATE_MIPS)
        miscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
    
    return miscFlags;
}

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

D3D11GPUTexture1D::D3D11GPUTexture1D(const GPU_TEXTURE1D_DESC *pDesc,
                                     ID3D11Texture1D *pD3DTexture, ID3D11Texture1D *pD3DStagingTexture,
                                     ID3D11ShaderResourceView *pD3DSRV,
                                     ID3D11SamplerState *pD3DSamplerState)
    : GPUTexture1D(pDesc),
      m_pD3DTexture(pD3DTexture),
      m_pD3DStagingTexture(pD3DStagingTexture),
      m_pD3DSRV(pD3DSRV),
      m_pD3DSamplerState(pD3DSamplerState)
{   

}

D3D11GPUTexture1D::~D3D11GPUTexture1D()
{
    if (m_pD3DSamplerState != nullptr)
        m_pD3DSamplerState->Release();
    if (m_pD3DSRV != nullptr)
        m_pD3DSRV->Release();
    if (m_pD3DStagingTexture != nullptr)
        m_pD3DStagingTexture->Release();
    if (m_pD3DTexture != nullptr)
        m_pD3DTexture->Release();
}

void D3D11GPUTexture1D::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), 1, 1);

        *gpuMemoryUsage = memoryUsage;
    }
}

void D3D11GPUTexture1D::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DTexture, name);
}


GPUTexture1D *D3D11GPUDevice::CreateTexture1D(const GPU_TEXTURE1D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc,
                                             const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */)
{
    HRESULT hResult;

    // validate pixel formats.
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DXGI_FORMAT creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr && creationFormat != DXGI_FORMAT_UNKNOWN);
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT);

    // determine view formats
    DXGI_FORMAT srvFormat, rtvFormat, dsvFormat;
    MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);

    // fill in known fields
    D3D11_TEXTURE1D_DESC D3DTextureDesc;
    D3DTextureDesc.Width = pTextureDesc->Width;
    D3DTextureDesc.MipLevels = pTextureDesc->MipLevels;
    D3DTextureDesc.ArraySize = 1;
    D3DTextureDesc.Format = creationFormat;

    // determine usage
    if (pTextureDesc->Flags & (GPU_TEXTURE_FLAG_READABLE | GPU_TEXTURE_FLAG_WRITABLE | GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER))
    {
        // set to default usage
        D3DTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    }
    else if (pTextureDesc->Flags)
    {
        // otherwise immutable, ensure we have data
        DebugAssert(ppInitialData != nullptr);
        D3DTextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    }

    // bindflags
    D3DTextureDesc.CPUAccessFlags = 0;
    D3DTextureDesc.BindFlags = MapTextureFlagsToD3DBindFlags(pTextureDesc->Flags);
    D3DTextureDesc.MiscFlags = MapTextureFlagsToD3DMiscFlags(pTextureDesc->Flags);

    // initial data
    D3D11_SUBRESOURCE_DATA *pD3DInitialData = nullptr;
    if (ppInitialData != nullptr)
    {
        uint32 nInitializers = pTextureDesc->MipLevels;
        pD3DInitialData = (D3D11_SUBRESOURCE_DATA *)alloca(sizeof(D3D11_SUBRESOURCE_DATA)* nInitializers);
        for (uint32 i = 0; i < nInitializers; i++)
        {
            pD3DInitialData[i].pSysMem = ppInitialData[i];
            pD3DInitialData[i].SysMemPitch = pInitialDataPitch[i];
            pD3DInitialData[i].SysMemSlicePitch = 0;
        }
    }

    // create texture
    ID3D11Texture1D *pD3DTexture;
    hResult = m_pD3DDevice->CreateTexture1D(&D3DTextureDesc, pD3DInitialData, &pD3DTexture);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUDevice::CreateTexture1D: CreateTexture1D failed with hResult %08X", hResult);
        return false;
    }

    // create staging texture if readable texture requested
    ID3D11Texture1D *pD3DStagingTexture = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_READABLE)
    {
        D3D11_TEXTURE1D_DESC D3DStagingTextureDesc;
        D3DStagingTextureDesc.Width = D3DTextureDesc.Width;
        D3DStagingTextureDesc.MipLevels = 1;
        D3DStagingTextureDesc.ArraySize = 1;
        D3DStagingTextureDesc.Format = D3DTextureDesc.Format;
        D3DStagingTextureDesc.Usage = D3D11_USAGE_STAGING;
        D3DStagingTextureDesc.BindFlags = 0;
        D3DStagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        D3DStagingTextureDesc.MiscFlags = 0;

        hResult = m_pD3DDevice->CreateTexture1D(&D3DStagingTextureDesc, nullptr, &pD3DStagingTexture);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture1D: CreateTexture1D failed for staging texture with hResult %08X", hResult);
            pD3DTexture->Release();
            return false;
        }
    }

    // create shader resource view
    ID3D11ShaderResourceView *pD3DSRV = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        DebugAssert(srvFormat != DXGI_FORMAT_UNKNOWN);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = srvFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = pTextureDesc->MipLevels;

        hResult = m_pD3DDevice->CreateShaderResourceView(pD3DTexture, &srvDesc, &pD3DSRV);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture1D: CreateShaderResourceView failed with hResult %08X", hResult);
            SAFE_RELEASE(pD3DStagingTexture);
            pD3DTexture->Release();
            return false;
        }
    }

    // create sampler state
    ID3D11SamplerState *pD3DSamplerState = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        DebugAssert(pSamplerStateDesc != nullptr);

        if ((pD3DSamplerState = D3D11Helpers::CreateD3D11SamplerState(m_pD3DDevice, pSamplerStateDesc)) == nullptr)
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture1D: Failed to create sampler state for texture.");
            SAFE_RELEASE(pD3DSRV);
            SAFE_RELEASE(pD3DStagingTexture);
            pD3DTexture->Release();
            return false;
        }
    }

    // create class
    return new D3D11GPUTexture1D(pTextureDesc, pD3DTexture, pD3DStagingTexture, pD3DSRV, pD3DSamplerState);
}

bool D3D11GPUContext::ReadTexture(GPUTexture1D *pTexture, void *pDestination, uint32 cbDestination, uint32 mipIndex, uint32 start, uint32 count)
{
    HRESULT hResult;
    D3D11GPUTexture1D *pD3DTexture = static_cast<D3D11GPUTexture1D *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_READABLE);
    DebugAssert(count > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pD3DTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    if ((start + count) > mipWidth)
        return false;

    // copy the texture to the staging texture
    // if this is a higher mip level, or a non-full-copy, it'll only partially fill the staging texture, this is okay.
    D3D11_BOX sourceBox = { start, 0, 0, start + count, 1, 1 };
    m_pD3DContext->CopySubresourceRegion(pD3DTexture->GetD3DStagingTexture(), 0, 0, 0, 0, pD3DTexture->GetD3DTexture(), D3D11CalcSubresource(mipIndex, 0, pD3DTexture->GetDesc()->MipLevels), &sourceBox);

    // map the staging texture
    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
    hResult = m_pD3DContext->Map(pD3DTexture->GetD3DStagingTexture(), 0, D3D11_MAP_READ, 0, &mappedSubResource);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUContext::ReadTexture[Texture1D]: Mapping staging texture failed with hResult %08X", hResult);
        return false;
    }

    // copy line by line
    Y_memcpy(pDestination, reinterpret_cast<const byte *>(mappedSubResource.pData), Min(cbDestination, (uint32)mappedSubResource.RowPitch));
    
    // unmap resource again
    m_pD3DContext->Unmap(pD3DTexture->GetD3DStagingTexture(), 0);
    return true;
}

bool D3D11GPUContext::WriteTexture(GPUTexture1D *pTexture, const void *pSource, uint32 cbSource, uint32 mipIndex, uint32 start, uint32 count)
{
    D3D11GPUTexture1D *pD3DTexture = static_cast<D3D11GPUTexture1D *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(count > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pD3DTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    if ((start + count) > mipWidth)
        return false;

    // find subresource
    uint32 subResourceToUpdate = D3D11CalcSubresource(mipIndex, 0, pD3DTexture->GetDesc()->MipLevels);

    // invoke update
    D3D11_BOX destinationBox = { start, 0, 0, start + count, 1, 1 };    
    m_pD3DContext->UpdateSubresource(pD3DTexture->GetD3DTexture(), subResourceToUpdate, &destinationBox, pSource, cbSource, 0);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

D3D11GPUTexture1DArray::D3D11GPUTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pDesc,
                                               ID3D11Texture1D *pD3DTexture, ID3D11Texture1D *pD3DStagingTexture,
                                               ID3D11ShaderResourceView *pD3DSRV,
                                               ID3D11SamplerState *pD3DSamplerState)
    : GPUTexture1DArray(pDesc),
      m_pD3DTexture(pD3DTexture),
      m_pD3DStagingTexture(pD3DStagingTexture),
      m_pD3DSRV(pD3DSRV),
      m_pD3DSamplerState(pD3DSamplerState)
{

}

D3D11GPUTexture1DArray::~D3D11GPUTexture1DArray()
{
    if (m_pD3DSamplerState != nullptr)
        m_pD3DSamplerState->Release();

    if (m_pD3DSRV != nullptr)
        m_pD3DSRV->Release();

    if (m_pD3DStagingTexture != nullptr)
        m_pD3DStagingTexture->Release();

    if (m_pD3DTexture != nullptr)
        m_pD3DTexture->Release();
}

void D3D11GPUTexture1DArray::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), 1, 1);

        *gpuMemoryUsage = memoryUsage * m_desc.ArraySize;
    }
}

void D3D11GPUTexture1DArray::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DTexture, name);
}

GPUTexture1DArray *D3D11GPUDevice::CreateTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc,
                                                       const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */)
{
    HRESULT hResult;

    // validate pixel formats.
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DXGI_FORMAT creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr && creationFormat != DXGI_FORMAT_UNKNOWN);
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT && pTextureDesc->ArraySize > 0);

    // determine view formats
    DXGI_FORMAT srvFormat, rtvFormat, dsvFormat;
    MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);

    // fill in known fields
    D3D11_TEXTURE1D_DESC D3DTextureDesc;
    D3DTextureDesc.Width = pTextureDesc->Width;
    D3DTextureDesc.MipLevels = pTextureDesc->MipLevels;
    D3DTextureDesc.ArraySize = pTextureDesc->ArraySize;
    D3DTextureDesc.Format = creationFormat;

    // determine usage
    if (pTextureDesc->Flags & (GPU_TEXTURE_FLAG_READABLE | GPU_TEXTURE_FLAG_WRITABLE | GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER))
    {
        // set to default usage
        D3DTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    }
    else if (pTextureDesc->Flags)
    {
        // otherwise immutable, ensure we have data
        DebugAssert(ppInitialData != nullptr);
        D3DTextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    }

    // bindflags
    D3DTextureDesc.CPUAccessFlags = 0;
    D3DTextureDesc.BindFlags = MapTextureFlagsToD3DBindFlags(pTextureDesc->Flags);
    D3DTextureDesc.MiscFlags = MapTextureFlagsToD3DMiscFlags(pTextureDesc->Flags);

    // initial data
    D3D11_SUBRESOURCE_DATA *pD3DInitialData = nullptr;
    if (ppInitialData != nullptr)
    {
        uint32 nInitializers = pTextureDesc->MipLevels * pTextureDesc->ArraySize;
        pD3DInitialData = (D3D11_SUBRESOURCE_DATA *)alloca(sizeof(D3D11_SUBRESOURCE_DATA)* nInitializers);
        for (uint32 i = 0; i < nInitializers; i++)
        {
            pD3DInitialData[i].pSysMem = ppInitialData[i];
            pD3DInitialData[i].SysMemPitch = pInitialDataPitch[i];
            pD3DInitialData[i].SysMemSlicePitch = 0;
        }
    }

    // create texture
    ID3D11Texture1D *pD3DTexture;
    hResult = m_pD3DDevice->CreateTexture1D(&D3DTextureDesc, pD3DInitialData, &pD3DTexture);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUDevice::CreateTexture1DArray: CreateTexture1D failed with hResult %08X", hResult);
        return false;
    }

    // create staging texture if readable texture requested
    ID3D11Texture1D *pD3DStagingTexture = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_READABLE)
    {
        D3D11_TEXTURE1D_DESC D3DStagingTextureDesc;
        D3DStagingTextureDesc.Width = D3DTextureDesc.Width;
        D3DStagingTextureDesc.MipLevels = 1;
        D3DStagingTextureDesc.ArraySize = 1;
        D3DStagingTextureDesc.Format = D3DTextureDesc.Format;
        D3DStagingTextureDesc.Usage = D3D11_USAGE_STAGING;
        D3DStagingTextureDesc.BindFlags = 0;
        D3DStagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        D3DStagingTextureDesc.MiscFlags = 0;

        hResult = m_pD3DDevice->CreateTexture1D(&D3DStagingTextureDesc, nullptr, &pD3DStagingTexture);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture1DArray: CreateTexture1D failed for staging texture with hResult %08X", hResult);
            pD3DTexture->Release();
            return false;
        }
    }

    // create shader resource view
    ID3D11ShaderResourceView *pD3DSRV = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        DebugAssert(srvFormat != DXGI_FORMAT_UNKNOWN);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = srvFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
        srvDesc.Texture1DArray.MostDetailedMip = 0;
        srvDesc.Texture1DArray.MipLevels = pTextureDesc->MipLevels;
        srvDesc.Texture1DArray.FirstArraySlice = 0;
        srvDesc.Texture1DArray.ArraySize = pTextureDesc->ArraySize;

        hResult = m_pD3DDevice->CreateShaderResourceView(pD3DTexture, &srvDesc, &pD3DSRV);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture1DArray: CreateShaderResourceView failed with hResult %08X", hResult);
            SAFE_RELEASE(pD3DStagingTexture);
            pD3DTexture->Release();
            return false;
        }
    }
    
    // create sampler state
    ID3D11SamplerState *pD3DSamplerState = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        DebugAssert(pSamplerStateDesc != nullptr);

        if ((pD3DSamplerState = D3D11Helpers::CreateD3D11SamplerState(m_pD3DDevice, pSamplerStateDesc)) == nullptr)
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture1DArray: Failed to create sampler state for texture.");
            SAFE_RELEASE(pD3DSRV);
            SAFE_RELEASE(pD3DStagingTexture);
            pD3DTexture->Release();
            return false;
        }
    }

    // create class
    return new D3D11GPUTexture1DArray(pTextureDesc, pD3DTexture, pD3DStagingTexture, pD3DSRV, pD3DSamplerState);
}

bool D3D11GPUContext::ReadTexture(GPUTexture1DArray *pTexture, void *pDestination, uint32 cbDestination, uint32 arrayIndex, uint32 mipIndex, uint32 start, uint32 count)
{
    HRESULT hResult;
    D3D11GPUTexture1DArray *pD3DTexture = static_cast<D3D11GPUTexture1DArray *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_READABLE);
    DebugAssert(count > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pD3DTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    if ((start + count) > mipWidth || arrayIndex >= pTexture->GetDesc()->ArraySize)
        return false;

    // copy the texture to the staging texture
    // if this is a higher mip level, or a non-full-copy, it'll only partially fill the staging texture, this is okay.
    D3D11_BOX sourceBox = { start, 0, 0, start + count, 1, 1 };
    m_pD3DContext->CopySubresourceRegion(pD3DTexture->GetD3DStagingTexture(), 0, 0, 0, 0, pD3DTexture->GetD3DTexture(), D3D11CalcSubresource(mipIndex, arrayIndex, pD3DTexture->GetDesc()->MipLevels), &sourceBox);

    // map the staging texture
    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
    hResult = m_pD3DContext->Map(pD3DTexture->GetD3DStagingTexture(), 0, D3D11_MAP_READ, 0, &mappedSubResource);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUContext::ReadTexture[Texture1DArray]: Mapping staging texture failed with hResult %08X", hResult);
        return false;
    }

    // copy line by line
    Y_memcpy(pDestination, reinterpret_cast<const byte *>(mappedSubResource.pData), Min(cbDestination, (uint32)mappedSubResource.RowPitch));

    // unmap resource again
    m_pD3DContext->Unmap(pD3DTexture->GetD3DStagingTexture(), 0);
    return true;
}

bool D3D11GPUContext::WriteTexture(GPUTexture1DArray *pTexture, const void *pSource, uint32 cbSource, uint32 arrayIndex, uint32 mipIndex, uint32 start, uint32 count)
{
    D3D11GPUTexture1DArray *pD3DTexture = static_cast<D3D11GPUTexture1DArray *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(count > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pD3DTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    if ((start + count) > mipWidth || arrayIndex >= pD3DTexture->GetDesc()->ArraySize)
        return false;

    // find subresource
    uint32 subResourceToUpdate = D3D11CalcSubresource(mipIndex, arrayIndex, pD3DTexture->GetDesc()->MipLevels);

    // invoke update
    D3D11_BOX destinationBox = { start, 0, 0, start + count, 1, 1 };
    m_pD3DContext->UpdateSubresource(pD3DTexture->GetD3DTexture(), subResourceToUpdate, &destinationBox, pSource, cbSource, 0);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

D3D11GPUTexture2D::D3D11GPUTexture2D(const GPU_TEXTURE2D_DESC *pDesc,
                                     ID3D11Texture2D *pD3DTexture, ID3D11Texture2D *pD3DStagingTexture,
                                     ID3D11ShaderResourceView *pD3DSRV,
                                     ID3D11SamplerState *pD3DSamplerState)
    : GPUTexture2D(pDesc),
      m_pD3DTexture(pD3DTexture),
      m_pD3DStagingTexture(pD3DStagingTexture),
      m_pD3DSRV(pD3DSRV),
      m_pD3DSamplerState(pD3DSamplerState)
{   

}

D3D11GPUTexture2D::~D3D11GPUTexture2D()
{
    if (m_pD3DSamplerState != nullptr)
        m_pD3DSamplerState->Release();

    if (m_pD3DSRV != nullptr)
        m_pD3DSRV->Release();

    if (m_pD3DStagingTexture != nullptr)
        m_pD3DStagingTexture->Release();

    if (m_pD3DTexture != nullptr)
        m_pD3DTexture->Release();
}

void D3D11GPUTexture2D::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
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

void D3D11GPUTexture2D::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DTexture, name);
}


GPUTexture2D *D3D11GPUDevice::CreateTexture2D(const GPU_TEXTURE2D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc,
                                             const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */)
{
    HRESULT hResult;

    // validate pixel formats.
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DXGI_FORMAT creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr && creationFormat != DXGI_FORMAT_UNKNOWN);
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT);

    // determine view formats
    DXGI_FORMAT srvFormat, rtvFormat, dsvFormat;
    MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);

    // fill in known fields
    D3D11_TEXTURE2D_DESC D3DTextureDesc;
    D3DTextureDesc.Width = pTextureDesc->Width;
    D3DTextureDesc.Height = pTextureDesc->Height;
    D3DTextureDesc.MipLevels = pTextureDesc->MipLevels;
    D3DTextureDesc.ArraySize = 1;
    D3DTextureDesc.Format = creationFormat;
    D3DTextureDesc.SampleDesc.Count = 1;
    D3DTextureDesc.SampleDesc.Quality = 0;

    // determine usage
    if (pTextureDesc->Flags & (GPU_TEXTURE_FLAG_READABLE | GPU_TEXTURE_FLAG_WRITABLE | GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER))
    {
        // set to default usage
        D3DTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    }
    else if (pTextureDesc->Flags)
    {
        // otherwise immutable, ensure we have data
        DebugAssert(ppInitialData != nullptr);
        D3DTextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    }

    // bindflags
    D3DTextureDesc.CPUAccessFlags = 0;
    D3DTextureDesc.BindFlags = MapTextureFlagsToD3DBindFlags(pTextureDesc->Flags);
    D3DTextureDesc.MiscFlags = MapTextureFlagsToD3DMiscFlags(pTextureDesc->Flags);

    // initial data
    D3D11_SUBRESOURCE_DATA *pD3DInitialData = nullptr;
    if (ppInitialData != nullptr)
    {
        uint32 nInitializers = pTextureDesc->MipLevels;
        pD3DInitialData = (D3D11_SUBRESOURCE_DATA *)alloca(sizeof(D3D11_SUBRESOURCE_DATA)* nInitializers);
        for (uint32 i = 0; i < nInitializers; i++)
        {
            pD3DInitialData[i].pSysMem = ppInitialData[i];
            pD3DInitialData[i].SysMemPitch = pInitialDataPitch[i];
            pD3DInitialData[i].SysMemSlicePitch = 0;
        }
    }

    // create texture
    ID3D11Texture2D *pD3DTexture;
    hResult = m_pD3DDevice->CreateTexture2D(&D3DTextureDesc, pD3DInitialData, &pD3DTexture);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUDevice::CreateTexture2D: CreateTexture2D failed with hResult %08X", hResult);
        return false;
    }

    // create staging texture if readable texture requested
    ID3D11Texture2D *pD3DStagingTexture = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_READABLE)
    {
        D3D11_TEXTURE2D_DESC D3DStagingTextureDesc;
        D3DStagingTextureDesc.Width = D3DTextureDesc.Width;
        D3DStagingTextureDesc.Height = D3DTextureDesc.Height;
        D3DStagingTextureDesc.MipLevels = 1;
        D3DStagingTextureDesc.ArraySize = 1;
        D3DStagingTextureDesc.Format = D3DTextureDesc.Format;
        D3DStagingTextureDesc.SampleDesc.Count = D3DTextureDesc.SampleDesc.Count;
        D3DStagingTextureDesc.SampleDesc.Quality = D3DTextureDesc.SampleDesc.Quality;
        D3DStagingTextureDesc.Usage = D3D11_USAGE_STAGING;
        D3DStagingTextureDesc.BindFlags = 0;
        D3DStagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        D3DStagingTextureDesc.MiscFlags = 0;

        hResult = m_pD3DDevice->CreateTexture2D(&D3DStagingTextureDesc, nullptr, &pD3DStagingTexture);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture2D: CreateTexture2D failed for staging texture with hResult %08X", hResult);
            pD3DTexture->Release();
            return false;
        }
    }

    // create shader resource view
    ID3D11ShaderResourceView *pD3DSRV = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        DebugAssert(srvFormat != DXGI_FORMAT_UNKNOWN);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = srvFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = pTextureDesc->MipLevels;

        hResult = m_pD3DDevice->CreateShaderResourceView(pD3DTexture, &srvDesc, &pD3DSRV);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture2D: CreateShaderResourceView failed with hResult %08X", hResult);
            SAFE_RELEASE(pD3DStagingTexture);
            pD3DTexture->Release();
            return false;
        }
    }
    
    // create sampler state
    ID3D11SamplerState *pD3DSamplerState = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        DebugAssert(pSamplerStateDesc != nullptr);

        if ((pD3DSamplerState = D3D11Helpers::CreateD3D11SamplerState(m_pD3DDevice, pSamplerStateDesc)) == nullptr)
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture2D: Failed to create sampler state for texture.");
            SAFE_RELEASE(pD3DSRV);
            SAFE_RELEASE(pD3DStagingTexture);
            pD3DTexture->Release();
            return false;
        }
    }

    // create class
    return new D3D11GPUTexture2D(pTextureDesc, pD3DTexture, pD3DStagingTexture, pD3DSRV, pD3DSamplerState);
}

bool D3D11GPUContext::ReadTexture(GPUTexture2D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    HRESULT hResult;
    D3D11GPUTexture2D *pD3DTexture = static_cast<D3D11GPUTexture2D *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_READABLE);
    DebugAssert(countX > 0 && countY > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pD3DTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pD3DTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight)
        return false;

    // check destination size
    if ((countY * destinationRowPitch) > cbDestination)
        return false;

    // copy the texture to the staging texture
    // if this is a higher mip level, or a non-full-copy, it'll only partially fill the staging texture, this is okay.
    D3D11_BOX sourceBox = { startX, startY, 0, startX + countX, startY + countY, 1 };
    m_pD3DContext->CopySubresourceRegion(pD3DTexture->GetD3DStagingTexture(), 0, 0, 0, 0, pD3DTexture->GetD3DTexture(), D3D11CalcSubresource(mipIndex, 0, pD3DTexture->GetDesc()->MipLevels), &sourceBox);

    // map the staging texture
    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
    hResult = m_pD3DContext->Map(pD3DTexture->GetD3DStagingTexture(), 0, D3D11_MAP_READ, 0, &mappedSubResource);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUContext::ReadTexture[Texture2D]: Mapping staging texture failed with hResult %08X", hResult);
        return false;
    }

    // copy line by line
    Y_memcpy_stride(pDestination, destinationRowPitch, reinterpret_cast<const byte *>(mappedSubResource.pData), mappedSubResource.RowPitch, Min(mappedSubResource.RowPitch, destinationRowPitch), countY);
    
    // unmap resource again
    m_pD3DContext->Unmap(pD3DTexture->GetD3DStagingTexture(), 0);
    return true;
}

bool D3D11GPUContext::WriteTexture(GPUTexture2D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    D3D11GPUTexture2D *pD3DTexture = static_cast<D3D11GPUTexture2D *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(countX > 0 && countY > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pD3DTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pD3DTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight)
        return false;

    // find subresource
    uint32 subResourceToUpdate = D3D11CalcSubresource(mipIndex, 0, pD3DTexture->GetDesc()->MipLevels);

    // invoke update
    D3D11_BOX destinationBox = { startX, startY, 0, startX + countX, startY + countY, 1 };    
    m_pD3DContext->UpdateSubresource(pD3DTexture->GetD3DTexture(), subResourceToUpdate, &destinationBox, pSource, sourceRowPitch, 0);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

D3D11GPUTexture2DArray::D3D11GPUTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pDesc,
                                               ID3D11Texture2D *pD3DTexture, ID3D11Texture2D *pD3DStagingTexture,
                                               ID3D11ShaderResourceView *pD3DSRV,
                                               ID3D11SamplerState *pD3DSamplerState)
    : GPUTexture2DArray(pDesc),
      m_pD3DTexture(pD3DTexture),
      m_pD3DStagingTexture(pD3DStagingTexture),
      m_pD3DSRV(pD3DSRV),
      m_pD3DSamplerState(pD3DSamplerState)
{

}

D3D11GPUTexture2DArray::~D3D11GPUTexture2DArray()
{
    if (m_pD3DSamplerState != nullptr)
        m_pD3DSamplerState->Release();

    if (m_pD3DSRV != nullptr)
        m_pD3DSRV->Release();

    if (m_pD3DStagingTexture != nullptr)
        m_pD3DStagingTexture->Release();

    if (m_pD3DTexture != nullptr)
        m_pD3DTexture->Release();
}

void D3D11GPUTexture2DArray::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), 1, 1);

        *gpuMemoryUsage = memoryUsage * m_desc.ArraySize;
    }
}

void D3D11GPUTexture2DArray::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DTexture, name);
}

GPUTexture2DArray *D3D11GPUDevice::CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc,
                                                       const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */)
{
    HRESULT hResult;

    // validate pixel formats.
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DXGI_FORMAT creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr && creationFormat != DXGI_FORMAT_UNKNOWN);
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT && pTextureDesc->ArraySize > 0);

    // determine view formats
    DXGI_FORMAT srvFormat, rtvFormat, dsvFormat;
    MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);

    // fill in known fields
    D3D11_TEXTURE2D_DESC D3DTextureDesc;
    D3DTextureDesc.Width = pTextureDesc->Width;
    D3DTextureDesc.Height = pTextureDesc->Height;
    D3DTextureDesc.MipLevels = pTextureDesc->MipLevels;
    D3DTextureDesc.ArraySize = pTextureDesc->ArraySize;
    D3DTextureDesc.Format = creationFormat;
    D3DTextureDesc.SampleDesc.Count = 1;
    D3DTextureDesc.SampleDesc.Quality = 0;

    // determine usage
    if (pTextureDesc->Flags & (GPU_TEXTURE_FLAG_READABLE | GPU_TEXTURE_FLAG_WRITABLE | GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER))
    {
        // set to default usage
        D3DTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    }
    else if (pTextureDesc->Flags)
    {
        // otherwise immutable, ensure we have data
        DebugAssert(ppInitialData != nullptr);
        D3DTextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    }

    // bindflags
    D3DTextureDesc.CPUAccessFlags = 0;
    D3DTextureDesc.BindFlags = MapTextureFlagsToD3DBindFlags(pTextureDesc->Flags);
    D3DTextureDesc.MiscFlags = MapTextureFlagsToD3DMiscFlags(pTextureDesc->Flags);

    // initial data
    D3D11_SUBRESOURCE_DATA *pD3DInitialData = nullptr;
    if (ppInitialData != nullptr)
    {
        uint32 nInitializers = pTextureDesc->MipLevels * pTextureDesc->ArraySize;
        pD3DInitialData = (D3D11_SUBRESOURCE_DATA *)alloca(sizeof(D3D11_SUBRESOURCE_DATA)* nInitializers);
        for (uint32 i = 0; i < nInitializers; i++)
        {
            pD3DInitialData[i].pSysMem = ppInitialData[i];
            pD3DInitialData[i].SysMemPitch = pInitialDataPitch[i];
            pD3DInitialData[i].SysMemSlicePitch = 0;
        }
    }

    // create texture
    ID3D11Texture2D *pD3DTexture;
    hResult = m_pD3DDevice->CreateTexture2D(&D3DTextureDesc, pD3DInitialData, &pD3DTexture);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUDevice::CreateTexture2DArray: CreateTexture2D failed with hResult %08X", hResult);
        return false;
    }

    // create staging texture if readable texture requested
    ID3D11Texture2D *pD3DStagingTexture = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_READABLE)
    {
        D3D11_TEXTURE2D_DESC D3DStagingTextureDesc;
        D3DStagingTextureDesc.Width = D3DTextureDesc.Width;
        D3DStagingTextureDesc.Height = D3DTextureDesc.Height;
        D3DStagingTextureDesc.MipLevels = 1;
        D3DStagingTextureDesc.ArraySize = 1;
        D3DStagingTextureDesc.Format = D3DTextureDesc.Format;
        D3DStagingTextureDesc.SampleDesc.Count = D3DTextureDesc.SampleDesc.Count;
        D3DStagingTextureDesc.SampleDesc.Quality = D3DTextureDesc.SampleDesc.Quality;
        D3DStagingTextureDesc.Usage = D3D11_USAGE_STAGING;
        D3DStagingTextureDesc.BindFlags = 0;
        D3DStagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        D3DStagingTextureDesc.MiscFlags = 0;

        hResult = m_pD3DDevice->CreateTexture2D(&D3DStagingTextureDesc, nullptr, &pD3DStagingTexture);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture2DArray: CreateTexture2D failed for staging texture with hResult %08X", hResult);
            pD3DTexture->Release();
            return false;
        }
    }

    // create shader resource view
    ID3D11ShaderResourceView *pD3DSRV = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        DebugAssert(srvFormat != DXGI_FORMAT_UNKNOWN);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = srvFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MostDetailedMip = 0;
        srvDesc.Texture2DArray.MipLevels = pTextureDesc->MipLevels;
        srvDesc.Texture2DArray.FirstArraySlice = 0;
        srvDesc.Texture2DArray.ArraySize = pTextureDesc->ArraySize;

        hResult = m_pD3DDevice->CreateShaderResourceView(pD3DTexture, &srvDesc, &pD3DSRV);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture2DArray: CreateShaderResourceView failed with hResult %08X", hResult);
            SAFE_RELEASE(pD3DStagingTexture);
            pD3DTexture->Release();
            return false;
        }
    }
    
    // create sampler state
    ID3D11SamplerState *pD3DSamplerState = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        DebugAssert(pSamplerStateDesc != nullptr);

        if ((pD3DSamplerState = D3D11Helpers::CreateD3D11SamplerState(m_pD3DDevice, pSamplerStateDesc)) == nullptr)
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture2DArray: Failed to create sampler state for texture.");
            SAFE_RELEASE(pD3DSRV);
            SAFE_RELEASE(pD3DStagingTexture);
            pD3DTexture->Release();
            return false;
        }
    }

    // create class
    return new D3D11GPUTexture2DArray(pTextureDesc, pD3DTexture, pD3DStagingTexture, pD3DSRV, pD3DSamplerState);
}

bool D3D11GPUContext::ReadTexture(GPUTexture2DArray *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 arrayIndex, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    HRESULT hResult;
    D3D11GPUTexture2DArray *pD3DTexture = static_cast<D3D11GPUTexture2DArray *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_READABLE);
    DebugAssert(countX > 0 && countY > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pD3DTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pD3DTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight || arrayIndex >= pTexture->GetDesc()->ArraySize)
        return false;

    // check destination size
    if ((countY * destinationRowPitch) > cbDestination)
        return false;

    // copy the texture to the staging texture
    // if this is a higher mip level, or a non-full-copy, it'll only partially fill the staging texture, this is okay.
    D3D11_BOX sourceBox = { startX, startY, 0, startX + countX, startY + countY, 1 };
    m_pD3DContext->CopySubresourceRegion(pD3DTexture->GetD3DStagingTexture(), 0, 0, 0, 0, pD3DTexture->GetD3DTexture(), D3D11CalcSubresource(mipIndex, arrayIndex, pD3DTexture->GetDesc()->MipLevels), &sourceBox);

    // map the staging texture
    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
    hResult = m_pD3DContext->Map(pD3DTexture->GetD3DStagingTexture(), 0, D3D11_MAP_READ, 0, &mappedSubResource);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUContext::ReadTexture[Texture2DArray]: Mapping staging texture failed with hResult %08X", hResult);
        return false;
    }

    // copy line by line
    Y_memcpy_stride(pDestination, destinationRowPitch, reinterpret_cast<const byte *>(mappedSubResource.pData), mappedSubResource.RowPitch, Min(mappedSubResource.RowPitch, destinationRowPitch), countY);

    // unmap resource again
    m_pD3DContext->Unmap(pD3DTexture->GetD3DStagingTexture(), 0);
    return true;
}

bool D3D11GPUContext::WriteTexture(GPUTexture2DArray *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 arrayIndex, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    D3D11GPUTexture2DArray *pD3DTexture = static_cast<D3D11GPUTexture2DArray *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(countX > 0 && countY > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pD3DTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pD3DTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight || arrayIndex >= pD3DTexture->GetDesc()->ArraySize)
        return false;

    // find subresource
    uint32 subResourceToUpdate = D3D11CalcSubresource(mipIndex, arrayIndex, pD3DTexture->GetDesc()->MipLevels);

    // invoke update
    D3D11_BOX destinationBox = { startX, startY, 0, startX + countX, startY + countY, 1 };
    m_pD3DContext->UpdateSubresource(pD3DTexture->GetD3DTexture(), subResourceToUpdate, &destinationBox, pSource, sourceRowPitch, 0);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

D3D11GPUTexture3D::D3D11GPUTexture3D(const GPU_TEXTURE3D_DESC *pDesc,
                                     ID3D11Texture3D *pD3DTexture, ID3D11Texture3D *pD3DStagingTexture,
                                     ID3D11ShaderResourceView *pD3DSRV,
                                     ID3D11SamplerState *pD3DSamplerState)
    : GPUTexture3D(pDesc),
      m_pD3DTexture(pD3DTexture),
      m_pD3DStagingTexture(pD3DStagingTexture),
      m_pD3DSRV(pD3DSRV),
      m_pD3DSamplerState(pD3DSamplerState)
{   

}

D3D11GPUTexture3D::~D3D11GPUTexture3D()
{
    if (m_pD3DSamplerState != nullptr)
        m_pD3DSamplerState->Release();

    if (m_pD3DSRV != nullptr)
        m_pD3DSRV->Release();

    if (m_pD3DStagingTexture != nullptr)
        m_pD3DStagingTexture->Release();

    if (m_pD3DTexture != nullptr)
        m_pD3DTexture->Release();
}

void D3D11GPUTexture3D::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), Max(m_desc.Height >> j, (uint32)1), Max(m_desc.Depth >> j, (uint32)1));

        *gpuMemoryUsage = memoryUsage;
    }
}

void D3D11GPUTexture3D::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DTexture, name);
}

GPUTexture3D *D3D11GPUDevice::CreateTexture3D(const GPU_TEXTURE3D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc,
                                             const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */, const uint32 *pInitialDataSlicePitch /* = NULL */)
{
    HRESULT hResult;

    // validate pixel formats.
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DXGI_FORMAT creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr && creationFormat != DXGI_FORMAT_UNKNOWN);
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT);

    // determine view formats
    DXGI_FORMAT srvFormat, rtvFormat, dsvFormat;
    MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);

    // fill in known fields
    D3D11_TEXTURE3D_DESC D3DTextureDesc;
    D3DTextureDesc.Width = pTextureDesc->Width;
    D3DTextureDesc.Height = pTextureDesc->Height;
    D3DTextureDesc.Depth = pTextureDesc->Depth;
    D3DTextureDesc.MipLevels = pTextureDesc->MipLevels;
    D3DTextureDesc.Format = creationFormat;

    // determine usage
    if (pTextureDesc->Flags & (GPU_TEXTURE_FLAG_READABLE | GPU_TEXTURE_FLAG_WRITABLE | GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER))
    {
        // set to default usage
        D3DTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    }
    else if (pTextureDesc->Flags)
    {
        // otherwise immutable, ensure we have data
        DebugAssert(ppInitialData != nullptr);
        D3DTextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    }

    // bindflags
    D3DTextureDesc.CPUAccessFlags = 0;
    D3DTextureDesc.BindFlags = MapTextureFlagsToD3DBindFlags(pTextureDesc->Flags);
    D3DTextureDesc.MiscFlags = MapTextureFlagsToD3DMiscFlags(pTextureDesc->Flags);

    // initial data
    D3D11_SUBRESOURCE_DATA *pD3DInitialData = nullptr;
    if (ppInitialData != nullptr)
    {
        uint32 nInitializers = pTextureDesc->MipLevels;
        pD3DInitialData = (D3D11_SUBRESOURCE_DATA *)alloca(sizeof(D3D11_SUBRESOURCE_DATA)* nInitializers);
        for (uint32 i = 0; i < nInitializers; i++)
        {
            pD3DInitialData[i].pSysMem = ppInitialData[i];
            pD3DInitialData[i].SysMemPitch = pInitialDataPitch[i];
            pD3DInitialData[i].SysMemSlicePitch = pInitialDataSlicePitch[i];
        }
    }

    // create texture
    ID3D11Texture3D *pD3DTexture;
    hResult = m_pD3DDevice->CreateTexture3D(&D3DTextureDesc, pD3DInitialData, &pD3DTexture);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUDevice::CreateTexture3D: CreateTexture3D failed with hResult %08X", hResult);
        return false;
    }

    // create staging texture if readable texture requested
    ID3D11Texture3D *pD3DStagingTexture = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_READABLE)
    {
        D3D11_TEXTURE3D_DESC D3DStagingTextureDesc;
        D3DStagingTextureDesc.Width = D3DTextureDesc.Width;
        D3DStagingTextureDesc.Height = D3DTextureDesc.Height;
        D3DStagingTextureDesc.Depth = D3DTextureDesc.Depth;
        D3DStagingTextureDesc.MipLevels = 1;
        D3DStagingTextureDesc.Format = D3DTextureDesc.Format;
        D3DStagingTextureDesc.Usage = D3D11_USAGE_STAGING;
        D3DStagingTextureDesc.BindFlags = 0;
        D3DStagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        D3DStagingTextureDesc.MiscFlags = 0;

        hResult = m_pD3DDevice->CreateTexture3D(&D3DStagingTextureDesc, nullptr, &pD3DStagingTexture);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture3D: CreateTexture3D failed for staging texture with hResult %08X", hResult);
            pD3DTexture->Release();
            return false;
        }
    }

    // create shader resource view
    ID3D11ShaderResourceView *pD3DSRV = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        DebugAssert(srvFormat != DXGI_FORMAT_UNKNOWN);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = srvFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MostDetailedMip = 0;
        srvDesc.Texture3D.MipLevels = pTextureDesc->MipLevels;

        hResult = m_pD3DDevice->CreateShaderResourceView(pD3DTexture, &srvDesc, &pD3DSRV);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture3D: CreateShaderResourceView failed with hResult %08X", hResult);
            SAFE_RELEASE(pD3DStagingTexture);
            pD3DTexture->Release();
            return false;
        }
    }

    // create sampler state
    ID3D11SamplerState *pD3DSamplerState = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        DebugAssert(pSamplerStateDesc != nullptr);

        if ((pD3DSamplerState = D3D11Helpers::CreateD3D11SamplerState(m_pD3DDevice, pSamplerStateDesc)) == nullptr)
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTexture3D: Failed to create sampler state for texture.");
            SAFE_RELEASE(pD3DSRV);
            SAFE_RELEASE(pD3DStagingTexture);
            pD3DTexture->Release();
            return false;
        }
    }

    // create class
    return new D3D11GPUTexture3D(pTextureDesc, pD3DTexture, pD3DStagingTexture, pD3DSRV, pD3DSamplerState);
}

bool D3D11GPUContext::ReadTexture(GPUTexture3D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 destinationSlicePitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 startZ, uint32 countX, uint32 countY, uint32 countZ)
{
    HRESULT hResult;
    D3D11GPUTexture3D *pD3DTexture = static_cast<D3D11GPUTexture3D *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_READABLE);
    DebugAssert(countX > 0 && countY > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pD3DTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pD3DTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    uint32 mipDepth = Max(pD3DTexture->GetDesc()->Depth >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight || (startZ + countZ) > mipDepth)
        return false;

    // check destination size
    if ((countZ * destinationSlicePitch) > cbDestination)
        return false;

    // copy the texture to the staging texture
    // if this is a higher mip level, or a non-full-copy, it'll only partially fill the staging texture, this is okay.
    D3D11_BOX sourceBox = { startX, startY, startZ, startX + countX, startY + countY, startZ + countZ };
    m_pD3DContext->CopySubresourceRegion(pD3DTexture->GetD3DStagingTexture(), 0, 0, 0, 0, pD3DTexture->GetD3DTexture(), D3D11CalcSubresource(mipIndex, 0, pD3DTexture->GetDesc()->MipLevels), &sourceBox);

    // map the staging texture
    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
    hResult = m_pD3DContext->Map(pD3DTexture->GetD3DStagingTexture(), 0, D3D11_MAP_READ, 0, &mappedSubResource);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUContext::ReadTexture[Texture3D]: Mapping staging texture failed with hResult %08X", hResult);
        return false;
    }

    // copy line by line
    if (destinationRowPitch != mappedSubResource.RowPitch)
    {
        const byte *pSourcePointer = reinterpret_cast<const byte *>(mappedSubResource.pData);
        byte *pDestinationPointer = reinterpret_cast<byte *>(pDestination);
        for (uint32 i = 0; i < countZ; i++)
        {
            Y_memcpy_stride(pDestinationPointer, destinationRowPitch, pSourcePointer, mappedSubResource.RowPitch, Min(destinationRowPitch, mappedSubResource.RowPitch), countY);
            pSourcePointer += mappedSubResource.DepthPitch;
            pDestinationPointer += destinationSlicePitch;
        }
    }
    else
    {
        Y_memcpy_stride(pDestination, destinationSlicePitch, reinterpret_cast<const byte *>(mappedSubResource.pData), mappedSubResource.DepthPitch, Min(mappedSubResource.DepthPitch, destinationSlicePitch), countZ);
    }
    
    // unmap resource again
    m_pD3DContext->Unmap(pD3DTexture->GetD3DStagingTexture(), 0);
    return true;
}

bool D3D11GPUContext::WriteTexture(GPUTexture3D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 sourceSlicePitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 startZ, uint32 countX, uint32 countY, uint32 countZ)
{
    D3D11GPUTexture3D *pD3DTexture = static_cast<D3D11GPUTexture3D *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(countX > 0 && countY > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pD3DTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pD3DTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    uint32 mipDepth = Max(pD3DTexture->GetDesc()->Depth >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight || (startZ + countZ) > mipDepth)
        return false;

    // find subresource
    uint32 subResourceToUpdate = D3D11CalcSubresource(mipIndex, 0, pD3DTexture->GetDesc()->MipLevels);

    // invoke update
    D3D11_BOX destinationBox = { startX, startY, startZ, startX + countX, startY + countY, startZ + countZ };    
    m_pD3DContext->UpdateSubresource(pD3DTexture->GetD3DTexture(), subResourceToUpdate, &destinationBox, pSource, sourceRowPitch, sourceSlicePitch);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

D3D11GPUTextureCube::D3D11GPUTextureCube(const GPU_TEXTURECUBE_DESC *pDesc,
                                         ID3D11Texture2D *pD3DTexture, ID3D11Texture2D *pD3DStagingTexture,
                                         ID3D11ShaderResourceView *pD3DSRV,
                                         ID3D11SamplerState *pD3DSamplerState)
    : GPUTextureCube(pDesc),
      m_pD3DTexture(pD3DTexture),
      m_pD3DStagingTexture(pD3DStagingTexture),
      m_pD3DSRV(pD3DSRV),
      m_pD3DSamplerState(pD3DSamplerState)
{

}

D3D11GPUTextureCube::~D3D11GPUTextureCube()
{
    if (m_pD3DSamplerState != nullptr)
        m_pD3DSamplerState->Release();

    if (m_pD3DSRV != nullptr)
        m_pD3DSRV->Release();

    if (m_pD3DStagingTexture != nullptr)
        m_pD3DStagingTexture->Release();

    if (m_pD3DTexture != nullptr)
        m_pD3DTexture->Release();
}

void D3D11GPUTextureCube::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), 1, 1);

        *gpuMemoryUsage = memoryUsage * CUBEMAP_FACE_COUNT;
    }
}

void D3D11GPUTextureCube::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DTexture, name);
}

GPUTextureCube *D3D11GPUDevice::CreateTextureCube(const GPU_TEXTURECUBE_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc,
                                                 const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */)
{
    HRESULT hResult;

    // validate pixel formats.
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DXGI_FORMAT creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr && creationFormat != DXGI_FORMAT_UNKNOWN);
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT);

    // determine view formats
    DXGI_FORMAT srvFormat, rtvFormat, dsvFormat;
    MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);

    // fill in known fields
    D3D11_TEXTURE2D_DESC D3DTextureDesc;
    D3DTextureDesc.Width = pTextureDesc->Width;
    D3DTextureDesc.Height = pTextureDesc->Height;
    D3DTextureDesc.MipLevels = pTextureDesc->MipLevels;
    D3DTextureDesc.ArraySize = CUBEMAP_FACE_COUNT;
    D3DTextureDesc.Format = creationFormat;
    D3DTextureDesc.SampleDesc.Count = 1;
    D3DTextureDesc.SampleDesc.Quality = 0;

    // determine usage
    if (pTextureDesc->Flags & (GPU_TEXTURE_FLAG_READABLE | GPU_TEXTURE_FLAG_WRITABLE | GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER))
    {
        // set to default usage
        D3DTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    }
    else if (pTextureDesc->Flags)
    {
        // otherwise immutable, ensure we have data
        DebugAssert(ppInitialData != nullptr);
        D3DTextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    }

    // bindflags
    D3DTextureDesc.CPUAccessFlags = 0;
    D3DTextureDesc.BindFlags = MapTextureFlagsToD3DBindFlags(pTextureDesc->Flags);
    D3DTextureDesc.MiscFlags = MapTextureFlagsToD3DMiscFlags(pTextureDesc->Flags) | D3D11_RESOURCE_MISC_TEXTURECUBE;

    // initial data
    D3D11_SUBRESOURCE_DATA *pD3DInitialData = nullptr;
    if (ppInitialData != nullptr)
    {
        uint32 nInitializers = pTextureDesc->MipLevels * CUBEMAP_FACE_COUNT;
        pD3DInitialData = (D3D11_SUBRESOURCE_DATA *)alloca(sizeof(D3D11_SUBRESOURCE_DATA)* nInitializers);
        for (uint32 i = 0; i < nInitializers; i++)
        {
            pD3DInitialData[i].pSysMem = ppInitialData[i];
            pD3DInitialData[i].SysMemPitch = pInitialDataPitch[i];
            pD3DInitialData[i].SysMemSlicePitch = 0;
        }
    }

    // create texture
    ID3D11Texture2D *pD3DTexture;
    hResult = m_pD3DDevice->CreateTexture2D(&D3DTextureDesc, pD3DInitialData, &pD3DTexture);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUDevice::CreateTextureCube: CreateTexture2D failed with hResult %08X", hResult);
        return false;
    }

    // create staging texture if readable texture requested
    ID3D11Texture2D *pD3DStagingTexture = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_READABLE)
    {
        D3D11_TEXTURE2D_DESC D3DStagingTextureDesc;
        D3DStagingTextureDesc.Width = D3DTextureDesc.Width;
        D3DStagingTextureDesc.Height = D3DTextureDesc.Height;
        D3DStagingTextureDesc.MipLevels = 1;
        D3DStagingTextureDesc.ArraySize = 1;
        D3DStagingTextureDesc.Format = D3DTextureDesc.Format;
        D3DStagingTextureDesc.SampleDesc.Count = D3DTextureDesc.SampleDesc.Count;
        D3DStagingTextureDesc.SampleDesc.Quality = D3DTextureDesc.SampleDesc.Quality;
        D3DStagingTextureDesc.Usage = D3D11_USAGE_STAGING;
        D3DStagingTextureDesc.BindFlags = 0;
        D3DStagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        D3DStagingTextureDesc.MiscFlags = 0;

        hResult = m_pD3DDevice->CreateTexture2D(&D3DStagingTextureDesc, nullptr, &pD3DStagingTexture);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTextureCube: CreateTexture2D failed for staging texture with hResult %08X", hResult);
            pD3DTexture->Release();
            return false;
        }
    }

    // create shader resource view
    ID3D11ShaderResourceView *pD3DSRV = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        DebugAssert(srvFormat != DXGI_FORMAT_UNKNOWN);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = srvFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MostDetailedMip = 0;
        srvDesc.TextureCube.MipLevels = pTextureDesc->MipLevels;

        hResult = m_pD3DDevice->CreateShaderResourceView(pD3DTexture, &srvDesc, &pD3DSRV);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTextureCube: CreateShaderResourceView failed with hResult %08X", hResult);
            SAFE_RELEASE(pD3DStagingTexture);
            pD3DTexture->Release();
            return false;
        }
    }

    // create sampler state
    ID3D11SamplerState *pD3DSamplerState = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        DebugAssert(pSamplerStateDesc != nullptr);

        if ((pD3DSamplerState = D3D11Helpers::CreateD3D11SamplerState(m_pD3DDevice, pSamplerStateDesc)) == nullptr)
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTextureCube: Failed to create sampler state for texture.");
            SAFE_RELEASE(pD3DSRV);
            SAFE_RELEASE(pD3DStagingTexture);
            pD3DTexture->Release();
            return false;
        }
    }

    // create class
    return new D3D11GPUTextureCube(pTextureDesc, pD3DTexture, pD3DStagingTexture, pD3DSRV, pD3DSamplerState);
}

bool D3D11GPUContext::ReadTexture(GPUTextureCube *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    HRESULT hResult;
    D3D11GPUTextureCube *pD3DTexture = static_cast<D3D11GPUTextureCube *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_READABLE);
    DebugAssert(countX > 0 && countY > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pD3DTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pD3DTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight || face >= CUBEMAP_FACE_COUNT)
        return false;

    // check destination size
    if ((countY * destinationRowPitch) > cbDestination)
        return false;

    // copy the texture to the staging texture
    // if this is a higher mip level, or a non-full-copy, it'll only partially fill the staging texture, this is okay.
    D3D11_BOX sourceBox = { startX, startY, 0, startX + countX, startY + countY, 1 };
    m_pD3DContext->CopySubresourceRegion(pD3DTexture->GetD3DStagingTexture(), 0, 0, 0, 0, pD3DTexture->GetD3DTexture(), D3D11CalcSubresource(mipIndex, (uint32)face, pD3DTexture->GetDesc()->MipLevels), &sourceBox);

    // map the staging texture
    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
    hResult = m_pD3DContext->Map(pD3DTexture->GetD3DStagingTexture(), 0, D3D11_MAP_READ, 0, &mappedSubResource);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUContext::ReadTexture[TextureCube]: Mapping staging texture failed with hResult %08X", hResult);
        return false;
    }

    // copy line by line
    Y_memcpy_stride(pDestination, destinationRowPitch, reinterpret_cast<const byte *>(mappedSubResource.pData), mappedSubResource.RowPitch, Min(mappedSubResource.RowPitch, destinationRowPitch), countY);

    // unmap resource again
    m_pD3DContext->Unmap(pD3DTexture->GetD3DStagingTexture(), 0);
    return true;
}

bool D3D11GPUContext::WriteTexture(GPUTextureCube *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    D3D11GPUTextureCube *pD3DTexture = static_cast<D3D11GPUTextureCube *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(countX > 0 && countY > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pD3DTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pD3DTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight || face >= CUBE_FACE_COUNT)
        return false;

    // find subresource
    uint32 subResourceToUpdate = D3D11CalcSubresource(mipIndex, (uint32)face, pD3DTexture->GetDesc()->MipLevels);

    // invoke update
    D3D11_BOX destinationBox = { startX, startY, 0, startX + countX, startY + countY, 1 };
    m_pD3DContext->UpdateSubresource(pD3DTexture->GetD3DTexture(), subResourceToUpdate, &destinationBox, pSource, sourceRowPitch, 0);
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

D3D11GPUTextureCubeArray::D3D11GPUTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pDesc,
                                                   ID3D11Texture2D *pD3DTexture, ID3D11Texture2D *pD3DStagingTexture,
                                                   ID3D11ShaderResourceView *pD3DSRV,
                                                   ID3D11SamplerState *pD3DSamplerState)
    : GPUTextureCubeArray(pDesc),
      m_pD3DTexture(pD3DTexture),
      m_pD3DStagingTexture(pD3DStagingTexture),
      m_pD3DSRV(pD3DSRV),
      m_pD3DSamplerState(pD3DSamplerState)
{

}

D3D11GPUTextureCubeArray::~D3D11GPUTextureCubeArray()
{
    if (m_pD3DSamplerState != nullptr)
        m_pD3DSamplerState->Release();

    if (m_pD3DSRV != nullptr)
        m_pD3DSRV->Release();

    if (m_pD3DStagingTexture != nullptr)
        m_pD3DStagingTexture->Release();

    if (m_pD3DTexture != nullptr)
        m_pD3DTexture->Release();
}

void D3D11GPUTextureCubeArray::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
    {
        uint32 memoryUsage = 0;

        for (uint32 j = 0; j < m_desc.MipLevels; j++)
            memoryUsage += PixelFormat_CalculateImageSize(m_desc.Format, Max(m_desc.Width >> j, (uint32)1), 1, 1);

        *gpuMemoryUsage = memoryUsage * (m_desc.ArraySize * CUBEMAP_FACE_COUNT);
    }
}

void D3D11GPUTextureCubeArray::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DTexture, name);
}

GPUTextureCubeArray *D3D11GPUDevice::CreateTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc,
                                                           const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */)
{
    HRESULT hResult;

    // validate pixel formats.
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DXGI_FORMAT creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr && creationFormat != DXGI_FORMAT_UNKNOWN);
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->MipLevels < TEXTURE_MAX_MIPMAP_COUNT && pTextureDesc->ArraySize > 0 && (pTextureDesc->ArraySize % CUBEMAP_FACE_COUNT) == 0);

    // determine view formats
    DXGI_FORMAT srvFormat, rtvFormat, dsvFormat;
    MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);

    // fill in known fields
    D3D11_TEXTURE2D_DESC D3DTextureDesc;
    D3DTextureDesc.Width = pTextureDesc->Width;
    D3DTextureDesc.Height = pTextureDesc->Height;
    D3DTextureDesc.MipLevels = pTextureDesc->MipLevels;
    D3DTextureDesc.ArraySize = pTextureDesc->ArraySize;
    D3DTextureDesc.Format = creationFormat;
    D3DTextureDesc.SampleDesc.Count = 1;
    D3DTextureDesc.SampleDesc.Quality = 0;

    // determine usage
    if (pTextureDesc->Flags & (GPU_TEXTURE_FLAG_READABLE | GPU_TEXTURE_FLAG_WRITABLE | GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER))
    {
        // set to default usage
        D3DTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    }
    else if (pTextureDesc->Flags)
    {
        // otherwise immutable, ensure we have data
        DebugAssert(ppInitialData != nullptr);
        D3DTextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    }

    // bindflags
    D3DTextureDesc.CPUAccessFlags = 0;
    D3DTextureDesc.BindFlags = MapTextureFlagsToD3DBindFlags(pTextureDesc->Flags);
    D3DTextureDesc.MiscFlags = MapTextureFlagsToD3DMiscFlags(pTextureDesc->Flags) | D3D11_RESOURCE_MISC_TEXTURECUBE;

    // initial data
    D3D11_SUBRESOURCE_DATA *pD3DInitialData = nullptr;
    if (ppInitialData != nullptr)
    {
        uint32 nInitializers = pTextureDesc->MipLevels * (pTextureDesc->ArraySize * CUBEMAP_FACE_COUNT);
        pD3DInitialData = (D3D11_SUBRESOURCE_DATA *)alloca(sizeof(D3D11_SUBRESOURCE_DATA)* nInitializers);
        for (uint32 i = 0; i < nInitializers; i++)
        {
            pD3DInitialData[i].pSysMem = ppInitialData[i];
            pD3DInitialData[i].SysMemPitch = pInitialDataPitch[i];
            pD3DInitialData[i].SysMemSlicePitch = 0;
        }
    }

    // create texture
    ID3D11Texture2D *pD3DTexture;
    hResult = m_pD3DDevice->CreateTexture2D(&D3DTextureDesc, pD3DInitialData, &pD3DTexture);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUDevice::CreateTextureCubeArray: CreateTexture2D failed with hResult %08X", hResult);
        return false;
    }

    // create staging texture if readable texture requested
    ID3D11Texture2D *pD3DStagingTexture = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_READABLE)
    {
        D3D11_TEXTURE2D_DESC D3DStagingTextureDesc;
        D3DStagingTextureDesc.Width = D3DTextureDesc.Width;
        D3DStagingTextureDesc.Height = D3DTextureDesc.Height;
        D3DStagingTextureDesc.MipLevels = 1;
        D3DStagingTextureDesc.ArraySize = 1;
        D3DStagingTextureDesc.Format = D3DTextureDesc.Format;
        D3DStagingTextureDesc.SampleDesc.Count = D3DTextureDesc.SampleDesc.Count;
        D3DStagingTextureDesc.SampleDesc.Quality = D3DTextureDesc.SampleDesc.Quality;
        D3DStagingTextureDesc.Usage = D3D11_USAGE_STAGING;
        D3DStagingTextureDesc.BindFlags = 0;
        D3DStagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        D3DStagingTextureDesc.MiscFlags = 0;

        hResult = m_pD3DDevice->CreateTexture2D(&D3DStagingTextureDesc, nullptr, &pD3DStagingTexture);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTextureCubeArray: CreateTexture2D failed for staging texture with hResult %08X", hResult);
            pD3DTexture->Release();
            return false;
        }
    }

    // create shader resource view
    ID3D11ShaderResourceView *pD3DSRV = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        DebugAssert(srvFormat != DXGI_FORMAT_UNKNOWN);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = srvFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
        srvDesc.TextureCubeArray.MostDetailedMip = 0;
        srvDesc.TextureCubeArray.MipLevels = pTextureDesc->MipLevels;
        srvDesc.TextureCubeArray.First2DArrayFace = 0;
        srvDesc.TextureCubeArray.NumCubes = pTextureDesc->ArraySize / CUBEMAP_FACE_COUNT;

        hResult = m_pD3DDevice->CreateShaderResourceView(pD3DTexture, &srvDesc, &pD3DSRV);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTextureCubeArray: CreateShaderResourceView failed with hResult %08X", hResult);
            SAFE_RELEASE(pD3DStagingTexture);
            pD3DTexture->Release();
            return false;
        }
    }
    
    // create sampler state
    ID3D11SamplerState *pD3DSamplerState = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE)
    {
        DebugAssert(pSamplerStateDesc != nullptr);

        if ((pD3DSamplerState = D3D11Helpers::CreateD3D11SamplerState(m_pD3DDevice, pSamplerStateDesc)) == nullptr)
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateTextureCubeArray: Failed to create sampler state for texture.");
            SAFE_RELEASE(pD3DSRV);
            SAFE_RELEASE(pD3DStagingTexture);
            pD3DTexture->Release();
            return false;
        }
    }

    // create class
    return new D3D11GPUTextureCubeArray(pTextureDesc, pD3DTexture, pD3DStagingTexture, pD3DSRV, pD3DSamplerState);
}

bool D3D11GPUContext::ReadTexture(GPUTextureCubeArray *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 arrayIndex, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    HRESULT hResult;
    D3D11GPUTextureCubeArray *pD3DTexture = static_cast<D3D11GPUTextureCubeArray *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_READABLE);
    DebugAssert(countX > 0 && countY > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pD3DTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pD3DTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight || arrayIndex >= pTexture->GetDesc()->ArraySize || face >= CUBEMAP_FACE_COUNT)
        return false;

    // check destination size
    if ((countY * destinationRowPitch) > cbDestination)
        return false;

    // copy the texture to the staging texture
    // if this is a higher mip level, or a non-full-copy, it'll only partially fill the staging texture, this is okay.
    D3D11_BOX sourceBox = { startX, startY, 0, startX + countX, startY + countY, 1 };
    m_pD3DContext->CopySubresourceRegion(pD3DTexture->GetD3DStagingTexture(), 0, 0, 0, 0, pD3DTexture->GetD3DTexture(), D3D11CalcSubresource(mipIndex, (arrayIndex * CUBEMAP_FACE_COUNT) + (uint32)face, pD3DTexture->GetDesc()->MipLevels), &sourceBox);

    // map the staging texture
    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
    hResult = m_pD3DContext->Map(pD3DTexture->GetD3DStagingTexture(), 0, D3D11_MAP_READ, 0, &mappedSubResource);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUContext::ReadTexture[TextureCubeArray]: Mapping staging texture failed with hResult %08X", hResult);
        return false;
    }

    // copy line by line
    Y_memcpy_stride(pDestination, destinationRowPitch, reinterpret_cast<const byte *>(mappedSubResource.pData), mappedSubResource.RowPitch, Min(mappedSubResource.RowPitch, destinationRowPitch), countY);

    // unmap resource again
    m_pD3DContext->Unmap(pD3DTexture->GetD3DStagingTexture(), 0);
    return true;
}

bool D3D11GPUContext::WriteTexture(GPUTextureCubeArray *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 arrayIndex, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    D3D11GPUTextureCubeArray *pD3DTexture = static_cast<D3D11GPUTextureCubeArray *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(countX > 0 && countY > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    uint32 mipWidth = Max(pD3DTexture->GetDesc()->Width >> mipIndex, (uint32)1);
    uint32 mipHeight = Max(pD3DTexture->GetDesc()->Height >> mipIndex, (uint32)1);
    if ((startX + countX) > mipWidth || (startY + countY) > mipHeight || arrayIndex >= pD3DTexture->GetDesc()->ArraySize || face >= CUBEMAP_FACE_COUNT)
        return false;

    // find subresource
    uint32 subResourceToUpdate = D3D11CalcSubresource(mipIndex, (arrayIndex * CUBEMAP_FACE_COUNT) + (uint32)face, pD3DTexture->GetDesc()->MipLevels);

    // invoke update
    D3D11_BOX destinationBox = { startX, startY, 0, startX + countX, startY + countY, 1 };
    m_pD3DContext->UpdateSubresource(pD3DTexture->GetD3DTexture(), subResourceToUpdate, &destinationBox, pSource, sourceRowPitch, 0);
    return true;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

D3D11GPUDepthTexture::D3D11GPUDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pDesc,
                                           ID3D11Texture2D *pD3DTexture,
                                           ID3D11Texture2D *pD3DStagingTexture)
    : GPUDepthTexture(pDesc),
      m_pD3DTexture(pD3DTexture),
      m_pD3DStagingTexture(pD3DStagingTexture)
{

}

D3D11GPUDepthTexture::~D3D11GPUDepthTexture()
{
    if (m_pD3DStagingTexture != nullptr)
        m_pD3DStagingTexture->Release();
    if (m_pD3DTexture != nullptr)
        m_pD3DTexture->Release();
}

void D3D11GPUDepthTexture::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = PixelFormat_CalculateImageSize(m_desc.Format, m_desc.Width, m_desc.Height, 1);
}

void D3D11GPUDepthTexture::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DTexture, name);
}

GPUDepthTexture *D3D11GPUDevice::CreateDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pTextureDesc)
{
    HRESULT hResult;

    // validate pixel formats.
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pTextureDesc->Format);
    DXGI_FORMAT textureFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(pTextureDesc->Format);
    DebugAssert(pPixelFormatInfo != nullptr && textureFormat != DXGI_FORMAT_UNKNOWN);
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // validate descriptor. we shouldn't be creating invalid textures on the device, so just assert out.
    DebugAssert(pTextureDesc->Width > 0 && pTextureDesc->Height > 0 && pTextureDesc->Flags & GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER);

    // fill in known fields
    D3D11_TEXTURE2D_DESC D3DTextureDesc;
    D3DTextureDesc.Width = pTextureDesc->Width;
    D3DTextureDesc.Height = pTextureDesc->Height;
    D3DTextureDesc.MipLevels = 1;
    D3DTextureDesc.ArraySize = 1;
    D3DTextureDesc.Format = textureFormat;
    D3DTextureDesc.SampleDesc.Count = 1;
    D3DTextureDesc.SampleDesc.Quality = 0;
    D3DTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    D3DTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    D3DTextureDesc.CPUAccessFlags = 0;
    D3DTextureDesc.MiscFlags = 0;

    // create texture
    ID3D11Texture2D *pD3DTexture;
    hResult = m_pD3DDevice->CreateTexture2D(&D3DTextureDesc, nullptr, &pD3DTexture);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUDevice::CreateDepthTexture: CreateTexture2D failed with hResult %08X", hResult);
        return false;
    }

    // create staging texture if readable texture requested
    ID3D11Texture2D *pD3DStagingTexture = nullptr;
    if (pTextureDesc->Flags & GPU_TEXTURE_FLAG_READABLE)
    {
        D3D11_TEXTURE2D_DESC D3DStagingTextureDesc;
        D3DStagingTextureDesc.Width = D3DTextureDesc.Width;
        D3DStagingTextureDesc.Height = D3DTextureDesc.Height;
        D3DStagingTextureDesc.MipLevels = 1;
        D3DStagingTextureDesc.ArraySize = 1;
        D3DStagingTextureDesc.Format = D3DTextureDesc.Format;
        D3DStagingTextureDesc.SampleDesc.Count = D3DTextureDesc.SampleDesc.Count;
        D3DStagingTextureDesc.SampleDesc.Quality = D3DTextureDesc.SampleDesc.Quality;
        D3DStagingTextureDesc.Usage = D3D11_USAGE_STAGING;
        D3DStagingTextureDesc.BindFlags = 0;
        D3DStagingTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        D3DStagingTextureDesc.MiscFlags = 0;

        hResult = m_pD3DDevice->CreateTexture2D(&D3DStagingTextureDesc, nullptr, &pD3DStagingTexture);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11GPUDevice::CreateDepthTexture: CreateTexture2D failed for staging texture with hResult %08X", hResult);
            pD3DTexture->Release();
            return false;
        }
    }
    
    // create class
    return new D3D11GPUDepthTexture(pTextureDesc, pD3DTexture, pD3DStagingTexture);
}

bool D3D11GPUContext::ReadTexture(GPUDepthTexture *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    HRESULT hResult;
    D3D11GPUDepthTexture *pD3DTexture = static_cast<D3D11GPUDepthTexture *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_READABLE);
    DebugAssert(countX > 0 && countY > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    if ((startX + countX) > pD3DTexture->GetDesc()->Width || (startY + countY) > pD3DTexture->GetDesc()->Height)
        return false;

    // check destination size
    if ((countY * destinationRowPitch) > cbDestination)
        return false;

    // copy the texture to the staging texture
    // if this is a higher mip level, or a non-full-copy, it'll only partially fill the staging texture, this is okay.
    D3D11_BOX sourceBox = { startX, startY, 0, startX + countX, startY + countY, 1 };
    m_pD3DContext->CopySubresourceRegion(pD3DTexture->GetD3DStagingTexture(), 0, 0, 0, 0, pD3DTexture->GetD3DTexture(), 0, &sourceBox);

    // map the staging texture
    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
    hResult = m_pD3DContext->Map(pD3DTexture->GetD3DStagingTexture(), 0, D3D11_MAP_READ, 0, &mappedSubResource);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUContext::ReadTexture[DepthTexture]: Mapping staging texture failed with hResult %08X", hResult);
        return false;
    }

    // copy line by line
    Y_memcpy_stride(pDestination, destinationRowPitch, reinterpret_cast<const byte *>(mappedSubResource.pData), mappedSubResource.RowPitch, Min(mappedSubResource.RowPitch, destinationRowPitch), countY);

    // unmap resource again
    m_pD3DContext->Unmap(pD3DTexture->GetD3DStagingTexture(), 0);
    return true;
}

bool D3D11GPUContext::WriteTexture(GPUDepthTexture *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    D3D11GPUDepthTexture *pD3DTexture = static_cast<D3D11GPUDepthTexture *>(pTexture);
    DebugAssert(pD3DTexture->GetDesc()->Flags & GPU_TEXTURE_FLAG_WRITABLE);
    DebugAssert(countX > 0 && countY > 0);

    // get pixel format
    const PIXEL_FORMAT_INFO *pPixelFormatInfo = PixelFormat_GetPixelFormatInfo(pD3DTexture->GetDesc()->Format);
    DebugAssert(!pPixelFormatInfo->IsBlockCompressed && ((pPixelFormatInfo->BitsPerPixel % 8) == 0));
    UNREFERENCED_PARAMETER(pPixelFormatInfo);

    // calculate mip level
    if ((startX + countX) > pD3DTexture->GetDesc()->Width || (startY + countY) > pD3DTexture->GetDesc()->Height)
        return false;

    // invoke update
    D3D11_BOX destinationBox = { startX, startY, 0, startX + countX, startY + countY, 1 };
    m_pD3DContext->UpdateSubresource(pD3DTexture->GetD3DTexture(), 0, &destinationBox, pSource, sourceRowPitch, 0);
    return true;
}

D3D11GPURenderTargetView::D3D11GPURenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc, ID3D11RenderTargetView *pD3DRTV, ID3D11Resource *pD3DResource)
    : GPURenderTargetView(pTexture, pDesc),
      m_pD3DRTV(pD3DRTV),
      m_pD3DResource(pD3DResource)
{

}

D3D11GPURenderTargetView::~D3D11GPURenderTargetView()
{
    m_pD3DRTV->Release();
}

void D3D11GPURenderTargetView::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
    if (gpuMemoryUsage != nullptr)
        gpuMemoryUsage = 0;
}

void D3D11GPURenderTargetView::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DRTV, name);
}

GPURenderTargetView *D3D11GPUDevice::CreateRenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc)
{
    DebugAssert(pTexture != nullptr);

    // we need to convert to d3d formats
    ID3D11Resource *pD3DResource;
    DXGI_FORMAT creationFormat, srvFormat, rtvFormat, dsvFormat;

    // fill in DSV structure
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    switch (pTexture->GetResourceType())
    {
    case GPU_RESOURCE_TYPE_TEXTURE1D:
        pD3DResource = static_cast<D3D11GPUTexture1D *>(pTexture)->GetD3DTexture();
        creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D11GPUTexture1D *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
        rtvDesc.Texture1D.MipSlice = pDesc->MipLevel;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
        pD3DResource = static_cast<D3D11GPUTexture1DArray *>(pTexture)->GetD3DTexture();
        creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D11GPUTexture1DArray *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1DARRAY;
        rtvDesc.Texture1DArray.MipSlice = pDesc->MipLevel;
        rtvDesc.Texture1DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        rtvDesc.Texture1DArray.ArraySize = pDesc->NumLayers;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        pD3DResource = static_cast<D3D11GPUTexture2D *>(pTexture)->GetD3DTexture();
        creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D11GPUTexture2D *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = pDesc->MipLevel;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        pD3DResource = static_cast<D3D11GPUTexture2DArray *>(pTexture)->GetD3DTexture();
        creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D11GPUTexture2DArray *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice = pDesc->MipLevel;
        rtvDesc.Texture2DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        rtvDesc.Texture2DArray.ArraySize = pDesc->NumLayers;
        break;

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        pD3DResource = static_cast<D3D11GPUTextureCube *>(pTexture)->GetD3DTexture();
        creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D11GPUTextureCube *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice = pDesc->MipLevel;
        rtvDesc.Texture2DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        rtvDesc.Texture2DArray.ArraySize = pDesc->NumLayers;
        break;

    case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
        pD3DResource = static_cast<D3D11GPUTextureCubeArray *>(pTexture)->GetD3DTexture();
        creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D11GPUTextureCubeArray *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice = pDesc->MipLevel;
        rtvDesc.Texture2DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        rtvDesc.Texture2DArray.ArraySize = pDesc->NumLayers;
        break;

    case GPU_RESOURCE_TYPE_DEPTH_TEXTURE:
        pD3DResource = static_cast<D3D11GPUDepthTexture *>(pTexture)->GetD3DTexture();
        creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D11GPUDepthTexture *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = 0;
        break;

    default:
        Log_ErrorPrintf("D3D11GPUDevice::CreateRenderTargetView: Invalid resource type %s", NameTable_GetNameString(NameTables::GPUResourceType, pTexture->GetResourceType()));
        return nullptr;
    }

    ID3D11RenderTargetView *pD3DRTV;
    HRESULT hResult = m_pD3DDevice->CreateRenderTargetView(pD3DResource, &rtvDesc, &pD3DRTV);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUDevice::CreateRenderTargetView: CreateRenderTargetView failed with hResult %08X", hResult);
        return nullptr;
    }

    return new D3D11GPURenderTargetView(pTexture, pDesc, pD3DRTV, pD3DResource);
}

D3D11GPUDepthStencilBufferView::D3D11GPUDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc, ID3D11DepthStencilView *pD3DDSV, ID3D11Resource *pD3DResource)
    : GPUDepthStencilBufferView(pTexture, pDesc),
      m_pD3DDSV(pD3DDSV),
      m_pD3DResource(pD3DResource)
{

}

D3D11GPUDepthStencilBufferView::~D3D11GPUDepthStencilBufferView()
{
    m_pD3DDSV->Release();
}

void D3D11GPUDepthStencilBufferView::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
    if (gpuMemoryUsage != nullptr)
        gpuMemoryUsage = 0;
}

void D3D11GPUDepthStencilBufferView::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DDSV, name);
}

GPUDepthStencilBufferView *D3D11GPUDevice::CreateDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc)
{
    DebugAssert(pTexture != nullptr);

    // we need to convert to d3d formats
    ID3D11Resource *pD3DResource;
    DXGI_FORMAT creationFormat, srvFormat, rtvFormat, dsvFormat;

    // fill in DSV structure
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    switch (pTexture->GetResourceType())
    {
    case GPU_RESOURCE_TYPE_TEXTURE1D:
        pD3DResource = static_cast<D3D11GPUTexture1D *>(pTexture)->GetD3DTexture();
        creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D11GPUTexture1D *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
        dsvDesc.Flags = 0;
        dsvDesc.Texture1D.MipSlice = pDesc->MipLevel;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
        pD3DResource = static_cast<D3D11GPUTexture1DArray *>(pTexture)->GetD3DTexture();
        creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D11GPUTexture1DArray *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1DARRAY;
        dsvDesc.Flags = 0;
        dsvDesc.Texture1DArray.MipSlice = pDesc->MipLevel;
        dsvDesc.Texture1DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        dsvDesc.Texture1DArray.ArraySize = pDesc->NumLayers;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        pD3DResource = static_cast<D3D11GPUTexture2D *>(pTexture)->GetD3DTexture();
        creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D11GPUTexture2D *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = 0;
        dsvDesc.Texture2D.MipSlice = pDesc->MipLevel;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        pD3DResource = static_cast<D3D11GPUTexture2DArray *>(pTexture)->GetD3DTexture();
        creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D11GPUTexture2DArray *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Flags = 0;
        dsvDesc.Texture2DArray.MipSlice = pDesc->MipLevel;
        dsvDesc.Texture2DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        dsvDesc.Texture2DArray.ArraySize = pDesc->NumLayers;
        break;

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        pD3DResource = static_cast<D3D11GPUTextureCube *>(pTexture)->GetD3DTexture();
        creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D11GPUTextureCube *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Flags = 0;
        dsvDesc.Texture2DArray.MipSlice = pDesc->MipLevel;
        dsvDesc.Texture2DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        dsvDesc.Texture2DArray.ArraySize = pDesc->NumLayers;
        break;

    case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
        pD3DResource = static_cast<D3D11GPUTextureCubeArray *>(pTexture)->GetD3DTexture();
        creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D11GPUTextureCubeArray *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Flags = 0;
        dsvDesc.Texture2DArray.MipSlice = pDesc->MipLevel;
        dsvDesc.Texture2DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        dsvDesc.Texture2DArray.ArraySize = pDesc->NumLayers;
        break;

    case GPU_RESOURCE_TYPE_DEPTH_TEXTURE:
        pD3DResource = static_cast<D3D11GPUDepthTexture *>(pTexture)->GetD3DTexture();
        creationFormat = D3D11TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D11GPUDepthTexture *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = 0;
        dsvDesc.Texture2D.MipSlice = 0;
        break;

    default:
        Log_ErrorPrintf("D3D11GPUDevice::CreateDepthBufferView: Invalid resource type %s", NameTable_GetNameString(NameTables::GPUResourceType, pTexture->GetResourceType()));
        return nullptr;
    }

    ID3D11DepthStencilView *pD3DDSV;
    HRESULT hResult = m_pD3DDevice->CreateDepthStencilView(pD3DResource, &dsvDesc, &pD3DDSV);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("D3D11GPUDevice::CreateDepthBufferView: CreateDepthStencilView failed with hResult %08X", hResult);
        return nullptr;
    }

    return new D3D11GPUDepthStencilBufferView(pTexture, pDesc, pD3DDSV, pD3DResource);
}

D3D11GPUComputeView::D3D11GPUComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc, ID3D11UnorderedAccessView *pD3DUAV, ID3D11Resource *pD3DResource)
    : GPUComputeView(pResource, pDesc),
      m_pD3DUAV(pD3DUAV),
      m_pD3DResource(pD3DResource)
{

}

D3D11GPUComputeView::~D3D11GPUComputeView()
{
    m_pD3DUAV->Release();
}

void D3D11GPUComputeView::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
    if (gpuMemoryUsage != nullptr)
        gpuMemoryUsage = 0;
}

void D3D11GPUComputeView::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DUAV, name);
}

GPUComputeView *D3D11GPUDevice::CreateComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc)
{
    Panic("Unimplemented");
    return nullptr;
}
