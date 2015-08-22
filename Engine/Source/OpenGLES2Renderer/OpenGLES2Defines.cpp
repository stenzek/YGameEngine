#include "OpenGLES2Renderer/PrecompiledHeader.h"
#include "OpenGLES2Renderer/OpenGLES2Common.h"
#include "OpenGLES2Renderer/OpenGLES2GPUTexture.h"
Log_SetChannel(OpenGLES2RenderBackend);

#define GL_ERROR_NAME(s) Y_NameTable_Entry(#s, s)

Y_Define_NameTable(NameTables::GLESErrors)
    GL_ERROR_NAME(GL_NO_ERROR)
    GL_ERROR_NAME(GL_INVALID_ENUM)
    GL_ERROR_NAME(GL_INVALID_VALUE)
    GL_ERROR_NAME(GL_INVALID_OPERATION)
    GL_ERROR_NAME(GL_STACK_OVERFLOW)
    GL_ERROR_NAME(GL_STACK_UNDERFLOW)
    GL_ERROR_NAME(GL_OUT_OF_MEMORY)
Y_NameTable_End()

GLenum g_eGLES2LastError = GL_NO_ERROR;
void GLES2Renderer_PrintError(const char *Format, ...)
{
    char buffer[1024];
    va_list ap;

    va_start(ap, Format);
    Y_vsnprintf(buffer, countof(buffer), Format, ap);
    va_end(ap);

    if (g_eGLES2LastError != GL_NO_ERROR)
        Log_ErrorPrintf("%s%s (0x%X)", buffer, NameTable_GetNameString(NameTables::GLESErrors, g_eGLES2LastError), g_eGLES2LastError);
    else
        Log_ErrorPrintf("%sno error", buffer);
}

struct OpenGLTextureFormatMapping
{
    GLint GLInternalFormat;
    GLenum GLFormat;
    GLenum GLType;
    bool IsCompressed;
};

static const OpenGLTextureFormatMapping s_OpenGLTextureFormatMapping[PIXEL_FORMAT_COUNT] =
{
//   // GLInternalFormat                        // GLFormat                 // GLType                           // IsCompressed
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R8_UINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R8_SINT
    { GL_LUMINANCE,                             GL_LUMINANCE,               GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R8_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R8_SNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R8G8_UINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R8G8_SINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R8G8_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R8G8_SNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R8G8B8A8_UINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R8G8B8A8_SINT
    { GL_RGBA,                                  GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R8G8B8A8_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R8G8B8A8_SNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R9G9B9E5_SHAREDEXP
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R10G10B10A2_UINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R10G10B10A2_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R11G11B10_FLOAT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R16_UINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R16_SINT
    { GL_LUMINANCE,                             GL_LUMINANCE,               GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R16_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R16_SNORM
    { GL_LUMINANCE,                             GL_LUMINANCE,               GL_HALF_FLOAT_OES,                  false           },  // PIXEL_FORMAT_R16_FLOAT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R16G16_UINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R16G16_SINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R16G16_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R16G16_SNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R16G16_FLOAT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R16G16B16A16_UINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R16G16B16A16_SINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R16G16B16A16_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R16G16B16A16_SNORM
    { GL_RGBA,                                  GL_RGBA,                    GL_HALF_FLOAT_OES,                  false           },  // PIXEL_FORMAT_R16G16B16A16_FLOAT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R32_UINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R32_SINT
    { GL_LUMINANCE,                             GL_LUMINANCE,               GL_FLOAT,                           false           },  // PIXEL_FORMAT_R32_FLOAT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R32G32_UINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R32G32_SINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R32G32_FLOAT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R32G32B32_UINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R32G32B32_SINT
    { GL_RGB,                                   GL_RGB,                     GL_FLOAT,                           false           },  // PIXEL_FORMAT_R32G32B32_FLOAT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R32G32B32A32_UINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R32G32B32A32_SINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R32G32B32A32_FLOAT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_B8G8R8A8_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_B8G8R8X8_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_B5G6R5_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_B5G5R5A1_UNORM
    { GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,         GL_RGBA,                    GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC1_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_BC1_UNORM_SRGB
    { GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,         GL_RGBA,                    GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC2_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_BC2_UNORM_SRGB
    { GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,         GL_RGBA,                    GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC3_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_BC3_UNORM_SRGB
    { 0,                                        0,                          0,                                  true            },  // PIXEL_FORMAT_BC4_UNORM
    { 0,                                        0,                          0,                                  true            },  // PIXEL_FORMAT_BC4_SNORM
    { 0,                                        0,                          0,                                  true            },  // PIXEL_FORMAT_BC5_UNORM
    { 0,                                        0,                          0,                                  true            },  // PIXEL_FORMAT_BC5_SNORM
    { 0,                                        0,                          0,                                  true            },  // PIXEL_FORMAT_BC6H_UF16
    { 0,                                        0,                          0,                                  true            },  // PIXEL_FORMAT_BC6H_SF16
    { 0,                                        0,                          0,                                  true            },  // PIXEL_FORMAT_BC7_UNORM
    { 0,                                        0,                          0,                                  true            },  // PIXEL_FORMAT_BC7_UNORM_SRGB
    { GL_DEPTH_COMPONENT,                       GL_DEPTH_COMPONENT,         GL_UNSIGNED_SHORT,                  false           },  // PIXEL_FORMAT_D16_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_D24_UNORM_S8_UINT
    { GL_DEPTH_COMPONENT,                       GL_DEPTH_COMPONENT,         GL_FLOAT,                           false           },  // PIXEL_FORMAT_D32_FLOAT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_D32_FLOAT_S8X24_UINT
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_R8G8B8_UNORM
    { 0,                                        0,                          0,                                  false           },  // PIXEL_FORMAT_B8G8R8_UNORM
};

bool OpenGLES2TypeConversion::GetOpenGLTextureFormat(PIXEL_FORMAT PixelFormat, GLint *pGLInternalFormat, GLenum *pGLFormat, GLenum *pGLType)
{
    DebugAssert(PixelFormat < PIXEL_FORMAT_COUNT);

    const OpenGLTextureFormatMapping *pMapping = &s_OpenGLTextureFormatMapping[PixelFormat];
    if (pMapping->GLFormat == 0)
        return false;

    if (pGLInternalFormat != nullptr)
        *pGLInternalFormat = pMapping->GLInternalFormat;

    if (pGLFormat != nullptr)
        *pGLFormat = pMapping->GLFormat;

    if (pGLType != nullptr)
        *pGLType = pMapping->GLType;
    
    return true;
}

struct OpenGLUniformShaderVertexElementTypeVAOMapping
{
    GPU_VERTEX_ELEMENT_TYPE VertexElementType;
    bool IntegerType;
    GLenum GLType;
    GLint GLSize;
    GLboolean GLNormalized;
};

static const OpenGLUniformShaderVertexElementTypeVAOMapping s_OpenGLUniformShaderVertexElementTypeVAOMapping[] =
{
    // VertexElementType                IntegerType     GLType              GLSize      GLNormalized
    { GPU_VERTEX_ELEMENT_TYPE_BYTE,     true,           GL_BYTE,            1,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_BYTE2,    true,           GL_BYTE,            2,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_BYTE4,    true,           GL_BYTE,            4,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_UBYTE,    true,           GL_UNSIGNED_BYTE,   1,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_UBYTE2,   true,           GL_UNSIGNED_BYTE,   2,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_UBYTE4,   true,           GL_UNSIGNED_BYTE,   4,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_HALF,     false,          GL_HALF_FLOAT_OES,  1,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_HALF2,    false,          GL_HALF_FLOAT_OES,  2,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_HALF4,    false,          GL_HALF_FLOAT_OES,  4,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_FLOAT,    false,          GL_FLOAT,           1,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_FLOAT2,   false,          GL_FLOAT,           2,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_FLOAT3,   false,          GL_FLOAT,           3,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_FLOAT4,   false,          GL_FLOAT,           4,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_INT,      true,           GL_INT,             1,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_INT2,     true,           GL_INT,             2,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_INT3,     true,           GL_INT,             3,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_INT4,     true,           GL_INT,             4,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_UINT,     true,           GL_UNSIGNED_INT,    1,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_UINT2,    true,           GL_UNSIGNED_INT,    2,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_UINT3,    true,           GL_UNSIGNED_INT,    3,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_UINT4,    true,           GL_UNSIGNED_INT,    4,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_SNORM4,   false,          GL_BYTE,            4,          GL_TRUE         },
    { GPU_VERTEX_ELEMENT_TYPE_UNORM4,   false,          GL_UNSIGNED_BYTE,   4,          GL_TRUE         },
};

void OpenGLES2TypeConversion::GetOpenGLVAOTypeAndSizeForVertexElementType(GPU_VERTEX_ELEMENT_TYPE elementType, bool *pIntegerType, GLenum *pGLType, GLint *pGLSize, GLboolean *pGLNormalized)
{
    uint32 i;

    for (i = 0; i < countof(s_OpenGLUniformShaderVertexElementTypeVAOMapping); i++)
    {
        const OpenGLUniformShaderVertexElementTypeVAOMapping *pTypeMapping = &s_OpenGLUniformShaderVertexElementTypeVAOMapping[i];
        if (pTypeMapping->VertexElementType == elementType)
        {
            *pIntegerType = pTypeMapping->IntegerType;
            *pGLType = pTypeMapping->GLType;
            *pGLSize = pTypeMapping->GLSize;
            *pGLNormalized = pTypeMapping->GLNormalized;
            return;
        }
    }

    Panic("Unhandled case!");
}

GLenum OpenGLES2TypeConversion::GetOpenGLComparisonFunc(GPU_COMPARISON_FUNC ComparisonFunc)
{
    static const GLenum OpenGLComparisonFuncs[GPU_COMPARISON_FUNC_COUNT] =
    {
        GL_NEVER,                               // RENDERER_COMPARISON_FUNC_NEVER
        GL_LESS,                                // RENDERER_COMPARISON_FUNC_LESS
        GL_EQUAL,                               // RENDERER_COMPARISON_FUNC_EQUAL
        GL_LEQUAL,                              // RENDERER_COMPARISON_FUNC_LESS_EQUAL
        GL_GREATER,                             // RENDERER_COMPARISON_FUNC_GREATER
        GL_NOTEQUAL,                            // RENDERER_COMPARISON_FUNC_NOT_EQUAL
        GL_GEQUAL,                              // RENDERER_COMPARISON_FUNC_GREATER_EQUAL
        GL_ALWAYS,                              // RENDERER_COMPARISON_FUNC_ALWAYS
    };

    DebugAssert(ComparisonFunc < GPU_COMPARISON_FUNC_COUNT);
    return OpenGLComparisonFuncs[ComparisonFunc];
}

GLenum OpenGLES2TypeConversion::GetOpenGLComparisonMode(TEXTURE_FILTER Filter)
{
    DebugAssert(Filter < TEXTURE_FILTER_COUNT);
    //if (Filter >= TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_POINT && Filter <= TEXTURE_FILTER_COMPARISON_ANISOTROPIC)
        //return GL_COMPARE_REF_TO_TEXTURE;
    //else
        return GL_NONE;
}

GLenum OpenGLES2TypeConversion::GetOpenGLTextureMinFilter(TEXTURE_FILTER Filter, bool hasMips)
{
    static const GLenum OpenGLMinFilters[TEXTURE_FILTER_COUNT] =
    {
        GL_NEAREST_MIPMAP_NEAREST,          // TEXTURE_FILTER_MIN_MAG_MIP_POINT
        GL_NEAREST_MIPMAP_LINEAR,           // TEXTURE_FILTER_MIN_MAG_POINT_MIP_LINEAR
        GL_NEAREST_MIPMAP_NEAREST,          // TEXTURE_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT
        GL_NEAREST_MIPMAP_LINEAR,           // TEXTURE_FILTER_MIN_POINT_MAG_MIP_LINEAR
        GL_LINEAR_MIPMAP_NEAREST,           // TEXTURE_FILTER_MIN_LINEAR_MAG_MIP_POINT
        GL_LINEAR_MIPMAP_LINEAR,            // TEXTURE_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR
        GL_LINEAR_MIPMAP_NEAREST,           // TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT
        GL_LINEAR_MIPMAP_LINEAR,            // TEXTURE_FILTER_MIN_MAG_MIP_LINEAR
        GL_LINEAR_MIPMAP_LINEAR,            // TEXTURE_FILTER_ANISOTROPIC
        GL_NEAREST_MIPMAP_NEAREST,          // TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_POINT
        GL_NEAREST_MIPMAP_LINEAR,           // TEXTURE_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR
        GL_NEAREST_MIPMAP_NEAREST,          // TEXTURE_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT
        GL_NEAREST_MIPMAP_LINEAR,           // TEXTURE_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR
        GL_LINEAR_MIPMAP_NEAREST,           // TEXTURE_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT
        GL_LINEAR_MIPMAP_LINEAR,            // TEXTURE_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR
        GL_LINEAR_MIPMAP_NEAREST,           // TEXTURE_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT
        GL_LINEAR_MIPMAP_LINEAR,            // TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR
        GL_LINEAR_MIPMAP_LINEAR,            // TEXTURE_FILTER_COMPARISON_ANISOTROPIC
    };

    DebugAssert(Filter < TEXTURE_FILTER_COUNT);
    GLenum glFilter = OpenGLMinFilters[Filter];     

    if (!hasMips)
    {
        if (glFilter == GL_NEAREST_MIPMAP_NEAREST || glFilter == GL_NEAREST_MIPMAP_LINEAR)
            glFilter = GL_NEAREST;
        else
            glFilter = GL_LINEAR;
    }

    return glFilter;    
}

GLenum OpenGLES2TypeConversion::GetOpenGLTextureMagFilter(TEXTURE_FILTER Filter)
{
    static const GLenum GLOpenMagFilters[TEXTURE_FILTER_COUNT] =
    {
        GL_NEAREST,                         // RENDERER_TEXTURE_FILTER_MIN_MAG_MIP_POINT
        GL_NEAREST,                         // RENDERER_TEXTURE_FILTER_MIN_MAG_POINT_MIP_LINEAR
        GL_LINEAR,                          // RENDERER_TEXTURE_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT
        GL_LINEAR,                          // RENDERER_TEXTURE_FILTER_MIN_POINT_MAG_MIP_LINEAR
        GL_NEAREST,                         // RENDERER_TEXTURE_FILTER_MIN_LINEAR_MAG_MIP_POINT
        GL_NEAREST,                         // RENDERER_TEXTURE_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR
        GL_LINEAR,                          // RENDERER_TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT
        GL_LINEAR,                          // RENDERER_TEXTURE_FILTER_MIN_MAG_MIP_LINEAR
        GL_LINEAR,                          // RENDERER_TEXTURE_FILTER_ANISOTROPIC
    };

    DebugAssert(Filter < TEXTURE_FILTER_COUNT);
    return GLOpenMagFilters[Filter];
}

GLenum OpenGLES2TypeConversion::GetOpenGLTextureWrap(TEXTURE_ADDRESS_MODE AddressMode)
{
    static const GLenum OpenGLTextureAddresses[TEXTURE_ADDRESS_MODE_COUNT] = 
    {
        GL_REPEAT,                          // RENDERER_TEXTURE_ADDRESS_WRAP
        GL_MIRRORED_REPEAT,                 // RENDERER_TEXTURE_ADDRESS_MIRROR
        GL_CLAMP_TO_EDGE,                   // RENDERER_TEXTURE_ADDRESS_CLAMP
        GL_CLAMP_TO_EDGE,                   // RENDERER_TEXTURE_ADDRESS_BORDER          // FIXME
        GL_MIRRORED_REPEAT,                 // RENDERER_TEXTURE_ADDRESS_MIRROR_ONCE     // FIXME
    };

    DebugAssert(AddressMode < TEXTURE_ADDRESS_MODE_COUNT);
    return OpenGLTextureAddresses[AddressMode];
}

GLenum OpenGLES2TypeConversion::GetOpenGLCullFace(RENDERER_CULL_MODE CullMode)
{
    static const GLenum OpenGLCullModes[RENDERER_CULL_MODE_COUNT] =
    {
        GL_BACK,                            // RENDERER_CULL_NONE
        GL_FRONT,                           // RENDERER_CULL_FRONT
        GL_BACK,                            // RENDERER_CULL_BACK
    };

    DebugAssert(CullMode < RENDERER_CULL_MODE_COUNT);
    return OpenGLCullModes[CullMode];
}

GLenum OpenGLES2TypeConversion::GetOpenGLStencilOp(RENDERER_STENCIL_OP StencilOp)
{
    static const GLenum OpenGLStencilOps[RENDERER_STENCIL_OP_COUNT] =
    {
        GL_KEEP,                            // RENDERER_STENCIL_OP_KEEP
        GL_ZERO,                            // RENDERER_STENCIL_OP_ZERO
        GL_REPLACE,                         // RENDERER_STENCIL_OP_REPLACE
        GL_INCR_WRAP,                       // RENDERER_STENCIL_OP_INCREMENT_CLAMPED
        GL_DECR_WRAP,                       // RENDERER_STENCIL_OP_DECREMENT_CLAMPED
        GL_INVERT,                          // RENDERER_STENCIL_OP_INVERT
        GL_INCR,                            // RENDERER_STENCIL_OP_INCREMENT
        GL_DECR,                            // RENDERER_STENCIL_OP_DECREMENT
    };

    DebugAssert(StencilOp < RENDERER_STENCIL_OP_COUNT);
    return OpenGLStencilOps[StencilOp];
}

GLenum OpenGLES2TypeConversion::GetOpenGLBlendEquation(RENDERER_BLEND_OP BlendOp)
{
    static const GLenum OpenGLBlendEquations[RENDERER_BLEND_OP_COUNT] =
    {
        GL_FUNC_ADD,                        // RENDERER_BLEND_OP_ADD
        GL_FUNC_SUBTRACT,                   // RENDERER_BLEND_OP_SUBTRACT
        GL_FUNC_REVERSE_SUBTRACT,           // RENDERER_BLEND_OP_REV_SUBTRACT
        GL_FUNC_ADD,                             // RENDERER_BLEND_OP_MIN FIXME
        GL_FUNC_ADD,                             // RENDERER_BLEND_OP_MAX FIXME
    };

    DebugAssert(BlendOp < RENDERER_BLEND_OP_COUNT);
    return OpenGLBlendEquations[BlendOp];
}

GLenum OpenGLES2TypeConversion::GetOpenGLBlendFunc(RENDERER_BLEND_OPTION BlendFactor)
{
    static const GLenum OpenGLBlendFuncs[RENDERER_BLEND_OPTION_COUNT] = 
    {
        GL_ZERO,                        // RENDERER_BLEND_ZERO
        GL_ONE,                         // RENDERER_BLEND_ONE
        GL_SRC_COLOR,                   // RENDERER_BLEND_SRC_COLOR
        GL_ONE_MINUS_SRC_COLOR,         // RENDERER_BLEND_INV_SRC_COLOR
        GL_SRC_ALPHA,                   // RENDERER_BLEND_SRC_ALPHA
        GL_ONE_MINUS_SRC_ALPHA,         // RENDERER_BLEND_INV_SRC_ALPHA
        GL_DST_ALPHA,                   // RENDERER_BLEND_DEST_ALPHA
        GL_ONE_MINUS_DST_ALPHA,         // RENDERER_BLEND_INV_DEST_ALPHA
        GL_DST_COLOR,                   // RENDERER_BLEND_DEST_COLOR
        GL_ONE_MINUS_DST_COLOR,         // RENDERER_BLEND_INV_DEST_COLOR
        GL_SRC_ALPHA_SATURATE,          // RENDERER_BLEND_SRC_ALPHA_SAT
        GL_CONSTANT_COLOR,              // RENDERER_BLEND_BLEND_FACTOR
        GL_ONE_MINUS_CONSTANT_COLOR,    // RENDERER_BLEND_INV_BLEND_FACTOR
        GL_ZERO,                  // RENDERER_BLEND_SRC1_COLOR FIXME
        GL_ZERO,        // RENDERER_BLEND_INV_SRC1_COLOR FIXME
        GL_ZERO,                  // RENDERER_BLEND_SRC1_ALPHA FIXME
        GL_ZERO,        // RENDERER_BLEND_INV_SRC1_ALPHA FIXME
    };

    DebugAssert(BlendFactor < RENDERER_BLEND_OPTION_COUNT);
    return OpenGLBlendFuncs[BlendFactor];
}

GLenum OpenGLES2TypeConversion::GetOpenGLTextureTarget(TEXTURE_TYPE textureType)
{
    switch (textureType)
    {
    case TEXTURE_TYPE_2D:           return GL_TEXTURE_2D;
    case TEXTURE_TYPE_CUBE:         return GL_TEXTURE_CUBE_MAP;
    }

    Panic("Unhandled type");
    return GL_TEXTURE_2D;
}

void OpenGLES2Helpers::CorrectProjectionMatrix(float4x4 &projectionMatrix)
{
    float4x4 scaleMatrix(1.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 1.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, 2.0f, 0.0f,
                         0.0f, 0.0f, 0.0f, 1.0f);

    float4x4 biasMatrix(1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, -1.0f,
                        0.0f, 0.0f, 0.0f, 1.0f);

    projectionMatrix = biasMatrix * scaleMatrix * projectionMatrix;
}

GLenum OpenGLES2Helpers::GetOpenGLTextureTarget(GPUTexture *pGPUTexture)
{
    if (pGPUTexture == NULL)
        return 0;

    switch (pGPUTexture->GetResourceType())
    {
    case GPU_RESOURCE_TYPE_TEXTURE2D:
        return GL_TEXTURE_2D;

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        return GL_TEXTURE_CUBE_MAP;
    }

    return 0;
}

GLuint OpenGLES2Helpers::GetOpenGLTextureId(GPUTexture *pGPUTexture)
{
    if (pGPUTexture == NULL)
        return 0;

    switch (pGPUTexture->GetResourceType())
    {
    case GPU_RESOURCE_TYPE_TEXTURE2D:
        return static_cast<OpenGLES2GPUTexture2D *>(pGPUTexture)->GetGLTextureId();

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        return static_cast<OpenGLES2GPUTextureCube *>(pGPUTexture)->GetGLTextureId();
    }

    return 0;
}

void OpenGLES2Helpers::BindOpenGLTexture(GPUTexture *pGPUTexture)
{
    if (pGPUTexture == nullptr)
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    switch (pGPUTexture->GetResourceType())
    {
    case GPU_RESOURCE_TYPE_TEXTURE2D:
        glBindTexture(GL_TEXTURE_2D, static_cast<OpenGLES2GPUTexture2D *>(pGPUTexture)->GetGLTextureId());
        break;

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<OpenGLES2GPUTextureCube *>(pGPUTexture)->GetGLTextureId());
        break;
    }
}

void OpenGLES2Helpers::GLUniformWrapper(SHADER_PARAMETER_TYPE type, GLuint location, GLuint arraySize, const void *pValue)
{
    // Invoke the appropriate glUniform() command
    switch (type)
    {
    case SHADER_PARAMETER_TYPE_BOOL:
        glUniform1iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_BOOL2:
        glUniform2iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_BOOL3:
        glUniform3iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_BOOL4:
        glUniform4iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_INT:
        glUniform1iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_INT2:
        glUniform2iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_INT3:
        glUniform3iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_INT4:
        glUniform4iv(location, arraySize, reinterpret_cast<const GLint *>(pValue));
        break;

//     case SHADER_PARAMETER_TYPE_UINT:
//         glUniform1uiv(location, arraySize, reinterpret_cast<const GLuint *>(pValue));
//         break;
// 
//     case SHADER_PARAMETER_TYPE_UINT2:
//         glUniform2uiv(location, arraySize, reinterpret_cast<const GLuint *>(pValue));
//         break;
// 
//     case SHADER_PARAMETER_TYPE_UINT3:
//         glUniform3uiv(location, arraySize, reinterpret_cast<const GLuint *>(pValue));
//         break;
// 
//     case SHADER_PARAMETER_TYPE_UINT4:
//         glUniform4uiv(location, arraySize, reinterpret_cast<const GLuint *>(pValue));
//         break;

    case SHADER_PARAMETER_TYPE_FLOAT:
        glUniform1fv(location, arraySize, reinterpret_cast<const GLfloat *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT2:
        glUniform2fv(location, arraySize, reinterpret_cast<const GLfloat *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT3:
        glUniform3fv(location, arraySize, reinterpret_cast<const GLfloat *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT4:
        glUniform4fv(location, arraySize, reinterpret_cast<const GLfloat *>(pValue));
        break;

#if defined(Y_PLATFORM_HTML5)
        // WebGL does not support in-flight transposition. We have to handle it ourselves.
    case SHADER_PARAMETER_TYPE_FLOAT2X2:
        {
            const float *source = reinterpret_cast<const float *>(pValue);
            for (uint32 i = 0; i < arraySize; i++)
            {
                float transposed[4];
                transposed[0] = source[0]; transposed[2] = source[1];
                transposed[1] = source[2]; transposed[3] = source[3];
                glUniformMatrix2fv(location + i, 1, GL_FALSE, transposed);
                source += 4;
            }
        }
        break;

    case SHADER_PARAMETER_TYPE_FLOAT3X3:
        {
            const float *source = reinterpret_cast<const float *>(pValue);
            for (uint32 i = 0; i < arraySize; i++)
            {
                float transposed[9];
                transposed[0] = source[0]; transposed[3] = source[1]; transposed[6] = source[2];
                transposed[1] = source[3]; transposed[4] = source[4]; transposed[7] = source[5];
                transposed[2] = source[6]; transposed[5] = source[7]; transposed[8] = source[8];
                glUniformMatrix3fv(location + i, 1, GL_FALSE, transposed);
                source += 9;
            }
        }
        break;

        // SHADER_PARAMETER_TYPE_FLOAT3X4 unsupported
    case SHADER_PARAMETER_TYPE_FLOAT4X4:
        {
            const float *source = reinterpret_cast<const float *>(pValue);
            for (uint32 i = 0; i < arraySize; i++)
            {
                float transposed[16];
                transposed[0] = source[0]; transposed[4] = source[1]; transposed[8] = source[2]; transposed[12] = source[3];
                transposed[1] = source[4]; transposed[5] = source[5]; transposed[9] = source[6]; transposed[13] = source[7];
                transposed[2] = source[8]; transposed[6] = source[9]; transposed[10] = source[10]; transposed[14] = source[11];
                transposed[3] = source[12]; transposed[7] = source[13]; transposed[11] = source[14]; transposed[15] = source[15];
                glUniformMatrix4fv(location + i, 1, GL_FALSE, transposed);
                source += 16;
            }
        }
        break;

#else       // Y_PLATFORM_HTML5


    case SHADER_PARAMETER_TYPE_FLOAT2X2:
        glUniformMatrix2fv(location, arraySize, GL_TRUE, reinterpret_cast<const GLfloat *>(pValue));
        break;

    case SHADER_PARAMETER_TYPE_FLOAT3X3:
        glUniformMatrix3fv(location, arraySize, GL_TRUE, reinterpret_cast<const GLfloat *>(pValue));
        break;

//     case SHADER_PARAMETER_TYPE_FLOAT3X4:
//         glUniformMatrix4x3fv(location, arraySize, GL_TRUE, reinterpret_cast<const GLfloat *>(pValue));
//         break;

    case SHADER_PARAMETER_TYPE_FLOAT4X4:
        glUniformMatrix4fv(location, arraySize, GL_TRUE, reinterpret_cast<const GLfloat *>(pValue));
        break;

#endif      // Y_PLATFORM_HTML5
    }
}

void OpenGLES2Helpers::SetObjectDebugName(GLenum type, GLuint id, const char *debugName)
{
#if defined(GL_KHR_debug) && defined(Y_BUILD_CONFIG_DEBUG)
    if (GLAD_GL_KHR_debug)
    {
        uint32 length = Y_strlen(debugName);
        if (length > 0)
            glObjectLabelKHR(type, id, length, debugName);
    }
#endif
}
