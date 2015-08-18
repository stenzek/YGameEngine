#pragma once
#include "BlockEngine/BlockWorld.h"

class BlockWorldGenerator
{
public:
    BlockWorldGenerator(BlockWorld *pBlockWorld);
    virtual ~BlockWorldGenerator();

    const BlockWorld *GetBlockWorld() const { return m_pBlockWorld; }
    BlockWorld *GetBlockWorld() { return m_pBlockWorld; }

    BlockWorldBlockType LookupBlockTypeByName(const char *name);

    virtual bool CanGenerateSection(int32 sectionX, int32 sectionY) const;
    virtual bool GetZRange(int32 startX, int32 startY, int32 endX, int32 endY, int32 *minBlockZ, int32 *maxBlockZ) const;
    virtual bool GenerateBlocks(int32 startX, int32 startY, int32 endX, int32 endY) const;

protected:
    // enumerate chunk coordinates for a specified section, returns global chunk coordinates
    template<typename T>
    void EnumerateChunksInSection(int32 sectionX, int32 sectionY, T callback) const
    {
        int32 baseChunkX = sectionX * m_pBlockWorld->GetSectionSize();
        int32 baseChunkY = sectionY * m_pBlockWorld->GetSectionSize();

        for (int32 chunkX = 0; chunkX < m_pBlockWorld->GetSectionSize(); chunkX++)
        {
            for (int32 chunkY = 0; chunkY < m_pBlockWorld->GetSectionSize(); chunkY++)
            {
                callback(baseChunkX + chunkX, baseChunkY + chunkY);
            }
        }
    }

protected:
    BlockWorld *m_pBlockWorld;
};
