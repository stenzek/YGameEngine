#include "Engine/PrecompiledHeader.h"
#include "Engine/TerrainSection.h"
#include "Engine/TerrainQuadTree.h"
#include "Engine/DataFormats.h"
#include "Engine/StaticMesh.h"
Log_SetChannel(TerrainSection);

static const uint32 MAX_CHANNELS_PER_SPLAT_MAP = 4;

TerrainSection::TerrainSection(const TerrainParameters *pParameters, int32 sectionX, int32 sectionY, uint32 LODLevel)
    : m_parameters(*pParameters),
      m_sectionX(sectionX),
      m_sectionY(sectionY),
      m_LODLevel(LODLevel),
      m_pointCount(0),
      m_bounds(TerrainUtilities::CalculateSectionBoundingBox(pParameters, sectionX, sectionY)),
      m_changed(false),
      m_pHeightValues(nullptr),
      m_heightValueSize(0),
      m_heightMapRowPitch(0),
      m_pSplatMaps(nullptr),
      m_nSplatMaps(0),
      m_pQuadTree(NULL)
{
    DebugAssert((pParameters->SectionSize >> LODLevel) > 0);
    DebugAssert(m_pHeightValues == NULL);

    m_pointCount = (pParameters->SectionSize >> LODLevel) + 1;
    m_heightValueSize = TerrainUtilities::GetHeightElementStorageSize(pParameters->HeightStorageFormat);
    m_heightMapRowPitch = PixelFormat_CalculateRowPitch(TerrainUtilities::GetHeightStoragePixelFormat(pParameters->HeightStorageFormat), m_pointCount);

    // allocate memory for height
    m_pHeightValues = (byte *)Y_malloc(m_heightMapRowPitch * m_pointCount);
}

TerrainSection::~TerrainSection()
{
    delete m_pQuadTree;

    for (uint32 i = 0; i < m_nSplatMaps; i++)
        Y_free(m_pSplatMaps[i].pData);
    delete[] m_pSplatMaps;

    Y_free(m_pHeightValues);
}

void TerrainSection::Create(float createHeight, uint8 createLayer)
{
    DebugAssert(m_LODLevel == 0);

    // set all heights this section owns to the specified height
    SetAllHeightMapValues(createHeight);
    SetAllSplatMapValues(createLayer, 1.0f);

    // build a quadtree
    RebuildQuadTree();
}

bool TerrainSection::LoadFromStream(ByteStream *pStream)
{
    // read header
    DF_TERRAIN_SECTION_HEADER sectionHeader;
    if (!pStream->Read2(&sectionHeader, sizeof(sectionHeader)) ||
        sectionHeader.Magic != DF_TERRAIN_SECTION_HEADER_MAGIC ||
        sectionHeader.HeaderSize != sizeof(sectionHeader))
    {
        return false;
    }

    // parse header
    if (sectionHeader.PointCount != m_pointCount ||
        sectionHeader.HeightMapValueSize != m_heightValueSize ||
        sectionHeader.HeightMapRowPitch != m_heightMapRowPitch)
    {
        return false;
    }

    // read height data
    if (!pStream->Read2(m_pHeightValues, m_heightMapRowPitch * m_pointCount))
        return false;

    // read splat maps
    m_nSplatMaps = sectionHeader.SplatMapCount;
    if (m_nSplatMaps > 0)
    {
        // this will nullany pointers
        m_pSplatMaps = new SplatMap[m_nSplatMaps];
        Y_memzero(m_pSplatMaps, sizeof(SplatMap) * m_nSplatMaps);

        // read the actual maps
        for (uint32 splatMapIndex = 0; splatMapIndex < m_nSplatMaps; splatMapIndex++)
        {
            SplatMap *pSplatMap = &m_pSplatMaps[splatMapIndex];

            DF_TERRAIN_SECTION_SPLAT_MAP_HEADER splatMapHeader;
            if (!pStream->Read2(&splatMapHeader, sizeof(splatMapHeader)))
                return false;

            uint32 splatMapSize = splatMapHeader.RowPitch * m_pointCount;
            DebugAssert(splatMapHeader.LayerCount < 4 && splatMapHeader.ChannelCount > 0 && splatMapHeader.ChannelCount <= 4);

            for (uint32 layerIndex = 0; layerIndex < 4; layerIndex++)
                pSplatMap->Layers[layerIndex] = splatMapHeader.Layers[layerIndex];

            pSplatMap->LayerCount = splatMapHeader.LayerCount;
            pSplatMap->ChannelCount = splatMapHeader.ChannelCount;
            pSplatMap->pData = (uint8 *)malloc(splatMapSize);
            pSplatMap->RowPitch = splatMapHeader.RowPitch;
            if (!pStream->Read2(pSplatMap->pData, splatMapSize))
                return false;
        }
    }

    // read quadtree
    m_pQuadTree = new TerrainSectionQuadTree(this);
    if (!m_pQuadTree->LoadFromStream(pStream))
        return false;

    return true;
}

bool TerrainSection::SaveToStream(ByteStream *pStream) const
{
    // write header
    DF_TERRAIN_SECTION_HEADER sectionHeader;
    sectionHeader.Magic = DF_TERRAIN_SECTION_HEADER_MAGIC;
    sectionHeader.HeaderSize = sizeof(sectionHeader);
    sectionHeader.PointCount = m_pointCount;
    sectionHeader.HeightMapValueSize = m_heightValueSize;
    sectionHeader.HeightMapRowPitch = m_heightMapRowPitch;
    sectionHeader.SplatMapCount = m_nSplatMaps;
    if (!pStream->Write2(&sectionHeader, sizeof(sectionHeader)))
        return false;

    // write heightmap
    if (!pStream->Write2(m_pHeightValues, m_heightMapRowPitch * m_pointCount))
        return false;

    // write splat maps
    for (uint32 splatMapIndex = 0; splatMapIndex < m_nSplatMaps; splatMapIndex++)
    {
        const SplatMap *pSplatMap = &m_pSplatMaps[splatMapIndex];
        uint32 splatMapSize = pSplatMap->RowPitch * m_pointCount;

        DF_TERRAIN_SECTION_SPLAT_MAP_HEADER splatMapHeader;
        for (uint32 layerIndex = 0; layerIndex < 4; layerIndex++)
            splatMapHeader.Layers[layerIndex] = pSplatMap->Layers[layerIndex];
        splatMapHeader.LayerCount = pSplatMap->LayerCount;
        splatMapHeader.ChannelCount = pSplatMap->ChannelCount;
        splatMapHeader.RowPitch = pSplatMap->RowPitch;

        // write splatmap header
        if (!pStream->Write2(&splatMapHeader, sizeof(splatMapHeader)))
            return false;

        // write splatmap data
        if (!pStream->Write2(pSplatMap->pData, splatMapSize))
            return false;
    }

    // write quadtree
    if (!m_pQuadTree->SaveToStream(pStream))
        return false;
    
    // done
    return true;
}

void TerrainSection::GetRawHeightMapData(const void **pDataPointer, uint32 *pValueSize, uint32 *pRowPitch) const
{
    *pDataPointer = m_pHeightValues;
    *pValueSize = m_heightValueSize;
    *pRowPitch = m_heightMapRowPitch;
}

void TerrainSection::GetRawSplatMapData(uint32 mapIndex, const void **pDataPointer, uint32 *pValueSize, uint32 *pRowPitch) const
{
    DebugAssert(mapIndex < m_nSplatMaps);
    *pDataPointer = m_pSplatMaps[mapIndex].pData;
    *pValueSize = sizeof(uint8) * m_pSplatMaps[mapIndex].ChannelCount;
    *pRowPitch = m_pSplatMaps[mapIndex].RowPitch;
}

bool TerrainSection::RayCast(const Ray &ray, float3 &contactNormal, float3 &contactPoint, bool exitAtFirstIntersection /*= false*/) const
{
    // best contacts
    float bestContactTimeSq = Y_FLT_INFINITE;
    float3 bestContactPoint;
    float3 bestContactNormal;
    bool continueSearch = true;

    // use quadtree to break down search
    m_pQuadTree->EnumerateNodesIntersectingRay(ray, 0, [this, ray, &bestContactTimeSq, &bestContactPoint, &bestContactNormal, exitAtFirstIntersection, &continueSearch](const TerrainQuadTreeNode *pNode)
    {
        if (!continueSearch)
            return;

        // vars
        const float3 &sectionMinBounds = m_bounds.GetMinBounds();
        uint32 scale = m_parameters.Scale;

        // get range
        uint32 startX = pNode->GetStartQuadX();
        uint32 startY = pNode->GetStartQuadY();
        uint32 endX = startX + pNode->GetNodeSize();
        uint32 endY = startY + pNode->GetNodeSize();

        float3 v0, v1, v2, v3;
        float3 triangleContactPoint, triangleContactNormal;
        float triangleContactTimeSq;

        // todo: contract search range to ray bounding box for perf

        // iterate over points
        for (uint32 ly = startY; ly < endY; ly++)
        {
            for (uint32 lx = startX; lx < endX; lx++)
            {
                // first triangle
                v0.Set(sectionMinBounds.x + (float)((lx)* scale),
                       sectionMinBounds.y + (float)((ly + 1) * scale),
                       GetHeightMapValue(lx, ly + 1));

                v1.Set(sectionMinBounds.x + (float)((lx)* scale),
                       sectionMinBounds.y + (float)((ly)* scale),
                       GetHeightMapValue(lx, ly));


                v2.Set(sectionMinBounds.x + (float)((lx + 1) * scale),
                       sectionMinBounds.y + (float)((ly + 1) * scale),
                       GetHeightMapValue(lx + 1, ly + 1));

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

                    if (exitAtFirstIntersection)
                    {
                        continueSearch = false;
                        return;
                    }

                    // don't bother testing the other triangle, it's unlikely to hit both?
                    continue;
                }

                // fill last vertex
                v3.Set(sectionMinBounds.x + (float)((lx + 1) * scale),
                       sectionMinBounds.y + (float)((ly)* scale),
                       GetHeightMapValue(lx + 1, ly));

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

                    if (exitAtFirstIntersection)
                    {
                        continueSearch = false;
                        return;
                    }
                }
            }
        }
    });

    if (bestContactTimeSq == Y_FLT_INFINITE)
        return false;

    contactNormal = bestContactNormal;
    contactPoint = bestContactPoint;
    return true;
}

const float TerrainSection::GetHeightMapValue(uint32 offsetX, uint32 offsetY) const
{
    DebugAssert(offsetX < m_pointCount && offsetY < m_pointCount);
    uint32 heightMapOffset = offsetY * m_heightMapRowPitch + offsetX * m_heightValueSize;

    float height;
    switch (m_parameters.HeightStorageFormat)
    {
    case TERRAIN_HEIGHT_STORAGE_FORMAT_UINT8:
        {
            float minHeight = (float)m_parameters.MinHeight;
            float heightRange = (float)m_parameters.MaxHeight - minHeight;
            const uint8 *pValuePointer = reinterpret_cast<const uint8 *>(m_pHeightValues + heightMapOffset);
            height = minHeight + (float(*pValuePointer) / 255.0f) * heightRange;
        }
        break;

    case TERRAIN_HEIGHT_STORAGE_FORMAT_UINT16:
        {
            float minHeight = (float)m_parameters.MinHeight;
            float heightRange = (float)m_parameters.MaxHeight - minHeight;
            const uint16 *pValuePointer = reinterpret_cast<const uint16 *>(m_pHeightValues + heightMapOffset);
            height = minHeight + (float(*pValuePointer) / 65535.0f) * heightRange;
        }
        break;

    case TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32:
        {
            // direct mapped
            const float *pValuePointer = reinterpret_cast<const float *>(m_pHeightValues + heightMapOffset);
            height = *pValuePointer;
        }
        break;

    default:
        UnreachableCode();
        return 0.0f;
    }

    return height;
}

void TerrainSection::SetHeightMapValue(uint32 offsetX, uint32 offsetY, float height)
{
    DebugAssert(offsetX < m_pointCount && offsetY < m_pointCount);
    uint32 heightMapOffset = offsetY * m_heightMapRowPitch + offsetX * m_heightValueSize;
    float oldHeight;

    switch (m_parameters.HeightStorageFormat)
    {
    case TERRAIN_HEIGHT_STORAGE_FORMAT_UINT8:
        {
            float minHeight = (float)m_parameters.MinHeight;
            float heightRange = (float)m_parameters.MaxHeight - minHeight;
            uint8 newHeightValue = (uint8)Min(Max(((height - minHeight) / heightRange) * 255.0f, 0.0f), 255.0f);

            uint8 *pValuePointer = reinterpret_cast<uint8 *>(m_pHeightValues + heightMapOffset);
            if (*pValuePointer == newHeightValue)
                return;

            oldHeight = minHeight + (float(*pValuePointer) / 255.0f) * heightRange;
            *pValuePointer = newHeightValue;
        }
        break;

    case TERRAIN_HEIGHT_STORAGE_FORMAT_UINT16:
        {
            float minHeight = (float)m_parameters.MinHeight;
            float heightRange = (float)m_parameters.MaxHeight - minHeight;
            uint16 newHeightValue = (uint16)Min(Max(((height - minHeight) / heightRange) * 65535.0f, 0.0f), 65535.0f);

            uint16 *pValuePointer = reinterpret_cast<uint16 *>(m_pHeightValues + heightMapOffset);
            if (*pValuePointer == newHeightValue)
                return;

            oldHeight = minHeight + (float(*pValuePointer) / 65535.0f) * heightRange;
            *pValuePointer = newHeightValue;
        }
        break;

    case TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32:
        {
            float *pValuePointer = reinterpret_cast<float *>(m_pHeightValues + heightMapOffset);
            if (*pValuePointer == height)
                return;

            oldHeight = *pValuePointer;
            *pValuePointer = height;
        }
        break;

    default:
        UnreachableCode();
        return;
    }

    m_changed = true;

    if (m_pQuadTree != NULL)
    {
        bool quadTreeNeedsRebuild = false;
        m_pQuadTree->UpdateMinMaxHeight(offsetX, offsetY, height, oldHeight, &quadTreeNeedsRebuild);
        if (quadTreeNeedsRebuild)
        {
            Log_PerfPrintf("TerrainSection::SetIndexedHeight: Setting height (%u,%u) %.2f -> %.2f triggered quadtree rebuild.", offsetX, offsetY, oldHeight, height);
            RebuildQuadTree();
        }
    }
}

void TerrainSection::SetAllHeightMapValues(float height)
{
    switch (m_parameters.HeightStorageFormat)
    {
    case TERRAIN_HEIGHT_STORAGE_FORMAT_UINT8:
        {
            float minHeight = (float)m_parameters.MinHeight;
            float heightRange = (float)m_parameters.MaxHeight - minHeight;
            uint8 value8 = (uint8)Min(Max(((height - minHeight) / heightRange) * 255.0f, 0.0f), 255.0f);

            byte *pRowPointer = m_pHeightValues;
            for (uint32 y = 0; y < m_pointCount; y++)
            {
                uint8 *pValuePointer = reinterpret_cast<uint8 *>(pRowPointer);
                for (uint32 x = 0; x < m_pointCount; x++)
                    *(pValuePointer++) = value8;

                pRowPointer += m_heightMapRowPitch;
            }
        }
        break;

    case TERRAIN_HEIGHT_STORAGE_FORMAT_UINT16:
        {
            float minHeight = (float)m_parameters.MinHeight;
            float heightRange = (float)m_parameters.MaxHeight - minHeight;
            uint16 value16 = (uint16)Min(Max(((height - minHeight) / heightRange) * 65535.0f, 0.0f), 65535.0f);

            byte *pRowPointer = m_pHeightValues;
            for (uint32 y = 0; y < m_pointCount; y++)
            {
                uint16 *pValuePointer = reinterpret_cast<uint16 *>(pRowPointer);
                for (uint32 x = 0; x < m_pointCount; x++)
                    *(pValuePointer++) = value16;

                pRowPointer += m_heightMapRowPitch;
            }
        }
        break;

    case TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32:
        {
            byte *pRowPointer = m_pHeightValues;
            for (uint32 y = 0; y < m_pointCount; y++)
            {
                float *pValuePointer = reinterpret_cast<float *>(pRowPointer);
                for (uint32 x = 0; x < m_pointCount; x++)
                    *(pValuePointer++) = height;

                pRowPointer += m_heightMapRowPitch;
            }
        }
        break;
    }

    m_changed = true;
    
    if (m_pQuadTree != NULL)
        RebuildQuadTree();
}

float3 TerrainSection::CalculateNormalAtPoint(uint32 x, uint32 y) const
{
    float val, valU, valV;

    // get value
    val = GetHeightMapValue(x, y);

    // get in direction right
    if (x == (m_pointCount - 1))
        valU = val;
    else
        valU = GetHeightMapValue(x + 1, y);

    // .. down
    if (y == (m_pointCount - 1))
        valV = val;
    else
        valV = GetHeightMapValue(x, y + 1);

    // get direction
    return float3(val - valU, val - valV, 0.5f).Normalize();
}

void TerrainSection::AllocateLayerInSplatMap(uint8 layerIndex, uint32 *pMapIndex, uint32 *pChannelIndex)
{
    static const PIXEL_FORMAT splatMapFormats[MAX_CHANNELS_PER_SPLAT_MAP + 1] = { PIXEL_FORMAT_UNKNOWN, PIXEL_FORMAT_R8_UNORM, PIXEL_FORMAT_R8G8_UNORM, PIXEL_FORMAT_R8G8B8A8_UNORM, PIXEL_FORMAT_R8G8B8A8_UNORM };

    // shouldn't be allocated
    DebugAssert(!GetSplatMapLocation(layerIndex, pMapIndex, pChannelIndex));

    // find a splat map with an unused channel
    uint32 mapIndex;
    for (mapIndex = 0; mapIndex < m_nSplatMaps; mapIndex++)
    {
        SplatMap *pSplatMap = &m_pSplatMaps[mapIndex];
        if (pSplatMap->LayerCount < 4)
            break;
    }

    // no free channels?
    if (mapIndex < m_nSplatMaps)
    {
        // reallocate this splatmap
        SplatMap *pSplatMap = &m_pSplatMaps[mapIndex];
        uint32 channelIndex = pSplatMap->LayerCount;
        uint32 newLayerCount = pSplatMap->LayerCount + 1;
        DebugAssert(pSplatMap->LayerCount > 0 && pSplatMap->LayerCount < 4);
        
        // allocate memory
        uint32 channelsToAllocate = (newLayerCount >= 3) ? MAX_CHANNELS_PER_SPLAT_MAP : newLayerCount;
        if (channelsToAllocate != pSplatMap->ChannelCount)
        {
            uint32 newSplatMapRowPitch = PixelFormat_CalculateRowPitch(splatMapFormats[channelsToAllocate], m_pointCount);
            uint32 newSplatMapSize = newSplatMapRowPitch * m_pointCount;
            uint8 *pNewSplatMap = (uint8 *)malloc(newSplatMapSize);
            Y_memzero(pNewSplatMap, newSplatMapSize);

            // copy values in
            for (uint32 y = 0; y < m_pointCount; y++)
            {
                const uint8 *pSourcePointer = pSplatMap->pData + y * pSplatMap->RowPitch;
                uint8 *pDestinationPointer = pNewSplatMap + y * newSplatMapRowPitch;

                for (uint32 x = 0; x < m_pointCount; x++)
                {
                    uint8 *pWritePointer = pDestinationPointer;

                    switch (pSplatMap->LayerCount)
                    {
                    case 3:
                        *(pWritePointer++) = *(pSourcePointer++);

                    case 2:
                        *(pWritePointer++) = *(pSourcePointer++);

                    case 1:
                        *(pWritePointer++) = *(pSourcePointer++);
                    }

                    pDestinationPointer += channelsToAllocate;
                }
            }

            // free old data
            Y_free(pSplatMap->pData);

            // update data
            pSplatMap->ChannelCount = channelsToAllocate;
            pSplatMap->pData = pNewSplatMap;
            pSplatMap->RowPitch = newSplatMapRowPitch;
        }

        // update struct
        pSplatMap->Layers[channelIndex] = layerIndex;
        pSplatMap->LayerCount = newLayerCount;

        // set pointers
        Log_DevPrintf("TerrainSection::AllocateLayerInSplatMap: For layer %i, reusing splat map %u channel %u", (uint32)layerIndex, mapIndex, channelIndex);
        *pMapIndex = mapIndex;
        *pChannelIndex = channelIndex;
    }
    else
    {
        // resize splatmap array
        SplatMap *pNewSplatMapArray = new SplatMap[m_nSplatMaps + 1];
        if (m_nSplatMaps > 0)
        {
            Y_memcpy(pNewSplatMapArray, m_pSplatMaps, sizeof(SplatMap) * m_nSplatMaps);
            delete[] m_pSplatMaps;
        }
        m_pSplatMaps = pNewSplatMapArray;
        m_nSplatMaps++;

        // allocate memory
        uint32 channelsToAllocate = 1;
        uint32 newSplatMapRowPitch = PixelFormat_CalculateRowPitch(splatMapFormats[channelsToAllocate], m_pointCount);
        uint32 newSplatMapSize = newSplatMapRowPitch * m_pointCount;
        uint8 *pNewSplatMap = (uint8 *)malloc(newSplatMapSize);
        Y_memzero(pNewSplatMap, newSplatMapSize);

        // fill the data
        SplatMap *pSplatMap = &m_pSplatMaps[mapIndex];
        pSplatMap->Layers[0] = layerIndex;
        pSplatMap->Layers[1] = 0xFF;
        pSplatMap->Layers[2] = 0xFF;
        pSplatMap->Layers[3] = 0xFF;
        pSplatMap->LayerCount = 1;
        pSplatMap->ChannelCount = 1;
        pSplatMap->pData = pNewSplatMap;
        pSplatMap->RowPitch = newSplatMapRowPitch;

        // set pointers
        Log_DevPrintf("TerrainSection::AllocateLayerInSplatMap: For layer %i, allocating new splat map %u", (uint32)layerIndex, mapIndex);
        *pMapIndex = mapIndex;
        *pChannelIndex = 0;
    }
}

bool TerrainSection::GetSplatMapLocation(uint8 layerIndex, uint32 *pMapIndex, uint32 *pChannelIndex) const
{
    for (uint32 mapIndex = 0; mapIndex < m_nSplatMaps; mapIndex++)
    {
        const SplatMap *pSplatMap = &m_pSplatMaps[mapIndex];

        for (uint32 channelIndex = 0; channelIndex < pSplatMap->LayerCount; channelIndex++)
        {
            if (pSplatMap->Layers[channelIndex] == layerIndex)
            {
                *pMapIndex = mapIndex;
                *pChannelIndex = channelIndex;
                return true;
            }
        }
    }

    return false;
}

const bool TerrainSection::HasSplatMapForLayer(uint8 layerIndex) const
{
    for (uint32 mapIndex = 0; mapIndex < m_nSplatMaps; mapIndex++)
    {
        const SplatMap *pSplatMap = &m_pSplatMaps[mapIndex];

        for (uint32 channelIndex = 0; channelIndex < pSplatMap->LayerCount; channelIndex++)
        {
            if (pSplatMap->Layers[channelIndex] == layerIndex)
                return true;
        }
    }

    return false;
}

const uint8 TerrainSection::GetSplatMapDominantLayer(uint32 offsetX, uint32 offsetY) const
{
    uint8 bestLayer = 0xFF;
    uint8 bestLayerValue = 0;

    for (uint32 mapIndex = 0; mapIndex < m_nSplatMaps; mapIndex++)
    {
        const SplatMap *pSplatMap = &m_pSplatMaps[mapIndex];
        const uint8 *pBasePointer = pSplatMap->pData + (offsetY * pSplatMap->RowPitch) + (offsetX * pSplatMap->ChannelCount);

        for (uint32 channelIndex = 0; channelIndex < pSplatMap->LayerCount; channelIndex++)
        {
            if (bestLayer == 0xFF || pBasePointer[channelIndex] > bestLayerValue)
            {
                bestLayer = pSplatMap->Layers[channelIndex];
                bestLayerValue = pBasePointer[channelIndex];
            }
        }
    }

    return bestLayer;
}

const uint32 TerrainSection::GetSplatMapValues(uint32 offsetX, uint32 offsetY, uint8 *outLayers, float *outLayerWeights, uint32 maxLayers) const
{
    uint32 layerCount = 0;

    for (uint32 mapIndex = 0; mapIndex < m_nSplatMaps && layerCount < maxLayers; mapIndex++)
    {
        const SplatMap *pSplatMap = &m_pSplatMaps[mapIndex];
        const uint8 *pBasePointer = pSplatMap->pData + (offsetY * pSplatMap->RowPitch) + (offsetX * pSplatMap->ChannelCount);

        for (uint32 channelIndex = 0; channelIndex < pSplatMap->LayerCount && layerCount < maxLayers; channelIndex++)
        {
            if (pBasePointer[channelIndex] != 0)
            {
                outLayers[layerCount] = pSplatMap->Layers[channelIndex];
                outLayerWeights[layerCount] = (float)pBasePointer[channelIndex] / 255.0f;
                layerCount++;
            }
        }
    }

    return layerCount;
}

uint8 TerrainSection::GetSplatMapValue(uint32 mapIndex, uint32 channelIndex, uint32 offsetX, uint32 offsetY) const
{
    DebugAssert(mapIndex < m_nSplatMaps && channelIndex < m_pSplatMaps[mapIndex].LayerCount);

    const SplatMap *pSplatMap = &m_pSplatMaps[mapIndex];
    const uint8 *pValuePointer = pSplatMap->pData + (offsetY * pSplatMap->RowPitch) + (offsetX * pSplatMap->ChannelCount) + channelIndex;
    
    return *pValuePointer;
}

void TerrainSection::SetSplatMapValue(uint32 mapIndex, uint32 channelIndex, uint32 offsetX, uint32 offsetY, uint8 value)
{
    DebugAssert(mapIndex < m_nSplatMaps && channelIndex < m_pSplatMaps[mapIndex].LayerCount);

    SplatMap *pSplatMap = &m_pSplatMaps[mapIndex];
    uint8 *pValuePointer = pSplatMap->pData + (offsetY * pSplatMap->RowPitch) + (offsetX * pSplatMap->ChannelCount) + channelIndex;

    *pValuePointer = value;
}

const float TerrainSection::GetSplatMapValue(uint32 offsetX, uint32 offsetY, uint8 layer) const
{
    // find the map/channel index
    uint32 mapIndex, channelIndex;
    if (!GetSplatMapLocation(layer, &mapIndex, &channelIndex))
        return 0.0f;

    // get the value
    return (float)GetSplatMapValue(mapIndex, channelIndex, offsetX, offsetY) / 255.0f;
}

void TerrainSection::SetSplatMapValue(uint32 offsetX, uint32 offsetY, uint8 layer, float value, bool renormalize /*= true*/)
{
    // find the map/channel index
    uint32 mapIndex, channelIndex;
    if (!GetSplatMapLocation(layer, &mapIndex, &channelIndex))
    {
        // allocate one
        AllocateLayerInSplatMap(layer, &mapIndex, &channelIndex);
    }

    // get the current weights
    if (renormalize)
    {
        float remainingWeight = 1.0f - value;
        if (Math::NearEqual(remainingWeight, 0.0f, Y_FLT_EPSILON))
        {
            // just clear everything
            ClearSplatMapValues(offsetX, offsetY);
        }
        else
        {
            // find the length of all current splatmap values
            float currentSplatMapLength = 0.0f;
            for (uint32 normalizeMapIndex = 0; normalizeMapIndex < m_nSplatMaps; normalizeMapIndex++)
            {
                SplatMap *pNormalizeSplatMap = &m_pSplatMaps[normalizeMapIndex];
                for (uint32 normalizeChannelIndex = 0; normalizeChannelIndex < pNormalizeSplatMap->LayerCount; normalizeChannelIndex++)
                {
                    if (normalizeMapIndex != mapIndex || normalizeChannelIndex != channelIndex)
                        currentSplatMapLength += (float)GetSplatMapValue(normalizeMapIndex, normalizeChannelIndex, offsetX, offsetY) / 255.0f;
                }
            }

            // has length?
            if (!Math::NearEqual(currentSplatMapLength, 0.0f, Y_FLT_EPSILON))
            {
                // mod everything
                for (uint32 normalizeMapIndex = 0; normalizeMapIndex < m_nSplatMaps; normalizeMapIndex++)
                {
                    SplatMap *pNormalizeSplatMap = &m_pSplatMaps[normalizeMapIndex];
                    for (uint32 normalizeChannelIndex = 0; normalizeChannelIndex < pNormalizeSplatMap->LayerCount; normalizeChannelIndex++)
                    {
                        if (normalizeMapIndex != mapIndex || normalizeChannelIndex != channelIndex)
                        {
                            // cram it into the remaining range
                            float newWeight = (float)GetSplatMapValue(normalizeMapIndex, normalizeChannelIndex, offsetX, offsetY) / 255.0f;
                            newWeight /= currentSplatMapLength;
                            newWeight *= remainingWeight;
                            SetSplatMapValue(normalizeMapIndex, normalizeChannelIndex, offsetX, offsetY, (uint8)(Math::Clamp(newWeight, 0.0f, 1.0f) * 255.0f));
                        }
                    }
                }
            }
            else
            {
                // just clear everything
                ClearSplatMapValues(offsetX, offsetY);
            }
        }
    }

    // set the value
    SetSplatMapValue(mapIndex, channelIndex, offsetX, offsetY, (uint8)(Math::Clamp(value, 0.0f, 1.0f) * 255.0f));
}

void TerrainSection::ClearSplatMapValues(uint32 offsetX, uint32 offsetY)
{
    for (uint32 mapIndex = 0; mapIndex < m_nSplatMaps; mapIndex++)
    {
        SplatMap *pSplatMap = &m_pSplatMaps[mapIndex];
        for (uint32 channelIndex = 0; channelIndex < pSplatMap->LayerCount; channelIndex++)
            SetSplatMapValue(mapIndex, channelIndex, offsetX, offsetY, 0);
    }
}

void TerrainSection::SetAllSplatMapValues(uint8 layer, float value /*= 1.0f*/)
{
    for (uint32 y = 0; y < m_pointCount; y++)
    {
        for (uint32 x = 0; x < m_pointCount; x++)
        {
            if (value == 1.0f)
                ClearSplatMapValues(x, y);

            SetSplatMapValue(x, y, layer, value);
        }
    }
}

void TerrainSection::CopySplatMapValue(const TerrainSection *pSourceSection, uint32 sourceOffsetX, uint32 sourceOffsetY, uint32 destinationOffsetX, uint32 destinationOffsetY)
{
    // get a list of layers and weights from the section
    uint8 layerIndices[TERRAIN_MAX_LAYERS];
    float layerWeights[TERRAIN_MAX_LAYERS];
    uint32 nLayers = pSourceSection->GetSplatMapValues(sourceOffsetX, sourceOffsetY, layerIndices, layerWeights, countof(layerIndices));

    // clear from the other section
    ClearSplatMapValues(destinationOffsetX, destinationOffsetY);

    // and copy weights in
    for (uint32 i = 0; i < nLayers; i++)
        SetSplatMapValue(destinationOffsetX, destinationOffsetY, layerIndices[i], layerWeights[i]);
}

void TerrainSection::NormalizeSplatMapValue(uint32 offsetX, uint32 offsetY)
{
    // get a list of layers and weights from the section
    uint8 layerIndices[TERRAIN_MAX_LAYERS];
    float layerWeights[TERRAIN_MAX_LAYERS];
    uint32 nLayers = GetSplatMapValues(offsetX, offsetY, layerIndices, layerWeights, countof(layerIndices));

    // get the total length of all weights
    float weightLength = 0.0f;
    for (uint32 i = 0; i < nLayers; i++)
        weightLength += layerWeights[i];

    // no weights or zero length?
    if (nLayers == 0 || Math::NearEqual(weightLength, 0.0f, Y_FLT_EPSILON))
        return;

    // divide all weights by this length
    for (uint32 i = 0; i < nLayers; i++)
    {
        float newWeight = layerWeights[i] / weightLength;
        if (layerWeights[i] != newWeight)
            SetSplatMapValue(offsetX, offsetY, layerIndices[i], newWeight);
    }
}

void TerrainSection::FilterSplatMapValues(uint32 offsetX, uint32 offsetY, float threshold /*= 0.1f*/, bool normalizeAfterRemove /*= true*/)
{
    // get a list of layers and weights from the section
    uint8 layerIndices[TERRAIN_MAX_LAYERS];
    float layerWeights[TERRAIN_MAX_LAYERS];
    uint32 nLayers = GetSplatMapValues(offsetX, offsetY, layerIndices, layerWeights, countof(layerIndices));

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
    if (normalizeAfterRemove && nNewLayers > 0 && Math::NearEqual(newLayerWeightLength, 0.0f, Y_FLT_EPSILON))
    {
        // normalize each weight
        for (uint32 i = 0; i < nNewLayers; i++)
            newLayerWeights[i] = newLayerWeights[i] / newLayerWeightLength;
    }

    // set them
    ClearSplatMapValues(offsetX, offsetY);
    for (uint32 i = 0; i < nNewLayers; i++)
        SetSplatMapValue(offsetX, offsetY, newLayerIndices[i], newLayerWeights[i]);
}

bool TerrainSection::RebuildQuadTree(ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    DebugAssert(m_LODLevel == 0);

#ifdef PROFILE_TERRAIN_QUADTREE_REBUILD_TIMES
    Timer rebuildTimer;
#endif

    AutoReleasePtr<ByteStream> pTemporaryStream = ByteStream_CreateGrowableMemoryStream();
    if (!TerrainSectionQuadTree::Build(&m_parameters, this, pTemporaryStream, pProgressCallbacks))
        return false;

    pTemporaryStream->SeekAbsolute(0);

    TerrainSectionQuadTree *pQuadTree = new TerrainSectionQuadTree(this);
    if (!pQuadTree->LoadFromStream(pTemporaryStream))
    {
        delete pQuadTree;
        return false;
    }

    delete m_pQuadTree;
    m_pQuadTree = pQuadTree;
    m_changed = true;

#ifdef PROFILE_SHADER_COMPILE_TIMES
    Log_ProfilePrintf("TerrainSection::RebuildQuadTree: Rebuild took %.4f msec for section (%i, %i)", rebuildTimer.GetTimeMilliseconds(), m_sectionX, m_sectionY);
#endif

    return true;
}

void TerrainSection::RebuildSplatMaps(bool filterValues /* = true */, float filterThreshold /* = 0.1f */)
{
    // filter first?
    if (filterValues)
    {
        for (uint32 y = 0; y < m_pointCount; y++)
        {
            for (uint32 x = 0; x < m_pointCount; x++)
            {
                FilterSplatMapValues(x, y, filterThreshold, true);
            }
        }
    }

    // TODO
    // generate a new list of used layers
    // regenerate all splat maps taking into account unused stuff
}

float TerrainSection::SampleHeightMap(float normalizedX, float normalizedY) const
{
    // convert normalized coordinates to offsets :: TODO check the -1 here
    float offsetXF = normalizedX * (float)(m_pointCount - 1);
    float offsetYF = normalizedY * (float)(m_pointCount - 1);

    // get fractional offset
    float offsetXFrac = Math::FractionalPart(offsetXF);
    float offsetYFrac = Math::FractionalPart(offsetYF);

    // get the 4 offsets
    uint32 offsetXL = (uint32)Math::Truncate(Math::Floor(offsetXF));
    uint32 offsetXR = (uint32)offsetXL + 1;
    uint32 offsetYT = (uint32)Math::Truncate(Math::Floor(offsetYF));
    uint32 offsetYB = (uint32)offsetYT + 1;

    // get 4 heights
    float heightTL(GetHeightMapValue(offsetXL, offsetYT));
    float heightTR(GetHeightMapValue(offsetXR, offsetYT));
    float heightBL(GetHeightMapValue(offsetXL, offsetYB));
    float heightBR(GetHeightMapValue(offsetXR, offsetYB));

    // bilinear interpolate between them
    float R1(heightTL + (heightTR - heightTL) * offsetXFrac);
    float R2(heightBL + (heightBR - heightBL) * offsetXFrac);
    float height = (R1 + (R2 - R1) * offsetYFrac);
    return height;
}

float3 TerrainSection::SampleNormal(float normalizedX, float normalizedY) const
{
    // convert normalized coordinates to offsets :: TODO check the -1 here
    float offsetXF = normalizedX * (float)(m_pointCount - 1);
    float offsetYF = normalizedY * (float)(m_pointCount - 1);

    // get fractional offset
    float offsetXFrac = Math::FractionalPart(offsetXF);
    float offsetYFrac = Math::FractionalPart(offsetYF);

    // get the 4 offsets
    uint32 offsetXL = (uint32)Math::Truncate(Math::Floor(offsetXF));
    uint32 offsetXR = (uint32)offsetXL + 1;
    uint32 offsetYT = (uint32)Math::Truncate(Math::Floor(offsetYF));
    uint32 offsetYB = (uint32)offsetYT + 1;

    // get 4 normals
    float3 normalTL(CalculateNormalAtPoint(offsetXL, offsetYT));
    float3 normalTR(CalculateNormalAtPoint(offsetXR, offsetYT));
    float3 normalBL(CalculateNormalAtPoint(offsetXL, offsetYB));
    float3 normalBR(CalculateNormalAtPoint(offsetXR, offsetYB));

    // bilinear interpolate between them
    float3 R1(normalTL + (normalTR - normalTL) * offsetXFrac);
    float3 R2(normalBL + (normalBR - normalBL) * offsetXFrac);
    float3 normal(R1 + (R2 - R1) * offsetYFrac);
    return normal.Normalize();
}

int32 TerrainSection::GetDetailMeshIndex(const StaticMesh *pStaticMesh) const
{
    for (const DetailMesh &detailMesh : m_detailMeshes)
    {
        if (detailMesh.pStaticMesh == pStaticMesh)
            return (int32)detailMesh.MeshIndex;
    }

    return -1;
}

int32 TerrainSection::AddDetailMesh(const StaticMesh *pStaticMesh, float drawDistance)
{
    uint32 maxIndex = 0;
    for (const DetailMesh &detailMesh : m_detailMeshes)
    {
        if (detailMesh.pStaticMesh == pStaticMesh)
            return (int32)detailMesh.MeshIndex;

        maxIndex = Max(maxIndex, detailMesh.MeshIndex);
    }

    DetailMesh detailMesh;
    detailMesh.MeshIndex = (m_detailMeshes.IsEmpty()) ? 0 : (maxIndex + 1);
    detailMesh.pStaticMesh = pStaticMesh; pStaticMesh->AddRef();
    detailMesh.DrawDistance = drawDistance;
    m_detailMeshes.Add(detailMesh);
    return detailMesh.MeshIndex;
}

void TerrainSection::SetDetailMeshDrawDistance(const StaticMesh *pStaticMesh, float drawDistance)
{
    for (DetailMesh &detailMesh : m_detailMeshes)
    {
        if (detailMesh.pStaticMesh == pStaticMesh)
        {
            detailMesh.DrawDistance = drawDistance;
            return;
        }
    }
}

void TerrainSection::AddDetailMeshInstance(uint32 meshIndex, float normalizedX, float normalizedY, float scale, float rotationZ)
{
    DetailMeshInstance meshInstance;
    meshInstance.MeshIndex = meshIndex;
    meshInstance.NormalizedX = normalizedX;
    meshInstance.NormalizedY = normalizedY;
    meshInstance.RotationZ = rotationZ;

    float3 meshPosition;
    Quaternion meshRotation;
    GetDetailMeshLocation(normalizedX, normalizedY, rotationZ, &meshPosition, &meshRotation);

    // calculate matrix
    meshInstance.Position = meshPosition;
    meshInstance.TransformMatrix = float3x4(float4x4::MakeTranslationMatrix(meshPosition) * meshRotation.GetMatrix4x4() * float4x4::MakeScaleMatrix(scale));

    // sort the list
    m_detailMeshInstances.Add(meshInstance);
    m_detailMeshInstances.Sort([](const DetailMeshInstance *left, const DetailMeshInstance *right) -> int {
        return static_cast<int32>(left->MeshIndex) - static_cast<int32>(right->MeshIndex);
    });

    //Log_DevPrintf("mesh instance %u %.3f %.3f -> %s %s", meshIndex, normalizedX, normalizedY, StringConverter::Float3ToString(meshPosition).GetCharArray(), StringConverter::Float3ToString(meshRotation.GetEulerAngles()).GetCharArray());
}

void TerrainSection::RemoveDetailMeshInstancesBox(float normalizedXStart, float normalizedXEnd, float normalizedYStart, float normalizedYEnd)
{

}

void TerrainSection::RemoveDetailMeshInstancesCircle(float normalizedX, float normalizedY, float radius)
{

}

void TerrainSection::UpdateDetailMeshLocations(uint32 offsetX, uint32 offsetY)
{

}

void TerrainSection::GetDetailMeshLocation(float normalizedX, float normalizedY, float rotationZ, float3 *pOutPosition, Quaternion *pOutRotation)
{
    // sample height and normal at this position
    float height = SampleHeightMap(normalizedX, normalizedY);
    float3 normal(SampleNormal(normalizedX, normalizedY));

    // set position
    *pOutPosition = float3(m_bounds.GetMinBounds().xy() + float2(normalizedX * (m_pointCount - 1), normalizedY * (m_pointCount - 1)), height);

    // set rotation
    *pOutRotation = (Quaternion::FromTwoUnitVectors(float3::UnitZ, normal) * Quaternion::MakeRotationZ(rotationZ)).Normalize();
}

/*

bool TerrainSection::GetLayerIndex(uint8 layer, uint32 *pLayerIndex) const
{
    for (uint32 i = 0; i < m_nUsedLayers; i++)
    {
        if (m_usedLayers[i] == layer)
        {
            *pLayerIndex = i;
            return true;
        }
    }

    return false;
}

uint8 TerrainSection::GetLayerValue(uint32 ix, uint32 iy, uint32 layerIndex) const
{
    DebugAssert(layerIndex < m_nUsedLayers);
    return m_pLayerWeightValues[layerIndex * m_layerWeightValuesStride + iy * m_pointCount + ix];
}

void TerrainSection::SetLayerValue(uint32 ix, uint32 iy, uint32 layerIndex, uint8 value)
{
    DebugAssert(layerIndex < m_nUsedLayers);
    m_pLayerWeightValues[layerIndex * m_layerWeightValuesStride + iy * m_pointCount + ix] = value;
    m_changed = true;
}

void TerrainSection::ExtendLayerArray(uint32 newLayerCount)
{
    DebugAssert(newLayerCount > m_nUsedLayers);

    uint32 newWeightArrayCount = (newLayerCount + 3) / 4;
    DebugAssert(newWeightArrayCount > 0);

    if (newWeightArrayCount > m_nLayerWeightArrays)
    {
        uint8 *pNewLayerWeightValues = new uint8[newWeightArrayCount * m_layerWeightValuesStride];
        if (m_nLayerWeightArrays > 0)
            Y_memcpy(pNewLayerWeightValues, m_pLayerWeightValues, sizeof(uint8) * m_layerWeightValuesStride * m_nLayerWeightArrays);

        Y_memzero(pNewLayerWeightValues + m_layerWeightValuesStride * m_nLayerWeightArrays, sizeof(uint8) * m_layerWeightValuesStride * (newWeightArrayCount - m_nLayerWeightArrays));

        delete[] m_pLayerWeightValues;
        m_pLayerWeightValues = pNewLayerWeightValues;
        m_nLayerWeightArrays = newWeightArrayCount;
    }

    uint8 *pNewLayerIndices = new uint8[newLayerCount];
    if (m_nUsedLayers > 0)
        Y_memcpy(pNewLayerIndices, m_pLayerIndices, sizeof(uint8) * m_nUsedLayers);

    Y_memset(pNewLayerIndices + m_nUsedLayers, 0xFF, sizeof(uint8) * (newLayerCount - m_nUsedLayers));
    delete[] m_pLayerIndices;
    m_pLayerIndices = pNewLayerIndices;
    m_nUsedLayers = newLayerCount;

    m_changed = true;
}


uint32 TerrainSection::AddLayer(uint8 layer)
{
    uint32 layerIndex;
    DebugAssert(!GetLayerIndex(layer, &layerIndex));

    layerIndex = m_nUsedLayers++;
    m_usedLayers[layerIndex] = layer;
    m_pLayerWeightValues = (uint8 *)realloc(m_pLayerWeightValues, sizeof(uint8) * m_layerWeightValuesStride * m_nUsedLayers);
    Y_memzero(m_pLayerWeightValues + (layerIndex * m_layerWeightValuesStride), m_layerWeightValuesStride);
    m_changed = true;
    
    return layerIndex;
}

float TerrainSection::GetIndexedBaseLayerWeight(uint32 ix, uint32 iy, uint8 layer) const
{
    uint32 layerIndex;
    if (!GetLayerIndex(layer, &layerIndex))
        return 0.0f;

    return (float)GetLayerValue(ix, iy, layerIndex) / 255.0f;
}

uint32 TerrainSection::GetIndexedBaseLayers(uint32 ix, uint32 iy, uint8 *outLayers, float *outLayerWeights, uint32 maxLayers) const
{
    uint32 writtenLayers = 0;

    for (uint32 i = 0; i < m_nUsedLayers && writtenLayers < maxLayers; i++)
    {
        uint8 value = m_pLayerWeightValues[i * m_layerWeightValuesStride + iy * m_pointCount + ix];
        if (value != 0)
        {
            outLayers[writtenLayers] = m_usedLayers[i];
            outLayerWeights[writtenLayers] = (float)value / 255.0f;
            writtenLayers++;
        }
    }

    return writtenLayers;
}

uint8 TerrainSection::GetIndexedDominantBaseLayer(uint32 ix, uint32 iy) const
{
    uint8 bestLayer = 0;
    uint8 bestValue = 0;

    for (uint32 i = 0; i < m_nUsedLayers; i++)
    {
        uint8 value = m_pLayerWeightValues[i * m_layerWeightValuesStride + iy * m_pointCount + ix];
        if (value != 0 && value > bestValue)
        {
            bestLayer = m_usedLayers[i];
            bestValue = value;
        }
    }

    return bestLayer;
}

void TerrainSection::AddIndexedBaseLayerWeight(uint32 ix, uint32 iy, uint8 layer, float amount)
{
    DebugAssert(layer < TERRAIN_MAX_LAYERS);

    uint32 layerIndex;
    if (!GetLayerIndex(layer, &layerIndex))
        layerIndex = AddLayer(layer);

    uint32 changeLayerIndices[TERRAIN_MAX_LAYERS];
    float changeLayerWeights[TERRAIN_MAX_LAYERS];
    uint32 changeLayerCount = 0;

    // collect layers for this texel
    for (uint32 i = 0; i < m_nUsedLayers; i++)
    {
        if (i != layerIndex)
        {
            uint8 value = m_pLayerWeightValues[i * m_layerWeightValuesStride + iy * m_pointCount + ix];
            if (value != 0)
            {
                changeLayerIndices[changeLayerCount] = i;
                changeLayerWeights[changeLayerCount] = (float)value / 255.0f;
                changeLayerCount++;
            }
        }
    }

    // subtract the amount off the other layers
    if (changeLayerCount > 0)
    {
        float subtractAmount = amount / (float)changeLayerCount;
        for (uint32 i = 0; i < changeLayerCount; i++)
        {
            float value = changeLayerWeights[i] - subtractAmount;
            m_pLayerWeightValues[changeLayerIndices[i] * m_layerWeightValuesStride + iy * m_pointCount + ix] = (uint8)Math::Truncate(Math::Clamp(value * 255.0f, 0.0f, 255.0f));
        }
    }

    // set it on the layer
    float value = ((float)m_pLayerWeightValues[layerIndex * m_layerWeightValuesStride + iy * m_pointCount + ix] / 255.0f) + amount;
    m_pLayerWeightValues[layerIndex * m_layerWeightValuesStride + iy * m_pointCount + ix] = (uint8)Math::Truncate(Math::Clamp(value * 255.0f, 0.0f, 255.0f));
    m_changed = true;
}

void TerrainSection::SetIndexedBaseLayerWeight(uint32 ix, uint32 iy, uint8 layer, float weight)
{
    DebugAssert(layer < TERRAIN_MAX_LAYERS);

    uint32 layerIndex;
    if (!GetLayerIndex(layer, &layerIndex))
        layerIndex = AddLayer(layer);

    uint8 value = (uint8)Math::Truncate(Math::Clamp(weight * 255.0f, 0.0f, 255.0f));
    if (m_pLayerWeightValues[layerIndex * m_layerWeightValuesStride + iy * m_pointCount + ix] == value)
        return;

    // update data
    m_pLayerWeightValues[layerIndex * m_layerWeightValuesStride + iy * m_pointCount + ix] = value;
    m_changed = true;
}

void TerrainSection::SetAndWeightIndexedBaseLayerWeight(uint32 ix, uint32 iy, uint8 layer, float weight)
{
    DebugAssert(layer < TERRAIN_MAX_LAYERS && weight <= 1.0f);

    // retreive current weights
    bool hadThisLayer = (GetIndexedBaseLayerWeight(ix, iy, layer) != 0.0f);
    float currentWeights[TERRAIN_MAX_LAYERS];
    uint8 currentLayers[TERRAIN_MAX_LAYERS];
    uint32 layerCount = GetIndexedBaseLayers(ix, iy, currentLayers, currentWeights, TERRAIN_MAX_LAYERS);

    // get this layer's weight
    float currentWeight = GetIndexedBaseLayerWeight(ix, iy, layer);
    float diffWeight = currentWeight - weight;
    if ((hadThisLayer && layerCount > 1) ||
        (!hadThisLayer && layerCount > 0))
    {
        // fraction of available weight
        float diffPerLayer = diffWeight / (float)((hadThisLayer) ? (layerCount - 1) : (layerCount));
        for (uint32 i = 0; i < layerCount; i++)
        {
            if (currentLayers[i] != layer)
                SetIndexedBaseLayerWeight(ix, iy, currentLayers[i], currentWeights[i] + diffPerLayer);
        }
    }

    // set layer weight
    SetIndexedBaseLayerWeight(ix, iy, layer, weight);
}

void TerrainSection::ClearIndexedBaseLayerWeights(uint32 ix, uint32 iy)
{
    for (uint32 i = 0; i < m_nUsedLayers; i++)
        SetLayerValue(ix, iy, i, 0);
}

void TerrainSection::SetAllBaseLayerWeights(uint8 layer, float weight)
{
    DebugAssert(layer < TERRAIN_MAX_LAYERS);

    uint32 layerIndex;
    if (!GetLayerIndex(layer, &layerIndex))
        layerIndex = AddLayer(layer);

    uint8 value = (uint8)Math::Truncate(Math::Clamp(weight * 255.0f, 0.0f, 255.0f));
    for (uint32 iy = 0; iy < m_pointCount; iy++)
    {
        for (uint32 ix = 0; ix < m_pointCount; ix++)
        {
            SetLayerValue(ix, iy, layerIndex, value);
        }
    }
}

void TerrainSection::ClearAllBaseLayerWeights()
{
    Y_free(m_pLayerWeightValues);
    m_pLayerWeightValues = NULL;
    Y_memset(m_usedLayers, 0xFF, sizeof(m_usedLayers));
    m_nUsedLayers = 0;
    m_changed = true;
}
*/


