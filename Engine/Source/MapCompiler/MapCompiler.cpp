#include "MapCompiler/MapCompiler.h"
#include "MapCompiler/MapSource.h"
#include "MapCompiler/MapSourceTerrainData.h"
#include "ResourceCompiler/ObjectTemplate.h"
#include "ResourceCompiler/ObjectTemplateManager.h"
#include "ResourceCompiler/TerrainLayerListGenerator.h"
#include "ResourceCompiler/ClassTableGenerator.h"
#include "Engine/TerrainSection.h"
#include "Engine/DataFormats.h"
#include "Core/ClassTable.h"
#include "YBaseLib/ZipArchive.h"
Log_SetChannel(MapCompiler);

MapCompiler::MapCompiler(MapSource *pMapSource)
    : m_pMapSource(pMapSource),
      m_pInputStream(nullptr),
      m_pOutputStream(nullptr),
      m_pOutputArchive(nullptr),
      m_pClassTableGenerator(nullptr),
      m_regionSize(0),
      m_regionLODLevels(0),
      m_worldBoundingBox(float3::NegativeInfinite, float3::Infinite),
      m_pGlobalEntityData(nullptr)
{

}

MapCompiler::~MapCompiler()
{
    for (uint32 i = 0; i < m_regions.GetSize(); i++)
        delete m_regions[i];

    delete m_pGlobalEntityData;

    delete m_pClassTableGenerator;

    delete m_pOutputArchive;
    SAFE_RELEASE(m_pInputStream);
    SAFE_RELEASE(m_pOutputStream);
}

bool MapCompiler::StartFreshCompile(ByteStream *pOutputStream, ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    m_pOutputStream = pOutputStream;
    m_pOutputStream->AddRef();

    m_pOutputArchive = ZipArchive::CreateArchive(pOutputStream);
    if (m_pOutputArchive == nullptr)
    {
        pProgressCallbacks->DisplayError("Could not open zip archive");
        return false;
    }

    // create class table
    m_pClassTableGenerator = new ClassTableGenerator();

    // run prep steps
    if (!PrepareSourceForCompiling(pProgressCallbacks))
    {
        pProgressCallbacks->DisplayError("Could not prepare source");
        return false;
    }

    return true;
}

bool MapCompiler::StartReuseCompile(ByteStream *pInputStream, ByteStream *pOutputStream, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    //DF_MAP_HEADER mapHeader;
    return false;

}

bool MapCompiler::BuildAll(int32 mapLOD /*= -1*/, ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    // if mapLOD < 0, build all lods
    if (mapLOD < 0)
    {
        pProgressCallbacks->SetProgressRange(m_regionLODLevels);
        pProgressCallbacks->SetProgressValue(0);
        for (uint32 lodLevel = 0; lodLevel < m_regionLODLevels; lodLevel++)
        {
            pProgressCallbacks->SetProgressValue(lodLevel);
            pProgressCallbacks->PushState();
            if (!BuildAll(lodLevel, pProgressCallbacks))
            {
                pProgressCallbacks->PopState();
                return false;
            }
            pProgressCallbacks->PopState();
        }

        pProgressCallbacks->SetProgressValue(m_regionLODLevels);
        return true;
    }

    pProgressCallbacks->SetProgressRange(m_regions.GetSize() + 1);
    pProgressCallbacks->SetProgressValue(0);

    if (mapLOD == 0)
    {
        // build global entities [always done] for level 0
        pProgressCallbacks->PushState();
        if (!BuildGlobalEntities(pProgressCallbacks))
        {
            pProgressCallbacks->DisplayError("Failed to build global regions");
            pProgressCallbacks->PopState();
            return false;
        }
        pProgressCallbacks->PopState();
    }
    else
    {
        // ensure each region at lod 0 has a corresponding lod region
        for (uint32 i = 0; i < m_regions.GetSize(); i++)
        {
            if (m_regions[i]->RegionLODLevel == 0)
            {
                int32 regionX = m_regions[i]->RegionX;
                int32 regionY = m_regions[i]->RegionY;
                if (GetRegion(regionX, regionY, (uint32)mapLOD) == nullptr)
                    CreateRegion(regionX, regionY, (uint32)mapLOD);
            }
        }

        pProgressCallbacks->SetProgressRange(m_regions.GetSize() + 1);
    }

    pProgressCallbacks->SetProgressValue(1);

    // build each region
    for (uint32 i = 0; i < m_regions.GetSize(); i++)
    {
        if (m_regions[i]->RegionLODLevel == (uint32)mapLOD)
        {
            pProgressCallbacks->PushState();

            if (mapLOD == 0)
            {
                if (!BuildRegion(m_regions[i]->RegionX, m_regions[i]->RegionY, pProgressCallbacks))
                {
                    pProgressCallbacks->DisplayFormattedError("Failed to build region [%i, %i]", m_regions[i]->RegionX, m_regions[i]->RegionY);
                    pProgressCallbacks->PopState();
                    return false;
                }
            }
            else
            {
                if (!BuildRegionLOD(m_regions[i]->RegionX, m_regions[i]->RegionY, (uint32)mapLOD, pProgressCallbacks))
                {
                    pProgressCallbacks->DisplayFormattedError("Failed to build generate region [%i, %i] lod level %u", m_regions[i]->RegionX, m_regions[i]->RegionY, (uint32)mapLOD);
                    pProgressCallbacks->PopState();
                    return false;
                }
            }

            pProgressCallbacks->PopState();
        }

        pProgressCallbacks->SetProgressValue(i + 1);
    }

    // ok
    return true;
}

bool MapCompiler::BuildGlobalEntities(ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    if (m_newGlobalEntityRefs.GetSize() == 0)
    {
        delete m_pGlobalEntityData;
        return true;
    }

    pProgressCallbacks->SetStatusText("Building global regions...");

    // kill old data
    delete m_pGlobalEntityData;

    // build new data
    return CompileEntityData(m_newGlobalEntityRefs.GetBasePointer(), m_newGlobalEntityRefs.GetSize(), &m_pGlobalEntityData, pProgressCallbacks);
}

bool MapCompiler::BuildRegion(int32 regionX, int32 regionY, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    // load, and build all 3 components to the region
    pProgressCallbacks->SetProgressRange(3);
    pProgressCallbacks->SetProgressValue(0);

    // load if present in old stream
    Region *pRegion = GetRegion(regionX, regionY, 0);
    if (pRegion->RegionExistsInCurrentStream && !pRegion->RegionLoaded && !pRegion->LoadData())
    {
        pProgressCallbacks->DisplayFormattedError("Failed to load existing data for region [%i, %i]", regionX, regionY);
        return false;
    }
    pProgressCallbacks->SetProgressValue(1);

    // build entities
    {
        pProgressCallbacks->PushState();
        if (!BuildRegionEntities(regionX, regionY, pProgressCallbacks))
        {
            pProgressCallbacks->PopState();
            return false;
        }   
        pProgressCallbacks->PopState();
    }
    pProgressCallbacks->SetProgressValue(2);

    // build terrain
    if (m_pMapSource->HasTerrain())
    {
        pProgressCallbacks->PushState();
        if (!BuildRegionTerrain(regionX, regionY, pProgressCallbacks))
        {
            pProgressCallbacks->PopState();
            return false;
        }
        pProgressCallbacks->PopState();
    }
    pProgressCallbacks->SetProgressValue(3);

    // ok
    pProgressCallbacks->DisplayFormattedInformation("Built region [%i, %i]", regionX, regionY);
    return true;
}

bool MapCompiler::BuildRegionEntities(int32 regionX, int32 regionY, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    pProgressCallbacks->SetFormattedStatusText("Building region [%i, %i] entities...", regionX, regionY);

    // load if present in old stream
    Region *pRegion = GetRegion(regionX, regionY, 0);
    if (pRegion->RegionExistsInCurrentStream && !pRegion->RegionLoaded && !pRegion->LoadData())
    {
        pProgressCallbacks->DisplayFormattedError("Failed to load existing data for region [%i, %i]", regionX, regionY);
        return false;
    }

    // build entities
    return pRegion->RecompileEntityData(pProgressCallbacks);
}

bool MapCompiler::BuildRegionTerrain(int32 regionX, int32 regionY, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    DebugAssert(m_pMapSource->HasTerrain());

    pProgressCallbacks->SetFormattedStatusText("Building region [%i, %i] terrain...", regionX, regionY);

    // load if present in old stream
    Region *pRegion = GetRegion(regionX, regionY, 0);
    if (pRegion->RegionExistsInCurrentStream && !pRegion->RegionLoaded && !pRegion->LoadData())
    {
        pProgressCallbacks->DisplayFormattedError("Failed to load existing data for region [%i, %i]", regionX, regionY);
        return false;
    }

    // build terrain data
    return pRegion->RecompileTerrainData(pProgressCallbacks);
}

bool MapCompiler::FinalizeCompile(ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    pProgressCallbacks->SetProgressRange(m_regions.GetSize() + 6);
    pProgressCallbacks->SetProgressValue(0);

    // save any unsaved regions
    pProgressCallbacks->SetStatusText("Writing regions...");
    for (uint32 i = 0; i < m_regions.GetSize(); i++)
    {
        Region *pRegion = m_regions[i];
        
        // region has been changed but not saved?
        if (pRegion->RegionChanged && !pRegion->RegionExistsInNewStream && !SaveRegion(pRegion))
        {
            pProgressCallbacks->DisplayFormattedError("Failed to write region [%i, %i, %u]", pRegion->RegionX, pRegion->RegionY, pRegion->RegionLODLevel);
            return false;
        }

        pProgressCallbacks->IncrementProgressValue();
    }

    // rewrite header
    pProgressCallbacks->SetStatusText("Writing map header...");
    if (!SaveMapHeader())
    {
        pProgressCallbacks->DisplayError("Failed to write map header");
        return false;
    }
    pProgressCallbacks->IncrementProgressValue();

    // write regions header
    if (m_regions.GetSize() > 0)
    {
        if (!SaveRegionsHeader())
        {
            pProgressCallbacks->DisplayError("Failed to write regions header");
            return false;
        }
    }
    pProgressCallbacks->IncrementProgressValue();

    // write terrain header
    if (m_pMapSource->HasTerrain())
    {
        if (!SaveTerrainHeader())
        {
            pProgressCallbacks->DisplayError("Failed to write terrain header");
            return false;
        }
    }
    pProgressCallbacks->IncrementProgressValue();

    // write global entities header
    if (m_pGlobalEntityData != nullptr)
    {
        if (!SaveGlobalEntityData())
        {
            pProgressCallbacks->DisplayError("Failed to write global entities data");
            return false;
        }
    }
    pProgressCallbacks->IncrementProgressValue();

    // write class table
    if (m_pClassTableGenerator != nullptr)
    {
        if (!SaveClassTable())
        {
            pProgressCallbacks->DisplayError("Failed to write class table");
            return false;
        }
    }
    pProgressCallbacks->IncrementProgressValue();

    // commit the zipfile
    pProgressCallbacks->SetStatusText("Commiting archive...");
    if (!m_pOutputArchive->CommitChanges())
    {
        pProgressCallbacks->DisplayError("Failed to commit zip file");
        return false;
    }
    pProgressCallbacks->IncrementProgressValue();

    // read pointer no longer valid
    if (m_pInputStream != nullptr)
    {
        m_pInputStream->Release();
        m_pInputStream = nullptr;
    }

    // ok
    return true;
}

void MapCompiler::DiscardCompile()
{
    // discard write zip archive
    m_pOutputArchive->DiscardChanges();
    m_pInputStream = nullptr;
    m_pOutputStream = nullptr;
    m_pOutputArchive = nullptr;
}

bool MapCompiler::BuildRegionLOD(int32 regionX, int32 regionY, uint32 lodLevel, ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    // load, and build all 3 components to the region
    pProgressCallbacks->SetProgressRange(4);
    pProgressCallbacks->SetProgressValue(0);

    // load if present in old stream
    Region *pRegion = GetRegion(regionX, regionY, lodLevel);
    if (pRegion->RegionExistsInCurrentStream && !pRegion->RegionLoaded && !pRegion->LoadData())
    {
        pProgressCallbacks->DisplayFormattedError("Failed to load existing data for region [%i, %i, %u]", regionX, regionY, lodLevel);
        return false;
    }
    pProgressCallbacks->SetProgressValue(1);

    // build entities
    {
        pProgressCallbacks->PushState();
        if (!BuildRegionLODEntities(regionX, regionY, lodLevel, pProgressCallbacks))
        {
            pProgressCallbacks->PopState();
            return false;
        }
        pProgressCallbacks->PopState();
    }
    pProgressCallbacks->SetProgressValue(2);

    // build terrain
    if (m_pMapSource->HasTerrain())
    {
        pProgressCallbacks->PushState();
        if (!BuildRegionLODTerrain(regionX, regionY, lodLevel, pProgressCallbacks))
        {
            pProgressCallbacks->PopState();
            return false;
        }
        pProgressCallbacks->PopState();
    }
    pProgressCallbacks->SetProgressValue(3);

    // ok
    pProgressCallbacks->DisplayFormattedInformation("Built region [%i, %i, %u]", regionX, regionY, lodLevel);
    return true;
}

bool MapCompiler::BuildRegionLODEntities(int32 regionX, int32 regionY, uint32 lodLevel, ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    pProgressCallbacks->SetFormattedStatusText("Building region [%i, %i, %u] entities...", regionX, regionY, lodLevel);
    /*
    // load if present in old stream
    Region *pRegion = GetRegion(regionX, regionY, lodLevel);
    if (pRegion->RegionExistsInCurrentStream && !pRegion->RegionLoaded && !pRegion->LoadData())
    {
        pProgressCallbacks->DisplayFormattedError("Failed to load existing data for region [%i, %i, %i]", regionX, regionY, lodLevel);
        return false;
    }

    // build entities
    return pRegion->RecompileEntityData(pProgressCallbacks);
    */
    return true;
}

bool MapCompiler::BuildRegionLODTerrain(int32 regionX, int32 regionY, uint32 lodLevel, ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    DebugAssert(m_pMapSource->HasTerrain());

    pProgressCallbacks->SetFormattedStatusText("Building region [%i, %i, %u] terrain...", regionX, regionY, lodLevel);

    /*// load if present in old stream
    Region *pRegion = GetRegion(regionX, regionY);
    if (pRegion->RegionExistsInCurrentStream && !pRegion->RegionLoaded && !pRegion->LoadData())
    {
        pProgressCallbacks->DisplayFormattedError("Failed to load existing data for region [%i, %i, %u]", regionX, regionY, lodLevel);
        return false;
    }

    // build terrain data
    return pRegion->RecompileTerrainData(pProgressCallbacks);*/
    return true;
}

void MapCompiler::UpdateWorldBoundingBox(const AABox &boundingBox)
{
    UpdateWorldBoundingBox(boundingBox.GetMinBounds());
    UpdateWorldBoundingBox(boundingBox.GetMaxBounds());
}

void MapCompiler::UpdateWorldBoundingBox(const float3 &point)
{
    float3 newMinBounds(m_worldBoundingBox.GetMinBounds().Min(point));
    float3 newMaxBounds(m_worldBoundingBox.GetMaxBounds().Max(point));

    // update all 3 coordinates, if there is currently an infinte value, replace it
    for (uint32 i = 0; i < 3; i++)
    {
        if (newMinBounds[i] == -Y_FLT_INFINITE)
            newMinBounds[i] = point[i];
        if (newMaxBounds[i] == Y_FLT_INFINITE)
            newMaxBounds[i] = point[i];
    }

    m_worldBoundingBox.SetBounds(newMinBounds, newMaxBounds);
}

bool MapCompiler::CompileEntityDataSingle(const MapSourceEntityData *pEntityData, const ObjectTemplate *pTemplate, ByteStream *pOutputStream, ProgressCallbacks *pProgressCallbacks)
{
    uint64 reseekOffset;

    // does the entity exist in the class table? if not, add it
    if (m_pClassTableGenerator->GetTypeByName(pTemplate->GetTypeName()) == nullptr)
        m_pClassTableGenerator->CreateTypeFromPropertyTemplate(pTemplate->GetTypeName(), pTemplate->GetPropertyTemplate());

    // write entity header
    BinaryWriter binaryWriter(pOutputStream, ENDIAN_TYPE_LITTLE);
    uint64 entityHeaderOffset = binaryWriter.GetStreamPosition();
    DF_MAP_ENTITY_HEADER entityHeader;
    entityHeader.HeaderSize = sizeof(entityHeader);
    entityHeader.EntityNameLength = pEntityData->GetEntityName().GetLength();
    entityHeader.EntityTypeIndex = 0xFFFFFFFF;
    entityHeader.EntitySize = 0;
    entityHeader.ComponentsSize = 0;
    entityHeader.ComponentCount = 0;
    if (!binaryWriter.SafeWriteType(&entityHeader))
        return false;

    // write the entity name
    if (!binaryWriter.SafeWriteFixedString(pEntityData->GetEntityName(), pEntityData->GetEntityName().GetLength()))
        return false;

    // serialize the entity object
    uint32 writtenBytes;
    if (!m_pClassTableGenerator->SerializeObjectBinary(pOutputStream, pTemplate->GetTypeName(), pTemplate->GetPropertyTemplate(), pEntityData->GetPropertyTable(), &entityHeader.EntityTypeIndex, &writtenBytes))
        return false;

    // calculate size
    entityHeader.EntitySize = (uint32)(binaryWriter.GetStreamPosition() - entityHeaderOffset - sizeof(entityHeader));

    // write components
    uint64 componentsStartOffset = binaryWriter.GetStreamPosition();
    for (uint32 componentIndex = 0; componentIndex < pEntityData->GetComponentCount(); componentIndex++)
    {
        // template exists?
        const MapSourceEntityComponent *pComponentData = pEntityData->GetComponentByIndex(componentIndex);
        const ObjectTemplate *pComponentTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate(pComponentData->GetTypeName());
        if (pComponentTemplate == nullptr)
        {
            pProgressCallbacks->DisplayFormattedError("Skipping component '%s' of type '%s' in entity '%s' due to it being unknown.", pComponentData->GetComponentName().GetCharArray(), pComponentData->GetTypeName().GetCharArray(), pEntityData->GetEntityName().GetCharArray());
            continue;
        }

        // ensure it exists in the class table
        if (m_pClassTableGenerator->GetTypeByName(pComponentTemplate->GetTypeName()) == nullptr)
            m_pClassTableGenerator->CreateTypeFromPropertyTemplate(pComponentTemplate->GetTypeName(), pComponentTemplate->GetPropertyTemplate());

        // write the component header
        uint64 thisComponentHeaderOffset = binaryWriter.GetStreamPosition();
        DF_MAP_ENTITY_COMPONENT_HEADER componentHeader;
        componentHeader.ComponentSize = 0;
        componentHeader.ComponentNameLength = pComponentData->GetComponentName().GetLength();
        componentHeader.ComponentTypeIndex = 0xFFFFFFFF;
        if (!binaryWriter.SafeWriteType(&componentHeader))
            return false;

        // write component name
        if (!binaryWriter.SafeWriteFixedString(pComponentData->GetComponentName(), pComponentData->GetComponentName().GetLength()))
            return false;

        // serialize the component object
        if (!m_pClassTableGenerator->SerializeObjectBinary(pOutputStream, pComponentTemplate->GetTypeName(), pComponentTemplate->GetPropertyTemplate(), pComponentData->GetPropertyTable(), &componentHeader.ComponentTypeIndex, &writtenBytes))
            return false;

        // update the component size
        componentHeader.ComponentSize = (uint32)(binaryWriter.GetStreamPosition() - thisComponentHeaderOffset - sizeof(componentHeader));

        // rewrite the component header
        reseekOffset = binaryWriter.GetStreamPosition();
        if (!binaryWriter.SafeSeekAbsolute(thisComponentHeaderOffset) || !binaryWriter.SafeWriteType(&componentHeader) || !binaryWriter.SafeSeekAbsolute(reseekOffset))
            return false;

        // increment component count
        entityHeader.ComponentCount++;
    }

    // update components size
    entityHeader.ComponentsSize = (uint32)(binaryWriter.GetStreamPosition() - componentsStartOffset);

    // rewrite entity header
    reseekOffset = binaryWriter.GetStreamPosition();
    if (!binaryWriter.SafeSeekAbsolute(entityHeaderOffset) || !binaryWriter.SafeWriteType(&entityHeader) || !binaryWriter.SafeSeekAbsolute(reseekOffset))
        return false;

    // done
    return true;
}

bool MapCompiler::CompileEntityData(const MapSourceEntityData *const *ppEntities, uint32 entityCount, RegionEntityData **ppOutData, ProgressCallbacks *pProgressCallbacks)
{
    pProgressCallbacks->SetProgressRange(entityCount);
    pProgressCallbacks->SetProgressValue(0);

    // create stream
    AutoReleasePtr<ByteStream> pEntityStream = ByteStream_CreateGrowableMemoryStream();

    // compile entities
    for (uint32 i = 0; i < entityCount; i++)
    {
        const MapSourceEntityData *pEntityData = ppEntities[i];

        // find the template for it
        const ObjectTemplate *pObjectTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate(pEntityData->GetTypeName());
        if (pObjectTemplate == nullptr)
        {
            pProgressCallbacks->DisplayFormattedWarning("Cannot compile entity '%s': No template found for entity type '%s'", pEntityData->GetEntityName().GetCharArray(), pEntityData->GetTypeName().GetCharArray());
            pProgressCallbacks->IncrementProgressValue();
            continue;
        }

        // compile the entity
        if (!CompileEntityDataSingle(ppEntities[i], pObjectTemplate, pEntityStream, pProgressCallbacks))
        {
            pProgressCallbacks->DisplayFormattedError("Failed to compile entity '%s' ('%s')", pEntityData->GetEntityName().GetCharArray(), pEntityData->GetTypeName().GetCharArray());
            return false;
        }
        
        pProgressCallbacks->IncrementProgressValue();
    }

    // create output data
    *ppOutData = new RegionEntityData(entityCount, BinaryBlob::CreateFromStream(pEntityStream));
    return true;
}

bool MapCompiler::CompileTerrainSection(int32 sectionX, int32 sectionY, RegionTerrainSectionData **ppOutData)
{
    bool wasLoaded = m_pMapSource->GetTerrainData()->IsSectionLoaded(sectionX, sectionY);
    if (!wasLoaded && !m_pMapSource->GetTerrainData()->LoadSection(sectionX, sectionY))
        return false;

    // load the section
    TerrainSection *pSection = m_pMapSource->GetTerrainData()->GetSection(sectionX, sectionY);
    DebugAssert(pSection != nullptr);

    // optimize the section
    pSection->RebuildSplatMaps();
    if (!pSection->RebuildQuadTree())
    {
        Log_ErrorPrintf("MapCompiler::CompileTerrainSection: Quadtree generation failed [%i, %i]", sectionX, sectionY);
        delete pSection;
        return false;
    }

    // write it out
    AutoReleasePtr<ByteStream> pSectionStream = ByteStream_CreateGrowableMemoryStream();
    if (!pSection->SaveToStream(pSectionStream))
    {
        Log_ErrorPrintf("MapCompiler::CompileTerrainSection: Section write failed [%i, %i]", sectionX, sectionY);
        delete pSection;
        return false;
    }

    // if it wasn't loaded, flag it as unchanged and unload it
    if (!wasLoaded)
    {
        pSection->ClearChangedFlag();
        m_pMapSource->GetTerrainData()->UnloadSection(sectionX, sectionY);
    }

    // create data from it, easy
    *ppOutData = new RegionTerrainSectionData(sectionX, sectionY, BinaryBlob::CreateFromStream(pSectionStream));
    return true;
}

bool MapCompiler::PrepareSourceForCompiling(ProgressCallbacks *pProgressCallbacks)
{
    pProgressCallbacks->SetProgressRange(3);
    pProgressCallbacks->SetProgressValue(0);

    // init fields
    m_regionSize = m_pMapSource->GetRegionSize();
    m_regionLODLevels = m_pMapSource->GetRegionLODLevels();
    m_regionLoadRadius = m_pMapSource->GetProperties()->GetPropertyValueDefaultUInt32("RegionLoadRadius", 2);
    m_regionActivationRadius = m_pMapSource->GetProperties()->GetPropertyValueDefaultUInt32("RegionActivationRadius", 1);

    pProgressCallbacks->SetStatusText("Allocating regions for entities...");
    pProgressCallbacks->PushState();
    CreateRegions_Entities(pProgressCallbacks);
    pProgressCallbacks->PopState();

    if (m_pMapSource->HasTerrain())
    {
        pProgressCallbacks->SetStatusText("Allocating regions for terrain...");
        pProgressCallbacks->PushState();
        CreateRegions_Terrain(pProgressCallbacks);
        pProgressCallbacks->PopState();
    }

    return true;
}

MapCompiler::Region::Region(MapCompiler *pMapCompiler, int32 regionX, int32 regionY, uint32 lodLevel)
    : pMapCompiler(pMapCompiler), 
      RegionX(regionX),
      RegionY(regionY),
      RegionLODLevel(lodLevel),
      RegionExistsInCurrentStream(false),
      RegionExistsInNewStream(false),
      RegionLoaded(false),
      RegionChanged(false),
      CompiledEntityData(nullptr)
{

}

MapCompiler::Region::~Region()
{
    for (uint32 i = 0; i < CompiledTerrainData.GetSize(); i++)
        delete CompiledTerrainData[i];
    delete CompiledEntityData;
}

bool MapCompiler::Region::RecompileTerrainData(ProgressCallbacks *pProgressCallbacks)
{
    // delete existing data
    for (uint32 i = 0; i < CompiledTerrainData.GetSize(); i++)
        delete CompiledTerrainData[i];
    CompiledTerrainData.Clear();

    // create new data
    for (uint32 i = 0; i < NewTerrainSections.GetSize(); i++)
    {
        // compile this section
        RegionTerrainSectionData *pSectionData;
        if (!pMapCompiler->CompileTerrainSection(NewTerrainSections[i].x, NewTerrainSections[i].y, &pSectionData))
            return false;

        pProgressCallbacks->DisplayFormattedDebugMessage("terrain section %i,%i -> region %i,%i", NewTerrainSections[i].x, NewTerrainSections[i].y, RegionX, RegionY);
        CompiledTerrainData.Add(pSectionData);
    }

    return true;    
}

bool MapCompiler::Region::RecompileEntityData(ProgressCallbacks *pProgressCallbacks)
{
    // delete existing data
    delete CompiledEntityData;
    CompiledEntityData = nullptr;

    // create new data
    if (NewEntityRefs.GetSize() > 0)
    {
        if (!pMapCompiler->CompileEntityData(NewEntityRefs.GetBasePointer(), NewEntityRefs.GetSize(), &CompiledEntityData, pProgressCallbacks))
            return false;
    }

    return true;
}

bool MapCompiler::Region::LoadData()
{
    // load everything, nothing should've been written at this point!
    DebugAssert(CompiledTerrainData.GetSize() == 0);
    DebugAssert(CompiledEntityData == nullptr);
    RegionLoaded = true;
    return true;
}

MapCompiler::Region *MapCompiler::CreateRegion(int32 regionX, int32 regionY, uint32 lodLevel)
{
    DebugAssert(GetRegion(regionX, regionY, lodLevel) == nullptr);

    Region *pRegion = new Region(this, regionX, regionY, lodLevel);
    pRegion->RegionChanged = true;
    pRegion->RegionLoaded = true;
    m_regions.Add(pRegion);

    // calculate the min/max bounds of the region, and update the map bounding box
    UpdateWorldBoundingBox(float3(static_cast<float>(pRegion->RegionX * (int32)m_pMapSource->GetRegionSize()), static_cast<float>(pRegion->RegionY * (int32)m_pMapSource->GetRegionSize()), -Y_FLT_INFINITE));
    UpdateWorldBoundingBox(float3(static_cast<float>((pRegion->RegionX + 1) * (int32)m_pMapSource->GetRegionSize()), static_cast<float>((pRegion->RegionY + 1) * (int32)m_pMapSource->GetRegionSize()), Y_FLT_INFINITE));

    // return region
    return pRegion;
}

MapCompiler::Region *MapCompiler::GetRegion(int32 regionX, int32 regionY, uint32 lodLevel)
{
    for (uint32 i = 0; i < m_regions.GetSize(); i++)
    {
        Region *pRegion = m_regions[i];
        if (pRegion->RegionX == regionX && pRegion->RegionY == regionY && pRegion->RegionLODLevel == lodLevel)
            return pRegion;
    }

    return nullptr;
}

void MapCompiler::CreateRegions_Entities(ProgressCallbacks *pProgressCallbacks)
{
    // iterate through all entities in the map source, find the region for them, create the region if it doesn't exist, and append them to the new list for that section
    pProgressCallbacks->SetProgressRange(m_pMapSource->GetEntityCount());
    pProgressCallbacks->SetProgressValue(0);

    // find entities
    m_pMapSource->EnumerateEntities([this, pProgressCallbacks](const MapSourceEntityData *pEntityData) 
    {
        // is this a global entity?
        bool entityIsGlobal = pEntityData->GetPropertyTable()->GetPropertyValueDefaultBool("Global", false);
        if (entityIsGlobal)
        {
            // insert into global list
            m_newGlobalEntityRefs.Add(pEntityData);
        }
        else
        {
            // get region for this entity
            float3 entityPosition(pEntityData->GetPropertyTable()->GetPropertyValueDefaultFloat3("Position", float3::Zero));
            int2 entityRegion(m_pMapSource->GetRegionForPosition(entityPosition));

            // does this region exist?
            Region *pRegion = GetRegion(entityRegion.x, entityRegion.y, 0);
            if (pRegion == nullptr)
            {
                // create it
                pRegion = CreateRegion(entityRegion.x, entityRegion.y, 0);
            }

            // add new ref to region
            pRegion->NewEntityRefs.Add(pEntityData);
        }

        pProgressCallbacks->IncrementProgressValue();
    });
}

void MapCompiler::CreateRegions_Terrain(ProgressCallbacks *pProgressCallbacks)
{
    MapSourceTerrainData *pTerrainData = m_pMapSource->GetTerrainData();
    DebugAssert(pTerrainData != nullptr);
    if (pTerrainData == nullptr)
        return;

    // iterate through available sections, figure out the region from them, and create if needed
    pTerrainData->EnumerateAvailableSections([this](int32 sectionX, int32 sectionY)
    {
        // get region
        int2 sectionRegion(m_pMapSource->GetRegionForTerrainSection(sectionX, sectionY));

        // does this region exist?
        Region *pRegion = GetRegion(sectionRegion.x, sectionRegion.y, 0);
        if (pRegion == nullptr)
        {
            // create it
            pRegion = CreateRegion(sectionRegion.x, sectionRegion.y, 0);
        }

        // add new ref to region
        pRegion->NewTerrainSections.Add(int2(sectionX, sectionY));
    });
}

bool MapCompiler::SaveRegion(Region *pRegion)
{
    SmallString regionFileName;
    regionFileName.Format("region_%i_%i.%u", pRegion->RegionX, pRegion->RegionY, pRegion->RegionLODLevel);
    
    AutoReleasePtr<ByteStream> pStream = m_pOutputArchive->OpenFile(regionFileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
        return false;

    // sum up the data size of all terrains/block terrains/entitys
    uint32 terrainDataSize = 0;
    for (uint32 i = 0; i < pRegion->CompiledTerrainData.GetSize(); i++)
        terrainDataSize += sizeof(DF_MAP_REGION_TERRAIN_SECTION_HEADER) + pRegion->CompiledTerrainData[i]->pData->GetDataSize();

    // write header
    DF_MAP_REGION_HEADER regionHeader;
    regionHeader.HeaderSize = sizeof(regionHeader);
    regionHeader.LODLevel = pRegion->RegionLODLevel;
    regionHeader.RegionX = pRegion->RegionX;
    regionHeader.RegionY = pRegion->RegionY;
    regionHeader.TerrainSectionCount = pRegion->CompiledTerrainData.GetSize();
    regionHeader.TerrainDataSize = terrainDataSize;
    regionHeader.EntityCount = (pRegion->CompiledEntityData != nullptr) ? pRegion->CompiledEntityData->EntityCount : 0;
    regionHeader.EntityDataSize = (pRegion->CompiledEntityData != nullptr) ? pRegion->CompiledEntityData->pData->GetDataSize() : 0;
    if (!pStream->Write2(&regionHeader, sizeof(regionHeader)))
        return false;

    // write terrain data
    for (uint32 i = 0; i < pRegion->CompiledTerrainData.GetSize(); i++)
    {
        RegionTerrainSectionData *pData = pRegion->CompiledTerrainData[i];

        // write section header
        DF_MAP_REGION_TERRAIN_SECTION_HEADER sectionHeader;
        sectionHeader.SectionX = pData->SectionX;
        sectionHeader.SectionY = pData->SectionY;
        sectionHeader.LODLevel = 0;
        sectionHeader.DataSize = pData->pData->GetDataSize();
        if (!pStream->Write2(&sectionHeader, sizeof(sectionHeader)))
            return false;

        // write section data
        if (!pStream->Write2(pData->pData->GetDataPointer(), pData->pData->GetDataSize()))
            return false;
    }

    // write entity data
    if (pRegion->CompiledEntityData != nullptr)
    {
        if (!pStream->Write2(pRegion->CompiledEntityData->pData->GetDataPointer(), pRegion->CompiledEntityData->pData->GetDataSize()))
            return false;
    }

    // now exists in new file
    pRegion->RegionExistsInNewStream = true;

    // ok
    return true;
}

bool MapCompiler::SaveMapHeader()
{
    // open zeh file
    AutoReleasePtr<ByteStream> pStream = m_pOutputArchive->OpenFile(DF_MAP_HEADER_FILENAME, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
        return false;

    // write header
    DF_MAP_HEADER mapHeader;
    mapHeader.HeaderSize = sizeof(mapHeader);
    mapHeader.Version = DF_MAP_HEADER_VERSION;
    mapHeader.RegionSize = m_regionSize;
    mapHeader.RegionLODLevels = m_regionLODLevels;
    mapHeader.RegionLoadRadius = m_regionLoadRadius;
    mapHeader.RegionActivationRadius = m_regionActivationRadius;
    m_worldBoundingBox.GetMinBounds().Store(mapHeader.WorldBoundingBoxMin);
    m_worldBoundingBox.GetMaxBounds().Store(mapHeader.WorldBoundingBoxMax);
    mapHeader.HasRegions = (m_regions.GetSize() > 0) ? 1 : 0;
    mapHeader.HasTerrain = (m_pMapSource->HasTerrain()) ? 1 : 0;
    mapHeader.HasGlobalEntities = (m_pGlobalEntityData != nullptr) ? 1 : 0;
    if (!pStream->Write2(&mapHeader, sizeof(mapHeader)))
        return false;

    return true;
}

bool MapCompiler::SaveRegionsHeader()
{
    AutoReleasePtr<ByteStream> pStream = m_pOutputArchive->OpenFile(DF_MAP_REGIONS_HEADER_FILENAME, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
        return false;

    // count regions at lod 0
    uint32 regionCount = 0;
    for (uint32 i = 0; i < m_regions.GetSize(); i++)
    {
        if (m_regions[i]->RegionLODLevel == 0)
            regionCount++;
    }

    // write the header
    DF_MAP_REGIONS_HEADER regionsHeader;
    regionsHeader.HeaderSize = sizeof(regionsHeader);
    regionsHeader.RegionCount = regionCount;
    if (!pStream->Write2(&regionsHeader, sizeof(regionsHeader)))
        return false;

    // write region index
    for (uint32 i = 0; i < m_regions.GetSize(); i++)
    {
        if (m_regions[i]->RegionLODLevel != 0)
            continue;

        int2 regionCoordinates(m_regions[i]->RegionX, m_regions[i]->RegionY);
        if (!pStream->Write2(&regionCoordinates, sizeof(int2)))
            return false;
    }

    return true;
}

bool MapCompiler::SaveTerrainHeader()
{
    PathString fileName;
    const MapSourceTerrainData *pTerrainData = m_pMapSource->GetTerrainData();

    // write the header
    {
        AutoReleasePtr<ByteStream> pStream = m_pOutputArchive->OpenFile(DF_MAP_TERRAIN_HEADER_FILENAME, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_STREAMED);
        if (pStream == nullptr)
            return false;

        // retreive parameters
        const TerrainParameters *pTerrainParameters = pTerrainData->GetParameters();

        // write the header
        DF_MAP_TERRAIN_HEADER terrainHeader;
        terrainHeader.HeaderSize = sizeof(terrainHeader);
        terrainHeader.HeightStorageFormat = (uint32)pTerrainParameters->HeightStorageFormat;
        terrainHeader.MinHeight = pTerrainParameters->MinHeight;
        terrainHeader.MaxHeight = pTerrainParameters->MaxHeight;
        terrainHeader.BaseHeight = pTerrainParameters->BaseHeight;
        terrainHeader.Scale = pTerrainParameters->Scale;
        terrainHeader.SectionSize = pTerrainParameters->SectionSize;
        terrainHeader.LODCount = pTerrainParameters->LODCount;
        if (!pStream->Write2(&terrainHeader, sizeof(terrainHeader)))
            return false;
    }

    // write the layer list
    {
        fileName.Format("%s", DF_MAP_TERRAIN_LAYERS_FILENAME_PREFIX);

        AutoReleasePtr<ByteStream> pStream = m_pOutputArchive->OpenFile(fileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_SEEKABLE);
        if (pStream == nullptr)
            return false;

        // compile the layer list
        AutoReleasePtr<ByteStream> pLayerListStream = ByteStream_CreateGrowableMemoryStream();
        if (!pTerrainData->GetLayerListGenerator()->Compile(pStream))
        {
            Log_ErrorPrintf("Failed to compile terrain layers");
            return false;
        }        
    }
   
    return true;
}

bool MapCompiler::SaveClassTable()
{
    DebugAssert(m_pClassTableGenerator != nullptr);

    AutoReleasePtr<ByteStream> pStream = m_pOutputArchive->OpenFile(DF_MAP_CLASS_TABLE_FILENAME, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_SEEKABLE);
    if (pStream == nullptr)
        return false;

    if (!m_pClassTableGenerator->Compile(pStream))
        return false;

    return true;
}

bool MapCompiler::LoadGlobalEntityData()
{
    DebugAssert(m_pGlobalEntityData == nullptr);

    // write to the appropriate file
    AutoReleasePtr<ByteStream> pStream = m_pOutputArchive->OpenFile(DF_MAP_GLOBAL_ENTITIES_FILENAME, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
        return false;

    // read the header
    DF_MAP_GLOBAL_ENTITIES_HEADER header;
    if (!pStream->Read2(&header, sizeof(header)) || header.HeaderSize != sizeof(header))
        return false;

    // create data
    BinaryBlob *pData = BinaryBlob::Allocate(header.EntityDataSize);
    if (!pStream->Read2(pData->GetDataPointer(), header.EntityDataSize))
    {
        pData->Release();
        return false;
    }

    // store it
    m_pGlobalEntityData = new RegionEntityData(header.EntityCount, pData);
    return true;
}

bool MapCompiler::CompileGlobalEntityData(ProgressCallbacks *pProgressCallbacks)
{
    // delete old data
    delete m_pGlobalEntityData;
    m_pGlobalEntityData = nullptr;

    // write new data
    if (m_newGlobalEntityRefs.GetSize() > 0)
    {
        if (!CompileEntityData(m_newGlobalEntityRefs.GetBasePointer(), m_newGlobalEntityRefs.GetSize(), &m_pGlobalEntityData, pProgressCallbacks))
            return false;
    }

    return true;
}

bool MapCompiler::SaveGlobalEntityData()
{
    if (m_pGlobalEntityData == nullptr)
        return true;

    // write to the appropriate file
    ByteStream *pStream = m_pOutputArchive->OpenFile(DF_MAP_GLOBAL_ENTITIES_FILENAME, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
        return false;

    // write out the header
    DF_MAP_GLOBAL_ENTITIES_HEADER header;
    header.HeaderSize = sizeof(header);
    header.EntityCount = m_pGlobalEntityData->EntityCount;
    header.EntityDataSize = m_pGlobalEntityData->pData->GetDataSize();
    if (!pStream->Write2(&header, sizeof(header)))
    {
        pStream->Release();
        return false;
    }

    // write out the data
    if (!pStream->Write2(m_pGlobalEntityData->pData->GetDataPointer(), m_pGlobalEntityData->pData->GetDataSize()))
    {
        pStream->Release();
        return false;
    }

    // ok
    pStream->Release();
    return true;
}
