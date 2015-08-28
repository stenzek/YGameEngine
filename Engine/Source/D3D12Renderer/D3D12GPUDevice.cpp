#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUContext.h"
#include "D3D12Renderer/D3D12RenderBackend.h"
#include "D3D12Renderer/D3D12GPUOutputBuffer.h"
#include "Engine/EngineCVars.h"
Log_SetChannel(D3D12RenderBackend);

D3D12GPUDevice::D3D12GPUDevice(D3D12RenderBackend *pBackend, IDXGIFactory4 *pDXGIFactory, IDXGIAdapter3 *pDXGIAdapter, ID3D12Device *pD3DDevice, DXGI_FORMAT outputBackBufferFormat, DXGI_FORMAT outputDepthStencilFormat)
    : m_pBackend(pBackend)
    , m_pDXGIFactory(pDXGIFactory)
    , m_pDXGIAdapter(pDXGIAdapter)
    , m_pD3DDevice(pD3DDevice)
    , m_pGPUContext(nullptr)
    , m_pOffThreadCommandAllocator(nullptr)
    , m_pOffThreadCommandQueue(nullptr)
    , m_pOffThreadCommandList(nullptr)
    , m_pOffThreadFence(nullptr)
    , m_offThreadFenceReachedEvent(nullptr)
    , m_offThreadFenceValue(0)
    , m_outputBackBufferFormat(outputBackBufferFormat)
    , m_outputDepthStencilFormat(outputDepthStencilFormat)
{
    if (m_pDXGIFactory != nullptr)
        m_pDXGIFactory->AddRef();
    if (m_pDXGIAdapter != nullptr)
        m_pDXGIAdapter->AddRef();
    if (m_pD3DDevice != nullptr)
        m_pD3DDevice->AddRef();
}

D3D12GPUDevice::~D3D12GPUDevice()
{
    if (m_offThreadCopyQueueLength > 0)
        FlushCopyQueue();

    if (m_offThreadFenceReachedEvent != nullptr)
        CloseHandle(m_offThreadFenceReachedEvent);
    SAFE_RELEASE(m_pOffThreadFence);
    SAFE_RELEASE(m_pOffThreadCommandList);
    SAFE_RELEASE(m_pOffThreadCommandQueue);
    SAFE_RELEASE(m_pOffThreadCommandAllocator);

    SAFE_RELEASE(m_pD3DDevice);

    SAFE_RELEASE(m_pDXGIAdapter);
    SAFE_RELEASE(m_pDXGIFactory);
}

bool D3D12GPUDevice::CreateOffThreadResources()
{
    HRESULT hResult;

    // create allocator
    hResult = m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_pOffThreadCommandAllocator));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommandAllocator failed with hResult %08X", hResult);
        return false;
    }

    // create queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };
    hResult = m_pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pOffThreadCommandQueue));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommandAllocator failed with hResult %08X", hResult);
        return false;
    }

    // create command list
    hResult = m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, m_pOffThreadCommandAllocator, nullptr, IID_PPV_ARGS(&m_pOffThreadCommandList));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommandList failed with hResult %08X", hResult);
        return false;
    }

    // create fence
    hResult = m_pD3DDevice->CreateFence(m_offThreadFenceValue, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&m_pOffThreadFence));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateFence failed with hResult %08X", hResult);
        return false;
    }

    // create event
    m_offThreadFenceReachedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_offThreadFenceReachedEvent == INVALID_HANDLE_VALUE)
    {
        Log_ErrorPrintf("CreateEvent failed (%u)", GetLastError());
        return false;
    }

    // done
    return true;
}

void D3D12GPUDevice::BeginResourceBatchUpload()
{
    if (m_pGPUContext != nullptr)
        return;

    m_offThreadCopyQueueEnabled++;
}

void D3D12GPUDevice::EndResourceBatchUpload()
{
    if (m_pGPUContext != nullptr)
        return;

    DebugAssert(m_offThreadCopyQueueEnabled > 0);
    m_offThreadCopyQueueEnabled--;
    if (m_offThreadCopyQueueEnabled == 0 && m_offThreadCopyQueueLength > 0)
        FlushCopyQueue();
}

void D3D12GPUDevice::FlushCopyQueue()
{
    if (m_pGPUContext != nullptr)
        return;

    if (m_offThreadCopyQueueEnabled > 0)
    {
        m_offThreadCopyQueueLength++;
        return;
    }

    m_pOffThreadCommandList->Close();
    m_pOffThreadCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList **)&m_pOffThreadCommandList);

    // @TODO other way around??? 
    HRESULT hResult = m_pOffThreadCommandQueue->Signal(m_pOffThreadFence, 1);
    if (SUCCEEDED(hResult))
    {
        // wait until the fence is reached
        hResult = m_pOffThreadFence->SetEventOnCompletion(++m_offThreadFenceValue, m_offThreadFenceReachedEvent);
        if (SUCCEEDED(hResult))
            WaitForSingleObject(m_offThreadFenceReachedEvent, INFINITE);
        else
            Log_ErrorPrintf("ID3D12Fence::SetEventOnCompletion failed with hResult %08X", hResult);
    }
    else
    {
        Log_ErrorPrintf("ID3D12Fence::Signal failed with hResult %08X", hResult);
    }

    // reset the command allocator/list
    m_pOffThreadCommandAllocator->Reset();
    m_pOffThreadCommandList->Reset(m_pOffThreadCommandAllocator, nullptr);

    // reset vars
    m_offThreadCopyQueueLength = 0;

    // release resources
    for (ID3D12Pageable *pResource : m_offThreadUploadResources)
        pResource->Release();
    m_offThreadUploadResources.Clear();
}

void D3D12GPUDevice::ResourceBarrier(ID3D12Resource *pResource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    DebugAssert(m_offThreadCopyQueueEnabled || m_pGPUContext != nullptr);

    D3D12_RESOURCE_BARRIER resourceBarrier =
    {
        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,
        { pResource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, beforeState, afterState }
    };

    GetCommandList()->ResourceBarrier(1, &resourceBarrier);
}

void D3D12GPUDevice::ResourceBarrier(ID3D12Resource *pResource, uint32 subResource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    DebugAssert(m_offThreadCopyQueueEnabled ||  m_pGPUContext != nullptr);

    D3D12_RESOURCE_BARRIER resourceBarrier =
    {
        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,
        { pResource, subResource, beforeState, afterState }
    };

    GetCommandList()->ResourceBarrier(1, &resourceBarrier);
}

ID3D12GraphicsCommandList *D3D12GPUDevice::GetCommandList()
{
    return (m_pGPUContext != nullptr) ? m_pGPUContext->GetCurrentCommandList() : m_pOffThreadCommandList;
}

void D3D12GPUDevice::ScheduleUploadResourceDeletion(ID3D12Pageable *pResource)
{
    if (m_pGPUContext != nullptr)
    {
        m_pBackend->ScheduleResourceForDeletion(pResource);
        return;
    }

    if (m_offThreadCopyQueueEnabled)
    {
        m_offThreadUploadResources.Add(pResource);
        return;
    }

    // should have already completed the command by here
    pResource->Release();
}

static const D3D12_COMPARISON_FUNC s_D3D12ComparisonFuncs[GPU_COMPARISON_FUNC_COUNT] =
{
    D3D12_COMPARISON_FUNC_NEVER,             // RENDERER_COMPARISON_FUNC_NEVER
    D3D12_COMPARISON_FUNC_LESS,              // RENDERER_COMPARISON_FUNC_LESS
    D3D12_COMPARISON_FUNC_EQUAL,             // RENDERER_COMPARISON_FUNC_EQUAL
    D3D12_COMPARISON_FUNC_LESS_EQUAL,        // RENDERER_COMPARISON_FUNC_LESS_EQUAL
    D3D12_COMPARISON_FUNC_GREATER,           // RENDERER_COMPARISON_FUNC_GREATER
    D3D12_COMPARISON_FUNC_NOT_EQUAL,         // RENDERER_COMPARISON_FUNC_NOT_EQUAL
    D3D12_COMPARISON_FUNC_GREATER_EQUAL,     // RENDERER_COMPARISON_FUNC_GREATER_EQUAL
    D3D12_COMPARISON_FUNC_ALWAYS,            // RENDERER_COMPARISON_FUNC_ALWAYS
};

D3D12GPUSamplerState::D3D12GPUSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc, const D3D12DescriptorHandle &samplerHandle)
    : GPUSamplerState(pSamplerStateDesc)
    , m_samplerHandle(samplerHandle)
{
    
}

D3D12GPUSamplerState::~D3D12GPUSamplerState()
{
    D3D12RenderBackend::GetInstance()->ScheduleDescriptorForDeletion(m_samplerHandle);
}

void D3D12GPUSamplerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12GPUSamplerState::SetDebugName(const char *name)
{

}

GPUSamplerState *D3D12GPUDevice::CreateSamplerState(const GPU_SAMPLER_STATE_DESC *pSamplerStateDesc)
{
    D3D12_SAMPLER_DESC D3DSamplerDesc;
    if (!D3D12Helpers::FillD3D12SamplerStateDesc(pSamplerStateDesc, &D3DSamplerDesc))
    {
        Log_ErrorPrintf("Invalid sampler state description");
        return nullptr;
    }

    // allocate a descriptor
    D3D12DescriptorHandle descriptorHandle;
    if (!m_pBackend->GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)->Allocate(&descriptorHandle))
    {
        Log_ErrorPrintf("Failed to allocate descriptor handle");
        return false;
    }

    // fill descriptor
    m_pD3DDevice->CreateSampler(&D3DSamplerDesc, descriptorHandle);
    return new D3D12GPUSamplerState(pSamplerStateDesc, descriptorHandle);
}

D3D12GPURasterizerState::D3D12GPURasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc, const D3D12_RASTERIZER_DESC *pD3DRasterizerDesc)
    : GPURasterizerState(pRasterizerStateDesc)
{
    Y_memcpy(&m_D3DRasterizerDesc, pD3DRasterizerDesc, sizeof(m_D3DRasterizerDesc));
}

D3D12GPURasterizerState::~D3D12GPURasterizerState()
{

}

void D3D12GPURasterizerState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12GPURasterizerState::SetDebugName(const char *name)
{

}

GPURasterizerState *D3D12GPUDevice::CreateRasterizerState(const RENDERER_RASTERIZER_STATE_DESC *pRasterizerStateDesc)
{
    D3D12_RASTERIZER_DESC D3DRasterizerDesc;
    if (!D3D12Helpers::FillD3D12RasterizerStateDesc(pRasterizerStateDesc, &D3DRasterizerDesc))
    {
        Log_ErrorPrintf("Invalid rasterizer state description");
        return nullptr;
    }

    return new D3D12GPURasterizerState(pRasterizerStateDesc, &D3DRasterizerDesc);
}

D3D12GPUDepthStencilState::D3D12GPUDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc, const D3D12_DEPTH_STENCIL_DESC *pD3DDepthStencilDesc)
    : GPUDepthStencilState(pDepthStencilStateDesc)
{
    Y_memcpy(&m_D3DDepthStencilDesc, pD3DDepthStencilDesc, sizeof(m_D3DDepthStencilDesc));
}

D3D12GPUDepthStencilState::~D3D12GPUDepthStencilState()
{

}

void D3D12GPUDepthStencilState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12GPUDepthStencilState::SetDebugName(const char *name)
{

}

GPUDepthStencilState *D3D12GPUDevice::CreateDepthStencilState(const RENDERER_DEPTHSTENCIL_STATE_DESC *pDepthStencilStateDesc)
{
    D3D12_DEPTH_STENCIL_DESC D3DDepthStencilDesc;
    if (!D3D12Helpers::FillD3D12DepthStencilStateDesc(pDepthStencilStateDesc, &D3DDepthStencilDesc))
    {
        Log_ErrorPrintf("Invalid depth-stencil state description");
        return nullptr;
    }

    return new D3D12GPUDepthStencilState(pDepthStencilStateDesc, &D3DDepthStencilDesc);
}

D3D12GPUBlendState::D3D12GPUBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc, const D3D12_BLEND_DESC *pD3DBlendDesc)
    : GPUBlendState(pBlendStateDesc)
{
    Y_memcpy(&m_D3DBlendDesc, pD3DBlendDesc, sizeof(m_D3DBlendDesc));
}

D3D12GPUBlendState::~D3D12GPUBlendState()
{
    
}

void D3D12GPUBlendState::GetMemoryUsage(uint32 *cpuMemoryUsage, uint32 *gpuMemoryUsage) const
{
    if (cpuMemoryUsage != nullptr)
        *cpuMemoryUsage = sizeof(*this);
}

void D3D12GPUBlendState::SetDebugName(const char *name)
{

}

GPUBlendState *D3D12GPUDevice::CreateBlendState(const RENDERER_BLEND_STATE_DESC *pBlendStateDesc)
{
    D3D12_BLEND_DESC D3DBlendDesc;
    if (!D3D12Helpers::FillD3D12BlendStateDesc(pBlendStateDesc, &D3DBlendDesc))
    {
        Log_ErrorPrintf("Invalid blend state description");
        return nullptr;
    }

    return new D3D12GPUBlendState(pBlendStateDesc, &D3DBlendDesc);
}
