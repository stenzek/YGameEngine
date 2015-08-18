#include "Editor/PrecompiledHeader.h"
#include "Editor/Editor.h"
#include "Editor/MapEditor/EditorMap.h"
#include "Editor/MapEditor/EditorMapWindow.h"
#include "Editor/MapEditor/EditorMapEntity.h"
#include "Editor/MapEditor/EditorEditMode.h"
#include "Editor/MapEditor/EditorMapEditMode.h"
#include "Editor/MapEditor/EditorEntityEditMode.h"
#include "Editor/MapEditor/EditorGeometryEditMode.h"
#include "Editor/MapEditor/EditorTerrainEditMode.h"
#include "Editor/MapEditor/EditorCreateTerrainDialog.h"
#include "Editor/MapEditor/ui_EditorMapWindow.h"
#include "Editor/EditorVisual.h"
#include "Editor/EditorSettings.h"
#include "Editor/EditorProgressDialog.h"
#include "Engine/ResourceManager.h"
#include "Engine/Entity.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/RenderProxies/StaticMeshRenderProxy.h"
#include "Renderer/RenderProxies/DirectionalLightRenderProxy.h"
#include "MapCompiler/MapSource.h"
#include "MapCompiler/MapSourceTerrainData.h"
#include "ResourceCompiler/TerrainLayerListGenerator.h"
#include "ResourceCompiler/ObjectTemplate.h"
#include "ResourceCompiler/ObjectTemplateManager.h"
Log_SetChannel(EditorMap);

EditorMap::EditorMap(EditorMapWindow *pMapWindow)
    : m_pMapWindow(pMapWindow),
      m_pMapSource(nullptr),
      m_pRenderWorld(nullptr),
      m_mapBoundingBox(AABox::Zero),
      m_entityVisualUpdatePending(false),
      m_pMapEditMode(nullptr),
      m_pEntityEditMode(nullptr),
      m_pGeometryEditMode(nullptr),
      m_pTerrainEditMode(nullptr),
      m_gridLinesInterval(1.0f),
      m_gridSnapInterval(0.25f),
      m_gridLinesVisible(true),
      m_gridSnapEnabled(true),
      m_skyVisibility(true),
      m_skyAutoSize(true),
      m_skySize(1.0f),
      m_skyGroundHeight(0.0f),
      m_pSkyRenderProxy(nullptr),      
      m_sunVisibility(true),
      m_pSunRenderProxy(nullptr),
      m_pSunAmbientRenderProxy(nullptr)
{

}

EditorMap::~EditorMap()
{
    // delete edit modes
    delete m_pTerrainEditMode;
    delete m_pGeometryEditMode;
    delete m_pEntityEditMode;
    delete m_pMapEditMode;

    for (uint32 i = 0; i < m_entities.GetSize(); i++)
        delete m_entities[i];

    SAFE_RELEASE(m_pSunRenderProxy);
    SAFE_RELEASE(m_pSkyRenderProxy);

    // release other objects
    m_pRenderWorld->Release();
    delete m_pMapSource;

    // compact resource manager resources (unload as many as possible)
    g_pResourceManager->CompactResources();
}

bool EditorMap::CreateMap()
{
    // setup progress dialog
    EditorProgressDialog progressDialog(m_pMapWindow);
    progressDialog.show();

    m_pRenderWorld = new RenderWorld();
    m_pMapSource = new MapSource();

    // create empty map
    m_pMapSource->Create();

    // initialize
    return Initialize(&progressDialog);
}

bool EditorMap::OpenMap(const char *FileName, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
#define LOAD_ERROR(str) { QMessageBox::information(m_pMapWindow, "Map load error", "Map load error: " str, QMessageBox::Ok); }

    // create objects
    m_strMapFileName = FileName;
    m_pRenderWorld = new RenderWorld();
    m_pMapSource = new MapSource();

    // read in the map source
    if (!m_pMapSource->Load(FileName, pProgressCallbacks))
    {
        LOAD_ERROR("Could not parse map file.");
        return false;
    }

    // initialize
    return Initialize(pProgressCallbacks);

#undef LOAD_ERROR
}

bool EditorMap::Initialize(ProgressCallbacks *pProgressCallbacks)
{
    // camera edit mode
    m_pMapEditMode = new EditorMapEditMode(this);
    if (!m_pMapEditMode->Initialize(pProgressCallbacks))
        return false;

    // entity edit mode
    m_pEntityEditMode = new EditorEntityEditMode(this);
    if (!m_pEntityEditMode->Initialize(pProgressCallbacks))
        return false;

    // geometry edit mode
    m_pGeometryEditMode = new EditorGeometryEditMode(this);
    if (!m_pGeometryEditMode->Initialize(pProgressCallbacks))
        return false;

    // initialize entities
    if (!InitializeEntities(pProgressCallbacks))
        return false;

    // create terrain
    if (m_pMapSource->HasTerrain() && !InitializeTerrain(pProgressCallbacks))
    {
        Log_ErrorPrintf("EditorMap::Initialize: Failed to initialize terrain.");
        return false;
    }

    // update visuals on first frame
    m_entityVisualUpdatePending = true;

    // update sky + sun
    UpdateSkyVariables();
    UpdateSunVariables();

    // ok
    return true;
}

bool EditorMap::SaveMap(const char *NewFileName /* = NULL */, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
#define SAVE_ERROR(str) { QMessageBox::information(m_pMapWindow, "Map save error", "Map save error: " str, QMessageBox::Ok); }

    // do the save
    bool saveResult = (NewFileName != NULL) ? m_pMapSource->SaveAs(NewFileName, pProgressCallbacks) : m_pMapSource->Save(pProgressCallbacks);
    if (!saveResult)
        SAVE_ERROR("Could not save map file.");

    return saveResult;

#undef SAVE_ERROR
}

const String &EditorMap::GetMapProperty(const char *propertyName)
{
    const String *pValue = m_pMapSource->GetProperties()->GetPropertyValuePointer(propertyName);
    if (pValue != nullptr)
        return *pValue;

    const PropertyTemplateProperty *pPropertyDefinition = m_pMapSource->GetObjectTemplate()->GetPropertyDefinitionByName(propertyName);
    if (pPropertyDefinition == nullptr)
        return EmptyString;

    return pPropertyDefinition->GetDefaultValue();
}

void EditorMap::SetMapProperty(const char *propertyName, const char *propertyValue)
{
    // try to use proper capitalization
    const PropertyTemplateProperty *pPropertyDefinition = m_pMapSource->GetObjectTemplate()->GetPropertyDefinitionByName(propertyName);
    if (pPropertyDefinition != nullptr)
    {
        m_pMapSource->GetProperties()->SetPropertyValueString(pPropertyDefinition->GetName(), propertyValue);
        OnMapPropertyChange(pPropertyDefinition->GetName(), propertyValue);
    }
    else
    {
        m_pMapSource->GetProperties()->SetPropertyValueString(propertyName, propertyValue);
        OnMapPropertyChange(propertyName, propertyValue);
    }

    // redraw everything
    RedrawAllViewports();
}

EditorMapEntity *EditorMap::CreateEntity(const ObjectTemplate *pTemplate, const char *entityName /* = nullptr */)
{
    if (!pTemplate->CanCreate())
        return nullptr;

    MapSourceEntityData *pEntityData = m_pMapSource->CreateEntityFromTemplate(pTemplate, entityName);
    if (pEntityData == nullptr)
        return nullptr;

    EditorMapEntity *pEntity = CreateInternalEntity(pEntityData);
    DebugAssert(pEntity != nullptr);

    // update summary
    m_pMapEditMode->UpdateSummary();

    // redraw everything
    RedrawAllViewports();
    return pEntity;
}

EditorMapEntity *EditorMap::CreateEntity(const char *entityTypeName, const char *entityName /* = nullptr */)
{
    const ObjectTemplate *pTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate(entityTypeName);
    if (pTemplate == nullptr)
        return nullptr;

    return CreateEntity(pTemplate, entityName);
}

void EditorMap::DeleteEntity(EditorMapEntity *pEntity)
{
    // find in source
    MapSourceEntityData *pEntityData = const_cast<MapSourceEntityData *>(pEntity->GetEntityData());
    DebugAssert(pEntityData != nullptr);

    // is it selected?
    if (m_pEntityEditMode->IsSelected(pEntity))
        m_pEntityEditMode->RemoveSelection(pEntity);

    // remove from outliner
    m_pMapWindow->GetUI()->worldOutliner->RemoveEntity(pEntity);

    // cleanup the internal structure
    DebugAssert(m_entities[pEntity->GetArrayIndex()] == pEntity);
    m_entities[pEntity->GetArrayIndex()] = nullptr;
    delete pEntity;

    // remove from source
    m_pMapSource->RemoveEntity(pEntityData);

    // update summary
    m_pMapEditMode->UpdateSummary();

    // redraw everything
    RedrawAllViewports();
}

EditorMapEntity *EditorMap::CopyEntity(const EditorMapEntity *pEntity, const char *newName /* = nullptr */)
{
    // create new entity from existing entity
    MapSourceEntityData *pNewEntityData = m_pMapSource->CopyEntity(pEntity->GetEntityData(), newName);

    // create internal structure
    EditorMapEntity *pNewEntity = CreateInternalEntity(pNewEntityData);
    DebugAssert(pNewEntity != nullptr);

    // update summary
    m_pMapEditMode->UpdateSummary();

    // redraw everything
    RedrawAllViewports();
    return pNewEntity;
}

const EditorMapEntity *EditorMap::GetEntityByArrayIndex(uint32 indexID) const
{
    return (indexID < m_entities.GetSize()) ? m_entities[indexID] : nullptr;
}

EditorMapEntity *EditorMap::GetEntityByArrayIndex(uint32 indexID)
{
    return (indexID < m_entities.GetSize()) ? m_entities[indexID] : nullptr;
}

const EditorMapEntity *EditorMap::GetEntityByName(const char *entityName) const
{
    for (uint32 i = 0; i < m_entities.GetSize(); i++)
    {
        if (m_entities[i] != nullptr && m_entities[i]->GetEntityData()->GetEntityName().Compare(entityName))
            return m_entities[i];
    }

    return nullptr;
}

EditorMapEntity *EditorMap::GetEntityByName(const char *entityName)
{
    for (uint32 i = 0; i < m_entities.GetSize(); i++)
    {
        if (m_entities[i] != nullptr && m_entities[i]->GetEntityData()->GetEntityName().Compare(entityName))
            return m_entities[i];
    }

    return nullptr;
}

void EditorMap::OnEntityPropertyChanged(const EditorMapEntity *pEntity, const char *propertyName, const char *propertyValue)
{
    // handle base position + rotation
    if (Y_stricmp(propertyName, "Position") == 0)
    {
        OnEntityBoundsChanged(pEntity);

        // update world outliner
        m_pMapWindow->GetUI()->worldOutliner->OnEntityPositionChanged(pEntity);
    }
    else if (Y_stricmp(propertyName, "Rotation") == 0 ||
             Y_stricmp(propertyName, "Scale") == 0)
    {
        OnEntityBoundsChanged(pEntity);
    }

    // if the active edit mode is entity, notify this
    if (m_pMapWindow->GetEditMode() == EDITOR_EDIT_MODE_ENTITY)
        m_pEntityEditMode->OnEntityPropertyChanged(pEntity, propertyName, propertyValue);

    // queue viewport redraw
    RedrawAllViewports();
}

void EditorMap::OnEntityBoundsChanged(const EditorMapEntity *pEntity)
{
    MergeMapBoundingBox(pEntity->GetBoundingBox());
}

void EditorMap::MergeMapBoundingBox(const AABox &box)
{
    float3 mapMinBounds(m_mapBoundingBox.GetMinBounds());
    float3 mapMaxBounds(m_mapBoundingBox.GetMaxBounds());
    float3 boxMinBounds(box.GetMinBounds());
    float3 boxMaxBounds(box.GetMaxBounds());
    bool changed = false;

    // handle those pesky infinites
    for (uint32 i = 0; i < 3; i++)
    {
        if (boxMinBounds[i] != Y_FLT_INFINITE && boxMinBounds[i] < mapMinBounds[i])
        {
            mapMinBounds[i] = boxMinBounds[i];
            changed = true;
        }
        if (boxMaxBounds[i] != Y_FLT_INFINITE && boxMaxBounds[i] > mapMaxBounds[i])
        {
            mapMaxBounds[i] = boxMaxBounds[i];
            changed = true;
        }
    }

    if (changed)
    {
        m_mapBoundingBox.SetBounds(mapMinBounds, mapMaxBounds);
        OnMapBoundingBoxChange();
    }

//     // does this change the bounding box of the map?
//     if (box.GetMinBounds().AnyLess(m_mapBoundingBox.GetMinBounds()) ||
//         box.GetMaxBounds().AnyGreater(m_mapBoundingBox.GetMaxBounds()))
//     {
//         m_mapBoundingBox.Merge(box);
//         OnMapBoundingBoxChange();
//     }
}

void EditorMap::OnMapBoundingBoxChange()
{
    Log_DevPrintf("Map bounds change: (%s) - (%s)", StringConverter::Float3ToString(m_mapBoundingBox.GetMinBounds()).GetCharArray(), StringConverter::Float3ToString(m_mapBoundingBox.GetMaxBounds()).GetCharArray());

    // update sky transform
    UpdateSkyTransform();
}

// const MapSourceEntityData *EditorMap::GetEntityData(uint32 entityId) const
// {
//     return m_pMapSource->GetEntityData(entityId);
// }
// 
// String EditorMap::GetEntityPropertyValue(uint32 entityId, const char *propertyName) const
// {
//     const EditorMapEntity *pEntity = GetEntityByID(entityId);
//     if (pEntity == nullptr)
//         return EmptyString;
// 
//     return pEntity->GetEntityPropertyValue(propertyName);
// }
// 
// bool EditorMap::SetEntityPropertyValue(uint32 entityId, const char *propertyName, const char *propertyValue)
// {
//     EditorMapEntity *pEntity = GetEntityByID(entityId);
//     if (pEntity == NULL)
//     {
//         Log_WarningPrintf("Attempting to set property '%s' on untracked entity %u to '%s'.", propertyName, entityId, propertyValue);
//         return false;
//     }
// 
//     return m_pEntityEditMode->SetEntityPropertyValue(pEntity, propertyName, propertyValue);
// }

bool EditorMap::InitializeEntities(ProgressCallbacks *pProgressCallbacks)
{
    pProgressCallbacks->SetStatusText("Creating entities...");
    pProgressCallbacks->SetCancellable(false);
    pProgressCallbacks->SetProgressRange(m_pMapSource->GetEntityCount());
    pProgressCallbacks->SetProgressValue(0);

    m_pMapSource->EnumerateEntities([this, pProgressCallbacks](MapSourceEntityData *pEntityData)
    {
        CreateInternalEntity(pEntityData);
        pProgressCallbacks->IncrementProgressValue();
    });

    return true;
}

EditorMapEntity *EditorMap::CreateInternalEntity(MapSourceEntityData *pEntityData)
{
    // find a free slot
    uint32 arrayIndex;
    for (arrayIndex = 0; arrayIndex < m_entities.GetSize(); arrayIndex++)
    {
        if (m_entities[arrayIndex] == nullptr)
            break;
    }
    if (arrayIndex == m_entities.GetSize())
        m_entities.Add(nullptr);

    // create the visual
    EditorMapEntity *pEntity = EditorMapEntity::Create(this, arrayIndex, pEntityData);
    if (pEntity == nullptr)
    {
        Log_WarningPrintf("Could not create visual entity for entity '%s' (%s)", pEntityData->GetEntityName().GetCharArray(), pEntityData->GetTypeName().GetCharArray());
        return nullptr;
    }

    // store it
    m_entities[arrayIndex] = pEntity;

    // add to outliner
    m_pMapWindow->GetUI()->worldOutliner->AddEntity(pEntity);

    // merge with bounds
    OnEntityBoundsChanged(pEntity);

    // done
    return pEntity;
}

bool EditorMap::IsEntityInVisibleRange(const EditorMapEntity *pEntity) const
{
//     uint32 i;
//     float2 *cameraPositions = (float2 *)alloca(sizeof(float2) * m_viewports.GetSize());
//     uint32 nCameraPositions = 0;
// 
//     for (i = 0; i < m_viewports.GetSize(); i++)
//     {
//         const EditorMapViewport *pViewport = m_viewports[i];
//         if (pViewport != NULL)
//         {
//             const float3 &realCameraPosition(pViewport->GetViewController().GetCameraPosition());
//             cameraPositions[nCameraPositions++] = float2(realCameraPosition.x, realCameraPosition.y);
//         }
//     }
    return true;
}

void EditorMap::UpdateEntityVisuals()
{
    for (uint32 i = 0; i < m_entities.GetSize(); i++)
    {
        EditorMapEntity *pEntity = m_entities[i];
        if (pEntity == nullptr)
            continue;

        // in visual range?
        bool inVisualRange = IsEntityInVisibleRange(pEntity);

        // currently visible?
        if (inVisualRange)
        {
            if (!pEntity->IsVisualCreated())
                pEntity->CreateVisual();
        }
        else
        {
            if (pEntity->IsVisualCreated())
                pEntity->DeleteVisual();
        }
    }
}

bool EditorMap::CreateTerrain(const char *importLayerListName, TERRAIN_HEIGHT_STORAGE_FORMAT heightStorageFormat, uint32 scale, uint32 sectionSize, uint32 renderLODCount, int32 minHeight, int32 maxHeight, int32 baseHeight)
{
    EditorProgressDialog progressDialog(m_pMapWindow);
    progressDialog.show();

    // create the layer list
    TerrainLayerListGenerator *pLayerListGenerator;
    if (importLayerListName != nullptr)
    {
        // load from this file
        PathString importFileName;
        importFileName.Format("%s.layerlist.zip", importLayerListName);
        ByteStream *pStream = g_pVirtualFileSystem->OpenFile(importLayerListName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE);
        if (pStream == nullptr)
        {
            progressDialog.DisplayFormattedError("Failed to open '%s'", importFileName.GetCharArray());
            return false;
        }

        pLayerListGenerator = new TerrainLayerListGenerator();
        if (!pLayerListGenerator->Load(importLayerListName, pStream, &progressDialog))
        {
            progressDialog.DisplayFormattedError("Failed to load layerlist at '%s'", importFileName.GetCharArray());
            delete pLayerListGenerator;
            pStream->Release();
            return false;
        }        
    }
    else
    {
        // create the default one
        pLayerListGenerator = EditorTerrainEditMode::CreateEmptyLayerList();
        if (pLayerListGenerator == nullptr)
            return false;
    }

    // create the terrain data
    MapSourceTerrainData *pTerrainData = m_pMapSource->CreateTerrainData(pLayerListGenerator, heightStorageFormat, scale, sectionSize, renderLODCount, minHeight, maxHeight, baseHeight, &progressDialog);
    if (pTerrainData == nullptr)
    {
        delete pLayerListGenerator;
        return false;
    }

    // init it
    if (!InitializeTerrain(&progressDialog))
    {
        DeleteTerrain();
        return false;
    }

    // update ui
    //m_pMapWindow->GetUI()->OnMapTerrainCreatedOrDeleted(this);
    Panic("Fixme UI update");
    return true;
}

bool EditorMap::InitializeTerrain(ProgressCallbacks *pProgressCallbacks)
{
    // Create edit mode
    DebugAssert(m_pTerrainEditMode == nullptr);
    m_pTerrainEditMode = new EditorTerrainEditMode(this);
    if (!m_pTerrainEditMode->Initialize(pProgressCallbacks))
        return false;

    // merge bounding box
    MergeMapBoundingBox(m_pMapSource->GetTerrainData()->CalculateTerrainBoundingBox());

    // store
    return true;
}

void EditorMap::DeleteTerrain()
{
    if (m_pTerrainEditMode != nullptr)
    {
        delete m_pTerrainEditMode;
        m_pTerrainEditMode = nullptr;
    }

    if (m_pMapSource->HasTerrain())
        m_pMapSource->DeleteTerrainData();

    Panic("Fixme UI update");
}

void EditorMap::Update(float timeSinceLastUpdate)
{
    // update edit modes
    m_pMapEditMode->Update(timeSinceLastUpdate);
    m_pEntityEditMode->Update(timeSinceLastUpdate);
    m_pGeometryEditMode->Update(timeSinceLastUpdate);
    if (m_pTerrainEditMode != nullptr)
        m_pTerrainEditMode->Update(timeSinceLastUpdate);

    // update visual entities if prompted
    if (m_entityVisualUpdatePending)
    {
        UpdateEntityVisuals();
        m_entityVisualUpdatePending = true;
    }
}

void EditorMap::RedrawAllViewports()
{
    m_pMapWindow->RedrawAllViewports();
}

void EditorMap::OnMapPropertyChange(const char *propertyName, const char *propertyValue)
{
#define PROPERTY_IS(pname) ((Y_stricmp(pname, propertyName) == 0))

    if (PROPERTY_IS("SkyEnabled") || PROPERTY_IS("SkyType") ||
        PROPERTY_IS("SkyMaterial") || PROPERTY_IS("SkySize") ||
        PROPERTY_IS("SkyGroundLevel"))
    {
        UpdateSkyVariables();
    }

#undef PROPERTY_IS

    // update map edit mode
    m_pMapEditMode->OnMapPropertyChanged(propertyName, propertyValue);
}

void EditorMap::SetSkyVisibility(bool enabled)
{
    m_skyVisibility = enabled;
    if (m_pSkyRenderProxy != nullptr)
        m_pSkyRenderProxy->SetVisibility(enabled);
}

void EditorMap::SetSunVisibility(bool enabled)
{
    m_sunVisibility = enabled;

    if (m_pSunRenderProxy != nullptr)
        m_pSunRenderProxy->SetEnabled(enabled);
}

void EditorMap::UpdateSkyVariables()
{
    bool skyEnabled = StringConverter::StringToBool(GetMapProperty("SkyEnabled"));
    uint32 skyType = StringConverter::StringToUInt32(GetMapProperty("SkyType"));
    const char *skyMaterial = GetMapProperty("SkyMaterial");
    float skySize = StringConverter::StringToFloat(GetMapProperty("SkySize"));
    float skyGroundLevel = StringConverter::StringToFloat(GetMapProperty("SkyGroundLevel"));

    if (!skyEnabled)
    {
        if (m_pSkyRenderProxy != nullptr)
        {
            m_pRenderWorld->RemoveRenderable(m_pSkyRenderProxy);
            m_pSkyRenderProxy->Release();
            m_pSkyRenderProxy = nullptr;
        }

        return;
    }

    static const char *skyMeshNames[MAP_SKY_TYPE_COUNT] =
    {
        "models/engine/skybox",
        "models/engine/skysphere",
        "models/engine/skydome"
    };
    DebugAssert(skyType < MAP_SKY_TYPE_COUNT);

    AutoReleasePtr<const StaticMesh> pStaticMesh = g_pResourceManager->GetStaticMesh(skyMeshNames[skyType]);
    if (pStaticMesh == nullptr)
    {
        if (m_pSkyRenderProxy != nullptr)
        {
            m_pRenderWorld->RemoveRenderable(m_pSkyRenderProxy);
            m_pSkyRenderProxy->Release();
            m_pSkyRenderProxy = nullptr;
        }

        return;
    }

    AutoReleasePtr<const Material> pMaterial = g_pResourceManager->GetMaterial(skyMaterial);
    if (pMaterial == nullptr)
        pMaterial = g_pResourceManager->GetDefaultMaterial();

    if (m_pSkyRenderProxy == nullptr)
    {
        m_pSkyRenderProxy = new StaticMeshRenderProxy(0, pStaticMesh, Transform::Identity, 0);
        m_pRenderWorld->AddRenderable(m_pSkyRenderProxy);
    }

    m_pSkyRenderProxy->SetStaticMesh(pStaticMesh);
    m_pSkyRenderProxy->SetMaterial(0, pMaterial);
    m_skyGroundHeight = skyGroundLevel;
    m_skySize = skySize;
    UpdateSkyTransform();
}

void EditorMap::UpdateSkyTransform()
{
    if (m_pSkyRenderProxy == nullptr)
        return;

    float skySize;

    if (m_skyAutoSize)
    {
        const float MINIMUM_SIZE = 128.0f;
        const float SCALE_DOWN = 1000.0f;
        const float SCALE_UP = 2.0f;

        float3 maxDimensions(m_mapBoundingBox.GetMinBounds().Abs().Max(m_mapBoundingBox.GetMaxBounds().Abs()));
        skySize = Max(MINIMUM_SIZE, Max(maxDimensions.x, Max(maxDimensions.y, maxDimensions.z)));

        skySize /= SCALE_DOWN;
        skySize *= SCALE_UP;
    }
    else
    {
        skySize = m_skySize;
    }

    // work out sky position
    float3 skyPosition(float3(m_skyGroundHeight, m_skyGroundHeight, m_skyGroundHeight) * -skySize);

    m_pSkyRenderProxy->SetTransform(Transform(skyPosition, Quaternion::Identity, float3(skySize, skySize, skySize)));
    RedrawAllViewports();
}

void EditorMap::UpdateSunVariables()
{

}


