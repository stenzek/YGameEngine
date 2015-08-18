#include "Renderer/PrecompiledHeader.h"
#include "Renderer/Renderer.h"
#include "Engine/ResourceManager.h"
#include "Engine/Font.h"
#include "Engine/Material.h"
#include "Engine/MaterialShader.h"
#include "Engine/Camera.h"
#include "Engine/World.h"
#include "Engine/Texture.h"
#include "Engine/EngineCVars.h"
#include "Engine/Engine.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/ShaderConstantBuffer.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/Shaders/OverlayShader.h"
#include "Renderer/Shaders/TextureBlitShader.h"
#include "Renderer/Shaders/DownsampleShader.h"
#include "Engine/SDLHeaders.h"
Log_SetChannel(Renderer);

// Calculates the scissor rectangle for a given point light.
// If this returns false, it means the light is not present on the screen at all.
// bool CalculatePointLightScissor(const Vector3 &LightPosition, float LightRadius, const Matrix4 &ViewMatrix, const Matrix4 &ProjectionMatrix, float NearDistance, RENDERER_SCISSOR_RECT *pScissorRect);
// bool CalculateAABoxScissor(const AABox &aabBounds, const Matrix4 &ViewMatrix, const Matrix4 &ProjectionMatrix, RENDERER_SCISSOR_RECT *pScissorRect);

//----------------------------------------------------- RenderSystem Creation Functions -----------------------------------------------------------------------------------------------
// renderer creation functions
typedef Renderer *(*RendererFactoryFunction)(const RendererInitializationParameters *pCreateParameters);
#if defined(WITH_RENDERER_D3D11)
    extern Renderer *D3D11Renderer_CreateRenderer(const RendererInitializationParameters *pCreateParameters);
#endif
#if defined(WITH_RENDERER_D3D12)
    extern Renderer *D3D12Renderer_CreateRenderer(const RendererInitializationParameters *pCreateParameters);
#endif
#if defined(WITH_RENDERER_OPENGL)
    extern Renderer *OpenGLRenderer_CreateRenderer(const RendererInitializationParameters *pCreateParameters);
#endif
#if defined(WITH_RENDERER_OPENGLES2)
    extern Renderer *OpenGLESRenderer_CreateRenderer(const RendererInitializationParameters *pCreateParameters);
#endif
struct RENDERER_PLATFORM_FACTORY_FUNCTION
{
    RENDERER_PLATFORM Platform;
    RendererFactoryFunction Function;
};

static const RENDERER_PLATFORM_FACTORY_FUNCTION s_renderSystemDeclarations[] =
{
#if defined(WITH_RENDERER_D3D11)
    { RENDERER_PLATFORM_D3D11,      D3D11Renderer_CreateRenderer    },
#endif
#if defined(WITH_RENDERER_D3D12)
    { RENDERER_PLATFORM_D3D12,      D3D12Renderer_CreateRenderer    },
#endif
#if defined(WITH_RENDERER_OPENGL)
    { RENDERER_PLATFORM_OPENGL,     OpenGLRenderer_CreateRenderer   },
#endif
#if defined(WITH_RENDERER_OPENGLES2)
    { RENDERER_PLATFORM_OPENGLES2,  OpenGLESRenderer_CreateRenderer },
#endif
};

//----------------------------------------------------- Global Variables ----------------------------------------------------------------------------------------------------------

// active renderer pointer
Renderer *g_pRenderer = NULL;
Thread::ThreadIdType Renderer::s_renderThreadId = static_cast<Thread::ThreadIdType>(0);
CommandQueue Renderer::s_renderCommandQueue;

//----------------------------------------------------- Output Window Class ----------------------------------------------------------------------------------------------------------

RendererOutputWindow::RendererOutputWindow(SDL_Window *pSDLWindow, RendererOutputBuffer *pBuffer, RENDERER_FULLSCREEN_STATE fullscreenState)
    : m_pSDLWindow(pSDLWindow),
      m_pOutputBuffer(pBuffer),
      m_fullscreenState(fullscreenState),
      m_hasFocus(true),
      m_visible(true),
      m_title(SDL_GetWindowTitle(pSDLWindow)),
      m_mouseGrabbed(false),
      m_mouseRelativeMovement(false)
{
    SDL_GetWindowPosition(m_pSDLWindow, &m_positionX, &m_positionY);
    SDL_GetWindowSize(m_pSDLWindow, reinterpret_cast<int *>(&m_width), reinterpret_cast<int *>(&m_height));
}

RendererOutputWindow::~RendererOutputWindow()
{
    // Buffer should no longer be used
    uint32 bufferReferenceCount = m_pOutputBuffer->Release();
    Assert(bufferReferenceCount == 0);

    // Destroy window itself
    SDL_DestroyWindow(m_pSDLWindow);
}

void RendererOutputWindow::SetWindowTitle(const char *title)
{
    DebugAssert(Renderer::IsOnRenderThread());

    m_title = title;
    SDL_SetWindowTitle(m_pSDLWindow, title);
}

void RendererOutputWindow::SetWindowVisibility(bool visible)
{
    DebugAssert(Renderer::IsOnRenderThread());

    m_visible = visible;
    if (visible)
        SDL_ShowWindow(m_pSDLWindow);
    else
        SDL_HideWindow(m_pSDLWindow);
}

void RendererOutputWindow::SetWindowPosition(int32 x, int32 y)
{
    DebugAssert(Renderer::IsOnRenderThread());

    SDL_SetWindowPosition(m_pSDLWindow, x, y);
    SDL_GetWindowPosition(m_pSDLWindow, &m_positionX, &m_positionY);
}

void RendererOutputWindow::SetWindowSize(uint32 width, uint32 height)
{
    DebugAssert(Renderer::IsOnRenderThread());

    SDL_SetWindowSize(m_pSDLWindow, width, height);
    SDL_GetWindowSize(m_pSDLWindow, reinterpret_cast<int *>(&m_width), reinterpret_cast<int *>(&m_height));
}

void RendererOutputWindow::SetMouseGrab(bool enabled)
{
    DebugAssert(Renderer::IsOnRenderThread()); 
    
    SDL_SetWindowGrab(m_pSDLWindow, (enabled) ? SDL_TRUE : SDL_FALSE);
}

void RendererOutputWindow::SetMouseRelativeMovement(bool enabled)
{
    DebugAssert(Renderer::IsOnRenderThread());

    if (enabled)
    {
        SDL_RaiseWindow(m_pSDLWindow);
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }
    else
    {
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }
}

// Thread-local pointer to current context
Y_DECLARE_THREAD_LOCAL(GPUContext *) s_pCurrentThreadGPUContext = nullptr;

GPUContext::GPUContext()
{
    m_ownerThreadID = 0;
    m_drawCallCounter = 0;
}

GPUContext::~GPUContext()
{
    DebugAssert(m_ownerThreadID == 0);
}

GPUContext *GPUContext::GetContextForCurrentThread()
{
    return s_pCurrentThreadGPUContext;
}

void GPUContext::SetContextForCurrentThread(GPUContext *pContext)
{
    Thread::ThreadIdType currentThreadID = Thread::GetCurrentThreadId();

    if (s_pCurrentThreadGPUContext != nullptr)
    {
        DebugAssert(s_pCurrentThreadGPUContext->GetOwnerThread() == currentThreadID);
        s_pCurrentThreadGPUContext->m_ownerThreadID = 0;
    }

    if ((s_pCurrentThreadGPUContext = pContext) != nullptr)
    {
        DebugAssert(s_pCurrentThreadGPUContext->GetOwnerThread() == 0);
        s_pCurrentThreadGPUContext->m_ownerThreadID = currentThreadID;
    }
}

//----------------------------------------------------- Init/Startup/Shutdown -----------------------------------------------------------------------------------------------------

RENDERER_PLATFORM Renderer::GetDefaultPlatform()
{
#if defined(Y_PLATFORM_WINDOWS)
    return RENDERER_PLATFORM_D3D11;
#elif defined(Y_PLATFORM_LINUX) || defined(Y_PLATFORM_OSX)
    return RENDERER_PLATFORM_OPENGL;
#elif defined(Y_PLATFORM_ANDROID) || defined(Y_PLATFORM_IOS) || defined(Y_PLATFORM_HTML5)
    return RENDERER_PLATFORM_OPENGLES2;
#else
    return RENDERER_PLATFORM_OPENGL;
#endif
}

bool Renderer::StartRenderThread(bool createWorkerThread)
{  
    // TODO: Make use of command buffer optional
    //uint32 commandBufferSize = CommandQueue::DEFAULT_COMMAND_QUEUE_SIZE * 4;
    uint32 commandBufferSize = (createWorkerThread) ? CommandQueue::DEFAULT_COMMAND_QUEUE_SIZE * 4 : 0;
    uint32 workerThreadCount = (createWorkerThread) ? 1 : 0;
    if (createWorkerThread)
        Log_InfoPrintf(" Creating render thread with a command buffer of %u bytes", commandBufferSize);
    else
        //Log_PerfPrintf(" Not using threaded rendering. Creating a command buffer of %u bytes.", commandBufferSize);
        Log_PerfPrintf(" Not using threaded rendering.", commandBufferSize);

    if (!s_renderCommandQueue.Initialize(commandBufferSize, workerThreadCount))
    {
        Log_ErrorPrintf(" Failed to create renderer worker thread.");
        return false;
    }

    s_renderThreadId = (createWorkerThread) ? s_renderCommandQueue.GetWorkerThreadID(0) : Thread::GetCurrentThreadId();
    Log_DevPrintf(" Renderer is owned by thread ID %u", (uint32)s_renderThreadId);
    return true;
}

void Renderer::StopRenderThread()
{
    Log_InfoPrintf("Shutting down render thread...");
    
    s_renderCommandQueue.ExitWorkers();
}

bool Renderer::Create(const RendererInitializationParameters *pCreateParameters)
{
    DebugAssert(pCreateParameters != nullptr);

    // apply cvar changes
    Log_DevPrintf("Applying pending renderer cvar changes...");
    g_pConsole->ApplyPendingRenderCVars();

    // if we were compiled with resource compiler, ensure the shader compiler is ready-to-go
#if defined(WITH_RESOURCECOMPILER_EMBEDDED) || defined(WITH_RESOURCECOMPILER_SUBPROCESS)
    ShaderCompilerFrontend::InitializeShaderCompilerSupport();
#endif

    // create start
    Log_InfoPrint("------- Renderer::Create() -------");
    Log_DevPrintf("Platform: %s", NameTable_GetNameString(NameTables::RendererPlatformFullName, pCreateParameters->Platform));
    Log_DevPrintf("Render thread is %s", pCreateParameters->EnableThreadedRendering ? "enabled" : "disabled");
    Log_DevPrintf("Backbuffer format: %s", PixelFormat_GetPixelFormatName(pCreateParameters->BackBufferFormat));
    Log_DevPrintf("Depth-stencil format: %s", PixelFormat_GetPixelFormatName(pCreateParameters->DepthStencilBufferFormat));
    Log_DevPrintf("Implicit window is %s", pCreateParameters->HideImplicitSwapChain ? "hidden" : ((pCreateParameters->ImplicitSwapChainFullScreen == RENDERER_FULLSCREEN_STATE_FULLSCREEN) ? "fullscreen" : "windowed"));
    Log_DevPrintf("Implicit window dimensions: %ux%u", pCreateParameters->ImplicitSwapChainWidth, pCreateParameters->ImplicitSwapChainHeight);

    // create render thread
    StartRenderThread(pCreateParameters->EnableThreadedRendering && CVars::r_use_render_thread.GetBool());

    // event that gets triggered once the renderer is created, or creation fails
    s_renderCommandQueue.QueueBlockingLambdaCommand([pCreateParameters]()
    {
        // initialize video subsystem
        static bool sdlVideoSubSystemInitialized = false;
        if (!sdlVideoSubSystemInitialized)
        {
            Log_DevPrintf(" Calling SDL_Init(SDL_INIT_VIDEO)...");
            if (SDL_Init(SDL_INIT_VIDEO) < 0)
            {
                Log_ErrorPrintf(" Failed to initialize SDL video subsystem: %s", SDL_GetError());
                return;
            }

            sdlVideoSubSystemInitialized = true;
        }

        // Try the preferred renderer first
        Log_InfoPrintf("  Requested renderer: %s", NameTable_GetNameString(NameTables::RendererPlatform, pCreateParameters->Platform));
        for (uint32 i = 0; i < countof(s_renderSystemDeclarations); i++)
        {
            if (s_renderSystemDeclarations[i].Platform == pCreateParameters->Platform)
            {
                Log_InfoPrintf(" Creating \"%s\" renderer...", NameTable_GetNameString(NameTables::RendererPlatform, s_renderSystemDeclarations[i].Platform));
                Renderer *pRenderer = s_renderSystemDeclarations[i].Function(pCreateParameters);
                if (pRenderer != NULL)
                {
                    g_pRenderer = pRenderer;

                    if (!pRenderer->BaseOnStart())
                    {
                        Log_ErrorPrintf(" Renderer creation succeeded, but base startup failed.");
                        pRenderer->BaseOnShutdown();
                        g_pRenderer = NULL;
                        delete pRenderer;
                    }
                }
                break;
            }
        }

        if (g_pRenderer == NULL)
        {
            Log_ErrorPrintf(" Unknown renderer or failed to create: %s", NameTable_GetNameString(NameTables::RendererPlatform, pCreateParameters->Platform));
            for (uint32 i = 0; i < countof(s_renderSystemDeclarations); i++)
            {
                Log_InfoPrintf(" Creating \"%s\" renderer...", NameTable_GetNameString(NameTables::RendererPlatform, s_renderSystemDeclarations[i].Platform));
                Renderer *pRenderer = s_renderSystemDeclarations[i].Function(pCreateParameters);
                if (pRenderer != NULL)
                {
                    g_pRenderer = pRenderer;

                    if (!pRenderer->BaseOnStart())
                    {
                        Log_ErrorPrintf(" Renderer creation succeeded, but base startup failed.");
                        pRenderer->BaseOnShutdown();
                        g_pRenderer = NULL;
                        delete pRenderer;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    });

    // fail?
    if (g_pRenderer == NULL)
    {
        Log_ErrorPrintf(" No renderer could be created.");
        StopRenderThread();
        return false;
    }

    Log_InfoPrint("-----------------------------------");
    return true;
}

void Renderer::Shutdown()
{
    DebugAssert(g_pRenderer != NULL);

    Log_InfoPrint("------ Renderer::Shutdown() -------");

    // has to be executed on the render thread
    s_renderCommandQueue.QueueBlockingLambdaCommand([]()
    {
        g_pRenderer->BaseOnShutdown();

        delete g_pRenderer;
        g_pRenderer = NULL;
    });

    Log_InfoPrint("-----------------------------------");   
}

Renderer::Renderer()
    : m_fixedResources(this)
{
    m_eRendererPlatform = RENDERER_PLATFORM_COUNT;
    m_eRendererFeatureLevel = RENDERER_FEATURE_LEVEL_COUNT;
    m_eTexturePlatform = NUM_TEXTURE_PLATFORMS;
    
    // default caps, supports nothing
    Y_memzero(&m_RendererCapabilities, sizeof(m_RendererCapabilities));
    m_RendererCapabilities.SupportsTextureArrays = false;
    m_RendererCapabilities.SupportsCubeMapTextureArrays = false;
    m_RendererCapabilities.SupportsGeometryShaders = false;
    m_RendererCapabilities.SupportsSinglePassCubeMaps = false;
    m_RendererCapabilities.SupportsInstancing = false;

    m_fTexelOffset = 0.0f;
}

Renderer::~Renderer()
{

}

bool Renderer::BaseOnStart()
{
    // set render thread id
    s_renderThreadId = Thread::GetCurrentThreadId();

    // states
    if (!m_fixedResources.CreateResources())
    {
        m_fixedResources.ReleaseResources();
        return false;
    }

    // resource manager
    g_pResourceManager->CreateDeviceResources();

    // init gui context
    m_guiContext.SetGPUContext(GetMainContext());
    m_guiContext.SetViewportDimensions(GetMainContext()->GetViewport());

    // ok
    return true;
}

void Renderer::BaseOnShutdown()
{
    // clear all state
    GetMainContext()->ClearState(true, true, true, true);

    // release resources
    m_fixedResources.ReleaseResources();

    // release gpu resources on all managed resources
    g_pResourceManager->ReleaseDeviceResources();

    // release all non-material shaders
    m_nullMaterialShaderMap.ReleaseGPUResources();
}

bool Renderer::InternalDrawPlain(GPUContext *pGPUDevice, const PlainVertexFactory::Vertex *pVertices, uint32 nVertices, uint32 flags)
{
    const uint32 vertexSize = PlainVertexFactory::GetVertexSize(m_eRendererPlatform, m_eRendererFeatureLevel, flags);
    const uint32 bufferSize = vertexSize * nVertices;
    
    // allocate buffer
    byte buffer[4096];
    byte *pHeapBuffer = NULL;
    byte *pBuffer = buffer;
    if (bufferSize > countof(buffer))
    {
        // use heap buffer
        pHeapBuffer = new byte[bufferSize];
        pBuffer = pHeapBuffer;
    }

    // fill buffer
    if (!PlainVertexFactory::FillBuffer(m_eRendererPlatform, m_eRendererFeatureLevel, flags, pVertices, nVertices, pBuffer, bufferSize))
    {
        delete[] pHeapBuffer;
        return false;
    }

    // invoke draw
    pGPUDevice->DrawUserPointer(pBuffer, vertexSize, nVertices);

    // free any memory
    delete[] pHeapBuffer;
    return true;
}

void Renderer::DrawFullScreenQuad(GPUContext *pContext)
{
    pContext->SetVertexBuffer(0, m_fixedResources.GetFullScreenQuadVertexBuffer(), 0, sizeof(FixedResources::ScreenQuadVertex));
    pContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_STRIP);
    pContext->Draw(0, 4);
}

void Renderer::BlitTextureUsingShader(GPUContext *pContext, GPUTexture2D *pSourceTexture, uint32 sourceX, uint32 sourceY, uint32 sourceWidth, uint32 sourceHeight, int32 sourceLevel, uint32 destX, uint32 destY, uint32 destWidth, uint32 destHeight, RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER resizeFilter /* = RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_NEAREST */, RENDERER_FRAMEBUFFER_BLIT_BLEND_MODE blendMode /* = RENDERER_FRAMEBUFFER_BLIT_BLEND_MODE_NONE */)
{
    // calculate source size and offset in texels
    uint32 sourceTextureWidth = pSourceTexture->GetDesc()->Width;
    uint32 sourceTextureHeight = pSourceTexture->GetDesc()->Height;
    float2 sourceOffset((float)sourceX / (float)sourceTextureWidth, (float)sourceY / (float)sourceTextureHeight);
    float2 sourceSize((float)sourceWidth / (float)sourceTextureWidth, (float)sourceHeight / (float)sourceTextureHeight);

    // select sampler state
    GPUSamplerState *pSamplerState;
    if (resizeFilter == RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_NEAREST)
        pSamplerState = m_fixedResources.GetPointSamplerState();
    else // if (resizeFilter == RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_LINEAR)
        pSamplerState = m_fixedResources.GetLinearSamplerState();

    // select blend state
    GPUBlendState *pBlendState;
    if (blendMode == RENDERER_FRAMEBUFFER_BLIT_BLEND_MODE_ADDITIVE)
        pBlendState = m_fixedResources.GetBlendStateAdditive();
    else if (blendMode == RENDERER_FRAMEBUFFER_BLIT_BLEND_MODE_ALPHA_BLENDING)
        pBlendState = m_fixedResources.GetBlendStateAlphaBlending();
    else
        pBlendState = m_fixedResources.GetBlendStateNoBlending();

    // get program
    ShaderProgram *pShaderProgram = (sourceLevel < 0) ? m_fixedResources.GetTextureBlitShader() : m_fixedResources.GetTextureBlitLODShader();
    DebugAssert(pShaderProgram != nullptr);

    // set state
    pContext->SetRasterizerState(m_fixedResources.GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false, false));
    pContext->SetDepthStencilState(m_fixedResources.GetDepthStencilState(false, false), 0);
    pContext->SetBlendState(pBlendState);
    pContext->SetShaderProgram(pShaderProgram->GetGPUProgram());
    TextureBlitShader::SetProgramParameters(pContext, pShaderProgram, pSourceTexture, pSamplerState, sourceLevel);

    // create vertices
    FixedResources::ScreenQuadVertex vertices[4];
    vertices[0].Set((float)destX, (float)destY, sourceOffset.x, sourceOffset.y);
    vertices[1].Set((float)destX, (float)(destY + destHeight), sourceOffset.x, sourceOffset.y + sourceSize.y);
    vertices[2].Set((float)(destX + destWidth), (float)destY, sourceOffset.x + sourceSize.x, sourceOffset.y);
    vertices[3].Set((float)(destX + destWidth), (float)(destY + destHeight), sourceOffset.x + sourceSize.x, sourceOffset.y + sourceSize.y);

    // draw the quad
    pContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_STRIP);
    pContext->DrawUserPointer(vertices, sizeof(vertices[0]), countof(vertices));
}

// void Renderer::DrawTextureMips(GPUContext *pContext, GPUTexture2D *pTexture, int32 maxLevel /* = -1 */)
// {
//     const GPU_TEXTURE2D_DESC *pDesc = pTexture->GetDesc();
// 
//     // set common state
//     ShaderProgram *pDownsampleShader = m_fixedResources.GetDownsampleShader();
//     pContext->SetRasterizerState(m_fixedResources.GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false, false));
//     pContext->SetDepthStencilState(m_fixedResources.GetDepthStencilState(false, false), 0);
//     pContext->SetBlendState(m_fixedResources.GetBlendStateNoBlending());
//     pContext->SetShaderProgram(pDownsampleShader->GetGPUProgram());
// 
//     // draw each mip level apart from the top
//     uint32 maxMipLevel = (maxLevel > 0) ? (uint32)maxLevel : pDesc->MipLevels;
//     for (uint32 mipLevel = 1; mipLevel < maxMipLevel; mipLevel++)
//     {
//         uint32 mipWidth = Max((uint32)1, (pDesc->Width >> mipLevel));
//         uint32 mipHeight = Max((uint32)1, (pDesc->Height >> mipLevel));
//         RENDERER_VIEWPORT mipViewport(0, 0, mipWidth, mipHeight, 0.0f, 1.0f);
//         int32 layer = -1;
// 
//         // set source/destination texture
//         pContext->SetRenderTargetLayers(1, reinterpret_cast<GPUTexture **>(pTexture), &mipLevel, &layer, nullptr, 0, -1);
//         DownsampleShader::SetProgramParameters(pContext, pDownsampleShader, pTexture, mipLevel - 1);
//         pContext->SetViewport(&mipViewport);
// 
//         // draw away
//         DrawFullScreenQuad(pContext);
//     }
// 
//     // clear out the targets in case they are used immediately afterwards before being unbound
//     pContext->ClearState(true, false, false, true);
// }

// 
// void Renderer::DrawDebugText(int32 x, int32 y, int32 Height, const char *Text, MINIGUI_HORIZONTAL_ALIGNMENT hAlign /* = MINIGUI_HORIZONTAL_ALIGNMENT_LEFT */, MINIGUI_VERTICAL_ALIGNMENT vAlign /* = MINIGUI_VERTICAL_ALIGNMENT_TOP */)
// {
//     m_GUIContext.DrawText(m_pDebugTextFont, Height, x, y, Text, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), false, hAlign, vAlign);
// }

ShaderProgram *Renderer::GetShaderProgram(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const GPU_VERTEX_ELEMENT_DESC *pVertexAttributes, uint32 nVertexAttributes, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    DebugAssert(IsOnRenderThread());

    // select shader map to use
    ShaderMap &shaderMap = (pMaterialShader != NULL) ? pMaterialShader->GetShaderMap() : m_nullMaterialShaderMap;

    // look up shader
    return shaderMap.GetShaderPermutation(globalShaderFlags, pBaseShaderTypeInfo, baseShaderFlags, pVertexAttributes, nVertexAttributes, pMaterialShader, materialShaderFlags);
}

ShaderProgram *Renderer::GetShaderProgram(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    DebugAssert(IsOnRenderThread());

    // select shader map to use
    ShaderMap &shaderMap = (pMaterialShader != NULL) ? pMaterialShader->GetShaderMap() : m_nullMaterialShaderMap;

    // look up shader
    return shaderMap.GetShaderPermutation(globalShaderFlags, pBaseShaderTypeInfo, baseShaderFlags, pVertexFactoryTypeInfo, vertexFactoryFlags, pMaterialShader, materialShaderFlags);
}

uint3 Renderer::GetTextureDimensions(const GPUTexture *pTexture)
{
    switch (pTexture->GetTextureType())
    {
    case TEXTURE_TYPE_1D:
        {
            const GPU_TEXTURE1D_DESC *pDesc = static_cast<const GPUTexture1D *>(pTexture)->GetDesc();
            return uint3(pDesc->Width, 1, 1);
        }

    case TEXTURE_TYPE_1D_ARRAY:
        {
            const GPU_TEXTURE1DARRAY_DESC *pDesc = static_cast<const GPUTexture1DArray *>(pTexture)->GetDesc();
            return uint3(pDesc->Width, pDesc->ArraySize, 1);
        }

    case TEXTURE_TYPE_2D:
        {
            const GPU_TEXTURE2D_DESC *pDesc = static_cast<const GPUTexture2D *>(pTexture)->GetDesc();
            return uint3(pDesc->Width, pDesc->Height, 1);
        }

    case TEXTURE_TYPE_2D_ARRAY:
        {
            const GPU_TEXTURE2DARRAY_DESC *pDesc = static_cast<const GPUTexture2DArray *>(pTexture)->GetDesc();
            return uint3(pDesc->Width, pDesc->Height, pDesc->ArraySize);
        }

    case TEXTURE_TYPE_3D:
        {
            const GPU_TEXTURE3D_DESC *pDesc = static_cast<const GPUTexture3D *>(pTexture)->GetDesc();
            return uint3(pDesc->Width, pDesc->Height, pDesc->Depth);
        }

    case TEXTURE_TYPE_CUBE:
        {
            const GPU_TEXTURECUBE_DESC *pDesc = static_cast<const GPUTextureCube *>(pTexture)->GetDesc();
            return uint3(pDesc->Width, pDesc->Height, 1);
        }

    case TEXTURE_TYPE_CUBE_ARRAY:
        {
            const GPU_TEXTURECUBEARRAY_DESC *pDesc = static_cast<const GPUTextureCubeArray *>(pTexture)->GetDesc();
            return uint3(pDesc->Width, pDesc->Height, pDesc->ArraySize);
        }

    case TEXTURE_TYPE_DEPTH:
        {
            const GPU_DEPTH_TEXTURE_DESC *pDesc = static_cast<const GPUDepthTexture *>(pTexture)->GetDesc();
            return uint3(pDesc->Width, pDesc->Height, 1);
        }

    default:
        UnreachableCode();
    }

    return uint3::Zero;
}
// 
// bool Renderer::DrawDirectionalShadowMap(const RenderWorld *pRenderWorld, const Camera *pCamera, RENDER_QUEUE_LIGHT_ENTRY *pLight)
// {
//     uint32 i;
// 
//     // find a free directional shadow map
//     GPUTexture *pShadowMapTexture = NULL;
//     for (i = 0; i < m_DirectionalShadowMapTextures.GetSize(); i++)
//     {
//         ShadowMapTextureUsedPair &sm = m_DirectionalShadowMapTextures[i];
//         if (!sm.Right)
//         {
//             pShadowMapTexture = sm.Left;
//             sm.Right = true;
//             break;
//         }
//     }
// 
//     // none found?
//     if (i == m_DirectionalShadowMapTextures.GetSize())
//     {
//         Log_PerfPrintf("Allocating new directional shadowmap...");
//         if ((pShadowMapTexture = CreateTexture2D(&m_DirectionalShadowMapTextureDesc, &m_DirectionalShadowMapSamplerStateDesc)) == NULL)
//         {
//             Log_ErrorPrintf("Renderer::DrawDirectionalShadowMap: Failed to create shadow map texture.");
//             return false;
//         }
// 
//         m_DirectionalShadowMapTextures.Add(ShadowMapTextureUsedPair(pShadowMapTexture, true));
//     }
// 
//     // get light paramters
//     Vector3 lightDirection = pLight->TypeSpecificParameters.Direction;
// 
//     // get view camera parameters
//     Vector3 viewCameraPosition = pCamera->GetEyePosition();
//     Vector3 viewCameraDirection = pCamera->GetViewDirection();
//     float viewCameraNearPlaneDistance = pCamera->GetNearPlaneDistance();
//     float viewCameraFarPlaneDistance = pCamera->GetFarPlaneDistance();
//     //float extrusionDistance = 100.0f;
//     float extrusionDistance = (viewCameraFarPlaneDistance - viewCameraNearPlaneDistance) * 0.5f;
//     float viewMoveDistance = 1.0f;
// 
//     // calculate the light camera parameters
//     //PerspectiveCamera lightCamera;
//     //lightCamera.SetFieldOfView(90.0f);
//     OrthographicCamera lightCamera;
//     lightCamera.SetViewportDimensions(m_DirectionalShadowMapTextureDesc.Width, m_DirectionalShadowMapTextureDesc.Height);
//     lightCamera.SetNearPlaneDistance(1.0f);
//     lightCamera.SetFarPlaneDistance(10000.0f);
//     
//     // setup light camera position
//     //Vector3 lightCameraPosition = (viewCameraPosition + (viewCameraDirection * viewMoveDistance)) - (lightDirection * extrusionDistance);
//     
//     Vector3 lightCameraPosition;
//     lightCameraPosition = viewCameraPosition;
//     //lightCameraPosition -= (lightDirection * extrusionDistance);    
//     //lightCameraPosition.Set(-258.553f, -56.204f, 189.980f);
//     //lightCameraPosition.Set(-258.553f, -56.204f, 530.0f);
//     //lightCameraPosition.Set(-258.553f, 256.204f, 530.0f);
//     //lightCameraPosition.SetZero();
//     
//     // get camera up vector
//     Vector3 lightCameraUpVector = Vector3::UnitZ;
//     if (Math::Abs(lightCameraUpVector.Dot(lightDirection)) >= 1.0f)
//         lightCameraUpVector = Vector3::UnitY;
// 
//     lightCamera.SetPosition(lightCameraPosition);
//     //lightCamera.LookDirection(lightDirection, lightCameraUpVector);
//     //lightCamera.LookAt(lightCameraPosition + lightDirection, lightCameraUpVector);
//     //lightCamera.SetRotation(Quaternion::FromRotationYXZ(Math::DegreesToRadians(0.0f), Math::DegreesToRadians(0.0f), Math::DegreesToRadians(0.0f)));
//     lightCamera.LookDirection(lightDirection, lightCameraUpVector);
// 
//     Vector3 temp = lightCamera.GetViewDirection();
// 
//     // setup device
//     GPUDevice *pGPUDevice = GetGPUDevice();
//     pGPUDevice->SetRenderTargets(1, &pShadowMapTexture, NULL, m_pDirectionalShadowMapDepthStencilBuffer);
//     pGPUDevice->SetDefaultViewport(pShadowMapTexture);
//     
//     // clear the targets
//     pGPUDevice->ClearTargets(true, true, true);
// 
//     // get shadow map render context
//     RenderContext *pRenderContext = GetSecondaryRenderContext();
//     DebugAssert(pRenderContext->GetDevice() == NULL);
// 
//     // init render context common fields
//     pRenderContext->SetDevice(pGPUDevice);
//     pRenderContext->SetRenderWorld(pRenderWorld);
//     pRenderContext->SetCamera(&lightCamera);
//     pRenderContext->SetRenderFlags(0);
// 
//     // draw it
//     pRenderContext->DrawDirectionalShadowMap();
// 
//     // clear context
//     pRenderContext->ClearState();
// 
//     // set shadow map pointer
//     pLight->pShadowMapTexture = pShadowMapTexture;
// 
//     // generate projection matrix
//     Matrix4 correctedProjectionMatrix = lightCamera.GetProjectionMatrix();
//     CorrectProjectionMatrix(correctedProjectionMatrix);
//     pLight->LightViewProjectionMatrix = correctedProjectionMatrix * Matrix4(lightCamera.GetViewMatrix());
// 
//     // ok
//     return true;
// }
// 
// bool Renderer::DrawPointShadowMap(const RenderWorld *pRenderWorld, RENDER_QUEUE_LIGHT_ENTRY *pLight)
// {
//     return false;
// 
// //     uint32 i;
// // 
// //     // find a free directional shadow map
// //     GPUTexture *pShadowMapTexture = NULL;
// //     for (i = 0; i < m_OmnidirectionalShadowMapTextures.GetSize(); i++)
// //     {
// //         ShadowMapTextureUsedPair &sm = m_OmnidirectionalShadowMapTextures[i];
// //         if (!sm.Right)
// //         {
// //             pShadowMapTexture = sm.Left;
// //             sm.Right = true;
// //             break;
// //         }
// //     }
// // 
// //     // none found?
// //     if (i == m_OmnidirectionalShadowMapTextures.GetSize())
// //     {
// //         Log_PerfPrintf("Allocating new point shadowmap...");
// //         if ((pShadowMapTexture = CreateTextureCube(&m_OmnidirectionalShadowMapTextureDesc, &m_OmnidrectionalShadowMapSamplerStateDesc)) == NULL)
// //         {
// //             Log_ErrorPrintf("Renderer::DrawPointShadowMap: Failed to create shadow map texture.");
// //             return false;
// //         }
// // 
// //         m_OmnidirectionalShadowMapTextures.Add(ShadowMapTextureUsedPair(pShadowMapTexture, true));
// //     }
// // 
// //     // calculate light projection matrix
// //     Matrix4 lightProjectionMatrix = Make4x4OrthographicProjectionMatrix(0.0f, (float)m_DirectionalShadowMapTextureDesc.Width, 0.0f, (float)m_DirectionalShadowMapTextureDesc.Height,
// //                                                                         0.1f, pLight->Range);
// // 
// //     // store the shadow map texture
// //     pLight->pShadowMapTexture = pShadowMapTexture;
// // 
// //     // get shadow map render context
// //     RenderContext *pRenderContext = GetSecondaryRenderContext();
// // 
// //     // state should be cleared
// //     DebugAssert(pRenderContext->GetDevice() == NULL);
// // 
// //     // init render context common fields
// //     pRenderContext->SetDevice(GetGPUDevice());
// //     pRenderContext->SetRenderWorld(pRenderWorld);
// //     pRenderContext->SetRenderFlags(0);
// //     //pRenderContext->SetProjectionMatrix(lightProjectionMatrix);
// // 
// //     // draw it
// //     pRenderContext->DrawPointShadowMap();
// // 
// //     // clear context
// //     pRenderContext->ClearState();
// //     return true;
// }
// 
// void Renderer::ReleaseShadowMap(GPUTexture *pShadowMapTexture)
// {
//     DebugAssert(pShadowMapTexture != NULL);
// 
//     uint32 i;
//     if (pShadowMapTexture->GetType() == TEXTURE_TYPE_2D)
//     {
//         for (i = 0; i < m_DirectionalShadowMapTextures.GetSize(); i++)
//         {
//             ShadowMapTextureUsedPair &sm = m_DirectionalShadowMapTextures[i];
//             if (sm.Left == pShadowMapTexture)
//             {
//                 DebugAssert(sm.Right);
//                 sm.Right = false;
//                 return;
//             }
//         }
//     }
//     else if (pShadowMapTexture->GetType() == TEXTURE_TYPE_CUBE)
//     {
//         for (i = 0; i < m_OmnidirectionalShadowMapTextures.GetSize(); i++)
//         {
//             ShadowMapTextureUsedPair &sm = m_OmnidirectionalShadowMapTextures[i];
//             if (sm.Left == pShadowMapTexture)
//             {
//                 DebugAssert(sm.Right);
//                 sm.Right = false;
//                 return;
//             }
//         }
//     }
//     else
//     {
//         UnreachableCode();
//     }
// 
//     Panic("Attempting to release an untracked shadow map texture.");
// }

void Renderer::FillDefaultRasterizerState(RENDERER_RASTERIZER_STATE_DESC *pRasterizerState)
{
    pRasterizerState->FillMode = RENDERER_FILL_SOLID;
    //pRasterizerState->CullMode = RENDERER_CULL_NONE;
    pRasterizerState->CullMode = RENDERER_CULL_BACK;
    //pRasterizerState->FrontCounterClockwise = false;
    pRasterizerState->FrontCounterClockwise = true;
    pRasterizerState->ScissorEnable = false;
    pRasterizerState->DepthBias = 0;
    pRasterizerState->SlopeScaledDepthBias = 0.0f;
    pRasterizerState->DepthClipEnable = true;
}

void Renderer::FillDefaultDepthStencilState(RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilState)
{
    pDepthStencilState->DepthTestEnable = true;
    pDepthStencilState->DepthWriteEnable = true;
    pDepthStencilState->DepthFunc = GPU_COMPARISON_FUNC_LESS;

    pDepthStencilState->StencilTestEnable = false;
    pDepthStencilState->StencilReadMask = 0xFF;
    pDepthStencilState->StencilWriteMask = 0xFF;
    pDepthStencilState->StencilFrontFace.FailOp = RENDERER_STENCIL_OP_KEEP;
    pDepthStencilState->StencilFrontFace.DepthFailOp = RENDERER_STENCIL_OP_KEEP;
    pDepthStencilState->StencilFrontFace.PassOp = RENDERER_STENCIL_OP_KEEP;
    pDepthStencilState->StencilFrontFace.CompareFunc = GPU_COMPARISON_FUNC_ALWAYS;
    pDepthStencilState->StencilBackFace.FailOp = RENDERER_STENCIL_OP_KEEP;
    pDepthStencilState->StencilBackFace.DepthFailOp = RENDERER_STENCIL_OP_KEEP;
    pDepthStencilState->StencilBackFace.PassOp = RENDERER_STENCIL_OP_KEEP;
    pDepthStencilState->StencilBackFace.CompareFunc = GPU_COMPARISON_FUNC_ALWAYS;
}

void Renderer::FillDefaultBlendState(RENDERER_BLEND_STATE_DESC *pBlendState)
{
    pBlendState->BlendEnable = false;
    pBlendState->SrcBlend = RENDERER_BLEND_ONE;
    pBlendState->DestBlend = RENDERER_BLEND_ZERO;
    pBlendState->BlendOp = RENDERER_BLEND_OP_ADD;
    pBlendState->SrcBlendAlpha = RENDERER_BLEND_ONE;
    pBlendState->DestBlendAlpha = RENDERER_BLEND_ZERO;
    pBlendState->BlendOpAlpha = RENDERER_BLEND_OP_ADD;
    pBlendState->ColorWriteEnable = true;
}

void Renderer::FillDefaultSamplerState(GPU_SAMPLER_STATE_DESC *pSamplerState)
{
    pSamplerState->Filter = TEXTURE_FILTER_ANISOTROPIC;
    pSamplerState->MaxAnisotropy = 16;   
    pSamplerState->AddressU = TEXTURE_ADDRESS_MODE_WRAP;
    pSamplerState->AddressV = TEXTURE_ADDRESS_MODE_WRAP;
    pSamplerState->AddressW = TEXTURE_ADDRESS_MODE_WRAP;
    pSamplerState->BorderColor.SetZero();
    pSamplerState->LODBias = 0.0f;
    pSamplerState->MinLOD = Y_INT32_MIN;
    pSamplerState->MaxLOD = Y_INT32_MAX;
    pSamplerState->ComparisonFunc = GPU_COMPARISON_FUNC_NEVER;
}

void Renderer::CorrectTextureFilter(GPU_SAMPLER_STATE_DESC *pSamplerState)
{
    const RendererCapabilities &capabilities = g_pRenderer->GetCapabilities();
    bool isComparisonFilter = (pSamplerState->Filter < TEXTURE_FILTER_COUNT && pSamplerState->Filter >= TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_POINT);

    switch (CVars::r_texture_filtering.GetInt())
    {
        // bilinear
    case 1:
        {
            if (isComparisonFilter)
            {
                if (pSamplerState->Filter > TEXTURE_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT)
                    pSamplerState->Filter = TEXTURE_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
            }
            else
            {
                if (pSamplerState->Filter > TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT)
                    pSamplerState->Filter = TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            }

            pSamplerState->MaxAnisotropy = 1;
        }
        break;

        // trilinear
    case 2:
        {
            if (isComparisonFilter)
            {
                if (pSamplerState->Filter > TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR)
                    pSamplerState->Filter = TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
            }
            else
            {
                if (pSamplerState->Filter > TEXTURE_FILTER_MIN_MAG_MIP_LINEAR)
                    pSamplerState->Filter = TEXTURE_FILTER_MIN_MAG_MIP_LINEAR;
            }

            pSamplerState->MaxAnisotropy = 1;
        }
        break;

        // anisotropic
    case 3:
        {
            if (capabilities.MaxTextureAnisotropy > 0)
            {
                // HACKFIX! FIXME!
                if (pSamplerState->Filter == TEXTURE_FILTER_COUNT)
                    pSamplerState->Filter = TEXTURE_FILTER_ANISOTROPIC;

                if (pSamplerState->Filter == TEXTURE_FILTER_ANISOTROPIC || pSamplerState->Filter == TEXTURE_FILTER_COMPARISON_ANISOTROPIC)
                    pSamplerState->MaxAnisotropy = Min((uint32)CVars::r_max_anisotropy.GetInt(), capabilities.MaxTextureAnisotropy);
                else
                    pSamplerState->MaxAnisotropy = 1;
            }
            else
            {
                // fallback to trilinear
                if (isComparisonFilter)
                {
                    if (pSamplerState->Filter > TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR)
                        pSamplerState->Filter = TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
                }
                else
                {
                    if (pSamplerState->Filter > TEXTURE_FILTER_MIN_MAG_MIP_LINEAR)
                        pSamplerState->Filter = TEXTURE_FILTER_MIN_MAG_MIP_LINEAR;
                }

                pSamplerState->MaxAnisotropy = 1;
            }
        }
        break;

        // point
    default:
        {
            if (isComparisonFilter)
            {
                if (pSamplerState->Filter > TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_POINT)
                    pSamplerState->Filter = TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
            }
            else
            {
                if (pSamplerState->Filter > TEXTURE_FILTER_MIN_MAG_MIP_POINT)
                    pSamplerState->Filter = TEXTURE_FILTER_MIN_MAG_MIP_POINT;
            }

            pSamplerState->MaxAnisotropy = 1;
        }
        break;
    }
}

bool Renderer::TextureFilterRequiresMips(TEXTURE_FILTER filter)
{
    switch (filter)
    {
    case TEXTURE_FILTER_MIN_MAG_POINT_MIP_LINEAR:
    case TEXTURE_FILTER_MIN_POINT_MAG_MIP_LINEAR:
    case TEXTURE_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
    case TEXTURE_FILTER_MIN_MAG_MIP_LINEAR:
    case TEXTURE_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
    case TEXTURE_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
    case TEXTURE_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
    case TEXTURE_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR:
        return true;

    default:
        return false;
    }
}

uint32 Renderer::CalculateMipCount(uint32 width, uint32 height /* = 1 */, uint32 depth /* = 1 */)
{
    uint32 currentWidth = width;
    uint32 currentHeight = height;
    uint32 currentDepth = depth;
    uint32 mipCount = 1;

    while (currentWidth > 1 || currentHeight > 1 || currentDepth > 1)
    {
        if (currentWidth > 1)
            currentWidth /= 2;
        if (currentHeight > 1)
            currentHeight /= 2;
        if (currentDepth > 1)
            currentDepth /= 2;

        mipCount++;
    }

    return mipCount;
}

bool Renderer::CalculateAABoxScissorRect(RENDERER_SCISSOR_RECT *pScissorRect, const AABox &Bounds, const float4x4 &ViewMatrix, int32 RTWidth, int32 RTHeight)
{
    return false;
}

bool Renderer::CalculatePointLightScissorRect(RENDERER_SCISSOR_RECT *pScissorRect, const float3 &LightPosition, const float &LightRange, const float4x4 &ViewMatrix, int32 RTWidth, int32 RTHeight)
{
    // http://www.opengl.org/discussion_boards/ubbthreads.php?ubb=showflat&Number=173666&page=2
    float aspect = (float)RTWidth / (float)RTHeight;
    const int32 &sx = RTWidth;
    const int32 &sy = RTHeight;
    
    int32 outRect[4] = { 0, 0, RTWidth, RTHeight };

    const float &r = LightRange;
    float r2 = r * r;

    SIMDVector3f l = (ViewMatrix * SIMDVector4f(LightPosition, 1.0f)).xyz();
    SIMDVector3f l2 = l * l;

    float e1 = 1.2f;
    float e2 = 1.2f * aspect;

    float d = r2 * l2.x - (l2.x + l2.z) * (r2 - l2.z);
    if (d>=0)
    {
        d = Y_sqrtf(d);

        float nx1 = (r*l.x + d) / (l2.x + l2.z);
        float nx2 = (r*l.x - d) / (l2.x + l2.z);

        float nz1 = (r - nx1*l.x) / l.z;
        float nz2 = (r - nx2*l.x) / l.z;

        //float e = 1.25f;
        //float a = aspect;

        float pz1 = (l2.x + l2.z-r2) / (l.z - (nz1 / nx1) * l.x);
        float pz2 = (l2.x + l2.z-r2) / (l.z - (nz2 / nx2) * l.x);

        if (pz1 >= 0.0f && pz2 >= 0.0f)
            return false;

        if (pz1 < 0)
        {
            float fx=nz1*e1/nx1;
            int ix=(int)((fx+1.0f)*sx*0.5f);

            float px=-pz1*nz1/nx1;
            if (px<l.x)
                outRect[0] = Max(outRect[0],ix);
            else
                outRect[2] = Min(outRect[2],ix);
        }

        if (pz2<0)
        {
            float fx=nz2*e1/nx2;
            int ix=(int)((fx+1.0f)*sx*0.5f);

            float px=-pz2*nz2/nx2;
            if (px<l.x)
                outRect[0] = Max(outRect[0],ix);
            else
                outRect[2] = Min(outRect[2],ix);
        }

        if (outRect[2] <= outRect[0])
            return false;
    }

    d=r2*l2.y - (l2.y+l2.z)*(r2-l2.z);
    if (d >= 0.0f)
    {
        d = Y_sqrtf(d);

        float ny1=(r*l.y + d)/(l2.y + l2.z);
        float ny2=(r*l.y - d)/(l2.y + l2.z);

        float nz1=(r - ny1*l.y)/l.z;
        float nz2=(r - ny2*l.y)/l.z;

        float pz1=(l2.y + l2.z-r2)/(l.z - (nz1/ny1)*l.y);
        float pz2=(l2.y + l2.z-r2)/(l.z - (nz2/ny2)*l.y);

        if (pz1>=0 && pz2>=0)
            return false;

        if (pz1<0)
        {
            float fy=nz1*e2/ny1;
            int iy=(int)((fy+1.0f)*sy*0.5f);

            float py=-pz1*nz1/ny1;
            if (py<l.y)
                outRect[1]=Max(outRect[1],iy);
            else
                outRect[3]=Min(outRect[3],iy);
        }

        if (pz2<0)
        {
            float fy=nz2*e2/ny2;
            int iy=(int)((fy+1.0f)*sy*0.5f);

            float py=-pz2*nz2/ny2;
            if (py<l.y)
                outRect[1]=Max(outRect[1],iy);
            else
                outRect[3]=Min(outRect[3],iy);
        }

        if (outRect[3]<=outRect[1])
            return false;
    }

    // gen scissor rect
    pScissorRect->Left = outRect[0];
    pScissorRect->Top = outRect[1];
    pScissorRect->Right = outRect[2];
    pScissorRect->Bottom = outRect[3];
    return true;
}

bool Renderer::CalculateAABoxScissorRect(RENDERER_SCISSOR_RECT *pScissorRect, const AABox &Bounds)
{
    GPURenderTargetView *pRenderTargetView;
    g_pRenderer->GetMainContext()->GetRenderTargets(1, &pRenderTargetView, nullptr);

    const float4x4 &viewMatrix = g_pRenderer->GetMainContext()->GetConstants()->GetCameraViewMatrix();
    uint3 renderTargetDimensions = (pRenderTargetView != NULL) ? GetTextureDimensions(pRenderTargetView->GetTargetTexture()) : uint3::One;

    return CalculateAABoxScissorRect(pScissorRect, Bounds, viewMatrix, renderTargetDimensions.x, renderTargetDimensions.y);
}

bool Renderer::CalculatePointLightScissorRect(RENDERER_SCISSOR_RECT *pScissorRect, const float3 &LightPosition, const float &LightRange)
{
    GPURenderTargetView *pRenderTargetView;
    g_pRenderer->GetMainContext()->GetRenderTargets(1, &pRenderTargetView, nullptr);

    const float4x4 &viewMatrix = g_pRenderer->GetMainContext()->GetConstants()->GetCameraViewMatrix();
    uint3 renderTargetDimensions = (pRenderTargetView != NULL) ? GetTextureDimensions(pRenderTargetView->GetTargetTexture()) : uint3::One;

    return CalculatePointLightScissorRect(pScissorRect, LightPosition, LightRange, viewMatrix, renderTargetDimensions.x, renderTargetDimensions.y);
}

void Renderer::BuildIndexBufferContents(uint32 sourceVertexCount, const uint32 *pSourceIndices, uint32 nIndicesPerPrimitive, uint32 nPrimitives, uint32 sourceIndicesStride, void **ppOutBuffer, GPU_INDEX_FORMAT *pOutIndexFormat, uint32 *pOutIndexCount)
{
    uint32 nIndices = nIndicesPerPrimitive * nPrimitives;

    GPU_INDEX_FORMAT indexFormat;
    void *pOutIndices;
    uint32 cbOutIndices;
    if (sourceVertexCount <= Y_UINT16_MAX)
    {
        indexFormat = GPU_INDEX_FORMAT_UINT16;
        pOutIndices = Y_mallocT<uint16>(nIndices);
        cbOutIndices = sizeof(uint16) * nIndices;
    }
    else
    {
        indexFormat = GPU_INDEX_FORMAT_UINT32;
        pOutIndices = Y_mallocT<uint32>(nIndices);
        cbOutIndices = sizeof(uint32) * nIndices;
    }

    FillIndexBufferContents(pSourceIndices, nIndicesPerPrimitive, nPrimitives, sourceIndicesStride, indexFormat, pOutIndices, cbOutIndices);

    *ppOutBuffer = pOutIndices;
    *pOutIndexFormat = indexFormat;
    *pOutIndexCount = nIndices;
}

void Renderer::FillIndexBufferContents(const uint32 *pSourceIndices, uint32 nIndicesPerPrimitive, uint32 nPrimitives, uint32 sourceIndicesStride, GPU_INDEX_FORMAT indexFormat, void *pDestinationIndices, uint32 cbDestinationIndices)
{
    const byte *pSourcePtr = reinterpret_cast<const byte *>(pSourceIndices);

    if (indexFormat == GPU_INDEX_FORMAT_UINT16)
    {
        uint16 *pOutPtr = reinterpret_cast<uint16 *>(pDestinationIndices);
        Assert(cbDestinationIndices >= (sizeof(uint16) * nIndicesPerPrimitive * nPrimitives));

        for (uint32 i = 0; i < nPrimitives; i++)
        {
            const uint32 *pCurrentPrimitive = reinterpret_cast<const uint32 *>(pSourcePtr);
            for (uint32 j = 0; j < nIndicesPerPrimitive; j++)
                *(pOutPtr++) = static_cast<uint16>(*(pCurrentPrimitive++));

            pSourcePtr += sourceIndicesStride;
        }
    }
    else
    {
        uint32 *pOutPtr = reinterpret_cast<uint32 *>(pDestinationIndices);
        Assert(cbDestinationIndices >= (sizeof(uint32) * nIndicesPerPrimitive * nPrimitives));

        for (uint32 i = 0; i < nPrimitives; i++)
        {
            const uint32 *pCurrentPrimitive = reinterpret_cast<const uint32 *>(pSourcePtr);
            for (uint32 j = 0; j < nIndicesPerPrimitive; j++)
                *(pOutPtr++) = *(pCurrentPrimitive++);

            pSourcePtr += sourceIndicesStride;
        }
    }
}

float2 Renderer::GetClipSpaceCoordinatesForPixelCoordinates(int32 x, int32 y, uint32 windowWidth, uint32 windowHeight)
{
    DebugAssert(windowWidth > 1 && windowHeight > 1);

    float outX = (float)x / (float)(windowWidth - 1);
    float outY = (float)y / (float)(windowHeight - 1);
    
    outX *= 2.0f;
    outY *= 2.0f;
    outX -= 1.0f;
    outY = 1.0f - outY;
    
    return float2(outX, outY);
}

float2 Renderer::GetTextureSpaceCoordinatesForPixelCoordinates(int32 x, int32 y, uint32 textureWidth, uint32 textureHeight)
{
    DebugAssert(textureWidth > 1 && textureHeight > 1);

    float outX = (float)x / (float)(textureWidth - 1);
    float outY = (float)y / (float)(textureHeight - 1);

    return float2(outX, outY);
}

Renderer::FixedResources::FixedResources(Renderer *pRenderer)
    : m_pRenderer(pRenderer),
      m_pBlendStateNoBlending(nullptr),
      m_pBlendStateNoColorWrites(nullptr),
      m_pBlendStateAdditive(nullptr),
      m_pBlendStateAlphaBlending(nullptr),
      m_pBlendStateAlphaBlendingAdditive(nullptr),
      m_pBlendStatePremultipliedAlpha(nullptr),
      m_pBlendStatePremultipliedAlphaAdditive(nullptr),
      m_pBlendStateBlendFactor(nullptr),
      m_pBlendStateBlendFactorAdditive(nullptr),
      m_pBlendStateBlendFactorPremultipliedAlpha(nullptr),
      m_pBlendStateApplyBlendFactorAlpha(nullptr),
      m_pPointSamplerState(nullptr),
      m_pLinearSamplerState(nullptr),
      m_pLinearMipSamplerState(nullptr),
      m_pDebugFont(nullptr),
      m_pNoiseTexture(nullptr),
      m_pRandomTexture(nullptr),
      m_pTextureBlitShader(nullptr),
      m_pTextureBlitLODShader(nullptr),
      m_pDownsampleShader(nullptr),
      m_pFullScreenQuadVertexBuffer(nullptr),
      m_pOverlayShaderColoredScreen(nullptr),
      m_pOverlayShaderColoredWorld(nullptr),
      m_pOverlayShaderTexturedScreen(nullptr),
      m_pOverlayShaderTexturedWorld(nullptr)
{
    Y_memzero(m_pRasterizerStates, sizeof(m_pRasterizerStates));
    Y_memzero(m_pDepthStencilStates, sizeof(m_pDepthStencilStates));
}

Renderer::FixedResources::~FixedResources()
{
    for (uint32 fillMode = 0; fillMode < RENDERER_FILL_MODE_COUNT; fillMode++) {
        for (uint32 cullMode = 0; cullMode < RENDERER_CULL_MODE_COUNT; cullMode++) {
            for (uint32 depthBiasFlag = 0; depthBiasFlag < 2; depthBiasFlag++) {
                for (uint32 depthClampFlag = 0; depthClampFlag < 2; depthClampFlag++) {
                    for (uint32 scissorFlag = 0; scissorFlag < 2; scissorFlag++) {
                        DebugAssert(m_pRasterizerStates[fillMode][cullMode][depthBiasFlag][depthClampFlag][scissorFlag] == nullptr);
                    }
                }
            }
        }
    }

    for (uint32 depthTestFlag = 0; depthTestFlag < 2; depthTestFlag++) {
        for (uint32 depthWriteFlag = 0; depthWriteFlag < 2; depthWriteFlag++) {
            for (uint32 comparisonFunc = 0; comparisonFunc < GPU_COMPARISON_FUNC_COUNT; comparisonFunc++) {
                DebugAssert(m_pDepthStencilStates[depthTestFlag][depthWriteFlag][comparisonFunc] == nullptr);
            }
        }
    }  

    DebugAssert(m_pBlendStateNoBlending == nullptr);
    DebugAssert(m_pBlendStateNoColorWrites == nullptr);
    DebugAssert(m_pBlendStateAdditive == nullptr);
    DebugAssert(m_pBlendStateAlphaBlending == nullptr);
    DebugAssert(m_pBlendStateAlphaBlendingAdditive == nullptr);
    DebugAssert(m_pBlendStatePremultipliedAlpha == nullptr);
    DebugAssert(m_pBlendStatePremultipliedAlphaAdditive == nullptr);
    DebugAssert(m_pBlendStateBlendFactor == nullptr);
    DebugAssert(m_pBlendStateBlendFactorAdditive == nullptr);
    DebugAssert(m_pBlendStateBlendFactorPremultipliedAlpha == nullptr);
    DebugAssert(m_pBlendStateApplyBlendFactorAlpha == nullptr);

    DebugAssert(m_pPointSamplerState == nullptr);
    DebugAssert(m_pLinearSamplerState == nullptr);
    DebugAssert(m_pLinearMipSamplerState == nullptr);

    DebugAssert(m_pDebugFont == nullptr);
    DebugAssert(m_pNoiseTexture == nullptr);
    DebugAssert(m_pRandomTexture == nullptr);

    DebugAssert(m_pTextureBlitShader == nullptr);
    DebugAssert(m_pTextureBlitLODShader == nullptr);
    DebugAssert(m_pDownsampleShader == nullptr);

    DebugAssert(m_pFullScreenQuadVertexBuffer == nullptr);
}

GPURasterizerState *Renderer::FixedResources::GetRasterizerState(RENDERER_FILL_MODE fillMode /*= RENDERER_FILL_SOLID*/, RENDERER_CULL_MODE cullMode /*= RENDERER_CULL_BACK*/, bool depthBias /*= false*/, bool depthClamping /*= false*/, bool scissorTest /*= false*/)
{
    DebugAssert(IsOnRenderThread());

    GPURasterizerState *pRasterizerState = m_pRasterizerStates[fillMode][cullMode][depthBias][depthClamping][scissorTest];
    if (pRasterizerState == nullptr)
    {
        RENDERER_RASTERIZER_STATE_DESC rasterizerStateDesc;
        Renderer::FillDefaultRasterizerState(&rasterizerStateDesc);

        rasterizerStateDesc.FillMode = (RENDERER_FILL_MODE)fillMode;
        rasterizerStateDesc.CullMode = (RENDERER_CULL_MODE)cullMode;
        rasterizerStateDesc.DepthBias = (depthBias) ? -1000 : 0;
        rasterizerStateDesc.DepthClipEnable = (depthClamping) ? false : true;
        rasterizerStateDesc.ScissorEnable = (scissorTest) ? true : false;

        if ((pRasterizerState = m_pRenderer->CreateRasterizerState(&rasterizerStateDesc)) == nullptr)
        {
            Log_ErrorPrintf("Renderer::FixedResources::GetRasterizerState: Rasterizer state failed creation");
            return nullptr;
        }

        m_pRasterizerStates[fillMode][cullMode][depthBias][depthClamping][scissorTest] = pRasterizerState;
    }

    return pRasterizerState;
}

GPUDepthStencilState *Renderer::FixedResources::GetDepthStencilState(bool depthTestEnabled /*= true*/, bool depthWriteEnabled /*= true*/, GPU_COMPARISON_FUNC comparisonFunc /*= GPU_COMPARISON_FUNC_LESS*/)
{
    DebugAssert(IsOnRenderThread());

    GPUDepthStencilState *pDepthStencilState = m_pDepthStencilStates[depthTestEnabled][depthWriteEnabled][comparisonFunc];
    if (pDepthStencilState == nullptr)
    {
        RENDERER_DEPTHSTENCIL_STATE_DESC depthStencilStateDesc;
        Renderer::FillDefaultDepthStencilState(&depthStencilStateDesc);

        depthStencilStateDesc.DepthTestEnable = depthTestEnabled;
        depthStencilStateDesc.DepthWriteEnable = depthWriteEnabled;
        depthStencilStateDesc.DepthFunc = comparisonFunc;

        if ((pDepthStencilState = m_pRenderer->CreateDepthStencilState(&depthStencilStateDesc)) == nullptr)
        {
            Log_ErrorPrintf("Renderer::FixedResources::GetDepthStencilState: DepthStencil state failed creation");
            return nullptr;
        }

        m_pDepthStencilStates[depthTestEnabled][depthWriteEnabled][comparisonFunc] = pDepthStencilState;
    }

    return pDepthStencilState;
}



const GPU_VERTEX_ELEMENT_DESC *Renderer::FixedResources::GetPositionOnlyVertexAttributes()
{
    static const GPU_VERTEX_ELEMENT_DESC attributes[] =
    {
        { GPU_VERTEX_ELEMENT_SEMANTIC_POSITION, 0, GPU_VERTEX_ELEMENT_TYPE_FLOAT3, 0, 0, 0 },
    };

    return attributes;
}

const uint32 Renderer::FixedResources::GetPositionOnlyVertexAttributeCount()
{
    // has to match above!
    return 1;
}

const GPU_VERTEX_ELEMENT_DESC *Renderer::FixedResources::GetFullScreenQuadVertexAttributes()
{
    static const GPU_VERTEX_ELEMENT_DESC fullscreenQuadAttributes[] =
    {
        { GPU_VERTEX_ELEMENT_SEMANTIC_POSITION, 0, GPU_VERTEX_ELEMENT_TYPE_FLOAT4, 0, offsetof(ScreenQuadVertex, Position), 0 },
        { GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD, 0, GPU_VERTEX_ELEMENT_TYPE_FLOAT2, 0, offsetof(ScreenQuadVertex, TexCoord), 0 }
    };

    return fullscreenQuadAttributes;
}

const uint32 Renderer::FixedResources::GetFullScreenQuadVertexAttributeCount()
{
    // has to match above!
    return 2;
}

bool Renderer::FixedResources::CreateResources()
{
#if 0
    // rasterizer
    {
        // rs[wireframe][culling][depthbias][depthclip][scissor]
        RENDERER_RASTERIZER_STATE_DESC rasterizerStateDesc;
        Renderer::FillDefaultRasterizerState(&rasterizerStateDesc);

        // wireframe
        for (uint32 fillMode = 0; fillMode < RENDERER_FILL_MODE_COUNT; fillMode++)
        {
            rasterizerStateDesc.FillMode = (RENDERER_FILL_MODE)fillMode;

            // culling
            for (uint32 cullMode = 0; cullMode < RENDERER_CULL_MODE_COUNT; cullMode++)
            {
                rasterizerStateDesc.CullMode = (RENDERER_CULL_MODE)cullMode;

                // depthbias
                for (uint32 depthBiasFlag = 0; depthBiasFlag < 2; depthBiasFlag++)
                {
                    rasterizerStateDesc.DepthBias = (depthBiasFlag == 0) ? 0 : 1;

                    // depthclip
                    for (uint32 depthClampFlag = 0; depthClampFlag < 2; depthClampFlag++)
                    {
                        rasterizerStateDesc.DepthClipEnable = (depthClampFlag == 0) ? TRUE : FALSE;

                        // scissor
                        for (uint32 scissorFlag = 0; scissorFlag < 2; scissorFlag++)
                        {
                            rasterizerStateDesc.ScissorEnable = (scissorFlag == 0) ? FALSE : TRUE;

                            if ((m_pRasterizerStates[fillMode][cullMode][depthBiasFlag][depthClampFlag][scissorFlag] = pRenderer->CreateRasterizerState(&rasterizerStateDesc)) == nullptr)
                                return false;
                        }
                    }
                }
            }
        }
        // normal
        
        if ((m_pRasterizerStateNormal = pRenderer->CreateRasterizerState(&rasterizerStateDesc)) == NULL)
            return false;

        // nocull
        Renderer::FillDefaultRasterizerState(&rasterizerStateDesc);
        rasterizerStateDesc.FillMode = RENDERER_FILL_SOLID;
        rasterizerStateDesc.CullMode = RENDERER_CULL_NONE;
        if ((m_pRasterizerStateNoCull = pRenderer->CreateRasterizerState(&rasterizerStateDesc)) == NULL)
            return false;

        // 1 depthbias, normal
        Renderer::FillDefaultRasterizerState(&rasterizerStateDesc);
        rasterizerStateDesc.DepthBias = 16;
        rasterizerStateDesc.SlopeScaledDepthBias = 16.0f;
        if ((m_pRasterizerStateOneDepthBias = pRenderer->CreateRasterizerState(&rasterizerStateDesc)) == NULL)
            return false;

        // wireframe
        Renderer::FillDefaultRasterizerState(&rasterizerStateDesc);
        rasterizerStateDesc.FillMode = RENDERER_FILL_WIREFRAME;
        if ((m_pRasterizerStateWireframe = pRenderer->CreateRasterizerState(&rasterizerStateDesc)) == NULL)
            return false;

        // wireframe, depth bias
        Renderer::FillDefaultRasterizerState(&rasterizerStateDesc);
        rasterizerStateDesc.FillMode = RENDERER_FILL_WIREFRAME;
        rasterizerStateDesc.DepthBias = 1;
        if ((m_pRasterizerStateWireframeOneDepthBias = pRenderer->CreateRasterizerState(&rasterizerStateDesc)) == NULL)
            return false;

        // wireframe no cull
        Renderer::FillDefaultRasterizerState(&rasterizerStateDesc);
        rasterizerStateDesc.FillMode = RENDERER_FILL_WIREFRAME;
        rasterizerStateDesc.CullMode = RENDERER_CULL_NONE;
        if ((m_pRasterizerStateWireframeNoCull = pRenderer->CreateRasterizerState(&rasterizerStateDesc)) == NULL)
            return false;

        // scissor
        Renderer::FillDefaultRasterizerState(&rasterizerStateDesc);
        rasterizerStateDesc.ScissorEnable = true;
        if ((m_pRasterizerStateScissor = pRenderer->CreateRasterizerState(&rasterizerStateDesc)) == NULL)
            return false;
    }

    // depthstencil
    {
        RENDERER_DEPTHSTENCIL_STATE_DESC DepthStencilStateDesc;

        // less
        Renderer::FillDefaultDepthStencilState(&DepthStencilStateDesc);
        if ((m_pDepthStencilStateLess = pRenderer->CreateDepthStencilState(&DepthStencilStateDesc)) == NULL)
            return false;

        // less + no writes
        Renderer::FillDefaultDepthStencilState(&DepthStencilStateDesc);
        DepthStencilStateDesc.DepthWriteEnable = false;
        if ((m_pDepthStencilStateLessNoWrite = pRenderer->CreateDepthStencilState(&DepthStencilStateDesc)) == NULL)
            return false;

        // lessequal
        Renderer::FillDefaultDepthStencilState(&DepthStencilStateDesc);
        DepthStencilStateDesc.DepthFunc = GPU_COMPARISON_FUNC_LESS_EQUAL;
        if ((m_pDepthStencilStateLessEqual = pRenderer->CreateDepthStencilState(&DepthStencilStateDesc)) == NULL)
            return false;

        // lessequal + no writes
        Renderer::FillDefaultDepthStencilState(&DepthStencilStateDesc);
        DepthStencilStateDesc.DepthFunc = GPU_COMPARISON_FUNC_LESS_EQUAL;
        DepthStencilStateDesc.DepthWriteEnable = false;
        if ((m_pDepthStencilStateLessEqualNoWrite = pRenderer->CreateDepthStencilState(&DepthStencilStateDesc)) == NULL)
            return false;

        // equal
        Renderer::FillDefaultDepthStencilState(&DepthStencilStateDesc);
        DepthStencilStateDesc.DepthFunc = GPU_COMPARISON_FUNC_EQUAL;
        DepthStencilStateDesc.DepthWriteEnable = false;
        if ((m_pDepthStencilStateEqual = pRenderer->CreateDepthStencilState(&DepthStencilStateDesc)) == NULL)
            return false;

        // notests
        Renderer::FillDefaultDepthStencilState(&DepthStencilStateDesc);
        DepthStencilStateDesc.DepthTestEnable = false;
        DepthStencilStateDesc.DepthWriteEnable = false;
        if ((m_pDepthStencilStateNoTest = pRenderer->CreateDepthStencilState(&DepthStencilStateDesc)) == NULL)
            return false;
    }

#endif

    // get some more common states
    GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false, false);
    GetDepthStencilState(false, false);
    GetDepthStencilState(true, true);

    // blend
    {
        RENDERER_BLEND_STATE_DESC blendStateDesc;

        // noblending
        Renderer::FillDefaultBlendState(&blendStateDesc);
        if ((m_pBlendStateNoBlending = m_pRenderer->CreateBlendState(&blendStateDesc)) == nullptr)
            return false;

        // nocolorwrites
        Renderer::FillDefaultBlendState(&blendStateDesc);
        blendStateDesc.ColorWriteEnable = false;
        if ((m_pBlendStateNoColorWrites = m_pRenderer->CreateBlendState(&blendStateDesc)) == nullptr)
            return false;

        // additive
        Renderer::FillDefaultBlendState(&blendStateDesc);
        blendStateDesc.BlendEnable = true;
        blendStateDesc.SrcBlend = RENDERER_BLEND_ONE;
        blendStateDesc.BlendOp = RENDERER_BLEND_OP_ADD;
        blendStateDesc.DestBlend = RENDERER_BLEND_ONE;
        blendStateDesc.SrcBlendAlpha = RENDERER_BLEND_ONE;
        blendStateDesc.BlendOp = RENDERER_BLEND_OP_ADD;
        blendStateDesc.DestBlendAlpha = RENDERER_BLEND_ONE;
        if ((m_pBlendStateAdditive = m_pRenderer->CreateBlendState(&blendStateDesc)) == nullptr)
            return false;

        // srcalpha + invsrcalpha
        Renderer::FillDefaultBlendState(&blendStateDesc);
        blendStateDesc.BlendEnable = true;
        blendStateDesc.SrcBlend = RENDERER_BLEND_SRC_ALPHA;
        blendStateDesc.SrcBlendAlpha = RENDERER_BLEND_ONE;
        blendStateDesc.BlendOp = RENDERER_BLEND_OP_ADD;
        blendStateDesc.DestBlend = RENDERER_BLEND_INV_SRC_ALPHA;
        blendStateDesc.DestBlendAlpha = RENDERER_BLEND_INV_SRC_ALPHA;
        if ((m_pBlendStateAlphaBlending = m_pRenderer->CreateBlendState(&blendStateDesc)) == nullptr)
            return false;

        // srcalpha + dstcolor
        Renderer::FillDefaultBlendState(&blendStateDesc);
        blendStateDesc.BlendEnable = true;
        blendStateDesc.SrcBlend = RENDERER_BLEND_SRC_ALPHA;
        blendStateDesc.SrcBlendAlpha = RENDERER_BLEND_ZERO;
        blendStateDesc.BlendOp = RENDERER_BLEND_OP_ADD;
        blendStateDesc.DestBlend = RENDERER_BLEND_ONE;
        blendStateDesc.DestBlendAlpha = RENDERER_BLEND_ONE;
        if ((m_pBlendStateAlphaBlendingAdditive = m_pRenderer->CreateBlendState(&blendStateDesc)) == nullptr)
            return false;

        // premultiplied alpha
        Renderer::FillDefaultBlendState(&blendStateDesc);
        blendStateDesc.BlendEnable = true;
        blendStateDesc.SrcBlend = RENDERER_BLEND_ONE;
        blendStateDesc.SrcBlendAlpha = RENDERER_BLEND_ONE;
        blendStateDesc.BlendOp = RENDERER_BLEND_OP_ADD;
        blendStateDesc.DestBlend = RENDERER_BLEND_INV_SRC_ALPHA;
        blendStateDesc.DestBlendAlpha = RENDERER_BLEND_INV_SRC_ALPHA;
        if ((m_pBlendStatePremultipliedAlpha = m_pRenderer->CreateBlendState(&blendStateDesc)) == nullptr)
            return false;

        // premultiplied alpha additive
        Renderer::FillDefaultBlendState(&blendStateDesc);
        blendStateDesc.BlendEnable = true;
        blendStateDesc.SrcBlend = RENDERER_BLEND_ONE;
        blendStateDesc.SrcBlendAlpha = RENDERER_BLEND_ONE;
        blendStateDesc.BlendOp = RENDERER_BLEND_OP_ADD;
        blendStateDesc.DestBlend = RENDERER_BLEND_ONE;
        blendStateDesc.DestBlendAlpha = RENDERER_BLEND_ONE;
        if ((m_pBlendStatePremultipliedAlphaAdditive = m_pRenderer->CreateBlendState(&blendStateDesc)) == nullptr)
            return false;

        // blend factor
        Renderer::FillDefaultBlendState(&blendStateDesc);
        blendStateDesc.BlendEnable = true;
        blendStateDesc.SrcBlend = RENDERER_BLEND_BLEND_FACTOR;
        blendStateDesc.BlendOp = RENDERER_BLEND_OP_ADD;
        blendStateDesc.DestBlend = RENDERER_BLEND_ZERO;
        blendStateDesc.SrcBlendAlpha = RENDERER_BLEND_BLEND_FACTOR;
        blendStateDesc.BlendOpAlpha = RENDERER_BLEND_OP_ADD;
        blendStateDesc.DestBlendAlpha = RENDERER_BLEND_ZERO;
        if ((m_pBlendStateBlendFactor = m_pRenderer->CreateBlendState(&blendStateDesc)) == nullptr)
            return false;

        // blend factor additive
        Renderer::FillDefaultBlendState(&blendStateDesc);
        blendStateDesc.BlendEnable = true;
        blendStateDesc.SrcBlend = RENDERER_BLEND_BLEND_FACTOR;
        blendStateDesc.BlendOp = RENDERER_BLEND_OP_ADD;
        blendStateDesc.DestBlend = RENDERER_BLEND_ONE;
        blendStateDesc.SrcBlendAlpha = RENDERER_BLEND_BLEND_FACTOR;
        blendStateDesc.BlendOpAlpha = RENDERER_BLEND_OP_ADD;
        blendStateDesc.DestBlendAlpha = RENDERER_BLEND_ONE;
        if ((m_pBlendStateBlendFactorAdditive = m_pRenderer->CreateBlendState(&blendStateDesc)) == nullptr)
            return false;

        // blend factor premultiplied alpha
        Renderer::FillDefaultBlendState(&blendStateDesc);
        blendStateDesc.BlendEnable = true;
        blendStateDesc.SrcBlend = RENDERER_BLEND_BLEND_FACTOR;
        blendStateDesc.BlendOp = RENDERER_BLEND_OP_ADD;
        blendStateDesc.DestBlend = RENDERER_BLEND_INV_SRC_ALPHA;
        blendStateDesc.SrcBlendAlpha = RENDERER_BLEND_BLEND_FACTOR;
        blendStateDesc.BlendOpAlpha = RENDERER_BLEND_OP_ADD;
        blendStateDesc.DestBlendAlpha = RENDERER_BLEND_INV_SRC_ALPHA;
        if ((m_pBlendStateBlendFactorPremultipliedAlpha = m_pRenderer->CreateBlendState(&blendStateDesc)) == nullptr)
            return false;

        // blend factor apply blend factor alpha
        Renderer::FillDefaultBlendState(&blendStateDesc);
        blendStateDesc.BlendEnable = true;
        blendStateDesc.SrcBlend = RENDERER_BLEND_BLEND_FACTOR;
        blendStateDesc.BlendOp = RENDERER_BLEND_OP_ADD;
        blendStateDesc.DestBlend = RENDERER_BLEND_INV_BLEND_FACTOR;
        blendStateDesc.SrcBlendAlpha = RENDERER_BLEND_BLEND_FACTOR;
        blendStateDesc.BlendOpAlpha = RENDERER_BLEND_OP_ADD;
        blendStateDesc.DestBlendAlpha = RENDERER_BLEND_INV_BLEND_FACTOR;
        if ((m_pBlendStateApplyBlendFactorAlpha = m_pRenderer->CreateBlendState(&blendStateDesc)) == nullptr)
            return false;
    }

    // sampler, only generate on SM4+
    if (m_pRenderer->GetFeatureLevel() >= RENDERER_FEATURE_LEVEL_SM4)
    {
        GPU_SAMPLER_STATE_DESC samplerStateDesc;
        
        // point
        samplerStateDesc.Set(TEXTURE_FILTER_MIN_MAG_MIP_POINT, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, float4::Zero, 0.0f, 0, 0, 1, GPU_COMPARISON_FUNC_ALWAYS);
        if ((m_pPointSamplerState = m_pRenderer->CreateSamplerState(&samplerStateDesc)) == nullptr)
            return false;

        // linear
        samplerStateDesc.Set(TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, float4::Zero, 0.0f, 0, 0, 1, GPU_COMPARISON_FUNC_ALWAYS);
        if ((m_pLinearSamplerState = m_pRenderer->CreateSamplerState(&samplerStateDesc)) == nullptr)
            return false;

        // linear/mips
        samplerStateDesc.Set(TEXTURE_FILTER_MIN_MAG_MIP_LINEAR, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, float4::Zero, 0.0f, 0, 0, 1, GPU_COMPARISON_FUNC_ALWAYS);
        if ((m_pLinearMipSamplerState = m_pRenderer->CreateSamplerState(&samplerStateDesc)) == nullptr)
            return false;
    }

    // resources
    {
        // debug font
        if ((m_pDebugFont = g_pResourceManager->GetFont(g_pEngine->GetRendererDebugFontName())) == nullptr)
        {
            Log_WarningPrintf("RendererFixedResources::CreateResources: Failed to load renderer debug font (%s), using default font.", g_pEngine->GetRendererDebugFontName().GetCharArray());
            m_pDebugFont = g_pEngine->GetDefaultFont();
        }

        // noise texture
        if ((m_pNoiseTexture = g_pResourceManager->GetTexture2D("textures/engine/noise")) == nullptr)
        {
            Log_ErrorPrintf("RendererFixedResources::CreateResources: Failed to load noise texture");
            return false;
        }

        // random texture
        if ((m_pRandomTexture = g_pResourceManager->GetTexture2D("textures/engine/random")) == nullptr)
        {
            Log_ErrorPrintf("RendererFixedResources::CreateResources: Failed to load random texture");
            return false;
        }

        // fullscreen quad buffer

        {
            static const ScreenQuadVertex vertices[4] =
            {
                { { -1.0f, 1.0f }, { 0.0f, 0.0f } },
                { { -1.0f, -1.0f }, { 0.0f, 1.0f } },
                { { 1.0f, 1.0f }, { 1.0f, 0.0f } },     // CCW, move up one for CW
                { { 1.0f, -1.0f }, { 1.0f, 1.0f } }
            };

            GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER, sizeof(vertices));
            if ((m_pFullScreenQuadVertexBuffer = m_pRenderer->CreateBuffer(&bufferDesc, vertices)) == nullptr)
            {
                Log_ErrorPrintf("RendererFixedResources::CreateResources: Failed to create fullscreen quad vertex buffer.");
                return false;
            }

#ifdef Y_BUILD_CONFIG_DEBUG
            m_pFullScreenQuadVertexBuffer->SetDebugName("Static FullScreen Quad");
#endif
        }

        // texture blit shader
        if ((m_pTextureBlitShader = m_pRenderer->GetShaderProgram(0, SHADER_COMPONENT_INFO(TextureBlitShader), 0, GetFullScreenQuadVertexAttributes(), GetFullScreenQuadVertexAttributeCount(), nullptr, 0)) == nullptr ||
            (m_pRenderer->GetFeatureLevel() >= RENDERER_FEATURE_LEVEL_ES3 && (m_pTextureBlitLODShader = m_pRenderer->GetShaderProgram(0, SHADER_COMPONENT_INFO(TextureBlitShader), TextureBlitShader::USE_TEXTURE_LOD, GetFullScreenQuadVertexAttributes(), GetFullScreenQuadVertexAttributeCount(), nullptr, 0)) == nullptr))
        {
            Log_ErrorPrintf("RendererFixedResources::CreateResources: Failed to create texture blit shader.");
            return false;
        }

        // overlay shaders
        {
            static const GPU_VERTEX_ELEMENT_DESC attributes2D[] =
            {
                { GPU_VERTEX_ELEMENT_SEMANTIC_POSITION, 0, GPU_VERTEX_ELEMENT_TYPE_FLOAT2, 0, offsetof(OverlayShader::Vertex2D, Position), 0 },
                { GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD, 0, GPU_VERTEX_ELEMENT_TYPE_FLOAT2, 0, offsetof(OverlayShader::Vertex2D, TexCoord), 0 },
                { GPU_VERTEX_ELEMENT_SEMANTIC_COLOR, 0, GPU_VERTEX_ELEMENT_TYPE_UNORM4, 0, offsetof(OverlayShader::Vertex2D, Color), 0 }
            };

            static const GPU_VERTEX_ELEMENT_DESC attributes3D[] =
            {
                { GPU_VERTEX_ELEMENT_SEMANTIC_POSITION, 0, GPU_VERTEX_ELEMENT_TYPE_FLOAT3, 0, offsetof(OverlayShader::Vertex3D, Position), 0 },
                { GPU_VERTEX_ELEMENT_SEMANTIC_TEXCOORD, 0, GPU_VERTEX_ELEMENT_TYPE_FLOAT2, 0, offsetof(OverlayShader::Vertex3D, TexCoord), 0 },
                { GPU_VERTEX_ELEMENT_SEMANTIC_COLOR, 0, GPU_VERTEX_ELEMENT_TYPE_UNORM4, 0, offsetof(OverlayShader::Vertex3D, Color), 0 }
            };

            if ((m_pOverlayShaderColoredScreen = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(OverlayShader), 0, attributes2D, countof(attributes2D), nullptr, 0)) == nullptr ||
                (m_pOverlayShaderColoredWorld = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(OverlayShader), OverlayShader::WITH_3D_VERTEX, attributes3D, countof(attributes3D), nullptr, 0)) == nullptr ||
                (m_pOverlayShaderTexturedScreen = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(OverlayShader), OverlayShader::WITH_TEXTURE, attributes2D, countof(attributes2D), nullptr, 0)) == nullptr ||
                (m_pOverlayShaderTexturedWorld = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(OverlayShader), OverlayShader::WITH_TEXTURE | OverlayShader::WITH_3D_VERTEX, attributes3D, countof(attributes3D), nullptr, 0)) == nullptr)
            {
                Log_ErrorPrintf("RendererFixedResources::CreateResources: Failed to create overlay shaders.");
                return false;
            }
        }

        // only on SM4+
        if (m_pRenderer->GetFeatureLevel() >= RENDERER_FEATURE_LEVEL_SM4)
        {
            // downsample shader
            if ((m_pDownsampleShader = m_pRenderer->GetShaderProgram(0, SHADER_COMPONENT_INFO(DownsampleShader), 0, GetFullScreenQuadVertexAttributes(), GetFullScreenQuadVertexAttributeCount(), nullptr, 0)) == nullptr)
            {
                Log_ErrorPrintf("RendererFixedResources::CreateResources: Failed to create DownsampleShader.");
                return false;
            }
        }
    }

    return true;
}

void Renderer::FixedResources::ReleaseResources()
{
    for (uint32 fillMode = 0; fillMode < RENDERER_FILL_MODE_COUNT; fillMode++) {
        for (uint32 cullMode = 0; cullMode < RENDERER_CULL_MODE_COUNT; cullMode++) {
            for (uint32 depthBiasFlag = 0; depthBiasFlag < 2; depthBiasFlag++) {
                for (uint32 depthClampFlag = 0; depthClampFlag < 2; depthClampFlag++) {
                    for (uint32 scissorFlag = 0; scissorFlag < 2; scissorFlag++) {
                        SAFE_RELEASE(m_pRasterizerStates[fillMode][cullMode][depthBiasFlag][depthClampFlag][scissorFlag]);
                    }
                }
            }
        }
    }

    for (uint32 depthTestFlag = 0; depthTestFlag < 2; depthTestFlag++) {
        for (uint32 depthWriteFlag = 0; depthWriteFlag < 2; depthWriteFlag++) {
            for (uint32 comparisonFunc = 0; comparisonFunc < GPU_COMPARISON_FUNC_COUNT; comparisonFunc++) {
                SAFE_RELEASE(m_pDepthStencilStates[depthTestFlag][depthWriteFlag][comparisonFunc]);
            }
        }
    }

    // Owned by ShaderMap hence null not release
    m_pOverlayShaderColoredScreen = nullptr;
    m_pOverlayShaderColoredWorld = nullptr;
    m_pOverlayShaderTexturedScreen = nullptr;
    m_pOverlayShaderTexturedWorld = nullptr;

    SAFE_RELEASE(m_pFullScreenQuadVertexBuffer);

    SAFE_RELEASE(m_pRandomTexture);
    SAFE_RELEASE(m_pNoiseTexture);
    SAFE_RELEASE(m_pDebugFont);

    SAFE_RELEASE(m_pLinearMipSamplerState);
    SAFE_RELEASE(m_pLinearSamplerState);
    SAFE_RELEASE(m_pPointSamplerState);

    SAFE_RELEASE(m_pBlendStateNoBlending);
    SAFE_RELEASE(m_pBlendStateNoColorWrites);
    SAFE_RELEASE(m_pBlendStateAdditive);
    SAFE_RELEASE(m_pBlendStateAlphaBlending);
    SAFE_RELEASE(m_pBlendStateAlphaBlendingAdditive);
    SAFE_RELEASE(m_pBlendStatePremultipliedAlpha);
    SAFE_RELEASE(m_pBlendStatePremultipliedAlphaAdditive);
    SAFE_RELEASE(m_pBlendStateBlendFactor);
    SAFE_RELEASE(m_pBlendStateBlendFactorAdditive);
    SAFE_RELEASE(m_pBlendStateBlendFactorPremultipliedAlpha);
    SAFE_RELEASE(m_pBlendStateApplyBlendFactorAlpha);

    // unset input layouts/shaders, these will be cleaned up by the cache list
    m_pTextureBlitShader = nullptr;
    m_pTextureBlitLODShader = nullptr;
    m_pDownsampleShader = nullptr;
}

// constant buffer declarations
DECLARE_SHADER_CONSTANT_BUFFER(cbObjectConstants);
DECLARE_SHADER_CONSTANT_BUFFER(cbViewConstants);
DECLARE_SHADER_CONSTANT_BUFFER(cbViewportConstants);
DECLARE_SHADER_CONSTANT_BUFFER(cbWorldConstants);

// constant buffer definitions
BEGIN_SHADER_CONSTANT_BUFFER(cbObjectConstants, "ObjectConstantsBuffer", "ObjectConstants", RENDERER_PLATFORM_COUNT, RENDERER_FEATURE_LEVEL_COUNT)
    SHADER_CONSTANT_BUFFER_FIELD("WorldMatrix", SHADER_PARAMETER_TYPE_FLOAT4X4, 1)
    SHADER_CONSTANT_BUFFER_FIELD("InverseWorldMatrix", SHADER_PARAMETER_TYPE_FLOAT4X4, 1)
    SHADER_CONSTANT_BUFFER_FIELD("MaterialTintColor", SHADER_PARAMETER_TYPE_FLOAT4, 1)
END_SHADER_CONSTANT_BUFFER(cbObjectConstants)
BEGIN_SHADER_CONSTANT_BUFFER(cbViewConstants, "ViewConstantsBuffer", "ViewConstants", RENDERER_PLATFORM_COUNT, RENDERER_FEATURE_LEVEL_COUNT)
    SHADER_CONSTANT_BUFFER_FIELD("ViewMatrix", SHADER_PARAMETER_TYPE_FLOAT4X4, 1)
    SHADER_CONSTANT_BUFFER_FIELD("InverseViewMatrix", SHADER_PARAMETER_TYPE_FLOAT4X4, 1)
    SHADER_CONSTANT_BUFFER_FIELD("ProjectionMatrix", SHADER_PARAMETER_TYPE_FLOAT4X4, 1)
    SHADER_CONSTANT_BUFFER_FIELD("InverseProjectionMatrix", SHADER_PARAMETER_TYPE_FLOAT4X4, 1)
    SHADER_CONSTANT_BUFFER_FIELD("ViewProjectionMatrix", SHADER_PARAMETER_TYPE_FLOAT4X4, 1)
    SHADER_CONSTANT_BUFFER_FIELD("InverseViewProjectionMatrix", SHADER_PARAMETER_TYPE_FLOAT4X4, 1)
    SHADER_CONSTANT_BUFFER_FIELD("ScreenProjectionMatrix", SHADER_PARAMETER_TYPE_FLOAT4X4, 1)
    SHADER_CONSTANT_BUFFER_FIELD("EyePosition", SHADER_PARAMETER_TYPE_FLOAT3, 1)
    SHADER_CONSTANT_BUFFER_FIELD("ZRatio", SHADER_PARAMETER_TYPE_FLOAT2, 1)
    SHADER_CONSTANT_BUFFER_FIELD("ZNear", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("ZFar", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("PerspectiveAspectRatio", SHADER_PARAMETER_TYPE_FLOAT, 1)
    SHADER_CONSTANT_BUFFER_FIELD("PerspectiveFOV", SHADER_PARAMETER_TYPE_FLOAT, 1)
END_SHADER_CONSTANT_BUFFER(cbViewConstants)
BEGIN_SHADER_CONSTANT_BUFFER(cbViewportConstants, "ViewportConstantsBuffer", "ViewportConstants", RENDERER_PLATFORM_COUNT, RENDERER_FEATURE_LEVEL_COUNT)
    SHADER_CONSTANT_BUFFER_FIELD("ViewportOffset", SHADER_PARAMETER_TYPE_FLOAT2, 1)
    SHADER_CONSTANT_BUFFER_FIELD("ViewportSize", SHADER_PARAMETER_TYPE_FLOAT2, 1)
    SHADER_CONSTANT_BUFFER_FIELD("ViewportOffsetFraction", SHADER_PARAMETER_TYPE_FLOAT2, 1)
    SHADER_CONSTANT_BUFFER_FIELD("InverseViewportSize", SHADER_PARAMETER_TYPE_FLOAT2, 1)
END_SHADER_CONSTANT_BUFFER(cbViewportConstants)
BEGIN_SHADER_CONSTANT_BUFFER(cbWorldConstants, "WorldConstantsBuffer", "WorldConstants", RENDERER_PLATFORM_COUNT, RENDERER_FEATURE_LEVEL_COUNT)
    SHADER_CONSTANT_BUFFER_FIELD("WorldTime", SHADER_PARAMETER_TYPE_FLOAT, 1)
END_SHADER_CONSTANT_BUFFER(cbWorldConstants)

GPUContextConstants::GPUContextConstants(GPUContext *pContext)
    : m_pContext(pContext),
      m_recalculateCombinedMatrices(false),
      m_recalculateViewportFractions(false)
{
    m_localToWorldMatrix.SetZero();
    m_materialTintColor.SetZero();
    m_cameraViewMatrix.SetZero();
    m_cameraProjectionMatrix.SetZero();
    m_cameraEyePosition.SetZero();
    m_viewportOffset.SetZero();
    m_viewportSize.SetZero();
    m_worldTime = 0.0f;
}

GPUContextConstants::~GPUContextConstants()
{

}

void GPUContextConstants::SetLocalToWorldMatrix(const float4x4 &rMatrix, bool Commit /*= true*/)
{
    if (Y_memcmp(&rMatrix, &m_localToWorldMatrix, sizeof(m_localToWorldMatrix)) == 0)
        return;

    // set
    m_localToWorldMatrix = rMatrix;

    // calculate the inverse (transpose will do)
    float4x4 worldToLocalMatrix(m_localToWorldMatrix.Inverse());

    // write constant buffers
    cbObjectConstants.SetFieldFloat4x4(m_pContext, 0, m_localToWorldMatrix, false);
    cbObjectConstants.SetFieldFloat4x4(m_pContext, 1, worldToLocalMatrix, false);

    // commit away
    if (Commit)
        CommitChanges();
}

void GPUContextConstants::SetMaterialTintColor(const float4 &tintColor, bool Commit /*= true*/)
{
    if (Y_memcmp(&tintColor, &m_materialTintColor, sizeof(m_localToWorldMatrix)) == 0)
        return;

    m_materialTintColor = tintColor;

    // write constant buffers
    cbObjectConstants.SetFieldFloat4(m_pContext, 2, m_materialTintColor, false);

    // commit away
    if (Commit)
        CommitChanges();
}

void GPUContextConstants::SetCameraViewMatrix(const float4x4 &rMatrix, bool Commit /*= true*/)
{
    if (Y_memcmp(&rMatrix, &m_cameraViewMatrix, sizeof(m_cameraViewMatrix)) == 0)
        return;

    m_cameraViewMatrix = rMatrix;
    m_recalculateCombinedMatrices = true;

    if (Commit)
        CommitChanges();
}

void GPUContextConstants::SetCameraProjectionMatrix(const float4x4 &rMatrix, bool Commit /*= true*/)
{
    if (Y_memcmp(&rMatrix, &m_cameraProjectionMatrix, sizeof(m_cameraProjectionMatrix)) == 0)
        return;

    m_cameraProjectionMatrix = rMatrix;
    m_recalculateCombinedMatrices = true;

    if (Commit)
        CommitChanges();
}

void GPUContextConstants::SetCameraEyePosition(const float3 &Position, bool Commit /*= true*/)
{
    if (Y_memcmp(&Position, &m_cameraEyePosition, sizeof(m_cameraEyePosition)) == 0)
        return;

    m_cameraEyePosition = Position;
    cbViewConstants.SetFieldFloat3(m_pContext, 7, Position, false);

    if (Commit)
        CommitChanges();
}

void GPUContextConstants::SetFromCamera(const Camera &camera, bool commit /*= true*/)
{
    // push view/projection matrices through, let the inverses be deferred for all we care
    SetCameraViewMatrix(camera.GetViewMatrix(), false);
    SetCameraProjectionMatrix(camera.GetProjectionMatrix(), false);
    SetCameraEyePosition(camera.GetPosition(), false);

    // calc z ratio uniform
    // http://www.humus.name/temp/Linearize%20depth.txt
    // https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/ with negated far plane distance
    //float2 zRatioValue((camera.GetFarPlaneDistance() / camera.GetNearPlaneDistance()), 1.0f - (camera.GetFarPlaneDistance() / camera.GetNearPlaneDistance()));
    float2 zRatioValue(camera.GetFarPlaneDistance() / (camera.GetFarPlaneDistance() - camera.GetNearPlaneDistance()), (camera.GetFarPlaneDistance() * camera.GetNearPlaneDistance()) / (camera.GetFarPlaneDistance() - camera.GetNearPlaneDistance()));

    // set the remaining fields directly to the constant buffer
    cbViewConstants.SetFieldFloat2(m_pContext, 8, zRatioValue, false);
    cbViewConstants.SetFieldFloat(m_pContext, 9, camera.GetNearPlaneDistance(), false);
    cbViewConstants.SetFieldFloat(m_pContext, 10, camera.GetFarPlaneDistance(), false);
    cbViewConstants.SetFieldFloat(m_pContext, 11, camera.GetPerspectiveAspect(), false);
    cbViewConstants.SetFieldFloat(m_pContext, 12, Math::DegreesToRadians(camera.GetPerspectiveFieldOfView()), false);

    // commit
    if (commit)
        CommitChanges();
}

void GPUContextConstants::SetViewportOffset(float offsetX, float offsetY, bool commit /*= true*/)
{
    float2 viewportOffset(offsetX, offsetY);
    if (Y_memcmp(&viewportOffset, &m_viewportOffset, sizeof(m_viewportOffset)) == 0)
        return;

    m_viewportOffset = viewportOffset;
    cbViewportConstants.SetFieldFloat2(m_pContext, 0, viewportOffset, false);
    m_recalculateViewportFractions = true;

    if (commit)
        CommitChanges();
}

void GPUContextConstants::SetViewportSize(float width, float height, bool commit /*= true*/)
{
    float2 viewportSize(width, height);
    if (Y_memcmp(&viewportSize, &m_viewportSize, sizeof(m_viewportSize)) == 0)
        return;

    m_viewportSize = viewportSize;
    cbViewportConstants.SetFieldFloat2(m_pContext, 1, viewportSize, false);
    m_recalculateViewportFractions = true;

    if (commit)
        CommitChanges();
}

void GPUContextConstants::SetWorldTime(float worldTime, bool commit /*= true*/)
{
    if (m_worldTime == worldTime)
        return;

    m_worldTime = worldTime;
    cbWorldConstants.SetFieldFloat(m_pContext, 0, worldTime, false);

    if (commit)
        CommitChanges();
}

void GPUContextConstants::CommitChanges()
{
    // commit object buffer
    cbObjectConstants.CommitChanges(m_pContext);

    // rebuild view constants
    if (m_recalculateCombinedMatrices)
    {
        // adjust projection matrix to renderer-specific attributes
        float4x4 adjustedProjectionMatrix(m_cameraProjectionMatrix);
        g_pRenderer->CorrectProjectionMatrix(adjustedProjectionMatrix);

        // calculate inverse matrices
        float4x4 inverseViewMatrix(m_cameraViewMatrix.Inverse());
        float4x4 inverseProjectionMatrix(m_cameraProjectionMatrix.Inverse());

        // calculate view/projection matrix and inverses
        float4x4 viewProjectionMatrix(adjustedProjectionMatrix * m_cameraViewMatrix);
        float4x4 inverseViewProjectionMatrix(viewProjectionMatrix.Inverse());// replace with view * proj?

        // set in constant buffers
        cbViewConstants.SetFieldFloat4x4(m_pContext, 0, m_cameraViewMatrix, false);
        cbViewConstants.SetFieldFloat4x4(m_pContext, 1, inverseViewMatrix, false);
        cbViewConstants.SetFieldFloat4x4(m_pContext, 2, adjustedProjectionMatrix, false);
        cbViewConstants.SetFieldFloat4x4(m_pContext, 3, inverseProjectionMatrix, false);
        cbViewConstants.SetFieldFloat4x4(m_pContext, 4, viewProjectionMatrix, false);
        cbViewConstants.SetFieldFloat4x4(m_pContext, 5, inverseViewProjectionMatrix, false);
        m_recalculateCombinedMatrices = false;
    }

    // rebuild viewport constants
    if (m_recalculateViewportFractions)
    {
        float2 vpSizeF((float)m_viewportSize.x, (float)m_viewportSize.y);
        float2 invVPSizeF(vpSizeF.Reciprocal());
        float2 offsetFraction(float2((float)m_viewportOffset.x, (float)m_viewportOffset.y) * invVPSizeF);
        cbViewportConstants.SetFieldFloat2(m_pContext, 2, offsetFraction, false);
        cbViewportConstants.SetFieldFloat2(m_pContext, 3, invVPSizeF, false);

        // create ortho projection matrix
        float texelOffset = g_pRenderer->GetTexelOffset();
        float4x4 screenProjectionMatrix(float4x4::MakeOrthographicOffCenterProjectionMatrix((float)0.0f, (float)m_viewportSize.x, (float)m_viewportSize.y, 0.0f, 0.0f, 1.0f) *
                                        float4x4::MakeTranslationMatrix(texelOffset, texelOffset, 0.0f));

        g_pRenderer->CorrectProjectionMatrix(screenProjectionMatrix);
        cbViewConstants.SetFieldFloat4x4(m_pContext, 6, screenProjectionMatrix, false);
        m_recalculateViewportFractions = false;
    }

    // commit view buffer
    cbViewConstants.CommitChanges(m_pContext);

    // commit viewport buffer
    cbViewportConstants.CommitChanges(m_pContext);

    // commit world buffer
    cbWorldConstants.CommitChanges(m_pContext);
}
