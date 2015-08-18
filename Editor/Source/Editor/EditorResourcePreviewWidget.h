#pragma once
#include "Editor/Common.h"
#include "Editor/EditorRendererSwapChainWidget.h"
#include "Editor/ui_EditorResourcePreviewWidget.h"
#include "Engine/ArcBallCamera.h"
#include "Renderer/WorldRenderer.h"
#include "Renderer/MiniGUIContext.h"

class Resource;
class ResourceTypeInfo;
class Material;
class MaterialShader;
class Texture;
class StaticMesh;
class BlockMesh;

class RenderWorld;
class RenderProxy;
class EditorLightSimulator;

class EditorResourcePreviewWidget : public QWidget
{
    Q_OBJECT

    enum PREVIEW_TYPE
    {
        PREVIEW_TYPE_NONE,
        PREVIEW_TYPE_MATERIAL,
        PREVIEW_TYPE_MATERIALSHADER,
        PREVIEW_TYPE_TEXTURE2D,
        PREVIEW_TYPE_STATICMESH,
        PREVIEW_TYPE_STATICBLOCKMESH,
    };

    enum STATE
    {
        STATE_NONE,
        STATE_ROTATE_CAMERA,
        STATE_COUNT,
    };

public:
    EditorResourcePreviewWidget(QWidget *pParent = NULL);
    ~EditorResourcePreviewWidget();

    void ClearPreview();
    bool SetPreviewResource(const Resource *pResource);
    bool SetPreviewResourceByName(const ResourceTypeInfo *pResourceTypeInfo, const char *resourceName);
    void SetPreviewMaterial(const Material *pMaterial);
    void SetPreviewMaterialShader(const MaterialShader *pMaterialShader);
    void SetPreviewTexture(const Texture *pTexture);
    void SetPreviewStaticMesh(const StaticMesh *pStaticMesh);
    void SetPreviewStaticBlockMesh(const BlockMesh *pStaticBlockMesh);
    void SetPreviewErrorMessage(const char *message);

    // render options
    const EDITOR_RENDER_MODE GetRenderMode() const { return m_renderMode; }
    void SetRenderMode(EDITOR_RENDER_MODE renderMode);
    void SetViewportFlag(uint32 flag);
    void ClearViewportFlag(uint32 flag);

    // gpu resources
    bool CreateGPUResources();
    void ReleaseGPUResources();

    // flag for redraw
    void FlagForRedraw() { m_bRedrawPending = true; }

private:
    void SetupStaticMesh(const StaticMesh *pStaticMeshToRender, const Material *pMaterialOverride);
    void UpdateZoomScale();
    void ResetCamera();

    void Draw();
    void DrawPreview();
    void DrawPreviewOverlays();

    // ui
    Ui_EditorResourcePreviewWidget *m_ui;

    // render settings
    EDITOR_RENDER_MODE m_renderMode;
    uint32 m_viewportFlags;

    // preview object
    PREVIEW_TYPE m_ePreviewType;
    union
    {
        const Resource *asResource;

        const Material *asMaterial;
        const MaterialShader *asMaterialShader;
        const Texture *asTexture;
        const StaticMesh *asStaticMesh;
        const BlockMesh *asStaticBlockMesh;
    } m_Resource;

    // redraw pending?
    bool m_bRedrawPending;

    // renderer stuff
    uint32 m_renderTargetWidth, m_renderTargetHeight;
    bool m_bHardwareResourcesCreated;

    // renderer objects
    RendererOutputBuffer *m_pSwapChain;
    WorldRenderer *m_pWorldRenderer;
    WorldRenderer::ViewParameters m_viewParameters;
    MiniGUIContext m_guiContext;
    
    // cameras
    ArcBallCamera m_ArcBallCamera;
    float m_fZoomScale;

    const Font *m_pOverlayTextFont;
    String m_errorMessage;

    RenderWorld *m_pRenderWorld;
    RenderProxy *m_pResourceRenderProxy;
    EditorLightSimulator *m_pLightSimulator;

    // --- key/button states ---
    STATE m_eCurrentState;
    uint32 m_iStateData;
    int2 m_LastMousePosition;

    //===================================================================================================================================================================
    // UI Event Handlers
    //===================================================================================================================================================================
    void ConnectUIEvents();

private Q_SLOTS:
    void OnFocusGained();
    void OnFocusLost();
    void OnToolbarViewWireframeClicked(bool checked) { SetRenderMode(EDITOR_RENDER_MODE_WIREFRAME); }
    void OnToolbarViewUnlitClicked(bool checked) { SetRenderMode(EDITOR_RENDER_MODE_FULLBRIGHT); }
    void OnToolbarViewLitClicked(bool checked) { SetRenderMode(EDITOR_RENDER_MODE_LIT); }
    void OnToolbarViewLightingOnlyClicked(bool checked) { SetRenderMode(EDITOR_RENDER_MODE_LIGHTING_ONLY); }
    void OnToolbarFlagShadowsClicked(bool checked) { (checked) ? SetViewportFlag(EDITOR_VIEWPORT_FLAG_ENABLE_SHADOWS) : ClearViewportFlag(EDITOR_VIEWPORT_FLAG_ENABLE_SHADOWS); }
    void OnToolbarFlagWireframeOverlayClicked(bool checked) { (checked) ? SetViewportFlag(EDITOR_VIEWPORT_FLAG_WIREFRAME_OVERLAY) : ClearViewportFlag(EDITOR_VIEWPORT_FLAG_WIREFRAME_OVERLAY); }
    void OnSwapChainWidgetResized();
    void OnSwapChainWidgetPaint();
    void OnSwapChainWidgetKeyboardEvent(const QKeyEvent *pKeyboardEvent);
    void OnSwapChainWidgetMouseEvent(const QMouseEvent *pMouseEvent);
    void OnSwapChainWidgetWheelEvent(const QWheelEvent *pWheelEvent);
    void OnFrameExecutionTriggered(float timeSinceLastFrame);
};