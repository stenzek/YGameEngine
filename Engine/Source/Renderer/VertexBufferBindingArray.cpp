#include "Renderer/PrecompiledHeader.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Renderer/Renderer.h"

VertexBufferBindingArray::VertexBufferBindingArray()
{
    Y_memzero(m_pVertexBuffers, sizeof(m_pVertexBuffers));
    Y_memzero(m_iVertexBufferOffsets, sizeof(m_iVertexBufferOffsets));
    Y_memzero(m_iVertexBufferStrides, sizeof(m_iVertexBufferStrides));
    m_nActiveBuffers = 0;
}

VertexBufferBindingArray::VertexBufferBindingArray(const VertexBufferBindingArray &vbb)
{
    Y_memzero(m_pVertexBuffers, sizeof(m_pVertexBuffers));
    Y_memzero(m_iVertexBufferOffsets, sizeof(m_iVertexBufferOffsets));
    Y_memzero(m_iVertexBufferStrides, sizeof(m_iVertexBufferStrides));

    for (uint32 i = 0; i < vbb.m_nActiveBuffers; i++)
    {
        if ((m_pVertexBuffers[i] = vbb.m_pVertexBuffers[i]) != NULL)
            m_pVertexBuffers[i]->AddRef();

        m_iVertexBufferOffsets[i] = vbb.m_iVertexBufferOffsets[i];
        m_iVertexBufferStrides[i] = vbb.m_iVertexBufferStrides[i];
    }
    m_nActiveBuffers = vbb.m_nActiveBuffers;
}

VertexBufferBindingArray::~VertexBufferBindingArray()
{
    for (uint32 i = 0; i < m_nActiveBuffers; i++)
    {
        if (m_pVertexBuffers[i] != NULL)
            m_pVertexBuffers[i]->Release();
    }
}

void VertexBufferBindingArray::SetBuffer(uint32 bufferIndex, GPUBuffer *pVertexBuffer, uint32 offset, uint32 stride)
{
    uint32 i;
    DebugAssert(bufferIndex < GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS);

    if (m_pVertexBuffers[bufferIndex] != pVertexBuffer)
    {
        if (m_pVertexBuffers[bufferIndex] != NULL)
            m_pVertexBuffers[bufferIndex]->Release();

        if ((m_pVertexBuffers[bufferIndex] = pVertexBuffer) != NULL)
            m_pVertexBuffers[bufferIndex]->AddRef();
    }

    if (pVertexBuffer != NULL)
    {
        m_iVertexBufferOffsets[bufferIndex] = offset;
        m_iVertexBufferStrides[bufferIndex] = stride;
    }
    else
    {
        m_iVertexBufferOffsets[bufferIndex] = 0;
        m_iVertexBufferStrides[bufferIndex] = 0;
    }

    m_nActiveBuffers = 0;
    for (i = 0; i < GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS; i++)
    {
        if (m_pVertexBuffers[i] != NULL)
            m_nActiveBuffers = i + 1;
    }
}

void VertexBufferBindingArray::SetDebugName(const char *debugName)
{
    for (uint32 i = 0; i < GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS; i++)
    {
        if (m_pVertexBuffers[i] != NULL)
            m_pVertexBuffers[i]->SetDebugName(debugName);
    }
}

VertexBufferBindingArray &VertexBufferBindingArray::operator=(const VertexBufferBindingArray &vbb)
{
    for (uint32 i = 0; i < m_nActiveBuffers; i++)
    {
        if (m_pVertexBuffers[i] != NULL)
            m_pVertexBuffers[i]->Release();
    }

    Y_memzero(m_pVertexBuffers, sizeof(m_pVertexBuffers));
    Y_memzero(m_iVertexBufferOffsets, sizeof(m_iVertexBufferOffsets));
    Y_memzero(m_iVertexBufferStrides, sizeof(m_iVertexBufferStrides));

    for (uint32 i = 0; i < vbb.m_nActiveBuffers; i++)
    {
        if ((m_pVertexBuffers[i] = vbb.m_pVertexBuffers[i]) != NULL)
            m_pVertexBuffers[i]->AddRef();

        m_iVertexBufferOffsets[i] = vbb.m_iVertexBufferOffsets[i];
        m_iVertexBufferStrides[i] = vbb.m_iVertexBufferStrides[i];
    }
    m_nActiveBuffers = vbb.m_nActiveBuffers;
    
    return *this;
}

void VertexBufferBindingArray::BindBuffers(GPUCommandList *pCommandList) const
{
    pCommandList->SetVertexBuffers(0, m_nActiveBuffers, m_pVertexBuffers, m_iVertexBufferOffsets, m_iVertexBufferStrides);
}

void VertexBufferBindingArray::Clear()
{
    for (uint32 i = 0; i < m_nActiveBuffers; i++)
    {
        if (m_pVertexBuffers[i] != NULL)
            m_pVertexBuffers[i]->Release();
    }

    Y_memzero(m_pVertexBuffers, sizeof(m_pVertexBuffers));
    Y_memzero(m_iVertexBufferOffsets, sizeof(m_iVertexBufferOffsets));
    Y_memzero(m_iVertexBufferStrides, sizeof(m_iVertexBufferStrides));
    m_nActiveBuffers = 0;
}

