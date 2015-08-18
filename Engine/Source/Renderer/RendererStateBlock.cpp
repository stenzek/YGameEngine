#include "Renderer/PrecompiledHeader.h"
#include "Renderer/RendererStateBlock.h"

RendererStateBlock::RendererStateBlock()
{
    m_pGPUDevice = NULL;
    m_iCaptureFlags = 0;

    m_pSavedRasterizerState = NULL;

    m_pSavedDepthStencilState = 0;
    m_iSavedDepthStencilRef = 0;

    m_pSavedBlendState = NULL;
    m_SavedBlendStateBlendFactors.SetZero();

    Y_memzero(m_pSavedRenderTargets, sizeof(m_pSavedRenderTargets));
    m_nSavedRenderTargets = 0;
    m_pSavedDepthStencilBuffer = NULL;

    Y_memzero(m_pSavedVertexBuffers, sizeof(m_pSavedVertexBuffers));
    Y_memzero(m_iSavedVertexBufferOffsets, sizeof(m_iSavedVertexBufferOffsets));
    Y_memzero(m_iSavedVertexBufferStrides, sizeof(m_iSavedVertexBufferStrides));
    m_nSavedVertexBuffers = 0;

    m_pSavedIndexBuffer = NULL;
    m_savedIndexFormat = GPU_INDEX_FORMAT_UINT16;
    m_iSavedIndexBufferOffset = 0;

    m_eSavedDrawTopology = DRAW_TOPOLOGY_TRIANGLE_LIST;

    Y_memzero(&m_SavedViewport, sizeof(m_SavedViewport));

    Y_memzero(&m_SavedScissorRect, sizeof(m_SavedScissorRect));

    m_SavedWorldMatrix.SetZero();

    m_SavedViewMatrix.SetZero();
    m_SavedProjectionMatrix.SetZero();
    m_SavedEyePosition.SetZero();
}

RendererStateBlock::~RendererStateBlock()
{
    uint32 i;

    if (m_pGPUDevice != NULL)
    {
        uint32 captureFlags = m_iCaptureFlags;

        if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_RASTERIZER_STATE)
            m_pSavedRasterizerState->Release();

        if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_DEPTHSTENCIL_STATE)
            m_pSavedDepthStencilState->Release();

        if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_BLEND_STATE)
            m_pSavedBlendState->Release();

        if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_RENDER_TARGETS)
        {
            for (i = 0; i < m_nSavedRenderTargets; i++)
            {
                if (m_pSavedRenderTargets[i] != NULL)
                    m_pSavedRenderTargets[i]->Release();
            }

            if (m_pSavedDepthStencilBuffer != NULL)
            {
                m_pSavedDepthStencilBuffer->Release();
                m_pSavedDepthStencilBuffer = NULL;
            }
        }

        if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_VERTEX_BUFFERS)
        {
            for (i = 0; i < m_nSavedVertexBuffers; i++)
            {
                if (m_pSavedVertexBuffers[i] != NULL)
                    m_pSavedVertexBuffers[i]->Release();
            }
        }

        if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_INDEX_BUFFER)
        {
            if (m_pSavedIndexBuffer != NULL)
                m_pSavedIndexBuffer->Release();
        }
    }
}

void RendererStateBlock::Capture(GPUContext *pGPUDevice, uint32 captureFlags)
{
    uint32 i;

    DebugAssert(pGPUDevice != NULL);
    if (m_pGPUDevice != NULL)
        Clear();

    m_pGPUDevice = pGPUDevice;
    m_iCaptureFlags = captureFlags;

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_RASTERIZER_STATE)
    {
        if ((m_pSavedRasterizerState = pGPUDevice->GetRasterizerState()) != NULL)
            m_pSavedRasterizerState->AddRef();
    }

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_DEPTHSTENCIL_STATE)
    {
        if ((m_pSavedDepthStencilState = pGPUDevice->GetDepthStencilState()) != NULL)
            m_pSavedDepthStencilState->AddRef();

        m_iSavedDepthStencilRef = pGPUDevice->GetDepthStencilStateStencilRef();
    }

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_BLEND_STATE)
    {
        if ((m_pSavedBlendState = pGPUDevice->GetBlendState()) != NULL)
            m_pSavedBlendState->AddRef();

        m_SavedBlendStateBlendFactors = pGPUDevice->GetBlendStateBlendFactor();
    }

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_RENDER_TARGETS)
    {
        m_nSavedRenderTargets = pGPUDevice->GetRenderTargets(countof(m_pSavedRenderTargets), m_pSavedRenderTargets, &m_pSavedDepthStencilBuffer);
        for (i = 0; i < m_nSavedRenderTargets; i++)
        {
            if (m_pSavedRenderTargets[i] != NULL)
                m_pSavedRenderTargets[i]->AddRef();
        }

        if (m_pSavedDepthStencilBuffer != NULL)
            m_pSavedDepthStencilBuffer->AddRef();
    }

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_VERTEX_BUFFERS)
    {
        m_nSavedVertexBuffers = pGPUDevice->GetVertexBuffers(0, countof(m_pSavedVertexBuffers), m_pSavedVertexBuffers, m_iSavedVertexBufferOffsets, m_iSavedVertexBufferStrides);
        for (i = 0; i < m_nSavedVertexBuffers; i++)
        {
            if (m_pSavedVertexBuffers[i] != NULL)
                m_pSavedVertexBuffers[i]->AddRef();
        }
    }

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_INDEX_BUFFER)
    {
        pGPUDevice->GetIndexBuffer(&m_pSavedIndexBuffer, &m_savedIndexFormat, &m_iSavedIndexBufferOffset);
        if (m_pSavedIndexBuffer != NULL)
            m_pSavedIndexBuffer->AddRef();
    }

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_DRAW_TOPOLOGY)
        m_eSavedDrawTopology = pGPUDevice->GetDrawTopology();

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_VIEWPORTS)
        Y_memcpy(&m_SavedViewport, pGPUDevice->GetViewport(), sizeof(m_SavedViewport));

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_SCISSOR_RECTS)
        Y_memcpy(&m_SavedScissorRect, pGPUDevice->GetScissorRect(), sizeof(m_SavedScissorRect));

    GPUContextConstants *pGPUConstants = pGPUDevice->GetConstants();

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_OBJECT_CONSTANTS)
        m_SavedWorldMatrix = pGPUConstants->GetLocalToWorldMatrix();

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_CAMERA_CONSTANTS)
    {
        m_SavedViewMatrix = pGPUConstants->GetCameraViewMatrix();
        m_SavedProjectionMatrix = pGPUConstants->GetCameraProjectionMatrix();
        m_SavedEyePosition = pGPUConstants->GetCameraEyePosition();
    }
}

void RendererStateBlock::Restore()
{
    if (m_pGPUDevice == NULL)
        return;

    GPUContext *pGPUDevice = m_pGPUDevice;
    uint32 captureFlags = m_iCaptureFlags;

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_RASTERIZER_STATE)
        pGPUDevice->SetRasterizerState(m_pSavedRasterizerState);

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_DEPTHSTENCIL_STATE)
        pGPUDevice->SetDepthStencilState(m_pSavedDepthStencilState, m_iSavedDepthStencilRef);

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_BLEND_STATE)
        pGPUDevice->SetBlendState(m_pSavedBlendState, m_SavedBlendStateBlendFactors);

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_RENDER_TARGETS)
        pGPUDevice->SetRenderTargets(m_nSavedRenderTargets, m_pSavedRenderTargets, m_pSavedDepthStencilBuffer);

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_VERTEX_BUFFERS)
        pGPUDevice->SetVertexBuffers(0, m_nSavedVertexBuffers, m_pSavedVertexBuffers, m_iSavedVertexBufferOffsets, m_iSavedVertexBufferStrides);

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_INDEX_BUFFER)
        pGPUDevice->SetIndexBuffer(m_pSavedIndexBuffer, m_savedIndexFormat, m_iSavedIndexBufferOffset);

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_DRAW_TOPOLOGY)
        pGPUDevice->SetDrawTopology(m_eSavedDrawTopology);
        
    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_VIEWPORTS)
        pGPUDevice->SetViewport(&m_SavedViewport);
    
    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_SCISSOR_RECTS)
        pGPUDevice->SetScissorRect(&m_SavedScissorRect);

    GPUContextConstants *pGPUConstants = pGPUDevice->GetConstants();

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_OBJECT_CONSTANTS)
        pGPUConstants->SetLocalToWorldMatrix(m_SavedWorldMatrix, false);

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_CAMERA_CONSTANTS)
    {
        pGPUConstants->SetCameraViewMatrix(m_SavedViewMatrix, false);
        pGPUConstants->SetCameraProjectionMatrix(m_SavedProjectionMatrix, false);
        pGPUConstants->SetCameraEyePosition(m_SavedEyePosition, false);
    }

    if (captureFlags & (GPU_STATE_BLOCK_CAPTURE_FLAG_OBJECT_CONSTANTS | GPU_STATE_BLOCK_CAPTURE_FLAG_CAMERA_CONSTANTS))
        pGPUConstants->CommitChanges();
}

void RendererStateBlock::Clear()
{
    uint32 i;
    uint32 captureFlags = m_iCaptureFlags;

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_RASTERIZER_STATE)
    {
        if (m_pSavedRasterizerState != NULL)
        {
            m_pSavedRasterizerState->Release();
            m_pSavedRasterizerState = NULL;
        }
    }

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_DEPTHSTENCIL_STATE)
    {
        if (m_pSavedDepthStencilState != NULL)
        {
            m_pSavedDepthStencilState->Release();
            m_pSavedDepthStencilState = NULL;
        }

        m_iSavedDepthStencilRef = 0;
    }

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_BLEND_STATE)
    {
        if (m_pSavedBlendState != NULL)
        {
            m_pSavedBlendState->Release();
            m_pSavedBlendState = NULL;
        }

        m_SavedBlendStateBlendFactors.SetZero();
    }

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_RENDER_TARGETS)
    {
        for (i = 0; i < m_nSavedRenderTargets; i++)
        {
            if (m_pSavedRenderTargets[i] != NULL)
            {
                m_pSavedRenderTargets[i]->Release();
                m_pSavedRenderTargets[i] = NULL;
            }
        }

        if (m_pSavedDepthStencilBuffer != NULL)
        {
            m_pSavedDepthStencilBuffer->Release();
            m_pSavedDepthStencilBuffer = NULL;
        }
    }

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_VERTEX_BUFFERS)
    {
        for (i = 0; i < m_nSavedVertexBuffers; i++)
        {
            if (m_pSavedVertexBuffers[i] != NULL)
            {
                m_pSavedVertexBuffers[i]->Release();
                m_pSavedVertexBuffers[i] = NULL;
                m_iSavedVertexBufferOffsets[i] = 0;
                m_iSavedVertexBufferStrides[i] = 0;
            }
        }
    }

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_INDEX_BUFFER)
    {
        if (m_pSavedIndexBuffer != NULL)
        {
            m_pSavedIndexBuffer->Release();
            m_pSavedIndexBuffer = NULL;
            m_savedIndexFormat = GPU_INDEX_FORMAT_UINT16;
            m_iSavedIndexBufferOffset = 0;
        }
    }

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_DRAW_TOPOLOGY)
        m_eSavedDrawTopology = DRAW_TOPOLOGY_TRIANGLE_LIST;

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_VIEWPORTS)
        Y_memzero(&m_SavedViewport, sizeof(m_SavedViewport));

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_SCISSOR_RECTS)
        Y_memzero(&m_SavedScissorRect, sizeof(m_SavedScissorRect));

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_OBJECT_CONSTANTS)
        m_SavedWorldMatrix.SetZero();

    if (captureFlags & GPU_STATE_BLOCK_CAPTURE_FLAG_CAMERA_CONSTANTS)
    {
        m_SavedViewMatrix.SetZero();
        m_SavedProjectionMatrix.SetZero();
        m_SavedEyePosition.SetZero();
    }

    m_pGPUDevice = NULL;
    m_iCaptureFlags = 0;
}

// void RendererStateBlock::SetSavedRasterizerState(GPURasterizerState *pRasterizerState)
// {
//     if (m_pSavedRasterizerState == pRasterizerState)
//         return;
// 
//     if (m_pSavedRasterizerState == NULL)
//         m_pSavedRasterizerState->Release();
// 
//     if ((m_pSavedRasterizerState = pRasterizerState) != NULL)
//         m_pSavedRasterizerState->AddRef();
// 
//     m_iCaptureFlags |= GPU_STATE_BLOCK_CAPTURE_FLAG_RASTERIZER_STATE;
// }
// 
// void RendererStateBlock::SetSavedDepthStencilState(GPUDepthStencilState *pDepthStencilState, uint8 stencilRef)
// {
//     if (m_pSavedDepthStencilState != pDepthStencilState)
//     {
//         if (m_pSavedDepthStencilState == NULL)
//             m_pSavedDepthStencilState->Release();
// 
//         if ((m_pSavedDepthStencilState = pDepthStencilState) != NULL)
//             m_pSavedDepthStencilState->AddRef();
// 
//         m_iCaptureFlags |= GPU_STATE_BLOCK_CAPTURE_FLAG_RASTERIZER_STATE;
//     }
// 
//     if (m_iSavedDepthStencilRef != stencilRef)
//     {
//         m_iSavedDepthStencilRef = stencilRef;
//         m_iCaptureFlags |= GPU_STATE_BLOCK_CAPTURE_FLAG_DEPTHSTENCIL_STATE;
//     }
// }
