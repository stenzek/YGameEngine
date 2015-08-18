#pragma once
#include "MapCompiler/MapSource.h"
#include "Engine/TerrainTypes.h"
#include "Engine/TerrainLayerList.h"
#include "Engine/TerrainSection.h"
#include "Engine/TerrainQuadTree.h"
#include "YBaseLib/ProgressCallbacks.h"

class Camera;
class TerrainSection;
class TerrainRenderer;

class TerrainStreamingCallbacks;

// todo: shrink method

class MapSourceTerrainData
{
    friend class MapSource;

public:
    class EditCallbacks
    {
    public:
        virtual void OnSectionCreated(int32 sectionX, int32 sectionY) {}
        virtual void OnSectionDeleted(int32 sectionX, int32 sectionY) {}

        virtual void OnSectionLoaded(const TerrainSection *pSection) {}
        virtual void OnSectionUnloaded(const TerrainSection *pSection) {}

        virtual void OnSectionLayersModified(const TerrainSection *pSection) {}
        virtual void OnSectionPointHeightModified(const TerrainSection *pSection, uint32 offsetX, uint32 offsetY) {}
        virtual void OnSectionPointLayersModified(const TerrainSection *pSection, uint32 offsetX, uint32 offsetY) {}
    };

    enum HeightmapImportScaleType
    {
        HeightmapImportScaleType_None,
        HeightmapImportScaleType_Downscale,
        HeightmapImportScaleType_Upscale,
        HeightmapImportScaleType_Count,
    };
    
public:
    MapSourceTerrainData(MapSource *pMapSource);
    ~MapSourceTerrainData();

public: 
    // accessors
    const TerrainParameters *GetParameters() const { return &m_parameters; }
    const TerrainLayerListGenerator *GetLayerListGenerator() const { return m_pLayerListGenerator; }
    const TerrainLayerList *GetLayerList() const { return m_pLayerList; }

    // callbacks interface
    EditCallbacks *GetEditCallbacks() { return m_pEditCallbacks; }
    void SetEditCallbacks(EditCallbacks *pEditCallbacks) { m_pEditCallbacks = pEditCallbacks; }

    // test if changed
    bool IsChanged() const;

    // lod queries
    int32 GetMinSectionX() const { return m_minSectionX; }
    int32 GetMinSectionY() const { return m_minSectionY; }
    int32 GetMaxSectionX() const { return m_maxSectionX; }
    int32 GetMaxSectionY() const { return m_maxSectionY; }
    int32 GetSectionCount() const { return m_sectionCount; }
    int32 GetSectionCountX() const { return m_sectionCountX; }
    int32 GetSectionCountY() const { return m_sectionCountY; }

    // helper function to calculate tile bounds
    AABox CalculateSectionBoundingBox(int32 sectionX, int32 sectionY) const;

    // helper function to determine world bounds
    AABox CalculateTerrainBoundingBox() const;

    // helper function to calculate the section of the specified global indices
    int2 CalculateSectionForPoint(int32 globalX, int32 globalY) const;

    // helper function to calculate the section of a specified position
    int2 CalculateSectionForPosition(const float3 &position) const;
    
    // helper function to calculate the section and indexed position of the specified global indices
    void CalculateSectionAndOffsetForPoint(int32 *pSectionX, int32 *pSectionY, uint32 *pIndexX, uint32 *pIndexY, int32 globalX, int32 globalY) const;

    // helper function to calculate the section and index position closest to the specified position
    void CalculateSectionAndOffsetForPosition(int32 *pSectionX, int32 *pSectionY, uint32 *pIndexX, uint32 *pIndexY, const float3 &position) const;

    // helper function to calculate a global point from a position
    int2 CalculatePointForPosition(const float3 &position) const;

    // helper function for going from point to position
    float3 CalculatePositionForPoint(int32 globalX, int32 globalY) const;

    // helper function to calculate global coordinates from the specified section and offsets
    void CalculatePointForSectionAndOffset(int32 *pGlobalX, int32 *pGlobalY, int32 sectionX, int32 sectionY, uint32 offsetX, uint32 offsetY) const;

    // tile availability
    bool IsSectionAvailable(int32 sectionX, int32 sectionY) const;
    bool IsSectionLoaded(int32 sectionX, int32 sectionY) const;

    // tile accessors
    const TerrainSection *GetSection(int32 sectionX, int32 sectionY) const;
    TerrainSection *GetSection(int32 sectionX, int32 sectionY);

    // creates a blank terrain
    void Create(ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // enumerates available sections, format is Callback(int32 sectionX, int32 sectionY)
    template<class T> void EnumerateAvailableSections(T callback) const;

    // enumerates loaded sections, format is Callback(const TerrainSection *pSection)
    template<class T> void EnumerateLoadedSections(T callback) const;
    template<class T> void EnumerateLoadedSections(T callback);

    // ensures that all sections are loaded (loads all lods)
    bool LoadAllSections(ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // unloads all sections
    void UnloadAllSections(bool discardChanges = false);

    // coarse then fine iteration of sections to render in a frustum
    // callback is in format of callback(const TerrainSection *pSection)
    template<typename CALLBACK_TYPE> void EnumerateSectionsInFrustum(const Frustum &frustum, CALLBACK_TYPE callback) const;

    // callback is in format of callback(const TerrainSection *pSection)
    template<typename CALLBACK_TYPE> void EnumerateSectionsOverlappingBox(const AABox &box, CALLBACK_TYPE callback);

    // callback is in format of callback(const TerrainSection *pSection)
    template<typename CALLBACK_TYPE> void EnumerateSectionsIntersectingRay(const Ray &ray, CALLBACK_TYPE callback);

    // callback is in format of callback(const float3 vertices[4])
    template<typename CALLBACK_TYPE> void EnumerateQuadsIntersectingBox(const AABox &box, CALLBACK_TYPE callback);

    // ray casting
    bool RayCast(const Ray &ray, float3 &contactNormal, float3 &contactPoint);

    //////////////////////////////////////////////////////////////////////////
    // Editor Methods
    //////////////////////////////////////////////////////////////////////////

    // section loading/unloading
    bool LoadSection(int32 sectionX, int32 sectionY);
    void UnloadSection(int32 sectionX, int32 sectionY);

    // ensure the adjacent sections (+/- x/y) are loaded
    bool EnsureAdjacentSectionsLoaded(int32 sectionX, int32 sectionY);

    // section create/delete
    // if the index is negative on either axis, the terrain will be moved to fit in the new section
    uint32 CreateSections(const int2 *pNewSectionIndices, uint32 newSectionCount, float createHeight, uint8 createLayer, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    void DeleteSections(const int2 *pSectionIndices, uint32 sectionCount, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // single versions of above
    bool CreateSection(int32 sectionX, int32 sectionY, float createHeight, uint8 createLayer, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    void DeleteSection(int32 sectionX, int32 sectionY, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    void DeleteAllSections();

    // changing heights/weights
    float GetPointHeight(int32 pointX, int32 pointY);
    float GetPointLayerWeight(int32 pointX, int32 pointY, uint8 layer);
    bool SetPointHeight(int32 pointX, int32 pointY, float height);
    bool AddPointHeight(int32 pointX, int32 pointY, float mod);
    bool SetPointLayerWeight(int32 pointX, int32 pointY, uint8 layer, float weight, bool renormalize = true);
    bool AddPointLayerWeight(int32 pointX, int32 pointY, uint8 layer, float amount, bool renormalize = true);

    // import a heightmap
    bool ImportHeightmap(const Image *pHeightmap, int32 startSectionX, int32 startSectionY, float minHeight, float maxHeight, HeightmapImportScaleType scaleType, uint32 scaleAmount, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // rebuilds quadtree on all section
    bool RebuildQuadTree(uint32 newLODCount, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // helpers
    static void MakeTerrainStorageFileName(String &str, int32 sectionX, int32 sectionY); 
    static int32 GetSectionArrayIndex(int32 sectionX, int32 sectionY, int32 minSectionX, int32 minSectionY, int32 maxSectionX, int32 maxSectionY);
    int32 GetSectionArrayIndex(int32 sectionX, int32 sectionY) const;

private:
    // compile the layer list
    TerrainLayerList *CompileLayerList(const TerrainLayerListGenerator *pGenerator, BinaryBlob **ppStoreBlob);

    // create a new terrain
    bool Create(const TerrainParameters *pParameters, const TerrainLayerListGenerator *pLayerListGenerator, ProgressCallbacks *pProgressCallbacks);

    // load an existing terrain
    bool Load(ProgressCallbacks *pProgressCallbacks);

    // save it
    bool Save(ProgressCallbacks *pProgressCallbacks);

    // delete it
    void Delete();

private:
    // rebuild section array
    void ResizeSectionArray(int32 newMinSectionX, int32 newMinSectionY, int32 newMaxSectionX, int32 newMaxSectionY);

    // find the default layer index
    uint8 GetDefaultLayer() const;

    // copy weights from one section's point to another
    void CopySectionPointWeights(const TerrainSection *pSourceSection, uint32 sourceOffsetX, uint32 sourceOffsetY, TerrainSection *pDestinationSection, uint32 destinationOffsetX, uint32 destinationOffsetY);

    // handle edge point updates
    void HandleEdgePointsHeightUpdate(TerrainSection *pSection, uint32 offsetX, uint32 offsetY);
    void HandleEdgePointsWeightUpdate(TerrainSection *pSection, uint32 offsetX, uint32 offsetY);

    // normalize a single point's layer weights
    bool NormalizePointLayerWeights(TerrainSection *pSection, uint32 offsetX, uint32 offsetY);

    // normalize a section's layer weights
    bool NormalizeSectionLayerWeights(TerrainSection *pSection);

    // remove low weighted points from a single point
    bool RemovePointLayerWeights(TerrainSection *pSection, uint32 offsetX, uint32 offsetY, float threshold = 0.1f, bool normalizeAfterRemove = true);
    
    // remove low weighted points from a section
    bool RemoveSectionLayerWeights(TerrainSection *pSection, float threshold = 0.1f, bool normalizeAfterRemove = true);

    // map we are associated with
    MapSource *m_pMapSource;

    // parameters
    TerrainParameters m_parameters;

    // layer list
    const TerrainLayerListGenerator *m_pLayerListGenerator;
    const TerrainLayerList *m_pLayerList;
    bool m_layerListChanged;

    // section storage
    int32 m_minSectionX;
    int32 m_minSectionY;
    int32 m_maxSectionX;
    int32 m_maxSectionY;
    int32 m_sectionCountX;
    int32 m_sectionCountY;
    int32 m_sectionCount;
    TerrainSection **m_ppSections;
    BitSet32 m_availableSectionMask;
    bool m_availableSectionsChanged;

    // for faster iteration when unloading/streaming, we store loaded sections here
    PODArray<TerrainSection *> m_loadedSections;

    // track deleted sections, that way upon save the storage can be nuked for them
    typedef MemArray<int2> DeletedSectionArray;
    DeletedSectionArray m_deletedSections;

    // edit callbacks
    EditCallbacks *m_pEditCallbacks;
};

// enumeration
template<class T>
void MapSourceTerrainData::EnumerateAvailableSections(T callback) const
{
    uint32 sectionIndex = 0;
    for (int32 y = m_minSectionY; y <= m_maxSectionY; y++)
    {
        for (int32 x = m_minSectionX; x <= m_maxSectionX; x++)
        {
            if (m_availableSectionMask[sectionIndex++])
                callback(x, y);
        }
    }
}

template<class T>
void MapSourceTerrainData::EnumerateLoadedSections(T callback) const
{
    for (int32 i = 0; i < m_sectionCount; i++)
    {
        const TerrainSection *pSection = m_ppSections[i];
        if (pSection != NULL)
            callback(pSection);
    }
}

template<class T>
void MapSourceTerrainData::EnumerateLoadedSections(T callback)
{
    for (int32 i = 0; i < m_sectionCount; i++)
    {
        TerrainSection *pSection = m_ppSections[i];
        if (pSection != NULL)
            callback(pSection);
    }
}

template<typename CALLBACK_TYPE>
void MapSourceTerrainData::EnumerateSectionsInFrustum(const Frustum &frustum, CALLBACK_TYPE callback) const
{
    // get a box containing the frustum
    // this will be much wider than necessary at the camera origin, but provides an estimation for a starting point
    AABox frustumBoundingBox(frustum.GetBoundingAABox());

    // find the sections this box encompasses
    int2 startSection(CalculateSectionForPosition(frustumBoundingBox.GetMinBounds()));
    int2 endSection(CalculateSectionForPosition(frustumBoundingBox.GetMaxBounds()));

    // clamp to terrain bounds
    int32 startSectionX = Math::Clamp(startSection.x, m_minSectionX, m_maxSectionX);
    int32 startSectionY = Math::Clamp(startSection.y, m_minSectionY, m_maxSectionY);
    int32 endSectionX = Math::Clamp(endSection.x, m_minSectionX, m_maxSectionX);
    int32 endSectionY = Math::Clamp(endSection.y, m_minSectionY, m_maxSectionY);

    // iterate over these sections
    for (int32 sectionY = startSectionY; sectionY <= endSectionY; sectionY++)
    {
        for (int32 sectionX = startSectionX; sectionX <= endSectionX; sectionX++)
        {
            // loaded?
            const TerrainSection *pSection = GetSection(sectionX, sectionY);
            if (pSection != NULL)
            {
                // do finer bounds test
                if (frustum.AABoxIntersection(pSection->GetBoundingBox()))
                    callback(pSection);
            }
        }
    }
}

template<typename CALLBACK_TYPE>
void MapSourceTerrainData::EnumerateSectionsOverlappingBox(const AABox &box, CALLBACK_TYPE callback)
{
    // get the min/max regions
    int2 searchSectionMin(CalculateSectionForPosition(box.GetMinBounds()));
    int2 searchSectionMax(CalculateSectionForPosition(box.GetMaxBounds()));

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

            callback(pSection);
        }
    }
}

template<typename CALLBACK_TYPE>
void MapSourceTerrainData::EnumerateSectionsIntersectingRay(const Ray &ray, CALLBACK_TYPE callback)
{
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

            // test if it intersects the top level node of the quadtree, since this has the correct heights
            if (!ray.AABoxIntersection(pSection->GetQuadTree()->GetTopLevelNode()->GetBoundingBox()))
                continue;

            callback(pSection);
        }
    }
}

template<typename CALLBACK_TYPE>
void MapSourceTerrainData::EnumerateQuadsIntersectingBox(const AABox &box, CALLBACK_TYPE callback)
{
    // find the sections this overlaps
    EnumerateSectionsOverlappingBox(box, [this, box, callback](const TerrainSection *pSection)
    {
        uint32 unitsPerPoint = m_parameters.Scale;
        float fUnitsPerPoint = (float)unitsPerPoint;
        int32 sectionSizeMinusOne = (int32)m_parameters.SectionSize - 1;

        // get region min bounds
        float3 sectionMinBounds(pSection->GetBoundingBox().GetMinBounds());

        // find the overlapping points
        float3 regionMinPoints((box.GetMinBounds() - sectionMinBounds) / fUnitsPerPoint);
        float3 regionMaxPoints((box.GetMaxBounds() - sectionMinBounds) / fUnitsPerPoint);

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

        // iterate over points
        float3 triangleVertices[4];
        for (uint32 ly = startY; ly <= endY; ly++)
        {
            for (uint32 lx = startX; lx <= endX; lx++)
            {
                // first triangle
                triangleVertices[0].Set(sectionMinBounds.x + (float)((lx) * unitsPerPoint),
                                        sectionMinBounds.y + (float)((ly + 1) * unitsPerPoint),
                                        pSection->GetHeightMapValue(lx, ly + 1));

                triangleVertices[1].Set(sectionMinBounds.x + (float)((lx + 1) * unitsPerPoint),
                                        sectionMinBounds.y + (float)((ly + 1) * unitsPerPoint),
                                        pSection->GetHeightMapValue(lx + 1, ly + 1));

                triangleVertices[2].Set(sectionMinBounds.x + (float)((lx) * unitsPerPoint),
                                        sectionMinBounds.y + (float)((ly) * unitsPerPoint),
                                        pSection->GetHeightMapValue(lx, ly));

                triangleVertices[3].Set(sectionMinBounds.x + (float)((lx + 1) * unitsPerPoint),
                                        sectionMinBounds.y + (float)((ly) * unitsPerPoint),
                                        pSection->GetHeightMapValue(lx + 1, ly));

                callback(triangleVertices);
                /*
                // second triangle
                triangleVertices[1][0] = triangleVertices[0][2];
                triangleVertices[1][1] = triangleVertices[0][1];
                triangleVertices[1][2].Set(sectionMinBounds.x + (float)((lx + 1) * unitsPerPoint),
                                           sectionMinBounds.y + (float)((ly) * unitsPerPoint),
                                           pSection->GetHeightMapValue(lx + 1, ly));

                // invoke callback
                callback(triangleVertices[0]);
                callback(triangleVertices[1]);
                */

            }
        }
    });
}
