#include "Engine/PrecompiledHeader.h"
#include "Engine/BlockMeshUtilities.h"
#include "Engine/BlockPalette.h"
//Log_SetChannel(BlockMeshUtilities);

namespace BlockMeshUtilities {

struct RayMeshIntersectionParameters
{
    // in
    const BlockPalette *pBlockList;
    Vector3i MeshMinCoordinates;
    Vector3i MeshMaxCoordinates;
    int32 DataZStride, DataYStride;
    const byte *pBlockData;
    Ray TestRay;

    // out
    float ClosestDistance;
    Vector3i ClosestBlock;
    uint8 ClosestFace;
};

inline static byte RayMeshIntersectionGetBlock(const RayMeshIntersectionParameters *pParameters, const Vector3i &blockPosition)
{
    Vector3i local = blockPosition - pParameters->MeshMinCoordinates;
    return pParameters->pBlockData[local.z * pParameters->DataZStride + local.y * pParameters->DataYStride + local.x];
}

static void RayMeshIntersectionInternal(RayMeshIntersectionParameters *pParameters, const SIMDVector3i &minNodeBounds, const SIMDVector3i &maxNodeBounds, const Vector3i &nodeSize)
{
    float hitDistance;

    // if we can't subdivide the block any more, test everything inside the bounds
    if (nodeSize % 2 != Vector3i::Zero)
    {
        Vector3i currentBlock;

        for (currentBlock = minNodeBounds; currentBlock.z <= maxNodeBounds.z; currentBlock.z++)
        {
            for (currentBlock.y = minNodeBounds.y; currentBlock.y <= maxNodeBounds.y; currentBlock.y++)
            {
                for (currentBlock.x = minNodeBounds.x; currentBlock.x <= maxNodeBounds.x; currentBlock.x++)
                {
                    if ((pParameters->pBlockList->GetBlockType(RayMeshIntersectionGetBlock(pParameters, currentBlock))->Flags & (BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE | BLOCK_MESH_BLOCK_TYPE_FLAG_COLLIDABLE)) == 0)
                        continue;

                    SIMDVector3f blockMinBounds = SIMDVector3f(currentBlock);
                    SIMDVector3f blockMaxBounds = blockMinBounds + 1.0f;

                    // intersect the ray with it
                    uint32 hitFace = 0;
                    hitDistance = pParameters->TestRay.AABoxIntersectionTime(blockMinBounds, blockMaxBounds);
                    if (hitDistance < pParameters->ClosestDistance)
                    {
                        pParameters->ClosestDistance = hitDistance;
                        pParameters->ClosestBlock = currentBlock;
                        pParameters->ClosestFace = (uint8)hitFace;
                    }
                }
            }
        }

        // exit early
        return;
    }

    // split it up by half
    Vector3i halfNodeSize = nodeSize / 2;
    Vector3i childMinBounds, childMaxBounds;

    // front bottomleft block
    childMinBounds = (minNodeBounds).Max(pParameters->MeshMinCoordinates);
    childMaxBounds = (childMinBounds + halfNodeSize).Min(pParameters->MeshMaxCoordinates) + Vector3i::One;
    if (pParameters->TestRay.AABoxIntersection(SIMDVector3f(childMinBounds), SIMDVector3f(childMaxBounds)))
        RayMeshIntersectionInternal(pParameters, childMinBounds, childMaxBounds, halfNodeSize);

    // front bottomright block
    childMinBounds = (minNodeBounds + SIMDVector3i(halfNodeSize.x, 0, 0)).Max(pParameters->MeshMinCoordinates);
    childMaxBounds = (childMinBounds + halfNodeSize).Min(pParameters->MeshMaxCoordinates) + Vector3i::One;
    if (pParameters->TestRay.AABoxIntersection(SIMDVector3f(childMinBounds), SIMDVector3f(childMaxBounds)))
        RayMeshIntersectionInternal(pParameters, childMinBounds, childMaxBounds, halfNodeSize);

    // front topleft block
    childMinBounds = (minNodeBounds + SIMDVector3i(0, 0, halfNodeSize.z)).Max(pParameters->MeshMinCoordinates);
    childMaxBounds = (childMinBounds + halfNodeSize).Min(pParameters->MeshMaxCoordinates) + Vector3i::One;
    if (pParameters->TestRay.AABoxIntersection(SIMDVector3f(childMinBounds), SIMDVector3f(childMaxBounds)))
        RayMeshIntersectionInternal(pParameters, childMinBounds, childMaxBounds, halfNodeSize);

    // front topright block
    childMinBounds = (minNodeBounds + SIMDVector3i(halfNodeSize.x, 0, halfNodeSize.z)).Max(pParameters->MeshMinCoordinates);
    childMaxBounds = (childMinBounds + halfNodeSize).Min(pParameters->MeshMaxCoordinates) + Vector3i::One;
    if (pParameters->TestRay.AABoxIntersection(SIMDVector3f(childMinBounds), SIMDVector3f(childMaxBounds)))
        RayMeshIntersectionInternal(pParameters, childMinBounds, childMaxBounds, halfNodeSize);

    // back bottomleft block
    childMinBounds = (minNodeBounds + SIMDVector3i(0, halfNodeSize.y, 0)).Max(pParameters->MeshMinCoordinates);
    childMaxBounds = (childMinBounds + halfNodeSize).Min(pParameters->MeshMaxCoordinates) + Vector3i::One;
    if (pParameters->TestRay.AABoxIntersection(SIMDVector3f(childMinBounds), SIMDVector3f(childMaxBounds)))
        RayMeshIntersectionInternal(pParameters, childMinBounds, childMaxBounds, halfNodeSize);

    // back bottomright block
    childMinBounds = (minNodeBounds + SIMDVector3i(halfNodeSize.x, halfNodeSize.y, 0)).Max(pParameters->MeshMinCoordinates);
    childMaxBounds = (childMinBounds + halfNodeSize).Min(pParameters->MeshMaxCoordinates) + Vector3i::One;
    if (pParameters->TestRay.AABoxIntersection(SIMDVector3f(childMinBounds), SIMDVector3f(childMaxBounds)))
        RayMeshIntersectionInternal(pParameters, childMinBounds, childMaxBounds, halfNodeSize);

    // back topleft block
    childMinBounds = (minNodeBounds + SIMDVector3i(0, halfNodeSize.y, halfNodeSize.z)).Max(pParameters->MeshMinCoordinates);
    childMaxBounds = (childMinBounds + halfNodeSize).Min(pParameters->MeshMaxCoordinates) + Vector3i::One;
    if (pParameters->TestRay.AABoxIntersection(SIMDVector3f(childMinBounds), SIMDVector3f(childMaxBounds)))
        RayMeshIntersectionInternal(pParameters, childMinBounds, childMaxBounds, halfNodeSize);

    // back topright block
    childMinBounds = (minNodeBounds + SIMDVector3i(halfNodeSize.x, halfNodeSize.y, halfNodeSize.z)).Max(pParameters->MeshMinCoordinates);
    childMaxBounds = (childMinBounds + halfNodeSize).Min(pParameters->MeshMaxCoordinates) + Vector3i::One;
    if (pParameters->TestRay.AABoxIntersection(SIMDVector3f(childMinBounds), SIMDVector3f(childMaxBounds)))
        RayMeshIntersectionInternal(pParameters, childMinBounds, childMaxBounds, halfNodeSize);
}

float RayMeshIntersection(int3 *pHitPosition, uint8 *pHitFace, const BlockPalette *pBlockList, const int3 &meshMinCoordinates, const int3 &meshMaxCoordinates, const BlockVolumeBlockType *pBlockData, const Ray &ray)
{
    // get mesh size, then align it to powers of 2 for faster iteration
    Vector3i meshSize = meshMaxCoordinates - meshMinCoordinates + Vector3i::One;
    int32 nextPow2SmallestComponent = Y_nextpow2i(Min(meshSize.x, Min(meshSize.y, meshSize.z)));
    Vector3i alignedMeshSize = Vector3i(Max(nextPow2SmallestComponent, meshSize.x), Max(nextPow2SmallestComponent, meshSize.y), Max(nextPow2SmallestComponent, meshSize.z));
    DebugAssert(!meshSize.AnyLessEqual(Vector3i::Zero));

    // create query struct
    // ray origin is scaled down by each block size, so that each unit is equal to "one block"
    RayMeshIntersectionParameters query;
    query.pBlockList = pBlockList;
    query.MeshMinCoordinates = meshMinCoordinates;
    query.MeshMaxCoordinates = meshMaxCoordinates;
    query.DataYStride = sizeof(byte) * meshSize.x;
    query.DataZStride = query.DataYStride * meshSize.y;
    query.pBlockData = pBlockData;
    query.TestRay = Ray(ray.GetOrigin(), ray.GetDirection());
    query.ClosestDistance = Y_FLT_INFINITE;       

    // invoke it
    RayMeshIntersectionInternal(&query, query.MeshMinCoordinates, query.MeshMaxCoordinates, alignedMeshSize);

    // got a result?
    if (query.ClosestDistance != Y_FLT_INFINITE)
    {
        // store them
        *pHitPosition = query.ClosestBlock;
        if (pHitFace != NULL)
            *pHitFace = query.ClosestFace;
    }

    return query.ClosestDistance;
}

float RayMeshIntersection(int32 *pHitX, int32 *pHitY, int32 *pHitZ, uint8 *pHitFace, const BlockPalette *pBlockList, int32 meshMinX, int32 meshMinY, int32 meshMinZ, int32 meshMaxX, int32 meshMaxY, int32 meshMaxZ, const BlockVolumeBlockType *pBlockData, const Ray &ray)
{
    // get mesh size, then align it to powers of 2 for faster iteration
    Vector3i meshMinCoordinates = Vector3i(meshMinX, meshMinY, meshMinZ);
    Vector3i meshMaxCoordinates = Vector3i(meshMaxX, meshMaxY, meshMaxZ);
    Vector3i meshSize = meshMaxCoordinates - meshMinCoordinates + Vector3i::One;
    int32 nextPow2SmallestComponent = Y_nextpow2i(Min(meshSize.x, Min(meshSize.y, meshSize.z)));
    Vector3i alignedMeshSize = Vector3i(Max(nextPow2SmallestComponent, meshSize.x), Max(nextPow2SmallestComponent, meshSize.y), Max(nextPow2SmallestComponent, meshSize.z));
    DebugAssert(!meshSize.AnyLessEqual(Vector3i::Zero));

    // create query struct
    // ray origin is scaled down by each block size, so that each unit is equal to "one block"
    RayMeshIntersectionParameters query;
    query.pBlockList = pBlockList;
    query.MeshMinCoordinates = meshMinCoordinates;
    query.MeshMaxCoordinates = meshMaxCoordinates;
    query.DataYStride = sizeof(byte) * meshSize.x;
    query.DataZStride = query.DataYStride * meshSize.y;
    query.pBlockData = pBlockData;
    query.TestRay = Ray(ray.GetOrigin(), ray.GetDirection());
    query.ClosestDistance = Y_FLT_INFINITE;       
        
    // invoke it
    RayMeshIntersectionInternal(&query, query.MeshMinCoordinates, query.MeshMaxCoordinates, alignedMeshSize);

    // store them
    if (query.ClosestDistance != Y_FLT_INFINITE)
    {
        *pHitX = query.ClosestBlock.x;
        *pHitY = query.ClosestBlock.y;
        *pHitZ = query.ClosestBlock.z;
        if (pHitFace != NULL)
            *pHitFace = query.ClosestFace;
    }

    return query.ClosestDistance;
}

void CreateMeshLOD(uint8 *pOutBlockData, uint32 lodLevel, const BlockPalette *pBlockList, const uint8 *pBlockData, uint32 meshSize)
{
    uint8 blockValues[1024];
    uint32 nBlockValues;
    Assert(lodLevel < 5);

    uint32 meshYPitch = (sizeof(uint8) * meshSize);
    uint32 meshZPitch = meshYPitch * meshSize;
    
    uint32 lodSize = meshSize >> lodLevel;
    uint32 lodYPitch = (sizeof(uint8) * lodSize);
    uint32 lodZPitch = lodYPitch * lodSize;
    uint32 collapseSize = (1 << lodLevel);

    uint32 i, j;
    uint32 x, y, z;
    uint32 x2, y2, z2;
    for (z = 0; z < lodSize; z++)
    {
        for (y = 0; y < lodSize; y++)
        {
            for (x = 0; x < lodSize; x++)
            {
                nBlockValues = 0;

                uint32 sx = x * collapseSize;
                uint32 sy = y * collapseSize;
                uint32 sz = z * collapseSize;
                for (z2 = 0; z2 < collapseSize; z2++)
                {
                    for (y2 = 0; y2 < collapseSize; y2++)
                    {
                        for (x2 = 0; x2 < collapseSize; x2++)
                        {
                            DebugAssert(nBlockValues < countof(blockValues));
                            blockValues[nBlockValues++] = pBlockData[(sz + z2) * meshZPitch + (sy + y2) * meshYPitch + (sx + x2)];
                        }
                    }
                }

                // find the most common block type
                uint8 mostCommonBlockType = 0;
                uint32 mostCommonBlockTypeCount = 0;
                uint8 mostCommonOpaqueBlockType = 0;
                uint32 mostCommonOpaqueBlockTypeCount = 0;
                for (i = 0; i < nBlockValues; i++)
                {
                    uint8 blockType = blockValues[i];
                    if (blockType == mostCommonBlockType)
                        continue;

                    uint32 count = 0;
                    for (j = 0; j < nBlockValues; j++)
                    {
                        if (blockValues[j] == blockType)
                            count++;
                    }

                    if (count > mostCommonBlockTypeCount)
                    {
                        mostCommonBlockType = blockType;
                        mostCommonBlockTypeCount = count;
                    }

                    if (count > mostCommonOpaqueBlockTypeCount)
                    {
                        if (blockType != 0 && pBlockList->GetBlockType(blockType)->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCKS_VISIBILITY)
                        {
                            mostCommonOpaqueBlockType = blockType;
                            mostCommonOpaqueBlockTypeCount = count;
                        }
                    }
                }

                // use the most common transparent over a solid block
                uint8 collapsedBlockType = (mostCommonOpaqueBlockTypeCount > 0) ? mostCommonOpaqueBlockType : mostCommonBlockType;
                pOutBlockData[z * lodZPitch + y * lodYPitch + x] = collapsedBlockType; 
            }
        }
    }
}

};      // namespace BlockMeshUtilities
