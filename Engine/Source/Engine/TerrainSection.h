#pragma once
#include "Engine/Common.h"
#include "Engine/TerrainTypes.h"
#include "Engine/TerrainQuadTree.h"

class GPUTexture2D;
class TerrainLayerList;
class TerrainManager;
class StaticMesh;

class TerrainSection : public ReferenceCounted
{
    friend class TerrainManager;

public:
    struct DetailMesh
    {
        uint32 MeshIndex;
        const StaticMesh *pStaticMesh;
        float DrawDistance;
    };
    struct DetailMeshInstance
    {
        // stored
        uint32 MeshIndex;
        float NormalizedX;
        float NormalizedY;
        float Scale;
        float RotationZ;

        // calculated
        float3 Position;
        float3x4 TransformMatrix;
    };

public:
    TerrainSection(const TerrainParameters *pParameters, int32 sectionX, int32 sectionY, uint32 LODLevel);
    ~TerrainSection();

    const TERRAIN_HEIGHT_STORAGE_FORMAT GetHeightStorageFormat() const { return m_parameters.HeightStorageFormat; }

    const int32 GetSectionX() const { return m_sectionX; }
    const int32 GetSectionY() const { return m_sectionY; }
    const uint32 GetStorageLODLevel() const { return m_LODLevel; }
    const uint32 GetPointCount() const { return m_pointCount; }
    const AABox &GetBoundingBox() const { return m_bounds; }
    const bool IsChanged() const { return m_changed; }
    void SetChanged() { m_changed = true; }
    void ClearChangedFlag() { m_changed = false; }

    void Create(float createHeight, uint8 createLayer);
    bool LoadFromStream(ByteStream *pStream);
    bool SaveToStream(ByteStream *pStream) const;

    // raw data for deploying straight to texture
    void GetRawHeightMapData(const void **pDataPointer, uint32 *pValueSize, uint32 *pRowPitch) const;
    void GetRawSplatMapData(uint32 mapIndex, const void **pDataPointer, uint32 *pValueSize, uint32 *pRowPitch) const;

    // quadtree
    const TerrainSectionQuadTree *GetQuadTree() const { return m_pQuadTree; }
    TerrainSectionQuadTree *GetQuadTree() { return m_pQuadTree; }

    // raycast into the section
    bool RayCast(const Ray &ray, float3 &contactNormal, float3 &contactPoint, bool exitAtFirstIntersection = false) const;

    // callback is in format of callback(const float3 vertices[4])
    template<typename CALLBACK_TYPE> void EnumerateQuadsIntersectingBox(const AABox &box, CALLBACK_TYPE callback) const;

    // callback is in format of callback(const float3 vertices[3])
    template<typename CALLBACK_TYPE> void EnumerateTrianglesIntersectingBox(const AABox &box, CALLBACK_TYPE callback) const;

    // heightmap access
    const float GetHeightMapValue(uint32 offsetX, uint32 offsetY) const;
    void SetHeightMapValue(uint32 offsetX, uint32 offsetY, float height);
    void SetAllHeightMapValues(float height);

    // normal calculation
    float3 CalculateNormalAtPoint(uint32 x, uint32 y) const;

    // sample heightmap
    float SampleHeightMap(float normalizedX, float normalizedY) const;
    float3 SampleNormal(float normalizedX, float normalizedY) const;

    // splatmap channel information access
    const uint32 GetSplatMapCount() const { return m_nSplatMaps; }
    const uint32 GetSplatMapChannelCount(uint32 mapIndex) const { DebugAssert(mapIndex < m_nSplatMaps); return m_pSplatMaps[mapIndex].LayerCount; }
    const uint8 GetSplatMapChannel(uint32 mapIndex, uint32 channelIndex) const { DebugAssert(mapIndex < m_nSplatMaps && channelIndex < m_pSplatMaps[mapIndex].LayerCount); return m_pSplatMaps[mapIndex].Layers[channelIndex]; }
    const bool HasSplatMapForLayer(uint8 layerIndex) const;
    const uint8 GetSplatMapDominantLayer(uint32 offsetX, uint32 offsetY) const;
    const uint32 GetSplatMapValues(uint32 offsetX, uint32 offsetY, uint8 *outLayers, float *outLayerWeights, uint32 maxLayers) const;

    // splatmap data access, setting a value will automatically re-weight/normalize the weights
    const float GetSplatMapValue(uint32 offsetX, uint32 offsetY, uint8 layer) const;
    void SetSplatMapValue(uint32 offsetX, uint32 offsetY, uint8 layer, float value, bool renormalize = true);
    void ClearSplatMapValues(uint32 offsetX, uint32 offsetY);
    void SetAllSplatMapValues(uint8 layer, float value = 1.0f);

    // copy weights from one section's point to another
    void CopySplatMapValue(const TerrainSection *pSourceSection, uint32 sourceOffsetX, uint32 sourceOffsetY, uint32 destinationOffsetX, uint32 destinationOffsetY);

    // normalize a single point's weights
    void NormalizeSplatMapValue(uint32 offsetX, uint32 offsetY);

    // remove low weighted points from a single point
    void FilterSplatMapValues(uint32 offsetX, uint32 offsetY, float threshold = 0.1f, bool normalizeAfterRemove = true);

    // detail mesh manipulation
    int32 GetDetailMeshIndex(const StaticMesh *pStaticMesh) const;
    int32 AddDetailMesh(const StaticMesh *pStaticMesh, float drawDistance);
    void SetDetailMeshDrawDistance(const StaticMesh *pStaticMesh, float drawDistance);

    // detail mesh accessors
    const DetailMesh *GetDetailMesh(uint32 index) const { return &m_detailMeshes[index]; }
    const uint32 GetDetailMeshCount() const { return m_detailMeshes.GetSize(); }
    const DetailMeshInstance *GetDetailMeshInstance(uint32 index) const { return &m_detailMeshInstances[index]; }
    const uint32 GetDetailMeshInstanceCount() const { return m_detailMeshInstances.GetSize(); }

    // detail mesh instance manipulation
    void AddDetailMeshInstance(uint32 meshIndex, float normalizedX, float normalizedY, float scale, float rotationZ);
    void RemoveDetailMeshInstancesBox(float normalizedXStart, float normalizedXEnd, float normalizedYStart, float normalizedYEnd);
    void RemoveDetailMeshInstancesCircle(float normalizedX, float normalizedY, float radius);

    // rebuilding
    bool RebuildQuadTree(ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    void RebuildSplatMaps(bool filterValues = true, float filterThreshold = 0.1f);

private:
    // splatmap manipulators
    void AllocateLayerInSplatMap(uint8 layerIndex, uint32 *pMapIndex, uint32 *pChannelIndex);
    bool GetSplatMapLocation(uint8 layerIndex, uint32 *pMapIndex, uint32 *pChannelIndex) const;
    uint8 GetSplatMapValue(uint32 mapIndex, uint32 channelIndex, uint32 offsetX, uint32 offsetY) const;
    void SetSplatMapValue(uint32 mapIndex, uint32 channelIndex, uint32 offsetX, uint32 offsetY, uint8 value);

    // detail mesh helpers
    void UpdateDetailMeshLocations(uint32 offsetX, uint32 offsetY);
    void GetDetailMeshLocation(float normalizedX, float normalizedY, float rotationZ, float3 *pOutPosition, Quaternion *pOutRotation);

    // vars
    TerrainParameters m_parameters;
    int32 m_sectionX;
    int32 m_sectionY;
    uint32 m_LODLevel;

    // includes the neighbouring row/column
    uint32 m_pointCount;

    AABox m_bounds;
    bool m_changed;

    // height map
    byte *m_pHeightValues;
    uint32 m_heightValueSize;
    uint32 m_heightMapRowPitch;

    // splat map
    struct SplatMap
    {
        uint8 Layers[4];
        uint32 LayerCount;
        uint32 ChannelCount;
        uint32 RowPitch;
        uint8 *pData;
    };
    SplatMap *m_pSplatMaps;
    uint32 m_nSplatMaps;

    // details
    MemArray<DetailMesh> m_detailMeshes;
    MemArray<DetailMeshInstance> m_detailMeshInstances;

    // quadtree
    TerrainSectionQuadTree *m_pQuadTree;
};

template<typename CALLBACK_TYPE>
void TerrainSection::EnumerateQuadsIntersectingBox(const AABox &box, CALLBACK_TYPE callback) const
{
    int32 unitsPerPoint = (int32)m_parameters.Scale;
    float fUnitsPerPoint = (float)m_parameters.Scale;
    int32 sectionSizeMinusOne = (int32)m_parameters.SectionSize - 1;

    // find the overlapping points
    float3 sectionMinBounds(m_bounds.GetMinBounds());
    float3 sectionMaxBounds(m_bounds.GetMaxBounds());
    float3 sectionMinPoints((box.GetMinBounds() - sectionMinBounds) / fUnitsPerPoint);
    float3 sectionMaxPoints((box.GetMaxBounds() - sectionMinBounds) / fUnitsPerPoint);

    // quantize them 
    int32 startXi = Math::Truncate(Math::Floor(sectionMinPoints.x));
    int32 startYi = Math::Truncate(Math::Floor(sectionMinPoints.y));
    int32 endXi = Math::Truncate(Math::Ceil(sectionMaxPoints.x));
    int32 endYi = Math::Truncate(Math::Ceil(sectionMaxPoints.y));

    // fix up range
    uint32 startX = (uint32)Min(Max(startXi, (int32)0), sectionSizeMinusOne);
    uint32 startY = (uint32)Min(Max(startYi, (int32)0), sectionSizeMinusOne);
    uint32 endX = (uint32)Min(Max(endXi, (int32)0), sectionSizeMinusOne);
    uint32 endY = (uint32)Min(Max(endYi, (int32)0), sectionSizeMinusOne);

    // iterate over points
    float3 triangleVertices[4];
    for (uint32 ly = startY; ly <= endY; ly++)
    {
        for (uint32 lx = startX; lx <= endX; lx++)
        {
            triangleVertices[0].Set(sectionMinBounds.x + (float)((lx)* unitsPerPoint),
                                    sectionMinBounds.y + (float)((ly + 1) * unitsPerPoint),
                                    GetHeightMapValue(lx, ly + 1));

            triangleVertices[1].Set(sectionMinBounds.x + (float)((lx) * unitsPerPoint),
                                    sectionMinBounds.y + (float)((ly) * unitsPerPoint),
                                    GetHeightMapValue(lx, ly));

            triangleVertices[2].Set(sectionMinBounds.x + (float)((lx + 1) * unitsPerPoint),
                                    sectionMinBounds.y + (float)((ly + 1) * unitsPerPoint),
                                    GetHeightMapValue(lx + 1, ly + 1));

            triangleVertices[3].Set(sectionMinBounds.x + (float)((lx + 1) * unitsPerPoint),
                                    sectionMinBounds.y + (float)((ly) * unitsPerPoint),
                                    GetHeightMapValue(lx + 1, ly));

            // run callback
            callback(triangleVertices);
        }
    }
}

template<typename CALLBACK_TYPE>
void TerrainSection::EnumerateTrianglesIntersectingBox(const AABox &box, CALLBACK_TYPE callback) const
{
    EnumerateQuadsIntersectingBox(box, [box, &callback](const float3 quadVertices[4])
    {
        // recreate the array with a triangle
        float3 triangleVertices[3];
        triangleVertices[0] = quadVertices[0];
        triangleVertices[1] = quadVertices[1];
        triangleVertices[2] = quadVertices[2];
        callback(triangleVertices);

        // and the second triangle with flipped first two vertices
        triangleVertices[0] = quadVertices[2];
        triangleVertices[1] = quadVertices[1];
        triangleVertices[2] = quadVertices[3];
        callback(triangleVertices);
    });
}
