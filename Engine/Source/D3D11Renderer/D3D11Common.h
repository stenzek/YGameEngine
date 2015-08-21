#pragma once
#include "Renderer/Common.h"
#include "Renderer/Renderer.h"

#define INITGUID 1
#include <d3d11.h>
#include <d3d11_1.h>
#include <D3Dcompiler.h>
#include <dxgidebug.h>
#include <Uxtheme.h>
#include <VersionHelpers.h>

// Forward declare all our types
class D3D11GPUBuffer;
class D3D11GPUConstants;
class D3D11GPUContext;
class D3D11GPUQuery;
class D3D11GPUShaderProgram;
class D3D11GPUOutputBuffer;
class D3D11GPUTexture1D;
class D3D11GPUTexture1DArray;
class D3D11GPUTexture2D;
class D3D11GPUTexture2DArray;
class D3D11GPUTexture3D;
class D3D11GPUTextureCube;
class D3D11GPUTextureCubeArray;
class D3D11GPUDepthTexture;
class D3D11GPUDepthTextureArray;
class D3D11GPUDevice;
class D3D11SamplerState;
class D3D11RenderBackend;

#include "D3D11Renderer/D3D11Defines.h"
#include "D3D11Renderer/D3D11CVars.h"

