#pragma once
#include "BlockEngine/BlockWorldTypes.h"
#include "BlockEngine/BlockWorldChunk.h"

class BlockWorldSection : public ReferenceCounted
{
public:
    enum LoadState
    {
        LoadState_Loaded,
        LoadState_Changed,
        LoadState_Generating,
        LoadState_Count,
    };

public:
    BlockWorldSection(BlockWorld *pWorld, int32 sectionX, int32 sectionY);
    ~BlockWorldSection();

    // accessors
    const BlockWorld *GetWorld() const { return m_pWorld; }
    const int32 GetSectionSize() const { return m_sectionSize; }
    const int32 GetChunkSize() const { return m_chunkSize; }
    const int32 GetSectionX() const { return m_sectionX; }
    const int32 GetSectionY() const { return m_sectionY; }
    const int32 GetLODLevels() const { return m_lodLevels; }
    const int32 GetBaseChunkX() const { return m_baseChunkX; }
    const int32 GetBaseChunkY() const { return m_baseChunkY; }
    const AABox &GetBoundingBox() const { return m_boundingBox; }
    const int32 GetLoadedLODLevel() const { return m_loadedLODLevel; }

    // changed state
    const bool IsChanged() const { return (m_loadState != LoadState_Loaded); }
    const LoadState GetLoadState() const { return m_loadState; }
    void SetLoadState(LoadState loadState) { m_loadState = loadState; }

    // initialization
    void Create(int32 minChunkZ = 0, int32 maxChunkZ = 0);
    bool LoadFromStream(ByteStream *pStream, int32 maxLODLevel);
    bool SaveToStream(ByteStream *pStream) const;

    // load any lods to the specified level
    bool LoadLODs(ByteStream *pStream, int32 maxLODLevel);

    // unload any lods below the specified lod
    void UnloadLODs(int32 lodLevel);

    // chunks - everything here uses relative coordinates
    const int32 GetChunkCountZ() const { return m_chunkCountZ; }
    const int32 GetChunkCount() const { return m_chunkCount; }
    const int32 GetMinChunkZ() const { return m_minChunkZ; }
    const int32 GetMaxChunkZ() const { return m_maxChunkZ; }

    // chunks
    bool GetChunkAvailability(int32 chunkX, int32 chunkY, int32 chunkZ) const;
    const BlockWorldChunk *GetChunk(int32 chunkX, int32 chunkY, int32 chunkZ) const;
    BlockWorldChunk *GetChunk(int32 chunkX, int32 chunkY, int32 chunkZ);
    const BlockWorldChunk *SafeGetChunk(int32 chunkX, int32 chunkY, int32 chunkZ) const;
    BlockWorldChunk *SafeGetChunk(int32 chunkX, int32 chunkY, int32 chunkZ);
    BlockWorldChunk *CreateChunk(int32 chunkX, int32 chunkY, int32 chunkZ);
    void DeleteChunk(int32 chunkX, int32 chunkY, int32 chunkZ);

    // render groups pending meshing
    const int32 GetChunksPendingMeshing() const { return m_chunksPendingMeshing; }
    void AddChunkPendingMeshing() { m_chunksPendingMeshing++; }
    void RemoveChunkPendingMeshing() { m_chunksPendingMeshing--; }

    // update all lods that depend on a block
    void UpdateChunkLODLevels(BlockWorldChunk *pChunk, int32 lodLevel, int32 blockX, int32 blockY, int32 blockZ);

    // rebuild lods for a chunk
    void RebuildLODsForChunk(int32 chunkX, int32 chunkY, int32 chunkZ);

    // rebuild all lods for this section
    void RebuildLODs(int32 lodCount);

    // enumerate chunks
    template<typename CALLBACK_TYPE>
    void EnumerateChunks(CALLBACK_TYPE callback) const
    {
        for (int32 i = 0; i < m_chunkCount; i++)
        {
            if (m_ppChunks[i] != nullptr)
                callback(m_ppChunks[i]);
        }
    }
    template<typename CALLBACK_TYPE>
    void EnumerateChunks(CALLBACK_TYPE callback)
    {
        for (int32 i = 0; i < m_chunkCount; i++)
        {
            if (m_ppChunks[i] != nullptr)
                callback(m_ppChunks[i]);
        }
    }
    
    // remove empty chunks
    void RemoveEmptyChunks();

    // add/remove entities, does not clean them up afterwards
    void AddEntity(Entity *pEntity);
    void MoveEntity(Entity *pEntity);
    void RemoveEntity(Entity *pEntity);

private:
    // chunk availability
    void SetChunkAvailability(int32 chunkX, int32 chunkY, int32 chunkZ, bool availability);

    // chunk array initializers
    int32 GetChunkArrayIndex(int32 chunkX, int32 chunkY, int32 chunkZ) const;
    void InitializeChunkArray(int32 minChunkZ, int32 maxChunkZ);
    void ResizeChunkArray(int32 minChunkZ, int32 maxChunkZ);

    // internal load procedure
    bool InternalLoadLODs(ByteStream *pStream, int32 maxLODLevel);

    // info
    BlockWorld *m_pWorld;
    int32 m_sectionSize;
    int32 m_chunkSize;
    int32 m_sectionX;
    int32 m_sectionY;
    int32 m_lodLevels;
    int32 m_baseChunkX;
    int32 m_baseChunkY;

    // state
    AABox m_boundingBox;
    int32 m_loadedLODLevel;
    int32 m_chunksPendingMeshing;

    // chunk storage
    int32 m_minChunkZ;
    int32 m_maxChunkZ;
    int32 m_chunkCountZ;
    int32 m_chunkCount;
    BitSet32 m_chunkAvailability;
    BlockWorldChunk **m_ppChunks;

    // entities in section, only for lod0
    MemArray<BlockWorldEntityReference> m_entities;

    // changed state
    LoadState m_loadState;
};

