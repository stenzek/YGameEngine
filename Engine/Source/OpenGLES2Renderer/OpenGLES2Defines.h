#pragma once

namespace NameTables {
    Y_Declare_NameTable(GLESErrors);
}

extern GLenum g_eGLES2LastError;
extern void GLES2Renderer_PrintError(const char *Format, ...);

#define GL_CHECKED_SECTION_BEGIN() glGetError();
#define GL_CHECK_ERROR_STATE() ((g_eGLES2LastError = glGetError()) != GL_NO_ERROR)
#define GL_PRINT_ERROR(...) GLES2Renderer_PrintError(__VA_ARGS__)

namespace OpenGLES2TypeConversion
{   
    // shader stuff
    void GetOpenGLVAOTypeAndSizeForVertexElementType(GPU_VERTEX_ELEMENT_TYPE elementType, bool *pIntegerType, GLenum *pGLType, GLint *pGLSize, GLboolean *pGLNormalized);

    // state stuff
    GLenum GetOpenGLComparisonFunc(GPU_COMPARISON_FUNC ComparisonFunc);
    GLenum GetOpenGLComparisonMode(TEXTURE_FILTER Filter);
    GLenum GetOpenGLTextureMinFilter(TEXTURE_FILTER Filter, bool hasMips);
    GLenum GetOpenGLTextureMagFilter(TEXTURE_FILTER Filter);
    GLenum GetOpenGLTextureWrap(TEXTURE_ADDRESS_MODE AddressMode);
    GLenum GetOpenGLCullFace(RENDERER_CULL_MODE CullMode);
    GLenum GetOpenGLStencilOp(RENDERER_STENCIL_OP StencilOp);
    GLenum GetOpenGLBlendEquation(RENDERER_BLEND_OP BlendOp);
    GLenum GetOpenGLBlendFunc(RENDERER_BLEND_OPTION BlendFactor);
    GLenum GetOpenGLTextureTarget(TEXTURE_TYPE textureType);

    // Convert pixel format to GL texture internalFormat, format, type
    bool GetOpenGLTextureFormat(PIXEL_FORMAT PixelFormat, GLint *pGLInternalFormat, GLenum *pGLFormat, GLenum *pGLType);
}

namespace OpenGLES2Helpers
{
    void CorrectProjectionMatrix(float4x4 &projectionMatrix);

    GLenum GetOpenGLTextureTarget(GPUTexture *pGPUTexture);
    GLuint GetOpenGLTextureId(GPUTexture *pGPUTexture);
    void BindOpenGLTexture(GPUTexture *pGPUTexture);

    void GLUniformWrapper(SHADER_PARAMETER_TYPE type, GLuint location, GLuint arraySize, const void *pValue);    

    void SetObjectDebugName(GLenum type, GLuint id, const char *debugName);
}
