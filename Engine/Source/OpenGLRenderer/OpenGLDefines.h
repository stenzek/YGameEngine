#pragma once

namespace NameTables {
    Y_Declare_NameTable(GLErrors);
}

namespace OpenGLTypeConversion
{
    void GetOpenGLVersionForRendererFeatureLevel(RENDERER_FEATURE_LEVEL featureLevel, uint32 *pMajorVersion, uint32 *pMinorVersion);
    RENDERER_FEATURE_LEVEL GetRendererFeatureLevelForOpenGLVersion(uint32 majorVersion, uint32 minorVersion);
    
    // shader stuff
    void GetOpenGLVAOTypeAndSizeForVertexElementType(GPU_VERTEX_ELEMENT_TYPE elementType, bool *pIntegerType, GLenum *pGLType, GLint *pGLSize, GLboolean *pGLNormalized);

    // state stuff
    GLenum GetOpenGLComparisonFunc(GPU_COMPARISON_FUNC ComparisonFunc);
    GLenum GetOpenGLComparisonMode(TEXTURE_FILTER Filter);
    GLenum GetOpenGLTextureMinFilter(TEXTURE_FILTER Filter);
    GLenum GetOpenGLTextureMagFilter(TEXTURE_FILTER Filter);
    GLenum GetOpenGLTextureWrap(TEXTURE_ADDRESS_MODE AddressMode);
    GLenum GetOpenGLPolygonMode(RENDERER_FILL_MODE FillMode);
    GLenum GetOpenGLCullFace(RENDERER_CULL_MODE CullMode);
    GLenum GetOpenGLStencilOp(RENDERER_STENCIL_OP StencilOp);
    GLenum GetOpenGLBlendEquation(RENDERER_BLEND_OP BlendOp);
    GLenum GetOpenGLBlendFunc(RENDERER_BLEND_OPTION BlendFactor);
    GLenum GetOpenGLTextureTarget(TEXTURE_TYPE textureType);

    // Convert pixel format to GL texture internalFormat, format, type
    bool GetOpenGLTextureFormat(PIXEL_FORMAT PixelFormat, GLint *pGLInternalFormat, GLenum *pGLFormat, GLenum *pGLType);
}

namespace OpenGLHelpers
{
    GLenum GetOpenGLTextureTarget(GPUTexture *pGPUTexture);
    GLuint GetOpenGLTextureId(GPUTexture *pGPUTexture);
    void BindOpenGLTexture(GPUTexture *pGPUTexture);

    //void OpenGLSetUniform(GLint Location, GLsizei ArraySize, SHADER_UNIFORM_TYPE UniformType, const void *pData);

    void ApplySamplerStateToTexture(GLenum target, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc);

    void SetObjectDebugName(GLenum type, GLuint id, const char *debugName);
}
