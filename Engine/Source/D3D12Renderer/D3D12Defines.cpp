#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12Common.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUTexture.h"
Log_SetChannel(D3D12RenderBackend);

// Conflicts with D3D11
// Y_Define_NameTable(NameTables::D3DFeatureLevels)
//     Y_NameTable_VEntry(D3D_FEATURE_LEVEL_9_1,    "D3D_FEATURE_LEVEL_9_1")
//     Y_NameTable_VEntry(D3D_FEATURE_LEVEL_9_2,    "D3D_FEATURE_LEVEL_9_2")
//     Y_NameTable_VEntry(D3D_FEATURE_LEVEL_9_3,    "D3D_FEATURE_LEVEL_9_3")
//     Y_NameTable_VEntry(D3D_FEATURE_LEVEL_10_0,   "D3D_FEATURE_LEVEL_10_0")
//     Y_NameTable_VEntry(D3D_FEATURE_LEVEL_10_1,   "D3D_FEATURE_LEVEL_10_1")
//     Y_NameTable_VEntry(D3D_FEATURE_LEVEL_11_0,   "D3D_FEATURE_LEVEL_11_0")
// Y_NameTable_End()

static const DXGI_FORMAT s_PixelFormatToDXGIFormat[PIXEL_FORMAT_COUNT] =
{
    DXGI_FORMAT_R8_UINT,                // PIXEL_FORMAT_R8_UINT
    DXGI_FORMAT_R8_SINT,                // PIXEL_FORMAT_R8_SINT
    DXGI_FORMAT_R8_UNORM,               // PIXEL_FORMAT_R8_UNORM
    DXGI_FORMAT_R8_SNORM,               // PIXEL_FORMAT_R8_SNORM
    DXGI_FORMAT_R8G8_UINT,              // PIXEL_FORMAT_R8G8_UINT
    DXGI_FORMAT_R8G8_SINT,              // PIXEL_FORMAT_R8G8_SINT
    DXGI_FORMAT_R8G8_UNORM,             // PIXEL_FORMAT_R8G8_UNORM
    DXGI_FORMAT_R8G8_SNORM,             // PIXEL_FORMAT_R8G8_SNORM
    DXGI_FORMAT_R8G8B8A8_UINT,          // PIXEL_FORMAT_R8G8B8A8_UINT
    DXGI_FORMAT_R8G8B8A8_SINT,          // PIXEL_FORMAT_R8G8B8A8_SINT
    DXGI_FORMAT_R8G8B8A8_UNORM,         // PIXEL_FORMAT_R8G8B8A8_UNORM
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,    // PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB
    DXGI_FORMAT_R8G8B8A8_SNORM,         // PIXEL_FORMAT_R8G8B8A8_SNORM
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP,     // PIXEL_FORMAT_R9G9B9E5_SHAREDEXP
    DXGI_FORMAT_R10G10B10A2_UINT,       // PIXEL_FORMAT_R10G10B10A2_UINT
    DXGI_FORMAT_R10G10B10A2_UNORM,      // PIXEL_FORMAT_R10G10B10A2_UNORM
    DXGI_FORMAT_R11G11B10_FLOAT,        // PIXEL_FORMAT_R11G11B10_FLOAT
    DXGI_FORMAT_R16_UINT,               // PIXEL_FORMAT_R16_UINT
    DXGI_FORMAT_R16_SINT,               // PIXEL_FORMAT_R16_SINT
    DXGI_FORMAT_R16_UNORM,              // PIXEL_FORMAT_R16_UNORM
    DXGI_FORMAT_R16_SNORM,              // PIXEL_FORMAT_R16_SNORM
    DXGI_FORMAT_R16_FLOAT,              // PIXEL_FORMAT_R16_FLOAT
    DXGI_FORMAT_R16G16_UINT,            // PIXEL_FORMAT_R16G16_UINT
    DXGI_FORMAT_R16G16_SINT,            // PIXEL_FORMAT_R16G16_SINT
    DXGI_FORMAT_R16G16_UNORM,           // PIXEL_FORMAT_R16G16_UNORM
    DXGI_FORMAT_R16G16_SNORM,           // PIXEL_FORMAT_R16G16_SNORM
    DXGI_FORMAT_R16G16_FLOAT,           // PIXEL_FORMAT_R16G16_FLOAT
    DXGI_FORMAT_R16G16B16A16_UINT,      // PIXEL_FORMAT_R16G16B16A16_UINT
    DXGI_FORMAT_R16G16B16A16_SINT,      // PIXEL_FORMAT_R16G16B16A16_SINT
    DXGI_FORMAT_R16G16B16A16_UNORM,     // PIXEL_FORMAT_R16G16B16A16_UNORM
    DXGI_FORMAT_R16G16B16A16_SNORM,     // PIXEL_FORMAT_R16G16B16A16_SNORM
    DXGI_FORMAT_R16G16B16A16_FLOAT,     // PIXEL_FORMAT_R16G16B16A16_FLOAT
    DXGI_FORMAT_R32_UINT,               // PIXEL_FORMAT_R32_UINT
    DXGI_FORMAT_R32_SINT,               // PIXEL_FORMAT_R32_SINT
    DXGI_FORMAT_R32_FLOAT,              // PIXEL_FORMAT_R32_FLOAT
    DXGI_FORMAT_R32G32_UINT,            // PIXEL_FORMAT_R32G32_UINT
    DXGI_FORMAT_R32G32_SINT,            // PIXEL_FORMAT_R32G32_SINT
    DXGI_FORMAT_R32G32_FLOAT,           // PIXEL_FORMAT_R32G32_FLOAT
    DXGI_FORMAT_R32G32B32_UINT,         // PIXEL_FORMAT_R32G32B32_UINT
    DXGI_FORMAT_R32G32B32_SINT,         // PIXEL_FORMAT_R32G32B32_SINT
    DXGI_FORMAT_R32G32B32_FLOAT,        // PIXEL_FORMAT_R32G32B32_FLOAT
    DXGI_FORMAT_R32G32B32A32_UINT,      // PIXEL_FORMAT_R32G32B32A32_UINT
    DXGI_FORMAT_R32G32B32A32_SINT,      // PIXEL_FORMAT_R32G32B32A32_SINT
    DXGI_FORMAT_R32G32B32A32_FLOAT,     // PIXEL_FORMAT_R32G32B32A32_FLOAT
    DXGI_FORMAT_B8G8R8A8_UNORM,         // PIXEL_FORMAT_B8G8R8A8_UNORM
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,    // PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB
    DXGI_FORMAT_B8G8R8X8_UNORM,         // PIXEL_FORMAT_B8G8R8X8_UNORM
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,    // PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB
    DXGI_FORMAT_B5G6R5_UNORM,           // PIXEL_FORMAT_B5G6R5_UNORM
    DXGI_FORMAT_B5G5R5A1_UNORM,         // PIXEL_FORMAT_B5G5R5A1_UNORM
    DXGI_FORMAT_BC1_UNORM,              // PIXEL_FORMAT_BC1_UNORM
    DXGI_FORMAT_BC1_UNORM_SRGB,         // PIXEL_FORMAT_BC1_UNORM_SRGB
    DXGI_FORMAT_BC2_UNORM,              // PIXEL_FORMAT_BC2_UNORM
    DXGI_FORMAT_BC2_UNORM_SRGB,         // PIXEL_FORMAT_BC2_UNORM_SRGB
    DXGI_FORMAT_BC3_UNORM,              // PIXEL_FORMAT_BC3_UNORM
    DXGI_FORMAT_BC3_UNORM_SRGB,         // PIXEL_FORMAT_BC3_UNORM_SRGB
    DXGI_FORMAT_BC4_UNORM,              // PIXEL_FORMAT_BC4_UNORM
    DXGI_FORMAT_BC4_SNORM,              // PIXEL_FORMAT_BC4_SNORM
    DXGI_FORMAT_BC5_UNORM,              // PIXEL_FORMAT_BC5_UNORM
    DXGI_FORMAT_BC5_SNORM,              // PIXEL_FORMAT_BC5_SNORM
    DXGI_FORMAT_BC6H_UF16,              // PIXEL_FORMAT_BC6H_UF16
    DXGI_FORMAT_BC6H_SF16,              // PIXEL_FORMAT_BC6H_SF16
    DXGI_FORMAT_BC7_UNORM,              // PIXEL_FORMAT_BC7_UNORM
    DXGI_FORMAT_BC7_UNORM_SRGB,         // PIXEL_FORMAT_BC7_UNORM_SRGB
    DXGI_FORMAT_D16_UNORM,              // PIXEL_FORMAT_D16_UNORM
    DXGI_FORMAT_D24_UNORM_S8_UINT,      // PIXEL_FORMAT_D24_UNORM_S8_UINT
    DXGI_FORMAT_D32_FLOAT,              // PIXEL_FORMAT_D32_FLOAT
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT,   // PIXEL_FORMAT_D32_FLOAT_S8X24_UINT
    DXGI_FORMAT_UNKNOWN,                // PIXEL_FORMAT_R8G8B8_UNORM
    DXGI_FORMAT_UNKNOWN,                // PIXEL_FORMAT_B8G8R8_UNORM
};

DXGI_FORMAT D3D12TypeConversion::PixelFormatToDXGIFormat(PIXEL_FORMAT Format)
{
    return (Format < PIXEL_FORMAT_COUNT) ? s_PixelFormatToDXGIFormat[Format] : DXGI_FORMAT_UNKNOWN;
}

PIXEL_FORMAT D3D12TypeConversion::DXGIFormatToPixelFormat(DXGI_FORMAT Format)
{
    uint32 i;
    for (i = 0; i < PIXEL_FORMAT_COUNT; i++)
    {
        if (s_PixelFormatToDXGIFormat[i] == Format)
            return (PIXEL_FORMAT)i;
    }

    return PIXEL_FORMAT_UNKNOWN;
}

// ID3D11ShaderResourceView *D3D12Helpers::GetResourceShaderResourceView(GPUResource *pResource)
// {
//     if (pResource == nullptr)
//         return nullptr;
// 
//     switch (pResource->GetResourceType())
//     {
//     case GPU_RESOURCE_TYPE_TEXTURE1D:
//         return static_cast<D3D11GPUTexture1D *>(pResource)->GetD3DSRV();
// 
//     case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
//         return static_cast<D3D11GPUTexture1DArray *>(pResource)->GetD3DSRV();
// 
//     case GPU_RESOURCE_TYPE_TEXTURE2D:
//         return static_cast<D3D11GPUTexture2D *>(pResource)->GetD3DSRV();
// 
//     case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
//         return static_cast<D3D11GPUTexture2DArray *>(pResource)->GetD3DSRV();
// 
//     case GPU_RESOURCE_TYPE_TEXTURE3D:
//         return static_cast<D3D11GPUTexture3D *>(pResource)->GetD3DSRV();
// 
//     case GPU_RESOURCE_TYPE_TEXTURECUBE:
//         return static_cast<D3D11GPUTextureCube *>(pResource)->GetD3DSRV();
// 
//     case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
//         return static_cast<D3D11GPUTextureCubeArray *>(pResource)->GetD3DSRV();
//     }
// 
//     return nullptr;
// }
// 
// ID3D11SamplerState *D3D12Helpers::GetResourceSamplerState(GPUResource *pResource)
// {
//     if (pResource == nullptr)
//         return nullptr;
// 
//     switch (pResource->GetResourceType())
//     {
//     case GPU_RESOURCE_TYPE_SAMPLER_STATE:
//         return static_cast<D3D12SamplerState *>(pResource)->GetD3DSamplerState();
// 
//     case GPU_RESOURCE_TYPE_TEXTURE1D:
//         return static_cast<D3D11GPUTexture1D *>(pResource)->GetD3DSamplerState();
// 
//     case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
//         return static_cast<D3D11GPUTexture1DArray *>(pResource)->GetD3DSamplerState();
// 
//     case GPU_RESOURCE_TYPE_TEXTURE2D:
//         return static_cast<D3D11GPUTexture2D *>(pResource)->GetD3DSamplerState();
// 
//     case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
//         return static_cast<D3D11GPUTexture2DArray *>(pResource)->GetD3DSamplerState();
// 
//     case GPU_RESOURCE_TYPE_TEXTURE3D:
//         return static_cast<D3D11GPUTexture3D *>(pResource)->GetD3DSamplerState();
// 
//     case GPU_RESOURCE_TYPE_TEXTURECUBE:
//         return static_cast<D3D11GPUTextureCube *>(pResource)->GetD3DSamplerState();
// 
//     case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
//         return static_cast<D3D11GPUTextureCubeArray *>(pResource)->GetD3DSamplerState();
//     }
// 
//     return nullptr;
// }

void D3D12Helpers::SetD3D12DeviceChildDebugName(ID3D12DeviceChild *pDeviceChild, const char *debugName)
{
#ifdef Y_BUILD_CONFIG_DEBUG
    uint32 nameLength = Y_strlen(debugName);
    if (nameLength > 0)
        pDeviceChild->SetPrivateData(WKPDID_D3DDebugObjectName, nameLength, debugName);
#endif
}

