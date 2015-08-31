#pragma once
#include "D3D12Renderer/D3D12Common.h"

namespace D3D12Helpers
{
    DXGI_FORMAT PixelFormatToDXGIFormat(PIXEL_FORMAT Format);
    PIXEL_FORMAT DXGIFormatToPixelFormat(DXGI_FORMAT Format);

    void SetD3D12DeviceChildDebugName(ID3D12DeviceChild *pDeviceChild, const char *debugName);

    bool FillD3D12RasterizerStateDesc(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerState, D3D12_RASTERIZER_DESC *pOutRasterizerDesc);
    bool FillD3D12DepthStencilStateDesc(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilState, D3D12_DEPTH_STENCIL_DESC *pOutDepthStencilDesc);
    bool FillD3D12BlendStateDesc(const RENDERER_BLEND_STATE_DESC *pBlendState, D3D12_BLEND_DESC *pOutBlendDesc);
    bool FillD3D12SamplerStateDesc(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, D3D12_SAMPLER_DESC *pOutSamplerStateDesc);

    bool GetResourceSRVHandle(GPUResource *pResource, D3D12DescriptorHandle *pHandle);
    bool GetResourceSamplerHandle(GPUResource *pResource, D3D12DescriptorHandle *pHandle);
    bool GetOptimizedClearValue(PIXEL_FORMAT format, D3D12_CLEAR_VALUE *pValue);
}
