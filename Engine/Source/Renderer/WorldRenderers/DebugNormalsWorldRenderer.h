#pragma once
#include "Renderer/WorldRenderers/SingleShaderWorldRenderer.h"

class DebugNormalsWorldRenderer : public SingleShaderWorldRenderer
{
public:
    DebugNormalsWorldRenderer(GPUContext *pGPUContext, const Options *pOptions);
    virtual ~DebugNormalsWorldRenderer();

protected:
    virtual void DrawQueueEntry(const ViewParameters *pViewParameters, RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry) override;
};
