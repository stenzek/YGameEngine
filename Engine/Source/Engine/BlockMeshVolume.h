#pragma once
#include "Engine/Common.h"
#include "Engine/BlockPalette.h"
#include "MathLib/CollisionDetection.h"

typedef uint8 BlockVolumeBlockType;

class BlockMeshVolume
{
public:
    BlockMeshVolume();
    BlockMeshVolume(const BlockPalette *pPalette, float scale, const int3 &minCoordinates, const int3 &maxCoordinates);
    BlockMeshVolume(const BlockMeshVolume &copy);
    ~BlockMeshVolume();

    const BlockPalette *GetPalette() const { return m_pPalette; }
    const float GetScale() const { return m_scale; }
    const int3 &GetMinCoordinates() const { return m_minCoordinates; }
    const int3 &GetMaxCoordinates() const { return m_maxCoordinates; }
    const uint32 GetWidth() const { return m_width; }
    const uint32 GetLength() const { return m_length; }
    const uint32 GetHeight() const { return m_height; }
    const BlockVolumeBlockType *GetData() const { return m_pData; }

    // calculate the bounding box
    AABox CalculateBoundingBox() const;

    // calculate the active bounding box
    AABox CalculateActiveBoundingBox() const;
    
    // get volume data pointer
    BlockVolumeBlockType *GetData() { return m_pData; }

    // settings
    void SetPalette(const BlockPalette *pPalette);
    void SetScale(float scale) { m_scale = scale; }

    // block management
    BlockVolumeBlockType GetBlock(int32 x, int32 y, int32 z) const;
    void SetBlock(int32 x, int32 y, int32 z, BlockVolumeBlockType value);

    // block management via vectors
    BlockVolumeBlockType GetBlock(const int3 &blockPosition) const { return GetBlock(blockPosition.x, blockPosition.y, blockPosition.z); }
    void SetBlock(const int3 &blockPosition, BlockVolumeBlockType value) { SetBlock(blockPosition.x, blockPosition.y, blockPosition.z, value); }

    // get active (nonzero) coordinate range
    bool GetActiveCoordinatesRange(int3 *pMinActiveCoordinates, int3 *pMaxActiveCoordinates) const;

    // clear mesh
    void Clear();

    // resize mesh
    void Resize(const int3 &newMinCoordinates, const int3 &newMaxCoordinates);

    // center the mesh
    void Center();

    // shrink the mesh to the minimum size possible
    void Shrink();

    // move blocks
    void MoveBlock(const int3 &blockCoordinates, const int3 &moveDelta);
    void MoveBlocks(const int3 &selectionMin, const int3 &selectionMax, const int3 &moveDelta);
    
    // raycasting
    bool RayCastTime(const Ray &ray, int3 *pIntersectingBlock, float *pIntersectionTime, bool exitAtFirstIntersection = false) const;
    bool RayCastTimeFace(const Ray &ray, int3 *pIntersectingBlock, float *pIntersectionTime, CUBE_FACE *pIntersectingFace, bool exitAtFirstIntersection = false) const;

    // collision helpers
    // callback is in format of callback(const int3 &blockLocation)
    template<typename CALLBACK_TYPE> void EnumerateBlocksInBox(const AABox &box, CALLBACK_TYPE callback) const;

    // callback is in format of callback(const int3 &blockLocation)
    template<typename CALLBACK_TYPE> void EnumerateBlocksInSphere(const Sphere &sphere, CALLBACK_TYPE callback) const;

    // callback is in format of callback(const int3 &blockLocation, const float3 &contactNormal, const float3 &contactPoint)
    template<typename CALLBACK_TYPE> void EnumerateBlocksIntersectingBox(const AABox &box, CALLBACK_TYPE callback) const;

    // callback is in format of callback(const int3 &blockLocation, const float3 &contactNormal, const float3 &contactPoint)
    template<typename CALLBACK_TYPE> void EnumerateBlocksIntersectingSphere(const Sphere &sphere, CALLBACK_TYPE callback) const;

    // callback is in format of callback(const float3 vertices[3])
    template<typename CALLBACK_TYPE> void EnumerateTrianglesIntersectingBox(const AABox &box, CALLBACK_TYPE callback) const;

    // copy operator
    BlockMeshVolume &operator=(const BlockMeshVolume &copy);

private:
    const BlockPalette *m_pPalette;
    float m_scale;
    int3 m_minCoordinates;
    int3 m_maxCoordinates;
    uint32 m_width, m_length, m_height;
    BlockVolumeBlockType *m_pData;
};

template<typename CALLBACK_TYPE>
void BlockMeshVolume::EnumerateBlocksInBox(const AABox &box, CALLBACK_TYPE callback) const
{
    // get the range to search
    float inverseScale = 1.0f / m_scale;
    SIMDVector3f boxMinBounds(box.GetMinBounds());
    SIMDVector3f boxMaxBounds(box.GetMaxBounds());
    SIMDVector3f minSearchFloat(boxMinBounds * inverseScale);
    SIMDVector3f maxSearchFloat(boxMaxBounds * inverseScale);
    SIMDVector3i minSearch(SIMDVector3i(Math::Truncate(Math::Floor(minSearchFloat.x)), Math::Truncate(Math::Floor(minSearchFloat.y)), Math::Truncate(Math::Floor(minSearchFloat.z))).Clamp(m_minCoordinates, m_maxCoordinates));
    SIMDVector3i maxSearch(SIMDVector3i(Math::Truncate(Math::Ceil(maxSearchFloat.x)), Math::Truncate(Math::Ceil(maxSearchFloat.y)), Math::Truncate(Math::Ceil(maxSearchFloat.z))).Clamp(m_minCoordinates, m_maxCoordinates));

    // search blocks
    for (int32 z = minSearch.z; z <= maxSearch.z; z++)
    {
        for (int32 y = minSearch.y; y <= maxSearch.y; y++)
        {
            for (int32 x = minSearch.z; x <= maxSearch.z; x++)
            {
                BlockVolumeBlockType blockType = GetBlock(x, y, z);
                if (blockType == 0)
                    continue;

                callback(int3(x, y, z));
            }
        }
    }
}

template<typename CALLBACK_TYPE>
void BlockMeshVolume::EnumerateBlocksInSphere(const Sphere &sphere, CALLBACK_TYPE callback) const
{
    // get the range to search
    float inverseScale = 1.0f / m_scale;
    AABox sphereBox(AABox::FromSphere(sphere));
    SIMDVector3f sphereBoxMinBounds(sphereBox.GetMinBounds());
    SIMDVector3f sphereBoxMaxBounds(sphereBox.GetMaxBounds());
    SIMDVector3f minSearchFloat(sphereBoxMinBounds * inverseScale);
    SIMDVector3f maxSearchFloat(sphereBoxMaxBounds * inverseScale);
    SIMDVector3i minSearch(SIMDVector3i(Math::Truncate(Math::Floor(minSearchFloat.x)), Math::Truncate(Math::Floor(minSearchFloat.y)), Math::Truncate(Math::Floor(minSearchFloat.z))).Clamp(m_minCoordinates, m_maxCoordinates));
    SIMDVector3i maxSearch(SIMDVector3i(Math::Truncate(Math::Ceil(maxSearchFloat.x)), Math::Truncate(Math::Ceil(maxSearchFloat.y)), Math::Truncate(Math::Ceil(maxSearchFloat.z))).Clamp(m_minCoordinates, m_maxCoordinates));

    // search blocks
    for (int32 z = minSearch.z; z <= maxSearch.z; z++)
    {
        for (int32 y = minSearch.y; y <= maxSearch.y; y++)
        {
            for (int32 x = minSearch.z; x <= maxSearch.z; x++)
            {
                BlockVolumeBlockType blockType = GetBlock(x, y, z);
                if (blockType == 0)
                    continue;

                callback(int3(x, y, z));
            }
        }
    }
}

template<typename CALLBACK_TYPE>
void BlockMeshVolume::EnumerateBlocksIntersectingBox(const AABox &box, CALLBACK_TYPE callback) const
{
    // get the range to search
    float inverseScale = 1.0f / m_scale;
    SIMDVector3f boxMinBounds(box.GetMinBounds());
    SIMDVector3f boxMaxBounds(box.GetMaxBounds());
    SIMDVector3f minSearchFloat(boxMinBounds * inverseScale);
    SIMDVector3f maxSearchFloat(boxMaxBounds * inverseScale);
    SIMDVector3i minSearch(SIMDVector3i(Math::Truncate(Math::Floor(minSearchFloat.x)), Math::Truncate(Math::Floor(minSearchFloat.y)), Math::Truncate(Math::Floor(minSearchFloat.z))).Clamp(m_minCoordinates, m_maxCoordinates));
    SIMDVector3i maxSearch(SIMDVector3i(Math::Truncate(Math::Ceil(maxSearchFloat.x)), Math::Truncate(Math::Ceil(maxSearchFloat.y)), Math::Truncate(Math::Ceil(maxSearchFloat.z))).Clamp(m_minCoordinates, m_maxCoordinates));

    // search blocks
    for (int32 z = minSearch.z; z <= maxSearch.z; z++)
    {
        for (int32 y = minSearch.y; y <= maxSearch.y; y++)
        {
            for (int32 x = minSearch.z; x <= maxSearch.z; x++)
            {
                BlockVolumeBlockType blockType = GetBlock(x, y, z);
                if (blockType == 0)
                    continue;

                const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType((uint32)blockType);
                if (pBlockType != NULL && pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
                {
                    // calculate bounds of block
                    SIMDVector3f blockMinBounds(SIMDVector3f((float)x, (float)y, (float)z) * m_scale);
                    SIMDVector3f blockMaxBounds(blockMinBounds + m_scale);
                    float3 contactNormal, contactPoint;

                    // intersects?
                    if (CollisionDetection::AABoxIntersectsAABox(boxMinBounds, boxMaxBounds, blockMinBounds, blockMaxBounds, contactNormal, contactPoint))
                    {
                        contactPoint *= m_scale;
                        callback(int3(x, y, z), contactNormal, contactPoint);
                    }
                }
            }
        }
    }
}

template<typename CALLBACK_TYPE>
void BlockMeshVolume::EnumerateBlocksIntersectingSphere(const Sphere &sphere, CALLBACK_TYPE callback) const
{
    // get the range to search
    float inverseScale = 1.0f / m_scale;
    SIMDVector3f sphereCenter(sphere.GetCenter());
    float sphereRadius = sphere.GetRadius();
    AABox sphereBox(AABox::FromSphere(sphere));
    SIMDVector3f sphereBoxMinBounds(sphereBox.GetMinBounds());
    SIMDVector3f sphereBoxMaxBounds(sphereBox.GetMaxBounds());
    SIMDVector3f minSearchFloat(sphereBoxMinBounds * inverseScale);
    SIMDVector3f maxSearchFloat(sphereBoxMaxBounds * inverseScale);
    SIMDVector3i minSearch(SIMDVector3i(Math::Truncate(Math::Floor(minSearchFloat.x)), Math::Truncate(Math::Floor(minSearchFloat.y)), Math::Truncate(Math::Floor(minSearchFloat.z))).Clamp(m_minCoordinates, m_maxCoordinates));
    SIMDVector3i maxSearch(SIMDVector3i(Math::Truncate(Math::Ceil(maxSearchFloat.x)), Math::Truncate(Math::Ceil(maxSearchFloat.y)), Math::Truncate(Math::Ceil(maxSearchFloat.z))).Clamp(m_minCoordinates, m_maxCoordinates));

    // search blocks
    for (int32 z = minSearch.z; z <= maxSearch.z; z++)
    {
        for (int32 y = minSearch.y; y <= maxSearch.y; y++)
        {
            for (int32 x = minSearch.z; x <= maxSearch.z; x++)
            {
                BlockVolumeBlockType blockType = GetBlock(x, y, z);
                if (blockType == 0)
                    continue;

                const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType((uint32)blockType);
                if (pBlockType != NULL && pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
                {
                    // calculate bounds of block
                    SIMDVector3f blockMinBounds(SIMDVector3f((float)x, (float)y, (float)z) * m_scale);
                    SIMDVector3f blockMaxBounds(blockMinBounds + m_scale);
                    float3 contactNormal, contactPoint;

                    // intersects?
                    if (CollisionDetection::SphereIntersectsBox(sphereCenter, sphereRadius, blockMinBounds, blockMaxBounds, contactNormal, contactPoint))
                    {
                        contactPoint *= m_scale;
                        callback(int3(x, y, z), contactNormal, contactPoint);
                    }
                }
            }
        }
    }
}

template<typename CALLBACK_TYPE>
void BlockMeshVolume::EnumerateTrianglesIntersectingBox(const AABox &box, CALLBACK_TYPE callback) const
{
    // get the range to search
    float scale = m_scale;
    float inverseScale = 1.0f / scale;
    SIMDVector3f boxMinBounds(box.GetMinBounds());
    SIMDVector3f boxMaxBounds(box.GetMaxBounds());
    SIMDVector3f minSearchFloat(boxMinBounds * inverseScale);
    SIMDVector3f maxSearchFloat(boxMaxBounds * inverseScale);
    SIMDVector3i minSearch(SIMDVector3i(Math::Truncate(Math::Floor(minSearchFloat.x)), Math::Truncate(Math::Floor(minSearchFloat.y)), Math::Truncate(Math::Floor(minSearchFloat.z))).Clamp(m_minCoordinates, m_maxCoordinates));
    SIMDVector3i maxSearch(SIMDVector3i(Math::Truncate(Math::Ceil(maxSearchFloat.x)), Math::Truncate(Math::Ceil(maxSearchFloat.y)), Math::Truncate(Math::Ceil(maxSearchFloat.z))).Clamp(m_minCoordinates, m_maxCoordinates));

    // search blocks
    for (int32 z = minSearch.z; z <= maxSearch.z; z++)
    {
        for (int32 y = minSearch.y; y <= maxSearch.y; y++)
        {
            for (int32 x = minSearch.z; x <= maxSearch.z; x++)
            {
                BlockVolumeBlockType blockType = GetBlock(x, y, z);
                if (blockType == 0)
                    continue;

                const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType((uint32)blockType);
                if (pBlockType != NULL && pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
                {
                    // calculate bounds of block
                    float sx = (float)x * scale;
                    float sy = (float)y * scale;
                    float sz = (float)z * scale;
                    SIMDVector3f blockMinBounds(sx, sy, sz);
                    SIMDVector3f blockMaxBounds(blockMinBounds + m_scale);

                    // intersects?
                    if (CollisionDetection::AABoxIntersectsAABox(boxMinBounds, boxMaxBounds, blockMinBounds, blockMaxBounds))
                    {
                        // generate block triangles
                        float3 blockVertices[8] = 
                        {
                            float3( sx, sy, sz ),                            // bottom-front-left
                            float3( sx + scale, sy, sz ),                    // bottom-front-right
                            float3( sx, sy + scale, sz ),                    // bottom-back-left
                            float3( sx + scale, sy + scale, sz ),            // bottom-back-right
                            float3( sx, sy, sz + scale ),                    // top-front-left
                            float3( sx + scale, sy, sz + scale ),            // top-front-right
                            float3( sx, sy + scale, sz + scale ),            // top-back-left
                            float3( sx + scale, sy + scale, sz + scale )     // top-back-right
                        };
                        
                        float3 triangleVerticesA[3];
                        float3 triangleVerticesB[3];

                        // right face
                        triangleVerticesA[0] = blockVertices[5]; triangleVerticesA[1] = blockVertices[1]; triangleVerticesA[2] = blockVertices[3];
                        triangleVerticesB[0] = blockVertices[5]; triangleVerticesB[1] = blockVertices[3]; triangleVerticesB[2] = blockVertices[7];
                        callback(triangleVerticesA);
                        callback(triangleVerticesB);

                        // left face
                        triangleVerticesA[0] = blockVertices[4]; triangleVerticesA[1] = blockVertices[0]; triangleVerticesA[2] = blockVertices[2];
                        triangleVerticesB[0] = blockVertices[2]; triangleVerticesB[1] = blockVertices[4]; triangleVerticesB[2] = blockVertices[6];
                        callback(triangleVerticesA);
                        callback(triangleVerticesB);

                        // back face
                        triangleVerticesA[0] = blockVertices[6]; triangleVerticesA[1] = blockVertices[7]; triangleVerticesA[2] = blockVertices[2];
                        triangleVerticesB[0] = blockVertices[7]; triangleVerticesB[1] = blockVertices[3]; triangleVerticesB[2] = blockVertices[2];
                        callback(triangleVerticesA);
                        callback(triangleVerticesB);

                        // front face
                        triangleVerticesA[0] = blockVertices[0]; triangleVerticesA[1] = blockVertices[5]; triangleVerticesA[2] = blockVertices[4];
                        triangleVerticesB[0] = blockVertices[4]; triangleVerticesB[1] = blockVertices[1]; triangleVerticesB[2] = blockVertices[5];
                        callback(triangleVerticesA);
                        callback(triangleVerticesB);

                        // top face
                        triangleVerticesA[0] = blockVertices[6]; triangleVerticesA[1] = blockVertices[5]; triangleVerticesA[2] = blockVertices[7];
                        triangleVerticesB[0] = blockVertices[6]; triangleVerticesB[1] = blockVertices[4]; triangleVerticesB[2] = blockVertices[5];
                        callback(triangleVerticesA);
                        callback(triangleVerticesB);

                        // bottom face
                        triangleVerticesA[0] = blockVertices[0]; triangleVerticesA[1] = blockVertices[2]; triangleVerticesA[2] = blockVertices[3];
                        triangleVerticesB[0] = blockVertices[2]; triangleVerticesB[1] = blockVertices[3]; triangleVerticesB[2] = blockVertices[1];
                        callback(triangleVerticesA);
                        callback(triangleVerticesB);
                    }
                }
            }
        }
    }
}

