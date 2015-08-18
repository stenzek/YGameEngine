#pragma once
#include "Renderer/Renderer.h"

enum GPU_STATE_BLOCK_CAPTURE_FLAG
{
    GPU_STATE_BLOCK_CAPTURE_FLAG_RASTERIZER_STATE           = (1 << 0),
    GPU_STATE_BLOCK_CAPTURE_FLAG_DEPTHSTENCIL_STATE         = (1 << 1),
    GPU_STATE_BLOCK_CAPTURE_FLAG_BLEND_STATE                = (1 << 2),
    GPU_STATE_BLOCK_CAPTURE_FLAG_RENDER_TARGETS             = (1 << 3),
    GPU_STATE_BLOCK_CAPTURE_FLAG_VERTEX_BUFFERS             = (1 << 4),
    GPU_STATE_BLOCK_CAPTURE_FLAG_INDEX_BUFFER               = (1 << 5),
    GPU_STATE_BLOCK_CAPTURE_FLAG_DRAW_TOPOLOGY              = (1 << 6),
    GPU_STATE_BLOCK_CAPTURE_FLAG_VIEWPORTS                  = (1 << 7),
    GPU_STATE_BLOCK_CAPTURE_FLAG_SCISSOR_RECTS              = (1 << 8),
    GPU_STATE_BLOCK_CAPTURE_FLAG_OBJECT_CONSTANTS           = (1 << 9),
    GPU_STATE_BLOCK_CAPTURE_FLAG_CAMERA_CONSTANTS           = (1 << 10),
};

class RendererStateBlock
{
public:
    RendererStateBlock();
    ~RendererStateBlock();

    void Capture(GPUContext *pGPUDevice, uint32 captureFlags);
    void Restore();
    void Clear();

//     void SetSavedRasterizerState(GPURasterizerState *pRasterizerState);
//     void SetSavedDepthStencilState(GPUDepthStencilState *pDepthStencilState, uint8 stencilRef);
//     void SetSavedBlendState(GPUBlendState *pBlendState, const float4 &blendFactors);
//     void SetSavedRenderTarget(uint32 index, GPUTexture *pRenderTarget);
//     void SetSavedRenderTarget(uint32 count, GPUTexture **ppRenderTargets);
//     void SetSavedDepthStencilBuffer(GPUDepthStencilBuffer *pDepthStencilBuffer);
//     void SetSavedVertexBuffer(uint32 index, GPUVertexBuffer *pVertexBuffer, uint32 offset, uint32 stride);
//     void SetSavedVertexBuffers(uint32 count, GPUVertexBuffer **ppVertexBuffers, uint32 *pOffsets, uint32 *pStrides);
//     void SetSavedIndexBuffer(const GPUIndexBuffer *pIndexBuffer, uint32 offset);
//     void SetSavedDrawTopology(DRAW_TOPOLOGY topology);
//     void SetSavedViewport(const VIEWPORT *pViewport);
//     void SetSavedScissorRect(const RENDERER_SCISSOR_RECT *pScissorRect);

private:
    GPUContext *m_pGPUDevice;
    uint32 m_iCaptureFlags;

    // GPU_STATE_BLOCK_CAPTURE_FLAG_RASTERIZER_STATE
    GPURasterizerState *m_pSavedRasterizerState;

    // GPU_STATE_BLOCK_CAPTURE_FLAG_DEPTHSTENCIL_STATE
    GPUDepthStencilState *m_pSavedDepthStencilState;
    uint8 m_iSavedDepthStencilRef;

    // GPU_STATE_BLOCK_CAPTURE_FLAG_BLEND_STATE
    GPUBlendState *m_pSavedBlendState;
    float4 m_SavedBlendStateBlendFactors;

    // GPU_STATE_BLOCK_CAPTURE_FLAG_RENDER_TARGETS
    GPURenderTargetView *m_pSavedRenderTargets[GPU_MAX_SIMULTANEOUS_RENDER_TARGETS];
    GPUDepthStencilBufferView *m_pSavedDepthStencilBuffer;
    uint32 m_nSavedRenderTargets;

    // GPU_STATE_BLOCK_CAPTURE_FLAG_VERTEX_BUFFERS
    GPUBuffer *m_pSavedVertexBuffers[GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS];
    uint32 m_iSavedVertexBufferOffsets[GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS];
    uint32 m_iSavedVertexBufferStrides[GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS];
    uint32 m_nSavedVertexBuffers;

    // GPU_STATE_BLOCK_CAPTURE_FLAG_INDEX_BUFFER
    GPUBuffer *m_pSavedIndexBuffer;
    GPU_INDEX_FORMAT m_savedIndexFormat;
    uint32 m_iSavedIndexBufferOffset;

    // GPU_STATE_BLOCK_CAPTURE_FLAG_DRAW_TOPOLOGY
    DRAW_TOPOLOGY m_eSavedDrawTopology;

    // GPU_STATE_BLOCK_CAPTURE_FLAG_VIEWPORTS
    RENDERER_VIEWPORT m_SavedViewport;

    // GPU_STATE_BLOCK_CAPTURE_FLAG_SCISSOR_RECTS
    RENDERER_SCISSOR_RECT m_SavedScissorRect;

    // GPU_STATE_BLOCK_CAPTURE_FLAG_OBJECT_CONSTANTS
    float4x4 m_SavedWorldMatrix;

    // GPU_STATE_BLOCK_CAPTURE_FLAG_CAMERA_CONSTANTS
    float4x4 m_SavedViewMatrix;
    float4x4 m_SavedProjectionMatrix;
    float3 m_SavedEyePosition;
};