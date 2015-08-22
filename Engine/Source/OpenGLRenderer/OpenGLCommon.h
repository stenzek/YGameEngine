#pragma once
#include "Renderer/Common.h"
#include "Renderer/Renderer.h"

// include sdl before GLEW
#include "Engine/SDLHeaders.h"
#include "glad/glad.h"

// Forward declare all our types
class OpenGLGPUBuffer;
class OpenGLGPUConstants;
class OpenGLGPUContext;
class OpenGLGPUQuery;
class OpenGLGPUShaderProgram;
class OpenGLGPUTexture1D;
class OpenGLGPUTexture1DArray;
class OpenGLGPUTexture2D;
class OpenGLGPUTexture2DArray;
class OpenGLGPUTexture3D;
class OpenGLGPUTextureCube;
class OpenGLGPUTextureCubeArray;
class OpenGLGPUDepthTexture;
class OpenGLGPUDevice;
class OpenGLShaderCompiler;
class OpenGLGPUSamplerState;
class OpenGLGPURasterizerState;
class OpenGLGPUDepthStencilState;
class OpenGLGPUBlendState;

#include "OpenGLRenderer/OpenGLDefines.h"
#include "OpenGLRenderer/OpenGLCVars.h"
