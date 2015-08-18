#include "Engine/PrecompiledHeader.h"
#include "Engine/TerrainTypes.h"

Y_Define_NameTable(NameTables::TerrainRendererType)
    Y_NameTable_VEntry(TERRAIN_RENDERER_TYPE_NULL, "null")
    Y_NameTable_VEntry(TERRAIN_RENDERER_TYPE_CDLOD, "cdlod")
Y_NameTable_End()

Y_Define_NameTable(NameTables::TerrainHeightStorageFormat)
    Y_NameTable_VEntry(TERRAIN_HEIGHT_STORAGE_FORMAT_UINT8, "uint8")
    Y_NameTable_VEntry(TERRAIN_HEIGHT_STORAGE_FORMAT_UINT16, "uint16")
    Y_NameTable_VEntry(TERRAIN_HEIGHT_STORAGE_FORMAT_FLOAT32, "float32")
Y_NameTable_End()

bool TerrainUtilities::IsValidParameters(const TerrainParameters *pParameters)
{
    if (pParameters->MinHeight > pParameters->MaxHeight)
        return false;

    if (pParameters->BaseHeight > pParameters->MaxHeight)
        return false;

    if (pParameters->Scale < 1)
        return false;

    if (pParameters->SectionSize == 0 || !Math::IsPowerOfTwo(pParameters->SectionSize))
        return false;

    if (pParameters->LODCount == 0 || ((pParameters->SectionSize >> (pParameters->LODCount - 1)) == 0))
        return false;

    return true;
}

uint32 TerrainUtilities::GetHeightElementStorageSize(TERRAIN_HEIGHT_STORAGE_FORMAT heightStorageFormat)
{
    static const uint32 s_heightStorageFormatElementSizes[TERRAIN_HEIGHT_STORAGE_FORMAT_COUNT] =
    {
        1,  // uint8
        2,  // uint16
        4,  // float32
    };

    DebugAssert(heightStorageFormat < countof(s_heightStorageFormatElementSizes));
    return s_heightStorageFormatElementSizes[heightStorageFormat];
}

PIXEL_FORMAT TerrainUtilities::GetHeightStoragePixelFormat(TERRAIN_HEIGHT_STORAGE_FORMAT heightStorageFormat)
{
    static const PIXEL_FORMAT s_heightStorageFormatPixelFormats[TERRAIN_HEIGHT_STORAGE_FORMAT_COUNT] =
    {
        PIXEL_FORMAT_R8_UNORM,      // uint8
        PIXEL_FORMAT_R16_SNORM,     // uint16
        PIXEL_FORMAT_R32_FLOAT,     // float32
    };

    DebugAssert(heightStorageFormat < countof(s_heightStorageFormatPixelFormats));
    return s_heightStorageFormatPixelFormats[heightStorageFormat];
}

AABox TerrainUtilities::CalculateSectionBoundingBox(const TerrainParameters *pTerrainParameters, int32 sectionX, int32 sectionY)
{
    int32 scale = (int32)pTerrainParameters->Scale;
    int32 sectionSize = (int32)pTerrainParameters->SectionSize;

    int32 minX = (sectionX * sectionSize) * scale;
    int32 maxX = ((sectionX + 1) * sectionSize) * scale;
    int32 minY = (sectionY * sectionSize) * scale;
    int32 maxY = ((sectionY + 1) * sectionSize) * scale;

    return AABox((float)minX, (float)minY, (float)pTerrainParameters->MinHeight, (float)maxX, (float)maxY, (float)pTerrainParameters->MaxHeight);
}

int2 TerrainUtilities::CalculateSectionForPoint(const TerrainParameters *pTerrainParameters, int32 globalX, int32 globalY)
{
    int32 sectionX = (globalX >= 0) ? (globalX / (int32)pTerrainParameters->SectionSize) : (((globalX + 1) / (int32)pTerrainParameters->SectionSize) - 1);
    int32 sectionY = (globalY >= 0) ? (globalY / (int32)pTerrainParameters->SectionSize) : (((globalY + 1) / (int32)pTerrainParameters->SectionSize) - 1);

    return int2(sectionX, sectionY);
}

int2 TerrainUtilities::CalculateSectionForPosition(const TerrainParameters *pTerrainParameters, const float3 &position)
{
    float px = position.x / (float)pTerrainParameters->Scale;
    float py = position.y / (float)pTerrainParameters->Scale;
    int32 gx = Math::Truncate(px);
    int32 gy = Math::Truncate(py);

    return CalculateSectionForPoint(pTerrainParameters, gx, gy);
}

void TerrainUtilities::CalculateSectionAndOffsetForPoint(const TerrainParameters *pTerrainParameters, int32 *pSectionX, int32 *pSectionY, uint32 *pIndexX, uint32 *pIndexY, int32 globalX, int32 globalY)
{
    int32 sectionSize = (int32)pTerrainParameters->SectionSize;

    if (globalX >= 0)
    {
        *pSectionX = globalX / sectionSize;
        *pIndexX = (uint32)(globalX % sectionSize);
    }
    else
    {
        *pSectionX = ((globalX + 1) / sectionSize) - 1;
        *pIndexX = (sectionSize - 1) - (-(globalX + 1) % sectionSize);
    }

    if (globalY >= 0)
    {
        *pSectionY = globalY / sectionSize;
        *pIndexY = (uint32)(globalY % sectionSize);
    }
    else
    {
        *pSectionY = ((globalY + 1) / sectionSize) - 1;
        *pIndexY = (sectionSize - 1) - (-(globalY + 1) % sectionSize);
    }
}

void TerrainUtilities::CalculateSectionAndOffsetForPosition(const TerrainParameters *pTerrainParameters, int32 *pSectionX, int32 *pSectionY, uint32 *pIndexX, uint32 *pIndexY, const float3 &position)
{
    // round instead of truncate here
    float px = position.x / (float)pTerrainParameters->Scale;
    float py = position.y / (float)pTerrainParameters->Scale;
    int32 gx = Math::Truncate(Math::Round(px));
    int32 gy = Math::Truncate(Math::Round(py));

    CalculateSectionAndOffsetForPoint(pTerrainParameters, pSectionX, pSectionY, pIndexX, pIndexY, gx, gy);
}

int2 TerrainUtilities::CalculatePointForPosition(const TerrainParameters *pTerrainParameters, const float3 &position)
{
    // round instead of truncate here
    float px = position.x / (float)pTerrainParameters->Scale;
    float py = position.y / (float)pTerrainParameters->Scale;
    int32 gx = Math::Truncate(Math::Round(px));
    int32 gy = Math::Truncate(Math::Round(py));

    return int2(gx, gy);
}

float3 TerrainUtilities::CalculatePositionForPoint(const TerrainParameters *pTerrainParameters, int32 globalX, int32 globalY)
{
    int32 sectionX, sectionY;
    uint32 offsetX, offsetY;
    CalculateSectionAndOffsetForPoint(pTerrainParameters, &sectionX, &sectionY, &offsetX, &offsetY, globalX, globalY);

    return float3((float)(globalX * (int32)pTerrainParameters->Scale), (float)(globalY * (int32)pTerrainParameters->Scale), (float)pTerrainParameters->BaseHeight);
}

void TerrainUtilities::CalculatePointForSectionAndOffset(const TerrainParameters *pTerrainParameters, int32 *pGlobalX, int32 *pGlobalY, int32 sectionX, int32 sectionY, uint32 offsetX, uint32 offsetY)
{
    int32 sectionSize = (int32)pTerrainParameters->SectionSize;

    if (sectionX >= 0)
        *pGlobalX = (sectionX * sectionSize) + (int32)offsetX;
    else
        *pGlobalX = (sectionX * sectionSize) + (sectionSize - (int32)offsetX);

    if (sectionY >= 0)
        *pGlobalY = (sectionY * sectionSize) + (int32)offsetY;
    else
        *pGlobalY = (sectionY * sectionSize) + (sectionSize - (int32)offsetY);
}
