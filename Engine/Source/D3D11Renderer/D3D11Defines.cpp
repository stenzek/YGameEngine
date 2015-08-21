#include "D3D11Renderer/PrecompiledHeader.h"
#include "D3D11Renderer/D3D11Common.h"
#include "D3D11Renderer/D3D11GPUDevice.h"
#include "D3D11Renderer/D3D11GPUTexture.h"
Log_SetChannel(D3D11GPUDevice);

Y_Define_NameTable(NameTables::D3DFeatureLevels)
    Y_NameTable_VEntry(D3D_FEATURE_LEVEL_9_1,    "D3D_FEATURE_LEVEL_9_1")
    Y_NameTable_VEntry(D3D_FEATURE_LEVEL_9_2,    "D3D_FEATURE_LEVEL_9_2")
    Y_NameTable_VEntry(D3D_FEATURE_LEVEL_9_3,    "D3D_FEATURE_LEVEL_9_3")
    Y_NameTable_VEntry(D3D_FEATURE_LEVEL_10_0,   "D3D_FEATURE_LEVEL_10_0")
    Y_NameTable_VEntry(D3D_FEATURE_LEVEL_10_1,   "D3D_FEATURE_LEVEL_10_1")
    Y_NameTable_VEntry(D3D_FEATURE_LEVEL_11_0,   "D3D_FEATURE_LEVEL_11_0")
Y_NameTable_End()

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

static const D3D11_COMPARISON_FUNC s_D3D11ComparisonFuncs[GPU_COMPARISON_FUNC_COUNT] =
{
    D3D11_COMPARISON_NEVER,             // RENDERER_COMPARISON_FUNC_NEVER
    D3D11_COMPARISON_LESS,              // RENDERER_COMPARISON_FUNC_LESS
    D3D11_COMPARISON_EQUAL,             // RENDERER_COMPARISON_FUNC_EQUAL
    D3D11_COMPARISON_LESS_EQUAL,        // RENDERER_COMPARISON_FUNC_LESS_EQUAL
    D3D11_COMPARISON_GREATER,           // RENDERER_COMPARISON_FUNC_GREATER
    D3D11_COMPARISON_NOT_EQUAL,         // RENDERER_COMPARISON_FUNC_NOT_EQUAL
    D3D11_COMPARISON_GREATER_EQUAL,     // RENDERER_COMPARISON_FUNC_GREATER_EQUAL
    D3D11_COMPARISON_ALWAYS,            // RENDERER_COMPARISON_FUNC_ALWAYS
};

const char *D3D11TypeConversion::D3DFeatureLevelToString(D3D_FEATURE_LEVEL FeatureLevel)
{
    return NameTable_GetNameString(NameTables::D3DFeatureLevels, FeatureLevel, "D3D_FEATURE_LEVEL_UNKNOWN");
}

DXGI_FORMAT D3D11TypeConversion::PixelFormatToDXGIFormat(PIXEL_FORMAT Format)
{
    return (Format < PIXEL_FORMAT_COUNT) ? s_PixelFormatToDXGIFormat[Format] : DXGI_FORMAT_UNKNOWN;
}

PIXEL_FORMAT D3D11TypeConversion::DXGIFormatToPixelFormat(DXGI_FORMAT Format)
{
    uint32 i;
    for (i = 0; i < PIXEL_FORMAT_COUNT; i++)
    {
        if (s_PixelFormatToDXGIFormat[i] == Format)
            return (PIXEL_FORMAT)i;
    }

    return PIXEL_FORMAT_UNKNOWN;
}

D3D11_MAP D3D11TypeConversion::MapTypetoD3D11MapType(GPU_MAP_TYPE MapType)
{
    static const D3D11_MAP D3D11MapTypes[GPU_MAP_TYPE_COUNT] = 
    {
        D3D11_MAP_READ,                 // RENDERER_MAP_READ
        D3D11_MAP_READ_WRITE,           // RENDERER_MAP_READ_WRITE
        D3D11_MAP_WRITE,                // RENDERER_MAP_WRITE
        D3D11_MAP_WRITE_DISCARD,        // RENDERER_MAP_WRITE_DISCARD
        D3D11_MAP_WRITE_NO_OVERWRITE,   // RENDERER_MAP_WRITE_NO_OVERWRITE
    };

    DebugAssert(MapType < GPU_MAP_TYPE_COUNT);
    return D3D11MapTypes[MapType];
}

const char *D3D11TypeConversion::VertexElementSemanticToString(GPU_VERTEX_ELEMENT_SEMANTIC semantic)
{
    // lookup table for semantics
    static const char *elementSemanticNameStrings[GPU_VERTEX_ELEMENT_SEMANTIC_COUNT] =
    {
        "POSITION",         // GPU_VERTEX_ELEMENT_SEMANTIC_POSITION
        "TEXCOORD",         // GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD
        "COLOR",            // GPU_VERTEX_ELEMENT_SEMANTIC_COLOR
        "TANGENT",          // GPU_VERTEX_ELEMENT_SEMANTIC_TANGENT
        "BINORMAL",         // GPU_VERTEX_ELEMENT_SEMANTIC_BINORMAL
        "NORMAL",           // GPU_VERTEX_ELEMENT_SEMANTIC_NORMAL
        "POINTSIZE",        // GPU_VERTEX_ELEMENT_SEMANTIC_POINTSIZE
        "BLENDINDICES",     // GPU_VERTEX_ELEMENT_SEMANTIC_BLENDINDICES
        "BLENDWEIGHTS",     // GPU_VERTEX_ELEMENT_SEMANTIC_BLENDWEIGHTS
    };

    DebugAssert(semantic < GPU_VERTEX_ELEMENT_SEMANTIC_COUNT);
    return elementSemanticNameStrings[semantic];
}

const char *D3D11TypeConversion::VertexElementTypeToShaderTypeString(GPU_VERTEX_ELEMENT_TYPE type)
{
    // lookup table for types
    static const char *elementTypeNameStrings[GPU_VERTEX_ELEMENT_TYPE_COUNT] =
    {
        "int",              // GPU_VERTEX_ELEMENT_TYPE_BYTE
        "int2",             // GPU_VERTEX_ELEMENT_TYPE_BYTE2
        "int4",             // GPU_VERTEX_ELEMENT_TYPE_BYTE4
        "uint",             // GPU_VERTEX_ELEMENT_TYPE_UBYTE
        "uint2",            // GPU_VERTEX_ELEMENT_TYPE_UBYTE2
        "uint4",            // GPU_VERTEX_ELEMENT_TYPE_UBYTE4
        "half",             // GPU_VERTEX_ELEMENT_TYPE_HALF
        "half2",            // GPU_VERTEX_ELEMENT_TYPE_HALF2
        "half4",            // GPU_VERTEX_ELEMENT_TYPE_HALF4
        "float",            // GPU_VERTEX_ELEMENT_TYPE_FLOAT
        "float2",           // GPU_VERTEX_ELEMENT_TYPE_FLOAT2
        "float3",           // GPU_VERTEX_ELEMENT_TYPE_FLOAT3
        "float4",           // GPU_VERTEX_ELEMENT_TYPE_FLOAT4
        "int",              // GPU_VERTEX_ELEMENT_TYPE_INT
        "int2",             // GPU_VERTEX_ELEMENT_TYPE_INT2
        "int3",             // GPU_VERTEX_ELEMENT_TYPE_INT3
        "int4",             // GPU_VERTEX_ELEMENT_TYPE_INT4
        "uint",             // GPU_VERTEX_ELEMENT_TYPE_UINT
        "uint2",            // GPU_VERTEX_ELEMENT_TYPE_UINT2
        "uint3",            // GPU_VERTEX_ELEMENT_TYPE_UINT3
        "uint4",            // GPU_VERTEX_ELEMENT_TYPE_UINT4
        "float4",           // GPU_VERTEX_ELEMENT_TYPE_SNORM4
        "float4",           // GPU_VERTEX_ELEMENT_TYPE_UNORM4
    };

    DebugAssert(type < GPU_VERTEX_ELEMENT_TYPE_COUNT);
    return elementTypeNameStrings[type];
}

DXGI_FORMAT D3D11TypeConversion::VertexElementTypeToDXGIFormat(GPU_VERTEX_ELEMENT_TYPE type)
{
    // lookup table for types
    static const DXGI_FORMAT elementTypeNameStrings[GPU_VERTEX_ELEMENT_TYPE_COUNT] =
    {
        DXGI_FORMAT_R8_SINT,            // GPU_VERTEX_ELEMENT_TYPE_BYTE
        DXGI_FORMAT_R8G8_SINT,          // GPU_VERTEX_ELEMENT_TYPE_BYTE2
        DXGI_FORMAT_R8G8B8A8_SINT,      // GPU_VERTEX_ELEMENT_TYPE_BYTE4
        DXGI_FORMAT_R8_UINT,            // GPU_VERTEX_ELEMENT_TYPE_UBYTE
        DXGI_FORMAT_R8G8_UINT,          // GPU_VERTEX_ELEMENT_TYPE_UBYTE2
        DXGI_FORMAT_R8G8B8A8_UINT,      // GPU_VERTEX_ELEMENT_TYPE_UBYTE4
        DXGI_FORMAT_R16_FLOAT,          // GPU_VERTEX_ELEMENT_TYPE_HALF
        DXGI_FORMAT_R16G16_FLOAT,       // GPU_VERTEX_ELEMENT_TYPE_HALF2
        DXGI_FORMAT_R16G16B16A16_FLOAT, // GPU_VERTEX_ELEMENT_TYPE_HALF4
        DXGI_FORMAT_R32_FLOAT,          // GPU_VERTEX_ELEMENT_TYPE_FLOAT
        DXGI_FORMAT_R32G32_FLOAT,       // GPU_VERTEX_ELEMENT_TYPE_FLOAT2
        DXGI_FORMAT_R32G32B32_FLOAT,    // GPU_VERTEX_ELEMENT_TYPE_FLOAT3
        DXGI_FORMAT_R32G32B32A32_FLOAT, // GPU_VERTEX_ELEMENT_TYPE_FLOAT4
        DXGI_FORMAT_R32_SINT,           // GPU_VERTEX_ELEMENT_TYPE_INT
        DXGI_FORMAT_R32G32_SINT,        // GPU_VERTEX_ELEMENT_TYPE_INT2
        DXGI_FORMAT_R32G32B32_SINT,     // GPU_VERTEX_ELEMENT_TYPE_INT3
        DXGI_FORMAT_R32G32B32A32_SINT,  // GPU_VERTEX_ELEMENT_TYPE_INT4
        DXGI_FORMAT_R32_UINT,           // GPU_VERTEX_ELEMENT_TYPE_UINT
        DXGI_FORMAT_R32G32_UINT,        // GPU_VERTEX_ELEMENT_TYPE_UINT2
        DXGI_FORMAT_R32G32B32_UINT,     // GPU_VERTEX_ELEMENT_TYPE_UINT3
        DXGI_FORMAT_R32G32B32A32_UINT,  // GPU_VERTEX_ELEMENT_TYPE_UINT4
        DXGI_FORMAT_R8G8B8A8_SNORM,     // GPU_VERTEX_ELEMENT_TYPE_SNORM4
        DXGI_FORMAT_R8G8B8A8_UNORM,     // GPU_VERTEX_ELEMENT_TYPE_UNORM4
    };

    DebugAssert(type < GPU_VERTEX_ELEMENT_TYPE_COUNT);
    return elementTypeNameStrings[type];
}

ID3D11RasterizerState *D3D11Helpers::CreateD3D11RasterizerState(ID3D11Device *pD3DDevice, const RENDERER_RASTERIZER_STATE_DESC *pRasterizerState)
{
    static const D3D11_FILL_MODE D3D11FillModes[RENDERER_FILL_MODE_COUNT] =
    {
        D3D11_FILL_WIREFRAME,               // RENDERER_FILL_WIREFRAME
        D3D11_FILL_SOLID,                   // RENDERER_FILL_SOLID
    };

    static const D3D11_CULL_MODE D3D11CullModes[RENDERER_CULL_MODE_COUNT] =
    {
        D3D11_CULL_NONE,                    // RENDERER_CULL_NONE
        D3D11_CULL_FRONT,                   // RENDERER_CULL_FRONT
        D3D11_CULL_BACK,                    // RENDERER_CULL_BACK
    };

    DebugAssert(pRasterizerState->FillMode < RENDERER_FILL_MODE_COUNT);
    DebugAssert(pRasterizerState->CullMode < RENDERER_CULL_MODE_COUNT);
    
    D3D11_RASTERIZER_DESC RasterizerDesc;
    RasterizerDesc.FillMode = D3D11FillModes[pRasterizerState->FillMode];
    RasterizerDesc.CullMode = D3D11CullModes[pRasterizerState->CullMode];
    RasterizerDesc.FrontCounterClockwise = pRasterizerState->FrontCounterClockwise ? TRUE : FALSE;
    RasterizerDesc.DepthBias = pRasterizerState->DepthBias;
    RasterizerDesc.DepthBiasClamp = 0;
    RasterizerDesc.SlopeScaledDepthBias = pRasterizerState->SlopeScaledDepthBias;
    RasterizerDesc.DepthClipEnable = pRasterizerState->DepthClipEnable ? TRUE : FALSE;
    RasterizerDesc.ScissorEnable = pRasterizerState->ScissorEnable ? TRUE : FALSE;
    RasterizerDesc.MultisampleEnable = FALSE;
    RasterizerDesc.AntialiasedLineEnable = FALSE;

    ID3D11RasterizerState *pD3DRasterizerState;
    HRESULT hResult = pD3DDevice->CreateRasterizerState(&RasterizerDesc, &pD3DRasterizerState);
    if (SUCCEEDED(hResult))
    {
        return pD3DRasterizerState;
    }
    else
    {
        Log_WarningPrintf("CreateRasterizerState failed with HResult %08X", hResult);
        return nullptr;
    }
}

ID3D11DepthStencilState *D3D11Helpers::CreateD3D11DepthStencilState(ID3D11Device *pD3DDevice, const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilState)
{
    static const D3D11_STENCIL_OP D3D11StencilOps[RENDERER_STENCIL_OP_COUNT] =
    {
        D3D11_STENCIL_OP_KEEP,              // RENDERER_STENCIL_OP_KEEP
        D3D11_STENCIL_OP_ZERO,              // RENDERER_STENCIL_OP_ZERO
        D3D11_STENCIL_OP_REPLACE,           // RENDERER_STENCIL_OP_REPLACE
        D3D11_STENCIL_OP_INCR_SAT,          // RENDERER_STENCIL_OP_INCREMENT_CLAMPED
        D3D11_STENCIL_OP_DECR_SAT,          // RENDERER_STENCIL_OP_DECREMENT_CLAMPED
        D3D11_STENCIL_OP_INVERT,            // RENDERER_STENCIL_OP_INVERT
        D3D11_STENCIL_OP_INCR,              // RENDERER_STENCIL_OP_INCREMENT
        D3D11_STENCIL_OP_DECR,              // RENDERER_STENCIL_OP_DECREMENT
    };

    DebugAssert(pDepthStencilState->DepthFunc < GPU_COMPARISON_FUNC_COUNT);

    D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
    DepthStencilDesc.DepthEnable = pDepthStencilState->DepthTestEnable ? TRUE : FALSE;
    DepthStencilDesc.DepthWriteMask = pDepthStencilState->DepthWriteEnable ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    DepthStencilDesc.DepthFunc = s_D3D11ComparisonFuncs[pDepthStencilState->DepthFunc];
    
    DebugAssert(pDepthStencilState->StencilFrontFace.FailOp < RENDERER_STENCIL_OP_COUNT);
    DebugAssert(pDepthStencilState->StencilFrontFace.DepthFailOp < RENDERER_STENCIL_OP_COUNT);
    DebugAssert(pDepthStencilState->StencilFrontFace.PassOp < RENDERER_STENCIL_OP_COUNT);
    DebugAssert(pDepthStencilState->StencilFrontFace.CompareFunc < GPU_COMPARISON_FUNC_COUNT);
    DebugAssert(pDepthStencilState->StencilBackFace.FailOp < RENDERER_STENCIL_OP_COUNT);
    DebugAssert(pDepthStencilState->StencilBackFace.DepthFailOp < RENDERER_STENCIL_OP_COUNT);
    DebugAssert(pDepthStencilState->StencilBackFace.PassOp < RENDERER_STENCIL_OP_COUNT);
    DebugAssert(pDepthStencilState->StencilBackFace.CompareFunc < GPU_COMPARISON_FUNC_COUNT);

    DepthStencilDesc.StencilEnable = pDepthStencilState->StencilTestEnable ? TRUE : FALSE;
    DepthStencilDesc.StencilReadMask = pDepthStencilState->StencilReadMask;
    DepthStencilDesc.StencilWriteMask = pDepthStencilState->StencilWriteMask;
    DepthStencilDesc.FrontFace.StencilFailOp = D3D11StencilOps[pDepthStencilState->StencilFrontFace.FailOp];
    DepthStencilDesc.FrontFace.StencilDepthFailOp = D3D11StencilOps[pDepthStencilState->StencilFrontFace.DepthFailOp];
    DepthStencilDesc.FrontFace.StencilPassOp = D3D11StencilOps[pDepthStencilState->StencilFrontFace.PassOp];
    DepthStencilDesc.FrontFace.StencilFunc = s_D3D11ComparisonFuncs[pDepthStencilState->StencilFrontFace.CompareFunc];
    DepthStencilDesc.BackFace.StencilFailOp = D3D11StencilOps[pDepthStencilState->StencilBackFace.FailOp];
    DepthStencilDesc.BackFace.StencilDepthFailOp = D3D11StencilOps[pDepthStencilState->StencilBackFace.DepthFailOp];
    DepthStencilDesc.BackFace.StencilPassOp = D3D11StencilOps[pDepthStencilState->StencilBackFace.PassOp];
    DepthStencilDesc.BackFace.StencilFunc = s_D3D11ComparisonFuncs[pDepthStencilState->StencilBackFace.CompareFunc];

    ID3D11DepthStencilState *pD3DDepthStencilState;
    HRESULT hResult = pD3DDevice->CreateDepthStencilState(&DepthStencilDesc, &pD3DDepthStencilState);
    if (SUCCEEDED(hResult))
    {
        return pD3DDepthStencilState;
    }
    else
    {
        Log_WarningPrintf("CreateDepthStencilState failed with HResult %08X", hResult);
        return nullptr;
    }
}

ID3D11BlendState *D3D11Helpers::CreateD3D11BlendState(ID3D11Device *pD3DDevice, const RENDERER_BLEND_STATE_DESC *pBlendState)
{
    static const D3D11_BLEND D3D11BlendOptions[RENDERER_BLEND_OPTION_COUNT] = 
    {
        D3D11_BLEND_ZERO,                   // RENDERER_BLEND_ZERO
        D3D11_BLEND_ONE,                    // RENDERER_BLEND_ONE
        D3D11_BLEND_SRC_COLOR,              // RENDERER_BLEND_SRC_COLOR
        D3D11_BLEND_INV_SRC_COLOR,          // RENDERER_BLEND_INV_SRC_COLOR
        D3D11_BLEND_SRC_ALPHA,              // RENDERER_BLEND_SRC_ALPHA
        D3D11_BLEND_INV_SRC_ALPHA,          // RENDERER_BLEND_INV_SRC_ALPHA
        D3D11_BLEND_DEST_ALPHA,             // RENDERER_BLEND_DEST_ALPHA
        D3D11_BLEND_INV_DEST_ALPHA,         // RENDERER_BLEND_INV_DEST_ALPHA
        D3D11_BLEND_DEST_COLOR,             // RENDERER_BLEND_DEST_COLOR
        D3D11_BLEND_INV_DEST_COLOR,         // RENDERER_BLEND_INV_DEST_COLOR
        D3D11_BLEND_SRC_ALPHA_SAT,          // RENDERER_BLEND_SRC_ALPHA_SAT
        D3D11_BLEND_BLEND_FACTOR,           // RENDERER_BLEND_BLEND_FACTOR
        D3D11_BLEND_INV_BLEND_FACTOR,       // RENDERER_BLEND_INV_BLEND_FACTOR
        D3D11_BLEND_SRC1_COLOR,             // RENDERER_BLEND_SRC1_COLOR
        D3D11_BLEND_INV_SRC1_COLOR,         // RENDERER_BLEND_INV_SRC1_COLOR
        D3D11_BLEND_SRC1_ALPHA,             // RENDERER_BLEND_SRC1_ALPHA
        D3D11_BLEND_INV_SRC1_ALPHA,         // RENDERER_BLEND_INV_SRC1_ALPHA
    };

    static const D3D11_BLEND_OP D3D11BlendOps[RENDERER_BLEND_OP_COUNT] =
    {
        D3D11_BLEND_OP_ADD,                 // RENDERER_BLEND_OP_ADD
        D3D11_BLEND_OP_SUBTRACT,            // RENDERER_BLEND_OP_SUBTRACT
        D3D11_BLEND_OP_REV_SUBTRACT,        // RENDERER_BLEND_OP_REV_SUBTRACT
        D3D11_BLEND_OP_MIN,                 // RENDERER_BLEND_OP_MIN
        D3D11_BLEND_OP_MAX,                 // RENDERER_BLEND_OP_MAX
    };

    DebugAssert(pBlendState->SrcBlend < RENDERER_BLEND_OPTION_COUNT);
    DebugAssert(pBlendState->BlendOp < RENDERER_BLEND_OP_COUNT);
    DebugAssert(pBlendState->DestBlend < RENDERER_BLEND_OPTION_COUNT);
    DebugAssert(pBlendState->SrcBlendAlpha < RENDERER_BLEND_OPTION_COUNT);
    DebugAssert(pBlendState->BlendOpAlpha < RENDERER_BLEND_OP_COUNT);
    DebugAssert(pBlendState->DestBlendAlpha < RENDERER_BLEND_OPTION_COUNT);

    D3D11_BLEND_DESC BlendDesc;
    BlendDesc.AlphaToCoverageEnable = FALSE;
    BlendDesc.IndependentBlendEnable = FALSE;

    for (uint32 i = 0; i < 8; i++)
    {
        BlendDesc.RenderTarget[i].BlendEnable = pBlendState->BlendEnable ? TRUE : FALSE;
        BlendDesc.RenderTarget[i].RenderTargetWriteMask = pBlendState->ColorWriteEnable ? D3D10_COLOR_WRITE_ENABLE_ALL : 0;
        BlendDesc.RenderTarget[i].SrcBlend = D3D11BlendOptions[pBlendState->SrcBlend];
        BlendDesc.RenderTarget[i].BlendOp = D3D11BlendOps[pBlendState->BlendOp];
        BlendDesc.RenderTarget[i].DestBlend = D3D11BlendOptions[pBlendState->DestBlend];
        BlendDesc.RenderTarget[i].SrcBlendAlpha = D3D11BlendOptions[pBlendState->SrcBlendAlpha];
        BlendDesc.RenderTarget[i].BlendOpAlpha = D3D11BlendOps[pBlendState->BlendOpAlpha];
        BlendDesc.RenderTarget[i].DestBlendAlpha = D3D11BlendOptions[pBlendState->DestBlendAlpha];
    }  
   
    ID3D11BlendState *pD3DBlendState;
    HRESULT hResult = pD3DDevice->CreateBlendState(&BlendDesc, &pD3DBlendState);
    if (SUCCEEDED(hResult))
    {
        return pD3DBlendState;
    }
    else
    {
        Log_WarningPrintf("CreateBlendState failed with HResult %08X", hResult);
        return nullptr;
    }
}

ID3D11SamplerState *D3D11Helpers::CreateD3D11SamplerState(ID3D11Device *pD3DDevice, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc)
{
    static const D3D11_FILTER D3D11TextureFilters[TEXTURE_FILTER_COUNT] =
    {
        D3D11_FILTER_MIN_MAG_MIP_POINT,                             // RENDERER_TEXTURE_FILTER_MIN_MAG_MIP_POINT
        D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,                      // RENDERER_TEXTURE_FILTER_MIN_MAG_POINT_MIP_LINEAR
        D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,                // RENDERER_TEXTURE_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT
        D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,                      // RENDERER_TEXTURE_FILTER_MIN_POINT_MAG_MIP_LINEAR
        D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,                      // RENDERER_TEXTURE_FILTER_MIN_LINEAR_MAG_MIP_POINT
        D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,               // RENDERER_TEXTURE_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR
        D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,                      // RENDERER_TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,                            // RENDERER_TEXTURE_FILTER_MIN_MAG_MIP_LINEAR
        D3D11_FILTER_ANISOTROPIC,                                   // RENDERER_TEXTURE_FILTER_ANISOTROPIC
        D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT,                  // RENDERER_TEXTURE_FILTER_MIN_MAG_MIP_POINT
        D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,           // RENDERER_TEXTURE_FILTER_MIN_MAG_POINT_MIP_LINEAR
        D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,     // RENDERER_TEXTURE_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT
        D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,           // RENDERER_TEXTURE_FILTER_MIN_POINT_MAG_MIP_LINEAR
        D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,           // RENDERER_TEXTURE_FILTER_MIN_LINEAR_MAG_MIP_POINT
        D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,    // RENDERER_TEXTURE_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR
        D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,           // RENDERER_TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT
        D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,                 // RENDERER_TEXTURE_FILTER_MIN_MAG_MIP_LINEAR
        D3D11_FILTER_COMPARISON_ANISOTROPIC,                        // RENDERER_TEXTURE_FILTER_ANISOTROPIC
    };

    static const D3D11_TEXTURE_ADDRESS_MODE D3D11TextureAddresses[TEXTURE_ADDRESS_MODE_COUNT] = 
    {
        D3D11_TEXTURE_ADDRESS_WRAP,                     // RENDERER_TEXTURE_ADDRESS_WRAP
        D3D11_TEXTURE_ADDRESS_MIRROR,                   // RENDERER_TEXTURE_ADDRESS_MIRROR
        D3D11_TEXTURE_ADDRESS_CLAMP,                    // RENDERER_TEXTURE_ADDRESS_CLAMP
        D3D11_TEXTURE_ADDRESS_BORDER,                   // RENDERER_TEXTURE_ADDRESS_BORDER
        D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,              // RENDERER_TEXTURE_ADDRESS_MIRROR_ONCE
    };

    DebugAssert(pSamplerStateDesc->Filter < TEXTURE_FILTER_COUNT);
    DebugAssert(pSamplerStateDesc->AddressU < TEXTURE_ADDRESS_MODE_COUNT);
    DebugAssert(pSamplerStateDesc->AddressV < TEXTURE_ADDRESS_MODE_COUNT);
    DebugAssert(pSamplerStateDesc->AddressW < TEXTURE_ADDRESS_MODE_COUNT);
    DebugAssert(pSamplerStateDesc->ComparisonFunc < GPU_COMPARISON_FUNC_COUNT);

    D3D11_SAMPLER_DESC SamplerDesc;
    SamplerDesc.Filter = D3D11TextureFilters[pSamplerStateDesc->Filter];
    SamplerDesc.AddressU = D3D11TextureAddresses[pSamplerStateDesc->AddressU];
    SamplerDesc.AddressV = D3D11TextureAddresses[pSamplerStateDesc->AddressV];
    SamplerDesc.AddressW = D3D11TextureAddresses[pSamplerStateDesc->AddressW];
    Y_memcpy(SamplerDesc.BorderColor, &pSamplerStateDesc->BorderColor, sizeof(float) * 4);
    SamplerDesc.MipLODBias = pSamplerStateDesc->LODBias;
    SamplerDesc.MinLOD = (float)pSamplerStateDesc->MinLOD;
    SamplerDesc.MaxLOD = (float)pSamplerStateDesc->MaxLOD;
    SamplerDesc.MaxAnisotropy = pSamplerStateDesc->MaxAnisotropy;
    SamplerDesc.ComparisonFunc = s_D3D11ComparisonFuncs[pSamplerStateDesc->ComparisonFunc];

    // create the state
    ID3D11SamplerState *pSamplerState;
    HRESULT hResult = pD3DDevice->CreateSamplerState(&SamplerDesc, &pSamplerState);
    if (FAILED(hResult))
    {
        Log_WarningPrintf("CreateSamplerState failed with HResult %08X", hResult);
        return nullptr;
    }

    return pSamplerState;
}

ID3D11ShaderResourceView *D3D11Helpers::GetResourceShaderResourceView(GPUResource *pResource)
{
    if (pResource == nullptr)
        return nullptr;

    switch (pResource->GetResourceType())
    {
    case GPU_RESOURCE_TYPE_TEXTURE1D:
        return static_cast<D3D11GPUTexture1D *>(pResource)->GetD3DSRV();

    case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
        return static_cast<D3D11GPUTexture1DArray *>(pResource)->GetD3DSRV();

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        return static_cast<D3D11GPUTexture2D *>(pResource)->GetD3DSRV();

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        return static_cast<D3D11GPUTexture2DArray *>(pResource)->GetD3DSRV();

    case GPU_RESOURCE_TYPE_TEXTURE3D:
        return static_cast<D3D11GPUTexture3D *>(pResource)->GetD3DSRV();

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        return static_cast<D3D11GPUTextureCube *>(pResource)->GetD3DSRV();

    case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
        return static_cast<D3D11GPUTextureCubeArray *>(pResource)->GetD3DSRV();
    }

    return nullptr;
}

ID3D11SamplerState *D3D11Helpers::GetResourceSamplerState(GPUResource *pResource)
{
    if (pResource == nullptr)
        return nullptr;

    switch (pResource->GetResourceType())
    {
    case GPU_RESOURCE_TYPE_SAMPLER_STATE:
        return static_cast<D3D11SamplerState *>(pResource)->GetD3DSamplerState();

    case GPU_RESOURCE_TYPE_TEXTURE1D:
        return static_cast<D3D11GPUTexture1D *>(pResource)->GetD3DSamplerState();

    case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
        return static_cast<D3D11GPUTexture1DArray *>(pResource)->GetD3DSamplerState();

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        return static_cast<D3D11GPUTexture2D *>(pResource)->GetD3DSamplerState();

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        return static_cast<D3D11GPUTexture2DArray *>(pResource)->GetD3DSamplerState();

    case GPU_RESOURCE_TYPE_TEXTURE3D:
        return static_cast<D3D11GPUTexture3D *>(pResource)->GetD3DSamplerState();

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        return static_cast<D3D11GPUTextureCube *>(pResource)->GetD3DSamplerState();

    case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
        return static_cast<D3D11GPUTextureCubeArray *>(pResource)->GetD3DSamplerState();
    }

    return nullptr;
}

void D3D11Helpers::SetD3D11DeviceChildDebugName(ID3D11DeviceChild *pDeviceChild, const char *debugName)
{
#ifdef Y_BUILD_CONFIG_DEBUG
    uint32 nameLength = Y_strlen(debugName);
    if (nameLength > 0)
        pDeviceChild->SetPrivateData(WKPDID_D3DDebugObjectName, nameLength, debugName);
#endif
}

