#include "D3D11Renderer/PrecompiledHeader.h"
#include "D3D11Renderer/D3D11GPUQuery.h"
#include "D3D11Renderer/D3D11GPUContext.h"
#include "D3D11Renderer/D3D11Renderer.h"
Log_SetChannel(D3D11GPUContext);

D3D11GPUQuery::D3D11GPUQuery(GPU_QUERY_TYPE type, ID3D11Query *pQuery, ID3D11Predicate *pPredicate)
    : m_eType(type)
    , m_pD3DQuery(pQuery)
    , m_pD3DPredicate(pPredicate)
    , m_pOwningContext(nullptr)
{

}

D3D11GPUQuery::~D3D11GPUQuery()
{
    SAFE_RELEASE(m_pD3DQuery);
    DebugAssert(m_pOwningContext == nullptr);
}

void D3D11GPUQuery::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this) + sizeof(ID3D11Query);

    // estimation...
    if (gpuMemoryUsage != nullptr)
        *gpuMemoryUsage = 128;
}

void D3D11GPUQuery::SetDebugName(const char *name)
{
    D3D11Helpers::SetD3D11DeviceChildDebugName(m_pD3DQuery, name);
}

bool D3D11GPUContext::BeginQuery(GPUQuery *pQuery)
{
    D3D11GPUQuery *pD3DQuery = static_cast<D3D11GPUQuery *>(pQuery);
    DebugAssert(pD3DQuery->GetOwningContext() == nullptr);

    pD3DQuery->SetOwningContext(this);
    m_pD3DContext->Begin(pD3DQuery->GetD3DQuery());
    return true;
}

bool D3D11GPUContext::EndQuery(GPUQuery *pQuery)
{
    D3D11GPUQuery *pD3DQuery = static_cast<D3D11GPUQuery *>(pQuery);
    DebugAssert(pD3DQuery->GetOwningContext() == this);

    m_pD3DContext->End(pD3DQuery->GetD3DQuery());
    pD3DQuery->SetOwningContext(nullptr);
    return true;
}

GPU_QUERY_GETDATA_RESULT D3D11GPUContext::GetQueryData(GPUQuery *pQuery, void *pData, uint32 cbData, uint32 flags)
{
    D3D11GPUQuery *pD3DQuery = static_cast<D3D11GPUQuery *>(pQuery);
    DebugAssert(pD3DQuery->GetOwningContext() == nullptr);

    uint32 D3DGetDataFlags = 0;
    if (flags & GPU_QUERY_GETDATA_FLAG_NOFLUSH)
        D3DGetDataFlags = D3D11_ASYNC_GETDATA_DONOTFLUSH;

    HRESULT hResult;
    switch (pD3DQuery->GetQueryType())
    {
    case GPU_QUERY_TYPE_SAMPLES_PASSED:
        {
            // uint64
            DebugAssert(cbData == sizeof(uint64));
            hResult = m_pD3DContext->GetData(pD3DQuery->GetD3DQuery(), pData, sizeof(uint64), D3DGetDataFlags);
            if (hResult == S_FALSE)
                return GPU_QUERY_GETDATA_RESULT_NOT_READY;
            else if (FAILED(hResult))
                return GPU_QUERY_GETDATA_RESULT_ERROR;
            else
                return GPU_QUERY_GETDATA_RESULT_OK;
        }

    case GPU_QUERY_TYPE_OCCLUSION:
        {
            // BOOL -> bool
            BOOL tempQueryData;
            hResult = m_pD3DContext->GetData(pD3DQuery->GetD3DQuery(), &tempQueryData, sizeof(tempQueryData), D3DGetDataFlags);
            if (hResult == S_FALSE)
                return GPU_QUERY_GETDATA_RESULT_NOT_READY;
            else if (FAILED(hResult))
                return GPU_QUERY_GETDATA_RESULT_ERROR;

            DebugAssert(cbData == sizeof(bool));
            *reinterpret_cast<bool *>(pData) = (tempQueryData == TRUE);
            return GPU_QUERY_GETDATA_RESULT_OK;
        }

    case GPU_QUERY_TYPE_PRIMITIVES_GENERATED:
        {
            // struct -> uint64
            D3D11_QUERY_DATA_PIPELINE_STATISTICS pipelineStatistics;
            hResult = m_pD3DContext->GetData(pD3DQuery->GetD3DQuery(), &pipelineStatistics, sizeof(pipelineStatistics), D3DGetDataFlags);
            if (hResult == S_FALSE)
                return GPU_QUERY_GETDATA_RESULT_NOT_READY;
            else if (FAILED(hResult))
                return GPU_QUERY_GETDATA_RESULT_ERROR;

            // pull from field
            DebugAssert(cbData == sizeof(uint64));
            *reinterpret_cast<uint64 *>(pData) = pipelineStatistics.CInvocations;
            return GPU_QUERY_GETDATA_RESULT_OK;
        }

    case GPU_QUERY_TYPE_TIMESTAMP:
        {
            // uint64
            DebugAssert(cbData == sizeof(uint64));
            hResult = m_pD3DContext->GetData(pD3DQuery->GetD3DQuery(), pData, sizeof(uint64), D3DGetDataFlags);
            if (hResult == S_FALSE)
                return GPU_QUERY_GETDATA_RESULT_NOT_READY;
            else if (FAILED(hResult))
                return GPU_QUERY_GETDATA_RESULT_ERROR;
            else
                return GPU_QUERY_GETDATA_RESULT_OK;
        }

    case GPU_QUERY_TYPE_FREQUENCY:
        {
            // struct -> uint64
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;
            hResult = m_pD3DContext->GetData(pD3DQuery->GetD3DQuery(), &tsDisjoint, sizeof(tsDisjoint), D3DGetDataFlags);
            if (hResult == S_FALSE)
                return GPU_QUERY_GETDATA_RESULT_NOT_READY;
            else if (FAILED(hResult))
                return GPU_QUERY_GETDATA_RESULT_ERROR;

            // if disjoint, return zero
            DebugAssert(cbData == sizeof(uint64));
            *reinterpret_cast<uint64 *>(pData) = (tsDisjoint.Disjoint) ? 0 : tsDisjoint.Frequency;
            return GPU_QUERY_GETDATA_RESULT_OK;
        }

    default:
        UnreachableCode();
        return GPU_QUERY_GETDATA_RESULT_ERROR;
    }
}


GPUQuery *D3D11Renderer::CreateQuery(GPU_QUERY_TYPE type)
{
    ID3D11Query *pD3DQuery = nullptr;
    ID3D11Predicate *pD3DPredicate = nullptr;

    // create occlusion as predicates
    if (type == GPU_QUERY_TYPE_OCCLUSION)
    {
        D3D11_QUERY_DESC D3DQueryDesc;
        D3DQueryDesc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
        D3DQueryDesc.MiscFlags = 0;

        HRESULT hResult = m_pD3DDevice->CreatePredicate(&D3DQueryDesc, &pD3DPredicate);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11Renderer::CreateQuery: CreatePredicate failed with hResult %08X", hResult);
            return false;
        }

        // also the query field
        pD3DQuery = pD3DPredicate;
    }
    else
    {
        D3D11_QUERY_DESC D3DQueryDesc;

        switch (type)
        {
        case GPU_QUERY_TYPE_SAMPLES_PASSED:
            D3DQueryDesc.Query = D3D11_QUERY_OCCLUSION;
            D3DQueryDesc.MiscFlags = 0;
            break;

        case GPU_QUERY_TYPE_PRIMITIVES_GENERATED:
            D3DQueryDesc.Query = D3D11_QUERY_PIPELINE_STATISTICS;
            D3DQueryDesc.MiscFlags = 0;
            break;

        case GPU_QUERY_TYPE_TIMESTAMP:
            D3DQueryDesc.Query = D3D11_QUERY_TIMESTAMP;
            D3DQueryDesc.MiscFlags = 0;
            break;

        case GPU_QUERY_TYPE_FREQUENCY:
            D3DQueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
            D3DQueryDesc.MiscFlags = 0;
            break;

        default:
            UnreachableCode();
            break;
        }

        HRESULT hResult = m_pD3DDevice->CreateQuery(&D3DQueryDesc, &pD3DQuery);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("D3D11Renderer::CreateQuery: CreateQuery failed with hResult %08X", hResult);
            return false;
        }
    }

    return new D3D11GPUQuery(type, pD3DQuery, pD3DPredicate);
}

void D3D11GPUContext::SetPredication(GPUQuery *pQuery)
{
    if (m_pCurrentPredicate == pQuery)
        return;

    if (pQuery != nullptr)
        m_pCurrentPredicateD3D = static_cast<D3D11GPUQuery *>(pQuery)->GetD3DPredicate();
    else
        m_pCurrentPredicateD3D = nullptr;
        
    // switch in d3d
    if (m_predicateBypassCount == 0)
        m_pD3DContext->SetPredication(m_pCurrentPredicateD3D, FALSE);

    // update pointer
    if (m_pCurrentPredicate != nullptr)
        m_pCurrentPredicate->Release();
    if ((m_pCurrentPredicate = pQuery) != nullptr)
        m_pCurrentPredicate->AddRef();
}

void D3D11GPUContext::BypassPredication()
{
    if ((m_predicateBypassCount++) == 0 && m_pCurrentPredicateD3D != nullptr)
        m_pD3DContext->SetPredication(nullptr, FALSE);
}

void D3D11GPUContext::RestorePredication()
{
    DebugAssert(m_predicateBypassCount > 0);
    if ((--m_predicateBypassCount) == 0 && m_pCurrentPredicateD3D != nullptr)
        m_pD3DContext->SetPredication(m_pCurrentPredicateD3D, FALSE);
}
