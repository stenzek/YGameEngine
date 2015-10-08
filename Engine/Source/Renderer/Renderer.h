#pragma once
#include "Renderer/Common.h"
#include "Renderer/RendererTypes.h"
#include "Engine/CommandQueue.h"

// Include helper classes
#include "Renderer/VertexFactories/PlainVertexFactory.h"        // <--- TODO REMOVE ME
#include "Renderer/MiniGUIContext.h"
#include "Renderer/ShaderMap.h"

// Forward declare classes
class Camera;
class World;
class RenderWorld;
class Font;
class ShaderProgram;

// Declare the render system window handle type
#if defined(Y_PLATFORM_WINDOWS)
    typedef HWND RenderSystemWindowHandle;
#else
    typedef void *RenderSystemWindowHandle;
#endif

// Required for output window
struct SDL_Window;

// Base class of all gpu resources
class GPUResource : public ReferenceCounted
{
    DeclareNonCopyable(GPUResource);

public:
    GPUResource() {}
    virtual ~GPUResource() {}

    // Common methods for all resources
    virtual GPU_RESOURCE_TYPE GetResourceType() const = 0;
    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const = 0;
    virtual void SetDebugName(const char *name) = 0;
};

class GPURasterizerState : public GPUResource
{
public:
    GPURasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); }
    virtual ~GPURasterizerState() {}

    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_RASTERIZER_STATE; }

    const RENDERER_RASTERIZER_STATE_DESC *GetDesc() const { return &m_desc; }

protected:
    RENDERER_RASTERIZER_STATE_DESC m_desc;

private:
};

class GPUDepthStencilState : public GPUResource
{
public:
    GPUDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); }
    virtual ~GPUDepthStencilState() {}

    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_DEPTH_STENCIL_STATE; }

    const RENDERER_DEPTHSTENCIL_STATE_DESC *GetDesc() const { return &m_desc; }

protected:
    RENDERER_DEPTHSTENCIL_STATE_DESC m_desc;

private:
};

class GPUBlendState : public GPUResource
{
public:
    GPUBlendState(const RENDERER_BLEND_STATE_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); }
    virtual ~GPUBlendState() {}

    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_BLEND_STATE; }

    const RENDERER_BLEND_STATE_DESC *GetDesc() const { return &m_desc; }

protected:
    RENDERER_BLEND_STATE_DESC m_desc;

private:
};

class GPUSamplerState : public GPUResource
{
public:
    GPUSamplerState(const GPU_SAMPLER_STATE_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); }
    virtual ~GPUSamplerState() {}

    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_SAMPLER_STATE; }

    const GPU_SAMPLER_STATE_DESC *GetDesc() const { return &m_desc; }

protected:
    GPU_SAMPLER_STATE_DESC m_desc;
};

class GPUBuffer : public GPUResource
{
public:
    GPUBuffer(const GPU_BUFFER_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); }
    virtual ~GPUBuffer() {}

    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_BUFFER; }

    const GPU_BUFFER_DESC *GetDesc() const { return &m_desc; }

protected:
    GPU_BUFFER_DESC m_desc;

private:
};

class GPUTexture : public GPUResource
{
public:
    virtual ~GPUTexture() {}

    virtual TEXTURE_TYPE GetTextureType() const = 0;
};

class GPUTexture1D : public GPUTexture
{
public:
    GPUTexture1D(const GPU_TEXTURE1D_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); }
    virtual ~GPUTexture1D() {}

    virtual TEXTURE_TYPE GetTextureType() const override { return TEXTURE_TYPE_1D; }
    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_TEXTURE1D; }

    const GPU_TEXTURE1D_DESC *GetDesc() const { return &m_desc; }

protected:
    GPU_TEXTURE1D_DESC m_desc;
};

class GPUTexture1DArray : public GPUTexture
{
public:
    GPUTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); }
    virtual ~GPUTexture1DArray() {}

    virtual TEXTURE_TYPE GetTextureType() const override { return TEXTURE_TYPE_1D_ARRAY; }
    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_TEXTURE1DARRAY; }

    const GPU_TEXTURE1DARRAY_DESC *GetDesc() const { return &m_desc; }

protected:
    GPU_TEXTURE1DARRAY_DESC m_desc;
};

class GPUTexture2D : public GPUTexture
{
public:
    GPUTexture2D(const GPU_TEXTURE2D_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); }
    virtual ~GPUTexture2D() {}

    virtual TEXTURE_TYPE GetTextureType() const override { return TEXTURE_TYPE_2D; }
    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_TEXTURE2D; }

    const GPU_TEXTURE2D_DESC *GetDesc() const { return &m_desc; }

protected:
    GPU_TEXTURE2D_DESC m_desc;
};

class GPUTexture2DArray : public GPUTexture
{
public:
    GPUTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); }
    virtual ~GPUTexture2DArray() {}

    virtual TEXTURE_TYPE GetTextureType() const override { return TEXTURE_TYPE_2D_ARRAY; }
    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_TEXTURE2DARRAY; }

    const GPU_TEXTURE2DARRAY_DESC *GetDesc() const { return &m_desc; }

protected:
    GPU_TEXTURE2DARRAY_DESC m_desc;
};

class GPUTexture3D : public GPUTexture
{
public:
    GPUTexture3D(const GPU_TEXTURE3D_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); }
    virtual ~GPUTexture3D() {}

    virtual TEXTURE_TYPE GetTextureType() const override { return TEXTURE_TYPE_3D; }
    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_TEXTURE3D; }

    const GPU_TEXTURE3D_DESC *GetDesc() const { return &m_desc; }

protected:
    GPU_TEXTURE3D_DESC m_desc;
};

class GPUTextureCube : public GPUTexture
{
public:
    GPUTextureCube(const GPU_TEXTURECUBE_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); }
    virtual ~GPUTextureCube() {}

    virtual TEXTURE_TYPE GetTextureType() const override { return TEXTURE_TYPE_CUBE; }
    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_TEXTURECUBE; }

    const GPU_TEXTURECUBE_DESC *GetDesc() const { return &m_desc; }

protected:
    GPU_TEXTURECUBE_DESC m_desc;
};

class GPUTextureCubeArray : public GPUTexture
{
public:
    GPUTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); }
    virtual ~GPUTextureCubeArray() {}

    virtual TEXTURE_TYPE GetTextureType() const override { return TEXTURE_TYPE_CUBE_ARRAY; }
    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_TEXTURECUBEARRAY; }

    const GPU_TEXTURECUBEARRAY_DESC *GetDesc() const { return &m_desc; }

protected:
    GPU_TEXTURECUBEARRAY_DESC m_desc;
};

class GPUDepthTexture : public GPUTexture
{
public:
    GPUDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); }
    virtual ~GPUDepthTexture() {}

    virtual TEXTURE_TYPE GetTextureType() const override { return TEXTURE_TYPE_DEPTH; }
    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_DEPTH_TEXTURE; }

    const GPU_DEPTH_TEXTURE_DESC *GetDesc() const { return &m_desc; }

protected:
    GPU_DEPTH_TEXTURE_DESC m_desc;
};

class GPURenderTargetView : public GPUResource
{
public:
    GPURenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); m_pTexture = pTexture; m_pTexture->AddRef(); }
    virtual ~GPURenderTargetView() { m_pTexture->Release(); }

    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_RENDER_TARGET_VIEW; }
  
    const GPU_RENDER_TARGET_VIEW_DESC *GetDesc() const { return &m_desc; }
    GPUTexture *GetTargetTexture() const { return m_pTexture; }

protected:
    GPUTexture *m_pTexture;
    GPU_RENDER_TARGET_VIEW_DESC m_desc;
};

class GPUDepthStencilBufferView : public GPUResource
{
public:
    GPUDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); m_pTexture = pTexture; m_pTexture->AddRef(); }
    virtual ~GPUDepthStencilBufferView() { m_pTexture->Release(); }

    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_DEPTH_BUFFER_VIEW; }

    const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *GetDesc() const { return &m_desc; }
    GPUTexture *GetTargetTexture() const { return m_pTexture; }

protected:
    GPUTexture *m_pTexture;
    GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC m_desc;
};

class GPUComputeView : public GPUResource
{
public:
    GPUComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc) { Y_memcpy(&m_desc, pDesc, sizeof(m_desc)); m_pResource->AddRef(); }
    virtual ~GPUComputeView() { m_pResource->Release(); }

    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_COMPUTE_VIEW; }

    const GPU_COMPUTE_VIEW_DESC *GetDesc() const { return &m_desc; }
    GPUResource *GetTargetResource() const { return m_pResource; }

protected:
    GPUResource *m_pResource;
    GPU_COMPUTE_VIEW_DESC m_desc;
};

class GPUQuery : public GPUResource
{
public:
    virtual ~GPUQuery() {}

    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_QUERY; }
    virtual GPU_QUERY_TYPE GetQueryType() const = 0;
};

class GPUShaderProgram : public GPUResource
{
public:
    GPUShaderProgram() {}
    virtual ~GPUShaderProgram() {}

    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_SHADER_PROGRAM; }

    // --- parameters ---
    virtual uint32 GetParameterCount() const = 0;
    virtual void GetParameterInformation(uint32 index, const char **name, SHADER_PARAMETER_TYPE *type, uint32 *arraySize) = 0;
    virtual void SetParameterValue(GPUContext *pContext, uint32 index, SHADER_PARAMETER_TYPE valueType, const void *pValue) = 0;
    virtual void SetParameterValueArray(GPUContext *pContext, uint32 index, SHADER_PARAMETER_TYPE valueType, const void *pValue, uint32 firstElement, uint32 numElements) = 0;
    virtual void SetParameterStruct(GPUContext *pContext, uint32 index, const void *pValue, uint32 valueSize) = 0;
    virtual void SetParameterStructArray(GPUContext *pContext, uint32 index, const void *pValue, uint32 valueSize, uint32 firstElement, uint32 numElements) = 0;
    virtual void SetParameterResource(GPUContext *pContext, uint32 index, GPUResource *pResource) = 0;
    virtual void SetParameterTexture(GPUContext *pContext, uint32 index, GPUTexture *pTexture, GPUSamplerState *pSamplerState) = 0;
};

class GPUShaderPipeline : public GPUShaderProgram
{
public:
    GPUShaderPipeline() {}
    virtual ~GPUShaderPipeline() {}

    virtual GPU_RESOURCE_TYPE GetResourceType() const override { return GPU_RESOURCE_TYPE_SHADER_PIPELINE; }

    // TODO: Accessors for states....
};

class GPUOutputBuffer : public ReferenceCounted
{
    DeclareNonCopyable(GPUOutputBuffer);

public:
    GPUOutputBuffer(RENDERER_VSYNC_TYPE vsyncType) : m_vsyncType(vsyncType) {}
    virtual ~GPUOutputBuffer() {}

    // Get the dimensions of this swap chain.
    virtual uint32 GetWidth() const = 0;
    virtual uint32 GetHeight() const = 0;

    // Change the vsync type associated with this swap chain.
    RENDERER_VSYNC_TYPE GetVSyncType() const { return m_vsyncType; }
    virtual void SetVSyncType(RENDERER_VSYNC_TYPE vsyncType) = 0;

protected:
    RENDERER_VSYNC_TYPE m_vsyncType;
};

class RendererOutputWindow : public ReferenceCounted
{
    DeclareNonCopyable(RendererOutputWindow);

public:
    RendererOutputWindow(SDL_Window *pSDLWindow, GPUOutputBuffer *pBuffer, RENDERER_FULLSCREEN_STATE fullscreenState);
    virtual ~RendererOutputWindow();

    // window accessors
    SDL_Window *GetSDLWindow() const { return m_pSDLWindow; }
    GPUOutputBuffer *GetOutputBuffer() const { return m_pOutputBuffer; }
    RENDERER_FULLSCREEN_STATE GetFullscreenState() const { return m_fullscreenState; }
    void SetFullscreenState(RENDERER_FULLSCREEN_STATE state) { m_fullscreenState = state; }
    void SetDimensions(uint32 width, uint32 height) { m_width = width; m_height = height; }
    void SetOutputBuffer(GPUOutputBuffer *pBuffer) { m_pOutputBuffer = pBuffer; }
    int32 GetPositionX() const { return m_positionX; }
    int32 GetPositionY() const { return m_positionY; }
    uint32 GetWidth() const { return m_width; }
    uint32 GetHeight() const { return m_height; }
    bool HasFocus() const { return m_hasFocus; }
    bool IsVisible() const { return m_visible; }
    const String &GetTitle() const { return m_title; }
    bool IsMouseGrabbed() const { return m_mouseGrabbed; }
    bool IsMouseRelativeMovementEnabled() const { return m_mouseRelativeMovement; }

    // Change the caption of the window
    void SetWindowTitle(const char *title);

    // Show/hide the window
    void SetWindowVisibility(bool visible);

    // Change the position of the window
    void SetWindowPosition(int32 x, int32 y);

    // Changes the window size
    void SetWindowSize(uint32 width, uint32 height);

    // Capture the mouse, if it leaves the window it will still receive movement events
    void SetMouseGrab(bool enabled);

    // Enable relative mouse movement, when enabled the mouse cursor will be locked to the window and the cursor will be hidden
    void SetMouseRelativeMovement(bool enabled);

protected:
    SDL_Window *m_pSDLWindow;
    GPUOutputBuffer *m_pOutputBuffer;
    RENDERER_FULLSCREEN_STATE m_fullscreenState;
    int32 m_positionX;
    int32 m_positionY;
    uint32 m_width;
    uint32 m_height;
    bool m_hasFocus;
    bool m_visible;
    String m_title;
    bool m_mouseGrabbed;
    bool m_mouseRelativeMovement;
};

// constants interface
class GPUContextConstants
{
    DeclareNonCopyable(GPUContextConstants);

public:
    GPUContextConstants(GPUCommandList *pCommandList);
    virtual ~GPUContextConstants();

    // Object Constants
    const float4x4 &GetLocalToWorldMatrix() const { return m_localToWorldMatrix; }
    void SetLocalToWorldMatrix(const float4x4 &rMatrix, bool Commit = true);
    void SetMaterialTintColor(const float4 &tintColor, bool Commit = true);

    // Camera Constants
    const float4x4 &GetCameraViewMatrix() const { return m_cameraViewMatrix; }
    const float4x4 &GetCameraProjectionMatrix() const { return m_cameraProjectionMatrix; }
    const float3 &GetCameraEyePosition() const { return m_cameraEyePosition; }
    void SetCameraViewMatrix(const float4x4 &rMatrix, bool Commit = true);
    void SetCameraProjectionMatrix(const float4x4 &rMatrix, bool Commit = true);
    void SetCameraEyePosition(const float3 &Position, bool Commit = true);
    void SetFromCamera(const Camera &camera, bool commit = true);

    // Viewport Constants
    const float2 &GetViewportOffset() const { return m_viewportOffset; }
    const float2 &GetViewportSize() const { return m_viewportSize; }
    void SetViewportOffset(float offsetX, float offsetY, bool commit = true);
    void SetViewportSize(float width, float height, bool commit = true);

    // World Constants
    const float GetWorldTime() const { return m_worldTime; }
    void SetWorldTime(float worldTime, bool commit = true);

    // Commit
    void CommitChanges();

    // Commit the shared program globals constant buffer
    void CommitGlobalConstantBufferChanges();

private:
    GPUCommandList *m_pCommandList;

    // Object constants
    float4x4 m_localToWorldMatrix;
    float4 m_materialTintColor;

    // View constants
    float4x4 m_cameraViewMatrix;
    float4x4 m_cameraProjectionMatrix;
    float3 m_cameraEyePosition;

    // Viewport constants
    float2 m_viewportOffset;
    float2 m_viewportSize;

    // World constants
    float m_worldTime;

    // dirty flags
    bool m_recalculateCombinedMatrices;
    bool m_recalculateViewportFractions;
};

class GPUDevice : public ReferenceCounted
{
    DeclareNonCopyable(GPUDevice);

public:
    GPUDevice() {}
    virtual ~GPUDevice() {}

    // Creates a swap chain on an existing window.
    virtual GPUOutputBuffer *CreateOutputBuffer(RenderSystemWindowHandle hWnd, RENDERER_VSYNC_TYPE vsyncType) = 0;
    virtual GPUOutputBuffer *CreateOutputBuffer(SDL_Window *pSDLWindow, RENDERER_VSYNC_TYPE vsyncType) = 0;

    // Resource creation
    virtual GPUDepthStencilState *CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc) = 0;
    virtual GPURasterizerState *CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc) = 0;
    virtual GPUBlendState *CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc) = 0;
    virtual GPUQuery *CreateQuery(GPU_QUERY_TYPE type) = 0;
    virtual GPUBuffer *CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData = NULL) = 0;
    virtual GPUTexture1D *CreateTexture1D(const GPU_TEXTURE1D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) = 0;
    virtual GPUTexture1DArray *CreateTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) = 0;
    virtual GPUTexture2D *CreateTexture2D(const GPU_TEXTURE2D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) = 0;
    virtual GPUTexture2DArray *CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) = 0;
    virtual GPUTexture3D *CreateTexture3D(const GPU_TEXTURE3D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL, const uint32 *pInitialDataSlicePitch = NULL) = 0;
    virtual GPUTextureCube *CreateTextureCube(const GPU_TEXTURECUBE_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) = 0;
    virtual GPUTextureCubeArray *CreateTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) = 0;
    virtual GPUDepthTexture *CreateDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pTextureDesc) = 0;
    virtual GPUSamplerState *CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc) = 0;
    virtual GPURenderTargetView *CreateRenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc) = 0;
    virtual GPUDepthStencilBufferView *CreateDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc) = 0;
    virtual GPUComputeView *CreateComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc) = 0;
    virtual GPUShaderProgram *CreateGraphicsProgram(const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream) = 0;
    virtual GPUShaderProgram *CreateComputeProgram(ByteStream *pByteCodeStream) = 0;
    // CreateGraphicsPipeline
    // CreateComputePipeline

    // When creating resources off-thread, there is an implicit flush/wait after each resource creation. This forces a group to be batched together.
    virtual void BeginResourceBatchUpload() = 0;
    virtual void EndResourceBatchUpload() = 0;
};

class GPUCommandList : public ReferenceCounted
{
    DeclareNonCopyable(GPUCommandList);

public:
    GPUCommandList() {}
    virtual ~GPUCommandList() {}

    // State clearing
    virtual void ClearState(bool clearShaders = true, bool clearBuffers = true, bool clearStates = true, bool clearRenderTargets = true) = 0;

    // Retrieve RendererVariables interface.
    virtual GPUContextConstants *GetConstants() = 0;
    
    // State Management    
    virtual GPURasterizerState *GetRasterizerState() = 0;
    virtual void SetRasterizerState(GPURasterizerState *pRasterizerState) = 0;
    virtual GPUDepthStencilState *GetDepthStencilState() = 0;
    virtual uint8 GetDepthStencilStateStencilRef() = 0;
    virtual void SetDepthStencilState(GPUDepthStencilState *pDepthStencilState, uint8 stencilRef) = 0;
    virtual GPUBlendState *GetBlendState() = 0;
    virtual const float4 &GetBlendStateBlendFactor() = 0;
    virtual void SetBlendState(GPUBlendState *pBlendState, const float4 &blendFactor = float4::One) = 0;

    // Viewport Management
    virtual const RENDERER_VIEWPORT *GetViewport() = 0;
    virtual void SetViewport(const RENDERER_VIEWPORT *pNewViewport) = 0;
    virtual void SetFullViewport(GPUTexture *pForRenderTarget = nullptr) = 0;

    // Scissor Rect Management
    virtual const RENDERER_SCISSOR_RECT *GetScissorRect() = 0;
    virtual void SetScissorRect(const RENDERER_SCISSOR_RECT *pScissorRect) = 0;

    // Texture copying
    virtual bool CopyTexture(GPUTexture2D *pSourceTexture, GPUTexture2D *pDestinationTexture) = 0;
    virtual bool CopyTextureRegion(GPUTexture2D *pSourceTexture, uint32 sourceX, uint32 sourceY, uint32 width, uint32 height, uint32 sourceMipLevel, GPUTexture2D *pDestinationTexture, uint32 destX, uint32 destY, uint32 destMipLevel) = 0;

    // Blit (copy) a texture to the currently bound framebuffer. If this texture is a different size, it'll be resized. This may destroy the shader and target state, so use carefully.
    virtual void BlitFrameBuffer(GPUTexture2D *pTexture, uint32 sourceX, uint32 sourceY, uint32 sourceWidth, uint32 sourceHeight, uint32 destX, uint32 destY, uint32 destWidth, uint32 destHeight, RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER resizeFilter = RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_NEAREST) = 0;

    // Mipmap generation, texture must be created with GPU_TEXTURE_FLAG_GENERATE_MIPS
    virtual void GenerateMips(GPUTexture *pTexture) = 0;

    // Query accessing
    virtual bool BeginQuery(GPUQuery *pQuery) = 0;
    virtual bool EndQuery(GPUQuery *pQuery) = 0;

    // Predicated drawing
    virtual void SetPredication(GPUQuery *pQuery) = 0;

    // RT Clearing
    virtual void ClearTargets(bool clearColor = true, bool clearDepth = true, bool clearStencil = true, const float4 &clearColorValue = float4::Zero, float clearDepthValue = 1.0f, uint8 clearStencilValue = 0) = 0;
    virtual void DiscardTargets(bool discardColor = true, bool discardDepth = true, bool discardStencil = true) = 0;

    // Swap chain changing
    virtual GPUOutputBuffer *GetOutputBuffer() = 0;
    virtual void SetOutputBuffer(GPUOutputBuffer *pOutputBuffer) = 0;

    // Render target changing
    virtual uint32 GetRenderTargets(uint32 nRenderTargets, GPURenderTargetView **ppRenderTargetViews, GPUDepthStencilBufferView **ppDepthBufferView) = 0;
    virtual void SetRenderTargets(uint32 nRenderTargets, GPURenderTargetView **ppRenderTargets, GPUDepthStencilBufferView *pDepthBufferView) = 0;

    // Drawing Setup
    virtual DRAW_TOPOLOGY GetDrawTopology() = 0;
    virtual void SetDrawTopology(DRAW_TOPOLOGY Topology) = 0;
    virtual uint32 GetVertexBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer **ppVertexBuffers, uint32 *pVertexBufferOffsets, uint32 *pVertexBufferStrides) = 0;
    virtual void SetVertexBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer *const *ppVertexBuffers, const uint32 *pVertexBufferOffsets, const uint32 *pVertexBufferStrides) = 0;
    virtual void SetVertexBuffer(uint32 bufferIndex, GPUBuffer *pVertexBuffer, uint32 offset, uint32 stride) = 0;
    virtual void GetIndexBuffer(GPUBuffer **ppBuffer, GPU_INDEX_FORMAT *pFormat, uint32 *pOffset) = 0;
    virtual void SetIndexBuffer(GPUBuffer *pBuffer, GPU_INDEX_FORMAT format, uint32 offset) = 0;

    // Shader Setup
    virtual void SetShaderProgram(GPUShaderProgram *pShaderProgram) = 0;
//     virtual void GetShaderConstantBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer **ppBuffers, uint32 *pBufferOffsets, uint32 *pBufferSizes) = 0;
//     virtual void SetShaderConstantBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer *const *ppBuffers, const uint32 *pBufferOffsets, const uint32 *pBufferSizes) = 0;
//     virtual void SetShaderConstantBuffer(uint32 bufferIndex, GPUBuffer *pBuffer, uint32 offset, uint32 size) = 0;
//     virtual void GetShaderTextures(uint32 firstTexture, uint32 nTextures, GPUTexture **ppTextures) = 0;
//     virtual void SetShaderTextures(uint32 firstTexture, uint32 nTextures, GPUTexture *const *ppTextures) = 0;
//     virtual void SetShaderTexture(uint32 textureIndex, GPUTexture *pTexture) = 0;
//     virtual void GetShaderSamplers(uint32 firstSampler, uint32 nSamplers, GPUSamplerState **ppSamplers) = 0;
//     virtual void SetShaderSamplers(uint32 firstSampler, uint32 nSamplers, GPUSamplerState *const *ppSamplers) = 0;
//     virtual void SetShaderSampler(uint32 samplerIndex, GPUSamplerState *pSampler) = 0;

    // Constant Buffers
    virtual void WriteConstantBuffer(uint32 bufferIndex, uint32 fieldIndex, uint32 offset, uint32 count, const void *pData, bool commit = false) = 0;
    virtual void WriteConstantBufferStrided(uint32 bufferIndex, uint32 fieldIndex, uint32 offset, uint32 bufferStride, uint32 copySize, uint32 count, const void *pData, bool commit = false) = 0;
    virtual void CommitConstantBuffer(uint32 bufferIndex) = 0;

    // Draw calls
    virtual void Draw(uint32 firstVertex, uint32 nVertices) = 0;
    virtual void DrawInstanced(uint32 firstVertex, uint32 nVertices, uint32 nInstances) = 0;
    virtual void DrawIndexed(uint32 startIndex, uint32 nIndices, uint32 baseVertex) = 0;
    virtual void DrawIndexedInstanced(uint32 startIndex, uint32 nIndices, uint32 baseVertex, uint32 nInstances) = 0;

    // Draw calls with user-space buffer
    virtual void DrawUserPointer(const void *pVertices, uint32 vertexSize, uint32 nVertices) = 0;

    // Compute shaders
    virtual void Dispatch(uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ) = 0;
};

class GPUContext : public GPUCommandList
{
    DeclareNonCopyable(GPUContext);

public:
    GPUContext() {}
    virtual ~GPUContext() {}

    // Start of frame
    virtual void BeginFrame() = 0;

    // Ensure all queued commands are sent to the GPU.
    virtual void Flush() = 0;

    // Ensure all commands have been completed by the GPU.
    virtual void Finish() = 0;

    // Swap chain manipulation
    virtual bool GetExclusiveFullScreen() = 0;
    virtual bool SetExclusiveFullScreen(bool enabled, uint32 width, uint32 height, uint32 refreshRate) = 0;
    virtual bool ResizeOutputBuffer(uint32 width = 0, uint32 height = 0) = 0;
    virtual void PresentOutputBuffer(GPU_PRESENT_BEHAVIOUR presentBehaviour) = 0;

    // Command list execution
    virtual GPUCommandList *CreateCommandList() = 0;
    virtual bool OpenCommandList(GPUCommandList *pCommandList) = 0;
    virtual bool CloseCommandList(GPUCommandList *pCommandList) = 0;
    virtual void ExecuteCommandList(GPUCommandList *pCommandList) = 0;

    // Buffer mapping/reading/writing
    virtual bool ReadBuffer(GPUBuffer *pBuffer, void *pDestination, uint32 start, uint32 count) = 0;
    virtual bool WriteBuffer(GPUBuffer *pBuffer, const void *pSource, uint32 start, uint32 count) = 0;
    virtual bool MapBuffer(GPUBuffer *pBuffer, GPU_MAP_TYPE mapType, void **ppPointer) = 0;
    virtual void Unmapbuffer(GPUBuffer *pBuffer, void *pPointer) = 0;

    // Texture reading/writing
    virtual bool ReadTexture(GPUTexture1D *pTexture, void *pDestination, uint32 cbDestination, uint32 mipIndex, uint32 start, uint32 count) = 0;
    virtual bool ReadTexture(GPUTexture1DArray *pTexture, void *pDestination, uint32 cbDestination, uint32 arrayIndex, uint32 mipIndex, uint32 start, uint32 count) = 0;
    virtual bool ReadTexture(GPUTexture2D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) = 0;
    virtual bool ReadTexture(GPUTexture2DArray *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 arrayIndex, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) = 0;
    virtual bool ReadTexture(GPUTexture3D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 destinationSlicePitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 startZ, uint32 countX, uint32 countY, uint32 countZ) = 0;
    virtual bool ReadTexture(GPUTextureCube *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) = 0;
    virtual bool ReadTexture(GPUTextureCubeArray *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 arrayIndex, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) = 0;
    virtual bool ReadTexture(GPUDepthTexture *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 startX, uint32 startY, uint32 countX, uint32 countY) = 0;
    virtual bool WriteTexture(GPUTexture1D *pTexture, const void *pSource, uint32 cbSource, uint32 mipIndex, uint32 start, uint32 count) = 0;
    virtual bool WriteTexture(GPUTexture1DArray *pTexture, const void *pSource, uint32 cbSource, uint32 arrayIndex, uint32 mipIndex, uint32 start, uint32 count) = 0;
    virtual bool WriteTexture(GPUTexture2D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) = 0;
    virtual bool WriteTexture(GPUTexture2DArray *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 arrayIndex, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) = 0;
    virtual bool WriteTexture(GPUTexture3D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 sourceSlicePitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 startZ, uint32 countX, uint32 countY, uint32 countZ) = 0;
    virtual bool WriteTexture(GPUTextureCube *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) = 0;
    virtual bool WriteTexture(GPUTextureCubeArray *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 arrayIndex, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) = 0;
    virtual bool WriteTexture(GPUDepthTexture *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 startX, uint32 startY, uint32 countX, uint32 countY) = 0;

    // Query readback
    virtual GPU_QUERY_GETDATA_RESULT GetQueryData(GPUQuery *pQuery, void *pData, uint32 cbData, uint32 flags) = 0;
};

struct RendererCapabilities
{
    uint32 MaxTextureAnisotropy;
    uint32 MaximumVertexBuffers;
    uint32 MaximumConstantBuffers;
    uint32 MaximumTextureUnits;
    uint32 MaximumSamplers;
    uint32 MaximumRenderTargets;
    struct
    {
        bool SupportsMultithreadedResourceCreation : 1;
        bool SupportsDrawBaseVertex : 1;
        bool SupportsDepthTextures : 1;
        bool SupportsTextureArrays : 1;
        bool SupportsCubeMapTextureArrays : 1;
        bool SupportsGeometryShaders : 1;
        bool SupportsSinglePassCubeMaps : 1;
        bool SupportsInstancing : 1;
    uint32: 1;
    };
};

class RenderBackend
{
public:
    // Device queries.
    virtual RENDERER_PLATFORM GetPlatform() const = 0;
    virtual RENDERER_FEATURE_LEVEL GetFeatureLevel() const = 0;
    virtual TEXTURE_PLATFORM GetTexturePlatform() const = 0;
    virtual void GetCapabilities(RendererCapabilities *pCapabilities) const = 0;
    virtual bool CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat = nullptr) const = 0;

    // Create a device interface. Must be called on the thread that wishes to own this device interface.
    virtual GPUDevice *CreateDeviceInterface() = 0;

    // Shuts down the renderer backend.
    virtual void Shutdown() = 0;
};


struct RendererInitializationParameters
{
    RendererInitializationParameters() 
        : Platform(RENDERER_PLATFORM_D3D11), EnableThreadedRendering(true)
        , BackBufferFormat(PIXEL_FORMAT_R8G8B8A8_UNORM), DepthStencilBufferFormat(PIXEL_FORMAT_D24_UNORM_S8_UINT)
        , HideImplicitSwapChain(false), ImplicitSwapChainCaption("Renderer"), ImplicitSwapChainWidth(800), ImplicitSwapChainHeight(600)
        , ImplicitSwapChainFullScreen(RENDERER_FULLSCREEN_STATE_WINDOWED), ImplicitSwapChainVSyncType(RENDERER_VSYNC_TYPE_NONE)
        , GPUFrameLatency(3)
    {
    }

    // Platform to create.
    RENDERER_PLATFORM Platform;

    // Override default behaviour of creating a render thread should the cvar be set.
    bool EnableThreadedRendering;

    // Pixel format of all on-screen windows.
    PIXEL_FORMAT BackBufferFormat;
    PIXEL_FORMAT DepthStencilBufferFormat;

    // OpenGL at least, requires at least one swap chain be created with the device (implicit swap chain), even if it is not used.
    // If this is set, the swap chain will be created at a minimum size (to save memory) and the render window will be hidden.
    // On other APIs that do not require an implicit swap chain, none will be created.
    bool HideImplicitSwapChain;

    // If created, this specifies the caption of the implicit swap chain window.
    const char *ImplicitSwapChainCaption;

    // If created, this specifies the dimensions of the implicit swap chain window.
    uint32 ImplicitSwapChainWidth;
    uint32 ImplicitSwapChainHeight;

    // If created, whether the implicit swap chain will be created full-screen.
    RENDERER_FULLSCREEN_STATE ImplicitSwapChainFullScreen;

    // Implicit swap chain vsync behaviour
    RENDERER_VSYNC_TYPE ImplicitSwapChainVSyncType;

    // Frame latency
    uint32 GPUFrameLatency;
};

// Renderer stats
class RendererCounters
{
public:
    RendererCounters();
    ~RendererCounters();

    // Counters
    uint32 GetFrameNumber() const { return m_frameNumber; }
    uint32 GetDrawCallCounter() const { return m_drawCallCounter; }
    uint32 GetShaderChangeCounter() const { return m_shaderChangeCounter; }
    uint32 GetPipelineChangeCounter() const { return m_pipelineChangeCounter; }
    uint32 GetFramesDroppedCounter() const { return m_framesDroppedCounter; }

    // Counter updating
    void IncrementDrawCallCounter() { Y_AtomicIncrement(m_drawCallCounter); }
    void IncrementShaderChangeCounter() { Y_AtomicIncrement(m_shaderChangeCounter); }
    void IncrementPipelineChangeCounter() { Y_AtomicIncrement(m_pipelineChangeCounter); }
    void IncrementFramesDroppedCounter() { Y_AtomicIncrement(m_framesDroppedCounter); }
    void ResetPerFrameCounters();

    // Resource memory management
    void OnResourceCreated(const GPUResource *pResource);
    void OnResourceDeleted(const GPUResource *pResource);

private:
    uint32 m_frameNumber;

    uint32 m_drawCallCounter;
    uint32 m_shaderChangeCounter;
    uint32 m_pipelineChangeCounter;
    uint32 m_framesDroppedCounter;

    Y_ATOMIC_DECL ptrdiff_t m_resourceCPUMemoryUsage[GPU_RESOURCE_TYPE_COUNT];
    Y_ATOMIC_DECL ptrdiff_t m_resourceGPUMemoryUsage[GPU_RESOURCE_TYPE_COUNT];
};

class Renderer
{
public:
    class FixedResources
    {
    public:
        struct ScreenQuadVertex
        { 
            float2 Position;
            float2 TexCoord;

            void Set(const float2 &position, const float2 &texcoord) { Position = position; TexCoord = texcoord; }
            void Set(float x, float y, float u, float v) { Position.Set(x, y); TexCoord.Set(u, v); }
        };

    public:
        FixedResources(Renderer *pRenderer);
        ~FixedResources();

        bool CreateResources();
        void ReleaseResources();

        GPURasterizerState *GetRasterizerState(RENDERER_FILL_MODE fillMode = RENDERER_FILL_SOLID, RENDERER_CULL_MODE cullMode = RENDERER_CULL_BACK, bool depthBias = false, bool depthClamping = false, bool scissorTest = false);
        GPUDepthStencilState *GetDepthStencilState(bool depthTestEnabled = true, bool depthWriteEnabled = true, GPU_COMPARISON_FUNC comparisonFunc = GPU_COMPARISON_FUNC_LESS);

        GPUBlendState *GetBlendStateNoBlending() const { return m_pBlendStateNoBlending; }
        GPUBlendState *GetBlendStateNoColorWrites() const { return m_pBlendStateNoColorWrites; }
        GPUBlendState *GetBlendStateAdditive() const { return m_pBlendStateAdditive; }
        GPUBlendState *GetBlendStateAlphaBlending() const { return m_pBlendStateAlphaBlending; }
        GPUBlendState *GetBlendStateAlphaBlendingAdditive() const { return m_pBlendStateAlphaBlendingAdditive; }
        GPUBlendState *GetBlendStatePremultipliedAlpha() const { return m_pBlendStatePremultipliedAlpha; }
        GPUBlendState *GetBlendStatePremultipliedAlphaAdditive() const { return m_pBlendStatePremultipliedAlphaAdditive; }
        GPUBlendState *GetBlendStateBlendFactor() const { return m_pBlendStateBlendFactor; }
        GPUBlendState *GetBlendStateBlendFactorAdditive() const { return m_pBlendStateBlendFactorAdditive; }
        GPUBlendState *GetBlendStateBlendFactorPremultipledAlpha() const { return m_pBlendStateBlendFactorPremultipliedAlpha; }
        GPUBlendState *GetBlendStateBlendFactorApplyBlendFactorAlpha() const { return m_pBlendStateApplyBlendFactorAlpha; }

        GPUSamplerState *GetPointSamplerState() const { return m_pPointSamplerState; }
        GPUSamplerState *GetLinearSamplerState() const { return m_pLinearSamplerState; }
        GPUSamplerState *GetLinearMipSamplerState() const { return m_pLinearMipSamplerState; }

        const Font *GetDebugFont() const { return m_pDebugFont; }
        const Texture2D *GetNoiseTexture() const { return m_pNoiseTexture; }
        const Texture2D *GetRandomTexture() const { return m_pRandomTexture; }

        ShaderProgram *GetTextureBlitShader() const { return m_pTextureBlitShader; }
        ShaderProgram *GetTextureBlitLODShader() const { return m_pTextureBlitLODShader; }
        ShaderProgram *GetDownsampleShader() const { return m_pDownsampleShader; }

        GPUBuffer *GetFullScreenQuadVertexBuffer() const { return m_pFullScreenQuadVertexBuffer; }

        ShaderProgram *GetOverlayShaderColoredScreen() const { return m_pOverlayShaderColoredScreen; }
        ShaderProgram *GetOverlayShaderColoredWorld() const { return m_pOverlayShaderColoredWorld; }
        ShaderProgram *GetOverlayShaderTexturedScreen() const { return m_pOverlayShaderTexturedScreen; }
        ShaderProgram *GetOverlayShaderTexturedWorld() const { return m_pOverlayShaderTexturedWorld; }

        // predefined vertex layouts
        static const GPU_VERTEX_ELEMENT_DESC *GetPositionOnlyVertexAttributes();
        static const uint32 GetPositionOnlyVertexAttributeCount();
        static const GPU_VERTEX_ELEMENT_DESC *GetFullScreenQuadVertexAttributes();
        static const uint32 GetFullScreenQuadVertexAttributeCount();

    private:
        Renderer *m_pRenderer;

        // rasterizer
        // rs[wireframe][culling][depthbias][depthclip][scissor]
        GPURasterizerState *m_pRasterizerStates[RENDERER_FILL_MODE_COUNT][RENDERER_CULL_MODE_COUNT][2][2][2];

        // depthstencil
        // ds[test][writes][comparison]
        GPUDepthStencilState *m_pDepthStencilStates[2][2][GPU_COMPARISON_FUNC_COUNT];

        // blending
        GPUBlendState *m_pBlendStateNoBlending;
        GPUBlendState *m_pBlendStateNoColorWrites;
        GPUBlendState *m_pBlendStateAdditive;
        GPUBlendState *m_pBlendStateAlphaBlending;
        GPUBlendState *m_pBlendStateAlphaBlendingAdditive;
        GPUBlendState *m_pBlendStatePremultipliedAlpha;
        GPUBlendState *m_pBlendStatePremultipliedAlphaAdditive;
        GPUBlendState *m_pBlendStateBlendFactor;
        GPUBlendState *m_pBlendStateBlendFactorAdditive;
        GPUBlendState *m_pBlendStateBlendFactorPremultipliedAlpha;
        GPUBlendState *m_pBlendStateApplyBlendFactorAlpha;

        // sampler states
        GPUSamplerState *m_pPointSamplerState;
        GPUSamplerState *m_pLinearSamplerState;
        GPUSamplerState *m_pLinearMipSamplerState;

        // fixed resources
        const Font *m_pDebugFont;
        const Texture2D *m_pNoiseTexture;
        const Texture2D *m_pRandomTexture;

        // texture copy shader
        ShaderProgram *m_pTextureBlitShader;
        ShaderProgram *m_pTextureBlitLODShader;

        // downsample shader
        ShaderProgram *m_pDownsampleShader;

        // full-screen quad stuff
        GPUBuffer *m_pFullScreenQuadVertexBuffer;

        // overlay shader stuff
        ShaderProgram *m_pOverlayShaderColoredScreen;
        ShaderProgram *m_pOverlayShaderColoredWorld;
        ShaderProgram *m_pOverlayShaderTexturedScreen;
        ShaderProgram *m_pOverlayShaderTexturedWorld;
    };

public:
    Renderer(RenderBackend *pBackendInterface, RendererOutputWindow *pOutputWindow);
    virtual ~Renderer();

    // Find the default renderer platform for this running platform
    static RENDERER_PLATFORM GetDefaultPlatform();

    // Called at start of application.
    static bool Create(const RendererInitializationParameters *pCreateParameters);

    // Shutdown the renderer.
    static void Shutdown();

    // draw a fullscreen quad
    void DrawFullScreenQuad(GPUContext *pContext);

    // copy a texture using a shader
    void BlitTextureUsingShader(GPUContext *pContext, GPUTexture2D *pSourceTexture, uint32 sourceX, uint32 sourceY, uint32 sourceWidth, uint32 sourceHeight, int32 sourceLevel, uint32 destX, uint32 destY, uint32 destWidth, uint32 destHeight, RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER resizeFilter = RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_NEAREST, RENDERER_FRAMEBUFFER_BLIT_BLEND_MODE blendMode = RENDERER_FRAMEBUFFER_BLIT_BLEND_MODE_NONE);

    // helper for creating an index buffer. buffer is freed with Y_free.
    static void BuildIndexBufferContents(uint32 sourceVertexCount, const uint32 *pSourceIndices, uint32 nIndicesPerPrimitive, uint32 nPrimitives, uint32 sourceIndicesStride, void **ppOutBuffer, GPU_INDEX_FORMAT *pOutIndexFormat, uint32 *pOutIndexCount);
    static void FillIndexBufferContents(const uint32 *pSourceIndices, uint32 nIndicesPerPrimitive, uint32 nPrimitives, uint32 sourceIndicesStride, GPU_INDEX_FORMAT indexFormat, void *pDestinationIndices, uint32 cbDestinationIndices);

    // --- helper methods ---

    // device queries
    const RENDERER_PLATFORM GetPlatform() const { return m_eRendererPlatform; }
    const RENDERER_FEATURE_LEVEL GetFeatureLevel() const { return m_eRendererFeatureLevel; }
    const TEXTURE_PLATFORM GetTexturePlatform() const { return m_eTexturePlatform; }
    const RendererCapabilities &GetCapabilities() const { return m_RendererCapabilities; }
    const float GetTexelOffset() const { return m_fTexelOffset; }

    // Accesses the implicit swap chain.
    RendererOutputWindow *GetImplicitOutputWindow() { return m_pImplicitOutputWindow; }
    bool ChangeResolution(RENDERER_FULLSCREEN_STATE state, uint32 width = 0, uint32 height = 0, uint32 refreshRate = 0);

    // raw backend access, should not be needed in most cases.
    RenderBackend *GetBackendInterface() const { return m_pBackendInterface; }

    // creates a device object for the thread calling.
    bool EnableResourceCreationForCurrentThread();
    void DisableResourceCreationForCurrentThread();

    // device/context access. these will return the value for the current thread, and thus attempting to
    // call GetGPUDevice() on a thread that has not registered for uploads, or GetGPUContext() on a thread
    // other than the render thread will cause the process to crash.
    static GPUDevice *GetGPUDevice();
    static GPUContext *GetGPUContext();

    // Stats access
    RendererCounters *GetCounters() { return &m_stats; }

    // render thread
    static const Thread::ThreadIdType GetRenderThreadId() { return s_renderThreadId; }
    static const bool IsOnRenderThread() { return (Thread::GetCurrentThreadId() == s_renderThreadId); }
    static const bool HasRenderThread() { return (s_renderCommandQueue.GetWorkerThreadCount() != 0); }
    static CommandQueue *GetCommandQueue() { return &s_renderCommandQueue; }

    // accesses shader permutation
    ShaderProgram *GetShaderProgram(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const GPU_VERTEX_ELEMENT_DESC *pVertexAttributes, uint32 nVertexAttributes, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    ShaderProgram *GetShaderProgram(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    template<class T> ShaderProgram *GetShaderProgram(uint32 globalShaderFlags, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags) { return GetShaderProgram(globalShaderFlags, SHADER_COMPONENT_INFO(T), baseShaderFlags, pVertexFactoryTypeInfo, vertexFactoryFlags, pMaterialShader, materialShaderFlags); }
    template<class SHADERTYPE, class VERTEXFACTORYTYPE> ShaderProgram *GetShaderProgram(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags) { return GetShaderProgram(globalShaderFlags, SHADER_COMPONENT_INFO(SHADERTYPE), baseShaderFlags, VERTEX_FACTORY_TYPE_INFO(VERTEXFACTORYTYPE), vertexFactoryFlags, pMaterialShader, materialShaderFlags); }

    // determine texture dimensions on a gpu texture
    static uint3 GetTextureDimensions(const GPUTexture *pTexture);

    // convert pixel coordinates to clip space coordinates
    static float2 GetClipSpaceCoordinatesForPixelCoordinates(int32 x, int32 y, uint32 windowWidth, uint32 windowHeight);

    // convert pixel coordinates to texture space coordinates
    static float2 GetTextureSpaceCoordinatesForPixelCoordinates(int32 x, int32 y, uint32 textureWidth, uint32 textureHeight);

    // fixed resource accessors. these DO NOT increment the reference count upon retrieval, as they are assumed to be persistent.
    Renderer::FixedResources *GetFixedResources() { return &m_fixedResources; }

    // gui context
    MiniGUIContext *GetGUIContext() { DebugAssert(Renderer::IsOnRenderThread()); return &m_guiContext; }

    // scissor rect calculators
    static bool CalculateAABoxScissorRect(RENDERER_SCISSOR_RECT *pScissorRect, const AABox &Bounds, const float4x4 &ViewMatrix, int32 RTWidth, int32 RTHeight);
    static bool CalculatePointLightScissorRect(RENDERER_SCISSOR_RECT *pScissorRect, const float3 &LightPosition, const float &LightRange, const float4x4 &ViewMatrix, int32 RTWidth, int32 RTHeight);

    // wrappers of above functions for current renderer state
    static bool CalculateAABoxScissorRect(RENDERER_SCISSOR_RECT *pScissorRect, const AABox &Bounds);
    static bool CalculatePointLightScissorRect(RENDERER_SCISSOR_RECT *pScissorRect, const float3 &LightPosition, const float &LightRange);

    // default state
    static void FillDefaultRasterizerState(RENDERER_RASTERIZER_STATE_DESC *pRasterizerState);
    static void FillDefaultDepthStencilState(RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilState);
    static void FillDefaultBlendState(RENDERER_BLEND_STATE_DESC *pBlendState);
    static void FillDefaultSamplerState(GPU_SAMPLER_STATE_DESC *pSamplerState);
    static void CorrectTextureFilter(GPU_SAMPLER_STATE_DESC *pSamplerState);
    static bool TextureFilterRequiresMips(TEXTURE_FILTER filter);
    static uint32 CalculateMipCount(uint32 width, uint32 height = 1, uint32 depth = 1);

    // determine global shader flags for current environment
    static uint32 GetGlobalShaderFlagsForCurrentEnvironment();

    // Convert a projection matrix into the depth range expected by the renderer.
    void CorrectProjectionMatrix(float4x4 &projectionMatrix);

    // Device queries.
    bool CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat = NULL) const;

    // Creates a swap chain on a new window.
    RendererOutputWindow *CreateOutputWindow(const char *windowTitle, uint32 windowWidth, uint32 windowHeight, RENDERER_VSYNC_TYPE vsyncType);

    // Creates a swap chain on an existing window.
    static GPUOutputBuffer *CreateOutputBuffer(RenderSystemWindowHandle hWnd, RENDERER_VSYNC_TYPE vsyncType);
    static GPUOutputBuffer *CreateOutputBuffer(SDL_Window *pSDLWindow, RENDERER_VSYNC_TYPE vsyncType);

    // Resource creation
    static GPUDepthStencilState *CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc);
    static GPURasterizerState *CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc);
    static GPUBlendState *CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc);
    static GPUQuery *CreateQuery(GPU_QUERY_TYPE type);
    static GPUBuffer *CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData = NULL);
    static GPUTexture1D *CreateTexture1D(const GPU_TEXTURE1D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL);
    static GPUTexture1DArray *CreateTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL);
    static GPUTexture2D *CreateTexture2D(const GPU_TEXTURE2D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL);
    static GPUTexture2DArray *CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL);
    static GPUTexture3D *CreateTexture3D(const GPU_TEXTURE3D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL, const uint32 *pInitialDataSlicePitch = NULL);
    static GPUTextureCube *CreateTextureCube(const GPU_TEXTURECUBE_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL);
    static GPUTextureCubeArray *CreateTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL);
    static GPUDepthTexture *CreateDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pTextureDesc);
    static GPUSamplerState *CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc);
    static GPURenderTargetView *CreateRenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc);
    static GPUDepthStencilBufferView *CreateDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc);
    static GPUComputeView *CreateComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc);
    static GPUShaderProgram *CreateGraphicsProgram(const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream);
    static GPUShaderProgram *CreateComputeProgram(ByteStream *pByteCodeStream);
    // CreateGraphicsPipeline
    // CreateComputePipeline

protected:
    // allowed to be called by upper classes
    bool BaseOnStart();
    void BaseOnShutdown();

    // render thread management
    static bool StartRenderThread(bool createWorkerThread);
    static void StopRenderThread();
    static Thread::ThreadIdType s_renderThreadId;
    static CommandQueue s_renderCommandQueue;

    // renderer
    RENDERER_PLATFORM m_eRendererPlatform;
    RENDERER_FEATURE_LEVEL m_eRendererFeatureLevel;
    TEXTURE_PLATFORM m_eTexturePlatform;
    RendererCapabilities m_RendererCapabilities;
    float m_fTexelOffset;

    // backend
    RenderBackend *m_pBackendInterface;

    // output window
    RendererOutputWindow *m_pImplicitOutputWindow;

    // non-material shader map
    ShaderMap m_nullMaterialShaderMap;

    // state manager
    Renderer::FixedResources m_fixedResources;

    // gui context - owned by, and usage only by render thread
    MiniGUIContext m_guiContext;

    // stats
    RendererCounters m_stats;

private:
    bool InternalDrawPlain(GPUContext *pGPUDevice, const PlainVertexFactory::Vertex *pVertices, uint32 nVertices, uint32 flags);
};

extern Renderer *g_pRenderer;

// render thread helper macros
#define QUEUE_RENDERER_COMMAND(obj) g_pRenderer->GetCommandQueue()->QueueCommand(&obj, sizeof(obj))
#define QUEUE_RENDERER_LAMBDA_COMMAND g_pRenderer->GetCommandQueue()->QueueLambdaCommand
#define QUEUE_BLOCKING_RENDERER_COMMAND(obj) g_pRenderer->GetCommandQueue()->QueueBlockingCommand(&obj, sizeof(obj))
#define QUEUE_BLOCKING_RENDERER_LAMBA_COMMAND g_pRenderer->GetCommandQueue()->QueueBlockingLambdaCommand

