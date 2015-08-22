#pragma once
#include "D3D12Renderer/D3D12Common.h"
#include "D3D12Renderer/D3D12DescriptorHeap.h"

class D3D12GPURenderTargetView : public GPURenderTargetView
{
public:
    D3D12GPURenderTargetView(GPUTexture *pTexture, const GPU_RENDER_TARGET_VIEW_DESC *pDesc, const D3D12DescriptorHeap::Handle &descriptorHandle, ID3D12Resource *pD3DResource);
    virtual ~D3D12GPURenderTargetView();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const D3D12DescriptorHeap::Handle &GetDescriptorHandle() const { return m_descriptorHandle; }
    ID3D12Resource *GetD3DResource() const { return m_pD3DResource; }

protected:
    D3D12DescriptorHeap::Handle m_descriptorHandle;
    ID3D12Resource *m_pD3DResource;
};

class D3D12GPUDepthStencilBufferView : public GPUDepthStencilBufferView
{
public:
    D3D12GPUDepthStencilBufferView(GPUTexture *pTexture, const GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC *pDesc, const D3D12DescriptorHeap::Handle &descriptorHandle, ID3D12Resource *pD3DResource);
    virtual ~D3D12GPUDepthStencilBufferView();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const D3D12DescriptorHeap::Handle &GetDescriptorHandle() const { return m_descriptorHandle; }
    ID3D12Resource *GetD3DResource() const { return m_pD3DResource; }

protected:
    D3D12DescriptorHeap::Handle m_descriptorHandle;
    ID3D12Resource *m_pD3DResource;
};

class D3D12GPUComputeView : public GPUComputeView
{
public:
    D3D12GPUComputeView(GPUResource *pResource, const GPU_COMPUTE_VIEW_DESC *pDesc, const D3D12DescriptorHeap::Handle &descriptorHandle, ID3D12Resource *pD3DResource);
    virtual ~D3D12GPUComputeView();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    const D3D12DescriptorHeap::Handle &GetDescriptorHandle() const { return m_descriptorHandle; }
    ID3D12Resource *GetD3DResource() const { return m_pD3DResource; }

protected:
    D3D12DescriptorHeap::Handle m_descriptorHandle;
    ID3D12Resource *m_pD3DResource;
};

