#include "OpenGLRenderer/PrecompiledHeader.h"
#include "OpenGLRenderer/OpenGLGPUBuffer.h"
#include "OpenGLRenderer/OpenGLGPUContext.h"
#include "OpenGLRenderer/OpenGLGPUDevice.h"
Log_SetChannel(OpenGLGPUBuffer);

OpenGLGPUBuffer::OpenGLGPUBuffer(const GPU_BUFFER_DESC *pBufferDesc, GLuint glBufferId, GLenum glBufferUsage)
    : GPUBuffer(pBufferDesc),
      m_glBufferId(glBufferId),
      m_glBufferUsage(glBufferUsage),
      m_pMappedContext(NULL),
      m_pMappedPointer(NULL)
{

}

OpenGLGPUBuffer::~OpenGLGPUBuffer()
{
    DebugAssert(m_pMappedContext == NULL);
    glDeleteBuffers(1, &m_glBufferId);
}

void OpenGLGPUBuffer::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = m_desc.Size;
}

void OpenGLGPUBuffer::SetDebugName(const char *debugName)
{
    OpenGLHelpers::SetObjectDebugName(GL_BUFFER, m_glBufferId, debugName);
}

GPUBuffer *OpenGLGPUDevice::CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData /* = NULL */)
{
    GL_CHECKED_SECTION_BEGIN();

    GLuint bufferId = 0;
    glGenBuffers(1, &bufferId);
    if (bufferId == 0)
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateBuffer: Buffer allocation failed.");
        return NULL;
    }

    GLenum bufferUsage;
    if (pDesc->Flags & GPU_BUFFER_FLAG_MAPPABLE)
    {
        bufferUsage = GL_DYNAMIC_DRAW;
    }
    else if (pDesc->Flags & (GPU_BUFFER_FLAG_READABLE | GPU_BUFFER_FLAG_WRITABLE))
    {
        bufferUsage = GL_STREAM_DRAW;
    }
    else
    {
        DebugAssert(pInitialData != NULL);
        bufferUsage = GL_STATIC_DRAW;
    }

    // allocate buffer storage
    glBindBuffer(GL_ARRAY_BUFFER, bufferId);
    glBufferData(GL_ARRAY_BUFFER, pDesc->Size, pInitialData, bufferUsage);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // check error state
    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("OpenGLGPUDevice::CreateBuffer: One or more GL calls failed.");
        if (bufferId != 0)
            glDeleteBuffers(1, &bufferId);

        return NULL;
    }

    // create object
    return new OpenGLGPUBuffer(pDesc, bufferId, bufferUsage);
}

bool OpenGLGPUContext::ReadBuffer(GPUBuffer *pBuffer, void *pDestination, uint32 start, uint32 count)
{
    OpenGLGPUBuffer *pOpenGLBuffer = static_cast<OpenGLGPUBuffer *>(pBuffer);
    DebugAssert((start + count) <= pOpenGLBuffer->GetDesc()->Size);
    DebugAssert(pOpenGLBuffer->GetMappedPointer() == NULL);

    GL_CHECKED_SECTION_BEGIN();

    glBindBuffer(GL_ARRAY_BUFFER, pOpenGLBuffer->GetGLBufferId());
    glGetBufferSubData(GL_ARRAY_BUFFER, start, count, pDestination);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("D3D11GPUContext::ReadBuffer: One or more GL calls failed.");
        return false;
    }

    return true;
}

bool OpenGLGPUContext::WriteBuffer(GPUBuffer *pBuffer, const void *pSource, uint32 start, uint32 count)
{
    OpenGLGPUBuffer *pOpenGLBuffer = static_cast<OpenGLGPUBuffer *>(pBuffer);
    DebugAssert((start + count) <= pOpenGLBuffer->GetDesc()->Size);
    DebugAssert(pOpenGLBuffer->GetMappedPointer() == NULL);

    GL_CHECKED_SECTION_BEGIN();

    if (GLAD_GL_EXT_direct_state_access)
    {
        glNamedBufferSubDataEXT(pOpenGLBuffer->GetGLBufferId(), start, count, pSource);
    }
    else
    {
        glBindBuffer(GL_ARRAY_BUFFER, pOpenGLBuffer->GetGLBufferId());
        glBufferSubData(GL_ARRAY_BUFFER, start, count, pSource);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("D3D11GPUContext::ReadBuffer: One or more GL calls failed.");
        return false;
    }

    return true;
}

bool OpenGLGPUContext::MapBuffer(GPUBuffer *pBuffer, GPU_MAP_TYPE mapType, void **ppPointer)
{
    OpenGLGPUBuffer *pOpenGLBuffer = static_cast<OpenGLGPUBuffer *>(pBuffer);
    DebugAssert(pOpenGLBuffer->GetDesc()->Flags & GPU_BUFFER_FLAG_MAPPABLE);
    DebugAssert(pOpenGLBuffer->GetMappedContext() == NULL && pOpenGLBuffer->GetMappedPointer() == NULL);

    GLbitfield access = 0;
    switch(mapType)
    {
    case GPU_MAP_TYPE_READ:
        access = GL_MAP_READ_BIT;
        break;

    case GPU_MAP_TYPE_READ_WRITE:
        access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        break;

    case GPU_MAP_TYPE_WRITE:
        access = GL_MAP_WRITE_BIT;
        break;

    case GPU_MAP_TYPE_WRITE_DISCARD:
        access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
        break;

    case GPU_MAP_TYPE_WRITE_NO_OVERWRITE:
        access = GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT;
        break;

    default:
        UnreachableCode();
        break;
    }

    GL_CHECKED_SECTION_BEGIN();

    void *pMappedPointer;
    if (GLAD_GL_EXT_direct_state_access)
    {
        pMappedPointer = glMapNamedBufferRangeEXT(pOpenGLBuffer->GetGLBufferId(), 0, pOpenGLBuffer->GetDesc()->Size, access);
        if (GL_CHECK_ERROR_STATE())
        {
            GL_PRINT_ERROR("OpenGLGPUContext::Unmapbuffer: glMapNamedBufferRange failed: ");
            if (pMappedPointer != NULL)
                glUnmapNamedBufferEXT(pOpenGLBuffer->GetGLBufferId());
        }
    }
    else
    {
        glBindBuffer(GL_ARRAY_BUFFER, pOpenGLBuffer->GetGLBufferId());
        pMappedPointer = glMapBufferRange(GL_ARRAY_BUFFER, 0, pOpenGLBuffer->GetDesc()->Size, access);

        if (GL_CHECK_ERROR_STATE())
        {
            GL_PRINT_ERROR("OpenGLGPUContext::MapBuffer: One or more GL calls failed.");
            if (pMappedPointer != NULL)
                glUnmapBuffer(GL_ARRAY_BUFFER);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            return false;
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    pOpenGLBuffer->SetMappedContextPointer(this, pMappedPointer);
    *ppPointer = pMappedPointer;
    return true;
}

void OpenGLGPUContext::Unmapbuffer(GPUBuffer *pBuffer, void *pPointer)
{
    OpenGLGPUBuffer *pOpenGLBuffer = static_cast<OpenGLGPUBuffer *>(pBuffer);
    DebugAssert(pOpenGLBuffer->GetDesc()->Flags & GPU_BUFFER_FLAG_MAPPABLE);
    DebugAssert(pOpenGLBuffer->GetMappedContext() == this && pOpenGLBuffer->GetMappedPointer() == pPointer);

    GL_CHECKED_SECTION_BEGIN();

    if (GLAD_GL_EXT_direct_state_access)
    {
        if (glUnmapNamedBufferEXT(pOpenGLBuffer->GetGLBufferId()) != GL_TRUE)
            GL_PRINT_ERROR("OpenGLGPUContext::Unmapbuffer: glUnmapNamedBuffer failed: ");
    }
    else
    {
        glBindBuffer(GL_ARRAY_BUFFER, pOpenGLBuffer->GetGLBufferId());
        GLboolean unmapResult = glUnmapBuffer(GL_ARRAY_BUFFER);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        if (unmapResult != GL_TRUE)
            GL_PRINT_ERROR("OpenGLGPUContext::Unmapbuffer: glUnmapBuffer failed: ");
    }

    pOpenGLBuffer->SetMappedContextPointer(NULL, NULL);
}

