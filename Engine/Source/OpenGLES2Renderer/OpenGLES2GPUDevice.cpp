#include "OpenGLES2Renderer/PrecompiledHeader.h"
#include "OpenGLES2Renderer/OpenGLES2GPUDevice.h"
#include "OpenGLES2Renderer/OpenGLES2ConstantLibrary.h"
#include "Engine/EngineCVars.h"
Log_SetChannel(OpenGLES2RenderBackend);

OpenGLES2GPUDevice::OpenGLES2GPUDevice(OpenGLES2RenderBackend *pBackend, SDL_GLContext pSDLGLContext, PIXEL_FORMAT outputBackBufferFormat, PIXEL_FORMAT outputDepthStencilFormat)
    : m_pRenderBackend(pBackend)
    , m_pSDLGLContext(pSDLGLContext)
    , m_outputBackBufferFormat(outputBackBufferFormat)
    , m_outputDepthStencilFormat(outputDepthStencilFormat)
{

}

OpenGLES2GPUDevice::~OpenGLES2GPUDevice()
{
    // nuke GL context
    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DeleteContext(m_pSDLGLContext);
}

void OpenGLES2GPUDevice::BindMutatorTextureUnit()
{
    if (m_pGPUContext != nullptr)
        m_pGPUContext->BindMutatorTextureUnit();
}

void OpenGLES2GPUDevice::RestoreMutatorTextureUnit()
{
    if (m_pGPUContext != nullptr)
        m_pGPUContext->RestoreMutatorTextureUnit();
}

// ------------------

OpenGLES2GPURasterizerState::OpenGLES2GPURasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc)
    : GPURasterizerState(pRasterizerStateDesc)
{
    m_GLCullFace = OpenGLES2TypeConversion::GetOpenGLCullFace(pRasterizerStateDesc->CullMode);
    m_GLCullEnabled = (pRasterizerStateDesc->CullMode != RENDERER_CULL_NONE) ? true : false;
    m_GLFrontFace = (pRasterizerStateDesc->FrontCounterClockwise) ? GL_CCW : GL_CW;
    m_GLScissorTestEnable = pRasterizerStateDesc->ScissorEnable;
}

OpenGLES2GPURasterizerState::~OpenGLES2GPURasterizerState()
{

}

void OpenGLES2GPURasterizerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 0;
}

void OpenGLES2GPURasterizerState::SetDebugName(const char *name)
{

}

void OpenGLES2GPURasterizerState::Apply()
{
    glCullFace(m_GLCullFace);
    (m_GLCullEnabled) ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    glFrontFace(m_GLFrontFace);
    (m_GLScissorTestEnable) ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
}

void OpenGLES2GPURasterizerState::Unapply()
{
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glDisable(GL_SCISSOR_TEST);
}

GPURasterizerState *OpenGLES2GPUDevice::CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc)
{
    return new OpenGLES2GPURasterizerState(pRasterizerStateDesc);
}

OpenGLES2GPUDepthStencilState::OpenGLES2GPUDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc)
    : GPUDepthStencilState(pDepthStencilStateDesc)
{
    m_GLDepthTestEnable = pDepthStencilStateDesc->DepthTestEnable;
    m_GLDepthMask = (pDepthStencilStateDesc->DepthWriteEnable) ? GL_TRUE : GL_FALSE;
    m_GLDepthFunc = OpenGLES2TypeConversion::GetOpenGLComparisonFunc(pDepthStencilStateDesc->DepthFunc);
    m_GLStencilTestEnable = pDepthStencilStateDesc->StencilTestEnable;
    m_GLStencilReadMask = pDepthStencilStateDesc->StencilReadMask;
    m_GLStencilWriteMask = pDepthStencilStateDesc->StencilWriteMask;
    m_GLStencilFuncFront = OpenGLES2TypeConversion::GetOpenGLComparisonFunc(pDepthStencilStateDesc->StencilFrontFace.CompareFunc);
    m_GLStencilOpFrontFail = OpenGLES2TypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilFrontFace.FailOp);
    m_GLStencilOpFrontDepthFail = OpenGLES2TypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilFrontFace.DepthFailOp);
    m_GLStencilOpFrontPass = OpenGLES2TypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilFrontFace.PassOp);
    m_GLStencilFuncBack = OpenGLES2TypeConversion::GetOpenGLComparisonFunc(pDepthStencilStateDesc->StencilBackFace.CompareFunc);
    m_GLStencilOpBackFail = OpenGLES2TypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilBackFace.FailOp);
    m_GLStencilOpBackDepthFail = OpenGLES2TypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilBackFace.DepthFailOp);
    m_GLStencilOpBackPass = OpenGLES2TypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilBackFace.PassOp);
}

OpenGLES2GPUDepthStencilState::~OpenGLES2GPUDepthStencilState()
{

}

void OpenGLES2GPUDepthStencilState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 0;
}

void OpenGLES2GPUDepthStencilState::SetDebugName(const char *name)
{

}

void OpenGLES2GPUDepthStencilState::Apply(uint8 StencilRef)
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

void OpenGLES2GPUDepthStencilState::Unapply()
{
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_STENCIL_TEST);
    glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS, 0, 0xFF);
    glStencilOpSeparate(GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);
}

GPUDepthStencilState *OpenGLES2GPUDevice::CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc)
{
    return new OpenGLES2GPUDepthStencilState(pDepthStencilStateDesc);
}

OpenGLES2GPUBlendState::OpenGLES2GPUBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc)
    : GPUBlendState(pBlendStateDesc)
{
    m_GLBlendEnable = pBlendStateDesc->BlendEnable;
    m_GLBlendEquationRGB = OpenGLES2TypeConversion::GetOpenGLBlendEquation(pBlendStateDesc->BlendOp);
    m_GLBlendEquationAlpha = OpenGLES2TypeConversion::GetOpenGLBlendEquation(pBlendStateDesc->BlendOpAlpha);
    m_GLBlendFuncSrcRGB = OpenGLES2TypeConversion::GetOpenGLBlendFunc(pBlendStateDesc->SrcBlend);
    m_GLBlendFuncSrcAlpha = OpenGLES2TypeConversion::GetOpenGLBlendFunc(pBlendStateDesc->SrcBlendAlpha);
    m_GLBlendFuncDstRGB = OpenGLES2TypeConversion::GetOpenGLBlendFunc(pBlendStateDesc->DestBlend);
    m_GLBlendFuncDstAlpha = OpenGLES2TypeConversion::GetOpenGLBlendFunc(pBlendStateDesc->DestBlendAlpha);
    m_GLColorWritesEnabled = pBlendStateDesc->ColorWriteEnable;
}

OpenGLES2GPUBlendState::~OpenGLES2GPUBlendState()
{

}

void OpenGLES2GPUBlendState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 0;
}

void OpenGLES2GPUBlendState::SetDebugName(const char *name)
{

}

void OpenGLES2GPUBlendState::Apply()
{
    (m_GLBlendEnable) ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    glBlendEquationSeparate(m_GLBlendEquationRGB, m_GLBlendEquationAlpha);
    glBlendFuncSeparate(m_GLBlendFuncSrcRGB, m_GLBlendFuncDstRGB, m_GLBlendFuncSrcAlpha, m_GLBlendFuncDstAlpha);
    (m_GLColorWritesEnabled) ? glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE) : glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}

void OpenGLES2GPUBlendState::Unapply()
{
    glDisable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

GPUBlendState *OpenGLES2GPUDevice::CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc)
{
    return new OpenGLES2GPUBlendState(pBlendStateDesc);
}

// stubs

GPUSamplerState *OpenGLES2GPUDevice::CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc)
{
    Log_ErrorPrintf("OpenGLES2GPUDevice::CreateSamplerState: Unsupported on GLES");
    return nullptr;
}

GPUQuery *OpenGLES2GPUDevice::CreateQuery(GPU_QUERY_TYPE type)
{
    Log_ErrorPrintf("OpenGLES2GPUDevice::CreateQuery: Unsupported on GLES");
    return nullptr;
}

GPUTexture1D *OpenGLES2GPUDevice::CreateTexture1D(const GPU_TEXTURE1D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */)
{
    Log_ErrorPrintf("OpenGLES2GPUDevice::CreateTexture1D: Unsupported on GLES");
    return nullptr;
}

GPUTexture1DArray *OpenGLES2GPUDevice::CreateTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */)
{
    Log_ErrorPrintf("OpenGLES2GPUDevice::CreateTexture1DArray: Unsupported on GLES");
    return nullptr;
}

GPUTexture2DArray *OpenGLES2GPUDevice::CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */)
{
    Log_ErrorPrintf("OpenGLES2GPUDevice::CreateTexture2DArray: Unsupported on GLES");
    return nullptr;
}

GPUTexture3D *OpenGLES2GPUDevice::CreateTexture3D(const GPU_TEXTURE3D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */, const uint32 *pInitialDataSlicePitch /* = NULL */)
{
    Log_ErrorPrintf("OpenGLES2GPUDevice::CreateTexture3D: Unsupported on GLES");
    return nullptr;
}

GPUTextureCubeArray *OpenGLES2GPUDevice::CreateTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */)
{
    Log_ErrorPrintf("OpenGLES2GPUDevice::CreateTextureCubeArray: Unsupported on GLES");
    return nullptr;
}

GPUComputeView *OpenGLES2GPUDevice::CreateComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc)
{
    Log_ErrorPrintf("OpenGLES2GPUDevice::CreateComputeView: Unsupported on GLES");
    return nullptr;
}
