#include "Renderer/PrecompiledHeader.h"
#include "Renderer/WorldRenderers/SingleShaderWorldRenderer.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/RenderQueue.h"
#include "Renderer/RenderProfiler.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderProgram.h"
#include "Engine/Material.h"
#include "Engine/ResourceManager.h"
#include "Engine/Texture.h"
#include "Engine/EngineCVars.h"

SingleShaderWorldRenderer::SingleShaderWorldRenderer(GPUContext *pGPUContext, const Options *pOptions)
    : WorldRenderer(pGPUContext, pOptions)
{
    const uint32 availableRenderPassMask = RENDER_PASS_LIGHTMAP | RENDER_PASS_EMISSIVE |
                                           RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING |
                                           RENDER_PASS_TINT;

    // initialize render queues
    m_renderQueue.SetAcceptingLights(false);
    m_renderQueue.SetAcceptingRenderPassMask(availableRenderPassMask);
    m_renderQueue.SetAcceptingOccluders(false);
    m_renderQueue.SetAcceptingDebugObjects(CVars::r_show_debug_info.GetBool());
}

SingleShaderWorldRenderer::~SingleShaderWorldRenderer()
{

}

void SingleShaderWorldRenderer::DrawWorld(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, GPURenderTargetView *pRenderTargetView, GPUDepthStencilBufferView *pDepthStencilBufferView, RenderProfiler *pRenderProfiler)
{
    // initialize and clear render targets, kick this off first
    RENDER_PROFILER_BEGIN_SECTION(pRenderProfiler, "Prepare", false);
    {
        // set render targets, for pipelining we do this before sorting
        m_pGPUContext->SetRenderTargets(1, &pRenderTargetView, pDepthStencilBufferView);
        m_pGPUContext->SetViewport(&pViewParameters->Viewport);

        // clear targets
        m_pGPUContext->ClearTargets(true, true, true, pViewParameters->FogColor);

        // set up view-dependent constants
        GPUContextConstants *pConstants = m_pGPUContext->GetConstants();
        pConstants->SetFromCamera(pViewParameters->ViewCamera, false);
        pConstants->SetWorldTime(pViewParameters->WorldTime, false);
        pConstants->CommitChanges();
    }
    RENDER_PROFILER_END_SECTION(pRenderProfiler);

    // fill render queue
    RENDER_PROFILER_BEGIN_SECTION(pRenderProfiler, "FillRenderQueue", false);
    FillRenderQueue(&pViewParameters->ViewCamera, pRenderWorld);
    RENDER_PROFILER_END_SECTION(pRenderProfiler);

    // no renderables?
    if (m_renderQueue.GetQueueSize() == 0)
        return;

    // opaque
    RENDER_PROFILER_BEGIN_SECTION(pRenderProfiler, "DrawOpqaueObjects", true);
    {
        PreDraw(pViewParameters);
        DrawOpqaueObjects(pViewParameters);
        PostDraw(pViewParameters);

        if (m_options.ShowWireframeOverlay)
            DrawWireframeOverlay(&pViewParameters->ViewCamera, &m_renderQueue.GetOpaqueRenderables());
    }
    RENDER_PROFILER_END_SECTION(pRenderProfiler);

    // postprocess
    RENDER_PROFILER_BEGIN_SECTION(pRenderProfiler, "DrawPostProcessObjects", true);
    {
        PreDraw(pViewParameters);
        DrawPostProcessObjects(pViewParameters);
        PostDraw(pViewParameters);

        if (m_options.ShowWireframeOverlay)
            DrawWireframeOverlay(&pViewParameters->ViewCamera, &m_renderQueue.GetPostProcessRenderables());
    }
    RENDER_PROFILER_END_SECTION(pRenderProfiler);

    // translucent
    RENDER_PROFILER_BEGIN_SECTION(pRenderProfiler, "DrawTranslucentObjects", true);
    {
        PreDraw(pViewParameters);
        DrawTranslucentObjects(pViewParameters);
        PostDraw(pViewParameters);

        if (m_options.ShowWireframeOverlay)
            DrawWireframeOverlay(&pViewParameters->ViewCamera, &m_renderQueue.GetTranslucentRenderables());
    }
    RENDER_PROFILER_END_SECTION(pRenderProfiler);

    // debug info
    if (m_options.ShowDebugInfo && m_pGUIContext != nullptr)
    {
        RENDER_PROFILER_BEGIN_SECTION(pRenderProfiler, "DrawDebugInfo", false);
        {
            DrawDebugInfo(&pViewParameters->ViewCamera, pRenderProfiler);
        }
        RENDER_PROFILER_END_SECTION(pRenderProfiler);
    }

    // clear targets and shaders
    m_pGPUContext->ClearState(true, true, true, true);
    OnFrameComplete();
}

void SingleShaderWorldRenderer::DrawOpqaueObjects(const ViewParameters *pViewParameters)
{
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_renderQueue.GetOpaqueRenderables().GetBasePointer();
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_renderQueue.GetOpaqueRenderables().GetBasePointer() + m_renderQueue.GetOpaqueRenderables().GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        if (pQueueEntry->RenderPassMask & (RENDER_PASS_EMISSIVE | RENDER_PASS_LIGHTMAP | RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING))
            DrawQueueEntry(pViewParameters, pQueueEntry);
    }
}

void SingleShaderWorldRenderer::DrawPostProcessObjects(const ViewParameters *pViewParameters)
{
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_renderQueue.GetPostProcessRenderables().GetBasePointer();
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_renderQueue.GetPostProcessRenderables().GetBasePointer() + m_renderQueue.GetPostProcessRenderables().GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        if (pQueueEntry->RenderPassMask & (RENDER_PASS_EMISSIVE | RENDER_PASS_LIGHTMAP | RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING))
            DrawQueueEntry(pViewParameters, pQueueEntry);
    }
}

void SingleShaderWorldRenderer::DrawTranslucentObjects(const ViewParameters *pViewParameters)
{
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_renderQueue.GetTranslucentRenderables().GetBasePointer();
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_renderQueue.GetTranslucentRenderables().GetBasePointer() + m_renderQueue.GetTranslucentRenderables().GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        if (pQueueEntry->RenderPassMask & (RENDER_PASS_EMISSIVE | RENDER_PASS_LIGHTMAP | RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING))
            DrawQueueEntry(pViewParameters, pQueueEntry);
    }
}

void SingleShaderWorldRenderer::SetCommonShaderProgramParameters(const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, ShaderProgram *pShaderProgram)
{
    pQueueEntry->pMaterial->BindDeviceResources(m_pGPUContext, pShaderProgram);
    pQueueEntry->pRenderProxy->SetupForDraw(&pViewParameters->ViewCamera, pQueueEntry, m_pGPUContext, pShaderProgram);

    // post process material?
    const MaterialShader *pMaterialShader = pQueueEntry->pMaterial->GetShader();
    if (pMaterialShader->GetRenderMode() == MATERIAL_RENDER_MODE_POST_PROCESS)
    {
        // set post process textures
        const uint32 BASE_INDEX = pMaterialShader->GetTextureParameterCount();
        pShaderProgram->SetMaterialParameterTexture(m_pGPUContext, BASE_INDEX + 0, g_pResourceManager->GetDefaultTexture2D()->GetGPUTexture(), nullptr);       // fixme
        pShaderProgram->SetMaterialParameterTexture(m_pGPUContext, BASE_INDEX + 1, g_pResourceManager->GetDefaultTexture2D()->GetGPUTexture(), nullptr);       // fixme
    }
}

void SingleShaderWorldRenderer::PreDraw(const ViewParameters *pViewParameters)
{

}

void SingleShaderWorldRenderer::DrawQueueEntry(const ViewParameters *pViewParameters, RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry)
{

}

void SingleShaderWorldRenderer::PostDraw(const ViewParameters *pViewParameters)
{

}
