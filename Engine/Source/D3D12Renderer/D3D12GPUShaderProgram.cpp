#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12GPUShaderProgram.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUContext.h"

GPUShaderProgram *D3D12GPUDevice::CreateGraphicsProgram(const GPU_VERTEX_ELEMENT_DESC *pVertexElements, uint32 nVertexElements, ByteStream *pByteCodeStream)
{
    return nullptr;
}

GPUShaderProgram *D3D12GPUDevice::CreateComputeProgram(ByteStream *pByteCodeStream)
{
    return nullptr;
}
