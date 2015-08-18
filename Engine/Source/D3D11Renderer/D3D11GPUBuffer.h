#pragma once
#include "D3D11Renderer/D3D11Common.h"

class D3D11GPUBuffer : public GPUBuffer
{
public:
    D3D11GPUBuffer(const GPU_BUFFER_DESC *pBufferDesc, ID3D11Buffer *pD3DBuffer, ID3D11Buffer *pD3DStagingBuffer);
    virtual ~D3D11GPUBuffer();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *debugName) override;

    ID3D11Buffer *GetD3DBuffer() { return m_pD3DBuffer; }
    ID3D11Buffer *GetD3DStagingBuffer() { return m_pD3DStagingBuffer; }
    D3D11GPUContext *GetMappedContext() const { return m_pMappedContext; }
    void *GetMappedPointer() const { return m_pMappedPointer; }
    void SetMappedContextPointer(D3D11GPUContext *pContext, void *pPointer) { m_pMappedContext = pContext; m_pMappedPointer = pPointer; }

private:
    ID3D11Buffer *m_pD3DBuffer;
    ID3D11Buffer *m_pD3DStagingBuffer;
    D3D11GPUContext *m_pMappedContext;
    void *m_pMappedPointer;
};
