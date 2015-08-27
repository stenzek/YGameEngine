#pragma once
#include "D3D12Renderer/D3D12Common.h"

class D3D12GPUBuffer : public GPUBuffer
{
public:
    D3D12GPUBuffer(const GPU_BUFFER_DESC *pBufferDesc, ID3D12Resource *pD3DResource, D3D12_RESOURCE_STATES defaultResourceState);
    virtual ~D3D12GPUBuffer();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *debugName) override;

    ID3D12Resource *GetD3DResource() { return m_pD3DResource; }

    ID3D12Resource *GetMapResource() const { return m_pD3DMapResource; }
    GPU_MAP_TYPE GetMapType() const { return m_mapType; }
    void SetMapResource(ID3D12Resource *pResource, GPU_MAP_TYPE mapType) { m_pD3DMapResource = pResource; m_mapType = mapType; }

    D3D12_RESOURCE_STATES GetDefaultResourceState() const { return m_defaultResourceState; }

    ID3D12Resource *CreateUploadResource(ID3D12Device *pD3DDevice, uint32 size) const;
    ID3D12Resource *CreateReadbackResource(ID3D12Device *pD3DDevice, uint32 size) const;

private:
    ID3D12Resource *m_pD3DResource;
    ID3D12Resource *m_pD3DMapResource;
    D3D12_RESOURCE_STATES m_defaultResourceState;
    GPU_MAP_TYPE m_mapType;
};
