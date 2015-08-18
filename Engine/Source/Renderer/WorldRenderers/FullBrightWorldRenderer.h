#pragma once
#include "Renderer/WorldRenderers/SingleShaderWorldRenderer.h"
#include "Renderer/RenderQueue.h"
#include "Renderer/MiniGUIContext.h"

class FullBrightWorldRenderer : public SingleShaderWorldRenderer
{
public:
    FullBrightWorldRenderer(GPUContext *pGPUContext, const Options *pOptions);
    virtual ~FullBrightWorldRenderer();

protected:
    virtual void DrawQueueEntry(const ViewParameters *pViewParameters, RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry) override;
};
