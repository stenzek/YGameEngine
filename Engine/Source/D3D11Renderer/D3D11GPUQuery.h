#pragma once
#include "D3D11Renderer/D3D11Common.h"

class D3D11GPUQuery : public GPUQuery
{
public:
    D3D11GPUQuery(GPU_QUERY_TYPE type, ID3D11Query *pQuery, ID3D11Predicate *pPredicate);
    ~D3D11GPUQuery();

    virtual GPU_QUERY_TYPE GetQueryType() const override { return m_eType; }
    virtual void GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const override;
    virtual void SetDebugName(const char *name) override;

    ID3D11Query *GetD3DQuery() const { return m_pD3DQuery; }
    ID3D11Predicate *GetD3DPredicate() const { return m_pD3DPredicate; }

    D3D11GPUContext *GetOwningContext() const { return m_pOwningContext; }
    void SetOwningContext(D3D11GPUContext *pContext) { m_pOwningContext = pContext; }

protected:
    GPU_QUERY_TYPE m_eType;
    ID3D11Query *m_pD3DQuery;
    ID3D11Predicate *m_pD3DPredicate;
    D3D11GPUContext *m_pOwningContext;
};
