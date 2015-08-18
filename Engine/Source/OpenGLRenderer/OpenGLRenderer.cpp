#include "OpenGLRenderer/PrecompiledHeader.h"
#include "OpenGLRenderer/OpenGLRenderer.h"
#include "Engine/EngineCVars.h"
Log_SetChannel(OpenGLRenderer);

// our gl debug callback
static void APIENTRY GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
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
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, (srgbEnabled) ? 1 : 0);
    return true;
}

static bool SetSDLGLVersionAttributes(RENDERER_FEATURE_LEVEL featureLevel)
{
    // we always want h/w acceleration
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    // determine context flags
    uint32 contextFlags = 0;

    // forward compatibilty preferred
    contextFlags |= SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG;

#if Y_BUILD_CONFIG_DEBUG
    // use debug context in debug builds
    contextFlags |= SDL_GL_CONTEXT_DEBUG_FLAG;
#endif

    // we need the major version, minor version, and profile mask
    uint32 majorVersion, minorVersion, profileMask;

    // branch out depending on version
    switch (featureLevel)
    {
    case RENDERER_FEATURE_LEVEL_SM5:            majorVersion = 4;   minorVersion = 4;   profileMask = SDL_GL_CONTEXT_PROFILE_CORE;  break;
    case RENDERER_FEATURE_LEVEL_SM4:            majorVersion = 3;   minorVersion = 3;   profileMask = SDL_GL_CONTEXT_PROFILE_CORE;  break;
    default:
        Log_ErrorPrintf("SetSDLGLVersionAttributes: Unhandled renderer feature level: %s", NameTable_GetNameString(NameTables::RendererFeatureLevel, featureLevel));
        return false;
    }

    // set attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profileMask);
    return true;
}

// static bool InitializeGLEW()
// {
//     static bool glewInitialized = false;
//     if (!glewInitialized)
//     {
//         // glewExperimental has to be set as the extensions should be resolved individually instead of by version
//         glewExperimental = GL_TRUE;
// 
//         // Initialize GLEW
//         GLenum glewInitError = glewInit();
//         if (glewInitError != GLEW_NO_ERROR)
//         {
//             Log_ErrorPrintf("OpenGLRenderer::Create: Failed to initialize GLEW: %u (%s)", (uint32)glewInitError, glewGetErrorString(glewInitError));
//             return false;
//         }
// 
//         glewInitialized = true;
//     }
// 
//     return true;
// }

static bool InitializeGLAD()
{
    // use the sdl loader
    if (gladLoadGLLoader(SDL_GL_GetProcAddress) != GL_TRUE)
    {
        Log_ErrorPrint("OpenGLRenderer::Create: Failed to initialize GLAD");
        return false;
    }

    return true;
}

// set common stuff for this new gl context
static void InitializeCurrentGLContext()
{
    // set debug callbacks
    if (GLAD_GL_ARB_debug_output && CVars::r_use_debug_device.GetBool())
    {
        Log_DevPrintf("OpenGLRenderer: Using GL_ARB_debug_output.");
        glDebugMessageCallbackARB(GLDebugCallback, NULL);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }
}

OpenGLRenderer::OpenGLRenderer()
{
    m_eRendererPlatform = RENDERER_PLATFORM_OPENGL;
    m_eRendererFeatureLevel = RENDERER_FEATURE_LEVEL_COUNT;
    m_swapChainBackBufferFormat = PIXEL_FORMAT_UNKNOWN;
    m_swapChainDepthStencilBufferFormat = PIXEL_FORMAT_UNKNOWN;
    m_pMainContext = NULL;
    m_pImplicitRenderWindow = NULL;
}

OpenGLRenderer::~OpenGLRenderer()
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

SDL_GLContext OpenGLRenderer::CreateMainSDLGLContext(OpenGLRendererOutputBuffer *pOutputBuffer)
{
    DebugAssert(m_eRendererFeatureLevel == RENDERER_FEATURE_LEVEL_COUNT);

    // set up the colour buffer formats
    if (!SetSDLGLColorAttributes(pOutputBuffer->GetBackBufferFormat(), pOutputBuffer->GetDepthStencilBufferFormat()))
        return nullptr;

    // no sharing possible as there is no current context
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);

    // define an order of versions to attempt to create
    RENDERER_FEATURE_LEVEL featureLevelCreationOrder[] = {
        RENDERER_FEATURE_LEVEL_SM5,
        RENDERER_FEATURE_LEVEL_SM4
    };

    // try creating each one until we get one
    for (uint32 i = 0; i < countof(featureLevelCreationOrder); i++)
    {
        // set the attributes
        if (!SetSDLGLVersionAttributes(featureLevelCreationOrder[i]))
            continue;

        // try creation
        Log_DevPrintf("OpenGLRenderer::CreateMainSDLGLContext: Trying to create a %s context", NameTable_GetNameString(NameTables::RendererFeatureLevel, featureLevelCreationOrder[i]));
        SDL_GLContext pSDLGLContext = SDL_GL_CreateContext(pOutputBuffer->GetSDLWindow());
        if (pSDLGLContext == nullptr)
            continue;

        // log+return
        Log_InfoPrintf("OpenGLRenderer::CreateMainSDLGLContext: Got a %s context (%s)", NameTable_GetNameString(NameTables::RendererFeatureLevel, featureLevelCreationOrder[i]), NameTable_GetNameString(NameTables::RendererFeatureLevelFullName, featureLevelCreationOrder[i]));

        // initialize glew
        if (!InitializeGLAD())
        {
            Log_ErrorPrintf("OpenGLRenderer::CreateMainSDLGLContext: Failed to initialize GLEW");
            SDL_GL_MakeCurrent(nullptr, nullptr);
            SDL_GL_DeleteContext(pSDLGLContext);
            continue;
        }

        // set context settings
        m_eRendererFeatureLevel = featureLevelCreationOrder[i];
        m_eTexturePlatform = (featureLevelCreationOrder[i] == RENDERER_FEATURE_LEVEL_SM4) ? TEXTURE_PLATFORM_DXTC : TEXTURE_PLATFORM_DXTC;
        InitializeCurrentGLContext();
        return pSDLGLContext;
    }

    Log_ErrorPrintf("OpenGLRenderer::CreateMainSDLGLContext: Failed to create an acceptable GL context");
    return nullptr;
}

SDL_GLContext OpenGLRenderer::CreateUploadSDLGLContext(OpenGLRendererOutputBuffer *pOutputBuffer)
{
    // get the current context, it should be the same as the main context
    SDL_GLContext pCurrentGLContext = SDL_GL_GetCurrentContext();
    SDL_Window *pCurrentGLWindow = SDL_GL_GetCurrentWindow();
    DebugAssert(pCurrentGLContext == m_pMainContext->GetSDLGLContext());

    // set the attributes
    if (!SetSDLGLVersionAttributes(m_eRendererFeatureLevel))
        return nullptr;

    // instruct it to share resources with the current context
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);

    // create the context
    SDL_GLContext pNewGLContext = SDL_GL_CreateContext(pOutputBuffer->GetSDLWindow());
    if (pNewGLContext == nullptr)
    {
        Log_ErrorPrintf("OpenGLRenderer::CreateUploadSDLGLContext: Failed to create an upload GL context");
        return nullptr;
    }

    // initialize it
    InitializeCurrentGLContext();

    // restore the context back
    if (SDL_GL_MakeCurrent(pCurrentGLWindow, pCurrentGLContext) != 0)
    {
        Log_ErrorPrintf("OpenGLRenderer::CreateUploadSDLGLContext: Failed to reset context back to original: %s", SDL_GetError());
        Panic("SDL_GL_MakeCurrent failed");
        SDL_GL_DeleteContext(pNewGLContext);
        return nullptr;
    }

    // return the context
    return pNewGLContext;
}


bool OpenGLRenderer::Create(const RendererInitializationParameters *pInitializationParameters)
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
        Log_ErrorPrintf("OpenGLRenderer::Create: Failed to set opengl window system parameters.");
        return false;
    }

    // Create the render window
    m_pImplicitRenderWindow = OpenGLRendererOutputWindow::Create(pInitializationParameters->ImplicitSwapChainCaption,
                                                                 implicitSwapChainWidth, implicitSwapChainHeight,
                                                                 pInitializationParameters->BackBufferFormat,
                                                                 pInitializationParameters->DepthStencilBufferFormat,
                                                                 pInitializationParameters->ImplicitSwapChainVSyncType,
                                                                 !pInitializationParameters->HideImplicitSwapChain);

    // Success?
    if (m_pImplicitRenderWindow == nullptr)
    {
        Log_ErrorPrintf("OpenGLRenderer::Create: Could not create implicit render window.");
        return false;
    }

    // Create the actual opengl context
    SDL_GLContext pSDLGLContext = CreateMainSDLGLContext(m_pImplicitRenderWindow->GetOpenGLOutputBuffer());
    if (pSDLGLContext == nullptr)
    {
        Log_ErrorPrintf("OpenGLRenderer::Create: Could not create OpenGL context.");
        return false;
    }

    // log some info
    {
        Log_InfoPrintf("OpenGL Renderer Feature Level: %s", NameTable_GetNameString(NameTables::RendererFeatureLevelFullName, m_eRendererFeatureLevel));
        Log_InfoPrintf("Texture Platform: %s", NameTable_GetNameString(NameTables::TexturePlatform, m_eTexturePlatform));

        // log vendor info
        Log_InfoPrintf("GL_VENDOR: %s", (glGetString(GL_VENDOR) != NULL) ? glGetString(GL_VENDOR) : (const GLubyte *)"NULL");
        Log_InfoPrintf("GL_RENDERER: %s", (glGetString(GL_RENDERER) != NULL) ? glGetString(GL_RENDERER) : (const GLubyte *)"NULL");

        GLint majorVersion, minorVersion;
        glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
        glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
        Log_InfoPrintf("GL_VERSION: %s (%i.%i)", (glGetString(GL_VERSION) != NULL) ? glGetString(GL_VERSION) : (const GLubyte *)"NULL", majorVersion, minorVersion);

        // log extensions
#if 0
        GLint numExtensions;
        String extensionsString;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);   
        extensionsString.Format("GL_NUM_EXTENSIONS: %i [", numExtensions);
        for (GLint i = 0; i < numExtensions; i++)
        {
            if (i != 0)
                extensionsString.AppendCharacter(' ');

            extensionsString.AppendString((const char *)glGetStringi(GL_EXTENSIONS, i));
        }
        extensionsString.AppendCharacter(']');
        Log_InfoPrint(extensionsString);
#endif
    }

    // run glget calls
    uint32 maxTextureAnisotropy = 0;
    glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, reinterpret_cast<GLint *>(&maxTextureAnisotropy));
    Log_DevPrintf("glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT): %u", maxTextureAnisotropy);

    // query max attributes and max render targets
    uint32 maxVertexAttributes = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, reinterpret_cast<GLint *>(&maxVertexAttributes));
    Log_DevPrintf("glGetIntegerv(GL_MAX_VERTEX_ATTRIBS): %u", maxVertexAttributes);

    uint32 maxColorAttachments = 0;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, reinterpret_cast<GLint *>(&maxColorAttachments));
    Log_DevPrintf("glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS): %u", maxColorAttachments);

    // max uniform buffers
    uint32 maxUniformBufferBindings = 0;
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, reinterpret_cast<GLint *>(&maxUniformBufferBindings));
    Log_DevPrintf("glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS): %u", maxUniformBufferBindings);

    // check minspec
    if (maxVertexAttributes < GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS || maxColorAttachments < GPU_MAX_SIMULTANEOUS_RENDER_TARGETS)
    {
        Log_ErrorPrintf("OpenGLRenderer::Create: Requires support for at least %u vertex attributes and %u render targets. (%u/%u)", (uint32)GPU_INPUT_LAYOUT_MAX_ELEMENTS, (uint32)GPU_MAX_SIMULTANEOUS_RENDER_TARGETS, maxVertexAttributes, maxColorAttachments);
        SDL_GL_MakeCurrent(nullptr, nullptr);
        SDL_GL_DeleteContext(pSDLGLContext);
        return false;
    }

    // for debugging only
    if (CVars::r_opengl_disable_vertex_attrib_binding.GetBool())
        *const_cast<int *>(&GLAD_GL_ARB_vertex_attrib_binding) = GL_FALSE;
    if (CVars::r_opengl_disable_direct_state_access.GetBool())
        *const_cast<int *>(&GLAD_GL_EXT_direct_state_access) = GL_FALSE;
    if (CVars::r_opengl_disable_multi_bind.GetBool())
        *const_cast<int *>(&GLAD_GL_ARB_multi_bind) = GL_FALSE;

    // log warnings about missing but not required extensions
    if (!GLAD_GL_ARB_vertex_attrib_binding)
        Log_WarningPrint("OpenGLRenderer: Missing GL_ARB_vertex_attrib_binding, performance will suffer as a result. Please update your drivers.");
    else
        Log_InfoPrint("OpenGLRenderer: Using GL_ARB_vertex_attrib_binding.");

    // multi bind
    if (!GLAD_GL_ARB_multi_bind)
        Log_WarningPrint("OpenGLRenderer: Missing GL_ARB_multi_bind, performance will suffer as a result. Please update your drivers.");
    else
        Log_InfoPrint("OpenGLRenderer: Using GL_ARB_multi_bind.");

    // direct state access
    if (!GLAD_GL_EXT_direct_state_access)
        Log_WarningPrint("OpenGLRenderer: Missing GL_EXT_direct_state_access, performance will suffer as a result. Please update your drivers.");
    else
        Log_InfoPrint("OpenGLRenderer: Using GL_EXT_direct_state_access.");

    // direct state access
    if (GLAD_GL_ARB_copy_image)
        Log_InfoPrint("OpenGLRenderer: Using GL_ARB_copy_image.");
    else if (GLAD_GL_NV_copy_image)
        Log_InfoPrint("OpenGLRenderer: Using GL_NV_copy_image.");
    else
        Log_WarningPrint("OpenGLRenderer: Missing GL_ARB_copy_image and GL_NV_copy_image, performance will suffer as a result. Please update your drivers.");

    // fill in caps
    m_RendererCapabilities.MaxTextureAnisotropy = (uint32)maxTextureAnisotropy;
    m_RendererCapabilities.SupportsMultithreadedResourceCreation = false;
    m_RendererCapabilities.SupportsDrawBaseVertex = true;
    m_RendererCapabilities.SupportsDepthTextures = (GLAD_GL_ARB_depth_texture == GL_TRUE);
    m_RendererCapabilities.SupportsTextureArrays = (GLAD_GL_EXT_texture_array == GL_TRUE);
    m_RendererCapabilities.SupportsCubeMapTextureArrays = (GLAD_GL_ARB_texture_cube_map_array == GL_TRUE);
    m_RendererCapabilities.SupportsGeometryShaders = (GLAD_GL_EXT_geometry_shader4 == GL_TRUE);
    m_RendererCapabilities.SupportsSinglePassCubeMaps = (GLAD_GL_EXT_geometry_shader4 == GL_TRUE && GLAD_GL_ARB_viewport_array == GL_TRUE);
    m_RendererCapabilities.SupportsInstancing = (GLAD_GL_EXT_draw_instanced == GL_TRUE);

    // create device
    m_pMainContext = new OpenGLGPUContext();
    if (!m_pMainContext->Create(this, pSDLGLContext, m_pImplicitRenderWindow->GetOpenGLOutputBuffer()))
    {
        Log_ErrorPrintf("OpenGLRenderer::Create: Could not create device context.");
        return false;
    }

#if Y_PLATFORM_WINDOWS
    // windows-specific: SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT has to be set to the main window
    SDL_SetHint(SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT, String::FromFormat("%p", m_pImplicitRenderWindow->GetSDLWindow()));
#endif

    Log_InfoPrint("OpenGLRenderer::Create: Creation successful.");
    return true;
}

void OpenGLRenderer::CorrectProjectionMatrix(float4x4 &projectionMatrix) const
{
    OpenGLHelpers::CorrectProjectionMatrix(projectionMatrix);
}

bool OpenGLRenderer::CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat /*= NULL*/) const
{
    return true;
}

GPUContext *OpenGLRenderer::CreateUploadContext()
{
    return NULL;
}

void OpenGLRenderer::GetGPUMemoryUsage(GPUMemoryUsage *pMemoryUsage) const
{
    Y_memzero(pMemoryUsage, sizeof(GPUMemoryUsage));
}

// ------------------

OpenGLGPUSamplerState::OpenGLGPUSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, GLuint samplerID)
    : GPUSamplerState(pSamplerStateDesc),
      m_GLSamplerID(samplerID)
{

}

OpenGLGPUSamplerState::~OpenGLGPUSamplerState()
{
    glDeleteSamplers(1, &m_GLSamplerID);
}

void OpenGLGPUSamplerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 128;
}

void OpenGLGPUSamplerState::SetDebugName(const char *name)
{
    OpenGLHelpers::SetObjectDebugName(GL_SAMPLER, m_GLSamplerID, name);
}

GPUSamplerState *OpenGLRenderer::CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc)
{
    GL_CHECKED_SECTION_BEGIN();

    GLuint samplerID = 0;
    glGenSamplers(1, &samplerID);
    if (samplerID == 0)
    {
        GL_PRINT_ERROR("OpenGLRenderer::CreateSamplerState: Sampler allocation failed: ");
        return nullptr;
    }

    glSamplerParameterfv(samplerID, GL_TEXTURE_BORDER_COLOR, pSamplerStateDesc->BorderColor);
    glSamplerParameteri(samplerID, GL_TEXTURE_COMPARE_FUNC, OpenGLTypeConversion::GetOpenGLComparisonFunc(pSamplerStateDesc->ComparisonFunc));
    glSamplerParameteri(samplerID, GL_TEXTURE_COMPARE_MODE, OpenGLTypeConversion::GetOpenGLComparisonMode(pSamplerStateDesc->Filter));
    glSamplerParameterf(samplerID, GL_TEXTURE_LOD_BIAS, pSamplerStateDesc->LODBias);
    glSamplerParameteri(samplerID, GL_TEXTURE_MIN_FILTER, OpenGLTypeConversion::GetOpenGLTextureMinFilter(pSamplerStateDesc->Filter));
    glSamplerParameteri(samplerID, GL_TEXTURE_MAG_FILTER, OpenGLTypeConversion::GetOpenGLTextureMagFilter(pSamplerStateDesc->Filter));
    glSamplerParameterf(samplerID, GL_TEXTURE_MIN_LOD, (float)pSamplerStateDesc->MinLOD);
    glSamplerParameterf(samplerID, GL_TEXTURE_MAX_LOD, (float)pSamplerStateDesc->MaxLOD);
    glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_S, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressU));
    glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_T, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressV));
    glSamplerParameteri(samplerID, GL_TEXTURE_WRAP_R, OpenGLTypeConversion::GetOpenGLTextureWrap(pSamplerStateDesc->AddressW));

    if (pSamplerStateDesc->Filter == TEXTURE_FILTER_ANISOTROPIC || pSamplerStateDesc->Filter == TEXTURE_FILTER_COMPARISON_ANISOTROPIC)
        glSamplerParameterf(samplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)pSamplerStateDesc->MaxAnisotropy);
    else
        glSamplerParameterf(samplerID, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);

    return new OpenGLGPUSamplerState(pSamplerStateDesc, samplerID);
}

OpenGLGPURasterizerState::OpenGLGPURasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc)
    : GPURasterizerState(pRasterizerStateDesc)
{
    m_GLPolygonMode = OpenGLTypeConversion::GetOpenGLPolygonMode(pRasterizerStateDesc->FillMode);
    m_GLCullFace = OpenGLTypeConversion::GetOpenGLCullFace(pRasterizerStateDesc->CullMode);
    m_GLCullEnabled = (pRasterizerStateDesc->CullMode != RENDERER_CULL_NONE) ? true : false;
    m_GLFrontFace = (pRasterizerStateDesc->FrontCounterClockwise) ? GL_CCW : GL_CW;
    m_GLScissorTestEnable = pRasterizerStateDesc->ScissorEnable;
    m_GLDepthClampEnable = !pRasterizerStateDesc->DepthClipEnable;
}

OpenGLGPURasterizerState::~OpenGLGPURasterizerState()
{

}

void OpenGLGPURasterizerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 0;
}

void OpenGLGPURasterizerState::SetDebugName(const char *name)
{

}

void OpenGLGPURasterizerState::Apply()
{
    glPolygonMode(GL_FRONT_AND_BACK, m_GLPolygonMode);
    glCullFace(m_GLCullFace);
    (m_GLCullEnabled) ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    glFrontFace(m_GLFrontFace);
    (m_GLScissorTestEnable) ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
    (m_GLDepthClampEnable) ? glEnable(GL_DEPTH_CLAMP) : glDisable(GL_DEPTH_CLAMP);
}

void OpenGLGPURasterizerState::Unapply()
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_CLAMP);
}

GPURasterizerState *OpenGLRenderer::CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc)
{
    return new OpenGLGPURasterizerState(pRasterizerStateDesc);
}

OpenGLGPUDepthStencilState::OpenGLGPUDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc)
    : GPUDepthStencilState(pDepthStencilStateDesc)
{
    m_GLDepthTestEnable = pDepthStencilStateDesc->DepthTestEnable;
    m_GLDepthMask = (pDepthStencilStateDesc->DepthWriteEnable) ? GL_TRUE : GL_FALSE;
    m_GLDepthFunc = OpenGLTypeConversion::GetOpenGLComparisonFunc(pDepthStencilStateDesc->DepthFunc);
    m_GLStencilTestEnable = pDepthStencilStateDesc->StencilTestEnable;
    m_GLStencilReadMask = pDepthStencilStateDesc->StencilReadMask;
    m_GLStencilWriteMask = pDepthStencilStateDesc->StencilWriteMask;
    m_GLStencilFuncFront = OpenGLTypeConversion::GetOpenGLComparisonFunc(pDepthStencilStateDesc->StencilFrontFace.CompareFunc);
    m_GLStencilOpFrontFail = OpenGLTypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilFrontFace.FailOp);
    m_GLStencilOpFrontDepthFail = OpenGLTypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilFrontFace.DepthFailOp);
    m_GLStencilOpFrontPass = OpenGLTypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilFrontFace.PassOp);
    m_GLStencilFuncBack = OpenGLTypeConversion::GetOpenGLComparisonFunc(pDepthStencilStateDesc->StencilBackFace.CompareFunc);
    m_GLStencilOpBackFail = OpenGLTypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilBackFace.FailOp);
    m_GLStencilOpBackDepthFail = OpenGLTypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilBackFace.DepthFailOp);
    m_GLStencilOpBackPass = OpenGLTypeConversion::GetOpenGLStencilOp(pDepthStencilStateDesc->StencilBackFace.PassOp);
}

OpenGLGPUDepthStencilState::~OpenGLGPUDepthStencilState()
{

}

void OpenGLGPUDepthStencilState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 0;
}

void OpenGLGPUDepthStencilState::SetDebugName(const char *name)
{

}

void OpenGLGPUDepthStencilState::Apply(uint8 StencilRef)
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

void OpenGLGPUDepthStencilState::Unapply()
{
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_STENCIL_TEST);
    glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS, 0, 0xFF);
    glStencilOpSeparate(GL_FRONT_AND_BACK, GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);
}


GPUDepthStencilState *OpenGLRenderer::CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc)
{
    return new OpenGLGPUDepthStencilState(pDepthStencilStateDesc);
}

OpenGLGPUBlendState::OpenGLGPUBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc)
    : GPUBlendState(pBlendStateDesc)
{
    m_GLBlendEnable = pBlendStateDesc->BlendEnable;
    m_GLBlendEquationRGB = OpenGLTypeConversion::GetOpenGLBlendEquation(pBlendStateDesc->BlendOp);
    m_GLBlendEquationAlpha = OpenGLTypeConversion::GetOpenGLBlendEquation(pBlendStateDesc->BlendOpAlpha);
    m_GLBlendFuncSrcRGB = OpenGLTypeConversion::GetOpenGLBlendFunc(pBlendStateDesc->SrcBlend);
    m_GLBlendFuncSrcAlpha = OpenGLTypeConversion::GetOpenGLBlendFunc(pBlendStateDesc->SrcBlendAlpha);
    m_GLBlendFuncDstRGB = OpenGLTypeConversion::GetOpenGLBlendFunc(pBlendStateDesc->DestBlend);
    m_GLBlendFuncDstAlpha = OpenGLTypeConversion::GetOpenGLBlendFunc(pBlendStateDesc->DestBlendAlpha);
    m_GLColorWritesEnabled = pBlendStateDesc->ColorWriteEnable;
}

OpenGLGPUBlendState::~OpenGLGPUBlendState()
{

}

void OpenGLGPUBlendState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 0;
}

void OpenGLGPUBlendState::SetDebugName(const char *name)
{

}

void OpenGLGPUBlendState::Apply()
{
    (m_GLBlendEnable) ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    glBlendEquationSeparate(m_GLBlendEquationRGB, m_GLBlendEquationAlpha);
    glBlendFuncSeparate(m_GLBlendFuncSrcRGB, m_GLBlendFuncDstRGB, m_GLBlendFuncSrcAlpha, m_GLBlendFuncDstAlpha);
    (m_GLColorWritesEnabled) ? glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE) : glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}

void OpenGLGPUBlendState::Unapply()
{
    glDisable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

GPUBlendState *OpenGLRenderer::CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc)
{
    return new OpenGLGPUBlendState(pBlendStateDesc);
}

Renderer *OpenGLRenderer_CreateRenderer(const RendererInitializationParameters *pCreateParameters)
{
    OpenGLRenderer *pRenderer = new OpenGLRenderer();
    if (!pRenderer->Create(pCreateParameters))
    {
        delete pRenderer;
        return NULL;
    }

    return pRenderer;
}

