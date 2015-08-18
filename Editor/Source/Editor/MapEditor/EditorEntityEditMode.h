#include "Editor/MapEditor/EditorEditMode.h"

class Texture2D;
class EditorMapViewport;
class EditorMapEntity;
struct ui_EditorEntityEditMode;

class EditorEntityEditMode : public EditorEditMode
{
    Q_OBJECT

public:
    EditorEntityEditMode(EditorMap *pMap);
    ~EditorEntityEditMode();

    // widget
    const EDITOR_ENTITY_EDIT_MODE_WIDGET GetActiveWidget() const { return m_activeWidget; }
    void SetActiveWidget(EDITOR_ENTITY_EDIT_MODE_WIDGET widget);

    // selection reading
    const AABox &GetSelectionBounds() const { return m_selectionBounds; }
    const uint32 GetSelectionCount() const { return m_selectedEntities.GetSize(); }
    const EditorMapEntity *GetSelection(uint32 i) const { DebugAssert(i < m_selectedEntities.GetSize()); return m_selectedEntities[i]; }
    EditorMapEntity *GetSelection(uint32 i) { DebugAssert(i < m_selectedEntities.GetSize()); return m_selectedEntities[i]; }

    // selection enumeration
    template<typename T>
    void EnumerateSelection(T callback) const
    {
        for (uint32 i = 0; i < m_selectedEntities.GetSize(); i++)
            callback(m_selectedEntities[i]);
    }

    // any modifications should be done through here to preserve the UI
    void OnEntityPropertyChanged(const EditorMapEntity *pEntity, const char *propertyName, const char *propertyValue);

    // set property on selection
    void SetPropertyOnSelection(const char *propertyName, const char *propertyValue);

    // add/remove selection
    void AddSelection(EditorMapEntity *pEntity);
    void RemoveSelection(EditorMapEntity *pEntity);
    void ClearSelection();

    // select a component
    void SetSelectedComponentIndex(int32 componentIndex);

    // selection queries
    bool IsSelected(const EditorMapEntity *pEntity) const;

    // selection operations
    void MoveSelection(const float3 &moveDelta);
    void RotateSelection(const float3 &rotateDelta);
    void ScaleSelection(const float3 &scaleDelta);
    void PushSelection(const float3 &pushDirection, float maxDistance = 100.0f);
    void SnapSelectionToGrid();
    void DeleteSelection();

    // create a copy of the selection, and optionally change the active selection, and deselect the old selection
    void CloneSelection(bool addToSelection, bool clearOldSelection);

    // helper method for creating a entity in a viewport
    EditorMapEntity *CreateEntityUsingViewport(EditorMapViewport *pViewport, const char *typeName);

    // helper method to move a viewport camera in range of an entity
    void MoveViewportCameraToEntity(EditorMapViewport *pViewport, const EditorMapEntity *pEntity, const Quaternion cameraRotation = Quaternion::FromEulerAngles(-45.0f, 0.0f, 0.0f));

    // update selection axis position/rotation
    void UpdateSelectionAxis();

    // implemented base methods 
    virtual bool Initialize(ProgressCallbacks *pProgressCallbacks) override;
    virtual QWidget *CreateUI(QWidget *pParentWidget) override;
    virtual void Activate() override;
    virtual void Deactivate() override;
    virtual void Update(const float timeSinceLastUpdate) override;
    virtual void OnPropertyEditorPropertyChanged(const char *propertyName, const char *propertyValue) override;
    virtual void OnActiveViewportChanged(EditorMapViewport *pOldActiveViewport, EditorMapViewport *pNewActiveViewport) override;
    virtual void OnViewportDrawBeforeWorld(EditorMapViewport *pViewport) override;
    virtual void OnViewportDrawAfterWorld(EditorMapViewport *pViewport) override;
    virtual void OnViewportDrawAfterPost(EditorMapViewport *pViewport) override;
    virtual void OnPickingTextureDrawBeforeWorld(EditorMapViewport *pViewport) override;
    virtual void OnPickingTextureDrawAfterWorld(EditorMapViewport *pViewport) override;
    virtual bool HandleViewportKeyboardInputEvent(EditorMapViewport *pViewport, const QKeyEvent *pKeyboardEvent) override;
    virtual bool HandleViewportMouseInputEvent(EditorMapViewport *pViewport, const QMouseEvent *pMouseEvent) override;
    virtual bool HandleViewportWheelInputEvent(EditorMapViewport *pViewport, const QWheelEvent *pWheelEvent) override;
    virtual bool HandleResourceViewResourceActivatedEvent(const ResourceTypeInfo *pResourceTypeInfo, const char *resourceName) override;
    virtual bool HandleResourceViewResourceDroppedEvent(const ResourceTypeInfo *pResourceTypeInfo, const char *resourceName, const EditorMapViewport *pViewport, int32 x, int32 y) override;

private:
    enum STATE
    {
        STATE_NONE,
        STATE_CAMERA_ROTATION,
        STATE_SELECTION_RECT,
        STATE_WIDGET,
    };

    // draw move gismo
    void DrawMoveGismo(EditorMapViewport *pViewport, const uint32 XAxisColor, const uint32 YAxisColor, const uint32 ZAxisColor, const uint32 anyAxisColor, const uint32 XPlaneFillColor, const uint32 YPlaneFillColor, const uint32 ZPlaneFillColor, int32 onlyAxis = -1);

    // draw rotate gismo
    void DrawRotationGismo(EditorMapViewport *pViewport, const uint32 XAxisColor, const uint32 YAxisColor, const uint32 ZAxisColor, bool transparent = true, int32 onlyAxis = -1);

    // draw scale gismo
    void DrawScaleGismo(EditorMapViewport *pViewport, const uint32 XAxisColor, const uint32 YAxisColor, const uint32 ZAxisColor, const uint32 anyAxisColor, int32 onlyAxis = -1);

    // update the selections
    void UpdateSelection(EditorMapViewport *pViewport);

    // update the axis position
    void UpdateSelectionBounds();

    // update the property editor for the current selection
    void UpdatePropertyEditor();
    void UpdatePropertyEditorForComponent();

    // ui
    ui_EditorEntityEditMode *m_ui;

    // resources
    const Texture2D *m_pRotationGismoTexture;
    const Texture2D *m_pRotationGismoTransparentTexture;

    // widget
    EDITOR_ENTITY_EDIT_MODE_WIDGET m_activeWidget;

    // state
    uint32 m_lastMousePositionEntityId;
    int2 m_mouseSelectionRegion[2];
    STATE m_currentState;
    uint32 m_currentStateData[2];
    float3 m_currentStateDataVector;

    // selection
    AABox m_selectionBounds;
    PODArray<EditorMapEntity *> m_selectedEntities;
    int32 m_selectedComponentIndex;
    float3 m_selectionAxisPosition;
    Quaternion m_selectionAxisRotation;

private Q_SLOTS:
    // ui events
    void OnUIOptionsFilterSelectionClicked();
    void OnUIWidgetSelectClicked(bool checked);
    void OnUIWidgetMoveClicked(bool checked);
    void OnUIWidgetRotateClicked(bool checked);
    void OnUIWidgetScaleClicked(bool checked);
    void OnUIPushLeftClicked();
    void OnUIPushRightClicked();
    void OnUIPushForwardClicked();
    void OnUIPushBackClicked();
    void OnUIPushUpClicked();
    void OnUIPushDownClicked();
    void OnUIActionCreateEntityTriggered();
    void OnUIActionDeleteEntityTriggered();
    void OnUIActionEntityLayersTriggered();
    void OnUIActionSnapSelectionToGridTriggered();
    void OnUIActionCreateComponentTriggered();
    void OnUIActionRemoveComponentTriggered();
    void OnUIActionSelectionLayersTriggered();
    void OnUISelectionComponentListCurrentRowChanged(int currentRow);
};

