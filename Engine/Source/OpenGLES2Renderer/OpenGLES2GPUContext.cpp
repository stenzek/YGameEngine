#include "OpenGLES2Renderer/PrecompiledHeader.h"
#include "OpenGLES2Renderer/OpenGLES2GPUContext.h"
#include "OpenGLES2Renderer/OpenGLES2Renderer.h"
#include "OpenGLES2Renderer/OpenGLES2RendererOutputBuffer.h"
#include "OpenGLES2Renderer/OpenGLES2GPUTexture.h"
#include "OpenGLES2Renderer/OpenGLES2GPUBuffer.h"
#include "OpenGLES2Renderer/OpenGLES2GPUShaderProgram.h"
#include "OpenGLES2Renderer/OpenGLES2ConstantLibrary.h"
#include "Renderer/ShaderConstantBuffer.h"
Log_SetChannel(OpenGLES2GPUContext);

#define DEFER_SHADER_STATE_CHANGES 1

OpenGLES2GPUContext::OpenGLES2GPUContext()
{
    m_pRenderer = nullptr;

    m_pSDLGLContext = nullptr;
    m_pCurrentOutputBuffer = nullptr;
    m_isUploadContext = false;

    m_pConstants = nullptr;
    m_pConstantLibrary = nullptr;
    m_pConstantLibraryBuffer = nullptr;

    Y_memzero(&m_currentViewport, sizeof(m_currentViewport));
    Y_memzero(&m_scissorRect, sizeof(m_scissorRect));
    m_drawTopology = DRAW_TOPOLOGY_TRIANGLE_LIST;
    m_glDrawTopology = GL_TRIANGLES;

    m_activeVertexBuffers = 0;
    m_dirtyVertexBuffers = false;
    m_activeVertexAttributes = 0;
    m_dirtyVertexAttributes = false;

    m_pCurrentIndexBuffer = nullptr;
    m_currentIndexFormat = GPU_INDEX_FORMAT_UINT16;
    m_currentIndexBufferOffset = 0;

    m_pCurrentShaderProgram = nullptr;
    m_activeTextureUnitBindings = 0;
    m_dirtyTextureUnitsLowerBounds = -1;
    m_dirtyTextureUnitsUpperBounds = -1;

    m_pCurrentRasterizerState = nullptr;
    m_pCurrentDepthStencilState = nullptr;
    m_currentDepthStencilStateStencilRef = 0;
    m_pCurrentBlendState = nullptr;
    m_currentBlendStateBlendFactors.SetZero();

    m_drawFrameBufferObjectId = 0;
    m_readFrameBufferObjectId = 0;
    m_usingFrameBufferObject = false;

    m_pCurrentRenderTarget = nullptr;
    m_pCurrentDepthStencilBuffer = nullptr;

    m_pUserVertexBuffer = nullptr;
    m_userVertexBufferSize = 1024 * 1024;
    m_userVertexBufferPosition = 0;

    m_drawCallCounter = 0;
}

OpenGLES2GPUContext::~OpenGLES2GPUContext()
{
    // the thread that deletes the context should be the thread that owns it
    DebugAssert(GetContextForCurrentThread() == this);

    // unbind shader stuff
    SetShaderVertexAttributes(nullptr, 0);
    CommitVertexAttributes();
    for (uint32 i = 0; i < m_activeTextureUnitBindings; i++)
    {
        if (m_currentTextureUnitBindings[i] != nullptr)
            SetShaderTextureUnit(i, nullptr);
    }

    if (m_pCurrentShaderProgram != nullptr)
        SetShaderProgram(nullptr);

    CommitShaderResources();

    if (m_drawFrameBufferObjectId > 0)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &m_drawFrameBufferObjectId);
    }

    if (m_readFrameBufferObjectId > 0)
        glDeleteFramebuffers(1, &m_readFrameBufferObjectId);

    for (uint32 i = 0; i < m_activeVertexBuffers; i++)
        SAFE_RELEASE(m_currentVertexBuffers[i].pVertexBuffer);

    SAFE_RELEASE(m_pCurrentIndexBuffer);

    SAFE_RELEASE(m_pCurrentRasterizerState);
    SAFE_RELEASE(m_pCurrentDepthStencilState);
    SAFE_RELEASE(m_pCurrentBlendState);

    SAFE_RELEASE(m_pCurrentRenderTarget);
    SAFE_RELEASE(m_pCurrentDepthStencilBuffer);

    //SAFE_RELEASE(m_pUserIndexBuffer);
    SAFE_RELEASE(m_pUserVertexBuffer);

    Y_free(m_pConstantLibraryBuffer);
    delete m_pConstants;

    // unbind the context, and delete it. no more gl calls are allowed after this!
    DebugAssert(SDL_GL_GetCurrentContext() == m_pSDLGLContext);
    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DeleteContext(m_pSDLGLContext);

    // last thing to release is the output window, this is safe because it won't make any gl calls
    SAFE_RELEASE(m_pCurrentOutputBuffer);

    // no longer the context
    SetContextForCurrentThread(nullptr);
}

void OpenGLES2GPUContext::UpdateVSyncState(RENDERER_VSYNC_TYPE vsyncType)
{
    int swapInterval = 0;

    switch (vsyncType)
    {
    case RENDERER_VSYNC_TYPE_NONE:              swapInterval = 0;       break;
    case RENDERER_VSYNC_TYPE_VSYNC:             swapInterval = 1;       break;
    case RENDERER_VSYNC_TYPE_ADAPTIVE_VSYNC:    swapInterval = -1;      break;
    case RENDERER_VSYNC_TYPE_TRIPLE_BUFFERING:  swapInterval = 1;       break;
    }

    SDL_GL_SetSwapInterval(swapInterval);
}

void OpenGLES2GPUContext::BindToCurrentThread()
{
    // we shouldn't have a context
    DebugAssert(SDL_GL_GetCurrentContext() == nullptr);

    // make it current
    if (SDL_GL_MakeCurrent(m_pCurrentOutputBuffer->GetSDLWindow(), m_pSDLGLContext) != 0)
    {
        Log_ErrorPrintf("OpenGLES2GPUContext::BindToCurrentThread: SDL_GL_MakeCurrent failed: %s", SDL_GetError());
        Panic("SDL_GL_MakeCurrent failed");
    }

    // and update tls pointer
    GPUContext::SetContextForCurrentThread(nullptr);
}

void OpenGLES2GPUContext::UnbindFromCurrentThread()
{
    // should be the current context
    DebugAssert(SDL_GL_GetCurrentContext() == m_pSDLGLContext);
    SDL_GL_MakeCurrent(nullptr, nullptr);
    GPUContext::SetContextForCurrentThread(nullptr);
}

bool OpenGLES2GPUContext::Create(OpenGLES2Renderer *pRenderer, SDL_GLContext pGLContext, OpenGLES2RendererOutputBuffer *pOutputBuffer)
{
    // make us the current context for this thread
    SetContextForCurrentThread(this);

    // set vars
    m_pRenderer = pRenderer;
    m_pSDLGLContext = pGLContext;
    m_pCurrentOutputBuffer = pOutputBuffer;
    m_pCurrentOutputBuffer->AddRef();
    m_isUploadContext = false;

    glGenFramebuffers(1, &m_drawFrameBufferObjectId);
    glGenFramebuffers(1, &m_readFrameBufferObjectId);
    if (m_drawFrameBufferObjectId == 0 || m_readFrameBufferObjectId == 0)
    {
        GL_PRINT_ERROR("OpenGLES2GPUContext::Create: glGenFrameBuffers failed: ");
        return false;
    }

    // allocate shader state arrays
    uint32 maxCombinedTextureImageUnits = 0;
    uint32 maxVertexAttributes = 0;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, reinterpret_cast<GLint *>(&maxCombinedTextureImageUnits));
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, reinterpret_cast<GLint *>(&maxVertexAttributes));

    // these should be >0
    if (maxCombinedTextureImageUnits == 0 || maxVertexAttributes == 0)
    {
        Log_ErrorPrintf("OpenGLES2GPUContext::Create: GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS (%u) or GL_MAX_VERTEX_ATTRIBS (%u) is below required amounts.", maxCombinedTextureImageUnits, maxVertexAttributes);
        return false;
    }

    // allocate storage
    m_currentTextureUnitBindings.Resize(maxCombinedTextureImageUnits);
    m_currentTextureUnitBindings.ZeroContents();
    m_currentVertexAttributes.Resize(maxVertexAttributes);
    m_currentVertexAttributes.ZeroContents();
    m_currentVertexBuffers.Resize(maxVertexAttributes);
    m_currentVertexBuffers.ZeroContents();

    // set deps
    m_mutatorTextureUnit = m_currentTextureUnitBindings.GetSize() - 1;

    // create plain vertex buffer
    {
        GPU_BUFFER_DESC vertexBufferDesc(GPU_BUFFER_FLAG_MAPPABLE | GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER, m_userVertexBufferSize);
        if ((m_pUserVertexBuffer = static_cast<OpenGLES2GPUBuffer *>(pRenderer->CreateBuffer(&vertexBufferDesc, NULL))) == NULL)
        {
            Log_ErrorPrintf("OpenGLES2GPUContext::Create: Failed to create plain dynamic vertex buffer.");
            return false;
        }
    }

    // create constants
    m_pConstants = new GPUContextConstants(this);

    // allocate constant library buffer
    m_pConstantLibrary = pRenderer->GetConstantLibrary();
    m_pConstantLibraryBuffer = (byte *)Y_malloc(pRenderer->GetConstantLibrary()->GetConstantStorageBufferSize());

    // update swap interval
    UpdateVSyncState(pOutputBuffer->GetVSyncType());

    Log_InfoPrint("OpenGLES2GPUContext::Create: Context creation complete.");
    return true;
}

bool OpenGLES2GPUContext::CreateUploadContext(OpenGLES2Renderer *pRenderer, SDL_GLContext pGLContext, OpenGLES2RendererOutputBuffer *pOutputBuffer)
{
    m_pRenderer = pRenderer;

    m_pSDLGLContext = pGLContext;
    m_pCurrentOutputBuffer = pOutputBuffer;
    m_pCurrentOutputBuffer->AddRef();
    m_isUploadContext = true;

    Log_InfoPrint("OpenGLES2GPUContext::Create: Upload context creation complete.");
    return true;
}

void OpenGLES2GPUContext::ClearState(bool clearShaders /* = true */, bool clearBuffers /* = true */, bool clearStates /* = true */, bool clearRenderTargets /* = true */)
{
    if (clearShaders)
    {
        SetShaderProgram(nullptr);

        // unbind shader stuff
        SetShaderVertexAttributes(nullptr, 0);
        CommitVertexAttributes();
        for (uint32 i = 0; i < m_activeTextureUnitBindings; i++)
        {
            if (m_currentTextureUnitBindings[i] != nullptr)
                SetShaderTextureUnit(i, nullptr);
        }
        CommitShaderResources();
    }

    if (clearBuffers)
    {
        for (uint32 i = 0; i < m_activeVertexBuffers; i++)
            SetVertexBuffer(i, nullptr, 0, 0);

        SetIndexBuffer(nullptr, GPU_INDEX_FORMAT_UINT16, 0);

        CommitVertexAttributes();
    }

    if (clearStates)
    {
        SetRasterizerState(nullptr);
        SetDepthStencilState(nullptr, 0);
        SetBlendState(nullptr);

        RENDERER_VIEWPORT viewport(0, 0, 0, 0, 0.0f, 1.0f);
        SetViewport(&viewport);

        RENDERER_SCISSOR_RECT scissor(0, 0, 0, 0);
        SetScissorRect(&scissor);
    }

    if (clearRenderTargets)
        SetRenderTargets(0, nullptr, nullptr);
}

GPURasterizerState *OpenGLES2GPUContext::GetRasterizerState()
{
    return m_pCurrentRasterizerState;
}

void OpenGLES2GPUContext::SetRasterizerState(GPURasterizerState *pRasterizerState)
{
    if (m_pCurrentRasterizerState == pRasterizerState)
        return;

    // if new buffer is null, unbind old
    if (m_pCurrentRasterizerState != nullptr)
    {
        if (pRasterizerState == nullptr)
            m_pCurrentRasterizerState->Unapply();

        m_pCurrentRasterizerState->Release();
    }

    if ((m_pCurrentRasterizerState = static_cast<OpenGLES2GPURasterizerState *>(pRasterizerState)) != NULL)
    {
        m_pCurrentRasterizerState->AddRef();
        m_pCurrentRasterizerState->Apply();
    }
}

GPUDepthStencilState *OpenGLES2GPUContext::GetDepthStencilState()
{
    return m_pCurrentDepthStencilState;
}

uint8 OpenGLES2GPUContext::GetDepthStencilStateStencilRef()
{
    return m_currentDepthStencilStateStencilRef;
}

void OpenGLES2GPUContext::SetDepthStencilState(GPUDepthStencilState *pDepthStencilState, uint8 StencilRef)
{
    if (m_pCurrentDepthStencilState == pDepthStencilState && m_currentDepthStencilStateStencilRef == StencilRef)
        return;

    m_currentDepthStencilStateStencilRef = StencilRef;

    // if new buffer is null, unbind old
    if (m_pCurrentDepthStencilState != nullptr)
    {
        if (pDepthStencilState == nullptr)
            m_pCurrentDepthStencilState->Unapply();

        m_pCurrentDepthStencilState->Release();
    }

    if ((m_pCurrentDepthStencilState = static_cast<OpenGLES2GPUDepthStencilState *>(pDepthStencilState)) != NULL)
    {
        m_pCurrentDepthStencilState->AddRef();
        m_pCurrentDepthStencilState->Apply(StencilRef);
    }
}

GPUBlendState *OpenGLES2GPUContext::GetBlendState()
{
    return m_pCurrentBlendState;
}

const float4 &OpenGLES2GPUContext::GetBlendStateBlendFactor()
{
    return m_currentBlendStateBlendFactors;
}

void OpenGLES2GPUContext::SetBlendState(GPUBlendState *pBlendState, const float4 &BlendFactor /* = float4::One */)
{
    if (m_pCurrentBlendState != pBlendState)
    {
        // if new buffer is null, unbind old
        if (m_pCurrentBlendState != nullptr)
        {
            if (pBlendState == nullptr)
                m_pCurrentBlendState->Unapply();

            m_pCurrentBlendState->Release();
        }

        if ((m_pCurrentBlendState = static_cast<OpenGLES2GPUBlendState *>(pBlendState)) != NULL)
        {
            m_pCurrentBlendState->AddRef();
            m_pCurrentBlendState->Apply();
        }
    }

    if (BlendFactor != m_currentBlendStateBlendFactors)
    {
        m_currentBlendStateBlendFactors = BlendFactor;
        glBlendColor(BlendFactor.r, BlendFactor.g, BlendFactor.b, BlendFactor.a);
    }
}

const RENDERER_VIEWPORT *OpenGLES2GPUContext::GetViewport()
{
    return &m_currentViewport;
}

void OpenGLES2GPUContext::SetViewport(const RENDERER_VIEWPORT *pNewViewport)
{
    if (Y_memcmp(&m_currentViewport, pNewViewport, sizeof(RENDERER_VIEWPORT)) == 0)
        return;

    Y_memcpy(&m_currentViewport, pNewViewport, sizeof(m_currentViewport));
    glViewport(m_currentViewport.TopLeftX, m_currentViewport.TopLeftY, m_currentViewport.Width, m_currentViewport.Height);
    glDepthRangef(m_currentViewport.MinDepth, m_currentViewport.MaxDepth);

    // update constants
    m_pConstants->SetViewportOffset((float)m_currentViewport.TopLeftX, (float)m_currentViewport.TopLeftY, false);
    m_pConstants->SetViewportSize((float)m_currentViewport.Width, (float)m_currentViewport.Height, false);
    m_pConstants->CommitChanges();
}

void OpenGLES2GPUContext::SetDefaultViewport(GPUTexture *pForRenderTarget /*= NULL*/)
{
    RENDERER_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;

    if (pForRenderTarget == nullptr && m_pCurrentRenderTarget == nullptr && m_pCurrentDepthStencilBuffer == nullptr)
    {
        viewport.Width = m_pCurrentOutputBuffer->GetWidth();
        viewport.Height = m_pCurrentOutputBuffer->GetHeight();
    }
    else
    {
        DebugAssert(m_pCurrentRenderTarget != nullptr || m_pCurrentDepthStencilBuffer != nullptr || pForRenderTarget != nullptr);

        GPUTexture *pRT = pForRenderTarget;
        if (pRT == nullptr)
            pRT = (m_pCurrentRenderTarget != nullptr) ? m_pCurrentDepthStencilBuffer->GetTargetTexture() : m_pCurrentRenderTarget->GetTargetTexture();

        DebugAssert(pRT != nullptr);
        uint3 renderTargetDimensions = Renderer::GetTextureDimensions(pRT);
        viewport.Width = renderTargetDimensions.x;
        viewport.Height = renderTargetDimensions.y;
    }

    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    SetViewport(&viewport);

    // fix scissor rect
    if (m_scissorRect.Top != m_scissorRect.Bottom)
        glScissor(m_scissorRect.Left, m_currentViewport.Height - m_scissorRect.Bottom, m_scissorRect.Right - m_scissorRect.Left, m_scissorRect.Bottom - m_scissorRect.Top);
}

const RENDERER_SCISSOR_RECT *OpenGLES2GPUContext::GetScissorRect()
{
    return &m_scissorRect;
}

void OpenGLES2GPUContext::SetScissorRect(const RENDERER_SCISSOR_RECT *pScissorRect)
{
    if (Y_memcmp(&m_scissorRect, pScissorRect, sizeof(m_scissorRect)) == 0)
        return;

    Y_memcpy(&m_scissorRect, pScissorRect, sizeof(m_scissorRect));
    glScissor(m_scissorRect.Left, m_currentViewport.Height - m_scissorRect.Bottom, m_scissorRect.Right - m_scissorRect.Left, m_scissorRect.Bottom - m_scissorRect.Top);
}

void OpenGLES2GPUContext::ClearTargets(bool clearColor /* = true */, bool clearDepth /* = true */, bool clearStencil /* = true */, const float4 &clearColorValue /* = float4::Zero */, float clearDepthValue /* = 1.0f */, uint8 clearStencilValue /* = 0 */)
{
    bool oldColorWritesEnabled = (m_pCurrentBlendState == nullptr) ? true : m_pCurrentBlendState->GetGLColorWritesEnabled();
    GLboolean oldDepthMask = (m_pCurrentDepthStencilState == nullptr) ? GL_TRUE : m_pCurrentDepthStencilState->GetGLDepthMask();
    GLuint oldStencilMask = (m_pCurrentDepthStencilState == nullptr) ? 0xFF : m_pCurrentDepthStencilState->GetGLStencilWriteMask();
    bool oldScissorTest = (m_pCurrentRasterizerState == nullptr) ? false : m_pCurrentRasterizerState->GetGLScissorTestEnable();

    // change settings so the whole RT gets cleared
    if (!oldColorWritesEnabled)
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    if (oldDepthMask == GL_FALSE)
        glDepthMask(GL_TRUE);
    if (oldStencilMask != 0xFF)
        glStencilMask(0xFF);
    if (oldScissorTest)
        glDisable(GL_SCISSOR_TEST);
    
    // new functions aren't available on GLES
    GLbitfield clearMask = 0;
    if (clearColor)
    {
        glClearColor(clearColorValue.r, clearColorValue.g, clearColorValue.b, clearColorValue.a);
        clearMask |= GL_COLOR_BUFFER_BIT;
    }
    if (clearDepth)
    {
        glDepthMask(GL_TRUE);
        glClearDepthf(clearDepthValue);
        clearMask |= GL_DEPTH_BUFFER_BIT;
    }
    if (clearStencil)
    {
        glClearStencil(clearStencilValue);
        clearMask |= GL_STENCIL_BUFFER_BIT;
    }

    // invoke
    if (clearMask != 0)
        glClear(clearMask);

    // restore settings
    if (oldScissorTest)
        glEnable(GL_SCISSOR_TEST);
    if (oldStencilMask != 0xFF)
        glStencilMask(0xFF);
    if (oldDepthMask == GL_FALSE)
        glDepthMask(GL_FALSE);
    if (!oldColorWritesEnabled)
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}

void OpenGLES2GPUContext::DiscardTargets(bool discardColor /* = true */, bool discardDepth /* = true */, bool discardStencil /* = true */)
{
#ifdef GL_EXT_discard_framebuffer
    if (GLAD_GL_EXT_discard_framebuffer)
    {
        GLenum attachments[3];
        uint32 nAttachments = 0;

        // ugh, multiple paths again??!
        if (m_pCurrentRenderTarget != nullptr || m_pCurrentDepthStencilBuffer != nullptr)
        {
            if (discardColor)
                attachments[nAttachments++] = GL_COLOR_ATTACHMENT0;
            if (discardDepth)
                attachments[nAttachments++] = GL_DEPTH_ATTACHMENT;
            if (discardStencil)
                attachments[nAttachments++] = GL_STENCIL_ATTACHMENT;
        }
        else
        {
            if (discardColor)
                attachments[nAttachments++] = GL_COLOR_EXT;
            if (discardDepth)
                attachments[nAttachments++] = GL_DEPTH_EXT;
            if (discardStencil)
                attachments[nAttachments++] = GL_STENCIL_EXT;
        }

        glDiscardFramebufferEXT(GL_FRAMEBUFFER, nAttachments, attachments);
    }
#endif
}

void OpenGLES2GPUContext::SetOutputBuffer(RendererOutputBuffer *pSwapChain)
{
    OpenGLES2RendererOutputBuffer *pOpenGLOutputBuffer = static_cast<OpenGLES2RendererOutputBuffer *>(pSwapChain);
    DebugAssert(pOpenGLOutputBuffer != nullptr);

    // same?
    if (m_pCurrentOutputBuffer == pOpenGLOutputBuffer)
        return;

    // make this the new current context
    if (SDL_GL_MakeCurrent(pOpenGLOutputBuffer->GetSDLWindow(), m_pSDLGLContext) != 0)
    {
        Log_ErrorPrintf("OpenGLES2GPUContext::SetOutputBuffer: SDL_GL_MakeCurrent failed: %s", SDL_GetError());
        Panic("SDL_GL_MakeCurrent failed");
    }

    // update swap interval
    if (pOpenGLOutputBuffer->GetVSyncType() != m_pCurrentOutputBuffer->GetVSyncType())
        UpdateVSyncState(pOpenGLOutputBuffer->GetVSyncType());

    // update pointers
    m_pCurrentOutputBuffer->Release();
    m_pCurrentOutputBuffer = pOpenGLOutputBuffer;
    m_pCurrentOutputBuffer->AddRef();
}

uint32 OpenGLES2GPUContext::GetRenderTargets(uint32 nRenderTargets, GPURenderTargetView **ppRenderTargetViews, GPUDepthStencilBufferView **ppDepthBufferView)
{
    if (nRenderTargets >= 1)
        ppRenderTargetViews[0] = m_pCurrentRenderTarget;

    if (ppDepthBufferView != nullptr)
        *ppDepthBufferView = m_pCurrentDepthStencilBuffer;

    return (m_pCurrentRenderTarget != nullptr) ? Min(nRenderTargets, (uint32)1) : 0;
}

void OpenGLES2GPUContext::SetRenderTargets(uint32 nRenderTargets, GPURenderTargetView **ppRenderTargets, GPUDepthStencilBufferView *pDepthBufferView)
{
    // GLES only does 1 render target
    DebugAssert(nRenderTargets <= 1);

    // switch to swap chain?
    if ((nRenderTargets == 0 || ppRenderTargets[0] == nullptr) && pDepthBufferView == nullptr)
    {
        // currently using FBO?
        if (m_usingFrameBufferObject)
        {
            if (m_pCurrentRenderTarget != nullptr)
            {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);
                m_pCurrentRenderTarget->Release();
                m_pCurrentRenderTarget = nullptr;
            }

            if (m_pCurrentDepthStencilBuffer != nullptr)
            {
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, m_pCurrentDepthStencilBuffer->GetAttachmentPoint(), GL_RENDERBUFFER, 0);
                m_pCurrentDepthStencilBuffer->Release();
                m_pCurrentDepthStencilBuffer = nullptr;
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            m_usingFrameBufferObject = false;
        }

        // done
        return;
    }

    // not currently using fbo?
    if (!m_usingFrameBufferObject)
    {
        // bind fbo
        glBindFramebuffer(GL_FRAMEBUFFER, m_drawFrameBufferObjectId);
        m_usingFrameBufferObject = true;
    }


    // rtv changed?
    if ((nRenderTargets == 0 && m_pCurrentRenderTarget != nullptr) || (nRenderTargets > 0 && m_pCurrentRenderTarget != ppRenderTargets[0]))
    {
        OpenGLES2GPURenderTargetView *pOldRTV = m_pCurrentRenderTarget;
        if (nRenderTargets > 0 && ppRenderTargets[0] != nullptr)
        {
            m_pCurrentRenderTarget = static_cast<OpenGLES2GPURenderTargetView *>(ppRenderTargets[0]);
            m_pCurrentRenderTarget->AddRef();
            switch (m_pCurrentRenderTarget->GetTextureType())
            {
            case TEXTURE_TYPE_2D:
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pCurrentRenderTarget->GetTextureName(), m_pCurrentRenderTarget->GetDesc()->MipLevel);
                break;

            default:
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);
                break;
            }

            if (pOldRTV != nullptr)
                pOldRTV->Release();
        }
        else if (pOldRTV != nullptr)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);
            pOldRTV->Release();
        }
    }

    // dsv changed?
    if (m_pCurrentDepthStencilBuffer != pDepthBufferView)
    {
        OpenGLES2GPUDepthStencilBufferView *pOldDSV = m_pCurrentDepthStencilBuffer;
        if (pDepthBufferView != nullptr)
        {
            m_pCurrentDepthStencilBuffer = static_cast<OpenGLES2GPUDepthStencilBufferView *>(pDepthBufferView);
            m_pCurrentDepthStencilBuffer->AddRef();
            switch (m_pCurrentDepthStencilBuffer->GetTextureType())
            {
            case TEXTURE_TYPE_2D:
                glFramebufferTexture2D(GL_FRAMEBUFFER, m_pCurrentDepthStencilBuffer->GetAttachmentPoint(), GL_TEXTURE_2D, m_pCurrentDepthStencilBuffer->GetTextureName(), m_pCurrentDepthStencilBuffer->GetDesc()->MipLevel);
                break;

            case TEXTURE_TYPE_DEPTH:
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, m_pCurrentDepthStencilBuffer->GetAttachmentPoint(), GL_RENDERBUFFER, m_pCurrentDepthStencilBuffer->GetTextureName());
                break;

            default:
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, m_pCurrentDepthStencilBuffer->GetAttachmentPoint(), GL_RENDERBUFFER, 0);
                break;
            }

            if (pOldDSV != nullptr)
                pOldDSV->Release();
        }
        else if (pOldDSV != nullptr)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, pOldDSV->GetAttachmentPoint(), 0, 0, 0);
            pOldDSV->Release();
        }
    }

#ifdef Y_BUILD_CONFIG_DEBUG
    // check fbo completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    DebugAssert(status == GL_FRAMEBUFFER_COMPLETE);
#endif
}

DRAW_TOPOLOGY OpenGLES2GPUContext::GetDrawTopology()
{
    return m_drawTopology;
}

void OpenGLES2GPUContext::SetDrawTopology(DRAW_TOPOLOGY topology)
{
    DebugAssert(topology < DRAW_TOPOLOGY_COUNT);
    if (topology == m_drawTopology)
        return;

    m_drawTopology = topology;
    switch (topology)
    {
    case DRAW_TOPOLOGY_POINTS:
        m_glDrawTopology = GL_POINTS;
        break;

    case DRAW_TOPOLOGY_LINE_LIST:
        m_glDrawTopology = GL_LINES;
        break;

    case DRAW_TOPOLOGY_LINE_STRIP:
        m_glDrawTopology = GL_LINE_STRIP;
        break;

    case DRAW_TOPOLOGY_TRIANGLE_LIST:
        m_glDrawTopology = GL_TRIANGLES;
        break;

    case DRAW_TOPOLOGY_TRIANGLE_STRIP:
        m_glDrawTopology = GL_TRIANGLE_STRIP;
        break;
    }
}

uint32 OpenGLES2GPUContext::GetVertexBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer **ppVertexBuffers, uint32 *pVertexBufferOffsets, uint32 *pVertexBufferStrides)
{
    DebugAssert(firstBuffer + nBuffers < m_currentVertexBuffers.GetSize());

    uint32 count;
    for (count = 0; count < nBuffers; count++)
    {
        uint32 idx = firstBuffer + count;
        if (idx >= m_activeVertexBuffers)
            break;

        VertexBufferBinding *vb = &m_currentVertexBuffers[idx];
        ppVertexBuffers[count] = vb->pVertexBuffer;
        pVertexBufferOffsets[count] = vb->Offset;
        pVertexBufferStrides[count] = vb->Stride;
    }

    return count;
}

void OpenGLES2GPUContext::SetVertexBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer *const *ppVertexBuffers, const uint32 *pVertexBufferOffsets, const uint32 *pVertexBufferStrides)
{
    DebugAssert(firstBuffer + nBuffers < m_currentVertexBuffers.GetSize());

    // update each buffer
    bool updateDirtyFlag = false;
    for (uint32 i = 0; i < nBuffers; i++)
    {
        uint32 bufferIndex = firstBuffer + i;
        VertexBufferBinding *vbBinding = &m_currentVertexBuffers[bufferIndex];
        OpenGLES2GPUBuffer *pBuffer = static_cast<OpenGLES2GPUBuffer *>(ppVertexBuffers[i]);
        uint32 bufferOffset = pVertexBufferOffsets[i];
        uint32 bufferStride = pVertexBufferStrides[i];

        // any changes?
        if (vbBinding->pVertexBuffer != pBuffer ||
            vbBinding->Offset != bufferOffset ||
            vbBinding->Stride != bufferStride)
        {
            // buffer change?
            if (vbBinding->pVertexBuffer != pBuffer)
            {
                if (vbBinding->pVertexBuffer != nullptr)
                    vbBinding->pVertexBuffer->Release();
                if ((vbBinding->pVertexBuffer = pBuffer) != nullptr)
                {
                    vbBinding->pVertexBuffer->AddRef();
                    vbBinding->Offset = pVertexBufferOffsets[i];
                    vbBinding->Stride = pVertexBufferStrides[i];
                }
                else
                {
                    vbBinding->Offset = 0;
                    vbBinding->Stride = 0;
                }
            }

            // dirty flag
            vbBinding->Dirty = true;
            updateDirtyFlag = true;
        }
    }

    // update active range
    uint32 searchCount = Max(m_activeVertexBuffers, firstBuffer + nBuffers);
    uint32 activeCount = 0;
    for (uint32 i = 0; i < searchCount; i++)
    {
        const VertexBufferBinding *binding = &m_currentVertexBuffers[i];
        if (binding->pVertexBuffer != nullptr || binding->Dirty)
            activeCount = i + 1;
    }
    m_activeVertexBuffers = activeCount;

    // dirty flag
    m_dirtyVertexBuffers |= updateDirtyFlag;
}

void OpenGLES2GPUContext::SetVertexBuffer(uint32 bufferIndex, GPUBuffer *pVertexBuffer, uint32 offset, uint32 stride)
{
    DebugAssert(bufferIndex < m_currentVertexBuffers.GetSize());

    VertexBufferBinding *vbBinding = &m_currentVertexBuffers[bufferIndex];
    OpenGLES2GPUBuffer *pBuffer = static_cast<OpenGLES2GPUBuffer *>(pVertexBuffer);

    // any changes?
    if (vbBinding->pVertexBuffer != pBuffer ||
        vbBinding->Offset != offset ||
        vbBinding->Stride != stride)
    {
        // buffer change?
        if (vbBinding->pVertexBuffer != pBuffer)
        {
            if (vbBinding->pVertexBuffer != nullptr)
                vbBinding->pVertexBuffer->Release();
            if ((vbBinding->pVertexBuffer = pBuffer) != nullptr)
            {
                vbBinding->pVertexBuffer->AddRef();
                vbBinding->Offset = offset;
                vbBinding->Stride = stride;
            }
            else
            {
                vbBinding->Offset = 0;
                vbBinding->Stride = 0;
            }
        }

        // dirty flag
        vbBinding->Dirty = true;
    }
    else
    {
        // no changes
        return;
    }

    // update active range
    if (bufferIndex >= m_activeVertexBuffers)
    {
        m_activeVertexBuffers = bufferIndex + 1;
    }
    else if ((bufferIndex + 1) == m_activeVertexBuffers)
    {
        uint32 lastPos = 0;
        for (uint32 i = 0; i < m_activeVertexBuffers; i++)
        {
            const VertexBufferBinding *binding = &m_currentVertexBuffers[i];
            if (binding->pVertexBuffer != nullptr || binding->Dirty)
                lastPos = i + 1;
        }
        m_activeVertexBuffers = lastPos;
    }

    // dirty flag
    m_dirtyVertexBuffers = true;
}

void OpenGLES2GPUContext::SetShaderVertexAttributes(const GPU_VERTEX_ELEMENT_DESC *pAttributeDescriptors, uint32 nAttributes)
{
    uint32 attributeIndex;
    for (attributeIndex = 0; attributeIndex < nAttributes; attributeIndex++)
    {
        const GPU_VERTEX_ELEMENT_DESC *pAttributeDesc = &pAttributeDescriptors[attributeIndex];
        VertexAttributeBinding *pAttributeBinding = &m_currentVertexAttributes[attributeIndex];

        // convert to gl
        bool integerType;
        GLenum glType;
        GLint glSize;
        GLboolean glNormalized;
        OpenGLES2TypeConversion::GetOpenGLVAOTypeAndSizeForVertexElementType(pAttributeDesc->Type, &integerType, &glType, &glSize, &glNormalized);

        // handle format changes
        if (!pAttributeBinding->Initialized || pAttributeBinding->VertexBufferIndex != pAttributeDesc->StreamIndex || pAttributeBinding->Offset != pAttributeDesc->StreamOffset ||
            pAttributeBinding->IntegerFormat != integerType || pAttributeBinding->Type != glType || pAttributeBinding->Size != glSize || pAttributeBinding->Normalized != glNormalized)
        {
            // initialize struct
            // todo: different dirty flags for each field (main, dividor, etc), worth it?
            pAttributeBinding->Initialized = true;
            pAttributeBinding->VertexBufferIndex = pAttributeDesc->StreamIndex;
            pAttributeBinding->Offset = pAttributeDesc->StreamOffset;
            pAttributeBinding->IntegerFormat = integerType;
            pAttributeBinding->Type = glType;
            pAttributeBinding->Size = glSize;
            pAttributeBinding->Normalized = glNormalized;
            pAttributeBinding->Dirty = true;
            m_dirtyVertexAttributes = true;
        }
    }

    // disable remaining attributes
    for (; attributeIndex < m_activeVertexAttributes; attributeIndex++)
    {
        VertexAttributeBinding *pAttributeBinding = &m_currentVertexAttributes[attributeIndex];
        if (pAttributeBinding->Initialized)
        {
            pAttributeBinding->VertexBufferIndex = 0;
            pAttributeBinding->Offset = 0;
            pAttributeBinding->Type = 0;
            pAttributeBinding->Size = 0;
            pAttributeBinding->Normalized = GL_FALSE;
            pAttributeBinding->Divisor = 0;
            pAttributeBinding->IntegerFormat = false;
            pAttributeBinding->Initialized = false;
            pAttributeBinding->Dirty = true;
            m_dirtyVertexAttributes = true;
        }
    }  

    // update active count, keep it as low as possible
    m_activeVertexAttributes = Max(m_activeVertexAttributes, nAttributes);
}

void OpenGLES2GPUContext::CommitVertexAttributes()
{
    if (!(m_dirtyVertexAttributes | m_dirtyVertexBuffers))
        return;

    uint32 lastBufferIndex = 0xFFFFFFFF;
    uint32 nActiveAttributes = 0;
    for (uint32 attributeIndex = 0; attributeIndex < m_activeVertexAttributes; attributeIndex++)
    {
        VertexAttributeBinding *pAttributeBinding = &m_currentVertexAttributes[attributeIndex];
        if (!pAttributeBinding->Initialized)
        {
            // non-initialized attributes, should be disabled, could also check dirty flag here too
            if (pAttributeBinding->Enabled)
            {
                glDisableVertexAttribArray(attributeIndex);
                pAttributeBinding->Enabled = false;
                pAttributeBinding->Dirty = false;
            }

            // skip
            continue;
        }

        // skip non-dirty attributes+buffers
        const VertexBufferBinding *pVertexBufferBinding = &m_currentVertexBuffers[pAttributeBinding->VertexBufferIndex];
        if (!(pAttributeBinding->Dirty | pVertexBufferBinding->Dirty))
        {
            nActiveAttributes = attributeIndex + 1;
            continue;
        }

        // determine whether it should be enabled
        if (pVertexBufferBinding->pVertexBuffer != nullptr)
        {
            // should it be enabled?
            if (lastBufferIndex != pAttributeBinding->VertexBufferIndex)
            {
                glBindBuffer(GL_ARRAY_BUFFER, pVertexBufferBinding->pVertexBuffer->GetGLBufferId());
                lastBufferIndex = pAttributeBinding->VertexBufferIndex;
            }

            // set the pointer
            if (pAttributeBinding->IntegerFormat)
                glVertexAttribIPointer(attributeIndex, pAttributeBinding->Size, pAttributeBinding->Type, pVertexBufferBinding->Stride, reinterpret_cast<const void *>(pVertexBufferBinding->Offset + pAttributeBinding->Offset));
            else
                glVertexAttribPointer(attributeIndex, pAttributeBinding->Size, pAttributeBinding->Type, pAttributeBinding->Normalized, pVertexBufferBinding->Stride, reinterpret_cast<const void *>(pVertexBufferBinding->Offset + pAttributeBinding->Offset));

            // and divisor
            glVertexAttribDivisor(attributeIndex, pAttributeBinding->Divisor);

            // enable attribute
            if (!pAttributeBinding->Enabled)
            {
                glEnableVertexAttribArray(attributeIndex);
                pAttributeBinding->Enabled = true;
            }

            // update max index
            nActiveAttributes = attributeIndex + 1;
        }
        else
        {
            // disable attribute if enabled
            if (pAttributeBinding->Enabled)
            {
                glDisableVertexAttribArray(attributeIndex);
                pAttributeBinding->Enabled = false;
            }
        }

        // clear dirty flag on attribute
        pAttributeBinding->Dirty = false;
    }

    // clear buffer binding
    if (lastBufferIndex != 0xFFFFFFFF)
        glBindBuffer(GL_ARRAY_BUFFER, 0);

    // flag all vertex buffers as clean
    for (uint32 vertexBufferIndex = 0; vertexBufferIndex < m_activeVertexBuffers; vertexBufferIndex++)
        m_currentVertexBuffers[vertexBufferIndex].Dirty = false;

    // and the global flags
    m_dirtyVertexAttributes = false;
    m_dirtyVertexBuffers = false;
}

void OpenGLES2GPUContext::GetIndexBuffer(GPUBuffer **ppBuffer, GPU_INDEX_FORMAT *pFormat, uint32 *pOffset)
{
    *ppBuffer = m_pCurrentIndexBuffer;
    *pFormat = m_currentIndexFormat;
    *pOffset = m_currentIndexBufferOffset;
}

void OpenGLES2GPUContext::SetIndexBuffer(GPUBuffer *pBuffer, GPU_INDEX_FORMAT format, uint32 offset)
{
    if (m_pCurrentIndexBuffer == pBuffer && m_currentIndexFormat == format && m_currentIndexBufferOffset == offset)
        return;

    GPUBuffer *pOldIndexBuffer = m_pCurrentIndexBuffer;
    if ((m_pCurrentIndexBuffer = static_cast<OpenGLES2GPUBuffer *>(pBuffer)) != NULL)
    {
        m_pCurrentIndexBuffer->AddRef();
        m_currentIndexFormat = format;

        DebugAssert(m_pCurrentIndexBuffer->GetGLBufferTarget() == GL_ELEMENT_ARRAY_BUFFER);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pCurrentIndexBuffer->GetGLBufferId());
    }
    else
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        m_currentIndexFormat = GPU_INDEX_FORMAT_UINT16;
    }

    if (pOldIndexBuffer != NULL)
        pOldIndexBuffer->Release();
}

void OpenGLES2GPUContext::SetShaderProgram(GPUShaderProgram *pShaderProgram)
{
    if (m_pCurrentShaderProgram == pShaderProgram)
        return;

    OpenGLES2GPUShaderProgram *pGLShaderProgram = static_cast<OpenGLES2GPUShaderProgram *>(pShaderProgram);
    if (m_pCurrentShaderProgram != nullptr && pGLShaderProgram != nullptr)
    {
        pGLShaderProgram->Switch(this, m_pCurrentShaderProgram);
        m_pCurrentShaderProgram->Release();
        m_pCurrentShaderProgram = pGLShaderProgram;
        m_pCurrentShaderProgram->AddRef();
    }
    else
    {
        if (m_pCurrentShaderProgram != nullptr)
        {
            m_pCurrentShaderProgram->Unbind(this);
            m_pCurrentShaderProgram->Release();
        }

        if ((m_pCurrentShaderProgram = pGLShaderProgram) != nullptr)
        {
            m_pCurrentShaderProgram->Bind(this);
            m_pCurrentShaderProgram->AddRef();
        }
    }
}

void OpenGLES2GPUContext::WriteConstantBuffer(uint32 bufferIndex, uint32 fieldIndex, uint32 offset, uint32 count, const void *pData, bool commit /* = false */)
{
    // Get the starting offset
    uint32 globalBufferOffset = m_pConstantLibrary->GetBufferStartOffset(bufferIndex);
    if (globalBufferOffset == OpenGLES2ConstantLibrary::BufferOffsetInvalid)
        return;

    // Check buffer bounds
    globalBufferOffset += offset;
    DebugAssert((globalBufferOffset + count) <= m_pConstantLibrary->GetConstantStorageBufferSize());

    // No changes, bail out
    if (Y_memcmp(pData, m_pConstantLibraryBuffer + globalBufferOffset, count) == 0)
        return;

    // Move data in
    Y_memcpy(m_pConstantLibraryBuffer + globalBufferOffset, pData, count);

    // Update shader
    if (m_pCurrentShaderProgram != nullptr)
    {
        // got a field index?
        if (fieldIndex != 0xFFFFFFFF)
        {
            // Lookup this constant in the library
            OpenGLES2ConstantLibrary::ConstantID constantID = m_pConstantLibrary->LookupConstantID(bufferIndex, fieldIndex);
            if (constantID != OpenGLES2ConstantLibrary::ConstantIndexInvalid)
                m_pCurrentShaderProgram->OnLibraryConstantChanged(constantID);
        }
        else
        {
            // if field offset isn't specified, we have to find what intersects
            OpenGLES2ConstantLibrary::ConstantID firstConstantID, lastConstantID;
            if (!m_pConstantLibrary->FindConstantsAtOffset(bufferIndex, offset, offset + count - 1, &firstConstantID, &lastConstantID))
                return;

            // Notify shader
            if (m_pCurrentShaderProgram != nullptr)
            {
                for (OpenGLES2ConstantLibrary::ConstantID constantID = firstConstantID; constantID <= lastConstantID; constantID++)
                    m_pCurrentShaderProgram->OnLibraryConstantChanged(constantID);
            }
        }
    }
}

void OpenGLES2GPUContext::WriteConstantBufferStrided(uint32 bufferIndex, uint32 fieldIndex, uint32 offset, uint32 bufferStride, uint32 copySize, uint32 count, const void *pData, bool commit)
{
    // Get the starting offset
    uint32 globalBufferOffset = m_pConstantLibrary->GetBufferStartOffset(bufferIndex);
    if (globalBufferOffset == OpenGLES2ConstantLibrary::BufferOffsetInvalid)
        return;

    // Check buffer bounds
    globalBufferOffset += offset;
    DebugAssert((globalBufferOffset + count * bufferStride) <= m_pConstantLibrary->GetConstantStorageBufferSize());

    // Anything changed?
    if (Y_memcmp_stride(pData, copySize, m_pConstantLibraryBuffer + globalBufferOffset, bufferStride, copySize, count) == 0)
        return;

    // Move data in
    Y_memcpy_stride(m_pConstantLibraryBuffer + globalBufferOffset, bufferStride, pData, copySize, copySize, count);

    // Update shader
    if (m_pCurrentShaderProgram != nullptr)
    {
        // got a field index?
        if (fieldIndex != 0xFFFFFFFF)
        {
            // Lookup this constant in the library
            OpenGLES2ConstantLibrary::ConstantID constantID = m_pConstantLibrary->LookupConstantID(bufferIndex, fieldIndex);
            if (constantID != OpenGLES2ConstantLibrary::ConstantIndexInvalid)
                m_pCurrentShaderProgram->OnLibraryConstantChanged(constantID);
        }
        else
        {
            // if field offset isn't specified, we have to find what intersects
            OpenGLES2ConstantLibrary::ConstantID firstConstantID, lastConstantID;
            if (!m_pConstantLibrary->FindConstantsAtOffset(bufferIndex, offset, offset + count - 1, &firstConstantID, &lastConstantID))
                return;

            // Notify shader
            if (m_pCurrentShaderProgram != nullptr)
            {
                for (OpenGLES2ConstantLibrary::ConstantID constantID = firstConstantID; constantID <= lastConstantID; constantID++)
                    m_pCurrentShaderProgram->OnLibraryConstantChanged(constantID);
            }
        }
    }
}

void OpenGLES2GPUContext::CommitConstantBuffer(uint32 bufferIndex)
{
    // Update the current program's view of the constants in this buffer (just do all buffers, ehh.)
    // TODO: Apply range only
#if !DEFER_SHADER_STATE_CHANGES
    if (m_pCurrentShaderProgram != nullptr)
        m_pCurrentShaderProgram->CommitLibraryConstants(this);
#endif
}

void OpenGLES2GPUContext::SetShaderTextureUnit(uint32 index, GPUTexture *pTexture)
{
    GPUTexture *pOldTexture = m_currentTextureUnitBindings[index];

#if DEFER_SHADER_STATE_CHANGES

    if (pOldTexture == pTexture)
        return;

    if (pOldTexture != pTexture)
    {
        // release the old (if any) texture reference
        if (pOldTexture != nullptr)
            pOldTexture->Release();
        if ((m_currentTextureUnitBindings[index] = pTexture) != nullptr)
            pTexture->AddRef();
    }

    // update dirty range
    if (m_dirtyTextureUnitsLowerBounds < 0)
    {
        m_dirtyTextureUnitsLowerBounds = m_dirtyTextureUnitsUpperBounds = (int32)index;
    }
    else
    {
        m_dirtyTextureUnitsLowerBounds = Min(m_dirtyTextureUnitsLowerBounds, (int32)index);
        m_dirtyTextureUnitsUpperBounds = Max(m_dirtyTextureUnitsUpperBounds, (int32)index);
    }

#else

    // texture changed?
    if (pOldTexture != pTexture)
    {
        // alter the binding
        glActiveTexture(GL_TEXTURE0 + index);
        OpenGLES2Helpers::BindOpenGLTexture(pTexture);
        glActiveTexture(GL_TEXTURE0);

        if (pOldTexture != nullptr)
            pOldTexture->Release();
        if ((m_currentTextureUnitBindings[index] = pTexture) != nullptr)
            pTexture->AddRef();
    }

#endif

    // update counters
    if (pTexture != nullptr)
    {
        if (index >= m_activeTextureUnitBindings)
            m_activeTextureUnitBindings = index + 1;
    }
    else
    {
        if ((index + 1) == m_activeTextureUnitBindings)
        {
            uint32 lastPos = 0;
            for (uint32 i = 0; i < m_activeTextureUnitBindings; i++)
            {
                if (m_currentTextureUnitBindings[i] != nullptr)
                    lastPos = i + 1;
            }
            m_activeTextureUnitBindings = lastPos;
        }
    }
}

void OpenGLES2GPUContext::CommitShaderResources()
{
#if DEFER_SHADER_STATE_CHANGES
    // texture units
    if (m_dirtyTextureUnitsLowerBounds >= 0)
    {
        uint32 countToBind = (uint32)(m_dirtyTextureUnitsUpperBounds - m_dirtyTextureUnitsLowerBounds + 1);
        for (uint32 i = 0; i < countToBind; i++)
        {
            GPUTexture *pTexture = m_currentTextureUnitBindings[m_dirtyTextureUnitsLowerBounds + i];
            glActiveTexture(GL_TEXTURE0 + m_dirtyTextureUnitsLowerBounds + i);
            OpenGLES2Helpers::BindOpenGLTexture(pTexture);
        }
        glActiveTexture(GL_TEXTURE0);

        // reset dirty range
        m_dirtyTextureUnitsLowerBounds = m_dirtyTextureUnitsUpperBounds = -1;
    }

    if (m_pCurrentShaderProgram != nullptr)
        m_pCurrentShaderProgram->CommitLibraryConstants(this);
#endif
}

void OpenGLES2GPUContext::BindMutatorTextureUnit()
{
    glActiveTexture(GL_TEXTURE0 + m_mutatorTextureUnit);
}

void OpenGLES2GPUContext::RestoreMutatorTextureUnit()
{
    OpenGLES2Helpers::BindOpenGLTexture(m_currentTextureUnitBindings[m_mutatorTextureUnit]);
    glActiveTexture(GL_TEXTURE0);
}

void OpenGLES2GPUContext::Draw(uint32 firstVertex, uint32 nVertices)
{
    DebugAssert(m_pCurrentShaderProgram != nullptr);
    if (nVertices == 0)
        return;

    CommitVertexAttributes();
    CommitShaderResources();

    glDrawArrays(m_glDrawTopology, firstVertex, nVertices);
    m_drawCallCounter++;
}

void OpenGLES2GPUContext::DrawInstanced(uint32 firstVertex, uint32 nVertices, uint32 nInstances)
{
    Log_ErrorPrintf("OpenGLES2GPUContext::DrawInstanced: Unsupported on GLES 2.0");
}

void OpenGLES2GPUContext::DrawIndexed(uint32 startIndex, uint32 nIndices, uint32 baseVertex)
{
    DebugAssert(m_pCurrentShaderProgram != nullptr);
    if (nIndices == 0)
        return;

    DebugAssert(m_pCurrentIndexBuffer != nullptr);

    CommitVertexAttributes();
    CommitShaderResources();

    GLenum arrayType;
    uint32 indexBufferOffset = m_currentIndexBufferOffset;
    if (m_currentIndexFormat == GPU_INDEX_FORMAT_UINT32)
    {
        indexBufferOffset += startIndex * sizeof(uint32);
        arrayType = GL_UNSIGNED_INT;
    }
    else
    {
        indexBufferOffset += startIndex * sizeof(uint16);
        arrayType = GL_UNSIGNED_SHORT;
    }

    // TODO handle baseVertex > 0 by shuffling around the vertex pointers.. ugh.
    DebugAssert(baseVertex == 0);
    glDrawElements(m_glDrawTopology, nIndices, arrayType, reinterpret_cast<GLvoid *>(indexBufferOffset));

    m_drawCallCounter++;
}

void OpenGLES2GPUContext::DrawIndexedInstanced(uint32 startIndex, uint32 nIndices, uint32 baseVertex, uint32 nInstances)
{
    Log_ErrorPrintf("OpenGLES2GPUContext::DrawIndexedInstanced: Unsupported on GLES 2.0");
}

void OpenGLES2GPUContext::Dispatch(uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ)
{
    Log_ErrorPrintf("OpenGLES2GPUContext::Dispatch: Unsupported on GLES 2.0");
}

void OpenGLES2GPUContext::DrawUserPointer(const void *pVertices, uint32 VertexSize, uint32 nVertices)
{
    uint32 maxVerticesPerPass = m_userVertexBufferSize / VertexSize;
    DebugAssert(m_pCurrentShaderProgram != nullptr);

    // obtain the current vertex buffer in slot zero, since we need to overwrite this
    OpenGLES2GPUBuffer *pRestoreVertexBuffer = m_currentVertexBuffers[0].pVertexBuffer;
    uint32 restoreVertexBufferOffset = m_currentVertexBuffers[0].Offset;
    uint32 restoreVertexBufferStride = m_currentVertexBuffers[0].Stride;
    if (pRestoreVertexBuffer != nullptr)
        pRestoreVertexBuffer->AddRef();

    // update uniforms
    CommitShaderResources();

    // draw vertices loop
    const byte *pCurrentVertexPointer = reinterpret_cast<const byte *>(pVertices);
    uint32 nRemainingVertices = nVertices;
    while (nRemainingVertices > 0)
    {
        uint32 nVerticesThisPass = Min(nRemainingVertices, maxVerticesPerPass);
        uint32 spaceRequired = VertexSize * nVerticesThisPass;

        // use buffer sub data for ES2. discard buffer if it'll overflow.
        if ((m_userVertexBufferPosition + spaceRequired) >= m_userVertexBufferSize)
            m_userVertexBufferPosition = 0;

        // write to end of buffer
        WriteBuffer(m_pUserVertexBuffer, pCurrentVertexPointer, m_userVertexBufferPosition, spaceRequired);

        // bind the vertex buffer
        OpenGLES2GPUContext::SetVertexBuffer(0, m_pUserVertexBuffer, m_userVertexBufferPosition, VertexSize);
        CommitVertexAttributes();

        // invoke draw
        glDrawArrays(m_glDrawTopology, 0, nVerticesThisPass);
        m_drawCallCounter++;

        // increment pointers
        m_userVertexBufferPosition += spaceRequired;
        pCurrentVertexPointer += VertexSize * nVerticesThisPass;
        nRemainingVertices -= nVerticesThisPass;
    }

    // re-bind saved vertex buffer
    OpenGLES2GPUContext::SetVertexBuffer(0, pRestoreVertexBuffer, restoreVertexBufferOffset, restoreVertexBufferStride);
    if (pRestoreVertexBuffer != nullptr)
        pRestoreVertexBuffer->Release();

    // leave the commit out here, next draw will fix it up anyway
    //CommitVertexAttributes();
}

void OpenGLES2GPUContext::DrawIndexedUserPointer(const void *pVertices, uint32 VertexSize, uint32 nVertices, const void *pIndices, GPU_INDEX_FORMAT IndexFormat, uint32 nIndices)
{
    Panic("");
}

bool OpenGLES2GPUContext::CopyTexture(GPUTexture2D *pSourceTexture, GPUTexture2D *pDestinationTexture)
{
    // textures have to be compatible, for now this means same texture format
    OpenGLES2GPUTexture2D *pOpenGLSourceTexture = static_cast<OpenGLES2GPUTexture2D *>(pSourceTexture);
    OpenGLES2GPUTexture2D *pOpenGLDestinationTexture = static_cast<OpenGLES2GPUTexture2D *>(pDestinationTexture);
    if (pOpenGLSourceTexture->GetDesc()->Width != pOpenGLDestinationTexture->GetDesc()->Width ||
        pOpenGLSourceTexture->GetDesc()->Height != pOpenGLDestinationTexture->GetDesc()->Height ||
        pOpenGLSourceTexture->GetDesc()->Format != pOpenGLDestinationTexture->GetDesc()->Format ||
        pOpenGLSourceTexture->GetDesc()->MipLevels != pOpenGLDestinationTexture->GetDesc()->MipLevels)
    {
        return false;
    }

    Panic("");
    return true;
}

bool OpenGLES2GPUContext::CopyTextureRegion(GPUTexture2D *pSourceTexture, uint32 sourceX, uint32 sourceY, uint32 width, uint32 height, uint32 sourceMipLevel, GPUTexture2D *pDestinationTexture, uint32 destX, uint32 destY, uint32 destMipLevel)
{
    // textures have to be compatible, for now this means same texture format
    OpenGLES2GPUTexture2D *pOpenGLSourceTexture = static_cast<OpenGLES2GPUTexture2D *>(pSourceTexture);
    OpenGLES2GPUTexture2D *pOpenGLDestinationTexture = static_cast<OpenGLES2GPUTexture2D *>(pDestinationTexture);
    if (pOpenGLSourceTexture->GetDesc()->Format != pOpenGLDestinationTexture->GetDesc()->Format ||
        pOpenGLSourceTexture->GetDesc()->MipLevels != pOpenGLDestinationTexture->GetDesc()->MipLevels)
    {
        return false;
    }

    Panic("");
    return true;
}

void OpenGLES2GPUContext::GenerateMips(GPUTexture *pTexture)
{
    BindMutatorTextureUnit();

    switch (pTexture->GetTextureType())
    {
    case TEXTURE_TYPE_2D:
        glBindTexture(GL_TEXTURE_2D, static_cast<OpenGLES2GPUTexture2D *>(pTexture)->GetGLTextureId());
        glGenerateMipmap(GL_TEXTURE_2D);
        break;

    case TEXTURE_TYPE_CUBE:
        glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<OpenGLES2GPUTextureCube *>(pTexture)->GetGLTextureId());
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        break;
    }

    RestoreMutatorTextureUnit();
}

// stubs
bool OpenGLES2GPUContext::BeginQuery(GPUQuery *pQuery)
{
    Log_ErrorPrint("OpenGLES2GPUContext::BeginQuery: Unsupported on GLES2");
    return false;
}

bool OpenGLES2GPUContext::EndQuery(GPUQuery *pQuery)
{
    Log_ErrorPrint("OpenGLES2GPUContext::EndQuery: Unsupported on GLES2");
    return false;
}

GPU_QUERY_GETDATA_RESULT OpenGLES2GPUContext::GetQueryData(GPUQuery *pQuery, void *pData, uint32 cbData, uint32 flags)
{
    Log_ErrorPrint("OpenGLES2GPUContext::GetQueryData: Unsupported on GLES2");
    return GPU_QUERY_GETDATA_RESULT_ERROR;
}

bool OpenGLES2GPUContext::ReadTexture(GPUTexture1D *pTexture, void *pDestination, uint32 cbDestination, uint32 mipIndex, uint32 start, uint32 count)
{
    Log_ErrorPrint("OpenGLES2GPUContext::ReadTexture<Texture1D>: Unsupported on GLES2");
    return false;
}

bool OpenGLES2GPUContext::WriteTexture(GPUTexture1D *pTexture, const void *pSource, uint32 cbSource, uint32 mipIndex, uint32 start, uint32 count)
{
    Log_ErrorPrint("OpenGLES2GPUContext::WriteTexture<Texture1D>: Unsupported on GLES2");
    return false;
}

bool OpenGLES2GPUContext::ReadTexture(GPUTexture1DArray *pTexture, void *pDestination, uint32 cbDestination, uint32 arrayIndex, uint32 mipIndex, uint32 start, uint32 count)
{
    Log_ErrorPrint("OpenGLES2GPUContext::ReadTexture<Texture1DArray>: Unsupported on GLES2");
    return false;
}

bool OpenGLES2GPUContext::WriteTexture(GPUTexture1DArray *pTexture, const void *pSource, uint32 cbSource, uint32 arrayIndex, uint32 mipIndex, uint32 start, uint32 count)
{
    Log_ErrorPrint("OpenGLES2GPUContext::WriteTexture<Texture1DArray>: Unsupported on GLES2");
    return false;
}

bool OpenGLES2GPUContext::ReadTexture(GPUTexture2DArray *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 arrayIndex, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    Log_ErrorPrint("OpenGLES2GPUContext::ReadTexture<Texture2DArray>: Unsupported on GLES2");
    return false;
}

bool OpenGLES2GPUContext::WriteTexture(GPUTexture2DArray *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 arrayIndex, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    Log_ErrorPrint("OpenGLES2GPUContext::WriteTexture<Texture2DArray>: Unsupported on GLES2");
    return false;
}

bool OpenGLES2GPUContext::ReadTexture(GPUTexture3D *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 destinationSlicePitch, uint32 cbDestination, uint32 mipIndex, uint32 startX, uint32 startY, uint32 startZ, uint32 countX, uint32 countY, uint32 countZ)
{
    Log_ErrorPrint("OpenGLES2GPUContext::ReadTexture<Texture3D>: Unsupported on GLES2");
    return false;
}

bool OpenGLES2GPUContext::WriteTexture(GPUTexture3D *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 sourceSlicePitch, uint32 cbSource, uint32 mipIndex, uint32 startX, uint32 startY, uint32 startZ, uint32 countX, uint32 countY, uint32 countZ)
{
    Log_ErrorPrint("OpenGLES2GPUContext::WriteTexture<Texture3D>: Unsupported on GLES2");
    return false;
}

bool OpenGLES2GPUContext::ReadTexture(GPUTextureCubeArray *pTexture, void *pDestination, uint32 destinationRowPitch, uint32 cbDestination, uint32 arrayIndex, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    Log_ErrorPrint("OpenGLES2GPUContext::ReadTexture<GPUTextureCubeArray>: Unsupported on GLES2");
    return false;
}

bool OpenGLES2GPUContext::WriteTexture(GPUTextureCubeArray *pTexture, const void *pSource, uint32 sourceRowPitch, uint32 cbSource, uint32 arrayIndex, CUBEMAP_FACE face, uint32 mipIndex, uint32 startX, uint32 startY, uint32 countX, uint32 countY)
{
    Log_ErrorPrint("OpenGLES2GPUContext::WriteTexture<TextureCubeArray>: Unsupported on GLES2");
    return false;
}

void OpenGLES2GPUContext::SetPredication(GPUQuery *pQuery)
{
    Log_ErrorPrint("OpenGLES2GPUContext::SetPredication: Unsupported on GLES2");
}

void OpenGLES2GPUContext::BlitFrameBuffer(GPUTexture2D *pTexture, uint32 sourceX, uint32 sourceY, uint32 sourceWidth, uint32 sourceHeight, uint32 destX, uint32 destY, uint32 destWidth, uint32 destHeight, RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER resizeFilter /*= RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_NEAREST*/)
{
    Log_ErrorPrint("OpenGLES2GPUContext::BlitFrameBuffer: Unsupported on GLES2");
}