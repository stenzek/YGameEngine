#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
//#include "D3D12Renderer/D3D12GPUContext.h"
#include "D3D12Renderer/D3D12RenderBackend.h"
#include "D3D12Renderer/D3D12GPUOutputBuffer.h"
#include "Engine/EngineCVars.h"
Log_SetChannel(D3D12RenderBackend);

D3D12GPUSamplerState::D3D12GPUSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const D3D12_SAMPLER_DESC *pD3DSamplerDesc)
    : GPUSamplerState(pSamplerStateDesc)
{
    Y_memcpy(&m_D3DSamplerStateDesc, pD3DSamplerDesc, sizeof(m_D3DSamplerStateDesc));
}

D3D12GPUSamplerState::~D3D12GPUSamplerState()
{
    
}

void D3D12GPUSamplerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12GPUSamplerState::SetDebugName(const char *name)
{

}

D3D12RasterizerState::D3D12RasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc, const D3D12_RASTERIZER_DESC *pD3DRasterizerDesc)
    : GPURasterizerState(pRasterizerStateDesc)
{
    Y_memcpy(&m_D3DRasterizerDesc, pD3DRasterizerDesc, sizeof(m_D3DRasterizerDesc));
}

D3D12RasterizerState::~D3D12RasterizerState()
{
    
}

void D3D12RasterizerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12RasterizerState::SetDebugName(const char *name)
{

}

D3D12DepthStencilState::D3D12DepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc, const D3D12_DEPTH_STENCIL_DESC *pD3DDepthStencilDesc)
    : GPUDepthStencilState(pDepthStencilStateDesc)
{
    Y_memcpy(&m_D3DDepthStencilDesc, pD3DDepthStencilDesc, sizeof(m_D3DDepthStencilDesc));
}

D3D12DepthStencilState::~D3D12DepthStencilState()
{
    
}

void D3D12DepthStencilState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12DepthStencilState::SetDebugName(const char *name)
{

}

D3D12BlendState::D3D12BlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc, const D3D12_BLEND_DESC *pD3DBlendDesc)
    : GPUBlendState(pBlendStateDesc)
{
    Y_memcpy(&m_D3DBlendDesc, pD3DBlendDesc, sizeof(m_D3DBlendDesc));
}

D3D12BlendState::~D3D12BlendState()
{
    
}

void D3D12BlendState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12BlendState::SetDebugName(const char *name)
{

}

D3D12GPUDevice::D3D12GPUDevice(D3D12RenderBackend *pBackend, IDXGIFactory3 *pDXGIFactory, IDXGIAdapter3 *pDXGIAdapter, ID3D12Device *pD3DDevice, DXGI_FORMAT outputBackBufferFormat, DXGI_FORMAT outputDepthStencilFormat)
    : m_pBackend(pBackend)
    , m_pDXGIFactory(pDXGIFactory)
    , m_pDXGIAdapter(pDXGIAdapter)
    , m_pD3DDevice(pD3DDevice)
    , m_pGPUContext(nullptr)
    , m_outputBackBufferFormat(outputBackBufferFormat)
    , m_outputDepthStencilFormat(outputDepthStencilFormat)
{
    if (m_pDXGIFactory != nullptr)
        m_pDXGIFactory->AddRef();
    if (m_pDXGIAdapter != nullptr)
        m_pDXGIAdapter->AddRef();
    if (m_pD3DDevice != nullptr)
        m_pD3DDevice->AddRef();
}

D3D12GPUDevice::~D3D12GPUDevice()
{
    SAFE_RELEASE(m_pD3DDevice);

    SAFE_RELEASE(m_pDXGIAdapter);
    SAFE_RELEASE(m_pDXGIFactory);
}

GPUQuery *D3D12GPUDevice::CreateQuery(GPU_QUERY_TYPE type)
{
    return nullptr;
}

GPUBuffer *D3D12GPUDevice::CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData /*= nullptr*/)
{
    return nullptr;
}

GPUTexture1D *D3D12GPUDevice::CreateTexture1D(const GPU_TEXTURE1D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/)
{
    return nullptr;
}

GPUTexture1DArray *D3D12GPUDevice::CreateTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/)
{
    return nullptr;
}

GPUTexture2D *D3D12GPUDevice::CreateTexture2D(const GPU_TEXTURE2D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/)
{
    return nullptr;
}

GPUTexture2DArray *D3D12GPUDevice::CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/)
{
    return nullptr;
}

GPUTexture3D *D3D12GPUDevice::CreateTexture3D(const GPU_TEXTURE3D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/, const uint32 *pInitialDataSlicePitch /*= nullptr*/)
{
    return nullptr;
}

GPUTextureCube *D3D12GPUDevice::CreateTextureCube(const GPU_TEXTURECUBE_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/)
{
    return nullptr;
}

GPUTextureCubeArray *D3D12GPUDevice::CreateTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData /*= nullptr*/, const uint32 *pInitialDataPitch /*= nullptr*/)
{
    return nullptr;
}

GPUDepthTexture *D3D12GPUDevice::CreateDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pTextureDesc)
{
    return nullptr;
}

GPUSamplerState *D3D12GPUDevice::CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc)
{
    return nullptr;
}

GPURenderTargetView *D3D12GPUDevice::CreateRenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc)
{
    return nullptr;
}

GPUDepthStencilBufferView *D3D12GPUDevice::CreateDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc)
{
    return nullptr;
}

GPUComputeView *D3D12GPUDevice::CreateComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc)
{
    return nullptr;
}

GPUDepthStencilState *D3D12GPUDevice::CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc)
{
    return nullptr;
}

GPURasterizerState *D3D12GPUDevice::CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc)
{
    return nullptr;
}

GPUBlendState *D3D12GPUDevice::CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc)
{
    return nullptr;
}

GPUShaderProgram *D3D12GPUDevice::CreateGraphicsProgram(const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream)
{
    return nullptr;
}

GPUShaderProgram *D3D12GPUDevice::CreateComputeProgram(ByteStream *pByteCodeStream)
{
    return nullptr;
}
