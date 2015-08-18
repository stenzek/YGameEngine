#pragma once
#include "OpenGLRenderer/OpenGLCommon.h"

class OpenGLGPUQuery : public GPUQuery
{
public:
    OpenGLGPUQuery(GPU_QUERY_TYPE type, GLuint id);
    ~OpenGLGPUQuery();

    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override final;
    virtual void SetDebugName(const char *name) override final;

    virtual GPU_QUERY_TYPE GetQueryType() const override final { return m_eType; }

    GLuint GetGLQueryID() const { return m_iGLQueryId; }

    OpenGLGPUContext *GetOwningContext() const { return m_pOwningContext; }
    void SetOwningContext(OpenGLGPUContext *pContext) { m_pOwningContext = pContext; }

protected:
    GPU_QUERY_TYPE m_eType;
    GLuint m_iGLQueryId;
    OpenGLGPUContext *m_pOwningContext;
};
