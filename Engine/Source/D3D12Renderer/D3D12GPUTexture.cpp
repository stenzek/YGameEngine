#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12GPUTexture.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUContext.h"
#include "D3D12Renderer/D3D12RenderBackend.h"
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

GPUTexture2D *D3D12GPUDevice::CreateTexture2D(const GPU_TEXTURE2D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/)
{
    return nullptr;
}

bool D3D12GPUContext::ReadTexture(GPUTexture2D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    return false;
}

bool D3D12GPUContext::WriteTexture(GPUTexture2D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    return false;
}

GPUTexture2DArray *D3D12GPUDevice::CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/)
{
    return nullptr;
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

D3D12GPURenderTargetView::D3D12GPURenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc, const D3D12DescriptorHeap::Handle &descriptorHandle, ID3D12Resource *pD3DResource)
    : GPURenderTargetView(pTexture, pDesc)
    , m_descriptorHandle(descriptorHandle)
    , m_pD3DResource(pD3DResource)
{

}

D3D12GPURenderTargetView::~D3D12GPURenderTargetView()
{
    D3D12RenderBackend::GetInstance()->ScheduleDescriptorForDeletion(&m_descriptorHandle);
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

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        pD3DResource = static_cast<D3D12GPUTexture2D *>(pTexture)->GetD3DTexture();
        creationFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUTexture2D *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = pDesc->MipLevel;
        rtvDesc.Texture2D.PlaneSlice = 0;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        pD3DResource = static_cast<D3D12GPUTexture2DArray *>(pTexture)->GetD3DTexture();
        creationFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUTexture2DArray *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        rtvDesc.Format = rtvFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice = pDesc->MipLevel;
        rtvDesc.Texture2DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        rtvDesc.Texture2DArray.ArraySize = pDesc->NumLayers;
        rtvDesc.Texture2DArray.PlaneSlice = 0;
        break;

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
    D3D12DescriptorHeap::Handle handle;
    if (!m_pBackend->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->Allocate(&handle))
    {
        Log_ErrorPrintf("D3D12GPUDevice::CreateRenderTargetView: Failed to allocate descriptor.");
        return nullptr;
    }

    // create RTV
    m_pD3DDevice->CreateRenderTargetView(pD3DResource, &rtvDesc, handle);
    return new D3D12GPURenderTargetView(pTexture, pDesc, handle, pD3DResource);
}

D3D12GPUDepthStencilBufferView::D3D12GPUDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc, const D3D12DescriptorHeap::Handle &descriptorHandle, ID3D12Resource *pD3DResource)
    : GPUDepthStencilBufferView(pTexture, pDesc)
    , m_descriptorHandle(descriptorHandle)
    , m_pD3DResource(pD3DResource)
{

}

D3D12GPUDepthStencilBufferView::~D3D12GPUDepthStencilBufferView()
{
    D3D12RenderBackend::GetInstance()->ScheduleDescriptorForDeletion(&m_descriptorHandle);
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

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        pD3DResource = static_cast<D3D12GPUTexture2D *>(pTexture)->GetD3DTexture();
        creationFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUTexture2D *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = 0;
        dsvDesc.Texture2D.MipSlice = pDesc->MipLevel;
        break;

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        pD3DResource = static_cast<D3D12GPUTexture2DArray *>(pTexture)->GetD3DTexture();
        creationFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUTexture2DArray *>(pTexture)->GetDesc()->Format);
        MapTextureFormatToViewFormat(&creationFormat, &srvFormat, &rtvFormat, &dsvFormat);
        dsvDesc.Format = dsvFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Flags = 0;
        dsvDesc.Texture2DArray.MipSlice = pDesc->MipLevel;
        dsvDesc.Texture2DArray.FirstArraySlice = pDesc->FirstLayerIndex;
        dsvDesc.Texture2DArray.ArraySize = pDesc->NumLayers;
        break;

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
    D3D12DescriptorHeap::Handle handle;
    if (!m_pBackend->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)->Allocate(&handle))
    {
        Log_ErrorPrintf("D3D12GPUDevice::CreateRenderTargetView: Failed to allocate descriptor.");
        return nullptr;
    }

    // create RTV
    m_pD3DDevice->CreateDepthStencilView(pD3DResource, &dsvDesc, handle);
    return new D3D12GPUDepthStencilBufferView(pTexture, pDesc, handle, pD3DResource);
}

D3D12GPUComputeView::D3D12GPUComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc, const D3D12DescriptorHeap::Handle &descriptorHandle, ID3D12Resource *pD3DResource)
    : GPUComputeView(pResource, pDesc)
    , m_descriptorHandle(descriptorHandle)
    , m_pD3DResource(pD3DResource)
{

}

D3D12GPUComputeView::~D3D12GPUComputeView()
{
    D3D12RenderBackend::GetInstance()->ScheduleDescriptorForDeletion(&m_descriptorHandle);
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
