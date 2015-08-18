#include "OpenGLES2Renderer/PrecompiledHeader.h"
#include "OpenGLES2Renderer/OpenGLES2Renderer.h"
#include "OpenGLES2Renderer/OpenGLES2ConstantLibrary.h"
#include "Engine/EngineCVars.h"
Log_SetChannel(OpenGLES2Renderer);

// our gl debug callback
static void APIENTRY GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    struct IgnoredWarning { GLenum type; GLuint id; GLenum severity; };
    static const IgnoredWarning ignoredWarnings[] = 
    {
        //type                              id              severity
        //{ GL_DEBUG_TYPE_PERFORMANCE,        131154,         GL_DEBUG_SEVERITY_MEDIUM        },  // 131154 Pixel-path performance warning: Pixel transfer is synchronized with 3D rendering.
        { GL_DEBUG_TYPE_OTHER,              131185,         GL_DEBUG_SEVERITY_NOTIFICATION  },  // Buffer detailed info: Buffer object 5 (bound to GL_ARRAY_BUFFER_ARB, usage hint is GL_STATIC_DRAW) will use VIDEO memory as the source for buffer object operations.
    };
    for (uint32 i = 0; i < countof(ignoredWarnings); i++)
    {
        if (type == ignoredWarnings[i].type &&
            id == ignoredWarnings[i].id &&
            severity == ignoredWarnings[i].severity)
        {
            return;
        }
    }

    const char *sourceStr = "UNKNOWN";
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
        sourceStr = "GL_DEBUG_SOURCE_API";
        break;

    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        sourceStr = "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
        break;

    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        sourceStr = "GL_DEBUG_SOURCE_SHADER_COMPILER";
        break;

    case GL_DEBUG_SOURCE_THIRD_PARTY:
        sourceStr = "GL_DEBUG_SOURCE_THIRD_PARTY";
        break;

    case GL_DEBUG_SOURCE_APPLICATION:
        sourceStr = "GL_DEBUG_SOURCE_APPLICATION";
        break;

    case GL_DEBUG_SOURCE_OTHER:
        sourceStr = "GL_DEBUG_SOURCE_OTHER";
        break;
    }

    const char *typeStr = "UNKNOWN";
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        typeStr = "GL_DEBUG_TYPE_ERROR";
        break;

    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeStr = "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
        break;

    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeStr = "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
        break;

    case GL_DEBUG_TYPE_PORTABILITY:
        typeStr = "GL_DEBUG_TYPE_PORTABILITY";
        break;

    case GL_DEBUG_TYPE_PERFORMANCE:
        typeStr = "GL_DEBUG_TYPE_PERFORMANCE";
        break;

    case GL_DEBUG_TYPE_OTHER:
        typeStr = "GL_DEBUG_TYPE_OTHER";
        break;
    }
    
    LOGLEVEL outputLogLevel = LOGLEVEL_DEV;
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        outputLogLevel = LOGLEVEL_ERROR;
        break;

    case GL_DEBUG_SEVERITY_MEDIUM:
        outputLogLevel = LOGLEVEL_WARNING;
        break;

    case GL_DEBUG_SEVERITY_LOW:
        outputLogLevel = LOGLEVEL_INFO;
        break;

    case GL_DEBUG_SEVERITY_NOTIFICATION:
        outputLogLevel = LOGLEVEL_DEV;
        break;
    }

    char tempStr[1024];
    uint32 copyLength = (length >= countof(tempStr)) ? countof(tempStr) - 1 : length;
    Y_memcpy(tempStr, message, copyLength);
    tempStr[copyLength] = 0;

    Log::GetInstance().Writef(___LogChannel___, outputLogLevel, "*** OpenGL Debug: src: %s, type: %s, id: %u, msg: %s ***", sourceStr, typeStr, id, tempStr);
}

static bool SetSDLGLColorAttributes(PIXEL_FORMAT backBufferFormat, PIXEL_FORMAT depthStencilBufferFormat)
{
    // set colour buffer attributes
    uint32 redBits = 0;
    uint32 greenBits = 0;
    uint32 blueBits = 0;
    uint32 alphaBits = 0;
    bool srgbEnabled = false;
    switch (backBufferFormat)
    {
    case PIXEL_FORMAT_R8G8B8A8_UNORM:       redBits = 8;  greenBits = 8;  blueBits = 8;   alphaBits = 8;  srgbEnabled = false;  break;
    case PIXEL_FORMAT_R8G8B8A8_UNORM_SRGB:  redBits = 8;  greenBits = 8;  blueBits = 8;   alphaBits = 8;  srgbEnabled = true;   break;
    case PIXEL_FORMAT_B8G8R8A8_UNORM:       redBits = 8;  greenBits = 8;  blueBits = 8;   alphaBits = 0;  srgbEnabled = true;   break;
    case PIXEL_FORMAT_B8G8R8X8_UNORM:       redBits = 8;  greenBits = 8;  blueBits = 8;   alphaBits = 0;  srgbEnabled = false;  break;
    default:
        Log_ErrorPrintf("SetSDLGLColorAttributes: Unhandled backbuffer format '%s'.", PixelFormat_GetPixelFormatInfo(backBufferFormat)->Name);
        return false;
    }

    // set depth buffer attributes
    uint32 depthBits = 0;
    uint32 stencilBits = 0;
    switch (depthStencilBufferFormat)
    {
    case PIXEL_FORMAT_D16_UNORM:                depthBits = 16; stencilBits = 0;    break;
    case PIXEL_FORMAT_D24_UNORM_S8_UINT:        depthBits = 24; stencilBits = 8;    break;
    case PIXEL_FORMAT_D32_FLOAT:                depthBits = 32; stencilBits = 0;    break;
    case PIXEL_FORMAT_D32_FLOAT_S8X24_UINT:     depthBits = 32; stencilBits = 8;    break;
    case PIXEL_FORMAT_UNKNOWN:                  depthBits = 0;  stencilBits = 0;    break;
    default:
        Log_ErrorPrintf("SetSDLGLColorAttributes: Unhandled depthstencil format '%s'.", PixelFormat_GetPixelFormatInfo(depthStencilBufferFormat)->Name);
        return false;
    }

    // pass them to sdl
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, redBits);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, greenBits);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, blueBits);

    // GLX doesn't seem to like the alpha bits... :S
#if !Y_PLATFORM_LINUX
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, alphaBits);
#endif

    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthBits);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencilBits);
    //SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, (srgbEnabled) ? 1 : 0);
    
    // no MSAA for now
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    return true;
}

static bool SetSDLGLVersionAttributes()
{
    // we always want h/w acceleration
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    // determine context flags
    uint32 contextFlags = 0;

#if Y_BUILD_CONFIG_DEBUG
    // use debug context in debug builds
    contextFlags |= SDL_GL_CONTEXT_DEBUG_FLAG;
#endif

    // set attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    //SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
    return true;
}

static bool InitializeGLAD()
{
    // use the sdl loader
    if (gladLoadGLES2Loader(SDL_GL_GetProcAddress) != GL_TRUE)
    {
        Log_ErrorPrint("OpenGLRenderer::Create: Failed to initialize GLAD");
        return false;
    }

    return true;
}

// set common stuff for this new gl context
static void InitializeCurrentGLContext()
{
#ifdef GL_KHR_debug
    // set debug callbacks
    if (GLAD_GL_KHR_debug && CVars::r_use_debug_device.GetBool())
    {
        Log_DevPrintf("OpenGLES2Renderer: Using GL_ARB_debug_output.");
        glDebugMessageCallback(GLDebugCallback, NULL);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }
#endif
}

OpenGLES2Renderer::OpenGLES2Renderer()
{
    m_eRendererPlatform = RENDERER_PLATFORM_OPENGLES2;
    m_eRendererFeatureLevel = RENDERER_FEATURE_LEVEL_COUNT;
    m_swapChainBackBufferFormat = PIXEL_FORMAT_UNKNOWN;
    m_swapChainDepthStencilBufferFormat = PIXEL_FORMAT_UNKNOWN;
    m_pMainContext = nullptr;
    m_pImplicitRenderWindow = nullptr;
}

OpenGLES2Renderer::~OpenGLES2Renderer()
{
    // clear device state
    if (m_pMainContext != NULL)
    {
        m_pMainContext->ClearState(true, true, true, true);
        m_pMainContext->Release();
    }

#if Y_PLATFORM_WINDOWS
    // windows-specific: clear the pixel format hint
    SDL_SetHint(SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT, nullptr);
#endif

    // destroy the window
    SAFE_RELEASE(m_pImplicitRenderWindow);
}

SDL_GLContext OpenGLES2Renderer::CreateMainSDLGLContext(OpenGLES2RendererOutputBuffer *pOutputBuffer)
{
    DebugAssert(m_eRendererFeatureLevel == RENDERER_FEATURE_LEVEL_COUNT);

    // reset attributes
    SDL_GL_ResetAttributes();

    // set up the colour buffer formats
    if (!SetSDLGLColorAttributes(pOutputBuffer->GetBackBufferFormat(), pOutputBuffer->GetDepthStencilBufferFormat()))
        return nullptr;

    // no sharing possible as there is no current context
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);

    // set the attributes
    if (!SetSDLGLVersionAttributes())
        return nullptr;

    // try creation
    Log_DevPrintf("OpenGLES2Renderer::CreateMainSDLGLContext: Trying to create a GLES 2.0 context");
    SDL_GLContext pSDLGLContext = SDL_GL_CreateContext(pOutputBuffer->GetSDLWindow());
    if (pSDLGLContext == nullptr)
    {
        Log_ErrorPrintf("OpenGLES2Renderer::CreateMainSDLGLContext: Failed to create context");
        return nullptr;
    }

    // log+return
    Log_InfoPrintf("OpenGLES2Renderer::CreateMainSDLGLContext: Got a GLES 2.0 context.");

    // initialize glew
    if (!InitializeGLAD())
    {
        Log_ErrorPrintf("OpenGLRenderer::CreateMainSDLGLContext: Failed to initialize GLEW");
        SDL_GL_MakeCurrent(nullptr, nullptr);
        SDL_GL_DeleteContext(pSDLGLContext);
        return nullptr;
    }

    // set context settings
    m_eRendererFeatureLevel = RENDERER_FEATURE_LEVEL_ES2;
    m_eTexturePlatform = TEXTURE_PLATFORM_ES2_DXTC;
    InitializeCurrentGLContext();
    return pSDLGLContext;
}

SDL_GLContext OpenGLES2Renderer::CreateUploadSDLGLContext(OpenGLES2RendererOutputBuffer *pOutputBuffer)
{
    // get the current context, it should be the same as the main context
    SDL_GLContext pCurrentGLContext = SDL_GL_GetCurrentContext();
    SDL_Window *pCurrentGLWindow = SDL_GL_GetCurrentWindow();
    DebugAssert(pCurrentGLContext == m_pMainContext->GetSDLGLContext());

    // reset attributes
    SDL_GL_ResetAttributes();

    // set up the colour buffer formats
    if (!SetSDLGLColorAttributes(pOutputBuffer->GetBackBufferFormat(), pOutputBuffer->GetDepthStencilBufferFormat()))
        return nullptr;

    // set the attributes
    if (!SetSDLGLVersionAttributes())
        return nullptr;

    // instruct it to share resources with the current context
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    // create the context
    SDL_GLContext pNewGLContext = SDL_GL_CreateContext(pOutputBuffer->GetSDLWindow());
    if (pNewGLContext == nullptr)
    {
        Log_ErrorPrintf("OpenGLES2Renderer::CreateUploadSDLGLContext: Failed to create an upload GL context");
        return nullptr;
    }

    // initialize it
    InitializeCurrentGLContext();

    // restore the context back
    if (SDL_GL_MakeCurrent(pCurrentGLWindow, pCurrentGLContext) != 0)
    {
        Log_ErrorPrintf("OpenGLES2Renderer::CreateUploadSDLGLContext: Failed to reset context back to original: %s", SDL_GetError());
        Panic("SDL_GL_MakeCurrent failed");
        SDL_GL_DeleteContext(pNewGLContext);
        return nullptr;
    }

    // return the context
    return pNewGLContext;
}


bool OpenGLES2Renderer::Create(const RendererInitializationParameters *pInitializationParameters)
{
    // select formats
    m_swapChainBackBufferFormat = pInitializationParameters->BackBufferFormat;
    m_swapChainDepthStencilBufferFormat = pInitializationParameters->DepthStencilBufferFormat;

    // if the backbuffer format is unspecified, use the best for the platform
    if (m_swapChainBackBufferFormat == PIXEL_FORMAT_UNKNOWN)
    {
        // by default this is rgba8
        m_swapChainBackBufferFormat = PIXEL_FORMAT_R8G8B8A8_UNORM;
    }

    // Determine the implicit swap chain size.
    uint32 implicitSwapChainWidth = (pInitializationParameters->HideImplicitSwapChain || pInitializationParameters->ImplicitSwapChainFullScreen) ? 1 : pInitializationParameters->ImplicitSwapChainWidth;
    uint32 implicitSwapChainHeight = (pInitializationParameters->HideImplicitSwapChain || pInitializationParameters->ImplicitSwapChainFullScreen) ? 1 : pInitializationParameters->ImplicitSwapChainHeight;

    // initialize colour attributes
    if (!SetSDLGLColorAttributes(m_swapChainBackBufferFormat, m_swapChainDepthStencilBufferFormat))
    {
        Log_ErrorPrintf("OpenGLES2Renderer::Create: Failed to set opengl window system parameters.");
        return false;
    }

    // Create the render window
    m_pImplicitRenderWindow = OpenGLES2RendererOutputWindow::Create(pInitializationParameters->ImplicitSwapChainCaption,
                                                                   implicitSwapChainWidth, implicitSwapChainHeight,
                                                                   pInitializationParameters->BackBufferFormat,
                                                                   pInitializationParameters->DepthStencilBufferFormat,
                                                                   pInitializationParameters->ImplicitSwapChainVSyncType,
                                                                   !pInitializationParameters->HideImplicitSwapChain);

    // Success?
    if (m_pImplicitRenderWindow == nullptr)
    {
        Log_ErrorPrintf("OpenGLES2Renderer::Create: Could not create implicit render window.");
        return false;
    }

    // Create the actual opengl context
    SDL_GLContext pSDLGLContext = CreateMainSDLGLContext(m_pImplicitRenderWindow->GetOpenGLOutputBuffer());
    if (pSDLGLContext == nullptr)
    {
        Log_ErrorPrintf("OpenGLES2Renderer::Create: Could not create OpenGL context.");
        return false;
    }

    // log some info
    {
        Log_InfoPrintf("OpenGL ES Renderer Feature Level: %s", NameTable_GetNameString(NameTables::RendererFeatureLevelFullName, m_eRendererFeatureLevel));
        Log_InfoPrintf("Texture Platform: %s", NameTable_GetNameString(NameTables::TexturePlatform, m_eTexturePlatform));

        // log vendor info
        Log_InfoPrintf("GL_VENDOR: %s", (glGetString(GL_VENDOR) != NULL) ? glGetString(GL_VENDOR) : (const GLubyte *)"NULL");
        Log_InfoPrintf("GL_RENDERER: %s", (glGetString(GL_RENDERER) != NULL) ? glGetString(GL_RENDERER) : (const GLubyte *)"NULL");
        Log_InfoPrintf("GL_VERSION: %s", (glGetString(GL_VERSION) != NULL) ? glGetString(GL_VERSION) : (const GLubyte *)"NULL");

        // log extensions
        //Log_InfoPrintf("GL_EXTENSIONS: %s", (glGetString(GL_EXTENSIONS) != NULL) ? glGetString(GL_EXTENSIONS) : (const GLubyte *)"NULL");
        const char *extensionString = (const char *)glGetString(GL_EXTENSIONS);
        if (extensionString != nullptr)
        {
            Log_InfoPrint("GL_EXTENSIONS:");
            Log_InfoPrint(extensionString);
        }
    }

#ifdef GL_EXT_texture_filter_anisotropic
    // run glget calls
    uint32 maxTextureAnisotropy = 0;
    if (GLAD_GL_EXT_texture_filter_anisotropic)
    {
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, reinterpret_cast<GLint *>(&maxTextureAnisotropy));
        Log_DevPrintf("glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT): %u", maxTextureAnisotropy);
    }
#endif
    
    // query max attributes and max render targets
    uint32 maxVertexAttributes = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, reinterpret_cast<GLint *>(&maxVertexAttributes));
    Log_DevPrintf("glGetIntegerv(GL_MAX_VERTEX_ATTRIBS): %u", maxVertexAttributes);

    // fill in caps
    m_RendererCapabilities.MaxTextureAnisotropy = (uint32)maxTextureAnisotropy;
    m_RendererCapabilities.SupportsMultithreadedResourceCreation = false;
    m_RendererCapabilities.SupportsDrawBaseVertex = false;
    m_RendererCapabilities.SupportsDepthTextures = true;
    m_RendererCapabilities.SupportsTextureArrays = false;
    m_RendererCapabilities.SupportsCubeMapTextureArrays = false;
    m_RendererCapabilities.SupportsGeometryShaders = false;
    m_RendererCapabilities.SupportsSinglePassCubeMaps = false;
    m_RendererCapabilities.SupportsInstancing = false;

    // create constant library
    m_pConstantLibrary = new OpenGLES2ConstantLibrary(m_eRendererFeatureLevel);

    // create device
    m_pMainContext = new OpenGLES2GPUContext();
    if (!m_pMainContext->Create(this, pSDLGLContext, m_pImplicitRenderWindow->GetOpenGLOutputBuffer()))
    {
        Log_ErrorPrintf("OpenGLES2Renderer::Create: Could not create device context.");
        return false;
    }

#if Y_PLATFORM_WINDOWS
    // windows-specific: SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT has to be set to the main window
    SDL_SetHint(SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT, String::FromFormat("%p", m_pImplicitRenderWindow->GetSDLWindow()));
#endif

    Log_InfoPrint("OpenGLES2Renderer::Create: Creation successful.");
    return true;
}

void OpenGLES2Renderer::CorrectProjectionMatrix(float4x4 &projectionMatrix) const
{
    OpenGLES2Helpers::CorrectProjectionMatrix(projectionMatrix);
}

bool OpenGLES2Renderer::CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat /*= NULL*/) const
{
    return true;
}

GPUContext *OpenGLES2Renderer::CreateUploadContext()
{
    return NULL;
}

void OpenGLES2Renderer::GetGPUMemoryUsage(GPUMemoryUsage *pMemoryUsage) const
{
    Y_memzero(pMemoryUsage, sizeof(GPUMemoryUsage));
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

GPURasterizerState *OpenGLES2Renderer::CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc)
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

GPUDepthStencilState *OpenGLES2Renderer::CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc)
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

GPUBlendState *OpenGLES2Renderer::CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc)
{
    return new OpenGLES2GPUBlendState(pBlendStateDesc);
}

// stubs

GPUSamplerState *OpenGLES2Renderer::CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc)
{
    Log_ErrorPrintf("OpenGLES2Renderer::CreateSamplerState: Unsupported on GLES");
    return nullptr;
}

GPUQuery *OpenGLES2Renderer::CreateQuery(GPU_QUERY_TYPE type)
{
    Log_ErrorPrintf("OpenGLES2Renderer::CreateQuery: Unsupported on GLES");
    return nullptr;
}

GPUTexture1D *OpenGLES2Renderer::CreateTexture1D(const GPU_TEXTURE1D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */)
{
    Log_ErrorPrintf("OpenGLES2Renderer::CreateTexture1D: Unsupported on GLES");
    return nullptr;
}

GPUTexture1DArray *OpenGLES2Renderer::CreateTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */)
{
    Log_ErrorPrintf("OpenGLES2Renderer::CreateTexture1DArray: Unsupported on GLES");
    return nullptr;
}

GPUTexture2DArray *OpenGLES2Renderer::CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */)
{
    Log_ErrorPrintf("OpenGLES2Renderer::CreateTexture2DArray: Unsupported on GLES");
    return nullptr;
}

GPUTexture3D *OpenGLES2Renderer::CreateTexture3D(const GPU_TEXTURE3D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */, const uint32 *pInitialDataSlicePitch /* = NULL */)
{
    Log_ErrorPrintf("OpenGLES2Renderer::CreateTexture3D: Unsupported on GLES");
    return nullptr;
}

GPUTextureCubeArray *OpenGLES2Renderer::CreateTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /* = NULL */, const uint32 *pInitialDataPitch /* = NULL */)
{
    Log_ErrorPrintf("OpenGLES2Renderer::CreateTextureCubeArray: Unsupported on GLES");
    return nullptr;
}

GPUComputeView *OpenGLES2Renderer::CreateComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc)
{
    Log_ErrorPrintf("OpenGLES2Renderer::CreateComputeView: Unsupported on GLES");
    return nullptr;
}

Renderer *OpenGLESRenderer_CreateRenderer(const RendererInitializationParameters *pCreateParameters)
{
    OpenGLES2Renderer *pRenderer = new OpenGLES2Renderer();
    if (!pRenderer->Create(pCreateParameters))
    {
        delete pRenderer;
        return nullptr;
    }

    return pRenderer;
}

