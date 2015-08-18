#include "OpenGLES2Renderer/PrecompiledHeader.h"
#include "OpenGLES2Renderer/OpenGLES2GPUBuffer.h"
#include "OpenGLES2Renderer/OpenGLES2GPUContext.h"
#include "OpenGLES2Renderer/OpenGLES2Renderer.h"
Log_SetChannel(OpenGLES2GPUBuffer);

OpenGLES2GPUBuffer::OpenGLES2GPUBuffer(const GPU_BUFFER_DESC *pBufferDesc, GLuint glBufferId, GLenum glBufferTarget, GLenum glBufferUsage)
    : GPUBuffer(pBufferDesc),
      m_glBufferId(glBufferId),
      m_glBufferTarget(glBufferTarget),
      m_glBufferUsage(glBufferUsage)
{

}

OpenGLES2GPUBuffer::~OpenGLES2GPUBuffer()
{
    glDeleteBuffers(1, &m_glBufferId);
}

void OpenGLES2GPUBuffer::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = m_desc.Size;
}

void OpenGLES2GPUBuffer::SetDebugName(const char *debugName)
{
    OpenGLES2Helpers::SetObjectDebugName(GL_BUFFER, m_glBufferId, debugName);
}

GPUBuffer *OpenGLES2Renderer::CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData /* = NULL */)
{
    // find target
    GLenum bufferTarget;
    if (pDesc->Flags & GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER)
        bufferTarget = GL_ARRAY_BUFFER;
    else if (pDesc->Flags & GPU_BUFFER_FLAG_BIND_INDEX_BUFFER)
        bufferTarget = GL_ELEMENT_ARRAY_BUFFER;
    else
    {
        Log_ErrorPrintf("OpenGLES2Renderer::CreateBuffer: Buffer target unknown for flags %u", pDesc->Flags);
        return nullptr;
    }

    // find usage
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

    // allocate buffer
    GL_CHECKED_SECTION_BEGIN();
    GLuint bufferId = 0;
    glGenBuffers(1, &bufferId);
    if (bufferId == 0)
    {
        GL_PRINT_ERROR("OpenGLES2Renderer::CreateBuffer: Buffer allocation failed.");
        return NULL;
    }

    // allocate buffer storage
    glBindBuffer(bufferTarget, bufferId);
    glBufferData(bufferTarget, pDesc->Size, pInitialData, bufferUsage);
    glBindBuffer(bufferTarget, 0);

    // check error state
    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("OpenGLES2Renderer::CreateBuffer: One or more GL calls failed.");
        if (bufferId != 0)
            glDeleteBuffers(1, &bufferId);

        return NULL;
    }

    // create object
    return new OpenGLES2GPUBuffer(pDesc, bufferId, bufferTarget, bufferUsage);
}

bool OpenGLES2GPUContext::ReadBuffer(GPUBuffer *pBuffer, void *pDestination, uint32 start, uint32 count)
{
    Log_ErrorPrint("OpenGLES2GPUContext::ReadBuffer: Unsupported on GLES");
    return false;
}

bool OpenGLES2GPUContext::WriteBuffer(GPUBuffer *pBuffer, const void *pSource, uint32 start, uint32 count)
{
    OpenGLES2GPUBuffer *pOpenGLBuffer = static_cast<OpenGLES2GPUBuffer *>(pBuffer);
    DebugAssert((start + count) <= pOpenGLBuffer->GetDesc()->Size);

    GL_CHECKED_SECTION_BEGIN();

    glBindBuffer(pOpenGLBuffer->GetGLBufferTarget(), pOpenGLBuffer->GetGLBufferId());
    glBufferSubData(pOpenGLBuffer->GetGLBufferTarget(), start, count, pSource);
    glBindBuffer(pOpenGLBuffer->GetGLBufferTarget(), 0);

    if (GL_CHECK_ERROR_STATE())
    {
        GL_PRINT_ERROR("D3D11GPUContext::ReadBuffer: One or more GL calls failed.");
        return false;
    }

    return true;
}

bool OpenGLES2GPUContext::MapBuffer(GPUBuffer *pBuffer, GPU_MAP_TYPE mapType, void **ppPointer)
{
    Log_ErrorPrint("OpenGLES2GPUContext::MapBuffer: Unsupported on GLES");
    return false;
}

void OpenGLES2GPUContext::Unmapbuffer(GPUBuffer *pBuffer, void *pPointer)
{
    Log_ErrorPrint("OpenGLES2GPUContext::UnmapBuffer: Unsupported on GLES");
}

