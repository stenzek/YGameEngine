#include "Engine/PrecompiledHeader.h"
#include "Engine/Map.h"
#include "Engine/EngineCVars.h"
#include "Engine/DataFormats.h"
#include "Engine/ResourceManager.h"
#include "Engine/Camera.h"
#include "Engine/TerrainSection.h"
#include "Engine/TerrainSectionCollisionShape.h"
#include "Engine/TerrainRenderer.h"
#include "Engine/World.h"
#include "Engine/Entity.h"
#include "Engine/Brush.h"
#include "Engine/Component.h"
#include "Engine/Physics/StaticObject.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/StaticMesh.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderWorld.h"
#include "Core/ClassTable.h"
#include "YBaseLib/ZipArchive.h"
Log_SetChannel(Map);

Map::Map()
    : m_pMapArchive(nullptr),
      m_pClassTable(nullptr),
      m_pWorld(nullptr),
      m_pTerrainLayerList(nullptr),
      m_pTerrainRenderer(nullptr)
{

}

Map::~Map()
{
    UnloadAllRegions();

    // we don't call the remove method for terrain entities since they'll be dropped in the remove queue
    // anyway, but they'll be cleaned up when the world is deleted

    delete m_pWorld;

    if (m_pTerrainRenderer != nullptr)
        delete m_pTerrainRenderer;

    if (m_pTerrainLayerList != nullptr)
        m_pTerrainLayerList->Release();

    delete m_pClassTable;
    
    delete m_pMapArchive;
}

const int2 Map::GetRegionForPosition(const float3 &position) const
{
    return int2((Math::Truncate(position.x) / (int32)m_regionSize) - ((position.x < 0.0f) ? 1 : 0),
                (Math::Truncate(position.y) / (int32)m_regionSize) - ((position.y < 0.0f) ? 1 : 0));
}

const uint32 Map::GetWorldEntityIdForMapEntityName(const char *mapEntityName)
{
    CIStringHashTable<uint32>::Member *pMember = m_mapEntityNameMapping.Find(mapEntityName);
    return (pMember != nullptr) ? pMember->Value : 0;
}

bool Map::LoadMap(const char *mapName, const float3 *pInitialPositionOverride, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    PathString fileName;
    PathString resourceName;
    AutoReleasePtr<ByteStream> pArchiveStream;
    Timer loadTimer;

    pProgressCallbacks->SetCancellable(false);
    pProgressCallbacks->SetStatusText("Loading map...");
    pProgressCallbacks->SetProgressRange(1);
    pProgressCallbacks->SetProgressValue(0);

    fileName.Format("%s.cmap", mapName);
    if (!g_pVirtualFileSystem->GetFileName(fileName) ||
        (pArchiveStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE)) == NULL ||
        (m_pMapArchive = ZipArchive::OpenArchiveReadOnly(pArchiveStream)) == NULL)
    {
        Log_ErrorPrintf("Map::LoadMap: Could not open file '%s'", fileName.GetCharArray());
        return false;
    }

    // get map name
    m_mapName = fileName.SubString(0, -5);

    // map header
    DF_MAP_HEADER mapHeader;
    {
        AutoReleasePtr<ByteStream> pStream = m_pMapArchive->OpenFile(DF_MAP_HEADER_FILENAME, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
        if (pStream == nullptr)
        {
            Log_ErrorPrintf("Map::LoadMap: Could not open " DF_MAP_HEADER_FILENAME " in zipfile '%s'", fileName.GetCharArray());
            return false;
        }

        // parse the map header
        if (!pStream->Read2(&mapHeader, sizeof(mapHeader)) ||
            mapHeader.Version != DF_MAP_HEADER_VERSION ||
            mapHeader.HeaderSize != sizeof(mapHeader))
        {
            Log_ErrorPrintf("Map::LoadMap: Invalid header");
            return false;
        }
    }

    // load the class table
    {
        AutoReleasePtr<ByteStream> pStream = m_pMapArchive->OpenFile(DF_MAP_CLASS_TABLE_FILENAME, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
        if (pStream == nullptr)
        {
            Log_ErrorPrintf("Map::LoadMap: Could not open " DF_MAP_CLASS_TABLE_FILENAME " in zipfile '%s'", fileName.GetCharArray());
            return false;
        }

        m_pClassTable = new ClassTable();
        if (!m_pClassTable->LoadFromStream(pStream, false))
        {
            Log_ErrorPrintf("Map::LoadMap: Failed to load class table in map '%s'.", fileName.GetCharArray());
            return false;
        }
    }

    // create world
    m_pWorld = new DynamicWorld();

    // initialize fields
    m_regionSize = mapHeader.RegionSize;
    m_regionLODLevels = mapHeader.RegionLODLevels;
    m_regionLoadRadius = mapHeader.RegionLoadRadius;
    m_regionActivationRadius = mapHeader.RegionActivationRadius;
    m_mapBounds.SetBounds(float3(mapHeader.WorldBoundingBoxMin), float3(mapHeader.WorldBoundingBoxMax));

    // log messages
    Log_InfoPrintf("Map::LoadMap: Map '%s' starting creation, bounds are (%f, %f, %f - %f %f %f)", mapName,
                   m_mapBounds.GetMinBounds().x, m_mapBounds.GetMinBounds().y, m_mapBounds.GetMinBounds().z, 
                   m_mapBounds.GetMaxBounds().x, m_mapBounds.GetMaxBounds().y, m_mapBounds.GetMaxBounds().z);
    Log_DevPrintf("Region size is %u units", m_regionSize);
    Log_DevPrintf("Map has %u levels of detail per region", m_regionLODLevels);
    Log_DevPrintf("Loading radius is %u regions", m_regionLoadRadius);
    Log_DevPrintf("Activation radius is %u regions", m_regionActivationRadius);

    // has regions?
    if (mapHeader.HasRegions)
    {
        pProgressCallbacks->SetStatusText("Loading regions...");
        if (!LoadRegionsHeader(pProgressCallbacks))
            return false;
    }

    // has terrain?
    if (mapHeader.HasTerrain)
    {
        pProgressCallbacks->SetStatusText("Loading terrain...");
        if (!LoadTerrainHeader(pProgressCallbacks))
            return false;
    }
    pProgressCallbacks->SetProgressValue(3);

    // has global entities?
    if (mapHeader.HasGlobalEntities)
    {
        pProgressCallbacks->SetStatusText("Loading global entities...");
        if (!LoadGlobalEntities(pProgressCallbacks))
            return false;
    }

    // starting position: todo move to map header
    float3 initialStreamingPosition((pInitialPositionOverride != nullptr) ? *pInitialPositionOverride : float3::Zero);
    pProgressCallbacks->SetStatusText("Loading starting regions...");
    pProgressCallbacks->PushState();
    m_pWorld->AddObserver(this, initialStreamingPosition);
    HandleStreaming(pProgressCallbacks);
    m_pWorld->RemoveObserver(this);
    pProgressCallbacks->PopState();
    pProgressCallbacks->SetProgressValue(1);

    // done    
    Log_InfoPrintf("Map '%s' loaded in %.4f seconds", mapName, loadTimer.GetTimeSeconds());
    return true;
}

void Map::LoadAllRegions(ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    pProgressCallbacks->SetProgressRange(m_regions.GetMemberCount());
    pProgressCallbacks->SetProgressValue(0);

    uint32 progressValue = 0;
    for (MapRegionTable::Iterator itr = m_regions.Begin(); !itr.AtEnd(); itr.Forward())
    {
        Vector2i regionPosition(itr->Key);
        MapRegion *pRegion = itr->Value;
        if (!pRegion->IsLoaded())
        {
            pProgressCallbacks->SetFormattedStatusText("Loading region (%i, %i)", pRegion->GetRegionX(), pRegion->GetRegionY());
            pProgressCallbacks->PushState();
            if (!LoadRegion(regionPosition.x, regionPosition.y, pProgressCallbacks))
            {
                Log_WarningPrintf("Map::LoadAllRegions: Failed to load region (%i, %i)", regionPosition.x, regionPosition.y);
                pProgressCallbacks->PopState();
                continue;
            }
            pProgressCallbacks->PopState();
        }

        pProgressCallbacks->SetProgressValue(++progressValue);
    }
}
    
void Map::UnloadAllRegions()
{
    for (MapRegionTable::Iterator itr = m_regions.Begin(); !itr.AtEnd(); itr.Forward())
    {
        Vector2i regionPosition(itr->Key);
        MapRegion *pRegion = itr->Value;
        if (pRegion->IsLoaded())
            UnloadRegion(regionPosition.x, regionPosition.y);
    }
}

void Map::HandleStreaming(ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    const uint32 observerCount = m_pWorld->GetObserverCount();
    if (observerCount == 0)
        return;

    const int32 loadRadius = (int32)m_regionLoadRadius;
    pProgressCallbacks->SetProgressRange(m_regions.GetMemberCount());
    pProgressCallbacks->SetProgressValue(0);

    // get region for camera position
    int2 *pObserverRegions = (int2 *)alloca(sizeof(int2) * observerCount);
    for (uint32 i = 0; i < observerCount; i++)
        pObserverRegions[i] = GetRegionForPosition(m_pWorld->GetObserverLocation(i));

    // load any regions that are within distance of the camera, unload anything that's not
    for (MapRegionTable::Iterator itr = m_regions.Begin(); !itr.AtEnd(); itr.Forward())
    {
        const int2 &regionPosition = itr->Key;
        int32 regionsFromCamera = Y_INT32_MAX;
        for (uint32 i = 0; i < observerCount; i++)
            regionsFromCamera = Min(regionsFromCamera, Max(Math::Abs(pObserverRegions[i].x - regionPosition.x), Math::Abs(pObserverRegions[i].y - regionPosition.y)));

        // determine the lod level applicable for this region
        int32 lodLevelForRegion;
        int32 maxLoadDistance = loadRadius;
        for (lodLevelForRegion = 0; lodLevelForRegion < (int32)m_regionLODLevels; lodLevelForRegion++)
        {
            if (regionsFromCamera <= maxLoadDistance)
                break;

            if (maxLoadDistance == 1)
                maxLoadDistance++;
            else
                maxLoadDistance *= maxLoadDistance;
        }

        // out of range?
        if (lodLevelForRegion == (int32)m_regionLODLevels)
            lodLevelForRegion = -1;

        const MapRegion *pRegion = itr->Value;
        DebugAssert(pRegion != NULL);

        // matches?
        if (pRegion->GetLoadedLODLevel() == lodLevelForRegion)
            continue;

        // switch the lod level
        Log_PerfPrintf("Map::HandleStreaming: Region (%i, %i) transitioning from LOD %i to %i...", regionPosition.x, regionPosition.y, pRegion->GetLoadedLODLevel(), lodLevelForRegion);
        if (!ChangeRegionLOD(regionPosition.x, regionPosition.y, lodLevelForRegion))
        {
            Log_WarningPrintf("Map::HandleStreaming: Failed to transition region (%i, %i) from LOD %i to %i", regionPosition.x, regionPosition.y, pRegion->GetLoadedLODLevel(), lodLevelForRegion);
            continue;
        }

        pProgressCallbacks->IncrementProgressValue();
    }
}

bool Map::LoadRegionsHeader(ProgressCallbacks *pProgressCallbacks)
{
    // read header
    AutoReleasePtr<ByteStream> pStream = m_pMapArchive->OpenFile(DF_MAP_REGIONS_HEADER_FILENAME, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
        return false;

    DF_MAP_REGIONS_HEADER regionsHeader;
    if (!pStream->Read2(&regionsHeader, sizeof(regionsHeader)) || regionsHeader.HeaderSize != sizeof(regionsHeader))
    {
        pProgressCallbacks->DisplayFormattedModalError("Map::LoadMap: Invalid regions header");
        return false;
    }

    for (uint32 i = 0; i < regionsHeader.RegionCount; i++)
    {
        int2 regionCoordinates;
        if (!pStream->Read2(&regionCoordinates, sizeof(int2)))
            return false;

        DebugAssert(m_regions.Find(regionCoordinates) == nullptr);

        MapRegion *pRegion = new MapRegion(this, regionCoordinates.x, regionCoordinates.y);
        m_regions.Insert(regionCoordinates, pRegion);
    }

    return true;
}

bool Map::LoadTerrainHeader(ProgressCallbacks *pProgressCallbacks)
{
    PathString fileName;

    // read header
    {
        AutoReleasePtr<ByteStream> pStream = m_pMapArchive->OpenFile(DF_MAP_TERRAIN_HEADER_FILENAME, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
        if (pStream == nullptr)
            return false;

        DF_MAP_TERRAIN_HEADER terrainHeader;
        if (!pStream->Read2(&terrainHeader, sizeof(terrainHeader)) || terrainHeader.HeaderSize != sizeof(terrainHeader))
        {
            pProgressCallbacks->DisplayFormattedModalError("Map::LoadTerrainHeader: Invalid terrain header");
            return false;
        }

        // fill in terrain parameters
        m_terrainParameters.HeightStorageFormat = (TERRAIN_HEIGHT_STORAGE_FORMAT)terrainHeader.HeightStorageFormat;
        m_terrainParameters.MinHeight = terrainHeader.MinHeight;
        m_terrainParameters.MaxHeight = terrainHeader.MaxHeight;
        m_terrainParameters.BaseHeight = terrainHeader.BaseHeight;
        m_terrainParameters.Scale = terrainHeader.Scale;
        m_terrainParameters.SectionSize = terrainHeader.SectionSize;
        m_terrainParameters.LODCount = terrainHeader.LODCount;
        if (!TerrainUtilities::IsValidParameters(&m_terrainParameters))
        {
            pProgressCallbacks->DisplayFormattedModalError("Map::LoadTerrainHeader: Invalid terrain parameters");
            return false;
        }
    }

    // load layer list
    {
        fileName.Format("%s", DF_MAP_TERRAIN_LAYERS_FILENAME_PREFIX);

        // since this layer list is a chunked file, it needs to be seekable
        AutoReleasePtr<ByteStream> pStream = m_pMapArchive->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE);
        if (pStream == nullptr)
            return false;

        // load it
        TerrainLayerList *pTerrainLayerList = new TerrainLayerList();
        if (!pTerrainLayerList->Load(fileName, pStream))
        {
            pProgressCallbacks->DisplayFormattedModalError("Map::LoadTerrainHeader: Could not load terrain layers");
            pTerrainLayerList->Release();
            return false;
        }
        m_pTerrainLayerList = pTerrainLayerList;
    }

    // create terrain renderer
    if (g_pRenderer != nullptr)
    {
        m_pTerrainRenderer = TerrainRenderer::CreateTerrainRenderer(&m_terrainParameters, m_pTerrainLayerList);
        if (m_pTerrainRenderer == nullptr)
        {
            pProgressCallbacks->DisplayFormattedModalError("Map::LoadTerrainHeader: Failed to create terrain renderer");
            return false;
        }
    }

    // ok
    return true;
}

bool Map::LoadGlobalEntities(ProgressCallbacks *pProgressCallbacks)
{
    AutoReleasePtr<ByteStream> pStream = m_pMapArchive->OpenFile(DF_MAP_GLOBAL_ENTITIES_FILENAME, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
        return false;

    DF_MAP_GLOBAL_ENTITIES_HEADER globalEntitiesHeader;
    if (!pStream->Read2(&globalEntitiesHeader, sizeof(globalEntitiesHeader)) || globalEntitiesHeader.HeaderSize != sizeof(globalEntitiesHeader))
    {
        pProgressCallbacks->DisplayFormattedModalError("Map::LoadMap: Invalid regions header");
        return false;
    }

    // create the entities, no need to track the ids
    CreateEntitiesFromStream(pStream, globalEntitiesHeader.EntityCount, globalEntitiesHeader.EntityDataSize, nullptr, nullptr, ProgressCallbacks::NullProgressCallback);

    // all good
    return true;
}

uint32 Map::CreateEntitiesFromStream(ByteStream *pStream, uint32 entityCount, uint32 entityDataSize, PODArray<uint32> *pOutDynamicEntityIDArray, PODArray<Brush *> *pOutStaticObjectsArray, ProgressCallbacks *pProgressCallbacks)
{
    SmallString objectName;
    SmallString componentName;

    // create reader
    BinaryReader binaryReader(pStream);

    // initalize progress
    pProgressCallbacks->SetProgressRange(entityCount);
    pProgressCallbacks->SetProgressValue(0);

    // iterate over entities
    uint32 createdEntityCount = 0;
    for (uint32 i = 0; i < entityCount; i++)
    {
        DF_MAP_ENTITY_HEADER entityHeader;
        if (!binaryReader.SafeReadType(&entityHeader) || entityHeader.HeaderSize != sizeof(entityHeader))
            return createdEntityCount;

        // calculate offset to next entity
        uint64 nextEntityOffset = binaryReader.GetStreamPosition() + entityHeader.EntitySize + entityHeader.ComponentsSize;

        // read the entity name
        if (!binaryReader.SafeReadFixedString(entityHeader.EntityNameLength, &objectName))
            return createdEntityCount;

        // deserialize the entity object
        Object *pObject = m_pClassTable->UnserializeObject(pStream, entityHeader.EntityTypeIndex);
        if (pObject == nullptr)
        {
            // creation failed.. or corruption.. could be either.
            pProgressCallbacks->DisplayFormattedWarning("Failed to deserialize object '%s' (type index %u)", objectName.GetCharArray(), entityHeader.EntityTypeIndex);
            if (!binaryReader.SafeSeekAbsolute(nextEntityOffset))
                return createdEntityCount;
            else
                continue;
        }

        // should be the correct type
        if (pObject->IsDerived(OBJECT_TYPEINFO(Brush)))
        {
            Brush *pBrush = pObject->Cast<Brush>();

            // shouldn't have any components
            if (entityHeader.ComponentCount > 0)
            {
                pProgressCallbacks->DisplayFormattedWarning("Brush '%s' has components on a brush type.", objectName.GetCharArray());
                pBrush->Release();

                if (!binaryReader.SafeSeekAbsolute(nextEntityOffset))
                    return createdEntityCount;
                else
                    continue;
            }

            // finalize it
            if (!pBrush->Initialize())
            {
                pProgressCallbacks->DisplayFormattedWarning("Failed to initialize Brush '%s'.", objectName.GetCharArray());
                pBrush->Release();

                if (!binaryReader.SafeSeekAbsolute(nextEntityOffset))
                    return createdEntityCount;
                else
                    continue;
            }

            // store it
            if (pOutStaticObjectsArray != nullptr)
            {
                pBrush->AddRef();
                pOutStaticObjectsArray->Add(pBrush);
            }

            // and add it to the world
            m_pWorld->AddBrush(pBrush);
            pBrush->Release();
            createdEntityCount++;
        }
        else if (pObject->IsDerived(OBJECT_TYPEINFO(Entity)))
        {
            Entity *pEntity = pObject->Cast<Entity>();
            uint32 entityID = m_pWorld->AllocateEntityID();

            // construct the entity
            if (!pEntity->Initialize(entityID, objectName))
            {
                pProgressCallbacks->DisplayFormattedWarning("Failed to initialize Entity '%s'.", objectName.GetCharArray());
                pEntity->Release();

                if (!binaryReader.SafeSeekAbsolute(nextEntityOffset))
                    return createdEntityCount;
                else
                    continue;
            }

            // deserialize any components
            for (uint32 componentIndex = 0; componentIndex < entityHeader.ComponentCount; componentIndex++)
            {
                DF_MAP_ENTITY_COMPONENT_HEADER componentHeader;
                if (!binaryReader.SafeReadType(&componentHeader))
                {
                    pEntity->Release();
                    return createdEntityCount;
                }

                // calc offset to next component
                uint64 nextComponentOffset = binaryReader.GetStreamPosition() + componentHeader.ComponentSize;

                // read component name
                if (!binaryReader.SafeReadFixedString(componentHeader.ComponentNameLength, &componentName))
                {
                    pEntity->Release();
                    return createdEntityCount;
                }

                // deserialize the component object
                Object *pComponentObject = m_pClassTable->UnserializeObject(pStream, componentHeader.ComponentTypeIndex);
                if (pComponentObject == nullptr)
                {
                    pProgressCallbacks->DisplayFormattedWarning("Failed to deserialize component '%s' of entity '%s'.", componentName.GetCharArray(), objectName.GetCharArray());

                    if (!binaryReader.SafeSeekAbsolute(nextComponentOffset))
                    {
                        pEntity->Release();
                        return createdEntityCount;
                    }
                    else
                    {
                        continue;
                    }
                }

                // if not a component, skip it
                if (!pComponentObject->IsDerived(OBJECT_TYPEINFO(Component)))
                {
                    pProgressCallbacks->DisplayFormattedWarning("Failed to deserialize component '%s' of entity '%s' is an invalid type '%s'.", componentName.GetCharArray(), objectName.GetCharArray(), pComponentObject->GetObjectTypeInfo()->GetTypeName());
                    pComponentObject->GetObjectTypeInfo()->GetFactory()->DeleteObject(pComponentObject);

                    if (!binaryReader.SafeSeekAbsolute(nextComponentOffset))
                    {
                        pEntity->Release();
                        return createdEntityCount;
                    }
                    else
                    {
                        continue;
                    }
                }

                // initialize the component
                Component *pComponent = pComponentObject->Cast<Component>();
                if (!pComponent->Initialize())
                {
                    pProgressCallbacks->DisplayFormattedWarning("Failed to initialize Component '%s' of entity '%s'.", componentName.GetCharArray(), objectName.GetCharArray());
                    pComponent->Release();

                    if (!binaryReader.SafeSeekAbsolute(nextComponentOffset))
                    {
                        pEntity->Release();
                        return createdEntityCount;
                    }
                    else
                    {
                        continue;
                    }
                }

                // add it to the entity
                pEntity->AddComponent(pComponent);
                pComponent->Release();

                // seek to next
                if (!binaryReader.SafeSeekAbsolute(nextComponentOffset))
                {
                    pEntity->Release();
                    return createdEntityCount;
                }
            }

            if (pOutDynamicEntityIDArray != nullptr)
                pOutDynamicEntityIDArray->Add(pEntity->GetEntityID());

            m_pWorld->AddEntity(pEntity);
            createdEntityCount++;
            pEntity->Release();
        }
        else
        {
            pProgressCallbacks->DisplayFormattedWarning("Object '%s' is an invalid type '%s'.", objectName.GetCharArray(), pObject->GetObjectTypeInfo()->GetTypeName());
            pObject->GetObjectTypeInfo()->GetFactory()->DeleteObject(pObject);

            if (!binaryReader.SafeSeekAbsolute(nextEntityOffset))
                return createdEntityCount;
            else
                continue;
        }

        // seek to the next object
        if (!binaryReader.SafeSeekAbsolute(nextEntityOffset))
            return createdEntityCount;
    }

    // ok
    return createdEntityCount;
}

bool Map::LoadRegion(int32 regionX, int32 regionY, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    int2 regionPosition(regionX, regionY);
    MapRegionTable::Member *pMember = m_regions.Find(regionPosition);
    if (pMember == NULL)
    {
        Log_WarningPrintf("Map::LoadRegion: Attempting to load unknown region (%i, %i)", regionX, regionY);
        return false;
    }

    MapRegion *pRegion = pMember->Value;
    if (!pRegion->IsLoaded())
    {
        PathString fileName;
        fileName.Format("region_%i_%i.0", regionX, regionY);

        AutoReleasePtr<ByteStream> pStream = m_pMapArchive->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
        if (pStream == NULL)
        {
            Log_WarningPrintf("Map::LoadRegion: Could not open region file for (%i, %i) '%s'", regionX, regionY, fileName.GetCharArray());
            return false;
        }

        if (!pRegion->LoadRegion(pStream, 0, pProgressCallbacks))
        {
            Log_WarningPrintf("Map::LoadRegion: Could not load region (%i, %i)", regionX, regionY);
            //pRegion->UnloadRegion();
            return false;
        }
    }

    return true;
}

bool Map::ChangeRegionLOD(int32 regionX, int32 regionY, int32 newLOD, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    int2 regionPosition(regionX, regionY);
    MapRegionTable::Member *pMember = m_regions.Find(regionPosition);
    if (pMember == NULL)
    {
        Log_WarningPrintf("Map::ChangeRegionLOD: Attempting to load unknown region (%i, %i)", regionX, regionY);
        return false;
    }

    MapRegion *pRegion = pMember->Value;
    if (pRegion->GetLoadedLODLevel() == newLOD)
        return true;

    // changed
    Log_DevPrintf("Map::ChangeRegionLOD: Region (%i, %i) LOD %i -> %i", regionX, regionY, pRegion->GetLoadedLODLevel(), newLOD);

    // unload old data
    if (pRegion->IsLoaded())
        pRegion->UnloadRegion();

    // handle new data
    if (newLOD >= 0)
    {
        PathString fileName;
        fileName.Format("region_%i_%i.%u", regionX, regionY, newLOD);

        AutoReleasePtr<ByteStream> pStream = m_pMapArchive->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
        if (pStream == NULL)
        {
            Log_WarningPrintf("Map::LoadRegion: Could not open region file for (%i, %i, LOD %i), '%s'", regionX, regionY, newLOD, fileName.GetCharArray());
            return false;
        }

        if (!pRegion->LoadRegion(pStream, (uint32)newLOD, pProgressCallbacks))
        {
            Log_WarningPrintf("Map::LoadRegion: Could not load region (%i, %i, LOD %i)", regionX, regionY, newLOD);
            //pRegion->UnloadRegion();
            return false;
        }
    }

    return true;
}

void Map::UnloadRegion(int32 regionX, int32 regionY)
{
    int2 regionPosition(regionX, regionY);
    MapRegionTable::Member *pMember = m_regions.Find(regionPosition);
    if (pMember == NULL)
    {
        Log_WarningPrintf("Map::UnloadRegion: Attempting to unload unknown region (%i, %i)", regionX, regionY);
        return;
    }

    MapRegion *pRegion = pMember->Value;
    if (pRegion->IsLoaded())
        pRegion->UnloadRegion();
}

MapRegion::MapRegion(Map *pMap, int32 regionX, int32 regionY)
    : m_pMap(pMap),
      m_regionX(regionX),
      m_regionY(regionY),
      m_loadedLODLevel(-1)
{

}

MapRegion::~MapRegion()
{

}

bool MapRegion::LoadRegion(ByteStream *pStream, uint32 lodLevel, ProgressCallbacks *pProgressCallbacks)
{
    uint64 nextChunkPosition;
    DebugAssert(m_loadedLODLevel < 0);

    // prevent any double loading
    m_loadedLODLevel = (int32)lodLevel;
    
    // load header
    DF_MAP_REGION_HEADER regionHeader;
    if (!pStream->Read2(&regionHeader, sizeof(regionHeader)) || regionHeader.HeaderSize != sizeof(regionHeader))
    {
        Log_ErrorPrintf("MapRegion::LoadRegion: Invalid region header");
        return false;
    }

    // bit of validation
    if (regionHeader.RegionX != m_regionX || regionHeader.RegionY != m_regionY || regionHeader.LODLevel != lodLevel)
    {
        Log_ErrorPrintf("MapRegion::LoadRegion: Invalid region header");
        return false;
    }

    // read terrain sections
    nextChunkPosition = pStream->GetPosition() + (uint64)regionHeader.TerrainDataSize;
    if (regionHeader.TerrainSectionCount > 0)
    {
        for (uint32 i = 0; i < regionHeader.TerrainSectionCount; i++)
        {
            DF_MAP_REGION_TERRAIN_SECTION_HEADER terrainSectionHeader;
            if (!pStream->Read2(&terrainSectionHeader, sizeof(terrainSectionHeader)))
            {
                Log_ErrorPrintf("MapRegion::LoadRegion: Failed to read terrain section header.");
                return false;
            }

            TerrainSection *pTerrainSection = new TerrainSection(&m_pMap->m_terrainParameters, terrainSectionHeader.SectionX, terrainSectionHeader.SectionY, terrainSectionHeader.LODLevel);
            if (!pTerrainSection->LoadFromStream(pStream))
            {
                Log_ErrorPrintf("MapRegion::LoadRegion: Failed to load terrain section.");
                delete pTerrainSection;
                return false;
            }

            // remove me
            if (terrainSectionHeader.SectionX == 0 && terrainSectionHeader.SectionY == 0)
            {
                AutoReleasePtr<const StaticMesh> pStaticMesh = g_pResourceManager->GetStaticMesh("models/terrain/grass_blades");
                int32 idx = pTerrainSection->AddDetailMesh(pStaticMesh, 30.0f);
                const uint32 XN = 200;
                const uint32 YN = 300;
                for (uint32 x = 0; x < XN; x++)
                {
                    for (uint32 y = 0; y < YN; y++)
                    {
                        pTerrainSection->AddDetailMeshInstance(idx, x / (float)XN, y / (float)YN, 1.0f, (float)((x * y) % 360));
                    }
                }
            }

            // create data struct
            RegionTerrainSection sectionData;
            sectionData.pData = pTerrainSection;

            // create collision shape and insert the object, the translation is the base position of the section with no height modification
            sectionData.pCollisionShape = new TerrainSectionCollisionShape(&m_pMap->m_terrainParameters, pTerrainSection);
            sectionData.pCollisionObject = new Physics::StaticObject(0, sectionData.pCollisionShape, TerrainSectionCollisionShape::GetSectionTransform(&m_pMap->m_terrainParameters, pTerrainSection));
            m_pMap->m_pWorld->GetPhysicsWorld()->AddObject(sectionData.pCollisionObject);               

            // create renderer
            if (m_pMap->m_pTerrainRenderer != nullptr)
            {
                sectionData.pRenderProxy = m_pMap->m_pTerrainRenderer->CreateSectionRenderProxy(0, pTerrainSection);
                if (sectionData.pRenderProxy == nullptr)
                    Log_WarningPrintf("MapRegion::LoadRegion: Failed to create terrain section render proxy [%i, %i: %i, %i]", m_regionX, m_regionY, pTerrainSection->GetSectionX(), pTerrainSection->GetSectionY());
                else
                    m_pMap->m_pWorld->GetRenderWorld()->AddRenderable(sectionData.pRenderProxy);
            }
            else
            {
                sectionData.pRenderProxy = nullptr;
            }

            // store it
            m_terrainSections.Add(sectionData);
        }
    }

    // check position
    if (pStream->GetPosition() != nextChunkPosition && !pStream->SeekAbsolute(nextChunkPosition))
    {
        Log_ErrorPrintf("MapRegion::LoadRegion: Stream not in correct position after terrain load, and seek failed.");
        return false;
    }

    // read entities
    nextChunkPosition = pStream->GetPosition() + (uint64)regionHeader.EntityDataSize;
    if (regionHeader.EntityCount > 0)
    {
        pProgressCallbacks->PushState();
        m_pMap->CreateEntitiesFromStream(pStream, regionHeader.EntityCount, regionHeader.EntityDataSize, &m_loadedEntityIDs, &m_loadedStaticObjects, pProgressCallbacks);
        pProgressCallbacks->PopState();
    }

    // check position
    if (pStream->GetPosition() != nextChunkPosition && !pStream->SeekAbsolute(nextChunkPosition))
    {
        Log_ErrorPrintf("MapRegion::LoadRegion: Stream not in correct position after entity load, and seek failed.");
        return false;
    }

    // done
    return true;
}

void MapRegion::UnloadRegion()
{
    // remove all static entities
    for (uint32 i = 0; i < m_loadedStaticObjects.GetSize(); i++)
    {
        Brush *pObject = m_loadedStaticObjects[i];
        if (pObject->IsInWorld())
            m_pMap->m_pWorld->RemoveBrush(pObject);
        else
            Log_WarningPrintf("MapRegion::UnloadRegion: StaticObject %p from region (%i, %i) was not in world at region unload time.", pObject, m_regionX, m_regionY);

        pObject->Release();
    }
    m_loadedStaticObjects.Obliterate();

    // remove dynamic entities

//     // unload all static entities
//     {
//         for (i = 0; i < m_loadedStaticEntities.GetSize(); i++)
//         {
//             uint32 mapStaticEntityId = m_loadedStaticEntities[i];
//             uint32 worldStaticEntityId = m_pMap->GetWorldEntityIdForMapEntityId(mapStaticEntityId);
//             DebugAssert(worldStaticEntityId != 0);
// 
//             m_pMap->m_mapEntityIdMapping.Remove(mapStaticEntityId);
// 
//             Entity *pEntity = m_pMap->GetWorld()->GetEntityById(worldStaticEntityId);
//             DebugAssert(pEntity != NULL);
// 
//             m_pMap->GetWorld()->QueueRemoveEntity(pEntity);
//         }
//     }

    // remove terrain sections
    for (uint32 i = 0; i < m_terrainSections.GetSize(); i++)
    {
        RegionTerrainSection *pSection = &m_terrainSections[i];

        if (pSection->pRenderProxy != nullptr)
        {
            m_pMap->m_pWorld->GetRenderWorld()->RemoveRenderable(pSection->pRenderProxy);
            pSection->pRenderProxy->Release();
        }

        m_pMap->m_pWorld->GetPhysicsWorld()->RemoveObject(pSection->pCollisionObject);
        pSection->pCollisionObject->Release();
        pSection->pCollisionShape->Release();

        pSection->pData->Release();
    }
    m_terrainSections.Obliterate();

    m_loadedLODLevel = -1;
}

