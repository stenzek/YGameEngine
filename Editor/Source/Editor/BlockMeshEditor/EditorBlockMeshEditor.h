#pragma once
#include "Editor/Common.h"
#include "Editor/EditorViewController.h"
#include "ResourceCompiler/BlockMeshGenerator.h"
#include "Engine/BlockPalette.h"
#include "Renderer/VertexFactories/LocalVertexFactory.h"
#include "Renderer/VertexFactories/PlainVertexFactory.h"
#include "Renderer/MiniGUIContext.h"
#include "Renderer/Renderer.h"
#include "Renderer/WorldRenderer.h"
#include "Core/Image.h"

class EditorLightSimulator;
class EditorBlockVolumeRenderProxy;

class Ui_EditorBlockMeshEditor;

class EditorBlockMeshEditor : public QMainWindow
{
    Q_OBJECT

public:
    enum STATE
    {
        STATE_NONE,
        STATE_MOVE_CAMERA,
        STATE_ROTATE_CAMERA,
        STATE_MANIPULATE_LIGHT,
    };

    enum WIDGET
    {
        WIDGET_CAMERA,
        WIDGET_SELECT_BLOCKS,
        WIDGET_SELECT_AREA,
        WIDGET_PLACE_BLOCKS,
        WIDGET_DELETE_BLOCKS,
        WIDGET_LIGHT_MANIPULATOR,
        WIDGET_COUNT,
    };

    struct AvailableBlockType
    {
        const BlockPalette::BlockType *pBlockType;
        String BlockName;
        QIcon BlockTypeIcon;
    };

public:
    EditorBlockMeshEditor();
    ~EditorBlockMeshEditor();

    // mainly for overlay drawing in edit modes
    EditorViewController &GetCamera() { return m_viewController; }
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

    // creation/loading/saving
    bool Create(const char *meshName, const BlockPalette *pBlockList, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool Load(const char *meshName, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool Save(ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool SaveAs(const char *meshName, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // generator
    const BlockMeshGenerator *GetBlockMeshGenerator() const { return m_pGenerator; }

    // current lod
    const uint32 GetCurrentLOD() const { return m_currentLOD; }
    void SetCurrentLOD(uint32 LOD);
   
    // tool management
    const WIDGET GetWidget() const { return m_eCurrentWidget; }
    void SetWidget(WIDGET t);

    // if in PLACE_BLOCKS widget mode
    const BlockVolumeBlockType GetPlaceBlockWidgetBlockType() const { return m_iPlaceBlockToolBlockType; }
    void SetPlaceBlockWidgetBlockType(BlockVolumeBlockType t);
    void SetNextPlaceBlockWidgetBlockType();
    void SetPreviousPlaceBlockWidgetBlockType();

    // if in SELECT_BLOCKS widget mode
    const uint32 GetSelectedBlockCount() const { return m_SelectedBlocks.GetSize(); }
    inline void SelectBlock(int32 x, int32 y, int32 z) { SelectBlock(Vector3i(x, y, z)); }
    inline void DeselectBlock(int32 x, int32 y, int32 z) { DeselectBlock(Vector3i(x, y, z)); }
    bool IsBlockSelected(const int3 &coords);
    void SelectBlock(const int3 &coords);
    void DeselectBlock(const int3 &coords);
    void DeselectAllBlocks();

    // if in SELECT_AREA widget mode
    const int3 &GetSelectedAreaMinBounds() const { return m_SelectedArea[0]; }
    const int3 &GetSelectedAreaMaxBounds() const { return m_SelectedArea[1]; }
    void ExpandSelectionToHeight();
    void ExpandSelection(const int3 &direction);
    void ClearSelectedArea();

    // if in SELECT_BLOCKS or SELECT_AREA widget mode
    void MoveSelectedBlocks(const int3 &direction);

    // flag for redraw
    void FlagForRedraw() { m_bRedrawPending = true; }

protected:
    // --- common initialization whether creating/loading ---
    bool Initialize(ProgressCallbacks *pProgressCallbacks);
    bool GenerateAvailableBlockTypes(ProgressCallbacks *pProgressCallbacks);

    // --- close event ---
    bool OnCloseAttempt();
    void Deinitialize();

    // retrieve the block position that is under the specified mouse position.
    bool GetBlockCoordinatesForMousePosition(int3 *pBlockCoordinates, CUBE_FACE *pFaceIndex, const int2 &mousePosition) const;
    bool GetPlaceBlockCoordinatesForMousePosition(int3 *pBlockCoordinates, const int2 &mousePosition) const;

    // resources
    bool CreateHardwareResources();
    void ReleaseHardwareResources();

    // events
    void Tick(const float dt);

    // --- draw methods ---
    void Draw();
    void DrawView();
    void DrawGrid();
    void Draw3DOverlays();

    // --- mesh methods ---
    void SetBlock(const int3 &blockPosition, BlockVolumeBlockType value);
    void CheckForMeshSizeChanges();
    void UpdateMouseBlockCoordinates();
    void UpdateMeshBounds();
    void UpdateMeshView();
    void UpdateSelectedBlocksView();
    void UpdatePreviewMesh(BlockVolumeBlockType blockType);

    // --- ui ---
    Ui_EditorBlockMeshEditor *m_ui;

    // --- mesh ---
    const BlockPalette *m_pBlockList;
    BlockMeshGenerator *m_pGenerator;
    String m_meshName;
    String m_meshFileName;

    // --- block previews ---
    AvailableBlockType *m_pAvailableBlockTypes;
    uint32 m_nAvailableBlockTypes;

    // --- editor info ---
    EditorViewController m_viewController;
    EDITOR_RENDER_MODE m_renderMode;
    uint32 m_viewportFlags;
    
    // --- renderer stuff ---
    RendererOutputBuffer *m_pSwapChain;
    WorldRenderer *m_pWorldRenderer;
    MiniGUIContext m_guiContext;
    bool m_bHardwareResourcesCreated;
    bool m_bRedrawPending;

    // --- volumes ---
    BlockMeshVolume m_meshVolume;
    BlockMeshVolume m_selectedVolume;
    BlockMeshVolume m_previewVolume;

    // --- mesh render proxies ---
    RenderWorld *m_pRenderWorld;
    EditorLightSimulator *m_pLightSimulator;
    EditorBlockVolumeRenderProxy *m_pMeshRenderProxy;
    EditorBlockVolumeRenderProxy *m_pSelectedRenderProxy;
    EditorBlockVolumeRenderProxy *m_pPreviewRenderProxy;

    // --- tool info ---
    uint32 m_currentLOD;
    WIDGET m_eCurrentWidget;
    int3 m_MouseOverBlock;
    bool m_bMouseOverBlockSet;
    CUBE_FACE m_iMouseOverBlockFaceIndex;
    bool m_bMouseOverPlaceBlockSet;
    int3 m_MouseOverPlaceBlock;
    //     -- place tool
    BlockVolumeBlockType m_iPlaceBlockToolBlockType;
    //     -- select tool
    MemArray<int3> m_SelectedBlocks;
    //     -- select area tool --
    uint8 m_iSelectedAreaCount;
    int3 m_SelectedArea[2];

    // --- key/button states ---
    STATE m_eCurrentState;
    uint32 m_iStateData;
    int2 m_lastMousePosition;

    //===================================================================================================================================================================
    // UI Event Handlers
    //===================================================================================================================================================================
    void ConnectUIEvents();

private:
    //void focusInEvent(QFocusEvent *pFocusEvent);
    void closeEvent(QCloseEvent *pCloseEvent);

private Q_SLOTS:
    void OnActionSaveMeshClicked();
    void OnActionSaveMeshAsClicked();
    void OnActionCloseClicked();
    void OnActionEditUndoClicked();
    void OnActionEditRedoClicked();
    void OnActionCameraPerspectiveTriggered(bool checked) { SetCameraMode(EDITOR_CAMERA_MODE_PERSPECTIVE); }
    void OnActionCameraArcballTriggered(bool checked) { SetCameraMode(EDITOR_CAMERA_MODE_ARCBALL); }
    void OnActionCameraIsometricTriggered(bool checked) { SetCameraMode(EDITOR_CAMERA_MODE_ISOMETRIC); }
    void OnActionViewWireframeTriggered(bool checked) { SetRenderMode(EDITOR_RENDER_MODE_WIREFRAME); }
    void OnActionViewUnlitTriggered(bool checked) { SetRenderMode(EDITOR_RENDER_MODE_FULLBRIGHT); }
    void OnActionViewLitTriggered(bool checked) { SetRenderMode(EDITOR_RENDER_MODE_LIT); }
    void OnActionViewFlagShadowsTriggered(bool checked) { (checked) ? SetViewportFlag(EDITOR_VIEWPORT_FLAG_ENABLE_SHADOWS) : ClearViewportFlag(EDITOR_VIEWPORT_FLAG_ENABLE_SHADOWS); }
    void OnActionViewFlagWireframeOverlayTriggered(bool checked) { (checked) ? SetViewportFlag(EDITOR_VIEWPORT_FLAG_WIREFRAME_OVERLAY) : ClearViewportFlag(EDITOR_VIEWPORT_FLAG_WIREFRAME_OVERLAY); }
    void OnActionWidgetCameraTriggered(bool checked) { SetWidget(WIDGET_CAMERA); }
    void OnActionWidgetSelectBlocksTriggered(bool checked) { SetWidget(WIDGET_SELECT_BLOCKS); }
    void OnActionWidgetSelectAreaTriggered(bool checked) { SetWidget(WIDGET_SELECT_AREA); }
    void OnActionWidgetPlaceBlocksTriggered(bool checked) { SetWidget(WIDGET_PLACE_BLOCKS); }
    void OnActionWidgetDeleteBlocksTriggered(bool checked) { SetWidget(WIDGET_DELETE_BLOCKS); }
    void OnActionWidgetLightManipulatorTriggered(bool checked) { SetWidget(WIDGET_LIGHT_MANIPULATOR); }
    void OnPlaceBlockWidgetToolButtonClicked(bool checked);
    void OnPlaceBlockWidgetListWidgetItemClicked(QListWidgetItem *pItem);
    void OnSwapChainWidgetResized();
    void OnSwapChainWidgetPaint();
    void OnSwapChainWidgetKeyboardEvent(const QKeyEvent *pKeyboardEvent);
    void OnSwapChainWidgetMouseEvent(const QMouseEvent *pMouseEvent);
    void OnSwapChainWidgetWheelEvent(const QWheelEvent *pWheelEvent);
    void OnSwapChainWidgetGainedFocusEvent();
    void OnFrameExecutionTriggered(float timeSinceLastFrame);
};
