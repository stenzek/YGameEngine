#include "BlockEngine/PrecompiledHeader.h"
#include "BlockEngine/BlockWorldChunk.h"
#include "BlockEngine/BlockWorldSection.h"
#include "BlockEngine/BlockWorldMesher.h"
#include "BlockEngine/BlockWorldChunkRenderProxy.h"
#include "BlockEngine/BlockWorldChunkCollisionShape.h"
#include "BlockEngine/BlockWorld.h"
#include "Engine/Physics/StaticObject.h"
#include "Renderer/Renderer.h"
Log_SetChannel(BlockWorldChunk);

BlockWorldChunk::BlockWorldChunk(BlockWorldSection *pSection, int32 relativeChunkX, int32 relativeChunkY, int32 relativeChunkZ)
    : m_pSection(pSection),
      m_chunkSize(pSection->GetChunkSize()),
      m_loadedLODLevel(pSection->GetWorld()->GetLODLevels()),
      m_renderLODLevel(pSection->GetWorld()->GetLODLevels()),
      m_relativeChunkX(relativeChunkX),
      m_relativeChunkY(relativeChunkY),
      m_relativeChunkZ(relativeChunkZ),
      m_globalChunkX(pSection->GetBaseChunkX() + relativeChunkX),
      m_globalChunkY(pSection->GetBaseChunkY() + relativeChunkY),
      m_globalChunkZ(relativeChunkZ),
      m_pCollisionShape(nullptr),
      m_pCollisionObject(nullptr),
      m_pRenderProxy(nullptr),
      m_meshState(MeshState_Idle)
{
    // calculate base position, or translation
    m_basePosition.Set(static_cast<float>((pSection->GetBaseChunkX() + relativeChunkX) * m_chunkSize),
                       static_cast<float>((pSection->GetBaseChunkY() + relativeChunkY) * m_chunkSize),
                       static_cast<float>(relativeChunkZ * m_chunkSize));

    // calculate bounding box -- fixme for LOD
    m_boundingBox = BlockWorld::CalculateChunkBoundingBox(m_chunkSize, m_globalChunkX, m_globalChunkY, m_globalChunkZ);

    // combine into one allocation
    Y_memzero(m_pBlockValues, sizeof(m_pBlockValues));
    Y_memzero(m_pBlockData, sizeof(m_pBlockData));
    Y_memzero(m_zStride, sizeof(m_zStride));

    // allocate collision shape and object
    m_pCollisionShape = new BlockWorldChunkCollisionShape(pSection->GetWorld()->GetPalette(), m_chunkSize, this);
    m_pCollisionObject = new Physics::StaticObject(0, m_pCollisionShape, Transform(m_basePosition, Quaternion::Identity, float3::One));
}

BlockWorldChunk::~BlockWorldChunk()
{
    if (m_pRenderProxy != nullptr)
        m_pRenderProxy->Release();

    // kill all data levels
    for (int32 i = BLOCK_WORLD_MAX_LOD_LEVELS - 1; i >= 0; i--)
    {
        delete[] m_pBlockData[i];
        delete[] m_pBlockValues[i];
    }

    m_pCollisionObject->Release();
    m_pCollisionShape->Release();
}

void BlockWorldChunk::Create()
{
    int32 lodLevels = m_pSection->GetWorld()->GetLODLevels();

    for (int32 lodLevel = 0; lodLevel < lodLevels; lodLevel++)
    {
        // allocate block values, data and set z stride
        int32 blockCount = (m_chunkSize >> lodLevel) * (m_chunkSize >> lodLevel) * (m_chunkSize >> lodLevel);
        m_pBlockValues[lodLevel] = new BlockWorldBlockType[blockCount];
        Y_memzero(m_pBlockValues[lodLevel], sizeof(BlockWorldBlockType) * blockCount);
        m_pBlockData[lodLevel] = new BlockWorldBlockDataType[blockCount];
        Y_memzero(m_pBlockData[lodLevel], sizeof(BlockWorldBlockDataType) * blockCount);
        m_zStride[lodLevel] = (m_chunkSize >> lodLevel) * (m_chunkSize >> lodLevel);
    }

    // has everything loaded to start with
    m_loadedLODLevel = 0;
}

bool BlockWorldChunk::LoadFromStream(int32 lodLevel, ByteStream *pStream)
{
    // allocate block values, data and set z stride
    int32 blockCount = (m_chunkSize >> lodLevel) * (m_chunkSize >> lodLevel) * (m_chunkSize >> lodLevel);
    if (m_pBlockValues[lodLevel] == nullptr)
    {
        DebugAssert(m_pBlockData[lodLevel] == nullptr && m_zStride[lodLevel] == 0);
        m_pBlockValues[lodLevel] = new BlockWorldBlockType[blockCount];
        m_pBlockData[lodLevel] = new BlockWorldBlockDataType[blockCount];
        m_zStride[lodLevel] = (m_chunkSize >> lodLevel) * (m_chunkSize >> lodLevel);
    }

    // read in block values, then data
    if (!pStream->Read2(m_pBlockValues[lodLevel], sizeof(BlockWorldBlockType) * blockCount) ||
        !pStream->Read2(m_pBlockData[lodLevel], sizeof(BlockWorldBlockDataType) * blockCount))
    {
        //delete[] m_pBlockData[lodLevel];
        //m_pBlockData[lodLevel] = nullptr;
        //delete[] m_pBlockValues[lodLevel];
        //m_pBlockValues[lodLevel] = nullptr;
        //m_zStride[lodLevel] = 0;
        return false;
    }

    // update loaded level
    m_loadedLODLevel = Min(m_loadedLODLevel, lodLevel);
    return true;
}

bool BlockWorldChunk::SaveToStream(int32 lodLevel, ByteStream *pStream)
{
    int32 blockCount = (m_chunkSize >> lodLevel) * (m_chunkSize >> lodLevel) * (m_chunkSize >> lodLevel);
    DebugAssert(m_pBlockValues[lodLevel] != nullptr);

    // write block values, then data
    if (!pStream->Write2(m_pBlockValues[lodLevel], sizeof(BlockWorldBlockType) * blockCount) ||
        !pStream->Write2(m_pBlockData[lodLevel], sizeof(BlockWorldBlockDataType) * blockCount))
    {
        return false;
    }

    return true;
}

void BlockWorldChunk::UnloadLODLevel(int32 lodLevel)
{
    delete[] m_pBlockData[lodLevel];
    m_pBlockData[lodLevel] = nullptr;

    delete[] m_pBlockValues[lodLevel];
    m_pBlockValues[lodLevel] = nullptr;

    m_zStride[lodLevel] = 0;

    // is this the highest lod level? update the loaded level
    if (lodLevel == m_loadedLODLevel)
    {
        int32 lodLevels = m_pSection->GetWorld()->GetLODLevels();
        for (m_loadedLODLevel = lodLevel + 1; m_loadedLODLevel < lodLevels; m_loadedLODLevel++)
        {
            if (m_pBlockValues[m_loadedLODLevel] != nullptr)
                break;
        }
    }
}

bool BlockWorldChunk::IsAirChunk() const
{
    uint32 blockCount = m_chunkSize * m_chunkSize * m_chunkSize;
    for (uint32 i = 0; i < blockCount; i++)
    {
        if (m_pBlockValues[i] != 0)
            return false;
    }

    return true;
}

BlockWorldBlockType BlockWorldChunk::GetBlock(int32 lodLevel, int32 bx, int32 by, int32 bz) const
{
    DebugAssert(bx < (m_chunkSize >> lodLevel) && by < (m_chunkSize >> lodLevel) && bz < (m_chunkSize >> lodLevel));
    return m_pBlockValues[lodLevel][(bz * m_zStride[lodLevel]) + (by * (m_chunkSize >> lodLevel)) + bx];
}

void BlockWorldChunk::SetBlock(int32 lodLevel, int32 bx, int32 by, int32 bz, BlockWorldBlockType blockType)
{
    DebugAssert(bx < (m_chunkSize >> lodLevel) && by < (m_chunkSize >> lodLevel) && bz < (m_chunkSize >> lodLevel));
    DebugAssert((blockType & BLOCK_WORLD_BLOCK_VALUE_COLORED_FLAG_BIT) != 0 || blockType < BLOCK_MESH_MAX_BLOCK_TYPES);
    uint32 index = (bz * m_zStride[lodLevel]) + (by * (m_chunkSize >> lodLevel)) + bx;
    if (m_pBlockValues[lodLevel][index] == blockType)
        return;

    m_pBlockValues[lodLevel][index] = blockType;
}

BlockWorldBlockDataType BlockWorldChunk::GetBlockData(int32 lodLevel, int32 bx, int32 by, int32 bz) const
{
    DebugAssert(bx < (m_chunkSize >> lodLevel) && by < (m_chunkSize >> lodLevel) && bz < (m_chunkSize >> lodLevel));
    return m_pBlockData[lodLevel][(bz * m_zStride[lodLevel]) + (by * (m_chunkSize >> lodLevel)) + bx];
}

void BlockWorldChunk::SetBlockData(int32 lodLevel, int32 bx, int32 by, int32 bz, BlockWorldBlockDataType blockType)
{
    DebugAssert(bx < (m_chunkSize >> lodLevel) && by < (m_chunkSize >> lodLevel) && bz < (m_chunkSize >> lodLevel));

    uint32 index = (bz * m_zStride[lodLevel]) + (by * (m_chunkSize >> lodLevel)) + bx;
    if (m_pBlockData[lodLevel][index] == blockType)
        return;

    m_pBlockData[lodLevel][index] = blockType;
}

uint8 BlockWorldChunk::GetBlockLight(int32 lodLevel, int32 bx, int32 by, int32 bz) const
{
    DebugAssert(bx < (m_chunkSize >> lodLevel) && by < (m_chunkSize >> lodLevel) && bz < (m_chunkSize >> lodLevel));
    return BLOCK_WORLD_BLOCK_DATA_GET_LIGHTING(m_pBlockData[lodLevel][(bz * m_zStride[lodLevel]) + (by * (m_chunkSize >> lodLevel)) + bx]);
}

void BlockWorldChunk::SetBlockLight(int32 lodLevel, int32 bx, int32 by, int32 bz, uint8 lightLevel)
{
    DebugAssert(bx < (m_chunkSize >> lodLevel) && by < (m_chunkSize >> lodLevel) && bz < (m_chunkSize >> lodLevel));

    uint32 index = (bz * m_zStride[lodLevel]) + (by * (m_chunkSize >> lodLevel)) + bx;
    uint8 data = m_pBlockData[lodLevel][index];
    if (BLOCK_WORLD_BLOCK_DATA_GET_LIGHTING(data) == lightLevel)
        return;

    m_pBlockData[lodLevel][index] = BLOCK_WORLD_BLOCK_DATA_SET_LIGHTING(data, lightLevel);
}

uint8 BlockWorldChunk::GetBlockRotation(int32 lodLevel, int32 bx, int32 by, int32 bz) const
{
    DebugAssert(bx < (m_chunkSize >> lodLevel) && by < (m_chunkSize >> lodLevel) && bz < (m_chunkSize >> lodLevel));
    return BLOCK_WORLD_BLOCK_DATA_GET_ROTATION(m_pBlockData[lodLevel][(bz * m_zStride[lodLevel]) + (by * (m_chunkSize >> lodLevel)) + bx]);
}

void BlockWorldChunk::SetBlockRotation(int32 lodLevel, int32 bx, int32 by, int32 bz, uint8 rotation)
{
    DebugAssert(bx < (m_chunkSize >> lodLevel) && by < (m_chunkSize >> lodLevel) && bz < (m_chunkSize >> lodLevel));

    uint32 index = (bz * m_zStride[lodLevel]) + (by * (m_chunkSize >> lodLevel)) + bx;
    uint8 data = m_pBlockData[lodLevel][index];
    if (BLOCK_WORLD_BLOCK_DATA_GET_ROTATION(data) == rotation)
        return;

    m_pBlockData[lodLevel][index] = BLOCK_WORLD_BLOCK_DATA_SET_ROTATION(data, rotation);
}

void BlockWorldChunk::UpdateLODs(int32 lodLevel, int32 blockX, int32 blockY, int32 blockZ)
{
    // should be at lod 0
    int32 lodLevels = m_pSection->GetWorld()->GetLODLevels();
    DebugAssert(m_loadedLODLevel == 0);

    // skip if the next level is this level
    if (lodLevel == (lodLevels - 1))
        return;

    // gather the 8 blocks
    int32 baseBlockX = blockX & ~1;
    int32 baseBlockY = blockY & ~1;
    int32 baseBlockZ = blockZ & ~1;
    BlockWorldBlockType blocks[8];
    blocks[0] = GetBlock(lodLevel, baseBlockX + 0, baseBlockY + 0, baseBlockZ + 0);
    blocks[1] = GetBlock(lodLevel, baseBlockX + 1, baseBlockY + 0, baseBlockZ + 0);
    blocks[2] = GetBlock(lodLevel, baseBlockX + 0, baseBlockY + 1, baseBlockZ + 0);
    blocks[3] = GetBlock(lodLevel, baseBlockX + 1, baseBlockY + 1, baseBlockZ + 0);
    blocks[4] = GetBlock(lodLevel, baseBlockX + 0, baseBlockY + 0, baseBlockZ + 1);
    blocks[5] = GetBlock(lodLevel, baseBlockX + 1, baseBlockY + 0, baseBlockZ + 1);
    blocks[6] = GetBlock(lodLevel, baseBlockX + 0, baseBlockY + 1, baseBlockZ + 1);
    blocks[7] = GetBlock(lodLevel, baseBlockX + 1, baseBlockY + 1, baseBlockZ + 1);
    BlockWorldBlockDataType blockData[8];
    blockData[0] = GetBlockData(lodLevel, baseBlockX + 0, baseBlockY + 0, baseBlockZ + 0);
    blockData[1] = GetBlockData(lodLevel, baseBlockX + 1, baseBlockY + 0, baseBlockZ + 0);
    blockData[2] = GetBlockData(lodLevel, baseBlockX + 0, baseBlockY + 1, baseBlockZ + 0);
    blockData[3] = GetBlockData(lodLevel, baseBlockX + 1, baseBlockY + 1, baseBlockZ + 0);
    blockData[4] = GetBlockData(lodLevel, baseBlockX + 0, baseBlockY + 0, baseBlockZ + 1);
    blockData[5] = GetBlockData(lodLevel, baseBlockX + 1, baseBlockY + 0, baseBlockZ + 1);
    blockData[6] = GetBlockData(lodLevel, baseBlockX + 0, baseBlockY + 1, baseBlockZ + 1);
    blockData[7] = GetBlockData(lodLevel, baseBlockX + 1, baseBlockY + 1, baseBlockZ + 1);

    //Log_DevPrintf("BLOCK SET %i [%i,%i,%i] (base %i %i %i) [[[%u]]]", lodLevel, blockX, blockY, blockZ, baseBlockX, baseBlockY, baseBlockZ, pChunk->GetBlock(blockX, blockY, blockZ));

    // do average operation fixme
    BlockWorldBlockType lodBlockValue = blocks[0];
    BlockWorldBlockDataType lodBlockData = blockData[0];
    for (uint32 i = 0; i < 8; i++)
    {
        if (blocks[i] != 0)
        {
            lodBlockValue = blocks[i];
            lodBlockData = blockData[i];
            break;
        }
    }
    //if (lodBlockValue == 0)
        //Log_WarningPrintf("no block found");

    // get next level chunk coordinates
    int32 nextLevel = lodLevel + 1;
    int32 nextLevelBlockX = baseBlockX / 2;
    int32 nextLevelBlockY = baseBlockY / 2;
    int32 nextLevelBlockZ = baseBlockZ / 2;
    //Log_DevPrintf("  SET LOD %i BLOCK %i,%i,%i -> %u", lodLevel + 1, nextLevelBlockX, nextLevelBlockY, nextLevelBlockZ, lodBlockValue);

    // set the next level block
    SetBlock(nextLevel, nextLevelBlockX, nextLevelBlockY, nextLevelBlockZ, lodBlockValue);
    SetBlockData(nextLevel, nextLevelBlockX, nextLevelBlockY, nextLevelBlockZ, lodBlockData);

    // update the next level of detail
    UpdateLODs(lodLevel + 1, nextLevelBlockX, nextLevelBlockY, nextLevelBlockZ);
}
