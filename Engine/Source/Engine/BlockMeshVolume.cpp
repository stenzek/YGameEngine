#include "Engine/PrecompiledHeader.h"
#include "Engine/BlockMeshVolume.h"

BlockMeshVolume::BlockMeshVolume()
    : m_pPalette(NULL),
      m_scale(1.0f),
      m_minCoordinates(int3::Zero),
      m_maxCoordinates(int3::Zero),
      m_width(0),
      m_length(0),
      m_height(0),
      m_pData(NULL)
{

}

BlockMeshVolume::BlockMeshVolume(const BlockPalette *pPalette, float scale, const int3 &minCoordinates, const int3 &maxCoordinates)
{
    DebugAssert(scale > 0.0f);
    DebugAssert(!minCoordinates.AnyGreater(maxCoordinates));

    int3 range(maxCoordinates - minCoordinates);
    DebugAssert(!range.AnyLess(int3::Zero));

    if ((m_pPalette = pPalette) != NULL)
        m_pPalette->AddRef();
    
    m_scale = scale;
    m_minCoordinates = minCoordinates;
    m_maxCoordinates = maxCoordinates;
    m_width = (uint32)range.x + 1;
    m_length = (uint32)range.y + 1;
    m_height = (uint32)range.z + 1;

    uint32 nBlocks = m_width * m_length * m_height;
    m_pData = new BlockVolumeBlockType[nBlocks];
    Y_memzero(m_pData, sizeof(BlockVolumeBlockType) * nBlocks);
}

BlockMeshVolume::BlockMeshVolume(const BlockMeshVolume &copy)
    : m_scale(copy.m_scale),
      m_minCoordinates(copy.m_minCoordinates),
      m_maxCoordinates(copy.m_maxCoordinates),
      m_width(copy.m_width),
      m_length(copy.m_length),
      m_height(copy.m_height)
{
    uint32 nBlocks = m_width * m_length * m_height;
    m_pData = new BlockVolumeBlockType[nBlocks];
    Y_memcpy(m_pData, copy.m_pData, sizeof(BlockVolumeBlockType) * nBlocks);
}

BlockMeshVolume::~BlockMeshVolume()
{
    delete[] m_pData;
}

AABox BlockMeshVolume::CalculateBoundingBox() const
{
    float3 minBounds(float3((float)m_minCoordinates.x, (float)m_minCoordinates.y, (float)m_minCoordinates.z) * m_scale);
    float3 maxBounds((float3((float)m_maxCoordinates.x, (float)m_maxCoordinates.y, (float)m_maxCoordinates.z) + 1.0f) * m_scale);

    return AABox(minBounds, maxBounds);
}

AABox BlockMeshVolume::CalculateActiveBoundingBox() const
{
    int3 minActiveCoordinates;
    int3 maxActiveCoordinates;

    float3 minBounds(float3::Zero);
    float3 maxBounds(float3::Zero);

    if (GetActiveCoordinatesRange(&minActiveCoordinates, &maxActiveCoordinates))
    {
        minBounds.x = (float)minActiveCoordinates.x * m_scale;
        minBounds.y = (float)minActiveCoordinates.y * m_scale;
        minBounds.z = (float)minActiveCoordinates.z * m_scale;
        maxBounds.x = (float)(maxActiveCoordinates.x + 1) * m_scale;
        maxBounds.y = (float)(maxActiveCoordinates.y + 1) * m_scale;
        maxBounds.z = (float)(maxActiveCoordinates.z + 1) * m_scale;
    }

    return AABox(minBounds, maxBounds);
}

void BlockMeshVolume::SetPalette(const BlockPalette *pPalette)
{
    if (m_pPalette == pPalette)
        return;

    if (m_pPalette != NULL)
        m_pPalette->Release();

    if ((m_pPalette = pPalette) != NULL)
        m_pPalette->AddRef();
}

BlockVolumeBlockType BlockMeshVolume::GetBlock(int32 x, int32 y, int32 z) const
{
    DebugAssert(x >= m_minCoordinates.x && y >= m_minCoordinates.y && z >= m_minCoordinates.z);
    DebugAssert(x <= m_maxCoordinates.x && y <= m_maxCoordinates.y && z <= m_maxCoordinates.z);

    uint32 offsetX = (uint32)(x - m_minCoordinates.x);
    uint32 offsetY = (uint32)(y - m_minCoordinates.y);
    uint32 offsetZ = (uint32)(z - m_minCoordinates.z);
    DebugAssert(offsetX < m_width && offsetY < m_length && offsetZ < m_height);
    return m_pData[offsetZ * (m_width * m_length) + offsetY * m_width + offsetX];
}

void BlockMeshVolume::SetBlock(int32 x, int32 y, int32 z, BlockVolumeBlockType value)
{
    DebugAssert(x >= m_minCoordinates.x && y >= m_minCoordinates.y && z >= m_minCoordinates.z);
    DebugAssert(x <= m_maxCoordinates.x && y <= m_maxCoordinates.y && z <= m_maxCoordinates.z);

    uint32 offsetX = (uint32)(x - m_minCoordinates.x);
    uint32 offsetY = (uint32)(y - m_minCoordinates.y);
    uint32 offsetZ = (uint32)(z - m_minCoordinates.z);
    DebugAssert(offsetX < m_width && offsetY < m_length && offsetZ < m_height);
    m_pData[offsetZ * (m_width * m_length) + offsetY * m_width + offsetX] = value;
}

bool BlockMeshVolume::GetActiveCoordinatesRange(int3 *pMinActiveCoordinates, int3 *pMaxActiveCoordinates) const
{
    Vector3i minActiveBlockPosition;
    Vector3i maxActiveBlockPosition;
    const BlockVolumeBlockType *pCurrentBlock = m_pData;
    bool first = true;

    for (uint32 z = 0; z < m_height; z++)
    {
        for (uint32 y = 0; y < m_length; y++)
        {
            for (uint32 x = 0; x < m_width; x++)
            {
                if (*(pCurrentBlock++) != 0)
                {
                    int32 realX = (int32)x + m_minCoordinates.x;
                    int32 realY = (int32)y + m_minCoordinates.y;
                    int32 realZ = (int32)z + m_minCoordinates.z;
                    Vector3i blockPos(realX, realY, realZ);

                    if (first)
                    {
                        minActiveBlockPosition = blockPos;
                        maxActiveBlockPosition = blockPos;
                        first = false;
                    }
                    else
                    {
                        minActiveBlockPosition = minActiveBlockPosition.Min(blockPos);
                        maxActiveBlockPosition = maxActiveBlockPosition.Max(blockPos);
                    }
                }
            }
        }
    }

    if (first)
        return false;

    *pMinActiveCoordinates = minActiveBlockPosition;
    *pMaxActiveCoordinates = maxActiveBlockPosition;
    return true;

    /*
    bool hasBlocks = false;
    int3 minActiveCoordinates, maxActiveCoordinates;

    uint32 nBlocks = m_width * m_length * m_height;
    uint32 zStride = m_length * m_width;
    uint32 yStride = m_width;
    for (uint32 i = 0; i < nBlocks; i++)
    {
        if (m_pData[i] != 0)
        {
            // reverse the index to coordinates
            uint32 temp = i;
            uint32 lz = temp / zStride;     temp %= zStride;
            uint32 ly = temp / yStride;     temp %= yStride;
            uint32 lx = temp;
            int3 coords(int3((int32)lx, (int32)ly, (int32)lz) + m_minCoordinates);
            if (hasBlocks)
            {
                minActiveCoordinates = minActiveCoordinates.Min(coords);
                maxActiveCoordinates = maxActiveCoordinates.Max(coords);
            }
            else
            {
                minActiveCoordinates = coords;
                maxActiveCoordinates = coords;
                hasBlocks = true;
            }
        }
    }

    if (!hasBlocks)
        return false;

    *pMinActiveCoordinates = minActiveCoordinates;
    *pMaxActiveCoordinates = maxActiveCoordinates;
    return true;*/
}

void BlockMeshVolume::Clear()
{
    uint32 nBlocks = m_width * m_length * m_height;
    Y_memzero(m_pData, sizeof(BlockVolumeBlockType) * nBlocks);
}

void BlockMeshVolume::Resize(const int3 &newMinCoordinates, const int3 &newMaxCoordinates)
{
    DebugAssert(!newMinCoordinates.AnyGreater(newMaxCoordinates));

    int3 newCoordinateRange(newMaxCoordinates - newMinCoordinates);
    uint32 newWidth = (uint32)newCoordinateRange.x + 1;
    uint32 newLength = (uint32)newCoordinateRange.y + 1;
    uint32 newHeight = (uint32)newCoordinateRange.z + 1;

    uint32 nNewBlocks = newWidth * newLength * newHeight;
    uint8 *pNewData = new uint8[nNewBlocks];
    Y_memzero(pNewData, sizeof(uint8) * nNewBlocks);

    if (m_pData != NULL)
    {
        // load old data
        int32 oldMinX = m_minCoordinates.x;
        int32 oldMinY = m_minCoordinates.y;
        int32 oldMinZ = m_minCoordinates.z;
        int32 oldMaxX = m_maxCoordinates.x;
        int32 oldMaxY = m_maxCoordinates.y;
        int32 oldMaxZ = m_maxCoordinates.z;
        const uint8 *pOldDataPtr = m_pData;

        // map the old block data across
        int32 x, y, z;
        for (z = oldMinZ; z <= oldMaxZ; z++)
        {
            for (y = oldMinY; y <= oldMaxY; y++)
            {
                for (x = oldMinX; x <= oldMaxX; x++)
                {
                    uint8 blockValue = *(pOldDataPtr++);
                    if (blockValue != 0)
                    {
                        int3 blockPosition(x, y, z);
                        if (blockPosition.AnyLess(newMinCoordinates) || blockPosition.AnyGreater(newMaxCoordinates))
                            continue;

                        Vector3i blockArrayIndices = blockPosition - newMinCoordinates;
                        uint32 newArrayIndex = (uint32)blockArrayIndices.z * (newWidth * newLength) + (uint32)blockArrayIndices.y * (newWidth) + (uint32)blockArrayIndices.x;
                        DebugAssert(newArrayIndex < nNewBlocks);
                        pNewData[newArrayIndex] = blockValue;
                    }
                }
            }
        }

        delete[] m_pData;
    }

    // store new values
    m_minCoordinates = newMinCoordinates;
    m_maxCoordinates = newMaxCoordinates;
    m_width = newWidth;
    m_length = newLength;
    m_height = newHeight;
    m_pData = pNewData;
}

void BlockMeshVolume::Center()
{
    int3 activeCoordinateMin, activeCoordinateMax;
    if (!GetActiveCoordinatesRange(&activeCoordinateMin, &activeCoordinateMax))
        return;

    // get the current 'mid point'
    int3 currentMidPoint = activeCoordinateMin + ((activeCoordinateMax - activeCoordinateMin + int3::One) / 2);

    // work out the difference we have to move them
    int3 moveDelta = int3::Zero - currentMidPoint;
    if (moveDelta != int3::Zero)
    {
        // use copies here, since the dimensions will be resized
        MoveBlocks(int3(m_minCoordinates), int3(m_maxCoordinates), moveDelta);
    }
}

void BlockMeshVolume::Shrink()
{
    // simply resize to the min/max range
    int3 newMinCoordinates, newMaxCoordinates;
    if (GetActiveCoordinatesRange(&newMinCoordinates, &newMaxCoordinates))
        Resize(newMinCoordinates, newMaxCoordinates);
    else
        Resize(int3::Zero, int3::Zero);
}

void BlockMeshVolume::MoveBlock(const int3 &blockCoordinates, const int3 &moveDelta)
{
    BlockVolumeBlockType oldValue = GetBlock(blockCoordinates);
    SetBlock(blockCoordinates + moveDelta, oldValue);
    SetBlock(blockCoordinates, 0);
}

void BlockMeshVolume::MoveBlocks(const int3 &selectionMin, const int3 &selectionMax, const int3 &moveDelta)
{
    int3 moveSelectionMin = m_minCoordinates.Max(selectionMin);
    int3 moveSelectionMax = m_maxCoordinates.Min(selectionMax);
    int32 x, y, z;

    // take a copy of the mesh
    int3 oldMinCoordinates = m_minCoordinates;
    int3 oldMaxCoordinates = m_maxCoordinates;
    uint32 oldZStride = m_length * m_width;
    uint32 oldYStride = m_width;
    uint32 nBlocks = m_width * m_length * m_height;
    BlockVolumeBlockType *pMeshCopy = new BlockVolumeBlockType[nBlocks];
    Y_memcpy(pMeshCopy, m_pData, sizeof(uint8) * nBlocks);

    // make sure we can fit
    int3 newMinCoordinates = oldMinCoordinates;
    int3 newMaxCoordinates = oldMaxCoordinates;
    (moveDelta.x < 0) ? newMinCoordinates.x += moveDelta.x : newMaxCoordinates.x += moveDelta.x;
    (moveDelta.y < 0) ? newMinCoordinates.y += moveDelta.y : newMaxCoordinates.y += moveDelta.y;
    (moveDelta.z < 0) ? newMinCoordinates.z += moveDelta.z : newMaxCoordinates.z += moveDelta.z;
    //Resize(newMinCoordinates, newMaxCoordinates);

    // pass 1: nuke the source blocks
    for (z = moveSelectionMin.z; z <= moveSelectionMax.z; z++)
    {
        for (y = moveSelectionMin.y; y <= moveSelectionMax.y; y++)
        {
            for (x = moveSelectionMin.x; x <= moveSelectionMax.x; x++)
            {
                SetBlock(x, y, z, 0);
            }
        }
    }

    // pass 2: set the blocks from the copy
    for (z = moveSelectionMin.z; z <= moveSelectionMax.z; z++)
    {
        for (y = moveSelectionMin.y; y <= moveSelectionMax.y; y++)
        {
            for (x = moveSelectionMin.x; x <= moveSelectionMax.x; x++)
            {
                int3 coords(x, y, z);
                int3 arrayIndices = coords - oldMinCoordinates;
                DebugAssert(!arrayIndices.AnyLess(Vector3i::Zero));
                uint32 arrayIndex = (uint32)arrayIndices.z * oldZStride + (uint32)arrayIndices.y * oldYStride + (uint32)arrayIndices.x;
                DebugAssert(arrayIndex < nBlocks);

                BlockVolumeBlockType value = pMeshCopy[arrayIndex];
                SetBlock(coords + moveDelta, value);
            }
        }
    }

    delete[] pMeshCopy;
}

bool BlockMeshVolume::RayCastTime(const Ray &ray, int3 *pIntersectingBlock, float *pIntersectionTime, bool exitAtFirstIntersection /* = false */) const
{
    //Vector3 rayStartPosition = ray.sp / scale - mincoords;

    int32 minCoordsX = m_minCoordinates.x;
    int32 minCoordsY = m_minCoordinates.y;
    int32 minCoordsZ = m_minCoordinates.z;
    SIMDVector3f vec_scale(m_scale, m_scale, m_scale);
    SIMDVector3f vec_one_mul_scale(vec_scale);
    int3 bestBlock;
    float bestTime = Y_FLT_INFINITE;

    const BlockVolumeBlockType *pCurrentBlock = m_pData;
    for (uint32 z = 0; z < m_height; z++)
    {
        for (uint32 y = 0; y < m_length; y++)
        {
            for (uint32 x = 0; x < m_width; x++)
            {
                BlockVolumeBlockType blockType = *(pCurrentBlock++);
                if (blockType != 0)
                {
                    int32 realX = (int32)x + minCoordsX;
                    int32 realY = (int32)y + minCoordsY;
                    int32 realZ = (int32)z + minCoordsZ;

                    SIMDVector3f minBlockBounds(SIMDVector3f((float)realX, (float)realY, (float)realZ) * vec_scale);
                    SIMDVector3f maxBlockBounds(minBlockBounds + vec_one_mul_scale);

                    float time = ray.AABoxIntersectionTime(minBlockBounds, maxBlockBounds);
                    if (time < bestTime)
                    {
                        bestBlock.Set(realX, realY, realZ);
                        bestTime = time;

                        if (exitAtFirstIntersection)
                        {
                            *pIntersectingBlock = bestBlock;
                            *pIntersectionTime = time;
                            return true;
                        }
                    }
                }
            }
        }
    }

    if (bestTime != Y_FLT_INFINITE)
    {
        *pIntersectingBlock = bestBlock;
        *pIntersectionTime = bestTime;
        return true;
    }

    return false;
}

bool BlockMeshVolume::RayCastTimeFace(const Ray &ray, int3 *pIntersectingBlock, float *pIntersectionTime, CUBE_FACE *pIntersectingFace, bool exitAtFirstIntersection /*= false*/) const
{
    int32 minCoordsX = m_minCoordinates.x;
    int32 minCoordsY = m_minCoordinates.y;
    int32 minCoordsZ = m_minCoordinates.z;
    SIMDVector3f vec_scale(m_scale, m_scale, m_scale);
    int3 bestBlock(int3::Zero);
    float bestTime = Y_FLT_INFINITE;
    CUBE_FACE bestFace = CUBE_FACE_COUNT;

    const BlockVolumeBlockType *pCurrentBlock = m_pData;
    for (uint32 z = 0; z < m_height; z++)
    {
        for (uint32 y = 0; y < m_length; y++)
        {
            for (uint32 x = 0; x < m_width; x++)
            {
                BlockVolumeBlockType blockType = *(pCurrentBlock++);
                if (blockType != 0)
                {
                    int32 realX = (int32)x + minCoordsX;
                    int32 realY = (int32)y + minCoordsY;
                    int32 realZ = (int32)z + minCoordsZ;

                    SIMDVector3f minBlockBounds(SIMDVector3f((float)realX, (float)realY, (float)realZ) * vec_scale);
                    SIMDVector3f maxBlockBounds(minBlockBounds + vec_scale);

                    float time;
                    CUBE_FACE face;
                    if (ray.AABoxIntersectionTimeFace(minBlockBounds, maxBlockBounds, &time, &face) &&
                        time < bestTime)
                    {
                        bestBlock.Set(realX, realY, realZ);
                        bestTime = time;
                        bestFace = face;

                        if (exitAtFirstIntersection)
                        {
                            *pIntersectingBlock = bestBlock;
                            *pIntersectionTime = time;
                            *pIntersectingFace = face;
                            return true;
                        }
                    }
                }
            }
        }
    }

    if (bestTime != Y_FLT_INFINITE)
    {
        *pIntersectingBlock = bestBlock;
        *pIntersectionTime = bestTime;
        *pIntersectingFace = bestFace;
        return true;
    }

    return false;
}

BlockMeshVolume & BlockMeshVolume::operator=(const BlockMeshVolume &copy)
{
    uint32 nBlocks = copy.m_width * copy.m_length * copy.m_height;

    if (m_width != copy.m_width || m_length != copy.m_length || m_height != copy.m_height)
    {
        m_width = copy.m_width;
        m_length = copy.m_length;
        m_height = copy.m_height;

        delete[] m_pData;
        m_pData = new BlockVolumeBlockType[nBlocks];
    }

    m_scale = copy.m_scale;
    m_minCoordinates = copy.m_minCoordinates;
    m_maxCoordinates = copy.m_maxCoordinates;

    Y_memcpy(m_pData, copy.m_pData, sizeof(BlockVolumeBlockType) * nBlocks);
    return *this;
}
