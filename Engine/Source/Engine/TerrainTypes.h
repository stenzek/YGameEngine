#pragma once
#include "Engine/Common.h"

enum TERRAIN_RENDERER_TYPE
{
    TERRAIN_RENDERER_TYPE_NULL,
    TERRAIN_RENDERER_TYPE_CDLOD,
    TERRAIN_RENDERER_TYPE_COUNT,
};

enum TERRAIN_HEIGHT_STORAGE_FORMAT
{
    TERRAIN_HEIGHT_STORAGE_FORMAT_UINT8,
    TERRAIN_HEIGHT_STORAGE_FORMAT_UINT16,
    TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32,
    TERRAIN_HEIGHT_STORAGE_FORMAT_COUNT,
};

namespace NameTables {
    Y_Declare_NameTable(TerrainRendererType);
    Y_Declare_NameTable(TerrainHeightStorageFormat);
}

#define TERRAIN_MAX_LAYERS (64)
#define TERRAIN_MAX_STORAGE_LODS (4)
#define TERRAIN_MAX_RENDER_LODS (10)

struct TerrainParameters
{
    TerrainParameters() {}
    TerrainParameters(TERRAIN_HEIGHT_STORAGE_FORMAT heightStorageFormat, int32 minHeight, int32 maxHeight, int32 baseHeight, uint32 scale, uint32 sectionSize, uint32 lodCount) :
        HeightStorageFormat(heightStorageFormat), MinHeight(minHeight), MaxHeight(maxHeight), BaseHeight(baseHeight), Scale(scale), SectionSize(sectionSize), LODCount(lodCount) {}

    // Height storage format
    TERRAIN_HEIGHT_STORAGE_FORMAT HeightStorageFormat;

    // Required for integer height formats, minimum/maximum heights
    int32 MinHeight;
    int32 MaxHeight;

    // Default [base] height
    int32 BaseHeight;

    // Scale of the terrain, i.e. number of world units (meters) per heightmap point
    uint32 Scale;

    // Size of each terrain page in quads
    uint32 SectionSize;

    // Number of levels of details at the highest storage level that is possible to render
    // Each storage level will decrement this number by one when that level is loaded
    uint32 LODCount;
};

namespace TerrainUtilities
{
    // validate terrain parameters
    bool IsValidParameters(const TerrainParameters *pParameters);

    // get the size of each point's height data
    uint32 GetHeightElementStorageSize(TERRAIN_HEIGHT_STORAGE_FORMAT heightStorageFormat);

    // get the corresponding pixel format for a height map storage format
    PIXEL_FORMAT GetHeightStoragePixelFormat(TERRAIN_HEIGHT_STORAGE_FORMAT heightStorageFormat);

    // helper function to calculate tile bounds
    AABox CalculateSectionBoundingBox(const TerrainParameters *pTerrainParameters, int32 sectionX, int32 sectionY);

    // helper function to calculate the section of the specified global indices
    int2 CalculateSectionForPoint(const TerrainParameters *pTerrainParameters, int32 globalX, int32 globalY);

    // helper function to calculate the section of a specified position
    int2 CalculateSectionForPosition(const TerrainParameters *pTerrainParameters, const float3 &position);

    // helper function to calculate the section and indexed position of the specified global indices
    void CalculateSectionAndOffsetForPoint(const TerrainParameters *pTerrainParameters, int32 *pSectionX, int32 *pSectionY, uint32 *pIndexX, uint32 *pIndexY, int32 globalX, int32 globalY);

    // helper function to calculate the section and index position closest to the specified position
    void CalculateSectionAndOffsetForPosition(const TerrainParameters *pTerrainParameters, int32 *pSectionX, int32 *pSectionY, uint32 *pIndexX, uint32 *pIndexY, const float3 &position);

    // helper function to calculate a global point from a position
    int2 CalculatePointForPosition(const TerrainParameters *pTerrainParameters, const float3 &position);

    // helper function for going from point to position
    float3 CalculatePositionForPoint(const TerrainParameters *pTerrainParameters, int32 globalX, int32 globalY);

    // helper function to calculate global coordinates from the specified section and offsets
    void CalculatePointForSectionAndOffset(const TerrainParameters *pTerrainParameters, int32 *pGlobalX, int32 *pGlobalY, int32 sectionX, int32 sectionY, uint32 offsetX, uint32 offsetY);
}

