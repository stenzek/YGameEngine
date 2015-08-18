#include "BlockEngine/PrecompiledHeader.h"
#include "BlockEngine/BlockWorld.h"
#include "BlockEngine/BlockWorldSection.h"
#include "BlockEngine/BlockWorldChunk.h"
#include "BlockEngine/BlockWorldChunkRenderProxy.h"
#include "BlockEngine/BlockWorldMesher.h"
#include "BlockEngine/BlockEngineCVars.h"
#include "BlockEngine/BlockWorldGenerator.h"
#include "BlockEngine/BlockDrawTemplate.h"
#include "Engine/ResourceManager.h"
#include "Engine/Entity.h"
#include "Engine/Engine.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Physics/StaticObject.h"
#include "Engine/Physics/RigidBody.h"
#include "Engine/Physics/BoxCollisionShape.h"
#include "Renderer/RenderWorld.h"
#include "Core/FIFVolume.h"
Log_SetChannel(BlockWorld);

#define USE_FIF 1

static const char *BLOCK_WORLD_INDEX_FILE_NAME = "index.bin";
static const char *BLOCK_WORLD_GLOBAL_ENTITIES_FILE_NAME = "global_entities.bin";

BlockWorld::BlockWorld()
    : World(),
      m_pFIFVolume(nullptr),
      m_pPalette(nullptr),
      m_pBlockDrawTemplate(nullptr),
      m_chunkSize(0),
      m_sectionSize(0),
      m_lodLevels(0),
      m_sectionSizeInBlocks(0),
      m_minSectionX(0),
      m_minSectionY(0),
      m_maxSectionX(0),
      m_maxSectionY(0),
      m_sectionCountX(0),
      m_sectionCountY(0),
      m_sectionCount(0),
      m_ppSections(nullptr),
      m_chunksMeshingInProgress(0),
      m_pGenerator(nullptr)
{

}

BlockWorld::~BlockWorld()
{
    // clear out animations
    for (uint32 blockAnimationIndex = 0; blockAnimationIndex < m_blockAnimations.GetSize(); blockAnimationIndex++)
    {
        BlockAnimation *pAnimation = &m_blockAnimations[blockAnimationIndex];
        if (pAnimation->pRigidBody != nullptr)
        {
            m_pPhysicsWorld->RemoveObject(pAnimation->pRigidBody);
            pAnimation->pRigidBody->Release();
        }

        m_pRenderWorld->RemoveRenderable(pAnimation->pRenderProxy);
        pAnimation->pRenderProxy->Release();

        // todo: set block if it's still being animated?
    }

    delete m_pGenerator;
    SAFE_RELEASE(m_pBlockDrawTemplate);

    // TODO delete entities
    UnloadAllSections();

    for (int32 i = 0; i < m_sectionCount; i++)
        delete m_ppSections[i];
    delete[] m_ppSections;

    if (m_pPalette != nullptr)
        m_pPalette->Release();

#ifdef USE_FIF
    if (m_pFIFVolume != nullptr)
        delete m_pFIFVolume;
#endif
}

bool BlockWorld::IsValidChunkSize(uint32 chunkSize, uint32 sectionSize, uint32 lodCount)
{
    if (sectionSize <= 0 || chunkSize <= 0 || lodCount <= 0)
        return false;

    if (!Y_ispow2(chunkSize))
        return false;

    if ((chunkSize >> (lodCount - 1)) == 0 ||
        (sectionSize >> (lodCount - 1)) == 0)
    {
        return false;
    }

    return true;
}

int32 BlockWorld::GetSectionArrayIndex(int32 sectionX, int32 sectionY, int32 minSectionX, int32 minSectionY, int32 maxSectionX, int32 maxSectionY)
{
    return ((sectionY - minSectionY) * (maxSectionX - minSectionX)) + (sectionX - minSectionX);
}

int32 BlockWorld::GetSectionArrayIndex(int32 sectionX, int32 sectionY) const
{
    DebugAssert(sectionX >= m_minSectionX && sectionX <= m_maxSectionX && sectionY >= m_minSectionY && sectionY <= m_maxSectionY);
    return ((sectionY - m_minSectionY) * m_sectionCountX) + (sectionX - m_minSectionX);
}

bool BlockWorld::AllocateIndex()
{
    m_sectionSizeInBlocks = m_chunkSize * m_sectionSize;
    m_renderGroupsPerSectionXY = m_sectionSize >> (m_lodLevels - 1);
    m_sectionCountX = m_maxSectionX - m_minSectionX + 1;
    m_sectionCountY = m_maxSectionY - m_minSectionY + 1;
    m_sectionCount = m_sectionCountX * m_sectionCountY;

    // allocate bitset
    m_availableSectionMask.Resize((uint32)m_sectionCount);
    m_availableSectionMask.Clear();

    // and the array
    m_ppSections = new BlockWorldSection *[m_sectionCount];
    Y_memzero(m_ppSections, sizeof(BlockWorldSection *)* m_sectionCount);

    // update world bounds
    AABox blockWorldBoundingBox = CalculateBlockWorldBoundingBox();
    m_worldBoundingBox = blockWorldBoundingBox;
    m_worldBoundingSphere = Sphere::FromAABox(blockWorldBoundingBox);

    // create draw template
    if (g_pRenderer != nullptr)
    {
        m_pBlockDrawTemplate = new BlockDrawTemplate(m_pPalette);
        if (!m_pBlockDrawTemplate->Initialize())
            return false;
    }

    return true;
}

void BlockWorld::ResizeIndex(int32 newMinSectionX, int32 newMinSectionY, int32 newMaxSectionX, int32 newMaxSectionY)
{
    int32 newSectionCountX = (newMaxSectionX - newMinSectionX) + 1;
    int32 newSectionCountY = (newMaxSectionY - newMinSectionY) + 1;
    int32 newSectionCount = newSectionCountX * newSectionCountY;
    DebugAssert(newMinSectionX <= m_minSectionX && newMinSectionY <= m_minSectionY);
    DebugAssert(newMaxSectionX >= m_maxSectionX && newMaxSectionY >= m_maxSectionY);
    DebugAssert(newSectionCountX > 0 && newSectionCountY > 0);

    // build new bitset and array
    BitSet32 newAvailableSectionMask(newSectionCount);
    newAvailableSectionMask.Clear();
    BlockWorldSection **ppNewSectionArray = new BlockWorldSection *[newSectionCount];
    Y_memzero(ppNewSectionArray, sizeof(BlockWorldSection *) * newSectionCount);

    // fill new bitset/array
    for (int32 sectionX = m_minSectionX; sectionX <= m_maxSectionX; sectionX++)
    {
        for (int32 sectionY = m_minSectionY; sectionY <= m_maxSectionY; sectionY++)
        {
            int32 oldIndex = ((sectionY - m_minSectionY) * m_sectionCountX) + (sectionX - m_minSectionX);
            DebugAssert(oldIndex >= 0 && oldIndex < m_sectionCount);
            if (m_availableSectionMask.IsSet((uint32)oldIndex))
            {
                int32 newIndex = ((sectionY - newMinSectionY) * newSectionCountX) + (sectionX - newMinSectionX);
                DebugAssert(newIndex >= 0 && newIndex < newSectionCount);

                newAvailableSectionMask.Set((uint32)newIndex);
                ppNewSectionArray[newIndex] = m_ppSections[oldIndex];
            }
        }
    }

    // swap everything over
    m_minSectionX = newMinSectionX;
    m_minSectionY = newMinSectionY;
    m_maxSectionX = newMaxSectionX;
    m_maxSectionY = newMaxSectionY;
    m_sectionCountX = newSectionCountX;
    m_sectionCountY = newSectionCountY;
    m_sectionCount = newSectionCount;
    delete[] m_ppSections;
    m_ppSections = ppNewSectionArray;
    m_availableSectionMask.Swap(newAvailableSectionMask);

    // update world bounds
    AABox blockWorldBoundingBox = CalculateBlockWorldBoundingBox();
    m_worldBoundingBox.Merge(blockWorldBoundingBox);
    m_worldBoundingSphere.Merge(Sphere::FromAABox(blockWorldBoundingBox));

    // rewrite the index to file
    if (!SaveIndex())
        Log_WarningPrint("BlockWorld::ResizeIndex: Failed to save resized index.");
}

bool BlockWorld::IsVisibilityBlockingBlockValue(BlockWorldBlockType blockValue, CUBE_FACE checkFace /* = CUBE_FACE_COUNT */) const
{
    // fastpath for zero blocks
    if (blockValue == 0)
        return false;

    // coloured blocks always block and are always cubic
    if (blockValue & BLOCK_WORLD_BLOCK_VALUE_COLORED_FLAG_BIT)
        return true;

    // extract block type
    const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType(blockValue);
    if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
    {
        // transparent blocks can't block visibility, not entirely anyway
        return (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCKS_VISIBILITY) != 0;
    }
    else if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB)
    {
        // slabs can block visibility, but not on the top face
        if (checkFace == CUBE_FACE_TOP)
            return false;

        // transparent blocks can't block visibility, not entirely anyway
        return (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCKS_VISIBILITY) != 0;
    }
    else
    {
        // nothing else blocks
        return false;
    }
}

bool BlockWorld::IsLightBlockingBlockValue(BlockWorldBlockType blockValue, CUBE_FACE checkFace /* = CUBE_FACE_COUNT */) const
{
    // fastpath for zero blocks
    if (blockValue == 0)
        return false;

    // coloured blocks always block and are always cubic
    if (blockValue & BLOCK_WORLD_BLOCK_VALUE_COLORED_FLAG_BIT)
        return true;

    // extract block type
    const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType(blockValue);

    // is not cube?
    if (pBlockType->ShapeType != BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
        return false;

    // or visibility-blocking/solid
    if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCKS_VISIBILITY)
        return true;

    // not blocking
    return false;
}

AABox BlockWorld::CalculateSectionBoundingBox(int32 sectionSize, int32 chunkSize, int32 sectionX, int32 sectionY)
{
    int32 minX = (sectionX * sectionSize * chunkSize);
    int32 maxX = ((sectionX + 1) * sectionSize * chunkSize);
    int32 minY = (sectionY * sectionSize * chunkSize);
    int32 maxY = ((sectionY + 1) * sectionSize * chunkSize);

    return AABox((float)minX, (float)minY, Y_FLT_MIN, (float)maxX, (float)maxY, Y_FLT_MAX);
}

AABox BlockWorld::CalculateSectionBoundingBox(int32 sectionSize, int32 chunkSize, int32 sectionX, int32 sectionY, int32 minChunkZ, int32 maxChunkZ)
{
    int32 minX = (sectionX * sectionSize * chunkSize);
    int32 maxX = ((sectionX + 1) * sectionSize * chunkSize);
    int32 minY = (sectionY * sectionSize * chunkSize);
    int32 maxY = ((sectionY + 1) * sectionSize * chunkSize);
    int32 minZ = (minChunkZ * chunkSize);
    int32 maxZ = ((maxChunkZ + 1) * chunkSize);

    return AABox((float)minX, (float)minY, (float)minZ, (float)maxX, (float)maxY, (float)maxZ);
}

AABox BlockWorld::CalculateChunkBoundingBox(int32 chunkSize, int32 chunkX, int32 chunkY, int32 chunkZ)
{
    // then add the chunks
    int32 minX = chunkX * chunkSize;
    int32 minY = chunkY * chunkSize;
    int32 minZ = chunkZ * chunkSize;

    // work out max
    int32 maxX = minX + chunkSize;
    int32 maxY = minY + chunkSize;
    int32 maxZ = minZ + chunkSize;

    // return result
    return AABox(static_cast<float>(minX), static_cast<float>(minY), static_cast<float>(minZ), 
                 static_cast<float>(maxX), static_cast<float>(maxY), static_cast<float>(maxZ));
}

AABox BlockWorld::CalculateChunkBoundingBox(int32 chunkSize, int32 sectionSize, int32 sectionX, int32 sectionY, int32 lodLevel, int32 relativeChunkX, int32 relativeChunkY, int32 relativeChunkZ)
{
    int32 realChunkSize = (chunkSize << lodLevel);

    // find start x,y of section
    int32 sectionStartX = sectionX * (sectionSize * chunkSize);
    int32 sectionStartY = sectionY * (sectionSize * chunkSize);

    // add in chunk position
    int32 minX = sectionStartX + relativeChunkX * realChunkSize;
    int32 minY = sectionStartY + relativeChunkY * realChunkSize;
    int32 minZ = relativeChunkZ * realChunkSize;

    // find the max
    int32 maxX = minX + realChunkSize;
    int32 maxY = minY + realChunkSize;
    int32 maxZ = minZ + realChunkSize;

    // return result
    return AABox(static_cast<float>(minX), static_cast<float>(minY), static_cast<float>(minZ), 
                 static_cast<float>(maxX), static_cast<float>(maxY), static_cast<float>(maxZ));
}

AABox BlockWorld::CalculateBlockWorldBoundingBox() const
{
    int32 minX = m_minSectionX * m_sectionSizeInBlocks;
    int32 minY = m_minSectionY * m_sectionSizeInBlocks;
    int32 maxX = (m_maxSectionX + 1) * m_sectionSizeInBlocks;
    int32 maxY = (m_maxSectionY + 1) * m_sectionSizeInBlocks;

    return AABox((float)minX, (float)minY, Y_FLT_MIN, (float)maxX, (float)maxY, Y_FLT_MAX);
}

// hack hack!
#define div(numerator, denominator) { ((numerator) / (denominator)), ((numerator) % (denominator)) }

void BlockWorld::SplitCoordinates(int32 *sectionX, int32 *sectionY, int32 *localChunkX, int32 *localChunkY, int32 *localChunkZ, int32 *localX, int32 *localY, int32 *localZ, int32 chunkSize, int32 sectionSize, int32 bx, int32 by, int32 bz)
{
    int32 sx, sy;
    int32 lcx, lcy, lcz;
    int32 lx, ly, lz;
    int32 sectionSizeInBlocks = chunkSize * sectionSize;

    // x axis
    if (bx >= 0)
    {
        div_t res = div(bx, sectionSizeInBlocks);
        div_t res2 = div(res.rem, chunkSize);
        sx = res.quot;
        lcx = res2.quot;
        lx = res2.rem;
    }
    else
    {
        div_t res = div(bx + 1, sectionSizeInBlocks);
        div_t res2 = div((sectionSizeInBlocks - 1) + res.rem, chunkSize);
        sx = res.quot - 1;
        lcx = res2.quot;
        lx = res2.rem;
    }

    // y axis
    if (by >= 0)
    {
        div_t res = div(by, sectionSizeInBlocks);
        div_t res2 = div(res.rem, chunkSize);
        sy = res.quot;
        lcy = res2.quot;
        ly = res2.rem;
    }
    else
    {
        div_t res = div(by + 1, sectionSizeInBlocks);
        div_t res2 = div((sectionSizeInBlocks - 1) + res.rem, chunkSize);
        sy = res.quot - 1;
        lcy = res2.quot;
        ly = res2.rem;
    }

    // z axis
    if (bz >= 0)
    {
        div_t res2 = div(bz, chunkSize);
        lcz = res2.quot;
        lz = res2.rem;
    }
    else
    {
        div_t res = div(bz + 1, chunkSize);
        lcz = res.quot - 1;
        lz = (chunkSize - 1) + res.rem;
    }

    DebugAssert(lcx < sectionSize && lcy < sectionSize);
    DebugAssert(lx < chunkSize && ly < chunkSize && lz < chunkSize);
    //Log_DevPrintf("%i %i %i -> %i %i : %i %i %i : %i %i %i", bx, by, bz, sx, sy, lcx, lcy, lcz, lx, ly, lz);

    *sectionX = sx;
    *sectionY = sy;
    *localChunkX = lcx;
    *localChunkY = lcy;
    *localChunkZ = lcz;
    *localX = (uint32)lx;
    *localY = (uint32)ly;
    *localZ = (uint32)lz;
}

void BlockWorld::SplitCoordinates(int32 *chunkX, int32 *chunkY, int32 *chunkZ, int32 *localX, int32 *localY, int32 *localZ, int32 chunkSize, int32 bx, int32 by, int32 bz)
{
    int32 cx, cy, cz;
    int32 lx, ly, lz;

    // x axis
    if (bx >= 0)
    {
        div_t res = div(bx, chunkSize);
        cx = res.quot;
        lx = res.rem;
    }
    else
    {
        div_t res = div(bx + 1, chunkSize);
        cx = res.quot - 1;
        lx = (chunkSize - 1) + res.rem;
    }

    // y axis
    if (by >= 0)
    {
        div_t res = div(by, chunkSize);
        cy = res.quot;
        ly = res.rem;
    }
    else
    {
        div_t res = div(by + 1, chunkSize);
        cy = res.quot - 1;
        ly = (chunkSize - 1) + res.rem;;
    }

    // z axis
    if (bz >= 0)
    {
        div_t res = div(bz, chunkSize);
        cz = res.quot;
        lz = res.rem;
    }
    else
    {
        div_t res = div(bz + 1, chunkSize);
        cz = res.quot - 1;
        lz = (chunkSize - 1) + res.rem;
    }

    DebugAssert(lx < chunkSize && ly < chunkSize && lz < chunkSize);
    //Log_DevPrintf("%i %i %i -> %i %i %i : %i %i %i", bx, by, bz, cx, cy, cz, lx, ly, lz);

    *chunkX = cx;
    *chunkY = cy;
    *chunkZ = cz;
    *localX = lx;
    *localY = ly;
    *localZ = lz;
}

#undef div

void BlockWorld::ConvertChunkBlockCoordinatesToGlobalCoordinates(int32 chunkSize, int32 chunkX, int32 chunkY, int32 chunkZ, int32 blockX, int32 blockY, int32 blockZ, int32 *pGlobalBlockX, int32 *pGlobalBlockY, int32 *pGlobalBlockZ)
{
    if (chunkX >= 0)
        *pGlobalBlockX = chunkX * chunkSize + blockX;
    else
        *pGlobalBlockX = (chunkX + 1) * chunkSize - (chunkSize - blockX);

    if (chunkY >= 0)
        *pGlobalBlockY = chunkY * chunkSize + blockY;
    else
        *pGlobalBlockY = (chunkY + 1) * chunkSize - (chunkSize - blockY);

    if (chunkZ >= 0)
        *pGlobalBlockZ = chunkZ * chunkSize + blockZ;
    else
        *pGlobalBlockZ = (chunkZ + 1) * chunkSize - (chunkSize - blockZ);
}

void BlockWorld::CalculateRelativeChunkCoordinates(int32 *sectionX, int32 *sectionY, int32 *relativeChunkX, int32 *relativeChunkY, int32 *relativeChunkZ, int32 sectionSize, int32 chunkSize, int32 chunkX, int32 chunkY, int32 chunkZ)
{
    // x axis
    if (chunkX >= 0)
    {
        div_t res = div(chunkX, sectionSize);
        *sectionX = res.quot;
        *relativeChunkX = res.rem;
    }
    else
    {
        div_t res = div(chunkX + 1, sectionSize);
        *sectionX = res.quot - 1;
        *relativeChunkX = (sectionSize - 1) + res.rem;
    }

    // y axis
    if (chunkY >= 0)
    {
        div_t res = div(chunkY, sectionSize);
        *sectionY = res.quot;
        *relativeChunkY = res.rem;
    }
    else
    {
        div_t res = div(chunkY + 1, sectionSize);
        *sectionY = res.quot - 1;
        *relativeChunkY = (sectionSize - 1) + res.rem;
    }

    // z is easy, copy across
    *relativeChunkZ = chunkZ;
}

void BlockWorld::SplitCoordinates(int32 *sectionX, int32 *sectionY, int32 *localChunkX, int32 *localChunkY, int32 *localChunkZ, int32 *localX, int32 *localY, int32 *localZ, int32 bx, int32 by, int32 bz) const
{
    SplitCoordinates(sectionX, sectionY, localChunkX, localChunkY, localChunkZ, localX, localY, localZ, m_chunkSize, m_sectionSize, bx, by, bz);
}

void BlockWorld::SplitCoordinates(int32 *chunkX, int32 *chunkY, int32 *chunkZ, int32 *localX, int32 *localY, int32 *localZ, int32 bx, int32 by, int32 bz) const
{
    SplitCoordinates(chunkX, chunkY, chunkZ, localX, localY, localZ, m_chunkSize, bx, by, bz);
}

void BlockWorld::CalculateRelativeChunkCoordinates(int32 *sectionX, int32 *sectionY, int32 *relativeChunkX, int32 *relativeChunkY, int32 *relativeChunkZ, int32 chunkX, int32 chunkY, int32 chunkZ) const
{
    CalculateRelativeChunkCoordinates(sectionX, sectionY, relativeChunkX, relativeChunkY, relativeChunkZ, m_sectionSize, m_chunkSize, chunkX, chunkY, chunkZ);
}


int3 BlockWorld::CalculatePointForPosition(const float3 &position) const
{
    return int3(Math::Truncate(position.x) - ((position.x < 0.0f) ? 1 : 0),
                Math::Truncate(position.y) - ((position.y < 0.0f) ? 1 : 0),
                Math::Truncate(position.z) - ((position.z < 0.0f) ? 1 : 0));
}

void BlockWorld::CalculateSectionForPosition(int32 *sx, int32 *sy, const float3 &position) const
{
    // convert to block coordinates
    int3 blockCoordinates(CalculatePointForPosition(position));

    // convert to sections
    if (blockCoordinates.x >= 0)
        *sx = blockCoordinates.x / m_sectionSizeInBlocks;
    else
        *sx = ((blockCoordinates.x + 1) / m_sectionSizeInBlocks) - 1;

    if (blockCoordinates.y >= 0)
        *sy = blockCoordinates.y / m_sectionSizeInBlocks;
    else
        *sy = ((blockCoordinates.y + 1) / m_sectionSizeInBlocks) - 1;
}

void BlockWorld::CalculateChunkForPosition(int32 *cx, int32 *cy, int32 *cz, const float3 &position) const
{
    // convert to block coordinates
    int3 blockCoordinates(CalculatePointForPosition(position));

    // convert to chunks
    if (blockCoordinates.x >= 0)
        *cx = blockCoordinates.x / m_chunkSize;
    else
        *cx = ((blockCoordinates.x + 1) / m_chunkSize) - 1;

    if (blockCoordinates.y >= 0)
        *cy = blockCoordinates.y / m_chunkSize;
    else
        *cy = ((blockCoordinates.y + 1) / m_chunkSize) - 1;

    if (blockCoordinates.z >= 0)
        *cz = blockCoordinates.z / m_chunkSize;
    else
        *cz = ((blockCoordinates.z + 1) / m_chunkSize) - 1;
}

void BlockWorld::GetSectionStatus(bool *available, bool *loaded, int32 sectionX, int32 sectionY, int32 requiredLODLevel /* = Y_INT32_MAX */) const
{
    // section in-range?
    if (sectionX < m_minSectionX || sectionX > m_maxSectionX || sectionY < m_minSectionY || sectionY > m_maxSectionY)
    {
        // query generator
        if (m_pGenerator != nullptr)
            *available = m_pGenerator->CanGenerateSection(sectionX, sectionY);
        else
            *available = false;

        *loaded = false;
        return;
    }

    // test section availablity
    int32 sectionArrayIndex = GetSectionArrayIndex(sectionX, sectionY);
    if (!m_availableSectionMask.IsSet(sectionArrayIndex))
    {
        // query generator
        if (m_pGenerator != nullptr)
            *available = m_pGenerator->CanGenerateSection(sectionX, sectionY);
        else
            *available = false;

        *loaded = false;
        return;
    }

    // chunk is available
    *available = true;

    // check loaded status
    *loaded = (m_ppSections[sectionArrayIndex] != nullptr && m_ppSections[sectionArrayIndex]->GetLoadedLODLevel() <= requiredLODLevel);
}

bool BlockWorld::IsSectionAvailable(int32 sectionX, int32 sectionY) const
{
    bool available, loaded;
    GetSectionStatus(&available, &loaded, sectionX, sectionY, 0);
    return available;
}

bool BlockWorld::IsSectionLoaded(int32 sectionX, int32 sectionY, int32 requiredLODLevel /* = Y_INT32_MAX */) const
{
    bool available, loaded;
    GetSectionStatus(&available, &loaded, sectionX, sectionY, requiredLODLevel);
    return loaded;
}

void BlockWorld::GetChunkStatus(bool *available, bool *loaded, int32 chunkX, int32 chunkY, int32 chunkZ, int32 requiredLODLevel) const
{
    // convert to section + relative chunk
    int32 sectionX, sectionY;
    int32 relativeChunkX, relativeChunkY, relativeChunkZ;
    CalculateRelativeChunkCoordinates(&sectionX, &sectionY, &relativeChunkX, &relativeChunkY, &relativeChunkZ, chunkX, chunkY, chunkZ);

    // section in-range?
    if (sectionX < m_minSectionX || sectionX > m_maxSectionX || sectionY < m_minSectionY || sectionY > m_maxSectionY)
    {
        *available = false;
        *loaded = false;
        return;
    }

    // test section availablity
    int32 sectionArrayIndex = GetSectionArrayIndex(sectionX, sectionY);
    if (!m_availableSectionMask.IsSet(sectionArrayIndex))
    {
        *available = false;
        *loaded = false;
        return;
    }

    // get section 
    const BlockWorldSection *pSection = GetSection(sectionX, sectionY);
    if (pSection == nullptr)
    {
        // let's assume it's available, but we won't know for sure until it's loaded
        *available = true;
        *loaded = false;
        return;
    }

    // use highest loaded lod as a reference, adjust relative chunk coordinates for lod
    int32 checkLodLevel = Min(requiredLODLevel, pSection->GetLODLevels() - 1);
    DebugAssert(relativeChunkX >= 0 && relativeChunkY >= 0);
    relativeChunkX >>= checkLodLevel;
    relativeChunkY >>= checkLodLevel;
    relativeChunkZ >>= checkLodLevel;

    // test for section
    if ((*available = pSection->GetChunkAvailability(relativeChunkX, relativeChunkY, relativeChunkZ)) == true)
    {
        // chunk is available, so set the loaded status to whether the lod is loaded (the chunk should be present)
        DebugAssert(pSection->GetChunk(relativeChunkX, relativeChunkY, relativeChunkZ) != nullptr);
        *loaded = (pSection->GetLoadedLODLevel() <= checkLodLevel);
    }
    else
    {
        // chunk not available, so thus not loaded
        *loaded = false;
    }
}

bool BlockWorld::IsChunkAvailable(int32 chunkX, int32 chunkY, int32 chunkZ) const
{
    bool available, loaded;
    GetChunkStatus(&available, &loaded, chunkX, chunkY, chunkZ, Y_INT32_MAX);
    return available;
}

bool BlockWorld::IsChunkLoaded(int32 chunkX, int32 chunkY, int32 chunkZ, int32 requiredLODLevel /* = Y_INT32_MAX */) const
{
    bool available, loaded;
    GetChunkStatus(&available, &loaded, chunkX, chunkY, chunkZ, requiredLODLevel);
    return available;
}

const BlockWorldSection *BlockWorld::GetSection(int32 sectionX, int32 sectionY) const
{
    if (sectionX < m_minSectionX || sectionX > m_maxSectionX || sectionY < m_minSectionY || sectionY > m_maxSectionY)
        return nullptr;

    int32 sectionIndex = GetSectionArrayIndex(sectionX, sectionY);
    DebugAssert(sectionIndex >= 0);

    return m_ppSections[sectionIndex];
}

BlockWorldSection *BlockWorld::GetSection(int32 sectionX, int32 sectionY)
{
    if (sectionX < m_minSectionX || sectionX > m_maxSectionX || sectionY < m_minSectionY || sectionY > m_maxSectionY)
        return nullptr;

    int32 sectionIndex = GetSectionArrayIndex(sectionX, sectionY);
    DebugAssert(sectionIndex >= 0);

    return m_ppSections[sectionIndex];
}

const BlockWorldChunk *BlockWorld::GetChunk(int32 chunkX, int32 chunkY, int32 chunkZ) const
{
    // convert to section + relative chunk
    int32 sectionX, sectionY;
    int32 relativeChunkX, relativeChunkY, relativeChunkZ;
    CalculateRelativeChunkCoordinates(&sectionX, &sectionY, &relativeChunkX, &relativeChunkY, &relativeChunkZ, chunkX, chunkY, chunkZ);

    // test for section
    const BlockWorldSection *pSection = GetSection(sectionX, sectionY);
    return (pSection != nullptr) ? pSection->SafeGetChunk(relativeChunkX, relativeChunkY, relativeChunkZ) : nullptr;
}

BlockWorldChunk *BlockWorld::GetChunk(int32 chunkX, int32 chunkY, int32 chunkZ)
{
    // convert to section + relative chunk
    int32 sectionX, sectionY;
    int32 relativeChunkX, relativeChunkY, relativeChunkZ;
    CalculateRelativeChunkCoordinates(&sectionX, &sectionY, &relativeChunkX, &relativeChunkY, &relativeChunkZ, chunkX, chunkY, chunkZ);

    // test for section
    BlockWorldSection *pSection = GetSection(sectionX, sectionY);
    return (pSection != nullptr) ? pSection->SafeGetChunk(relativeChunkX, relativeChunkY, relativeChunkZ) : nullptr;
}

bool BlockWorld::IsChunkNeighboursLoaded(const BlockWorldChunk *pChunk, int32 lodLevel) const
{
    const BlockWorldSection *pSection = pChunk->GetSection();

    // neighbour chunk on x axis
    if (pChunk->GetRelativeChunkX() == 0)
    {
        if (IsSectionAvailable(pSection->GetSectionX() - 1, pSection->GetSectionY()) && !IsSectionLoaded(pSection->GetSectionX() - 1, pSection->GetSectionY(), lodLevel))
            return false;
    }
    else if (pChunk->GetRelativeChunkX() == (m_sectionSize - 1))
    {
        if (IsSectionAvailable(pSection->GetSectionX() + 1, pSection->GetSectionY()) && !IsSectionLoaded(pSection->GetSectionX() + 1, pSection->GetSectionY(), lodLevel))
            return false;
    }

    // neighbour chunk on y axis
    if (pChunk->GetRelativeChunkY() == 0)
    {
        if (IsSectionAvailable(pSection->GetSectionX(), pSection->GetSectionY() - 1) && !IsSectionLoaded(pSection->GetSectionX(), pSection->GetSectionY() - 1, lodLevel))
            return false;
    }
    else if (pChunk->GetRelativeChunkY() == (m_sectionSize - 1))
    {
        if (IsSectionAvailable(pSection->GetSectionX(), pSection->GetSectionY() + 1) && !IsSectionLoaded(pSection->GetSectionX(), pSection->GetSectionY() + 1, lodLevel))
            return false;
    }

    // neighbours present or not edge chunk
    return true;
}

bool BlockWorld::EnsureChunkNeighboursLoaded(const BlockWorldChunk *pChunk, int32 lodLevel)
{
    const BlockWorldSection *pSection = pChunk->GetSection();
    int32 sectionX, sectionY;
    
    // neighbour chunk on x axis
    if (pChunk->GetRelativeChunkX() == 0)
    {
        sectionX = pSection->GetSectionX() - 1;
        sectionY = pSection->GetSectionY();
        if (IsSectionAvailable(sectionX, sectionY) && !IsSectionLoaded(sectionX, sectionY, lodLevel) && !LoadSection(sectionX, sectionY, lodLevel, false))
            return false;
    }
    else if (pChunk->GetRelativeChunkX() == (m_sectionSize - 1))
    {
        sectionX = pSection->GetSectionX() + 1;
        sectionY = pSection->GetSectionY();
        if (IsSectionAvailable(sectionX, sectionY) && !IsSectionLoaded(sectionX, sectionY, lodLevel) && !LoadSection(sectionX, sectionY, lodLevel, false))
            return false;
    }

    // neighbour chunk on y axis
    if (pChunk->GetRelativeChunkY() == 0)
    {
        sectionX = pSection->GetSectionX();
        sectionY = pSection->GetSectionY() - 1;
        if (IsSectionAvailable(sectionX, sectionY) && !IsSectionLoaded(sectionX, sectionY, lodLevel) && !LoadSection(sectionX, sectionY, lodLevel, false))
            return false;
    }
    else if (pChunk->GetRelativeChunkY() == (m_sectionSize - 1))
    {
        sectionX = pSection->GetSectionX();
        sectionY = pSection->GetSectionY() + 1;
        if (IsSectionAvailable(sectionX, sectionY) && !IsSectionLoaded(sectionX, sectionY, lodLevel) && !LoadSection(sectionX, sectionY, lodLevel, false))
            return false;
    }

    // neighbours present or not edge chunk
    return true;
}

ByteStream *BlockWorld::OpenWorldFile(const char *fileName, uint32 access)
{
    ByteStream *pStream;

#ifdef USE_FIF
    pStream = m_pFIFVolume->OpenFile(fileName, access);
#else
    PathString fullFileName;
    fullFileName.Format("%s/%s", m_basePath.GetCharArray(), fileName);
    pStream = g_pVirtualFileSystem->OpenFile(fullFileName, access);
#endif

    if (pStream == nullptr)
        Log_ErrorPrintf("BlockWorld::OpenWorldFile: Failed to open file '%s' with access %X", fileName, access);

    return pStream;
}

bool BlockWorld::Create(const char *basePath, const BlockPalette *pPalette, int32 chunkSize, int32 sectionSize, int32 lodLevels)
{
    if (!IsValidChunkSize(chunkSize, sectionSize, lodLevels))
        return false;

    // save path
    m_basePath = basePath;

#ifdef USE_FIF
    // open stream
    AutoReleasePtr<ByteStream> pFIFStream = g_pVirtualFileSystem->OpenFile(SmallString::FromFormat("%s.fif", basePath), BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_SEEKABLE);
    //AutoReleasePtr<ByteStream> pTraceStream = g_pVirtualFileSystem->OpenFile(SmallString::FromFormat("%s.fif.trace", basePath), BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_SEEKABLE);
    AutoReleasePtr<ByteStream> pTraceStream = nullptr;
    if (pFIFStream == nullptr || (m_pFIFVolume = FIFVolume::CreateVolume(pFIFStream, pTraceStream)) == nullptr)
    {
        Log_ErrorPrintf("BlockWorld::Create: Failed to create '%s'.", m_basePath.GetCharArray());
        return false;
    }
#endif

    m_pPalette = pPalette;
    m_pPalette->AddRef();
    m_chunkSize = chunkSize;
    m_sectionSize = sectionSize;
    m_lodLevels = lodLevels;
    m_minSectionX = m_maxSectionX = 0;
    m_minSectionY = m_minSectionY = 0;
    AllocateIndex();

    return SaveIndex();
}

bool BlockWorld::Load(const char *basePath)
{
    // save path
    m_basePath = basePath;

#ifdef USE_FIF
    // open stream
    AutoReleasePtr<ByteStream> pFIFStream = g_pVirtualFileSystem->OpenFile(SmallString::FromFormat("%s.fif", basePath), BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_SEEKABLE);
    if (pFIFStream == nullptr || (m_pFIFVolume = FIFVolume::OpenVolume(pFIFStream)) == nullptr)
    {
        Log_ErrorPrintf("BlockWorld::Create: Failed to open '%s'.", m_basePath.GetCharArray());
        return false;
    }
#else
    m_basePath = basePath;
#endif

    // load the index
    if (!LoadIndex())
    {
        Log_ErrorPrintf("BlockWorld::Load(%s): failed to load index", basePath);
        return false;
    }

    return true;
}

bool BlockWorld::LoadIndex()
{
    AutoReleasePtr<ByteStream> pIndexStream = OpenWorldFile(BLOCK_WORLD_INDEX_FILE_NAME, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pIndexStream == nullptr)
        return false;

    BinaryReader binaryReader(pIndexStream);
    SmallString paletteName;
    bool readResult = true;

    readResult &= binaryReader.SafeReadCString(&paletteName);
    readResult &= binaryReader.SafeReadInt32(&m_chunkSize);
    readResult &= binaryReader.SafeReadInt32(&m_sectionSize);
    readResult &= binaryReader.SafeReadInt32(&m_lodLevels);
    if (!readResult || (m_pPalette = g_pResourceManager->GetBlockPalette(paletteName)) == nullptr || !IsValidChunkSize(m_chunkSize, m_sectionSize, m_lodLevels))
        return false;

    readResult &= binaryReader.SafeReadInt32(&m_minSectionX);
    readResult &= binaryReader.SafeReadInt32(&m_minSectionY);
    readResult &= binaryReader.SafeReadInt32(&m_maxSectionX);
    readResult &= binaryReader.SafeReadInt32(&m_maxSectionY);
    if (!readResult || m_minSectionX > m_maxSectionX || m_minSectionY > m_maxSectionY)
        return false;

    // allocate index
    AllocateIndex();

    // read index
    uint32 nAvailableSections;
    if (!binaryReader.SafeReadUInt32(&nAvailableSections))
        return false;

    for (uint32 j = 0; j < nAvailableSections; j++)
    {
        int32 sectionX, sectionY;
        readResult &= binaryReader.SafeReadInt32(&sectionX);
        readResult &= binaryReader.SafeReadInt32(&sectionY);
        if (!readResult || sectionX < m_minSectionX || sectionX > m_maxSectionX || sectionY < m_minSectionY || sectionY > m_maxSectionY)
            return false;

        int32 arrayIndex = GetSectionArrayIndex(sectionX, sectionY);
        if (m_availableSectionMask.IsSet((uint32)arrayIndex))
            return false;

        m_availableSectionMask.Set((uint32)arrayIndex);
    }

    // handle animation-in-progress block type
    const BlockPalette::BlockType *pAnimationInProgressBlockType = m_pPalette->GetBlockTypeByName("ANIMATION_IN_PROGRESS_BLOCK");
    m_animationInProgressBlockType = (pAnimationInProgressBlockType != nullptr) ? (BlockWorldBlockType)pAnimationInProgressBlockType->BlockTypeIndex : 0;
    if (m_animationInProgressBlockType == 0)
        Log_WarningPrintf("BlockWorld::LoadIndex: Animation in progress block does not exist, fluidity may be impacted as a result.");    

    return true;
}

bool BlockWorld::SaveIndex()
{
    ByteStream *pIndexStream = OpenWorldFile(BLOCK_WORLD_INDEX_FILE_NAME, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_STREAMED | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_ATOMIC_UPDATE);
    if (pIndexStream == nullptr)
        return false;

    BinaryWriter binaryWriter(pIndexStream);
    bool writeResult = true;

    writeResult &= binaryWriter.SafeWriteCString(m_pPalette->GetName());
    writeResult &= binaryWriter.SafeWriteInt32(m_chunkSize);
    writeResult &= binaryWriter.SafeWriteInt32(m_sectionSize);
    writeResult &= binaryWriter.SafeWriteInt32(m_lodLevels);
    writeResult &= binaryWriter.SafeWriteInt32(m_minSectionX);
    writeResult &= binaryWriter.SafeWriteInt32(m_minSectionY);
    writeResult &= binaryWriter.SafeWriteInt32(m_maxSectionX);
    writeResult &= binaryWriter.SafeWriteInt32(m_maxSectionY);

    uint32 availableSectionCount = 0;
    for (int32 i = 0; i < m_sectionCount; i++)
    {
        if (m_availableSectionMask[i])
            availableSectionCount++;
    }

    writeResult &= binaryWriter.SafeWriteUInt32(availableSectionCount);

    // todo: optimize this to writing a bitmask
    for (int32 sectionX = m_minSectionX; sectionX <= m_maxSectionX; sectionX++)
    {
        for (int32 sectionY = m_minSectionY; sectionY <= m_maxSectionY; sectionY++)
        {
            int32 arrayIndex = GetSectionArrayIndex(sectionX, sectionY);
            if (m_availableSectionMask[arrayIndex])
            {
                writeResult &= binaryWriter.SafeWriteInt32(sectionX);
                writeResult &= binaryWriter.SafeWriteInt32(sectionY);
            }
        }
    }

    if (!writeResult)
        pIndexStream->Discard();
    else
        writeResult = pIndexStream->Commit();

    pIndexStream->Release();
    return writeResult;
}

bool BlockWorld::LoadGlobalEntities()
{
    return true;
}

bool BlockWorld::SaveGlobalEntities()
{
    return false;
}

bool BlockWorld::LoadSection(int32 sectionX, int32 sectionY, int32 lodLevel, bool unloadHigherLODs)
{
    Timer loadTimer;

    // handle section generation
    if (sectionX < m_minSectionX || sectionX > m_maxSectionX || sectionY < m_minSectionY || sectionY > m_maxSectionY ||     // section is out-of-range
        !m_availableSectionMask.IsSet(GetSectionArrayIndex(sectionX, sectionY)))                                            // section is not generated
    {
        // we only generate at lod0 for now.. fix this later
        if (lodLevel != 0)
            return false;

        // can it be generated?
        if (m_pGenerator == nullptr || !m_pGenerator->CanGenerateSection(sectionX, sectionY))
            return false;

        // calculate the block range
        int32 blockStartX = sectionX * m_sectionSizeInBlocks;
        int32 blockStartY = sectionY * m_sectionSizeInBlocks;
        int32 blockEndX = blockStartX + m_sectionSizeInBlocks - 1;
        int32 blockEndY = blockStartY + m_sectionSizeInBlocks - 1;

        // get block range
        int32 minBlockZ, maxBlockZ;
        if (!m_pGenerator->GetZRange(blockStartX, blockStartY, blockEndX, blockEndY, &minBlockZ, &maxBlockZ))
            return false;

        // convert to chunks
        int32 minChunkZ = minBlockZ / m_chunkSize;
        int32 maxChunkZ = maxBlockZ / m_chunkSize;

        // create the section
        BlockWorldSection *pSection = CreateSection(sectionX, sectionY, minChunkZ, maxChunkZ);
        if (pSection == nullptr)
            return false;

        // set to generating state
        pSection->SetLoadState(BlockWorldSection::LoadState_Generating);

        // log it
        Log_DevPrintf("BlockWorld::LoadSection: Generating section [%i, %i]... (block Z range %i - %i)", sectionX, sectionY, minBlockZ, maxBlockZ);
        if (!m_pGenerator->GenerateBlocks(blockStartX, blockStartY, blockEndX, blockEndY))
        {
            // delete the section
            Log_ErrorPrintf("BlockWorld::LoadSection: Failed to generate section [%i, %i]. Deleting section.", sectionX, sectionY);
            DeleteSection(sectionX, sectionY);
            return false;
        }

        // force a load update
        pSection->SetLoadState(BlockWorldSection::LoadState_Changed);
        pSection->RebuildLODs(m_lodLevels);

        // flag the section as unchanged since it can be regenerated quite easily... this will screw with lods forcing a regen if it changed among other things...
        //pSection->SetUnchanged();
        Log_DevPrintf("BlockWorld::LoadSection: Generated section [%i, %i] in %.3f ms", sectionX, sectionY, loadTimer.GetTimeMilliseconds());
        return true;
    }

    DebugAssert(sectionX >= m_minSectionX && sectionX <= m_maxSectionX && sectionY >= m_minSectionY && sectionY <= m_maxSectionY);

    int32 sectionIndex = GetSectionArrayIndex(sectionX, sectionY);
    DebugAssert(sectionIndex >= 0);

    // section is either loaded at a bad lod, or not loaded at all
    BlockWorldSection *pSection = m_ppSections[sectionIndex];
    if (pSection != nullptr && pSection->GetLoadedLODLevel() <= lodLevel)
    {
        if (unloadHigherLODs && pSection->GetLoadedLODLevel() < lodLevel)
        {
            // squish any pending mesh requests at a higher lod
            pSection->EnumerateChunks([this, lodLevel, pSection](BlockWorldChunk *pChunk)
            {
                // do we want to remove the render model if the lod is higher? it won't hurt, since it doesn't have any link to the data..
                if (pChunk->GetMeshState() == BlockWorldChunk::MeshState_Pending)
                {
                    for (PendingMeshingChunk &pendingChunk : m_pendingChunks)
                    {
                        if (pendingChunk.pChunk == pChunk)
                        {
                            if (pendingChunk.NewLODLevel > lodLevel)
                                pendingChunk.NewLODLevel = lodLevel;

                            break;
                        }
                    }
                }
            });

            // save the section if it's changed and we're going from lod0
            if (pSection->IsChanged() && pSection->GetLoadedLODLevel() == 0)
            {
                if (!SaveSection(pSection))
                    Log_WarningPrintf("BlockWorld::LoadSection: SaveSection(%i, %i) failed, changes to this section are now lost", sectionX, sectionY);
            }

            // actually unload the lods
            Log_DevPrintf("BlockWorld::LoadSection: Section [%i, %i] unloading lods (lod %i -> %i)", sectionX, sectionY, pSection->GetLoadedLODLevel(), lodLevel);
            pSection->UnloadLODs(lodLevel);
        }

        return true;
    }

    // open file for section
    ByteStream *pStream = OpenWorldFile(SmallString::FromFormat("%i_%i.section", sectionX, sectionY), BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
        return false;

    // create/load new section
    int32 oldLODLevel = (pSection != nullptr) ? pSection->GetLoadedLODLevel() : m_lodLevels;
    if (pSection == nullptr)
    {
        pSection = new BlockWorldSection(this, sectionX, sectionY);
        if (!pSection->LoadFromStream(pStream, lodLevel))
        {
            // failed to load new lods
            pSection->Release();
            pStream->Release();
            return false;
        }

        // add it to the loaded list
        m_ppSections[sectionIndex] = pSection;
        m_loadedSections.Add(pSection);
    }
    else
    {
        // load it up
        if (!pSection->LoadLODs(pStream, lodLevel))
        {
            // failed to load new lods
            pStream->Release();
            return false;
        }
    }

    // stream no longer needed
    pStream->Release();

    // events
    Log_DevPrintf("BlockWorld::LoadSection: Loaded section [%i, %i] at lod %i (from lod %i)... in %.3f ms", sectionX, sectionY, lodLevel, oldLODLevel, loadTimer.GetTimeMilliseconds());
    return true;
}

void BlockWorld::UnloadSection(int32 sectionX, int32 sectionY)
{
    int32 arrayIndex = GetSectionArrayIndex(sectionX, sectionY);
    DebugAssert(arrayIndex >= 0);

    // if it's not loaded, bail out early
    if (m_ppSections[arrayIndex] == nullptr)
        return;

    // load index
    BlockWorldSection *pSection = m_ppSections[arrayIndex];
    int32 loadedSectionIndex = m_loadedSections.IndexOf(pSection);
    DebugAssert(loadedSectionIndex >= 0);

    // save changes to the section
    if (pSection->GetLoadedLODLevel() == 0 && pSection->IsChanged() && !SaveSection(sectionX, sectionY))
        Log_WarningPrintf("BlockWorld::UnloadSection: SaveSection(%i, %i) failed, changes to this section are now lost", sectionX, sectionY);

    // kill any pending meshing
    for (uint32 i = 0; i < m_pendingChunks.GetSize();)
    {
        if (m_pendingChunks[i].pSection == pSection)
        {
            m_pendingChunks.FastRemove(i);
            continue;
        }
        else
        {
            i++;
        }
    }

    // unload it
    m_ppSections[arrayIndex] = nullptr;
    m_loadedSections.FastRemove(loadedSectionIndex);
    pSection->Release();
}

BlockWorldSection *BlockWorld::CreateSection(int32 sectionX, int32 sectionY, int32 minChunkZ, int32 maxChunkZ)
{
    // resize index if necessary
    if (sectionX < m_minSectionX || sectionX > m_maxSectionX || sectionY < m_minSectionY || sectionY > m_maxSectionY)
        ResizeIndex(Min(sectionX, m_minSectionX), Min(sectionY, m_minSectionY), Max(sectionX, m_maxSectionX), Max(sectionY, m_maxSectionY));

    // get index and ensure it doesn't already exist
    int32 arrayIndex = GetSectionArrayIndex(sectionX, sectionY);
    DebugAssert(m_ppSections[arrayIndex] == nullptr);
    if (m_availableSectionMask.IsSet(arrayIndex))
    {
        Log_ErrorPrintf("BlockWorld::CreateSection: Section [%i, %i] already exists", sectionX, sectionY);
        return nullptr;
    }

    // create the section
    BlockWorldSection *pSection = new BlockWorldSection(this, sectionX, sectionY);
    pSection->Create(minChunkZ, maxChunkZ);

    // store section
    m_availableSectionMask.Set(arrayIndex);
    m_ppSections[arrayIndex] = pSection;
    m_loadedSections.Add(pSection);

    // save the index, and an empty copy of this section
    if (!SaveIndex() || !SaveSection(pSection))
        Log_WarningPrintf("BlockWorld::CreateSection: Failed to save index or section [%i, %i]", sectionX, sectionY);

    // done
    Log_DevPrintf("BlockWorld::CreateSection: Section [%i, %i] created", sectionX, sectionY);
    return pSection;
}

void BlockWorld::DeleteSection(int32 sectionX, int32 sectionY)
{
    if (!IsSectionAvailable(sectionX, sectionY))
    {
        Log_ErrorPrintf("BlockWorld::DeleteSection: Section [%u, %u] does not exist", sectionX, sectionY);
        return;
    }

    // delete the section
    uint32 arrayIndex = GetSectionArrayIndex(sectionX, sectionY);
    BlockWorldSection *pSection = m_ppSections[arrayIndex];
    if (pSection != nullptr)
    {
        // kill any pending meshing
        for (uint32 i = 0; i < m_pendingChunks.GetSize();)
        {
            if (m_pendingChunks[i].pSection == pSection)
            {
                m_pendingChunks.FastRemove(i);
                continue;
            }
            else
            {
                i++;
            }
        }

        // unload section
        delete pSection;
        m_ppSections[arrayIndex] = nullptr;
    }

    // set unavailable
    m_availableSectionMask.Unset(arrayIndex);
}

bool BlockWorld::SaveSection(int32 sectionX, int32 sectionY)
{
    // get section
    BlockWorldSection *pSection = GetSection(sectionX, sectionY);
    if (pSection == nullptr)
        return false;

    return SaveSection(pSection);
}

bool BlockWorld::SaveSection(BlockWorldSection *pSection)
{
    Log_DevPrintf("Saving section %i,%i", pSection->GetSectionX(), pSection->GetSectionY());
    DebugAssert(pSection->GetLoadState() == BlockWorldSection::LoadState_Changed);

    // open file
    ByteStream *pStream = OpenWorldFile(SmallString::FromFormat("%i_%i.section", pSection->GetSectionX(), pSection->GetSectionY()), BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_STREAMED | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_ATOMIC_UPDATE);
    if (pStream == nullptr)
        return false;

    if (!pSection->SaveToStream(pStream))
    {
        Log_ErrorPrintf("BlockWorld::SaveSection: Failed to save section %i,%i", pSection->GetSectionX(), pSection->GetSectionY());
        pStream->Discard();
        pStream->Release();
        return false;
    }

    pStream->Commit();
    pStream->Release();
    pSection->SetLoadState(BlockWorldSection::LoadState_Loaded);
    return true;
}

void BlockWorld::DeleteAllSections()
{
    for (int32 sectionX = m_minSectionX; sectionX <= m_maxSectionX; sectionX++)
    {
        for (int32 sectionY = m_minSectionY; sectionY <= m_maxSectionY; sectionY++)
        {
            int32 arrayIndex = GetSectionArrayIndex(sectionX, sectionY);
            if (!m_availableSectionMask[arrayIndex])
                continue;

            DeleteSection(sectionX, sectionY);
        }
    }

    // resize back to (0,0)
    delete[] m_ppSections;
    m_ppSections = nullptr;
    m_minSectionX = m_maxSectionX = 0;
    m_minSectionY = m_maxSectionY = 0;
    AllocateIndex();
}

bool BlockWorld::LoadAllSections(int32 maxLODLevel /* = 0 */, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    maxLODLevel = Min(maxLODLevel, m_lodLevels - 1);
    pProgressCallbacks->SetFormattedStatusText("Loading block terrain sections up to LOD %u", maxLODLevel);
    pProgressCallbacks->SetProgressRange(m_sectionCount);
    pProgressCallbacks->SetProgressValue(0);
    pProgressCallbacks->SetCancellable(false);

    for (int32 sectionX = m_minSectionX; sectionX <= m_maxSectionX; sectionX++)
    {
        for (int32 sectionY = m_minSectionY; sectionY <= m_maxSectionY; sectionY++)
        {
            int32 arrayIndex = GetSectionArrayIndex(sectionX, sectionY);
            if (m_availableSectionMask.IsSet(arrayIndex) && (m_ppSections[arrayIndex] == nullptr || m_ppSections[arrayIndex]->GetLoadedLODLevel() > maxLODLevel))
            {
                if (!LoadSection(sectionX, sectionY, maxLODLevel, false))
                    return false;
            }

            pProgressCallbacks->IncrementProgressValue();
        }
    }

    return true;
}

void BlockWorld::UnloadAllSections()
{
    while (m_loadedSections.GetSize() > 0)
        UnloadSection(m_loadedSections[0]->GetSectionX(), m_loadedSections[0]->GetSectionY());
}

bool BlockWorld::SaveChangedSections()
{
    for (BlockWorldSection *pSection : m_loadedSections)
    {
        if (pSection->IsChanged())
        {
            if (!SaveSection(pSection->GetSectionX(), pSection->GetSectionY()))
                return false;
        }
    }

    return true;
}

BlockWorldChunk *BlockWorld::CreateChunk(int32 chunkX, int32 chunkY, int32 chunkZ)
{
    int32 sectionX, sectionY;
    int32 relativeChunkX, relativeChunkY, relativeChunkZ;
    CalculateRelativeChunkCoordinates(&sectionX, &sectionY, &relativeChunkX, &relativeChunkY, &relativeChunkZ, chunkX, chunkY, chunkZ);

    BlockWorldSection *pSection = GetSection(sectionX, sectionY);
    if (pSection == nullptr || pSection->GetLoadedLODLevel() != 0)
    {
        if (IsSectionAvailable(sectionX, sectionY))
        {
            if (!LoadSection(sectionX, sectionY, 0, false))
                return nullptr;

            pSection = GetSection(sectionX, sectionY);
            DebugAssert(pSection != nullptr);
        }
        else
        {
            if ((pSection = CreateSection(sectionX, sectionY, chunkZ, chunkZ)) == nullptr)
                return nullptr;
        }
    }

    DebugAssert(!pSection->GetChunkAvailability(relativeChunkX, relativeChunkY, relativeChunkZ));
    BlockWorldChunk *pChunk = pSection->CreateChunk(relativeChunkX, relativeChunkY, relativeChunkZ);
    if (pChunk == nullptr)
        return nullptr;

    OnChunkLoaded(pSection, pChunk);
    return pChunk;
}

BlockWorldChunk *BlockWorld::GetWritableChunk(int32 chunkX, int32 chunkY, int32 chunkZ, bool allowCreate /* = true */)
{
    int32 sectionX, sectionY;
    int32 relativeChunkX, relativeChunkY, relativeChunkZ;
    CalculateRelativeChunkCoordinates(&sectionX, &sectionY, &relativeChunkX, &relativeChunkY, &relativeChunkZ, chunkX, chunkY, chunkZ);

    BlockWorldSection *pSection = GetSection(sectionX, sectionY);
    if (pSection == nullptr || pSection->GetLoadedLODLevel() != 0)
    {
        if (IsSectionAvailable(sectionX, sectionY))
        {
            if (!LoadSection(sectionX, sectionY, 0, false))
                return nullptr;

            pSection = GetSection(sectionX, sectionY);
            DebugAssert(pSection != nullptr);
        }
        else
        {
            if (!allowCreate || (pSection = CreateSection(sectionX, sectionY, chunkZ, chunkZ)) == nullptr)
                return nullptr;
        }
    }

    if (!pSection->GetChunkAvailability(relativeChunkX, relativeChunkY, relativeChunkZ))
    {
        if (!allowCreate)
            return nullptr;

        BlockWorldChunk *pChunk = pSection->CreateChunk(relativeChunkX, relativeChunkY, relativeChunkZ);
        if (pChunk == nullptr)
            return nullptr;

        OnChunkLoaded(pSection, pChunk);
        return pChunk;
    }
    else
    {
        return pSection->GetChunk(relativeChunkX, relativeChunkY, relativeChunkZ);
    }
}

void BlockWorld::OnChunkLoaded(BlockWorldSection *pSection, BlockWorldChunk *pChunk)
{
    // add collision object to physics world
    m_pPhysicsWorld->AddObject(pChunk->GetCollisionObject());
}

void BlockWorld::OnChunkUnloaded(BlockWorldSection *pSection, BlockWorldChunk *pChunk)
{
    DebugAssert(pChunk->GetMeshState() != BlockWorldChunk::MeshState_InProgress);

    // remove from pending mesh queue if it is pending
    if (pChunk->IsMeshPending())
    {
        for (uint32 index = 0; index < m_pendingChunks.GetSize(); index++)
        {
            if (m_pendingChunks[index].pChunk == pChunk)
            {
                m_pendingChunks.FastRemove(index);
                break;
            }
        }
    }

    // remove render proxy if there is one created
    BlockWorldChunkRenderProxy *pRenderProxy = pChunk->GetRenderProxy();
    if (pRenderProxy != nullptr)
        m_pRenderWorld->RemoveRenderable(pRenderProxy);

    // remove collision object
    m_pPhysicsWorld->RemoveObject(pChunk->GetCollisionObject());
}

const BlockWorldBlockType BlockWorld::GetBlockValue(int32 bx, int32 by, int32 bz) const
{
    // determine chunks
    int32 sx, sy;
    int32 lcx, lcy, lcz;
    int32 lx, ly, lz;
    SplitCoordinates(&sx, &sy, &lcx, &lcy, &lcz, &lx, &ly, &lz, bx, by, bz);

    // get section
    const BlockWorldSection *pSection = GetSection(sx, sy);
    if (pSection == nullptr || pSection->GetLoadedLODLevel() > 0)
        return 0;

    // get chunk
    const BlockWorldChunk *pChunk = pSection->SafeGetChunk(lcx, lcy, lcz);
    if (pChunk == nullptr)
        return 0;

    // get value
    return pChunk->GetBlock(0, lx, ly, lz);
}

bool BlockWorld::ClearBlock(int32 bx, int32 by, int32 bz)
{
    return SetBlockType(bx, by, bz, 0, BLOCK_WORLD_BLOCK_ROTATION_NORTH, false);
}

bool BlockWorld::SetBlockType(int32 bx, int32 by, int32 bz, BlockWorldBlockType blockType, BLOCK_WORLD_BLOCK_ROTATION blockRotation /* = BLOCK_WORLD_BLOCK_ROTATION_NORTH */, bool createNonExistantChunks /* = false */)
{
    DebugAssert(blockType == 0 || m_pPalette->GetBlockType(blockType)->IsAllocated);

    // determine chunks
    int32 chunkX, chunkY, chunkZ;
    int32 localX, localY, localZ;
    SplitCoordinates(&chunkX, &chunkY, &chunkZ, &localX, &localY, &localZ, bx, by, bz);

    // find chunk
    BlockWorldChunk *pChunk = GetWritableChunk(chunkX, chunkY, chunkZ, (createNonExistantChunks && blockType != 0));
    if (pChunk == nullptr)
        return false;
    
    // if no change in block value, skip everything. this works because the top bit will be clear on both sides if appropriate
    BlockWorldBlockType oldBlockValue = pChunk->GetBlock(0, localX, localY, localZ);
    BLOCK_WORLD_BLOCK_ROTATION oldBlockRotation = (BLOCK_WORLD_BLOCK_ROTATION)pChunk->GetBlockRotation(0, localX, localY, localZ);
    if (oldBlockValue == blockType && oldBlockRotation == blockRotation)
        return true;

    // lookup old and new block types
    const BlockPalette::BlockType *pOldBlockType = (oldBlockValue != 0) ? m_pPalette->GetBlockType((uint32)oldBlockValue) : nullptr;
    const BlockPalette::BlockType *pNewBlockType = (blockType != 0) ? m_pPalette->GetBlockType((uint32)blockType) : nullptr;
  
    // commit the value to this chunk
    pChunk->SetBlock(0, localX, localY, localZ, blockType);
    pChunk->SetBlockRotation(0, localX, localY, localZ, (uint8)blockRotation);

    // only call block changed methods when not generating
    if (pChunk->GetSection()->GetLoadState() != BlockWorldSection::LoadState_Generating)
        OnBlockChanged(pChunk, localX, localY, localZ);

    // if we're changing block types, we need to look at neighbouring chunks
    if (pNewBlockType != pOldBlockType && pNewBlockType != nullptr && pOldBlockType != nullptr && pNewBlockType->Flags != pOldBlockType->Flags)
    {
        // enure it's an edge block
        if (localX == 0 || localX == (m_chunkSize - 1) || localY == 0 || localY == (m_chunkSize - 1) || localZ == 0 || localZ == (m_chunkSize - 1))
            OnEdgeBlockChanged(pChunk, localX, localY, localZ);
    }

    // if the old block value was an emitter, unspread it's light
    if ((pOldBlockType != nullptr && (pOldBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCK_LIGHT_EMITTER)) ||
        (IsLightBlockingBlockValue(blockType) && pChunk->GetBlockLight(0, localX, localY, localZ) > 0))
    {
        UnspreadBlockLighting(pChunk, localX, localY, localZ);
    }

    // if the new block type is an emitter, spread it's light
    if (pNewBlockType != nullptr && (pNewBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCK_LIGHT_EMITTER))
        SpreadBlockLighting(pChunk, localX, localY, localZ, (uint8)pNewBlockType->BlockLightEmitterSettings.Radius);

    // changing from an empty block to a solid block
    bool oldBlockSolid = (oldBlockValue != 0) ? ((pOldBlockType != nullptr) ? ((pOldBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_COLLIDABLE) != 0) : true) : false;
    bool newBlockSolid = (blockType != 0) ? ((pNewBlockType != nullptr) ? ((pNewBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_COLLIDABLE) != 0) : true) : false;
    if (oldBlockSolid != newBlockSolid)
    {
        // bit of a hack for now, if the chunk isn't renderable, don't bother updating physics
        if (pChunk->GetMeshState() != BlockWorldChunk::MeshState_Pending && pChunk->GetRenderLODLevel() != m_lodLevels)
        {
            // wake any physics objects in-range
            //m_pPhysicsWorld->WakeObjectsInBox(AABox((float)bx, (float)by, (float)bz, (float)(bx + 1), (float)(by + 1), (float)(bz + 1)));
            m_pPhysicsWorld->UpdateSingleObject(pChunk->GetCollisionObject());
        }
    }

    return true;
}

void BlockWorld::OnEdgeBlockChanged(BlockWorldChunk *pChunk, int32 lx, int32 ly, int32 lz)
{
    const int32 chunkSizeMinusOne = m_chunkSize - 1;
    const int32 chunkX = pChunk->GetGlobalChunkX();
    const int32 chunkY = pChunk->GetGlobalChunkY();
    const int32 chunkZ = pChunk->GetGlobalChunkZ();
    BlockWorldChunk *pNeighbourChunk;

    if (lx == 0 && (pNeighbourChunk = GetChunk(chunkX - 1, chunkY, chunkZ)) != nullptr && pNeighbourChunk->GetRenderLODLevel() != m_lodLevels && pNeighbourChunk->GetMeshState() != BlockWorldChunk::MeshState_Pending)
        QueueSingleChunkForMeshing(pNeighbourChunk, pNeighbourChunk->GetRenderLODLevel());

    if (lx == chunkSizeMinusOne && (pNeighbourChunk = GetChunk(chunkX + 1, chunkY, chunkZ)) != nullptr && pNeighbourChunk->GetRenderLODLevel() != m_lodLevels && pNeighbourChunk->GetMeshState() != BlockWorldChunk::MeshState_Pending)
        QueueSingleChunkForMeshing(pNeighbourChunk, pNeighbourChunk->GetRenderLODLevel());

    if (ly == 0 && (pNeighbourChunk = GetChunk(chunkX, chunkY - 1, chunkZ)) != nullptr && pNeighbourChunk->GetRenderLODLevel() != m_lodLevels && pNeighbourChunk->GetMeshState() != BlockWorldChunk::MeshState_Pending)
        QueueSingleChunkForMeshing(pNeighbourChunk, pNeighbourChunk->GetRenderLODLevel());

    if (ly == chunkSizeMinusOne && (pNeighbourChunk = GetChunk(chunkX, chunkY + 1, chunkZ)) != nullptr && pNeighbourChunk->GetRenderLODLevel() != m_lodLevels && pNeighbourChunk->GetMeshState() != BlockWorldChunk::MeshState_Pending)
        QueueSingleChunkForMeshing(pNeighbourChunk, pNeighbourChunk->GetRenderLODLevel());

    if (lz == 0 && (pNeighbourChunk = GetChunk(chunkX, chunkY, chunkZ - 1)) != nullptr && pNeighbourChunk->GetRenderLODLevel() != m_lodLevels && pNeighbourChunk->GetMeshState() != BlockWorldChunk::MeshState_Pending)
        QueueSingleChunkForMeshing(pNeighbourChunk, pNeighbourChunk->GetRenderLODLevel());

    if (lz == chunkSizeMinusOne && (pNeighbourChunk = GetChunk(chunkX, chunkY, chunkZ + 1)) != nullptr && pNeighbourChunk->GetRenderLODLevel() != m_lodLevels && pNeighbourChunk->GetMeshState() != BlockWorldChunk::MeshState_Pending)
        QueueSingleChunkForMeshing(pNeighbourChunk, pNeighbourChunk->GetRenderLODLevel());
}

void BlockWorld::OnBlockChanged(BlockWorldChunk *pChunk, int32 lx, int32 ly, int32 lz)
{
    // set the changed flag on the section
    pChunk->GetSection()->SetLoadState(BlockWorldSection::LoadState_Changed);

    // update child lods
    //pSection->UpdateChunkLODLevels(pChunk, 0, lx, ly, lz);
    pChunk->UpdateLODs(0, lx, ly, lz);

    // queue for meshing, even if we're pending insertion, it'll result in a double meshing but it'll be up to date
    if (pChunk->GetMeshState() != BlockWorldChunk::MeshState_Pending && pChunk->GetRenderLODLevel() != m_lodLevels)
        QueueSingleChunkForMeshing(pChunk, pChunk->GetRenderLODLevel());
}

bool BlockWorld::SetBlockBit(int32 x, int32 y, int32 z, BlockWorldBlockType bit)
{
    // determine chunks
    int32 sx, sy;
    int32 lcx, lcy, lcz;
    int32 lx, ly, lz;
    SplitCoordinates(&sx, &sy, &lcx, &lcy, &lcz, &lx, &ly, &lz, x, y, z);

    // get section/chunk, we only modify lod 0
    BlockWorldSection *pSection = GetSection(sx, sy);
    if (pSection == nullptr || pSection->GetLoadedLODLevel() != 0)
        return false;

    // get the chunk, we don't create nonexistant chunks
    BlockWorldChunk *pChunk = pSection->SafeGetChunk(lcx, lcy, lcz);
    if (pChunk == nullptr)
        return false;

    // don't change the value if the bit is already set
    BlockWorldBlockType currentValue = pChunk->GetBlock(0, lx, ly, lz);
    if (currentValue != 0 && (currentValue & bit) == 0)
    {
        // add the bit
        pChunk->SetBlock(0, lx, ly, lz, currentValue | bit);
        OnBlockChanged(pChunk, lx, ly, lz);
    }

    return true;
}

bool BlockWorld::ClearBlockBit(int32 x, int32 y, int32 z, BlockWorldBlockType bit)
{
    // determine chunks
    int32 sx, sy;
    int32 lcx, lcy, lcz;
    int32 lx, ly, lz;
    SplitCoordinates(&sx, &sy, &lcx, &lcy, &lcz, &lx, &ly, &lz, x, y, z);

    // get section/chunk, we only modify lod 0
    BlockWorldSection *pSection = GetSection(sx, sy);
    if (pSection == nullptr || pSection->GetLoadedLODLevel() != 0)
        return false;

    // get the chunk, we don't create nonexistant chunks
    BlockWorldChunk *pChunk = pSection->SafeGetChunk(lcx, lcy, lcz);
    if (pChunk == nullptr)
        return false;

    // don't change the value if the bit is already cleared
    BlockWorldBlockType currentValue = pChunk->GetBlock(0, lx, ly, lz);
    if (currentValue != 0 && (currentValue & bit) != 0)
    {
        // add the bit
        pChunk->SetBlock(0, lx, ly, lz, currentValue & ~bit);
        OnBlockChanged(pChunk, lx, ly, lz);
    }

    return true;
}

int32 BlockWorld::CalculateSectionLODForViewer(int32 sectionX, int32 sectionY, const float3 &viewerPosition)
{
    /*
    float3 sectionCenter(pSection->GetBoundingBox().GetCenter());
    float currentDistanceSq = Math::Square((float)(m_sectionSizeInBlocks * 2));
    float distanceToSectionSq = pSection->GetBoundingBox().GetCenter().xy().SquaredDistance(viewerPosition.xy());

    int32 lodLevel;
    for (lodLevel = 0; lodLevel < m_lodLevels; lodLevel++)
    {
        if (distanceToSectionSq < currentDistanceSq)
            break;

        //currentDistanceSq *= 4.0f;
        currentDistanceSq = Math::Square((float)((m_sectionSizeInBlocks * 2) << (lodLevel + 1)));
    }

    //Log_DevPrintf("Section[%i,%i] dist = %.3f, lod = %i", pSection->GetSectionX(), pSection->GetSectionY(), Math::Sqrt(distanceToSectionSq), lodLevel);
    return lodLevel;*/

    //float3 sectionCenter(pSection->GetBoundingBox().GetCenter());
    //float distanceToSectionSq = pSection->GetBoundingBox().GetCenter().xy().SquaredDistance(viewerPosition.xy());

    const int32 VIEW_DISTANCE_IN_CHUNKS = 10;

    //float dx = Min(Math::Abs(static_cast<float>(sectionX * m_sectionSizeInBlocks) - viewerPosition.x), Math::Abs(static_cast<float>((sectionX + 1) * m_sectionSizeInBlocks) - viewerPosition.x));
    //float dy = Min(Math::Abs(static_cast<float>(sectionY * m_sectionSizeInBlocks) - viewerPosition.y), Math::Abs(static_cast<float>((sectionY + 1) * m_sectionSizeInBlocks) - viewerPosition.y));
    float dx = Math::Abs(static_cast<float>(sectionX * m_sectionSizeInBlocks + m_sectionSizeInBlocks / 2) - viewerPosition.x);
    float dy = Math::Abs(static_cast<float>(sectionY * m_sectionSizeInBlocks + m_sectionSizeInBlocks / 2) - viewerPosition.y);
    float squaredDistance = Math::Sqrt((dx * dx) + (dy * dy));

    // current distance
    //float viewDistanceForLOD = (float)Math::Square(m_sectionSizeInBlocks * VIEW_DISTANCE_IN_SECTIONS);
    float viewDistanceForLOD = (float)(m_chunkSize * VIEW_DISTANCE_IN_CHUNKS);
    int32 lodLevel;
    for (lodLevel = 0; lodLevel < m_lodLevels; lodLevel++)
    {
        if (squaredDistance <= viewDistanceForLOD)
            break;

        // multiplied by 2, then 2 again because each chunk covers twice the space
        viewDistanceForLOD *= (float)(2 << lodLevel);
    }

    // lodlevel
    //Log_DevPrintf("Section[%i,%i] dist = %.3f, lod = %i", sectionX, sectionY, Math::Sqrt(squaredDistance), lodLevel);
    //Log_DevPrintf("Section[%i,%i] dist = %.3f, lod = %i", sectionX, sectionY, squaredDistance, lodLevel);
    return lodLevel;
}

void BlockWorld::LoadNewInRangeSections()
{
    // number of sections in each direction from the observer that we will check for
    // TODO: move load/visible radius to cvar, unload/lod change timer for sections to avoid jarring
    const int32 SECTION_LOAD_RADIUS = 32;

    // loop through observers
    for (const ObserverEntry &observer : m_observers)
    {
        // start by finding the section the observer is sitting in
        int32 observerSectionX, observerSectionY;
        CalculateSectionForPosition(&observerSectionX, &observerSectionY, observer.Value);

        // find start/end section indices
        int32 startSectionX = Math::Clamp(observerSectionX - SECTION_LOAD_RADIUS, m_minSectionX, m_maxSectionX);
        int32 startSectionY = Math::Clamp(observerSectionY - SECTION_LOAD_RADIUS, m_minSectionY, m_maxSectionY);
        int32 endSectionX = Math::Clamp(observerSectionX + SECTION_LOAD_RADIUS, m_minSectionX, m_maxSectionX);
        int32 endSectionY = Math::Clamp(observerSectionY + SECTION_LOAD_RADIUS, m_minSectionY, m_maxSectionY);

        // do both positive and negative directions
        for (int32 checkSectionX = startSectionX; checkSectionX <= endSectionX; checkSectionX++)
        {
            for (int32 checkSectionY = startSectionY; checkSectionY <= endSectionY; checkSectionY++)
            {
                int32 checkSectionLOD = CalculateSectionLODForViewer(checkSectionX, checkSectionY, observer.Value);
                if (checkSectionLOD == m_lodLevels)
                    continue;

                // section exists?
                if (!IsSectionAvailable(checkSectionX, checkSectionY))
                    continue;

                // check the section is present, loaded
                BlockWorldSection *pSection = GetSection(checkSectionX, checkSectionY);
                if (pSection == nullptr || pSection->GetLoadedLODLevel() > checkSectionLOD)
                {
                    if (!LoadSection(checkSectionX, checkSectionY, checkSectionLOD, false))
                        continue;
                }
            }
        }
    }
}

void BlockWorld::UnloadOutOfRangeSections(float timeSinceLastUpdate)
{
    for (uint32 loadedSectionIndex = 0; loadedSectionIndex < m_loadedSections.GetSize(); )
    {
        BlockWorldSection *pSection = m_loadedSections[loadedSectionIndex];

        int32 sectionLOD = m_lodLevels;
        for (const ObserverEntry &observer : m_observers)
            sectionLOD = Min(sectionLOD, CalculateSectionLODForViewer(pSection->GetSectionX(), pSection->GetSectionY(), observer.Value));

        if (sectionLOD == m_lodLevels)
        {
            UnloadSection(pSection->GetSectionX(), pSection->GetSectionY());
            continue;
        }

        // drop the loaded lod down as well
        if (pSection->GetLoadedLODLevel() < sectionLOD)
        {
            // prevent unloading the section while the new lod is still being processed
            if (pSection->GetChunksPendingMeshing() == 0)
                LoadSection(pSection->GetSectionX(), pSection->GetSectionY(), sectionLOD, true);
        }

        loadedSectionIndex++;
    }
}

void BlockWorld::StreamSections(float timeSinceLastUpdate)
{
    // calculate the view ranges for each section at each lod level
    int32 viewRanges[BLOCK_WORLD_MAX_LOD_LEVELS];
    viewRanges[0] = CVars::r_block_world_section_load_radius.GetInt();
    for (int32 i = 1; i < m_lodLevels; i++)
        viewRanges[i] = viewRanges[i - 1] * 2;

    // square each view range to avoid sqrt later on
    int32 searchRadius = viewRanges[m_lodLevels - 1];
    for (int32 i = 0; i < m_lodLevels; i++)
        viewRanges[i] = Math::Square(viewRanges[i]);

    // calculate the global chunk of each observer
    int2 *observerSections = (int2 *)alloca(sizeof(int2) * m_observers.GetSize());
    uint32 observerCount = 0;
    for (uint32 i = 0; i < m_observers.GetSize(); i++)
    {
        int2 observerSection;
        CalculateSectionForPosition(&observerSection.x, &observerSection.y, m_observers[i].Value);

        // collapse duplicate entries
        uint32 matchIndex = 0;
        for (; matchIndex < observerCount; matchIndex++)
        {
            if (observerSections[matchIndex] == observerSection)
                break;
        }
        if (matchIndex == observerCount)
            observerSections[observerCount++] = observerSection;
    }

    // phase 1: unload any sections that are out of range, that way we don't waste time checking sections that we just loaded
    for (uint32 loadedSectionIndex = 0; loadedSectionIndex < m_loadedSections.GetSize(); )
    {
        BlockWorldSection *pSection = m_loadedSections[loadedSectionIndex];

        // find the smallest distance to this section
        int2 sectionCoordinates(pSection->GetSectionX(), pSection->GetSectionY());
        int32 minSectionRange = Y_INT32_MAX;
        for (uint32 i = 0; i < observerCount; i++)
        {
            int3 sectionDiff(sectionCoordinates - observerSections[i]);
            int32 sectionRange = sectionDiff.x * sectionDiff.x + sectionDiff.y * sectionDiff.y;
            minSectionRange = Min(minSectionRange, sectionRange);
        }

        // find the section lod level
        int32 sectionLODLevel;
        for (sectionLODLevel = 0; sectionLODLevel < m_lodLevels; sectionLODLevel++)
        {
            if (minSectionRange <= viewRanges[sectionLODLevel])
                break;
        }

        // is the current lod correct?
        if (pSection->GetLoadedLODLevel() != sectionLODLevel)
        {
            // if we are currently meshing and the new lod is lower (higher numerically), we can't do anything yet
            if (sectionLODLevel > pSection->GetLoadedLODLevel() && pSection->GetChunksPendingMeshing() != 0)
            {
                loadedSectionIndex++;
                continue;
            }

            // handle unloading a section
            if (sectionLODLevel == m_lodLevels)
            {
                UnloadSection(pSection->GetSectionX(), pSection->GetSectionY());
                continue;
            }

            // update the lod
            LoadSection(pSection->GetSectionX(), pSection->GetSectionY(), sectionLODLevel, true);
        }

        // next section
        loadedSectionIndex++;
    }

    // phase 2: search for searchRange sections around each observer's section, and only load those that aren't loaded (streaming to new lods is done above)
    m_queuedLoadSections.Clear();
    for (uint32 observerIndex = 0; observerIndex < observerCount; observerIndex++)
    {
        // find start/end section indices
        //int32 startSectionX = Math::Clamp(observerSections[observerIndex].x - searchRadius, m_minSectionX, m_maxSectionX);
        //int32 startSectionY = Math::Clamp(observerSections[observerIndex].y - searchRadius, m_minSectionY, m_maxSectionY);
        //int32 endSectionX = Math::Clamp(observerSections[observerIndex].x + searchRadius, m_minSectionX, m_maxSectionX);
        //int32 endSectionY = Math::Clamp(observerSections[observerIndex].y + searchRadius, m_minSectionY, m_maxSectionY);
        int32 startSectionX = (observerSections[observerIndex].x - searchRadius);
        int32 startSectionY = (observerSections[observerIndex].y - searchRadius);
        int32 endSectionX = (observerSections[observerIndex].x + searchRadius);
        int32 endSectionY = (observerSections[observerIndex].y + searchRadius);

        // do both positive and negative directions
        for (int32 checkSectionX = startSectionX; checkSectionX <= endSectionX; checkSectionX++)
        {
            for (int32 checkSectionY = startSectionY; checkSectionY <= endSectionY; checkSectionY++)
            {
                BlockWorldSection *pSection = GetSection(checkSectionX, checkSectionY);
                if (pSection != nullptr || !IsSectionAvailable(checkSectionX, checkSectionY))
                    continue;

                // yay, a new section to load.
                // find the smallest distance to this section
                int2 sectionCoordinates(checkSectionX, checkSectionY);
                int32 minSectionRange = Y_INT32_MAX;
                for (uint32 i = 0; i < observerCount; i++)
                {
                    int3 sectionDiff(sectionCoordinates - observerSections[i]);
                    int32 sectionRange = sectionDiff.x * sectionDiff.x + sectionDiff.y * sectionDiff.y;
                    minSectionRange = Min(minSectionRange, sectionRange);
                }

                // find the section lod level
                int32 sectionLODLevel;
                for (sectionLODLevel = 0; sectionLODLevel < m_lodLevels; sectionLODLevel++)
                {
                    if (minSectionRange <= viewRanges[sectionLODLevel])
                        break;
                }

                // should be in range to at least one observer since we're here
                if (sectionLODLevel == m_lodLevels)
                    continue;

                // queue for loading
                QueuedSectionLoad sectionLoction = { checkSectionX, checkSectionY, sectionLODLevel, minSectionRange };
                m_queuedLoadSections.Add(sectionLoction);
            }
        }
    }

    // sort them
    uint32 maxSectionsToLoad = CVars::r_block_world_max_sections_per_frame.GetUInt();
    uint32 sectionsLoaded = 0;
    m_queuedLoadSections.Sort([](const QueuedSectionLoad *left, const QueuedSectionLoad *right) { return (left->ViewRange - right->ViewRange); });
    for (QueuedSectionLoad &sectionLocation : m_queuedLoadSections)
    {
        // force load anything with a distance of zero, safe to break after since the zeros will be ordered at the front
        if (sectionLocation.ViewRange == 0 || sectionsLoaded < maxSectionsToLoad)
        {
            LoadSection(sectionLocation.SectionX, sectionLocation.SectionY, sectionLocation.LODLevel, false);
            sectionsLoaded++;
        }
        else
        {
            // no point searching any more
            break;
        }
    }
}

void BlockWorld::TransitionLoadedChunkRenderLODs()
{
    // calculate view ranges for each lod level
    int32 viewRanges[BLOCK_WORLD_MAX_LOD_LEVELS];
    viewRanges[0] = CVars::r_block_world_visible_radius.GetInt();
    for (int32 i = 1; i < m_lodLevels; i++)
        viewRanges[i] = viewRanges[i - 1] * 2;

    // square each view range to avoid sqrt later on
    for (int32 i = 0; i < m_lodLevels; i++)
        viewRanges[i] = Math::Square(viewRanges[i]);

    // calculate the global chunk of each observer
    uint32 observerCount = m_observers.GetSize();
    int3 *observerChunks = (int3 *)alloca(sizeof(int3) * observerCount);
    for (uint32 i = 0; i < observerCount; i++)
        CalculateChunkForPosition(&observerChunks[i].x, &observerChunks[i].y, &observerChunks[i].z, m_observers[i].Value);

    // for each loaded section, find if we need to switch the lod level
    for (BlockWorldSection *pSection : m_loadedSections)
    {
        // enumerate chunks
        pSection->EnumerateChunks([this, &viewRanges, observerCount, observerChunks, pSection](BlockWorldChunk *pChunk)
        {
            // find range in chunks to chunk
            int3 chunkCoordinates(pChunk->GetGlobalChunkX(), pChunk->GetGlobalChunkY(), pChunk->GetGlobalChunkZ());
            int32 minChunkRange = Y_INT32_MAX;
            for (uint32 i = 0; i < observerCount; i++)
            {
                int3 chunkDiff(chunkCoordinates - observerChunks[i]);
                int32 chunkRange = chunkDiff.x * chunkDiff.x + chunkDiff.y * chunkDiff.y;
                minChunkRange = Min(minChunkRange, chunkRange);
            }

            // find the chunk render level
            int32 chunkRenderLevel;
            for (chunkRenderLevel = 0; chunkRenderLevel < m_lodLevels; chunkRenderLevel++)
            {
                if (minChunkRange <= viewRanges[chunkRenderLevel])
                    break;
            }

            // cap at loaded lod level
            chunkRenderLevel = Max(chunkRenderLevel, pChunk->GetLoadedLODLevel());

            // level changed?
            if (pChunk->GetRenderLODLevel() != chunkRenderLevel)
            {
                // can't do anything for chunks that are pending insertion, have to wait for that to finish first.
                if (pChunk->GetMeshState() == BlockWorldChunk::MeshState_InProgress)
                    return;

                // chunk out of view completely?
                if (chunkRenderLevel == m_lodLevels)
                {
                    // remove the chunk's mesh
                    RemoveChunkMesh(pChunk);
                }
                else
                {
                    // if the chunk's neighbours aren't loaded, we can't change the level either
                    if (!IsChunkNeighboursLoaded(pChunk, chunkRenderLevel))
                        return;

                    // mesh the chunk at the new lod
                    QueueSingleChunkForMeshing(pChunk, chunkRenderLevel);
                }
            }
        });
    }
}

void BlockWorld::RemoveChunkMesh(BlockWorldChunk *pChunk)
{
    BlockWorldChunkRenderProxy *pRenderProxy = pChunk->GetRenderProxy();
    if (pRenderProxy != nullptr)
    {
        //Log_DevPrintf("BlockWorld::RemoveSectionMeshes: Removed render proxy for section[%i,%i] chunk [%i,%i,%i] lod %i", pSection->GetSectionX(), pSection->GetSectionY(), pSearchChunk->GetGlobalChunkStartX(), pSearchChunk->GetGlobalChunkStartY(), pSearchChunk->GetGlobalChunkStartZ(), pSearchChunk->GetLODLevel());
        m_pRenderWorld->RemoveRenderable(pRenderProxy);

        // remove the chunk
        pChunk->SetRenderProxy(nullptr);
        pRenderProxy->Release();
    }

    // is it pending?
    if (pChunk->GetMeshState() == BlockWorldChunk::MeshState_Pending)
    {
        for (uint32 i = 0; i < m_pendingChunks.GetSize(); i++)
        {
            if (m_pendingChunks[i].pChunk == pChunk)
            {
                m_pendingChunks.FastRemove(i);
                break;
            }
        }
    }

    // clear mesh state
    pChunk->SetMeshState(BlockWorldChunk::MeshState_Idle);
    pChunk->SetRenderLODLevel(m_lodLevels);
}

void BlockWorld::RemoveSectionMeshes(BlockWorldSection *pSection)
{
    pSection->EnumerateChunks([this, pSection](BlockWorldChunk *pSearchChunk)
    {
        RemoveChunkMesh(pSearchChunk);
    });
}

void BlockWorld::QueueSingleChunkForMeshing(BlockWorldChunk *pChunk, int32 lodLevel)
{
    BlockWorldSection *pSection = pChunk->GetSection();
    DebugAssert(lodLevel < m_lodLevels);

    // mesh is pending?
    if (pChunk->GetMeshState() == BlockWorldChunk::MeshState_Pending)
    {
        // search the pending list, and switch the lod level
        for (PendingMeshingChunk &pendingChunk : m_pendingChunks)
        {
            if (pendingChunk.pChunk == pChunk)
            {
                pChunk->SetRenderLODLevel(lodLevel);
                pendingChunk.NewLODLevel = lodLevel;
                return;
            }
        }
    }
    else if (pChunk->GetMeshState() == BlockWorldChunk::MeshState_InProgress)
    {
        // in progress in background, switch the state and bail
        pChunk->SetMeshState(BlockWorldChunk::MeshState_InProgressWithChanges);
        return;
    }

    // shouldn't be pending to be here.. this is more of a catcher of messed-up internal state
    DebugAssert(pChunk->GetMeshState() != BlockWorldChunk::MeshState_InProgress);
    DebugAssert(pChunk->GetMeshState() != BlockWorldChunk::MeshState_Pending);

    // queue it
    PendingMeshingChunk pendingChunk;
    pendingChunk.pSection = pSection;
    pendingChunk.pChunk = pChunk;
    pendingChunk.ViewCenter = pChunk->GetBoundingBox().GetCenter();
    pendingChunk.MinimumViewDistance = Y_FLT_INFINITE;
    pendingChunk.OldLODLevel = pChunk->GetRenderLODLevel();
    pendingChunk.NewLODLevel = lodLevel;
    m_pendingChunks.Add(pendingChunk);

    // add to section's pending chunk count
    pSection->AddChunkPendingMeshing();
    pChunk->SetMeshState(BlockWorldChunk::MeshState_Pending);
    pChunk->SetRenderLODLevel(lodLevel);
}

void BlockWorld::SortPendingMeshChunks()
{
    if (m_pendingChunks.IsEmpty())
        return;

    // for each chunk in the pending list, calculate it's minimum view distance to any observers
    for (PendingMeshingChunk &pmc : m_pendingChunks)
    {
        float minDistance = Y_FLT_INFINITE;
        for (const ObserverEntry &observer : m_observers)
            minDistance = Min(minDistance, pmc.ViewCenter.SquaredDistance(observer.Value));

        pmc.MinimumViewDistance = minDistance;
    }

    // sort the array according to this
    m_pendingChunks.Sort([](const BlockWorld::PendingMeshingChunk *left, const BlockWorld::PendingMeshingChunk *right) {
        return Math::CompareResult(left->MinimumViewDistance, right->MinimumViewDistance);
    });
}

void BlockWorld::MeshSingleChunk(BlockWorldSection *pSection, BlockWorldChunk *pChunk, int32 lodLevel)
{
    DebugAssert(pChunk->GetMeshState() == BlockWorldChunk::MeshState_Pending);

    // switch mesh state to in-progress
    pChunk->SetMeshState(BlockWorldChunk::MeshState_InProgress);
    m_chunksMeshingInProgress++;

    // grab the data from the chunk and adjacent chunks
    QUEUE_ASYNC_LAMBDA_COMMAND([this, pSection, pChunk, lodLevel]()
    {
        BlockWorldMesher *pBuilder = BlockWorldChunkRenderProxy::CreateMesher(this, pSection, pChunk, lodLevel);

        // if this chunk is new, we can push it to the background
        if (pChunk->GetRenderProxy() == nullptr)
        {
            // kick off the mesh operation in background
            QUEUE_BACKGROUND_LAMBDA_COMMAND([this, pSection, pChunk, lodLevel, pBuilder]()
            {
                Timer meshTimer;

                // mesh away
                pBuilder->GenerateMesh();
                if (meshTimer.GetTimeMilliseconds() > 30.0f)
                    Log_PerfPrintf("Background meshing of chunk %i/%i/%i lod %u took %.4fms", pChunk->GetGlobalChunkX(), pChunk->GetGlobalChunkY(), pChunk->GetGlobalChunkZ(), lodLevel, meshTimer.GetTimeMilliseconds());

                // anything generated?
                if (pBuilder->GetOutputBatchCount() == 0 && pBuilder->GetOutputLightCount() == 0 && pBuilder->GetOutputMeshInstancesCount() == 0)
                {
                    // wipe out the builder
                    delete pBuilder;
                }
                else
                {
                    // create the render proxy and bring it into the world
                    BlockWorldChunkRenderProxy *pRenderProxy = nullptr;
                    pRenderProxy = BlockWorldChunkRenderProxy::CreateForChunk(0, this, pSection, pChunk, lodLevel, pBuilder);
                    pChunk->SetRenderProxy(pRenderProxy);
                    m_pRenderWorld->AddRenderable(pRenderProxy);
                }

                // update the state on the main thread
                QUEUE_MAIN_THREAD_LAMBDA_COMMAND([this, pSection, pChunk, lodLevel]()
                {
                    // section has one less chunk outstanding
                    pSection->RemoveChunkPendingMeshing();
                    m_chunksMeshingInProgress--;

                    // has something re-queued us?
                    if (pChunk->GetMeshState() == BlockWorldChunk::MeshState_InProgressWithChanges)
                    {
                        // queue chunk for meshing
                        QueueSingleChunkForMeshing(pChunk, lodLevel);
                    }
                    else
                    {
                        // clear state
                        pChunk->SetMeshState(BlockWorldChunk::MeshState_Idle);
                    }
                });
            });
        }
        else
        {
            Timer meshTimer;

            // mesh away
            pBuilder->GenerateMesh();
            if (meshTimer.GetTimeMilliseconds() > 30.0f)
                Log_PerfPrintf("Async meshing of chunk %i/%i/%i lod %u took %.4fms", pChunk->GetGlobalChunkX(), pChunk->GetGlobalChunkY(), pChunk->GetGlobalChunkZ(), lodLevel, meshTimer.GetTimeMilliseconds());

            // this a re-mesh, so it must be done on the main thread
            QUEUE_MAIN_THREAD_LAMBDA_COMMAND([this, pSection, pChunk, lodLevel, pBuilder]()
            {
                BlockWorldChunkRenderProxy *pRenderProxy = pChunk->GetRenderProxy();
                DebugAssert(pRenderProxy != nullptr);

                // anything generated?
                if (pBuilder->GetOutputBatchCount() == 0 && pBuilder->GetOutputLightCount() == 0 && pBuilder->GetOutputMeshInstancesCount() == 0)
                {
                    delete pBuilder;

                    // clear render proxy if one exists
                    m_pRenderWorld->RemoveRenderable(pRenderProxy);
                    pChunk->SetRenderProxy(nullptr);
                    pRenderProxy->Release();
                }
                else
                {
                    // update the mesh on it
                    pRenderProxy->RebuildForChunk(this, pSection, pChunk, lodLevel, pBuilder);
                }

                // section has one less chunk outstanding
                pSection->RemoveChunkPendingMeshing();
                m_chunksMeshingInProgress--;

                // has something re-queued us?
                if (pChunk->GetMeshState() == BlockWorldChunk::MeshState_InProgressWithChanges)
                {
                    // queue chunk for meshing
                    QueueSingleChunkForMeshing(pChunk, lodLevel);
                }
                else
                {
                    // clear state
                    pChunk->SetMeshState(BlockWorldChunk::MeshState_Idle);
                }
            });
        }
    });
}

void BlockWorld::ProcessPendingMeshChunks()
{
    if (m_pendingChunks.IsEmpty())
    {
#if 0
        for (BlockWorldSection *pSection : m_loadedSections)
        {
            for (int32 lod = pSection->GetLoadedLODLevel(); lod < m_lodLevels; lod++)
            {
                pSection->EnumerateChunks(lod, [](BlockWorldChunk *pChunk) {
                    DebugAssert(pChunk->GetMeshState() == BlockWorldChunk::MeshState_Idle);
                });
            }
        }
#endif

        return;
    }

    // todo: limit this rate
    //uint32 chunksToMesh = m_pendingMeshChunks.GetSize();
    //uint32 chunksToMesh = Min(m_pendingChunks.GetSize(), (uint32)1);
    uint32 chunksToMesh = CVars::r_block_world_max_chunks_per_frame.GetUInt();
    uint32 chunksMeshed = 0;
    uint32 pendingChunkIndex = 0;
    for (; pendingChunkIndex < m_pendingChunks.GetSize(); pendingChunkIndex++)
    {
        // mesh a chunk!
        PendingMeshingChunk &pmc = m_pendingChunks[pendingChunkIndex];
        BlockWorldSection *pSection = pmc.pSection;

        // meshing a single chunk
        BlockWorldChunk *pChunk = pmc.pChunk;
        int32 lodLevel = pmc.NewLODLevel;
        /*QUEUE_ASYNC_LAMBDA_COMMAND([this, pSection, pChunk, lodLevel]() {
            MeshSingleChunk(pSection, pChunk, lodLevel);
        });*/

        // mesh chunk
        MeshSingleChunk(pSection, pChunk, lodLevel);

        // remove from list
        //pSection->RemoveChunkPendingMeshing();

        chunksMeshed++;
        if (chunksMeshed == chunksToMesh)
        {
            // this increment is necessary so that the pending entry is removed
            pendingChunkIndex++;
            break;
        }
    }

    // remove the chunks that were meshed
    if (pendingChunkIndex != 0)
        m_pendingChunks.RemoveRange(0, pendingChunkIndex);
}

bool BlockWorld::RayCastBlock(const Ray &ray, int32 *pBlockX, int32 *pBlockY, int32 *pBlockZ, CUBE_FACE *pBlockFace, BlockWorldBlockType *pBlockValue, float *pDistance) const
{
    // this is rather crude and can definitely be optimized with an octree to further eliminate tests
    // also, the current hit distance can be compared against when testing the section, to skip further tests

    // hit information
    const BlockWorldSection *pHitSection = nullptr;
    const BlockWorldChunk *pHitChunk = nullptr;
    int32 hitBlockX = 0, hitBlockY = 0, hitBlockZ = 0;
    BlockWorldBlockType hitBlockValue = 0;
    CUBE_FACE hitBlockFace = CUBE_FACE_COUNT;
    float hitBlockDistance = Y_FLT_INFINITE;

    // get bounding box of ray
    AABox rayBoundingBox(ray.GetAABox());

    // transform to section indices, clamp to actual section range
    int32 minSectionX = Math::Clamp(Math::Truncate(Math::Floor(rayBoundingBox.GetMinBounds().x / (float)m_sectionSizeInBlocks)), m_minSectionX, m_maxSectionX);
    int32 minSectionY = Math::Clamp(Math::Truncate(Math::Floor(rayBoundingBox.GetMinBounds().y / (float)m_sectionSizeInBlocks)), m_minSectionY, m_maxSectionY);
    int32 maxSectionX = Math::Clamp(Math::Truncate(Math::Ceil(rayBoundingBox.GetMaxBounds().x / (float)m_sectionSizeInBlocks)), m_minSectionX, m_maxSectionX);
    int32 maxSectionY = Math::Clamp(Math::Truncate(Math::Ceil(rayBoundingBox.GetMaxBounds().y / (float)m_sectionSizeInBlocks)), m_minSectionY, m_maxSectionY);

    // iterate over these sections
    for (int32 sectionY = minSectionY; sectionY <= maxSectionY; sectionY++)
    {
        for (int32 sectionX = minSectionX; sectionX <= maxSectionX; sectionX++)
        {
            // loaded at lod0? also check if it hits the section
            const BlockWorldSection *pSection = GetSection(sectionX, sectionY);
            if (pSection == nullptr || pSection->GetLoadedLODLevel() != 0 || ray.AABoxIntersectionTime(pSection->GetBoundingBox()) >= hitBlockDistance)
                continue;

            // move the search range into section space
            float3 rayBoundingBoxMinSectionSpace(rayBoundingBox.GetMinBounds() - pSection->GetBoundingBox().GetMinBounds());
            float3 rayBoundingBoxMaxSectionSpace(rayBoundingBox.GetMaxBounds() - pSection->GetBoundingBox().GetMinBounds());

            // convert to chunks
            int32 minChunkX = Math::Clamp(Math::Truncate(Math::Floor(rayBoundingBoxMinSectionSpace.x / (float)m_chunkSize)), 0, m_sectionSize - 1);
            int32 minChunkY = Math::Clamp(Math::Truncate(Math::Floor(rayBoundingBoxMinSectionSpace.y / (float)m_chunkSize)), 0, m_sectionSize - 1);
            int32 minChunkZ = Math::Clamp(Math::Truncate(Math::Floor(rayBoundingBoxMinSectionSpace.z / (float)m_chunkSize)), pSection->GetMinChunkZ(), pSection->GetMaxChunkZ());
            int32 maxChunkX = Math::Clamp(Math::Truncate(Math::Ceil(rayBoundingBoxMaxSectionSpace.x / (float)m_chunkSize)), 0, m_sectionSize - 1);
            int32 maxChunkY = Math::Clamp(Math::Truncate(Math::Ceil(rayBoundingBoxMaxSectionSpace.y / (float)m_chunkSize)), 0, m_sectionSize - 1);
            int32 maxChunkZ = Math::Clamp(Math::Truncate(Math::Ceil(rayBoundingBoxMaxSectionSpace.z / (float)m_chunkSize)), pSection->GetMinChunkZ(), pSection->GetMaxChunkZ());

            // iterate through these chunks
            for (int32 chunkZ = minChunkZ; chunkZ <= maxChunkZ; chunkZ++)
            {
                for (int32 chunkY = minChunkY; chunkY <= maxChunkY; chunkY++)
                {
                    for (int32 chunkX = minChunkX; chunkX <= maxChunkX; chunkX++)
                    {
                        // ensure chunk exists and actually intersects
                        const BlockWorldChunk *pChunk = pSection->GetChunk(chunkX, chunkY, chunkZ);
                        if (pChunk == nullptr || ray.AABoxIntersectionTime(pChunk->GetBoundingBox()) >= hitBlockDistance)
                            continue;

                        // we hit the chunk, so move the search range into chunk space
                        float3 rayBoundingBoxMinChunkSpace(rayBoundingBox.GetMinBounds() - pChunk->GetBasePosition());
                        float3 rayBoundingBoxMaxChunkSpace(rayBoundingBox.GetMaxBounds() - pChunk->GetBasePosition());

                        // convert to blocks
                        int32 minBlockX = Math::Clamp(Math::Truncate(Math::Floor(rayBoundingBoxMinChunkSpace.x)), 0, m_chunkSize - 1);
                        int32 minBlockY = Math::Clamp(Math::Truncate(Math::Floor(rayBoundingBoxMinChunkSpace.y)), 0, m_chunkSize - 1);
                        int32 minBlockZ = Math::Clamp(Math::Truncate(Math::Floor(rayBoundingBoxMinChunkSpace.z)), 0, m_chunkSize - 1);
                        int32 maxBlockX = Math::Clamp(Math::Truncate(Math::Ceil(rayBoundingBoxMaxChunkSpace.x)), 0, m_chunkSize - 1);
                        int32 maxBlockY = Math::Clamp(Math::Truncate(Math::Ceil(rayBoundingBoxMaxChunkSpace.y)), 0, m_chunkSize - 1);
                        int32 maxBlockZ = Math::Clamp(Math::Truncate(Math::Ceil(rayBoundingBoxMaxChunkSpace.z)), 0, m_chunkSize - 1);

                        // iterate through blocks
                        for (int32 blockZ = minBlockZ; blockZ <= maxBlockZ; blockZ++)
                        {
                            for (int32 blockY = minBlockY; blockY <= maxBlockY; blockY++)
                            {
                                for (int32 blockX = minBlockX; blockX <= maxBlockX; blockX++)
                                {
                                    // get block value
                                    BlockWorldBlockType blockValue = pChunk->GetBlock(0, blockX, blockY, blockZ);
                                    if (blockValue == 0)
                                        continue;

                                    // get block coordinates
                                    float3 blockMinBounds(pChunk->GetBasePosition() + float3((float)blockX, (float)blockY, (float)blockZ));
                                    float3 blockMaxBounds(blockMinBounds + 1.0f);

                                    // test an intersection :: todo: move this test to local chunk space instead of world space for accuracy?
                                    float thisBlockHitTime;
                                    CUBE_FACE thisBlockHitFace;
                                    if (!ray.AABoxIntersectionTimeFace(blockMinBounds, blockMaxBounds, &thisBlockHitTime, &thisBlockHitFace))
                                        continue;

                                    // less than current?
                                    if (thisBlockHitTime < hitBlockDistance)
                                    {
                                        pHitSection = pSection;
                                        pHitChunk = pChunk;
                                        hitBlockX = blockX;
                                        hitBlockY = blockY;
                                        hitBlockZ = blockZ;
                                        hitBlockValue = blockValue;
                                        hitBlockFace = thisBlockHitFace;
                                        hitBlockDistance = thisBlockHitTime;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // did we hit anything?
    if (pHitSection == nullptr)
        return false;

    // convert local coordinates to global coordinates
    ConvertChunkBlockCoordinatesToGlobalCoordinates(m_chunkSize, pHitChunk->GetGlobalChunkX(), pHitChunk->GetGlobalChunkY(), pHitChunk->GetGlobalChunkZ(), hitBlockX, hitBlockY, hitBlockZ, pBlockX, pBlockY, pBlockZ);
    *pBlockFace = hitBlockFace;
    *pBlockValue = hitBlockValue;
    *pDistance = hitBlockDistance;

    int tsx, tsy, tcx, tcy, tcz, tbx, tby, tbz;
    SplitCoordinates(&tsx, &tsy, &tcx, &tcy, &tcz, &tbx, &tby, &tbz, *pBlockX, *pBlockY, *pBlockZ);
    DebugAssert(tsx == pHitSection->GetSectionX() && tsy == pHitSection->GetSectionY() && tcx == pHitChunk->GetRelativeChunkX() && tcy == pHitChunk->GetRelativeChunkY() && tcz == pHitChunk->GetRelativeChunkZ() && tbx == hitBlockX && tby == hitBlockY && tbz == hitBlockZ);
    return true;
}

void BlockWorld::AddBrush(Brush *pObject)
{
    Panic("Brushes are not supported in BlockWorld");
}

void BlockWorld::RemoveBrush(Brush *pObject)
{
    Panic("Brushes are not supported in BlockWorld");
}

const Entity *BlockWorld::GetEntityByID(uint32 EntityId) const
{
    // lookup in hash table
    const EntityHashTable::Member *pEntityData = m_entityHashTable.Find(EntityId);
    if (pEntityData == nullptr)
        return nullptr;

    return pEntityData->Value;
}

Entity *BlockWorld::GetEntityByID(uint32 EntityId)
{
    // lookup in hash table
    const EntityHashTable::Member *pEntityData = m_entityHashTable.Find(EntityId);
    if (pEntityData == nullptr)
        return nullptr;

    return pEntityData->Value;
}

void BlockWorld::AddEntity(Entity *pEntity)
{
    DebugAssert(pEntity->GetEntityID() != 0);
    DebugAssert(GetEntityByID(pEntity->GetEntityID()) == nullptr);

    // entity added routine
    pEntity->AddRef();
    pEntity->OnAddToWorld(this);
    OnLoadEntity(pEntity);

    // handle global entities
    BlockWorldSection *pSection = nullptr;
    if (pEntity->GetMobility() != ENTITY_MOBILITY_GLOBAL)
    {
        // find it a section
        int32 newSectionX, newSectionY;
        CalculateSectionForPosition(&newSectionX, &newSectionY, pEntity->GetBoundingBox().GetCenter());

        // sections not at lod0 are useless
        pSection = GetSection(newSectionX, newSectionY);
        if (pSection != nullptr && pSection->GetLoadedLODLevel() != 0)
            pSection = nullptr;
    }

    // add to section
    if (pSection != nullptr)
    {
        // add him to the section
        pSection->AddEntity(pEntity);
        pEntity->SetWorldData(pSection);
    }
    else
    {
        // add to global list
        BlockWorldEntityReference entityRef;
        entityRef.pEntity = pEntity;
        entityRef.EntityID = pEntity->GetEntityID();
        entityRef.BoundingBox = pEntity->GetBoundingBox();
        entityRef.BoundingSphere = pEntity->GetBoundingSphere();
        m_globalEntityReferences.Add(entityRef);
        pEntity->SetWorldData(nullptr);
    }

    // update bounds
    m_worldBoundingBox.Merge(pEntity->GetBoundingBox());
    m_worldBoundingSphere.Merge(pEntity->GetBoundingSphere());
}

void BlockWorld::MoveEntity(Entity *pEntity)
{
    // find the section it lives in
    BlockWorldSection *pOldSection = reinterpret_cast<BlockWorldSection *>(pEntity->GetWorldData());
    DebugAssert(pOldSection == nullptr || m_loadedSections.Contains(pOldSection));

    // update bounds
    m_worldBoundingBox.Merge(pEntity->GetBoundingBox());
    m_worldBoundingSphere.Merge(pEntity->GetBoundingSphere());

    // handle global entities
    BlockWorldSection *pNewSection = nullptr;
    if (pEntity->GetMobility() != ENTITY_MOBILITY_GLOBAL)
    {
        // moving to a new section?
        int32 newSectionX, newSectionY;
        CalculateSectionForPosition(&newSectionX, &newSectionY, pEntity->GetBoundingBox().GetCenter());

        // changed? save a lookup if it hasn't
        if (pOldSection != nullptr && pOldSection->GetSectionX() == newSectionX && pOldSection->GetSectionY() == newSectionY)
        {
            pOldSection->MoveEntity(pEntity);
            return;
        }

        // sections that aren't at lod0 are useless
        pNewSection = GetSection(newSectionX, newSectionY);
        if (pNewSection != nullptr && pNewSection->GetLoadedLODLevel() != 0)
            pNewSection = nullptr;
    }    

    // did we previously not have a section?
    if (pOldSection == nullptr)
    {
        // removing or updating from global list?
        for (uint32 idx = 0; idx < m_globalEntityReferences.GetSize(); idx++)
        {
            if (m_globalEntityReferences[idx].pEntity == pEntity)
            {
                if (pNewSection != nullptr)
                {
                    // we have a new section, so remove from the global list
                    m_globalEntityReferences.FastRemove(idx);
                    break;
                }
                else
                {
                    // update the global list, and bail out
                    m_globalEntityReferences[idx].BoundingBox = pEntity->GetBoundingBox();
                    m_globalEntityReferences[idx].BoundingSphere = pEntity->GetBoundingSphere();
                    return;
                }
            }
        }
    }
    else
    {
        // removing from section
        pOldSection->RemoveEntity(pEntity);
    }

    // move into the new section
    if (pNewSection != nullptr)
    {
        // add it
        pNewSection->AddEntity(pEntity);
        pEntity->SetWorldData(pNewSection);
    }
    else
    {
        // add to global list
        BlockWorldEntityReference entityRef;
        entityRef.pEntity = pEntity;
        entityRef.EntityID = pEntity->GetEntityID();
        entityRef.BoundingBox = pEntity->GetBoundingBox();
        entityRef.BoundingSphere = pEntity->GetBoundingSphere();
        m_globalEntityReferences.Add(entityRef);
        pEntity->SetWorldData(nullptr);
    }
}

void BlockWorld::UpdateEntity(Entity *pEntity)
{
    BlockWorldSection *pSection = reinterpret_cast<BlockWorldSection *>(pEntity->GetWorldData());
    DebugAssert(pSection == nullptr || m_loadedSections.Contains(pSection));

    // if we're in a section, flag it as changed
    if (pSection != nullptr && pSection->GetLoadState() == BlockWorldSection::LoadState_Loaded)
        pSection->SetLoadState(BlockWorldSection::LoadState_Changed);
}

void BlockWorld::RemoveEntity(Entity *pEntity)
{
    DebugAssert(GetEntityByID(pEntity->GetEntityID()) == pEntity);

    // find the section it lives in
    BlockWorldSection *pSection = reinterpret_cast<BlockWorldSection *>(pEntity->GetWorldData());
    DebugAssert(pSection == nullptr || m_loadedSections.Contains(pSection));

    // remove from hash tables, update lists, etc
    OnUnloadEntity(pEntity);

    // remove from section
    if (pSection != nullptr)
    {
        pSection->RemoveEntity(pEntity);
    }
    else
    {
        for (uint32 idx = 0; idx < m_globalEntityReferences.GetSize(); idx++)
        {
            if (m_globalEntityReferences[idx].pEntity == pEntity)
            {
                m_globalEntityReferences.FastRemove(idx);
                break;
            }
        }
    }

    // delete entity
    pEntity->OnRemoveFromWorld(this);
    pEntity->Release();
}

void BlockWorld::OnLoadEntity(Entity *pEntity)
{
    // add it to the hash table
    DebugAssert(m_entityHashTable.Find(pEntity->GetEntityID()) == nullptr);
    m_entityHashTable.Insert(pEntity->GetEntityID(), pEntity);
}

void BlockWorld::OnUnloadEntity(Entity *pEntity)
{
    // ensure it isn't in the active queues
    for (uint32 j = 0; j < m_activeEntities.GetSize(); j++)
    {
        if (m_activeEntities[j].pEntity == pEntity)
        {
            m_activeEntities.FastRemove(j);
            SortActiveEntities();
            break;
        }
    }

    for (uint32 j = 0; j < m_activeAsyncEntities.GetSize(); j++)
    {
        if (m_activeAsyncEntities[j].pEntity == pEntity)
        {
            m_activeAsyncEntities.FastRemove(j);
            SortActiveAsyncEntities();
            break;
        }
    }

    // ensure it isn't in the removal queue
    int32 removeQueueIndex = m_removeQueue.IndexOf(pEntity);
    if (removeQueueIndex >= 0)
        m_removeQueue.FastRemove(removeQueueIndex);

    // remove from hash table
    BlockWorld::EntityHashTable::Member *pHashTableMember = m_entityHashTable.Find(pEntity->GetEntityID());
    DebugAssert(pHashTableMember != nullptr);
    m_entityHashTable.Remove(pHashTableMember);
}

void BlockWorld::BeginFrame(float deltaTime)
{
    World::BeginFrame(deltaTime);
}

void BlockWorld::UpdateAsync(float deltaTime)
{
    World::UpdateAsync(deltaTime);

    ProcessPendingMeshChunks();
}

void BlockWorld::Update(float deltaTime)
{
    World::Update(deltaTime);

    UpdateBlockAnimations(deltaTime);

    //UnloadOutOfRangeSections(deltaTime);
    //LoadNewInRangeSections();
    StreamSections(deltaTime);
    TransitionLoadedChunkRenderLODs();
    SortPendingMeshChunks();
}

void BlockWorld::EndFrame()
{
    World::EndFrame();
}

struct BlockLightingUpdateNode
{
    inline BlockLightingUpdateNode() {}
    inline BlockLightingUpdateNode(BlockWorldChunk *pChunk_, int32 x_, int32 y_, int32 z_) : pChunk(pChunk_), x(x_), y(y_), z(z_), level(0) {}
    inline BlockLightingUpdateNode(BlockWorldChunk *pChunk_, int32 x_, int32 y_, int32 z_, uint8 level_) : pChunk(pChunk_), x(x_), y(y_), z(z_), level(level_) {}
    inline BlockLightingUpdateNode(const BlockLightingUpdateNode &copy) { Y_memcpy(this, &copy, sizeof(*this)); }
    inline void Set(BlockWorldChunk *pChunk_, int32 x_, int32 y_, int32 z_) { pChunk = pChunk_; x = x_; y = y_; z = z_; level = 0;  }
    inline void Set(BlockWorldChunk *pChunk_, int32 x_, int32 y_, int32 z_, uint8 level_) { pChunk = pChunk_; x = x_; y = y_; z = z_; level = level_; }

    BlockWorldChunk *pChunk;
    int32 x;
    int32 y;
    int32 z;
    uint8 level;

    //////////////////////////////////////////////////////////////////////////
    inline bool GetLeftNode(BlockWorld *pWorld, BlockLightingUpdateNode *node) const
    {
        int32 chunkSizeMinusOne = pWorld->GetChunkSize() - 1;
        if (x == 0)
        {
            BlockWorldChunk *pAdjChunk;
            if ((pAdjChunk = pWorld->GetWritableChunk(pChunk->GetGlobalChunkX() - 1, pChunk->GetGlobalChunkY(), pChunk->GetGlobalChunkZ(), true)) == nullptr)
                return false;

            node->Set(pAdjChunk, chunkSizeMinusOne, this->y, this->z);
        }
        else
        {
            node->Set(this->pChunk, this->x - 1, this->y, this->z);
        }

        return true;
    }

    inline bool GetRightNode(BlockWorld *pWorld, BlockLightingUpdateNode *node) const
    {
        int32 chunkSizeMinusOne = pWorld->GetChunkSize() - 1;
        if (x == chunkSizeMinusOne)
        {
            BlockWorldChunk *pAdjChunk;
            if ((pAdjChunk = pWorld->GetWritableChunk(pChunk->GetGlobalChunkX() + 1, pChunk->GetGlobalChunkY(), pChunk->GetGlobalChunkZ(), true)) == nullptr)
                return false;

            node->Set(pAdjChunk, 0, this->y, this->z);
        }
        else
        {
            node->Set(this->pChunk, this->x + 1, this->y, this->z);
        }

        return true;
    }

    inline bool GetBackNode(BlockWorld *pWorld, BlockLightingUpdateNode *node) const
    {
        int32 chunkSizeMinusOne = pWorld->GetChunkSize() - 1;
        if (y == 0)
        {
            BlockWorldChunk *pAdjChunk;
            if ((pAdjChunk = pWorld->GetWritableChunk(pChunk->GetGlobalChunkX(), pChunk->GetGlobalChunkY() - 1, pChunk->GetGlobalChunkZ(), true)) == nullptr)
                return false;

            node->Set(pAdjChunk, this->x, chunkSizeMinusOne, this->z);
        }
        else
        {
            node->Set(this->pChunk, this->x, this->y - 1, this->z);
        }

        return true;
    }

    inline bool GetFrontNode(BlockWorld *pWorld, BlockLightingUpdateNode *node) const
    {
        int32 chunkSizeMinusOne = pWorld->GetChunkSize() - 1;
        if (y == chunkSizeMinusOne)
        {
            BlockWorldChunk *pAdjChunk;
            if ((pAdjChunk = pWorld->GetWritableChunk(pChunk->GetGlobalChunkX(), pChunk->GetGlobalChunkY() + 1, pChunk->GetGlobalChunkZ(), true)) == nullptr)
                return false;

            node->Set(pAdjChunk, this->x, 0, this->z);
        }
        else
        {
            node->Set(this->pChunk, this->x, this->y + 1, this->z);
        }

        return true;
    }

    inline bool GetBottomNode(BlockWorld *pWorld, BlockLightingUpdateNode *node) const
    {
        int32 chunkSizeMinusOne = pWorld->GetChunkSize() - 1;
        if (z == 0)
        {
            BlockWorldChunk *pAdjChunk;
            if ((pAdjChunk = pWorld->GetWritableChunk(pChunk->GetGlobalChunkX(), pChunk->GetGlobalChunkY(), pChunk->GetGlobalChunkZ() - 1, true)) == nullptr)
                return false;

            node->Set(pAdjChunk, this->x, this->y, chunkSizeMinusOne);
        }
        else
        {
            node->Set(this->pChunk, this->x, this->y, this->z - 1);
        }

        return true;
    }

    inline bool GetTopNode(BlockWorld *pWorld, BlockLightingUpdateNode *node) const
    {
        int32 chunkSizeMinusOne = pWorld->GetChunkSize() - 1;
        if (z == chunkSizeMinusOne)
        {
            BlockWorldChunk *pAdjChunk;
            if ((pAdjChunk = pWorld->GetWritableChunk(pChunk->GetGlobalChunkX(), pChunk->GetGlobalChunkY(), pChunk->GetGlobalChunkZ() + 1, true)) == nullptr)
                return false;

            node->Set(pAdjChunk, this->x, this->y, 0);
        }
        else
        {
            node->Set(this->pChunk, this->x, this->y, this->z + 1);
        }

        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    inline bool SpreadToNode(BlockWorld *pWorld, uint8 lightLevel)
    {
        // light cannot spread to translucent nodes
        BlockWorldBlockType blockValue = pChunk->GetBlock(0, x, y, z);
        if (pWorld->IsLightBlockingBlockValue(blockValue))
            return false;

        // check lightlevel is within threshold
        uint8 currentLightLevel = pChunk->GetBlockLight(0, x, y, z);
        if ((currentLightLevel + 2) <= lightLevel)
        {
            // update the light level of this node
            pChunk->SetBlockLight(0, x, y, z, lightLevel - 1);
            pWorld->OnBlockChanged(pChunk, x, y, z);
            this->level = lightLevel - 1;
            //Log_DevPrintf("  -- spread to [%i,%i,%i] %i,%i,%i %u", pChunk->GetGlobalChunkX(), pChunk->GetGlobalChunkY(), pChunk->GetGlobalChunkZ(), x, y, z, level);
            return true;
        }

        return false;
    }

    inline int32 UnspreadFromNode(BlockWorld *pWorld, uint8 lightLevel)
    {
        // check lightlevel is within threshold
        uint8 currentLightLevel = pChunk->GetBlockLight(0, x, y, z);
        if (currentLightLevel != 0 && currentLightLevel < lightLevel)
        {
            // set light level of node to zero, and store the level for future use
            pChunk->SetBlockLight(0, x, y, z, 0);
            pWorld->OnBlockChanged(pChunk, x, y, z);
            this->level = currentLightLevel;
            return -1;
        }
        else if (currentLightLevel >= lightLevel)
        {
            this->level = currentLightLevel;
            return 1;
        }

        return 0;
    }
};

void BlockWorld::SpreadBlockLighting(BlockWorldChunk *pChunk, int32 x, int32 y, int32 z, uint8 lightLevel)
{
    //Log_DevPrintf("SPREAD BLOCK LIGHTING: [%i,%i,%i] %i,%i,%i %u", pChunk->GetGlobalChunkX(), pChunk->GetGlobalChunkY(), pChunk->GetGlobalChunkZ(), x, y, z, lightLevel);

    // set light of node
    pChunk->SetBlockLight(0, x, y, z, lightLevel);
    OnBlockChanged(pChunk, x, y, z);

    // create queue and push node to it (this is gonna have a lot of memmoves.. use a better container)
    MemArray<BlockLightingUpdateNode> queue(128);
    queue.Add(BlockLightingUpdateNode(pChunk, x, y, z, lightLevel));

    // while the queue has elements
    while (!queue.IsEmpty())
    {
        // retrieve the front node from the queue
        BlockLightingUpdateNode node;
        queue.PopFront(&node);

        // spread it to each neighbor
        BlockLightingUpdateNode neighbourNode;
        if (node.GetLeftNode(this, &neighbourNode) && neighbourNode.SpreadToNode(this, node.level))
            queue.Add(neighbourNode);
        if (node.GetRightNode(this, &neighbourNode) && neighbourNode.SpreadToNode(this, node.level))
            queue.Add(neighbourNode);
        if (node.GetBackNode(this, &neighbourNode) && neighbourNode.SpreadToNode(this, node.level))
            queue.Add(neighbourNode);
        if (node.GetFrontNode(this, &neighbourNode) && neighbourNode.SpreadToNode(this, node.level))
            queue.Add(neighbourNode);
        if (node.GetBottomNode(this, &neighbourNode) && neighbourNode.SpreadToNode(this, node.level))
            queue.Add(neighbourNode);
        if (node.GetTopNode(this, &neighbourNode) && neighbourNode.SpreadToNode(this, node.level))
            queue.Add(neighbourNode);                
    }
}

void BlockWorld::UnspreadBlockLighting(BlockWorldChunk *pChunk, int32 x, int32 y, int32 z)
{
    // create queue and push node to it (this is gonna have a lot of memmoves.. use a better container)
    MemArray<BlockLightingUpdateNode> removeQueue(128);
    MemArray<BlockLightingUpdateNode> updateQueue(128);
    removeQueue.Add(BlockLightingUpdateNode(pChunk, x, y, z, pChunk->GetBlockLight(0, x, y, z)));

    // clear light of node
    pChunk->SetBlockLight(0, x, y, z, 0);
    OnBlockChanged(pChunk, x, y, z);

    // while the queue has elements
    while (!removeQueue.IsEmpty())
    {
        // retrieve the front node from the queue
        BlockLightingUpdateNode node;
        removeQueue.PopFront(&node);

        // create neighbour node
        BlockLightingUpdateNode neighbourNode;
        int32 unspreadResult;

        // spread it to each neighbor
        if (node.GetLeftNode(this, &neighbourNode))
        {
            unspreadResult = neighbourNode.UnspreadFromNode(this, node.level);
            if (unspreadResult < 0)
                removeQueue.Add(neighbourNode);
            else if (unspreadResult > 0)
                updateQueue.Add(neighbourNode);
        }

        if (node.GetRightNode(this, &neighbourNode))
        {
            unspreadResult = neighbourNode.UnspreadFromNode(this, node.level);
            if (unspreadResult < 0)
                removeQueue.Add(neighbourNode);
            else if (unspreadResult > 0)
                updateQueue.Add(neighbourNode);
        }

        if (node.GetBackNode(this, &neighbourNode))
        {
            unspreadResult = neighbourNode.UnspreadFromNode(this, node.level);
            if (unspreadResult < 0)
                removeQueue.Add(neighbourNode);
            else if (unspreadResult > 0)
                updateQueue.Add(neighbourNode);
        }

        if (node.GetFrontNode(this, &neighbourNode))
        {
            unspreadResult = neighbourNode.UnspreadFromNode(this, node.level);
            if (unspreadResult < 0)
                removeQueue.Add(neighbourNode);
            else if (unspreadResult > 0)
                updateQueue.Add(neighbourNode);
        }

        if (node.GetBottomNode(this, &neighbourNode))
        {
            unspreadResult = neighbourNode.UnspreadFromNode(this, node.level);
            if (unspreadResult < 0)
                removeQueue.Add(neighbourNode);
            else if (unspreadResult > 0)
                updateQueue.Add(neighbourNode);
        }

        if (node.GetTopNode(this, &neighbourNode))
        {
            unspreadResult = neighbourNode.UnspreadFromNode(this, node.level);
            if (unspreadResult < 0)
                removeQueue.Add(neighbourNode);
            else if (unspreadResult > 0)
                updateQueue.Add(neighbourNode);
        }
    }

    // process remaining propagation nodes
    while (!updateQueue.IsEmpty())
    {
        // retrieve the front node from the queue
        BlockLightingUpdateNode node;
        updateQueue.PopFront(&node);

        // spread it to each neighbor
        BlockLightingUpdateNode neighbourNode;
        if (node.GetLeftNode(this, &neighbourNode) && neighbourNode.SpreadToNode(this, node.level))
            updateQueue.Add(neighbourNode);
        if (node.GetRightNode(this, &neighbourNode) && neighbourNode.SpreadToNode(this, node.level))
            updateQueue.Add(neighbourNode);
        if (node.GetBackNode(this, &neighbourNode) && neighbourNode.SpreadToNode(this, node.level))
            updateQueue.Add(neighbourNode);
        if (node.GetFrontNode(this, &neighbourNode) && neighbourNode.SpreadToNode(this, node.level))
            updateQueue.Add(neighbourNode);
        if (node.GetBottomNode(this, &neighbourNode) && neighbourNode.SpreadToNode(this, node.level))
            updateQueue.Add(neighbourNode);
        if (node.GetTopNode(this, &neighbourNode) && neighbourNode.SpreadToNode(this, node.level))
            updateQueue.Add(neighbourNode);
    }
}

bool BlockWorld::CreateAnimatedPhysicsBlock(const float3 &basePosition, const Quaternion &rotation, BlockWorldBlockType blockValue, const float3 &forceVector, float despawnTime /*= 5.0f*/)
{
    // lookup block info
    const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType(blockValue);
    if (pBlockType->ShapeType != BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
        return false;

    // transform
    Transform animationTransform(basePosition, rotation, float3::One);

    // create render proxy
    BlockDrawTemplate::BlockRenderProxy *pRenderProxy = m_pBlockDrawTemplate->CreateBlockRenderProxy(blockValue, 0, animationTransform.GetTransformMatrix4x4(), 0);
    if (pRenderProxy == nullptr)
        return false;

    // calculate the physics block size
    float3 physicsBlockExtents(1.0f);

    // create the collision shape and rigid body
    AutoReleasePtr<Physics::BoxCollisionShape> pCollisionShape = new Physics::BoxCollisionShape(physicsBlockExtents * 0.5f, physicsBlockExtents * 0.5f);
    Physics::RigidBody *pRigidBody = new Physics::RigidBody(0, pCollisionShape, animationTransform);

    // add to world
    m_pRenderWorld->AddRenderable(pRenderProxy);
    m_pPhysicsWorld->AddObject(pRigidBody);

    // create physics block
    BlockAnimation blockAnimation;
    blockAnimation.BlockType = blockValue;
    blockAnimation.LocationX = Math::Truncate(basePosition.x);
    blockAnimation.LocationY = Math::Truncate(basePosition.y);
    blockAnimation.LocationZ = Math::Truncate(basePosition.z);
    blockAnimation.pRenderProxy = pRenderProxy;
    blockAnimation.LastTransform = animationTransform;
    blockAnimation.LifeTime = despawnTime;
    blockAnimation.TimeRemaining = despawnTime;
    blockAnimation.pRigidBody = pRigidBody;
    blockAnimation.AnimationFunction = EasingFunction::Linear;
    blockAnimation.AnimationStartTransform = Transform::Identity;
    blockAnimation.AnimationEndTransform = Transform::Identity;
    blockAnimation.SetAfterCompletion = false;
    m_blockAnimations.Add(blockAnimation);

    // apply the impulse
    if (forceVector.SquaredLength() > 0.0f)
        static_cast<Physics::RigidBody *>(pRigidBody)->ApplyCentralImpulse(forceVector);

    Log_DevPrintf("BlockWorld::CreateAnimatedPhysicsBlock: spawned '%s' at '%s'", pBlockType->Name.GetCharArray(), StringConverter::Vector3fToString(basePosition).GetCharArray());
    return true;
}

bool BlockWorld::CreateAnimatedPhysicsBlock(int32 x, int32 y, int32 z, const float3 &forceVector, bool removeBlock /*= true*/, float despawnTime /*= 5.0f*/)
{
    // retrieve the block value
    BlockWorldBlockType blockValue = GetBlockValue(x, y, z);
    if (blockValue == 0)
        return false;

    // calculate base position for this block
    float3 basePosition((float)x, (float)y, (float)z);

    // create physics block
    bool result = CreateAnimatedPhysicsBlock(basePosition, Quaternion::Identity, blockValue, forceVector, despawnTime);

    // remove the block from the world?
    if (removeBlock)
        ClearBlock(x, y, z);

    // done
    return result;
}

bool BlockWorld::CreateBlockAnimation(BlockWorldBlockType blockValue, BLOCK_WORLD_BLOCK_ROTATION blockRotation, const Transform &startTransform, const Transform &endTransform, float spawnTime /* = 1.0f */, EasingFunction::Type easingFunction /* = EasingFunction::Linear */, bool setAfterSpawn /* = false */, int32 setBlockX /* = 0 */, int32 setBlockY /* = 0 */, int32 setBlockZ /* = 0 */)
{
    // create render proxy
    const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType(blockValue);
    BlockDrawTemplate::BlockRenderProxy *pRenderProxy = m_pBlockDrawTemplate->CreateBlockRenderProxy(blockValue, 0, startTransform.GetTransformMatrix4x4(), 0);
    if (pRenderProxy == nullptr)
        return false;

    // add to world
    m_pRenderWorld->AddRenderable(pRenderProxy);

    // create physics block
    BlockAnimation blockAnimation;
    blockAnimation.BlockType = blockValue;
    blockAnimation.LocationX = setBlockX;
    blockAnimation.LocationY = setBlockY;
    blockAnimation.LocationZ = setBlockZ;
    blockAnimation.Rotation = blockRotation;
    blockAnimation.pRenderProxy = pRenderProxy;
    blockAnimation.LastTransform = startTransform;
    blockAnimation.LifeTime = spawnTime;
    blockAnimation.TimeRemaining = spawnTime;
    blockAnimation.pRigidBody = nullptr;
    blockAnimation.AnimationFunction = easingFunction;
    blockAnimation.AnimationStartTransform = startTransform;
    blockAnimation.AnimationEndTransform = endTransform;
    blockAnimation.SetAfterCompletion = setAfterSpawn;
    m_blockAnimations.Add(blockAnimation);

    Log_DevPrintf("BlockWorld::CreateBlockAnimation: spawned '%s' at '%s' moving to '%s'", pBlockType->Name.GetCharArray(), StringConverter::TransformToString(startTransform).GetCharArray(), StringConverter::TransformToString(endTransform).GetCharArray());

    // if setting afterwards, mark it as animation in progress
    if (setAfterSpawn)
        SetBlockType(setBlockX, setBlockY, setBlockZ, m_animationInProgressBlockType, BLOCK_WORLD_BLOCK_ROTATION_NORTH, true);

    return true;
}

void BlockWorld::SetBlockWithAnimation(int32 blockX, int32 blockY, int32 blockZ, BlockWorldBlockType blockValue, BLOCK_WORLD_BLOCK_ROTATION blockRotation, const Transform &startTransform, float spawnTime /* = 1.0f */, EasingFunction::Type easingFunction /* = EasingFunction::Linear */, bool setAfterSpawn /* = true */)
{
    // create end location
    float3 endLocation((float)blockX, (float)blockY, (float)blockZ);
    Transform endTransform(endLocation, Quaternion::Identity, float3::One);

    // pass through
    if (!CreateBlockAnimation(blockValue, blockRotation, startTransform, endTransform, spawnTime, easingFunction, setAfterSpawn, blockX, blockY, blockZ) && setAfterSpawn)
    {
        // still set it anyway
        SetBlockType(blockX, blockY, blockZ, blockValue, blockRotation, true);
    }
}

void BlockWorld::SetBlockWithAnimation(int32 blockX, int32 blockY, int32 blockZ, BlockWorldBlockType blockValue, BLOCK_WORLD_BLOCK_ROTATION blockRotation)
{
    // find the longest direction, up to a certain point, where there is no block
    const int32 MAX_RANGE = 2;
    const int32 values[CUBE_FACE_COUNT][3] =
    {
        { 1, 0, 0 },    // right
        { -1, 0, 0 },   // left
        { 0, 1, 0 },    // back
        { 0, -1, 0 },   // front
        { 0, 0, 1 },    // top
        { 0, 0, -1 },   // bottom
    };

    int32 bestSourceX, bestSourceY, bestSourceZ;
    int32 bestDirectionSteps = -1;
    for (int32 i = 0; i < CUBE_FACE_COUNT; i++)
    {
        int32 dirX = values[i][0];
        int32 dirY = values[i][1];
        int32 dirZ = values[i][2];
        int32 stepCount = -1;
        for (int32 steps = 1; steps <= MAX_RANGE; steps++)
        {
            int32 searchBlockX = blockX + dirX * steps;
            int32 searchBlockY = blockY + dirY * steps;
            int32 searchBlockZ = blockZ + dirZ * steps;
            if (GetBlockValue(searchBlockX, searchBlockY, searchBlockZ) == 0)
                stepCount = steps;
            else
                break;
        }

        if (stepCount < bestDirectionSteps)
        {
            // no point trying any more
            continue;
        }
        else if (stepCount == bestDirectionSteps)
        {
            // random chance to use this direction
            if (!g_pEngine->GetRandomNumberGenerator()->NextBoolean(50.0f))
                continue;
        }

        bestSourceX = blockX + dirX * stepCount;
        bestSourceY = blockY + dirY * stepCount;
        bestSourceZ = blockZ + dirZ * stepCount;
        bestDirectionSteps = stepCount;
    }

    if (bestDirectionSteps < 0)
    {
        // just set the block, can't animate it from anywhere
        SetBlockType(blockX, blockY, blockZ, blockValue, blockRotation, true);
    }
    else
    {
        // use an animation
        float3 sourceLocation((float)bestSourceX, (float)bestSourceY, (float)bestSourceZ);
        SetBlockWithAnimation(blockX, blockY, blockZ, blockValue, blockRotation, Transform(sourceLocation, Quaternion::Identity, float3::One), 0.2f, EasingFunction::Quadratic, true);
    }
}

void BlockWorld::UpdateBlockAnimations(float deltaTime)
{
    static const float BLOCK_START_FADEOUT_TIME = 1.0f;

    for (uint32 blockAnimationIndex = 0; blockAnimationIndex < m_blockAnimations.GetSize();)
    {
        BlockAnimation *pAnimation = &m_blockAnimations[blockAnimationIndex];

        // update time remaining
        pAnimation->TimeRemaining -= deltaTime;

        // physics block?
        if (pAnimation->pRigidBody != nullptr)
        {
            // handle timeouts
            if (pAnimation->TimeRemaining <= 0.0f)
            {
                m_pPhysicsWorld->RemoveObject(pAnimation->pRigidBody);
                pAnimation->pRigidBody->Release();
                m_pRenderWorld->RemoveRenderable(pAnimation->pRenderProxy);
                pAnimation->pRenderProxy->Release();
                m_blockAnimations.OrderedRemove(blockAnimationIndex);
                continue;
            }

            // pull the transform from the rigid body, test if it's changed, if so, update the render proxy
            Transform newTransform(pAnimation->pRigidBody->GetTransform());
            if (newTransform != pAnimation->LastTransform)
                pAnimation->pRenderProxy->SetTransform(newTransform.GetTransformMatrix4x4());

            // handle fade-out
            if (pAnimation->TimeRemaining <= BLOCK_START_FADEOUT_TIME)
            {
                float opacity = Math::Saturate(pAnimation->TimeRemaining / BLOCK_START_FADEOUT_TIME);
                pAnimation->pRenderProxy->SetTintColor(MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, Math::Truncate(opacity * 255.0f)));
            }
        }
        // spawn block?
        else
        {
            // handle timeouts
            if (pAnimation->TimeRemaining <= 0.0f)
            {
                // handle set after spawn
                if (pAnimation->SetAfterCompletion)
                {
                    // we keep the animation around for one more frame, to allow the chunk to re-mesh
                    SetBlockType(pAnimation->LocationX, pAnimation->LocationY, pAnimation->LocationZ, pAnimation->BlockType, BLOCK_WORLD_BLOCK_ROTATION_NORTH, true);
                    pAnimation->SetAfterCompletion = false;
                    pAnimation->TimeRemaining = 0.0f;
                }
                else
                {
                    // remove it
                    m_pRenderWorld->RemoveRenderable(pAnimation->pRenderProxy);
                    pAnimation->pRenderProxy->Release();
                    m_blockAnimations.OrderedRemove(blockAnimationIndex);
                    continue;
                }
            }

            // update the transform
            float factor = EasingFunction::GetCoefficient(pAnimation->AnimationFunction, 1.0f - (pAnimation->TimeRemaining / pAnimation->LifeTime));
            Transform newTransform(Transform::LinearInterpolate(pAnimation->AnimationStartTransform, pAnimation->AnimationEndTransform, factor));
            pAnimation->pRenderProxy->SetTransform(newTransform.GetTransformMatrix4x4());
        }

        // next block
        blockAnimationIndex++;
    }
}

