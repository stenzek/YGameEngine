#pragma once
#include "BlockEngine/BlockWorldTypes.h"

namespace Physics { class StaticObject; }

class BlockWorldChunk
{
    friend class BlockWorld;

public:
    enum MeshState
    {
        MeshState_Idle,
        MeshState_Pending,
        MeshState_InProgress,
        MeshState_InProgressWithChanges
    };

public:
    BlockWorldChunk(BlockWorldSection *pSection, int32 relativeChunkX, int32 relativeChunkY, int32 relativeChunkZ);
    ~BlockWorldChunk();

    BlockWorldSection *GetSection() { return m_pSection; }
    const BlockWorldSection *GetSection() const { return m_pSection; }
    const int32 GetChunkSize() const { return m_chunkSize; }
    const int32 GetLoadedLODLevel() const { return m_loadedLODLevel; }
    const int32 GetRenderLODLevel() const { return m_renderLODLevel; }
    const int32 GetRelativeChunkX() const { return m_relativeChunkX; }
    const int32 GetRelativeChunkY() const { return m_relativeChunkY; }
    const int32 GetRelativeChunkZ() const { return m_relativeChunkZ; }
    const int32 GetGlobalChunkX() const { return m_globalChunkX; }
    const int32 GetGlobalChunkY() const { return m_globalChunkY; }
    const int32 GetGlobalChunkZ() const { return m_globalChunkZ; }
    const float3 &GetBasePosition() const { return m_basePosition; }
    const AABox &GetBoundingBox() const { return m_boundingBox; }

    void SetRenderLODLevel(int32 lodLevel) { m_renderLODLevel = lodLevel; }

    // serialization
    void Create();
    bool LoadFromStream(int32 lodLevel, ByteStream *pStream);
    bool SaveToStream(int32 lodLevel, ByteStream *pStream);
    void UnloadLODLevel(int32 lodLevel);

    // chunk data is indexed by (bz * CHUNKSIZE * CHUNKHEIGHT) + (by * CHUNKSIZE) + bx
    const BlockWorldBlockType *GetBlockValues(int32 lodLevel) const { return m_pBlockValues[lodLevel]; }
    const BlockWorldBlockDataType *GetBlockData(int32 lodLevel) const { return m_pBlockData[lodLevel]; }
    BlockWorldBlockType *GetBlockValues(int32 lodLevel) { return m_pBlockValues[lodLevel]; }
    BlockWorldBlockDataType *GetBlockData(int32 lodLevel) { return m_pBlockData[lodLevel]; }
    int32 GetZStride(int32 lodLevel) { return m_zStride[lodLevel]; }

    // check if the chunk is empty
    bool IsAirChunk() const;

    // manipulators
    BlockWorldBlockType GetBlock(int32 lodLevel, int32 bx, int32 by, int32 bz) const;
    void SetBlock(int32 lodLevel, int32 bx, int32 by, int32 bz, BlockWorldBlockType blockType);

    // data manipulators
    BlockWorldBlockDataType GetBlockData(int32 lodLevel, int32 bx, int32 by, int32 bz) const;
    void SetBlockData(int32 lodLevel, int32 bx, int32 by, int32 bz, BlockWorldBlockDataType blockType);

    // lighting manipulators
    uint8 GetBlockLight(int32 lodLevel, int32 bx, int32 by, int32 bz) const;
    void SetBlockLight(int32 lodLevel, int32 bx, int32 by, int32 bz, uint8 lightLevel);

    // rotation manipulators
    uint8 GetBlockRotation(int32 lodLevel, int32 bx, int32 by, int32 bz) const;
    void SetBlockRotation(int32 lodLevel, int32 bx, int32 by, int32 bz, uint8 rotation);

    // update the lods for a particular block
    void UpdateLODs(int32 lodLevel, int32 blockX, int32 blockY, int32 blockZ);

    // mesh pending flag
    MeshState GetMeshState() const { return m_meshState; }
    bool IsMeshPending() const { return (m_meshState != MeshState_Idle); }
    void SetMeshState(MeshState state) { DebugAssert(state <= MeshState_InProgress); m_meshState = state; }

    // collision object
    BlockWorldChunkCollisionShape *GetCollisionShape() { return m_pCollisionShape; }
    Physics::StaticObject *GetCollisionObject() { return m_pCollisionObject; }

    // render object, when setting this takes the reference
    BlockWorldChunkRenderProxy *GetRenderProxy() { return m_pRenderProxy; }
    void SetRenderProxy(BlockWorldChunkRenderProxy *pRenderProxy) { m_pRenderProxy = pRenderProxy; }

private:
    BlockWorldSection *m_pSection;
    int32 m_chunkSize;
    int32 m_loadedLODLevel;
    int32 m_renderLODLevel;
    int32 m_relativeChunkX;
    int32 m_relativeChunkY;
    int32 m_relativeChunkZ;
    int32 m_globalChunkX;
    int32 m_globalChunkY;
    int32 m_globalChunkZ;
    float3 m_basePosition;
    AABox m_boundingBox;

    // chunk data is indexed by (bz * CHUNKSIZE * CHUNKHEIGHT) + (by * CHUNKSIZE) + bx
    BlockWorldBlockType *m_pBlockValues[BLOCK_WORLD_MAX_LOD_LEVELS];
    BlockWorldBlockDataType *m_pBlockData[BLOCK_WORLD_MAX_LOD_LEVELS];
    int32 m_zStride[BLOCK_WORLD_MAX_LOD_LEVELS];
   
    // physics object for this chunk
    BlockWorldChunkCollisionShape *m_pCollisionShape;
    Physics::StaticObject *m_pCollisionObject;

    // render object for this chunk
    BlockWorldChunkRenderProxy *m_pRenderProxy;

    // mesh pending flag
    MeshState m_meshState;
};
