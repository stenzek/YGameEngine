#pragma once
#include "Editor/Common.h"
#include "Editor/EditorRendererSwapChainWidget.h"
#include "Editor/EditorViewController.h"
#include "Renderer/MiniGUIContext.h"
#include "Renderer/Renderer.h"
#include "Renderer/WorldRenderer.h"
#include "Editor/MapEditor/ui_EditorMapViewport.h"
#include "Core/Image.h"

class EditorMapWindow;
class EditorMap;
class Font;

class EditorMapViewport : public QWidget
{
    Q_OBJECT

public:
    EditorMapViewport(EditorMapWindow *pMainWindow, EditorMap *pMap, uint32 viewportId, EDITOR_CAMERA_MODE cameraMode, EDITOR_RENDER_MODE renderMode, uint32 viewportFlags);
    ~EditorMapViewport();

    // pod accessors
    const EditorMapWindow *GetMainWindow() const { return m_pMapWindow; }
    const EditorMap *GetMap() const { return m_pMap; }
    const uint32 GetViewportId() const { return m_viewportId; }
    const EditorViewController &GetViewController() const { return m_viewController; }
    const MiniGUIContext &GetGUIContext() const { return m_guiContext; }
    const bool IsActiveViewport() const;

    // mainly for overlay drawing in edit modes
    EditorViewController &GetViewController() { return m_viewController; }
    MiniGUIContext &GetGUIContext() { return m_guiContext; }

    // render options accessors
    const EDITOR_CAMERA_MODE GetCameraMode() const { return m_viewController.GetCameraMode(); }
    const EDITOR_RENDER_MODE GetRenderMode() const { return m_renderMode; }
    const uint32 GetViewportFlags() const { return m_viewportFlags; }
    const bool HasViewportFlag(uint32 flag) const { return (m_viewportFlags & flag) != 0; }

    // mode setters
    void SetCameraMode(EDITOR_CAMERA_MODE cameraMode);
    void SetRenderMode(EDITOR_RENDER_MODE renderMode);
    void SetViewportFlag(uint32 flag);
    void ClearViewportFlag(uint32 flag);

    // set the focus to this viewport
    void SetFocus();

    // to avoid hammering our hardware with resizes, we delay creation of the swap chain until
    // all the windows have been placed and sized.
    bool CreateHardwareResources();
    void ReleaseHardwareResources();

    // draws the viewport, returns false if nothing was drawn
    bool Draw(const float timeDiff);

    // locks cursor in the middle of the viewport?
    const bool IsMouseGrabbed() const { return m_mouseGrabbed; }
    const int2 &GetMousePosition() const { return m_mousePosition; }
    const int2 &GetMouseDelta() const { return m_mouseDelta; }
    const int2 CalculateMouseDelta(const int2 &newPosition) const { return (newPosition - m_mousePosition); }
    const int2 CalculateMouseDelta(const QMouseEvent *pMouseEvent) const { return CalculateMouseDelta(int2(pMouseEvent->x(), pMouseEvent->y())); }
    void LockMouseCursor();
    void UnlockMouseCursor();

    // sets cursor on viewport
    void SetMouseCursor(EDITOR_CURSOR_TYPE cursorType);

    // flag for redraw
    void FlagForRedraw();

    // picking texture methods
    bool GetPickingTextureValues(const int2 &MinCoordinates, const int2 &MaxCoordinates, uint32 *pValues) const;
    uint32 GetPickingTextureValue(const int2 &Position) const;

    // obtain a pick ray
    Ray GetPickRay() const;

private:
    // grid draw
    void DrawView();
    void DrawAfterPost();
    void DrawPickTexture();

    // vars
    Ui_EditorMapViewport *m_ui;
    EditorMapWindow *m_pMapWindow;
    EditorMap *m_pMap;
    uint32 m_viewportId;

    // view controller
    EditorViewController m_viewController;

    // flags
    EDITOR_RENDER_MODE m_renderMode;
    uint32 m_viewportFlags;

    // state
    float m_worldTime;
    int2 m_mousePosition;
    int2 m_mouseDelta;
    bool m_mouseGrabbed;
    int2 m_mouseGrabPosition;
    EDITOR_CURSOR_TYPE m_cursorType;

    // redraw pending?
    bool m_redrawPending;

    // renderer stuff
    bool m_hardwareResourcesCreated;

    // renderer objects
    RendererOutputBuffer *m_pSwapChain;
    GPUTexture2D *m_pPickingRenderTarget;
    GPURenderTargetView *m_pPickingRenderTargetView;
    GPUDepthTexture *m_pPickingDepthStencilBuffer;
    GPUDepthStencilBufferView *m_pPickingDepthStencilBufferView;
    Image m_PickingTextureCopy;
    WorldRenderer *m_pWorldRenderer;
    WorldRenderer *m_pPickingWorldRenderer;
    MiniGUIContext m_guiContext;

    //===================================================================================================================================================================
    // UI Event Handlers
    //===================================================================================================================================================================
    void ConnectUIEvents();

private:
    void focusInEvent(QFocusEvent *pFocusEvent);

private Q_SLOTS:
    void OnToolbarMoreOptionsClicked();
    void OnToolbarRealtimeClicked(bool checked) { (checked) ? SetViewportFlag(EDITOR_VIEWPORT_FLAG_REALTIME) : ClearViewportFlag(EDITOR_VIEWPORT_FLAG_REALTIME); }
    void OnToolbarGameViewClicked(bool checked) { (checked) ? SetViewportFlag(EDITOR_VIEWPORT_FLAG_GAME_VIEW) : ClearViewportFlag(EDITOR_VIEWPORT_FLAG_GAME_VIEW); }
    void OnToolbarCameraFrontClicked(bool checked) { SetCameraMode(EDITOR_CAMERA_MODE_ORTHOGRAPHIC_FRONT); }
    void OnToolbarCameraSideClicked(bool checked) { SetCameraMode(EDITOR_CAMERA_MODE_ORTHOGRAPHIC_SIDE); }
    void OnToolbarCameraTopClicked(bool checked) { SetCameraMode(EDITOR_CAMERA_MODE_ORTHOGRAPHIC_TOP); }
    void OnToolbarCameraIsometricClicked(bool checked) { SetCameraMode(EDITOR_CAMERA_MODE_ISOMETRIC); }
    void OnToolbarCameraPerspectiveClicked(bool checked) { SetCameraMode(EDITOR_CAMERA_MODE_PERSPECTIVE); }
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
    void OnSwapChainWidgetDropEvent(QDropEvent *pDropEvent);
    void OnSwapChainWidgetGainedFocusEvent();
};
