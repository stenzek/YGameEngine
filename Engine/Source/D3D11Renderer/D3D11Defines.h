#pragma once

namespace NameTables {
    Y_Declare_NameTable(D3DFeatureLevels);
}

namespace D3D11TypeConversion
{
    const char *D3DFeatureLevelToString(D3D_FEATURE_LEVEL FeatureLevel);
    DXGI_FORMAT PixelFormatToDXGIFormat(PIXEL_FORMAT Format);
    PIXEL_FORMAT DXGIFormatToPixelFormat(DXGI_FORMAT Format);
    D3D11_MAP MapTypetoD3D11MapType(GPU_MAP_TYPE MapType);

    const char *VertexElementSemanticToString(GPU_VERTEX_ELEMENT_SEMANTIC semantic);
    const char *VertexElementTypeToShaderTypeString(GPU_VERTEX_ELEMENT_TYPE type);
    DXGI_FORMAT VertexElementTypeToDXGIFormat(GPU_VERTEX_ELEMENT_TYPE type);
}

namespace D3D11Helpers
{
    ID3D11RasterizerState *CreateD3D11RasterizerState(ID3D11Device *pD3DDevice, const RENDERER_RASTERIZER_STATE_DESC *pRasterizerState);
    ID3D11DepthStencilState *CreateD3D11DepthStencilState(ID3D11Device *pD3DDevice, const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilState);
    ID3D11BlendState *CreateD3D11BlendState(ID3D11Device *pD3DDevice, const RENDERER_BLEND_STATE_DESC *pBlendState);
    ID3D11SamplerState *CreateD3D11SamplerState(ID3D11Device *pD3DDevice, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc);
    ID3D11ShaderResourceView *GetResourceShaderResourceView(GPUResource *pResource);
    ID3D11SamplerState *GetResourceSamplerState(GPUResource *pResource);
    void CorrectProjectionMatrix(float4x4 &projectionMatrix);
    void SetD3D11DeviceChildDebugName(ID3D11DeviceChild *pDeviceChild, const char *debugName);
}

