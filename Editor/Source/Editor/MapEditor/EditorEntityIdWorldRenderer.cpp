#include "Editor/PrecompiledHeader.h"
#include "Editor/MapEditor/EditorEntityIdWorldRenderer.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/RenderQueue.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/Shaders/OneColorShader.h"
#include "Engine/Material.h"

EditorEntityIdWorldRenderer::EditorEntityIdWorldRenderer(GPUContext *pGPUContext, const Options *pOptions)
    : SingleShaderWorldRenderer(pGPUContext, pOptions)
{
    const uint32 availableRenderPassMask = RENDER_PASS_LIGHTMAP | RENDER_PASS_EMISSIVE |
                                           RENDER_PASS_STATIC_LIGHTING | RENDER_PASS_DYNAMIC_LIGHTING | RENDER_PASS_SHADOWED_LIGHTING |
                                           RENDER_PASS_TINT;

    m_renderQueue.SetAcceptingLights(false);
    m_renderQueue.SetAcceptingRenderPassMask(availableRenderPassMask);
    m_renderQueue.SetAcceptingOccluders(false);
    m_renderQueue.SetAcceptingDebugObjects(false);
}

EditorEntityIdWorldRenderer::~EditorEntityIdWorldRenderer()
{

}

void EditorEntityIdWorldRenderer::DrawQueueEntry(const ViewParameters *pViewParameters, RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry)
{
    ShaderProgram *pShaderProgram;
    if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(OneColorShader), 0, pQueueEntry)) == NULL)
        return;

    m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());
    m_pGPUContext->SetRasterizerState(pQueueEntry->pMaterial->GetShader()->SelectRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false));
    m_pGPUContext->SetDepthStencilState(pQueueEntry->pMaterial->GetShader()->SelectDepthStencilState(true, true, GPU_COMPARISON_FUNC_LESS), 0);
    SetBlendingModeForMaterial(m_pGPUContext, pQueueEntry);

    SetCommonShaderProgramParameters(pViewParameters, pQueueEntry, pShaderProgram);

    OneColorShader::SetColor(m_pGPUContext, pShaderProgram, PixelFormatHelpers::ConvertRGBAToFloat4(pQueueEntry->pRenderProxy->GetEntityId()));

    pQueueEntry->pRenderProxy->DrawQueueEntry(&pViewParameters->ViewCamera, pQueueEntry, m_pGPUContext);
}

