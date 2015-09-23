#pragma once
#include "Editor/Common.h"
#include "Editor/EditorViewController.h"
#include "Renderer/MiniGUIContext.h"
#include "Renderer/VertexFactories/PlainVertexFactory.h"
#include "Engine/Skeleton.h"

class SkeletalAnimationGenerator;
class SkeletalAnimation;
class SkeletalMesh;
class Skeleton;
class RenderWorld;
class SkeletalMeshRenderProxy;
class EditorLightSimulator;
class Ui_EditorSkeletalAnimationEditor;

class EditorSkeletalAnimationEditor : public QMainWindow
{
    Q_OBJECT

public:
    enum Tool
    {
        Tool_Information,
        Tool_BoneTrack,
        Tool_RootMotion,
        Tool_Clipper,
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
    EditorSkeletalAnimationEditor();
    virtual ~EditorSkeletalAnimationEditor();

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
    bool Load(const char *animationName, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool Save(ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool SaveAs(const char *animationName, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    void Close();

    // generator
    const SkeletalAnimationGenerator *GetSkeletalAnimationGenerator() const { return m_pGenerator; }

    // update the preview for a specific frame
    const uint32 GetFrameCount() const { return m_frameTimes.GetSize(); }
    bool SetPreviewMeshName(const char *meshName);
    void SetPreviewFrameNumber(uint32 frameNumber);

    // flag for redraw
    void FlagForRedraw() { m_bRedrawPending = true; }

private:
    // resources
    bool CreateHardwareResources();
    void ReleaseHardwareResources();

    // validate the animation
    bool IsValidAnimation() const;

    // when the file name changes
    void OnFileNameChanged();

    // when the animation changes
    void UpdateTimeDependantVariables();

    // update the hierarchy panel
    void UpdateHierarchyRecursive(const Skeleton::Bone *pBone, QTreeWidgetItem *pParent);
    void UpdateHierarchy();

    // update the preview mesh transforms
    void UpdatePreviewBoneTransformRecursive(float time, const Skeleton::Bone *pBone, const Transform *pParentBoneTransform);
    void UpdatePreviewBoneTransforms();

    // update the information tab
    void UpdateInformationTab();

    // draw the scene
    void Draw();
    void DrawGrid();
    void DrawView();
    void DrawOverlays(GPUContext *pGPUContext, MiniGUIContext *pGUIContext);

    // --- ui ---
    Ui_EditorSkeletalAnimationEditor *m_ui;

    // --- editor info ---
    EditorViewController m_viewController;
    EDITOR_RENDER_MODE m_renderMode;
    uint32 m_viewportFlags;

    // --- generator ---
    SkeletalAnimationGenerator *m_pGenerator;
    String m_animationName;
    String m_animationFileName;
    const Skeleton *m_pSkeleton;
    const SkeletalMesh *m_pPreviewMesh;

    // --- information generated on demand ---
    PODArray<float> m_frameTimes;
    float m_duration;
    MemArray<Transform> m_previewBoneTransforms;
    uint32 m_previewFrameNumber;
    uint32 m_selectedBoneIndex;

    // --- render mesh ---
    RenderWorld *m_pRenderWorld;
    EditorLightSimulator *m_pLightSimulator;
    SkeletalMeshRenderProxy *m_pPreviewMeshRenderProxy;

    // --- renderer stuff ---
    GPUOutputBuffer *m_pSwapChain;
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
    void OnActionToolBoneTrackTriggered(bool checked) { SetCurrentTool((checked) ? Tool_BoneTrack : Tool_Information); }
    void OnActionToolRootMotionTriggered(bool checked) { SetCurrentTool((checked) ? Tool_RootMotion : Tool_Information); }
    void OnActionToolClipperTriggered(bool checked) { SetCurrentTool((checked) ? Tool_Clipper : Tool_Information); }
    void OnActionToolLightManipulatorTriggered(bool checked) { SetCurrentTool((checked) ? Tool_LightManipulator : Tool_Information); }
    void OnScrubberValueChanged(int value);
    void OnHierarchyItemClicked(QTreeWidgetItem *pItem, int column);
    void OnInformationPreviewMeshLinkActivated(const QString &link);
    void OnClipperSetStartFrameNumberClicked();
    void OnClipperSetEndFrameNumberClicked();
    void OnClipperClipClicked();
    void OnSwapChainWidgetResized();
    void OnSwapChainWidgetPaint();
    void OnSwapChainWidgetKeyboardEvent(const QKeyEvent *pKeyboardEvent);
    void OnSwapChainWidgetMouseEvent(const QMouseEvent *pMouseEvent);
    void OnSwapChainWidgetWheelEvent(const QWheelEvent *pWheelEvent);
    void OnSwapChainWidgetGainedFocusEvent();
    void OnFrameExecutionTriggered(float timeSinceLastFrame);
};

