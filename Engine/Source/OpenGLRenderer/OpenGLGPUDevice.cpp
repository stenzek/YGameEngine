#include "OpenGLRenderer/PrecompiledHeader.h"
#include "OpenGLRenderer/OpenGLGPUDevice.h"
#include "Engine/EngineCVars.h"
Log_SetChannel(OpenGLRenderBackend);

Y_DECLARE_THREAD_LOCAL(SDL_GLContext) s_currentThreadGLContext = nullptr;
Y_DECLARE_THREAD_LOCAL(uint32) s_currentThreadUploadBatchCount = 0;

OpenGLGPUDevice::UploadContextReference::UploadContextReference(OpenGLGPUDevice *pDevice)
    : pDevice(pDevice)
{
    // Check if we have a context already. If not, use the off-thread context.
    if (s_currentThreadGLContext != nullptr)
    {
        ContextNeedsRelease = false;
        return;
    }
    else if (pDevice->GetOffThreadGLContext() != nullptr)
    {
        ContextNeedsRelease = true;
        return;
    }

    // Flag us as not getting a context.
    ContextNeedsRelease = false;
    pDevice = nullptr;
}

OpenGLGPUDevice::UploadContextReference::~UploadContextReference()
{
    if (ContextNeedsRelease)
        pDevice->ReleaseOffThreadGLContext();
}

bool OpenGLGPUDevice::UploadContextReference::HasContext() const
{
    return (pDevice != nullptr);
}

OpenGLGPUDevice::OpenGLGPUDevice(SDL_GLContext pMainGLContext, SDL_GLContext pOffThreadGLContext, OpenGLGPUOutputBuffer *pImplicitOutputBuffer, RENDERER_FEATURE_LEVEL featureLevel, TEXTURE_PLATFORM texturePlatform, PIXEL_FORMAT outputBackBufferFormat, PIXEL_FORMAT outputDepthStencilFormat)
    : m_pMainGLContext(pMainGLContext)
    , m_pOffThreadGLContext(pOffThreadGLContext)
    , m_pImmediateContext(nullptr)
    , m_pImplicitOutputBuffer(pImplicitOutputBuffer)
    , m_featureLevel(featureLevel)
    , m_texturePlatform(texturePlatform)
    , m_outputBackBufferFormat(outputBackBufferFormat)
    , m_outputDepthStencilFormat(outputDepthStencilFormat)
{
    m_pImplicitOutputBuffer->AddRef();
}

OpenGLGPUDevice::~OpenGLGPUDevice()
{
    DebugAssert(m_pImmediateContext == nullptr);

    // clear current context
    SDL_GL_MakeCurrent(nullptr, nullptr);

    // nuke off-thread gl context
    SDL_GL_DeleteContext(m_pOffThreadGLContext);

    // nuke main GL context
    SDL_GL_DeleteContext(m_pMainGLContext);

    // now the window/buffer can go
    m_pImplicitOutputBuffer->Release();
}

RENDERER_PLATFORM OpenGLGPUDevice::GetPlatform() const
{
    return RENDERER_PLATFORM_OPENGL;
}

RENDERER_FEATURE_LEVEL OpenGLGPUDevice::GetFeatureLevel() const
{
    return m_featureLevel;
}

TEXTURE_PLATFORM OpenGLGPUDevice::GetTexturePlatform() const
{
    return m_texturePlatform;
}

void OpenGLGPUDevice::GetCapabilities(RendererCapabilities *pCapabilities) const
{
    // run glget calls
    uint32 maxTextureAnisotropy = 0;
    uint32 maxVertexAttributes = 0;
    uint32 maxColorAttachments = 0;
    uint32 maxUniformBufferBindings = 0;
    uint32 maxTextureUnits = 0;
    uint32 maxRenderTargets = 0;
    glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, reinterpret_cast<GLint *>(&maxTextureAnisotropy));
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, reinterpret_cast<GLint *>(&maxVertexAttributes));
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, reinterpret_cast<GLint *>(&maxColorAttachments));
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, reinterpret_cast<GLint *>(&maxUniformBufferBindings));
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, reinterpret_cast<GLint *>(&maxTextureUnits));
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, reinterpret_cast<GLint *>(&maxRenderTargets));

    pCapabilities->MaxTextureAnisotropy = maxTextureAnisotropy;
    pCapabilities->MaximumVertexBuffers = maxVertexAttributes;
    pCapabilities->MaximumConstantBuffers = maxUniformBufferBindings;
    pCapabilities->MaximumTextureUnits = maxTextureUnits;
    pCapabilities->MaximumSamplers = maxTextureUnits;
    pCapabilities->MaximumRenderTargets = maxRenderTargets;
    pCapabilities->MaxTextureAnisotropy = maxTextureAnisotropy;
    pCapabilities->SupportsCommandLists = false;
    //pCapabilities->SupportsMultithreadedResourceCreation = true;
    pCapabilities->SupportsMultithreadedResourceCreation = false;
    pCapabilities->SupportsDrawBaseVertex = true; // @TODO
    pCapabilities->SupportsDepthTextures = (GLAD_GL_ARB_depth_texture == GL_TRUE);
    pCapabilities->SupportsTextureArrays = (GLAD_GL_EXT_texture_array == GL_TRUE);
    pCapabilities->SupportsCubeMapTextureArrays = (GLAD_GL_ARB_texture_cube_map_array == GL_TRUE);
    pCapabilities->SupportsGeometryShaders = (GLAD_GL_EXT_geometry_shader4 == GL_TRUE);
    pCapabilities->SupportsSinglePassCubeMaps = (GLAD_GL_EXT_geometry_shader4 == GL_TRUE && GLAD_GL_ARB_viewport_array == GL_TRUE);
    pCapabilities->SupportsInstancing = (GLAD_GL_EXT_draw_instanced == GL_TRUE);
}

bool OpenGLGPUDevice::CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat /*= NULL*/) const
{
    // @TODO
    return true;
}

void OpenGLGPUDevice::CorrectProjectionMatrix(float4x4 &projectionMatrix) const
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

float OpenGLGPUDevice::GetTexelOffset() const
{
    return 0.0f;
}

SDL_GLContext OpenGLGPUDevice::GetOffThreadGLContext()
{
    DebugAssert(s_currentThreadGLContext == nullptr);

    m_offThreadGLContextLock.Lock();

    SDL_GL_MakeCurrent(m_pImplicitOutputBuffer->GetSDLWindow(), m_pOffThreadGLContext);
    s_currentThreadGLContext = m_pOffThreadGLContext;

    return m_pOffThreadGLContext;
}

void OpenGLGPUDevice::ReleaseOffThreadGLContext()
{
    DebugAssert(s_currentThreadGLContext == m_pOffThreadGLContext);

    // glFlush() or glFinish() for shared lists?
    glFinish();

    s_currentThreadGLContext = nullptr;
    SDL_GL_MakeCurrent(nullptr, nullptr);

    m_offThreadGLContextLock.Unlock();
}


void OpenGLGPUDevice::BindMutatorTextureUnit()
{
    if (m_pImmediateContext != nullptr)
        m_pImmediateContext->BindMutatorTextureUnit();
}

void OpenGLGPUDevice::RestoreMutatorTextureUnit()
{
    if (m_pImmediateContext != nullptr)
        m_pImmediateContext->RestoreMutatorTextureUnit();
}

void OpenGLGPUDevice::BeginResourceBatchUpload()
{
    // First time?
    if ((s_currentThreadUploadBatchCount++) == 0)
    {
        // Either on main thread or no upload begun.
        DebugAssert(s_currentThreadGLContext == m_pMainGLContext || s_currentThreadGLContext == nullptr);

        // If off-thread, get a context.
        if (s_currentThreadGLContext == nullptr)
            GetOffThreadGLContext();
    }
}

void OpenGLGPUDevice::EndResourceBatchUpload()
{
    DebugAssert(s_currentThreadUploadBatchCount > 0);

    if ((--s_currentThreadUploadBatchCount) == 0)
    {
        DebugAssert(s_currentThreadGLContext != nullptr);

        // Are we on an off thread?
        if (s_currentThreadGLContext != m_pMainGLContext)
        {
            // Release context.
            ReleaseOffThreadGLContext();
        }
    }
}

// ------------------

OpenGLGPUSamplerState::OpenGLGPUSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, GLuint samplerID)
    : GPUSamplerState(pSamplerStateDesc),
      m_GLSamplerID(samplerID)
{

}

OpenGLGPUSamplerState::~OpenGLGPUSamplerState()
{
    glDeleteSamplers(1, &m_GLSamplerID);
}

void OpenGLGPUSamplerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 128;
}

void OpenGLGPUSamplerState::SetDebugName(const char *name)
{
    OpenGLHelpers::SetObjectDebugName(GL_SAMPLER, m_GLSamplerID, name);
}

GPUSamplerState *OpenGLGPUDevice::CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc)
{
    UploadContextReference ctxRef(this);
    if (!ctxRef.HasContext())
        return nullptr;

    GL_CHECKED_SECTION_BEGIN();

    GLuint samplerID = 0;
    glGenSamplers(1, &samplerID);
    if (samplerID == 0)
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateSamplerState: Sampler allocation failed: ");
        return nullptr;
    }

    glSamplerParameterfv(samplerID, GL_TEXTURE_BORDER_COLOR, pSamplerStateDesc->BorderColor);
    glSamplerParameteri(samplerID, GL_TEXTURE_COMPARE_FUNC, OpenGLTypeConversion::GetOpenGLComparisonFunc(pSamplerStateDesc->ComparisonFunc));
    glSamplerParameteri(samplerID, GL_TEXTURE_COMPARE_MODE, OpenGLTypeConversion::GetOpenGLComparisonMode(pSamplerStateDesc->Filter));
    glSamplerParameterf(samplerID, GL_TEXTURE_LOD_BIAS, pSamplerStateDesc->LODBias);
    glSamplerParameteri(samplerID, GL_TEXTURE_MIN_FILTER, OpenGLTypeConversion::GetOpenGLTextureMinFilter(pSamplerStateDesc->Filter));
    glSamplerParameteri(samplerID, GL_TEXTURE_MAG_FILTER, OpenGLTypeConversion::GetOpenGLTextureMagFilter(pSamplerStateDesc->Filter));
    glSamplerParameterf(samplerID, GL_TEXTURE_MIN_LOD, (float)pSamplerStateDesc->MinLOD);
    glSamplerParameterf(samplerID, GL_TEXTURE_MAX_LOD, (float)pSamplerStateDesc->MaxLOD);
    glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_S, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressU));
    glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_T, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressV));
    glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_R, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressW));

    if (pSamplerStateDesc->Filter == TEXTURE_FILTER_ANISOTROPIC || pSamplerStateDesc->Filter == TEXTURE_FILTER_COMPARISON_ANISOTROPIC)
        glSamplerParameterf(samplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)pSamplerStateDesc->MaxAnisotropy);
    else
        glSamplerParameterf(samplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);

    return new OpenGLGPUSamplerState(pSamplerStateDesc, samplerID);
}

OpenGLGPURasterizerState::OpenGLGPURasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc)
    : GPURasterizerState(pRasterizerStateDesc)
{
    m_GLPolygonMode = OpenGLTypeConversion::GetOpenGLPolygonMode(pRasterizerStateDesc->FillMode);
    m_GLCullFace = OpenGLTypeConversion::GetOpenGLCullFace(pRasterizerStateDesc->CullMode);
    m_GLCullEnabled = (pRasterizerStateDesc->CullMode != RENDERER_CULL_NONE) ? true : false;
    m_GLFrontFace = (pRasterizerStateDesc->FrontCounterClockwise) ? GL_CCW : GL_CW;
    m_GLScissorTestEnable = pRasterizerStateDesc->ScissorEnable;
    m_GLDepthClampEnable = !pRasterizerStateDesc->DepthClipEnable;
}

OpenGLGPURasterizerState::~OpenGLGPURasterizerState()
{

}

void OpenGLGPURasterizerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 0;
}

void OpenGLGPURasterizerState::SetDebugName(const char *name)
{

}

void OpenGLGPURasterizerState::Apply()
{
    glPolygonMode(GL_FRONT_AND_BACK, m_GLPolygonMode);
    glCullFace(m_GLCullFace);
    (m_GLCullEnabled) ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    glFrontFace(m_GLFrontFace);
    (m_GLScissorTestEnable) ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
    (m_GLDepthClampEnable) ? glEnable(GL_DEPTH_CLAMP) : glDisable(GL_DEPTH_CLAMP);
}

void OpenGLGPURasterizerState::Unapply()
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_CLAMP);
}

GPURasterizerState *OpenGLGPUDevice::CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc)
{
    return new OpenGLGPURasterizerState(pRasterizerStateDesc);
}

OpenGLGPUDepthStencilState::OpenGLGPUDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc)
    : GPUDepthStencilState(pDepthStencilStateDesc)
{
    m_GLDepthTestEnable = pDepthStencilStateDesc->DepthTestEnable;
    m_GLDepthMask = (pDepthStencilStateDesc->DepthWriteEnable) ? GL_TRUE : GL_FALSE;
    m_GLDepthFunc = OpenGLTypeConversion::GetOpenGLComparisonFunc(pDepthStencilStateDesc->DepthFunc);
    m_GLStencilTestEnable = pDepthStencilStateDesc->StencilTestEnable;
    m_GLStencilReadMask = pDepthStencilStateDesc->StencilReadMask;
    m_GLStencilWriteMask = pDepthStencilStateDesc->StencilWriteMask;
    m_GLStencilFuncFront = OpenGLTypeConversion::GetOpenGLComparisonFunc(pDepthStencilStateDesc->StencilFrontFace.CompareFunc);
    m_GLStencilOpFrontFail = OpenGLTypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilFrontFace.FailOp);
    m_GLStencilOpFrontDepthFail = OpenGLTypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilFrontFace.DepthFailOp);
    m_GLStencilOpFrontPass = OpenGLTypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilFrontFace.PassOp);
    m_GLStencilFuncBack = OpenGLTypeConversion::GetOpenGLComparisonFunc(pDepthStencilStateDesc->StencilBackFace.CompareFunc);
    m_GLStencilOpBackFail = OpenGLTypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilBackFace.FailOp);
    m_GLStencilOpBackDepthFail = OpenGLTypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilBackFace.DepthFailOp);
    m_GLStencilOpBackPass = OpenGLTypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilBackFace.PassOp);
}

OpenGLGPUDepthStencilState::~OpenGLGPUDepthStencilState()
{

}

void OpenGLGPUDepthStencilState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 0;
}

void OpenGLGPUDepthStencilState::SetDebugName(const char *name)
{

}

void OpenGLGPUDepthStencilState::Apply(uint8 StencilRef)
{
    (m_GLDepthTestEnable) ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    glDepthMask(m_GLDepthMask);
    glDepthFunc(m_GLDepthFunc);
    (m_GLStencilTestEnable) ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
    glStencilFuncSeparate(GL_FRONT, m_GLStencilFuncFront, StencilRef, m_GLStencilReadMask);
    glStencilOpSeparate(GL_FRONT, m_GLStencilOpFrontFail, m_GLStencilOpFrontDepthFail, m_GLStencilOpFrontPass);
    glStencilFuncSeparate(GL_BACK, m_GLStencilFuncBack, StencilRef, m_GLStencilReadMask);
    glStencilOpSeparate(GL_BACK, m_GLStencilOpBackFail, m_GLStencilOpBackDepthFail, m_GLStencilOpBackPass);
    glStencilMask(m_GLStencilWriteMask);
}

void OpenGLGPUDepthStencilState::Unapply()
{
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_STENCIL_TEST);
    glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS, 0, 0xFF);
    glStencilOpSeparate(GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);
}


GPUDepthStencilState *OpenGLGPUDevice::CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc)
{
    return new OpenGLGPUDepthStencilState(pDepthStencilStateDesc);
}

OpenGLGPUBlendState::OpenGLGPUBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc)
    : GPUBlendState(pBlendStateDesc)
{
    m_GLBlendEnable = pBlendStateDesc->BlendEnable;
    m_GLBlendEquationRGB = OpenGLTypeConversion::GetOpenGLBlendEquation(pBlendStateDesc->BlendOp);
    m_GLBlendEquationAlpha = OpenGLTypeConversion::GetOpenGLBlendEquation(pBlendStateDesc->BlendOpAlpha);
    m_GLBlendFuncSrcRGB = OpenGLTypeConversion::GetOpenGLBlendFunc(pBlendStateDesc->SrcBlend);
    m_GLBlendFuncSrcAlpha = OpenGLTypeConversion::GetOpenGLBlendFunc(pBlendStateDesc->SrcBlendAlpha);
    m_GLBlendFuncDstRGB = OpenGLTypeConversion::GetOpenGLBlendFunc(pBlendStateDesc->DestBlend);
    m_GLBlendFuncDstAlpha = OpenGLTypeConversion::GetOpenGLBlendFunc(pBlendStateDesc->DestBlendAlpha);
    m_GLColorWritesEnabled = pBlendStateDesc->ColorWriteEnable;
}

OpenGLGPUBlendState::~OpenGLGPUBlendState()
{

}

void OpenGLGPUBlendState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 0;
}

void OpenGLGPUBlendState::SetDebugName(const char *name)
{

}

void OpenGLGPUBlendState::Apply()
{
    (m_GLBlendEnable) ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    glBlendEquationSeparate(m_GLBlendEquationRGB, m_GLBlendEquationAlpha);
    glBlendFuncSeparate(m_GLBlendFuncSrcRGB, m_GLBlendFuncDstRGB, m_GLBlendFuncSrcAlpha, m_GLBlendFuncDstAlpha);
    (m_GLColorWritesEnabled) ? glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE) : glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}

void OpenGLGPUBlendState::Unapply()
{
    glDisable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

GPUBlendState *OpenGLGPUDevice::CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc)
{
    return new OpenGLGPUBlendState(pBlendStateDesc);
}
