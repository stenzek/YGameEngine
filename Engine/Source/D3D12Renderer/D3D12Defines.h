#pragma once

namespace NameTables {
    Y_Declare_NameTable(D3DFeatureLevels);
}

namespace D3D12TypeConversion
{
    DXGI_FORMAT PixelFormatToDXGIFormat(PIXEL_FORMAT Format);
    PIXEL_FORMAT DXGIFormatToPixelFormat(DXGI_FORMAT Format);
}

namespace D3D12Helpers
{
    bool FillD3D12RasterizerStateDesc(const RENDERER_RASTERIZER_STATE_DESC * pRasterizerState, D3D12_RASTERIZER_DESC * pOutRasterizerDesc);
    bool FillD3D12DepthStencilStateDesc(const RENDERER_DEPTHSTENCIL_STATE_DESC * pDepthStencilState, D3D12_DEPTH_STENCIL_DESC * pOutDepthStencilDesc);
    bool FillD3D12BlendStateDesc(const RENDERER_BLEND_STATE_DESC * pBlendState, D3D12_BLEND_DESC * pOutBlendDesc);
    bool FillD3D12SamplerStateDesc(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, D3D12_SAMPLER_DESC *pOutSamplerStateDesc);
    //ID3D11ShaderResourceView *GetResourceShaderResourceView(GPUResource *pResource);
    //ID3D11SamplerState *GetResourceSamplerState(GPUResource *pResource);
    void SetD3D12DeviceChildDebugName(ID3D12DeviceChild *pDeviceChild, const char *debugName);
}

#define D3D12_CONSTANT_BUFFER_ALIGNMENT (65536)         // 64KiB
