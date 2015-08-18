#pragma once
#include "Editor/Common.h"
#include "Editor/Editor.h"
#include "Editor/EditorPropertyEditorWidget.h"
#include "Editor/MapEditor/EditorMap.h"

class EditorEditMode;
class EditorMapViewport;
class EditorWorldOutlinerWidget;
class EditorResourceBrowserWidget;
struct Ui_EditorMapWindow;

class EditorMapWindow : public QMainWindow
{
    Q_OBJECT

    friend class EditorMap;

public:
    EditorMapWindow();
    ~EditorMapWindow();

    // const accessors
    const bool IsMapOpen() const { return (m_pOpenMap != NULL); }
    const EditorMap *GetMap() const { return m_pOpenMap; }

    const EditorEditMode *GetActiveEditModePointer() const { return m_pActiveEditModePointer; }
    const Ui_EditorMapWindow *GetUI() const { return m_ui; }
    const EditorWorldOutlinerWidget *GetWorldOutlinerWidget() const;
    const EditorResourceBrowserWidget *GetResourceBrowserWidget() const;
    const EditorPropertyEditorWidget *GetPropertyEditorWidget() const;

    // accessors
    EditorMap *GetMap() { return m_pOpenMap; }
    EditorEditMode *GetActiveEditModePointer() { return m_pActiveEditModePointer; }
    Ui_EditorMapWindow *GetUI() { return m_ui; }
    EditorWorldOutlinerWidget *GetWorldOutlinerWidget();
    EditorResourceBrowserWidget *GetResourceBrowserWidget();
    EditorPropertyEditorWidget *GetPropertyEditorWidget();

    // map open/close
    bool NewMap();
    bool OpenMap(const char *fileName);
    bool SaveMap();
    bool SaveMapAs(const char *newFileName);
    void CloseMap();

    // reference coordinate system
    const EDITOR_REFERENCE_COORDINATE_SYSTEM GetReferenceCoordinateSystem() const { return m_referenceCoordinateSystem; }
    void SetReferenceCoordinateSystem(EDITOR_REFERENCE_COORDINATE_SYSTEM referenceCoordinateSystem);

    // edit mode
    const EDITOR_EDIT_MODE GetEditMode() const { return m_editMode; }
    void SetEditMode(EDITOR_EDIT_MODE mode);

    // property editor management
    // todo: virtual property editor
    //void ClearPropertyEditor();
    //void BuildPropertyEditorForEntities(const uint32 *pEntities, uint32 nEntities);
    //void UpdatePropertyEditorEntity(uint32 entityId, const char *propertyName, const char *oldPropertyValue, const char *newPropertyValue);

    // viewport management
    const EditorMapViewport *GetActiveViewport() const { return m_pActiveViewport; }
    const EditorMapViewport *GetViewport(uint32 i) const { DebugAssert(i < m_viewports.GetSize()); return m_viewports[i]; }
    EditorMapViewport *GetViewport(uint32 i) { DebugAssert(i < m_viewports.GetSize()); return m_viewports[i]; }
    EditorMapViewport *GetActiveViewport() { return m_pActiveViewport; }
    uint32 GetViewportCount() const { return m_viewports.GetSize(); }
    void SetActiveViewport(EditorMapViewport *pViewport);
    void SetViewportLayout(EDITOR_VIEWPORT_LAYOUT layout);
    void RedrawAllViewports();

    // when viewport camera changes
    void OnViewportCameraChange(EditorMapViewport *pViewport);

private:
    // map
    bool OnMapOpened(ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    
    // viewports
    EditorMapViewport *CreateViewport(EDITOR_CAMERA_MODE cameraMode, EDITOR_RENDER_MODE renderMode, uint32 flags);
    void DeleteAllViewports();

    // ui pointers
    Ui_EditorMapWindow *m_ui;

    // open map
    EditorMap *m_pOpenMap;

    // reference coordinate system
    EDITOR_REFERENCE_COORDINATE_SYSTEM m_referenceCoordinateSystem;

    // edit mode
    EDITOR_EDIT_MODE m_editMode;
    EditorEditMode *m_pActiveEditModePointer;

    // viewports
    EDITOR_VIEWPORT_LAYOUT m_viewportLayout;
    typedef PODArray<EditorMapViewport *> ViewportWidgetArray;
    ViewportWidgetArray m_viewports;
    EditorMapViewport *m_pActiveViewport;

    //===================================================================================================================================================================
    // logging callback
    //===================================================================================================================================================================
    static void LogCallback(void *UserParam, const char *ChannelName, LOGLEVEL Level, const char *Message);

    //===================================================================================================================================================================
    // implemented qt methods
    //===================================================================================================================================================================
    virtual void closeEvent(QCloseEvent *pCloseEvent);

    //===================================================================================================================================================================
    // gui event callbacks
    //===================================================================================================================================================================
    void ConnectUIEvents();
    void UpdateStatusBarCameraFields();
    void UpdateStatusBarFPSField();

private Q_SLOTS:
    void OnActionFileNewMapTriggered();
    void OnActionFileOpenMapTriggered();
    void OnActionFileSaveMapTriggered();
    void OnActionFileSaveMapAsTriggered();
    void OnActionFileCloseMapTriggered();
    void OnActionFileExitTriggered();
    void OnActionEditReferenceCoordinateSystemLocalTriggered(bool checked) { SetReferenceCoordinateSystem(EDITOR_REFERENCE_COORDINATE_SYSTEM_LOCAL); }
    void OnActionEditReferenceCoordinateSystemWorldTriggered(bool checked) { SetReferenceCoordinateSystem(EDITOR_REFERENCE_COORDINATE_SYSTEM_WORLD); }
    void OnActionViewWorldOutlinerTriggered(bool checked);
    void OnActionViewResourceBrowserTriggered(bool checked);
    void OnActionViewToolboxTriggered(bool checked);
    void OnActionViewPropertyEditorTriggered(bool checked);
    void OnActionViewThemeNone() { g_pEditor->SetEditorTheme(EDITOR_THEME_NONE); }
    void OnActionViewThemeDark() { g_pEditor->SetEditorTheme(EDITOR_THEME_DARK); }
    void OnActionViewThemeDarkOther() { g_pEditor->SetEditorTheme(EDITOR_THEME_DARK_OTHER); }
    void OnActionToolsMapEditModeTriggered(bool checked) { SetEditMode(EDITOR_EDIT_MODE_MAP); }
    void OnActionToolsEntityEditModeTriggered(bool checked) { SetEditMode((checked) ? EDITOR_EDIT_MODE_ENTITY : EDITOR_EDIT_MODE_MAP); }
    void OnActionToolsGeometryEditModeTriggered(bool checked) { SetEditMode((checked) ? EDITOR_EDIT_MODE_GEOMETRY : EDITOR_EDIT_MODE_MAP); }
    void OnActionToolsHeightfieldTerrainEditModeTriggered(bool checked) { SetEditMode((checked) ? EDITOR_EDIT_MODE_TERRAIN : EDITOR_EDIT_MODE_MAP); }
    void OnActionToolsCreateTerrainTriggered();
    void OnActionToolsDeleteTerrainTriggered();
    void OnActionToolsCreateBlockTerrainTriggered();
    void OnActionToolsDeleteBlockTerrainTriggered();
    void OnStatusBarGridSnapEnabledToggled(bool checked);
    void OnStatusBarGridSnapIntervalChanged(double value);
    void OnStatusBarGridLinesVisibleToggled(bool checked);
    void OnStatusBarGridLinesIntervalChanged(double value);
    void OnWorldOutlinerEntitySelected(const EditorMapEntity *pEntity);
    void OnWorldOutlinerEntityActivated(const EditorMapEntity *pEntity);
    void OnToolboxTabWidgetCurrentChanged(int index);
    void OnPropertyEditorPropertyChanged(const char *propertyName, const char *propertyValue);
    void OnFrameExecutionTriggered(float timeSinceLastFrame);
};

