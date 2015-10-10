#pragma once
#include "Renderer/WorldRenderer.h"

class CompositingWorldRenderer : public WorldRenderer
{
public:
    CompositingWorldRenderer(GPUContext *pGPUContext, const Options *pOptions);
    virtual ~CompositingWorldRenderer();

    // create renderer resources
    virtual bool Initialize();

    // Get render stats.
    virtual void GetRenderStats(RenderStats *pRenderStats) const;

    // On completion of frame.
    virtual void OnFrameComplete();
   
protected:
    // applies a guassian blur to a texture. uses intermediate buffer.
    void BlurTexture(GPUCommandList *pCommandList, GPUTexture2D *pBlurTexture, GPURenderTargetView *pBlurTextureRTV, float blurSigma = 0.8f, bool restoreViewport = true);

    // apply tone mapping. NOTE: Will overwrite the current viewport with the view parameters viewport, and render target.
    void ApplyFinalCompositePostProcess(const ViewParameters *pViewParameters, GPUTexture2D *pSceneColorTexture, GPURenderTargetView *pOutputRTV);

    // debug drawing
    void DrawDebugInfo(const Camera *pCamera);

    // composite programs
    ShaderProgram *m_pExtractLuminanceProgram;
    ShaderProgram *m_pBloomProgram;
    ShaderProgram *m_pToneMappingProgram;

    // blur programs
    ShaderProgram *m_pGaussianBlurProgram;
};
