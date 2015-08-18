#include "Renderer/PrecompiledHeader.h"
#include "Renderer/WorldRenderers/DebugNormalsWorldRenderer.h"
#include "Renderer/RenderQueue.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderComponent.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/ShaderProgram.h"
#include "Engine/Material.h"

class DebugNormalsShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(DebugNormalsShader, ShaderComponent);

public:
    DebugNormalsShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
    {
        if (pVertexFactoryTypeInfo == nullptr || pMaterialShader == nullptr)
            return false;

        return true;
    }

    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
    {
        pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/DebugNormalsShader.hlsl", "VSMain");
        pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/DebugNormalsShader.hlsl", "PSMain");
        return true;
    }
};

DEFINE_SHADER_COMPONENT_INFO(DebugNormalsShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(DebugNormalsShader)
END_SHADER_COMPONENT_PARAMETERS()

DebugNormalsWorldRenderer::DebugNormalsWorldRenderer(GPUContext *pGPUContext, const Options *pOptions)
    : SingleShaderWorldRenderer(pGPUContext, pOptions)
{

}

DebugNormalsWorldRenderer::~DebugNormalsWorldRenderer()
{

}

void DebugNormalsWorldRenderer::DrawQueueEntry(const ViewParameters *pViewParameters, RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry)
{
    ShaderProgram *pShaderProgram;
    if ((pShaderProgram = GetShaderProgram(OBJECT_TYPEINFO(DebugNormalsShader), 0, pQueueEntry)) == NULL)
        return;

    m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());
    m_pGPUContext->SetRasterizerState(pQueueEntry->pMaterial->GetShader()->SelectRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK, false, false));
    m_pGPUContext->SetDepthStencilState(pQueueEntry->pMaterial->GetShader()->SelectDepthStencilState(true, true, GPU_COMPARISON_FUNC_LESS), 0);
    SetBlendingModeForMaterial(m_pGPUContext, pQueueEntry);

    SetCommonShaderProgramParameters(pViewParameters, pQueueEntry, pShaderProgram);
    pQueueEntry->pRenderProxy->DrawQueueEntry(&pViewParameters->ViewCamera, pQueueEntry, m_pGPUContext);
}

