#pragma once

namespace NameTables {
    Y_Declare_NameTable(D3DFeatureLevels);
}

namespace D3D12TypeConversion
{
    const char *D3DFeatureLevelToString(D3D_FEATURE_LEVEL FeatureLevel);
    DXGI_FORMAT PixelFormatToDXGIFormat(PIXEL_FORMAT Format);
    PIXEL_FORMAT DXGIFormatToPixelFormat(DXGI_FORMAT Format);
    D3D11_MAP MapTypetoD3D11MapType(GPU_MAP_TYPE MapType);

    const char *VertexElementSemanticToString(GPU_VERTEX_ELEMENT_SEMANTIC semantic);
    const char *VertexElementTypeToShaderTypeString(GPU_VERTEX_ELEMENT_TYPE type);
    DXGI_FORMAT VertexElementTypeToDXGIFormat(GPU_VERTEX_ELEMENT_TYPE type);
}

namespace D3D12Helpers
{
    bool CreateD3D11RasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerState, D3D12_RASTERIZER_DESC *pOutRasterizerDesc);
    bool CreateD3D11DepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilState, D3D12_DEPTH_STENCIL_DESC *pOutDepthStencilDesc);
    bool CreateD3D11BlendState(const RENDERER_BLEND_STATE_DESC *pBlendState, D3D12_BLEND_DESC *pOutBlendDesc);
    bool CreateD3D11SamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, D3D12_SAMPLER_DESC *pOutSamplerStateDesc);
    //ID3D11ShaderResourceView *GetResourceShaderResourceView(GPUResource *pResource);
    //ID3D11SamplerState *GetResourceSamplerState(GPUResource *pResource);
    void CorrectProjectionMatrix(float4x4 &projectionMatrix);
    void SetD3D12DeviceChildDebugName(ID3D12DeviceChild *pDeviceChild, const char *debugName);
}

