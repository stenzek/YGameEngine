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
{
    // add references
    m_pDevice->SetGPUContext(this);
    m_pDevice->AddRef();

    // null memory
    Y_memzero(&m_currentViewport, sizeof(m_currentViewport));
    Y_memzero(&m_scissorRect, sizeof(m_scissorRect));
    m_currentTopology = DRAW_TOPOLOGY_UNDEFINED;

    // null current states
    Y_memzero(m_pCurrentVertexBuffers, sizeof(m_pCurrentVertexBuffers));
    Y_memzero(m_currentVertexBufferOffsets, sizeof(m_currentVertexBufferOffsets));
    Y_memzero(m_currentVertexBufferStrides, sizeof(m_currentVertexBufferStrides));
    m_currentVertexBufferBindCount = 0;

    m_pCurrentIndexBuffer = NULL;
    m_currentIndexFormat = GPU_INDEX_FORMAT_COUNT;
    m_currentIndexBufferOffset = 0;

    m_pCurrentShaderProgram = NULL;
    Y_memzero(m_shaderStates, sizeof(m_shaderStates));
    for (uint32 i = 0; i < SHADER_PROGRAM_STAGE_COUNT; i++)
    {
        ShaderStageState *state = &m_shaderStates[i];
        state->ConstantBufferDirtyLowerBounds = state->ConstantBufferDirtyUpperBounds = -1;
        state->ResourceDirtyLowerBounds = state->ResourceDirtyUpperBounds = -1;
        state->SamplerDirtyLowerBounds = state->SamplerDirtyUpperBounds = -1;
        state->UAVDirtyLowerBounds = state->UAVDirtyUpperBounds = -1;
    }

    m_pCurrentRasterizerState = NULL;
    m_pCurrentDepthStencilState = NULL;
    m_currentDepthStencilRef = 0;
    m_pCurrentBlendState = NULL;
    m_currentBlendStateBlendFactors.SetZero();
    m_pipelineChanged = false;

    m_pCurrentSwapChain = NULL;

    Y_memzero(m_pCurrentRenderTargetViews, sizeof(m_pCurrentRenderTargetViews));
    m_pCurrentDepthBufferView = nullptr;
    m_nCurrentRenderTargets = 0;

    m_pCurrentPredicate = nullptr;
    //m_pCurrentPredicateD3D = nullptr;
    m_predicateBypassCount = 0;
}

D3D12GPUContext::~D3D12GPUContext()
{
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

    // release our references to states and targets. d3d references will die when the device goes down.
    for (uint32 i = 0; i < countof(m_pCurrentVertexBuffers); i++) {
        SAFE_RELEASE(m_pCurrentVertexBuffers[i]);
    }
    SAFE_RELEASE(m_pCurrentIndexBuffer);

    SAFE_RELEASE(m_pCurrentShaderProgram);
    for (uint32 stage = 0; stage < SHADER_PROGRAM_STAGE_COUNT; stage++)
    {
        ShaderStageState &state = m_shaderStates[stage];

        for (uint32 i = 0; i < state.ConstantBufferBindCount; i++) {
            SAFE_RELEASE(state.ConstantBuffers[i]);
        }

        for (uint32 i = 0; i < state.ResourceBindCount; i++) {
            SAFE_RELEASE(state.Resources[i]);
        }

        for (uint32 i = 0; i < state.SamplerBindCount; i++) {
            SAFE_RELEASE(state.Samplers[i]);
        }

        for (uint32 i = 0; i < state.UAVBindCount; i++) {
            SAFE_RELEASE(state.UAVs[i]);
        }
    }

    for (uint32 i = 0; i < m_constantBuffers.GetSize(); i++)
    {
        ConstantBuffer *constantBuffer = &m_constantBuffers[i];
        if (constantBuffer->pGPUBuffer != nullptr)
            constantBuffer->pGPUBuffer->Release();

        delete[] constantBuffer->pLocalMemory;
    }

    SAFE_RELEASE(m_pCurrentRasterizerState);
    SAFE_RELEASE(m_pCurrentDepthStencilState);
    SAFE_RELEASE(m_pCurrentBlendState);

    for (uint32 i = 0; i < countof(m_pCurrentRenderTargetViews); i++) {
        SAFE_RELEASE(m_pCurrentRenderTargetViews[i]);
    }
    SAFE_RELEASE(m_pCurrentDepthBufferView);

#endif

    delete m_pConstants;

    //SAFE_RELEASE(m_pCurrentSwapChain);
    m_pDevice->SetGPUContext(nullptr);
    m_pDevice->Release();
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
    HRESULT hResult;

    // allocate constants
    m_pConstants = new GPUContextConstants(this);
    CreateConstantBuffers();

    // allocate command allocator
    hResult = m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void **)&m_pCommandAllocator);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommandAllocator failed with hResult %08X", hResult);
        return false;
    }

    // allocate command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE, 0 };
    hResult = m_pD3DDevice->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), (void **)&m_pCommandQueue);
    if (FAILED(hResult))
    {
        Log_ErrorPrintf("CreateCommandQueue failed with hResult %08X", hResult);
        return false;
    }

    // create queued frame data
    if (!CreateQueuedFrameData())
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

bool D3D12GPUContext::CreateQueuedFrameData()
{
    HRESULT hResult;
    uint32 frameLatency = m_pBackend->GetFrameLatency();
    m_queuedFrameData.Resize(frameLatency);
    m_queuedFrameData.ZeroContents();

    for (uint32 i = 0; i < frameLatency; i++)
    {
        QueuedFrameData &qfd = m_queuedFrameData[i];
        hResult = m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator, nullptr, __uuidof(ID3D12CommandList), (void **)&qfd.pCommandList);
        if (FAILED(hResult))
        {
            Log_ErrorPrintf("CreateCommandList failed with hResult %08X", hResult);
            return false;
        }

        qfd.pScratchBuffer = D3D12ScratchBuffer::Create(m_pD3DDevice, 16 * 1024 * 1024);
        if (qfd.pScratchBuffer == nullptr)
            return false;

        qfd.FenceValue = 0;
        qfd.FrameNumber = 0;
    }

    // set current frame
    m_currentQueuedFrameIndex = 0;
    m_pCurrentScratchBuffer = m_queuedFrameData[m_currentQueuedFrameIndex].pScratchBuffer;
    m_pCurrentCommandList = m_queuedFrameData[m_currentQueuedFrameIndex].pCommandList;
    return true;
}

void D3D12GPUContext::MoveToNextFrameList()
{
    m_currentQueuedFrameIndex++;
    m_currentQueuedFrameIndex %= D3D12RenderBackend::GetInstance()->GetFrameLatency();

    // current frame is queued?
    QueuedFrameData &qfd = m_queuedFrameData[m_currentQueuedFrameIndex];
    if (qfd.FrameNumber != 0)
    {
        // @TODO wait on fence
        qfd.pScratchBuffer->Reset();
        qfd.FrameNumber = 0;
        qfd.FenceValue = 0;
    }

    // update pointers
    m_pCurrentCommandList = qfd.pCommandList;
    m_pCurrentScratchBuffer = qfd.pScratchBuffer;
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
        m_queuedFrameData[m_currentQueuedFrameIndex].pScratchBuffer = pNewScratchBuffer;

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

void D3D12GPUContext::BeginFrame()
{
    MoveToNextFrameList();
    g_pRenderer->BeginFrame();
}

void D3D12GPUContext::ClearState(bool clearShaders /* = true */, bool clearBuffers /* = true */, bool clearStates /* = true */, bool clearRenderTargets /* = true */)
{
    if (clearShaders)
    {
        SetShaderProgram(nullptr);

#if 0
        for (uint32 stage = 0; stage < SHADER_PROGRAM_STAGE_COUNT; stage++)
        {
            ShaderStageState &state = m_shaderStates[stage];

            if (state.ConstantBufferBindCount > 0)
            {
                for (uint32 i = 0; i < state.ConstantBufferBindCount; i++) {
                    SAFE_RELEASE(state.ConstantBuffers[i]);
                }
                state.ConstantBufferDirtyLowerBounds = 0;
                state.ConstantBufferDirtyUpperBounds = state.ConstantBufferBindCount - 1;
            }

            if (state.ResourceBindCount > 0)
            {
                for (uint32 i = 0; i < state.ResourceBindCount; i++) {
                    SAFE_RELEASE(state.Resources[i]);
                }
                state.ResourceDirtyLowerBounds = 0;
                state.ResourceDirtyUpperBounds = state.ResourceBindCount - 1;
            }

            if (state.SamplerBindCount > 0)
            {
                for (uint32 i = 0; i < state.SamplerBindCount; i++) {
                    SAFE_RELEASE(state.Samplers[i]);
                }
                state.SamplerDirtyLowerBounds = 0;
                state.SamplerDirtyUpperBounds = state.SamplerBindCount - 1;
            }

            if (state.UAVBindCount > 0)
            {
                for (uint32 i = 0; i < state.UAVBindCount; i++) {
                    SAFE_RELEASE(state.UAVs[i]);
                }
                state.UAVDirtyLowerBounds = 0;
                state.UAVDirtyUpperBounds = state.UAVBindCount - 1;
            }
        }
#endif
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
#if 0
    uint32 clearFlags = 0;
    if (clearDepth)
        clearFlags |= D3D12_CLEAR_DEPTH;
    if (clearStencil)
        clearFlags |= D3D12_CLEAR_STENCIL;

    if (m_nCurrentRenderTargets == 0 && m_pCurrentDepthBufferView == nullptr)
    {
        // on swapchain
        if (m_pCurrentSwapChain != nullptr)
        {
            ID3D12RenderTargetView *pRTV = m_pCurrentSwapChain->GetRenderTargetView();
            ID3D12DepthStencilView *pDSV = m_pCurrentSwapChain->GetDepthStencilView();

            if (clearColor)
                m_pD3DContext->ClearRenderTargetView(pRTV, clearColorValue);

            if (clearFlags != 0 && pDSV != nullptr)
                m_pD3DContext->ClearDepthStencilView(pDSV, clearFlags, clearDepthValue, clearStencilValue);
        }
    }
    else
    {
        if (clearColor)
        {
            for (uint32 i = 0; i < m_nCurrentRenderTargets; i++)
            {
                if (m_pCurrentRenderTargetViews[i] != nullptr)
                    m_pD3DContext->ClearRenderTargetView(m_pCurrentRenderTargetViews[i]->GetD3DRTV(), clearColorValue);
            }
        }

        if (clearFlags != 0 && m_pCurrentDepthBufferView != nullptr)
            m_pD3DContext->ClearDepthStencilView(m_pCurrentDepthBufferView->GetD3DDSV(), clearFlags, clearDepthValue, clearStencilValue);
    }
#endif
}

void D3D12GPUContext::DiscardTargets(bool discardColor /* = true */, bool discardDepth /* = true */, bool discardStencil /* = true */)
{
#if 0
    // only supported on 11.1+
    if (m_pD3DContext1 == nullptr)
        return;

    if (m_nCurrentRenderTargets == 0 && m_pCurrentDepthBufferView == nullptr)
    {
        // on swapchain
        if (m_pCurrentSwapChain != nullptr)
        {
            if (discardColor)
                m_pD3DContext1->DiscardView(m_pCurrentSwapChain->GetRenderTargetView());
            if (discardDepth && discardStencil && m_pCurrentSwapChain->GetDepthStencilView() != nullptr)
                m_pD3DContext1->DiscardView(m_pCurrentSwapChain->GetDepthStencilView());
        }
    }
    else
    {
        if (discardColor)
        {
            for (uint32 i = 0; i < m_nCurrentRenderTargets; i++)
            {
                if (m_pCurrentRenderTargetViews[i] != nullptr)
                    m_pD3DContext1->DiscardView(m_pCurrentRenderTargetViews[i]->GetD3DRTV());
            }
        }

        if (discardDepth && discardStencil && m_pCurrentDepthBufferView != nullptr)
            m_pD3DContext1->DiscardView(m_pCurrentDepthBufferView->GetD3DDSV());
    }
#endif
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
    m_pCurrentSwapChain->GetDXGISwapChain()->Present((presentBehaviour == GPU_PRESENT_BEHAVIOUR_WAIT_FOR_VBLANK) ? 1 : 0, 0);
    // @TODO exec command list
    // @TODO set up fences
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
        if (m_nCurrentRenderTargets == 0 && pDepthBufferView == nullptr)
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
        D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,     // DRAW_TOPOLOGY_UNDEFINED
        D3D_PRIMITIVE_TOPOLOGY_POINTLIST,     // DRAW_TOPOLOGY_POINTS
        D3D_PRIMITIVE_TOPOLOGY_LINELIST,      // DRAW_TOPOLOGY_LINE_LIST
        D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,     // DRAW_TOPOLOGY_LINE_STRIP
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,  // DRAW_TOPOLOGY_TRIANGLE_LIST
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, // DRAW_TOPOLOGY_TRIANGLE_STRIP
    };

    if (topology == m_currentTopology)
        return;

    DebugAssert(topology < DRAW_TOPOLOGY_COUNT);
    m_pCurrentCommandList->IASetPrimitiveTopology(D3D12Topologies[topology]);
    m_currentTopology = topology;
}

uint32 D3D12GPUContext::GetVertexBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer **ppVertexBuffers, uint32 *pVertexBufferOffsets, uint32 *pVertexBufferStrides)
{
#if 0
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
#endif
    return 0;
}

void D3D12GPUContext::SetVertexBuffers(uint32 firstBuffer, uint32 nBuffers, GPUBuffer *const *ppVertexBuffers, const uint32 *pVertexBufferOffsets, const uint32 *pVertexBufferStrides)
{
#if 0
    ID3D12Buffer *pD3DBuffers[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    UINT D3DBufferOffsets[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    UINT D3DBufferStrides[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];

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

            pD3DBuffers[i] = pD3D12VertexBuffer->GetD3DBuffer();
            D3DBufferOffsets[i] = pVertexBufferOffsets[i];
            D3DBufferStrides[i] = pVertexBufferStrides[i];
        }
        else
        {
            m_currentVertexBufferOffsets[bufferIndex] = 0;
            m_currentVertexBufferStrides[bufferIndex] = 0;

            pD3DBuffers[i] = nullptr;
            D3DBufferOffsets[i] = 0;
            D3DBufferStrides[i] = 0;
        }
    }

    // todo: pending set
    m_pD3DContext->IASetVertexBuffers(firstBuffer, nBuffers, pD3DBuffers, D3DBufferStrides, D3DBufferOffsets);

    // update new bind count
    uint32 bindCount = 0;
    uint32 searchCount = Max((firstBuffer + nBuffers), m_currentVertexBufferBindCount);
    for (uint32 i = 0; i < searchCount; i++)
    {
        if (m_pCurrentVertexBuffers[i] != nullptr)
            bindCount = i + 1;
    }
    m_currentVertexBufferBindCount = bindCount;
#endif
}

void D3D12GPUContext::SetVertexBuffer(uint32 bufferIndex, GPUBuffer *pVertexBuffer, uint32 offset, uint32 stride)
{
#if 0
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

    // todo: pending set
    ID3D12Buffer *pD3DBuffer = (pVertexBuffer != nullptr) ? static_cast<D3D12GPUBuffer *>(pVertexBuffer)->GetD3DBuffer() : nullptr;
    UINT D3DOffset = (pVertexBuffer != nullptr) ? offset : 0;
    UINT D3DStride = (pVertexBuffer != nullptr) ? stride : 0;
    m_pD3DContext->IASetVertexBuffers(bufferIndex, 1, &pD3DBuffer, &D3DStride, &D3DOffset);

    // update new bind count
    uint32 bindCount = 0;
    uint32 searchCount = Max((bufferIndex + 1), m_currentVertexBufferBindCount);
    for (uint32 i = 0; i < searchCount; i++)
    {
        if (m_pCurrentVertexBuffers[i] != nullptr)
            bindCount = i + 1;
    }
    m_currentVertexBufferBindCount = bindCount;
#endif
}

void D3D12GPUContext::GetIndexBuffer(GPUBuffer **ppBuffer, GPU_INDEX_FORMAT *pFormat, uint32 *pOffset)
{
#if 0
    *ppBuffer = m_pCurrentIndexBuffer;
    *pFormat = m_currentIndexFormat;
    *pOffset = m_currentIndexBufferOffset;
#endif
}

void D3D12GPUContext::SetIndexBuffer(GPUBuffer *pBuffer, GPU_INDEX_FORMAT format, uint32 offset)
{
#if 0
    if (m_pCurrentIndexBuffer == pBuffer && m_currentIndexFormat == format && m_currentIndexBufferOffset == offset)
        return;

    if (m_pCurrentIndexBuffer != pBuffer)
    {
        if (m_pCurrentIndexBuffer != NULL)
            m_pCurrentIndexBuffer->Release();

        if ((m_pCurrentIndexBuffer = static_cast<D3D12GPUBuffer *>(pBuffer)) != NULL)
            m_pCurrentIndexBuffer->AddRef();
    }

    m_currentIndexFormat = format;
    m_currentIndexBufferOffset = offset;

    if (m_pCurrentIndexBuffer != NULL)
        m_pD3DContext->IASetIndexBuffer(m_pCurrentIndexBuffer->GetD3DBuffer(), (format == GPU_INDEX_FORMAT_UINT16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, m_currentIndexBufferOffset);
    else
        m_pD3DContext->IASetIndexBuffer(NULL, DXGI_FORMAT_R16_UINT, 0);
#endif
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

void D3D12GPUContext::SetShaderConstantBuffers(SHADER_PROGRAM_STAGE stage, uint32 index, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
    ShaderStageState *state = &m_shaderStates[stage];
    if (state->ConstantBuffers[index].ptr == gpuHandle.ptr)
        return;

    state->ConstantBuffers[index].ptr = gpuHandle.ptr;
    if (state->ConstantBufferDirtyLowerBounds < 0)
    {
        state->ConstantBufferDirtyLowerBounds = index;
        state->ConstantBufferDirtyUpperBounds = index;
    }
    else
    {
        state->ConstantBufferDirtyLowerBounds = Min(state->ConstantBufferDirtyLowerBounds, (int32)index);
        state->ConstantBufferDirtyUpperBounds = Max(state->ConstantBufferDirtyUpperBounds, (int32)index);
    }
}

void D3D12GPUContext::SetShaderResources(SHADER_PROGRAM_STAGE stage, uint32 index, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
    ShaderStageState *state = &m_shaderStates[stage];
    if (state->Resources[index].ptr == gpuHandle.ptr)
        return;

    state->Resources[index].ptr = gpuHandle.ptr;
    if (state->ResourceDirtyLowerBounds < 0)
    {
        state->ResourceDirtyLowerBounds = index;
        state->ResourceDirtyUpperBounds = index;
    }
    else
    {
        state->ResourceDirtyLowerBounds = Min(state->ResourceDirtyLowerBounds, (int32)index);
        state->ResourceDirtyUpperBounds = Max(state->ResourceDirtyUpperBounds, (int32)index);
    }
}

void D3D12GPUContext::SetShaderSamplers(SHADER_PROGRAM_STAGE stage, uint32 index, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
    ShaderStageState *state = &m_shaderStates[stage];
    if (state->Samplers[index].ptr == gpuHandle.ptr)
        return;

    state->Samplers[index].ptr = gpuHandle.ptr;
    if (state->SamplerDirtyLowerBounds < 0)
    {
        state->SamplerDirtyLowerBounds = index;
        state->SamplerDirtyUpperBounds = index;
    }
    else
    {
        state->SamplerDirtyLowerBounds = Min(state->SamplerDirtyLowerBounds, (int32)index);
        state->SamplerDirtyUpperBounds = Max(state->SamplerDirtyUpperBounds, (int32)index);
    }
}

void D3D12GPUContext::SetShaderUAVs(SHADER_PROGRAM_STAGE stage, uint32 index, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
    ShaderStageState *state = &m_shaderStates[stage];
    if (state->UAVs[index].ptr == gpuHandle.ptr)
        return;

    state->UAVs[index].ptr = gpuHandle.ptr;
    if (state->UAVDirtyLowerBounds < 0)
    {
        state->UAVDirtyLowerBounds = index;
        state->UAVDirtyUpperBounds = index;
    }
    else
    {
        state->UAVDirtyLowerBounds = Min(state->UAVDirtyLowerBounds, (int32)index);
        state->UAVDirtyUpperBounds = Max(state->UAVDirtyUpperBounds, (int32)index);
    }
}

void D3D12GPUContext::SynchronizeRenderTargetsAndUAVs()
{
#if 0
    ID3D12RenderTargetView *renderTargetViews[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
    ID3D12DepthStencilView *depthStencilView = nullptr;
    uint32 renderTargetCount = 0;

    // get render target views
    if (m_nCurrentRenderTargets == 0 && m_pCurrentDepthBufferView == nullptr)
    {
        // using swap chain
        if (m_pCurrentSwapChain != nullptr)
        {
            renderTargetViews[0] = m_pCurrentSwapChain->GetRenderTargetView();
            depthStencilView = m_pCurrentSwapChain->GetDepthStencilView();
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
                if ((renderTargetViews[renderTargetIndex] = m_pCurrentRenderTargetViews[renderTargetIndex]->GetD3DRTV()) != nullptr)
                    renderTargetCount = renderTargetIndex + 1;
            }
        }

        // get depth stencil view
        if (m_pCurrentDepthBufferView != nullptr)
            depthStencilView = m_pCurrentDepthBufferView->GetD3DDSV();
    }

    // get unordered access views
    ID3D12UnorderedAccessView *unorderedAccessViews[D3D12_PS_CS_UAV_REGISTER_COUNT];
    UINT unorderedAccessViewInitialCounts[D3D12_PS_CS_UAV_REGISTER_COUNT];
    uint32 unorderedAccessCount = 0;
    if (m_shaderStates[SHADER_PROGRAM_STAGE_PIXEL_SHADER].UAVBindCount > 0)
    {
        const ShaderStageState *state = &m_shaderStates[SHADER_PROGRAM_STAGE_PIXEL_SHADER];
        for (uint32 i = 0; i < state->UAVBindCount; i++)
            unorderedAccessViewInitialCounts[i] = 0;

        unorderedAccessCount = state->UAVBindCount;
    }

    // call through
    if (unorderedAccessCount > 0)
        m_pD3DContext->OMSetRenderTargetsAndUnorderedAccessViews(renderTargetCount, (renderTargetCount > 0) ? renderTargetViews : nullptr, depthStencilView, 0, unorderedAccessCount, unorderedAccessViews, unorderedAccessViewInitialCounts);
    else
        m_pD3DContext->OMSetRenderTargets(renderTargetCount, (renderTargetCount > 0) ? renderTargetViews : nullptr, depthStencilView);
#endif
}

bool D3D12GPUContext::UpdatePipelineState()
{
    // new pipeline state required?
    if (m_pipelineChanged)
    {
        if (m_pCurrentShaderProgram == nullptr)
            return false;

        // @TODO fill pipeline state key
        D3D12GPUShaderProgram::PipelineStateKey key;
        if (!m_pCurrentShaderProgram->Switch(m_pCurrentCommandList, &key))
            return false;

        // up-to-date
        m_pipelineChanged = false;
    }

#if 0
    // switch shader

    // update states
    for (uint32 stage = 0; stage < SHADER_PROGRAM_STAGE_COUNT; stage++)
    {
        ShaderStageState *state = &m_shaderStates[stage];

        // Constant buffers
        if (state->ConstantBufferDirtyLowerBounds >= 0)
        {
            int32 firstConstantBuffer = state->ConstantBufferDirtyLowerBounds;
            int32 setCount = state->ConstantBufferDirtyUpperBounds - state->ConstantBufferDirtyLowerBounds + 1;

            // find the new bind count
            uint32 searchMax = Max((uint32)state->ConstantBufferDirtyUpperBounds + 1, state->ConstantBufferBindCount);
            uint32 bindCount = 0;
            for (uint32 i = 0; i < searchMax; i++)
            {
                if (state->ConstantBuffers[i] != nullptr)
                    bindCount = i + 1;
            }
            state->ConstantBufferBindCount = bindCount;
            state->ConstantBufferDirtyLowerBounds = state->ConstantBufferDirtyUpperBounds = -1;

            // update d3d
            switch (stage)
            {
            case SHADER_PROGRAM_STAGE_VERTEX_SHADER:    m_pD3DContext->VSSetConstantBuffers(firstConstantBuffer, setCount, state->ConstantBuffers + firstConstantBuffer);   break;
            case SHADER_PROGRAM_STAGE_HULL_SHADER:      m_pD3DContext->HSSetConstantBuffers(firstConstantBuffer, setCount, state->ConstantBuffers + firstConstantBuffer);   break;
            case SHADER_PROGRAM_STAGE_DOMAIN_SHADER:    m_pD3DContext->DSSetConstantBuffers(firstConstantBuffer, setCount, state->ConstantBuffers + firstConstantBuffer);   break;
            case SHADER_PROGRAM_STAGE_GEOMETRY_SHADER:  m_pD3DContext->GSSetConstantBuffers(firstConstantBuffer, setCount, state->ConstantBuffers + firstConstantBuffer);   break;
            case SHADER_PROGRAM_STAGE_PIXEL_SHADER:     m_pD3DContext->PSSetConstantBuffers(firstConstantBuffer, setCount, state->ConstantBuffers + firstConstantBuffer);   break;
            case SHADER_PROGRAM_STAGE_COMPUTE_SHADER:   m_pD3DContext->CSSetConstantBuffers(firstConstantBuffer, setCount, state->ConstantBuffers + firstConstantBuffer);   break;
            }
        }

        // Resources
        if (state->ResourceDirtyLowerBounds >= 0)
        {
            int32 firstResource = state->ResourceDirtyLowerBounds;
            int32 setCount = state->ResourceDirtyUpperBounds - state->ResourceDirtyLowerBounds + 1;

            // find the new bind count
            uint32 searchMax = Max((uint32)state->ResourceDirtyUpperBounds + 1, state->ResourceBindCount);
            uint32 bindCount = 0;
            for (uint32 i = 0; i < searchMax; i++)
            {
                if (state->Resources[i] != nullptr)
                    bindCount = i + 1;
            }
            state->ResourceBindCount = bindCount;
            state->ResourceDirtyLowerBounds = state->ResourceDirtyUpperBounds = -1;

            // update d3d
            switch (stage)
            {
            case SHADER_PROGRAM_STAGE_VERTEX_SHADER:    m_pD3DContext->VSSetShaderResources(firstResource, setCount, state->Resources + firstResource);     break;
            case SHADER_PROGRAM_STAGE_HULL_SHADER:      m_pD3DContext->HSSetShaderResources(firstResource, setCount, state->Resources + firstResource);     break;
            case SHADER_PROGRAM_STAGE_DOMAIN_SHADER:    m_pD3DContext->DSSetShaderResources(firstResource, setCount, state->Resources + firstResource);     break;
            case SHADER_PROGRAM_STAGE_GEOMETRY_SHADER:  m_pD3DContext->GSSetShaderResources(firstResource, setCount, state->Resources + firstResource);     break;
            case SHADER_PROGRAM_STAGE_PIXEL_SHADER:     m_pD3DContext->PSSetShaderResources(firstResource, setCount, state->Resources + firstResource);     break;
            case SHADER_PROGRAM_STAGE_COMPUTE_SHADER:   m_pD3DContext->CSSetShaderResources(firstResource, setCount, state->Resources + firstResource);     break;
            }
        }

        // Samplers
        if (state->SamplerDirtyLowerBounds >= 0)
        {
            int32 firstSampler = state->SamplerDirtyLowerBounds;
            int32 setCount = state->SamplerDirtyUpperBounds - state->SamplerDirtyLowerBounds + 1;

            // find the new bind count
            uint32 searchMax = Max((uint32)state->SamplerDirtyUpperBounds + 1, state->SamplerBindCount);
            uint32 bindCount = 0;
            for (uint32 i = 0; i < searchMax; i++)
            {
                if (state->Samplers[i] != nullptr)
                    bindCount = i + 1;
            }
            state->SamplerBindCount = bindCount;
            state->SamplerDirtyLowerBounds = state->SamplerDirtyUpperBounds = -1;

            // update d3d
            switch (stage)
            {
            case SHADER_PROGRAM_STAGE_VERTEX_SHADER:    m_pD3DContext->VSSetSamplers(firstSampler, setCount, state->Samplers + firstSampler);       break;
            case SHADER_PROGRAM_STAGE_HULL_SHADER:      m_pD3DContext->HSSetSamplers(firstSampler, setCount, state->Samplers + firstSampler);       break;
            case SHADER_PROGRAM_STAGE_DOMAIN_SHADER:    m_pD3DContext->DSSetSamplers(firstSampler, setCount, state->Samplers + firstSampler);       break;
            case SHADER_PROGRAM_STAGE_GEOMETRY_SHADER:  m_pD3DContext->GSSetSamplers(firstSampler, setCount, state->Samplers + firstSampler);       break;
            case SHADER_PROGRAM_STAGE_PIXEL_SHADER:     m_pD3DContext->PSSetSamplers(firstSampler, setCount, state->Samplers + firstSampler);       break;
            case SHADER_PROGRAM_STAGE_COMPUTE_SHADER:   m_pD3DContext->CSSetSamplers(firstSampler, setCount, state->Samplers + firstSampler);       break;
            }
        }

        // UAVs
        if (state->UAVDirtyLowerBounds >= 0)
        {
            int32 firstUAV = state->UAVDirtyLowerBounds;
            int32 setCount = state->UAVDirtyUpperBounds - state->UAVDirtyLowerBounds + 1;

            // find the new bind count
            uint32 searchMax = Max((uint32)state->UAVDirtyUpperBounds + 1, state->UAVBindCount);
            uint32 bindCount = 0;
            for (uint32 i = 0; i < searchMax; i++)
            {
                if (state->UAVs[i] != nullptr)
                    bindCount = i + 1;
            }
            state->UAVBindCount = bindCount;
            state->UAVDirtyLowerBounds = state->UAVDirtyUpperBounds = -1;

            // update d3d
            switch (stage)
            {
            case SHADER_PROGRAM_STAGE_PIXEL_SHADER:     SynchronizeRenderTargetsAndUAVs();                                                              break;
            case SHADER_PROGRAM_STAGE_COMPUTE_SHADER:   m_pD3DContext->CSSetUnorderedAccessViews(firstUAV, setCount, state->UAVs + firstUAV, nullptr);  break;
            }
        }
    }

    // update the local constant buffer for the active program
    if (m_pCurrentShaderProgram != nullptr)
        m_pCurrentShaderProgram->CommitLocalConstantBuffers(this);
#endif

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
    // can use scratch buffer directly. NOTE: needs a resource barrier
#if 0
    const byte *pVerticesPtr = reinterpret_cast<const byte *>(pVertices);
    uint32 remainingVertices = nVertices;

//     // we need to disable any currently bound vertex array, as we handle it manually
//     if (m_currentVertexBufferBindCount > 0)
//     {
//         ID3D12Buffer *pNullD3DBuffers[GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS] = { NULL };
//         UINT nullD3DOffsetStrides[GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS] = { 0 };
//         m_pD3DContext->IASetVertexBuffers(0, m_currentVertexBufferBindCount, pNullD3DBuffers, nullD3DOffsetStrides, nullD3DOffsetStrides);
//     }

    // setup draw
    DebugAssert(m_pCurrentShaderProgram != nullptr);
    SynchronizeShaderStates();

    // while we still have vertices left to draw...
    while (remainingVertices > 0)
    {
        // map buffer
        void *pMappedPointer;
        uint32 bufferOffset;
        uint32 mappedVertices;
        if (!GetTempVertexBufferPointer(vertexSize, remainingVertices, &pMappedPointer, &bufferOffset, &mappedVertices))
            return;

        // fill buffer
        Y_memcpy(pMappedPointer, pVerticesPtr, vertexSize * mappedVertices);

        // unmap buffer
        m_pD3DContext->Unmap(m_pUserVertexBuffer, 0);

        // update vb
        m_pD3DContext->IASetVertexBuffers(0, 1, &m_pUserVertexBuffer, &vertexSize, &bufferOffset);

        // draw it
        m_pD3DContext->Draw(mappedVertices, 0);
        //m_drawCallCounter++;

        // reduce vertices
        pVerticesPtr += mappedVertices * vertexSize;
        remainingVertices -= mappedVertices;
    }

    // restore state
    if (m_currentVertexBufferBindCount > 0)
    {
//         // re-bind old vertex buffers
//         ID3D12Buffer *pD3DBuffers[GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS];
//         UINT D3DOffsets[GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS];
//         UINT D3DStrides[GPU_MAX_SIMULTANEOUS_VERTEX_BUFFERS];
//         for (uint32 i = 0; i < m_currentVertexBufferBindCount; i++)
//         {
//             pD3DBuffers[i] = (m_pCurrentVertexBuffers[i] != NULL) ? m_pCurrentVertexBuffers[i]->GetD3DBuffer() : NULL;
//             D3DOffsets[i] = m_currentVertexBufferOffsets[i];
//             D3DStrides[i] = m_currentVertexBufferStrides[i];
//         }
//         m_pD3DContext->IASetVertexBuffers(0, m_currentVertexBufferBindCount, pD3DBuffers, D3DOffsets, D3DStrides);

        ID3D12Buffer *pD3DBuffer = (m_pCurrentVertexBuffers[0] != nullptr) ? m_pCurrentVertexBuffers[0]->GetD3DBuffer() : nullptr;
        UINT D3DOffset = m_currentVertexBufferOffsets[0];
        UINT D3DStride = m_currentVertexBufferStrides[0];
        m_pD3DContext->IASetVertexBuffers(0, 1, &pD3DBuffer, &D3DStride, &D3DOffset);
    }
    else
    {
        // unbind the temp one
        ID3D12Buffer *pNullD3DBuffer = NULL;
        UINT nullD3DOffsetStride = 0;
        m_pD3DContext->IASetVertexBuffers(0, 1, &pNullD3DBuffer, &nullD3DOffsetStride, &nullD3DOffsetStride);
    }
#endif
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
