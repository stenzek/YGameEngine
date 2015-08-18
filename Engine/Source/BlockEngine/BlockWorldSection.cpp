#include "BlockEngine/PrecompiledHeader.h"
#include "BlockEngine/BlockWorldSection.h"
#include "BlockEngine/BlockWorldChunk.h"
#include "BlockEngine/BlockWorld.h"
#include "Engine/Entity.h"
#include "Core/ClassTable.h"
Log_SetChannel(BlockWorldSection);

BlockWorldSection::BlockWorldSection(BlockWorld *pWorld, int32 sectionX, int32 sectionY)
    : m_pWorld(pWorld),
      m_sectionSize(pWorld->GetSectionSize()),
      m_chunkSize(pWorld->GetChunkSize()),
      m_sectionX(sectionX),
      m_sectionY(sectionY),
      m_lodLevels(pWorld->GetLODLevels()),
      m_baseChunkX(sectionX * m_sectionSize),
      m_baseChunkY(sectionY * m_sectionSize),
      m_boundingBox(BlockWorld::CalculateSectionBoundingBox(m_sectionSize, m_chunkSize, sectionX, sectionY)),
      m_loadedLODLevel(m_lodLevels),
      m_chunksPendingMeshing(0),
      m_minChunkZ(0),
      m_maxChunkZ(0),
      m_chunkCountZ(0),
      m_chunkCount(0),
      m_ppChunks(nullptr),
      m_loadState(LoadState_Loaded)
{

}

BlockWorldSection::~BlockWorldSection()
{
    while (m_entities.GetSize() > 0)
    {
        Entity *pEntity = m_entities[m_entities.GetSize() - 1].pEntity;
        m_entities.RemoveBack();

        // unload entity
        pEntity->OnRemoveFromWorld(m_pWorld);
        m_pWorld->OnUnloadEntity(pEntity);
        pEntity->Release();
    }

    for (int32 i = 0; i < m_chunkCount; i++)
    {
        BlockWorldChunk *pChunk = m_ppChunks[i];
        if (pChunk != nullptr)
        {
            m_pWorld->OnChunkUnloaded(this, pChunk);
            delete pChunk;
        }
    }

    delete[] m_ppChunks;
}

int32 BlockWorldSection::GetChunkArrayIndex(int32 chunkX, int32 chunkY, int32 chunkZ) const
{
    DebugAssert(chunkX >= 0 && chunkX < m_sectionSize);
    DebugAssert(chunkY >= 0 && chunkY < m_sectionSize);
    DebugAssert(chunkZ >= m_minChunkZ && chunkZ <= m_maxChunkZ);

    const int32 yStride = m_sectionSize;
    const int32 zStride = m_sectionSize * yStride;

    return (chunkZ - m_minChunkZ) * zStride + (chunkY * yStride) + chunkX;
}

void BlockWorldSection::InitializeChunkArray(int32 minChunkZ, int32 maxChunkZ)
{
    DebugAssert(minChunkZ <= maxChunkZ);
    m_minChunkZ = minChunkZ;
    m_maxChunkZ = maxChunkZ;
    m_chunkCountZ = (m_maxChunkZ - m_minChunkZ) + 1;
    m_chunkCount = m_sectionSize * m_sectionSize * m_chunkCountZ;
    m_ppChunks = new BlockWorldChunk *[m_chunkCount];
    Y_memzero(m_ppChunks, sizeof(BlockWorldChunk *) * m_chunkCount);
    m_chunkAvailability.Resize(m_chunkCount);
    m_chunkAvailability.Clear();
}

void BlockWorldSection::ResizeChunkArray(int32 minChunkZ, int32 maxChunkZ)
{
    DebugAssert(minChunkZ <= maxChunkZ);

    int32 newChunkCountZ = (maxChunkZ - minChunkZ) + 1;
    int32 newChunkCount = m_sectionSize * m_sectionSize * newChunkCountZ;
    int32 newZStride = m_sectionSize * m_sectionSize;
    DebugAssert(newChunkCount > 0);

    BitSet32 newChunkAvailability;
    newChunkAvailability.Resize(newChunkCount);
    newChunkAvailability.Clear();

    BlockWorldChunk **ppChunks = new BlockWorldChunk *[newChunkCount];
    Y_memzero(ppChunks, sizeof(BlockWorldChunk *) * newChunkCount);

    uint32 chunkIndex = 0;
    for (int32 z = m_minChunkZ; z <= m_maxChunkZ; z++)
    {
        for (int32 y = 0; y < m_sectionSize; y++)
        {
            for (int32 x = 0; x < m_sectionSize; x++)
            {
                if (!m_chunkAvailability.IsSet(chunkIndex))
                {
                    chunkIndex++;
                    continue;
                }

                if (z >= minChunkZ && z <= maxChunkZ)
                {
                    // determine new chunk index
                    uint32 newChunkIndex = newZStride * (z - minChunkZ) + (y * m_sectionSize) + x;
                    ppChunks[newChunkIndex] = m_ppChunks[chunkIndex];
                    newChunkAvailability.Set(newChunkIndex);
                }

                chunkIndex++;
            }
        }
    }

    // swap everything over
    delete[] m_ppChunks;
    m_minChunkZ = minChunkZ;
    m_maxChunkZ = maxChunkZ;
    m_chunkCountZ = newChunkCountZ;
    m_chunkCount = newChunkCount;
    m_chunkAvailability.Swap(newChunkAvailability);
    m_ppChunks = ppChunks;
}

bool BlockWorldSection::GetChunkAvailability(int32 chunkX, int32 chunkY, int32 chunkZ) const
{
    if (chunkZ < m_minChunkZ || chunkZ > m_maxChunkZ)
        return false;

    int32 arrayIndex = GetChunkArrayIndex(chunkX, chunkY, chunkZ);
    DebugAssert(arrayIndex >= 0 && arrayIndex < m_chunkCount);
    return m_chunkAvailability.IsSet((uint32)arrayIndex);
}

void BlockWorldSection::SetChunkAvailability(int32 chunkX, int32 chunkY, int32 chunkZ, bool availability)
{
    int32 arrayIndex = GetChunkArrayIndex(chunkX, chunkY, chunkZ);
    DebugAssert(arrayIndex >= 0 && arrayIndex < m_chunkCount);
    if (availability)
        m_chunkAvailability.Set((uint32)arrayIndex);
    else
        m_chunkAvailability.Unset((uint32)arrayIndex);
}

const BlockWorldChunk *BlockWorldSection::GetChunk(int32 chunkX, int32 chunkY, int32 chunkZ) const
{
    int32 arrayIndex = GetChunkArrayIndex(chunkX, chunkY, chunkZ);
    DebugAssert(arrayIndex >= 0 && arrayIndex < m_chunkCount);
    return m_ppChunks[arrayIndex];
}

BlockWorldChunk *BlockWorldSection::GetChunk(int32 chunkX, int32 chunkY, int32 chunkZ)
{
    int32 arrayIndex = GetChunkArrayIndex(chunkX, chunkY, chunkZ);
    DebugAssert(arrayIndex >= 0 && arrayIndex < m_chunkCount);
    return m_ppChunks[arrayIndex];
}

const BlockWorldChunk *BlockWorldSection::SafeGetChunk(int32 chunkX, int32 chunkY, int32 chunkZ) const
{
    if (chunkZ < m_minChunkZ || chunkZ > m_maxChunkZ)
        return nullptr;

    int32 arrayIndex = GetChunkArrayIndex(chunkX, chunkY, chunkZ);
    DebugAssert(arrayIndex >= 0 && arrayIndex < m_chunkCount);
    return m_ppChunks[arrayIndex];
}

BlockWorldChunk *BlockWorldSection::SafeGetChunk(int32 chunkX, int32 chunkY, int32 chunkZ)
{
    if (chunkZ < m_minChunkZ || chunkZ > m_maxChunkZ)
        return nullptr;

    int32 arrayIndex = GetChunkArrayIndex(chunkX, chunkY, chunkZ);
    DebugAssert(arrayIndex >= 0 && arrayIndex < m_chunkCount);
    return m_ppChunks[arrayIndex];
}

BlockWorldChunk *BlockWorldSection::CreateChunk(int32 chunkX, int32 chunkY, int32 chunkZ)
{
    // we should be at lod 0
    DebugAssert(m_loadedLODLevel == 0);

    // ensure the chunk array can fit
    if (chunkZ < m_minChunkZ || chunkZ > m_maxChunkZ)
        ResizeChunkArray(Min(chunkZ, m_minChunkZ), Max(chunkZ, m_maxChunkZ));

    // shouldn't be allocated
    DebugAssert(!GetChunkAvailability(chunkX, chunkY, chunkZ));

    // allocate chunk
    BlockWorldChunk *pChunk = new BlockWorldChunk(this, chunkX, chunkY, chunkZ);
    pChunk->Create();

    // store chunk
    int32 arrayIndex = GetChunkArrayIndex(chunkX, chunkY, chunkZ);
    DebugAssert(arrayIndex >= 0 && arrayIndex < m_chunkCount);
    m_ppChunks[arrayIndex] = pChunk;
    m_chunkAvailability.Set(arrayIndex);

    // flag as changed
    if (m_loadState == LoadState_Loaded)
        m_loadState = LoadState_Changed;

    return pChunk;
}

void BlockWorldSection::DeleteChunk(int32 chunkX, int32 chunkY, int32 chunkZ)
{
    DebugAssert(m_loadedLODLevel == 0);
    if (chunkZ < m_minChunkZ || chunkZ > m_maxChunkZ)
        return;

    uint32 chunkIndex = GetChunkArrayIndex(chunkX, chunkY, chunkZ);
    if (!m_chunkAvailability.IsSet(chunkIndex))
        return;

    delete m_ppChunks[chunkIndex];
    m_ppChunks[chunkIndex] = nullptr;
    m_chunkAvailability.Unset(chunkIndex);
    if (m_loadState == LoadState_Loaded)
        m_loadState = LoadState_Changed;
}

void BlockWorldSection::Create(int32 minChunkZ /* = 0 */, int32 maxChunkZ /* = 0 */)
{
    InitializeChunkArray(minChunkZ, maxChunkZ);
    m_loadedLODLevel = 0;
    m_loadState = LoadState_Changed;
}

bool BlockWorldSection::LoadFromStream(ByteStream *pStream, int32 maxLODLevel)
{
    BinaryReader binaryReader(pStream);

    uint32 signature;
    if (!binaryReader.SafeReadUInt32(&signature) || signature != 0xCCBBAA03)
        return false;

    int32 chunkSize;
    int32 sectionSize;
    int32 lodLevels;
    if (!binaryReader.SafeReadInt32(&chunkSize) || chunkSize != m_chunkSize ||
        !binaryReader.SafeReadInt32(&sectionSize) || sectionSize != m_sectionSize ||
        !binaryReader.SafeReadInt32(&lodLevels) || lodLevels != m_lodLevels)
    {
        return false;
    }

    // read min, max
    int32 minChunkZ, maxChunkZ;
    if (!binaryReader.SafeReadInt32(&minChunkZ) ||
        !binaryReader.SafeReadInt32(&maxChunkZ))
    {
        return false;
    }

    // allocate array
    InitializeChunkArray(minChunkZ, maxChunkZ);

    // read mask size
    uint32 maskDwordCount;
    if (!binaryReader.SafeReadUInt32(&maskDwordCount) || maskDwordCount != m_chunkAvailability.GetDWordCount())
        return false;

    // read mask bits
    if (!pStream->Read2(m_chunkAvailability.GetPointer(), sizeof(uint32) * maskDwordCount))
        return false;

    // load the actual data
    return InternalLoadLODs(pStream, maxLODLevel);
}

bool BlockWorldSection::LoadLODs(ByteStream *pStream, int32 maxLODLevel)
{
    BinaryReader binaryReader(pStream);

    uint32 signature;
    if (!binaryReader.SafeReadUInt32(&signature) || signature != 0xCCBBAA03)
        return false;

    int32 chunkSize;
    int32 sectionSize;
    int32 lodLevels;
    if (!binaryReader.SafeReadInt32(&chunkSize) || chunkSize != m_chunkSize ||
        !binaryReader.SafeReadInt32(&sectionSize) || sectionSize != m_sectionSize ||
        !binaryReader.SafeReadInt32(&lodLevels) || lodLevels != m_lodLevels)
    {
        return false;
    }

    // read min, max
    int32 minChunkZ, maxChunkZ;
    if (!binaryReader.SafeReadInt32(&minChunkZ) || m_minChunkZ != minChunkZ ||
        !binaryReader.SafeReadInt32(&maxChunkZ) || m_maxChunkZ != maxChunkZ)
    {
        return false;
    }

    // read mask size
    uint32 maskDwordCount;
    if (!binaryReader.SafeReadUInt32(&maskDwordCount) || maskDwordCount != m_chunkAvailability.GetDWordCount())
        return false;

    // skip mask bits
    if (!pStream->SeekRelative(sizeof(uint32) * maskDwordCount))
        return false;

    // load the actual data
    return InternalLoadLODs(pStream, maxLODLevel);
}

bool BlockWorldSection::InternalLoadLODs(ByteStream *pStream, int32 maxLODLevel)
{
    BinaryReader binaryReader(pStream);

    // read in the offsets to each lod
    uint32 *lodOffsets = (uint32 *)alloca(sizeof(uint32) * m_lodLevels);
    if (!binaryReader.SafeReadBytes(lodOffsets, sizeof(uint32) * m_lodLevels))
        return false;

    // read in the offset to the entities
    uint32 entitiesOffset, entityCount;
    if (!binaryReader.SafeReadUInt32(&entitiesOffset) || !binaryReader.SafeReadUInt32(&entityCount))
        return false;

    // start at the bottom level (first in file), and stop when we reach the requested level
    for (int32 lodLevel = (m_lodLevels - 1); lodLevel >= maxLODLevel; lodLevel--)
    {
        // if it's already loaded, skip it
        if (lodLevel > m_loadedLODLevel)
            continue;

        // seek to offset
        if (!binaryReader.SafeSeekAbsolute((uint64)lodOffsets[lodLevel]))
            return false;

        // read chunks
        int32 chunkIndex = 0;
        for (int32 z = m_minChunkZ; z <= m_maxChunkZ; z++)
        {
            for (int32 y = 0; y < m_sectionSize; y++)
            {
                for (int32 x = 0; x < m_sectionSize; x++)
                {
                    int32 thisChunkIndex = chunkIndex++;
                    if (!m_chunkAvailability[thisChunkIndex])
                        continue;

                    BlockWorldChunk *pChunk = m_ppChunks[thisChunkIndex];
                    if (pChunk == nullptr)
                    {
                        pChunk = new BlockWorldChunk(this, x, y, z);
                        if (!pChunk->LoadFromStream(lodLevel, pStream))
                        {
                            delete pChunk;
                            return false;
                        }

                        m_ppChunks[thisChunkIndex] = pChunk;
                        m_pWorld->OnChunkLoaded(this, pChunk);
                    }
                    else
                    {
                        if (!pChunk->LoadFromStream(lodLevel, pStream))
                            return false;
                    }
                }
            }
        }
    }

    // update loaded level
    uint32 oldLODLevel = m_loadedLODLevel;
    m_loadedLODLevel = maxLODLevel;

    // if we loaded lod level 0, load entities
    if (oldLODLevel > 0 && maxLODLevel == 0 && entityCount > 0)
    {
        DebugAssert(m_entities.GetSize() == 0);

        // move to the position of the class table and entities
        if (!pStream->SeekAbsolute(entitiesOffset))
            return false;

        // read in class table
        ClassTable *pClassTable = new ClassTable;
        if (!pClassTable->LoadFromStream(pStream))
        {
            Log_ErrorPrintf("BlockWorldSection::LoadLODs: Failed to load class table.");
            return false;
        }

        // read entities
        for (uint32 i = 0; i < entityCount; i++)
        {
            uint32 typeIndex, entitySize;
            if (!binaryReader.SafeReadUInt32(&typeIndex) || !binaryReader.SafeReadUInt32(&entitySize))
            {
                delete pClassTable;
                return false;
            }

            // calculate offset to next entity
            uint32 nextEntityOffset = (uint32)pStream->GetPosition() + entitySize;

            // load the object
            Object *pObject = pClassTable->UnserializeObject(pStream, typeIndex);
            if (pObject != nullptr)
            {
                // should be an entity
                if (pObject->IsDerived(OBJECT_TYPEINFO(Entity)))
                {
                    // add it to the world, the reference will be moved across
                    Entity *pEntity = pObject->Cast<Entity>();
                    pEntity->OnAddToWorld(m_pWorld);
                    m_pWorld->OnLoadEntity(pEntity);
                    AddEntity(pEntity);
                }
                else
                {
                    Log_ErrorPrintf("BlockWorldSection::LoadLODs: Loaded bad entity object that is not derived from Entity.");
                    pObject->Release();
                }
            }

            // seek to next entity
            if (!pStream->SeekAbsolute(nextEntityOffset))
            {
                delete pClassTable;
                return false;
            }
        }

        // drop the class table
        delete pClassTable;
    }

    return true;
}

void BlockWorldSection::UnloadLODs(int32 lodLevel)
{
    // sanity checks here
    DebugAssert(lodLevel >= m_loadedLODLevel);

    // if the render lod is currently higher than the new lod level, we need to move all the chunks down to the new level,
    // or the easiest fix is to just nuke them if they're too high lod. this should only occur if we're lagging.
    {
        bool logged = false;

        // unload lod from all chunks
        for (int32 i = 0; i < m_chunkCount; i++)
        {
            BlockWorldChunk *pChunk = m_ppChunks[i];
            if (pChunk != nullptr && pChunk->GetRenderLODLevel() < lodLevel)
            {
                if (!logged)
                {
                    Log_WarningPrintf("BlockWorldSection::UnloadLODs: Unloading section [%i, %i] with render LODs at higher level of detail. This will be visually jarring until the chunks are re-meshed.", m_sectionX, m_sectionY);
                    logged = true;
                }

                m_pWorld->RemoveChunkMesh(pChunk);
            }
        }
    }

    // unload all levels below lodLevel
    for (int32 currentLODLevel = (lodLevel - 1); currentLODLevel >= m_loadedLODLevel; currentLODLevel--)
    {
        // unload entities if unloading lod0
        if (currentLODLevel == 0)
        {
            while (m_entities.GetSize() > 0)
            {
                Entity *pEntity = m_entities[m_entities.GetSize() - 1].pEntity;
                m_entities.RemoveBack();

                // unload entity
                pEntity->OnRemoveFromWorld(m_pWorld);
                m_pWorld->OnUnloadEntity(pEntity);
                pEntity->Release();
            }
        }

        // unload lod from all chunks
        for (int32 i = 0; i < m_chunkCount; i++)
        {
            if (m_ppChunks[i] != nullptr)
                m_ppChunks[i]->UnloadLODLevel(currentLODLevel);
        }
    }

    // and update the loaded lod level
    m_loadedLODLevel = lodLevel;
}

bool BlockWorldSection::SaveToStream(ByteStream *pStream) const
{
    // we should be at lod0 if we're saving
    DebugAssert(m_loadedLODLevel == 0);

    // create writer
    BinaryWriter binaryWriter(pStream);
    bool writeResult = true;

    // write header
    writeResult &= binaryWriter.SafeWriteUInt32(0xCCBBAA03);
    writeResult &= binaryWriter.SafeWriteInt32(m_chunkSize);
    writeResult &= binaryWriter.SafeWriteInt32(m_sectionSize);
    writeResult &= binaryWriter.SafeWriteInt32(m_lodLevels);
    writeResult &= binaryWriter.SafeWriteInt32(m_minChunkZ);
    writeResult &= binaryWriter.SafeWriteInt32(m_maxChunkZ);

    // write bitmask
    writeResult &= binaryWriter.SafeWriteUInt32(m_chunkAvailability.GetDWordCount());
    writeResult &= binaryWriter.SafeWriteBytes(m_chunkAvailability.GetPointer(), sizeof(uint32) * m_chunkAvailability.GetDWordCount());

    // count available chunks
    int32 availableChunkCount = 0;
    for (int32 i = 0; i < m_chunkCount; i++)
    {
        if (m_chunkAvailability.IsSet(i))
            availableChunkCount++;
    }
    
    // calculate the offset to each lod level, and save it for checking later
    uint32 *lodLevelOffsets = (uint32 *)alloca(sizeof(uint32) * m_lodLevels);
    uint32 currentOffset = (uint32)binaryWriter.GetStreamPosition() + (sizeof(uint32) * m_lodLevels) + sizeof(uint32) + sizeof(uint32);
    for (int32 lodLevel = (m_lodLevels - 1); lodLevel >= 0; lodLevel--)
    {
        // save offset
        lodLevelOffsets[lodLevel] = currentOffset;

        // calculate size of this lod
        int32 chunkSizeInBlocks = m_chunkSize >> lodLevel;
        int32 chunkSizeInBytes = (chunkSizeInBlocks * chunkSizeInBlocks * chunkSizeInBlocks) * (sizeof(BlockWorldBlockType) + sizeof(BlockWorldBlockDataType));
        currentOffset += chunkSizeInBytes * availableChunkCount;
    }

    // write the offsets
    writeResult &= binaryWriter.SafeWriteBytes(lodLevelOffsets, sizeof(uint32) * m_lodLevels);

    // write entity offsets and counts
    writeResult &= binaryWriter.SafeWriteUInt32(currentOffset);
    writeResult &= binaryWriter.SafeWriteUInt32(m_entities.GetSize());

    // now save each lod level's data
    for (int32 lodLevel = (m_lodLevels - 1); lodLevel >= 0; lodLevel--)
    {
        DebugAssert(binaryWriter.GetStreamPosition() == lodLevelOffsets[lodLevel]);

        int32 chunkIndex = 0;
        for (int32 z = m_minChunkZ; z <= m_maxChunkZ; z++)
        {
            for (int32 y = 0; y < m_sectionSize; y++)
            {
                for (int32 x = 0; x < m_sectionSize; x++)
                {
                    int32 thisChunkIndex = chunkIndex++;
                    if (!m_chunkAvailability[thisChunkIndex])
                        continue;

                    BlockWorldChunk *pChunk = m_ppChunks[thisChunkIndex];
                    DebugAssert(pChunk != nullptr);

                    if (!pChunk->SaveToStream(lodLevel, pStream))
                    {
                        Log_ErrorPrintf("Failed to write section %i,%i chunk %i,%i,%i", m_sectionX, m_sectionY, x, y, z);
                        return false;
                    }
                }
            }
        }
    }
    
    // should be equal
    DebugAssert(pStream->GetPosition() == currentOffset);

    // write entities
    if (m_entities.GetSize() > 0)
    {
        // allocate class table
        ClassTable *pClassTable = new ClassTable();

        // create a growable stream for temporary use, because unfortunately we have to write the class table first,
        // which we don't know about until we actually write entities, and then the entity size is unknown until written
        ByteStream *pMemoryStream = ByteStream_CreateGrowableMemoryStream();

        // write entities
        for (const BlockWorldEntityReference &entityRef : m_entities)
        {
            uint32 entityHeaderOffset = (uint32)pMemoryStream->GetPosition();
            const Entity *pEntity = entityRef.pEntity;

            // entity in type list? if not, add it
            int32 typeIndex = pClassTable->GetTypeMappingForType(pEntity->GetObjectTypeInfo());
            if (typeIndex < 0)
                typeIndex = pClassTable->AddType(pEntity->GetObjectTypeInfo());

            // start with a size of zero
            if (!binaryWriter.SafeWriteInt32(typeIndex) || !binaryWriter.SafeWriteUInt32(0))
            {
                pMemoryStream->Release();
                delete pClassTable;
                return false;
            }

            // write the actual entity
            uint32 entityStartOffset = (uint32)pMemoryStream->GetPosition();
            if (!pClassTable->SerializeObject(entityRef.pEntity, pStream))
            {
                Log_ErrorPrintf("Failed to write section %i,%i entity %u", m_sectionX, m_sectionY, entityRef.EntityID);
                pMemoryStream->Release();
                delete pClassTable;
                return false;
            }

            // now fix-up the size
            uint32 entityEndOffset = (uint32)pMemoryStream->GetPosition();
            if (!pMemoryStream->SeekAbsolute(entityHeaderOffset + sizeof(int32)) ||
                !binaryWriter.SafeWriteUInt32(entityEndOffset - entityStartOffset) ||
                !pMemoryStream->SeekAbsolute(entityEndOffset))
            {
                pMemoryStream->Release();
                delete pClassTable;
                return false;
            }
        }

        // write class table
        if (!pClassTable->SaveToStream(pStream))
        {
            pMemoryStream->Release();
            delete pClassTable;
            return false;
        }

        // write entities
        if (!ByteStream_AppendStream(pMemoryStream, pStream))
        {
            pMemoryStream->Release();
            delete pClassTable;
            return false;
        }

        // clean up
        pMemoryStream->Release();
        delete pClassTable;
    }

    return writeResult;
}

void BlockWorldSection::RemoveEmptyChunks()
{
    /*
    for (int32 i = 0; i < m_chunkCount; i++)
    {
        BlockWorldChunk *pChunk = m_ppChunks[i];
        if (pChunk != nullptr && pChunk->IsAirChunk())
        {
            Log_DevPrintf("Dropping air chunk %i, %i, %i (section %i, %i)", pChunk->GetRelativeChunkX(), pChunk->GetRelativeChunkY(), pChunk->GetRelativeChunkZ(), m_sectionX, m_sectionY);
            DeleteChunk(pChunk->GetRelativeChunkX(), pChunk->GetRelativeChunkY(), pChunk->GetRelativeChunkZ());
        }
    }
    */
}

void BlockWorldSection::UpdateChunkLODLevels(BlockWorldChunk *pChunk, int32 lodLevel, int32 blockX, int32 blockY, int32 blockZ)
{
    pChunk->UpdateLODs(lodLevel, blockX, blockY, blockZ);
}

void BlockWorldSection::RebuildLODsForChunk(int32 chunkX, int32 chunkY, int32 chunkZ)
{
    BlockWorldChunk *pChunk = GetChunk(chunkX, chunkY, chunkZ);
    DebugAssert(pChunk != nullptr);

    // update every 'block quad' for the chunk
    for (int32 blockZ = 0; blockZ < m_chunkSize; blockZ += 2)
    {
        for (int32 blockY = 0; blockY < m_chunkSize; blockY += 2)
        {
            for (int32 blockX = 0; blockX < m_chunkSize; blockX += 2)
                UpdateChunkLODLevels(pChunk, 0, blockX, blockY, blockZ);
        }
    }
}

void BlockWorldSection::RebuildLODs(int32 lodCount)
{
    // lod change? handle this
    DebugAssert(lodCount == m_lodLevels); 
    DebugAssert(m_loadedLODLevel == 0);
    
    // for each chunk, rebuild the lod
    for (int32 i = 0; i < m_chunkCount; i++)
    {
        BlockWorldChunk *pChunk = m_ppChunks[i];
        if (pChunk == nullptr)
            continue;

        // update every 'block quad' for the chunk
        for (int32 blockZ = 0; blockZ < m_chunkSize; blockZ += 2)
        {
            for (int32 blockY = 0; blockY < m_chunkSize; blockY += 2)
            {
                for (int32 blockX = 0; blockX < m_chunkSize; blockX += 2)
                    UpdateChunkLODLevels(pChunk, 0, blockX, blockY, blockZ);
            }
        }
    }
}

void BlockWorldSection::AddEntity(Entity *pEntity)
{
    // should only happen at lod0
    DebugAssert(m_loadedLODLevel == 0);

    // add ref
    BlockWorldEntityReference entityRef;
    entityRef.pEntity = pEntity;
    entityRef.EntityID = pEntity->GetEntityID();
    entityRef.BoundingBox = pEntity->GetBoundingBox();
    entityRef.BoundingSphere = pEntity->GetBoundingSphere();
    m_entities.Add(entityRef);
}

void BlockWorldSection::MoveEntity(Entity *pEntity)
{
    // should only happen at lod0
    DebugAssert(m_loadedLODLevel == 0);

    // search and remove
    for (uint32 idx = 0; idx < m_entities.GetSize(); idx++)
    {
        if (m_entities[idx].pEntity == pEntity)
        {
            m_entities[idx].BoundingBox = pEntity->GetBoundingBox();
            m_entities[idx].BoundingSphere = pEntity->GetBoundingSphere();
            return;
        }
    }
    Panic("entity not found in list");
}

void BlockWorldSection::RemoveEntity(Entity *pEntity)
{
    // should only happen at lod0
    DebugAssert(m_loadedLODLevel == 0);

    // search and remove
    for (uint32 idx = 0; idx < m_entities.GetSize(); idx++)
    {
        if (m_entities[idx].pEntity == pEntity)
        {
            m_entities.FastRemove(idx);
            return;
        }
    }
    Panic("entity not found in list");
}
