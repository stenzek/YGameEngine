#include "D3D12Renderer/PrecompiledHeader.h"
#include "D3D12Renderer/D3D12Common.h"
#include "D3D12Renderer/D3D12GPUDevice.h"
#include "D3D12Renderer/D3D12GPUContext.h"
#include "D3D12Renderer/D3D12GPUBuffer.h"
#include "D3D12Renderer/D3D12GPUTexture.h"
#include "D3D12Renderer/D3D12GPUShaderProgram.h"
#include "D3D12Renderer/D3D12RenderBackend.h"
#include "D3D12Renderer/D3D12GPUOutputBuffer.h"
#include "Renderer/ShaderConstantBuffer.h"
Log_SetChannel(D3D12GPUContext);

D3D12GPUContext::D3D12GPUContext(D3D12RenderBackend *pBackend, D3D12GPUDevice *pDevice, ID3D12Device *pD3DDevice)
    : m_pBackend(pBackend)
    , m_pDevice(pDevice)
    , m_pD3DDevice(pD3DDevice)
    , m_pConstants(nullptr)
    , m_pCommandQueue(nullptr)
    , m_pCurrentCommandList(nullptr)
    , m_pCurrentScratchBuffer(nullptr)
    , m_currentCommandQueueIndex(0)
    , m_nextFenceValue(0)
{
    // add references
    m_pDevice->SetGPUContext(this);

    // null memory
    Y_memzero(&m_currentViewport, sizeof(m_currentViewport));
    Y_memzero(&m_scissorRect, sizeof(m_scissorRect));
    m_currentTopology = DRAW_TOPOLOGY_UNDEFINED;
    m_currentD3DTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;

    // null current states
    Y_memzero(m_pCurrentVertexBuffers, sizeof(m_pCurrentVertexBuffers));
    Y_memzero(m_currentVertexBufferOffsets, sizeof(m_currentVertexBufferOffsets));
    Y_memzero(m_currentVertexBufferStrides, sizeof(m_currentVertexBufferStrides));
    m_currentVertexBufferBindCount = 0;

    m_pCurrentIndexBuffer = nullptr;
    m_currentIndexFormat = GPU_INDEX_FORMAT_COUNT;
    m_currentIndexBufferOffset = 0;

    m_pCurrentShaderProgram = nullptr;
    m_shaderStates.Resize(SHADER_PROGRAM_STAGE_COUNT);
    m_shaderStates.ZeroContents();

    m_pCurrentRasterizerState = nullptr;
    m_pCurrentDepthStencilState = nullptr;
    m_currentDepthStencilRef = 0;
    m_pCurrentBlendState = nullptr;
    m_currentBlendStateBlendFactors.SetZero();
    m_pipelineChanged = false;

    m_pCurrentSwapChain = nullptr;

    Y_memzero(m_pCurrentRenderTargetViews, sizeof(m_pCurrentRenderTargetViews));
    m_pCurrentDepthBufferView = nullptr;
    m_nCurrentRenderTargets = 0;

    m_pCurrentPredicate = nullptr;
    //m_pCurrentPredicateD3D = nullptr;
    m_predicateBypassCount = 0;
}

D3D12GPUContext::~D3D12GPUContext()
{
    // move to next frame, executing all commands
    if (m_pCurrentCommandList != nullptr)
    {
        ExecuteCurrentCommandList(false);
        MoveToNextCommandQueue();
        FinishPendingCommands();

        // close the current command queue (it'll be empty)
        m_pCurrentCommandList->Close();
    }

    // nuke command queues
    for (uint32 i = 0; i < m_commandQueues.GetSize(); i++)
    {
        DCommandQueue *pQueue = &m_commandQueues[i];
        if (pQueue->FenceReachedEvent != NULL)
            CloseHandle(pQueue->FenceReachedEvent);
        delete pQueue->pScratchSamplerHeap;
        delete pQueue->pScratchViewHeap;
        delete pQueue->pScratchBuffer;
        SAFE_RELEASE(pQueue->pFence);
        SAFE_RELEASE(pQueue->pCommandList);
        SAFE_RELEASE(pQueue->pCommandAllocator);
    }

    // nuke d3d objects
    m_pCurrentCommandList = nullptr;
    m_pCurrentScratchBuffer = nullptr;
    m_pCurrentScratchViewHeap = nullptr;
    m_pCurrentScratchSamplerHeap = nullptr;
    m_currentDescriptorHeaps.Obliterate();
    SAFE_RELEASE(m_pCommandQueue);

#if 0
    // clear any state
    ClearState(true, true, true, true);
    m_pD3DContext->OMSetRenderTargets(0, nullptr, nullptr);
    m_pD3DContext->ClearState();
    m_pD3DContext->Flush();

    DebugAssert(m_predicateBypassCount == 0);
    if (m_pCurrentPredicateD3D != nullptr)
    {
        m_pD3DContext->SetPredication(nullptr, FALSE);
        m_pCurrentPredicateD3D = nullptr;
    }
    SAFE_RELEASE(m_pCurrentPredicate);
#endif

    // release our references to states and targets. d3d references will die when the device goes down.
    for (uint32 i = 0; i < countof(m_pCurrentVertexBuffers); i++) {
        SAFE_RELEASE(m_pCurrentVertexBuffers[i]);
    }
    SAFE_RELEASE(m_pCurrentIndexBuffer);

    SAFE_RELEASE(m_pCurrentShaderProgram);
    m_shaderStates.Obliterate();

    for (uint32 i = 0; i < m_constantBuffers.GetSize(); i++)
    {
        ConstantBuffer *constantBuffer = &m_constantBuffers[i];
        delete[] constantBuffer->pLocalMemory;
    }

    SAFE_RELEASE(m_pCurrentRasterizerState);
    SAFE_RELEASE(m_pCurrentDepthStencilState);
    SAFE_RELEASE(m_pCurrentBlendState);

    for (uint32 i = 0; i < countof(m_pCurrentRenderTargetViews); i++) {
        SAFE_RELEASE(m_pCurrentRenderTargetViews[i]);
    }
    SAFE_RELEASE(m_pCurrentDepthBufferView);

    delete m_pConstants;

    SAFE_RELEASE(m_pCurrentSwapChain);
    m_pDevice->SetGPUContext(nullptr);
}

void D3D12GPUContext::ResourceBarrier(ID3D12Resource *pResource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    D3D12_RESOURCE_BARRIER resourceBarrier =
    {
        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,
        { pResource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, beforeState, afterState }
    };

    m_pCurrentCommandList->ResourceBarrier(1, &resourceBarrier);
}

void D3D12GPUContext::ResourceBarrier(ID3D12Resource *pResource, uint32 subResource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    D3D12_RESOURCE_BARRIER resourceBarrier =
    {
        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,
        { pResource, subResource, beforeState, afterState }
    };

    m_pCurrentCommandList->ResourceBarrier(1, &resourceBarrier);
}

bool D3D12GPUContext::Create()
{   
    // allocate constants
    m_pConstants = new GPUContextConstants(this);
    CreateConstantBuffers();

    // create queued frame data
    if (!CreateInternalCommandLists())
        return false;

    return true;
}

void D3D12GPUContext::CreateConstantBuffers()
{
    // allocate constant buffer storage
    const ShaderConstantBuffer::RegistryType *registry = ShaderConstantBuffer::GetRegistry();
    m_constantBuffers.Resize(registry->GetNumTypes());
    m_constantBuffers.ZeroContents();
    for (uint32 i = 0; i < m_constantBuffers.GetSize(); i++)
    {
        ConstantBuffer *constantBuffer = &m_constantBuffers[i];

        // applicable to us?
        const ShaderConstantBuffer *declaration = registry->GetTypeInfoByIndex(i);
        if (declaration == nullptr)
            continue;
        if (declaration->GetPlatformRequirement() != RENDERER_PLATFORM_COUNT && declaration->GetPlatformRequirement() != RENDERER_PLATFORM_D3D12)
            continue;
        if (declaration->GetMinimumFeatureLevel() != RENDERER_FEATURE_LEVEL_COUNT && declaration->GetMinimumFeatureLevel() > D3D12RenderBackend::GetInstance()->GetFeatureLevel())
            continue;

        // set size so we know to allocate it later or on demand
        constantBuffer->Size = declaration->GetBufferSize();
        constantBuffer->DirtyLowerBounds = constantBuffer->DirtyUpperBounds = -1;
        constantBuffer->pLocalMemory = new byte[constantBuffer->Size];
        Y_memzero(constantBuffer->pLocalMemory, constantBuffer->Size);
    }
}

bool D3D12GPUContext::CreateInternalCommandLists()
{
    HRESULT hResult;
    uint32 frameLatency = m_pBackend->GetFrameLatency();
    m_commandQueues.Resize(frameLatency);
    m_commandQueues.ZeroContents();

    // allocate command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };
    hResult = m_pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue));
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommandQueue failed with hResult %08X", hResult);
        return false;
    }

    // allocate command lists
    for (uint32 i = 0; i < frameLatency; i++)
    {
        DCommandQueue *pFrameData = &m_commandQueues[i];
        pFrameData->FenceValue = 0;
        pFrameData->Pending = false;

        // allocate command allocator
        hResult = m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&pFrameData->pCommandAllocator));
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("ID3D12Device::CreateCommandAllocator failed with hResult %08X", hResult);
            return false;
        }

        // allocate command list
        hResult = m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pFrameData->pCommandAllocator, nullptr, IID_PPV_ARGS(&pFrameData->pCommandList));
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("ID3D12Device::CreateCommandList failed with hResult %08X", hResult);
            return false;
        }

        // command list is expected to be in the closed state
        hResult = pFrameData->pCommandList->Close();
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("ID3D12CommandList::Close failed with hResult %08X", hResult);
            return false;
        }

        // allocate fence
        hResult = m_pD3DDevice->CreateFence(pFrameData->FenceValue, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(&pFrameData->pFence));
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("ID3D12Device::CreateFence failed with hResult %08X", hResult);
            return false;
        }

        // allocate scratch buffer
        pFrameData->pScratchBuffer = D3D12ScratchBuffer::Create(m_pD3DDevice, 16 * 1024 * 1024);
        if (pFrameData->pScratchBuffer == nullptr)
        {
            Log_ErrorPrintf("Failed to allocate scratch buffer.");
            return false;
        }

        // allocate view descriptor heap
        pFrameData->pScratchViewHeap = D3D12ScratchDescriptorHeap::Create(m_pD3DDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 16384);
        pFrameData->pScratchSamplerHeap = D3D12ScratchDescriptorHeap::Create(m_pD3DDevice, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 2048);
        if (pFrameData->pScratchViewHeap == nullptr || pFrameData->pScratchSamplerHeap == nullptr)
        {
            Log_ErrorPrintf("Failed to allocate scratch descriptor heaps.");
            return false;
        }

        // allocate reached event
        pFrameData->FenceReachedEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (pFrameData->FenceReachedEvent == INVALID_HANDLE_VALUE)
        {
            Log_ErrorPrintf("CreateEvent failed (%u).", GetLastError());
            return false;
        }
    }

    // first fence value is 1
    m_nextFenceValue = 1;

    // activate the first command queue
    ActivateCommandQueue(0);
    return true;
}

void D3D12GPUContext::BeginFrame()
{
    g_pRenderer->BeginFrame();
}

void D3D12GPUContext::Flush()
{
    ExecuteCurrentCommandList(true);
}

void D3D12GPUContext::Finish()
{
    ExecuteCurrentCommandList(false);
    MoveToNextCommandQueue();
    RestoreCommandListDependantState();
    FinishPendingCommands();
}

void D3D12GPUContext::ExecuteCurrentCommandList(bool reopen)
{
    // execute any queued commands on the immediate command list
    HRESULT hResult = m_pCurrentCommandList->Close();
    if (SUCCEEDED(hResult))
    {
        m_pCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList **)&m_pCurrentCommandList);
    }
    else
    {
        Log_ErrorPrintf("ID3D12CommandList::Close failed with hResult %08X, some may commands may not be executed. Recreating command list.", hResult);

        // create a new command list (documentation says that a command list that failed Close() will forever be in an error state)
        DCommandQueue *pFrameData = &m_commandQueues[m_currentCommandQueueIndex];
        ID3D12GraphicsCommandList *pNewCommandList;
        hResult = m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pFrameData->pCommandAllocator, nullptr, IID_PPV_ARGS(&pNewCommandList));
        if (SUCCEEDED(hResult))
        {
            hResult = pNewCommandList->Close();
            if (FAILED(hResult))
                Log_ErrorPrintf("ID3D12CommandList::Close on the new command list failed with hResult %08X. Something is seriously wrong.", hResult);

            // if close failed, the gpu won't be doing anything with it. right?
            pFrameData->pCommandList->Release();
            pFrameData->pCommandList = pNewCommandList;
            m_pCurrentCommandList = pNewCommandList;
        }
        else
        {
            // can't do much, leave the broken command list around, try next time..
            Log_ErrorPrintf("ID3D12Device::CreateCommandList failed with hResult %08X. Something is seriously wrong.", hResult);
        }
    }

    if (reopen)
    {
        hResult = m_pCurrentCommandList->Reset(m_commandQueues[m_currentCommandQueueIndex].pCommandAllocator, nullptr);
        if (FAILED(hResult))
            Log_WarningPrintf("ID3D12CommandList:Reset failed with hResult %08X", hResult);

        RestoreCommandListDependantState();
    }
}

void D3D12GPUContext::ActivateCommandQueue(uint32 index)
{
    HRESULT hResult;
    DCommandQueue *pFrameData = &m_commandQueues[index];
    m_currentCommandQueueIndex = index;

    // begin recording commands
    hResult = pFrameData->pCommandAllocator->Reset();
    if (FAILED(hResult))
        Log_ErrorPrintf("ID3D12CommandAllocator::Reset failed with hResult %08X", hResult);
    hResult = pFrameData->pCommandList->Reset(pFrameData->pCommandAllocator, nullptr);
    if (FAILED(hResult))
        Log_ErrorPrintf("ID3D12CommandList::Reset failed with hResult %08X", hResult);

    // update pointers
    pFrameData->FenceValue = m_nextFenceValue++;
    m_pCurrentCommandList = pFrameData->pCommandList;
    m_pCurrentScratchBuffer = pFrameData->pScratchBuffer;
    m_pCurrentScratchSamplerHeap = pFrameData->pScratchSamplerHeap;
    m_pCurrentScratchViewHeap = pFrameData->pScratchViewHeap;

    // reset scratch buffer
    pFrameData->pScratchBuffer->Reset();
    pFrameData->pScratchViewHeap->Reset();
    pFrameData->pScratchSamplerHeap->Reset();

    // add descriptor heaps
    m_currentDescriptorHeaps.Clear();
    m_currentDescriptorHeaps.Add(m_pCurrentScratchViewHeap->GetD3DHeap());
    m_currentDescriptorHeaps.Add(m_pCurrentScratchSamplerHeap->GetD3DHeap());
    m_pCurrentCommandList->SetDescriptorHeaps(m_currentDescriptorHeaps.GetSize(), m_currentDescriptorHeaps.GetBasePointer());
    m_pCurrentCommandList->SetGraphicsRootSignature(m_pBackend->GetLegacyGraphicsRootSignature());
    m_pCurrentCommandList->SetComputeRootSignature(m_pBackend->GetLegacyComputeRootSignature());

    // update destruction fence value
    m_pBackend->SetCleanupFenceValue(pFrameData->FenceValue);
    Log_DevPrintf("Command queue %u activated.", m_currentCommandQueueIndex);
}

void D3D12GPUContext::MoveToNextCommandQueue()
{
    // set up fence for the next time round (for the current command list)
    DCommandQueue *pFrameData = &m_commandQueues[m_currentCommandQueueIndex];
    ResetEvent(pFrameData->FenceReachedEvent);
    pFrameData->pFence->SetEventOnCompletion(pFrameData->FenceValue, pFrameData->FenceReachedEvent);
    m_pCommandQueue->Signal(pFrameData->pFence, pFrameData->FenceValue);
    pFrameData->Pending = true;

    // next frame
    uint32 nextCommandQueueIndex = (m_currentCommandQueueIndex + 1) % D3D12RenderBackend::GetInstance()->GetFrameLatency();

    // wait for these operations to finish
    if (m_commandQueues[nextCommandQueueIndex].Pending)
        WaitForCommandQueue(nextCommandQueueIndex);

    // activate it
    ActivateCommandQueue(nextCommandQueueIndex);
}

void D3D12GPUContext::FinishPendingCommands()
{
    // ensure all other frames have been reached
    uint32 frameLatency = m_pBackend->GetFrameLatency();
    for (uint32 queuedFrameIndex = (m_currentCommandQueueIndex + 1) % frameLatency; queuedFrameIndex != m_currentCommandQueueIndex; queuedFrameIndex = (queuedFrameIndex + 1) % frameLatency)
    {
        if (m_commandQueues[queuedFrameIndex].Pending)
            WaitForCommandQueue(queuedFrameIndex);
    }
}

void D3D12GPUContext::WaitForCommandQueue(uint32 index)
{
    DCommandQueue *pFrameData = &m_commandQueues[index];
    DebugAssert(pFrameData->Pending);

    WaitForSingleObject(pFrameData->FenceReachedEvent, INFINITE);
    ResetEvent(pFrameData->FenceReachedEvent);

    // release resources
    m_pBackend->DeletePendingResources(pFrameData->FenceValue);

    // no longer active
    pFrameData->Pending = false;
}

void D3D12GPUContext::ClearCommandListDependantState()
{
    // pipeline state
    SAFE_RELEASE(m_pCurrentShaderProgram);
    m_pipelineChanged = false;

    // render targets -- still needs to be re-synced
    for (uint32 i = 0; i < m_nCurrentRenderTargets; i++)
        SAFE_RELEASE(m_pCurrentRenderTargetViews[i]);
    SAFE_RELEASE(m_pCurrentDepthBufferView);
    m_nCurrentRenderTargets = 0;
    SynchronizeRenderTargetsAndUAVs();

    // predicate
    //SAFE_RELEASE(m_pCurrentPredicate);

    // other stuff
    m_currentBlendStateBlendFactors.SetZero();
    m_currentDepthStencilRef = 0;
}

void D3D12GPUContext::RestoreCommandListDependantState()
{
    // pipeline state
    m_pipelineChanged = true;
    UpdatePipelineState();

    // render targets
    SynchronizeRenderTargetsAndUAVs();

    // other stuff
    m_pCurrentCommandList->OMSetBlendFactor(m_currentBlendStateBlendFactors);
    m_pCurrentCommandList->OMSetStencilRef(m_currentDepthStencilRef);

    // current descriptor heaps
    m_pCurrentCommandList->SetDescriptorHeaps(m_currentDescriptorHeaps.GetSize(), m_currentDescriptorHeaps.GetBasePointer());
}

bool D3D12GPUContext::AllocateScratchBufferMemory(uint32 size, ID3D12Resource **ppScratchBufferResource, uint32 *pScratchBufferOffset, void **ppCPUPointer, D3D12_GPU_VIRTUAL_ADDRESS *pGPUAddress)
{
    uint32 offset;
    if (!m_pCurrentScratchBuffer->Allocate(size, &offset))
    {
        // work out new buffer size
        uint32 newBufferSize = Max(m_pCurrentScratchBuffer->GetSize() + size, m_pCurrentScratchBuffer->GetSize() * 2);
        Log_PerfPrintf("D3D12GPUContext::AllocateScratchBufferMemory: Scratch buffer (%u bytes) overflow, allocating new buffer of %u bytes.", m_pCurrentScratchBuffer->GetSize(), newBufferSize);

        // allocate new buffer
        D3D12ScratchBuffer *pNewScratchBuffer = D3D12ScratchBuffer::Create(m_pD3DDevice, newBufferSize);
        if (pNewScratchBuffer == nullptr)
        {
            Log_ErrorPrintf("D3D12GPUContext::AllocateScratchBufferMemory: Failed to allocate new scratch buffer of size %u", newBufferSize);
            return false;
        }

        // nuke current scratch buffer (the internal buffer will be scheduled for deletion after the frame)
        delete m_pCurrentScratchBuffer;
        m_pCurrentScratchBuffer = pNewScratchBuffer;
        m_commandQueues[m_currentCommandQueueIndex].pScratchBuffer = pNewScratchBuffer;

        // allocate from new scratch buffer
        if (!m_pCurrentScratchBuffer->Allocate(size, &offset))
        {
            Log_ErrorPrintf("D3D12GPUContext::AllocateScratchBufferMemory: Failed to allocate new scratch buffer (this shouldn't happen)");
            return false;
        }
    }

    if (ppScratchBufferResource != nullptr)
        *ppScratchBufferResource = m_pCurrentScratchBuffer->GetResource();
    if (pScratchBufferOffset != nullptr)
        *pScratchBufferOffset = offset;
    if (ppCPUPointer != nullptr)
        *ppCPUPointer = m_pCurrentScratchBuffer->GetPointer(offset);
    if (pGPUAddress != nullptr)
        *pGPUAddress = m_pCurrentScratchBuffer->GetGPUAddress(offset);

    return true;
}

bool D3D12GPUContext::AllocateScratchView(uint32 count, D3D12_CPU_DESCRIPTOR_HANDLE *pOutCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE *pOutGPUHandle)
{
    if (!m_pCurrentScratchViewHeap->Allocate(count, pOutCPUHandle, pOutGPUHandle))
    {
        // work out new descriptor count
        uint32 newViewCount = Max(m_pCurrentScratchViewHeap->GetSize() + count, m_pCurrentScratchViewHeap->GetSize() * 2);
        Log_PerfPrintf("D3D12GPUContext::AllocateScratchView: Scratch view heap (%u entries) overflow, allocating new heap of %u entries.", m_pCurrentScratchViewHeap->GetSize(), newViewCount);

        // allocate new heap
        D3D12ScratchDescriptorHeap *pNewHeap = D3D12ScratchDescriptorHeap::Create(m_pD3DDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, newViewCount);
        if (pNewHeap == nullptr)
        {
            Log_ErrorPrintf("D3D12GPUContext::AllocateScratchView: Failed to allocate new scratch heap of size %u", newViewCount);
            return false;
        }

        // nuke current heap [will get postponed to delete later]
        delete m_pCurrentScratchViewHeap;
        m_pCurrentScratchViewHeap = pNewHeap;
        m_commandQueues[m_currentCommandQueueIndex].pScratchViewHeap = pNewHeap;

        // add to descriptor table list
        m_currentDescriptorHeaps.Add(pNewHeap->GetD3DHeap());

        // allocate from new heap
        if (!m_pCurrentScratchViewHeap->Allocate(count, pOutCPUHandle, pOutGPUHandle))
        {
            Log_ErrorPrintf("D3D12GPUContext::AllocateScratchView: Failed to allocate new heap (this shouldn't happen)");
            return false;
        }
    }

    return true;
}

bool D3D12GPUContext::AllocateScratchSamplers(uint32 count, D3D12_CPU_DESCRIPTOR_HANDLE *pOutCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE *pOutGPUHandle)
{
    if (!m_pCurrentScratchSamplerHeap->Allocate(count, pOutCPUHandle, pOutGPUHandle))
    {
        // work out new descriptor count
        uint32 newSamplerCount = Max(m_pCurrentScratchViewHeap->GetSize() + count, m_pCurrentScratchViewHeap->GetSize() * 2);
        Log_PerfPrintf("D3D12GPUContext::AllocateScratchView: Scratch view heap (%u entries) overflow, allocating new heap of %u entries.", m_pCurrentScratchViewHeap->GetSize(), newSamplerCount);

        // allocate new heap
        D3D12ScratchDescriptorHeap *pNewHeap = D3D12ScratchDescriptorHeap::Create(m_pD3DDevice, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, newSamplerCount);
        if (pNewHeap == nullptr)
        {
            Log_ErrorPrintf("D3D12GPUContext::AllocateScratchView: Failed to allocate new scratch heap of size %u", newSamplerCount);
            return false;
        }

        // nuke current heap [will get postponed to delete later]
        delete m_pCurrentScratchSamplerHeap;
        m_pCurrentScratchSamplerHeap = pNewHeap;
        m_commandQueues[m_currentCommandQueueIndex].pScratchSamplerHeap = pNewHeap;

        // add to descriptor table list
        m_currentDescriptorHeaps.Add(pNewHeap->GetD3DHeap());

        // allocate from new heap
        if (!m_pCurrentScratchSamplerHeap->Allocate(count, pOutCPUHandle, pOutGPUHandle))
        {
            Log_ErrorPrintf("D3D12GPUContext::AllocateScratchView: Failed to allocate new heap (this shouldn't happen)");
            return false;
        }
    }

    return true;
}

void D3D12GPUContext::ClearState(bool clearShaders /* = true */, bool clearBuffers /* = true */, bool clearStates /* = true */, bool clearRenderTargets /* = true */)
{
    if (clearShaders)
    {
        SetShaderProgram(nullptr);

        for (uint32 stage = 0; stage < SHADER_PROGRAM_STAGE_COUNT; stage++)
        {
            ShaderStageState &state = m_shaderStates[stage];

            if (state.ConstantBufferBindCount > 0)
            {
                Y_memzero(state.ConstantBuffers, sizeof(state.ConstantBuffers[0]) * state.ConstantBufferBindCount);
                state.ConstantBufferBindCount = 0;
                state.ConstantBuffersDirty = false;
            }

            if (state.ResourceBindCount > 0)
            {
                Y_memzero(state.Resources, sizeof(state.Resources[0]) * state.ResourceBindCount);
                state.ResourceBindCount = 0;
                state.ResourcesDirty = false;
            }

            if (state.SamplerBindCount > 0)
            {
                Y_memzero(state.Samplers, sizeof(state.Samplers[0]) * state.SamplerBindCount);
                state.SamplerBindCount = 0;
                state.SamplersDirty = false;
            }

            if (state.UAVBindCount > 0)
            {
                Y_memzero(state.UAVs, sizeof(state.UAVs[0]) * state.UAVBindCount);
                state.UAVBindCount = 0;
                state.UAVsDirty = false;
            }
        }
    }

    if (clearBuffers)
    {
        if (m_currentVertexBufferBindCount > 0)
        {
            static GPUBuffer *nullVertexBuffers[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = { nullptr };
            static const uint32 nullSizeOrOffset[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = { 0 };
            SetVertexBuffers(0, m_currentVertexBufferBindCount, nullVertexBuffers, nullSizeOrOffset, nullSizeOrOffset);
        }

        if (m_pCurrentIndexBuffer != nullptr)
            SetIndexBuffer(nullptr, GPU_INDEX_FORMAT_UINT16, 0);
    }

    if (clearStates)
    {
        SetRasterizerState(nullptr);
        SetDepthStencilState(nullptr, 0);
        SetBlendState(nullptr);
        SetDrawTopology(DRAW_TOPOLOGY_UNDEFINED);

        RENDERER_VIEWPORT viewport(0, 0, 0, 0, 0.0f, 1.0f);
        SetViewport(&viewport);

        RENDERER_SCISSOR_RECT scissor(0, 0, 0, 0);
        SetScissorRect(&scissor);
    }

    if (clearRenderTargets)
    {
        SetRenderTargets(0, nullptr, nullptr);
    }
}

GPURasterizerState *D3D12GPUContext::GetRasterizerState()
{
    return m_pCurrentRasterizerState;
}

void D3D12GPUContext::SetRasterizerState(GPURasterizerState *pRasterizerState)
{
    if (m_pCurrentRasterizerState != pRasterizerState)
    {
        if (m_pCurrentRasterizerState != nullptr)
            m_pCurrentRasterizerState->Release();

        if ((m_pCurrentRasterizerState = static_cast<D3D12GPURasterizerState *>(pRasterizerState)) != nullptr)
            m_pCurrentRasterizerState->AddRef();

        m_pipelineChanged = true;
    }
}

GPUDepthStencilState *D3D12GPUContext::GetDepthStencilState()
{
    return m_pCurrentDepthStencilState;
}

uint8 D3D12GPUContext::GetDepthStencilStateStencilRef()
{
    return m_currentDepthStencilRef;
}

void D3D12GPUContext::SetDepthStencilState(GPUDepthStencilState *pDepthStencilState, uint8 stencilRef)
{
    if (m_pCurrentDepthStencilState != pDepthStencilState)
    {
        if (m_pCurrentDepthStencilState != nullptr)
            m_pCurrentDepthStencilState->Release();

        if ((m_pCurrentDepthStencilState = static_cast<D3D12GPUDepthStencilState *>(pDepthStencilState)) != nullptr)
            m_pCurrentDepthStencilState->AddRef();

        m_pipelineChanged = true;
    }

    if (m_currentDepthStencilRef != stencilRef)
    {
        m_pCurrentCommandList->OMSetStencilRef(stencilRef);
        m_currentDepthStencilRef = stencilRef;
    }
}

GPUBlendState *D3D12GPUContext::GetBlendState()
{
    return m_pCurrentBlendState;
}

const float4 &D3D12GPUContext::GetBlendStateBlendFactor()
{
    return m_currentBlendStateBlendFactors;
};

void D3D12GPUContext::SetBlendState(GPUBlendState *pBlendState, const float4 &blendFactor /* = float4::One */)
{
    if (m_pCurrentBlendState != pBlendState)
    {
        if (m_pCurrentBlendState != nullptr)
            m_pCurrentBlendState->Release();

        if ((m_pCurrentBlendState = static_cast<D3D12GPUBlendState *>(pBlendState)) != nullptr)
            m_pCurrentBlendState->AddRef();

        m_pipelineChanged = true;
    }

    if (blendFactor != m_currentBlendStateBlendFactors)
    {
        m_pCurrentCommandList->OMSetBlendFactor(blendFactor.ele);
        m_currentBlendStateBlendFactors = blendFactor;
    }
}

const RENDERER_VIEWPORT *D3D12GPUContext::GetViewport()
{
    return &m_currentViewport;
}

void D3D12GPUContext::SetViewport(const RENDERER_VIEWPORT *pNewViewport)
{
    if (Y_memcmp(&m_currentViewport, pNewViewport, sizeof(RENDERER_VIEWPORT)) == 0)
        return;

    Y_memcpy(&m_currentViewport, pNewViewport, sizeof(m_currentViewport));

    D3D12_VIEWPORT D3DViewport;
    D3DViewport.TopLeftX = (float)m_currentViewport.TopLeftX;
    D3DViewport.TopLeftY = (float)m_currentViewport.TopLeftY;
    D3DViewport.Width = (float)m_currentViewport.Width;
    D3DViewport.Height = (float)m_currentViewport.Height;
    D3DViewport.MinDepth = pNewViewport->MinDepth;
    D3DViewport.MaxDepth = pNewViewport->MaxDepth;
    m_pCurrentCommandList->RSSetViewports(1, &D3DViewport);

    // update constants
    m_pConstants->SetViewportOffset((float)m_currentViewport.TopLeftX, (float)m_currentViewport.TopLeftY, false);
    m_pConstants->SetViewportSize((float)m_currentViewport.Width, (float)m_currentViewport.Height, false);
    m_pConstants->CommitChanges();
}

void D3D12GPUContext::SetFullViewport(GPUTexture *pForRenderTarget /* = NULL */)
{
    RENDERER_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;

    if (pForRenderTarget == NULL && m_nCurrentRenderTargets == 0 && m_pCurrentDepthBufferView == NULL)
    {
        viewport.Width = m_pCurrentSwapChain->GetWidth();
        viewport.Height = m_pCurrentSwapChain->GetHeight();
    }
    else
    {
        DebugAssert(m_pCurrentRenderTargetViews[0] != NULL || pForRenderTarget != NULL);

        GPUTexture *pRT = pForRenderTarget;
        if (pRT != nullptr || m_nCurrentRenderTargets > 0)
        {
            uint3 renderTargetDimensions = Renderer::GetTextureDimensions((pRT != nullptr) ? pRT : m_pCurrentRenderTargetViews[0]->GetTargetTexture());
            viewport.Width = renderTargetDimensions.x;
            viewport.Height = renderTargetDimensions.y;
        }
        else
        {
            viewport.Width = m_pCurrentSwapChain->GetWidth();
            viewport.Height = m_pCurrentSwapChain->GetHeight();
        }
    }

    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    SetViewport(&viewport);

    RENDERER_SCISSOR_RECT scissorRect;
    scissorRect.Set(0, 0, viewport.Width - 1, viewport.Height - 1);
    SetScissorRect(&scissorRect);
}

const RENDERER_SCISSOR_RECT *D3D12GPUContext::GetScissorRect()
{
    return &m_scissorRect;
}

void D3D12GPUContext::SetScissorRect(const RENDERER_SCISSOR_RECT *pScissorRect)
{
    if (Y_memcmp(&m_scissorRect, pScissorRect, sizeof(m_scissorRect)) == 0)
        return;

    Y_memcpy(&m_scissorRect, pScissorRect, sizeof(m_scissorRect));

    D3D12_RECT D3DRect;
    D3DRect.left = pScissorRect->Left;
    D3DRect.top = pScissorRect->Top;
    D3DRect.right = pScissorRect->Right;
    D3DRect.bottom = pScissorRect->Bottom;
    m_pCurrentCommandList->RSSetScissorRects(1, &D3DRect);
}

void D3D12GPUContext::ClearTargets(bool clearColor /* = true */, bool clearDepth /* = true */, bool clearStencil /* = true */, const float4 &clearColorValue /* = float4::Zero */, float clearDepthValue /* = 1.0f */, uint8 clearStencilValue /* = 0 */)
{
    D3D12_CLEAR_FLAGS clearFlags = (D3D12_CLEAR_FLAGS)0;
    if (clearDepth)
        clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
    if (clearStencil)
        clearFlags |= D3D12_CLEAR_FLAG_STENCIL;

    if (m_nCurrentRenderTargets == 0 && m_pCurrentDepthBufferView == nullptr)
    {
        // on swapchain
        if (m_pCurrentSwapChain != nullptr)
        {
            if (clearColor)
                m_pCurrentCommandList->ClearRenderTargetView(m_pCurrentSwapChain->GetCurrentBackBufferViewDescriptorCPUHandle(), clearColorValue, 0, nullptr);

            if (clearDepth && m_pCurrentSwapChain->GetDepthStencilBufferResource() != nullptr)
                m_pCurrentCommandList->ClearDepthStencilView(m_pCurrentSwapChain->GetDepthStencilBufferViewDescriptorCPUHandle(), clearFlags, clearDepthValue, clearStencilValue, 0, nullptr);
        }
    }
    else
    {
        if (clearColor)
        {
            for (uint32 i = 0; i < m_nCurrentRenderTargets; i++)
            {
                if (m_pCurrentRenderTargetViews[i] != nullptr)
                    m_pCurrentCommandList->ClearRenderTargetView(m_pCurrentRenderTargetViews[i]->GetDescriptorHandle(), clearColorValue, 0, nullptr);
            }
        }

        if (clearFlags != 0 && m_pCurrentDepthBufferView != nullptr)
            m_pCurrentCommandList->ClearDepthStencilView(m_pCurrentDepthBufferView->GetDescriptorHandle(), clearFlags, clearDepthValue, clearStencilValue, 0, nullptr);
    }
}

void D3D12GPUContext::DiscardTargets(bool discardColor /* = true */, bool discardDepth /* = true */, bool discardStencil /* = true */)
{
    if (m_nCurrentRenderTargets == 0 && m_pCurrentDepthBufferView == nullptr)
    {
        // on swapchain
        if (m_pCurrentSwapChain != nullptr)
        {
            if (discardColor)
                m_pCurrentCommandList->DiscardResource(m_pCurrentSwapChain->GetCurrentBackBufferResource(), nullptr);
            if (discardDepth && discardStencil && m_pCurrentSwapChain->HasDepthStencilBuffer())
                m_pCurrentCommandList->DiscardResource(m_pCurrentSwapChain->GetDepthStencilBufferResource(), nullptr);
        }
    }
    else
    {
        if (discardColor)
        {
            for (uint32 i = 0; i < m_nCurrentRenderTargets; i++)
            {
                if (m_pCurrentRenderTargetViews[i] != nullptr)
                    m_pCurrentCommandList->DiscardResource(m_pCurrentRenderTargetViews[i]->GetD3DResource(), nullptr);
            }
        }

        if (discardDepth && discardStencil && m_pCurrentDepthBufferView != nullptr)
            m_pCurrentCommandList->DiscardResource(m_pCurrentDepthBufferView->GetD3DResource(), nullptr);
    }
}

GPUOutputBuffer *D3D12GPUContext::GetOutputBuffer()
{
    return m_pCurrentSwapChain;
}

void D3D12GPUContext::SetOutputBuffer(GPUOutputBuffer *pSwapChain)
{
    DebugAssert(pSwapChain != nullptr);
    if (m_pCurrentSwapChain == pSwapChain)
        return;

    // copy out old swap chain
    D3D12GPUOutputBuffer *pOldSwapChain = m_pCurrentSwapChain;

    // copy in new swap chain
    if ((m_pCurrentSwapChain = static_cast<D3D12GPUOutputBuffer *>(pSwapChain)) != nullptr)
        m_pCurrentSwapChain->AddRef();

    // Currently rendering to window?
    if (m_nCurrentRenderTargets == 0 && m_pCurrentDepthBufferView == nullptr)
        SynchronizeRenderTargetsAndUAVs();

    // update references
    if (pOldSwapChain != nullptr)
        pOldSwapChain->Release();
}

bool D3D12GPUContext::GetExclusiveFullScreen()
{
    BOOL currentState;
    HRESULT hResult = m_pCurrentSwapChain->GetDXGISwapChain()->GetFullscreenState(&currentState, nullptr);
    if (FAILED(hResult))
        return false;

    return (currentState == TRUE);
}

bool D3D12GPUContext::SetExclusiveFullScreen(bool enabled, uint32 width, uint32 height, uint32 refreshRate)
{
    // @TODO
    return false;
}

bool D3D12GPUContext::ResizeOutputBuffer(uint32 width /* = 0 */, uint32 height /* = 0 */)
{
    if (width == 0 || height == 0)
    {
        // get the new size of the window
        RECT clientRect;
        GetClientRect(m_pCurrentSwapChain->GetHWND(), &clientRect);

        // changed?
        width = Max(clientRect.right - clientRect.left, (LONG)1);
        height = Max(clientRect.bottom - clientRect.top, (LONG)1);
    }

    // changed?
    if (m_pCurrentSwapChain->GetWidth() == width && m_pCurrentSwapChain->GetHeight() == height)
        return true;

    // unbind if we're currently bound to the pipeline
    if (m_nCurrentRenderTargets == 0 && m_pCurrentDepthBufferView == nullptr)
        m_pCurrentCommandList->OMSetRenderTargets(0, nullptr, FALSE, nullptr);

    // invoke the resize
    m_pCurrentSwapChain->InternalResizeBuffers(width, height, m_pCurrentSwapChain->GetVSyncType());

    // synchronize render targets if we were bound
    if (m_nCurrentRenderTargets == 0 && m_pCurrentDepthBufferView == nullptr)
        SynchronizeRenderTargetsAndUAVs();

    // done
    return true;
}

void D3D12GPUContext::PresentOutputBuffer(GPU_PRESENT_BEHAVIOUR presentBehaviour)
{
    // the barrier *to* present state has to be queued
    ResourceBarrier(m_pCurrentSwapChain->GetCurrentBackBufferResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

    // ensure all commands have been queued
    ExecuteCurrentCommandList(false);

    // present the image
    m_pCurrentSwapChain->GetDXGISwapChain()->Present((presentBehaviour == GPU_PRESENT_BEHAVIOUR_WAIT_FOR_VBLANK) ? 1 : 0, 0);

    // move to next command list (placed after here so the present happens before the fence.
    MoveToNextCommandQueue();

    // transition back
    ResourceBarrier(m_pCurrentSwapChain->GetCurrentBackBufferResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // update the new backbuffer for the swap chain
    m_pCurrentSwapChain->UpdateCurrentBackBuffer();

    // restore state (since new command list)
    RestoreCommandListDependantState();
}

uint32 D3D12GPUContext::GetRenderTargets(uint32 nRenderTargets, GPURenderTargetView **ppRenderTargetViews, GPUDepthStencilBufferView **ppDepthBufferView)
{
    uint32 i, j;

    for (i = 0; i < m_nCurrentRenderTargets && i < nRenderTargets; i++)
        ppRenderTargetViews[i] = m_pCurrentRenderTargetViews[i];

    for (j = i; j < nRenderTargets; j++)
        ppRenderTargetViews[j] = nullptr;

    if (ppDepthBufferView != nullptr)
        *ppDepthBufferView = m_pCurrentDepthBufferView;

    return i;
}

void D3D12GPUContext::SetRenderTargets(uint32 nRenderTargets, GPURenderTargetView **ppRenderTargets, GPUDepthStencilBufferView *pDepthBufferView)
{
    // @TODO only update pipeline on format change / # targets change

    // to system framebuffer?
    if ((nRenderTargets == 0 || (nRenderTargets == 1 && ppRenderTargets[0] == nullptr)) && pDepthBufferView == nullptr)
    {
        // change?
        if (m_nCurrentRenderTargets == 0 && m_pCurrentDepthBufferView == nullptr)
            return;

        // kill current targets
        for (uint32 i = 0; i < m_nCurrentRenderTargets; i++)
        {
            m_pCurrentRenderTargetViews[i]->Release();
            m_pCurrentRenderTargetViews[i] = nullptr;
        }
        if (m_pCurrentDepthBufferView != nullptr)
        {
            m_pCurrentDepthBufferView->Release();
            m_pCurrentDepthBufferView = nullptr;
        }

        m_nCurrentRenderTargets = 0;
        SynchronizeRenderTargetsAndUAVs();
        m_pipelineChanged = true;
    }
    else
    {
        DebugAssert(nRenderTargets < countof(m_pCurrentRenderTargetViews));

        uint32 slot;
        uint32 newRenderTargetCount = 0;
        bool doUpdate = false;

        // set inclusive slots
        for (slot = 0; slot < nRenderTargets; slot++)
        {
            if (m_pCurrentRenderTargetViews[slot] != ppRenderTargets[slot])
            {
                if (m_pCurrentRenderTargetViews[slot] != nullptr)
                    m_pCurrentRenderTargetViews[slot]->Release();

                if ((m_pCurrentRenderTargetViews[slot] = static_cast<D3D12GPURenderTargetView *>(ppRenderTargets[slot])) != nullptr)
                    m_pCurrentRenderTargetViews[slot]->AddRef();

                doUpdate = true;
            }

            if (m_pCurrentRenderTargetViews[slot] != nullptr)
                newRenderTargetCount = slot + 1;
        }

        // clear extra slots
        for (; slot < m_nCurrentRenderTargets; slot++)
        {
            if (m_pCurrentRenderTargetViews[slot] != nullptr)
            {
                m_pCurrentRenderTargetViews[slot]->Release();
                m_pCurrentRenderTargetViews[slot] = nullptr;
                doUpdate = true;
            }
        }

        // update counter
        if (newRenderTargetCount != m_nCurrentRenderTargets)
            doUpdate = true;
        m_nCurrentRenderTargets = newRenderTargetCount;

        if (m_pCurrentDepthBufferView != pDepthBufferView)
        {
            if (m_pCurrentDepthBufferView != nullptr)
                m_pCurrentDepthBufferView->Release();

            if ((m_pCurrentDepthBufferView = static_cast<D3D12GPUDepthStencilBufferView *>(pDepthBufferView)) != nullptr)
                m_pCurrentDepthBufferView->AddRef();

            doUpdate = true;
        }

        if (doUpdate)
        {
            SynchronizeRenderTargetsAndUAVs();
            m_pipelineChanged = true;
        }
    }
}

DRAW_TOPOLOGY D3D12GPUContext::GetDrawTopology()
{
    return m_currentTopology;
}

void D3D12GPUContext::SetDrawTopology(DRAW_TOPOLOGY topology)
{
    static const D3D12_PRIMITIVE_TOPOLOGY D3D12Topologies[DRAW_TOPOLOGY_COUNT] =
    {
        D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,           // DRAW_TOPOLOGY_UNDEFINED
        D3D_PRIMITIVE_TOPOLOGY_POINTLIST,           // DRAW_TOPOLOGY_POINTS
        D3D_PRIMITIVE_TOPOLOGY_LINELIST,            // DRAW_TOPOLOGY_LINE_LIST
        D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,           // DRAW_TOPOLOGY_LINE_STRIP
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,        // DRAW_TOPOLOGY_TRIANGLE_LIST
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,       // DRAW_TOPOLOGY_TRIANGLE_STRIP
    };

    static const D3D12_PRIMITIVE_TOPOLOGY_TYPE D3D12ToplogyTypes[DRAW_TOPOLOGY_COUNT] =
    {
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED,    // DRAW_TOPOLOGY_UNDEFINED
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,        // DRAW_TOPOLOGY_POINTS
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,         // DRAW_TOPOLOGY_LINE_LIST
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,         // DRAW_TOPOLOGY_LINE_STRIP
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,     // DRAW_TOPOLOGY_TRIANGLE_LIST
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,     // DRAW_TOPOLOGY_TRIANGLE_STRIP
    };

    if (topology == m_currentTopology)
        return;

    DebugAssert(topology < DRAW_TOPOLOGY_COUNT);
    m_pCurrentCommandList->IASetPrimitiveTopology(D3D12Topologies[topology]);
    m_currentTopology = topology;

    if (D3D12ToplogyTypes[topology] != m_currentD3DTopologyType)
    {
        m_currentD3DTopologyType = D3D12ToplogyTypes[topology];
        m_pipelineChanged = true;
    }
}

uint32 D3D12GPUContext::GetVertexBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer **ppVertexBuffers, uint32 *pVertexBufferOffsets, uint32 *pVertexBufferStrides)
{
    DebugAssert(firstBuffer + nBuffers < countof(m_pCurrentVertexBuffers));

    uint32 saveCount;
    for (saveCount = 0; saveCount < nBuffers; saveCount++)
    {
        if ((firstBuffer + saveCount) > m_currentVertexBufferBindCount)
            break;

        ppVertexBuffers[saveCount] = m_pCurrentVertexBuffers[firstBuffer + saveCount];
        pVertexBufferOffsets[saveCount] = m_currentVertexBufferOffsets[firstBuffer + saveCount];
        pVertexBufferStrides[saveCount] = m_currentVertexBufferStrides[firstBuffer + saveCount];
    }

    return saveCount;
}

void D3D12GPUContext::SetVertexBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer *const *ppVertexBuffers, const uint32 *pVertexBufferOffsets, const uint32 *pVertexBufferStrides)
{
    D3D12_VERTEX_BUFFER_VIEW vertexBufferViews[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];

    for (uint32 i = 0; i < nBuffers; i++)
    {
        D3D12GPUBuffer *pD3D12VertexBuffer = static_cast<D3D12GPUBuffer *>(ppVertexBuffers[i]);
        uint32 bufferIndex = firstBuffer + i;

        if (m_pCurrentVertexBuffers[bufferIndex] != nullptr)
        {
            m_pCurrentVertexBuffers[bufferIndex]->Release();
            m_pCurrentVertexBuffers[bufferIndex] = nullptr;
        }

        if ((m_pCurrentVertexBuffers[bufferIndex] = pD3D12VertexBuffer) != nullptr)
        {
            pD3D12VertexBuffer->AddRef();
            m_currentVertexBufferOffsets[bufferIndex] = pVertexBufferOffsets[i];
            m_currentVertexBufferStrides[bufferIndex] = pVertexBufferStrides[i];

            // @TODO cache this address, save virtual call?
            DebugAssert(pVertexBufferOffsets[i] < pD3D12VertexBuffer->GetDesc()->Size);
            vertexBufferViews[i].BufferLocation = pD3D12VertexBuffer->GetD3DResource()->GetGPUVirtualAddress() + pVertexBufferOffsets[i];
            vertexBufferViews[i].SizeInBytes = pD3D12VertexBuffer->GetDesc()->Size - pVertexBufferOffsets[i];
            vertexBufferViews[i].StrideInBytes = pVertexBufferStrides[i];
        }
        else
        {
            m_currentVertexBufferOffsets[bufferIndex] = 0;
            m_currentVertexBufferStrides[bufferIndex] = 0;
            Y_memzero(&vertexBufferViews[i], sizeof(vertexBufferViews[0]));
        }
    }

    // pass to command list
    // @TODO may be worth batching this to a dirty range?
    m_pCurrentCommandList->IASetVertexBuffers(firstBuffer, nBuffers, vertexBufferViews);

    // update new bind count
    uint32 bindCount = 0;
    uint32 searchCount = Max((firstBuffer + nBuffers), m_currentVertexBufferBindCount);
    for (uint32 i = 0; i < searchCount; i++)
    {
        if (m_pCurrentVertexBuffers[i] != nullptr)
            bindCount = i + 1;
    }
    m_currentVertexBufferBindCount = bindCount;
}

void D3D12GPUContext::SetVertexBuffer(uint32 bufferIndex, GPUBuffer *pVertexBuffer, uint32 offset, uint32 stride)
{
    if (m_pCurrentVertexBuffers[bufferIndex] == pVertexBuffer &&
        m_currentVertexBufferOffsets[bufferIndex] == offset &&
        m_currentVertexBufferStrides[bufferIndex] == stride)
    {
        return;
    }

    if (m_pCurrentVertexBuffers[bufferIndex] != pVertexBuffer)
    {
        if (m_pCurrentVertexBuffers[bufferIndex] != nullptr)
            m_pCurrentVertexBuffers[bufferIndex]->Release();

        if ((m_pCurrentVertexBuffers[bufferIndex] = static_cast<D3D12GPUBuffer *>(pVertexBuffer)) != nullptr)
            m_pCurrentVertexBuffers[bufferIndex]->AddRef();
    }

    m_currentVertexBufferOffsets[bufferIndex] = offset;
    m_currentVertexBufferStrides[bufferIndex] = stride;

    // set in command list
    if (m_pCurrentVertexBuffers[bufferIndex] != nullptr)
    {
        D3D12_VERTEX_BUFFER_VIEW bufferView;
        DebugAssert(offset < m_pCurrentVertexBuffers[bufferIndex]->GetDesc()->Size);
        bufferView.BufferLocation = m_pCurrentVertexBuffers[bufferIndex]->GetD3DResource()->GetGPUVirtualAddress() + offset;
        bufferView.SizeInBytes = m_pCurrentVertexBuffers[bufferIndex]->GetDesc()->Size - offset;
        bufferView.StrideInBytes = stride;
        m_pCurrentCommandList->IASetVertexBuffers(bufferIndex, 1, &bufferView);
    }
    else
    {
        m_pCurrentCommandList->IASetVertexBuffers(bufferIndex, 1, nullptr);
    }

    // update new bind count
    uint32 bindCount = 0;
    uint32 searchCount = Max((bufferIndex + 1), m_currentVertexBufferBindCount);
    for (uint32 i = 0; i < searchCount; i++)
    {
        if (m_pCurrentVertexBuffers[i] != nullptr)
            bindCount = i + 1;
    }
    m_currentVertexBufferBindCount = bindCount;
}

void D3D12GPUContext::GetIndexBuffer(GPUBuffer **ppBuffer, GPU_INDEX_FORMAT *pFormat, uint32 *pOffset)
{
    *ppBuffer = m_pCurrentIndexBuffer;
    *pFormat = m_currentIndexFormat;
    *pOffset = m_currentIndexBufferOffset;
}

void D3D12GPUContext::SetIndexBuffer(GPUBuffer *pBuffer, GPU_INDEX_FORMAT format, uint32 offset)
{
    if (m_pCurrentIndexBuffer == pBuffer && m_currentIndexFormat == format && m_currentIndexBufferOffset == offset)
        return;

    if (m_pCurrentIndexBuffer != pBuffer)
    {
        if (m_pCurrentIndexBuffer != nullptr)
            m_pCurrentIndexBuffer->Release();

        if ((m_pCurrentIndexBuffer = static_cast<D3D12GPUBuffer *>(pBuffer)) != nullptr)
            m_pCurrentIndexBuffer->AddRef();
    }

    m_currentIndexFormat = format;
    m_currentIndexBufferOffset = offset;

    if (m_pCurrentIndexBuffer != nullptr)
    {
        D3D12_INDEX_BUFFER_VIEW bufferView;
        DebugAssert(offset < m_pCurrentIndexBuffer->GetDesc()->Size);
        bufferView.BufferLocation = m_pCurrentIndexBuffer->GetD3DResource()->GetGPUVirtualAddress() + offset;
        bufferView.SizeInBytes = m_pCurrentIndexBuffer->GetDesc()->Size - offset;
        bufferView.Format = (format == GPU_INDEX_FORMAT_UINT16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        m_pCurrentCommandList->IASetIndexBuffer(&bufferView);
    }
    else
    {
        m_pCurrentCommandList->IASetIndexBuffer(nullptr);
    }
}

void D3D12GPUContext::SetShaderProgram(GPUShaderProgram *pShaderProgram)
{
    if (m_pCurrentShaderProgram == pShaderProgram)
        return;

    if (m_pCurrentShaderProgram != nullptr)
        m_pCurrentShaderProgram->Release();
    if ((m_pCurrentShaderProgram = static_cast<D3D12GPUShaderProgram *>(pShaderProgram)) != nullptr)
        m_pCurrentShaderProgram->AddRef();

    m_pipelineChanged = true;
}

void D3D12GPUContext::WriteConstantBuffer(uint32 bufferIndex, uint32 fieldIndex, uint32 offset, uint32 count, const void *pData, bool commit /* = false */)
{
    ConstantBuffer *cbInfo = &m_constantBuffers[bufferIndex];
    if (cbInfo->pLocalMemory == nullptr)
    {
        Log_WarningPrintf("Skipping write of %u bytes to non-existant constant buffer %u", count, bufferIndex);
        return;
    }

    DebugAssert((offset + count) <= cbInfo->Size);
    if (Y_memcmp(cbInfo->pLocalMemory + offset, pData, count) != 0)
    {
        Y_memcpy(cbInfo->pLocalMemory + offset, pData, count);

        if (cbInfo->DirtyUpperBounds < 0)
        {
            cbInfo->DirtyLowerBounds = offset;
            cbInfo->DirtyUpperBounds = offset + count;
        }
        else
        {
            cbInfo->DirtyLowerBounds = Min(cbInfo->DirtyLowerBounds, (int32)offset);
            cbInfo->DirtyUpperBounds = Min(cbInfo->DirtyUpperBounds, (int32)(offset + count));
        }

        if (commit)
            CommitConstantBuffer(bufferIndex);
    }
}

void D3D12GPUContext::WriteConstantBufferStrided(uint32 bufferIndex, uint32 fieldIndex, uint32 offset, uint32 bufferStride, uint32 copySize, uint32 count, const void *pData, bool commit /*= false*/)
{
    ConstantBuffer *cbInfo = &m_constantBuffers[bufferIndex];
    if (cbInfo->pLocalMemory == nullptr)
    {
        Log_WarningPrintf("Skipping write of %u bytes to non-existant constant buffer %u", count, bufferIndex);
        return;
    }

    uint32 writeSize = bufferStride * count;
    DebugAssert((offset + writeSize) <= cbInfo->Size);

    if (Y_memcmp_stride(cbInfo->pLocalMemory + offset, bufferStride, pData, copySize, copySize, count) != 0)
    {
        Y_memcpy_stride(cbInfo->pLocalMemory + offset, bufferStride, pData, copySize, copySize, count);

        if (cbInfo->DirtyUpperBounds < 0)
        {
            cbInfo->DirtyLowerBounds = offset;
            cbInfo->DirtyUpperBounds = offset + writeSize;
        }
        else
        {
            cbInfo->DirtyLowerBounds = Min(cbInfo->DirtyLowerBounds, (int32)offset);
            cbInfo->DirtyUpperBounds = Min(cbInfo->DirtyUpperBounds, (int32)(offset + writeSize));
        }

        if (commit)
            CommitConstantBuffer(bufferIndex);
    }
}

void D3D12GPUContext::CommitConstantBuffer(uint32 bufferIndex)
{
    ConstantBuffer *cbInfo = &m_constantBuffers[bufferIndex];
    if (cbInfo->pLocalMemory == nullptr || cbInfo->DirtyLowerBounds < 0)
        return;

    // work out count to modify
    uint32 modifySize = cbInfo->DirtyUpperBounds - cbInfo->DirtyLowerBounds + 1;

    // allocate scratch buffer memory
    ID3D12Resource *pScratchBufferResource;
    uint32 scratchBufferOffset;
    void *pCPUPointer;
    if (!AllocateScratchBufferMemory(modifySize, &pScratchBufferResource, &scratchBufferOffset, &pCPUPointer, nullptr))
    {
        Log_ErrorPrintf("D3D12GPUContext::CommitConstantBuffer: Failed to allocate scratch buffer memory.");
        return;
    }

    // block predicates
    BypassPredication();

    // copy from the constant memory store to the scratch buffer
    Y_memcpy(pCPUPointer, cbInfo->pLocalMemory + cbInfo->DirtyLowerBounds, modifySize);

    // get constant buffer pointer
    ID3D12Resource *pConstantBufferResource = m_pBackend->GetConstantBufferResource(bufferIndex);
    DebugAssert(pConstantBufferResource != nullptr);

    // queue a copy to the actual buffer
    ResourceBarrier(pConstantBufferResource, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
    m_pCurrentCommandList->CopyBufferRegion(pConstantBufferResource, cbInfo->DirtyLowerBounds, pScratchBufferResource, scratchBufferOffset, modifySize);
    ResourceBarrier(pConstantBufferResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    // restore predicates
    RestorePredication();

    // reset range
    cbInfo->DirtyLowerBounds = cbInfo->DirtyUpperBounds = -1;
}

void D3D12GPUContext::SetShaderConstantBuffers(SHADER_PROGRAM_STAGE stage, uint32 index, const D3D12DescriptorHandle &handle)
{
    ShaderStageState *state = &m_shaderStates[stage];
    if (state->ConstantBuffers[index] == handle)
        return;

    state->ConstantBuffers[index] = handle;
    state->ConstantBufferBindCount = Max(state->ConstantBufferBindCount, index + 1);
    state->ConstantBuffersDirty = true;
}

void D3D12GPUContext::SetShaderResources(SHADER_PROGRAM_STAGE stage, uint32 index, const D3D12DescriptorHandle &handle)
{
    ShaderStageState *state = &m_shaderStates[stage];
    if (state->Resources[index] == handle)
        return;

    state->Resources[index] = handle;
    state->ResourceBindCount = Max(state->ResourceBindCount, index + 1);
    state->ResourcesDirty = true;
}

void D3D12GPUContext::SetShaderSamplers(SHADER_PROGRAM_STAGE stage, uint32 index, const D3D12DescriptorHandle &handle)
{
    ShaderStageState *state = &m_shaderStates[stage];
    if (state->Samplers[index] == handle)
        return;

    state->Samplers[index] = handle;
    state->SamplerBindCount = Max(state->SamplerBindCount, index + 1);
    state->SamplersDirty = true;
}

void D3D12GPUContext::SetShaderUAVs(SHADER_PROGRAM_STAGE stage, uint32 index, const D3D12DescriptorHandle &handle)
{
    ShaderStageState *state = &m_shaderStates[stage];
    if (state->UAVs[index] == handle)
        return;

    state->UAVs[index] = handle;
    state->UAVBindCount = Max(state->UAVBindCount, index + 1);
    state->UAVsDirty = true;
}

void D3D12GPUContext::SynchronizeRenderTargetsAndUAVs()
{
    D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandles[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
    D3D12_CPU_DESCRIPTOR_HANDLE depthStencilHandle;
    uint32 renderTargetCount = 0;
    bool hasDepthStencil = false;

    // get render target views
    if (m_nCurrentRenderTargets == 0 && m_pCurrentDepthBufferView == nullptr)
    {
        // using swap chain
        if (m_pCurrentSwapChain != nullptr)
        {
            renderTargetHandles[0] = m_pCurrentSwapChain->GetCurrentBackBufferViewDescriptorCPUHandle();
            depthStencilHandle = m_pCurrentSwapChain->GetDepthStencilBufferViewDescriptorCPUHandle();
            hasDepthStencil = m_pCurrentSwapChain->HasDepthStencilBuffer();
            renderTargetCount = 1;
        }
    }
    else
    {
        // get render target views
        for (uint32 renderTargetIndex = 0; renderTargetIndex < m_nCurrentRenderTargets; renderTargetIndex++)
        {
            if (m_pCurrentRenderTargetViews[renderTargetIndex] != nullptr)
            {
                renderTargetHandles[renderTargetIndex] = m_pCurrentRenderTargetViews[renderTargetIndex]->GetDescriptorHandle();
                renderTargetCount = renderTargetIndex + 1;
            }
            else
            {
                renderTargetHandles[renderTargetIndex].ptr = 0;
            }
        }

        // get depth stencil view
        if (m_pCurrentDepthBufferView != nullptr)
        {
            hasDepthStencil = true;
            depthStencilHandle = m_pCurrentDepthBufferView->GetDescriptorHandle();
        }
    }

    m_pCurrentCommandList->OMSetRenderTargets(renderTargetCount, (renderTargetCount > 0) ? renderTargetHandles : nullptr, FALSE, (hasDepthStencil) ? &depthStencilHandle : nullptr);
}

bool D3D12GPUContext::UpdatePipelineState()
{
    // new pipeline state required?
    if (m_pipelineChanged)
    {
        if (m_pCurrentShaderProgram == nullptr)
            return false;

        // fill pipeline state key
        D3D12GPUShaderProgram::PipelineStateKey key;
        Y_memzero(&key, sizeof(key));

        // render targets
        if (m_nCurrentRenderTargets == 0 && m_pCurrentDepthBufferView == nullptr)
        {
            if (m_pCurrentSwapChain != nullptr)
            {
                key.RenderTargetCount = 1;
                key.RTVFormats[0] = m_pCurrentSwapChain->GetBackBufferFormat();
                key.DSVFormat = m_pCurrentSwapChain->GetDepthStencilBufferFormat();
            }
        }
        else
        {
            key.RenderTargetCount = m_nCurrentRenderTargets;
            for (uint32 i = 0; i < m_nCurrentRenderTargets; i++)
                key.RTVFormats[i] = (m_pCurrentRenderTargetViews[i] != nullptr) ? m_pCurrentRenderTargetViews[i]->GetDesc()->Format : PIXEL_FORMAT_UNKNOWN;
            key.DSVFormat = (m_pCurrentDepthBufferView != nullptr) ? m_pCurrentDepthBufferView->GetDesc()->Format : PIXEL_FORMAT_UNKNOWN;
        }

        // rasterizer state
        if (m_pCurrentRasterizerState == nullptr)
            return false;
        Y_memcpy(&key.RasterizerState, m_pCurrentRasterizerState->GetD3DRasterizerStateDesc(), sizeof(key.RasterizerState));

        // depthstencil state
        if (m_pCurrentDepthStencilState == nullptr)
            return false;
        Y_memcpy(&key.DepthStencilState, m_pCurrentDepthStencilState->GetD3DDepthStencilDesc(), sizeof(key.DepthStencilState));

        // blend state
        if (m_pCurrentBlendState == nullptr)
            return false;
        Y_memcpy(&key.BlendState, m_pCurrentBlendState->GetD3DBlendDesc(), sizeof(key.BlendState));

        // topology
        key.PrimitiveTopologyType = m_currentD3DTopologyType;
        if (!m_pCurrentShaderProgram->Switch(this, m_pCurrentCommandList, &key))
            return false;

        // up-to-date
        m_pipelineChanged = false;
    }

    // allocate an array of cpu pointers, uses alloca so it's at the end of the stack
    D3D12_CPU_DESCRIPTOR_HANDLE *pCPUHandles = (D3D12_CPU_DESCRIPTOR_HANDLE *)alloca(sizeof(D3D12_CPU_DESCRIPTOR_HANDLE) * D3D12_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT);

    // update states
    for (uint32 stage = 0; stage < SHADER_PROGRAM_STAGE_COUNT; stage++)
    {
        ShaderStageState *state = &m_shaderStates[stage];

        // Constant buffers
        if (state->ConstantBuffersDirty)
        {
            // find the new bind count
            uint32 bindCount = 0;
            for (uint32 i = 0; i < state->ConstantBufferBindCount; i++)
            {
                if (!state->ConstantBuffers[i].IsNull())
                {
                    pCPUHandles[i] = state->ConstantBuffers[i];
                    bindCount = i + 1;
                }
                else
                {
                    pCPUHandles[i].ptr = 0;
                }
            }
            state->ConstantBufferBindCount = bindCount;
            state->ConstantBuffersDirty = false;

            // allocate scratch descriptors and copy
            if (bindCount > 0 && AllocateScratchView(bindCount, &state->CBVTableCPUHandle, &state->CBVTableGPUHandle))
            {
                m_pD3DDevice->CopyDescriptors(1, &state->CBVTableCPUHandle, nullptr, bindCount, pCPUHandles, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                m_pCurrentCommandList->SetGraphicsRootDescriptorTable(3 * stage + 0, state->CBVTableGPUHandle);
            }
        }

        // Resources
        if (state->ResourcesDirty)
        {
            // find the new bind count
            uint32 bindCount = 0;
            for (uint32 i = 0; i < state->ResourceBindCount; i++)
            {
                if (!state->Resources[i].IsNull())
                {
                    pCPUHandles[i] = state->Resources[i];
                    bindCount = i + 1;
                }
                else
                {
                    pCPUHandles[i].ptr = 0;
                }
            }
            state->ResourceBindCount = bindCount;
            state->ResourcesDirty = false;

            // allocate scratch descriptors and copy
            if (bindCount > 0 && AllocateScratchView(bindCount, &state->SRVTableCPUHandle, &state->SRVTableGPUHandle))
            {
                m_pD3DDevice->CopyDescriptors(1, &state->SRVTableCPUHandle, nullptr, bindCount, pCPUHandles, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                m_pCurrentCommandList->SetGraphicsRootDescriptorTable(3 * stage + 1, state->SRVTableGPUHandle);
            }
        }

        // Samplers
        if (state->SamplersDirty)
        {
            // find the new bind count
            uint32 bindCount = 0;
            for (uint32 i = 0; i < state->SamplerBindCount; i++)
            {
                if (!state->Samplers[i].IsNull())
                {
                    pCPUHandles[i] = state->Samplers[i];
                    bindCount = i + 1;
                }
                else
                {
                    pCPUHandles[i].ptr = 0;
                }
            }
            state->SamplerBindCount = bindCount;
            state->SamplersDirty = false;

            // allocate scratch descriptors and copy
            if (bindCount > 0 && AllocateScratchSamplers(bindCount, &state->SamplerTableCPUHandle, &state->SamplerTableGPUHandle))
            {
                m_pD3DDevice->CopyDescriptors(1, &state->SamplerTableCPUHandle, nullptr, bindCount, pCPUHandles, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
                m_pCurrentCommandList->SetGraphicsRootDescriptorTable(3 * stage + 2, state->SamplerTableGPUHandle);
            }
        }

        // UAVs
        if (stage == SHADER_PROGRAM_STAGE_PIXEL_SHADER && state->UAVsDirty)
        {
            // find the new bind count
            uint32 bindCount = 0;
            for (uint32 i = 0; i < state->SamplerBindCount; i++)
            {
                if (!state->UAVs[i].IsNull())
                {
                    pCPUHandles[i] = state->UAVs[i];
                    bindCount = i + 1;
                }
                else
                {
                    pCPUHandles[i].ptr = 0;
                }
            }
            state->UAVBindCount = bindCount;
            state->UAVsDirty = false;

            // allocate scratch descriptors and copy
            if (bindCount > 0 && AllocateScratchView(bindCount, &state->UAVTableCPUHandle, &state->UAVTableGPUHandle))
            {
                m_pD3DDevice->CopyDescriptors(1, &state->UAVTableCPUHandle, nullptr, bindCount, pCPUHandles, nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                m_pCurrentCommandList->SetGraphicsRootDescriptorTable(15, state->UAVTableGPUHandle);
            }
        }
    }

    // Commit global constant buffer
    m_pConstants->CommitGlobalConstantBufferChanges();
    return true;
}

void D3D12GPUContext::Draw(uint32 firstVertex, uint32 nVertices)
{
    if (nVertices == 0 || !UpdatePipelineState())
        return;
    
    m_pCurrentCommandList->DrawInstanced(nVertices, 1, firstVertex, 0);
    g_pRenderer->GetStats()->IncrementDrawCallCounter();
}

void D3D12GPUContext::DrawInstanced(uint32 firstVertex, uint32 nVertices, uint32 nInstances)
{
    if (nVertices == 0 || nInstances == 0 || !UpdatePipelineState())
        return;

    m_pCurrentCommandList->DrawInstanced(nVertices, nInstances, firstVertex, 0);
    g_pRenderer->GetStats()->IncrementDrawCallCounter();
}

void D3D12GPUContext::DrawIndexed(uint32 startIndex, uint32 nIndices, uint32 baseVertex)
{
    if (nIndices == 0 || !UpdatePipelineState())
        return;

    m_pCurrentCommandList->DrawIndexedInstanced(nIndices, 1, startIndex, baseVertex, 0);
    g_pRenderer->GetStats()->IncrementDrawCallCounter();
}

void D3D12GPUContext::DrawIndexedInstanced(uint32 startIndex, uint32 nIndices, uint32 baseVertex, uint32 nInstances)
{
    if (nIndices == 0 || !UpdatePipelineState())
        return;

    m_pCurrentCommandList->DrawIndexedInstanced(nIndices, nInstances, startIndex, baseVertex, 0);
    g_pRenderer->GetStats()->IncrementDrawCallCounter();
}

void D3D12GPUContext::Dispatch(uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ)
{
    if (!UpdatePipelineState())
        return;

    m_pCurrentCommandList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    g_pRenderer->GetStats()->IncrementDrawCallCounter();
}

void D3D12GPUContext::DrawUserPointer(const void *pVertices, uint32 vertexSize, uint32 nVertices)
{
    if (!UpdatePipelineState())
        return;

    // can use scratch buffer directly. 
    // this probably should use a threshold to go to an upload buffer..
    uint32 bufferSpaceRequired = vertexSize * nVertices;
    ID3D12Resource *pScratchBufferResource;
    void *pScratchBufferCPUPointer;
    D3D12_GPU_VIRTUAL_ADDRESS scratchBufferGPUPointer;
    uint32 scratchBufferOffset;
    if (!AllocateScratchBufferMemory(bufferSpaceRequired, &pScratchBufferResource, &scratchBufferOffset, &pScratchBufferCPUPointer, &scratchBufferGPUPointer))
        return;

    // set vertex buffer
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    vertexBufferView.BufferLocation = scratchBufferGPUPointer;
    vertexBufferView.SizeInBytes = bufferSpaceRequired;
    vertexBufferView.StrideInBytes = vertexSize;
    m_pCurrentCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    // invoke draw
    m_pCurrentCommandList->DrawInstanced(nVertices, 1, 0, 0);

    // restore vertex buffer
    if (m_pCurrentVertexBuffers[0] != nullptr)
    {
        vertexBufferView.BufferLocation = m_pCurrentVertexBuffers[0]->GetD3DResource()->GetGPUVirtualAddress() + m_currentVertexBufferOffsets[0];
        vertexBufferView.SizeInBytes = m_pCurrentVertexBuffers[0]->GetDesc()->Size - m_currentVertexBufferOffsets[0];
        vertexBufferView.StrideInBytes = m_currentVertexBufferStrides[0];
        m_pCurrentCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    }
    else
    {
        m_pCurrentCommandList->IASetVertexBuffers(0, 1, nullptr);
    }
}


bool D3D12GPUContext::CopyTexture(GPUTexture2D *pSourceTexture, GPUTexture2D *pDestinationTexture)
{
#if 0
    // textures have to be compatible, for now this means same texture format
    D3D12GPUTexture2D *pD3D12SourceTexture = static_cast<D3D12GPUTexture2D *>(pSourceTexture);
    D3D12GPUTexture2D *pD3D12DestinationTexture = static_cast<D3D12GPUTexture2D *>(pDestinationTexture);
    if (pD3D12SourceTexture->GetDesc()->Width != pD3D12DestinationTexture->GetDesc()->Width ||
        pD3D12SourceTexture->GetDesc()->Height != pD3D12DestinationTexture->GetDesc()->Height ||
        pD3D12SourceTexture->GetDesc()->Format != pD3D12DestinationTexture->GetDesc()->Format ||
        pD3D12SourceTexture->GetDesc()->MipLevels != pD3D12DestinationTexture->GetDesc()->MipLevels)
    {
        return false;
    }

    // copy it
    m_pCurrentCommandList->CopyResource(pD3D12DestinationTexture->GetD3DTexture(), pD3D12SourceTexture->GetD3DTexture());
    return true;
#endif
    return false;
}

bool D3D12GPUContext::CopyTextureRegion(GPUTexture2D *pSourceTexture, uint32 sourceX, uint32 sourceY, uint32 width, uint32 height, uint32 sourceMipLevel, GPUTexture2D *pDestinationTexture, uint32 destX, uint32 destY, uint32 destMipLevel)
{
#if 0
    // textures have to be compatible, for now this means same texture format
    D3D12GPUTexture2D *pD3D12SourceTexture = static_cast<D3D12GPUTexture2D *>(pSourceTexture);
    D3D12GPUTexture2D *pD3D12DestinationTexture = static_cast<D3D12GPUTexture2D *>(pDestinationTexture);
    if (pD3D12SourceTexture->GetDesc()->Format != pD3D12DestinationTexture->GetDesc()->Format ||
        pD3D12SourceTexture->GetDesc()->MipLevels != pD3D12DestinationTexture->GetDesc()->MipLevels)
    {
        return false;
    }

    // create source box, copy it
    D3D12_BOX sourceBox = { sourceX, sourceY, 0, sourceX + width, sourceY + height, 1 };
    m_pD3DContext->CopySubresourceRegion(pD3D12DestinationTexture->GetD3DTexture(), D3D12CalcSubresource(destMipLevel, 0, pD3D12DestinationTexture->GetDesc()->MipLevels), destX, destY, 0,
                                         pD3D12SourceTexture->GetD3DTexture(), D3D12CalcSubresource(sourceMipLevel, 0, pD3D12SourceTexture->GetDesc()->MipLevels), &sourceBox);

    return true;
#endif
    return false;
}

void D3D12GPUContext::BlitFrameBuffer(GPUTexture2D *pTexture, uint32 sourceX, uint32 sourceY, uint32 sourceWidth, uint32 sourceHeight, uint32 destX, uint32 destY, uint32 destWidth, uint32 destHeight, RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER resizeFilter /*= RENDERER_FRAMEBUFFER_BLIT_RESIZE_FILTER_NEAREST*/)
{
#if 0
    DebugAssert(m_nCurrentRenderTargets <= 1);

    // read source formats
    DebugAssert(static_cast<D3D12GPUTexture2D *>(pTexture)->GetDesc()->Flags & GPU_TEXTURE_FLAG_SHADER_BINDABLE);
    DXGI_FORMAT sourceTextureFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUTexture2D *>(pTexture)->GetDesc()->Format);
    ID3D12Resource *pSourceResource = static_cast<D3D12GPUTexture2D *>(pTexture)->GetD3DTexture();
    uint32 sourceTextureWidth = static_cast<D3D12GPUTexture2D *>(pTexture)->GetDesc()->Width;
    uint32 sourceTextureHeight = static_cast<D3D12GPUTexture2D *>(pTexture)->GetDesc()->Height;

    // read destination format
    DXGI_FORMAT destinationTextureFormat = DXGI_FORMAT_UNKNOWN;
    ID3D12Resource *pDestinationResource = NULL;
    uint32 destinationTextureWidth = 0, destinationTextureHeight = 0;
    if (m_nCurrentRenderTargets == 0)
    {
        destinationTextureFormat = static_cast<D3D12GPUOutputBuffer *>(m_pCurrentSwapChain)->GetBackBufferFormat();
        pDestinationResource = static_cast<D3D12GPUOutputBuffer *>(m_pCurrentSwapChain)->GetBackBufferTexture();
        destinationTextureWidth = static_cast<D3D12GPUOutputBuffer *>(m_pCurrentSwapChain)->GetWidth();
        destinationTextureHeight = static_cast<D3D12GPUOutputBuffer *>(m_pCurrentSwapChain)->GetHeight();
    }
    else
    {
        switch (m_pCurrentRenderTargetViews[0]->GetTargetTexture()->GetTextureType())
        {
        case TEXTURE_TYPE_2D:
            destinationTextureFormat = D3D12TypeConversion::PixelFormatToDXGIFormat(static_cast<D3D12GPUTexture2D *>(m_pCurrentRenderTargetViews[0]->GetTargetTexture())->GetDesc()->Format);
            pDestinationResource = static_cast<D3D12GPUTexture2D *>(m_pCurrentRenderTargetViews[0]->GetTargetTexture())->GetD3DTexture();
            destinationTextureWidth = static_cast<D3D12GPUTexture2D *>(m_pCurrentRenderTargetViews[0]->GetTargetTexture())->GetDesc()->Width;
            destinationTextureHeight = static_cast<D3D12GPUTexture2D *>(m_pCurrentRenderTargetViews[0]->GetTargetTexture())->GetDesc()->Height;
            break;

        default:
            Panic("D3D12GPUContext::BlitFrameBuffer: Destination is an unsupported texture type");
            return;
        }
    }

    // D3D12 can do a direct copy between identical types
    if (sourceTextureFormat == destinationTextureFormat && sourceWidth == destWidth && sourceHeight == destHeight)
    {
        // whole texture?
        if (sourceX == 0 && sourceY == 0 && destX == 0 && destY == 0 && sourceWidth == sourceTextureWidth && sourceHeight == sourceTextureHeight)
        {
            // use CopyResource
            m_pD3DContext->CopyResource(pDestinationResource, pSourceResource);
            return;
        }
        else
        {
            // use CopySubResourceRegion
            D3D12_BOX sourceBox = { sourceX, sourceY, 0, sourceX + sourceWidth, sourceY + sourceHeight, 1 };
            m_pD3DContext->CopySubresourceRegion(pDestinationResource, 0, destX, destY, 0, pSourceResource, 0, &sourceBox);
            return;
        }
    }

    // use shader
    g_pRenderer->BlitTextureUsingShader(this, pTexture, sourceX, sourceY, sourceWidth, sourceHeight, 0, destX, destY, destWidth, destHeight, resizeFilter, RENDERER_FRAMEBUFFER_BLIT_BLEND_MODE_NONE);
#endif
}

void D3D12GPUContext::GenerateMips(GPUTexture *pTexture)
{
    // @TODO has to be done using shaders :(
#if 0
    ID3D12ShaderResourceView *pSRV = nullptr;
    uint32 flags = 0;
    switch (pTexture->GetTextureType())
    {
    case TEXTURE_TYPE_1D:
        pSRV = static_cast<D3D12GPUTexture1D *>(pTexture)->GetD3DSRV();
        flags = static_cast<D3D12GPUTexture1D *>(pTexture)->GetDesc()->Flags;
        break;

    case TEXTURE_TYPE_1D_ARRAY:
        pSRV = static_cast<D3D12GPUTexture1DArray *>(pTexture)->GetD3DSRV();
        flags = static_cast<D3D12GPUTexture1DArray *>(pTexture)->GetDesc()->Flags;
        break;

    case TEXTURE_TYPE_2D:
        pSRV = static_cast<D3D12GPUTexture2D *>(pTexture)->GetD3DSRV();
        flags = static_cast<D3D12GPUTexture2D *>(pTexture)->GetDesc()->Flags;
        break;

    case TEXTURE_TYPE_2D_ARRAY:
        pSRV = static_cast<D3D12GPUTexture2DArray *>(pTexture)->GetD3DSRV();
        flags = static_cast<D3D12GPUTexture2DArray *>(pTexture)->GetDesc()->Flags;
        break;

    case TEXTURE_TYPE_3D:
        pSRV = static_cast<D3D12GPUTexture3D *>(pTexture)->GetD3DSRV();
        flags = static_cast<D3D12GPUTexture3D *>(pTexture)->GetDesc()->Flags;
        break;

    case TEXTURE_TYPE_CUBE:
        pSRV = static_cast<D3D12GPUTextureCube *>(pTexture)->GetD3DSRV();
        flags = static_cast<D3D12GPUTextureCube *>(pTexture)->GetDesc()->Flags;
        break;

    case TEXTURE_TYPE_CUBE_ARRAY:
        pSRV = static_cast<D3D12GPUTextureCubeArray *>(pTexture)->GetD3DSRV();
        flags = static_cast<D3D12GPUTextureCubeArray *>(pTexture)->GetDesc()->Flags;
        break;
    }

    if (pSRV == nullptr || !(flags & GPU_TEXTURE_FLAG_GENERATE_MIPS))
    {
        Log_ErrorPrintf("D3D12GPUContext::GenerateMips: Texture not created with GPU_TEXTURE_FLAG_GENERATE_MIPS.");
        return;
    }

    m_pD3DContext->GenerateMips(pSRV);
#endif
}
