#include "Editor/MapEditor/EditorEditMode.h"

class EditorGeometryEditMode : public EditorEditMode
{
    Q_OBJECT

public:
    EditorGeometryEditMode(EditorMap *pMap);
    ~EditorGeometryEditMode();

    // csg compiler
    //const CSGCompiler *GetCSGCompiler() const { return m_pCSGCompiler; }
    //CSGCompiler *GetCSGCompiler() { return m_pCSGCompiler; }

    // builder brush constructors
    void SetBuilderBrushCube(const float &Radius);

    // csg operations
    void PerformCSGAdd();
    void PerformCSGSubtract();
    void PerformCSGIntersection();
    void PerformCSGDeintersection();

    // implemented base methods 
    virtual bool Initialize(ProgressCallbacks *pProgressCallbacks);
    virtual void Activate();
    virtual void Deactivate();
    virtual void Update(const float timeSinceLastUpdate);
    virtual void OnActiveViewportChanged(EditorMapViewport *pOldActiveViewport, EditorMapViewport *pNewActiveViewport);
    virtual void OnViewportDrawBeforeWorld(EditorMapViewport *pViewport);
    virtual void OnViewportDrawAfterWorld(EditorMapViewport *pViewport);
    virtual void OnViewportDrawAfterPost(EditorMapViewport *pViewport);
    virtual void OnPickingTextureDrawBeforeWorld(EditorMapViewport *pViewport);
    virtual void OnPickingTextureDrawAfterWorld(EditorMapViewport *pViewport);
    virtual bool HandleViewportKeyboardInputEvent(EditorMapViewport *pViewport, const QKeyEvent *pKeyboardEvent) override;
    virtual bool HandleViewportMouseInputEvent(EditorMapViewport *pViewport, const QMouseEvent *pMouseEvent) override;
    virtual bool HandleViewportWheelInputEvent(EditorMapViewport *pViewport, const QWheelEvent *pWheelEvent) override;

private:
    //CSGCompiler *m_pCSGCompiler;

    // editor entities
    //CSGCompilerBrush *m_pBuilderBrush;
    //EditorBuilderBrushEntity *m_pBuilderBrushEntity;
};

