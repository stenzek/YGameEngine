#pragma once
#include "OpenGLRenderer/OpenGLCommon.h"

class OpenGLGPUBuffer : public GPUBuffer
{
public:
    OpenGLGPUBuffer(const GPU_BUFFER_DESC *pBufferDesc, GLuint glBufferId, GLenum glBufferUsage);
    virtual ~OpenGLGPUBuffer();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *debugName) override;

    const GLuint GetGLBufferId() const { return m_glBufferId; }
    const GLenum GetGLBufferUsage() const { return m_glBufferUsage; }

    OpenGLGPUContext *GetMappedContext() const { return m_pMappedContext; }
    void *GetMappedPointer() const { return m_pMappedPointer; }
    void SetMappedContextPointer(OpenGLGPUContext *pContext, void *pPointer) { m_pMappedContext = pContext; m_pMappedPointer = pPointer; }

private:
    GLuint m_glBufferId;
    GLenum m_glBufferUsage;

    OpenGLGPUContext *m_pMappedContext;
    void *m_pMappedPointer;
};

