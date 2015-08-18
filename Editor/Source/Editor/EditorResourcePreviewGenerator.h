#pragma once
#include "Editor/Common.h"
#include "Engine/ArcBallCamera.h"
#include "Engine/BlockPalette.h"
#include "Renderer/RendererStateBlock.h"
#include "Renderer/WorldRenderer.h"

class Image;
class Resource;
class ResourceTypeInfo;
class Texture2D;
class Material;
class MaterialShader;
class Texture;
class StaticMesh;
class Font;

class EditorResourcePreviewGenerator
{
public:
    EditorResourcePreviewGenerator();
    ~EditorResourcePreviewGenerator();

    void SetGPUContext(GPUContext *pGPUContext) { m_pGPUContext = pGPUContext; }

    bool CreateGPUResources();
    void ReleaseGPUResources();

    bool GenerateResourcePreview(Image *pDestinationImage, const ResourceTypeInfo *pResourceTypeInfo, const char *resourceName);
    bool GenerateResourcePreview(Image *pDestinationImage, const Resource *pResource);

    bool GenerateMaterialPreview(Image *pDestinationImage, const Material *pMaterial);
    bool GenerateMaterialShaderPreview(Image *pDestinationImage, const MaterialShader *pMaterialShader);
    bool GenerateTexture2DPreview(Image *pDestinationImage, const Texture2D *pTexture);
    bool GenerateStaticMeshPreview(Image *pDestinationImage, const StaticMesh *pStaticMesh);
    bool GenerateFontDataPreview(Image *pDestinationImage, const Font *pFontData);
    bool GenerateBlockMeshBlockListPreview(Image *pDestinationImage, const BlockPalette *pBlockList);
    bool GenerateBlockMeshBlockListBlockPreview(Image *pDestinationImage, const BlockPalette *pBlockList, uint32 blockType);

private:
    bool SetupRender(Image *pDestinationImage);
    bool CompleteRender(Image *pDestinationImage);
    void SetCameraMatricesFor2D();
    void SetCameraMatricesForArcBall();
    void ResetCamera();

    // target info
    uint32 m_targetWidth;
    uint32 m_targetHeight;

    // gpu resources
    GPUContext *m_pGPUContext;
    GPUTexture2D *m_pRenderTarget;
    GPURenderTargetView *m_pRenderTargetView;
    GPUDepthTexture *m_pDepthStencilBuffer;
    GPUDepthStencilBufferView *m_pDepthStencilBufferView;
    WorldRenderer *m_pWorldRenderer;
    WorldRenderer::ViewParameters m_viewParameters;
    MiniGUIContext m_GUIContext;
    bool m_bGPUResourcesCreated;

    // state block
    RendererStateBlock m_stateBlock;

    // fonts
    Font *m_pOverlayTextFont;

    // cameras
    ArcBallCamera m_ArcBallCamera;

    // world
    RenderWorld *m_pRenderWorld;
};

