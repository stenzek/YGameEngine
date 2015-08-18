#pragma once
#include "OpenGLES2Renderer/OpenGLES2Common.h"

class OpenGLES2GPUBuffer : public GPUBuffer
{
public:
    OpenGLES2GPUBuffer(const GPU_BUFFER_DESC *pBufferDesc, GLuint glBufferId, GLenum glBufferTarget, GLenum glBufferUsage);
    virtual ~OpenGLES2GPUBuffer();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *debugName) override;

    const GLuint GetGLBufferId() const { return m_glBufferId; }
    const GLenum GetGLBufferTarget() const { return m_glBufferTarget; }
    const GLenum GetGLBufferUsage() const { return m_glBufferUsage; }

private:
    GLuint m_glBufferId;
    GLenum m_glBufferTarget;
    GLenum m_glBufferUsage;
};

