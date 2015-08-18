#pragma once
#include "Editor/Common.h"
#include "Editor/EditorViewController.h"
#include "Renderer/MiniGUIContext.h"
#include "Renderer/VertexFactories/PlainVertexFactory.h"

class StaticMesh;
class RenderWorld;
class StaticMeshRenderProxy;
class CompositeRenderProxy;
class StaticMeshGenerator;
class EditorLightSimulator;
class Ui_EditorStaticMeshEditor;

class EditorStaticMeshEditor : public QMainWindow
{
    Q_OBJECT

public:
    enum Tool
    {
        Tool_Information,
        Tool_Operations,
        Tool_Materials,
        Tool_LOD,
        Tool_Collision,
        Tool_LightManipulator,
        Tool_Count
    };

    enum State
    {
        State_None,
        State_RotateCamera,
        State_Count
    };

public:
    EditorStaticMeshEditor();
    virtual ~EditorStaticMeshEditor();

    // render options accessors
    const EDITOR_CAMERA_MODE GetCameraMode() const { return m_viewController.GetCameraMode(); }
    const EDITOR_RENDER_MODE GetRenderMode() const { return m_renderMode; }
    const uint32 GetViewportFlags() const { return m_viewportFlags; }
    const bool HasViewportFlag(uint32 flag) const { return (m_viewportFlags & flag) != 0; }
    const Tool GetCurrentTool() const { return m_currentTool; }

    // mode setters
    void SetCameraMode(EDITOR_CAMERA_MODE cameraMode);
    void SetRenderMode(EDITOR_RENDER_MODE renderMode);
    void SetViewportFlag(uint32 flag);
    void ClearViewportFlag(uint32 flag);
    void SetCurrentTool(Tool tool);

    // creation/loading/saving
    bool Create(const char *meshName, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool Create(const StaticMeshGenerator *pGenerator, const char *meshName = nullptr, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool Load(const char *meshName, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool Save(ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool SaveAs(const char *meshName, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    void Close();

    // generator
    const StaticMeshGenerator *GetStaticMeshGenerator() const { return m_pGenerator; }

    // flag for redraw
    void FlagForRedraw() { m_bRedrawPending = true; }

private:
    // resources
    bool CreateHardwareResources();
    void ReleaseHardwareResources();

    // automatically determine the import directories
    void OnFileNameChanged();

    // load and allocate materials
    void UpdateMaterials();

    // recompile preview mesh
    bool RecompileMesh();

    // update the collision mesh preview
    void RefreshCollisionShapePreview();

    // update the information tab
    void UpdateInformationTab();

    // draw the scene
    void Draw();
    void DrawGrid();
    void DrawView();
    void DrawOverlays(GPUContext *pGPUContext);

    // --- ui ---
    Ui_EditorStaticMeshEditor *m_ui;

    // --- editor info ---
    EditorViewController m_viewController;
    EDITOR_RENDER_MODE m_renderMode;
    uint32 m_viewportFlags;

    // --- generator ---
    StaticMeshGenerator *m_pGenerator;

    String m_meshName;
    String m_meshFileName;

    // --- materials ---
    Array<String> m_meshMaterialNames;

    // --- compiled mesh ---
    StaticMesh *m_pCompiledMesh;

    // --- render mesh ---
    RenderWorld *m_pRenderWorld;
    EditorLightSimulator *m_pLightSimulator;
    StaticMeshRenderProxy *m_pMeshPreviewRenderProxy;
    CompositeRenderProxy *m_pCollisionShapePreviewRenderProxy;

    // --- renderer stuff ---
    RendererOutputBuffer *m_pSwapChain;
    WorldRenderer *m_pWorldRenderer;
    MiniGUIContext m_guiContext;
    bool m_bHardwareResourcesCreated;
    bool m_bRedrawPending;

    // --- state ---
    int2 m_lastMousePosition;
    Tool m_currentTool;
    State m_currentState;

    //===================================================================================================================================================================
    // UI Event Handlers
    //===================================================================================================================================================================
    void ConnectUIEvents();

private:
    void closeEvent(QCloseEvent *pCloseEvent);

private Q_SLOTS:
    void OnActionOpenMeshClicked();
    void OnActionSaveMeshClicked();
    void OnActionSaveMeshAsClicked();
    void OnActionCloseClicked();
    void OnActionCameraPerspectiveTriggered(bool checked) { SetCameraMode(EDITOR_CAMERA_MODE_PERSPECTIVE); }
    void OnActionCameraArcballTriggered(bool checked) { SetCameraMode(EDITOR_CAMERA_MODE_ARCBALL); }
    void OnActionCameraIsometricTriggered(bool checked) { SetCameraMode(EDITOR_CAMERA_MODE_ISOMETRIC); }
    void OnActionViewWireframeTriggered(bool checked) { SetRenderMode(EDITOR_RENDER_MODE_WIREFRAME); }
    void OnActionViewUnlitTriggered(bool checked) { SetRenderMode(EDITOR_RENDER_MODE_FULLBRIGHT); }
    void OnActionViewLitTriggered(bool checked) { SetRenderMode(EDITOR_RENDER_MODE_LIT); }
    void OnActionViewFlagShadowsTriggered(bool checked) { (checked) ? SetViewportFlag(EDITOR_VIEWPORT_FLAG_ENABLE_SHADOWS) : ClearViewportFlag(EDITOR_VIEWPORT_FLAG_ENABLE_SHADOWS); }
    void OnActionViewFlagWireframeOverlayTriggered(bool checked) { (checked) ? SetViewportFlag(EDITOR_VIEWPORT_FLAG_WIREFRAME_OVERLAY) : ClearViewportFlag(EDITOR_VIEWPORT_FLAG_WIREFRAME_OVERLAY); }
    void OnActionToolInformationTriggered(bool checked) { SetCurrentTool((checked) ? Tool_Information : Tool_Information); }
    void OnActionToolOperationsTriggered(bool checked) { SetCurrentTool((checked) ? Tool_Operations : Tool_Information); }
    void OnActionToolMaterialsTriggered(bool checked) { SetCurrentTool((checked) ? Tool_Materials : Tool_Information); }
    void OnActionToolLODTriggered(bool checked) { SetCurrentTool((checked) ? Tool_LOD : Tool_Information); }
    void OnActionToolCollisionTriggered(bool checked) { SetCurrentTool((checked) ? Tool_Collision : Tool_Information); }
    void OnActionToolLightManipulatorTriggered(bool checked) { SetCurrentTool((checked) ? Tool_LightManipulator : Tool_Information); }
    void OnSwapChainWidgetResized();
    void OnSwapChainWidgetPaint();
    void OnSwapChainWidgetKeyboardEvent(const QKeyEvent *pKeyboardEvent);
    void OnSwapChainWidgetMouseEvent(const QMouseEvent *pMouseEvent);
    void OnSwapChainWidgetWheelEvent(const QWheelEvent *pWheelEvent);
    void OnSwapChainWidgetGainedFocusEvent();
    void OnFrameExecutionTriggered(float timeSinceLastFrame);
    void OnOperationsGenerateTangentsClicked();
    void OnOperationsCenterMeshClicked();
    void OnOperationsRemoveUnusedVerticesClicked();
    void OnOperationsRemoveUnusedTrianglesClicked();
};

