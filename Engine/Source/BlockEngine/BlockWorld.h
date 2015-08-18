#pragma once
#include "Engine/World.h"
#include "Engine/BlockPalette.h"
#include "BlockEngine/BlockWorldTypes.h"
#include "BlockEngine/BlockDrawTemplate.h"

class BlockWorldSection;
class BlockWorldChunk;
class BlockWorldGenerator;
class BlockDrawTemplate;
class FIFVolume;

namespace Physics { class RigidBody; }

class BlockWorld : public World
{
public:
    BlockWorld();
    virtual ~BlockWorld();

    // world entity methods
    virtual void AddBrush(Brush *pObject) override;
    virtual void RemoveBrush(Brush *pObject) override;
    virtual const Entity *GetEntityByID(uint32 EntityId) const override;
    virtual Entity *GetEntityByID(uint32 EntityId) override;
    virtual void AddEntity(Entity *pEntity) override;
    virtual void MoveEntity(Entity *pEntity) override;
    virtual void RemoveEntity(Entity *pEntity) override;
    virtual void UpdateEntity(Entity *pEntity) override;

    // world update methods
    virtual void BeginFrame(float deltaTime) override;
    virtual void UpdateAsync(float deltaTime) override;
    virtual void Update(float deltaTime) override;
    virtual void EndFrame() override;

    // world creation/loading method
    bool Create(const char *basePath, const BlockPalette *pPalette, int32 chunkSize, int32 sectionSize, int32 lodLevels);
    bool Load(const char *basePath);

    // accessors
    const BlockPalette *GetPalette() const { return m_pPalette; }
    const BlockDrawTemplate *GetBlockDrawTemplate() const { return m_pBlockDrawTemplate; }
    const int32 GetChunkSize() const { return m_chunkSize; }
    const int32 GetSectionSize() const { return m_sectionSize; }
    const int32 GetLODLevels() const { return m_lodLevels; }
    const int32 GetRenderGroupsPerSectionXY() const { return m_renderGroupsPerSectionXY; }

    // size queries
    int32 GetMinSectionX() const { return m_minSectionX; }
    int32 GetMinSectionY() const { return m_minSectionY; }
    int32 GetMaxSectionX() const { return m_maxSectionX; }
    int32 GetMaxSectionY() const { return m_maxSectionY; }
    int32 GetSectionCount() const { return m_sectionCount; }
    int32 GetSectionCountX() const { return m_sectionCountX; }
    int32 GetSectionCountY() const { return m_sectionCountY; }

    // helper functions for determining block visibility
    bool IsVisibilityBlockingBlockValue(BlockWorldBlockType blockValue, CUBE_FACE checkFace = CUBE_FACE_COUNT) const;
    bool IsLightBlockingBlockValue(BlockWorldBlockType blockValue, CUBE_FACE checkFace = CUBE_FACE_COUNT) const;

    // helper function to calculate tile bounds
    static AABox CalculateSectionBoundingBox(int32 sectionSize, int32 chunkSize, int32 sectionX, int32 sectionY);
    static AABox CalculateSectionBoundingBox(int32 sectionSize, int32 chunkSize, int32 sectionX, int32 sectionY, int32 minChunkZ, int32 maxChunkZ);

    // helper function to calculate chunk bounds
    static AABox CalculateChunkBoundingBox(int32 chunkSize, int32 chunkX, int32 chunkY, int32 chunkZ);
    static AABox CalculateChunkBoundingBox(int32 chunkSize, int32 sectionSize, int32 sectionX, int32 sectionY, int32 lodLevel, int32 relativeChunkX, int32 relativeChunkY, int32 relativeChunkZ);

    // helper converters
    static void SplitCoordinates(int32 *sectionX, int32 *sectionY, int32 *localChunkX, int32 *localChunkY, int32 *localChunkZ, int32 *localX, int32 *localY, int32 *localZ, int32 chunkSize, int32 sectionSize, int32 bx, int32 by, int32 bz);
    static void SplitCoordinates(int32 *chunkX, int32 *chunkY, int32 *chunkZ, int32 *localX, int32 *localY, int32 *localZ, int32 chunkSize, int32 bx, int32 by, int32 bz);

    // convert local chunk block coordinates to global coordinates
    static void ConvertChunkBlockCoordinatesToGlobalCoordinates(int32 chunkSize, int32 chunkX, int32 chunkY, int32 chunkZ, int32 blockX, int32 blockY, int32 blockZ, int32 *pGlobalBlockX, int32 *pGlobalBlockY, int32 *pGlobalBlockZ);

    // global chunk coordinates to section + local chunk coordinates
    static void CalculateRelativeChunkCoordinates(int32 *sectionX, int32 *sectionY, int32 *relativeChunkX, int32 *relativeChunkY, int32 *relativeChunkZ, int32 sectionSize, int32 chunkSize, int32 chunkX, int32 chunkY, int32 chunkZ);

    // coordinate splitter
    void SplitCoordinates(int32 *sectionX, int32 *sectionY, int32 *localChunkX, int32 *localChunkY, int32 *localChunkZ, int32 *localX, int32 *localY, int32 *localZ, int32 bx, int32 by, int32 bz) const;
    void SplitCoordinates(int32 *chunkX, int32 *chunkY, int32 *chunkZ, int32 *localX, int32 *localY, int32 *localZ, int32 bx, int32 by, int32 bz) const;

    // global chunk coordinates to section + local chunk coordinates
    void CalculateRelativeChunkCoordinates(int32 *sectionX, int32 *sectionY, int32 *relativeChunkX, int32 *relativeChunkY, int32 *relativeChunkZ, int32 chunkX, int32 chunkY, int32 chunkZ) const;

    // helper function to calculate the section of a specified position
    void CalculateSectionForPosition(int32 *sx, int32 *sy, const float3 &position) const;

    // helper function to find the global chunk coordinates for a position
    void CalculateChunkForPosition(int32 *cx, int32 *cy, int32 *cz, const float3 &position) const;

    // helper function to calculate a global point from a position
    int3 CalculatePointForPosition(const float3 &position) const;

    // tile availability
    void GetSectionStatus(bool *available, bool *loaded, int32 sectionX, int32 sectionY, int32 requiredLODLevel = Y_INT32_MAX) const;
    bool IsSectionAvailable(int32 sectionX, int32 sectionY) const;
    bool IsSectionLoaded(int32 sectionX, int32 sectionY, int32 requiredLODLevel = Y_INT32_MAX) const;

    // chunk availablity
    void GetChunkStatus(bool *available, bool *loaded, int32 chunkX, int32 chunkY, int32 chunkZ, int32 requiredLODLevel) const;
    bool IsChunkAvailable(int32 chunkX, int32 chunkY, int32 chunkZ) const;
    bool IsChunkLoaded(int32 chunkX, int32 chunkY, int32 chunkZ, int32 requiredLODLevel = Y_INT32_MAX) const;

    // pending chunk count accessor (really block groups when transitioning)
    uint32 GetPendingChunkCount() const { return m_pendingChunks.GetSize(); }
    uint32 GetChunksMeshingInProgress() const { return m_chunksMeshingInProgress; }
    uint32 GetLoadedSectionCount() const { return m_loadedSections.GetSize(); }

    // tile accessors
    const BlockWorldSection *GetSection(int32 sectionX, int32 sectionY) const;
    BlockWorldSection *GetSection(int32 sectionX, int32 sectionY);

    // chunk accessors
    const BlockWorldChunk *GetChunk(int32 chunkX, int32 chunkY, int32 chunkZ) const;
    BlockWorldChunk *GetChunk(int32 chunkX, int32 chunkY, int32 chunkZ);

    // generator
    const BlockWorldGenerator *GetGenerator() const { return m_pGenerator; }
    BlockWorldGenerator *GetGenerator() { return m_pGenerator; }
    void SetGenerator(BlockWorldGenerator *pGenerator) { m_pGenerator = pGenerator; }

    // ensures that all sections are loaded (loads all lods)
    bool LoadAllSections(int32 maxLODLevel = 0, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // unloads all sections
    void UnloadAllSections();

    // save changes to any sections
    bool SaveChangedSections();

    // can this chunk be modified? (ie checks if we are the boundary chunk on a section and checks for neighbour sections)
    bool IsChunkNeighboursLoaded(const BlockWorldChunk *pChunk, int32 lodLevel) const;
    bool EnsureChunkNeighboursLoaded(const BlockWorldChunk *pChunk, int32 lodLevel);

    // create a new section where it doesn't exist
    BlockWorldSection *CreateSection(int32 sectionX, int32 sectionY, int32 minChunkZ, int32 maxChunkZ);
    void DeleteSection(int32 sectionX, int32 sectionY);
    void DeleteAllSections();

    // expanded version
    const BlockWorldBlockType GetBlockValue(int32 bx, int32 by, int32 bz) const;
    bool ClearBlock(int32 bx, int32 by, int32 bz);
    bool SetBlockType(int32 bx, int32 by, int32 bz, BlockWorldBlockType blockType, BLOCK_WORLD_BLOCK_ROTATION blockRotation = BLOCK_WORLD_BLOCK_ROTATION_NORTH, bool createNonExistantChunks = false);
    bool SetBlockColor(int32 bx, int32 by, int32 bz, uint32 blockColor, bool createNonExistantChunks = false);

    // block-only raycast, this will hit blocks that aren't cubes as well
    bool RayCastBlock(const Ray &ray, int32 *pBlockX, int32 *pBlockY, int32 *pBlockZ, CUBE_FACE *pBlockFace, BlockWorldBlockType *pBlockValue, float *pDistance) const;
    bool RayCastBlock(const float3 &origin, const float3 &direction, float maxDistance, int32 *pBlockX, int32 *pBlockY, int32 *pBlockZ, CUBE_FACE *pBlockFace, BlockWorldBlockType *pBlockValue, float *pDistance) const { return RayCastBlock(Ray(origin, direction, maxDistance), pBlockX, pBlockY, pBlockZ, pBlockFace, pBlockValue, pDistance); }

    // helpers
    static bool IsValidChunkSize(uint32 chunkSize, uint32 sectionSize, uint32 lodCount);
    static int32 GetSectionArrayIndex(int32 sectionX, int32 sectionY, int32 minSectionX, int32 minSectionY, int32 maxSectionX, int32 maxSectionY);
    int32 GetSectionArrayIndex(int32 sectionX, int32 sectionY) const;

    // Create an animated block 'explosion'-type effect.
    bool CreateAnimatedPhysicsBlock(const float3 &basePosition, const Quaternion &rotation, BlockWorldBlockType blockValue, const float3 &forceVector, float despawnTime = 5.0f);
    bool CreateAnimatedPhysicsBlock(int32 x, int32 y, int32 z, const float3 &forceVector, bool removeBlock = true, float despawnTime = 5.0f);

    // create a block animation
    bool CreateBlockAnimation(BlockWorldBlockType blockValue, BLOCK_WORLD_BLOCK_ROTATION blockRotation, const Transform &startTransform, const Transform &endTransform, float spawnTime = 1.0f, EasingFunction::Type easingFunction = EasingFunction::Linear, bool setAfterSpawn = false, int32 setBlockX = 0, int32 setBlockY = 0, int32 setBlockZ = 0);

    // Create an animated block spawn event
    void SetBlockWithAnimation(int32 blockX, int32 blockY, int32 blockZ, BlockWorldBlockType blockValue, BLOCK_WORLD_BLOCK_ROTATION blockRotation, const Transform &startTransform, float spawnTime = 1.0f, EasingFunction::Type easingFunction = EasingFunction::Linear, bool setAfterSpawn = true);
    void SetBlockWithAnimation(int32 blockX, int32 blockY, int32 blockZ, BlockWorldBlockType blockValue, BLOCK_WORLD_BLOCK_ROTATION blockRotation);

private:
    // helper function to determine world bounds
    AABox CalculateBlockWorldBoundingBox() const;

    // file access
    ByteStream *OpenWorldFile(const char *fileName, uint32 access);

    // index initialization
    bool AllocateIndex();
    bool LoadIndex();
    bool SaveIndex();

    // global entity load/save
    bool LoadGlobalEntities();
    bool SaveGlobalEntities();

    // rebuild section array
    void ResizeIndex(int32 newMinSectionX, int32 newMinSectionY, int32 newMaxSectionX, int32 newMaxSectionY);

    // section loading/unloading
    bool LoadSection(int32 sectionX, int32 sectionY, int32 lodLevel, bool unloadHigherLODs);
    void UnloadSection(int32 sectionX, int32 sectionY);
    bool SaveSection(int32 sectionX, int32 sectionY);
    bool SaveSection(BlockWorldSection *pSection);

    // call when a section/chunk is created or loaded
    void OnChunkLoaded(BlockWorldSection *pSection, BlockWorldChunk *pChunk);
    void OnChunkUnloaded(BlockWorldSection *pSection, BlockWorldChunk *pChunk);

    // helper for calculating lod of section
    int32 CalculateSectionLODForViewer(int32 sectionX, int32 sectionY, const float3 &viewerPosition);

    // load any sections that are now in-range
    void LoadNewInRangeSections();

    // unload any sections that are out-of-range
    void UnloadOutOfRangeSections(float timeSinceLastUpdate);

    // stream in/out any needed sections
    void StreamSections(float timeSinceLastUpdate);

    // update the lod level of currently-loaded chunks
    void TransitionLoadedChunkRenderLODs();

    // remove chunks from the specified lod level/render group from the world
    void RemoveChunkMesh(BlockWorldChunk *pChunk);

    // remove all meshes from the specified section at the specified lod
    void RemoveSectionMeshes(BlockWorldSection *pSection);

    // queue a chunk for meshing
    void QueueSingleChunkForMeshing(BlockWorldChunk *pChunk, int32 lodLevel);

    // call once per frame, sorts queued chunks according to distance from viewers
    void SortPendingMeshChunks();

    // mesh a single chunk, returns true if any triangles are to be rendered
    void MeshSingleChunk(BlockWorldSection *pSection, BlockWorldChunk *pChunk, int32 lodLevel);

    // call once per frame, meshes queued chunks
    void ProcessPendingMeshChunks();

    // chunk creator
    BlockWorldChunk *CreateChunk(int32 chunkX, int32 chunkY, int32 chunkZ);
    BlockWorldChunk *GetWritableChunk(int32 chunkX, int32 chunkY, int32 chunkZ, bool allowCreate = true);

    // helper functions for block setting
    void OnBlockChanged(BlockWorldChunk *pChunk, int32 lx, int32 ly, int32 lz);
    void OnEdgeBlockChanged(BlockWorldChunk *pChunk, int32 lx, int32 ly, int32 lz);
    bool ClearBlockBit(int32 x, int32 y, int32 z, BlockWorldBlockType bit);
    bool SetBlockBit(int32 x, int32 y, int32 z, BlockWorldBlockType bit);

    // block lighting update helper
    friend struct BlockLightingUpdateNode;
    void SpreadBlockLighting(BlockWorldChunk *pChunk, int32 x, int32 y, int32 z, uint8 lightLevel);
    void UnspreadBlockLighting(BlockWorldChunk *pChunk, int32 x, int32 y, int32 z);

    // so it can access the entity lookup hash table
    friend BlockWorldSection;
    void OnLoadEntity(Entity *pEntity);
    void OnUnloadEntity(Entity *pEntity);

    // all entities hash table
    typedef HashTable<uint32, Entity *> EntityHashTable;
    EntityHashTable m_entityHashTable;

    // global entities, ie entities that are global, or a section does not exist for them
    typedef MemArray<BlockWorldEntityReference> GlobalEntityReferenceArray;
    GlobalEntityReferenceArray m_globalEntityReferences;

    // settings
    String m_basePath;
    FIFVolume *m_pFIFVolume;
    const BlockPalette *m_pPalette;
    BlockDrawTemplate *m_pBlockDrawTemplate;
    int32 m_chunkSize;
    int32 m_sectionSize;
    int32 m_lodLevels;
    int32 m_sectionSizeInBlocks;
    int32 m_renderGroupsPerSectionXY;

    // section storage
    int32 m_minSectionX;
    int32 m_minSectionY;
    int32 m_maxSectionX;
    int32 m_maxSectionY;
    int32 m_sectionCountX;
    int32 m_sectionCountY;
    int32 m_sectionCount;
    BlockWorldSection **m_ppSections;
    BitSet32 m_availableSectionMask;

    // for faster iteration when unloading/streaming, we store loaded sections here, todo store x/y for cache efficiency
    PODArray<BlockWorldSection *> m_loadedSections;

    // sections queued to load
    struct QueuedSectionLoad 
    {
        int32 SectionX, SectionY;
        int32 LODLevel;
        int32 ViewRange;
    };
    MemArray<QueuedSectionLoad> m_queuedLoadSections;

    // chunk or chunk render group that is pending meshing
    struct PendingMeshingChunk
    {
        BlockWorldSection *pSection;
        BlockWorldChunk *pChunk;
        float3 ViewCenter;
        float MinimumViewDistance;
        int32 OldLODLevel;
        int32 NewLODLevel;
    };
    MemArray<PendingMeshingChunk> m_pendingChunks;
    uint32 m_chunksMeshingInProgress;

    // generator
    BlockWorldGenerator *m_pGenerator;

    // block animation
    struct BlockAnimation
    {
        // Common.. make this use less memory perhaps? not like there will be a lot of them...
        BlockWorldBlockType BlockType;
        int32 LocationX, LocationY, LocationZ;
        BLOCK_WORLD_BLOCK_ROTATION Rotation;
        BlockDrawTemplate::BlockRenderProxy *pRenderProxy;
        Transform LastTransform;
        float LifeTime;
        float TimeRemaining;

        // Physics-only
        Physics::RigidBody *pRigidBody;

        // Spawn-only
        EasingFunction::Type AnimationFunction;
        Transform AnimationStartTransform;
        Transform AnimationEndTransform;
        bool SetAfterCompletion;
    };
    MemArray<BlockAnimation> m_blockAnimations;
    BlockWorldBlockType m_animationInProgressBlockType;
    void UpdateBlockAnimations(float deltaTime);
};
