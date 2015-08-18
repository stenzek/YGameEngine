#pragma once
#include "ResourceCompiler/Common.h"
#include "Engine/BlockMesh.h"
#include "Engine/BlockMeshVolume.h"
#include "Engine/Physics/CollisionShape.h"

class BlockPalette;

class XMLReader;
class XMLWriter;

class BlockMeshGenerator
{
    DeclareNonCopyable(BlockMeshGenerator);

public:
    BlockMeshGenerator();
    ~BlockMeshGenerator();

    const bool IsChanged() const { return m_changed; }

    const BlockPalette *GetBlockList() const { return m_pPalette; }

    const float GetScale() const { return m_scale; }
    const bool GetUseAmbientOcclusion() const { return m_useAmbientOcclusion; }
    const uint32 GetLODCount() const { return m_nLODLevels; }

    void SetScale(float scale);
    void SetUseAmbientOcclusion(bool enabled) { m_useAmbientOcclusion = enabled; }
    void SetCollisionShapeType(Physics::COLLISION_SHAPE_TYPE type) { m_collisionShapeType = type; }

    const BlockMeshVolume &GetMeshVolume(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD]; }

    const int3 &GetMeshMinCoordinates(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].GetMinCoordinates(); }
    const int3 &GetMeshMaxCoordinates(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].GetMaxCoordinates(); }
    const uint32 GetMeshWidth(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].GetWidth(); }
    const uint32 GetMeshLength(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].GetLength(); }
    const uint32 GetMeshHeight(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].GetHeight(); }
    const uint8 *GetMeshData(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].GetData(); }

    // calculate the bounding box for an lod
    AABox CalculateBoundingBox(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].CalculateBoundingBox(); }
    AABox CalculateActiveBoundingBox(uint32 LOD) const { DebugAssert(LOD < m_nLODLevels); return m_pLODLevels[LOD].CalculateActiveBoundingBox(); }

    bool Create(const BlockPalette *pPalette, float scale = 1.0f, Physics::COLLISION_SHAPE_TYPE collisionShapeType = Physics::COLLISION_SHAPE_TYPE_TRIANGLE_MESH);
    bool Compile(ByteStream *pOutputStream) const;

    bool LoadFromXML(const char *FileName, ByteStream *pStream);
    bool SaveToXML(ByteStream *pStream);

    // clear (set all blocks to default)
    void Clear();
    void Clear(uint32 LOD);

    // resize
    void Resize(uint32 LOD, const int3 &newMinCoordinates, const int3 &newMaxCoordinates);

    // centers the mesh
    void Center();
    void Center(uint32 LOD);

    // shrinks the mesh to the minimum size possible
    void Shrink();
    void Shrink(uint32 LOD);

    // move blocks
    void MoveBlock(uint32 LOD, const int3 &blockCoordinates, const int3 &moveDelta);
    void MoveBlocks(uint32 LOD, const int3 &selectionMin, const int3 &selectionMax, const int3 &moveDelta);

    // block management
    inline uint8 GetBlock(uint32 LOD, int32 x, int32 y, int32 z) const { return GetBlock(LOD, int3(x, y, z)); }
    inline void SetBlock(uint32 LOD, int32 x, int32 y, int32 z, BlockVolumeBlockType v) { SetBlock(LOD, int3(x, y, z), v); }
    uint8 GetBlock(uint32 LOD, const int3 &coords) const;
    void SetBlock(uint32 LOD, const int3 &coords, BlockVolumeBlockType v);

    // block list management
    void SetPalette(const BlockPalette *pPalette);

private:
    bool GetActiveCoordinatesRange(uint32 LOD, int3 *pMinActiveCoordinates, int3 *pMaxActiveCoordinates);

    bool m_changed;

    const BlockPalette *m_pPalette;
    float m_scale;
    bool m_useAmbientOcclusion;
    Physics::COLLISION_SHAPE_TYPE m_collisionShapeType;

    BlockMeshVolume *m_pLODLevels;
    uint32 m_nLODLevels;
};
