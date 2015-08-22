#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12GPUBuffer.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUContext.h"

GPUBuffer *D3D12GPUDevice::CreateBuffer(const GPU_BUFFER_DESC *pDesc, const void *pInitialData /*= nullptr*/)
{
    return nullptr;
}

bool D3D12GPUContext::ReadBuffer(GPUBuffer *pBuffer, void *pDestination, uint32 start, uint32 count)
{
    return false;
}

bool D3D12GPUContext::WriteBuffer(GPUBuffer *pBuffer, const void *pSource, uint32 start, uint32 count)
{
    return false;
}

bool D3D12GPUContext::MapBuffer(GPUBuffer *pBuffer, GPU_MAP_TYPE mapType, void **ppPointer)
{
    return false;
}

void D3D12GPUContext::Unmapbuffer(GPUBuffer *pBuffer, void *pPointer)
{

}
