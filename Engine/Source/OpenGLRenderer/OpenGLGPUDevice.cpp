#include "OpenGLRenderer/PrecompiledHeader.h"
#include "OpenGLRenderer/OpenGLGPUDevice.h"
#include "Engine/EngineCVars.h"
Log_SetChannel(OpenGLGPUDevice);

OpenGLGPUDevice::OpenGLGPUDevice(SDL_GLContext pSDLGLContext, PIXEL_FORMAT outputBackBufferFormat, PIXEL_FORMAT outputDepthStencilFormat)
    : m_pSDLGLContext(pSDLGLContext)
    , m_outputBackBufferFormat(outputBackBufferFormat)
    , m_outputDepthStencilFormat(outputDepthStencilFormat)
{

}

OpenGLGPUDevice::~OpenGLGPUDevice()
{
    // nuke GL context
    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DeleteContext(m_pSDLGLContext);
}


void OpenGLGPUDevice::BindMutatorTextureUnit()
{
    if (m_pGPUContext != nullptr)
        m_pGPUContext->BindMutatorTextureUnit();
}

void OpenGLGPUDevice::RestoreMutatorTextureUnit()
{
    if (m_pGPUContext != nullptr)
        m_pGPUContext->RestoreMutatorTextureUnit();
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
