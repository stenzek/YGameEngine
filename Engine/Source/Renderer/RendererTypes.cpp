#include "Renderer/PrecompiledHeader.h"
#include "Renderer/RendererTypes.h"
#include "Renderer/Renderer.h"

int32 DrawTopology_CalculateNumVertices(DRAW_TOPOLOGY Topology, int32 nPrimitives)
{
    switch (Topology)
    {
    case DRAW_TOPOLOGY_POINTS:
        return nPrimitives;

    case DRAW_TOPOLOGY_LINE_LIST:
        return nPrimitives * 2;

    case DRAW_TOPOLOGY_LINE_STRIP:
        return nPrimitives + 1;

    case DRAW_TOPOLOGY_TRIANGLE_LIST:
        return nPrimitives * 3;

    case DRAW_TOPOLOGY_TRIANGLE_STRIP:
        return nPrimitives + 2;
    }

    UnreachableCode();
    return 0;
}

int32 DrawTopology_CalculateNumPrimitives(DRAW_TOPOLOGY Topology, int32 nVertices)
{
    switch (Topology)
    {
    case DRAW_TOPOLOGY_POINTS:
        return nVertices;

    case DRAW_TOPOLOGY_LINE_LIST:
        return nVertices / 2;

    case DRAW_TOPOLOGY_LINE_STRIP:
        return nVertices - 1;

    case DRAW_TOPOLOGY_TRIANGLE_LIST:
        return nVertices / 3;

    case DRAW_TOPOLOGY_TRIANGLE_STRIP:
        return nVertices - 2;
    }

    UnreachableCode();
    return 0;
}

Y_Define_NameTable(NameTables::GPUResourceType)
    Y_NameTable_Entry("RasterizerState", GPU_RESOURCE_TYPE_RASTERIZER_STATE)
    Y_NameTable_Entry("DepthStencilState", GPU_RESOURCE_TYPE_DEPTH_STENCIL_STATE)
    Y_NameTable_Entry("BlendState", GPU_RESOURCE_TYPE_BLEND_STATE)
    Y_NameTable_Entry("SamplerState", GPU_RESOURCE_TYPE_SAMPLER_STATE)
    Y_NameTable_Entry("Buffer", GPU_RESOURCE_TYPE_BUFFER)
    Y_NameTable_Entry("Texture1D", GPU_RESOURCE_TYPE_TEXTURE1D)
    Y_NameTable_Entry("Texture1DArray", GPU_RESOURCE_TYPE_TEXTURE1DARRAY)
    Y_NameTable_Entry("Texture2D", GPU_RESOURCE_TYPE_TEXTURE2D)
    Y_NameTable_Entry("Texture2DArray", GPU_RESOURCE_TYPE_TEXTURE2DARRAY)
    Y_NameTable_Entry("Texture3D", GPU_RESOURCE_TYPE_TEXTURE3D)
    Y_NameTable_Entry("TextureCube", GPU_RESOURCE_TYPE_TEXTURECUBE)
    Y_NameTable_Entry("TextureCubeArray", GPU_RESOURCE_TYPE_TEXTURECUBEARRAY)
    Y_NameTable_Entry("TextureBuffer", GPU_RESOURCE_TYPE_TEXTUREBUFFER)
    Y_NameTable_Entry("DepthTexture", GPU_RESOURCE_TYPE_DEPTH_TEXTURE)
    Y_NameTable_Entry("Query", GPU_RESOURCE_TYPE_QUERY)
    Y_NameTable_Entry("ShaderProgram", GPU_RESOURCE_TYPE_SHADER_PROGRAM)
Y_NameTable_End()

Y_Define_NameTable(NameTables::ShaderParameterType)
    Y_NameTable_Entry("bool", SHADER_PARAMETER_TYPE_BOOL)
    Y_NameTable_Entry("bool2", SHADER_PARAMETER_TYPE_BOOL2)
    Y_NameTable_Entry("bool3", SHADER_PARAMETER_TYPE_BOOL3)
    Y_NameTable_Entry("bool4", SHADER_PARAMETER_TYPE_BOOL4)
    Y_NameTable_Entry("half", SHADER_PARAMETER_TYPE_HALF)
    Y_NameTable_Entry("half2", SHADER_PARAMETER_TYPE_HALF2)
    Y_NameTable_Entry("half3", SHADER_PARAMETER_TYPE_HALF3)
    Y_NameTable_Entry("half4", SHADER_PARAMETER_TYPE_HALF4)
    Y_NameTable_Entry("int", SHADER_PARAMETER_TYPE_INT)
    Y_NameTable_Entry("int2", SHADER_PARAMETER_TYPE_INT2)
    Y_NameTable_Entry("int3", SHADER_PARAMETER_TYPE_INT3)
    Y_NameTable_Entry("int4", SHADER_PARAMETER_TYPE_INT4)
    Y_NameTable_Entry("uint", SHADER_PARAMETER_TYPE_UINT)
    Y_NameTable_Entry("uint2", SHADER_PARAMETER_TYPE_UINT2)
    Y_NameTable_Entry("uint3", SHADER_PARAMETER_TYPE_UINT3)
    Y_NameTable_Entry("uint4", SHADER_PARAMETER_TYPE_UINT4)
    Y_NameTable_Entry("float", SHADER_PARAMETER_TYPE_FLOAT)
    Y_NameTable_Entry("float2", SHADER_PARAMETER_TYPE_FLOAT2)
    Y_NameTable_Entry("float3", SHADER_PARAMETER_TYPE_FLOAT3)
    Y_NameTable_Entry("float4", SHADER_PARAMETER_TYPE_FLOAT4)
    Y_NameTable_Entry("float2x2", SHADER_PARAMETER_TYPE_FLOAT2X2)
    Y_NameTable_Entry("float3x3", SHADER_PARAMETER_TYPE_FLOAT3X3)
    Y_NameTable_Entry("float3x4", SHADER_PARAMETER_TYPE_FLOAT3X4)
    Y_NameTable_Entry("float4x4", SHADER_PARAMETER_TYPE_FLOAT4X4)
    Y_NameTable_Entry("Texture1D", SHADER_PARAMETER_TYPE_TEXTURE1D)
    Y_NameTable_Entry("Texture1DArray", SHADER_PARAMETER_TYPE_TEXTURE1DARRAY)
    Y_NameTable_Entry("Texture2D", SHADER_PARAMETER_TYPE_TEXTURE2D)
    Y_NameTable_Entry("Texture2DArray", SHADER_PARAMETER_TYPE_TEXTURE2DARRAY)
    Y_NameTable_Entry("Texture2DMS", SHADER_PARAMETER_TYPE_TEXTURE2DMS)
    Y_NameTable_Entry("Texture2DMSArray", SHADER_PARAMETER_TYPE_TEXTURE2DMSARRAY)
    Y_NameTable_Entry("Texture3D", SHADER_PARAMETER_TYPE_TEXTURE3D)
    Y_NameTable_Entry("TextureCube", SHADER_PARAMETER_TYPE_TEXTURECUBE)
    Y_NameTable_Entry("TextureCubeArray", SHADER_PARAMETER_TYPE_TEXTURECUBEARRAY)
    Y_NameTable_Entry("TextureBuffer", SHADER_PARAMETER_TYPE_TEXTUREBUFFER)
    Y_NameTable_Entry("ConstantBuffer", SHADER_PARAMETER_TYPE_CONSTANT_BUFFER)
    Y_NameTable_Entry("SamplerState", SHADER_PARAMETER_TYPE_SAMPLER_STATE)
    Y_NameTable_Entry("Buffer", SHADER_PARAMETER_TYPE_BUFFER)
    Y_NameTable_Entry("Struct", SHADER_PARAMETER_TYPE_STRUCT)
Y_NameTable_End()

uint32 ShaderParameterValueTypeSize(SHADER_PARAMETER_TYPE Type)
{
    static const uint32 Sizes[SHADER_PARAMETER_TYPE_COUNT] =
    {
        4,      // SHADER_PARAMETER_TYPE_BOOL
        8,      // SHADER_PARAMETER_TYPE_BOOL2
        12,     // SHADER_PARAMETER_TYPE_BOOL3
        16,     // SHADER_PARAMETER_TYPE_BOOL4
        2,      // SHADER_PARAMETER_TYPE_HALF
        4,      // SHADER_PARAMETER_TYPE_HALF2
        6,      // SHADER_PARAMETER_TYPE_HALF3
        8,      // SHADER_PARAMETER_TYPE_HALF4
        4,      // SHADER_PARAMETER_TYPE_INT
        8,      // SHADER_PARAMETER_TYPE_INT2
        12,     // SHADER_PARAMETER_TYPE_INT3
        16,     // SHADER_PARAMETER_TYPE_INT4
        4,      // SHADER_PARAMETER_TYPE_UINT
        8,      // SHADER_PARAMETER_TYPE_UINT2
        12,     // SHADER_PARAMETER_TYPE_UINT3
        16,     // SHADER_PARAMETER_TYPE_UINT4
        4,      // SHADER_PARAMETER_TYPE_FLOAT
        8,      // SHADER_PARAMETER_TYPE_FLOAT2
        12,     // SHADER_PARAMETER_TYPE_FLOAT3
        16,     // SHADER_PARAMETER_TYPE_FLOAT4
        16,     // SHADER_PARAMETER_TYPE_FLOAT2X2
        36,     // SHADER_PARAMETER_TYPE_FLOAT3X3
        48,     // SHADER_PARAMETER_TYPE_FLOAT3X4
        64,     // SHADER_PARAMETER_TYPE_FLOAT4X4
        0,      // SHADER_PARAMETER_TYPE_TEXTURE1D
        0,      // SHADER_PARAMETER_TYPE_TEXTURE1DARRAY
        0,      // SHADER_PARAMETER_TYPE_TEXTURE2D
        0,      // SHADER_PARAMETER_TYPE_TEXTURE2DARRAY
        0,      // SHADER_PARAMETER_TYPE_TEXTURE2DMS
        0,      // SHADER_PARAMETER_TYPE_TEXTURE2DMSARRAY
        0,      // SHADER_PARAMETER_TYPE_TEXTURE3D
        0,      // SHADER_PARAMETER_TYPE_TEXTURECUBE
        0,      // SHADER_PARAMETER_TYPE_TEXTURECUBEARRAY
        0,      // SHADER_PARAMETER_TYPE_TEXTUREBUFFER
        0,      // SHADER_PARAMETER_TYPE_CONSTANT_BUFFER
        0,      // SHADER_PARAMETER_TYPE_SAMPLER_STATE
        0,      // SHADER_PARAMETER_TYPE_BUFFER
        0       // SHADER_PARAMETER_TYPE_STRUCT
    };

    DebugAssert(Type < SHADER_PARAMETER_TYPE_COUNT);
    return Sizes[Type];
}

GPU_RESOURCE_TYPE ShaderParameterResourceType(SHADER_PARAMETER_TYPE Type)
{
    static const GPU_RESOURCE_TYPE resourceTypes[SHADER_PARAMETER_TYPE_COUNT] =
    {
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_BOOL
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_BOOL2
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_BOOL3
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_BOOL4
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_HALF
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_HALF2
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_HALF3
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_HALF4
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_INT
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_INT2
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_INT3
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_INT4
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_UINT
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_UINT2
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_UINT3
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_UINT4
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_FLOAT
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_FLOAT2
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_FLOAT3
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_FLOAT4
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_FLOAT2X2
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_FLOAT3X3
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_FLOAT3X4
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_FLOAT4X4
        GPU_RESOURCE_TYPE_TEXTURE1D,        // SHADER_PARAMETER_TYPE_TEXTURE1D
        GPU_RESOURCE_TYPE_TEXTURE1DARRAY,   // SHADER_PARAMETER_TYPE_TEXTURE1DARRAY
        GPU_RESOURCE_TYPE_TEXTURE2D,        // SHADER_PARAMETER_TYPE_TEXTURE2D
        GPU_RESOURCE_TYPE_TEXTURE2DARRAY,   // SHADER_PARAMETER_TYPE_TEXTURE2DARRAY
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_TEXTURE2DMS
        GPU_RESOURCE_TYPE_COUNT,            // SHADER_PARAMETER_TYPE_TEXTURE2DMSARRAY
        GPU_RESOURCE_TYPE_TEXTURE3D,        // SHADER_PARAMETER_TYPE_TEXTURE3D
        GPU_RESOURCE_TYPE_TEXTURECUBE,      // SHADER_PARAMETER_TYPE_TEXTURECUBE
        GPU_RESOURCE_TYPE_TEXTURECUBEARRAY, // SHADER_PARAMETER_TYPE_TEXTURECUBEARRAY
        GPU_RESOURCE_TYPE_TEXTUREBUFFER,    // SHADER_PARAMETER_TYPE_TEXTUREBUFFER
        GPU_RESOURCE_TYPE_BUFFER,           // SHADER_PARAMETER_TYPE_CONSTANT_BUFFER
        GPU_RESOURCE_TYPE_SAMPLER_STATE,    // SHADER_PARAMETER_TYPE_SAMPLER_STATE
        GPU_RESOURCE_TYPE_BUFFER,           // SHADER_PARAMETER_TYPE_BUFFER
        GPU_RESOURCE_TYPE_COUNT             // SHADER_PARAMETER_TYPE_STRUCT
    };

    DebugAssert(Type < SHADER_PARAMETER_TYPE_COUNT);
    return resourceTypes[Type];
}

template<> SHADER_PARAMETER_TYPE ShaderParameterTypeFor<bool>() { return SHADER_PARAMETER_TYPE_BOOL; }
template<> SHADER_PARAMETER_TYPE ShaderParameterTypeFor<int32>() { return SHADER_PARAMETER_TYPE_INT; }
template<> SHADER_PARAMETER_TYPE ShaderParameterTypeFor<float>() { return SHADER_PARAMETER_TYPE_FLOAT; }
template<> SHADER_PARAMETER_TYPE ShaderParameterTypeFor<float2>() { return SHADER_PARAMETER_TYPE_FLOAT2; }
template<> SHADER_PARAMETER_TYPE ShaderParameterTypeFor<float3>() { return SHADER_PARAMETER_TYPE_FLOAT3; }
template<> SHADER_PARAMETER_TYPE ShaderParameterTypeFor<float4>() { return SHADER_PARAMETER_TYPE_FLOAT4; }
template<> SHADER_PARAMETER_TYPE ShaderParameterTypeFor<float4x4>() { return SHADER_PARAMETER_TYPE_FLOAT4X4; }

template<> bool ShaderParameterTypeFromString<bool>(const char *StringValue) { return StringConverter::StringToBool(StringValue); }
template<> int32 ShaderParameterTypeFromString<int32>(const char *StringValue) { return StringConverter::StringToInt32(StringValue); }
template<> float ShaderParameterTypeFromString<float>(const char *StringValue) { return StringConverter::StringToFloat(StringValue); }
template<> float2 ShaderParameterTypeFromString<float2>(const char *StringValue) { return StringConverter::StringToVector2f(StringValue); }
template<> float3 ShaderParameterTypeFromString<float3>(const char *StringValue) { return StringConverter::StringToVector3f(StringValue); }
template<> float4 ShaderParameterTypeFromString<float4>(const char *StringValue) { return StringConverter::StringToVector4f(StringValue); }

void ShaderParameterTypeFromString(SHADER_PARAMETER_TYPE Type, void *pDestination, const char *StringValue)
{
    switch (Type)
    {
    case SHADER_PARAMETER_TYPE_BOOL:
        *reinterpret_cast<bool *>(pDestination) = StringConverter::StringToBool(StringValue);
        break;

    case SHADER_PARAMETER_TYPE_INT:
        *reinterpret_cast<int32 *>(pDestination) = StringConverter::StringToInt32(StringValue);
        break;

    case SHADER_PARAMETER_TYPE_FLOAT:
        *reinterpret_cast<float *>(pDestination) = StringConverter::StringToFloat(StringValue);
        break;

    case SHADER_PARAMETER_TYPE_FLOAT2:
        *reinterpret_cast<float2 *>(pDestination) = StringConverter::StringToVector2f(StringValue);
        break;

    case SHADER_PARAMETER_TYPE_FLOAT3:
        *reinterpret_cast<float3 *>(pDestination) = StringConverter::StringToVector3f(StringValue);
        break;

    case SHADER_PARAMETER_TYPE_FLOAT4:
        *reinterpret_cast<float4 *>(pDestination) = StringConverter::StringToVector4f(StringValue);
        break;

    default:
        UnreachableCode();
        break;
    }
}

template<> void ShaderParameterTypeToString<bool>(String &Destination, const bool &Value) { return StringConverter::BoolToString(Destination, Value); }
template<> void ShaderParameterTypeToString<int32>(String &Destination, const int32 &Value) { return StringConverter::Int32ToString(Destination, Value); }
template<> void ShaderParameterTypeToString<float>(String &Destination, const float &Value) { return StringConverter::FloatToString(Destination, Value); }
template<> void ShaderParameterTypeToString<float2>(String &Destination, const float2 &Value) { return StringConverter::Vector2fToString(Destination, Value); }
template<> void ShaderParameterTypeToString<float3>(String &Destination, const float3 &Value) { return StringConverter::Vector3fToString(Destination, Value); }
template<> void ShaderParameterTypeToString<float4>(String &Destination, const float4 &Value) { return StringConverter::Vector4fToString(Destination, Value); }

void ShaderParameterTypeToString(String &Destination, SHADER_PARAMETER_TYPE Type, const void *pValue)
{
    switch (Type)
    {
    case SHADER_PARAMETER_TYPE_BOOL:
        StringConverter::BoolToString(Destination, *reinterpret_cast<const bool *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_INT:
        StringConverter::Int32ToString(Destination, *reinterpret_cast<const int32 *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT:
        StringConverter::FloatToString(Destination, *reinterpret_cast<const float *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT2:
        StringConverter::Vector2fToString(Destination, *reinterpret_cast<const float2 *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT3:
        StringConverter::Vector3fToString(Destination, *reinterpret_cast<const float3 *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT4:
        StringConverter::Vector4fToString(Destination, *reinterpret_cast<const float4 *>(pValue));
        break;

    default:
        UnreachableCode();
        break;
    }
}

Y_Define_NameTable(NameTables::GPUVertexElementType)
    Y_NameTable_Entry("byte",               GPU_VERTEX_ELEMENT_TYPE_BYTE)
    Y_NameTable_Entry("byte2",              GPU_VERTEX_ELEMENT_TYPE_BYTE2)
    Y_NameTable_Entry("byte4",              GPU_VERTEX_ELEMENT_TYPE_BYTE4)
    Y_NameTable_Entry("ubyte",              GPU_VERTEX_ELEMENT_TYPE_UBYTE)
    Y_NameTable_Entry("ubyte2",             GPU_VERTEX_ELEMENT_TYPE_UBYTE2)
    Y_NameTable_Entry("ubyte4",             GPU_VERTEX_ELEMENT_TYPE_UBYTE4)
    Y_NameTable_Entry("half",               GPU_VERTEX_ELEMENT_TYPE_HALF)
    Y_NameTable_Entry("half2",              GPU_VERTEX_ELEMENT_TYPE_HALF2)
    Y_NameTable_Entry("half4",              GPU_VERTEX_ELEMENT_TYPE_HALF4)
    Y_NameTable_Entry("float",              GPU_VERTEX_ELEMENT_TYPE_FLOAT)
    Y_NameTable_Entry("float2",             GPU_VERTEX_ELEMENT_TYPE_FLOAT2)
    Y_NameTable_Entry("float3",             GPU_VERTEX_ELEMENT_TYPE_FLOAT3)
    Y_NameTable_Entry("float4",             GPU_VERTEX_ELEMENT_TYPE_FLOAT4)
    Y_NameTable_Entry("int",                GPU_VERTEX_ELEMENT_TYPE_INT)
    Y_NameTable_Entry("int2",               GPU_VERTEX_ELEMENT_TYPE_INT2)
    Y_NameTable_Entry("int3",               GPU_VERTEX_ELEMENT_TYPE_INT3)
    Y_NameTable_Entry("int4",               GPU_VERTEX_ELEMENT_TYPE_INT4)
    Y_NameTable_Entry("uint",               GPU_VERTEX_ELEMENT_TYPE_UINT)
    Y_NameTable_Entry("uint2",              GPU_VERTEX_ELEMENT_TYPE_UINT2)
    Y_NameTable_Entry("uint3",              GPU_VERTEX_ELEMENT_TYPE_UINT3)
    Y_NameTable_Entry("uint4",              GPU_VERTEX_ELEMENT_TYPE_UINT4)
    Y_NameTable_Entry("color",              GPU_VERTEX_ELEMENT_TYPE_UNORM4)
Y_NameTable_End()

Y_Define_NameTable(NameTables::GPUVertexElementSemantic)
    Y_NameTable_Entry("POSITION",           GPU_VERTEX_ELEMENT_SEMANTIC_POSITION)
    Y_NameTable_Entry("TEXCOORD",           GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD)
    Y_NameTable_Entry("COLOR",              GPU_VERTEX_ELEMENT_SEMANTIC_COLOR)
    Y_NameTable_Entry("TANGENT",            GPU_VERTEX_ELEMENT_SEMANTIC_TANGENT)
    Y_NameTable_Entry("BINORMAL",           GPU_VERTEX_ELEMENT_SEMANTIC_BINORMAL)
    Y_NameTable_Entry("NORMAL",             GPU_VERTEX_ELEMENT_SEMANTIC_NORMAL)
    Y_NameTable_Entry("POINTSIZE",          GPU_VERTEX_ELEMENT_SEMANTIC_POINTSIZE)
    Y_NameTable_Entry("BLENDINDICES",       GPU_VERTEX_ELEMENT_SEMANTIC_BLENDINDICES)
    Y_NameTable_Entry("BLENDWEIGHTS",       GPU_VERTEX_ELEMENT_SEMANTIC_BLENDWEIGHTS)
Y_NameTable_End()

Y_Define_NameTable(NameTables::RendererFogMode)
    Y_NameTable_Entry("NONE",               RENDERER_FOG_MODE_NONE)
    Y_NameTable_Entry("LINEAR",             RENDERER_FOG_MODE_LINEAR)
    Y_NameTable_Entry("EXP",                RENDERER_FOG_MODE_EXP)
    Y_NameTable_Entry("EXP2",               RENDERER_FOG_MODE_EXP2)
Y_NameTable_End()

Y_Define_NameTable(NameTables::RendererPlatform)
    Y_NameTable_Entry("D3D11",                  RENDERER_PLATFORM_D3D11)
    Y_NameTable_Entry("D3D12",                  RENDERER_PLATFORM_D3D12)
    Y_NameTable_Entry("OPENGL",                 RENDERER_PLATFORM_OPENGL)
    Y_NameTable_Entry("OPENGLES2",              RENDERER_PLATFORM_OPENGLES2)
Y_NameTable_End()

Y_Define_NameTable(NameTables::RendererPlatformFullName)
    Y_NameTable_Entry("Direct3D 11",            RENDERER_PLATFORM_D3D11)
    Y_NameTable_Entry("Direct3D 12",            RENDERER_PLATFORM_D3D12)
    Y_NameTable_Entry("OpenGL",                 RENDERER_PLATFORM_OPENGL)
    Y_NameTable_Entry("OpenGL ES 2",            RENDERER_PLATFORM_OPENGLES2)
Y_NameTable_End()

Y_Define_NameTable(NameTables::RendererFeatureLevel)
    Y_NameTable_Entry("ES2",                    RENDERER_FEATURE_LEVEL_ES2)
    Y_NameTable_Entry("ES3",                    RENDERER_FEATURE_LEVEL_ES3)
    Y_NameTable_Entry("SM4",                    RENDERER_FEATURE_LEVEL_SM4)
    Y_NameTable_Entry("SM5",                    RENDERER_FEATURE_LEVEL_SM5)
Y_NameTable_End()

Y_Define_NameTable(NameTables::RendererFeatureLevelFullName)
    Y_NameTable_Entry("OpenGL ES 2.0",          RENDERER_FEATURE_LEVEL_ES2)
    Y_NameTable_Entry("OpenGL ES 3.0",          RENDERER_FEATURE_LEVEL_ES3)
    Y_NameTable_Entry("Shader Model 4",         RENDERER_FEATURE_LEVEL_SM4)
    Y_NameTable_Entry("Shader Model 5",         RENDERER_FEATURE_LEVEL_SM5)
Y_NameTable_End()

Y_Define_NameTable(NameTables::ShaderProgramStage)
    Y_NameTable_Entry("VertexShader",       SHADER_PROGRAM_STAGE_VERTEX_SHADER)
    Y_NameTable_Entry("DomainShader",       SHADER_PROGRAM_STAGE_DOMAIN_SHADER)
    Y_NameTable_Entry("HullShader",         SHADER_PROGRAM_STAGE_HULL_SHADER)
    Y_NameTable_Entry("GeometryShader",     SHADER_PROGRAM_STAGE_GEOMETRY_SHADER)
    Y_NameTable_Entry("PixelShader",        SHADER_PROGRAM_STAGE_PIXEL_SHADER)
    Y_NameTable_Entry("ComputeShader",      SHADER_PROGRAM_STAGE_COMPUTE_SHADER)
Y_NameTable_End()

Y_Define_NameTable(NameTables::RendererFillModes)
    Y_NameTable_Entry("Wireframe",          RENDERER_FILL_WIREFRAME)
    Y_NameTable_Entry("Solid",              RENDERER_FILL_SOLID)
Y_NameTable_End()

Y_Define_NameTable(NameTables::RendererCullModes)
    Y_NameTable_Entry("None",               RENDERER_CULL_NONE)
    Y_NameTable_Entry("Front",              RENDERER_CULL_FRONT)
    Y_NameTable_Entry("Back",               RENDERER_CULL_BACK)
Y_NameTable_End()

Y_Define_NameTable(NameTables::RendererComparisonFunc)
    Y_NameTable_Entry("Never",              GPU_COMPARISON_FUNC_NEVER)
    Y_NameTable_Entry("Less",               GPU_COMPARISON_FUNC_LESS)
    Y_NameTable_Entry("Equal",              GPU_COMPARISON_FUNC_EQUAL)
    Y_NameTable_Entry("LessEqual",          GPU_COMPARISON_FUNC_LESS_EQUAL)
    Y_NameTable_Entry("Greater",            GPU_COMPARISON_FUNC_GREATER)
    Y_NameTable_Entry("NotEqual",           GPU_COMPARISON_FUNC_NOT_EQUAL)
    Y_NameTable_Entry("GreaterEqual",       GPU_COMPARISON_FUNC_GREATER_EQUAL)
    Y_NameTable_Entry("Always",             GPU_COMPARISON_FUNC_ALWAYS)
Y_NameTable_End()

Y_Define_NameTable(NameTables::RendererBlendOptions)
    Y_NameTable_Entry("Zero",               RENDERER_BLEND_ZERO)
    Y_NameTable_Entry("One",                RENDERER_BLEND_ONE)
    Y_NameTable_Entry("SrcColor",           RENDERER_BLEND_SRC_COLOR)
    Y_NameTable_Entry("InvSrcColor",        RENDERER_BLEND_INV_SRC_COLOR)
    Y_NameTable_Entry("SrcAlpha",           RENDERER_BLEND_SRC_ALPHA)
    Y_NameTable_Entry("InvSrcAlpha",        RENDERER_BLEND_INV_SRC_ALPHA)
    Y_NameTable_Entry("DestAlpha",          RENDERER_BLEND_DEST_ALPHA)
    Y_NameTable_Entry("InvDestAlpha",       RENDERER_BLEND_INV_DEST_ALPHA)
    Y_NameTable_Entry("DestColor",          RENDERER_BLEND_DEST_COLOR)
    Y_NameTable_Entry("InvDestColor",       RENDERER_BLEND_INV_DEST_COLOR)
    Y_NameTable_Entry("SrcAlphaSat",        RENDERER_BLEND_SRC_ALPHA_SAT)
    Y_NameTable_Entry("BlendFactor",        RENDERER_BLEND_BLEND_FACTOR)
    Y_NameTable_Entry("InvBlendFactor",     RENDERER_BLEND_INV_BLEND_FACTOR)
    Y_NameTable_Entry("Src1Color",          RENDERER_BLEND_SRC1_COLOR)
    Y_NameTable_Entry("InvSrc1Color",       RENDERER_BLEND_INV_SRC1_COLOR)
    Y_NameTable_Entry("Src1Alpha",          RENDERER_BLEND_SRC1_ALPHA)
    Y_NameTable_Entry("InvSrc1Alpha",       RENDERER_BLEND_INV_SRC1_ALPHA)
Y_NameTable_End()

Y_Define_NameTable(NameTables::RendererBlendOps)
    Y_NameTable_Entry("Add",                RENDERER_BLEND_OP_ADD)
    Y_NameTable_Entry("Subtract",           RENDERER_BLEND_OP_SUBTRACT)
    Y_NameTable_Entry("RevSubtract",        RENDERER_BLEND_OP_REV_SUBTRACT)
    Y_NameTable_Entry("Min",                RENDERER_BLEND_OP_MIN)
    Y_NameTable_Entry("Max",                RENDERER_BLEND_OP_MAX)
Y_NameTable_End()

GPU_RENDER_TARGET_VIEW_DESC::GPU_RENDER_TARGET_VIEW_DESC(GPUTexture1D *pTexture, uint32 mipLevel /*= 0*/)
    : MipLevel(mipLevel), FirstLayerIndex(0), NumLayers(1)
{

}

GPU_RENDER_TARGET_VIEW_DESC::GPU_RENDER_TARGET_VIEW_DESC(GPUTexture1DArray *pTexture, uint32 mipLevel /*= 0*/)
    : MipLevel(mipLevel), FirstLayerIndex(0), NumLayers(pTexture->GetDesc()->ArraySize)
{

}

GPU_RENDER_TARGET_VIEW_DESC::GPU_RENDER_TARGET_VIEW_DESC(GPUTexture1DArray *pTexture, uint32 mipLevel, uint32 arrayIndex)
    : MipLevel(mipLevel), FirstLayerIndex(arrayIndex), NumLayers(1)
{

}

GPU_RENDER_TARGET_VIEW_DESC::GPU_RENDER_TARGET_VIEW_DESC(GPUTexture2D *pTexture, uint32 mipLevel /*= 0*/)
    : MipLevel(mipLevel), FirstLayerIndex(0), NumLayers(1)
{

}

GPU_RENDER_TARGET_VIEW_DESC::GPU_RENDER_TARGET_VIEW_DESC(GPUTexture2DArray *pTexture, uint32 mipLevel /*= 0*/)
    : MipLevel(mipLevel), FirstLayerIndex(0), NumLayers(pTexture->GetDesc()->ArraySize)
{

}

GPU_RENDER_TARGET_VIEW_DESC::GPU_RENDER_TARGET_VIEW_DESC(GPUTexture2DArray *pTexture, uint32 mipLevel, uint32 arrayIndex)
    : MipLevel(mipLevel), FirstLayerIndex(arrayIndex), NumLayers(1)
{

}

GPU_RENDER_TARGET_VIEW_DESC::GPU_RENDER_TARGET_VIEW_DESC(GPUTexture3D *pTexture, uint32 mipLevel, uint32 slideIndex)
    : MipLevel(mipLevel), FirstLayerIndex(slideIndex), NumLayers(1)
{

}

GPU_RENDER_TARGET_VIEW_DESC::GPU_RENDER_TARGET_VIEW_DESC(GPUTextureCube *pTexture, uint32 mipLevel /*= 0*/)
    : MipLevel(mipLevel), FirstLayerIndex(0), NumLayers(CUBE_FACE_COUNT)
{

}

GPU_RENDER_TARGET_VIEW_DESC::GPU_RENDER_TARGET_VIEW_DESC(GPUTextureCube *pTexture, uint32 mipLevel, CUBE_FACE faceIndex)
    : MipLevel(mipLevel), FirstLayerIndex(faceIndex), NumLayers(1)
{

}

GPU_RENDER_TARGET_VIEW_DESC::GPU_RENDER_TARGET_VIEW_DESC(GPUTextureCubeArray *pTexture, uint32 mipLevel, uint32 arrayIndex)
    : MipLevel(mipLevel), FirstLayerIndex(arrayIndex * CUBE_FACE_COUNT), NumLayers(CUBE_FACE_COUNT)
{

}

GPU_RENDER_TARGET_VIEW_DESC::GPU_RENDER_TARGET_VIEW_DESC(GPUTextureCubeArray *pTexture, uint32 mipLevel, uint32 arrayIndex, CUBE_FACE faceIndex)
    : MipLevel(mipLevel), FirstLayerIndex(arrayIndex * CUBE_FACE_COUNT + faceIndex), NumLayers(1)
{

}

GPU_RENDER_TARGET_VIEW_DESC::GPU_RENDER_TARGET_VIEW_DESC(GPUDepthTexture *pTexture)
    : MipLevel(0), FirstLayerIndex(0), NumLayers(1)
{

}

GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC::GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUTexture2D *pTexture, uint32 mipLevel /*= 0*/)
    : MipLevel(mipLevel), FirstLayerIndex(0), NumLayers(1)
{

}

GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC::GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUTexture2DArray *pTexture, uint32 mipLevel /*= 0*/)
    : MipLevel(mipLevel), FirstLayerIndex(0), NumLayers(pTexture->GetDesc()->ArraySize)
{

}

GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC::GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUTexture2DArray *pTexture, uint32 mipLevel, uint32 arrayIndex)
    : MipLevel(mipLevel), FirstLayerIndex(arrayIndex), NumLayers(1)
{

}

GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC::GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUTextureCube *pTexture, uint32 mipLevel /*= 0*/)
    : MipLevel(mipLevel), FirstLayerIndex(0), NumLayers(CUBE_FACE_COUNT)
{

}

GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC::GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUTextureCube *pTexture, uint32 mipLevel, CUBE_FACE faceIndex)
    : MipLevel(mipLevel), FirstLayerIndex(faceIndex), NumLayers(1)
{

}

GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC::GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUTextureCubeArray *pTexture, uint32 mipLevel, uint32 arrayIndex)
    : MipLevel(mipLevel), FirstLayerIndex(arrayIndex * CUBE_FACE_COUNT), NumLayers(CUBE_FACE_COUNT)
{

}

GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC::GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUTextureCubeArray *pTexture, uint32 mipLevel, uint32 arrayIndex, CUBE_FACE faceIndex)
    : MipLevel(mipLevel), FirstLayerIndex(arrayIndex * CUBE_FACE_COUNT + faceIndex), NumLayers(1)
{

}

GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC::GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC(GPUDepthTexture *pTexture)
    : MipLevel(0), FirstLayerIndex(0), NumLayers(1)
{

}
