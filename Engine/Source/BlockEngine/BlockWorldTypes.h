#pragma once
#include "Engine/Common.h"

// engine type forward declarations
class BlockPalette;
class Entity;

// forward declarations of data types
class BlockWorld;
class BlockWorldSection;
class BlockWorldChunk;
class BlockWorldChunkRenderProxy;
class BlockWorldChunkCollisionShape;

// fixed limits
#define BLOCK_WORLD_MAX_LOD_LEVELS (3)

// block data type
typedef uint16 BlockWorldBlockType;
typedef uint8 BlockWorldBlockDataType;

#define BLOCK_WORLD_BLOCK_VALUE_COLORED_FLAG_BIT    (0x8000U)
#define BLOCK_WORLD_BLOCK_DATA_LIGHTING_MASK (0xF)
#define BLOCK_WORLD_BLOCK_DATA_LIGHTING_SHIFT (0)
#define BLOCK_WORLD_BLOCK_DATA_GET_LIGHTING(data) (((data) >> BLOCK_WORLD_BLOCK_DATA_LIGHTING_SHIFT) & BLOCK_WORLD_BLOCK_DATA_LIGHTING_MASK)
#define BLOCK_WORLD_BLOCK_DATA_SET_LIGHTING(data, lighting) (((data) & ~(BLOCK_WORLD_BLOCK_DATA_LIGHTING_MASK << BLOCK_WORLD_BLOCK_DATA_LIGHTING_SHIFT)) | (((lighting) & BLOCK_WORLD_BLOCK_DATA_LIGHTING_MASK) << BLOCK_WORLD_BLOCK_DATA_LIGHTING_SHIFT))
#define BLOCK_WORLD_BLOCK_DATA_ROTATION_MASK (0x3)
#define BLOCK_WORLD_BLOCK_DATA_ROTATION_SHIFT (6)
#define BLOCK_WORLD_BLOCK_DATA_GET_ROTATION(data) (((data) >> BLOCK_WORLD_BLOCK_DATA_ROTATION_SHIFT) & BLOCK_WORLD_BLOCK_DATA_ROTATION_MASK)
#define BLOCK_WORLD_BLOCK_DATA_SET_ROTATION(data, rotation) (((data) & ~(BLOCK_WORLD_BLOCK_DATA_ROTATION_MASK << BLOCK_WORLD_BLOCK_DATA_ROTATION_SHIFT)) | (((rotation) & BLOCK_WORLD_BLOCK_DATA_ROTATION_MASK) << BLOCK_WORLD_BLOCK_DATA_ROTATION_SHIFT))

// rotation enumeration
enum BLOCK_WORLD_BLOCK_ROTATION
{
    BLOCK_WORLD_BLOCK_ROTATION_NORTH,
    BLOCK_WORLD_BLOCK_ROTATION_EAST,
    BLOCK_WORLD_BLOCK_ROTATION_SOUTH,
    BLOCK_WORLD_BLOCK_ROTATION_WEST,
    NUM_BLOCK_WORLD_BLOCK_ROTATIONS,
};

// data tracked by block world for entities in the all-inclusive table
struct BlockWorldEntityHashTableData
{
    Entity *pEntity;
    BlockWorldSection *pSection;
};

// data tracked by either block world or containing section
struct BlockWorldEntityReference
{
    Entity *pEntity;
    uint32 EntityID;
    AABox BoundingBox;
    Sphere BoundingSphere;
};
