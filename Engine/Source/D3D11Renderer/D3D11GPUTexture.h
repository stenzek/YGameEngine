#pragma once
#include "D3D11Renderer/D3D11Common.h"

class D3D11GPUTexture1D : public GPUTexture1D
{
public:
    D3D11GPUTexture1D(const GPU_TEXTURE1D_DESC *pDesc,
                      ID3D11Texture1D *pD3DTexture, ID3D11Texture1D *pD3DStagingTexture,
                      ID3D11ShaderResourceView *pD3DSRV,
                      ID3D11SamplerState *pD3DSamplerState);

    virtual ~D3D11GPUTexture1D();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11Texture1D *GetD3DTexture() const { return m_pD3DTexture; }
    ID3D11Texture1D *GetD3DStagingTexture() const { return m_pD3DStagingTexture; }

    ID3D11ShaderResourceView *GetD3DSRV() const { return m_pD3DSRV; }

    ID3D11SamplerState *GetD3DSamplerState() const { return m_pD3DSamplerState; }

protected:
    ID3D11Texture1D *m_pD3DTexture;
    ID3D11Texture1D *m_pD3DStagingTexture;

    ID3D11ShaderResourceView *m_pD3DSRV;

    ID3D11SamplerState *m_pD3DSamplerState;
};

class D3D11GPUTexture1DArray : public GPUTexture1DArray
{
public:
    D3D11GPUTexture1DArray(const GPU_TEXTURE1DARRAY_DESC *pDesc,
                           ID3D11Texture1D *pD3DTexture, ID3D11Texture1D *pD3DStagingTexture,
                           ID3D11ShaderResourceView *pD3DSRV,
                           ID3D11SamplerState *pD3DSamplerState);

    virtual ~D3D11GPUTexture1DArray();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11Texture1D *GetD3DTexture() const { return m_pD3DTexture; }
    ID3D11Texture1D *GetD3DStagingTexture() const { return m_pD3DStagingTexture; }

    ID3D11ShaderResourceView *GetD3DSRV() const { return m_pD3DSRV; }
    ID3D11SamplerState *GetD3DSamplerState() const { return m_pD3DSamplerState; }

protected:
    ID3D11Texture1D *m_pD3DTexture;
    ID3D11Texture1D *m_pD3DStagingTexture;

    ID3D11ShaderResourceView *m_pD3DSRV;
    ID3D11SamplerState *m_pD3DSamplerState;
};

class D3D11GPUTexture2D : public GPUTexture2D
{
public:
    D3D11GPUTexture2D(const GPU_TEXTURE2D_DESC *pDesc,
                      ID3D11Texture2D *pD3DTexture, ID3D11Texture2D *pD3DStagingTexture,
                      ID3D11ShaderResourceView *pD3DSRV,
                      ID3D11SamplerState *pD3DSamplerState);

    virtual ~D3D11GPUTexture2D();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11Texture2D *GetD3DTexture() const { return m_pD3DTexture; }
    ID3D11Texture2D *GetD3DStagingTexture() const { return m_pD3DStagingTexture; }

    ID3D11ShaderResourceView *GetD3DSRV() const { return m_pD3DSRV; }
    ID3D11SamplerState *GetD3DSamplerState() const { return m_pD3DSamplerState; }

protected:
    ID3D11Texture2D *m_pD3DTexture;
    ID3D11Texture2D *m_pD3DStagingTexture;

    ID3D11ShaderResourceView *m_pD3DSRV;
    ID3D11SamplerState *m_pD3DSamplerState;
};

class D3D11GPUTexture2DArray : public GPUTexture2DArray
{
public:
    D3D11GPUTexture2DArray(const GPU_TEXTURE2DARRAY_DESC *pDesc,
                           ID3D11Texture2D *pD3DTexture, ID3D11Texture2D *pD3DStagingTexture,
                           ID3D11ShaderResourceView *pD3DSRV,
                           ID3D11SamplerState *pD3DSamplerState);

    virtual ~D3D11GPUTexture2DArray();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11Texture2D *GetD3DTexture() const { return m_pD3DTexture; }
    ID3D11Texture2D *GetD3DStagingTexture() const { return m_pD3DStagingTexture; }

    ID3D11ShaderResourceView *GetD3DSRV() const { return m_pD3DSRV; }
    ID3D11SamplerState *GetD3DSamplerState() const { return m_pD3DSamplerState; }

protected:
    ID3D11Texture2D *m_pD3DTexture;
    ID3D11Texture2D *m_pD3DStagingTexture;

    ID3D11ShaderResourceView *m_pD3DSRV;
    ID3D11SamplerState *m_pD3DSamplerState;
};

class D3D11GPUTexture3D : public GPUTexture3D
{
public:
    D3D11GPUTexture3D(const GPU_TEXTURE3D_DESC *pDesc,
                      ID3D11Texture3D *pD3DTexture, ID3D11Texture3D *pD3DStagingTexture,
                      ID3D11ShaderResourceView *pD3DSRV,
                      ID3D11SamplerState *pD3DSamplerState);

    virtual ~D3D11GPUTexture3D();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11Texture3D *GetD3DTexture() const { return m_pD3DTexture; }
    ID3D11Texture3D *GetD3DStagingTexture() const { return m_pD3DStagingTexture; }

    ID3D11ShaderResourceView *GetD3DSRV() const { return m_pD3DSRV; }
    ID3D11SamplerState *GetD3DSamplerState() const { return m_pD3DSamplerState; }

protected:
    ID3D11Texture3D *m_pD3DTexture;
    ID3D11Texture3D *m_pD3DStagingTexture;

    ID3D11ShaderResourceView *m_pD3DSRV;
    ID3D11SamplerState *m_pD3DSamplerState;
};

class D3D11GPUTextureCube : public GPUTextureCube
{
public:
    D3D11GPUTextureCube(const GPU_TEXTURECUBE_DESC *pDesc,
                        ID3D11Texture2D *pD3DTexture, ID3D11Texture2D *pD3DStagingTexture,
                        ID3D11ShaderResourceView *pD3DSRV,
                        ID3D11SamplerState *pD3DSamplerState);

    virtual ~D3D11GPUTextureCube();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11Texture2D *GetD3DTexture() const { return m_pD3DTexture; }
    ID3D11Texture2D *GetD3DStagingTexture() const { return m_pD3DStagingTexture; }

    ID3D11ShaderResourceView *GetD3DSRV() const { return m_pD3DSRV; }
    ID3D11SamplerState *GetD3DSamplerState() const { return m_pD3DSamplerState; }

protected:
    ID3D11Texture2D *m_pD3DTexture;
    ID3D11Texture2D *m_pD3DStagingTexture;

    ID3D11ShaderResourceView *m_pD3DSRV;
    ID3D11SamplerState *m_pD3DSamplerState;
};

class D3D11GPUTextureCubeArray : public GPUTextureCubeArray
{
public:
    D3D11GPUTextureCubeArray(const GPU_TEXTURECUBEARRAY_DESC *pDesc,
                             ID3D11Texture2D *pD3DTexture, ID3D11Texture2D *pD3DStagingTexture,
                             ID3D11ShaderResourceView *pD3DSRV,
                             ID3D11SamplerState *pD3DSamplerState);

    virtual ~D3D11GPUTextureCubeArray();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11Texture2D *GetD3DTexture() const { return m_pD3DTexture; }
    ID3D11Texture2D *GetD3DStagingTexture() const { return m_pD3DStagingTexture; }

    ID3D11ShaderResourceView *GetD3DSRV() const { return m_pD3DSRV; }
    ID3D11SamplerState *GetD3DSamplerState() const { return m_pD3DSamplerState; }

protected:
    ID3D11Texture2D *m_pD3DTexture;
    ID3D11Texture2D *m_pD3DStagingTexture;

    ID3D11ShaderResourceView *m_pD3DSRV;
    ID3D11SamplerState *m_pD3DSamplerState;
};

class D3D11GPUDepthTexture : public GPUDepthTexture
{
public:
    D3D11GPUDepthTexture(const GPU_DEPTH_TEXTURE_DESC *pDesc,
                         ID3D11Texture2D *pD3DTexture,
                         ID3D11Texture2D *pD3DStagingTexture);

    virtual ~D3D11GPUDepthTexture();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11Texture2D *GetD3DTexture() const { return m_pD3DTexture; }
    ID3D11Texture2D *GetD3DStagingTexture() const { return m_pD3DStagingTexture; }

protected:
    ID3D11Texture2D *m_pD3DTexture;
    ID3D11Texture2D *m_pD3DStagingTexture;
};

class D3D11GPURenderTargetView : public GPURenderTargetView
{
public:
    D3D11GPURenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc, ID3D11RenderTargetView *pD3DRTV, ID3D11Resource *pD3DResource);
    virtual ~D3D11GPURenderTargetView();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11RenderTargetView *GetD3DRTV() const { return m_pD3DRTV; }
    ID3D11Resource *GetD3DResource() const { return m_pD3DResource; }

protected:
    ID3D11RenderTargetView *m_pD3DRTV;
    ID3D11Resource *m_pD3DResource;
};

class D3D11GPUDepthStencilBufferView : public GPUDepthStencilBufferView
{
public:
    D3D11GPUDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc, ID3D11DepthStencilView *pD3DDSV, ID3D11Resource *pD3DResource);
    virtual ~D3D11GPUDepthStencilBufferView();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11DepthStencilView *GetD3DDSV() const { return m_pD3DDSV; }
    ID3D11Resource *GetD3DResource() const { return m_pD3DResource; }

protected:
    ID3D11DepthStencilView *m_pD3DDSV;
    ID3D11Resource *m_pD3DResource;
};

class D3D11GPUComputeView : public GPUComputeView
{
public:
    D3D11GPUComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc, ID3D11UnorderedAccessView *pD3DUAV, ID3D11Resource *pD3DResource);
    virtual ~D3D11GPUComputeView();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11UnorderedAccessView *GetD3DUAV() const { return m_pD3DUAV; }
    ID3D11Resource *GetD3DResource() const { return m_pD3DResource; }

protected:
    ID3D11UnorderedAccessView *m_pD3DUAV;
    ID3D11Resource *m_pD3DResource;
};
