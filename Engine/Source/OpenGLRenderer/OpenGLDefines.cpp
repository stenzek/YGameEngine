#include "OpenGLRenderer/PrecompiledHeader.h"
#include "OpenGLRenderer/OpenGLCommon.h"
#include "OpenGLRenderer/OpenGLGPUTexture.h"
Log_SetChannel(OpenGLRenderBackend);

#define GL_ERROR_NAME(s) Y_NameTable_Entry(#s, s)

Y_Define_NameTable(NameTables::GLErrors)
    GL_ERROR_NAME(GL_NO_ERROR)
    GL_ERROR_NAME(GL_INVALID_ENUM)
    GL_ERROR_NAME(GL_INVALID_VALUE)
    GL_ERROR_NAME(GL_INVALID_OPERATION)
    GL_ERROR_NAME(GL_STACK_OVERFLOW)
    GL_ERROR_NAME(GL_STACK_UNDERFLOW)
    GL_ERROR_NAME(GL_OUT_OF_MEMORY)
Y_NameTable_End()

void OpenGLTypeConversion::GetOpenGLVersionForRendererFeatureLevel(RENDERER_FEATURE_LEVEL featureLevel, uint32 *pMajorVersion, uint32 *pMinorVersion)
{
    switch (featureLevel)
    {
//     case RENDERER_FEATURE_LEVEL_OPENGL_2_1:
//         *pMajorVersion = 2; *pMinorVersion = 0;
//         break;
// 
//     case RENDERER_FEATURE_LEVEL_OPENGL_3_0:
//         *pMajorVersion = 3; *pMinorVersion = 0;
//         break;
// 
//     case RENDERER_FEATURE_LEVEL_OPENGL_3_1:
//         *pMajorVersion = 3; *pMinorVersion = 1;
//         break;
// 
//     case RENDERER_FEATURE_LEVEL_OPENGL_3_2:
//         *pMajorVersion = 3; *pMinorVersion = 2;
//         break;
// 
//     case RENDERER_FEATURE_LEVEL_OPENGL_3_3:
//         *pMajorVersion = 3; *pMinorVersion = 3;
//         break;
// 
//     case RENDERER_FEATURE_LEVEL_OPENGL_4_0:
//         *pMajorVersion = 4; *pMinorVersion = 0;
//         break;
// 
//     case RENDERER_FEATURE_LEVEL_OPENGL_4_1:
//         *pMajorVersion = 4; *pMinorVersion = 1;
//         break;
// 
//     case RENDERER_FEATURE_LEVEL_OPENGL_4_2:
//         *pMajorVersion = 4; *pMinorVersion = 2;
//         break;
// 
//     case RENDERER_FEATURE_LEVEL_OPENGL_4_3:
//         *pMajorVersion = 4; *pMinorVersion = 3;
//         break;
// 
//     case RENDERER_FEATURE_LEVEL_OPENGL_4_4:
//         *pMajorVersion = 4; *pMinorVersion = 4;
//         break;

    case RENDERER_FEATURE_LEVEL_SM4:
        *pMajorVersion = 3;     *pMinorVersion = 3;
        break;

    case RENDERER_FEATURE_LEVEL_SM5:
        *pMajorVersion = 4;     *pMinorVersion = 1;
        break;

    default:
        UnreachableCode();
        break;
    }
}

RENDERER_FEATURE_LEVEL OpenGLTypeConversion::GetRendererFeatureLevelForOpenGLVersion(uint32 majorVersion, uint32 minorVersion)
{
    if (majorVersion >= 4)
    {
        if (minorVersion >= 4)
            return RENDERER_FEATURE_LEVEL_SM5;
        else if (minorVersion == 3)
            return RENDERER_FEATURE_LEVEL_SM5;
        else if (minorVersion == 2)
            return RENDERER_FEATURE_LEVEL_SM5;
        else if (minorVersion == 1)
            return RENDERER_FEATURE_LEVEL_SM4;
        else
            return RENDERER_FEATURE_LEVEL_SM4;
    }
    else if (majorVersion == 3)
    {
        if (minorVersion >= 3)
            return RENDERER_FEATURE_LEVEL_SM4;
        else if (minorVersion == 2)
            return RENDERER_FEATURE_LEVEL_SM4;
        else if (minorVersion == 1)
            return RENDERER_FEATURE_LEVEL_ES3;
        else
            return RENDERER_FEATURE_LEVEL_ES2;
    }
    else if (majorVersion == 2)
    {
        if (minorVersion >= 1)
            return RENDERER_FEATURE_LEVEL_ES2;
    }

    return RENDERER_FEATURE_LEVEL_COUNT;
}

struct OpenGLTextureFormatMapping
{
    GLint GLInternalFormat;
    GLenum GLFormat;
    GLenum GLType;
    bool IsCompressed;
};

/*
    // PixelFormat                              // GLInternalFormat                         // GLFormat                 // GLType                           // IsCompressed
    { PIXEL_FORMAT_R8G8B8A8_UNORM,              GL_RGBA8,                                   GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },
    { PIXEL_FORMAT_R16G16_FLOAT,                GL_RG16F,                                   GL_RG,                      GL_HALF_FLOAT,                      false           },
    { PIXEL_FORMAT_R16G16B16A16_FLOAT,          GL_RGBA16F,                                 GL_RGBA,                    GL_HALF_FLOAT,                      false           },
    { PIXEL_FORMAT_R32G32_FLOAT,                GL_RG32F,                                   GL_RG,                      GL_FLOAT,                           false           },
    { PIXEL_FORMAT_R32G32B32A32_FLOAT,          GL_RGBA32F,                                 GL_RGBA,                    GL_FLOAT,                           false           },
    { PIXEL_FORMAT_B8G8R8X8_UNORM,              GL_RGB8,                                    GL_BGRA,                    GL_UNSIGNED_BYTE,                   false           },
    { PIXEL_FORMAT_B8G8R8A8_UNORM,              GL_RGBA8,                                   GL_BGRA,                    GL_UNSIGNED_BYTE,                   false           },
    { PIXEL_FORMAT_R8G8B8A8_SNORM,              GL_RGBA8_SNORM,                             GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },
    { PIXEL_FORMAT_R16G16B16A16_SNORM,          GL_RGBA16_SNORM,                            GL_RGBA,                    GL_UNSIGNED_SHORT,                  false           },
    { PIXEL_FORMAT_BC1_UNORM,                   GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,           GL_RGBA,                    GL_UNSIGNED_BYTE,                   true            },
    { PIXEL_FORMAT_BC2_UNORM,                   GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,           GL_RGBA,                    GL_UNSIGNED_BYTE,                   true            },
    { PIXEL_FORMAT_BC3_UNORM,                   GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,           GL_RGBA,                    GL_UNSIGNED_BYTE,                   true            },
    { PIXEL_FORMAT_R8_SINT,                     GL_R8I,                                     GL_RED,                     GL_BYTE,                            false           },
    { PIXEL_FORMAT_R8_UINT,                     GL_R8UI,                                    GL_RED,                     GL_UNSIGNED_BYTE,                   false           },
    { PIXEL_FORMAT_R8_SNORM,                    GL_R8,                                      GL_RED,                     GL_BYTE,                            false           },
    { PIXEL_FORMAT_R8_UNORM,                    GL_R8,                                      GL_RED,                     GL_UNSIGNED_BYTE,                   false           },
    { PIXEL_FORMAT_R16_SINT,                    GL_R16I,                                    GL_RED,                     GL_SHORT,                           false           },
    { PIXEL_FORMAT_R16_UINT,                    GL_R16UI,                                   GL_RED,                     GL_UNSIGNED_SHORT,                  false           },
    { PIXEL_FORMAT_R16_SNORM,                   GL_R16,                                     GL_RED,                     GL_SHORT,                           false           },
    { PIXEL_FORMAT_R16_UNORM,                   GL_R16,                                     GL_RED,                     GL_UNSIGNED_SHORT,                  false           },
    { PIXEL_FORMAT_R16_FLOAT,                   GL_R16F,                                    GL_RED,                     GL_HALF_FLOAT,                      false           },
    { PIXEL_FORMAT_R32_SINT,                    GL_R32I,                                    GL_RED,                     GL_INT,                             false           },
    { PIXEL_FORMAT_R32_UINT,                    GL_R32UI,                                   GL_RED,                     GL_UNSIGNED_INT,                    false           },
    { PIXEL_FORMAT_R32_FLOAT,                   GL_R32F,                                    GL_RED,                     GL_FLOAT,                           false           },
    { PIXEL_FORMAT_D16_UNORM,                   GL_DEPTH_COMPONENT16,                       GL_DEPTH_COMPONENT,         GL_UNSIGNED_SHORT,                  false           },
    { PIXEL_FORMAT_D24_UNORM_S8_UINT,           GL_DEPTH24_STENCIL8,                        GL_DEPTH_STENCIL,           GL_UNSIGNED_INT_24_8,               false           },
    { PIXEL_FORMAT_D32_FLOAT,                   GL_DEPTH_COMPONENT32F,                      GL_DEPTH_COMPONENT,         GL_FLOAT,                           false           },
    { PIXEL_FORMAT_D32_FLOAT_S8X24_UINT,        GL_DEPTH32F_STENCIL8,                       GL_DEPTH_STENCIL,           GL_FLOAT_32_UNSIGNED_INT_24_8_REV,  false           },
    { PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB,         GL_SRGB8_ALPHA8,                            GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },
    { PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB,         GL_SRGB8_ALPHA8,                            GL_BGRA,                    GL_UNSIGNED_BYTE,                   false           },
    { PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB,         GL_SRGB8,                                   GL_BGRA,                    GL_UNSIGNED_BYTE,                   false           },
    { PIXEL_FORMAT_BC1_UNORM_SRGB,              GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,     GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },
    { PIXEL_FORMAT_BC2_UNORM_SRGB,              GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,     GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },
    { PIXEL_FORMAT_BC3_UNORM_SRGB,              GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,     GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },
    { PIXEL_FORMAT_R8G8B8_UNORM,                GL_RGB8,                                    GL_RGB,                     GL_UNSIGNED_BYTE,                   false           },
*/

// todo: 'cannot upload' format, for those with bad format/type
static const OpenGLTextureFormatMapping s_OpenGLTextureFormatMapping[PIXEL_FORMAT_COUNT] =
{
    // GLInternalFormat                         // GLFormat                 // GLType                           // IsCompressed
    { GL_R8UI,                                  GL_RED,                     GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R8_UINT
    { GL_R8I,                                   GL_RED,                     GL_BYTE,                            false           },  // PIXEL_FORMAT_R8_SINT
    { GL_R8,                                    GL_RED,                     GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R8_UNORM
    { GL_R8_SNORM,                              GL_RED,                     GL_BYTE,                            false           },  // PIXEL_FORMAT_R8_SNORM
    { GL_RG8UI,                                 GL_RG,                      GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R8G8_UINT
    { GL_RG8I,                                  GL_RG,                      GL_BYTE,                            false           },  // PIXEL_FORMAT_R8G8_SINT
    { GL_RG8,                                   GL_RG,                      GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R8G8_UNORM
    { GL_RG8_SNORM,                             GL_RG,                      GL_BYTE,                            false           },  // PIXEL_FORMAT_R8G8_SNORM
    { GL_RGBA8UI,                               GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R8G8B8A8_UINT
    { GL_RGBA8I,                                GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R8G8B8A8_SINT
    { GL_RGBA8,                                 GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R8G8B8A8_UNORM
    { GL_SRGB8_ALPHA8,                          GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB
    { GL_RGBA8_SNORM,                           GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R8G8B8A8_SNORM
    { GL_RGB9_E5,                               GL_RGB,                     GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R9G9B9E5_SHAREDEXP
    { GL_RGB10_A2UI,                            GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R10G10B10A2_UINT
    { GL_RGB10_A2,                              GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R10G10B10A2_UNORM
    { GL_R11F_G11F_B10F,                        GL_RGB,                     GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R11G11B10_FLOAT
    { GL_R16UI,                                 GL_RED,                     GL_UNSIGNED_SHORT,                  false           },  // PIXEL_FORMAT_R16_UINT
    { GL_R16I,                                  GL_RED,                     GL_SHORT,                           false           },  // PIXEL_FORMAT_R16_SINT
    { GL_R16,                                   GL_RED,                     GL_UNSIGNED_SHORT,                  false           },  // PIXEL_FORMAT_R16_UNORM
    { GL_R16_SNORM,                             GL_RED,                     GL_SHORT,                           false           },  // PIXEL_FORMAT_R16_SNORM
    { GL_R16F,                                  GL_RED,                     GL_HALF_FLOAT,                      false           },  // PIXEL_FORMAT_R16_FLOAT
    { GL_RG16UI,                                GL_RG,                      GL_UNSIGNED_SHORT,                  false           },  // PIXEL_FORMAT_R16G16_UINT
    { GL_RG16I,                                 GL_RG,                      GL_SHORT,                           false           },  // PIXEL_FORMAT_R16G16_SINT
    { GL_RG16,                                  GL_RG,                      GL_UNSIGNED_SHORT,                  false           },  // PIXEL_FORMAT_R16G16_UNORM
    { GL_RG16_SNORM,                            GL_RG,                      GL_SHORT,                           false           },  // PIXEL_FORMAT_R16G16_SNORM
    { GL_RG16F,                                 GL_RG,                      GL_HALF_FLOAT,                      false           },  // PIXEL_FORMAT_R16G16_FLOAT
    { GL_RGBA16UI,                              GL_RGBA,                    GL_UNSIGNED_SHORT,                  false           },  // PIXEL_FORMAT_R16G16B16A16_UINT
    { GL_RGBA16I,                               GL_RGBA,                    GL_SHORT,                           false           },  // PIXEL_FORMAT_R16G16B16A16_SINT
    { GL_RGBA16,                                GL_RGBA,                    GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R16G16B16A16_UNORM
    { GL_RGBA16_SNORM,                          GL_RGBA,                    GL_SHORT,                           false           },  // PIXEL_FORMAT_R16G16B16A16_SNORM
    { GL_RGBA16F,                               GL_RGBA,                    GL_HALF_FLOAT,                      false           },  // PIXEL_FORMAT_R16G16B16A16_FLOAT
    { GL_R32UI,                                 GL_RED,                     GL_UNSIGNED_INT,                    false           },  // PIXEL_FORMAT_R32_UINT
    { GL_R32I,                                  GL_RED,                     GL_INT,                             false           },  // PIXEL_FORMAT_R32_SINT
    { GL_R32F,                                  GL_RED,                     GL_FLOAT,                           false           },  // PIXEL_FORMAT_R32_FLOAT
    { GL_RG32UI,                                GL_RG,                      GL_UNSIGNED_INT,                    false           },  // PIXEL_FORMAT_R32G32_UINT
    { GL_RG32I,                                 GL_RG,                      GL_INT,                             false           },  // PIXEL_FORMAT_R32G32_SINT
    { GL_RG32F,                                 GL_RG,                      GL_FLOAT,                           false           },  // PIXEL_FORMAT_R32G32_FLOAT
    { GL_RGB32UI,                               GL_RGB,                     GL_UNSIGNED_INT,                    false           },  // PIXEL_FORMAT_R32G32B32_UINT
    { GL_RGB32I,                                GL_RGB,                     GL_INT,                             false           },  // PIXEL_FORMAT_R32G32B32_SINT
    { GL_RGB32F,                                GL_RGB,                     GL_FLOAT,                           false           },  // PIXEL_FORMAT_R32G32B32_FLOAT
    { GL_RGBA32UI,                              GL_RGB,                     GL_UNSIGNED_INT,                    false           },  // PIXEL_FORMAT_R32G32B32A32_UINT
    { GL_RGBA32I,                               GL_RGB,                     GL_INT,                             false           },  // PIXEL_FORMAT_R32G32B32A32_SINT
    { GL_RGBA32F,                               GL_RGB,                     GL_FLOAT,                           false           },  // PIXEL_FORMAT_R32G32B32A32_FLOAT
    { GL_RGBA8,                                 GL_BGRA,                    GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_B8G8R8A8_UNORM
    { GL_SRGB8_ALPHA8,                          GL_BGRA,                    GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB
    { GL_RGB8,                                  GL_BGRA,                    GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_B8G8R8X8_UNORM
    { GL_SRGB8,                                 GL_BGRA,                    GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB
    { GL_RGB565,                                GL_BGR,                     GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_B5G6R5_UNORM
    { GL_RGB5_A1,                               GL_BGRA,                    GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_B5G5R5A1_UNORM
    { GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,         GL_RGBA,                    GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC1_UNORM
    { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT,   GL_RGBA,                    GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC1_UNORM_SRGB
    { GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,         GL_RGBA,                    GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC2_UNORM
    { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT,   GL_RGBA,                    GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC2_UNORM_SRGB
    { GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,         GL_RGBA,                    GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC3_UNORM
    { GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,   GL_RGBA,                    GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC3_UNORM_SRGB
    { GL_COMPRESSED_RED_RGTC1,                  GL_RED,                     GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC4_UNORM
    { GL_COMPRESSED_SIGNED_RED_RGTC1,           GL_RED,                     GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC4_SNORM
    { GL_COMPRESSED_RG_RGTC2,                   GL_RG,                      GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC5_UNORM
    { GL_COMPRESSED_SIGNED_RED_RGTC1,           GL_RG,                      GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC5_SNORM
    { GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,    GL_RG,                      GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC6H_UF16
    { GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,      GL_RG,                      GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC6H_SF16
    { GL_COMPRESSED_RGBA_BPTC_UNORM,            GL_RGBA,                    GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC7_UNORM
    { GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,      GL_RGBA,                    GL_UNSIGNED_BYTE,                   true            },  // PIXEL_FORMAT_BC7_UNORM_SRGB
    { GL_DEPTH_COMPONENT16,                     GL_DEPTH_COMPONENT,         GL_UNSIGNED_SHORT,                  false           },  // PIXEL_FORMAT_D16_UNORM
    { GL_DEPTH24_STENCIL8,                      GL_DEPTH_STENCIL,           GL_UNSIGNED_INT_24_8,               false           },  // PIXEL_FORMAT_D24_UNORM_S8_UINT
    { GL_DEPTH_COMPONENT32F,                    GL_DEPTH_COMPONENT,         GL_FLOAT,                           false           },  // PIXEL_FORMAT_D32_FLOAT
    { GL_DEPTH32F_STENCIL8,                     GL_DEPTH_STENCIL,           GL_FLOAT_32_UNSIGNED_INT_24_8_REV,  false           },  // PIXEL_FORMAT_D32_FLOAT_S8X24_UINT
    { GL_RGB8,                                  GL_RGB,                     GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_R8G8B8_UNORM
    { GL_RGB8,                                  GL_BGR,                     GL_UNSIGNED_BYTE,                   false           },  // PIXEL_FORMAT_B8G8R8_UNORM
};

bool OpenGLTypeConversion::GetOpenGLTextureFormat(PIXEL_FORMAT PixelFormat, GLint *pGLInternalFormat, GLenum *pGLFormat, GLenum *pGLType)
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
    { GPU_VERTEX_ELEMENT_TYPE_HALF,     false,          GL_FLOAT16_NV,      1,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_HALF2,    false,          GL_FLOAT16_NV,      2,          GL_FALSE        },
    { GPU_VERTEX_ELEMENT_TYPE_HALF4,    false,          GL_FLOAT16_NV,      4,          GL_FALSE        },
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

void OpenGLTypeConversion::GetOpenGLVAOTypeAndSizeForVertexElementType(GPU_VERTEX_ELEMENT_TYPE elementType, bool *pIntegerType, GLenum *pGLType, GLint *pGLSize, GLboolean *pGLNormalized)
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

GLenum OpenGLTypeConversion::GetOpenGLComparisonFunc(GPU_COMPARISON_FUNC ComparisonFunc)
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

GLenum OpenGLTypeConversion::GetOpenGLComparisonMode(TEXTURE_FILTER Filter)
{
    DebugAssert(Filter < TEXTURE_FILTER_COUNT);
    if (Filter >= TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_POINT && Filter <= TEXTURE_FILTER_COMPARISON_ANISOTROPIC)
        return GL_COMPARE_REF_TO_TEXTURE;
    else
        return GL_NONE;
}

GLenum OpenGLTypeConversion::GetOpenGLTextureMinFilter(TEXTURE_FILTER Filter)
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
    return OpenGLMinFilters[Filter];
}

GLenum OpenGLTypeConversion::GetOpenGLTextureMagFilter(TEXTURE_FILTER Filter)
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
        GL_NEAREST,                         // TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_POINT
        GL_NEAREST,                         // TEXTURE_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR
        GL_LINEAR,                          // TEXTURE_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT
        GL_LINEAR,                          // TEXTURE_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR
        GL_NEAREST,                         // TEXTURE_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT
        GL_NEAREST,                         // TEXTURE_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR
        GL_LINEAR,                          // TEXTURE_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT
        GL_LINEAR,                          // TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR
        GL_LINEAR,                          // TEXTURE_FILTER_COMPARISON_ANISOTROPIC
    };

    DebugAssert(Filter < TEXTURE_FILTER_COUNT);
    return GLOpenMagFilters[Filter];
}

GLenum OpenGLTypeConversion::GetOpenGLTextureWrap(TEXTURE_ADDRESS_MODE AddressMode)
{
    static const GLenum OpenGLTextureAddresses[TEXTURE_ADDRESS_MODE_COUNT] = 
    {
        GL_REPEAT,                          // RENDERER_TEXTURE_ADDRESS_WRAP
        GL_MIRRORED_REPEAT,                 // RENDERER_TEXTURE_ADDRESS_MIRROR
        GL_CLAMP_TO_EDGE,                   // RENDERER_TEXTURE_ADDRESS_CLAMP
        GL_CLAMP_TO_BORDER,                 // RENDERER_TEXTURE_ADDRESS_BORDER
        GL_MIRRORED_REPEAT,                 // RENDERER_TEXTURE_ADDRESS_MIRROR_ONCE     // FIXME
    };

    DebugAssert(AddressMode < TEXTURE_ADDRESS_MODE_COUNT);
    return OpenGLTextureAddresses[AddressMode];
}

GLenum OpenGLTypeConversion::GetOpenGLPolygonMode(RENDERER_FILL_MODE FillMode)
{
    static const GLenum OpenGLPolygonModes[RENDERER_FILL_MODE_COUNT] =
    {
        GL_LINE,                            // RENDERER_FILL_WIREFRAME
        GL_FILL,                            // RENDERER_FILL_SOLID
    };

    DebugAssert(FillMode < RENDERER_FILL_MODE_COUNT);
    return OpenGLPolygonModes[FillMode];    
}

GLenum OpenGLTypeConversion::GetOpenGLCullFace(RENDERER_CULL_MODE CullMode)
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

GLenum OpenGLTypeConversion::GetOpenGLStencilOp(RENDERER_STENCIL_OP StencilOp)
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

GLenum OpenGLTypeConversion::GetOpenGLBlendEquation(RENDERER_BLEND_OP BlendOp)
{
    static const GLenum OpenGLBlendEquations[RENDERER_BLEND_OP_COUNT] =
    {
        GL_FUNC_ADD,                        // RENDERER_BLEND_OP_ADD
        GL_FUNC_SUBTRACT,                   // RENDERER_BLEND_OP_SUBTRACT
        GL_FUNC_REVERSE_SUBTRACT,           // RENDERER_BLEND_OP_REV_SUBTRACT
        GL_MIN,                             // RENDERER_BLEND_OP_MIN
        GL_MAX,                             // RENDERER_BLEND_OP_MAX
    };

    DebugAssert(BlendOp < RENDERER_BLEND_OP_COUNT);
    return OpenGLBlendEquations[BlendOp];
}

GLenum OpenGLTypeConversion::GetOpenGLBlendFunc(RENDERER_BLEND_OPTION BlendFactor)
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
        GL_SRC1_COLOR,                  // RENDERER_BLEND_SRC1_COLOR
        GL_ONE_MINUS_SRC1_COLOR,        // RENDERER_BLEND_INV_SRC1_COLOR
        GL_SRC1_ALPHA,                  // RENDERER_BLEND_SRC1_ALPHA
        GL_ONE_MINUS_SRC1_ALPHA,        // RENDERER_BLEND_INV_SRC1_ALPHA
    };

    DebugAssert(BlendFactor < RENDERER_BLEND_OPTION_COUNT);
    return OpenGLBlendFuncs[BlendFactor];
}

GLenum OpenGLTypeConversion::GetOpenGLTextureTarget(TEXTURE_TYPE textureType)
{
    switch (textureType)
    {
    case TEXTURE_TYPE_1D:           return GL_TEXTURE_1D;
    case TEXTURE_TYPE_1D_ARRAY:     return GL_TEXTURE_1D_ARRAY;
    case TEXTURE_TYPE_2D:           return GL_TEXTURE_2D;
    case TEXTURE_TYPE_2D_ARRAY:     return GL_TEXTURE_2D_ARRAY;
    case TEXTURE_TYPE_3D:           return GL_TEXTURE_3D;
    case TEXTURE_TYPE_CUBE:         return GL_TEXTURE_CUBE_MAP;
    case TEXTURE_TYPE_CUBE_ARRAY:   return GL_TEXTURE_CUBE_MAP_ARRAY;
    }

    Panic("Unhandled type");
    return GL_TEXTURE_2D;
}

GLenum OpenGLHelpers::GetOpenGLTextureTarget(GPUTexture *pGPUTexture)
{
    if (pGPUTexture == NULL)
        return 0;

    switch (pGPUTexture->GetResourceType())
    {
    case GPU_RESOURCE_TYPE_TEXTURE1D:
        return GL_TEXTURE_1D;

    case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
        return GL_TEXTURE_1D_ARRAY;

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        return GL_TEXTURE_2D;

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        return GL_TEXTURE_2D_ARRAY;

    case GPU_RESOURCE_TYPE_TEXTURE3D:
        return GL_TEXTURE_3D;

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        return GL_TEXTURE_CUBE_MAP;

    case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
        return GL_TEXTURE_CUBE_MAP_ARRAY;
    }

    return 0;
}

GLuint OpenGLHelpers::GetOpenGLTextureId(GPUTexture *pGPUTexture)
{
    if (pGPUTexture == NULL)
        return 0;

    switch (pGPUTexture->GetResourceType())
    {
    case GPU_RESOURCE_TYPE_TEXTURE1D:
        return static_cast<OpenGLGPUTexture1D *>(pGPUTexture)->GetGLTextureId();

    case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
        return static_cast<OpenGLGPUTexture1DArray *>(pGPUTexture)->GetGLTextureId();

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        return static_cast<OpenGLGPUTexture2D *>(pGPUTexture)->GetGLTextureId();

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        return static_cast<OpenGLGPUTexture2DArray *>(pGPUTexture)->GetGLTextureId();

    case GPU_RESOURCE_TYPE_TEXTURE3D:
        return static_cast<OpenGLGPUTexture3D *>(pGPUTexture)->GetGLTextureId();

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        return static_cast<OpenGLGPUTextureCube *>(pGPUTexture)->GetGLTextureId();

    case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
        return static_cast<OpenGLGPUTextureCubeArray *>(pGPUTexture)->GetGLTextureId();
    }

    return 0;
}

void OpenGLHelpers::BindOpenGLTexture(GPUTexture *pGPUTexture)
{
    if (pGPUTexture == nullptr)
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }

    switch (pGPUTexture->GetResourceType())
    {
    case GPU_RESOURCE_TYPE_TEXTURE1D:
        glBindTexture(GL_TEXTURE_1D, static_cast<OpenGLGPUTexture1D *>(pGPUTexture)->GetGLTextureId());
        break;

    case GPU_RESOURCE_TYPE_TEXTURE1DARRAY:
        glBindTexture(GL_TEXTURE_1D_ARRAY, static_cast<OpenGLGPUTexture1DArray *>(pGPUTexture)->GetGLTextureId());
        break;

    case GPU_RESOURCE_TYPE_TEXTURE2D:
        glBindTexture(GL_TEXTURE_2D, static_cast<OpenGLGPUTexture2D *>(pGPUTexture)->GetGLTextureId());
        break;

    case GPU_RESOURCE_TYPE_TEXTURE2DARRAY:
        glBindTexture(GL_TEXTURE_2D_ARRAY, static_cast<OpenGLGPUTexture2DArray *>(pGPUTexture)->GetGLTextureId());
        break;

    case GPU_RESOURCE_TYPE_TEXTURE3D:
        glBindTexture(GL_TEXTURE_3D, static_cast<OpenGLGPUTexture3D *>(pGPUTexture)->GetGLTextureId());
        break;

    case GPU_RESOURCE_TYPE_TEXTURECUBE:
        glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<OpenGLGPUTextureCube *>(pGPUTexture)->GetGLTextureId());
        break;

    case GPU_RESOURCE_TYPE_TEXTURECUBEARRAY:
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY,  static_cast<OpenGLGPUTextureCubeArray *>(pGPUTexture)->GetGLTextureId());
        break;
    }
}

#if 0

void OpenGLHelpers::OpenGLSetUniform(GLint Location, GLsizei ArraySize, SHADER_UNIFORM_TYPE UniformType, const void *pData)
{
    static bool useNvidiaHack = false;

/*#if Y_PLATFORM_LINUX
    static bool nvidiaHackInitialized = false;
    if (!nvidiaHackInitialized)
    {
        // I'm nvidia and my drivers are behaving weird, they seem to obey the row major layout for uniform buffers,
        // but also reverses the ordering of program uniforms. This only happens on linux, but not windows...
        // WTF?! Fix me properly at some point...
        if (Y_strcmp(reinterpret_cast<const char *>(glGetString(GL_VENDOR)), "NVIDIA Corporation") == 0)
            useNvidiaHack = true;

        nvidiaHackInitialized = true;
    }
#endif*/

    switch (UniformType)
    {
    case SHADER_UNIFORM_TYPE_BOOL:
        glUniform1iv(Location, ArraySize, reinterpret_cast<const GLint *>(pData));
        break;

    case SHADER_UNIFORM_TYPE_INT:
        glUniform1iv(Location, ArraySize, reinterpret_cast<const GLint *>(pData));
        break;

    case SHADER_UNIFORM_TYPE_INT2:
        glUniform2iv(Location, ArraySize, reinterpret_cast<const GLint *>(pData));
        break;

    case SHADER_UNIFORM_TYPE_INT3:
        glUniform3iv(Location, ArraySize, reinterpret_cast<const GLint *>(pData));
        break;

    case SHADER_UNIFORM_TYPE_INT4:
        glUniform4iv(Location, ArraySize, reinterpret_cast<const GLint *>(pData));
        break;

    case SHADER_UNIFORM_TYPE_FLOAT:
        glUniform1fv(Location, ArraySize, reinterpret_cast<const GLfloat *>(pData));
        break;

    case SHADER_UNIFORM_TYPE_FLOAT2:
        glUniform2fv(Location, ArraySize, reinterpret_cast<const GLfloat *>(pData));
        break;

    case SHADER_UNIFORM_TYPE_FLOAT3:
        glUniform3fv(Location, ArraySize, reinterpret_cast<const GLfloat *>(pData));
        break;

    case SHADER_UNIFORM_TYPE_FLOAT4:
        glUniform4fv(Location, ArraySize, reinterpret_cast<const GLfloat *>(pData));
        break;

    case SHADER_UNIFORM_TYPE_FLOAT2X2:
        glUniformMatrix2fv(Location, ArraySize, (useNvidiaHack) ? GL_FALSE : GL_TRUE, reinterpret_cast<const GLfloat *>(pData));
        break;

    case SHADER_UNIFORM_TYPE_FLOAT3X3:
        glUniformMatrix3fv(Location, ArraySize, (useNvidiaHack) ? GL_FALSE : GL_TRUE, reinterpret_cast<const GLfloat *>(pData));
        break;

    case SHADER_UNIFORM_TYPE_FLOAT3X4:
        glUniformMatrix4x3fv(Location, ArraySize, GL_TRUE, reinterpret_cast<const GLfloat *>(pData));
        break;

    case SHADER_UNIFORM_TYPE_FLOAT4X4:
        glUniformMatrix4fv(Location, ArraySize, (useNvidiaHack) ? GL_FALSE : GL_TRUE, reinterpret_cast<const GLfloat *>(pData));
        break;

    default:
        UnreachableCode();
        break;
    }
}

#endif

void OpenGLHelpers::ApplySamplerStateToTexture(GLenum target, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc)
{
    glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, pSamplerStateDesc->BorderColor);
    glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_ALWAYS);
    glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glTexParameterf(target, GL_TEXTURE_LOD_BIAS, pSamplerStateDesc->LODBias);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, OpenGLTypeConversion::GetOpenGLTextureMinFilter(pSamplerStateDesc->Filter));
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, OpenGLTypeConversion::GetOpenGLTextureMagFilter(pSamplerStateDesc->Filter));
    glTexParameterf(target, GL_TEXTURE_MIN_LOD, (float)pSamplerStateDesc->MinLOD);
    glTexParameterf(target, GL_TEXTURE_MAX_LOD, (float)pSamplerStateDesc->MaxLOD);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressU));
    glTexParameteri(target, GL_TEXTURE_WRAP_T, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressV));
    glTexParameteri(target, GL_TEXTURE_WRAP_R, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressW));
}

void OpenGLHelpers::SetObjectDebugName(GLenum type, GLuint id, const char *debugName)
{
#ifdef Y_BUILD_CONFIG_DEBUG
    if (GLAD_GL_KHR_debug)
    {
        uint32 length = Y_strlen(debugName);
        if (length > 0)
            glObjectLabel(type, id, length, debugName);
    }
#endif
}
