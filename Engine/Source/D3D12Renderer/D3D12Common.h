#pragma once
#include "Renderer/Common.h"
#include "Renderer/Renderer.h"

#define INITGUID 1
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <dxgidebug.h>
#include <Uxtheme.h>
#include <VersionHelpers.h>

// Forward declare all our types
class D3D12GPUBuffer;
class D3D12GPUDevice;
class D3D12GPUContext;
class D3D12GPUQuery;
class D3D12GPUShaderProgram;
class D3D12GPUOutputBuffer;
class D3D12GPUTexture1D;
class D3D12GPUTexture1DArray;
class D3D12GPUTexture2D;
class D3D12GPUTexture2DArray;
class D3D12GPUTexture3D;
class D3D12GPUTextureCube;
class D3D12GPUTextureCubeArray;
class D3D12GPUDepthTexture;
class D3D12GPUDepthTextureArray;
class D3D12GPUSamplerState;
class D3D12RenderBackend;

#include "D3D12Renderer/D3D12Defines.h"
#include "D3D12Renderer/D3D12CVars.h"

