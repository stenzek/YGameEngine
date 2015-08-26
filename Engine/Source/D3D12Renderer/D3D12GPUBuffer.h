#pragma once
#include "D3D12Renderer/D3D12Common.h"

class D3D12GPUBuffer : public GPUBuffer
{
public:
    D3D12GPUBuffer(const GPU_BUFFER_DESC *pBufferDesc, ID3D12Resource *pD3DResource, ID3D12Resource *pD3DReadBackResource, ID3D12Resource *pD3DStagingResource);
    virtual ~D3D12GPUBuffer();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *debugName) override;

    ID3D12Resource *GetD3DResource() { return m_pD3DResource; }
    ID3D12Resource *GetD3DStagingResource() { return m_pD3DStagingResource; }

    D3D12GPUContext *GetMappedContext() const { return m_pMappedContext; }
    void *GetMappedPointer() const { return m_pMappedPointer; }
    void SetMappedContextPointer(D3D12GPUContext *pContext, void *pPointer) { m_pMappedContext = pContext; m_pMappedPointer = pPointer; }

private:
    ID3D12Resource *m_pD3DResource;
    ID3D12Resource *m_pD3DReadBackResource;
    ID3D12Resource *m_pD3DStagingResource;
    D3D12GPUContext *m_pMappedContext;
    void *m_pMappedPointer;
};
