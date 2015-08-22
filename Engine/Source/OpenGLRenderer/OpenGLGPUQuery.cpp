#include "OpenGLRenderer/PrecompiledHeader.h"
#include "OpenGLRenderer/OpenGLGPUQuery.h"
#include "OpenGLRenderer/OpenGLGPUContext.h"
#include "OpenGLRenderer/OpenGLGPUDevice.h"
#include "OpenGLRenderer/OpenGLRenderBackend.h"
//Log_SetChannel(OpenGLRenderBackend);

OpenGLGPUQuery::OpenGLGPUQuery(GPU_QUERY_TYPE type, GLuint id)
    : m_eType(type)
    , m_iGLQueryId(id)
    , m_pOwningContext(nullptr)
{

}

OpenGLGPUQuery::~OpenGLGPUQuery()
{
    DebugAssert(m_pOwningContext == nullptr);
    if (m_iGLQueryId != 0)
        glDeleteQueries(1, &m_iGLQueryId);
}

void OpenGLGPUQuery::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);

    if (gpuMemoryUsage != nullptr && m_iGLQueryId != 0)
        *gpuMemoryUsage = 128;
}

void OpenGLGPUQuery::SetDebugName(const char *name)
{
    if (m_iGLQueryId != 0)
        OpenGLHelpers::SetObjectDebugName(GL_QUERY, m_iGLQueryId, name);
}


GPUQuery *OpenGLGPUDevice::CreateQuery(GPU_QUERY_TYPE Type)
{
    GLuint glQueryId = 0;

    switch (Type)
    {
    case GPU_QUERY_TYPE_SAMPLES_PASSED:
    case GPU_QUERY_TYPE_OCCLUSION:
    case GPU_QUERY_TYPE_PRIMITIVES_GENERATED:
    case GPU_QUERY_TYPE_TIMESTAMP:
        {
            GL_CHECKED_SECTION_BEGIN();
            glGenQueries(1, &glQueryId);
            if (glQueryId == 0)
            {
                GL_PRINT_ERROR("OpenGLGPUDevice::CreateQuery: glGenQueries failed: ");
                return nullptr;
            }
        }
        break;

    case GPU_QUERY_TYPE_FREQUENCY:
        // fixed at 1000000000.0
        break;

    default:
        UnreachableCode();
        break;
    }

    FlushOffThreadCommands();

    return new OpenGLGPUQuery(Type, glQueryId);
}

bool OpenGLGPUContext::BeginQuery(GPUQuery *pQuery)
{
    OpenGLGPUQuery *pGLQuery = static_cast<OpenGLGPUQuery *>(pQuery);
    DebugAssert(pGLQuery->GetOwningContext() == nullptr);
    pGLQuery->SetOwningContext(this);

    switch (pQuery->GetQueryType())
    {
    case GPU_QUERY_TYPE_SAMPLES_PASSED:
        glBeginQuery(GL_SAMPLES_PASSED, pGLQuery->GetGLQueryID());
        return true;

    case GPU_QUERY_TYPE_OCCLUSION:
        glBeginQuery(GL_ANY_SAMPLES_PASSED, pGLQuery->GetGLQueryID());
        return true;

    case GPU_QUERY_TYPE_PRIMITIVES_GENERATED:
        glBeginQuery(GL_PRIMITIVES_GENERATED, pGLQuery->GetGLQueryID());
        return true;

    case GPU_QUERY_TYPE_TIMESTAMP:
        glQueryCounter(pGLQuery->GetGLQueryID(), GL_TIMESTAMP);
        return true;

    case GPU_QUERY_TYPE_FREQUENCY:
        return true;

    default:
        UnreachableCode();
        return false;
    }
}

bool OpenGLGPUContext::EndQuery(GPUQuery *pQuery)
{
    OpenGLGPUQuery *pGLQuery = static_cast<OpenGLGPUQuery *>(pQuery);
    DebugAssert(pGLQuery->GetOwningContext() == this);
    pGLQuery->SetOwningContext(nullptr);

    switch (pQuery->GetQueryType())
    {
    case GPU_QUERY_TYPE_SAMPLES_PASSED:
        glEndQuery(GL_SAMPLES_PASSED);
        return true;

    case GPU_QUERY_TYPE_OCCLUSION:
        glEndQuery(GL_ANY_SAMPLES_PASSED);
        return true;

    case GPU_QUERY_TYPE_PRIMITIVES_GENERATED:
        glEndQuery(GL_PRIMITIVES_GENERATED);
        return true;

    case GPU_QUERY_TYPE_TIMESTAMP:
        return true;

    case GPU_QUERY_TYPE_FREQUENCY:
        return true;

    default:
        UnreachableCode();
        return false;
    }
}

GPU_QUERY_GETDATA_RESULT OpenGLGPUContext::GetQueryData(GPUQuery *pQuery, void *pData, uint32 cbData, uint32 flags)
{
    OpenGLGPUQuery *pGLQuery = static_cast<OpenGLGPUQuery *>(pQuery);
    DebugAssert(pGLQuery->GetOwningContext() == nullptr);

    // check result availability
    if (pGLQuery->GetGLQueryID() != 0)
    {
        GLint resultAvailable = GL_FALSE;
        glGetQueryObjectiv(pGLQuery->GetGLQueryID(), GL_QUERY_RESULT_AVAILABLE, &resultAvailable);
        if (!resultAvailable)
            return GPU_QUERY_GETDATA_RESULT_NOT_READY;
    }
 
    switch (pQuery->GetQueryType())
    {
    case GPU_QUERY_TYPE_SAMPLES_PASSED:
        {
            // uint64
            DebugAssert(cbData >= sizeof(uint64));
            glGetQueryObjectui64v(pGLQuery->GetGLQueryID(), GL_SAMPLES_PASSED, reinterpret_cast<GLuint64 *>(pData));
            return GPU_QUERY_GETDATA_RESULT_OK;
        }

    case GPU_QUERY_TYPE_OCCLUSION:
        {
            // int -> bool
            GLint val = 0;
            glGetQueryObjectiv(pGLQuery->GetGLQueryID(), GL_ANY_SAMPLES_PASSED, &val);
            DebugAssert(cbData >= sizeof(bool));
            *reinterpret_cast<bool *>(pData) = (val != 0);
            return GPU_QUERY_GETDATA_RESULT_OK;
        }

    case GPU_QUERY_TYPE_PRIMITIVES_GENERATED:
        {
            // uint64
            DebugAssert(cbData >= sizeof(uint64));
            glGetQueryObjectui64v(pGLQuery->GetGLQueryID(), GL_PRIMITIVES_GENERATED, reinterpret_cast<GLuint64 *>(pData));
            return GPU_QUERY_GETDATA_RESULT_OK;
        }

    case GPU_QUERY_TYPE_TIMESTAMP:
        {
            // uint64
            DebugAssert(cbData >= sizeof(uint64));
            glGetQueryObjectui64v(pGLQuery->GetGLQueryID(), GL_TIMESTAMP, reinterpret_cast<GLuint64 *>(pData));
            return GPU_QUERY_GETDATA_RESULT_OK;
        }

    case GPU_QUERY_TYPE_FREQUENCY:
        {
            // uint64
            DebugAssert(cbData >= sizeof(uint64));
            *reinterpret_cast<uint64 *>(pData) = 1000000000;
            return GPU_QUERY_GETDATA_RESULT_OK;
        }

    default:
        UnreachableCode();
        return GPU_QUERY_GETDATA_RESULT_ERROR;
    }
}

void OpenGLGPUContext::SetPredication(GPUQuery *pQuery)
{

}
