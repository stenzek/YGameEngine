#pragma once
#include "Renderer/WorldRenderer.h"

class SingleShaderWorldRenderer : public WorldRenderer
{
public:
    SingleShaderWorldRenderer(GPUContext *pGPUContext, const Options *pOptions);
    virtual ~SingleShaderWorldRenderer();

    virtual void DrawWorld(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, GPURenderTargetView *pRenderTargetView, GPUDepthStencilBufferView *pDepthStencilBufferView, RenderProfiler *pRenderProfiler) override;

protected:
    // Set by derived classes.
    virtual void PreDraw(const ViewParameters *pViewParameters);
    virtual void DrawQueueEntry(const ViewParameters *pViewParameters, RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry);
    virtual void PostDraw(const ViewParameters *pViewParameters);

    // common params
    void SetCommonShaderProgramParameters(const ViewParameters *pViewParameters, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, ShaderProgram *pShaderProgram);

private:
    void DrawOpqaueObjects(const ViewParameters *pViewParameters);
    void DrawPostProcessObjects(const ViewParameters *pViewParameters);
    void DrawTranslucentObjects(const ViewParameters *pViewParameters);
};

