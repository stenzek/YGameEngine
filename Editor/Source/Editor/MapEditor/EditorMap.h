#pragma once
#include "Editor/Common.h"
#include "Editor/MapEditor/EditorMapEntity.h"
#include "MapCompiler/MapSource.h"
#include "Engine/TerrainTypes.h"

class EntityTypeInfo;
class ObjectTemplate;
class EditorMapWindow;
class EditorMapViewport;
class RendererOutputBuffer;
class MapSource;
class RenderWorld;

class TerrainManager;
class TerrainEntity;

class BlockTerrainManager;
class BlockTerrainRenderProxy;

class EditorMapEditMode;
class EditorEntityEditMode;
class EditorGeometryEditMode;
class EditorTerrainEditMode;

class StaticMeshRenderProxy;
class DirectionalLightRenderProxy;
class AmbientLightRenderProxy;

class EditorMap
{
public:
    EditorMap(EditorMapWindow *pMapWindow);
    ~EditorMap();

    // current map accessors
    const EditorMapWindow *GetMapWindow() const { return m_pMapWindow; }
    const String &GetMapFileName() const { return m_strMapFileName; }
    const MapSource *GetMapSource() const { return m_pMapSource; }
    const RenderWorld *GetRenderWorld() const { return m_pRenderWorld; }
    const AABox &GetMapBoundingBox() const { return m_mapBoundingBox; }
    EditorMapWindow *GetMapWindow() { return m_pMapWindow; }
    MapSource *GetMapSource() { return m_pMapSource; }
    RenderWorld *GetRenderWorld() { return m_pRenderWorld; }

    // grid options
    const bool GetGridLinesVisible() const { return m_gridLinesVisible; }
    const float GetGridLinesInterval() const { return m_gridLinesInterval; }
    const bool GetGridSnapEnabled() const { return m_gridSnapEnabled; }
    const float GetGridSnapInterval() const { return m_gridSnapInterval; }
    void SetGridLinesVisible(bool visible) { m_gridLinesVisible = visible; RedrawAllViewports(); }
    void SetGridLinesInterval(float interval) { m_gridLinesInterval = interval; RedrawAllViewports(); }
    void SetGridSnapEnabled(bool enabled) { m_gridSnapEnabled = enabled; }
    void SetGridSnapInterval(float interval) { m_gridSnapInterval = interval; }

    // creates a map with the specified name in the specified location.
    bool CreateMap();

    // open a map.
    bool OpenMap(const char *FileName, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // saves the currently open map. if NewFileName is null, the existing filename is used.
    bool SaveMap(const char *NewFileName = NULL, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // map properties
    const String &GetMapProperty(const char *propertyName);
    void SetMapProperty(const char *propertyName, const char *propertyValue);

    // adding/removing entities
    EditorMapEntity *CreateEntity(const ObjectTemplate *pTemplate, const char *entityName = nullptr);
    EditorMapEntity *CreateEntity(const char *entityTypeName, const char *entityName = nullptr);
    void DeleteEntity(EditorMapEntity *pEntity);

    // copy an entity
    EditorMapEntity *CopyEntity(const EditorMapEntity *pEntity, const char *newName = nullptr);

    // map entity access
    const EditorMapEntity *GetEntityByArrayIndex(uint32 indexID) const;
    const EditorMapEntity *GetEntityByName(const char *entityName) const;
    EditorMapEntity *GetEntityByArrayIndex(uint32 indexID);
    EditorMapEntity *GetEntityByName(const char *entityName);
    uint32 GetEntityArraySize() const { return m_entities.GetSize(); }

    // when an entity property changes
    void OnEntityPropertyChanged(const EditorMapEntity *pEntity, const char *propertyName, const char *propertyValue);

    // when an entity transform changes
    void OnEntityBoundsChanged(const EditorMapEntity *pEntity);

    // merge map bounds with the specified bounds
    void MergeMapBoundingBox(const AABox &box);

    // when the map bounds change
    void OnMapBoundingBoxChange();

    // entity enumeration
    template<typename T>
    void EnumerateEntities(T callback) const
    {
        for (uint32 i = 0; i < m_entities.GetSize(); i++)
        {
            if (m_entities[i] != nullptr)
                callback(m_entities[i]);
        }
    }
    template<typename T>
    void EnumerateEntitiesWithVisuals(T callback) const
    {
        for (uint32 i = 0; i < m_entities.GetSize(); i++)
        {
            if (m_entities[i] != nullptr && m_entities[i]->IsVisualCreated())
                callback(m_entities[i]);
        }
    }

    // entity enumeration in space
    template<typename T>
    void EnumerateEntitiesInAABox(const AABox &box, T callback) const
    {
        for (uint32 i = 0; i < m_entities.GetSize(); i++)
        {
            if (m_entities[i] != nullptr && m_entities[i]->GetBoundingBox().AABoxIntersection(box))
                callback(m_entities[i]);
        }
    }
    template<typename T>
    void EnumerateEntitiesInSphere(const Sphere &sphere, T callback) const
    {
        for (uint32 i = 0; i < m_entities.GetSize(); i++)
        {
            if (m_entities[i] != nullptr && m_entities[i]->GetBoundingSphere().SphereIntersection(sphere))
                callback(m_entities[i]);
        }
    }

//     // entity properties
//     const MapSourceEntityData *GetEntityData(uint32 entityId) const;
//     String GetEntityPropertyValue(uint32 entityId, const char *propertyName) const;
//     bool SetEntityPropertyValue(uint32 entityId, const char *propertyName, const char *propertyValue);

    // edit mode
    const EditorMapEditMode *GetMapEditMode() const { return m_pMapEditMode; }
    const EditorEntityEditMode *GetEntityEditMode() const { return m_pEntityEditMode; }
    const EditorGeometryEditMode *GetGeometryEditMode() const { return m_pGeometryEditMode; }
    const EditorTerrainEditMode *GetTerrainEditMode() const { return m_pTerrainEditMode; }
    EditorMapEditMode *GetMapEditMode() { return m_pMapEditMode; }
    EditorEntityEditMode *GetEntityEditMode() { return m_pEntityEditMode; }
    EditorGeometryEditMode *GetGeometryEditMode() { return m_pGeometryEditMode; }
    EditorTerrainEditMode *GetTerrainEditMode() { return m_pTerrainEditMode; }

    // update, call every frame
    void Update(float timeSinceLastUpdate);

    // queue refresh of visible entities
    void OnViewportCameraChange() { m_entityVisualUpdatePending = true; }

    // queue redraw of all attached viewports
    void RedrawAllViewports();

    // terrain accessors
    const bool HasTerrain() const { return (m_pTerrainEditMode != nullptr); }

    // terrain creation
    bool CreateTerrain(const char *importLayerListName, TERRAIN_HEIGHT_STORAGE_FORMAT heightStorageFormat, uint32 scale, uint32 sectionSize, uint32 renderLODCount, int32 minHeight, int32 maxHeight, int32 baseHeight);
    void DeleteTerrain();

    // skybox accessors
    const bool GetSkyVisibility() const { return m_skyVisibility; }
    void SetSkyVisibility(bool enabled);

    // sun accessors
    const bool GetSunVisibility() const { return m_sunVisibility; }
    void SetSunVisibility(bool enabled);

private:
    bool Initialize(ProgressCallbacks *pProgressCallbacks);

    // create the visual entiies
    bool InitializeEntities(ProgressCallbacks *pProgressCallbacks);
    bool InitializeTerrain(ProgressCallbacks *pProgressCallbacks);

    // visual creator
    EditorMapEntity *CreateInternalEntity(MapSourceEntityData *pEntityData);

    // test whether an entity is in visual range
    bool IsEntityInVisibleRange(const EditorMapEntity *pEntity) const;

    // update entity visuals streaming
    void UpdateEntityVisuals();

    // hook for map property changing
    void OnMapPropertyChange(const char *propertyName, const char *propertyValue);

    // update skybox variables
    void UpdateSkyVariables();
    void UpdateSkyTransform();

    // update sun variables
    void UpdateSunVariables();

    // currently open map
    EditorMapWindow *m_pMapWindow;
    String m_strMapFileName;
    MapSource *m_pMapSource;

    // grid options
    float m_gridLinesInterval;
    float m_gridSnapInterval;
    bool m_gridLinesVisible;
    bool m_gridSnapEnabled;

    // visual world
    RenderWorld *m_pRenderWorld;

    // map bounds, not necessarily accurate
    AABox m_mapBoundingBox;

    // visuals
    MemArray<EditorMapEntity *> m_entities;
    bool m_entityVisualUpdatePending;

    // edit mode instances
    EditorMapEditMode *m_pMapEditMode;
    EditorEntityEditMode *m_pEntityEditMode;
    EditorGeometryEditMode *m_pGeometryEditMode;

    // terrain
    EditorTerrainEditMode *m_pTerrainEditMode;

    // skybox
    bool m_skyVisibility;
    bool m_skyAutoSize;
    float m_skySize;
    float m_skyGroundHeight;
    StaticMeshRenderProxy *m_pSkyRenderProxy;

    // sun
    bool m_sunVisibility;
    DirectionalLightRenderProxy *m_pSunRenderProxy;
    AmbientLightRenderProxy *m_pSunAmbientRenderProxy;
};
