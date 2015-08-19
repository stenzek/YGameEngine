#include "MapCompiler/MapSourceTerrainData.h"
#include "ResourceCompiler/TerrainLayerListGenerator.h"
#include "Core/Image.h"
#include "YBaseLib/ZipArchive.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
Log_SetChannel(MapSourceTerrainData);

MapSourceTerrainData::MapSourceTerrainData(MapSource *pMapSource)
    : m_pMapSource(pMapSource),
      m_pLayerListGenerator(nullptr),
      m_pLayerList(nullptr),
      m_layerListChanged(false),
      m_minSectionX(0),
      m_minSectionY(0),
      m_maxSectionX(0),
      m_maxSectionY(0),
      m_sectionCountX(0),
      m_sectionCountY(0),
      m_sectionCount(0),
      m_availableSectionsChanged(false),
      m_ppSections(nullptr),
      m_pEditCallbacks(nullptr)
{

}

MapSourceTerrainData::~MapSourceTerrainData()
{
    for (int32 i = 0; i < m_sectionCount; i++)
    {
        if (m_ppSections[i] != nullptr)
            m_ppSections[i]->Release();
    }
    delete[] m_ppSections;

    if (m_pLayerList != nullptr)
        m_pLayerList->Release();

    delete m_pLayerListGenerator;
}

void MapSourceTerrainData::MakeTerrainStorageFileName(String &str, int32 sectionX, int32 sectionY)
{
    str.Format("terrain_sections/%i_%i.dat", sectionX, sectionY);
}

int32 MapSourceTerrainData::GetSectionArrayIndex(int32 sectionX, int32 sectionY, int32 minSectionX, int32 minSectionY, int32 maxSectionX, int32 maxSectionY)
{
    return ((sectionY - minSectionY) * (maxSectionX - minSectionX)) + (sectionX - minSectionX);
}

int32 MapSourceTerrainData::GetSectionArrayIndex(int32 sectionX, int32 sectionY) const
{
    DebugAssert(sectionX >= m_minSectionX && sectionX <= m_maxSectionX && sectionY >= m_minSectionY && sectionY <= m_maxSectionY);
    return ((sectionY - m_minSectionY) * m_sectionCountX) + (sectionX - m_minSectionX);
}

AABox MapSourceTerrainData::CalculateSectionBoundingBox(int32 sectionX, int32 sectionY) const
{
    return TerrainUtilities::CalculateSectionBoundingBox(&m_parameters, sectionX, sectionY);
}

AABox MapSourceTerrainData::CalculateTerrainBoundingBox() const
{
    int32 scale = (int32)m_parameters.Scale;
    int32 sectionSize = (int32)m_parameters.SectionSize;

    int32 minX = m_minSectionX * sectionSize * scale;
    int32 minY = m_minSectionY * sectionSize * sectionSize;
    int32 maxX = (m_maxSectionX + 1) * sectionSize * scale;
    int32 maxY = (m_maxSectionY + 1) * sectionSize * scale;

    return AABox((float)minX, (float)minY, (float)m_parameters.MinHeight, (float)maxX, (float)maxY, (float)m_parameters.MaxHeight);
}

int2 MapSourceTerrainData::CalculateSectionForPoint(int32 globalX, int32 globalY) const
{
    return TerrainUtilities::CalculateSectionForPoint(&m_parameters, globalX, globalY);
}

int2 MapSourceTerrainData::CalculateSectionForPosition(const float3 &position) const
{
    return TerrainUtilities::CalculateSectionForPosition(&m_parameters, position);
}

void MapSourceTerrainData::CalculateSectionAndOffsetForPoint(int32 *pSectionX, int32 *pSectionY, uint32 *pIndexX, uint32 *pIndexY, int32 globalX, int32 globalY) const
{
    TerrainUtilities::CalculateSectionAndOffsetForPoint(&m_parameters, pSectionX, pSectionY, pIndexX, pIndexY, globalX, globalY);
}

void MapSourceTerrainData::CalculateSectionAndOffsetForPosition(int32 *pSectionX, int32 *pSectionY, uint32 *pIndexX, uint32 *pIndexY, const float3 &position) const
{
    TerrainUtilities::CalculateSectionAndOffsetForPosition(&m_parameters, pSectionX, pSectionY, pIndexX, pIndexY, position);
}

int2 MapSourceTerrainData::CalculatePointForPosition(const float3 &position) const
{
    return TerrainUtilities::CalculatePointForPosition(&m_parameters, position);
}

float3 MapSourceTerrainData::CalculatePositionForPoint(int32 globalX, int32 globalY) const
{
    return TerrainUtilities::CalculatePositionForPoint(&m_parameters, globalX, globalY);
}

void MapSourceTerrainData::CalculatePointForSectionAndOffset(int32 *pGlobalX, int32 *pGlobalY, int32 sectionX, int32 sectionY, uint32 offsetX, uint32 offsetY) const
{
    TerrainUtilities::CalculatePointForSectionAndOffset(&m_parameters, pGlobalX, pGlobalY, sectionX, sectionY, offsetX, offsetY);
}

uint8 MapSourceTerrainData::GetDefaultLayer() const
{
    return (uint8)m_pLayerList->GetFirstLayerIndex();
}

bool MapSourceTerrainData::IsSectionAvailable(int32 sectionX, int32 sectionY) const
{
    if (sectionX < m_minSectionX || sectionX > m_maxSectionX || sectionY < m_minSectionY || sectionY > m_maxSectionY)
        return false;

    int32 sectionIndex = GetSectionArrayIndex(sectionX, sectionY);
    DebugAssert(sectionIndex >= 0);

    return m_availableSectionMask.TestBit(sectionIndex);
}

bool MapSourceTerrainData::IsSectionLoaded(int32 sectionX, int32 sectionY) const
{
    if (sectionX < m_minSectionX || sectionX > m_maxSectionX || sectionY < m_minSectionY || sectionY > m_maxSectionY)
        return false;

    int32 sectionIndex = GetSectionArrayIndex(sectionX, sectionY);
    DebugAssert(sectionIndex >= 0);

    const TerrainSection *pSection = m_ppSections[sectionIndex];
    return (pSection != nullptr);
}

const TerrainSection *MapSourceTerrainData::GetSection(int32 sectionX, int32 sectionY) const
{
    if (sectionX < m_minSectionX || sectionX > m_maxSectionX || sectionY < m_minSectionY || sectionY > m_maxSectionY)
        return NULL;

    int32 sectionIndex = GetSectionArrayIndex(sectionX, sectionY);
    DebugAssert(sectionIndex >= 0);

    return m_ppSections[sectionIndex];
}

TerrainSection *MapSourceTerrainData::GetSection(int32 sectionX, int32 sectionY)
{
    if (sectionX < m_minSectionX || sectionX > m_maxSectionX || sectionY < m_minSectionY || sectionY > m_maxSectionY)
        return NULL;

    int32 sectionIndex = GetSectionArrayIndex(sectionX, sectionY);
    DebugAssert(sectionIndex >= 0);

    return m_ppSections[sectionIndex];
}

TerrainLayerList *MapSourceTerrainData::CompileLayerList(const TerrainLayerListGenerator *pGenerator, BinaryBlob **ppStoreBlob)
{
    ByteStream *pMemoryStream = ByteStream_CreateGrowableMemoryStream();
    if (!pGenerator->Compile(pMemoryStream))
        return nullptr;

    pMemoryStream->SeekAbsolute(0);
    TerrainLayerList *pLayerList = new TerrainLayerList();
    if (!pLayerList->Load("<in memory>", pMemoryStream))
    {
        pLayerList->Release();
        pMemoryStream->Release();
        return nullptr;
    }

    if (ppStoreBlob != nullptr)
    {
        pMemoryStream->SeekAbsolute(0);
        *ppStoreBlob = BinaryBlob::CreateFromStream(pMemoryStream);
    }

    pMemoryStream->Release();
    return pLayerList;
}

bool MapSourceTerrainData::Create(const TerrainParameters *pParameters, const TerrainLayerListGenerator *pLayerListGenerator, ProgressCallbacks *pProgressCallbacks)
{
    DebugAssert(TerrainUtilities::IsValidParameters(pParameters));

    // set parameters
    m_parameters = *pParameters;

    // compile the layer list
    {
        pProgressCallbacks->SetStatusText("Compiling layer list...");

        // make a copy
        TerrainLayerListGenerator *pOurLayerListGenerator = new TerrainLayerListGenerator();
        pOurLayerListGenerator->CreateCopy(pLayerListGenerator);

        // compile it
        TerrainLayerList *pLayerList;
        if ((pLayerList = CompileLayerList(pOurLayerListGenerator, nullptr)) == nullptr)
        {
            pProgressCallbacks->DisplayError("Failed to compile layer list.");
            delete pLayerListGenerator;
            return false;
        }

        // new so changed
        m_pLayerListGenerator = pOurLayerListGenerator;
        m_pLayerList = pLayerList;
        m_layerListChanged = true;
    }

    // init storage, set up sizes
    m_minSectionX = 0;
    m_minSectionY = 0;
    m_maxSectionX = 0;
    m_maxSectionY = 0;
    m_sectionCountX = m_maxSectionX - m_minSectionX + 1;
    m_sectionCountY = m_maxSectionY - m_minSectionY + 1;
    m_sectionCount = m_sectionCountX * m_sectionCountY;

    // allocate bitset
    m_availableSectionMask.Resize((uint32)m_sectionCount);
    m_availableSectionMask.Clear();

    // and the array
    m_ppSections = new TerrainSection *[m_sectionCount];
    Y_memzero(m_ppSections, sizeof(TerrainSection *)* m_sectionCount);
    return true;
}

bool MapSourceTerrainData::Load(ProgressCallbacks *pProgressCallbacks)
{
    // read the xml
    {
        AutoReleasePtr<ByteStream> pStream = m_pMapSource->GetMapArchive()->OpenFile("terrain.xml", BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
        if (pStream == NULL)
            return false;

        XMLReader xmlReader;
        if (!xmlReader.Create(pStream, "terrain.xml"))
            return false;

        if (!xmlReader.SkipToElement("terrain"))
        {
            xmlReader.PrintError("could not skip to terrain element");
            return false;
        }

        bool hasParameters = false;
        bool hasSections = false;
        for (;;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 terrainSelection = xmlReader.Select("parameters|sections");
                if (terrainSelection < 0)
                    return false;

                switch (terrainSelection)
                {
                    // parameters
                case 0:
                    {
                          const char *heightStorageFormatStr = xmlReader.FetchAttribute("height-storage-format");
                          const char *scaleStr = xmlReader.FetchAttribute("scale");
                          const char *sectionSizeStr = xmlReader.FetchAttribute("section-size");
                          const char *renderLODCountStr = xmlReader.FetchAttribute("render-lods");
                          const char *minHeightStr = xmlReader.FetchAttribute("min-height");
                          const char *maxHeightStr = xmlReader.FetchAttribute("max-height");
                          const char *baseHeightStr = xmlReader.FetchAttribute("base-height");

                          if (heightStorageFormatStr == NULL || scaleStr == NULL ||
                              sectionSizeStr == NULL || renderLODCountStr == NULL || 
                              minHeightStr == NULL || maxHeightStr == NULL || baseHeightStr == NULL)
                          {
                              xmlReader.PrintError("incomplete terrain parameters");
                              return false;
                          }

                          m_parameters.Scale = StringConverter::StringToUInt32(scaleStr);
                          m_parameters.SectionSize = StringConverter::StringToUInt32(sectionSizeStr);
                          m_parameters.LODCount = StringConverter::StringToUInt32(renderLODCountStr);
                          m_parameters.MinHeight = StringConverter::StringToInt32(minHeightStr);
                          m_parameters.MaxHeight = StringConverter::StringToInt32(maxHeightStr);
                          m_parameters.BaseHeight = StringConverter::StringToInt32(baseHeightStr);

                          if (!NameTable_TranslateType(NameTables::TerrainHeightStorageFormat, heightStorageFormatStr, &m_parameters.HeightStorageFormat))
                          {
                              xmlReader.PrintError("invalid height storage format");
                              return false;
                          }

                          if ((m_pMapSource->GetRegionSize() % (m_parameters.SectionSize * m_parameters.Scale)) != 0 ||
                              !TerrainUtilities::IsValidParameters(&m_parameters))
                          {
                              xmlReader.PrintError("invalid parameters");
                              return false;
                          }

                          hasParameters = true;
                    }
                    break;

                    // sections
                case 1:
                    {
                        const char *minXStr = xmlReader.FetchAttribute("min-x");
                        const char *minYStr = xmlReader.FetchAttribute("min-y");
                        const char *maxXStr = xmlReader.FetchAttribute("max-x");
                        const char *maxYStr = xmlReader.FetchAttribute("max-y");
                        if (minXStr == NULL || minYStr == NULL || maxXStr == NULL || maxYStr == NULL)
                        {
                            xmlReader.PrintError("incomplete sections definition");
                            return false;
                        }

                        m_minSectionX = StringConverter::StringToInt32(minXStr);
                        m_minSectionY = StringConverter::StringToInt32(minYStr);
                        m_maxSectionX = StringConverter::StringToInt32(maxXStr);
                        m_maxSectionY = StringConverter::StringToInt32(maxYStr);

                        // set up sizes
                        DebugAssert(m_minSectionX <= m_maxSectionX && m_minSectionY <= m_maxSectionY);
                        m_sectionCountX = m_maxSectionX - m_minSectionX + 1;
                        m_sectionCountY = m_maxSectionY - m_minSectionY + 1;
                        m_sectionCount = m_sectionCountX * m_sectionCountY;

                        // allocate bitset
                        m_availableSectionMask.Resize((uint32)m_sectionCount);
                        m_availableSectionMask.Clear();

                        // and the array
                        m_ppSections = new TerrainSection *[m_sectionCount];
                        Y_memzero(m_ppSections, sizeof(TerrainSection *)* m_sectionCount);

                        // fill the bitset
                        if (!xmlReader.IsEmptyElement())
                        {
                            for (;;)
                            {
                                if (!xmlReader.NextToken())
                                    return false;

                                if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                {
                                    int32 sectionsSelection = xmlReader.Select("section");
                                    if (sectionsSelection < 0)
                                        return false;

                                    const char *sectionXStr = xmlReader.FetchAttribute("x");
                                    const char *sectionYStr = xmlReader.FetchAttribute("y");
                                    if (sectionXStr == NULL || sectionYStr == NULL)
                                    {
                                        xmlReader.PrintError("missing x or y attributes");
                                        return false;
                                    }

                                    int32 sectionX = StringConverter::StringToInt32(sectionXStr);
                                    int32 sectionY = StringConverter::StringToInt32(sectionYStr);
                                    int32 arrayIndex = GetSectionArrayIndex(sectionX, sectionY);
                                    if (m_availableSectionMask.TestBit(arrayIndex))
                                    {
                                        xmlReader.PrintError("duplicate defined section (%u, %u)", sectionX, sectionY);
                                        return false;
                                    }

                                    m_availableSectionMask.SetBit(arrayIndex);
                                }
                                else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                {
                                    DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "sections") == 0);
                                    break;
                                }
                            }
                        }

                        hasSections = true;
                    }
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "terrain") == 0);
                break;
            }
        }

        if (!hasParameters || !hasSections)
        {
            xmlReader.PrintError("incomplete terrain xml definition");
            return false;
        }
    }

    // load the layer list
    {
        AutoReleasePtr<ByteStream> pStream = m_pMapSource->GetMapArchive()->OpenFile("terrain_layers.zip", BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE);
        if (pStream == NULL)
            return false;

        TerrainLayerListGenerator *pLayerListGenerator = new TerrainLayerListGenerator();
        if (!pLayerListGenerator->Load("terrain_layers.zip", pStream, pProgressCallbacks))
        {
            pProgressCallbacks->DisplayError("Failed to load terrain layer list.");
            delete pLayerListGenerator;
            return false;
        }

        pProgressCallbacks->SetStatusText("Compiling layer list...");

        m_pLayerListGenerator = pLayerListGenerator;
        if ((m_pLayerList = CompileLayerList(pLayerListGenerator, nullptr)) == nullptr)
        {
            pProgressCallbacks->DisplayError("Failed to compile terrain layer list.");
            return false;
        }
    }

    // all done
    return true;
}

bool MapSourceTerrainData::Save(ProgressCallbacks *pProgressCallbacks)
{
    PathString fileName;

    pProgressCallbacks->SetProgressRange(3);
    pProgressCallbacks->SetProgressValue(0);
    pProgressCallbacks->PushState();

    // for each loaded section, save any changed sections
    if (m_loadedSections.GetSize() > 0)
    {
        pProgressCallbacks->SetStatusText("Saving terrain sections...");
        pProgressCallbacks->SetProgressRange(m_loadedSections.GetSize());
        pProgressCallbacks->SetProgressValue(0);

        for (uint32 i = 0; i < m_loadedSections.GetSize(); i++)
        {
            const TerrainSection *pSection = m_loadedSections[i];
            if (pSection->IsChanged())
            {
                MakeTerrainStorageFileName(fileName, pSection->GetSectionX(), pSection->GetSectionY());

                ByteStream *pSectionStream = m_pMapSource->GetMapArchive()->OpenFile(fileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_STREAMED);
                if (pSectionStream == NULL)
                {
                    Log_WarningPrintf("MapSourceTerrainData::Save: Could not open '%s' for writing.", fileName.GetCharArray());
                    return false;
                }

                if (!pSection->SaveToStream(pSectionStream))
                {
                    Log_WarningPrintf("MapSourceTerrainData::Save: Failed to write '%s'.", fileName.GetCharArray());
                    return false;
                }

                pSectionStream->Release();
            }

            pProgressCallbacks->IncrementProgressValue();
        }
    }

    pProgressCallbacks->PopState();
    pProgressCallbacks->SetProgressValue(1);
    pProgressCallbacks->SetStatusText("Saving terrain header...");

    // write header xml
    {
        ByteStream *pHeaderStream = m_pMapSource->GetMapArchive()->OpenFile("terrain.xml", BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_STREAMED);
        if (pHeaderStream == NULL)
            return false;

        XMLWriter xmlWriter;
        if (!xmlWriter.Create(pHeaderStream))
        {
            pHeaderStream->Release();
            return false;
        }

        xmlWriter.StartElement("terrain");
        {
            xmlWriter.StartElement("parameters");
            {
                xmlWriter.WriteAttribute("height-storage-format", NameTable_GetNameString(NameTables::TerrainHeightStorageFormat, m_parameters.HeightStorageFormat));
                xmlWriter.WriteAttributef("scale", "%u", m_parameters.Scale);
                xmlWriter.WriteAttributef("section-size", "%u", m_parameters.SectionSize);
                xmlWriter.WriteAttributef("render-lods", "%u", m_parameters.LODCount);
                xmlWriter.WriteAttributef("min-height", "%i", m_parameters.MinHeight);
                xmlWriter.WriteAttributef("max-height", "%i", m_parameters.MaxHeight);
                xmlWriter.WriteAttributef("base-height", "%i", m_parameters.BaseHeight);
            }
            xmlWriter.EndElement();

            xmlWriter.StartElement("sections");
            {
                xmlWriter.WriteAttributef("min-x", "%i", m_minSectionX);
                xmlWriter.WriteAttributef("min-y", "%i", m_minSectionY);
                xmlWriter.WriteAttributef("max-x", "%i", m_maxSectionX);
                xmlWriter.WriteAttributef("max-y", "%i", m_maxSectionY);

                EnumerateAvailableSections([&xmlWriter](int32 sectionX, int32 sectionY)
                {
                    xmlWriter.StartElement("section");
                    xmlWriter.WriteAttributef("x", "%i", sectionX);
                    xmlWriter.WriteAttributef("y", "%i", sectionY);
                    xmlWriter.EndElement();
                });
            }
            xmlWriter.EndElement();
        }
        xmlWriter.EndElement();

        if (xmlWriter.InErrorState() || pHeaderStream->InErrorState())
        {
            xmlWriter.Close();
            pHeaderStream->Release();
            return false;
        }

        xmlWriter.Close();
        pHeaderStream->Release();
    }

    pProgressCallbacks->SetProgressValue(2);
    pProgressCallbacks->SetStatusText("Saving terrain layers...");

    // write layers to the archive
    {
        ByteStream *pLayersStream = m_pMapSource->GetMapArchive()->OpenFile("terrain_layers.zip", BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_SEEKABLE);
        if (pLayersStream == NULL)
            return false;

        if (!m_pLayerListGenerator->Save(pLayersStream, pProgressCallbacks))
        {
            pProgressCallbacks->DisplayError("Failed to save terrain layers.");
            return false;
        }

        pLayersStream->Release();
    }

    pProgressCallbacks->SetProgressValue(3);

    // delete any deleted sections from the archive
    for (uint32 i = 0; i < m_deletedSections.GetSize(); i++)
    {
        MakeTerrainStorageFileName(fileName, m_deletedSections[i].x, m_deletedSections[i].y);
        if (!m_pMapSource->GetMapArchive()->DeleteFile(fileName))
            Log_WarningPrintf("MapSourceTerrainStreamingCallbacks::UpdateArchive: Could not remove file '%s' for deleted section.", fileName.GetCharArray());
    }
    m_deletedSections.Clear();

    // clear changed flag on any loaded sections
    for (uint32 i = 0; i < m_loadedSections.GetSize(); i++)
    {
        if (m_loadedSections[i]->IsChanged())
            m_loadedSections[i]->ClearChangedFlag();
    }

    // clear saved flags
    m_layerListChanged = false;
    m_availableSectionsChanged = false;
    return true;
}

void MapSourceTerrainData::Delete()
{
    PathString fileName;

    // unload all sections ignoring any changes
    UnloadAllSections(true);
    DebugAssert(m_loadedSections.GetSize() == 0);

    // delete any deleted sections from the archive
    for (uint32 i = 0; i < m_deletedSections.GetSize(); i++)
    {
        MakeTerrainStorageFileName(fileName, m_deletedSections[i].x, m_deletedSections[i].y);
        if (!m_pMapSource->GetMapArchive()->DeleteFile(fileName))
            Log_WarningPrintf("MapSourceTerrainData::Delete: Could not remove file '%s' for deleted section.", fileName.GetCharArray());
    }
    m_deletedSections.Clear();

    // delete any available sections from the archive
    EnumerateAvailableSections([this, &fileName](int32 sectionX, int32 sectionY)
    {
        MakeTerrainStorageFileName(fileName, sectionX, sectionY);

        // this call may fail, if it was a cached section that was never saved.
        m_pMapSource->GetMapArchive()->DeleteFile(fileName);
    });

    // clear the available section mask
    m_availableSectionMask.Clear();

    // nuke the header file
    if (!m_pMapSource->GetMapArchive()->DeleteFile("terrain.xml"))
        Log_WarningPrintf("MapSourceTerrainData::Delete: Could not remove header file.");

    // nuke the header file
    if (!m_pMapSource->GetMapArchive()->DeleteFile("terrain_layers.zip"))
        Log_WarningPrintf("MapSourceTerrainData::Delete: Could not remove layers file.");
}

bool MapSourceTerrainData::IsChanged() const
{
    if (m_layerListChanged)
        return true;

    if (m_availableSectionsChanged)
        return true;

    for (uint32 i = 0; i < m_loadedSections.GetSize(); i++)
    {
        if (m_loadedSections[i]->IsChanged())
            return true;
    }

    return false;
}

bool MapSourceTerrainData::LoadAllSections(ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    pProgressCallbacks->SetStatusText("Loading terrain sections...");
    pProgressCallbacks->SetProgressRange(m_sectionCount);
    pProgressCallbacks->SetProgressValue(0);
    pProgressCallbacks->SetCancellable(false);

    for (int32 sectionX = m_minSectionX; sectionX <= m_maxSectionX; sectionX++)
    {
        for (int32 sectionY = m_minSectionY; sectionY <= m_maxSectionY; sectionY++)
        {
            int32 arrayIndex = GetSectionArrayIndex(sectionX, sectionY);
            if (m_availableSectionMask.TestBit(arrayIndex) && m_ppSections[arrayIndex] == nullptr)
            {
                if (!LoadSection(sectionX, sectionY))
                    return false;
            }

            pProgressCallbacks->IncrementProgressValue();
        }
    }

    return true;
}

void MapSourceTerrainData::UnloadAllSections(bool discardChanges /* = false */)
{
    while (m_loadedSections.GetSize() > 0)
    {
        if (discardChanges || !m_loadedSections[0]->IsChanged())
            UnloadSection(m_loadedSections[0]->GetSectionX(), m_loadedSections[0]->GetSectionY());
    }
}

void MapSourceTerrainData::ResizeSectionArray(int32 newMinSectionX, int32 newMinSectionY, int32 newMaxSectionX, int32 newMaxSectionY)
{
    int32 newSectionCountX = (newMaxSectionX - newMinSectionX) + 1;
    int32 newSectionCountY = (newMaxSectionY - newMinSectionY) + 1;
    int32 newSectionCount = newSectionCountX * newSectionCountY;
    DebugAssert(newMinSectionX <= m_minSectionX && newMinSectionY <= m_minSectionY);
    DebugAssert(newMaxSectionX >= m_maxSectionX && newMaxSectionY >= m_maxSectionY);
    DebugAssert(newSectionCountX > 0 && newSectionCountY > 0);

    // build new bitset and array
    BitSet32 newAvailableSectionMask(newSectionCount);
    newAvailableSectionMask.Clear();
    TerrainSection **ppNewSectionArray = new TerrainSection *[newSectionCount];
    Y_memzero(ppNewSectionArray, sizeof(TerrainSection *) * newSectionCount);

    // fill new bitset/array
    for (int32 sectionX = m_minSectionX; sectionX <= m_maxSectionX; sectionX++)
    {
        for (int32 sectionY = m_minSectionY; sectionY <= m_maxSectionY; sectionY++)
        {
            int32 oldIndex = ((sectionY - m_minSectionY) * m_sectionCountX) + (sectionX - m_minSectionX);
            DebugAssert(oldIndex >= 0 && oldIndex < m_sectionCount);
            if (m_availableSectionMask.TestBit((uint32)oldIndex))
            {
                int32 newIndex = ((sectionY - newMinSectionY) * newSectionCountX) + (sectionX - newMinSectionX);
                DebugAssert(newIndex >= 0 && newIndex < newSectionCount);

                newAvailableSectionMask.SetBit((uint32)newIndex);
                ppNewSectionArray[newIndex] = m_ppSections[oldIndex];
            }
        }
    }

    // swap everything over
    m_minSectionX = newMinSectionX;
    m_minSectionY = newMinSectionY;
    m_maxSectionX = newMaxSectionX;
    m_maxSectionY = newMaxSectionY;
    m_sectionCountX = newSectionCountX;
    m_sectionCountY = newSectionCountY;
    m_sectionCount = newSectionCount;
    delete[] m_ppSections;
    m_ppSections = ppNewSectionArray;
    m_availableSectionMask.Swap(newAvailableSectionMask);
}

uint32 MapSourceTerrainData::CreateSections(const int2 *pNewSectionIndices, uint32 newSectionCount, float createHeight, uint8 createLayer, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    uint32 sectionSize = m_parameters.SectionSize;
    uint32 sectionsCreated = 0;

    // get the min/max new section coordinates
    int32 minNewSectionX = pNewSectionIndices[0].x;
    int32 minNewSectionY = pNewSectionIndices[0].y;
    int32 maxNewSectionX = minNewSectionX;
    int32 maxNewSectionY = minNewSectionY;
    for (uint32 i = 1; i < newSectionCount; i++)
    {
        int32 sx = pNewSectionIndices[i].x;
        int32 sy = pNewSectionIndices[i].y;
        minNewSectionX = Min(minNewSectionX, sx);
        minNewSectionY = Min(minNewSectionY, sy);
        maxNewSectionX = Max(maxNewSectionX, sx);
        maxNewSectionY = Max(maxNewSectionY, sy);
    }

    // resize terrain?
    if (minNewSectionX < m_minSectionX || maxNewSectionX > m_maxSectionX ||
        minNewSectionY < m_minSectionY || maxNewSectionY > m_maxSectionY)
    {
        ResizeSectionArray(Min(minNewSectionX, m_minSectionX), Min(minNewSectionY, m_minSectionY), Max(maxNewSectionX, m_maxSectionX), Max(maxNewSectionY, m_maxSectionY));
    }
    
    // create the sections
    for (uint32 i = 0; i < newSectionCount; i++)
    {
        int32 newSectionX = pNewSectionIndices[i].x;
        int32 newSectionY = pNewSectionIndices[i].y;
        if (IsSectionAvailable(newSectionX, newSectionY))
        {
            Log_ErrorPrintf("Refusing to create section [%u, %u] (relative [%i, %i]), already exists", newSectionX, newSectionY, pNewSectionIndices[i].x, pNewSectionIndices[i].y);
            continue;
        }

        if (!EnsureAdjacentSectionsLoaded(newSectionX, newSectionY))
        {
            Log_ErrorPrintf("Refusing to create section [%u, %u], (relative [%i, %i]), could not load adjacent sections", newSectionX, newSectionY, pNewSectionIndices[i].x, pNewSectionIndices[i].y);
            continue;
        }

        // create the section
        TerrainSection *pSection = new TerrainSection(&m_parameters, newSectionX, newSectionY, 0);
        pSection->Create(createHeight, createLayer);
        /*
                         GetSection(newSectionX, newSectionY - 1),           // south
                         GetSection(newSectionX + 1, newSectionY - 1),       // south-east
                         GetSection(newSectionX + 1, newSectionY));          // east
                         */

        // store in lod 0
        int32 arrayIndex = GetSectionArrayIndex(newSectionX, newSectionY);
        DebugAssert(m_ppSections[arrayIndex] == NULL && !m_availableSectionMask.TestBit(arrayIndex));
        m_availableSectionMask.SetBit(arrayIndex);
        m_availableSectionsChanged = true;
        m_ppSections[arrayIndex] = pSection;
        m_loadedSections.Add(pSection);
        sectionsCreated++;

        // notify callbacks
        if (m_pEditCallbacks != nullptr)
        {
            m_pEditCallbacks->OnSectionCreated(newSectionX, newSectionY);
            m_pEditCallbacks->OnSectionLoaded(pSection);
        }

        // remove from deleted section list
        int32 deletedSectionIndex = m_deletedSections.IndexOf(int2(newSectionX, newSectionY));
        if (deletedSectionIndex >= 0)
            m_deletedSections.OrderedRemove(deletedSectionIndex);

        // update adjacent sections

        // west pixels
        for (uint32 y = 0; y < sectionSize; y++)
        {
            HandleEdgePointsHeightUpdate(pSection, 0, y);
            HandleEdgePointsWeightUpdate(pSection, 0, y);
        }

        // south pixels
        for (uint32 x = 0; x < sectionSize; x++)
        {
            HandleEdgePointsHeightUpdate(pSection, x, 0);
            HandleEdgePointsWeightUpdate(pSection, x, 0);
        }

        // south-west pixels
        HandleEdgePointsHeightUpdate(pSection, 0, 0);
        HandleEdgePointsWeightUpdate(pSection, 0, 0);
    }

    return sectionsCreated;
}

void MapSourceTerrainData::DeleteSections(const int2 *pSectionIndices, uint32 sectionCount, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    uint32 sectionSize = m_parameters.SectionSize;
    int32 baseHeight = m_parameters.BaseHeight;

    for (uint32 i = 0; i < sectionCount; i++)
    {
        int32 sectionX = pSectionIndices[i].x;
        int32 sectionY = pSectionIndices[i].y;

        if (!IsSectionAvailable(sectionX, sectionY))
        {
            Log_ErrorPrintf("Refusing to delete section [%u, %u] does not exist", sectionX, sectionY);
            continue;
        }

        // remove heights from adjacent sections
        if (!EnsureAdjacentSectionsLoaded(sectionX, sectionY))
        {
            Log_ErrorPrintf("Refusing to delete section [%u, %u], could not load adjacent sections", sectionX, sectionY);
            continue;
        }

        // update adjacent sections with base heights
        TerrainSection *pAdjacentSection;

        // section to the west
        pAdjacentSection = GetSection(sectionX - 1, sectionY);
        if (pAdjacentSection != NULL)
        {
            for (uint32 y = 0; y < sectionSize; y++)
            {
                pAdjacentSection->SetHeightMapValue(sectionSize, y, (float)baseHeight);
                pAdjacentSection->ClearSplatMapValues(sectionSize, y);

                if (m_pEditCallbacks != nullptr)
                {
                    m_pEditCallbacks->OnSectionPointHeightModified(pAdjacentSection, sectionSize, y);
                    m_pEditCallbacks->OnSectionPointLayersModified(pAdjacentSection, sectionSize, y);
                }
            }
        }

        // section to the north
        pAdjacentSection = GetSection(sectionX, sectionY + 1);
        if (pAdjacentSection != NULL)
        {
            for (uint32 x = 0; x < sectionSize; x++)
            {
                pAdjacentSection->SetHeightMapValue(x, sectionSize, (float)baseHeight);
                pAdjacentSection->ClearSplatMapValues(x, sectionSize);

                if (m_pEditCallbacks != nullptr)
                {
                    m_pEditCallbacks->OnSectionPointHeightModified(pAdjacentSection, x, sectionSize);
                    m_pEditCallbacks->OnSectionPointLayersModified(pAdjacentSection, x, sectionSize);
                }
            }
        }

        // section to the north-west
        pAdjacentSection = GetSection(sectionX - 1, sectionY + 1);
        if (pAdjacentSection != NULL)
        {
            pAdjacentSection->SetHeightMapValue(sectionSize, sectionSize, (float)baseHeight);
            pAdjacentSection->ClearSplatMapValues(sectionSize, sectionSize);

            if (m_pEditCallbacks != nullptr)
            {
                m_pEditCallbacks->OnSectionPointHeightModified(pAdjacentSection, sectionSize, sectionSize);
                m_pEditCallbacks->OnSectionPointLayersModified(pAdjacentSection, sectionSize, sectionSize);
            }
        }

        // delete the section
        uint32 arrayIndex = GetSectionArrayIndex(sectionX, sectionY);
        TerrainSection *pSection = m_ppSections[arrayIndex];

        // unload section
        if (pSection != nullptr)
        {
            if (m_pEditCallbacks != nullptr)
                m_pEditCallbacks->OnSectionUnloaded(pSection);

            pSection->Release();
            m_ppSections[arrayIndex] = nullptr;
        }

        // execute callbacks
        if (m_pEditCallbacks != nullptr)
            m_pEditCallbacks->OnSectionDeleted(sectionX, sectionY);

        // set unavailable
        m_availableSectionMask.UnsetBit(arrayIndex);

        // store in deleted section list
        if (m_deletedSections.IndexOf(int2(sectionX, sectionY)) < 0)
            m_deletedSections.Add(int2(sectionX, sectionY));
    }
}

void MapSourceTerrainData::DeleteAllSections()
{
    for (int32 sectionX = m_minSectionX; sectionX <= m_maxSectionX; sectionX++)
    {
        for (int32 sectionY = m_minSectionY; sectionY <= m_maxSectionY; sectionY++)
        {
            int32 arrayIndex = GetSectionArrayIndex(sectionX, sectionY);
            if (!m_availableSectionMask[arrayIndex])
                continue;

            TerrainSection *pSection = m_ppSections[arrayIndex];

            // unload section
            if (pSection != nullptr)
            {
                if (m_pEditCallbacks != nullptr)
                    m_pEditCallbacks->OnSectionUnloaded(pSection);

                pSection->Release();
                m_ppSections[arrayIndex] = nullptr;
            }

            // execute callbacks
            if (m_pEditCallbacks != nullptr)
                m_pEditCallbacks->OnSectionDeleted(sectionX, sectionY);
        }
    }

    // set up sizes
    m_minSectionX = 0;
    m_minSectionY = 0;
    m_maxSectionX = 0;
    m_maxSectionY = 0;
    m_sectionCountX = m_maxSectionX - m_minSectionX + 1;
    m_sectionCountY = m_maxSectionY - m_minSectionY + 1;
    m_sectionCount = m_sectionCountX * m_sectionCountY;

    // allocate bitset
    m_availableSectionMask.Resize((uint32)m_sectionCount);
    m_availableSectionMask.Clear();

    // and the array
    m_ppSections = new TerrainSection *[m_sectionCount];
    Y_memzero(m_ppSections, sizeof(TerrainSection *)* m_sectionCount);
}

bool MapSourceTerrainData::LoadSection(int32 sectionX, int32 sectionY)
{
    int32 arrayIndex = GetSectionArrayIndex(sectionX, sectionY);
    Assert(m_availableSectionMask.TestBit(arrayIndex));

    TerrainSection *pSection = m_ppSections[arrayIndex];
    if (pSection != nullptr)
        return true;

    // hit the archive for it
    PathString fileName;
    MakeTerrainStorageFileName(fileName, sectionX, sectionY);

    // open the file
    AutoReleasePtr<ByteStream> pStream = m_pMapSource->GetMapArchive()->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
    {
        Log_WarningPrintf("MapSourceTerrainData::LoadSection: Could not open '%s' in archive.", fileName.GetCharArray());
        return false;
    }

    // create section
    pSection = new TerrainSection(&m_parameters, sectionX, sectionY, 0);
    if (!pSection->LoadFromStream(pStream))
    {
        Log_WarningPrintf("MapSourceTerrainData::LoadSection: Terrain load for section (%i, %i) failed.", sectionX, sectionY);
        return false;
    }

    // store section
    m_ppSections[arrayIndex] = pSection;
    m_loadedSections.Add(pSection);

    // execute callbacks
    if (m_pEditCallbacks != nullptr)
        m_pEditCallbacks->OnSectionLoaded(pSection);

    return true;
}

void MapSourceTerrainData::UnloadSection(int32 sectionX, int32 sectionY)
{
    int32 arrayIndex = GetSectionArrayIndex(sectionX, sectionY);
    DebugAssert(arrayIndex >= 0);

    // ok, unload it
    TerrainSection *pSection = m_ppSections[arrayIndex];
    int32 loadedSectionIndex = m_loadedSections.IndexOf(pSection);
    Assert(pSection != nullptr && loadedSectionIndex >= 0);

    // warn if changed
    if (pSection->IsChanged())
        Log_WarningPrintf("MapSourceTerrainData::UnloadSection: Unloading section %i that has been modified, changes to this section are now lost", sectionX, sectionY);

    // invoke callbacks
    if (m_pEditCallbacks != nullptr)
        m_pEditCallbacks->OnSectionUnloaded(pSection);

    // kill the section
    pSection->Release();
    m_loadedSections.FastRemove(loadedSectionIndex);
    m_ppSections[arrayIndex] = nullptr;
}

bool MapSourceTerrainData::CreateSection(int32 sectionX, int32 sectionY, float createHeight, uint8 createLayer, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    int2 sectionLocation(sectionX, sectionY);
    return (CreateSections(&sectionLocation, 1, createHeight, createLayer, pProgressCallbacks) == 1);
}

void MapSourceTerrainData::DeleteSection(int32 sectionX, int32 sectionY, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    int2 sectionLocation(sectionX, sectionY);
    DeleteSections(&sectionLocation, 1, pProgressCallbacks);
}

float MapSourceTerrainData::GetPointHeight(int32 pointX, int32 pointY)
{
    int32 sectionX, sectionY;
    uint32 offsetX, offsetY;
    CalculateSectionAndOffsetForPoint(&sectionX, &sectionY, &offsetX, &offsetY, pointX, pointY);

    // is a valid section?
    if (!IsSectionAvailable(sectionX, sectionY))
        return (float)m_parameters.BaseHeight;

    // ensure this section is loaded
    TerrainSection *pSection = GetSection(sectionX, sectionY);
    if (!IsSectionLoaded(sectionX, sectionY))
    {
        if (!LoadSection(sectionX, sectionY))
            return (float)m_parameters.BaseHeight;

        pSection = GetSection(sectionX, sectionY);
        DebugAssert(pSection != NULL);
    }

    return pSection->GetHeightMapValue(offsetX, offsetY);
}

float MapSourceTerrainData::GetPointLayerWeight(int32 pointX, int32 pointY, uint8 layer)
{
    int32 sectionX, sectionY;
    uint32 offsetX, offsetY;
    CalculateSectionAndOffsetForPoint(&sectionX, &sectionY, &offsetX, &offsetY, pointX, pointY);

    // is a valid section?
    if (!IsSectionAvailable(sectionX, sectionY))
        return false;

    // ensure this section is loaded
    TerrainSection *pSection = GetSection(sectionX, sectionY);
    if (!IsSectionLoaded(sectionX, sectionY))
    {
        if (!LoadSection(sectionX, sectionY))
            return false;

        pSection = GetSection(sectionX, sectionY);
        DebugAssert(pSection != NULL);
    }

    return pSection->GetSplatMapValue(offsetX, offsetY, layer);
}

bool MapSourceTerrainData::SetPointHeight(int32 pointX, int32 pointY, float height)
{
    int32 sectionX, sectionY;
    uint32 offsetX, offsetY;
    CalculateSectionAndOffsetForPoint(&sectionX, &sectionY, &offsetX, &offsetY, pointX, pointY);
    
    // is a valid section?
    if (!IsSectionAvailable(sectionX, sectionY))
        return false;

    // ensure this section is loaded
    TerrainSection *pSection = GetSection(sectionX, sectionY);
    if (!IsSectionLoaded(sectionX, sectionY))
    {
        if (!LoadSection(sectionX, sectionY))
            return false;

        pSection = GetSection(sectionX, sectionY);
        DebugAssert(pSection != NULL);
    }

    // clamp height
    height = Math::Clamp(height, (float)m_parameters.MinHeight, (float)m_parameters.MaxHeight);

    // needs to be changed?
    if (pSection->GetHeightMapValue(offsetX, offsetY) == height)
        return true;

    // handle boundary cases, we need to update the neighbour section as well
    if (offsetX == 0 || offsetY == 0)
    {
        if (!EnsureAdjacentSectionsLoaded(sectionX, sectionY))
            return false;
    }

    // set the height in the section
    pSection->SetHeightMapValue(offsetX, offsetY, height);

    // callbacks
    if (m_pEditCallbacks != nullptr)
        m_pEditCallbacks->OnSectionPointHeightModified(pSection, offsetX, offsetY);

    // update edge sections
    HandleEdgePointsHeightUpdate(pSection, offsetX, offsetY);
    
    // success
    return true;
}

bool MapSourceTerrainData::AddPointHeight(int32 pointX, int32 pointY, float mod)
{
    int32 sectionX, sectionY;
    uint32 offsetX, offsetY;
    CalculateSectionAndOffsetForPoint(&sectionX, &sectionY, &offsetX, &offsetY, pointX, pointY);

    // is a valid section?
    if (!IsSectionAvailable(sectionX, sectionY))
        return false;

    // ensure this section is loaded
    TerrainSection *pSection = GetSection(sectionX, sectionY);
    if (!IsSectionLoaded(sectionX, sectionY))
    {
        if (!LoadSection(sectionX, sectionY))
            return false;

        pSection = GetSection(sectionX, sectionY);
        DebugAssert(pSection != nullptr);
    }

    return SetPointHeight(pointX, pointY, pSection->GetHeightMapValue(offsetX, offsetY) + mod);
}

bool MapSourceTerrainData::SetPointLayerWeight(int32 pointX, int32 pointY, uint8 layer, float weight, bool renormalize /* = true */)
{
    int32 sectionX, sectionY;
    uint32 offsetX, offsetY;
    CalculateSectionAndOffsetForPoint(&sectionX, &sectionY, &offsetX, &offsetY, pointX, pointY);

    // is a valid section?
    if (!IsSectionAvailable(sectionX, sectionY))
        return false;

    // ensure this section is loaded
    TerrainSection *pSection = GetSection(sectionX, sectionY);
    if (!IsSectionLoaded(sectionX, sectionY))
    {
        if (!LoadSection(sectionX, sectionY))
            return false;

        pSection = GetSection(sectionX, sectionY);
        DebugAssert(pSection != nullptr);
    }

    // check it has to be changed
    if (pSection->GetSplatMapValue(offsetX, offsetY, layer) == weight)
        return true;

    // handle boundary cases, we need to update the neighbour section as well
    if (offsetX == 0 || offsetY == 0)
    {
        if (!EnsureAdjacentSectionsLoaded(sectionX, sectionY))
            return false;
    }

    // set the height in the section
    bool hadLayer = pSection->HasSplatMapForLayer(layer);
    pSection->SetSplatMapValue(offsetX, offsetY, layer, weight, renormalize);

    // callbacks
    if (m_pEditCallbacks != nullptr)
    {
        if (!hadLayer)
            m_pEditCallbacks->OnSectionLayersModified(pSection);

        m_pEditCallbacks->OnSectionPointLayersModified(pSection, offsetX, offsetY);
    }

    // update edge points
    HandleEdgePointsWeightUpdate(pSection, offsetX, offsetY);
    return true;
}

bool MapSourceTerrainData::AddPointLayerWeight(int32 pointX, int32 pointY, uint8 layer, float amount, bool renormalize /* = true */)
{
    int32 sectionX, sectionY;
    uint32 offsetX, offsetY;
    CalculateSectionAndOffsetForPoint(&sectionX, &sectionY, &offsetX, &offsetY, pointX, pointY);

    // is a valid section?
    if (!IsSectionAvailable(sectionX, sectionY))
        return false;

    // ensure this section is loaded
    TerrainSection *pSection = GetSection(sectionX, sectionY);
    if (!IsSectionLoaded(sectionX, sectionY))
    {
        if (!LoadSection(sectionX, sectionY))
            return false;

        pSection = GetSection(sectionX, sectionY);
        DebugAssert(pSection != nullptr);
    }

    return SetPointLayerWeight(pointX, pointY, layer, Math::Clamp(pSection->GetSplatMapValue(offsetX, offsetY, layer) + amount, 0.0f, 1.0f), renormalize);
}

bool MapSourceTerrainData::EnsureAdjacentSectionsLoaded(int32 sectionX, int32 sectionY)
{
    DebugAssert(sectionX >= m_minSectionX && sectionX <= m_maxSectionX && sectionY >= m_minSectionY && sectionY <= m_maxSectionY);

    // self
    int32 arrayIndex = GetSectionArrayIndex(sectionX, sectionY);
    if (m_availableSectionMask[arrayIndex] && m_ppSections[arrayIndex] == nullptr &&
        !LoadSection(sectionX, sectionY))
    {
        return false;
    }

    // top-left
    if (sectionX > m_minSectionX && sectionY < m_maxSectionY)
    {
        arrayIndex = GetSectionArrayIndex(sectionX - 1, sectionY + 1);
        if (m_availableSectionMask[arrayIndex] && m_ppSections[arrayIndex] == nullptr &&
            !LoadSection(sectionX - 1, sectionY + 1))
        {
            return false;
        }
    }

    // top-middle
    if (sectionY < m_maxSectionY)
    {
        arrayIndex = GetSectionArrayIndex(sectionX, sectionY + 1);
        if (m_availableSectionMask[arrayIndex] && m_ppSections[arrayIndex] == nullptr &&
            !LoadSection(sectionX, sectionY + 1))
        {
            return false;
        }
    }

    // top-right
    if (sectionX < m_maxSectionX && sectionY < m_maxSectionY)
    {
        arrayIndex = GetSectionArrayIndex(sectionX + 1, sectionY + 1);
        if (m_availableSectionMask[arrayIndex] && m_ppSections[arrayIndex] == nullptr &&
            !LoadSection(sectionX + 1, sectionY + 1))
        {
            return false;
        }
    }

    // middle-left
    if (sectionX > m_minSectionX)
    {
        arrayIndex = GetSectionArrayIndex(sectionX - 1, sectionY);
        if (m_availableSectionMask[arrayIndex] && m_ppSections[arrayIndex] == nullptr &&
            !LoadSection(sectionX - 1, sectionY))
        {
            return false;
        }
    }

    // middle-right
    if (sectionX < m_maxSectionX)
    {
        arrayIndex = GetSectionArrayIndex(sectionX + 1, sectionY);
        if (m_availableSectionMask[arrayIndex] && m_ppSections[arrayIndex] == nullptr &&
            !LoadSection(sectionX + 1, sectionY))
        {
            return false;
        }
    }

    // bottom-left
    if (sectionX > m_minSectionX && sectionY > m_minSectionY)
    {
        arrayIndex = GetSectionArrayIndex(sectionX - 1, sectionY - 1);
        if (m_availableSectionMask[arrayIndex] && m_ppSections[arrayIndex] == nullptr &&
            !LoadSection(sectionX - 1, sectionY - 1))
        {
            return false;
        }
    }

    // bottom-middle
    if (sectionY > m_minSectionY)
    {
        arrayIndex = GetSectionArrayIndex(sectionX, sectionY - 1);
        if (m_availableSectionMask[arrayIndex] && m_ppSections[arrayIndex] == nullptr &&
            !LoadSection(sectionX, sectionY - 1))
        {
            return false;
        }
    }

    // bottom-right
    if (sectionX < m_maxSectionX && sectionY > m_minSectionY)
    {
        arrayIndex = GetSectionArrayIndex(sectionX + 1, sectionY - 1);
        if (m_availableSectionMask[arrayIndex] && m_ppSections[arrayIndex] == nullptr &&
            !LoadSection(sectionX + 1, sectionY - 1))
        {
            return false;
        }
    }

    return true;
}

bool MapSourceTerrainData::RayCast(const Ray &ray, float3 &contactNormal, float3 &contactPoint)
{
    // best contacts
    float bestContactTimeSq = Y_FLT_INFINITE;
    float3 bestContactPoint;
    float3 bestContactNormal;

#if 1
    // get ray bounding box
    AABox rayBoundingBox(ray.GetAABox());

    // get the min/max regions
    int2 searchSectionMin(CalculateSectionForPosition(rayBoundingBox.GetMinBounds()));
    int2 searchSectionMax(CalculateSectionForPosition(rayBoundingBox.GetMaxBounds()));

    // clamp to terrain bounds
    int32 startSectionX = Math::Clamp(searchSectionMin.x, m_minSectionX, m_maxSectionX);
    int32 startSectionY = Math::Clamp(searchSectionMin.y, m_minSectionY, m_maxSectionY);
    int32 endSectionX = Math::Clamp(searchSectionMax.x, m_minSectionX, m_maxSectionX);
    int32 endSectionY = Math::Clamp(searchSectionMax.y, m_minSectionY, m_maxSectionY);

    // iterate over these sections
    for (int32 sectionY = startSectionY; sectionY <= endSectionY; sectionY++)
    {
        for (int32 sectionX = startSectionX; sectionX <= endSectionX; sectionX++)
        {
            const TerrainSection *pSection = GetSection(sectionX, sectionY);
            if (pSection == NULL)
                continue;

            // use quadtree to break down search
            pSection->GetQuadTree()->EnumerateNodesIntersectingRay(ray, 0, [this, ray, pSection, &bestContactTimeSq, &bestContactPoint, &bestContactNormal](const TerrainQuadTreeNode *pNode)
            {
                // vars
                const float3 &sectionMinBounds = pSection->GetBoundingBox().GetMinBounds();
                uint32 scale = m_parameters.Scale;

                // get range
                uint32 startX = pNode->GetStartQuadX();
                uint32 startY = pNode->GetStartQuadY();
                uint32 endX = startX + pNode->GetNodeSize();
                uint32 endY = startY + pNode->GetNodeSize();

                float3 v0, v1, v2, v3;
                float3 triangleContactPoint, triangleContactNormal;
                float triangleContactTimeSq;

                // iterate over points
                for (uint32 ly = startY; ly < endY; ly++)
                {
                    for (uint32 lx = startX; lx < endX; lx++)
                    {
                        // first triangle
                        v0.Set(sectionMinBounds.x + (float)((lx)* scale),
                               sectionMinBounds.y + (float)((ly + 1) * scale),
                               pSection->GetHeightMapValue(lx, ly + 1));

                        v1.Set(sectionMinBounds.x + (float)((lx)* scale),
                               sectionMinBounds.y + (float)((ly)* scale),
                               pSection->GetHeightMapValue(lx, ly));

                        v2.Set(sectionMinBounds.x + (float)((lx + 1) * scale),
                               sectionMinBounds.y + (float)((ly + 1) * scale),
                               pSection->GetHeightMapValue(lx + 1, ly + 1));

                        // test first triangle
                        if (ray.TriangleIntersection(v0, v1, v2, triangleContactNormal, triangleContactPoint))
                        {
                            // success
                            triangleContactTimeSq = (triangleContactPoint - ray.GetOrigin()).SquaredLength();
                            if (triangleContactTimeSq < bestContactTimeSq)
                            {
                                bestContactTimeSq = triangleContactTimeSq;
                                bestContactNormal = triangleContactNormal;
                                bestContactPoint = triangleContactPoint;
                            }

                            // don't bother testing the other triangle, it's unlikely to hit both?
                            continue;
                        }

                        // fill last vertex
                        v3.Set(sectionMinBounds.x + (float)((lx + 1) * scale),
                               sectionMinBounds.y + (float)((ly)* scale),
                               pSection->GetHeightMapValue(lx + 1, ly));

                        // test second triangle: note reversed first vertices
                        if (ray.TriangleIntersection(v2, v1, v3, triangleContactNormal, triangleContactPoint))
                        {
                            // success
                            triangleContactTimeSq = (triangleContactPoint - ray.GetOrigin()).SquaredLength();
                            if (triangleContactTimeSq < bestContactTimeSq)
                            {
                                bestContactTimeSq = triangleContactTimeSq;
                                bestContactNormal = triangleContactNormal;
                                bestContactPoint = triangleContactPoint;
                            }
                        }
                    }
                }
            });
        }
    }

#elif 1
    // get the bounding box of the ray
    AABox rayBoundingBox(ray.GetAABox());

    // find the sections this overlaps
    EnumerateSectionsOverlappingBox(rayBoundingBox, [this, ray, &rayBoundingBox, &bestContactTimeSq, &bestContactPoint, &bestContactNormal](const TerrainSection *pSection)
    {
        uint32 unitsPerPoint = m_scale;
        float fUnitsPerPoint = (float)unitsPerPoint;
        int32 sectionSizeMinusOne = (int32)m_sectionSize - 1;

        // ensure the ray actually hits this box (since the box is very coarse)
        if (!ray.AABoxIntersection(pSection->GetBoundingBox()))
            return;

        // possible optimization here: use quadtree-style tests to eliminate triangles early
        // get region min bounds
        float3 sectionMinBounds(pSection->GetBoundingBox().GetMinBounds());

        // find the overlapping points
        float3 regionMinPoints((rayBoundingBox.GetMinBounds() - sectionMinBounds) / fUnitsPerPoint);
        float3 regionMaxPoints((rayBoundingBox.GetMaxBounds() - sectionMinBounds) / fUnitsPerPoint);

        // quantize them 
        int32 startXi = Math::Truncate(Math::Floor(regionMinPoints.x));
        int32 startYi = Math::Truncate(Math::Floor(regionMinPoints.y));
        int32 endXi = Math::Truncate(Math::Ceil(regionMaxPoints.x));
        int32 endYi = Math::Truncate(Math::Ceil(regionMaxPoints.y));

        // fix up range
        uint32 startX = (uint32)Min(Max(startXi, (int32)0), sectionSizeMinusOne);
        uint32 startY = (uint32)Min(Max(startYi, (int32)0), sectionSizeMinusOne);
        uint32 endX = (uint32)Min(Max(endXi, (int32)0), sectionSizeMinusOne);
        uint32 endY = (uint32)Min(Max(endYi, (int32)0), sectionSizeMinusOne);

        float3 v0, v1, v2, v3;
        float3 triangleContactPoint, triangleContactNormal;
        float triangleContactTimeSq;

        // iterate over points
        for (uint32 ly = startY; ly <= endY; ly++)
        {
            for (uint32 lx = startX; lx <= endX; lx++)
            {
                // first triangle
                v0.Set(sectionMinBounds.x + (float)((lx)* unitsPerPoint),
                       sectionMinBounds.y + (float)((ly + 1) * unitsPerPoint),
                       pSection->GetIndexedHeight(lx, ly + 1));

                v1.Set(sectionMinBounds.x + (float)((lx)* unitsPerPoint),
                       sectionMinBounds.y + (float)((ly)* unitsPerPoint),
                       pSection->GetIndexedHeight(lx, ly));

                v2.Set(sectionMinBounds.x + (float)((lx + 1) * unitsPerPoint),
                       sectionMinBounds.y + (float)((ly + 1) * unitsPerPoint),
                       pSection->GetIndexedHeight(lx + 1, ly + 1));

                // test first triangle
                if (ray.TriangleIntersection(v0, v1, v2, triangleContactNormal, triangleContactPoint))
                {
                    // success
                    triangleContactTimeSq = (triangleContactPoint - ray.GetOrigin()).SquaredLength();
                    if (triangleContactTimeSq < bestContactTimeSq)
                    {
                        bestContactTimeSq = triangleContactTimeSq;
                        bestContactNormal = triangleContactNormal;
                        bestContactPoint = triangleContactPoint;
                    }

                    // don't bother testing the other triangle, it's unlikely to hit both?
                    continue;
                }

                // fill last vertex
                v3.Set(sectionMinBounds.x + (float)((lx + 1) * unitsPerPoint),
                       sectionMinBounds.y + (float)((ly)* unitsPerPoint),
                       pSection->GetIndexedHeight(lx + 1, ly));

                // test second triangle: note reversed first vertices
                if (ray.TriangleIntersection(v2, v1, v3, triangleContactNormal, triangleContactPoint))
                {
                    // success
                    triangleContactTimeSq = (triangleContactPoint - ray.GetOrigin()).SquaredLength();
                    if (triangleContactTimeSq < bestContactTimeSq)
                    {
                        bestContactTimeSq = triangleContactTimeSq;
                        bestContactNormal = triangleContactNormal;
                        bestContactPoint = triangleContactPoint;
                    }
                }
            }
        }
    });

#endif

    if (bestContactTimeSq == Y_FLT_INFINITE)
        return false;

    contactNormal = bestContactNormal;
    contactPoint = bestContactPoint;
    return true;
}

bool MapSourceTerrainData::RebuildQuadTree(uint32 newLODCount, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    if (newLODCount != m_parameters.LODCount)
    {
        Log_ErrorPrintf("Cannot currently rebuild quadtree with a different lod count");
        return false;
    }

    return false;
}

bool MapSourceTerrainData::ImportHeightmap(const Image *pHeightmap, int32 startSectionX, int32 startSectionY, float minHeight, float maxHeight, HeightmapImportScaleType scaleType, uint32 scaleAmount, ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    int32 sectionSize = (int32)m_parameters.SectionSize;
    float heightRange = (maxHeight - minHeight);
    Log_DevPrintf("import heightmap %u x %u", pHeightmap->GetWidth(), pHeightmap->GetHeight());

    if (pHeightmap->GetPixelFormat() != PIXEL_FORMAT_R8_UNORM &&
        pHeightmap->GetPixelFormat() != PIXEL_FORMAT_R16_UNORM &&
        pHeightmap->GetPixelFormat() != PIXEL_FORMAT_R32_FLOAT)
    {
        return false;
    }

    // delete all visible sections, since there's going to be a lot of height changes, no point uploading them to the gpu one by one
    DeleteAllSections();

    // copy the heightmap and scale it if needed. flip the image vertically, as that's the direction it'll be inserted
    Image heightmapCopy;
    heightmapCopy.Copy(*pHeightmap);
    //heightmapCopy.FlipVertical();

    // work out how many sections we need
    int32 sectionsNeededX = Math::Truncate(Math::Ceil((float)heightmapCopy.GetWidth() / (float)sectionSize));
    int32 sectionsNeededY = Math::Truncate(Math::Ceil((float)heightmapCopy.GetHeight() / (float)sectionSize));

    // work out starting sections, we position the top-left corner of the heightmap in the top-left section
    int32 realStartSectionX = startSectionX;
    int32 realStartSectionY = startSectionY + (sectionsNeededY - 1);

    // from this, work out the starting global coordinates
    int32 globalCoordinateStartX, globalCoordinateStartY;
    CalculatePointForSectionAndOffset(&globalCoordinateStartX, &globalCoordinateStartY, realStartSectionX, realStartSectionY, 0, 0);

    // create them
    pProgressCallbacks->SetStatusText("Creating sections...");
    pProgressCallbacks->SetProgressRange(sectionsNeededX * sectionsNeededY);
    pProgressCallbacks->SetProgressValue(0);
    for (int32 sy = 0; sy < sectionsNeededY; sy++)
    {
        for (int32 sx = 0; sx < sectionsNeededX; sx++)
        {
            int32 realSectionX = realStartSectionX + sx;
            int32 realSectionY = realStartSectionY - sy;
            if (IsSectionAvailable(realSectionX, realSectionY))
            {
                // ensure it's loaded
                if (!LoadSection(realSectionX, realSectionY) && !IsSectionLoaded(realSectionX, realSectionY))
                    return false;
            }
            else
            {
                if (!CreateSection(realSectionX, realSectionY, (float)m_parameters.BaseHeight, GetDefaultLayer()))
                    return false;
            }

            pProgressCallbacks->IncrementProgressValue();
        }
    }

    // fill the values
    pProgressCallbacks->SetStatusText("Filling data...");
    pProgressCallbacks->SetProgressRange(pHeightmap->GetWidth() * pHeightmap->GetHeight());
    pProgressCallbacks->SetProgressValue(0);
    for (int32 pointY = 0; pointY < (int32)pHeightmap->GetHeight(); pointY++)
    {
        const byte *pHeightmapDataRow = pHeightmap->GetData() + ((uint32)pointY * pHeightmap->GetDataRowPitch());

        for (int32 pointX = 0; pointX < (int32)pHeightmap->GetWidth(); pointX++)
        {
            float height = 0.0f;

            switch (heightmapCopy.GetPixelFormat())
            {
            case PIXEL_FORMAT_R8_UNORM:
                {
                    uint8 heightmapPixel = *(pHeightmapDataRow++);
                    float valueFraction = (float)heightmapPixel / 255.0f;
                    height = minHeight + (valueFraction * heightRange);
                }
                break;

            case PIXEL_FORMAT_R16_UNORM:
                {
                    uint16 heightmapPixel = *(uint16 *)pHeightmapDataRow;
                    pHeightmapDataRow += sizeof(uint16);

                    float valueFraction = (float)heightmapPixel / 65535.0f;
                    height = minHeight + (valueFraction * heightRange);
                }
                break;

            case PIXEL_FORMAT_R32_FLOAT:
                {
                    float heightmapPixel = *(float *)pHeightmapDataRow;
                    pHeightmapDataRow += sizeof(float);
                    height = heightmapPixel;
                }
                break;
            }


//             int32 realPointX = (startSectionX * sectionSize) + pointX;
//             int32 realPointY = (startSectionY * sectionSize) + pointY;
//             m_pTerrainManager->SetPointHeight(realPointX, realPointY, height);
//             Log_DevPrintf("%i %i -> %f", realPointX, realPointY, height);

//             int32 writeSectionX = realStartSectionX + (pointX / sectionSize);
//             int32 writeSectionY = realStartSectionY + (pointY / sectionSize);
//             int32 writeOffsetX = pointX % sectionSize;
//             int32 writeOffsetY = pointY % sectionSize;
//             int32 globalX, globalY;
//             m_pTerrainManager->CalculatePointForSectionAndOffset(&globalX, &globalY, writeSectionX, writeSectionY, writeOffsetX, writeOffsetY);
//             m_pTerrainManager->SetPointHeight(globalX, globalY, height);
//             Log_DevPrintf("%i %i -> %i %i (%i %i :: %i %i) -> %f", pointX, pointY, globalX, globalY, writeSectionX, writeSectionY, writeOffsetX, writeOffsetY, height);
// 
//             int32 tsx, tsy;
//             uint32 tox, toy;
//             m_pTerrainManager->CalculateSectionAndOffsetForPoint(&tsx, &tsy, &tox, &toy, globalX, globalY);
//             DebugAssert(tsx == writeSectionX && tsy == writeSectionY);
//             DebugAssert(tox == writeOffsetX && toy == writeOffsetY);

            int32 globalPointX = globalCoordinateStartX + pointX;
            int32 globalPointY = globalCoordinateStartY - pointY;
            SetPointHeight(globalPointX, globalPointY, height);
            //Log_DevPrintf("%i %i -> %i %i -> %f", pointX, pointY, globalPointX, globalPointY, height);

            pProgressCallbacks->IncrementProgressValue();
        }
    }

    // rebuild quadtrees on affected sections
    pProgressCallbacks->SetStatusText("Rebuilding quadtree...");
    pProgressCallbacks->SetProgressRange(sectionsNeededX * sectionsNeededY);
    pProgressCallbacks->SetProgressValue(0);
    for (int32 sy = 0; sy < sectionsNeededY; sy++)
    {
        for (int32 sx = 0; sx < sectionsNeededX; sx++)
        {
            int32 realSectionX = startSectionX + sx;
            int32 realSectionY = startSectionY + sy;

            TerrainSection *pSection = GetSection(realSectionX, realSectionY);
            DebugAssert(pSection != nullptr);
            pSection->RebuildQuadTree();

            pProgressCallbacks->IncrementProgressValue();
        }
    }

    return true;
}

void MapSourceTerrainData::CopySectionPointWeights(const TerrainSection *pSourceSection, uint32 sourceOffsetX, uint32 sourceOffsetY, TerrainSection *pDestinationSection, uint32 destinationOffsetX, uint32 destinationOffsetY)
{
    // get a list of layers and weights from the section
    uint8 layerIndices[TERRAIN_MAX_LAYERS];
    float layerWeights[TERRAIN_MAX_LAYERS];
    uint32 nLayers = pSourceSection->GetSplatMapValues(sourceOffsetX, sourceOffsetY, layerIndices, layerWeights, countof(layerIndices));
    
    // clear from the other section
    pDestinationSection->ClearSplatMapValues(destinationOffsetX, destinationOffsetY);

    // and copy weights in
    bool layersModified = false;
    for (uint32 i = 0; i < nLayers; i++)
    {
        if (!pDestinationSection->HasSplatMapForLayer(layerIndices[i]))
            layersModified = true;

        pDestinationSection->SetSplatMapValue(destinationOffsetX, destinationOffsetY, layerIndices[i], layerWeights[i], false);
    }

    // invoke callbacks
    if (m_pEditCallbacks != nullptr)
    {
        if (layersModified)
            m_pEditCallbacks->OnSectionLayersModified(pDestinationSection);

        m_pEditCallbacks->OnSectionPointLayersModified(pDestinationSection, destinationOffsetX, destinationOffsetY);
    }
}

void MapSourceTerrainData::HandleEdgePointsHeightUpdate(TerrainSection *pSection, uint32 offsetX, uint32 offsetY)
{
    int32 sectionX = pSection->GetSectionX();
    int32 sectionY = pSection->GetSectionY();
    float height = pSection->GetHeightMapValue(offsetX, offsetY);

    // update section to the west
    if (offsetX == 0)
    {
        TerrainSection *pAdjacentSection = GetSection(sectionX - 1, sectionY);
        if (pAdjacentSection != nullptr && pAdjacentSection->GetHeightMapValue(m_parameters.SectionSize, offsetY) != height)
        {
            pAdjacentSection->SetHeightMapValue(m_parameters.SectionSize, offsetY, height);

            if (m_pEditCallbacks != nullptr)
                m_pEditCallbacks->OnSectionPointHeightModified(pAdjacentSection, m_parameters.SectionSize, offsetY);
        }

    }

    // update section to the south
    if (offsetY == 0)
    {
        TerrainSection *pAdjacentSection = GetSection(sectionX, sectionY - 1);
        if (pAdjacentSection != nullptr && pAdjacentSection->GetHeightMapValue(offsetX, m_parameters.SectionSize) != height)
        {
            pAdjacentSection->SetHeightMapValue(offsetX, m_parameters.SectionSize, height);

            if (m_pEditCallbacks != nullptr)
                m_pEditCallbacks->OnSectionPointHeightModified(pAdjacentSection, offsetX, m_parameters.SectionSize);
        }
    }

    // update section to the southwest
    if (offsetX == 0 && offsetY == 0)
    {
        TerrainSection *pAdjacentSection = GetSection(sectionX - 1, sectionY - 1);
        if (pAdjacentSection != nullptr && pAdjacentSection->GetHeightMapValue(m_parameters.SectionSize, m_parameters.SectionSize) != height)
        {
            pAdjacentSection->SetHeightMapValue(m_parameters.SectionSize, m_parameters.SectionSize, height);

            if (m_pEditCallbacks != nullptr)
                m_pEditCallbacks->OnSectionPointHeightModified(pAdjacentSection, m_parameters.SectionSize, m_parameters.SectionSize);
        }
    }
}

void MapSourceTerrainData::HandleEdgePointsWeightUpdate(TerrainSection *pSection, uint32 offsetX, uint32 offsetY)
{
    int32 sectionX = pSection->GetSectionX();
    int32 sectionY = pSection->GetSectionY();

    // update section to the west
    if (offsetX == 0)
    {
        TerrainSection *pAdjacentSection = GetSection(sectionX - 1, sectionY);
        if (pAdjacentSection != nullptr)
            CopySectionPointWeights(pSection, offsetX, offsetY, pAdjacentSection, m_parameters.SectionSize, offsetY);
    }

    // update section to the south
    if (offsetY == 0)
    {
        TerrainSection *pAdjacentSection = GetSection(sectionX, sectionY - 1);
        if (pAdjacentSection != nullptr)
            CopySectionPointWeights(pSection, offsetX, offsetY, pAdjacentSection, offsetX, m_parameters.SectionSize);
    }

    // update section to the southwest
    if (offsetX == 0 && offsetY == 0)
    {
        TerrainSection *pAdjacentSection = GetSection(sectionX - 1, sectionY - 1);
        if (pAdjacentSection != nullptr)
            CopySectionPointWeights(pSection, offsetX, offsetY, pAdjacentSection, m_parameters.SectionSize, m_parameters.SectionSize);
    }
}

bool MapSourceTerrainData::NormalizePointLayerWeights(TerrainSection *pSection, uint32 offsetX, uint32 offsetY)
{
    // needs adjacent sections for zero offsets
    if ((offsetX == 0 || offsetY == 0) && !EnsureAdjacentSectionsLoaded(pSection->GetSectionX(), pSection->GetSectionY()))
        return false;

    // get a list of layers and weights from the section
    uint8 layerIndices[TERRAIN_MAX_LAYERS];
    float layerWeights[TERRAIN_MAX_LAYERS];
    uint32 nLayers = pSection->GetSplatMapValues(offsetX, offsetY, layerIndices, layerWeights, countof(layerIndices));

    // get the total length of all weights
    float weightLength = 0.0f;
    for (uint32 i = 0; i < nLayers; i++)
        weightLength += layerWeights[i];

    // no weights or zero length?
    if (nLayers == 0 || Math::NearEqual(weightLength, 0.0f, Y_FLT_EPSILON))
    {
        // zero layers, ugh. set the default layer to full weight
        pSection->ClearSplatMapValues(offsetX, offsetY);
        pSection->SetSplatMapValue(offsetX, offsetY, GetDefaultLayer(), 1.0f, false);
        HandleEdgePointsWeightUpdate(pSection, offsetX, offsetY);
        return true;
    }

    // divide all weights by this length
    for (uint32 i = 0; i < nLayers; i++)
    {
        float newWeight = layerWeights[i] / weightLength;
        if (layerWeights[i] != newWeight)
            pSection->SetSplatMapValue(offsetX, offsetY, layerIndices[i], newWeight, false);
    }

    // invoke callbacks
    if (m_pEditCallbacks != nullptr)
        m_pEditCallbacks->OnSectionPointLayersModified(pSection, offsetX, offsetY);

    // handle edge cases
    HandleEdgePointsWeightUpdate(pSection, offsetX, offsetY);
    return true;
}

bool MapSourceTerrainData::NormalizeSectionLayerWeights(TerrainSection *pSection)
{
    // should have adjacent sections loaded
    if (!EnsureAdjacentSectionsLoaded(pSection->GetSectionX(), pSection->GetSectionY()))
        return false;

    // normalize each point
    for (uint32 y = 0; y < m_parameters.SectionSize; y++)
    {
        for (uint32 x = 0; x < m_parameters.SectionSize; x++)
        {
            NormalizePointLayerWeights(pSection, x, y);
        }
    }

    return true;
}

bool MapSourceTerrainData::RemovePointLayerWeights(TerrainSection *pSection, uint32 offsetX, uint32 offsetY, float threshold /*= 0.1f*/, bool normalizeAfterRemove /*= true*/)
{
    // needs adjacent sections for zero offsets
    if ((offsetX == 0 || offsetY == 0) && !EnsureAdjacentSectionsLoaded(pSection->GetSectionX(), pSection->GetSectionY()))
        return false;

    // get a list of layers and weights from the section
    uint8 layerIndices[TERRAIN_MAX_LAYERS];
    float layerWeights[TERRAIN_MAX_LAYERS];
    uint32 nLayers = pSection->GetSplatMapValues(offsetX, offsetY, layerIndices, layerWeights, countof(layerIndices));

    // new layer indices/weights
    uint8 newLayerIndices[TERRAIN_MAX_LAYERS];
    float newLayerWeights[TERRAIN_MAX_LAYERS];
    float newLayerWeightLength = 0.0f;
    uint32 nNewLayers = 0;

    // has layers?
    if (nLayers > 0)
    {
        // process each layer
        for (uint32 i = 0; i < nLayers; i++)
        {
            if (layerWeights[i] >= threshold)
            {
                newLayerIndices[nNewLayers] = layerIndices[i];
                newLayerWeights[nNewLayers] = layerWeights[i];
                newLayerWeightLength += layerWeights[i];
                nNewLayers++;
            }
        }
    }

    // normalize?
    if (normalizeAfterRemove)
    {
        if (nNewLayers == 0 || Math::NearEqual(newLayerWeightLength, 0.0f, Y_FLT_EPSILON))
        {
            // zero layers, ugh. set the default layer to full weight
            pSection->ClearSplatMapValues(offsetX, offsetY);
            pSection->SetSplatMapValue(offsetX, offsetY, GetDefaultLayer(), 1.0f, false);
            HandleEdgePointsWeightUpdate(pSection, offsetX, offsetY);
            return true;
        }

        // normalize each weight
        for (uint32 i = 0; i < nNewLayers; i++)
            newLayerWeights[i] = newLayerWeights[i] / newLayerWeightLength;
    }

    // set them
    pSection->ClearSplatMapValues(offsetX, offsetY);
    for (uint32 i = 0; i < nNewLayers; i++)
        pSection->SetSplatMapValue(offsetX, offsetY, newLayerIndices[i], newLayerWeights[i], false);

    // callbacks
    if (m_pEditCallbacks != nullptr)
        m_pEditCallbacks->OnSectionPointLayersModified(pSection, offsetX, offsetY);

    return true;
}

bool MapSourceTerrainData::RemoveSectionLayerWeights(TerrainSection *pSection, float threshold /* = 0.1f */, bool normalizeAfterRemove /* = true */)
{
    // should have adjacent sections loaded
    if (!EnsureAdjacentSectionsLoaded(pSection->GetSectionX(), pSection->GetSectionY()))
        return false;

    // clear each point
    for (uint32 y = 0; y < m_parameters.SectionSize; y++)
    {
        for (uint32 x = 0; x < m_parameters.SectionSize; x++)
        {
            RemoveSectionLayerWeights(pSection, threshold, normalizeAfterRemove);
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////

bool MapSource::LoadTerrain(ProgressCallbacks *pProgressCallbacks)
{
    FILESYSTEM_STAT_DATA statData;
    if (!m_pMapArchive->StatFile("terrain.xml", &statData))
        return true;

    MapSourceTerrainData *pTerrainData = new MapSourceTerrainData(this);
    if (!pTerrainData->Load(pProgressCallbacks))
    {
        delete pTerrainData;
        return false;
    }

    m_pTerrainData = pTerrainData;
    return true;
}

bool MapSource::SaveTerrain(ProgressCallbacks *pProgressCallbacks) const
{
    if (m_pTerrainData == NULL)
        return true;

    return m_pTerrainData->Save(pProgressCallbacks);
}

MapSourceTerrainData *MapSource::CreateTerrainData(const TerrainLayerListGenerator *pLayerList, TERRAIN_HEIGHT_STORAGE_FORMAT heightStorageFormat, uint32 scale, uint32 sectionSize, uint32 renderLODCount, int32 minHeight, int32 maxHeight, int32 baseHeight, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    DebugAssert(m_pTerrainData == NULL);
    DebugAssert((m_regionSize % sectionSize * scale) == 0);

    // create parameters
    TerrainParameters parameters(heightStorageFormat, minHeight, maxHeight, baseHeight, scale, sectionSize, renderLODCount);
    if (!TerrainUtilities::IsValidParameters(&parameters))
        return nullptr;

    // create data
    m_pTerrainData = new MapSourceTerrainData(this);
    if (!m_pTerrainData->Create(&parameters, pLayerList, pProgressCallbacks))
    {
        delete m_pTerrainData;
        m_pTerrainData = nullptr;
        return nullptr;
    }

    return m_pTerrainData;
}

void MapSource::DeleteTerrainData(ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    DebugAssert(m_pTerrainData != NULL);

    m_pTerrainData->Delete();
    m_pTerrainData = NULL;
}

/*
    // set all heights on the top row from the top section
    for (uint32 x = 0; x < sectionSize; x++)
    {
        SetIndexedHeight(x, sectionSize, (pTopSection != NULL) ? pTopSection->GetIndexedHeight(x, 0) : baseHeight);

        if (pTopSection != NULL)
        {
            const uint8 *pRawLayerValues = pTopSection->m_pLayerWeightValues;
            for (uint32 i = 0; i < pTopSection->m_nUsedLayers; i++)
            {
                uint8 value = pRawLayerValues[(i * m_layerWeightValuesStride) + x];
                if (value != 0)
                    SetIndexedBaseLayerWeight(x, sectionSize, pTopSection->m_usedLayers[i], (float)value / 255.0f);
            }
        }
    }

    // set all heights on the right column from the top section
    for (uint32 y = 0; y < sectionSize; y++)
    {
        SetIndexedHeight(sectionSize, y, (pRightSection != NULL) ? pRightSection->GetIndexedHeight(0, y) : baseHeight);

        if (pRightSection != NULL)
        {
            const uint8 *pRawLayerValues = pRightSection->m_pLayerWeightValues;
            for (uint32 i = 0; i < pRightSection->m_nUsedLayers; i++)
            {
                uint8 value = pRawLayerValues[(i * m_layerWeightValuesStride) + (y * m_pointCount) + sectionSize];
                if (value != 0)
                    SetIndexedBaseLayerWeight(sectionSize, y, pRightSection->m_usedLayers[i], (float)value / 255.0f);
            }
        }
    }

    // set corner texel from the origin of the top-right section
    SetIndexedHeight(sectionSize, sectionSize, (pTopRightSection != NULL) ? pTopRightSection->GetIndexedHeight(0, 0) : baseHeight);

    if (pTopRightSection != NULL)
    {
        const uint8 *pRawLayerValues = pTopRightSection->m_pLayerWeightValues;
        for (uint32 i = 0; i < pTopRightSection->m_nUsedLayers; i++)
        {
            uint8 value = pRawLayerValues[(i * m_layerWeightValuesStride) + (sectionSize * m_pointCount) + sectionSize];
            if (value != 0)
                SetIndexedBaseLayerWeight(sectionSize, sectionSize, pTopRightSection->m_usedLayers[i], (float)value / 255.0f);
        }
    }
    */
