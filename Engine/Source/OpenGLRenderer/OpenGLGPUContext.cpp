#include "OpenGLRenderer/PrecompiledHeader.h"
#include "OpenGLRenderer/OpenGLGPUContext.h"
#include "OpenGLRenderer/OpenGLGPUDevice.h"
#include "OpenGLRenderer/OpenGLGPUOutputBuffer.h"
#include "OpenGLRenderer/OpenGLGPUTexture.h"
#include "OpenGLRenderer/OpenGLGPUBuffer.h"
#include "OpenGLRenderer/OpenGLGPUShaderProgram.h"
#include "OpenGLRenderer/OpenGLRenderBackend.h"
#include "Renderer/ShaderConstantBuffer.h"
Log_SetChannel(OpenGLRenderBackend);

#define DEFER_SHADER_STATE_CHANGES 1

OpenGLGPUContext::OpenGLGPUContext(OpenGLGPUDevice *pDevice, SDL_GLContext pSDLGLContext, OpenGLGPUOutputBuffer *pOutputBuffer)
{
    m_pDevice = pDevice;
    m_pDevice->SetGPUContext(this);
    m_pDevice->AddRef();

    m_pSDLGLContext = pSDLGLContext;
    m_pCurrentOutputBuffer = pOutputBuffer;
    if (m_pCurrentOutputBuffer != nullptr)
        m_pCurrentOutputBuffer->AddRef();

    m_pConstants = nullptr;

    Y_memzero(&m_currentViewport, sizeof(m_currentViewport));
    Y_memzero(&m_scissorRect, sizeof(m_scissorRect));
    m_drawTopology = DRAW_TOPOLOGY_TRIANGLE_LIST;
    m_glDrawTopology = GL_TRIANGLES;

    m_vertexArrayObjectId = 0;
    m_activeVertexBuffers = 0;
    m_dirtyVertexBuffers = false;
    m_activeVertexAttributes = 0;
    m_dirtyVertexAttributes = false;

    m_pCurrentIndexBuffer = nullptr;
    m_currentIndexFormat = GPU_INDEX_FORMAT_UINT16;
    m_currentIndexBufferOffset = 0;

    m_pCurrentShaderProgram = nullptr;
    m_activeUniformBlockBindings = 0;
    m_dirtyUniformBlockBindingsLowerBounds = -1;
    m_dirtyUniformBlockBindingsUpperBounds = -1;
    m_activeTextureUnitBindings = 0;
    m_dirtyTextureUnitsLowerBounds = -1;
    m_dirtyTextureUnitsUpperBounds = -1;
    m_activeImageUnitBindings = 0;
    m_dirtyImageUnitsLowerBounds = -1;
    m_dirtyImageUnitsUpperBounds = -1;
    m_activeShaderStorageBufferBindings = 0;
    m_dirtyShaderStorageBuffersLowerBounds = -1;
    m_dirtyShaderStorageBuffersUpperBounds = -1;
    m_mutatorTextureUnit = 0;

    m_pCurrentRasterizerState = nullptr;
    m_pCurrentDepthStencilState = nullptr;
    m_currentDepthStencilStateStencilRef = 0;
    m_pCurrentBlendState = nullptr;
    m_currentBlendStateBlendFactors.SetZero();

    m_drawFrameBufferObjectId = 0;
    m_readFrameBufferObjectId = 0;

    Y_memzero(m_pCurrentRenderTargets, sizeof(m_pCurrentRenderTargets));
    m_pCurrentDepthStencilBuffer = nullptr;
    m_nCurrentRenderTargets = 0;

    m_pUserVertexBuffer = nullptr;
    m_userVertexBufferSize = 1024 * 1024;
    m_userVertexBufferPosition = 0;
}

OpenGLGPUContext::~OpenGLGPUContext()
{
    // unbind vertex attributes
    SetShaderVertexAttributes(nullptr, 0);
    CommitVertexAttributes();

    // unbind shader stuff
    for (uint32 i = 0; i < m_activeShaderStorageBufferBindings; i++)
    {
        if (m_currentShaderStorageBufferBindings[i] != nullptr)
            SetShaderStorageBuffer(i, nullptr);
    }
    for (uint32 i = 0; i < m_activeImageUnitBindings; i++)
    {
        if (m_currentImageUnitBindings[i] != nullptr)
            SetShaderImageUnit(i, nullptr);
    }
    for (uint32 i = 0; i < m_activeUniformBlockBindings; i++)
    {
        if (m_currentUniformBlockBindings[i] != nullptr)
            SetShaderUniformBlock(i, nullptr);
    }
    for (uint32 i = 0; i < m_activeTextureUnitBindings; i++)
    {
        if (m_currentTextureUnitBindings[i].pTexture != nullptr || m_currentTextureUnitBindings[i].pSampler != nullptr)
            SetShaderTextureUnit(i, nullptr, nullptr);
    }

    if (m_pCurrentShaderProgram != nullptr)
        SetShaderProgram(nullptr);

    CommitShaderResources();

    for (uint32 i = 0; i < m_constantBuffers.GetSize(); i++)
    {
        ConstantBuffer *constantBuffer = &m_constantBuffers[i];
        if (constantBuffer->pGPUBuffer != nullptr)
            constantBuffer->pGPUBuffer->Release();

        delete[] constantBuffer->pLocalMemory;
    }

    if (m_drawFrameBufferObjectId > 0)
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &m_drawFrameBufferObjectId);
    }

    if (m_readFrameBufferObjectId > 0)
        glDeleteFramebuffers(1, &m_readFrameBufferObjectId);

    if (m_vertexArrayObjectId > 0)
    {
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &m_vertexArrayObjectId);
    }

    for (uint32 i = 0; i < m_activeVertexBuffers; i++)
        SAFE_RELEASE(m_currentVertexBuffers[i].pVertexBuffer);

    SAFE_RELEASE(m_pCurrentIndexBuffer);

    SAFE_RELEASE(m_pCurrentRasterizerState);
    SAFE_RELEASE(m_pCurrentDepthStencilState);
    SAFE_RELEASE(m_pCurrentBlendState);

    for (uint32 i = 0; i < GPU_MAX_SIMULTANEOUS_RENDER_TARGETS; i++)
        SAFE_RELEASE(m_pCurrentRenderTargets[i]);
    
    SAFE_RELEASE(m_pCurrentDepthStencilBuffer);

    //SAFE_RELEASE(m_pUserIndexBuffer);
    SAFE_RELEASE(m_pUserVertexBuffer);

    delete m_pConstants;

    // last thing to release is the output window, this is safe because it won't make any gl calls
    SAFE_RELEASE(m_pCurrentOutputBuffer);

    // release device
    m_pDevice->SetGPUContext(nullptr);
    m_pDevice->Release();
}

void OpenGLGPUContext::UpdateVSyncState(RENDERER_VSYNC_TYPE vsyncType)
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

bool OpenGLGPUContext::Create()
{
    glGenFramebuffers(1, &m_drawFrameBufferObjectId);
    glGenFramebuffers(1, &m_readFrameBufferObjectId);
    if (m_drawFrameBufferObjectId == 0 || m_readFrameBufferObjectId == 0)
    {
        GL_PRINT_ERROR("OpenGLGPUContext::Create: glGenFrameBuffers failed: ");
        return false;
    }

    glGenVertexArrays(1, &m_vertexArrayObjectId);
    if (m_vertexArrayObjectId == 0)
    {
        GL_PRINT_ERROR("OpenGLGPUContext::Create: glGenVertexArrays failed: ");
        return false;
    }
    glBindVertexArray(m_vertexArrayObjectId);

    // allocate shader state arrays
    uint32 maxCombinedTextureImageUnits = 0;
    uint32 maxUniformBufferBindings = 0;
    uint32 maxCombinedImageUniforms = 0;
    uint32 maxCombinedShaderStorageBlocks = 0;
    uint32 maxVertexAttributes = 0;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, reinterpret_cast<GLint *>(&maxCombinedTextureImageUnits));
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, reinterpret_cast<GLint *>(&maxUniformBufferBindings));
    glGetIntegerv(GL_MAX_COMBINED_IMAGE_UNIFORMS, reinterpret_cast<GLint *>(&maxCombinedImageUniforms));
    glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, reinterpret_cast<GLint *>(&maxCombinedShaderStorageBlocks));
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, reinterpret_cast<GLint *>(&maxVertexAttributes));

    // these should be >0
    if (maxCombinedTextureImageUnits == 0 || maxUniformBufferBindings == 0 || maxVertexAttributes == 0)
    {
        Log_ErrorPrintf("OpenGLGPUContext::Create: GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS (%u) or GL_MAX_UNIFORM_BUFFER_BINDINGS (%u) is below required amounts.", maxCombinedTextureImageUnits, maxUniformBufferBindings);
        return false;
    }

    // allocate storage
    m_currentTextureUnitBindings.Resize(maxCombinedTextureImageUnits);
    m_currentTextureUnitBindings.ZeroContents();
    m_currentUniformBlockBindings.Resize(maxUniformBufferBindings);
    m_currentUniformBlockBindings.ZeroContents();
    m_currentImageUnitBindings.Resize(maxCombinedImageUniforms);
    m_currentImageUnitBindings.ZeroContents();
    m_currentShaderStorageBufferBindings.Resize(maxCombinedShaderStorageBlocks);
    m_currentShaderStorageBufferBindings.ZeroContents();
    m_currentVertexAttributes.Resize(maxVertexAttributes);
    m_currentVertexAttributes.ZeroContents();
    m_currentVertexBuffers.Resize(maxVertexAttributes);
    m_currentVertexBuffers.ZeroContents();

    // set deps
    m_mutatorTextureUnit = m_currentTextureUnitBindings.GetSize() - 1;

    // create plain vertex buffer
    {
        GPU_BUFFER_DESC vertexBufferDesc(GPU_BUFFER_FLAG_MAPPABLE | GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER, m_userVertexBufferSize);
        if ((m_pUserVertexBuffer = static_cast<OpenGLGPUBuffer *>(m_pDevice->CreateBuffer(&vertexBufferDesc, NULL))) == NULL)
        {
            Log_ErrorPrintf("OpenGLGPUContext::Create: Failed to create plain dynamic vertex buffer.");
            return false;
        }
    }

    // create constants buffers
    m_pConstants = new GPUContextConstants(this);

    // create constant buffers
    if (!CreateConstantBuffers())
        return false;

    // update swap interval
    UpdateVSyncState(m_pCurrentOutputBuffer->GetVSyncType());

    // enable srgb writes for textures in this format
    if (PixelFormatHelpers::IsSRGBFormat(m_pCurrentOutputBuffer->GetBackBufferFormat()))
        glEnable(GL_FRAMEBUFFER_SRGB);

    // done
    Log_InfoPrint("OpenGLGPUContext::Create: Context creation complete.");
    return true;
}

void OpenGLGPUContext::ClearState(bool clearShaders /* = true */, bool clearBuffers /* = true */, bool clearStates /* = true */, bool clearRenderTargets /* = true */)
{
    if (clearShaders)
    {
        SetShaderProgram(NULL);

        // unbind shader stuff
        SetShaderVertexAttributes(nullptr, 0);
        CommitVertexAttributes();
        for (uint32 i = 0; i < m_activeShaderStorageBufferBindings; i++)
        {
            if (m_currentShaderStorageBufferBindings[i] != nullptr)
                SetShaderStorageBuffer(i, nullptr);
        }
        for (uint32 i = 0; i < m_activeImageUnitBindings; i++)
        {
            if (m_currentImageUnitBindings[i] != nullptr)
                SetShaderImageUnit(i, nullptr);
        }
        for (uint32 i = 0; i < m_activeUniformBlockBindings; i++)
        {
            if (m_currentUniformBlockBindings[i] != nullptr)
                SetShaderUniformBlock(i, nullptr);
        }
        for (uint32 i = 0; i < m_activeTextureUnitBindings; i++)
        {
            if (m_currentTextureUnitBindings[i].pTexture != nullptr || m_currentTextureUnitBindings[i].pSampler != nullptr)
                SetShaderTextureUnit(i, nullptr, nullptr);
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
        SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState());
        SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(), 0);
        SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateNoBlending());
        SetDrawTopology(DRAW_TOPOLOGY_UNDEFINED);

        RENDERER_SCISSOR_RECT scissor(0, 0, 0, 0);
        SetFullViewport(nullptr);
        SetScissorRect(&scissor);
    }

    if (clearRenderTargets)
        SetRenderTargets(0, nullptr, nullptr);
}

GPURasterizerState *OpenGLGPUContext::GetRasterizerState()
{
    return m_pCurrentRasterizerState;
}

void OpenGLGPUContext::SetRasterizerState(GPURasterizerState *pRasterizerState)
{
    if (m_pCurrentRasterizerState == pRasterizerState)
        return;

    // if new buffer is null, unbind old
    if (m_pCurrentRasterizerState != NULL)
    {
        if (pRasterizerState == NULL)
            m_pCurrentRasterizerState->Unapply();

        m_pCurrentRasterizerState->Release();
    }

    if ((m_pCurrentRasterizerState = static_cast<OpenGLGPURasterizerState *>(pRasterizerState)) != NULL)
    {
        m_pCurrentRasterizerState->AddRef();
        m_pCurrentRasterizerState->Apply();
    }
}

GPUDepthStencilState *OpenGLGPUContext::GetDepthStencilState()
{
    return m_pCurrentDepthStencilState;
}

uint8 OpenGLGPUContext::GetDepthStencilStateStencilRef()
{
    return m_currentDepthStencilStateStencilRef;
}

void OpenGLGPUContext::SetDepthStencilState(GPUDepthStencilState *pDepthStencilState, uint8 StencilRef)
{
    if (m_pCurrentDepthStencilState == pDepthStencilState && m_currentDepthStencilStateStencilRef == StencilRef)
        return;

    m_currentDepthStencilStateStencilRef = StencilRef;

    // if new buffer is null, unbind old
    if (m_pCurrentDepthStencilState != NULL)
    {
        if (pDepthStencilState == NULL)
            m_pCurrentDepthStencilState->Unapply();

        m_pCurrentDepthStencilState->Release();
    }

    if ((m_pCurrentDepthStencilState = static_cast<OpenGLGPUDepthStencilState *>(pDepthStencilState)) != NULL)
    {
        m_pCurrentDepthStencilState->AddRef();
        m_pCurrentDepthStencilState->Apply(StencilRef);
    }
}

GPUBlendState *OpenGLGPUContext::GetBlendState()
{
    return m_pCurrentBlendState;
}

const float4 &OpenGLGPUContext::GetBlendStateBlendFactor()
{
    return m_currentBlendStateBlendFactors;
}

void OpenGLGPUContext::SetBlendState(GPUBlendState *pBlendState, const float4 &BlendFactor /* = float4::One */)
{
    if (m_pCurrentBlendState != pBlendState)
    {
        // if new buffer is null, unbind old
        if (m_pCurrentBlendState != NULL)
        {
            if (pBlendState == NULL)
                m_pCurrentBlendState->Unapply();

            m_pCurrentBlendState->Release();
        }

        if ((m_pCurrentBlendState = static_cast<OpenGLGPUBlendState *>(pBlendState)) != NULL)
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

const RENDERER_VIEWPORT *OpenGLGPUContext::GetViewport()
{
    return &m_currentViewport;
}

void OpenGLGPUContext::SetViewport(const RENDERER_VIEWPORT *pNewViewport)
{
    if (Y_memcmp(&m_currentViewport, pNewViewport, sizeof(RENDERER_VIEWPORT)) == 0)
        return;

    Y_memcpy(&m_currentViewport, pNewViewport, sizeof(m_currentViewport));
    glViewport(m_currentViewport.TopLeftX, m_currentViewport.TopLeftY, m_currentViewport.Width, m_currentViewport.Height);
    glDepthRange(m_currentViewport.MinDepth, m_currentViewport.MaxDepth);

    // update constants
    m_pConstants->SetViewportOffset((float)m_currentViewport.TopLeftX, (float)m_currentViewport.TopLeftY, false);
    m_pConstants->SetViewportSize((float)m_currentViewport.Width, (float)m_currentViewport.Height, false);
    m_pConstants->CommitChanges();

    // fix scissor rect
    if (m_scissorRect.Top != m_scissorRect.Bottom)
        glScissor(m_scissorRect.Left, m_currentViewport.Height - m_scissorRect.Bottom, m_scissorRect.Right - m_scissorRect.Left, m_scissorRect.Bottom - m_scissorRect.Top);
}

void OpenGLGPUContext::SetFullViewport(GPUTexture *pForRenderTarget /*= NULL*/)
{
    RENDERER_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;

    if (pForRenderTarget != nullptr)
    {
        uint3 renderTargetDimensions = Renderer::GetTextureDimensions(pForRenderTarget);
        viewport.Width = renderTargetDimensions.x;
        viewport.Height = renderTargetDimensions.y;
    }
    else if (m_nCurrentRenderTargets > 0 || m_pCurrentDepthStencilBuffer != nullptr)
    {
        uint3 renderTargetDimensions = Renderer::GetTextureDimensions((m_nCurrentRenderTargets > 0) ? m_pCurrentRenderTargets[0]->GetTargetTexture() : m_pCurrentDepthStencilBuffer->GetTargetTexture());
        viewport.Width = renderTargetDimensions.x;
        viewport.Height = renderTargetDimensions.y;
    }
    else
    {
        viewport.Width = m_pCurrentOutputBuffer->GetWidth();
        viewport.Height = m_pCurrentOutputBuffer->GetHeight();
    }

    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    SetViewport(&viewport);
}

const RENDERER_SCISSOR_RECT *OpenGLGPUContext::GetScissorRect()
{
    return &m_scissorRect;
}

void OpenGLGPUContext::SetScissorRect(const RENDERER_SCISSOR_RECT *pScissorRect)
{
    if (Y_memcmp(&m_scissorRect, pScissorRect, sizeof(m_scissorRect)) == 0)
        return;

    Y_memcpy(&m_scissorRect, pScissorRect, sizeof(m_scissorRect));
    glScissor(m_scissorRect.Left, m_currentViewport.Height - m_scissorRect.Bottom, m_scissorRect.Right - m_scissorRect.Left, m_scissorRect.Bottom - m_scissorRect.Top);
}

void OpenGLGPUContext::ClearTargets(bool clearColor /* = true */, bool clearDepth /* = true */, bool clearStencil /* = true */, const float4 &clearColorValue /* = float4::Zero */, float clearDepthValue /* = 1.0f */, uint8 clearStencilValue /* = 0 */)
{
    bool oldColorWritesEnabled = (m_pCurrentBlendState == NULL) ? true : m_pCurrentBlendState->GetGLColorWritesEnabled();
    GLboolean oldDepthMask = (m_pCurrentDepthStencilState == NULL) ? GL_TRUE : m_pCurrentDepthStencilState->GetGLDepthMask();
    GLuint oldStencilMask = (m_pCurrentDepthStencilState == NULL) ? 0xFF : m_pCurrentDepthStencilState->GetGLStencilWriteMask();
    bool oldScissorTest = (m_pCurrentRasterizerState == NULL) ? false : m_pCurrentRasterizerState->GetGLScissorTestEnable();

    // change settings so the whole RT gets cleared
    if (!oldColorWritesEnabled)
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    if (oldDepthMask == GL_FALSE)
        glDepthMask(GL_TRUE);
    if (oldStencilMask != 0xFF)
        glStencilMask(0xFF);
    if (oldScissorTest)
        glDisable(GL_SCISSOR_TEST);
    
    // use new functions if using FBOs
    if (m_nCurrentRenderTargets != 0 || m_pCurrentDepthStencilBuffer != NULL)
    {
        uint32 i;

        if (clearColor)
        {
            for (i = 0; i < m_nCurrentRenderTargets; i++)
            {
                if (m_pCurrentRenderTargets[i] != NULL)
                    glClearBufferfv(GL_COLOR, i, clearColorValue);
            }
        }

        if (m_pCurrentDepthStencilBuffer != NULL)
        {
            if (clearDepth && clearStencil)
                glClearBufferfi(GL_DEPTH_STENCIL, 0, clearDepthValue, clearStencilValue);
            else if (clearDepth)
                glClearBufferfv(GL_DEPTH, 0, &clearDepthValue);
            else if (clearStencil)
            {
                GLint clearStencilValueInt = (GLint)clearStencilValue;
                glClearBufferiv(GL_STENCIL, 0, &clearStencilValueInt);
            }
        }
    }
    else
    {
        GLbitfield clearMask = 0;
        if (clearColor)
        {
            glClearColor(clearColorValue.r, clearColorValue.g, clearColorValue.b, clearColorValue.a);
            clearMask |= GL_COLOR_BUFFER_BIT;
        }
        if (clearDepth)
        {
            glDepthMask(GL_TRUE);
            glClearDepth(clearDepthValue);
            clearMask |= GL_DEPTH_BUFFER_BIT;
        }
        if (clearStencil)
        {
            glClearStencil(clearStencilValue);
            clearMask |= GL_STENCIL_BUFFER_BIT;
        }

        if (clearMask != 0)
            glClear(clearMask);
    }

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

void OpenGLGPUContext::DiscardTargets(bool discardColor /* = true */, bool discardDepth /* = true */, bool discardStencil /* = true */)
{
    if (GLAD_GL_EXT_discard_framebuffer)
    {
        GLenum attachments[3];
        uint32 nAttachments = 0;

        // ugh, multiple paths again??!
        if (m_nCurrentRenderTargets > 0 || m_pCurrentDepthStencilBuffer != nullptr)
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

        glDiscardFramebufferEXT(GL_DRAW_FRAMEBUFFER, nAttachments, attachments);
    }
}

void OpenGLGPUContext::SetOutputBuffer(GPUOutputBuffer *pSwapChain)
{
    OpenGLGPUOutputBuffer *pOpenGLOutputBuffer = static_cast<OpenGLGPUOutputBuffer *>(pSwapChain);
    DebugAssert(pOpenGLOutputBuffer != nullptr);

    // same?
    if (m_pCurrentOutputBuffer == pOpenGLOutputBuffer)
        return;

    // make this the new current context
    if (SDL_GL_MakeCurrent(pOpenGLOutputBuffer->GetSDLWindow(), m_pSDLGLContext) != 0)
    {
        Log_ErrorPrintf("OpenGLGPUContext::SetOutputBuffer: SDL_GL_MakeCurrent failed: %s", SDL_GetError());
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

bool OpenGLGPUContext::GetExclusiveFullScreen()
{
    if (SDL_GetWindowFlags(m_pCurrentOutputBuffer->GetSDLWindow()) & SDL_WINDOW_FULLSCREEN)
        return true;
    else
        return false;
}

bool OpenGLGPUContext::SetExclusiveFullScreen(bool enabled, uint32 width, uint32 height, uint32 refreshRate)
{
    if (enabled)
    {
        // find the matching sdl pixel format
        SDL_DisplayMode preferredDisplayMode;
        preferredDisplayMode.w = (width == 0) ? m_pCurrentOutputBuffer->GetWidth() : width;
        preferredDisplayMode.h = (height == 0) ? m_pCurrentOutputBuffer->GetHeight() : height;
        preferredDisplayMode.refresh_rate = refreshRate;
        preferredDisplayMode.driverdata = nullptr;

        // we only have a limited number of backbuffer format possibilities
        switch (m_pCurrentOutputBuffer->GetBackBufferFormat())
        {
        case PIXEL_FORMAT_R8G8B8A8_UNORM:       preferredDisplayMode.format = SDL_PIXELFORMAT_RGBA8888;     break;
        case PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB:  preferredDisplayMode.format = SDL_PIXELFORMAT_RGBA8888;     break;
        case PIXEL_FORMAT_B8G8R8A8_UNORM:       preferredDisplayMode.format = SDL_PIXELFORMAT_BGRA8888;     break;
        case PIXEL_FORMAT_B8G8R8A8_UNORM_SRGB:  preferredDisplayMode.format = SDL_PIXELFORMAT_BGRA8888;     break;
        case PIXEL_FORMAT_B8G8R8X8_UNORM:       preferredDisplayMode.format = SDL_PIXELFORMAT_BGRX8888;     break;
        case PIXEL_FORMAT_B8G8R8X8_UNORM_SRGB:  preferredDisplayMode.format = SDL_PIXELFORMAT_BGRX8888;     break;
        default:
            Log_ErrorPrintf("OpenGLGPUContext::SetExclusiveFullScreen: Unable to find SDL pixel format for %s", PixelFormat_GetPixelFormatInfo(m_pCurrentOutputBuffer->GetBackBufferFormat())->Name);
            return false;
        }

        // find the closest match
        SDL_DisplayMode foundDisplayMode;
        if (SDL_GetClosestDisplayMode(0, &preferredDisplayMode, &foundDisplayMode) == nullptr)
        {
            Log_ErrorPrintf("OpenGLGPUContext::SetExclusiveFullScreen: Unable to find matching video mode for: %i x %i (%s)", preferredDisplayMode.w, preferredDisplayMode.h, PixelFormat_GetPixelFormatInfo(m_pCurrentOutputBuffer->GetBackBufferFormat())->Name);
            return false;
        }

        // change to this mode
        if (SDL_SetWindowDisplayMode(m_pCurrentOutputBuffer->GetSDLWindow(), &foundDisplayMode) != 0)
        {
            Log_ErrorPrintf("OpenGLGPUContext::SetExclusiveFullScreen: SDL_SetWindowDisplayMode failed: %s", SDL_GetError());
            return false;
        }

        // and switch to fullscreen if not already there
        if (!(SDL_GetWindowFlags(m_pCurrentOutputBuffer->GetSDLWindow()) & SDL_WINDOW_FULLSCREEN))
        {
            if (SDL_SetWindowFullscreen(m_pCurrentOutputBuffer->GetSDLWindow(), SDL_WINDOW_FULLSCREEN) != 0)
            {
                Log_ErrorPrintf("OpenGLGPUContext::SetExclusiveFullScreen: SDL_SetWindowFullscreen failed: %s", SDL_GetError());
                return false;
            }
        }

        // update the window size
        m_pCurrentOutputBuffer->Resize(foundDisplayMode.w, foundDisplayMode.h);
        return true;
    }
    else
    {
        // leaving fullscreen
        if (SDL_GetWindowFlags(m_pCurrentOutputBuffer->GetSDLWindow()) & SDL_WINDOW_FULLSCREEN)
        {
            if (SDL_SetWindowFullscreen(m_pCurrentOutputBuffer->GetSDLWindow(), 0) != 0)
            {
                Log_ErrorPrintf("OpenGLGPUContext::SetExclusiveFullScreen: SDL_SetWindowFullscreen failed: %s", SDL_GetError());
                return false;
            }
        }

        // get window size
        int windowWidth, windowHeight;
        SDL_GetWindowSize(m_pCurrentOutputBuffer->GetSDLWindow(), &windowWidth, &windowHeight);
        m_pCurrentOutputBuffer->Resize(windowWidth, windowHeight);
        return true;
    }
}

bool OpenGLGPUContext::ResizeOutputBuffer(uint32 width /* = 0 */, uint32 height /* = 0 */)
{
    // GL buffers are automatically resized
    m_pCurrentOutputBuffer->Resize(width, height);
    return true;
}

void OpenGLGPUContext::PresentOutputBuffer(GPU_PRESENT_BEHAVIOUR presentBehaviour)
{
    // @TODO: Present Behaviour
    SDL_GL_SwapWindow(m_pCurrentOutputBuffer->GetSDLWindow());
}

void OpenGLGPUContext::BeginFrame()
{

}

void OpenGLGPUContext::Flush()
{
    glFlush();
}

void OpenGLGPUContext::Finish()
{
    glFinish();
}

uint32 OpenGLGPUContext::GetRenderTargets(uint32 nRenderTargets, GPURenderTargetView **ppRenderTargetViews, GPUDepthStencilBufferView **ppDepthBufferView)
{
    uint32 i;

    for (i = 0; i < m_nCurrentRenderTargets && i < nRenderTargets; i++)
        ppRenderTargetViews[i] = m_pCurrentRenderTargets[i];

    if (ppDepthBufferView != nullptr)
        *ppDepthBufferView = m_pCurrentDepthStencilBuffer;

    return i;
}

static void BindRTVToFramebufferColorAttachment(GLenum frameBufferTarget, GLenum frameBufferAttachment, OpenGLGPURenderTargetView *pRTV)
{
    switch (pRTV->GetTextureType())
    {
    case TEXTURE_TYPE_1D:
        glFramebufferTexture1D(frameBufferTarget, frameBufferAttachment, GL_TEXTURE_1D, pRTV->GetTextureName(), pRTV->GetDesc()->MipLevel);
        break;

    case TEXTURE_TYPE_1D_ARRAY:
        (pRTV->IsMultiLayer()) ? glFramebufferTexture1D(frameBufferTarget, frameBufferAttachment, GL_TEXTURE_1D, pRTV->GetTextureName(), pRTV->GetDesc()->MipLevel) :
                                  glFramebufferTextureLayer(frameBufferTarget, frameBufferAttachment, pRTV->GetTextureName(), pRTV->GetDesc()->MipLevel, pRTV->GetDesc()->FirstLayerIndex);
        break;

    case TEXTURE_TYPE_2D:
        glFramebufferTexture2D(frameBufferTarget, frameBufferAttachment, GL_TEXTURE_2D, pRTV->GetTextureName(), pRTV->GetDesc()->MipLevel);
        break;

    case TEXTURE_TYPE_2D_ARRAY:
        (pRTV->IsMultiLayer()) ? glFramebufferTexture2D(frameBufferTarget, frameBufferAttachment, GL_TEXTURE_2D, pRTV->GetTextureName(), pRTV->GetDesc()->MipLevel) :
                                  glFramebufferTextureLayer(frameBufferTarget, frameBufferAttachment, pRTV->GetTextureName(), pRTV->GetDesc()->MipLevel, pRTV->GetDesc()->FirstLayerIndex);
        break;

    case TEXTURE_TYPE_3D:
        glFramebufferTexture3D(frameBufferTarget, frameBufferAttachment, GL_TEXTURE_3D, pRTV->GetTextureName(), pRTV->GetDesc()->MipLevel, pRTV->GetDesc()->FirstLayerIndex);
        break;

    case TEXTURE_TYPE_CUBE:
        (pRTV->IsMultiLayer()) ? glFramebufferTexture(frameBufferTarget, frameBufferAttachment, pRTV->GetTextureName(), pRTV->GetDesc()->MipLevel) :
                                  glFramebufferTextureLayer(frameBufferTarget, frameBufferAttachment, pRTV->GetTextureName(), pRTV->GetDesc()->MipLevel, pRTV->GetDesc()->FirstLayerIndex);
        break;

    case TEXTURE_TYPE_CUBE_ARRAY:
        (pRTV->IsMultiLayer()) ? glFramebufferTexture(frameBufferTarget, frameBufferAttachment, pRTV->GetTextureName(), pRTV->GetDesc()->MipLevel) :
                                  glFramebufferTextureLayer(frameBufferTarget, frameBufferAttachment, pRTV->GetTextureName(), pRTV->GetDesc()->MipLevel, pRTV->GetDesc()->FirstLayerIndex);
        break;
    }
}

static void BindDSVToFramebufferDepthAttachment(GLenum frameBufferTarget, OpenGLGPUDepthStencilBufferView *pDSV)
{
    switch (pDSV->GetTextureType())
    {
    case TEXTURE_TYPE_2D:
        glFramebufferTexture2D(frameBufferTarget, pDSV->GetAttachmentPoint(), GL_TEXTURE_2D, pDSV->GetTextureName(), pDSV->GetDesc()->MipLevel);
        break;

    case TEXTURE_TYPE_2D_ARRAY:
        (pDSV->IsMultiLayer()) ? glFramebufferTexture2D(frameBufferTarget, pDSV->GetAttachmentPoint(), GL_TEXTURE_2D, pDSV->GetTextureName(), pDSV->GetDesc()->MipLevel) :
                                  glFramebufferTextureLayer(frameBufferTarget, pDSV->GetAttachmentPoint(), pDSV->GetTextureName(), pDSV->GetDesc()->MipLevel, pDSV->GetDesc()->FirstLayerIndex);
        break;

    case TEXTURE_TYPE_DEPTH:
        glFramebufferRenderbuffer(frameBufferTarget, pDSV->GetAttachmentPoint(), GL_RENDERBUFFER, pDSV->GetTextureName());
        break;
    }
}

void OpenGLGPUContext::SetRenderTargets(uint32 nRenderTargets, GPURenderTargetView **ppRenderTargets, GPUDepthStencilBufferView *pDepthBufferView)
{
    DebugAssert(nRenderTargets < GPU_MAX_SIMULTANEOUS_RENDER_TARGETS);

    // switch to swap chain?
    if ((nRenderTargets == 0 || (nRenderTargets == 1 && ppRenderTargets[0] == nullptr)) && pDepthBufferView == nullptr)
    {
        // currently using FBO?
        if (m_nCurrentRenderTargets > 0 || m_pCurrentDepthStencilBuffer != nullptr)
        {
            for (uint32 i = 0; i < m_nCurrentRenderTargets; i++)
            {
                if (m_pCurrentRenderTargets[i] != nullptr)
                {
                    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, 0, 0, 0);
                    m_pCurrentRenderTargets[i]->Release();
                    m_pCurrentRenderTargets[i] = nullptr;
                }
            }
            m_nCurrentRenderTargets = 0;

            if (m_pCurrentDepthStencilBuffer != nullptr)
            {
                glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
                glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);

                m_pCurrentDepthStencilBuffer->Release();
                m_pCurrentDepthStencilBuffer = nullptr;
            }
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        }

        // disable srgb writes if the framebuffer is not srgb.. seems to be even though we didn't specify it :S
        if (!PixelFormatHelpers::IsSRGBFormat(m_pCurrentOutputBuffer->GetBackBufferFormat()))
            glDisable(GL_FRAMEBUFFER_SRGB);

        // rest of the path is unnecessary
        return;
    }

    // fbo bound?
    if (m_nCurrentRenderTargets == 0 && m_pCurrentDepthStencilBuffer == nullptr)
    {
        // set fbo
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_drawFrameBufferObjectId);

        // if the system framebuffer was not srgb, enable srgb writes
        if (!PixelFormatHelpers::IsSRGBFormat(m_pCurrentOutputBuffer->GetBackBufferFormat()))
            glEnable(GL_FRAMEBUFFER_SRGB);
    }

    // bind buffers
    for (uint32 i = 0; i < nRenderTargets; i++)
    {
        if (m_pCurrentRenderTargets[i] != ppRenderTargets[i])
        {
            OpenGLGPURenderTargetView *pOldRenderTarget = m_pCurrentRenderTargets[i];
            if ((m_pCurrentRenderTargets[i] = static_cast<OpenGLGPURenderTargetView *>(ppRenderTargets[i])) != nullptr)
            {
                m_pCurrentRenderTargets[i]->AddRef();
                BindRTVToFramebufferColorAttachment(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, m_pCurrentRenderTargets[i]);
                if (pOldRenderTarget != nullptr)
                    pOldRenderTarget->Release();
            }
            else if (pOldRenderTarget != nullptr)
            {
                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, 0, 0, 0);
                pOldRenderTarget->Release();
            }
        }
    }

    // unbind remaining buffers
    for (uint32 i = nRenderTargets; i < m_nCurrentRenderTargets; i++)
    {
        if (m_pCurrentRenderTargets[i] != nullptr)
        {
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, 0, 0, 0);
            m_pCurrentRenderTargets[i]->Release();
            m_pCurrentRenderTargets[i] = nullptr;
        }
    }
    m_nCurrentRenderTargets = nRenderTargets;

    if (m_pCurrentDepthStencilBuffer != pDepthBufferView)
    {
        OpenGLGPUDepthStencilBufferView *pOldDepthStencilBuffer = m_pCurrentDepthStencilBuffer;
        if ((m_pCurrentDepthStencilBuffer = static_cast<OpenGLGPUDepthStencilBufferView *>(pDepthBufferView)) != nullptr)
        {
            m_pCurrentDepthStencilBuffer->AddRef();
            BindDSVToFramebufferDepthAttachment(GL_DRAW_FRAMEBUFFER, m_pCurrentDepthStencilBuffer);

            // unbind before bind if attachment changes?
            if (pOldDepthStencilBuffer != nullptr)
                pOldDepthStencilBuffer->Release();
        }
        else if (pOldDepthStencilBuffer != nullptr)
        {
            glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, pOldDepthStencilBuffer->GetAttachmentPoint(), GL_RENDERBUFFER, 0);
            pOldDepthStencilBuffer->Release();
        }        
    }

    // check fbo completeness
    //GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    //DebugAssert(status == GL_FRAMEBUFFER_COMPLETE);
    DebugAssert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // set draw buffers
    static const GLenum drawBuffers[GPU_MAX_SIMULTANEOUS_RENDER_TARGETS] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3,
                                                                             GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 };
    
    glDrawBuffers(nRenderTargets, (nRenderTargets > 0) ? drawBuffers : NULL);

    // set state
    m_nCurrentRenderTargets = nRenderTargets;
}

DRAW_TOPOLOGY OpenGLGPUContext::GetDrawTopology()
{
    return m_drawTopology;
}

void OpenGLGPUContext::SetDrawTopology(DRAW_TOPOLOGY topology)
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

uint32 OpenGLGPUContext::GetVertexBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer **ppVertexBuffers, uint32 *pVertexBufferOffsets, uint32 *pVertexBufferStrides)
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

void OpenGLGPUContext::SetVertexBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer *const *ppVertexBuffers, const uint32 *pVertexBufferOffsets, const uint32 *pVertexBufferStrides)
{
    DebugAssert(firstBuffer + nBuffers < m_currentVertexBuffers.GetSize());

    // update each buffer
    bool updateDirtyFlag = false;
    for (uint32 i = 0; i < nBuffers; i++)
    {
        uint32 bufferIndex = firstBuffer + i;
        VertexBufferBinding *vbBinding = &m_currentVertexBuffers[bufferIndex];
        OpenGLGPUBuffer *pBuffer = static_cast<OpenGLGPUBuffer *>(ppVertexBuffers[i]);
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

void OpenGLGPUContext::SetVertexBuffer(uint32 bufferIndex, GPUBuffer *pVertexBuffer, uint32 offset, uint32 stride)
{
    DebugAssert(bufferIndex < m_currentVertexBuffers.GetSize());

    VertexBufferBinding *vbBinding = &m_currentVertexBuffers[bufferIndex];
    OpenGLGPUBuffer *pBuffer = static_cast<OpenGLGPUBuffer *>(pVertexBuffer);

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

void OpenGLGPUContext::SetShaderVertexAttributes(const GPU_VERTEX_ELEMENT_DESC *pAttributeDescriptors, uint32 nAttributes)
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
        OpenGLTypeConversion::GetOpenGLVAOTypeAndSizeForVertexElementType(pAttributeDesc->Type, &integerType, &glType, &glSize, &glNormalized);

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

void OpenGLGPUContext::CommitVertexAttributes()
{
    if (!(m_dirtyVertexAttributes | m_dirtyVertexBuffers))
        return;

    // using vertex formats?
    if (GLAD_GL_ARB_vertex_attrib_binding)
    {
        // fast path, handle vertex buffers and attributes separately
        if (m_dirtyVertexAttributes)
        {
            // fix up changed vertex attributes
            uint32 nActiveAttributes = 0;
            for (uint32 attributeIndex = 0; attributeIndex < m_activeVertexAttributes; attributeIndex++)
            {
                VertexAttributeBinding *pAttributeBinding = &m_currentVertexAttributes[attributeIndex];
                if (!pAttributeBinding->Dirty)
                {
                    if (pAttributeBinding->Initialized)
                        nActiveAttributes = attributeIndex + 1;

                    continue;
                }

                if (pAttributeBinding->Initialized)
                {
                    if (pAttributeBinding->IntegerFormat)
                        glVertexAttribIFormat(attributeIndex, pAttributeBinding->Size, pAttributeBinding->Type, pAttributeBinding->Offset);
                    else
                        glVertexAttribFormat(attributeIndex, pAttributeBinding->Size, pAttributeBinding->Type, pAttributeBinding->Normalized, pAttributeBinding->Offset);

                    glVertexAttribDivisor(attributeIndex, pAttributeBinding->Divisor);
                    glVertexAttribBinding(attributeIndex, pAttributeBinding->VertexBufferIndex);

                    // determine whether it should be enabled
                    bool enabledState = (m_currentVertexBuffers[pAttributeBinding->VertexBufferIndex].pVertexBuffer != nullptr);
                    if (pAttributeBinding->Enabled != enabledState)
                    {
                        pAttributeBinding->Enabled = enabledState;
                        if (enabledState)
                        {
                            // enable attribute
                            glEnableVertexAttribArray(attributeIndex);
                        }
                        else
                        {
                            // disable it and warn
                            //Log_WarningPrintf("OpenGLGPUContext::CommitVertexAttributes: Attribute %u is referencing vertex buffer %u with no buffer bound. Disabling attribute.", attributeIndex, pAttributeBinding->VertexBufferIndex);
                            glDisableVertexAttribArray(attributeIndex);
                        }
                    }

                    // update max index
                    nActiveAttributes = attributeIndex + 1;
                }
                else
                {
                    // disable now-non-existant vertex attributes
                    if (pAttributeBinding->Enabled)
                    {
                        glDisableVertexAttribArray(attributeIndex);
                        pAttributeBinding->Enabled = false;
                    }
                }

                // clear dirty flag
                pAttributeBinding->Dirty = false;
            }

            // update stored max index
            m_activeVertexAttributes = nActiveAttributes;
            m_dirtyVertexAttributes = false;
        }

        // handle buffer changes
        if (m_dirtyVertexBuffers)
        {
            // vertex buffers changed: TODO USE MULTI BIND HERE?

            // find dirty vertex buffers
            for (uint32 vertexBufferIndex = 0; vertexBufferIndex < m_activeVertexBuffers; vertexBufferIndex++)
            {
                VertexBufferBinding *pVertexBufferBinding = &m_currentVertexBuffers[vertexBufferIndex];
                if (!pVertexBufferBinding->Dirty)
                    continue;

                // have a buffer?
                if (pVertexBufferBinding->pVertexBuffer != nullptr)
                {
                    // change the pointer
                    glBindVertexBuffer(vertexBufferIndex, pVertexBufferBinding->pVertexBuffer->GetGLBufferId(), pVertexBufferBinding->Offset, pVertexBufferBinding->Stride);
                }
                else
                {
                    // if enabled, disable it
                    glBindVertexBuffer(vertexBufferIndex, 0, 0, 0);
                }

                // remove dirty flag
                pVertexBufferBinding->Dirty = false;
            }

            // disable attributes that match an unbound vertex buffer, the nvidia gl driver at least crashes on draw if an unbound vertex buffer isn't disabled
            for (uint32 attributeIndex = 0; attributeIndex < m_activeVertexAttributes; attributeIndex++)
            {
                VertexAttributeBinding *pAttributeBinding = &m_currentVertexAttributes[attributeIndex];
                if (!pAttributeBinding->Initialized)
                    continue;

                VertexBufferBinding *pVertexBufferBinding = &m_currentVertexBuffers[pAttributeBinding->VertexBufferIndex];
                bool enabledState = (pVertexBufferBinding->pVertexBuffer != nullptr);
                if (pAttributeBinding->Enabled != enabledState)
                {
                    pAttributeBinding->Enabled = enabledState;
                    if (enabledState)
                    {
                        // enable attribute
                        glEnableVertexAttribArray(attributeIndex);
                    }
                    else
                    {
                        // disable it and warn
                        //Log_WarningPrintf("OpenGLGPUContext::CommitVertexAttributes: Attribute %u is referencing vertex buffer %u with no buffer bound. Disabling attribute.", attributeIndex, pAttributeBinding->VertexBufferIndex);
                        glDisableVertexAttribArray(attributeIndex);
                    }
                }
            }

            // or globally
            m_dirtyVertexBuffers = false;
        }
    }
    else
    {
        // slow path

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
    }

    // and the global flags
    m_dirtyVertexAttributes = false;
    m_dirtyVertexBuffers = false;
}

void OpenGLGPUContext::GetIndexBuffer(GPUBuffer **ppBuffer, GPU_INDEX_FORMAT *pFormat, uint32 *pOffset)
{
    *ppBuffer = m_pCurrentIndexBuffer;
    *pFormat = m_currentIndexFormat;
    *pOffset = m_currentIndexBufferOffset;
}

void OpenGLGPUContext::SetIndexBuffer(GPUBuffer *pBuffer, GPU_INDEX_FORMAT format, uint32 offset)
{
    if (m_pCurrentIndexBuffer == pBuffer && m_currentIndexFormat == format && m_currentIndexBufferOffset == offset)
        return;

    GPUBuffer *pOldIndexBuffer = m_pCurrentIndexBuffer;
    if ((m_pCurrentIndexBuffer = static_cast<OpenGLGPUBuffer *>(pBuffer)) != NULL)
    {
        m_pCurrentIndexBuffer->AddRef();
        m_currentIndexFormat = format;

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

void OpenGLGPUContext::SetShaderProgram(GPUShaderProgram *pShaderProgram)
{
    if (m_pCurrentShaderProgram == pShaderProgram)
        return;

    OpenGLGPUShaderProgram *pGLShaderProgram = static_cast<OpenGLGPUShaderProgram *>(pShaderProgram);
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

OpenGLGPUBuffer *OpenGLGPUContext::GetConstantBuffer(uint32 index)
{
    ConstantBuffer *constantBuffer = &m_constantBuffers[index];
    if (constantBuffer->Size == 0)
        return nullptr;

    if (constantBuffer->pGPUBuffer == nullptr)
    {
        // lazy create?
        if (constantBuffer->pLocalMemory == nullptr)
            constantBuffer->pLocalMemory = new byte[constantBuffer->Size];

        // create the gpu buffer
        GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_CONSTANT_BUFFER | GPU_BUFFER_FLAG_WRITABLE, constantBuffer->Size);
        constantBuffer->pGPUBuffer = static_cast<OpenGLGPUBuffer *>(m_pDevice->CreateBuffer(&bufferDesc, constantBuffer->pLocalMemory));
        if (constantBuffer->pGPUBuffer == nullptr)
        {
            const ShaderConstantBuffer *declaration = ShaderConstantBuffer::GetRegistry()->GetTypeInfoByIndex(index);
            Log_ErrorPrintf("OpenGLGPUContext::GetConstantBuffer: Failed to lazy-allocate constant buffer %u (%s)", index, declaration->GetBufferName());
            return nullptr;
        }
    }

    return constantBuffer->pGPUBuffer;
}

bool OpenGLGPUContext::CreateConstantBuffers()
{
    uint32 memoryUsage = 0;
    uint32 activeCount = 0;

    // allocate constant buffer storage
    const ShaderConstantBuffer::RegistryType *registry = ShaderConstantBuffer::GetRegistry();
    m_constantBuffers.Resize(registry->GetNumTypes());
    for (uint32 i = 0; i < m_constantBuffers.GetSize(); i++)
    {
        ConstantBuffer *constantBuffer = &m_constantBuffers[i];
        constantBuffer->pGPUBuffer = nullptr;
        constantBuffer->Size = 0;
        constantBuffer->pLocalMemory = nullptr;
        constantBuffer->DirtyLowerBounds = constantBuffer->DirtyUpperBounds = -1;

        // applicable to us?
        const ShaderConstantBuffer *declaration = registry->GetTypeInfoByIndex(i);
        if (declaration == nullptr)
            continue;
        if (declaration->GetPlatformRequirement() != RENDERER_PLATFORM_COUNT && declaration->GetPlatformRequirement() != RENDERER_PLATFORM_OPENGL)
            continue;
        if (declaration->GetMinimumFeatureLevel() != RENDERER_FEATURE_LEVEL_COUNT && declaration->GetMinimumFeatureLevel() > OpenGLRenderBackend::GetInstance()->GetFeatureLevel())
            continue;

        // set size so we know to allocate it later or on demand
        constantBuffer->Size = declaration->GetBufferSize();

        // stats
        memoryUsage += constantBuffer->Size;
        activeCount++;
    }

    // preallocate constant buffers, todo lazy allocation
    Log_DevPrintf("Preallocating %u constant buffers, total VRAM usage: %u bytes", activeCount, memoryUsage);
    for (uint32 i = 0; i < m_constantBuffers.GetSize(); i++)
    {
        ConstantBuffer *constantBuffer = &m_constantBuffers[i];
        if (constantBuffer->Size == 0)
            continue;

        // allocate local memory first (so we can use it as initialization data)
        constantBuffer->pLocalMemory = new byte[constantBuffer->Size];
        Y_memzero(constantBuffer->pLocalMemory, constantBuffer->Size);

        // create the gpu buffer
        GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_CONSTANT_BUFFER | GPU_BUFFER_FLAG_WRITABLE, constantBuffer->Size);
        constantBuffer->pGPUBuffer = static_cast<OpenGLGPUBuffer *>(m_pDevice->CreateBuffer(&bufferDesc, constantBuffer->pLocalMemory));
        if (constantBuffer->pGPUBuffer == nullptr)
        {
            const ShaderConstantBuffer *declaration = ShaderConstantBuffer::GetRegistry()->GetTypeInfoByIndex(i);
            Log_ErrorPrintf("OpenGLGPUContext::CreateResources: Failed to allocate constant buffer %u (%s)", i, declaration->GetBufferName());
            return false;
        }
    }

    return true;
}

void OpenGLGPUContext::WriteConstantBuffer(uint32 bufferIndex, uint32 fieldIndex, uint32 offset, uint32 count, const void *pData, bool commit)
{
    ConstantBuffer *constantBuffer = &m_constantBuffers[bufferIndex];
    if (constantBuffer->pGPUBuffer == nullptr)
    {
        // buffer non-existant and creation has previously failed?
        if (constantBuffer->pLocalMemory == nullptr)
            return;
    }

    // changed?
    DebugAssert(count > 0 && (offset + count) <= constantBuffer->Size);
    if (Y_memcmp(constantBuffer->pLocalMemory + offset, pData, count) != 0)
    {
        // write it
        Y_memcpy(constantBuffer->pLocalMemory + offset, pData, count);

        // update dirty pointer
        if (constantBuffer->DirtyLowerBounds < 0)
        {
            constantBuffer->DirtyLowerBounds = (int32)offset;
            constantBuffer->DirtyUpperBounds = (int32)(offset + count);
        }
        else
        {
            constantBuffer->DirtyLowerBounds = Min(constantBuffer->DirtyLowerBounds, (int32)offset);
            constantBuffer->DirtyUpperBounds = Max(constantBuffer->DirtyUpperBounds, (int32)(offset + count - 1));
        }

        // commit buffer?
        if (commit)
            OpenGLGPUContext::CommitConstantBuffer(bufferIndex);
    }
}

void OpenGLGPUContext::WriteConstantBufferStrided(uint32 bufferIndex, uint32 fieldIndex, uint32 offset, uint32 bufferStride, uint32 copySize, uint32 count, const void *pData, bool commit)
{
    ConstantBuffer *constantBuffer = &m_constantBuffers[bufferIndex];
    if (constantBuffer->pGPUBuffer == nullptr)
    {
        // buffer non-existant and creation has previously failed?
        if (constantBuffer->pLocalMemory == nullptr)
            return;
    }

    // changed?
    DebugAssert(count > 0 && (offset + count) <= constantBuffer->Size);
    if (Y_memcmp_stride(constantBuffer->pLocalMemory + offset, bufferStride, pData, copySize, copySize, count) != 0)
    {
        // write it
        Y_memcpy_stride(constantBuffer->pLocalMemory + offset, bufferStride, pData, copySize, copySize, count);

        // update dirty pointer
        uint32 writtenCount = bufferStride * count;
        if (constantBuffer->DirtyLowerBounds < 0)
        {
            constantBuffer->DirtyLowerBounds = (int32)offset;
            constantBuffer->DirtyUpperBounds = (int32)(offset + writtenCount);
        }
        else
        {
            constantBuffer->DirtyLowerBounds = Min(constantBuffer->DirtyLowerBounds, (int32)offset);
            constantBuffer->DirtyUpperBounds = Max(constantBuffer->DirtyUpperBounds, (int32)(offset + writtenCount - 1));
        }

        // commit buffer?
        if (commit)
            OpenGLGPUContext::CommitConstantBuffer(bufferIndex);
    }
}

void OpenGLGPUContext::CommitConstantBuffer(uint32 bufferIndex)
{
    ConstantBuffer *constantBuffer = &m_constantBuffers[bufferIndex];
    if (constantBuffer->DirtyLowerBounds < 0)
        return;

    DebugAssert(constantBuffer->pGPUBuffer != nullptr);
    WriteBuffer(constantBuffer->pGPUBuffer, constantBuffer->pLocalMemory + constantBuffer->DirtyLowerBounds, constantBuffer->DirtyLowerBounds, constantBuffer->DirtyUpperBounds - constantBuffer->DirtyLowerBounds);
    constantBuffer->DirtyLowerBounds = constantBuffer->DirtyUpperBounds = -1;
}

void OpenGLGPUContext::SetShaderUniformBlock(uint32 index, OpenGLGPUBuffer *pBuffer)
{
    if (m_currentUniformBlockBindings[index] == pBuffer)
        return;

#if DEFER_SHADER_STATE_CHANGES

    // update dirty range
    if (m_dirtyUniformBlockBindingsLowerBounds < 0)
    {
        m_dirtyUniformBlockBindingsLowerBounds = m_dirtyUniformBlockBindingsUpperBounds = (int32)index;
    }
    else
    {
        m_dirtyUniformBlockBindingsLowerBounds = Min(m_dirtyUniformBlockBindingsLowerBounds, (int32)index);
        m_dirtyUniformBlockBindingsUpperBounds = Max(m_dirtyUniformBlockBindingsUpperBounds, (int32)index);
    }

#else

    // bind new buffer
    if (pBuffer != nullptr)
        glBindBufferRange(GL_UNIFORM_BUFFER, index, pBuffer->GetGLBufferId(), 0, pBuffer->GetDesc()->Size);
    else
        glBindBufferRange(GL_UNIFORM_BUFFER, index, 0, 0, 0);

#endif

    // update references
    if (m_currentUniformBlockBindings[index] != nullptr)
        m_currentUniformBlockBindings[index]->Release();
    if ((m_currentUniformBlockBindings[index] = pBuffer) != nullptr)
        pBuffer->AddRef();

    // update counters
    if (pBuffer != nullptr)
    {
        if (index >= m_activeUniformBlockBindings)
            m_activeUniformBlockBindings = index + 1;
    }
    else
    {
        if ((index + 1) == m_activeUniformBlockBindings)
        {
            uint32 lastPos = 0;
            for (uint32 i = 0; i < m_activeUniformBlockBindings; i++)
            {
                if (m_currentUniformBlockBindings[i] != nullptr)
                    lastPos = i + 1;
            }
            m_activeUniformBlockBindings = lastPos;
        }
    }
}

void OpenGLGPUContext::SetShaderTextureUnit(uint32 index, GPUTexture *pTexture, OpenGLGPUSamplerState *pSamplerState)
{
    TextureUnitBinding *texUnitState = &m_currentTextureUnitBindings[index];

#if DEFER_SHADER_STATE_CHANGES

    if (texUnitState->pTexture == pTexture && texUnitState->pSampler == pSamplerState)
        return;

    if (texUnitState->pTexture != pTexture)
    {
        // release the old (if any) texture reference
        if (texUnitState->pTexture != nullptr)
            texUnitState->pTexture->Release();
        if ((texUnitState->pTexture = pTexture) != nullptr)
            texUnitState->pTexture->AddRef();
    }

    if (texUnitState->pSampler != pSamplerState)
    {
        if (texUnitState->pSampler != nullptr)
            texUnitState->pSampler->Release();

        if ((texUnitState->pSampler = pSamplerState) != nullptr)
            texUnitState->pSampler->AddRef();
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
    if (texUnitState->pTexture != pTexture)
    {
        // alter the binding
        glActiveTexture(GL_TEXTURE0 + index);
        OpenGLHelpers::BindOpenGLTexture(pTexture);
        glActiveTexture(GL_TEXTURE0);


    }

    // sampler change?
    if (texUnitState->pSampler != pSamplerState)
    {
        // bind the sampler state override if provided
        if (pSamplerState != nullptr)
        {
            glBindSampler(index, pSamplerState->GetGLSamplerID());

            if (texUnitState->pSampler != nullptr)
                texUnitState->pSampler->Release();

            texUnitState->pSampler = pSamplerState;
            texUnitState->pSampler->AddRef();
        }
        else if (texUnitState->pSampler != nullptr)
        {
            glBindSampler(index, 0);
            texUnitState->pSampler->Release();
            texUnitState->pSampler = nullptr;
        }
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
                if (m_currentTextureUnitBindings[i].pTexture != nullptr)
                    lastPos = i + 1;
            }
            m_activeTextureUnitBindings = lastPos;
        }
    }
}

void OpenGLGPUContext::SetShaderImageUnit(uint32 index, GPUTexture *pTexture)
{
    if (m_currentImageUnitBindings[index] == pTexture)
        return;

    // update dirty range
    if (m_dirtyImageUnitsLowerBounds < 0)
    {
        m_dirtyImageUnitsLowerBounds = m_dirtyTextureUnitsUpperBounds = (int32)index;
    }
    else
    {
        m_dirtyTextureUnitsLowerBounds = Min(m_dirtyTextureUnitsLowerBounds, (int32)index);
        m_dirtyTextureUnitsUpperBounds = Max(m_dirtyTextureUnitsUpperBounds, (int32)index);
    }

    // update references
    if (m_currentImageUnitBindings[index] != nullptr)
        m_currentImageUnitBindings[index]->Release();
    if ((m_currentImageUnitBindings[index] = pTexture) != nullptr)
        pTexture->AddRef();

    // update counters
    if (pTexture != nullptr)
    {
        if (index >= m_activeImageUnitBindings)
            m_activeImageUnitBindings = index + 1;
    }
    else
    {
        if ((index + 1) == m_activeImageUnitBindings)
        {
            uint32 lastPos = 0;
            for (uint32 i = 0; i < m_activeImageUnitBindings; i++)
            {
                if (m_currentImageUnitBindings[i] != nullptr)
                    lastPos = i + 1;
            }
            m_activeImageUnitBindings = lastPos;
        }
    }
}

void OpenGLGPUContext::SetShaderStorageBuffer(uint32 index, GPUResource *pResource)
{
    if (m_currentShaderStorageBufferBindings[index] == pResource)
        return;

#if DEFER_SHADER_STATE_CHANGES

    // update dirty range
    if (m_dirtyShaderStorageBuffersLowerBounds < 0)
    {
        m_dirtyShaderStorageBuffersLowerBounds = m_dirtyShaderStorageBuffersUpperBounds = (int32)index;
    }
    else
    {
        m_dirtyShaderStorageBuffersLowerBounds = Min(m_dirtyShaderStorageBuffersLowerBounds, (int32)index);
        m_dirtyShaderStorageBuffersUpperBounds = Max(m_dirtyShaderStorageBuffersUpperBounds, (int32)index);
    }

#else

    // bind new buffer
    if (pResource != nullptr)
    {
        switch (pResource->GetResourceType())
        {
        case GPU_RESOURCE_TYPE_BUFFER:
            glBindBufferRange(GL_SHADER_STORAGE_BUFFER, index, static_cast<OpenGLGPUBuffer *>(pResource)->GetGLBufferId(), 0, static_cast<OpenGLGPUBuffer *>(pResource)->GetDesc()->Size);
            break;

        default:
            glBindBufferRange(GL_SHADER_STORAGE_BUFFER, index, 0, 0, 0);
            break;
        }
    }
    else
    {
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, index, 0, 0, 0);
    }

#endif

    // update references
    if (m_currentShaderStorageBufferBindings[index] != nullptr)
        m_currentShaderStorageBufferBindings[index]->Release();
    if ((m_currentShaderStorageBufferBindings[index] = pResource) != nullptr)
        pResource->AddRef();

    // update counters
    if (pResource != nullptr)
    {
        if (index >= m_activeShaderStorageBufferBindings)
            m_activeShaderStorageBufferBindings = index + 1;
    }
    else
    {
        if ((index + 1) == m_activeShaderStorageBufferBindings)
        {
            uint32 lastPos = 0;
            for (uint32 i = 0; i < m_activeShaderStorageBufferBindings; i++)
            {
                if (m_currentShaderStorageBufferBindings[i] != nullptr)
                    lastPos = i + 1;
            }
            m_activeShaderStorageBufferBindings = lastPos;
        }
    }
}

void OpenGLGPUContext::CommitShaderResources()
{
    if (GLAD_GL_ARB_multi_bind)
    {
        // uniform blocks
        if (m_dirtyUniformBlockBindingsLowerBounds >= 0)
        {
            uint32 countToBind = (uint32)(m_dirtyUniformBlockBindingsUpperBounds - m_dirtyUniformBlockBindingsLowerBounds + 1);
            GLuint *pBufferIDs = (GLuint *)alloca(sizeof(GLuint) * countToBind);
            GLintptr *pBufferOffsets = (GLintptr *)alloca(sizeof(GLintptr) * countToBind);
            GLsizeiptr *pBufferSizes = (GLsizeiptr *)alloca(sizeof(GLsizeiptr) * countToBind);
            for (uint32 i = 0; i < countToBind; i++)
            {
                OpenGLGPUBuffer *pBuffer = m_currentUniformBlockBindings[m_dirtyUniformBlockBindingsLowerBounds + i];
                if (pBuffer != nullptr)
                {
                    pBufferIDs[i] = pBuffer->GetGLBufferId();
                    pBufferOffsets[i] = 0;
                    pBufferSizes[i] = pBuffer->GetDesc()->Size;
                }
                else
                {
                    // ugh. this seems to be picky, the size has to be >0, even if the buffer is null...
                    //pBufferIDs[i] = 0;
                    //pBufferOffsets[i] = 0;
                    //pBufferSizes[i] = 0;

                    // for now, stuff it, we'll just do the slow path if we hit a null
                    for (uint32 j = 0; j < countToBind; j++)
                    {
                        OpenGLGPUBuffer *pInnerBuffer = m_currentUniformBlockBindings[m_dirtyUniformBlockBindingsLowerBounds + j];
                        if (pInnerBuffer != nullptr)
                            glBindBufferRange(GL_UNIFORM_BUFFER, m_dirtyUniformBlockBindingsLowerBounds + j, pInnerBuffer->GetGLBufferId(), 0, pInnerBuffer->GetDesc()->Size);
                        else
                            glBindBufferRange(GL_UNIFORM_BUFFER, m_dirtyUniformBlockBindingsLowerBounds + j, 0, 0, 0);
                    }
                    countToBind = 0;
                }
            }

            // send them across
            if (countToBind > 0)
                glBindBuffersRange(GL_UNIFORM_BUFFER, m_dirtyUniformBlockBindingsLowerBounds, countToBind, pBufferIDs, pBufferOffsets, pBufferSizes);

            m_dirtyUniformBlockBindingsLowerBounds = m_dirtyUniformBlockBindingsUpperBounds = -1;
        }

        // texture units
        if (m_dirtyTextureUnitsLowerBounds >= 0)
        {
            uint32 countToBind = (uint32)(m_dirtyTextureUnitsUpperBounds - m_dirtyTextureUnitsLowerBounds + 1);
            GLuint *pTextureIDs = (GLuint *)alloca(sizeof(GLuint) * countToBind);
            GLuint *pSamplerIDs = (GLuint *)alloca(sizeof(GLuint) * countToBind);
            for (uint32 i = 0; i < countToBind; i++)
            {
                TextureUnitBinding *texUnitBinding = &m_currentTextureUnitBindings[m_dirtyTextureUnitsLowerBounds + i];

                pTextureIDs[i] = (texUnitBinding->pTexture != nullptr) ? OpenGLHelpers::GetOpenGLTextureId(texUnitBinding->pTexture) : 0;
                pSamplerIDs[i] = (texUnitBinding->pSampler != nullptr) ? texUnitBinding->pSampler->GetGLSamplerID() : 0;
            }

            // send them across
            glBindTextures(m_dirtyTextureUnitsLowerBounds, countToBind, pTextureIDs);
            glBindSamplers(m_dirtyTextureUnitsLowerBounds, countToBind, pSamplerIDs);
            m_dirtyTextureUnitsLowerBounds = m_dirtyTextureUnitsUpperBounds = -1;
        }

        // image units
        if (m_dirtyImageUnitsLowerBounds >= 0)
        {
            uint32 countToBind = (uint32)(m_dirtyImageUnitsUpperBounds - m_dirtyImageUnitsLowerBounds + 1);
            GLuint *pTextureIDs = (GLuint *)alloca(sizeof(GLuint) * countToBind);
            for (uint32 i = 0; i < countToBind; i++)
            {
                GPUTexture *pTexture = m_currentImageUnitBindings[i];
                if (pTexture != nullptr)
                    pTextureIDs[i] = OpenGLHelpers::GetOpenGLTextureId(pTexture);
                else
                    pTextureIDs[i] = 0;
            }

            // send them across -- fixme on this one...
            glBindImageTextures(m_dirtyImageUnitsLowerBounds, countToBind, pTextureIDs);
            m_dirtyImageUnitsLowerBounds = m_dirtyTextureUnitsUpperBounds = -1;
        }

        // shader storage blocks
        if (m_dirtyShaderStorageBuffersLowerBounds >= 0)
        {
            uint32 countToBind = (uint32)(m_dirtyShaderStorageBuffersUpperBounds - m_dirtyShaderStorageBuffersLowerBounds + 1);
            GLuint *pBufferIDs = (GLuint *)alloca(sizeof(GLuint) * countToBind);
            GLintptr *pBufferOffsets = (GLintptr *)alloca(sizeof(GLintptr) * countToBind);
            GLsizeiptr *pBufferSizes = (GLsizeiptr *)alloca(sizeof(GLsizeiptr) * countToBind);
            for (uint32 i = 0; i < countToBind; i++)
            {
                GPUResource *pResource = m_currentShaderStorageBufferBindings[m_dirtyShaderStorageBuffersLowerBounds + i];

                switch ((pResource != nullptr) ? pResource->GetResourceType() : GPU_RESOURCE_TYPE_COUNT)
                {
                case GPU_RESOURCE_TYPE_BUFFER:
                    pBufferIDs[i] = static_cast<OpenGLGPUBuffer *>(pResource)->GetGLBufferId();
                    pBufferOffsets[i] = 0;
                    pBufferSizes[i] = static_cast<OpenGLGPUBuffer *>(pResource)->GetDesc()->Size;
                    break;

                default:
                    {
                        // ugh. this seems to be picky, the size has to be >0, even if the buffer is null...
                        //pBufferIDs[i] = 0;
                        //pBufferOffsets[i] = 0;
                        //pBufferSizes[i] = 0;

                        // for now, stuff it, we'll just do the slow path if we hit a null
                        for (uint32 j = 0; j < countToBind; j++)
                        {
                            GPUResource *pInnerResource = m_currentShaderStorageBufferBindings[m_dirtyShaderStorageBuffersLowerBounds + j];

                            switch ((pInnerResource != nullptr) ? pInnerResource->GetResourceType() : GPU_RESOURCE_TYPE_COUNT)
                            {
                            case GPU_RESOURCE_TYPE_BUFFER:
                                glBindBufferRange(GL_SHADER_STORAGE_BUFFER, m_dirtyShaderStorageBuffersLowerBounds + j, static_cast<OpenGLGPUBuffer *>(pInnerResource)->GetGLBufferId(), 0, static_cast<OpenGLGPUBuffer *>(pInnerResource)->GetDesc()->Size);
                                break;

                            default:
                                glBindBufferRange(GL_SHADER_STORAGE_BUFFER, m_dirtyShaderStorageBuffersLowerBounds + j, 0, 0, 0);
                                break;
                            }
                        }
                        countToBind = 0;
                    }
                    break;
                }
            }

            // send them across
            if (countToBind > 0)
                glBindBuffersRange(GL_SHADER_STORAGE_BUFFER, m_dirtyShaderStorageBuffersLowerBounds, countToBind, pBufferIDs, pBufferOffsets, pBufferSizes);

            m_dirtyShaderStorageBuffersLowerBounds = m_dirtyShaderStorageBuffersUpperBounds = -1;
        }
    }
    else
    {
        // uniform blocks
        if (m_dirtyUniformBlockBindingsLowerBounds >= 0)
        {
            uint32 countToBind = (uint32)(m_dirtyUniformBlockBindingsUpperBounds - m_dirtyUniformBlockBindingsLowerBounds + 1);
            for (uint32 i = 0; i < countToBind; i++)
            {
                OpenGLGPUBuffer *pBuffer = m_currentUniformBlockBindings[m_dirtyUniformBlockBindingsLowerBounds + i];
                if (pBuffer != nullptr)
                    glBindBufferRange(GL_UNIFORM_BUFFER, m_dirtyUniformBlockBindingsLowerBounds + i, pBuffer->GetGLBufferId(), 0, pBuffer->GetDesc()->Size);
                else
                    glBindBufferRange(GL_UNIFORM_BUFFER, m_dirtyUniformBlockBindingsLowerBounds + i, 0, 0, 0);
            }

            // reset dirty range
            m_dirtyUniformBlockBindingsLowerBounds = m_dirtyUniformBlockBindingsUpperBounds = -1;
        }

        // texture units
        if (m_dirtyTextureUnitsLowerBounds >= 0)
        {
            uint32 countToBind = (uint32)(m_dirtyTextureUnitsUpperBounds - m_dirtyTextureUnitsLowerBounds + 1);
            for (uint32 i = 0; i < countToBind; i++)
            {
                TextureUnitBinding *texUnitBinding = &m_currentTextureUnitBindings[m_dirtyTextureUnitsLowerBounds + i];
                glActiveTexture(GL_TEXTURE0 + m_dirtyTextureUnitsLowerBounds + i);
                OpenGLHelpers::BindOpenGLTexture(texUnitBinding->pTexture);
                glBindSampler(m_dirtyTextureUnitsLowerBounds + i, (texUnitBinding->pSampler != nullptr) ? texUnitBinding->pSampler->GetGLSamplerID() : 0);
            }
            glActiveTexture(GL_TEXTURE0);

            // reset dirty range
            m_dirtyTextureUnitsLowerBounds = m_dirtyTextureUnitsUpperBounds = -1;
        }

        // image units
        if (m_dirtyImageUnitsLowerBounds >= 0)
        {
            uint32 countToBind = (uint32)(m_dirtyImageUnitsUpperBounds - m_dirtyImageUnitsLowerBounds + 1);
            for (uint32 i = 0; i < countToBind; i++)
            {
                //GPUTexture *pTexture = m_currentImageUnitBindings[i];
                //glBindImageTexture(i, OpenGLHelpers::GetOpenGLTextureId(pTexture), 0, GL_FALSE, 0, 
            }

            // reset dirty range
            m_dirtyImageUnitsLowerBounds = m_dirtyTextureUnitsUpperBounds = -1;
        }

        // shader storage blocks
        if (m_dirtyShaderStorageBuffersLowerBounds >= 0)
        {
            uint32 countToBind = (uint32)(m_dirtyShaderStorageBuffersUpperBounds - m_dirtyShaderStorageBuffersLowerBounds + 1);
            for (uint32 i = 0; i < countToBind; i++)
            {
                GPUResource *pResource = m_currentShaderStorageBufferBindings[m_dirtyShaderStorageBuffersLowerBounds + i];

                switch ((pResource != nullptr) ? pResource->GetResourceType() : GPU_RESOURCE_TYPE_COUNT)
                {
                case GPU_RESOURCE_TYPE_BUFFER:
                    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, m_dirtyShaderStorageBuffersLowerBounds + i, static_cast<OpenGLGPUBuffer *>(pResource)->GetGLBufferId(), 0, static_cast<OpenGLGPUBuffer *>(pResource)->GetDesc()->Size);
                    break;

                default:
                    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, m_dirtyShaderStorageBuffersLowerBounds + i, 0, 0, 0);
                    break;
                }
            }

            // reset dirty range
            m_dirtyShaderStorageBuffersLowerBounds = m_dirtyShaderStorageBuffersUpperBounds = -1;
        }
    }
}

void OpenGLGPUContext::BindMutatorTextureUnit()
{
    glActiveTexture(GL_TEXTURE0 + m_mutatorTextureUnit);
}

void OpenGLGPUContext::RestoreMutatorTextureUnit()
{
    OpenGLHelpers::BindOpenGLTexture(m_currentTextureUnitBindings[m_mutatorTextureUnit].pTexture);
    glActiveTexture(GL_TEXTURE0);
}

void OpenGLGPUContext::Draw(uint32 firstVertex, uint32 nVertices)
{
    DebugAssert(m_pCurrentShaderProgram != nullptr);
    if (nVertices == 0)
        return;

    CommitVertexAttributes();

    m_pCurrentShaderProgram->CommitLocalConstantBuffers(this);
    CommitShaderResources();

    glDrawArrays(m_glDrawTopology, firstVertex, nVertices);
    //m_drawCallCounter++;
}

void OpenGLGPUContext::DrawInstanced(uint32 firstVertex, uint32 nVertices, uint32 nInstances)
{
    DebugAssert(m_pCurrentShaderProgram != nullptr);
    if (nVertices == 0 || nInstances == 0)
        return;

    CommitVertexAttributes();

    m_pCurrentShaderProgram->CommitLocalConstantBuffers(this);
    CommitShaderResources();

    glDrawArraysInstanced(m_glDrawTopology, firstVertex, nVertices, nInstances);
    //m_drawCallCounter++;
}

void OpenGLGPUContext::DrawIndexed(uint32 startIndex, uint32 nIndices, uint32 baseVertex)
{
    DebugAssert(m_pCurrentShaderProgram != nullptr);
    if (nIndices == 0)
        return;

    DebugAssert(m_pCurrentIndexBuffer != nullptr);
    CommitVertexAttributes();

    m_pCurrentShaderProgram->CommitLocalConstantBuffers(this);
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

    if (baseVertex == 0)
        glDrawElements(m_glDrawTopology, nIndices, arrayType, reinterpret_cast<GLvoid *>(indexBufferOffset));
    else
        glDrawElementsBaseVertex(m_glDrawTopology, nIndices, arrayType, reinterpret_cast<GLvoid *>(indexBufferOffset), baseVertex);

    //m_drawCallCounter++;
}

void OpenGLGPUContext::DrawIndexedInstanced(uint32 startIndex, uint32 nIndices, uint32 baseVertex, uint32 nInstances)
{
    DebugAssert(m_pCurrentShaderProgram != nullptr);
    if (nIndices == 0 || nInstances == 0)
        return;

    DebugAssert(m_pCurrentIndexBuffer != nullptr);
    CommitVertexAttributes();

    m_pCurrentShaderProgram->CommitLocalConstantBuffers(this);
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

    if (baseVertex == 0)
        glDrawElementsInstanced(m_glDrawTopology, nIndices, arrayType, reinterpret_cast<GLvoid *>(indexBufferOffset), nInstances);
    else
        glDrawElementsInstancedBaseVertex(m_glDrawTopology, nIndices, arrayType, reinterpret_cast<GLvoid *>(indexBufferOffset), nInstances, baseVertex);

    //m_drawCallCounter++;
}

void OpenGLGPUContext::Dispatch(uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ)
{
    DebugAssert(m_pCurrentShaderProgram != nullptr);
    m_pCurrentShaderProgram->CommitLocalConstantBuffers(this);
    CommitShaderResources();

    glDispatchCompute(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
}

void OpenGLGPUContext::DrawUserPointer(const void *pVertices, uint32 VertexSize, uint32 nVertices)
{
    uint32 maxVerticesPerPass = m_userVertexBufferSize / VertexSize;
    DebugAssert(m_pCurrentShaderProgram != nullptr);

    // obtain the current vertex buffer in slot zero, since we need to overwrite this
    OpenGLGPUBuffer *pRestoreVertexBuffer = m_currentVertexBuffers[0].pVertexBuffer;
    uint32 restoreVertexBufferOffset = m_currentVertexBuffers[0].Offset;
    uint32 restoreVertexBufferStride = m_currentVertexBuffers[0].Stride;
    if (pRestoreVertexBuffer != nullptr)
        pRestoreVertexBuffer->AddRef();

    // update uniforms
    m_pCurrentShaderProgram->CommitLocalConstantBuffers(this);
    CommitShaderResources();

    // draw vertices loop
    const byte *pCurrentVertexPointer = reinterpret_cast<const byte *>(pVertices);
    uint32 nRemainingVertices = nVertices;
    while (nRemainingVertices > 0)
    {
        byte *pWritePointer;
        uint32 nVerticesThisPass = Min(nRemainingVertices, maxVerticesPerPass);
        uint32 spaceRequired = VertexSize * nVerticesThisPass;

        if ((m_userVertexBufferPosition + spaceRequired) < m_userVertexBufferSize)
        {
            // we can fit into the remaining space
            if (!MapBuffer(m_pUserVertexBuffer, GPU_MAP_TYPE_WRITE_NO_OVERWRITE, reinterpret_cast<void **>(&pWritePointer)))
            {
                Log_ErrorPrintf("Failed to map plain vertex buffer.");
                return;
            }
        }
        else
        {
            // discard buffer
            m_userVertexBufferPosition = 0;
            if (!MapBuffer(m_pUserVertexBuffer, GPU_MAP_TYPE_WRITE_DISCARD, reinterpret_cast<void **>(&pWritePointer)))
            {
                Log_ErrorPrintf("Failed to map plain vertex buffer.");
                return;
            }
        }

        // write to it
        Y_memcpy(pWritePointer + m_userVertexBufferPosition, pCurrentVertexPointer, spaceRequired);

        // unmap and draw
        Unmapbuffer(m_pUserVertexBuffer, pWritePointer);

        // bind the vertex buffer
        OpenGLGPUContext::SetVertexBuffer(0, m_pUserVertexBuffer, m_userVertexBufferPosition, VertexSize);
        CommitVertexAttributes();

        // invoke draw
        glDrawArrays(m_glDrawTopology, 0, nVerticesThisPass);
        //m_drawCallCounter++;

        // increment pointers
        m_userVertexBufferPosition += spaceRequired;
        pCurrentVertexPointer += VertexSize * nVerticesThisPass;
        nRemainingVertices -= nVerticesThisPass;
    }

    // re-bind saved vertex buffer
    SetVertexBuffer(0, pRestoreVertexBuffer, restoreVertexBufferOffset, restoreVertexBufferStride);
    if (pRestoreVertexBuffer != nullptr)
        pRestoreVertexBuffer->Release();

    // leave the commit out here, next draw will fix it up anyway
    //CommitVertexAttributes();
}

bool OpenGLGPUContext::CopyTexture(GPUTexture2D *pSourceTexture, GPUTexture2D *pDestinationTexture)
{
    // textures have to be compatible, for now this means same texture format
    OpenGLGPUTexture2D *pOpenGLSourceTexture = static_cast<OpenGLGPUTexture2D *>(pSourceTexture);
    OpenGLGPUTexture2D *pOpenGLDestinationTexture = static_cast<OpenGLGPUTexture2D *>(pDestinationTexture);
    if (pOpenGLSourceTexture->GetDesc()->Width != pOpenGLDestinationTexture->GetDesc()->Width ||
        pOpenGLSourceTexture->GetDesc()->Height != pOpenGLDestinationTexture->GetDesc()->Height ||
        pOpenGLSourceTexture->GetDesc()->Format != pOpenGLDestinationTexture->GetDesc()->Format ||
        pOpenGLSourceTexture->GetDesc()->MipLevels != pOpenGLDestinationTexture->GetDesc()->MipLevels)
    {
        return false;
    }

    if (GLAD_GL_ARB_copy_image)
    {
        // preferred standard method
        for (uint32 level = 0; level < pOpenGLSourceTexture->GetDesc()->MipLevels; level++)
            glCopyImageSubData(pOpenGLSourceTexture->GetGLTextureId(), GL_TEXTURE_2D, level, 0, 0, 0, pOpenGLDestinationTexture->GetGLTextureId(), GL_TEXTURE_2D, level, 0, 0, 0, pOpenGLSourceTexture->GetDesc()->Width, pOpenGLSourceTexture->GetDesc()->Height, 1);
    }
    else if (GLAD_GL_NV_copy_image)
    {
        // fallback nvidia extension
        for (uint32 level = 0; level < pOpenGLSourceTexture->GetDesc()->MipLevels; level++)
            glCopyImageSubDataNV(pOpenGLSourceTexture->GetGLTextureId(), GL_TEXTURE_2D, level, 0, 0, 0, pOpenGLDestinationTexture->GetGLTextureId(), GL_TEXTURE_2D, level, 0, 0, 0, pOpenGLSourceTexture->GetDesc()->Width, pOpenGLSourceTexture->GetDesc()->Height, 1);
    }
    else
    {
        // use fbo
        BindMutatorTextureUnit();
        glBindTexture(GL_TEXTURE_2D, pOpenGLSourceTexture->GetGLTextureId());
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFrameBufferObjectId);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pOpenGLSourceTexture->GetGLTextureId(), 0);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, pOpenGLSourceTexture->GetDesc()->Width, pOpenGLSourceTexture->GetDesc()->Height);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        RestoreMutatorTextureUnit();
    }

    return true;
}

bool OpenGLGPUContext::CopyTextureRegion(GPUTexture2D *pSourceTexture, uint32 sourceX, uint32 sourceY, uint32 width, uint32 height, uint32 sourceMipLevel, GPUTexture2D *pDestinationTexture, uint32 destX, uint32 destY, uint32 destMipLevel)
{
    // textures have to be compatible, for now this means same texture format
    OpenGLGPUTexture2D *pOpenGLSourceTexture = static_cast<OpenGLGPUTexture2D *>(pSourceTexture);
    OpenGLGPUTexture2D *pOpenGLDestinationTexture = static_cast<OpenGLGPUTexture2D *>(pDestinationTexture);
    if (pOpenGLSourceTexture->GetDesc()->Format != pOpenGLDestinationTexture->GetDesc()->Format ||
        pOpenGLSourceTexture->GetDesc()->MipLevels != pOpenGLDestinationTexture->GetDesc()->MipLevels)
    {
        return false;
    }

    if (GLAD_GL_ARB_copy_image)
    {
        // preferred standard method
        glCopyImageSubData(pOpenGLSourceTexture->GetGLTextureId(), GL_TEXTURE_2D, sourceMipLevel, sourceX, sourceY, 0, pOpenGLDestinationTexture->GetGLTextureId(), GL_TEXTURE_2D, destMipLevel, destX, destY, 0, width, height, 1);
    }
    else if (GLAD_GL_NV_copy_image)
    {
        // fallback nvidia extension
        glCopyImageSubDataNV(pOpenGLSourceTexture->GetGLTextureId(), GL_TEXTURE_2D, sourceMipLevel, sourceX, sourceY, 0, pOpenGLDestinationTexture->GetGLTextureId(), GL_TEXTURE_2D, destMipLevel, destX, destY, 0, width, height, 1);
    }
    else
    {
        // use fbo
        BindMutatorTextureUnit();
        glBindTexture(GL_TEXTURE_2D, pOpenGLSourceTexture->GetGLTextureId());
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFrameBufferObjectId);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pOpenGLSourceTexture->GetGLTextureId(), sourceMipLevel);
        glCopyTexSubImage2D(GL_TEXTURE_2D, destMipLevel, destX, destY, sourceX, sourceY, width, height);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        RestoreMutatorTextureUnit();
    }

    return true;
}

void OpenGLGPUContext::BlitFrameBuffer(GPUTexture2D *pTexture, uint32 sourceX, uint32 sourceY, uint32 sourceWidth, uint32 sourceHeight, uint32 destX, uint32 destY, uint32 destWidth, uint32 destHeight, RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER resizeFilter /*= RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_NEAREST*/)
{
    // bind read framebuffer to source texture
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFrameBufferObjectId);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, static_cast<OpenGLGPUTexture2D *>(pTexture)->GetGLTextureId(), 0);

    // check fbo completeness
    DebugAssert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    // invoke the blit operation
    glBlitFramebuffer(sourceX, sourceY, sourceX + sourceWidth, sourceY + sourceHeight, destX, destY, destX + destWidth, destY + destHeight, GL_COLOR_BUFFER_BIT, (resizeFilter == RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_NEAREST) ? GL_NEAREST : GL_LINEAR);

    // unbind texture from fbo, and the fbo
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

void OpenGLGPUContext::GenerateMips(GPUTexture *pTexture)
{
    BindMutatorTextureUnit();

    switch (pTexture->GetTextureType())
    {
    case TEXTURE_TYPE_1D:
        glBindTexture(GL_TEXTURE_1D, static_cast<OpenGLGPUTexture1D *>(pTexture)->GetGLTextureId());
        glGenerateMipmap(GL_TEXTURE_1D);
        break;

    case TEXTURE_TYPE_1D_ARRAY:
        glBindTexture(GL_TEXTURE_1D_ARRAY, static_cast<OpenGLGPUTexture1DArray *>(pTexture)->GetGLTextureId());
        glGenerateMipmap(GL_TEXTURE_1D_ARRAY);
        break;

    case TEXTURE_TYPE_2D:
        glBindTexture(GL_TEXTURE_2D, static_cast<OpenGLGPUTexture2D *>(pTexture)->GetGLTextureId());
        glGenerateMipmap(GL_TEXTURE_2D);
        break;

    case TEXTURE_TYPE_2D_ARRAY:
        glBindTexture(TEXTURE_TYPE_2D_ARRAY, static_cast<OpenGLGPUTexture2DArray *>(pTexture)->GetGLTextureId());
        glGenerateMipmap(TEXTURE_TYPE_2D_ARRAY);
        break;

    case TEXTURE_TYPE_3D:
        glBindTexture(GL_TEXTURE_3D, static_cast<OpenGLGPUTexture3D *>(pTexture)->GetGLTextureId());
        glGenerateMipmap(GL_TEXTURE_3D);
        break;

    case TEXTURE_TYPE_CUBE:
        glBindTexture(GL_TEXTURE_CUBE_MAP, static_cast<OpenGLGPUTextureCube *>(pTexture)->GetGLTextureId());
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        break;

    case TEXTURE_TYPE_CUBE_ARRAY:
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, static_cast<OpenGLGPUTextureCubeArray *>(pTexture)->GetGLTextureId());
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP_ARRAY);
        break;
    }

    RestoreMutatorTextureUnit();
}
