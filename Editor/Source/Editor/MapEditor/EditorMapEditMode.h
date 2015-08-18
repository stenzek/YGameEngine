#include "Editor/MapEditor/EditorEditMode.h"

struct ui_EditorMapEditMode;

class EditorMapEditMode : public EditorEditMode
{
    Q_OBJECT

public:
    EditorMapEditMode(EditorMap *pMap);
    ~EditorMapEditMode();

    // property change callback
    void OnMapPropertyChanged(const char *propertyName, const char *propertyValue);

    // implemented base methods 
    virtual bool Initialize(ProgressCallbacks *pProgressCallbacks) override;
    virtual QWidget *CreateUI(QWidget *parentWidget) override;
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

    // update the summary
    void UpdateSummary();

private:
    ui_EditorMapEditMode *m_ui;

private Q_SLOTS:
    // ui events
    void OnUIShowSkyChanged(bool checked);
    void OnUIShowSunChanged(bool checked);
};

