#pragma once
#include "OpenGLES2Renderer/OpenGLES2Common.h"
#include "OpenGLES2Renderer/OpenGLES2RendererOutputBuffer.h"
#include "OpenGLES2Renderer/OpenGLES2GPUContext.h"
#include "OpenGLES2Renderer/OpenGLES2GPUTexture.h"
#include "OpenGLES2Renderer/OpenGLES2GPUBuffer.h"

class OpenGLES2ConstantLibrary;

class OpenGLES2GPURasterizerState : public GPURasterizerState
{
public:
    OpenGLES2GPURasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc);
    virtual ~OpenGLES2GPURasterizerState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const GLenum GetGLCullFace() const { return m_GLCullFace; }
    const GLenum GetGLFrontFace() const { return m_GLFrontFace; }
    const bool GetGLScissorTestEnable() const { return m_GLScissorTestEnable; }

    void Apply();
    void Unapply();

private:
    GLenum m_GLCullFace;
    bool m_GLCullEnabled;
    GLenum m_GLFrontFace;
    bool m_GLScissorTestEnable;
};

class OpenGLES2GPUDepthStencilState : public GPUDepthStencilState
{
public:
    OpenGLES2GPUDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc);
    virtual ~OpenGLES2GPUDepthStencilState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const bool GetGLDepthTestEnable() const { return m_GLDepthTestEnable; }
    const GLboolean GetGLDepthMask() const { return m_GLDepthMask; }
    const GLenum GetGLDepthFunc() const { return m_GLDepthFunc; }
    const bool GetGLStencilTestEnable() const { return m_GLDepthTestEnable; }
    const GLuint GetGLStencilReadMask() const { return m_GLStencilReadMask; }
    const GLuint GetGLStencilWriteMask() const { return m_GLStencilWriteMask; }
    const GLenum GetGLStencilFuncFront() const { return m_GLStencilFuncFront; }
    const GLenum GetGLStencilFuncBack() const { return m_GLStencilFuncBack; }

    void Apply(uint8 StencilRef);
    void Unapply();

private:
    bool m_GLDepthTestEnable;
    GLboolean m_GLDepthMask;
    GLenum m_GLDepthFunc;
    bool m_GLStencilTestEnable;
    GLuint m_GLStencilReadMask;
    GLuint m_GLStencilWriteMask;
    GLenum m_GLStencilFuncFront;
    GLenum m_GLStencilOpFrontFail;
    GLenum m_GLStencilOpFrontDepthFail;
    GLenum m_GLStencilOpFrontPass;
    GLenum m_GLStencilFuncBack;
    GLenum m_GLStencilOpBackFail;
    GLenum m_GLStencilOpBackDepthFail;
    GLenum m_GLStencilOpBackPass;
};

class OpenGLES2GPUBlendState : public GPUBlendState
{
public:
    OpenGLES2GPUBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc);
    virtual ~OpenGLES2GPUBlendState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const bool GetGLBlendEnable() const { return m_GLBlendEnable; }
    const GLenum GetGLBlendEquationRGB() const { return m_GLBlendEquationRGB; }
    const GLenum GetGLBlendEquationAlpha() const { return m_GLBlendEquationAlpha; }
    const GLenum GetGLBlendFuncSrcRGB() const { return m_GLBlendFuncSrcRGB; }
    const GLenum GetGLBlendFuncDstRGB() const { return m_GLBlendFuncSrcRGB; }
    const GLenum GetGLBlendFuncSrcAlpha() const { return m_GLBlendFuncSrcAlpha; }
    const GLenum GetGLBlendFuncDstAlpha() const { return m_GLBlendFuncDstAlpha; }
    const bool GetGLColorWritesEnabled() const { return m_GLColorWritesEnabled; }

    void Apply();
    void Unapply();

private:
    bool m_GLBlendEnable;
    GLenum m_GLBlendEquationRGB;
    GLenum m_GLBlendEquationAlpha;
    GLenum m_GLBlendFuncSrcRGB;
    GLenum m_GLBlendFuncDstRGB;
    GLenum m_GLBlendFuncSrcAlpha;
    GLenum m_GLBlendFuncDstAlpha;
    bool m_GLColorWritesEnabled;
};

class OpenGLES2Renderer : public Renderer
{
public:
    OpenGLES2Renderer();
    ~OpenGLES2Renderer();

    // --- our methods ---
    OpenGLES2GPUContext *GetOpenGLMainContext() const { return m_pMainContext; }
    const OpenGLES2ConstantLibrary *GetConstantLibrary() const { return m_pConstantLibrary; }

    // creation
    bool Create(const RendererInitializationParameters *pInitializationParameters);

    // --- virtual methods ---
    void CorrectProjectionMatrix(float4x4 &projectionMatrix) const override;
    bool CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat = NULL) const override;

    // Creates a context capable of uploading resources owned by the calling thread.
    virtual GPUContext *CreateUploadContext() override;

    // Creates a swap chain on an existing window.
    virtual GPUOutputBuffer *CreateOutputBuffer(RenderSystemWindowHandle hWnd, RENDERER_VSYNC_TYPE vsyncType) override;

    // Creates a swap chain on a new window.
    virtual RendererOutputWindow *CreateOutputWindow(const char *windowTitle, uint32 windowWidth, uint32 windowHeight, RENDERER_VSYNC_TYPE vsyncType) override;

    // Resource creation
    virtual GPUQuery *CreateQuery(GPU_QUERY_TYPE type) override;
    virtual GPUBuffer *CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData = NULL) override;
    virtual GPUTexture1D *CreateTexture1D(const GPU_TEXTURE1D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTexture1DArray *CreateTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTexture2D *CreateTexture2D(const GPU_TEXTURE2D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTexture2DArray *CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTexture3D *CreateTexture3D(const GPU_TEXTURE3D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL, const uint32 *pInitialDataSlicePitch = NULL) override;
    virtual GPUTextureCube *CreateTextureCube(const GPU_TEXTURECUBE_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTextureCubeArray *CreateTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUDepthTexture *CreateDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pTextureDesc) override;
    virtual GPUSamplerState *CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc) override;
    virtual GPURenderTargetView *CreateRenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc) override;
    virtual GPUDepthStencilBufferView *CreateDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc) override;
    virtual GPUComputeView *CreateComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc) override;
    virtual GPUDepthStencilState *CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc) override;
    virtual GPURasterizerState *CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc) override;
    virtual GPUBlendState *CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc) override;
    virtual GPUShaderProgram *CreateGraphicsProgram(const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream) override;
    virtual GPUShaderProgram *CreateComputeProgram(ByteStream *pByteCodeStream) override;

    virtual RendererOutputWindow *GetImplicitOutputWindow() override { return static_cast<RendererOutputWindow *>(m_pImplicitRenderWindow); }

    virtual GPUContext *GetGPUContext() const override { return static_cast<GPUContext *>(m_pMainContext); }

    virtual void HandlePendingSDLEvents() override;

    virtual void GetGPUMemoryUsage(GPUMemoryUsage *pMemoryUsage) const override;

private:
    // create the initial (main) context
    SDL_GLContext CreateMainSDLGLContext(OpenGLES2RendererOutputBuffer *pOutputBuffer);

    // create an upload context
    SDL_GLContext CreateUploadSDLGLContext(OpenGLES2RendererOutputBuffer *pOutputBuffer);

    // vars
    PIXEL_FORMAT m_swapChainBackBufferFormat;
    PIXEL_FORMAT m_swapChainDepthStencilBufferFormat;
    OpenGLES2GPUContext *m_pMainContext;
    OpenGLES2RendererOutputWindow *m_pImplicitRenderWindow;

    // constant library
    OpenGLES2ConstantLibrary *m_pConstantLibrary;
};


