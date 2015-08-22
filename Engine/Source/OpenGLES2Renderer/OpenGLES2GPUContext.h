#pragma once
#include "OpenGLES2Renderer/OpenGLES2Common.h"
#include "OpenGLES2Renderer/OpenGLES2GPUOutputBuffer.h"

class OpenGLES2GPUDevice;
class OpenGLES2ConstantLibrary;
class OpenGLES2GPURasterizerState;
class OpenGLES2GPUDepthStencilState;
class OpenGLES2GPUBlendState;
class OpenGLES2GPUBuffer;
class OpenGLESGPUInputLayout;
class OpenGLES2GPUShaderProgram;

class OpenGLES2GPUContext : public GPUContext
{
public:
    struct VertexBufferBinding
    {
        OpenGLES2GPUBuffer *pVertexBuffer;
        uint32 Offset;
        uint32 Stride;
        bool Dirty;
    };

    struct VertexAttributeBinding
    {
        uint32 VertexBufferIndex;
        uint32 Offset;

        GLenum Type;
        GLint Size;
        GLboolean Normalized;
        GLuint Divisor;

        bool IntegerFormat;
        bool Initialized;
        bool Enabled;
        bool Dirty;
    };

    typedef GPUTexture *TextureUnitBinding;

public:
    OpenGLES2GPUContext(OpenGLES2GPUDevice *pDevice, OpenGLES2ConstantLibrary *pConstantLibrary, SDL_GLContext pSDLGLContext, OpenGLES2GPUOutputBuffer *pOutputBuffer);
    ~OpenGLES2GPUContext();

    // State clearing
    virtual void ClearState(bool clearShaders = true, bool clearBuffers = true, bool clearStates = true, bool clearRenderTargets = true) override final;

    // Retrieve RendererVariables interface.
    virtual GPUContextConstants *GetConstants() override final { return m_pConstants; }

    // State Management (D3D11RenderSystem.cpp)
    virtual GPURasterizerState *GetRasterizerState() override final;
    virtual void SetRasterizerState(GPURasterizerState *pRasterizerState) override final;
    virtual GPUDepthStencilState *GetDepthStencilState() override final;
    virtual uint8 GetDepthStencilStateStencilRef() override final;
    virtual void SetDepthStencilState(GPUDepthStencilState *pDepthStencilState, uint8 stencilRef) override final;
    virtual GPUBlendState *GetBlendState() override final;
    virtual const float4 &GetBlendStateBlendFactor() override final;
    virtual void SetBlendState(GPUBlendState *pBlendState, const float4 &blendFactor = float4::One) override final;

    // Viewport Management (D3D11RenderSystem.cpp)
    virtual const RENDERER_VIEWPORT *GetViewport() override final;
    virtual void SetViewport(const RENDERER_VIEWPORT *pNewViewport) override final;
    virtual void SetFullViewport(GPUTexture *pForRenderTarget = NULL) override final;

    // Scissor Rect Management (D3D11RenderSystem.cpp)
    virtual const RENDERER_SCISSOR_RECT *GetScissorRect() override final;
    virtual void SetScissorRect(const RENDERER_SCISSOR_RECT *pScissorRect) override final;

    // Buffer mapping/reading/writing
    virtual bool ReadBuffer(GPUBuffer *pBuffer, void *pDestination, uint32 start, uint32 count) override final;
    virtual bool WriteBuffer(GPUBuffer *pBuffer, const void *pSource, uint32 start, uint32 count) override final;
    virtual bool MapBuffer(GPUBuffer *pBuffer, GPU_MAP_TYPE mapType, void **ppPointer) override final;
    virtual void Unmapbuffer(GPUBuffer *pBuffer, void *pPointer) override final;

    // Texture reading/writing
    virtual bool ReadTexture(GPUTexture1D *pTexture, void *pDestination, uint32 cbDestination, uint32 mipIndex, uint32 start, uint32 count) override final;
    virtual bool ReadTexture(GPUTexture1DArray *pTexture, void *pDestination, uint32 cbDestination, uint32 arrayIndex, uint32 mipIndex, uint32 start, uint32 count) override final;
    virtual bool ReadTexture(GPUTexture2D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool ReadTexture(GPUTexture2DArray *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 arrayIndex, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool ReadTexture(GPUTexture3D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 destinationSlicePitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 startZ, uint32 countX, uint32 countY, uint32 countZ) override final;
    virtual bool ReadTexture(GPUTextureCube *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool ReadTexture(GPUTextureCubeArray *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 arrayIndex, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool ReadTexture(GPUDepthTexture *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool WriteTexture(GPUTexture1D *pTexture, const void *pSource, uint32 cbSource, uint32 mipIndex, uint32 start, uint32 count) override final;
    virtual bool WriteTexture(GPUTexture1DArray *pTexture, const void *pSource, uint32 cbSource, uint32 arrayIndex, uint32 mipIndex, uint32 start, uint32 count) override final;
    virtual bool WriteTexture(GPUTexture2D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool WriteTexture(GPUTexture2DArray *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 arrayIndex, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool WriteTexture(GPUTexture3D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 sourceSlicePitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 startZ, uint32 countX, uint32 countY, uint32 countZ) override final;
    virtual bool WriteTexture(GPUTextureCube *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool WriteTexture(GPUTextureCubeArray *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 arrayIndex, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;
    virtual bool WriteTexture(GPUDepthTexture *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 startX, uint32 startY, uint32 countX, uint32 countY) override final;

    // Texture copying
    virtual bool CopyTexture(GPUTexture2D *pSourceTexture, GPUTexture2D *pDestinationTexture) override final;
    virtual bool CopyTextureRegion(GPUTexture2D *pSourceTexture, uint32 sourceX, uint32 sourceY, uint32 width, uint32 height, uint32 sourceMipLevel, GPUTexture2D *pDestinationTexture, uint32 destX, uint32 destY, uint32 destMipLevel) override final;

    // Blit (copy) a texture to the currently bound framebuffer. If this texture is a different size, it'll be resized
    virtual void BlitFrameBuffer(GPUTexture2D *pTexture, uint32 sourceX, uint32 sourceY, uint32 sourceWidth, uint32 sourceHeight, uint32 destX, uint32 destY, uint32 destWidth, uint32 destHeight, RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER resizeFilter = RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_NEAREST) override final;

    // Mip generation
    virtual void GenerateMips(GPUTexture *pTexture) override final;

    // Query accessing
    virtual bool BeginQuery(GPUQuery *pQuery) override final;
    virtual bool EndQuery(GPUQuery *pQuery) override final;
    virtual GPU_QUERY_GETDATA_RESULT GetQueryData(GPUQuery *pQuery, void *pData, uint32 cbData, uint32 flags) override final;

    // Predicated drawing
    virtual void SetPredication(GPUQuery *pQuery) override final;

    // RT Clearing
    virtual void ClearTargets(bool clearColor = true, bool clearDepth = true, bool clearStencil = true, const float4 &clearColorValue = float4::Zero, float clearDepthValue = 1.0f, uint8 clearStencilValue = 0) override final;
    virtual void DiscardTargets(bool discardColor = true, bool discardDepth = true, bool discardStencil = true) override final;

    // Swap chain
    virtual GPUOutputBuffer *GetOutputBuffer() override final { return m_pCurrentOutputBuffer; }
    virtual void SetOutputBuffer(GPUOutputBuffer *pSwapChain) override final;
    virtual bool GetExclusiveFullScreen() override final;
    virtual bool SetExclusiveFullScreen(bool enabled, uint32 width, uint32 height, uint32 refreshRate) override final;
    virtual bool ResizeOutputBuffer(uint32 width = 0, uint32 height = 0) override final;
    virtual void PresentOutputBuffer(GPU_PRESENT_BEHAVIOUR presentBehaviour) override final;

    // RT Changing
    virtual uint32 GetRenderTargets(uint32 nRenderTargets, GPURenderTargetView **ppRenderTargetViews, GPUDepthStencilBufferView **ppDepthBufferView) override final;
    virtual void SetRenderTargets(uint32 nRenderTargets, GPURenderTargetView **ppRenderTargets, GPUDepthStencilBufferView *pDepthBufferView) override final;

    // Drawing Setup
    virtual DRAW_TOPOLOGY GetDrawTopology() override final;
    virtual void SetDrawTopology(DRAW_TOPOLOGY topology) override final;

    // Vertex Buffer Setup
    virtual uint32 GetVertexBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer **ppVertexBuffers, uint32 *pVertexBufferOffsets, uint32 *pVertexBufferStrides) override final;
    virtual void SetVertexBuffer(uint32 bufferIndex, GPUBuffer *pVertexBuffer, uint32 offset, uint32 stride) override final;
    virtual void SetVertexBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer *const *ppVertexBuffers, const uint32 *pVertexBufferOffsets, const uint32 *pVertexBufferStrides) override final;
    virtual void GetIndexBuffer(GPUBuffer **ppBuffer, GPU_INDEX_FORMAT *pFormat, uint32 *pOffset) override final;
    virtual void SetIndexBuffer(GPUBuffer *pBuffer, GPU_INDEX_FORMAT format, uint32 offset) override final;

    // Shader Setup
    virtual void SetShaderProgram(GPUShaderProgram *pShaderProgram) override final;

    // constant buffer management
    virtual void WriteConstantBuffer(uint32 bufferIndex, uint32 fieldIndex, uint32 offset, uint32 count, const void *pData, bool commit = false) override final;
    virtual void WriteConstantBufferStrided(uint32 bufferIndex, uint32 fieldIndex, uint32 offset, uint32 bufferStride, uint32 copySize, uint32 count, const void *pData, bool commit = false) override final;
    virtual void CommitConstantBuffer(uint32 bufferIndex) override final;

    // Draw calls
    virtual void Draw(uint32 firstVertex, uint32 nVertices) override final;
    virtual void DrawInstanced(uint32 firstVertex, uint32 nVertices, uint32 nInstances) override final;
    virtual void DrawIndexed(uint32 startIndex, uint32 nIndices, uint32 baseVertex) override final;
    virtual void DrawIndexedInstanced(uint32 startIndex, uint32 nIndices, uint32 baseVertex, uint32 nInstances) override final;

    // Draw calls with user-space buffer
    virtual void DrawUserPointer(const void *pVertices, uint32 vertexSize, uint32 nVertices) override final;
    virtual void DrawIndexedUserPointer(const void *pVertices, uint32 vertexSize, uint32 nVertices, const void *pIndices, GPU_INDEX_FORMAT indexFormat, uint32 nIndices) override final;

    // Compute shaders
    virtual void Dispatch(uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ) override final;

    // --- gl methods ---
    bool Create();
    SDL_GLContext GetSDLGLContext() const { return m_pSDLGLContext; }
    OpenGLES2GPUShaderProgram *GetOpenGLCurrentShaderProgram() { return m_pCurrentShaderProgram; }
    const OpenGLES2ConstantLibrary *GetConstantLibrary() const { return m_pConstantLibrary; }
    byte *GetConstantLibraryBuffer() const { return m_pConstantLibraryBuffer; }

    // commit vertex buffer resources
    void CommitVertexAttributes();

    // access to shader states for the shader mutators to modify
    void SetShaderVertexAttributes(const GPU_VERTEX_ELEMENT_DESC *pAttributeDescriptors, uint32 nAttributes);
    void SetShaderTextureUnit(uint32 index, GPUTexture *pTexture);

    // commit shader resources (uniform blocks, texture units, image units, storage buffers)
    void CommitShaderResources();

    // sets the active texture unit to the unit that can be most easily used for mutation (ie less likely to be bound)
    void BindMutatorTextureUnit();

    // when using texture creation/modification functions, restores the texture if there was one bound to this texture unit
    void RestoreMutatorTextureUnit();

    // update vsync settings in opengl
    void UpdateVSyncState(RENDERER_VSYNC_TYPE vsyncType);

private:
    OpenGLES2GPUDevice *m_pDevice;
    const OpenGLES2ConstantLibrary *m_pConstantLibrary;

    SDL_GLContext m_pSDLGLContext;
    OpenGLES2GPUOutputBuffer *m_pCurrentOutputBuffer;

    GPUContextConstants *m_pConstants;
    byte *m_pConstantLibraryBuffer;

    RENDERER_VIEWPORT m_currentViewport;
    RENDERER_SCISSOR_RECT m_scissorRect;
    DRAW_TOPOLOGY m_drawTopology;
    GLenum m_glDrawTopology;

    // vertex buffer state
    MemArray<VertexBufferBinding> m_currentVertexBuffers;
    uint32 m_activeVertexBuffers;
    bool m_dirtyVertexBuffers;
    MemArray<VertexAttributeBinding> m_currentVertexAttributes;
    uint32 m_activeVertexAttributes;
    bool m_dirtyVertexAttributes;

    // index buffer state
    OpenGLES2GPUBuffer *m_pCurrentIndexBuffer;
    GPU_INDEX_FORMAT m_currentIndexFormat;
    uint32 m_currentIndexBufferOffset;

    // shader state
    OpenGLES2GPUShaderProgram *m_pCurrentShaderProgram;

    // shader states
    MemArray<GPUTexture *> m_currentTextureUnitBindings;
    uint32 m_activeTextureUnitBindings;
    uint32 m_mutatorTextureUnit;
    int32 m_dirtyTextureUnitsLowerBounds;
    int32 m_dirtyTextureUnitsUpperBounds;

    // states
    OpenGLES2GPURasterizerState *m_pCurrentRasterizerState;
    OpenGLES2GPUDepthStencilState *m_pCurrentDepthStencilState;
    uint8 m_currentDepthStencilStateStencilRef;
    OpenGLES2GPUBlendState *m_pCurrentBlendState;
    float4 m_currentBlendStateBlendFactors;

    GLuint m_drawFrameBufferObjectId;
    GLuint m_readFrameBufferObjectId;
    bool m_usingFrameBufferObject;

    OpenGLES2GPURenderTargetView *m_pCurrentRenderTarget;
    OpenGLES2GPUDepthStencilBufferView *m_pCurrentDepthStencilBuffer;

    OpenGLES2GPUBuffer *m_pUserVertexBuffer;
    uint32 m_userVertexBufferSize;
    uint32 m_userVertexBufferPosition;
};

