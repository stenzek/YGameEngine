#pragma once
#include "Renderer/Common.h"
#include "Renderer/RendererTypes.h"

class VertexBufferBindingArray
{
public:
    VertexBufferBindingArray();
    VertexBufferBindingArray(const VertexBufferBindingArray &vbb);
    ~VertexBufferBindingArray();

    GPUBuffer *GetBuffer(uint32 bufferIndex) const { DebugAssert(bufferIndex < GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS); return m_pVertexBuffers[bufferIndex]; }
    uint32 GetBufferOffset(uint32 bufferIndex) const { DebugAssert(bufferIndex < GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS); return m_iVertexBufferOffsets[bufferIndex]; }
    uint32 GetBufferStride(uint32 bufferIndex) const { DebugAssert(bufferIndex < GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS); return m_iVertexBufferStrides[bufferIndex]; }

    GPUBuffer *const *GetBuffers() const { return m_pVertexBuffers; }
    const uint32 *GetBufferOffsets() const { return m_iVertexBufferOffsets; }
    const uint32 *GetBufferStrides() const { return m_iVertexBufferStrides; }
    const uint32 GetActiveBufferCount() const { return m_nActiveBuffers; }
    
    void SetBuffer(uint32 bufferIndex, GPUBuffer *pVertexBuffer, uint32 offset, uint32 stride);
    void BindBuffers(GPUCommandList *pCommandList) const;
    void Clear();

    // sets debug name on all bound buffers
    void SetDebugName(const char *debugName);

    VertexBufferBindingArray &operator=(const VertexBufferBindingArray &vbb);

private:
    GPUBuffer *m_pVertexBuffers[GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS];
    uint32 m_iVertexBufferOffsets[GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS];
    uint32 m_iVertexBufferStrides[GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS];
    uint32 m_nActiveBuffers;
};