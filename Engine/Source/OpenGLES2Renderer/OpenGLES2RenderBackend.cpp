#include "OpenGLES2Renderer/PrecompiledHeader.h"
#include "OpenGLES2Renderer/OpenGLES2GPUContext.h"
#include "OpenGLES2Renderer/OpenGLES2GPUBuffer.h"
#include "OpenGLES2Renderer/OpenGLES2GPUOutputBuffer.h"
#include "OpenGLES2Renderer/OpenGLES2GPUDevice.h"
#include "Engine/EngineCVars.h"
#include "Engine/SDLHeaders.h"
Log_SetChannel(OpenGLES2RenderBackend);

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

bool OpenGLES2RenderBackend_Create(const RendererInitializationParameters *pCreateParameters, SDL_Window *pSDLWindow, GPUDevice **ppDevice, GPUContext **ppImmediateContext, GPUOutputBuffer **ppOutputBuffer)
{
    // select formats
    PIXEL_FORMAT outputBackBufferFormat = pCreateParameters->BackBufferFormat;
    PIXEL_FORMAT outputDepthStencilFormat = pCreateParameters->DepthStencilBufferFormat;

    // if the backbuffer format is unspecified, use the best for the platform
    if (outputBackBufferFormat == PIXEL_FORMAT_UNKNOWN)
    {
        // by default this is rgba8
        outputBackBufferFormat = PIXEL_FORMAT_R8G8B8A8_UNORM;
    }

    // initialize colour attributes
    if (!SetSDLGLColorAttributes(outputBackBufferFormat, outputDepthStencilFormat))
    {
        Log_ErrorPrintf("OpenGLES2RenderBackend::Create: Failed to set opengl window system parameters.");
        return false;
    }

    // no sharing possible as there is no current context
    SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 0);

    // set the attributes
    SetSDLGLVersionAttributes();

    // create context
    Log_DevPrintf("OpenGLES2RenderBackend::Create: Trying to create a GLES 2.0 context");
    SDL_GLContext pSDLGLContext = SDL_GL_CreateContext(pSDLWindow);
    if (pSDLGLContext == nullptr)
    {
        Log_ErrorPrintf("OpenGLES2RenderBackend::Create: Failed to create an acceptable GL context");
        return false;
    }

    // log+return
    Log_InfoPrintf("OpenGLES2RenderBackend::Create: Got a GLES 2.0 context.");

    // initialize glew
    if (!InitializeGLAD())
    {
        Log_ErrorPrintf("OpenGLRenderBackend::Create: Failed to initialize GLEW");
        SDL_GL_MakeCurrent(nullptr, nullptr);
        SDL_GL_DeleteContext(pSDLGLContext);
        return false;
    }

    // set context settings
    RENDERER_FEATURE_LEVEL featureLevel = RENDERER_FEATURE_LEVEL_ES2;
    TEXTURE_PLATFORM texturePlatform = TEXTURE_PLATFORM_ES2_DXTC;

    // log some info
    {
        Log_InfoPrintf("OpenGL ES2 Renderer Feature Level: %s", NameTable_GetNameString(NameTables::RendererFeatureLevelFullName, featureLevel));
        Log_InfoPrintf("Texture Platform: %s", NameTable_GetNameString(NameTables::TexturePlatform, texturePlatform));

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

    // create output buffer
    OpenGLES2GPUOutputBuffer *pOutputBuffer = new OpenGLES2GPUOutputBuffer(pSDLWindow, outputBackBufferFormat, outputDepthStencilFormat, pCreateParameters->ImplicitSwapChainVSyncType, false);

    // create device and context
    OpenGLES2GPUDevice *pGPUDevice = new OpenGLES2GPUDevice(pSDLGLContext, featureLevel, texturePlatform, outputBackBufferFormat, outputDepthStencilFormat);
    OpenGLES2GPUContext *pGPUContext = new OpenGLES2GPUContext(pGPUDevice, pSDLGLContext, pOutputBuffer);
    if (!pGPUContext->Create())
    {
        Log_ErrorPrintf("OpenGLRenderBackend::Create: Could not create device context.");
        pGPUContext->Release();
        pGPUDevice->Release();
        return false;
    }

    // set pointers
    *ppDevice = pGPUDevice;
    *ppImmediateContext = pGPUContext;
    *ppOutputBuffer = pOutputBuffer;

    Log_InfoPrint("OpenGL ES 2.0 render backend creation successful.");
    return true;
}
