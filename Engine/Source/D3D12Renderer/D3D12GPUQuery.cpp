#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12GPUQuery.h"
#include "D3D12Renderer/D3D12GPUContext.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUCommandList.h"

GPUQuery *D3D12GPUDevice::CreateQuery(GPU_QUERY_TYPE type)
{
    return nullptr;
}

bool D3D12GPUContext::BeginQuery(GPUQuery *pQuery)
{
    return false;
}

bool D3D12GPUContext::EndQuery(GPUQuery *pQuery)
{
    return false;
}

GPU_QUERY_GETDATA_RESULT D3D12GPUContext::GetQueryData(GPUQuery *pQuery, void *pData, uint32 cbData, uint32 flags)
{
    return GPU_QUERY_GETDATA_RESULT_ERROR;
}

void D3D12GPUContext::SetPredication(GPUQuery *pQuery)
{

}

void D3D12GPUContext::BypassPredication()
{

}

void D3D12GPUContext::RestorePredication()
{

}

bool D3D12GPUCommandList::BeginQuery(GPUQuery *pQuery)
{
    return false;
}

bool D3D12GPUCommandList::EndQuery(GPUQuery *pQuery)
{
    return false;
}

void D3D12GPUCommandList::SetPredication(GPUQuery *pQuery)
{

}

void D3D12GPUCommandList::BypassPredication()
{

}

void D3D12GPUCommandList::RestorePredication()
{

}
