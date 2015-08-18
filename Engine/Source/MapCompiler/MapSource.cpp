#include "MapCompiler/MapSource.h"
#include "MapCompiler/MapSourceTerrainData.h"
#include "ResourceCompiler/ObjectTemplate.h"
#include "ResourceCompiler/ObjectTemplateManager.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
#include "YBaseLib/ZipArchive.h"
Log_SetChannel(MapSource);

static const int32 GLOBAL_REGION_COORDINATE = Y_INT32_MAX;
static const int32 UNALLOCATED_REGION_COORDINATE = Y_INT32_MAX - 1;
static const char *MAP_CLASS_NAME_FOR_TEMPLATE = "Map";

MapSource::MapSource()
    : m_pMapArchive(nullptr),
      m_isChanged(false),
      m_version(0),
      m_regionSize(0),
      m_regionLODLevels(1),
      m_pObjectTemplate(nullptr),
      m_nextEntitySuffix(1),
      m_entityListChanged(false),
      m_pTerrainData(nullptr)
{

}

MapSource::~MapSource()
{
    delete m_pTerrainData;

    delete m_pMapArchive;
}

bool MapSource::IsEntitiesChanged() const
{
    if (m_entityListChanged)
        return true;

    for (EntityTable::ConstIterator itr = m_entities.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value->IsChanged())
            return true;
    }

    return false;
}

bool MapSource::IsChanged() const
{
    if (m_isChanged)
        return true;

    // terrain
    if (m_pTerrainData != NULL && m_pTerrainData->IsChanged())
        return true;

    // entities
    if (IsEntitiesChanged())
        return true;

    return false;
}

void MapSource::Create(uint32 regionSize /* = 1024 */)
{
    SetDefaults();

    DebugAssert(regionSize > 0 && regionSize < Y_INT32_MAX);
    m_regionSize = regionSize;
    m_isChanged = true;

    m_nextEntitySuffix = 1;
    m_entityListChanged = true;

    // set some default properties
    m_pObjectTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate(MAP_CLASS_NAME_FOR_TEMPLATE);
    if (m_pObjectTemplate == nullptr)
        Log_WarningPrintf("MapSource::Create: failed to get map object template");
    else
        m_properties.CreateFromTemplate(m_pObjectTemplate->GetPropertyTemplate());
}

bool MapSource::Load(const char *fileName, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    m_filename = fileName;
    FileSystem::BuildOSPath(m_filename);

    AutoReleasePtr<ByteStream> pStream;
    pStream = FileSystem::OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE);
    if (pStream == NULL)
    {
        Log_ErrorPrintf("MapSource::Load: Could not open '%s'", fileName);
        return false;
    }

    return LoadFromStream(pStream, pProgressCallbacks);
}

bool MapSource::LoadFromStream(ByteStream *pArchiveStream, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    // init progress
    pProgressCallbacks->SetStatusText("Reading main file...");
    pProgressCallbacks->SetCancellable(false);
    pProgressCallbacks->SetProgressRange(1);
    pProgressCallbacks->SetProgressValue(0);

    // open zip
    if ((m_pMapArchive = ZipArchive::OpenArchiveReadOnly(pArchiveStream)) == NULL)
    {
        Log_ErrorPrintf("MapSource::LoadFromStream: Failed to parse zipfile.");
        return false;
    }

    // set defaults
    SetDefaults();

    // set up template
    m_pObjectTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate(MAP_CLASS_NAME_FOR_TEMPLATE);
    if (m_pObjectTemplate == nullptr)
        Log_WarningPrintf("MapSource::LoadFromStream: failed to get map object template");

    // open up map.xml
    {
        AutoReleasePtr<ByteStream> pStream = m_pMapArchive->OpenFile("map.xml", BYTESTREAM_OPEN_READ);
        if (pStream == NULL)
        {
            Log_ErrorPrintf("Could not create open map.xml");
            return false;
        }

        // create xml reader
        XMLReader xmlReader;
        if (!xmlReader.Create(pStream, "map.xml"))
        {
            Log_ErrorPrintf("Could not create xml reader for map.xml");
            return false;
        }

        // skip to map
        if (!xmlReader.SkipToElement("map"))
        {
            xmlReader.PrintError("could not skip to 'map' element");
            return false;
        }

        // read version
        m_version = Y_strtouint32(xmlReader.FetchAttributeDefault("version", "0"));
        if (m_version != 5)
        {
            xmlReader.PrintError("invalid version");
            return false;
        }

        // region size
        m_regionSize = StringConverter::StringToUInt32(xmlReader.FetchAttributeDefault("region-size", "0"));
        if (m_regionSize < 64 || m_regionSize > 16777216)
        {
            xmlReader.PrintError("invalid region size");
            return false;
        }

        // region lod count
        m_regionLODLevels = StringConverter::StringToUInt32(xmlReader.FetchAttributeDefault("region-lod-levels", "1"));
        if (m_regionLODLevels < 1 || m_regionLODLevels > 16)
        {
            xmlReader.PrintError("invalid region lod levels");
            return false;
        }

        // read elements
        if (!xmlReader.IsEmptyElement())
        {
            for (;;)
            {
                if (!xmlReader.NextToken())
                    break;

                int32 mapSelection;
                if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                {
                    mapSelection = xmlReader.Select("properties|regions|global-entities");
                    if (mapSelection < 0)
                        return false;

                    switch (mapSelection)
                    {
                        // properties
                    case 0:
                        {
                            if (!xmlReader.IsEmptyElement())
                            {
                                if (!m_properties.LoadFromXML(xmlReader))
                                {
                                    xmlReader.PrintError("failed to parse properties");
                                    return false;
                                }

                                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "properties") == 0);
                            }
                        }
                        break;

                    default:
                        UnreachableCode();
                        break;
                    }
                }
                else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                {
                    DebugAssert(!Y_stricmp(xmlReader.GetNodeName(), "map"));
                    break;
                }
            }
        }
    }

    // load entities
    if (!LoadEntities(pProgressCallbacks))
        return false;

    // load terrain if present
    if (!LoadTerrain(pProgressCallbacks))
        return false;

    pProgressCallbacks->SetProgressValue(1);
    return true;
}

bool MapSource::Save(ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    PathString fileName;
    SmallString tempString;
    bool destroyArchiveOnFailure = false;
    Assert(m_filename.GetLength() > 0);

    // setup progress
    pProgressCallbacks->SetCancellable(false);
    pProgressCallbacks->SetProgressRange(6);
    pProgressCallbacks->SetProgressValue(0);
    pProgressCallbacks->SetStatusText("Opening destination map...");

    // open this file, using atomic updates the preserve the original for now
    AutoReleasePtr<ByteStream> pArchiveStream = FileSystem::OpenFile(m_filename, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_ATOMIC_UPDATE);
    if (pArchiveStream == NULL)
    {
        Log_ErrorPrintf("MapSource::Save: Could not open '%s'", m_filename.GetCharArray());
        return false;
    }

    if (m_pMapArchive != NULL)
    {
        // upgrade the zip archive to a writable archive
        if (!m_pMapArchive->UpgradeToWritableArchive(pArchiveStream))
        {
            pArchiveStream->Discard();
            return false;
        }
    }
    else
    {
        // map has not been saved once yet, so create the archive
        if ((m_pMapArchive = ZipArchive::CreateArchive(pArchiveStream)) == NULL)
        {
            pArchiveStream->Discard();
            return false;
        }

        // we should destroy it on failure
        destroyArchiveOnFailure = true;
    }

    pProgressCallbacks->SetProgressValue(1);
    pProgressCallbacks->SetStatusText("Writing map header...");

    // save map.xml if anything has changed
    {
        AutoReleasePtr<ByteStream> pStream = m_pMapArchive->OpenFile("map.xml", BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE);
        if (pStream == NULL)
            goto FAILURE;

        XMLWriter xmlWriter;
        if (!xmlWriter.Create(pStream))
            goto FAILURE;

        // write header
        xmlWriter.StartElement("map");
        {
            xmlWriter.WriteAttributef("version", "%u", m_version);
            xmlWriter.WriteAttributef("region-size", "%u", m_regionSize);
            xmlWriter.WriteAttributef("region-lod-levels", "%u", m_regionLODLevels);

            // write properties
            xmlWriter.StartElement("properties");
            m_properties.SaveToXML(xmlWriter);
            xmlWriter.EndElement();
        }
        xmlWriter.EndElement();

        if (xmlWriter.InErrorState())
        {
            xmlWriter.Close();
            goto FAILURE;
        }

        xmlWriter.Close();
    }

    pProgressCallbacks->SetProgressValue(2);

    // write entities
    {
        pProgressCallbacks->SetStatusText("Writing entities...");
        pProgressCallbacks->PushState();

        if (IsEntitiesChanged() && !SaveEntities(pProgressCallbacks))
        {
            pProgressCallbacks->PopState();
            goto FAILURE;
        }

        pProgressCallbacks->PopState();
        pProgressCallbacks->SetProgressValue(3);
    }

    // write terrain
    {
        pProgressCallbacks->SetStatusText("Writing terrain...");
        pProgressCallbacks->PushState();

        if (!SaveTerrain(pProgressCallbacks))
        {
            pProgressCallbacks->PopState();
            goto FAILURE;
        }

        pProgressCallbacks->PopState();
        pProgressCallbacks->SetProgressValue(4);
    }
    
    // commit the changes to zip
    pProgressCallbacks->SetStatusText("Commiting to archive...");
    if (!m_pMapArchive->CommitChanges())
        goto FAILURE;

    // read stream should now be closed, so we can commit the updated zip
    if (!pArchiveStream->Commit())
    {
        Log_ErrorPrintf("MapSource::Save: Failed to commit the archive stream.");
        goto FAILURE_DISCARD;
    }

    // clear all the flags
    m_isChanged = false;
    pProgressCallbacks->SetProgressValue(6);
    return true;

FAILURE:
    m_pMapArchive->DiscardChanges();

FAILURE_DISCARD:
    if (destroyArchiveOnFailure)
    {
        delete m_pMapArchive;
        m_pMapArchive = NULL;
    }
    pArchiveStream->Discard();
    return false;
}

bool MapSource::SaveAs(const char *newFileName, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    // preserve old filename
    String oldFileName = m_filename;

    // alter it, and save
    m_filename = newFileName;
    FileSystem::BuildOSPath(m_filename);
    if (!Save(pProgressCallbacks))
    {
        // revert to old filename
        m_filename = oldFileName;
        return false;
    }

    return true;
}

bool MapSource::LoadEntities(ProgressCallbacks *pProgressCallbacks)
{
    static const char *FILENAME = "entities.xml";

    AutoReleasePtr<ByteStream> pStream = m_pMapArchive->OpenFile(FILENAME, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
    {
        // entities should exist even if there are no entities
        pProgressCallbacks->DisplayError("Could not open entities.xml");
        return false;
    }

    // progress
    pProgressCallbacks->SetStatusText("Loading entities...");
    pProgressCallbacks->UpdateProgressFromStream(pStream);

    // create reader
    XMLReader xmlReader;
    if (!xmlReader.Create(pStream, FILENAME))
        return false;

    // skip to map
    if (!xmlReader.SkipToElement("entities"))
    {
        xmlReader.PrintError("could not skip to 'entities' element");
        return false;
    }

    // load the next suffix attribute
    const char *nextStuffixStr = xmlReader.FetchAttribute("next-entity-suffix");
    if (nextStuffixStr == nullptr)
    {
        xmlReader.PrintError("missing next-entity-suffix attribute");
        return false;
    }
    m_nextEntitySuffix = StringConverter::StringToUInt32(nextStuffixStr);

    // read elements
    if (!xmlReader.IsEmptyElement())
    {
        for (;;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                if (xmlReader.Select("entity") != 0)
                    return false;

                MapSourceEntityData *pEntityData = new MapSourceEntityData();
                if (!pEntityData->LoadFromXML(xmlReader))
                {
                    delete pEntityData;
                    return false;
                }

                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "entity") == 0);

                // search for entity with this name in map
                if (m_entities.Find(pEntityData->GetEntityName()) != nullptr)
                {
                    xmlReader.PrintError("entity with name '%s' already exists", pEntityData->GetEntityName().GetCharArray());
                    delete pEntityData;
                    return false;
                }

                // insert into map
                m_entities.Insert(pEntityData->GetEntityName(), pEntityData);
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "entities") == 0);
                break;
            }

            pProgressCallbacks->UpdateProgressFromStream(pStream);
        }
    }

    pProgressCallbacks->UpdateProgressFromStream(pStream);
    return true;
}

bool MapSource::SaveEntities(ProgressCallbacks *pProgressCallbacks) const
{
    static const char *FILENAME = "entities.xml";

    pProgressCallbacks->SetStatusText("Saving entities...");
    pProgressCallbacks->SetProgressRange(m_entities.GetMemberCount());
    pProgressCallbacks->SetProgressValue(0);

    ByteStream *pStream = m_pMapArchive->OpenFile(FILENAME, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
    {
        pProgressCallbacks->DisplayError("Could not open entities.xml");
        return false;
    }

    // create writer
    XMLWriter xmlWriter;
    if (!xmlWriter.Create(pStream))
    {
        pStream->Release();
        return false;
    }

    // write out header
    xmlWriter.StartElement("entities");
    xmlWriter.WriteAttribute("next-entity-suffix", StringConverter::UInt32ToString(m_nextEntitySuffix));
    
    // write entities
    for (EntityTable::ConstIterator itr = m_entities.Begin(); !itr.AtEnd(); itr.Forward())
    {
        const MapSourceEntityData *pEntityData = itr->Value;

        xmlWriter.StartElement("entity");
        pEntityData->SaveToXML(xmlWriter);
        xmlWriter.EndElement();
    }

    // end document
    xmlWriter.EndElement();
    xmlWriter.Close();
    pStream->Release();

    // clear saved flags
    m_entityListChanged = false;
    return true;
}

const MapSourceEntityData *MapSource::GetEntityData(const char *entityName) const
{
    const EntityTable::Member *pMember = m_entities.Find(entityName);
    return (pMember != NULL) ? pMember->Value : NULL;
}

MapSourceEntityData *MapSource::GetEntityData(const char *entityName)
{
    EntityTable::Member *pMember = m_entities.Find(entityName);
    return (pMember != NULL) ? pMember->Value : NULL;
}

String MapSource::GenerateEntityName(const char *typeName)
{
    SmallString entityName;

    // keep generating names until one is free
    do
    {
        entityName.Clear();
        entityName.AppendString(typeName);
        entityName.AppendFormattedString("_%u", m_nextEntitySuffix++);
    }
    while (m_entities.Find(entityName) != nullptr);

    return entityName;
}

MapSourceEntityData *MapSource::CreateEntity(const char *typeName, const char *entityName /* = nullptr */)
{
    String newEntityName;
    
    // check the name does not exist before using it
    if (entityName == nullptr || m_entities.Find(entityName) != nullptr)
        newEntityName = GenerateEntityName(typeName);
    else
        newEntityName = entityName;

    MapSourceEntityData *pEntityData = new MapSourceEntityData();
    pEntityData->Create(newEntityName, typeName);
    m_entities.Insert(pEntityData->GetEntityName(), pEntityData);
    m_entityListChanged = true;
    return pEntityData;
}

MapSourceEntityData *MapSource::CreateEntityFromTemplate(const ObjectTemplate *pTemplate, const char *entityName /* = nullptr */)
{
    String newEntityName;

    // check the name does not exist before using it
    if (entityName == nullptr || m_entities.Find(entityName) != nullptr)
        newEntityName = GenerateEntityName(pTemplate->GetTypeName());
    else
        newEntityName = entityName;

    MapSourceEntityData *pEntityData = new MapSourceEntityData();
    pEntityData->CreateFromTemplate(newEntityName, pTemplate);
    m_entities.Insert(pEntityData->GetEntityName(), pEntityData);
    m_entityListChanged = true;
    return pEntityData;
}

MapSourceEntityData *MapSource::CopyEntity(const MapSourceEntityData *pEntity, const char *entityName /* = nullptr */)
{
    String newEntityName;

    // check the name does not exist before using it
    if (entityName == nullptr || m_entities.Find(entityName) != nullptr)
        newEntityName = GenerateEntityName(pEntity->GetTypeName());
    else
        newEntityName = entityName;

    MapSourceEntityData *pNewEntityData = new MapSourceEntityData();
    pNewEntityData->Create(newEntityName, pEntity->GetTypeName());
    pNewEntityData->m_properties.CopyProperties(pEntity->GetPropertyTable());
    m_entities.Insert(pNewEntityData->GetEntityName(), pNewEntityData);
    m_entityListChanged = true;
    return pNewEntityData;
}

bool MapSource::AddEntity(MapSourceEntityData *pEntityData)
{
    if (m_entities.Find(pEntityData->GetEntityName()) != nullptr)
        return false;

    // add entity
    pEntityData->SetChangedFlag();
    m_entities.Insert(pEntityData->GetEntityName(), pEntityData);
    m_entityListChanged = true;
    return true;
}

void MapSource::RemoveEntity(MapSourceEntityData *pEntityData)
{
    EntityTable::Member *pMember = nullptr;
    for (EntityTable::Iterator itr = m_entities.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->Value == pEntityData)
        {
            pMember = &(*itr);
            break;
        }
    }

    DebugAssert(pMember != nullptr);
    m_entities.Remove(pMember);
    m_entityListChanged = true;
}

bool MapSource::RemoveEntity(const char *entityName)
{
    EntityTable::Member *pMember = m_entities.Find(entityName);
    if (pMember == nullptr)
        return false;

    // remove entity
    MapSourceEntityData *pEntityData = pMember->Value;
    RemoveEntity(pEntityData);
    delete pEntityData;
    return true;
}

void MapSource::SetDefaults()
{
    m_version = 5;
    m_regionSize = 1024;
    m_isChanged = true;
}

const int2 MapSource::GetRegionForPosition(const float3 &position) const
{
    return int2((Math::Truncate(position.x) / (int32)m_regionSize) - ((position.x < 0.0f) ? 1 : 0),
                (Math::Truncate(position.y) / (int32)m_regionSize) - ((position.y < 0.0f) ? 1 : 0));
}

const int2 MapSource::GetRegionForTerrainSection(int32 sectionX, int32 sectionY) const
{
    DebugAssert(m_pTerrainData != nullptr);
    int32 sectionsPerRegion = (int32)(m_regionSize / m_pTerrainData->GetParameters()->SectionSize);
    
    return int2((sectionX >= 0) ? (sectionX / sectionsPerRegion) : (Min(sectionX, -sectionsPerRegion) / sectionsPerRegion), 
                (sectionY >= 0) ? (sectionY / sectionsPerRegion) : (Min(sectionY, -sectionsPerRegion) / sectionsPerRegion));
}

MapSourceEntityComponent::MapSourceEntityComponent()
    : m_changed(false)
{

}

MapSourceEntityComponent::~MapSourceEntityComponent()
{

}

void MapSourceEntityComponent::Create(const String &componentName, const String &typeName)
{
    m_componentName = componentName;
    m_typeName = typeName;
    m_changed = true;
}

void MapSourceEntityComponent::CreateFromTemplate(const String &componentName, const ObjectTemplate *pTemplate)
{
    m_componentName = componentName;
    m_typeName = pTemplate->GetTypeName();
    m_properties.CreateFromTemplate(pTemplate->GetPropertyTemplate());
    m_changed = true;
}

bool MapSourceEntityComponent::LoadFromXML(XMLReader &xmlReader)
{
    const char *componentNameStr = xmlReader.FetchAttribute("name");
    const char *componentTypeStr = xmlReader.FetchAttribute("type");
    if (componentNameStr == nullptr || componentTypeStr == nullptr)
    {
        xmlReader.PrintError("incomplete component definition");
        return false;
    }

    m_componentName = componentNameStr;
    m_typeName = componentTypeStr;

    // read fields
    if (!xmlReader.IsEmptyElement())
    {
        for (;;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 objectSelection = xmlReader.Select("properties");
                if (objectSelection < 0)
                    return false;

                switch (objectSelection)
                {
                    // properties
                case 0:
                {
                    if (!xmlReader.IsEmptyElement())
                    {
                        if (!m_properties.LoadFromXML(xmlReader))
                            return false;

                        DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "properties") == 0);
                    }
                }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                break;
            }
        }
    }

    return true;
}

void MapSourceEntityComponent::SaveToXML(XMLWriter &xmlWriter) const
{
    xmlWriter.WriteAttribute("name", m_componentName);
    xmlWriter.WriteAttribute("type", m_typeName);

    // write properties
    xmlWriter.StartElement("properties");
    m_properties.SaveToXML(xmlWriter);
    xmlWriter.EndElement();

    m_changed = false;
}

const String *MapSourceEntityComponent::GetPropertyValuePointer(const char *propertyName) const
{
    return m_properties.GetPropertyValuePointer(propertyName);
}

String MapSourceEntityComponent::GetPropertyValueString(const char *propertyName) const
{
    return m_properties.GetPropertyValueDefaultString(propertyName, EmptyString);
}

void MapSourceEntityComponent::SetPropertyValue(const char *propertyName, const char *propertyValue)
{
    m_properties.SetPropertyValue(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityComponent::SetPropertyValue(const char *propertyName, const String &propertyValue)
{
    m_properties.SetPropertyValueString(propertyName, propertyValue);
    m_changed = true;
}

MapSourceEntityData::MapSourceEntityData()
    : m_changed(false)
{

}

MapSourceEntityData::~MapSourceEntityData()
{
    for (uint32 i = 0; i < m_components.GetSize(); i++)
        delete m_components[i];
}

const bool MapSourceEntityData::IsChanged() const
{
    if (m_changed)
        return true;

    for (uint32 i = 0; i < m_components.GetSize(); i++)
    {
        if (m_components[i]->IsChanged())
            return true;
    }

    return false;
}

void MapSourceEntityData::SetChangedFlag()
{
    m_changed = true;
}

void MapSourceEntityData::ClearChangedFlag()
{
    m_changed = false;

    for (uint32 i = 0; i < m_components.GetSize(); i++)
        m_components[i]->ClearChangedFlag();
}

const MapSourceEntityComponent *MapSourceEntityData::GetComponentByName(const char *name) const
{
    for (uint32 i = 0; i < m_components.GetSize(); i++)
    {
        if (m_components[i]->GetComponentName().CompareInsensitive(name))
            return m_components[i];
    }

    return nullptr;
}

MapSourceEntityComponent *MapSourceEntityData::GetComponentByName(const char *name)
{
    for (uint32 i = 0; i < m_components.GetSize(); i++)
    {
        if (m_components[i]->GetComponentName().CompareInsensitive(name))
            return m_components[i];
    }

    return nullptr;
}

MapSourceEntityComponent *MapSourceEntityData::CreateComponent(const char *typeName, const char *componentName /* = nullptr */)
{
    const ObjectTemplate *pTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate(typeName);
    if (pTemplate == nullptr)
        return  nullptr;
    
    return CreateComponent(pTemplate, componentName);
}

MapSourceEntityComponent *MapSourceEntityData::CreateComponent(const ObjectTemplate *pTemplate, const char *componentName /* = nullptr */)
{
    // check the name isn't used
    if (!pTemplate->CanCreate() || (componentName != nullptr && GetComponentByName(componentName) != nullptr))
        return nullptr;

    // generate a name
    String componentNameString;
    if (componentName == nullptr)
    {
        uint32 underscoreCount = 0;

        for (;;)
        {
            componentNameString.Format("%s_%u", pTemplate->GetTypeName().GetCharArray(), m_components.GetSize());
            for (uint32 i = 0; i < underscoreCount; i++)
                componentNameString.AppendCharacter('_');

            if (GetComponentByName(componentNameString) == nullptr)
                break;

            underscoreCount++;
        }
    }
    else
    {
        componentNameString = componentName;
    }

    // create it
    MapSourceEntityComponent *pComponentData = new MapSourceEntityComponent();
    pComponentData->CreateFromTemplate(componentNameString, pTemplate);
    m_components.Add(pComponentData);
    return pComponentData;
}

void MapSourceEntityData::AddComponent(MapSourceEntityComponent *pComponent)
{
    DebugAssert(m_components.IndexOf(pComponent) < 0);
    DebugAssert(GetComponentByName(pComponent->GetComponentName()) == nullptr);

    m_components.Add(pComponent);
}

void MapSourceEntityData::RemoveComponent(MapSourceEntityComponent *pComponent)
{
    int32 index = m_components.IndexOf(pComponent);
    Assert(index >= 0);

    m_components.OrderedRemove(index);
}

const String *MapSourceEntityData::GetPropertyValuePointer(const char *propertyName) const
{
    return m_properties.GetPropertyValuePointer(propertyName);
}

String MapSourceEntityData::GetPropertyValueString(const char *propertyName) const
{
    return m_properties.GetPropertyValueDefaultString(propertyName, EmptyString);
}

void MapSourceEntityData::SetPropertyValue(const char *propertyName, const char *propertyValue)
{
    m_properties.SetPropertyValue(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValue(const char *propertyName, const String &propertyValue)
{
    m_properties.SetPropertyValueString(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::Create(const String &entityName, const String &typeName)
{
    m_entityName = entityName;
    m_typeName = typeName;
    m_changed = true;
}

void MapSourceEntityData::CreateFromTemplate(const String &entityName, const ObjectTemplate *pTemplate)
{
    m_entityName = entityName;
    m_typeName = pTemplate->GetTypeName();
    m_properties.CreateFromTemplate(pTemplate->GetPropertyTemplate());
    m_changed = true;
}

bool MapSourceEntityData::LoadFromXML(XMLReader &xmlReader)
{
    const char *entityNameStr = xmlReader.FetchAttribute("name");
    const char *entityTypeStr = xmlReader.FetchAttribute("type");
    if (entityNameStr == nullptr || entityTypeStr == nullptr)
    {
        xmlReader.PrintError("incomplete entity definition");
        return false;
    }

    m_entityName = entityNameStr;
    m_typeName = entityTypeStr;

    // read fields
    if (!xmlReader.IsEmptyElement())
    {
        for (;;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 objectSelection = xmlReader.Select("components|properties|script");
                if (objectSelection < 0)
                    return false;

                switch (objectSelection)
                {
                    // components
                case 0:
                    {
                        if (!xmlReader.IsEmptyElement())
                        {
                            for (;;)
                            {
                                if (!xmlReader.NextToken())
                                    return false;

                                if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                {
                                    int32 componentsSelection = xmlReader.Select("component");
                                    if (componentsSelection < 0 || xmlReader.IsEmptyElement())
                                        return false;

                                    MapSourceEntityComponent *pComponent = new MapSourceEntityComponent();
                                    if (!pComponent->LoadFromXML(xmlReader))
                                    {
                                        delete pComponent;
                                        return false;
                                    }

                                    if (GetComponentByName(pComponent->GetComponentName()) != nullptr)
                                    {
                                        xmlReader.PrintError("duplicate component named '%s'", pComponent->GetComponentName().GetCharArray());
                                        delete pComponent;
                                        return false;
                                    }

                                    m_components.Add(pComponent);

                                    if (!xmlReader.ExpectEndOfElementName("component"))
                                        return false;
                                }
                                else
                                {
                                    if (!xmlReader.ExpectEndOfElementName("components"))
                                        return false;

                                    break;
                                }                            
                            }
                        }
                    }
                    break;

                    // properties
                case 1:
                    {
                        if (!xmlReader.IsEmptyElement())
                        {
                            if (!m_properties.LoadFromXML(xmlReader))
                                return false;
    
                            DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "properties") == 0);
                        }
                    }
                    break;

                    // script
                case 2:
                    {
                        if (!xmlReader.IsEmptyElement())
                        {
                            if (!xmlReader.MoveToElement())
                                return false;

                            m_script = xmlReader.GetElementText();
                            if (!xmlReader.SkipCurrentElement())
                                return false;
                        }
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                break;
            }
        }
    }

    return true;
}

void MapSourceEntityData::SaveToXML(XMLWriter &xmlWriter) const
{
    xmlWriter.WriteAttribute("name", m_entityName);
    xmlWriter.WriteAttribute("type", m_typeName);

    // write components
    xmlWriter.StartElement("components");
    {
        for (uint32 i = 0; i < m_components.GetSize(); i++)
        {
            xmlWriter.StartElement("component");
            m_components[i]->SaveToXML(xmlWriter);
            xmlWriter.EndElement();
        }
    }
    xmlWriter.EndElement();

    // write properties
    xmlWriter.StartElement("properties");
    m_properties.SaveToXML(xmlWriter);
    xmlWriter.EndElement();

    // write script
    if (!m_script.IsEmpty())
    {
        xmlWriter.StartElement("script");
        xmlWriter.WriteCDATA(m_script);
        xmlWriter.EndElement();
    }

    m_changed = false;
}

bool MapSourceEntityData::GetPropertyValueBool(const char *propertyName, bool *pValue) const
{
    return m_properties.GetPropertyValueBool(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueInt8(const char *propertyName, int8 *pValue) const
{
    return m_properties.GetPropertyValueInt8(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueInt16(const char *propertyName, int16 *pValue) const
{
    return m_properties.GetPropertyValueInt16(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueInt32(const char *propertyName, int32 *pValue) const
{
    return m_properties.GetPropertyValueInt32(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueInt64(const char *propertyName, int64 *pValue) const
{
    return m_properties.GetPropertyValueInt64(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueUInt8(const char *propertyName, uint8 *pValue) const
{
    return m_properties.GetPropertyValueUInt8(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueUInt16(const char *propertyName, uint16 *pValue) const
{
    return m_properties.GetPropertyValueUInt16(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueUInt32(const char *propertyName, uint32 *pValue) const
{
    return m_properties.GetPropertyValueUInt32(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueUInt64(const char *propertyName, uint64 *pValue) const
{
    return m_properties.GetPropertyValueUInt64(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueFloat(const char *propertyName, float *pValue) const
{
    return m_properties.GetPropertyValueFloat(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueDouble(const char *propertyName, double *pValue) const
{
    return m_properties.GetPropertyValueDouble(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueColor(const char *propertyName, uint32 *pValue) const
{
    return m_properties.GetPropertyValueColor(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueInt2(const char *propertyName, int2 *pValue) const
{
    return m_properties.GetPropertyValueInt2(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueInt3(const char *propertyName, int3 *pValue) const
{
    return m_properties.GetPropertyValueInt3(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueInt4(const char *propertyName, int4 *pValue) const
{
    return m_properties.GetPropertyValueInt4(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueUInt2(const char *propertyName, uint2 *pValue) const
{
    return m_properties.GetPropertyValueUInt2(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueUInt3(const char *propertyName, uint3 *pValue) const
{
    return m_properties.GetPropertyValueUInt3(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueUInt4(const char *propertyName, uint4 *pValue) const
{
    return m_properties.GetPropertyValueUInt4(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueFloat2(const char *propertyName, float2 *pValue) const
{
    return m_properties.GetPropertyValueFloat2(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueFloat3(const char *propertyName, float3 *pValue) const
{
    return m_properties.GetPropertyValueFloat3(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueFloat4(const char *propertyName, float4 *pValue) const
{
    return m_properties.GetPropertyValueFloat4(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueQuaternion(const char *propertyName, Quaternion *pValue) const
{
    return m_properties.GetPropertyValueQuaternion(propertyName, pValue);
}

bool MapSourceEntityData::GetPropertyValueDefaultBool(const char *propertyName, bool defaultValue /*= false*/) const
{
    return m_properties.GetPropertyValueDefaultBool(propertyName, defaultValue);
}

int8 MapSourceEntityData::GetPropertyValueDefaultInt8(const char *propertyName, int8 defaultValue /*= 0*/) const
{
    return m_properties.GetPropertyValueDefaultInt8(propertyName, defaultValue);
}

int16 MapSourceEntityData::GetPropertyValueDefaultInt16(const char *propertyName, int16 defaultValue /*= 0*/) const
{
    return m_properties.GetPropertyValueDefaultInt16(propertyName, defaultValue);
}

int32 MapSourceEntityData::GetPropertyValueDefaultInt32(const char *propertyName, int32 defaultValue /*= 0*/) const
{
    return m_properties.GetPropertyValueDefaultInt32(propertyName, defaultValue);
}

int64 MapSourceEntityData::GetPropertyValueDefaultInt64(const char *propertyName, int64 defaultValue /*= 0*/) const
{
    return m_properties.GetPropertyValueDefaultInt64(propertyName, defaultValue);
}

uint8 MapSourceEntityData::GetPropertyValueDefaultUInt8(const char *propertyName, uint8 defaultValue /*= 0*/) const
{
    return m_properties.GetPropertyValueDefaultUInt8(propertyName, defaultValue);
}

uint16 MapSourceEntityData::GetPropertyValueDefaultUInt16(const char *propertyName, uint16 defaultValue /*= 0*/) const
{
    return m_properties.GetPropertyValueDefaultUInt16(propertyName, defaultValue);
}

uint32 MapSourceEntityData::GetPropertyValueDefaultUInt32(const char *propertyName, uint32 defaultValue /*= 0*/) const
{
    return m_properties.GetPropertyValueDefaultUInt32(propertyName, defaultValue);
}

uint64 MapSourceEntityData::GetPropertyValueDefaultUInt64(const char *propertyName, uint64 defaultValue /*= 0*/) const
{
    return m_properties.GetPropertyValueDefaultUInt64(propertyName, defaultValue);
}

float MapSourceEntityData::GetPropertyValueDefaultFloat(const char *propertyName, float defaultValue /*= 0*/) const
{
    return m_properties.GetPropertyValueDefaultFloat(propertyName, defaultValue);
}

double MapSourceEntityData::GetPropertyValueDefaultDouble(const char *propertyName, double defaultValue /*= 0*/) const
{
    return m_properties.GetPropertyValueDefaultDouble(propertyName, defaultValue);
}

uint32 MapSourceEntityData::GetPropertyValueDefaultColor(const char *propertyName, uint32 defaultValue /*= 0*/) const
{
    return m_properties.GetPropertyValueDefaultColor(propertyName, defaultValue);
}

int2 MapSourceEntityData::GetPropertyValueDefaultInt2(const char *propertyName, const int2 &defaultValue /*= int2::Zero*/) const
{
    return m_properties.GetPropertyValueDefaultInt2(propertyName, defaultValue);
}

int3 MapSourceEntityData::GetPropertyValueDefaultInt3(const char *propertyName, const int3 &defaultValue /*= int3::Zero*/) const
{
    return m_properties.GetPropertyValueDefaultInt3(propertyName, defaultValue);
}

int4 MapSourceEntityData::GetPropertyValueDefaultInt4(const char *propertyName, const int4 &defaultValue /*= int4::Zero*/) const
{
    return m_properties.GetPropertyValueDefaultInt4(propertyName, defaultValue);
}

uint2 MapSourceEntityData::GetPropertyValueDefaultUInt2(const char *propertyName, const uint2 &defaultValue /*= uint2::Zero*/) const
{
    return m_properties.GetPropertyValueDefaultUInt2(propertyName, defaultValue);
}

uint3 MapSourceEntityData::GetPropertyValueDefaultUInt3(const char *propertyName, const uint3 &defaultValue /*= uint3::Zero*/) const
{
    return m_properties.GetPropertyValueDefaultUInt3(propertyName, defaultValue);
}

uint4 MapSourceEntityData::GetPropertyValueDefaultUInt4(const char *propertyName, const uint4 &defaultValue /*= uint4::Zero*/) const
{
    return m_properties.GetPropertyValueDefaultUInt4(propertyName, defaultValue);
}

float2 MapSourceEntityData::GetPropertyValueDefaultFloat2(const char *propertyName, const float2 &defaultValue /*= float2::Zero*/) const
{
    return m_properties.GetPropertyValueDefaultFloat2(propertyName, defaultValue);
}

float3 MapSourceEntityData::GetPropertyValueDefaultFloat3(const char *propertyName, const float3 &defaultValue /*= float3::Zero*/) const
{
    return m_properties.GetPropertyValueDefaultFloat3(propertyName, defaultValue);
}

float4 MapSourceEntityData::GetPropertyValueDefaultFloat4(const char *propertyName, const float4 &defaultValue /*= float4::Zero*/) const
{
    return m_properties.GetPropertyValueDefaultFloat4(propertyName, defaultValue);
}

Quaternion MapSourceEntityData::GetPropertyValueDefaultQuaternion(const char *propertyName, const Quaternion &defaultValue /*= Quaternion::Identity*/) const
{
    return m_properties.GetPropertyValueDefaultQuaternion(propertyName, defaultValue);
}

void MapSourceEntityData::SetPropertyValueBool(const char *propertyName, bool propertyValue)
{
    m_properties.SetPropertyValueBool(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueInt8(const char *propertyName, int8 propertyValue)
{
    m_properties.SetPropertyValueInt8(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueInt16(const char *propertyName, int16 propertyValue)
{
    m_properties.SetPropertyValueInt16(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueInt32(const char *propertyName, int32 propertyValue)
{
    m_properties.SetPropertyValueInt32(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueInt64(const char *propertyName, int64 propertyValue)
{
    m_properties.SetPropertyValueInt64(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueUInt8(const char *propertyName, uint8 propertyValue)
{
    m_properties.SetPropertyValueUInt8(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueUInt16(const char *propertyName, uint16 propertyValue)
{
    m_properties.SetPropertyValueUInt16(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueUInt32(const char *propertyName, uint32 propertyValue)
{
    m_properties.SetPropertyValueUInt32(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueUInt64(const char *propertyName, uint64 propertyValue)
{
    m_properties.SetPropertyValueUInt64(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueFloat(const char *propertyName, float propertyValue)
{
    m_properties.SetPropertyValueFloat(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueDouble(const char *propertyName, double propertyValue)
{
    m_properties.SetPropertyValueDouble(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueColor(const char *propertyName, uint32 propertyValue)
{
    m_properties.SetPropertyValueColor(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueInt2(const char *propertyName, const int2 &propertyValue)
{
    m_properties.SetPropertyValueInt2(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueInt3(const char *propertyName, const int3 &propertyValue)
{
    m_properties.SetPropertyValueInt3(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueInt4(const char *propertyName, const int4 &propertyValue)
{
    m_properties.SetPropertyValueInt4(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueUInt2(const char *propertyName, const uint2 &propertyValue)
{
    m_properties.SetPropertyValueUInt2(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueUInt3(const char *propertyName, const uint3 &propertyValue)
{
    m_properties.SetPropertyValueUInt3(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueUInt4(const char *propertyName, const uint4 &propertyValue)
{
    m_properties.SetPropertyValueUInt4(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueFloat2(const char *propertyName, const float2 &propertyValue)
{
    m_properties.SetPropertyValueFloat2(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueFloat3(const char *propertyName, const float3 &propertyValue)
{
    m_properties.SetPropertyValueFloat3(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueFloat4(const char *propertyName, const float4 &propertyValue)
{
    m_properties.SetPropertyValueFloat4(propertyName, propertyValue);
    m_changed = true;
}

void MapSourceEntityData::SetPropertyValueQuaternion(const char *propertyName, const Quaternion &propertyValue)
{
    m_properties.SetPropertyValueQuaternion(propertyName, propertyValue);
    m_changed = true;
}
