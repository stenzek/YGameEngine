#pragma once
#include "Renderer/Common.h"

// Forward declarations for all GPU objects
class GPUResource;
class GPUSamplerState;
class GPURasterizerState;
class GPUDepthStencilState;
class GPUBlendState;
class GPUBuffer;
class GPUTexture;
class GPUTexture1D;
class GPUTexture1DArray;
class GPUTexture2D;
class GPUTexture2DArray;
class GPUTexture3D;
class GPUTextureCube;
class GPUTextureCubeArray;
class GPUDepthTexture;
class GPUQuery;
class GPUShaderProgram;
class GPUShaderPipeline;
class GPURenderTargetView;
class GPUDepthStencilBufferView;
class GPUComputeView;
class GPUOutputBuffer;
class RendererOutputWindow;
class GPUContextConstants;
class GPUContext;
class GPUDevice;
class GPUCommandList;
class Renderer;

// Platform i.e. API
enum RENDERER_PLATFORM
{
    RENDERER_PLATFORM_D3D11,
    RENDERER_PLATFORM_D3D12,
    RENDERER_PLATFORM_OPENGL,
    RENDERER_PLATFORM_OPENGLES2,
    RENDERER_PLATFORM_COUNT,
};

// Each feature level is considered to be a superset of the previous level.
enum RENDERER_FEATURE_LEVEL
{
    RENDERER_FEATURE_LEVEL_ES2,
    RENDERER_FEATURE_LEVEL_ES3,
    RENDERER_FEATURE_LEVEL_SM4,
    RENDERER_FEATURE_LEVEL_SM5,
    RENDERER_FEATURE_LEVEL_COUNT,
};

enum RENDERER_FULLSCREEN_STATE
{
    RENDERER_FULLSCREEN_STATE_WINDOWED,
    RENDERER_FULLSCREEN_STATE_WINDOWED_FULLSCREEN,
    RENDERER_FULLSCREEN_STATE_FULLSCREEN,
    RENDERER_FULLSCREEN_STATE_COUNT,
};

enum RENDERER_VSYNC_TYPE
{
    RENDERER_VSYNC_TYPE_NONE,
    RENDERER_VSYNC_TYPE_VSYNC,
    RENDERER_VSYNC_TYPE_ADAPTIVE_VSYNC,
    RENDERER_VSYNC_TYPE_TRIPLE_BUFFERING,
    RENDERER_VSYNC_TYPE_COUNT,
};

enum GPU_PRESENT_BEHAVIOUR
{
    GPU_PRESENT_BEHAVIOUR_IMMEDIATE,
    GPU_PRESENT_BEHAVIOUR_WAIT_FOR_VBLANK,
    GPU_PRESENT_BEHAVIOUR_COUNT
};

enum GPU_INDEX_FORMAT
{
    GPU_INDEX_FORMAT_UINT16,
    GPU_INDEX_FORMAT_UINT32,
    GPU_INDEX_FORMAT_COUNT,
};

enum GPU_COMPARISON_FUNC
{
    GPU_COMPARISON_FUNC_NEVER,
    GPU_COMPARISON_FUNC_LESS,
    GPU_COMPARISON_FUNC_EQUAL,
    GPU_COMPARISON_FUNC_LESS_EQUAL,
    GPU_COMPARISON_FUNC_GREATER,
    GPU_COMPARISON_FUNC_NOT_EQUAL,
    GPU_COMPARISON_FUNC_GREATER_EQUAL,
    GPU_COMPARISON_FUNC_ALWAYS,
    GPU_COMPARISON_FUNC_COUNT,
};

enum DRAW_TOPOLOGY
{
    DRAW_TOPOLOGY_UNDEFINED,
    DRAW_TOPOLOGY_POINTS,
    DRAW_TOPOLOGY_LINE_LIST,
    DRAW_TOPOLOGY_LINE_STRIP,
    DRAW_TOPOLOGY_TRIANGLE_LIST,
    DRAW_TOPOLOGY_TRIANGLE_STRIP,
    DRAW_TOPOLOGY_COUNT,
};

int32 DrawTopology_CalculateNumVertices(DRAW_TOPOLOGY Topology, int32 nPrimitives);
int32 DrawTopology_CalculateNumPrimitives(DRAW_TOPOLOGY Topology, int32 nVertices);

enum GPU_MAP_TYPE
{
    GPU_MAP_TYPE_READ,
    GPU_MAP_TYPE_READ_WRITE,
    GPU_MAP_TYPE_WRITE,
    GPU_MAP_TYPE_WRITE_DISCARD,
    GPU_MAP_TYPE_WRITE_NO_OVERWRITE,
    GPU_MAP_TYPE_COUNT,
};

struct RENDERER_SCISSOR_RECT
{
    uint32 Left;
    uint32 Top;
    uint32 Right;
    uint32 Bottom;

    RENDERER_SCISSOR_RECT() {}
    RENDERER_SCISSOR_RECT(uint32 left, uint32 top, uint32 right, uint32 bottom) : Left(left), Top(top), Right(right), Bottom(bottom) {}
    void Set(uint32 left, uint32 top, uint32 right, uint32 bottom) { Left = left; Top = top; Right = right; Bottom = bottom; }
};

struct RENDERER_VIEWPORT
{
    uint32 TopLeftX;
    uint32 TopLeftY;
    uint32 Width;
    uint32 Height;
    float MinDepth;
    float MaxDepth;

    RENDERER_VIEWPORT() {}
    RENDERER_VIEWPORT(uint32 topLeftX, uint32 topLeftY, uint32 width, uint32 height, float minDepth, float maxDepth) : TopLeftX(topLeftX), TopLeftY(topLeftY), Width(width), Height(height), MinDepth(minDepth), MaxDepth(maxDepth) {}
    void Set(uint32 topLeftX, uint32 topLeftY, uint32 width, uint32 height, float minDepth, float maxDepth) { TopLeftX = topLeftX; TopLeftY = topLeftY; Width = width; Height = height; MinDepth = minDepth; MaxDepth = maxDepth; }
};

enum GPU_RESOURCE_TYPE
{
    GPU_RESOURCE_TYPE_RASTERIZER_STATE,
    GPU_RESOURCE_TYPE_DEPTH_STENCIL_STATE,
    GPU_RESOURCE_TYPE_BLEND_STATE,
    GPU_RESOURCE_TYPE_SAMPLER_STATE,
    GPU_RESOURCE_TYPE_BUFFER,
    GPU_RESOURCE_TYPE_TEXTURE1D,
    GPU_RESOURCE_TYPE_TEXTURE1DARRAY,
    GPU_RESOURCE_TYPE_TEXTURE2D,
    GPU_RESOURCE_TYPE_TEXTURE2DARRAY,
    GPU_RESOURCE_TYPE_TEXTURE3D,
    GPU_RESOURCE_TYPE_TEXTURECUBE,
    GPU_RESOURCE_TYPE_TEXTURECUBEARRAY,
    GPU_RESOURCE_TYPE_TEXTUREBUFFER,
    GPU_RESOURCE_TYPE_DEPTH_TEXTURE,
    GPU_RESOURCE_TYPE_RENDER_TARGET_VIEW,
    GPU_RESOURCE_TYPE_DEPTH_BUFFER_VIEW,
    GPU_RESOURCE_TYPE_COMPUTE_VIEW,
    GPU_RESOURCE_TYPE_QUERY,
    GPU_RESOURCE_TYPE_SHADER_PROGRAM,
    GPU_RESOURCE_TYPE_SHADER_PIPELINE,
    GPU_RESOURCE_TYPE_COUNT,
};

namespace NameTables {
    Y_Declare_NameTable(GPUResourceType);
}

#define GPU_MAX_SIMULTANEOUS_RENDER_TARGETS (8)
#define GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS (8)

#define GPU_INPUT_LAYOUT_MAX_ELEMENTS (16)

enum GPU_VERTEX_ELEMENT_TYPE
{
    GPU_VERTEX_ELEMENT_TYPE_BYTE,
    GPU_VERTEX_ELEMENT_TYPE_BYTE2,
    GPU_VERTEX_ELEMENT_TYPE_BYTE4,
    GPU_VERTEX_ELEMENT_TYPE_UBYTE,
    GPU_VERTEX_ELEMENT_TYPE_UBYTE2,
    GPU_VERTEX_ELEMENT_TYPE_UBYTE4,
    GPU_VERTEX_ELEMENT_TYPE_HALF,
    GPU_VERTEX_ELEMENT_TYPE_HALF2,
    GPU_VERTEX_ELEMENT_TYPE_HALF4,
    GPU_VERTEX_ELEMENT_TYPE_FLOAT,
    GPU_VERTEX_ELEMENT_TYPE_FLOAT2,
    GPU_VERTEX_ELEMENT_TYPE_FLOAT3,
    GPU_VERTEX_ELEMENT_TYPE_FLOAT4,
    GPU_VERTEX_ELEMENT_TYPE_INT,
    GPU_VERTEX_ELEMENT_TYPE_INT2,
    GPU_VERTEX_ELEMENT_TYPE_INT3,
    GPU_VERTEX_ELEMENT_TYPE_INT4,
    GPU_VERTEX_ELEMENT_TYPE_UINT,
    GPU_VERTEX_ELEMENT_TYPE_UINT2,
    GPU_VERTEX_ELEMENT_TYPE_UINT3,
    GPU_VERTEX_ELEMENT_TYPE_UINT4,
    GPU_VERTEX_ELEMENT_TYPE_SNORM4,
    GPU_VERTEX_ELEMENT_TYPE_UNORM4,
    GPU_VERTEX_ELEMENT_TYPE_COUNT,
};

enum GPU_VERTEX_ELEMENT_SEMANTIC
{
    GPU_VERTEX_ELEMENT_SEMANTIC_POSITION,
    GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD,
    GPU_VERTEX_ELEMENT_SEMANTIC_COLOR,
    GPU_VERTEX_ELEMENT_SEMANTIC_TANGENT,
    GPU_VERTEX_ELEMENT_SEMANTIC_BINORMAL,
    GPU_VERTEX_ELEMENT_SEMANTIC_NORMAL,
    GPU_VERTEX_ELEMENT_SEMANTIC_POINTSIZE,
    GPU_VERTEX_ELEMENT_SEMANTIC_BLENDINDICES,
    GPU_VERTEX_ELEMENT_SEMANTIC_BLENDWEIGHTS,
    GPU_VERTEX_ELEMENT_SEMANTIC_COUNT,
};


struct GPU_VERTEX_ELEMENT_DESC
{
    GPU_VERTEX_ELEMENT_SEMANTIC Semantic;
    uint32 SemanticIndex;
    GPU_VERTEX_ELEMENT_TYPE Type;
    uint32 StreamIndex;
    uint32 StreamOffset;
    uint32 InstanceStepRate;

    GPU_VERTEX_ELEMENT_DESC() {}

    GPU_VERTEX_ELEMENT_DESC(GPU_VERTEX_ELEMENT_SEMANTIC semantic, uint32 semanticIndex, GPU_VERTEX_ELEMENT_TYPE type, uint32 streamIndex, uint32 streamOffset, uint32 instanceStepRate)
        : Semantic(semantic), SemanticIndex(semanticIndex), Type(type), StreamIndex(streamIndex), StreamOffset(streamOffset), InstanceStepRate(instanceStepRate) {}

    void Set(GPU_VERTEX_ELEMENT_SEMANTIC semantic, uint32 semanticIndex, GPU_VERTEX_ELEMENT_TYPE type, uint32 streamIndex, uint32 streamOffset, uint32 instanceStepRate)
    {
        Semantic = semantic;
        SemanticIndex = semanticIndex;
        Type = type;
        StreamIndex = streamIndex;
        StreamOffset = streamOffset;
        InstanceStepRate = instanceStepRate;
    }

};

enum GPU_QUERY_TYPE
{
    GPU_QUERY_TYPE_SAMPLES_PASSED,            // uint64
    GPU_QUERY_TYPE_OCCLUSION,                 // bool
    GPU_QUERY_TYPE_PRIMITIVES_GENERATED,      // uint64
    GPU_QUERY_TYPE_TIMESTAMP,                 // api-specific key
    GPU_QUERY_TYPE_FREQUENCY,                 // frequency to divide timestamp by to get seconds, 0 if unstable
    GPU_QUERY_TYPE_COUNT,
};

enum GPU_QUERY_GETDATA_FLAGS
{
    GPU_QUERY_GETDATA_FLAG_NOFLUSH = (1 << 0),
};

enum GPU_QUERY_GETDATA_RESULT
{
    GPU_QUERY_GETDATA_RESULT_OK,
    GPU_QUERY_GETDATA_RESULT_NOT_READY,
    GPU_QUERY_GETDATA_RESULT_ERROR,
};

enum SHADER_PROGRAM_STAGE
{
    SHADER_PROGRAM_STAGE_VERTEX_SHADER,
    SHADER_PROGRAM_STAGE_HULL_SHADER,
    SHADER_PROGRAM_STAGE_DOMAIN_SHADER,
    SHADER_PROGRAM_STAGE_GEOMETRY_SHADER,
    SHADER_PROGRAM_STAGE_PIXEL_SHADER,
    SHADER_PROGRAM_STAGE_COMPUTE_SHADER,
    SHADER_PROGRAM_STAGE_COUNT,
};

enum SHADER_PROGRAM_STAGE_MASK
{
    SHADER_PROGRAM_STAGE_MASK_VERTEX_SHADER     = (1 << SHADER_PROGRAM_STAGE_VERTEX_SHADER),
    SHADER_PROGRAM_STAGE_MASK_HULL_SHADER       = (1 << SHADER_PROGRAM_STAGE_HULL_SHADER),
    SHADER_PROGRAM_STAGE_MASK_DOMAIN_SHADER     = (1 << SHADER_PROGRAM_STAGE_DOMAIN_SHADER),
    SHADER_PROGRAM_STAGE_MASK_GEOMETRY_SHADER   = (1 << SHADER_PROGRAM_STAGE_GEOMETRY_SHADER),
    SHADER_PROGRAM_STAGE_MASK_PIXEL_SHADER      = (1 << SHADER_PROGRAM_STAGE_PIXEL_SHADER),
    SHADER_PROGRAM_STAGE_MASK_COMPUTE_SHADER    = (1 << SHADER_PROGRAM_STAGE_COMPUTE_SHADER),
};

enum SHADER_GLOBAL_FLAG
{
    SHADER_GLOBAL_FLAG_MATERIAL_TINT            = (1 << 0),
    SHADER_GLOBAL_FLAG_SHADER_QUALITY_LOW       = (1 << 1),
    SHADER_GLOBAL_FLAG_SHADER_QUALITY_MEDIUM    = (1 << 2),
    SHADER_GLOBAL_FLAG_SHADER_QUALITY_HIGH      = (1 << 3),
    SHADER_GLOBAL_FLAG_USE_VERTEX_ID_QUADS      = (1 << 4)
};

enum SHADER_PARAMETER_TYPE
{
    // Value Types
    SHADER_PARAMETER_TYPE_BOOL,
    SHADER_PARAMETER_TYPE_BOOL2,
    SHADER_PARAMETER_TYPE_BOOL3,
    SHADER_PARAMETER_TYPE_BOOL4,
    SHADER_PARAMETER_TYPE_HALF,
    SHADER_PARAMETER_TYPE_HALF2,
    SHADER_PARAMETER_TYPE_HALF3,
    SHADER_PARAMETER_TYPE_HALF4,
    SHADER_PARAMETER_TYPE_INT,
    SHADER_PARAMETER_TYPE_INT2,
    SHADER_PARAMETER_TYPE_INT3,
    SHADER_PARAMETER_TYPE_INT4,
    SHADER_PARAMETER_TYPE_UINT,
    SHADER_PARAMETER_TYPE_UINT2,
    SHADER_PARAMETER_TYPE_UINT3,
    SHADER_PARAMETER_TYPE_UINT4,
    SHADER_PARAMETER_TYPE_FLOAT,
    SHADER_PARAMETER_TYPE_FLOAT2,
    SHADER_PARAMETER_TYPE_FLOAT3,
    SHADER_PARAMETER_TYPE_FLOAT4,
    SHADER_PARAMETER_TYPE_FLOAT2X2,
    SHADER_PARAMETER_TYPE_FLOAT3X3,
    SHADER_PARAMETER_TYPE_FLOAT3X4,
    SHADER_PARAMETER_TYPE_FLOAT4X4,

    // Texture Types
    SHADER_PARAMETER_TYPE_TEXTURE1D,
    SHADER_PARAMETER_TYPE_TEXTURE1DARRAY,
    SHADER_PARAMETER_TYPE_TEXTURE2D,
    SHADER_PARAMETER_TYPE_TEXTURE2DARRAY,
    SHADER_PARAMETER_TYPE_TEXTURE2DMS,
    SHADER_PARAMETER_TYPE_TEXTURE2DMSARRAY,
    SHADER_PARAMETER_TYPE_TEXTURE3D,
    SHADER_PARAMETER_TYPE_TEXTURECUBE,
    SHADER_PARAMETER_TYPE_TEXTURECUBEARRAY,
    SHADER_PARAMETER_TYPE_TEXTUREBUFFER,

    // Resource Types
    SHADER_PARAMETER_TYPE_CONSTANT_BUFFER,
    SHADER_PARAMETER_TYPE_SAMPLER_STATE,
    SHADER_PARAMETER_TYPE_BUFFER,
    SHADER_PARAMETER_TYPE_STRUCT,

    // Count
    SHADER_PARAMETER_TYPE_COUNT,
};

namespace NameTables {
    Y_Declare_NameTable(ShaderParameterType);
}

uint32 ShaderParameterValueTypeSize(SHADER_PARAMETER_TYPE Type);
GPU_RESOURCE_TYPE ShaderParameterResourceType(SHADER_PARAMETER_TYPE Type);
template<typename T> SHADER_PARAMETER_TYPE ShaderParameterTypeFor();

template<typename T> T ShaderParameterTypeFromString(const char *StringValue);
void ShaderParameterTypeFromString(SHADER_PARAMETER_TYPE Type, void *pDestination, const char *StringValue);

template<typename T> void ShaderParameterTypeToString(String &Destination, const T &Value);
void ShaderParameterTypeToString(String &Destination, SHADER_PARAMETER_TYPE Type, const void *pValue);

//-------------------------------- SamplerState -------------------------------
struct GPU_SAMPLER_STATE_DESC
{
    TEXTURE_FILTER Filter;
    TEXTURE_ADDRESS_MODE AddressU;
    TEXTURE_ADDRESS_MODE AddressV;
    TEXTURE_ADDRESS_MODE AddressW;
    float4 BorderColor;
    float LODBias;
    int32 MinLOD;
    int32 MaxLOD;
    uint32 MaxAnisotropy;
    GPU_COMPARISON_FUNC ComparisonFunc;

    GPU_SAMPLER_STATE_DESC() {}
    GPU_SAMPLER_STATE_DESC(TEXTURE_FILTER filter, TEXTURE_ADDRESS_MODE addressU, TEXTURE_ADDRESS_MODE addressV, TEXTURE_ADDRESS_MODE addressW, const float4 &borderColor, float lodBias, int32 minLOD, int32 maxLOD, uint32 maxAnisotropy, GPU_COMPARISON_FUNC comparisonFunc) : Filter(filter), AddressU(addressU), AddressV(addressV), AddressW(addressW), BorderColor(borderColor), LODBias(lodBias), MinLOD(minLOD), MaxLOD(maxLOD), MaxAnisotropy(maxAnisotropy), ComparisonFunc(comparisonFunc) {}
    void Set(TEXTURE_FILTER filter, TEXTURE_ADDRESS_MODE addressU, TEXTURE_ADDRESS_MODE addressV, TEXTURE_ADDRESS_MODE addressW, const float4 &borderColor, float lodBias, int32 minLOD, int32 maxLOD, uint32 maxAnisotropy, GPU_COMPARISON_FUNC comparisonFunc) { Filter = filter; AddressU = addressU; AddressV = addressV; AddressW = addressW; BorderColor = borderColor; LODBias = lodBias; MinLOD = minLOD; MaxLOD = maxLOD; MaxAnisotropy = maxAnisotropy; ComparisonFunc = comparisonFunc; }
};

//-------------------------------- RasterizerState -------------------------------
enum RENDERER_FILL_MODE
{
    RENDERER_FILL_WIREFRAME,
    RENDERER_FILL_SOLID,
    RENDERER_FILL_MODE_COUNT,
};


enum RENDERER_CULL_MODE
{
    RENDERER_CULL_NONE,
    RENDERER_CULL_FRONT,
    RENDERER_CULL_BACK,
    RENDERER_CULL_MODE_COUNT,
};


struct RENDERER_RASTERIZER_STATE_DESC
{
    RENDERER_FILL_MODE FillMode;
    RENDERER_CULL_MODE CullMode;
    bool FrontCounterClockwise;
    bool ScissorEnable;
    int32 DepthBias;
    float SlopeScaledDepthBias;
    bool DepthClipEnable;
};


//-------------------------------- DepthStencilState -------------------------------
enum RENDERER_STENCIL_OP
{
    RENDERER_STENCIL_OP_KEEP,
    RENDERER_STENCIL_OP_ZERO,
    RENDERER_STENCIL_OP_REPLACE,
    RENDERER_STENCIL_OP_INCREMENT_CLAMPED,
    RENDERER_STENCIL_OP_DECREMENT_CLAMPED,
    RENDERER_STENCIL_OP_INVERT,
    RENDERER_STENCIL_OP_INCREMENT,
    RENDERER_STENCIL_OP_DECREMENT,
    RENDERER_STENCIL_OP_COUNT,
};

struct RENDERER_FACE_STENCIL_OP
{
    RENDERER_STENCIL_OP FailOp;
    RENDERER_STENCIL_OP DepthFailOp;
    RENDERER_STENCIL_OP PassOp;
    GPU_COMPARISON_FUNC CompareFunc;
};

struct RENDERER_DEPTHSTENCIL_STATE_DESC
{
    bool DepthTestEnable;
    bool DepthWriteEnable;
    GPU_COMPARISON_FUNC DepthFunc;
    bool StencilTestEnable;
    uint8 StencilReadMask;
    uint8 StencilWriteMask;
    RENDERER_FACE_STENCIL_OP StencilFrontFace;
    RENDERER_FACE_STENCIL_OP StencilBackFace;
};

//-------------------------------- BlendState -------------------------------
enum RENDERER_BLEND_OPTION
{
    RENDERER_BLEND_ZERO,
    RENDERER_BLEND_ONE,
    RENDERER_BLEND_SRC_COLOR,
    RENDERER_BLEND_INV_SRC_COLOR,
    RENDERER_BLEND_SRC_ALPHA,
    RENDERER_BLEND_INV_SRC_ALPHA,
    RENDERER_BLEND_DEST_ALPHA,
    RENDERER_BLEND_INV_DEST_ALPHA,
    RENDERER_BLEND_DEST_COLOR,
    RENDERER_BLEND_INV_DEST_COLOR,
    RENDERER_BLEND_SRC_ALPHA_SAT,
    RENDERER_BLEND_BLEND_FACTOR,
    RENDERER_BLEND_INV_BLEND_FACTOR,
    RENDERER_BLEND_SRC1_COLOR,
    RENDERER_BLEND_INV_SRC1_COLOR,
    RENDERER_BLEND_SRC1_ALPHA,
    RENDERER_BLEND_INV_SRC1_ALPHA,
    RENDERER_BLEND_OPTION_COUNT,
};

enum RENDERER_BLEND_OP
{
    RENDERER_BLEND_OP_ADD,
    RENDERER_BLEND_OP_SUBTRACT,
    RENDERER_BLEND_OP_REV_SUBTRACT,
    RENDERER_BLEND_OP_MIN,
    RENDERER_BLEND_OP_MAX,
    RENDERER_BLEND_OP_COUNT,
};

struct RENDERER_BLEND_STATE_DESC
{
    bool BlendEnable;
    RENDERER_BLEND_OPTION SrcBlend;
    RENDERER_BLEND_OP BlendOp;
    RENDERER_BLEND_OPTION DestBlend;
    RENDERER_BLEND_OPTION SrcBlendAlpha;
    RENDERER_BLEND_OP BlendOpAlpha;
    RENDERER_BLEND_OPTION DestBlendAlpha;
    bool ColorWriteEnable;
};

enum GPU_TEXTURE_FLAGS
{
    GPU_TEXTURE_FLAG_READABLE                   = (1 << 0),     // Able to be read from user code. Usually via staging buffer.
    GPU_TEXTURE_FLAG_WRITABLE                   = (1 << 1),     // Able to be written to from user code. Usually via staging buffer.
    GPU_TEXTURE_FLAG_SHADER_BINDABLE            = (1 << 2),     // Bindable to shader samplers/resources.
    GPU_TEXTURE_FLAG_BIND_RENDER_TARGET         = (1 << 3),     // Bindable as a render target.
    GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER  = (1 << 4),     // Bindable as depth stencil buffer.
    GPU_TEXTURE_FLAG_BIND_COMPUTE_WRITABLE      = (1 << 5),     // Binds to image units in OpenGL, or unordered access views in Direct3D
    GPU_TEXTURE_FLAG_GENERATE_MIPS              = (1 << 6)      // Supports real time mip generation
};

struct GPU_TEXTURE1D_DESC
{
    uint32 Width;
    PIXEL_FORMAT Format;
    uint32 Flags;
    uint32 MipLevels;

    GPU_TEXTURE1D_DESC() {}
    GPU_TEXTURE1D_DESC(uint32 width, PIXEL_FORMAT format, uint32 flags, uint32 mipLevels) : Width(width), Format(format), Flags(flags), MipLevels(mipLevels) {}
    void Set(uint32 width, PIXEL_FORMAT format, uint32 flags, uint32 mipLevels) { Width = width; Format = format; Flags = flags; MipLevels = mipLevels; }
};

struct GPU_TEXTURE1DARRAY_DESC
{
    uint32 Width;
    PIXEL_FORMAT Format;
    uint32 Flags;
    uint32 MipLevels;
    uint32 ArraySize;

    GPU_TEXTURE1DARRAY_DESC() {}
    GPU_TEXTURE1DARRAY_DESC(uint32 width, PIXEL_FORMAT format, uint32 flags, uint32 mipLevels, uint32 arraySize) : Width(width), Format(format), Flags(flags), MipLevels(mipLevels), ArraySize(arraySize) {}
    void Set(uint32 width, PIXEL_FORMAT format, uint32 flags, uint32 mipLevels, uint32 arraySize) { Width = width; Format = format; Flags = flags; MipLevels = mipLevels; ArraySize = arraySize; }
};

struct GPU_TEXTURE2D_DESC
{
    uint32 Width;
    uint32 Height;
    PIXEL_FORMAT Format;
    uint32 Flags;
    uint32 MipLevels;

    GPU_TEXTURE2D_DESC() {}
    GPU_TEXTURE2D_DESC(uint32 width, uint32 height, PIXEL_FORMAT format, uint32 flags, uint32 mipLevels) : Width(width), Height(height), Format(format), Flags(flags), MipLevels(mipLevels) {}
    void Set(uint32 width, uint32 height, PIXEL_FORMAT format, uint32 flags, uint32 mipLevels) { Width = width; Height = height; Format = format; Flags = flags; MipLevels = mipLevels; }
};

struct GPU_TEXTURE2DARRAY_DESC
{
    uint32 Width;
    uint32 Height;
    PIXEL_FORMAT Format;
    uint32 Flags;
    uint32 MipLevels;
    uint32 ArraySize;

    GPU_TEXTURE2DARRAY_DESC() {}
    GPU_TEXTURE2DARRAY_DESC(uint32 width, uint32 height, PIXEL_FORMAT format, uint32 flags, uint32 mipLevels, uint32 arraySize) : Width(width), Height(height), Format(format), Flags(flags), MipLevels(mipLevels), ArraySize(arraySize) {}
    void Set(uint32 width, uint32 height, PIXEL_FORMAT format, uint32 flags, uint32 mipLevels, uint32 arraySize) { Width = width; Height = height; Format = format; Flags = flags; MipLevels = mipLevels; ArraySize = arraySize; }
};

struct GPU_TEXTURE3D_DESC
{
    uint32 Width;
    uint32 Height;
    uint32 Depth;
    PIXEL_FORMAT Format;
    uint32 Flags;
    uint32 MipLevels;

    GPU_TEXTURE3D_DESC() {}
    GPU_TEXTURE3D_DESC(uint32 width, uint32 height, uint32 depth, PIXEL_FORMAT format, uint32 flags, uint32 mipLevels) : Width(width), Height(height), Depth(depth), Format(format), Flags(flags), MipLevels(mipLevels) {}
    void Set(uint32 width, uint32 height, uint32 depth, PIXEL_FORMAT format, uint32 flags, uint32 mipLevels) { Width = width; Height = height; Depth = depth; Format = format; Flags = flags; MipLevels = mipLevels; }
};

struct GPU_TEXTURECUBE_DESC
{
    uint32 Width;
    uint32 Height;
    PIXEL_FORMAT Format;
    uint32 Flags;
    uint32 MipLevels;

    GPU_TEXTURECUBE_DESC() {}
    GPU_TEXTURECUBE_DESC(uint32 width, uint32 height, PIXEL_FORMAT format, uint32 flags, uint32 mipLevels) : Width(width), Height(height), Format(format), Flags(flags), MipLevels(mipLevels) {}
    void Set(uint32 width, uint32 height, PIXEL_FORMAT format, uint32 flags, uint32 mipLevels) { Width = width; Height = height; Format = format; Flags = flags; MipLevels = mipLevels; }
};

struct GPU_TEXTURECUBEARRAY_DESC
{
    uint32 Width;
    uint32 Height;
    PIXEL_FORMAT Format;
    uint32 Flags;
    uint32 MipLevels;
    uint32 ArraySize;

    GPU_TEXTURECUBEARRAY_DESC() {}
    GPU_TEXTURECUBEARRAY_DESC(uint32 width, uint32 height, PIXEL_FORMAT format, uint32 flags, uint32 mipLevels, uint32 arraySize) : Width(width), Height(height), Format(format), Flags(flags), MipLevels(mipLevels), ArraySize(arraySize) {}
    void Set(uint32 width, uint32 height, PIXEL_FORMAT format, uint32 flags, uint32 mipLevels, uint32 arraySize) { Width = width; Height = height; Format = format; Flags = flags; MipLevels = mipLevels; ArraySize = arraySize; }
};

struct GPU_DEPTH_TEXTURE_DESC
{
    uint32 Width;
    uint32 Height;
    PIXEL_FORMAT Format;
    uint32 Flags;

    GPU_DEPTH_TEXTURE_DESC() {}
    GPU_DEPTH_TEXTURE_DESC(uint32 width, uint32 height, PIXEL_FORMAT format, uint32 flags) : Width(width), Height(height), Format(format), Flags(flags) {}
    void Set(uint32 width, uint32 height, PIXEL_FORMAT format, uint32 flags) { Width = width; Height = height; Format = format; Flags = flags; }
};

enum GPU_BUFFER_FLAGS
{
    GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER = (1 << 0),
    GPU_BUFFER_FLAG_BIND_INDEX_BUFFER = (1 << 1),
    GPU_BUFFER_FLAG_BIND_CONSTANT_BUFFER = (1 << 2),
    GPU_BUFFER_FLAG_READABLE = (1 << 4),
    GPU_BUFFER_FLAG_WRITABLE = (1 << 5),
    GPU_BUFFER_FLAG_MAPPABLE = (1 << 6),
};

struct GPU_BUFFER_DESC
{
    uint32 Flags;
    uint32 Size;

    GPU_BUFFER_DESC() {}
    GPU_BUFFER_DESC(uint32 flags, uint32 size) : Flags(flags), Size(size) {}
};

struct GPU_RENDER_TARGET_VIEW_DESC
{
    GPU_RENDER_TARGET_VIEW_DESC() {}
    GPU_RENDER_TARGET_VIEW_DESC(uint32 mipLevel, uint32 firstLayerIndex, uint32 numLayers, PIXEL_FORMAT format) : MipLevel(mipLevel), FirstLayerIndex(firstLayerIndex), NumLayers(numLayers), Format(format) {}
    GPU_RENDER_TARGET_VIEW_DESC(GPUTexture1D *pTexture, uint32 mipLevel = 0);
    GPU_RENDER_TARGET_VIEW_DESC(GPUTexture1DArray *pTexture, uint32 mipLevel = 0);
    GPU_RENDER_TARGET_VIEW_DESC(GPUTexture1DArray *pTexture, uint32 mipLevel, uint32 arrayIndex);
    GPU_RENDER_TARGET_VIEW_DESC(GPUTexture2D *pTexture, uint32 mipLevel = 0);
    GPU_RENDER_TARGET_VIEW_DESC(GPUTexture2DArray *pTexture, uint32 mipLevel = 0);
    GPU_RENDER_TARGET_VIEW_DESC(GPUTexture2DArray *pTexture, uint32 mipLevel, uint32 arrayIndex);
    GPU_RENDER_TARGET_VIEW_DESC(GPUTexture3D *pTexture, uint32 mipLevel, uint32 slideIndex);
    GPU_RENDER_TARGET_VIEW_DESC(GPUTextureCube *pTexture, uint32 mipLevel = 0);
    GPU_RENDER_TARGET_VIEW_DESC(GPUTextureCube *pTexture, uint32 mipLevel, CUBE_FACE faceIndex);
    GPU_RENDER_TARGET_VIEW_DESC(GPUTextureCubeArray *pTexture, uint32 mipLevel, uint32 arrayIndex);
    GPU_RENDER_TARGET_VIEW_DESC(GPUTextureCubeArray *pTexture, uint32 mipLevel, uint32 arrayIndex, CUBE_FACE faceIndex);
    GPU_RENDER_TARGET_VIEW_DESC(GPUDepthTexture *pTexture);

    void Set(uint32 mipLevel, uint32 firstLayerIndex, uint32 numLayers, PIXEL_FORMAT format) { MipLevel = mipLevel; FirstLayerIndex = firstLayerIndex; NumLayers = numLayers; Format = format; }

    uint32 MipLevel;
    uint32 FirstLayerIndex;
    uint32 NumLayers;
    PIXEL_FORMAT Format;
};

struct GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC
{
    GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC() {}
    GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(uint32 mipLevel, uint32 firstLayerIndex, uint32 numLayers, PIXEL_FORMAT format) : MipLevel(mipLevel), FirstLayerIndex(firstLayerIndex), NumLayers(numLayers), Format(format) {}
    GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUTexture2D *pTexture, uint32 mipLevel = 0);
    GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUTexture2DArray *pTexture, uint32 mipLevel = 0);
    GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUTexture2DArray *pTexture, uint32 mipLevel, uint32 arrayIndex);
    GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUTextureCube *pTexture, uint32 mipLevel = 0);
    GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUTextureCube *pTexture, uint32 mipLevel, CUBE_FACE faceIndex);
    GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUTextureCubeArray *pTexture, uint32 mipLevel, uint32 arrayIndex);
    GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUTextureCubeArray *pTexture, uint32 mipLevel, uint32 arrayIndex, CUBE_FACE faceIndex);
    GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUDepthTexture *pTexture);
    
    void Set(uint32 mipLevel, uint32 firstLayerIndex, uint32 numLayers, PIXEL_FORMAT format) { MipLevel = mipLevel; FirstLayerIndex = firstLayerIndex; NumLayers = numLayers; Format = format; }

    uint32 MipLevel;
    uint32 FirstLayerIndex;
    uint32 NumLayers;
    PIXEL_FORMAT Format;
};

struct GPU_COMPUTE_VIEW_DESC
{
    GPU_COMPUTE_VIEW_DESC() {}

    union
    {
        struct
        {
            uint32 StartOffset;
            uint32 Stride;
        } Buffer;

        struct
        {
            uint32 MipLevel;
            uint32 FirstLayerIndex;
            uint32 NumLayers;
        } Texture;
    };
};

enum RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER
{
    RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_NEAREST,
    RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_LINEAR,
    RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_COUNT,
};

enum RENDERER_FRAMEBUFFER_BLIT_BLEND_MODE
{
    RENDERER_FRAMEBUFFER_BLIT_BLEND_MODE_NONE,
    RENDERER_FRAMEBUFFER_BLIT_BLEND_MODE_ADDITIVE,
    RENDERER_FRAMEBUFFER_BLIT_BLEND_MODE_ALPHA_BLENDING,
    NUM_RENDERER_FRAMEBUFFER_BLIT_BLEND_MODES
};

// World renderer stuff
enum RENDERER_FOG_MODE
{
    RENDERER_FOG_MODE_NONE,
    RENDERER_FOG_MODE_LINEAR,
    RENDERER_FOG_MODE_EXP,
    RENDERER_FOG_MODE_EXP2,
    RENDERER_FOG_MODE_COUNT,
};

enum RENDERER_SHADOW_FILTER
{
    RENDERER_SHADOW_FILTER_1X1,
    RENDERER_SHADOW_FILTER_3X3,
    RENDERER_SHADOW_FILTER_5X5,
    NUM_RENDERER_SHADOW_FILTERS
};

namespace NameTables {
    Y_Declare_NameTable(RendererFogMode);
}

// Nametables
namespace NameTables {
    Y_Declare_NameTable(RendererPlatform);
    Y_Declare_NameTable(RendererPlatformFullName);
    Y_Declare_NameTable(RendererFeatureLevel);
    Y_Declare_NameTable(RendererFeatureLevelFullName);
    Y_Declare_NameTable(RendererComparisonFunc);
    Y_Declare_NameTable(GPUVertexElementType);
    Y_Declare_NameTable(GPUVertexElementSemantic);
    Y_Declare_NameTable(ShaderProgramStage);
    Y_Declare_NameTable(RendererFillModes);
    Y_Declare_NameTable(RendererCullModes);
    Y_Declare_NameTable(RendererBlendOptions);
    Y_Declare_NameTable(RendererBlendOps);
}

