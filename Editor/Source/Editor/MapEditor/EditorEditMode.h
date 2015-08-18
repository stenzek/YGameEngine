#pragma once
#include "Editor/Common.h"

class ResourceTypeInfo;
class EditorMapWindow;
class EditorMap;
class EditorMapViewport;
class MiniGUIContext;

class EditorEditMode : public QObject
{
    Q_OBJECT

public:
    EditorEditMode(EditorMap *pMap);
    virtual ~EditorEditMode();

    const EditorMap *GetEditorMap() const { return m_pMap; }
    EditorMap *GetEditorMap() { return m_pMap; }

    virtual bool Initialize(ProgressCallbacks *pProgressCallbacks);
    virtual QWidget *CreateUI(QWidget *parentWidget);

    virtual void Activate();
    virtual void Deactivate();

    virtual void Update(const float timeSinceLastUpdate);
    
    virtual void OnPropertyEditorPropertyChanged(const char *propertyName, const char *propertyValue);

    virtual void OnActiveViewportChanged(EditorMapViewport *pOldActiveViewport, EditorMapViewport *pNewActiveViewport);

    virtual void OnViewportDrawBeforeWorld(EditorMapViewport *pViewport);
    virtual void OnViewportDrawAfterWorld(EditorMapViewport *pViewport);
    virtual void OnViewportDrawAfterPost(EditorMapViewport *pViewport);
    virtual void OnPickingTextureDrawBeforeWorld(EditorMapViewport *pViewport);
    virtual void OnPickingTextureDrawAfterWorld(EditorMapViewport *pViewport);

    virtual bool HandleViewportKeyboardInputEvent(EditorMapViewport *pViewport, const QKeyEvent *pKeyboardEvent);
    virtual bool HandleViewportMouseInputEvent(EditorMapViewport *pViewport, const QMouseEvent *pMouseEvent);
    virtual bool HandleViewportWheelInputEvent(EditorMapViewport *pViewport, const QWheelEvent *pWheelEvent);

    virtual bool HandleResourceViewResourceActivatedEvent(const ResourceTypeInfo *pResourceTypeInfo, const char *resourceName);
    virtual bool HandleResourceViewResourceDroppedEvent(const ResourceTypeInfo *pResourceTypeInfo, const char *resourceName, const EditorMapViewport *pViewport, int32 x, int32 y);

protected:
    EditorMap *m_pMap;
    bool m_bActive;

private:
    // left/mouse button states
    bool m_cameraLeftMouseButtonDown;
    bool m_cameraRightMouseButtonDown;
};

