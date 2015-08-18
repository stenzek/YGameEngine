#pragma once
#include "Engine/Common.h"
#include "Engine/DynamicWorld.h"
#include "Engine/TerrainTypes.h"

class Camera;
class MapRegion;
class ZipArchive;
class ClassTable;

namespace Physics { class StaticObject; }

class TerrainLayerList;
class TerrainSection;
class TerrainSectionCollisionShape;
class TerrainSectionRenderProxy;
class TerrainRenderer;

class Map
{
    friend class MapRegion;
public:
    Map();
    virtual ~Map();

    // accessors
    const String &GetMapName() const { return m_mapName; }
    const AABox &GetMapBounds() const { return m_mapBounds; }
    const uint32 GetRegionSize() const { return m_regionSize; }
    const uint32 GetLoadRadius() const { return m_regionLoadRadius; }
    const uint32 GetActivationRadius() const { return m_regionActivationRadius; }
    const World *GetWorld() const { return m_pWorld; }

    // terrain
    const TerrainParameters *GetTerrainParameters() const { return &m_terrainParameters; }
    const TerrainLayerList *GetTerrainManager() const { return m_pTerrainLayerList; }
    const TerrainRenderer *GetTerrainRenderer() const { return m_pTerrainRenderer; }
    TerrainRenderer *GetTerrainRenderer() { return m_pTerrainRenderer; }

    // non-const accessors
    DynamicWorld *GetWorld() { return m_pWorld; }

    // helper methods
    const int2 GetRegionForPosition(const float3 &position) const;

    // World entity id tracking
    const uint32 GetWorldEntityIdForMapEntityName(const char *mapEntityName);

    // Loading of map directory
    bool LoadMap(const char *mapName, const float3 *pInitialPositionOverride = nullptr, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // Region management
    bool LoadRegion(int32 regionX, int32 regionY, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool ChangeRegionLOD(int32 regionX, int32 regionY, int32 newLOD, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    void UnloadRegion(int32 regionX, int32 regionY);
    void LoadAllRegions(ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    void UnloadAllRegions();

    // Streaming
    void HandleStreaming(ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

private:
    bool LoadRegionsHeader(ProgressCallbacks *pProgressCallbacks);
    bool LoadTerrainHeader(ProgressCallbacks *pProgressCallbacks);
    bool LoadGlobalEntities(ProgressCallbacks *pProgressCallbacks);
    
    uint32 CreateEntitiesFromStream(ByteStream *pStream, uint32 entityCount, uint32 entityDataSize, PODArray<uint32> *pOutDynamicEntityIDArray, PODArray<Brush *> *pOutStaticObjectsArray, ProgressCallbacks *pProgressCallbacks);

    String m_mapName;
    ZipArchive *m_pMapArchive;
    ClassTable *m_pClassTable;

    AABox m_mapBounds;
    uint32 m_regionSize;
    uint32 m_regionLODLevels;
    uint32 m_regionLoadRadius;
    uint32 m_regionActivationRadius;

    typedef HashTable<int2, MapRegion *> MapRegionTable;
    MapRegionTable m_regions;

    DynamicWorld *m_pWorld;

    CIStringHashTable<uint32> m_mapEntityNameMapping;

    // TODO: Global static entities, load on camera distance < threshold

    // terrain
    TerrainParameters m_terrainParameters;
    const TerrainLayerList *m_pTerrainLayerList;
    TerrainRenderer *m_pTerrainRenderer;
};

class MapRegion
{
public:
    MapRegion(Map *pMap, int32 regionX, int32 regionY);
    ~MapRegion();

    const int32 GetRegionX() const { return m_regionX; }
    const int32 GetRegionY() const { return m_regionY; }
    const int32 GetLoadedLODLevel() const { return m_loadedLODLevel; }
    const bool IsLoaded() const { return (m_loadedLODLevel >= 0); }

    bool LoadRegion(ByteStream *pStream, uint32 lodLevel, ProgressCallbacks *pProgressCallbacks);
    void UnloadRegion();

private:
    Map *m_pMap;
    int32 m_regionX, m_regionY;
    int32 m_loadedLODLevel;
    
    PODArray<uint32> m_loadedEntityIDs;
    PODArray<Brush *> m_loadedStaticObjects;

    struct RegionTerrainSection
    {
        TerrainSection *pData;
        TerrainSectionCollisionShape *pCollisionShape;
        Physics::StaticObject *pCollisionObject;
        TerrainSectionRenderProxy *pRenderProxy;
    };
    MemArray<RegionTerrainSection> m_terrainSections;
};

