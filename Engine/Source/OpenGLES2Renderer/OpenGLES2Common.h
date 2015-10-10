#pragma once
#include "Renderer/Common.h"
#include "Renderer/Renderer.h"

// include sdl before GLEW
#include "Engine/SDLHeaders.h"

// use sdl's opengles2 header
#if 1
    #include <glad/glad.h>
#else
    #include <SDL_opengles2.h>

    // to get around SDL's lack of DXTC types
    #ifndef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
        #define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
    #endif
    #ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
        #define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
    #endif
    #ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
        #define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
    #endif
    #ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
        #define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
    #endif
#endif

// Forward declare all our types
class OpenGLES2GPUBuffer;
class OpenGLES2GPUContext;
class OpenGLES2GPUShaderProgram;
class OpenGLES2GPUTexture2D;
class OpenGLES2GPUTextureCube;
class OpenGLES2GPUDepthTexture;
class OpenGLES2GPUDevice;
class OpenGLES2GPURasterizerState;
class OpenGLES2GPUDepthStencilState;
class OpenGLES2GPUBlendState;
class OpenGLES2GPUOutputBuffer;
class OpenGLES2GPURenderTargetView;
class OpenGLES2GPUDepthStencilBufferView;
class OpenGLES2ConstantLibrary;

#include "OpenGLES2Renderer/OpenGLES2Defines.h"
#include "OpenGLES2Renderer/OpenGLES2CVars.h"
