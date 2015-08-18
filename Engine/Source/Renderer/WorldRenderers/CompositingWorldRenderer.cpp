#include "Renderer/PrecompiledHeader.h"
#include "Renderer/WorldRenderers/CompositingWorldRenderer.h"
#include "Renderer/ShaderComponent.h"
#include "Renderer/ShaderCompilerFrontend.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderProfiler.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/ShaderProgramSelector.h"
#include "Engine/Camera.h"
#include "Engine/EngineCVars.h"
#include "Engine/Material.h"
Log_SetChannel(CompositingWorldRenderer);

class GaussianBlurShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(GaussianBlurShader, ShaderComponent);

public:
    GaussianBlurShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static void SetProgramParameters(GPUContext *pContext, ShaderProgram *pProgram, GPUTexture2D *pSourceTexture, const float2 &blurDirection, float blurSigma);
    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

DEFINE_SHADER_COMPONENT_INFO(GaussianBlurShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(GaussianBlurShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("SourceTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("BlurDirection", SHADER_PARAMETER_TYPE_FLOAT2)
    DEFINE_SHADER_COMPONENT_PARAMETER("BlurSigma", SHADER_PARAMETER_TYPE_FLOAT)
END_SHADER_COMPONENT_PARAMETERS()

void GaussianBlurShader::SetProgramParameters(GPUContext *pContext, ShaderProgram *pProgram, GPUTexture2D *pSourceTexture, const float2 &blurDirection, float blurSigma)
{
    pProgram->SetBaseShaderParameterTexture(pContext, 0, pSourceTexture, g_pRenderer->GetFixedResources()->GetPointSamplerState());
    pProgram->SetBaseShaderParameterValue(pContext, 1, SHADER_PARAMETER_TYPE_FLOAT2, &blurDirection);
    pProgram->SetBaseShaderParameterValue(pContext, 2, SHADER_PARAMETER_TYPE_FLOAT, &blurSigma);
}

bool GaussianBlurShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo == nullptr || pMaterialShader == nullptr)
        return false;

    return true;
}

bool GaussianBlurShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/ScreenQuadVertexShader.hlsl", "Main");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/GaussianBlurPixelShader.hlsl", "Main");
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ExtractLuminanceShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(ExtractLuminanceShader, ShaderComponent);

public:
    ExtractLuminanceShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static void SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pSceneTexture);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

DEFINE_SHADER_COMPONENT_INFO(ExtractLuminanceShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(ExtractLuminanceShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("SceneTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
END_SHADER_COMPONENT_PARAMETERS()

void ExtractLuminanceShader::SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pSceneTexture)
{
    pShaderProgram->SetBaseShaderParameterTexture(pContext, 0, pSceneTexture, g_pRenderer->GetFixedResources()->GetPointSamplerState());
}

bool ExtractLuminanceShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr || pMaterialShader != nullptr)
        return false;

    return true;
}

bool ExtractLuminanceShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Requires feature level SM4
    if (pParameters->FeatureLevel < RENDERER_FEATURE_LEVEL_SM4)
        return false;

    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/ScreenQuadVertexShader.hlsl", "Main");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/ExtractLuminancePixelShader.hlsl", "PSMain");
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class BloomShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(BloomShader, ShaderComponent);

public:
    BloomShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static void SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pSceneTexture, float bloomThreshold = 0.3f);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

DEFINE_SHADER_COMPONENT_INFO(BloomShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(BloomShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("HDRTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("BloomThreshold", SHADER_PARAMETER_TYPE_FLOAT)
END_SHADER_COMPONENT_PARAMETERS()

void BloomShader::SetProgramParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pSceneTexture, float bloomThreshold /* = 0.3f */)
{
    pShaderProgram->SetBaseShaderParameterTexture(pContext, 0, pSceneTexture, g_pRenderer->GetFixedResources()->GetPointSamplerState());
    pShaderProgram->SetBaseShaderParameterValue(pContext, 1, SHADER_PARAMETER_TYPE_FLOAT, &bloomThreshold);
}

bool BloomShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr || pMaterialShader != nullptr)
        return false;

    return true;
}

bool BloomShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Requires feature level SM4
    if (pParameters->FeatureLevel < RENDERER_FEATURE_LEVEL_SM4)
        return false;

    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/ScreenQuadVertexShader.hlsl", "Main");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/BloomPixelShader.hlsl", "Main");
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ToneMapShader : public ShaderComponent
{
    DECLARE_SHADER_COMPONENT_INFO(ToneMapShader, ShaderComponent);

public:
    ToneMapShader(const ShaderComponentTypeInfo *pTypeInfo = &s_TypeInfo) : BaseClass(pTypeInfo) { }

    static void SetInputParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pHDRTexture);
    static void SetToneMapParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pAverageLuminanceTexture, float whiteLevel = 4.0f, float luminanceSaturation = 1.0f, bool useManualExposure = false, float manualExposure = 1.0f, float maximumExposure = 4.0f);
    static void SetBloomParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pBloomTexture, bool enableBloom = true, float bloomMagnitude = 1.0f);

    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);
};

DEFINE_SHADER_COMPONENT_INFO(ToneMapShader);
BEGIN_SHADER_COMPONENT_PARAMETERS(ToneMapShader)
    DEFINE_SHADER_COMPONENT_PARAMETER("HDRTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("AverageLuminanceTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("AverageLuminanceTextureMip", SHADER_PARAMETER_TYPE_UINT)
    DEFINE_SHADER_COMPONENT_PARAMETER("WhiteLevel", SHADER_PARAMETER_TYPE_FLOAT)
    DEFINE_SHADER_COMPONENT_PARAMETER("LuminanceSaturation", SHADER_PARAMETER_TYPE_FLOAT)
    DEFINE_SHADER_COMPONENT_PARAMETER("ManualExposure", SHADER_PARAMETER_TYPE_FLOAT)
    DEFINE_SHADER_COMPONENT_PARAMETER("MaximumExposure", SHADER_PARAMETER_TYPE_FLOAT)
    DEFINE_SHADER_COMPONENT_PARAMETER("UseManualExposure", SHADER_PARAMETER_TYPE_BOOL)
    DEFINE_SHADER_COMPONENT_PARAMETER("BloomTexture", SHADER_PARAMETER_TYPE_TEXTURE2D)
    DEFINE_SHADER_COMPONENT_PARAMETER("BloomMagnitude", SHADER_PARAMETER_TYPE_FLOAT)
    DEFINE_SHADER_COMPONENT_PARAMETER("EnableBloom", SHADER_PARAMETER_TYPE_BOOL)
END_SHADER_COMPONENT_PARAMETERS()

void ToneMapShader::SetInputParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pHDRTexture)
{
    pShaderProgram->SetBaseShaderParameterTexture(pContext, 0, pHDRTexture, g_pRenderer->GetFixedResources()->GetPointSamplerState());
}

void ToneMapShader::SetToneMapParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pAverageLuminanceTexture, float whiteLevel /* = 4.0f */, float luminanceSaturation /* = 1.0f */, bool useManualExposure /* = false */, float manualExposure /* = 1.0f */, float maximumExposure /* = 4.0f */)
{
    uint32 avgLuminanceMipLevel = pAverageLuminanceTexture->GetDesc()->MipLevels - 1;
    uint32 useManualExposureValue = (uint32)useManualExposure;
    pShaderProgram->SetBaseShaderParameterTexture(pContext, 1, pAverageLuminanceTexture, g_pRenderer->GetFixedResources()->GetPointSamplerState());
    pShaderProgram->SetBaseShaderParameterValue(pContext, 2, SHADER_PARAMETER_TYPE_UINT, &avgLuminanceMipLevel);
    pShaderProgram->SetBaseShaderParameterValue(pContext, 3, SHADER_PARAMETER_TYPE_FLOAT, &whiteLevel);
    pShaderProgram->SetBaseShaderParameterValue(pContext, 4, SHADER_PARAMETER_TYPE_FLOAT, &luminanceSaturation);
    pShaderProgram->SetBaseShaderParameterValue(pContext, 5, SHADER_PARAMETER_TYPE_FLOAT, &manualExposure);
    pShaderProgram->SetBaseShaderParameterValue(pContext, 6, SHADER_PARAMETER_TYPE_FLOAT, &maximumExposure);
    pShaderProgram->SetBaseShaderParameterValue(pContext, 7, SHADER_PARAMETER_TYPE_BOOL, &useManualExposureValue);
}

void ToneMapShader::SetBloomParameters(GPUContext *pContext, ShaderProgram *pShaderProgram, GPUTexture2D *pBloomTexture, bool enableBloom /* = true */, float bloomMagnitude /* = 1.0f */)
{
    uint32 enableBloomValue = (uint32)enableBloom;
    pShaderProgram->SetBaseShaderParameterTexture(pContext, 8, pBloomTexture, g_pRenderer->GetFixedResources()->GetPointSamplerState());
    pShaderProgram->SetBaseShaderParameterValue(pContext, 9, SHADER_PARAMETER_TYPE_FLOAT, &bloomMagnitude);
    pShaderProgram->SetBaseShaderParameterValue(pContext, 10, SHADER_PARAMETER_TYPE_BOOL, &enableBloomValue);
}

bool ToneMapShader::IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
{
    if (pVertexFactoryTypeInfo != nullptr || pMaterialShader != nullptr)
        return false;

    return true;
}

bool ToneMapShader::FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters)
{
    // Requires feature level SM4
    if (pParameters->FeatureLevel < RENDERER_FEATURE_LEVEL_SM4)
        return false;

    // Entry points
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_VERTEX_SHADER, "shaders/base/ScreenQuadVertexShader.hlsl", "Main");
    pParameters->SetStageEntryPoint(SHADER_PROGRAM_STAGE_PIXEL_SHADER, "shaders/base/ToneMapShader.hlsl", "PSMain");
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CompositingWorldRenderer::CompositingWorldRenderer(GPUContext *pGPUContext, const Options *pOptions)
    : WorldRenderer(pGPUContext, pOptions),
      m_pExtractLuminanceProgram(nullptr),
      m_pBloomProgram(nullptr),
      m_pToneMappingProgram(nullptr),
      m_pGaussianBlurProgram(nullptr)
{

}

CompositingWorldRenderer::~CompositingWorldRenderer()
{

}

bool CompositingWorldRenderer::Initialize()
{
    if (!WorldRenderer::Initialize())
        return false;

    // load programs
    if ((m_pExtractLuminanceProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(ExtractLuminanceShader), 0, g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributes(), g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributeCount(), nullptr, 0)) == nullptr ||
        (m_pBloomProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(BloomShader), 0, g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributes(), g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributeCount(), nullptr, 0)) == nullptr ||
        (m_pToneMappingProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(ToneMapShader), 0, g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributes(), g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributeCount(), nullptr, 0)) == nullptr ||
        (m_pGaussianBlurProgram = g_pRenderer->GetShaderProgram(0, OBJECT_TYPEINFO(GaussianBlurShader), 0, g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributes(), g_pRenderer->GetFixedResources()->GetFullScreenQuadVertexAttributeCount(), nullptr, 0)) == nullptr)
    {
        return false;
    }

    return true;
}

void CompositingWorldRenderer::GetRenderStats(RenderStats *pRenderStats) const
{
    WorldRenderer::GetRenderStats(pRenderStats);
}

void CompositingWorldRenderer::OnFrameComplete()
{
    WorldRenderer::OnFrameComplete();
}

void CompositingWorldRenderer::DrawDebugInfo(const Camera *pCamera, RenderProfiler *pRenderProfiler)
{
    WorldRenderer::DrawDebugInfo(pCamera, pRenderProfiler);
}

void CompositingWorldRenderer::BlurTexture(GPUTexture2D *pBlurTexture, GPURenderTargetView *pBlurTextureRTV, float blurSigma /* = 0.8f */, bool restoreViewport /* = true */)
{
    // request matching intermediate buffer
    IntermediateBuffer *pTempBuffer = RequestIntermediateBuffer(pBlurTexture->GetDesc()->Width, pBlurTexture->GetDesc()->Height, pBlurTexture->GetDesc()->Format, pBlurTexture->GetDesc()->MipLevels);
    if (pTempBuffer == nullptr)
        return;

    // save old viewport
    RENDERER_VIEWPORT oldViewport;
    if (restoreViewport)
        Y_memcpy(&oldViewport, m_pGPUContext->GetViewport(), sizeof(oldViewport));

    // setup common stuff
    m_pGPUContext->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK));
    m_pGPUContext->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false), 0);
    m_pGPUContext->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateNoBlending());
    m_pGPUContext->SetDefaultViewport(pBlurTexture);
    m_pGPUContext->SetShaderProgram(m_pGaussianBlurProgram->GetGPUProgram());

    // horizontal blur
    m_pGPUContext->SetRenderTargets(1, &pTempBuffer->pRTV, nullptr);
    m_pGPUContext->DiscardTargets(true, false, false);
    GaussianBlurShader::SetProgramParameters(m_pGPUContext, m_pGaussianBlurProgram, pBlurTexture, float2::UnitX, blurSigma);
    g_pRenderer->DrawFullScreenQuad(m_pGPUContext);

    // necessary since the textures will be swapped for inputs/outputs
    m_pGPUContext->ClearState(false, false, false, true);

    // vertical blur
    GaussianBlurShader::SetProgramParameters(m_pGPUContext, m_pGaussianBlurProgram, pTempBuffer->pTexture, float2::UnitY, blurSigma);
    m_pGPUContext->SetRenderTargets(1, &pBlurTextureRTV, nullptr);
    m_pGPUContext->DiscardTargets(true, false, false);
    g_pRenderer->DrawFullScreenQuad(m_pGPUContext);

    // restore viewport
    if (restoreViewport)
        m_pGPUContext->SetViewport(&oldViewport);

    // release int buffer
    ReleaseIntermediateBuffer(pTempBuffer);
}

void CompositingWorldRenderer::ApplyFinalCompositePostProcess(const ViewParameters *pViewParameters, GPUTexture2D *pSceneColorTexture, GPURenderTargetView *pOutputRTV)
{
    // common setup
    m_pGPUContext->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK));
    m_pGPUContext->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false), 0);
    m_pGPUContext->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateNoBlending());

    // create luminance texture
    IntermediateBuffer *pLuminanceBuffer = RequestIntermediateBuffer(m_options.RenderWidth, m_options.RenderHeight, PIXEL_FORMAT_R16_FLOAT, Renderer::CalculateMipCount(m_options.RenderWidth, m_options.RenderHeight));
    if (pLuminanceBuffer == nullptr)
        return;

    // run extract shader, generate mipmaps on luminance buffer
    if (!pViewParameters->EnableManualExposure)
    {
        m_pGPUContext->SetRenderTargets(1, &pLuminanceBuffer->pRTV, nullptr);
        m_pGPUContext->SetDefaultViewport(pLuminanceBuffer->pTexture);
        m_pGPUContext->SetShaderProgram(m_pExtractLuminanceProgram->GetGPUProgram());
        ExtractLuminanceShader::SetProgramParameters(m_pGPUContext, m_pExtractLuminanceProgram, pSceneColorTexture);
        g_pRenderer->DrawFullScreenQuad(m_pGPUContext);
        m_pGPUContext->GenerateMips(pLuminanceBuffer->pTexture);
        m_pGPUContext->ClearState(true, false, false, true);
    }

    // bloom buffer
    IntermediateBuffer *pBloomBuffer = RequestIntermediateBuffer(m_options.RenderWidth, m_options.RenderHeight, PIXEL_FORMAT_R8G8B8A8_UNORM, 1);
    if (pBloomBuffer == nullptr)
        return;

    // only fill bloom buffer when enabled
    if (pViewParameters->EnableBloom)
    {
        // generate bloom texture
        m_pGPUContext->SetRenderTargets(1, &pBloomBuffer->pRTV, nullptr);
        m_pGPUContext->SetDefaultViewport(pBloomBuffer->pTexture);
        m_pGPUContext->SetShaderProgram(m_pBloomProgram->GetGPUProgram());
        BloomShader::SetProgramParameters(m_pGPUContext, m_pBloomProgram, pSceneColorTexture, pViewParameters->BloomThreshold);
        g_pRenderer->DrawFullScreenQuad(m_pGPUContext);
        m_pGPUContext->ClearState(true, false, false, true);

        // allocate downsampled buffers
        IntermediateBuffer *pDownsampledBloomBuffer2 = RequestIntermediateBuffer(pBloomBuffer->Width / 2, pBloomBuffer->Height / 2, pBloomBuffer->PixelFormat, pBloomBuffer->MipLevels);
        IntermediateBuffer *pDownsampledBloomBuffer4 = RequestIntermediateBuffer(pBloomBuffer->Width / 4, pBloomBuffer->Height / 4, pBloomBuffer->PixelFormat, pBloomBuffer->MipLevels);
        IntermediateBuffer *pDownsampledBloomBuffer8 = RequestIntermediateBuffer(pBloomBuffer->Width / 8, pBloomBuffer->Height / 8, pBloomBuffer->PixelFormat, pBloomBuffer->MipLevels);
        if (pDownsampledBloomBuffer2 == nullptr || pDownsampledBloomBuffer4 == nullptr || pDownsampledBloomBuffer8 == nullptr)
        {
            ReleaseIntermediateBuffer(pDownsampledBloomBuffer8);
            ReleaseIntermediateBuffer(pDownsampledBloomBuffer4);
            ReleaseIntermediateBuffer(pDownsampledBloomBuffer2);
            ReleaseIntermediateBuffer(pBloomBuffer);
            ReleaseIntermediateBuffer(pLuminanceBuffer);
            return;
        }

        // downsample to /8
        ScaleTexture(pBloomBuffer->pTexture, pDownsampledBloomBuffer2->pRTV, false, false);
        ScaleTexture(pDownsampledBloomBuffer2->pTexture, pDownsampledBloomBuffer4->pRTV, false, false);
        ScaleTexture(pDownsampledBloomBuffer4->pTexture, pDownsampledBloomBuffer8->pRTV, false, false);

        // run blur passes
        BlurTexture(pDownsampledBloomBuffer8->pTexture, pDownsampledBloomBuffer8->pRTV, 0.8f, false);

        // upscale back to full size
        ScaleTexture(pDownsampledBloomBuffer8->pTexture, pDownsampledBloomBuffer4->pRTV, false, false);
        ScaleTexture(pDownsampledBloomBuffer4->pTexture, pDownsampledBloomBuffer2->pRTV, false, false);
        ScaleTexture(pDownsampledBloomBuffer2->pTexture, pBloomBuffer->pRTV, false, false);

        // release temporary downsampling buffers
        ReleaseIntermediateBuffer(pDownsampledBloomBuffer8);
        ReleaseIntermediateBuffer(pDownsampledBloomBuffer4);
        ReleaseIntermediateBuffer(pDownsampledBloomBuffer2);
    }

    // run tonemapping shader, output to destination (this requires destination viewport)
    m_pGPUContext->SetRenderTargets(1, &pOutputRTV, nullptr);
    m_pGPUContext->SetShaderProgram(m_pToneMappingProgram->GetGPUProgram());
    m_pGPUContext->SetViewport(&pViewParameters->Viewport);
    ToneMapShader::SetInputParameters(m_pGPUContext, m_pToneMappingProgram, pSceneColorTexture);
    ToneMapShader::SetToneMapParameters(m_pGPUContext, m_pToneMappingProgram, pLuminanceBuffer->pTexture, 4.0f, 1.0f, pViewParameters->EnableManualExposure, pViewParameters->ManualExposure, pViewParameters->MaximumExposure);
    ToneMapShader::SetBloomParameters(m_pGPUContext, m_pToneMappingProgram, pBloomBuffer->pTexture, pViewParameters->EnableBloom, pViewParameters->BloomMagnitude);
    g_pRenderer->DrawFullScreenQuad(m_pGPUContext);
    m_pGPUContext->ClearState(true, false, false, false);

    // todo: AddIntermediateBuffer for debugging
    AddDebugBufferView(pLuminanceBuffer, "Luminance", true);
    AddDebugBufferView(pBloomBuffer, "Bloom", true);
}
