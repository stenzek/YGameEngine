#pragma once
#include "Engine/Common.h"
#include "Engine/BlockPalette.h"
#include "Engine/BlockMeshVolume.h"

namespace BlockMeshUtilities {

float RayMeshIntersection(int3 *pHitPosition, uint8 *pHitFace, const BlockPalette *pBlockList, const int3 &meshMinCoordinates, const int3 &meshMaxCoordinates, const BlockVolumeBlockType *pBlockData, const Ray &ray);
float RayMeshIntersection(int32 *pHitX, int32 *pHitY, int32 *pHitZ, uint8 *pHitFace, const BlockPalette *pBlockList, int32 meshMinX, int32 meshMinY, int32 meshMinZ, int32 meshMaxX, int32 meshMaxY, int32 meshMaxZ, const BlockVolumeBlockType *pBlockData, const Ray &ray);

void CreateMeshLOD(uint8 *pOutBlockData, uint32 lodLevel, const BlockPalette *pBlockList, const uint8 *pBlockData, uint32 meshSize);

};      // namespace BlockMeshUtilities