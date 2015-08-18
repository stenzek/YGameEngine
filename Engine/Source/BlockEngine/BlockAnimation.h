#pragma once
#include "Engine/Entity.h"
#include "BlockEngine/BlockWorld.h"
#include "BlockEngine/BlockWorldMesher.h"
#include "Engine/Physics/CollisionObject.h"
#include "Engine/Physics/CollisionShape.h"

class BlockAnimationBlockRenderProxy;

class BlockAnimation : public Entity
{
    DECLARE_ENTITY_TYPEINFO(BlockAnimation, Entity);
    DECLARE_OBJECT_NO_FACTORY(BlockAnimation);

public:
    BlockAnimation(const EntityTypeInfo *pTypeInfo = &s_typeInfo);
    ~BlockAnimation();

    // Creation method also specifies block world.
    static BlockAnimation *Create(BlockWorld *pBlockWorld, bool autoDespawn = true);

    // Create an animated block 'explosion'-type effect.
    bool CreatePhysicsBlock(const float3 &basePosition, BlockWorldBlockType blockValue, const float3 &forceVector, float despawnTime = 5.0f);
    bool CreatePhysicsBlock(int32 x, int32 y, int32 z, const float3 &forceVector, bool removeBlock = true, float despawnTime = 5.0f);

    // Create an animated block spawn event
    void CreateSpawnBlock(const float3 &sourcePosition, int32 x, int32 y, int32 z, BlockWorldBlockType blockValue, float spawnTime = 1.0f, bool setAfterSpawn = true);

    // events
    virtual void OnAddToWorld(World *pWorld) override;
    virtual void OnRemoveFromWorld(World *pWorld) override;

    // Update event
    virtual void Update(float timeSinceLastUpdate) override;

private:
    // rebuild all vertex arrays and recalculate bounds
    void RebuildRenderData();

    // properties
    const BlockPalette *m_pBlockPalette;
    BlockWorld *m_pBlockWorld;
    bool m_autoDespawn;
    bool m_renderDataUpdatePending;

    // block render proxy
    BlockAnimationBlockRenderProxy *m_pBlockRenderProxy;

    // physics block info
    struct PhysicsBlock
    {
        BlockWorldBlockType SourceValue;
        Transform LastTransform;
        float TimeRemaining;
        AABox BoundingBox;

        Physics::CollisionObject *pCollisionObject;
    };

    // physics block instances
    PODArray<PhysicsBlock *> m_physicsBlocks;

    // spawning block
    struct SpawningBlock
    {
        float3 SourcePosition;
        int32 BlockX, BlockY, BlockZ;
        BlockWorldBlockType BlockValue;
        float TimeRemaining;
        bool SetAfterSpawn;
    };

    // spawning block instances
    MemArray<SpawningBlock> m_spawningBlocks;
};

