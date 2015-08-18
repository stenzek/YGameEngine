#pragma once
#include "Editor/Common.h"
#include "Renderer/WorldRenderers/SingleShaderWorldRenderer.h"
#include "Renderer/RenderQueue.h"

class EditorEntityIdWorldRenderer : public SingleShaderWorldRenderer
{
public:
    EditorEntityIdWorldRenderer(GPUContext *pGPUContext, const Options *pOptions);
    virtual ~EditorEntityIdWorldRenderer();

private:
    virtual void DrawQueueEntry(const ViewParameters *pViewParameters, RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry);
};

