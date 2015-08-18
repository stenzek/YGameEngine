#include "BlockEngine/PrecompiledHeader.h"
#include "BlockEngine/BlockWorldGenerator.h"

BlockWorldGenerator::BlockWorldGenerator(BlockWorld *pBlockWorld)
    : m_pBlockWorld(pBlockWorld)
{

}

BlockWorldGenerator::~BlockWorldGenerator()
{

}

BlockWorldBlockType BlockWorldGenerator::LookupBlockTypeByName(const char *name)
{
    for (uint32 i = 0; i < BLOCK_MESH_MAX_BLOCK_TYPES; i++)
    {
        const BlockPalette::BlockType *pBlockType = m_pBlockWorld->GetPalette()->GetBlockType(i);
        if (pBlockType->IsAllocated && pBlockType->Name.CompareInsensitive(name))
            return (BlockWorldBlockType)i;
    }

    return 0;
}

bool BlockWorldGenerator::CanGenerateSection(int32 sectionX, int32 sectionY) const
{
    return false;
}

bool BlockWorldGenerator::GetZRange(int32 startX, int32 startY, int32 endX, int32 endY, int32 *minBlockZ, int32 *maxBlockZ) const
{
    return false;
}

bool BlockWorldGenerator::GenerateBlocks(int32 startX, int32 startY, int32 endX, int32 endY) const
{
    return false;
}
