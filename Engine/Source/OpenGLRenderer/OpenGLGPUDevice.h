#pragma once
#include "OpenGLRenderer/OpenGLCommon.h"
#include "OpenGLRenderer/OpenGLGPUOutputBuffer.h"
#include "OpenGLRenderer/OpenGLGPUContext.h"
#include "OpenGLRenderer/OpenGLGPUTexture.h"
#include "OpenGLRenderer/OpenGLGPUBuffer.h"

class OpenGLGPUSamplerState : public GPUSamplerState
{
public:
    OpenGLGPUSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, GLuint samplerID);
    virtual ~OpenGLGPUSamplerState();

    const GLuint GetGLSamplerID() const { return m_GLSamplerID; }

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

private:
    GLuint m_GLSamplerID;
};

class OpenGLGPURasterizerState : public GPURasterizerState
{
public:
    OpenGLGPURasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc);
    virtual ~OpenGLGPURasterizerState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const GLenum GetGLPolygonMode() const { return m_GLPolygonMode; }
    const GLenum GetGLCullFace() const { return m_GLCullFace; }
    const GLenum GetGLFrontFace() const { return m_GLFrontFace; }
    const bool GetGLScissorTestEnable() const { return m_GLScissorTestEnable; }

    void Apply();
    void Unapply();

private:
    GLenum m_GLPolygonMode;
    GLenum m_GLCullFace;
    bool m_GLCullEnabled;
    GLenum m_GLFrontFace;
    bool m_GLScissorTestEnable;
    bool m_GLDepthClampEnable;
};

class OpenGLGPUDepthStencilState : public GPUDepthStencilState
{
public:
    OpenGLGPUDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc);
    virtual ~OpenGLGPUDepthStencilState();

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

class OpenGLGPUBlendState : public GPUBlendState
{
public:
    OpenGLGPUBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc);
    virtual ~OpenGLGPUBlendState();

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

class OpenGLGPUDevice : public GPUDevice
{
public:
    struct UploadContextReference
    {
        UploadContextReference(OpenGLGPUDevice *pDevice);
        ~UploadContextReference();
        bool HasContext() const;

        OpenGLGPUDevice *pDevice;
        bool ContextNeedsRelease;
    };

public:
    OpenGLGPUDevice(SDL_GLContext pMainGLContext, SDL_GLContext pOffThreadGLContext, OpenGLGPUOutputBuffer *pImplicitOutputBuffer,
                    RENDERER_FEATURE_LEVEL featureLevel, TEXTURE_PLATFORM texturePlatform, PIXEL_FORMAT outputBackBufferFormat, PIXEL_FORMAT outputDepthStencilFormat);

    ~OpenGLGPUDevice();

    // Device queries.
    virtual RENDERER_PLATFORM GetPlatform() const override final;
    virtual RENDERER_FEATURE_LEVEL GetFeatureLevel() const override final;
    virtual TEXTURE_PLATFORM GetTexturePlatform() const override final;
    virtual void GetCapabilities(RendererCapabilities *pCapabilities) const override final;
    virtual bool CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat = nullptr) const override final;

    // Creates a swap chain on an existing window.
    virtual GPUOutputBuffer *CreateOutputBuffer(RenderSystemWindowHandle hWnd, RENDERER_VSYNC_TYPE vsyncType) override final;
    virtual GPUOutputBuffer *CreateOutputBuffer(SDL_Window *pSDLWindow, RENDERER_VSYNC_TYPE vsyncType) override final;

    // Resource creation
    virtual GPUQuery *CreateQuery(GPU_QUERY_TYPE type) override final;
    virtual GPUBuffer *CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData = nullptr) override final;
    virtual GPUTexture1D *CreateTexture1D(const GPU_TEXTURE1D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = nullptr, const uint32 *pInitialDataPitch = nullptr) override final;
    virtual GPUTexture1DArray *CreateTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = nullptr, const uint32 *pInitialDataPitch = nullptr) override final;
    virtual GPUTexture2D *CreateTexture2D(const GPU_TEXTURE2D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = nullptr, const uint32 *pInitialDataPitch = nullptr) override final;
    virtual GPUTexture2DArray *CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = nullptr, const uint32 *pInitialDataPitch = nullptr) override final;
    virtual GPUTexture3D *CreateTexture3D(const GPU_TEXTURE3D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = nullptr, const uint32 *pInitialDataPitch = nullptr, const uint32 *pInitialDataSlicePitch = nullptr) override final;
    virtual GPUTextureCube *CreateTextureCube(const GPU_TEXTURECUBE_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = nullptr, const uint32 *pInitialDataPitch = nullptr) override final;
    virtual GPUTextureCubeArray *CreateTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = nullptr, const uint32 *pInitialDataPitch = nullptr) override final;
    virtual GPUDepthTexture *CreateDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pTextureDesc) override final;
    virtual GPUSamplerState *CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc) override final;
    virtual GPURenderTargetView *CreateRenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc) override final;
    virtual GPUDepthStencilBufferView *CreateDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc) override final;
    virtual GPUComputeView *CreateComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc) override final;
    virtual GPUDepthStencilState *CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc) override final;
    virtual GPURasterizerState *CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc) override final;
    virtual GPUBlendState *CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc) override final;
    virtual GPUShaderProgram *CreateGraphicsProgram(const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream) override final;
    virtual GPUShaderProgram *CreateComputeProgram(ByteStream *pByteCodeStream) override final;

    // off-thread resource creation
    virtual void BeginResourceBatchUpload() override final;
    virtual void EndResourceBatchUpload() override final;

    // helper methods
    SDL_GLContext GetGLContext() { return m_pMainGLContext; }
    SDL_GLContext GetOffThreadGLContext();
    void ReleaseOffThreadGLContext();
    OpenGLGPUContext *GetImmediateContext() { return m_pImmediateContext; }
    void SetImmediateContext(OpenGLGPUContext *pContext) { m_pImmediateContext = pContext; }
    void BindMutatorTextureUnit();
    void RestoreMutatorTextureUnit();

private:
    SDL_GLContext m_pMainGLContext;
    SDL_GLContext m_pOffThreadGLContext;
    Mutex m_offThreadGLContextLock;

    OpenGLGPUContext *m_pImmediateContext;
    OpenGLGPUOutputBuffer *m_pImplicitOutputBuffer;

    RENDERER_FEATURE_LEVEL m_featureLevel;
    TEXTURE_PLATFORM m_texturePlatform;

    PIXEL_FORMAT m_outputBackBufferFormat;
    PIXEL_FORMAT m_outputDepthStencilFormat;
};


