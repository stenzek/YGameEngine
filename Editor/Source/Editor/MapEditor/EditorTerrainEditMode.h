#pragma once
#include "Editor/MapEditor/EditorEditMode.h"
#include "MapCompiler/MapSourceTerrainData.h"
#include "Renderer/VertexFactories/PlainVertexFactory.h"

class Image;
class CompositeRenderProxy;
class TerrainLayerListGenerator;
class TerrainRenderer;
class TerrainSection;
class TerrainSectionRenderProxy;
class StaticMesh;
class Material;
struct ui_EditorTerrainEditMode;

class EditorTerrainEditMode : public EditorEditMode, private MapSourceTerrainData::EditCallbacks
{
    Q_OBJECT

public:
    // helper function to create the default layer list
    static TerrainLayerListGenerator *CreateEmptyLayerList();

public:
    EditorTerrainEditMode(EditorMap *pMap);
    ~EditorTerrainEditMode();

    // widget
    const EDITOR_TERRAIN_EDIT_MODE_WIDGET GetActiveWidget() const { return m_eActiveWidget; }
    void SetActiveWidget(EDITOR_TERRAIN_EDIT_MODE_WIDGET widget);

    // brush settings
    const float3 &GetBrushColor() const { return m_brushColor; }
    const float GetBrushRadius() const { return m_brushRadius; }
    const float GetBrushFalloff() const { return m_brushFalloff; }
    const float GetBrushHeightStep() const { return m_brushTerrainStep; }
    const float GetBrushLayerStep() const { return m_brushLayerStep; }
    const int32 GetBrushLayerSelectedLayer() const { return m_brushLayerSelectedBaseLayer; }
    void SetBrushColor(const float3 &color);
    void SetBrushRadius(float radius);
    void SetBrushFalloff(float falloff);
    void SetBrushHeightStep(float heightStep) { m_brushTerrainStep = heightStep; }
    void SetBrushLayerStep(float layerStep) { m_brushLayerStep = layerStep; }
    void SetBrushLayerSelectedLayer(int32 selectedLayer);

    // implemented base methods 
    virtual bool Initialize(ProgressCallbacks *pProgressCallbacks) override;
    virtual QWidget *CreateUI(QWidget *parentWidget) override;
    virtual void Activate() override;
    virtual void Deactivate() override;
    virtual void Update(const float timeSinceLastUpdate) override;
    virtual void OnActiveViewportChanged(EditorMapViewport *pOldActiveViewport, EditorMapViewport *pNewActiveViewport) override;
    virtual void OnViewportDrawBeforeWorld(EditorMapViewport *pViewport) override;
    virtual void OnViewportDrawAfterWorld(EditorMapViewport *pViewport) override;
    virtual void OnViewportDrawAfterPost(EditorMapViewport *pViewport) override;
    virtual void OnPickingTextureDrawBeforeWorld(EditorMapViewport *pViewport) override;
    virtual void OnPickingTextureDrawAfterWorld(EditorMapViewport *pViewport) override;
    virtual bool HandleViewportKeyboardInputEvent(EditorMapViewport *pViewport, const QKeyEvent *pKeyboardEvent) override;
    virtual bool HandleViewportMouseInputEvent(EditorMapViewport *pViewport, const QMouseEvent *pMouseEvent) override;
    virtual bool HandleViewportWheelInputEvent(EditorMapViewport *pViewport, const QWheelEvent *pWheelEvent) override;

    // import a heightmap
    bool ImportHeightmap(const Image *pHeightmap, int32 startSectionX, int32 startSectionY, float minHeight, float maxHeight, MapSourceTerrainData::HeightmapImportScaleType scaleType, uint32 scaleAmount, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // rebuild the entire terrain's quadtree
    bool RebuildQuadTree(uint32 newLodCount, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // edit methods
    float GetPointHeight(int32 pointX, int32 pointY) { return m_pTerrainData->GetPointHeight(pointX, pointY); }
    float GetPointLayerWeight(int32 pointX, int32 pointY, uint8 layer) { return m_pTerrainData->GetPointLayerWeight(pointX, pointY, layer); }
    bool SetPointHeight(int32 pointX, int32 pointY, float height) { return m_pTerrainData->SetPointHeight(pointX, pointY, height); }
    bool AddPointHeight(int32 pointX, int32 pointY, float mod) { return m_pTerrainData->AddPointHeight(pointX, pointY, mod); }
    bool SetPointLayerWeight(int32 pointX, int32 pointY, uint8 layer, float weight) { return m_pTerrainData->SetPointLayerWeight(pointX, pointY, layer, weight); }
    bool AddPointLayerWeight(int32 pointX, int32 pointY, uint8 layer, float amount) { return m_pTerrainData->AddPointLayerWeight(pointX, pointY, layer, amount); }

    // create a section
    bool CreateSection(int32 sectionX, int32 sectionY, float createHeight, uint8 createLayer);

    // delete a section
    bool DeleteSection(int32 sectionX, int32 sectionY);

private:
    // MapSourceTerrainData::EditCallbacks interface
    virtual void OnSectionCreated(int32 sectionX, int32 sectionY);
    virtual void OnSectionDeleted(int32 sectionX, int32 sectionY);
    virtual void OnSectionLoaded(const TerrainSection *pSection);
    virtual void OnSectionUnloaded(const TerrainSection *pSection);
    virtual void OnSectionLayersModified(const TerrainSection *pSection);
    virtual void OnSectionPointHeightModified(const TerrainSection *pSection, uint32 offsetX, uint32 offsetY);
    virtual void OnSectionPointLayersModified(const TerrainSection *pSection, uint32 offsetX, uint32 offsetY);

private:
    void ClearState();

    void UpdateTerrainCoordinates(EditorMapViewport *pViewport, const int2 &mousePosition, bool sectionOnly);
    void UpdateBrushVisual();

    // update the layer list for edit details
    void UpdateDetailLayerList();

    // painting
    float GetBrushPaintAmount(int32 pointX, int32 pointY) const;
    bool BeginPaint();
    void UpdatePaint();
    void EndPaint();

    // update ui with selected section
    void UpdateUIForSelectedSection();

    void DrawPostWorldOverlaysTerrainWidget(EditorMapViewport *pViewport);
    void DrawPostWorldOverlaysSectionWidget(EditorMapViewport *pViewport);

    enum STATE
    {
        STATE_NONE,
        STATE_PAINT_TERRAIN_HEIGHT,
        STATE_PAINT_TERRAIN_LAYER,
        STATE_CAMERA_ROTATION,
    };

    // ui
    ui_EditorTerrainEditMode *m_ui;

    // terrain handles
    MapSourceTerrainData *m_pTerrainData;
    TerrainRenderer *m_pTerrainRenderer;

    // helper objects
    MemArray<PlainVertexFactory::Vertex> m_brushVertices;
    Material *m_pBrushOverlayMaterial;

    // active widget
    EDITOR_TERRAIN_EDIT_MODE_WIDGET m_eActiveWidget;

    // brush settings
    //BRUSH_TYPE m_brushType;
    float3 m_brushColor;
    float m_brushRadius;
    float m_brushFalloff;
    //float m_brushSpeed;
    float m_brushTerrainStep;
    float m_brushTerrainMinHeight;
    float m_brushTerrainMaxHeight;
    float m_brushLayerStep;
    int32 m_brushLayerSelectedBaseLayer;

    // state
    STATE m_eCurrentState;

    // any mode that requires a brush
    float3 m_mouseRayHitLocation;
    int2 m_mouseOverClosestPoint;
    float3 m_mouseOverClosestPointPosition;
    bool m_validMouseOverPosition;
    
    // section edit mode -- the section the mouse is under
    int2 m_mouseOverSection;
    bool m_validMouseOverSection;
    int2 m_selectedSection;
    bool m_validSelectedSection;

    //////////////////////////////////////////////////////////////////////////
    // Section Render Proxy Management
    //////////////////////////////////////////////////////////////////////////
    TerrainSectionRenderProxy *GetSectionRenderProxy(const TerrainSection *pSection);
    TerrainSectionRenderProxy *GetSectionRenderProxy(int32 sectionX, int32 sectionY);
    bool CreateSectionRenderProxy(int32 sectionX, int32 sectionY);
    void DeleteSectionRenderProxy(int32 sectionX, int32 sectionY);
    bool CreateSectionRenderProxies();
    void DeleteSectionRenderProxies();

    PODArray<TerrainSectionRenderProxy *> m_sectionRenderProxies;

private Q_SLOTS:
    // ui events
    void OnUIWidgetEditTerrainClicked(bool checked);
    void OnUIWidgetEditDetailsClicked(bool checked);
    void OnUIWidgetEditHolesClicked(bool checked);
    void OnUIWidgetEditSectionsClicked(bool checked);
    void OnUIWidgetEditLayersClicked(bool checked);
    void OnUIWidgetImportHeightmapClicked(bool checked);
    void OnUIWidgetSettingsClicked(bool checked);
    void OnUIBrushRadiusChanged(double value);
    void OnUIBrushFalloffChanged(double value);
    void OnUIBrushInvertChanged(bool checked);
    void OnUITerrainStepHeightChanged(double value);
    void OnUILayersStepWeightChanged(double value);
    void OnUILayersLayerListItemClicked(QListWidgetItem *pListItem);
    void OnUILayersEditLayerListClicked();
    void OnUISectionsActiveSectionDeleteLayersClicked();
    void OnUISectionsActiveSectionDeleteSectionClicked();
    void OnUISectionsInactiveSectionCreateSectionClicked();
    void OnUIHeightmapImportPanelSourceTypeClicked(bool checked);
    void OnUIHeightmapImportPanelSelectSourceImageClicked();
    void OnUIHeightmapImportPanelScaleAmountChanged(int value);
    void OnUIHeightmapImportPanelScaleTypeClicked(bool checked);
    void OnUIHeightmapImportPanelImportClicked();
    void OnUISettingsPanelViewDistanceValueChanged(int value);
    void OnUISettingsPanelRenderResolutionMultiplierChanged(int value);
    void OnUISettingsPanelLODDistanceRatioChanged(double value);
    void OnUISettingsPanelRebuildQuadtreeClicked();
};

