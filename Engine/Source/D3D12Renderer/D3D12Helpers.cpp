#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12Common.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUTexture.h"
#include "D3D12Renderer/D3D12Helpers.h"
Log_SetChannel(D3D12RenderBackend);

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

static const D3D12_COMPARISON_FUNC s_D3D12ComparisonFuncs[GPU_COMPARISON_FUNC_COUNT] =
{
    D3D12_COMPARISON_FUNC_NEVER,             // RENDERER_COMPARISON_FUNC_NEVER
    D3D12_COMPARISON_FUNC_LESS,              // RENDERER_COMPARISON_FUNC_LESS
    D3D12_COMPARISON_FUNC_EQUAL,             // RENDERER_COMPARISON_FUNC_EQUAL
    D3D12_COMPARISON_FUNC_LESS_EQUAL,        // RENDERER_COMPARISON_FUNC_LESS_EQUAL
    D3D12_COMPARISON_FUNC_GREATER,           // RENDERER_COMPARISON_FUNC_GREATER
    D3D12_COMPARISON_FUNC_NOT_EQUAL,         // RENDERER_COMPARISON_FUNC_NOT_EQUAL
    D3D12_COMPARISON_FUNC_GREATER_EQUAL,     // RENDERER_COMPARISON_FUNC_GREATER_EQUAL
    D3D12_COMPARISON_FUNC_ALWAYS,            // RENDERER_COMPARISON_FUNC_ALWAYS
};

DXGI_FORMAT D3D12Helpers::PixelFormatToDXGIFormat(PIXEL_FORMAT Format)
{
    return (Format < PIXEL_FORMAT_COUNT) ? s_PixelFormatToDXGIFormat[Format] : DXGI_FORMAT_UNKNOWN;
}

PIXEL_FORMAT D3D12Helpers::DXGIFormatToPixelFormat(DXGI_FORMAT Format)
{
    uint32 i;
    for (i = 0; i < PIXEL_FORMAT_COUNT; i++)
    {
        if (s_PixelFormatToDXGIFormat[i] == Format)
            return (PIXEL_FORMAT)i;
    }

    return PIXEL_FORMAT_UNKNOWN;
}

void D3D12Helpers::SetD3D12ObjectName(ID3D12Object *pObject, const char *debugName)
{
#ifdef Y_BUILD_CONFIG_DEBUG
    uint32 nameLength = Y_strlen(debugName);
    if (nameLength > 0)
    {
        wchar_t *buffer = (wchar_t *)alloca(sizeof(wchar_t) * nameLength + sizeof(wchar_t));
        mbstowcs(buffer, debugName, nameLength);
        buffer[nameLength] = 0;
        pObject->SetName(buffer);
    }
#endif
}

bool D3D12Helpers::FillD3D12RasterizerStateDesc(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerState, D3D12_RASTERIZER_DESC *pOutRasterizerDesc)
{
    static const D3D12_FILL_MODE D3D11FillModes[RENDERER_FILL_MODE_COUNT] =
    {
        D3D12_FILL_MODE_WIREFRAME,               // RENDERER_FILL_WIREFRAME
        D3D12_FILL_MODE_SOLID,                   // RENDERER_FILL_SOLID
    };

    static const D3D12_CULL_MODE D3D11CullModes[RENDERER_CULL_MODE_COUNT] =
    {
        D3D12_CULL_MODE_NONE,                    // RENDERER_CULL_NONE
        D3D12_CULL_MODE_FRONT,                   // RENDERER_CULL_FRONT
        D3D12_CULL_MODE_BACK,                    // RENDERER_CULL_BACK
    };

    DebugAssert(pRasterizerState->FillMode < RENDERER_FILL_MODE_COUNT);
    DebugAssert(pRasterizerState->CullMode < RENDERER_CULL_MODE_COUNT);
    
    pOutRasterizerDesc->FillMode = D3D11FillModes[pRasterizerState->FillMode];
    pOutRasterizerDesc->CullMode = D3D11CullModes[pRasterizerState->CullMode];
    pOutRasterizerDesc->FrontCounterClockwise = pRasterizerState->FrontCounterClockwise ? TRUE : FALSE;
    pOutRasterizerDesc->DepthBias = pRasterizerState->DepthBias;
    pOutRasterizerDesc->DepthBiasClamp = 0;
    pOutRasterizerDesc->SlopeScaledDepthBias = pRasterizerState->SlopeScaledDepthBias;
    pOutRasterizerDesc->DepthClipEnable = pRasterizerState->DepthClipEnable ? TRUE : FALSE;
    //pOutRasterizerDesc->ScissorEnable = pRasterizerState->ScissorEnable ? TRUE : FALSE;
    pOutRasterizerDesc->MultisampleEnable = FALSE;
    pOutRasterizerDesc->AntialiasedLineEnable = FALSE;
    pOutRasterizerDesc->ForcedSampleCount = 0;
    pOutRasterizerDesc->ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    return true;
}

bool D3D12Helpers::FillD3D12DepthStencilStateDesc(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilState, D3D12_DEPTH_STENCIL_DESC *pOutDepthStencilDesc)
{
    static const D3D12_STENCIL_OP D3D11StencilOps[RENDERER_STENCIL_OP_COUNT] =
    {
        D3D12_STENCIL_OP_KEEP,              // RENDERER_STENCIL_OP_KEEP
        D3D12_STENCIL_OP_ZERO,              // RENDERER_STENCIL_OP_ZERO
        D3D12_STENCIL_OP_REPLACE,           // RENDERER_STENCIL_OP_REPLACE
        D3D12_STENCIL_OP_INCR_SAT,          // RENDERER_STENCIL_OP_INCREMENT_CLAMPED
        D3D12_STENCIL_OP_DECR_SAT,          // RENDERER_STENCIL_OP_DECREMENT_CLAMPED
        D3D12_STENCIL_OP_INVERT,            // RENDERER_STENCIL_OP_INVERT
        D3D12_STENCIL_OP_INCR,              // RENDERER_STENCIL_OP_INCREMENT
        D3D12_STENCIL_OP_DECR,              // RENDERER_STENCIL_OP_DECREMENT
    };

    DebugAssert(pDepthStencilState->DepthFunc < GPU_COMPARISON_FUNC_COUNT);

    pOutDepthStencilDesc->DepthEnable = pDepthStencilState->DepthTestEnable ? TRUE : FALSE;
    pOutDepthStencilDesc->DepthWriteMask = pDepthStencilState->DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    pOutDepthStencilDesc->DepthFunc = s_D3D12ComparisonFuncs[pDepthStencilState->DepthFunc];
    
    DebugAssert(pDepthStencilState->StencilFrontFace.FailOp < RENDERER_STENCIL_OP_COUNT);
    DebugAssert(pDepthStencilState->StencilFrontFace.DepthFailOp < RENDERER_STENCIL_OP_COUNT);
    DebugAssert(pDepthStencilState->StencilFrontFace.PassOp < RENDERER_STENCIL_OP_COUNT);
    DebugAssert(pDepthStencilState->StencilFrontFace.CompareFunc < GPU_COMPARISON_FUNC_COUNT);
    DebugAssert(pDepthStencilState->StencilBackFace.FailOp < RENDERER_STENCIL_OP_COUNT);
    DebugAssert(pDepthStencilState->StencilBackFace.DepthFailOp < RENDERER_STENCIL_OP_COUNT);
    DebugAssert(pDepthStencilState->StencilBackFace.PassOp < RENDERER_STENCIL_OP_COUNT);
    DebugAssert(pDepthStencilState->StencilBackFace.CompareFunc < GPU_COMPARISON_FUNC_COUNT);

    pOutDepthStencilDesc->StencilEnable = pDepthStencilState->StencilTestEnable ? TRUE : FALSE;
    pOutDepthStencilDesc->StencilReadMask = pDepthStencilState->StencilReadMask;
    pOutDepthStencilDesc->StencilWriteMask = pDepthStencilState->StencilWriteMask;
    pOutDepthStencilDesc->FrontFace.StencilFailOp = D3D11StencilOps[pDepthStencilState->StencilFrontFace.FailOp];
    pOutDepthStencilDesc->FrontFace.StencilDepthFailOp = D3D11StencilOps[pDepthStencilState->StencilFrontFace.DepthFailOp];
    pOutDepthStencilDesc->FrontFace.StencilPassOp = D3D11StencilOps[pDepthStencilState->StencilFrontFace.PassOp];
    pOutDepthStencilDesc->FrontFace.StencilFunc = s_D3D12ComparisonFuncs[pDepthStencilState->StencilFrontFace.CompareFunc];
    pOutDepthStencilDesc->BackFace.StencilFailOp = D3D11StencilOps[pDepthStencilState->StencilBackFace.FailOp];
    pOutDepthStencilDesc->BackFace.StencilDepthFailOp = D3D11StencilOps[pDepthStencilState->StencilBackFace.DepthFailOp];
    pOutDepthStencilDesc->BackFace.StencilPassOp = D3D11StencilOps[pDepthStencilState->StencilBackFace.PassOp];
    pOutDepthStencilDesc->BackFace.StencilFunc = s_D3D12ComparisonFuncs[pDepthStencilState->StencilBackFace.CompareFunc];
    return true;
}

bool D3D12Helpers::FillD3D12BlendStateDesc(const RENDERER_BLEND_STATE_DESC *pBlendState, D3D12_BLEND_DESC *pOutBlendDesc)
{
    static const D3D12_BLEND D3D12BlendOptions[RENDERER_BLEND_OPTION_COUNT] = 
    {
        D3D12_BLEND_ZERO,                   // RENDERER_BLEND_ZERO
        D3D12_BLEND_ONE,                    // RENDERER_BLEND_ONE
        D3D12_BLEND_SRC_COLOR,              // RENDERER_BLEND_SRC_COLOR
        D3D12_BLEND_INV_SRC_COLOR,          // RENDERER_BLEND_INV_SRC_COLOR
        D3D12_BLEND_SRC_ALPHA,              // RENDERER_BLEND_SRC_ALPHA
        D3D12_BLEND_INV_SRC_ALPHA,          // RENDERER_BLEND_INV_SRC_ALPHA
        D3D12_BLEND_DEST_ALPHA,             // RENDERER_BLEND_DEST_ALPHA
        D3D12_BLEND_INV_DEST_ALPHA,         // RENDERER_BLEND_INV_DEST_ALPHA
        D3D12_BLEND_DEST_COLOR,             // RENDERER_BLEND_DEST_COLOR
        D3D12_BLEND_INV_DEST_COLOR,         // RENDERER_BLEND_INV_DEST_COLOR
        D3D12_BLEND_SRC_ALPHA_SAT,          // RENDERER_BLEND_SRC_ALPHA_SAT
        D3D12_BLEND_BLEND_FACTOR,           // RENDERER_BLEND_BLEND_FACTOR
        D3D12_BLEND_INV_BLEND_FACTOR,       // RENDERER_BLEND_INV_BLEND_FACTOR
        D3D12_BLEND_SRC1_COLOR,             // RENDERER_BLEND_SRC1_COLOR
        D3D12_BLEND_INV_SRC1_COLOR,         // RENDERER_BLEND_INV_SRC1_COLOR
        D3D12_BLEND_SRC1_ALPHA,             // RENDERER_BLEND_SRC1_ALPHA
        D3D12_BLEND_INV_SRC1_ALPHA,         // RENDERER_BLEND_INV_SRC1_ALPHA
    };

    static const D3D12_BLEND_OP D3D12BlendOps[RENDERER_BLEND_OP_COUNT] =
    {
        D3D12_BLEND_OP_ADD,                 // RENDERER_BLEND_OP_ADD
        D3D12_BLEND_OP_SUBTRACT,            // RENDERER_BLEND_OP_SUBTRACT
        D3D12_BLEND_OP_REV_SUBTRACT,        // RENDERER_BLEND_OP_REV_SUBTRACT
        D3D12_BLEND_OP_MIN,                 // RENDERER_BLEND_OP_MIN
        D3D12_BLEND_OP_MAX,                 // RENDERER_BLEND_OP_MAX
    };

    DebugAssert(pBlendState->SrcBlend < RENDERER_BLEND_OPTION_COUNT);
    DebugAssert(pBlendState->BlendOp < RENDERER_BLEND_OP_COUNT);
    DebugAssert(pBlendState->DestBlend < RENDERER_BLEND_OPTION_COUNT);
    DebugAssert(pBlendState->SrcBlendAlpha < RENDERER_BLEND_OPTION_COUNT);
    DebugAssert(pBlendState->BlendOpAlpha < RENDERER_BLEND_OP_COUNT);
    DebugAssert(pBlendState->DestBlendAlpha < RENDERER_BLEND_OPTION_COUNT);

    pOutBlendDesc->AlphaToCoverageEnable = FALSE;
    pOutBlendDesc->IndependentBlendEnable = FALSE;

    for (uint32 i = 0; i < 8; i++)
    {
        pOutBlendDesc->RenderTarget[i].BlendEnable = pBlendState->BlendEnable ? TRUE : FALSE;
        pOutBlendDesc->RenderTarget[i].LogicOpEnable = FALSE;
        pOutBlendDesc->RenderTarget[i].SrcBlend = D3D12BlendOptions[pBlendState->SrcBlend];
        pOutBlendDesc->RenderTarget[i].BlendOp = D3D12BlendOps[pBlendState->BlendOp];
        pOutBlendDesc->RenderTarget[i].DestBlend = D3D12BlendOptions[pBlendState->DestBlend];
        pOutBlendDesc->RenderTarget[i].SrcBlendAlpha = D3D12BlendOptions[pBlendState->SrcBlendAlpha];
        pOutBlendDesc->RenderTarget[i].BlendOpAlpha = D3D12BlendOps[pBlendState->BlendOpAlpha];
        pOutBlendDesc->RenderTarget[i].DestBlendAlpha = D3D12BlendOptions[pBlendState->DestBlendAlpha];
        pOutBlendDesc->RenderTarget[i].LogicOp = D3D12_LOGIC_OP_CLEAR;
        pOutBlendDesc->RenderTarget[i].RenderTargetWriteMask = pBlendState->ColorWriteEnable ? D3D12_COLOR_WRITE_ENABLE_ALL : 0;
    }

    return true;
}

bool D3D12Helpers::FillD3D12SamplerStateDesc(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, D3D12_SAMPLER_DESC *pOutSamplerStateDesc)
{
    static const D3D12_FILTER D3D12TextureFilters[TEXTURE_FILTER_COUNT] =
    {
        D3D12_FILTER_MIN_MAG_MIP_POINT,                             // RENDERER_TEXTURE_FILTER_MIN_MAG_MIP_POINT
        D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR,                      // RENDERER_TEXTURE_FILTER_MIN_MAG_POINT_MIP_LINEAR
        D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,                // RENDERER_TEXTURE_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT
        D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR,                      // RENDERER_TEXTURE_FILTER_MIN_POINT_MAG_MIP_LINEAR
        D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT,                      // RENDERER_TEXTURE_FILTER_MIN_LINEAR_MAG_MIP_POINT
        D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,               // RENDERER_TEXTURE_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR
        D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,                      // RENDERER_TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,                            // RENDERER_TEXTURE_FILTER_MIN_MAG_MIP_LINEAR
        D3D12_FILTER_ANISOTROPIC,                                   // RENDERER_TEXTURE_FILTER_ANISOTROPIC
        D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,                  // RENDERER_TEXTURE_FILTER_MIN_MAG_MIP_POINT
        D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,           // RENDERER_TEXTURE_FILTER_MIN_MAG_POINT_MIP_LINEAR
        D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,     // RENDERER_TEXTURE_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT
        D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,           // RENDERER_TEXTURE_FILTER_MIN_POINT_MAG_MIP_LINEAR
        D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,           // RENDERER_TEXTURE_FILTER_MIN_LINEAR_MAG_MIP_POINT
        D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,    // RENDERER_TEXTURE_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,           // RENDERER_TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT
        D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,                 // RENDERER_TEXTURE_FILTER_MIN_MAG_MIP_LINEAR
        D3D12_FILTER_COMPARISON_ANISOTROPIC,                        // RENDERER_TEXTURE_FILTER_ANISOTROPIC
    };

    static const D3D12_TEXTURE_ADDRESS_MODE D3D12TextureAddresses[TEXTURE_ADDRESS_MODE_COUNT] = 
    {
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,                     // RENDERER_TEXTURE_ADDRESS_WRAP
        D3D12_TEXTURE_ADDRESS_MODE_MIRROR,                   // RENDERER_TEXTURE_ADDRESS_MIRROR
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,                    // RENDERER_TEXTURE_ADDRESS_CLAMP
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                   // RENDERER_TEXTURE_ADDRESS_BORDER
        D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE,              // RENDERER_TEXTURE_ADDRESS_MIRROR_ONCE
    };

    DebugAssert(pSamplerStateDesc->Filter < TEXTURE_FILTER_COUNT);
    DebugAssert(pSamplerStateDesc->AddressU < TEXTURE_ADDRESS_MODE_COUNT);
    DebugAssert(pSamplerStateDesc->AddressV < TEXTURE_ADDRESS_MODE_COUNT);
    DebugAssert(pSamplerStateDesc->AddressW < TEXTURE_ADDRESS_MODE_COUNT);
    DebugAssert(pSamplerStateDesc->ComparisonFunc < GPU_COMPARISON_FUNC_COUNT);

    pOutSamplerStateDesc->Filter = D3D12TextureFilters[pSamplerStateDesc->Filter];
    pOutSamplerStateDesc->AddressU = D3D12TextureAddresses[pSamplerStateDesc->AddressU];
    pOutSamplerStateDesc->AddressV = D3D12TextureAddresses[pSamplerStateDesc->AddressV];
    pOutSamplerStateDesc->AddressW = D3D12TextureAddresses[pSamplerStateDesc->AddressW];
    Y_memcpy(pOutSamplerStateDesc->BorderColor, &pSamplerStateDesc->BorderColor, sizeof(float) * 4);
    pOutSamplerStateDesc->MipLODBias = pSamplerStateDesc->LODBias;
    pOutSamplerStateDesc->MinLOD = (float)pSamplerStateDesc->MinLOD;
    pOutSamplerStateDesc->MaxLOD = (float)pSamplerStateDesc->MaxLOD;
    pOutSamplerStateDesc->MaxAnisotropy = pSamplerStateDesc->MaxAnisotropy;
    pOutSamplerStateDesc->ComparisonFunc = s_D3D12ComparisonFuncs[pSamplerStateDesc->ComparisonFunc];
    return true;
}

D3D12_PRIMITIVE_TOPOLOGY D3D12Helpers::GetD3D12PrimitiveTopology(DRAW_TOPOLOGY topology)
{
    static const D3D12_PRIMITIVE_TOPOLOGY D3D12Topologies[DRAW_TOPOLOGY_COUNT] =
    {
        D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,           // DRAW_TOPOLOGY_UNDEFINED
        D3D_PRIMITIVE_TOPOLOGY_POINTLIST,           // DRAW_TOPOLOGY_POINTS
        D3D_PRIMITIVE_TOPOLOGY_LINELIST,            // DRAW_TOPOLOGY_LINE_LIST
        D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,           // DRAW_TOPOLOGY_LINE_STRIP
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,        // DRAW_TOPOLOGY_TRIANGLE_LIST
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,       // DRAW_TOPOLOGY_TRIANGLE_STRIP
    };

    DebugAssert(topology < countof(D3D12Topologies));
    return D3D12Topologies[topology];
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE D3D12Helpers::GetD3D12PrimitiveTopologyType(DRAW_TOPOLOGY topology)
{
    static const D3D12_PRIMITIVE_TOPOLOGY_TYPE D3D12ToplogyTypes[DRAW_TOPOLOGY_COUNT] =
    {
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED,    // DRAW_TOPOLOGY_UNDEFINED
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,        // DRAW_TOPOLOGY_POINTS
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,         // DRAW_TOPOLOGY_LINE_LIST
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,         // DRAW_TOPOLOGY_LINE_STRIP
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,     // DRAW_TOPOLOGY_TRIANGLE_LIST
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,     // DRAW_TOPOLOGY_TRIANGLE_STRIP
    };

    DebugAssert(topology < countof(D3D12ToplogyTypes));
    return D3D12ToplogyTypes[topology];
}

bool D3D12Helpers::GetResourceSRVHandle(GPUResource *pResource, D3D12DescriptorHandle *pHandle)
{
    if (pResource == nullptr)
        return false;

    switch (pResource->GetResourceType())
    {
#if 0
    case GPU_RESOURCE_TYPE_TEXTURE1D:
        *pHandle = static_cast<D3D12GPUTexture1D *>(pResource)->GetD3DSRV();
        return (!pHandle->IsNull());

    case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
        *pHandle = static_cast<D3D12GPUTexture1DArray *>(pResource)->GetD3DSRV();
        return (!pHandle->IsNull());

#endif

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        *pHandle = static_cast<D3D12GPUTexture2D *>(pResource)->GetSRVHandle();
        return (!pHandle->IsNull());

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        *pHandle = static_cast<D3D12GPUTexture2DArray *>(pResource)->GetSRVHandle();
        return (!pHandle->IsNull());

#if 0
    case GPU_RESOURCE_TYPE_TEXTURE3D:
        *pHandle = static_cast<D3D12GPUTexture3D *>(pResource)->GetSRVHandle();
        return (!pHandle->IsNull());

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        *pHandle = static_cast<D3D12GPUTextureCube *>(pResource)->GetSRVHandle();
        return (!pHandle->IsNull());

    case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
        *pHandle = static_cast<D3D12GPUTextureCubeArray *>(pResource)->GetSRVHandle();
        return (!pHandle->IsNull());
#endif
    }

    return false;
}

bool D3D12Helpers::GetResourceSamplerHandle(GPUResource *pResource, D3D12DescriptorHandle *pHandle)
{
    if (pResource == nullptr)
        return false;

    switch (pResource->GetResourceType())
    {
#if 0
    case GPU_RESOURCE_TYPE_TEXTURE1D:
        *pHandle = static_cast<D3D12GPUTexture1D *>(pResource)->GetSamplerHandle();
        return (!pHandle->IsNull());

    case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
        *pHandle = static_cast<D3D12GPUTexture1DArray *>(pResource)->GetSamplerHandle();
        return (!pHandle->IsNull());

#endif

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        *pHandle = static_cast<D3D12GPUTexture2D *>(pResource)->GetSamplerHandle();
        return (!pHandle->IsNull());

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        *pHandle = static_cast<D3D12GPUTexture2DArray *>(pResource)->GetSamplerHandle();
        return (!pHandle->IsNull());

#if 0
    case GPU_RESOURCE_TYPE_TEXTURE3D:
        *pHandle = static_cast<D3D12GPUTexture3D *>(pResource)->GetSamplerHandle();
        return (!pHandle->IsNull());

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        *pHandle = static_cast<D3D12GPUTextureCube *>(pResource)->GetSamplerHandle();
        return (!pHandle->IsNull());

    case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
        *pHandle = static_cast<D3D12GPUTextureCubeArray *>(pResource)->GetSamplerHandle();
        return (!pHandle->IsNull());
#endif
    }

    return false;
}

bool D3D12Helpers::GetOptimizedClearValue(PIXEL_FORMAT format, D3D12_CLEAR_VALUE *pValue)
{
    pValue->Format = PixelFormatToDXGIFormat(format);
    if (PixelFormatHelpers::IsDepthFormat(format))
    {
        pValue->DepthStencil.Depth = 1.0f;
        pValue->DepthStencil.Stencil = 0;
    }
    else
    {
        pValue->Color[0] = 0.0f;
        pValue->Color[1] = 0.0f;
        pValue->Color[2] = 0.0f;
        pValue->Color[3] = 0.0f;
    }

    return true;
}
