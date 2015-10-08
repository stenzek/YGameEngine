#include "OpenGLRenderer/PrecompiledHeader.h"
#include "OpenGLRenderer/OpenGLRenderBackend.h"
#include "OpenGLRenderer/OpenGLGPUContext.h"
#include "OpenGLRenderer/OpenGLGPUBuffer.h"
#include "OpenGLRenderer/OpenGLGPUOutputBuffer.h"
#include "OpenGLRenderer/OpenGLGPUDevice.h"
#include "Engine/EngineCVars.h"
#include "Engine/SDLHeaders.h"
Log_SetChannel(OpenGLRenderBackend);

OpenGLRenderBackend *OpenGLRenderBackend::m_pInstance = nullptr;

OpenGLRenderBackend::OpenGLRenderBackend()
    : m_featureLevel(RENDERER_FEATURE_LEVEL_COUNT)
    , m_texturePlatform(NUM_TEXTURE_PLATFORMS)
    , m_outputBackBufferFormat(PIXEL_FORMAT_UNKNOWN)
    , m_outputDepthStencilFormat(PIXEL_FORMAT_UNKNOWN)
    , m_pGPUDevice(nullptr)
    , m_pGPUContext(nullptr)
{
    DebugAssert(m_pInstance == nullptr);
    m_pInstance = this;
}

OpenGLRenderBackend::~OpenGLRenderBackend()
{
    DebugAssert(m_pInstance == this);
    m_pInstance = nullptr;
}

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

    Log::GetInstance().Writef(___LogChannel___, LOG_MESSAGE_FUNCTION_NAME, outputLogLevel, "*** OpenGL Debug: src: %s, type: %s, id: %u, msg: %s ***", sourceStr, typeStr, id, tempStr);
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
    if (CVars::r_use_debug_device.GetBool())
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

static bool InitializeGLAD()
{
    // use the sdl loader
    if (gladLoadGLLoader(SDL_GL_GetProcAddress) != GL_TRUE)
    {
        Log_ErrorPrint("Failed to initialize GLAD");
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
        Log_DevPrintf("Using GL_ARB_debug_output.");
        glDebugMessageCallbackARB(GLDebugCallback, NULL);
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }
}

bool OpenGLRenderBackend::Create(const RendererInitializationParameters *pCreateParameters, SDL_Window *pSDLWindow, RenderBackend **ppBackend, GPUDevice **ppDevice, GPUContext **ppContext, GPUOutputBuffer **ppOutputBuffer)
{
    // select formats
    m_outputBackBufferFormat = pCreateParameters->BackBufferFormat;
    m_outputDepthStencilFormat = pCreateParameters->DepthStencilBufferFormat;

    // if the backbuffer format is unspecified, use the best for the platform
    if (m_outputBackBufferFormat == PIXEL_FORMAT_UNKNOWN)
    {
        // by default this is rgba8
        m_outputBackBufferFormat = PIXEL_FORMAT_R8G8B8A8_UNORM;
    }

    // initialize colour attributes
    if (!SetSDLGLColorAttributes(m_outputBackBufferFormat, m_outputDepthStencilFormat))
    {
        Log_ErrorPrintf("OpenGLRenderBackend::Create: Failed to set opengl window system parameters.");
        return false;
    }

    // no sharing possible as there is no current context
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);

    // define an order of versions to attempt to create
    RENDERER_FEATURE_LEVEL featureLevelCreationOrder[] = {
        RENDERER_FEATURE_LEVEL_SM5,
        RENDERER_FEATURE_LEVEL_SM4
    };

    // try creating each one until we get one
    SDL_GLContext pSDLGLContext = nullptr;
    for (uint32 i = 0; i < countof(featureLevelCreationOrder); i++)
    {
        // set the attributes
        if (!SetSDLGLVersionAttributes(featureLevelCreationOrder[i]))
            continue;

        // try creation
        Log_DevPrintf("OpenGLRenderBackend::Create: Trying to create a %s context", NameTable_GetNameString(NameTables::RendererFeatureLevel, featureLevelCreationOrder[i]));
        pSDLGLContext = SDL_GL_CreateContext(pSDLWindow);
        if (pSDLGLContext == nullptr)
            continue;

        // log+return
        Log_InfoPrintf("OpenGLRenderBackend::Create: Got a %s context (%s)", NameTable_GetNameString(NameTables::RendererFeatureLevel, featureLevelCreationOrder[i]), NameTable_GetNameString(NameTables::RendererFeatureLevelFullName, featureLevelCreationOrder[i]));

        // initialize glew
        if (!InitializeGLAD())
        {
            Log_ErrorPrintf("OpenGLRenderBackend::Create: Failed to initialize GLEW");
            SDL_GL_MakeCurrent(nullptr, nullptr);
            SDL_GL_DeleteContext(pSDLGLContext);
            continue;
        }

        // set context settings
        m_featureLevel = featureLevelCreationOrder[i];
        m_texturePlatform = (featureLevelCreationOrder[i] == RENDERER_FEATURE_LEVEL_SM4) ? TEXTURE_PLATFORM_DXTC : TEXTURE_PLATFORM_DXTC;
        InitializeCurrentGLContext();
        break;
    }

    // failed?
    if (pSDLGLContext == nullptr)
    {
        Log_ErrorPrintf("OpenGLRenderBackend::Create: Failed to create an acceptable GL context");
        return nullptr;
    }

    // log some info
    {
        Log_InfoPrintf("OpenGL Renderer Feature Level: %s", NameTable_GetNameString(NameTables::RendererFeatureLevelFullName, m_featureLevel));
        Log_InfoPrintf("Texture Platform: %s", NameTable_GetNameString(NameTables::TexturePlatform, m_texturePlatform));

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
        Log_ErrorPrintf("OpenGLRenderBackend::Create: Requires support for at least %u vertex attributes and %u render targets. (%u/%u)", (uint32)GPU_INPUT_LAYOUT_MAX_ELEMENTS, (uint32)GPU_MAX_SIMULTANEOUS_RENDER_TARGETS, maxVertexAttributes, maxColorAttachments);
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
        Log_WarningPrint("Missing GL_ARB_vertex_attrib_binding, performance will suffer as a result. Please update your drivers.");
    else
        Log_InfoPrint("Using GL_ARB_vertex_attrib_binding.");

    // multi bind
    if (!GLAD_GL_ARB_multi_bind)
        Log_WarningPrint("Missing GL_ARB_multi_bind, performance will suffer as a result. Please update your drivers.");
    else
        Log_InfoPrint("Using GL_ARB_multi_bind.");

    // direct state access
    if (!GLAD_GL_EXT_direct_state_access)
        Log_WarningPrint("Missing GL_EXT_direct_state_access, performance will suffer as a result. Please update your drivers.");
    else
        Log_InfoPrint("Using GL_EXT_direct_state_access.");

    // direct state access
    if (GLAD_GL_ARB_copy_image)
        Log_InfoPrint("Using GL_ARB_copy_image.");
    else if (GLAD_GL_NV_copy_image)
        Log_InfoPrint("Using GL_NV_copy_image.");
    else
        Log_WarningPrint("Missing GL_ARB_copy_image and GL_NV_copy_image, performance will suffer as a result. Please update your drivers.");

    // create output buffer
    m_pImplicitOutputBuffer = new OpenGLGPUOutputBuffer(pSDLWindow, m_outputBackBufferFormat, m_outputDepthStencilFormat, pCreateParameters->ImplicitSwapChainVSyncType, false);

    // create device and context
    m_pGPUDevice = new OpenGLGPUDevice(pSDLGLContext, m_outputBackBufferFormat, m_outputDepthStencilFormat);
    m_pGPUContext = new OpenGLGPUContext(m_pGPUDevice, pSDLGLContext, m_pImplicitOutputBuffer);
    if (!m_pGPUContext->Create())
    {
        Log_ErrorPrintf("OpenGLRenderBackend::Create: Could not create device context.");
        return false;
    }

    // add references for returned pointers
    m_pGPUDevice->AddRef();
    m_pGPUContext->AddRef();
    m_pImplicitOutputBuffer->AddRef();

    // set pointers
    *ppBackend = this;
    *ppDevice = m_pGPUDevice;
    *ppContext = m_pGPUContext;
    *ppOutputBuffer = m_pImplicitOutputBuffer;

    Log_InfoPrint("OpenGLRenderBackend::Create: Creation successful.");
    return true;
}

void OpenGLRenderBackend::Shutdown()
{
    // cleanup our objects
    SAFE_RELEASE(m_pImplicitOutputBuffer);
    SAFE_RELEASE(m_pGPUContext);
    SAFE_RELEASE(m_pGPUDevice);

    // done
    delete this;
}

// Since we have multiple threads, the last GL error has to be thread-local
Y_DECLARE_THREAD_LOCAL(GLenum) s_lastGLError = GL_NO_ERROR;

void OpenGLRenderBackend::ClearLastGLError()
{
    s_lastGLError = GL_NO_ERROR;
    while (glGetError() != GL_NO_ERROR)
        ;
}

GLenum OpenGLRenderBackend::CheckForGLError()
{
    GLenum error = glGetError();
    s_lastGLError = error;
    return (error != GL_NO_ERROR);
}

GLenum OpenGLRenderBackend::GetLastGLError()
{
    return s_lastGLError;
}

void OpenGLRenderBackend::PrintLastGLError(const char *format, ...)
{
    char buffer[128];
    va_list ap;

    va_start(ap, format);
    Y_vsnprintf(buffer, countof(buffer), format, ap);
    va_end(ap);

    GLenum error = s_lastGLError;
    if (error != GL_NO_ERROR)
        Log_ErrorPrintf("%s%s (0x%X)", buffer, NameTable_GetNameString(NameTables::GLErrors, error), error);
    else
        Log_ErrorPrintf("%sno error", buffer);
}

bool OpenGLRenderBackend::CheckTexturePixelFormatCompatibility(PIXEL_FORMAT PixelFormat, PIXEL_FORMAT *CompatibleFormat /*= NULL*/) const
{
    // @TODO
    return true;
}

RENDERER_PLATFORM OpenGLRenderBackend::GetPlatform() const
{
    return RENDERER_PLATFORM_OPENGL;
}

RENDERER_FEATURE_LEVEL OpenGLRenderBackend::GetFeatureLevel() const
{
    return m_featureLevel;
}

TEXTURE_PLATFORM OpenGLRenderBackend::GetTexturePlatform() const
{
    return m_texturePlatform;
}

void OpenGLRenderBackend::GetCapabilities(RendererCapabilities *pCapabilities) const
{
    // run glget calls
    uint32 maxTextureAnisotropy = 0;
    uint32 maxVertexAttributes = 0;
    uint32 maxColorAttachments = 0;
    uint32 maxUniformBufferBindings = 0;
    uint32 maxTextureUnits = 0;
    uint32 maxRenderTargets = 0;
    glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, reinterpret_cast<GLint *>(&maxTextureAnisotropy));
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, reinterpret_cast<GLint *>(&maxVertexAttributes));
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, reinterpret_cast<GLint *>(&maxColorAttachments));
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, reinterpret_cast<GLint *>(&maxUniformBufferBindings));
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, reinterpret_cast<GLint *>(&maxTextureUnits));
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, reinterpret_cast<GLint *>(&maxRenderTargets));

    pCapabilities->MaxTextureAnisotropy = maxTextureAnisotropy;
    pCapabilities->MaximumVertexBuffers = maxVertexAttributes;
    pCapabilities->MaximumConstantBuffers = maxUniformBufferBindings;
    pCapabilities->MaximumTextureUnits = maxTextureUnits;
    pCapabilities->MaximumSamplers = maxTextureUnits;
    pCapabilities->MaximumRenderTargets = maxRenderTargets;
    pCapabilities->MaxTextureAnisotropy = maxTextureAnisotropy;
    //pCapabilities->SupportsMultithreadedResourceCreation = true;
    pCapabilities->SupportsMultithreadedResourceCreation = false;
    pCapabilities->SupportsDrawBaseVertex = true; // @TODO
    pCapabilities->SupportsDepthTextures = (GLAD_GL_ARB_depth_texture == GL_TRUE);
    pCapabilities->SupportsTextureArrays = (GLAD_GL_EXT_texture_array == GL_TRUE);
    pCapabilities->SupportsCubeMapTextureArrays = (GLAD_GL_ARB_texture_cube_map_array == GL_TRUE);
    pCapabilities->SupportsGeometryShaders = (GLAD_GL_EXT_geometry_shader4 == GL_TRUE);
    pCapabilities->SupportsSinglePassCubeMaps = (GLAD_GL_EXT_geometry_shader4 == GL_TRUE && GLAD_GL_ARB_viewport_array == GL_TRUE);
    pCapabilities->SupportsInstancing = (GLAD_GL_EXT_draw_instanced == GL_TRUE);

}

GPUDevice *OpenGLRenderBackend::CreateDeviceInterface()
{
    // Create another SDL GL Context. Everything should already be initialized from the first context.
    SDL_GLContext newContext = nullptr;
    QUEUE_BLOCKING_RENDERER_LAMBA_COMMAND([this, &newContext]()
    {
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
        newContext = SDL_GL_CreateContext(m_pImplicitOutputBuffer->GetSDLWindow());
        if (newContext == nullptr)
        {
            Log_ErrorPrintf("SDL_GL_CreateContext failed: %s", SDL_GetError());
            return;
        }

        // restore old context
        SDL_GL_MakeCurrent(m_pImplicitOutputBuffer->GetSDLWindow(), m_pGPUDevice->GetSDLGLContext());
    });
    if (newContext == nullptr)
        return nullptr;

    // Activate this context.
    SDL_GL_MakeCurrent(m_pImplicitOutputBuffer->GetSDLWindow(), newContext);

    // Create a device wrapping this context.
    return new OpenGLGPUDevice(newContext, m_outputBackBufferFormat, m_outputDepthStencilFormat);
}

bool OpenGLRenderBackend_Create(const RendererInitializationParameters *pCreateParameters, SDL_Window *pSDLWindow, RenderBackend **ppBackend, GPUDevice **ppDevice, GPUContext **ppContext, GPUOutputBuffer **ppOutputBuffer)
{
    OpenGLRenderBackend *pBackend = new OpenGLRenderBackend();
    if (!pBackend->Create(pCreateParameters, pSDLWindow, ppBackend, ppDevice, ppContext, ppOutputBuffer))
    {
        pBackend->Shutdown();
        return false;
    }

    return true;
}

