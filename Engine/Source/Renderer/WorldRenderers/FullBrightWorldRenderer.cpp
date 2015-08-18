#include "Renderer/PrecompiledHeader.h"
#include "Renderer/WorldRenderers/FullBrightWorldRenderer.h"
#include "Renderer/RenderQueue.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/Shaders/FullBrightShader.h"
#include "Engine/Material.h"

FullBrightWorldRenderer::FullBrightWorldRenderer(GPUContext *pGPUContext, const Options *pOptions)
    : SingleShaderWorldRenderer(pGPUContext, pOptions)
{

}

FullBrightWorldRenderer::~FullBrightWorldRenderer()
{

}

void FullBrightWorldRenderer::DrawQueueEntry(const ViewParameters *pViewParameters, RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry)
{
    ShaderProgram *pShaderProgram;
    if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(FullBrightShader), 0, pQueueEntry)) == NULL)
        return;

    m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());
    m_pGPUContext->SetRasterizerState(pQueueEntry->pMaterial->GetShader()->SelectRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false));
    m_pGPUContext->SetDepthStencilState(pQueueEntry->pMaterial->GetShader()->SelectDepthStencilState(true, true, GPU_COMPARISON_FUNC_LESS), 0);
    SetBlendingModeForMaterial(m_pGPUContext, pQueueEntry);

    SetCommonShaderProgramParameters(pViewParameters, pQueueEntry, pShaderProgram);
    pQueueEntry->pRenderProxy->DrawQueueEntry(&pViewParameters->ViewCamera, pQueueEntry, m_pGPUContext);
}

