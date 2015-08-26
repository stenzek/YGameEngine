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
    //ID3D11ShaderResourceView *GetResourceShaderResourceView(GPUResource *pResource);
    //ID3D11SamplerState *GetResourceSamplerState(GPUResource *pResource);
    void SetD3D12DeviceChildDebugName(ID3D12DeviceChild *pDeviceChild, const char *debugName);
}

#define D3D12_CONSTANT_BUFFER_ALIGNMENT (65536)         // 64KiB
