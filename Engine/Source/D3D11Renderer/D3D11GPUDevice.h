#pragma once
#include "D3D11Renderer/D3D11Common.h"
#include "D3D11Renderer/D3D11GPUContext.h"
#include "D3D11Renderer/D3D11GPUOutputBuffer.h"

class D3D11SamplerState : public GPUSamplerState
{
public:
    D3D11SamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, ID3D11SamplerState *pD3DSamplerState);
    virtual ~D3D11SamplerState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11SamplerState *GetD3DSamplerState() { return m_pD3DSamplerState; }

private:
    ID3D11SamplerState *m_pD3DSamplerState;
};

class D3D11RasterizerState : public GPURasterizerState
{
public:
    D3D11RasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc, ID3D11RasterizerState *pD3DRasterizerState);
    virtual ~D3D11RasterizerState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11RasterizerState *GetD3DRasterizerState() { return m_pD3DRasterizerState; }

private:
    ID3D11RasterizerState *m_pD3DRasterizerState;
};

class D3D11DepthStencilState : public GPUDepthStencilState
{
public:
    D3D11DepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc, ID3D11DepthStencilState *pD3DDepthStencilState);
    virtual ~D3D11DepthStencilState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11DepthStencilState *GetD3DDepthStencilState() { return m_pD3DDepthStencilState; }

private:
    ID3D11DepthStencilState *m_pD3DDepthStencilState;
};

class D3D11BlendState : public GPUBlendState
{
public:
    D3D11BlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc, ID3D11BlendState *pD3DBlendState);
    virtual ~D3D11BlendState();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11BlendState *GetD3DBlendState() { return m_pD3DBlendState; }

private:
    ID3D11BlendState *m_pD3DBlendState;
};

class D3D11GPUDevice : public GPUDevice
{
public:
    D3D11GPUDevice(IDXGIFactory *pDXGIFactory, IDXGIAdapter *pDXGIAdapter, ID3D11Device *pD3DDevice, ID3D11Device1 *pD3DDevice1, DXGI_FORMAT windowBackBufferFormat, DXGI_FORMAT windowDepthStencilFormat);
    virtual ~D3D11GPUDevice();

    // private methods
    IDXGIFactory *GetDXGIFactory() const { return m_pDXGIFactory; }
    IDXGIAdapter *GetDXGIAdapter() const { return m_pDXGIAdapter; }
    ID3D11Device *GetD3DDevice() const { return m_pD3DDevice; }
    ID3D11Device1 *GetD3DDevice1() const { return m_pD3DDevice1; }
    DXGI_FORMAT GetSwapChainBackBufferFormat() const { return m_swapChainBackBufferFormat; }
    DXGI_FORMAT GetSwapChainDepthStencilBufferFormat() const { return m_swapChainDepthStencilBufferFormat; }

    // Creates a swap chain on an existing window.
    virtual GPUOutputBuffer *CreateOutputBuffer(RenderSystemWindowHandle hWnd, RENDERER_VSYNC_TYPE vsyncType) override;

    // Resource creation
    virtual GPUQuery *CreateQuery(GPU_QUERY_TYPE type) override;
    virtual GPUBuffer *CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData = NULL) override;
    virtual GPUTexture1D *CreateTexture1D(const GPU_TEXTURE1D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTexture1DArray *CreateTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTexture2D *CreateTexture2D(const GPU_TEXTURE2D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTexture2DArray *CreateTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTexture3D *CreateTexture3D(const GPU_TEXTURE3D_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL, const uint32 *pInitialDataSlicePitch = NULL) override;
    virtual GPUTextureCube *CreateTextureCube(const GPU_TEXTURECUBE_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUTextureCubeArray *CreateTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pTextureDesc, const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const void **ppInitialData = NULL, const uint32 *pInitialDataPitch = NULL) override;
    virtual GPUDepthTexture *CreateDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pTextureDesc) override;
    virtual GPUSamplerState *CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc) override;
    virtual GPURenderTargetView *CreateRenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc) override;
    virtual GPUDepthStencilBufferView *CreateDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc) override;
    virtual GPUComputeView *CreateComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc) override;
    virtual GPUDepthStencilState *CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc) override;
    virtual GPURasterizerState *CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc) override;
    virtual GPUBlendState *CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc) override;
    virtual GPUShaderProgram *CreateGraphicsProgram(const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream) override;
    virtual GPUShaderProgram *CreateComputeProgram(ByteStream *pByteCodeStream) override;

private:
    IDXGIFactory *m_pDXGIFactory;
    IDXGIAdapter *m_pDXGIAdapter;
    ID3D11Device *m_pD3DDevice;
    ID3D11Device1 *m_pD3DDevice1;

    DXGI_FORMAT m_swapChainBackBufferFormat;
    DXGI_FORMAT m_swapChainDepthStencilBufferFormat;
};
