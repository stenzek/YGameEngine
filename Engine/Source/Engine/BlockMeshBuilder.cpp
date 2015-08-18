#include "Engine/PrecompiledHeader.h"
#include "Engine/BlockMeshBuilder.h"
#include "Engine/BlockMeshVolume.h"
#include "Renderer/Renderer.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Renderer/VertexFactories/BlockMeshVertexFactory.h"
//Log_SetChannel(BlockMeshBuilder);

enum BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT
{
    // For lighting
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_LEFT            = (1 << 6),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_MIDDLE          = (1 << 7),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_RIGHT           = (1 << 8),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_LEFT          = (1 << 9),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_RIGHT         = (1 << 10),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_LEFT           = (1 << 11),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_MIDDLE         = (1 << 12),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_RIGHT          = (1 << 13),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_LEFT         = (1 << 14),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_MIDDLE       = (1 << 15),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_RIGHT        = (1 << 16),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_LEFT       = (1 << 17),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_RIGHT      = (1 << 18),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_LEFT        = (1 << 19),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_MIDDLE      = (1 << 20),
    BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_RIGHT       = (1 << 21),
};

const uint32 MATERIAL_INDEX_SHADOW_BIT_MASK = ((uint32)1 << 31);
const uint32 MATERIAL_INDEX_SHADOW_BIT_SET = ((uint32)1 << 31);
const uint32 MATERIAL_INDEX_MASK = ~((uint32)1 << 31);

BlockMeshBuilder::BlockMeshBuilder()
    : m_pPalette(NULL),
      m_width(0),
      m_length(0),
      m_height(0),
      m_pBlockData(NULL),
      m_translation(float3::Zero),
      m_scale(1.0f),
      m_ambientOcclusion(false),
      m_outputVertexFactoryFlags(0),
      m_outputBoundingBox(AABox::Zero),
      m_outputBoundingSphere(Sphere::Zero)
{
    Y_memzero(m_pNeighbourBlockData, sizeof(m_pNeighbourBlockData));
}

BlockMeshBuilder::~BlockMeshBuilder()
{

}

void BlockMeshBuilder::SetFromVolume(const BlockMeshVolume *pVolume)
{
    m_width = pVolume->GetWidth();
    m_length = pVolume->GetLength();
    m_height = pVolume->GetHeight();
    m_pBlockData = pVolume->GetData();
    m_translation = float3((float)pVolume->GetMinCoordinates().x, (float)pVolume->GetMinCoordinates().y, (float)pVolume->GetMinCoordinates().z) * pVolume->GetScale();    
    m_scale = pVolume->GetScale();

    if (pVolume->GetPalette() != NULL)
        m_pPalette = pVolume->GetPalette();
}

const BlockVolumeBlockType BlockMeshBuilder::GetBlockValueAt(uint32 x, uint32 y, uint32 z) const
{
    DebugAssert(x < m_width && y < m_length && z < m_height);
    return m_pBlockData[(z * m_width * m_length) + (y * m_width) + x];
}

const BlockVolumeBlockType BlockMeshBuilder::GetNeighbourBlockValueAt(NEIGHBOUR_VOLUME neighbour, uint32 x, uint32 y, uint32 z) const
{
    DebugAssert(neighbour < NEIGHBOUR_VOLUME_COUNT);
    if (m_pNeighbourBlockData[neighbour] == nullptr)
        return 0;

    DebugAssert(x < m_width && y < m_length && z < m_height);
    return m_pNeighbourBlockData[neighbour][(z * m_width * m_length) + (y * m_width) + x];
}

const bool BlockMeshBuilder::IsVisibleBlockAt(uint32 x, uint32 y, uint32 z) const
{
    BlockVolumeBlockType blockTypeId = GetBlockValueAt(x, y, z);
    const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType(blockTypeId);
    return (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE) != 0;
}

const bool BlockMeshBuilder::IsVisibleNonTransparentBlockAt(uint32 x, uint32 y, uint32 z) const
{
    BlockVolumeBlockType blockTypeId = GetBlockValueAt(x, y, z);
    const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType(blockTypeId);
    return (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE && !(pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE));
}

const bool BlockMeshBuilder::HasCubeLightBlockingBlockAt(uint32 x, uint32 y, uint32 z) const
{
    BlockVolumeBlockType blockValue = GetBlockValueAt(x, y, z);
    if (blockValue == 0)
        return false;

    const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType(blockValue);
    if (pBlockType->ShapeType != BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
        return false;

    return (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE) != 0;
}

const bool BlockMeshBuilder::HasCubeVisibilityBlockingBlockAt(const BlockPalette::BlockType *pVolumeBlockType, int32 x, int32 y, int32 z) const
{
    BlockVolumeBlockType blockValue = 0;
    int32 width = (int32)m_width;
    int32 length = (int32)m_length;
    int32 height = (int32)m_height;

    // can handle one direction of negativeness
    if (x < 0)
        blockValue = (y >= 0 && z >= 0 && y < length && z < height) ? GetNeighbourBlockValueAt(NEIGHBOUR_VOLUME_LEFT, m_width - 1, y, z) : 0;
    else if (x == width)
        blockValue = (y >= 0 && z >= 0 && y < length && z < height) ? GetNeighbourBlockValueAt(NEIGHBOUR_VOLUME_RIGHT, 0, y, z) : 0;
    else if (y < 0)
        blockValue = (x >= 0 && z >= 0 && x < width && z < height) ? GetNeighbourBlockValueAt(NEIGHBOUR_VOLUME_BACK, x, m_length - 1, z) : 0;
    else if (y == length)
        blockValue = (x >= 0 && z >= 0 && x < width && z < height) ? GetNeighbourBlockValueAt(NEIGHBOUR_VOLUME_FRONT, x, 0, z) : 0;
    else if (z < 0)
        blockValue = (x >= 0 && y >= 0 && x < width && y < length) ? GetNeighbourBlockValueAt(NEIGHBOUR_VOLUME_BOTTOM, x, y, m_height - 1) : 0;
    else if (z == height)
        blockValue = (x >= 0 && y >= 0 && x < width && y < length) ? GetNeighbourBlockValueAt(NEIGHBOUR_VOLUME_TOP, x, y, 0) : 0;
    else
        blockValue = GetBlockValueAt((uint32)x, (uint32)y, (uint32)z);

    if (blockValue == 0)
        return false;

    const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType(blockValue);

    // is volume and next to each other
    if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_VOLUME && blockValue == pVolumeBlockType->BlockTypeIndex)
        return true;

    // is not cube
    if (pBlockType->ShapeType != BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
        return false;

    // or visibility-blocking/solid
    if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCKS_VISIBILITY)
        return true;

    return false;
}

const uint32 BlockMeshBuilder::CalculateCubeVertexColor(const BlockPalette::BlockType *pBlockType, const uint32 vertexIndex, const uint32 blockData, uint32 faceColor)
{
    float4 outVertexColor;

    // start by taking the face colour
    outVertexColor = float4(float(faceColor & 0xFF), float((faceColor >> 8) & 0xFF), float((faceColor >> 16) & 0xFF), float((faceColor >> 24) & 0xFF)) / 255.0f;

    // calculate the vertex AO
    if (!(pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_VOLUME))
    {
        uint32 lightLevel = 4;
        switch (vertexIndex)
        {
        case 0:
            // top back left
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_LEFT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_LEFT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_MIDDLE) != 0) ? 1 : 0;
            break;

        case 1:
            // top back right
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_RIGHT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_RIGHT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_MIDDLE) != 0) ? 1 : 0;
            break;

        case 2:
            // top front left
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_LEFT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_LEFT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_MIDDLE) != 0) ? 1 : 0;
            break;

        case 3:
            // top front right
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_RIGHT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_RIGHT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_MIDDLE) != 0) ? 1 : 0;
            break;

        case 4:
            // bottom back left
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_LEFT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_LEFT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_MIDDLE) != 0) ? 1 : 0;
            break;

        case 5:
            // bottom back right
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_RIGHT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_RIGHT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_MIDDLE) != 0) ? 1 : 0;
            break;

        case 6:
            // bottom front left
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_LEFT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_LEFT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_MIDDLE) != 0) ? 1 : 0;
            break;

        case 7:
            // bottom front right
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_RIGHT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_RIGHT) != 0) ? 1 : 0;
            lightLevel -= ((blockData & BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_MIDDLE) != 0) ? 1 : 0;
            break;
        }

        // multiply that
        static const float lightLevels[5] = { 0.0f, 0.4f, 0.6f, 0.8f, 1.0f };
        outVertexColor *= float4(lightLevels[lightLevel], lightLevels[lightLevel], lightLevels[lightLevel], 1.0f);
    }

    // return vertex colour
    {
        uint8 r, g, b, a;
        r = (uint8)Math::Clamp(outVertexColor.r * 255.0f, 0.0f, 255.0f);
        g = (uint8)Math::Clamp(outVertexColor.g * 255.0f, 0.0f, 255.0f);
        b = (uint8)Math::Clamp(outVertexColor.b * 255.0f, 0.0f, 255.0f);
        a = (uint8)Math::Clamp(outVertexColor.a * 255.0f, 0.0f, 255.0f);
        return MAKE_COLOR_R8G8B8A8_UNORM(r, g, b, a);
    }
}

#define BLOCK_VALUE_ARRAY_ACCESS(x, y, z) m_pBlockData[(z) * zStride + (y) * yStride + (x)]
#define BLOCK_DATA_ARRAY_ACCESS(x, y, z) pBlockDataArray[(z) * zStride + (y) * yStride + (x)]
#define BLOCK_WORLD_COORDINATES(x, y, z) (SIMDVector3f((float)x, (float)y, (float)z) * blockScale + basePosition)

void BlockMeshBuilder::GenerateBlocks(float3 &outMinBounds, float3 &outMaxBounds)
{
    static const uint32 ALL_FACES_ALLOCATED_MASK = 0x3F;

    const float blockScale = m_scale;
    const uint32 yStride = m_width;
    const uint32 zStride = yStride * m_length;

    const BlockPalette::BlockType *pBlockType;
    BlockVolumeBlockType blockValue, searchBlockValue;
    uint32 blockData, searchBlockData;
    bool tileMismatchedData;
    uint32 x, y, z;

    SIMDVector3f basePosition(m_translation);
    uint32 *pBlockDataArray = new uint32[m_width * m_length * m_height];
    uint32 meshSizes[3] = { m_width, m_length, m_height };
    bool isEdgeZBlockPos, isEdgeZBlockNeg;
    bool isEdgeYBlockPos, isEdgeYBlockNeg;
    bool isEdgeXBlockPos, isEdgeXBlockNeg;

    // strip out faces that have blocks adjacent to them.
    for (z = 0; z < m_height; z++)
    {
        isEdgeZBlockNeg = (z == 0);
        isEdgeZBlockPos = (z == (m_height - 1));

        for (y = 0; y < m_length; y++)
        {
            isEdgeYBlockNeg = (y == 0);
            isEdgeYBlockPos = (y == (m_length - 1));

            for (x = 0; x < m_width; x++)
            {
                isEdgeXBlockNeg = (x == 0);
                isEdgeXBlockPos = (x == (m_width - 1));

                // get block type
                blockValue = BLOCK_VALUE_ARRAY_ACCESS(x, y, z);
                if (blockValue == 0)
                {
                    BLOCK_DATA_ARRAY_ACCESS(x, y, z) = 0;
                    continue;
                }

                pBlockType = m_pPalette->GetBlockType(blockValue);
                if (!(pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE))
                {
                    BLOCK_DATA_ARRAY_ACCESS(x, y, z) = 0;
                    continue;
                }

                if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE || pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB)
                {
                    // get block data
                    blockData = ALL_FACES_ALLOCATED_MASK;

                    // Culling values
                    if (HasCubeVisibilityBlockingBlockAt(pBlockType, (int32)x + 1, (int32)y + 0, (int32)z + 0)) { blockData &= ~(1 << CUBE_FACE_RIGHT); }
                    if (HasCubeVisibilityBlockingBlockAt(pBlockType, (int32)x - 1, (int32)y + 0, (int32)z + 0)) { blockData &= ~(1 << CUBE_FACE_LEFT); }
                    if (HasCubeVisibilityBlockingBlockAt(pBlockType, (int32)x + 0, (int32)y + 1, (int32)z + 0)) { blockData &= ~(1 << CUBE_FACE_BACK); }
                    if (HasCubeVisibilityBlockingBlockAt(pBlockType, (int32)x + 0, (int32)y - 1, (int32)z + 0)) { blockData &= ~(1 << CUBE_FACE_FRONT); }
                    if (HasCubeVisibilityBlockingBlockAt(pBlockType, (int32)x + 0, (int32)y + 0, (int32)z + 1)) { blockData &= ~(1 << CUBE_FACE_TOP); }                    
                    if (HasCubeVisibilityBlockingBlockAt(pBlockType, (int32)x + 0, (int32)y + 0, (int32)z - 1)) { blockData &= ~(1 << CUBE_FACE_BOTTOM); }

                    // work out the block data
                    if (m_ambientOcclusion)
                    {
                        if (!isEdgeZBlockNeg)
                        {
                            if (!isEdgeYBlockNeg)
                            {
                                blockData |= (uint32)HasCubeLightBlockingBlockAt(x + 0, y - 1, z - 1) << 20;        // bottom-front-middle

                                if (!isEdgeXBlockNeg)
                                    blockData |= (uint32)HasCubeLightBlockingBlockAt(x - 1, y - 1, z - 1) << 19;        // bottom-front-left

                                if (!isEdgeXBlockPos)
                                    blockData |= (uint32)HasCubeLightBlockingBlockAt(x + 1, y - 1, z - 1) << 21;        // bottom-front-right
                            }
                            if (!isEdgeYBlockPos)
                            {
                                blockData |= (uint32)HasCubeLightBlockingBlockAt(x + 0, y + 1, z - 1) << 15;        // bottom-back-middle

                                if (!isEdgeXBlockNeg)
                                    blockData |= (uint32)HasCubeLightBlockingBlockAt(x - 1, y + 1, z - 1) << 14;        // bottom-back-left

                                if (!isEdgeXBlockPos)
                                    blockData |= (uint32)HasCubeLightBlockingBlockAt(x + 1, y + 1, z - 1) << 16;        // bottom-back-right
                            }

                            if (!isEdgeXBlockNeg)
                                blockData |= (uint32)HasCubeLightBlockingBlockAt(x - 1, y + 0, z - 1) << 17;        // bottom-middle-left

                            if (!isEdgeXBlockPos)
                                blockData |= (uint32)HasCubeLightBlockingBlockAt(x + 1, y + 0, z - 1) << 18;        // bottom-middle-right
                        }
                        if (!isEdgeZBlockPos)
                        {
                            if (!isEdgeYBlockNeg)
                            {
                                blockData |= (uint32)HasCubeLightBlockingBlockAt(x + 0, y - 1, z + 1) << 12;        // top-front-middle

                                if (!isEdgeXBlockNeg)
                                    blockData |= (uint32)HasCubeLightBlockingBlockAt(x - 1, y - 1, z + 1) << 11;        // top-front-left

                                if (!isEdgeXBlockPos)
                                    blockData |= (uint32)HasCubeLightBlockingBlockAt(x + 1, y - 1, z + 1) << 13;        // top-front-right
                            }
                            if (!isEdgeYBlockPos)
                            {
                                blockData |= (uint32)HasCubeLightBlockingBlockAt(x + 0, y + 1, z + 1) << 7;         // top-back-middle

                                if (!isEdgeXBlockNeg)
                                    blockData |= (uint32)HasCubeLightBlockingBlockAt(x - 1, y + 1, z + 1) << 6;         // top-back-left

                                if (!isEdgeXBlockPos)
                                    blockData |= (uint32)HasCubeLightBlockingBlockAt(x + 1, y + 1, z + 1) << 8;         // top-back-right
                            }

                            if (!isEdgeXBlockNeg)
                                blockData |= (uint32)HasCubeLightBlockingBlockAt(x - 1, y + 0, z + 1) << 9;         // top-middle-left

                            if (!isEdgeXBlockPos)
                                blockData |= (uint32)HasCubeLightBlockingBlockAt(x + 1, y + 0, z + 1) << 10;        // top-middle-right
                        }
                    }

                    // store face mask
                    BLOCK_DATA_ARRAY_ACCESS(x, y, z) = blockData;
                }
                else if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE)
                {
                    // planes get set to 0 when generated
                    BLOCK_DATA_ARRAY_ACCESS(x, y, z) = 1;
                }
                else
                {
                    // mark all faces as generated, so we don't come back to them
                    BLOCK_DATA_ARRAY_ACCESS(x, y, z) = 0;
                }
            }
        }
    }

    for (z = 0; z < m_height; z++)
    {
        for (y = 0; y < m_length; y++)
        {
            for (x = 0; x < m_width; x++)
            {
                // get block type
                blockValue = BLOCK_VALUE_ARRAY_ACCESS(x, y, z);
                if (blockValue == 0 || (pBlockType = m_pPalette->GetBlockType(blockValue))->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_NONE)
                    continue;

                // get block data
                blockData = BLOCK_DATA_ARRAY_ACCESS(x, y, z);

                // switch shape type
                if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
                {
                    tileMismatchedData = (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_VOLUME) != 0;
                    //tileMismatchedData = true;
                    //tileMismatchedData = false;

                    // generate each face
                    for (uint32 faceIndex = 0; faceIndex < CUBE_FACE_COUNT; faceIndex++)
                    {
                        uint32 currentFaceMask = (1 << faceIndex);
                        if ((blockData & currentFaceMask) == 0)
                            continue;

                        // determine which axis to sweep
                        uint32 sliceAxis = 0, sweepAxis0 = 0, sweepAxis1 = 0;
                        switch (faceIndex)
                        {
                        case CUBE_FACE_RIGHT:
                        case CUBE_FACE_LEFT:
                            sliceAxis = 0;
                            sweepAxis0 = 1;
                            sweepAxis1 = 2;
                            break;

                        case CUBE_FACE_BACK:
                        case CUBE_FACE_FRONT:
                            sliceAxis = 1;
                            sweepAxis0 = 0;
                            sweepAxis1 = 2;
                            break;

                        case CUBE_FACE_TOP:
                        case CUBE_FACE_BOTTOM:
                            sliceAxis = 2;
                            sweepAxis0 = 1;
                            sweepAxis1 = 0;
                            break;
                        }

                        // mask of lighting bits that we care about for this face
                        static const uint32 faceLightingMatchMasks[CUBE_FACE_COUNT] =
                        {
                            // CUBE_FACE_RIGHT
                            BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_MIDDLE | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_MIDDLE | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_MIDDLE | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_MIDDLE,

                            // CUBE_FACE_LEFT
                            BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_MIDDLE | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_MIDDLE | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_MIDDLE | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_MIDDLE,

                            // CUBE_FACE_BACK
                            BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_MIDDLE | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_MIDDLE | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_RIGHT,

                            // CUBE_FACE_FRONT
                            BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_MIDDLE | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_MIDDLE | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_RIGHT,

                            // CUBE_FACE_TOP
                            BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_MIDDLE | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_BACK_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_MIDDLE | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_MIDDLE_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_TOP_FRONT_RIGHT,

                            // CUBE_FACE_BOTTOM
                            BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_MIDDLE | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_BACK_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_LEFT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_MIDDLE | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_MIDDLE_RIGHT | BLOCK_MESH_BLOCK_DATA_NEIGHBOUR_PRESENT_BOTTOM_FRONT_RIGHT,
                        };

                        // get the lighting bit mask
                        uint32 faceLightingMatchMask = faceLightingMatchMasks[faceIndex];
                        uint32 faceLightingMask = blockData & faceLightingMatchMask;

                        // determine the quad boundaries by sweeping each axis, then in reverse
                        uint32 baseCoordinates[3];
                        uint32 blockCoordinates[3];
                        uint32 quadBoundaries[2][3];
                        uint32 i, j, k;

#if 1
                        // base
                        baseCoordinates[0] = x;
                        baseCoordinates[1] = y;
                        baseCoordinates[2] = z;
                        Y_memcpy(&quadBoundaries[0], baseCoordinates, sizeof(quadBoundaries[0]));
                        Y_memcpy(&quadBoundaries[1], baseCoordinates, sizeof(quadBoundaries[1]));

                        // set slice axis
                        blockCoordinates[sliceAxis] = baseCoordinates[sliceAxis];

                        // forward...
                        {
                            // axis 0
                            for (i = quadBoundaries[0][sweepAxis0] + 1; i < meshSizes[sweepAxis0]; i++)
                            {
                                blockCoordinates[sweepAxis0] = i;
                                blockCoordinates[sweepAxis1] = baseCoordinates[sweepAxis1];
                                searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                searchBlockData = BLOCK_DATA_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                if ((searchBlockValue == blockValue) &&
                                    (searchBlockData & currentFaceMask) &&
                                    (tileMismatchedData || (searchBlockData & faceLightingMatchMask) == faceLightingMask))
                                {
                                    quadBoundaries[1][sweepAxis0]++;
                                    continue;
                                }
                                else
                                {
                                    break;
                                }
                            }

                            // axis 1
                            for (i = quadBoundaries[0][sweepAxis1] + 1; i < meshSizes[sweepAxis1]; i++)
                            {
                                blockCoordinates[sweepAxis1] = i;

                                // has to match everything on axis 0
                                bool merge = true;
                                for (j = quadBoundaries[0][sweepAxis0]; j <= quadBoundaries[1][sweepAxis0]; j++)
                                {
                                    blockCoordinates[sweepAxis0] = j;
                                    searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                    searchBlockData = BLOCK_DATA_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                    if ((searchBlockValue == blockValue) &&
                                        (searchBlockData & currentFaceMask) &&
                                        (tileMismatchedData || (searchBlockData & faceLightingMatchMask) == faceLightingMask))
                                    {
                                        continue;
                                    }
                                    else
                                    {
                                        merge = false;
                                        break;
                                    }
                                }

                                // ok?
                                if (merge)
                                    quadBoundaries[1][sweepAxis1]++;
                                else
                                    break;
                            }
                        }

#elif 0
                        int32 quadBoundariesForward[2][3];
                        int32 quadBoundariesReverse[2][3];

                        // base
                        baseCoordinates[0] = x;
                        baseCoordinates[1] = y;
                        baseCoordinates[2] = z;
                        Y_memcpy(&quadBoundariesForward[0], baseCoordinates, sizeof(quadBoundariesForward[0]));
                        Y_memcpy(&quadBoundariesForward[1], baseCoordinates, sizeof(quadBoundariesForward[1]));
                        Y_memcpy(quadBoundariesReverse, quadBoundariesForward, sizeof(quadBoundariesReverse));

                        // set slice axis
                        blockCoordinates[sliceAxis] = baseCoordinates[sliceAxis];

                        // forward...
                        {
                            // axis 0
                            for (i = quadBoundariesForward[0][sweepAxis0] + 1; i < meshSizes[sweepAxis0]; i++)
                            {
                                blockCoordinates[sweepAxis0] = i;
                                blockCoordinates[sweepAxis1] = baseCoordinates[sweepAxis1];
                                searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                searchBlockData = BLOCK_DATA_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                if ((searchBlockValue == blockValue) &&
                                    (searchBlockData & currentFaceMask) &&
                                    (tileMismatchedData || (searchBlockData & faceLightingMatchMask) == faceLightingMask))
                                {
                                    quadBoundariesForward[1][sweepAxis0]++;
                                    continue;
                                }
                                else
                                {
                                    break;
                                }
                            }

                            // axis 1
                            for (i = quadBoundariesForward[0][sweepAxis1] + 1; i < meshSizes[sweepAxis1]; i++)
                            {
                                blockCoordinates[sweepAxis1] = i;

                                // has to match everything on axis 0
                                bool merge = true;
                                for (j = quadBoundariesForward[0][sweepAxis0]; j <= quadBoundariesForward[1][sweepAxis0]; j++)
                                {
                                    blockCoordinates[sweepAxis0] = j;
                                    searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                    searchBlockData = BLOCK_DATA_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                    if ((searchBlockValue == blockValue) &&
                                        (searchBlockData & currentFaceMask) &&
                                        (tileMismatchedData || (searchBlockData & faceLightingMatchMask) == faceLightingMask))
                                    {
                                        continue;
                                    }
                                    else
                                    {
                                        merge = false;
                                        break;
                                    }
                                }

                                // ok?
                                if (merge)
                                    quadBoundariesForward[1][sweepAxis1]++;
                                else
                                    break;
                            }
                        }

                        // reverse
                        {
                            // axis 0
                            for (i = quadBoundariesReverse[0][sweepAxis1] + 1; i < meshSizes[sweepAxis0]; i++)
                            {
                                blockCoordinates[sweepAxis0] = baseCoordinates[sweepAxis0];
                                blockCoordinates[sweepAxis1] = i;
                                searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                searchBlockData = BLOCK_DATA_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                if ((searchBlockValue == blockValue) &&
                                    (searchBlockData & currentFaceMask) &&
                                    (tileMismatchedData || (searchBlockData & faceLightingMatchMask) == faceLightingMask))
                                {
                                    quadBoundariesReverse[1][sweepAxis1]++;
                                    continue;
                                }
                                else
                                {
                                    break;
                                }
                            }

                            // axis 1
                            for (i = quadBoundariesReverse[0][sweepAxis0] + 1; i < meshSizes[sweepAxis1]; i++)
                            {
                                blockCoordinates[sweepAxis0] = i;

                                // has to match everything on axis 0
                                bool merge = true;
                                for (j = quadBoundariesReverse[0][sweepAxis1]; j <= quadBoundariesReverse[1][sweepAxis1]; j++)
                                {
                                    blockCoordinates[sweepAxis1] = j;
                                    searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                    searchBlockData = BLOCK_DATA_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                    if ((searchBlockValue == blockValue) &&
                                        (searchBlockData & currentFaceMask) &&
                                        (tileMismatchedData || (searchBlockData & faceLightingMatchMask) == faceLightingMask))
                                    {
                                        continue;
                                    }
                                    else
                                    {
                                        merge = false;
                                        break;
                                    }
                                }

                                // ok?
                                if (merge)
                                    quadBoundariesReverse[1][sweepAxis0]++;
                                else
                                    break;
                            }
                        }

                        // take the best one
                        int32 areaForward = ((quadBoundariesForward[1][sweepAxis0] - quadBoundariesForward[0][sweepAxis0] + 1) * (quadBoundariesForward[1][sweepAxis1] - quadBoundariesForward[1][sweepAxis1] + 1));
                        int32 areaReverse = ((quadBoundariesForward[1][sweepAxis0] - quadBoundariesForward[0][sweepAxis0] + 1) * (quadBoundariesForward[1][sweepAxis1] - quadBoundariesForward[1][sweepAxis1] + 1));
                        if (areaReverse > areaForward)
                            Y_memcpy(quadBoundaries, quadBoundariesReverse, sizeof(quadBoundaries));
                        else
                            Y_memcpy(quadBoundaries, quadBoundariesReverse, sizeof(quadBoundaries));

#endif

                        // mark all the faces as generated    
                        for (i = quadBoundaries[0][2]; i <= quadBoundaries[1][2]; i++)
                        {
                            for (j = quadBoundaries[0][1]; j <= quadBoundaries[1][1]; j++)
                            {
                                for (k = quadBoundaries[0][0]; k <= quadBoundaries[1][0]; k++)
                                {
                                    // mark as allocated
                                    BLOCK_DATA_ARRAY_ACCESS(k, j, i) &= ~currentFaceMask;
                                }
                            }
                        }

                        // update the cached data too
                        blockData &= ~currentFaceMask;

                        // get uvs and color
                        const BlockPalette::BlockType::CubeShapeFace *pFaceDef = &pBlockType->CubeShapeFaces[faceIndex];
                        uint32 faceMaterialIndex = pFaceDef->Visual.MaterialIndex;
                        uint32 faceColor = pFaceDef->Visual.Color;
                        float3 faceMinUV = pFaceDef->Visual.MinUV;
                        float3 faceMaxUV = pFaceDef->Visual.MaxUV;
                        const float4 &faceAtlasUVRange = pFaceDef->Visual.AtlasUVRange;

                        // set msb in material index if the block type does not allow shadows
                        if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CAST_SHADOWS)
                            faceMaterialIndex |= MATERIAL_INDEX_SHADOW_BIT_SET;

                        // repeat the texture
                        uint32 blockCountX = (quadBoundaries[1][0] - quadBoundaries[0][0]) + 1;
                        uint32 blockCountY = (quadBoundaries[1][1] - quadBoundaries[0][1]) + 1;
                        uint32 blockCountZ = (quadBoundaries[1][2] - quadBoundaries[0][2]) + 1;
                        switch (faceIndex)
                        {
                        case CUBE_FACE_RIGHT:
                        case CUBE_FACE_LEFT:
                            faceMaxUV.x *= (float)blockCountY;
                            faceMaxUV.y *= (float)blockCountZ;
                            break;

                        case CUBE_FACE_BACK:
                        case CUBE_FACE_FRONT:
                            faceMaxUV.x *= (float)blockCountX;
                            faceMaxUV.y *= (float)blockCountZ;
                            break;

                        case CUBE_FACE_TOP:
                        case CUBE_FACE_BOTTOM:
                            faceMaxUV.x *= (float)blockCountX;
                            faceMaxUV.y *= (float)blockCountY;
                            break;
                        }

                        /*
                        0 top back left
                        1 top back right
                        2 top front left
                        3 top front right
                        4 bottom back left
                        5 bottom back right
                        6 bottom front left
                        7 bottom front right

                        x, y, z = 6
                        sx, y, z = 7
                        sx, sy, z = 5
                        sx, sy, sz = 1
                        x, sy, z = 4
                        x, sy, sz = 0
                        x, y, sz = 2
                        sx, y, sz = 3
                        */

                        // generate vertices
                        uint32 baseVertex = m_outputVertices.GetSize();

                        // generate quads
                        switch (faceIndex)
                        {
                        case CUBE_FACE_RIGHT:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 1, blockData, faceColor), CUBE_FACE_RIGHT));// top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 3, blockData, faceColor), CUBE_FACE_RIGHT));           // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 5, blockData, faceColor), CUBE_FACE_RIGHT));           // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[0][2]), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 7, blockData, faceColor), CUBE_FACE_RIGHT));                      // bottom-right
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_RIGHT, baseVertex + 0, baseVertex + 1, baseVertex + 2));
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_RIGHT, baseVertex + 1, baseVertex + 3, baseVertex + 2));
                            }
                            break;

                        case CUBE_FACE_LEFT:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 0, blockData, faceColor), CUBE_FACE_LEFT));           // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 2, blockData, faceColor), CUBE_FACE_LEFT));                      // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 4, blockData, faceColor), CUBE_FACE_LEFT));                      // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 6, blockData, faceColor), CUBE_FACE_LEFT));                                 // bottom-right
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_LEFT, baseVertex + 0, baseVertex + 2, baseVertex + 1));
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_LEFT, baseVertex + 1, baseVertex + 2, baseVertex + 3));
                            }
                            break;

                        case CUBE_FACE_BACK:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 0, blockData, faceColor), CUBE_FACE_BACK));           // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 1, blockData, faceColor), CUBE_FACE_BACK)); // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 4, blockData, faceColor), CUBE_FACE_BACK));                      // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 5, blockData, faceColor), CUBE_FACE_BACK));            // bottom-right
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_BACK, baseVertex + 0, baseVertex + 1, baseVertex + 2));
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_BACK, baseVertex + 1, baseVertex + 3, baseVertex + 2));
                            }
                            break;

                        case CUBE_FACE_FRONT:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 2, blockData, faceColor), CUBE_FACE_FRONT));                     // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 3, blockData, faceColor), CUBE_FACE_FRONT));           // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 6, blockData, faceColor), CUBE_FACE_FRONT));                                // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[0][2]), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 7, blockData, faceColor), CUBE_FACE_FRONT));                      // bottom-right
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_FRONT, baseVertex + 0, baseVertex + 2, baseVertex + 1));
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_FRONT, baseVertex + 1, baseVertex + 2, baseVertex + 3));
                            }
                            break;

                        case CUBE_FACE_TOP:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 0, blockData, faceColor), CUBE_FACE_TOP));            // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 1, blockData, faceColor), CUBE_FACE_TOP));  // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 2, blockData, faceColor), CUBE_FACE_TOP));                       // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 3, blockData, faceColor), CUBE_FACE_TOP));             // bottom-right
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_TOP, baseVertex + 0, baseVertex + 2, baseVertex + 1));
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_TOP, baseVertex + 1, baseVertex + 2, baseVertex + 3));
                            }
                            break;

                        case CUBE_FACE_BOTTOM:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 4, blockData, faceColor), CUBE_FACE_BOTTOM));                    // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 5, blockData, faceColor), CUBE_FACE_BOTTOM));          // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 6, blockData, faceColor), CUBE_FACE_BOTTOM));                               // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[0][2]), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 7, blockData, faceColor), CUBE_FACE_BOTTOM));                     // bottom-right
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_BOTTOM, baseVertex + 0, baseVertex + 1, baseVertex + 2));
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_BOTTOM, baseVertex + 1, baseVertex + 3, baseVertex + 2));
                            }
                            break;
                        }

                        // update bounds
                        outMinBounds = outMinBounds.Min(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]));
                        outMaxBounds = outMaxBounds.Max(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1));
                    }
                }
                else if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB)
                {
                    // slabs have two behaviours, depending on if volume bit is set
                    // if volume bit is set, they are generated like cubes, except the very top block
                    // otherwise, they are generated independantly                  
                    bool isVolumeBlock = (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_VOLUME) != 0;

                    // generate each face
                    for (uint32 faceIndex = 0; faceIndex < CUBE_FACE_COUNT; faceIndex++)
                    {
                        uint32 currentFaceMask = (1 << faceIndex);
                        if ((blockData & currentFaceMask) == 0)
                            continue;

                        // determine which axis to sweep
                        int32 sliceAxis = 0, sweepAxis0 = 0, sweepAxis1 = 0;
                        switch (faceIndex)
                        {
                        case CUBE_FACE_RIGHT:
                        case CUBE_FACE_LEFT:
                            sliceAxis = 0;
                            sweepAxis0 = 1;
                            sweepAxis1 = (isVolumeBlock) ? 2 : -1;
                            break;

                        case CUBE_FACE_BACK:
                        case CUBE_FACE_FRONT:
                            sliceAxis = 1;
                            sweepAxis0 = 0;
                            sweepAxis1 = (isVolumeBlock) ? 2 : -1;
                            break;

                        case CUBE_FACE_TOP:
                        case CUBE_FACE_BOTTOM:
                            sliceAxis = 2;
                            sweepAxis0 = 1;
                            sweepAxis1 = 0;
                            break;
                        }

                        // determine the quad boundaries by sweeping each axis, then in reverse
                        uint32 baseCoordinates[3];
                        uint32 blockCoordinates[3];
                        uint32 quadBoundaries[2][3];
                        uint32 i, j, k;

                        // base
                        baseCoordinates[0] = x;
                        baseCoordinates[1] = y;
                        baseCoordinates[2] = z;
                        Y_memcpy(&quadBoundaries[0], baseCoordinates, sizeof(quadBoundaries[0]));
                        Y_memcpy(&quadBoundaries[1], baseCoordinates, sizeof(quadBoundaries[1]));

                        // set slice axis
                        blockCoordinates[sliceAxis] = baseCoordinates[sliceAxis];

                        // forward...
                        {
                            // axis 0
                            for (i = quadBoundaries[0][sweepAxis0] + 1; i < meshSizes[sweepAxis0]; i++)
                            {
                                blockCoordinates[sweepAxis0] = i;
                                blockCoordinates[sweepAxis1] = baseCoordinates[sweepAxis1];
                                searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                searchBlockData = BLOCK_DATA_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                if ((searchBlockValue == blockValue) &&
                                    (searchBlockData & currentFaceMask))
                                {
                                    quadBoundaries[1][sweepAxis0]++;
                                    continue;
                                }
                                else
                                {
                                    break;
                                }
                            }

                            // axis 1
                            if (sweepAxis1 >= 0)
                            {
                                for (i = quadBoundaries[0][sweepAxis1] + 1; i < meshSizes[sweepAxis1]; i++)
                                {
                                    blockCoordinates[sweepAxis1] = i;

                                    // has to match everything on axis 0
                                    bool merge = true;
                                    for (j = quadBoundaries[0][sweepAxis0]; j <= quadBoundaries[1][sweepAxis0]; j++)
                                    {
                                        blockCoordinates[sweepAxis0] = j;
                                        searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                        searchBlockData = BLOCK_DATA_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                        if ((searchBlockValue == blockValue) &&
                                            (searchBlockData & currentFaceMask))
                                        {
                                            continue;
                                        }
                                        else
                                        {
                                            merge = false;
                                            break;
                                        }
                                    }

                                    // ok?
                                    if (merge)
                                        quadBoundaries[1][sweepAxis1]++;
                                    else
                                        break;
                                }
                            }
                        }

                        // mark all the faces as generated    
                        for (i = quadBoundaries[0][2]; i <= quadBoundaries[1][2]; i++)
                        {
                            for (j = quadBoundaries[0][1]; j <= quadBoundaries[1][1]; j++)
                            {
                                for (k = quadBoundaries[0][0]; k <= quadBoundaries[1][0]; k++)
                                {
                                    // mark as allocated
                                    BLOCK_DATA_ARRAY_ACCESS(k, j, i) &= ~currentFaceMask;
                                }
                            }
                        }

                        // update the cached data too
                        blockData &= ~currentFaceMask;

                        // get uvs and color
                        const BlockPalette::BlockType::CubeShapeFace *pFaceDef = &pBlockType->CubeShapeFaces[faceIndex];
                        uint32 faceMaterialIndex = pFaceDef->Visual.MaterialIndex;
                        uint32 faceColor = pFaceDef->Visual.Color;
                        float3 faceMinUV = pFaceDef->Visual.MinUV;
                        float3 faceMaxUV = pFaceDef->Visual.MaxUV;
                        const float4 &faceAtlasUVRange = pFaceDef->Visual.AtlasUVRange;

                        // set msb in material index if the block type does not allow shadows
                        if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CAST_SHADOWS)
                            faceMaterialIndex |= MATERIAL_INDEX_SHADOW_BIT_SET;

                        // repeat the texture
                        uint32 blockCountX = (quadBoundaries[1][0] - quadBoundaries[0][0]) + 1;
                        uint32 blockCountY = (quadBoundaries[1][1] - quadBoundaries[0][1]) + 1;
                        uint32 blockCountZ = (quadBoundaries[1][2] - quadBoundaries[0][2]) + 1;
                        switch (faceIndex)
                        {
                        case CUBE_FACE_RIGHT:
                        case CUBE_FACE_LEFT:
                            faceMaxUV.x *= (float)blockCountY;
                            faceMaxUV.y *= (float)blockCountZ;
                            break;

                        case CUBE_FACE_BACK:
                        case CUBE_FACE_FRONT:
                            faceMaxUV.x *= (float)blockCountX;
                            faceMaxUV.y *= (float)blockCountZ;
                            break;

                        case CUBE_FACE_TOP:
                        case CUBE_FACE_BOTTOM:
                            faceMaxUV.x *= (float)blockCountX;
                            faceMaxUV.y *= (float)blockCountY;
                            break;
                        }

                        /*
                        0 top back left
                        1 top back right
                        2 top front left
                        3 top front right
                        4 bottom back left
                        5 bottom back right
                        6 bottom front left
                        7 bottom front right

                        x, y, z = 6
                        sx, y, z = 7
                        sx, sy, z = 5
                        sx, sy, sz = 1
                        x, sy, z = 4
                        x, sy, sz = 0
                        x, y, sz = 2
                        sx, y, sz = 3
                        */

                        // generate vertices
                        uint32 baseVertex = m_outputVertices.GetSize();

                        // subtract amount
                        bool isTopSlab = true;
                        if (isVolumeBlock)
                        {
                            // is the block above this the same type?
                            if (z == (m_height - 1))
                                isTopSlab = (GetNeighbourBlockValueAt(NEIGHBOUR_VOLUME_TOP, x, y, 0) != blockValue);
                            else
                                isTopSlab = (GetBlockValueAt(x, y, z + 1) != blockValue);
                        }

                        // calculate top slab subtract amount
                        float3 topBlockSubtractAmount((isTopSlab) ? float3(0.0f, 0.0f, blockScale * (1.0f - pBlockType->SlabShapeSettings.Height)) : float3::Zero);

                        // generate quads
                        switch (faceIndex)
                        {
                        case CUBE_FACE_RIGHT:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1) - topBlockSubtractAmount, float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 1, blockData, faceColor), CUBE_FACE_RIGHT));// top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[1][2] + 1) - topBlockSubtractAmount, float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 3, blockData, faceColor), CUBE_FACE_RIGHT));           // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 5, blockData, faceColor), CUBE_FACE_RIGHT));           // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[0][2]), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 7, blockData, faceColor), CUBE_FACE_RIGHT));                      // bottom-right
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_RIGHT, baseVertex + 0, baseVertex + 1, baseVertex + 2));
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_RIGHT, baseVertex + 1, baseVertex + 3, baseVertex + 2));
                            }
                            break;

                        case CUBE_FACE_LEFT:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1) - topBlockSubtractAmount, float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 0, blockData, faceColor), CUBE_FACE_LEFT));           // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[1][2] + 1) - topBlockSubtractAmount, float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 2, blockData, faceColor), CUBE_FACE_LEFT));                      // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 4, blockData, faceColor), CUBE_FACE_LEFT));                      // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 6, blockData, faceColor), CUBE_FACE_LEFT));                                 // bottom-right
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_LEFT, baseVertex + 0, baseVertex + 2, baseVertex + 1));
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_LEFT, baseVertex + 1, baseVertex + 2, baseVertex + 3));
                            }
                            break;

                        case CUBE_FACE_BACK:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1) - topBlockSubtractAmount, float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 0, blockData, faceColor), CUBE_FACE_BACK));           // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1) - topBlockSubtractAmount, float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 1, blockData, faceColor), CUBE_FACE_BACK)); // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 4, blockData, faceColor), CUBE_FACE_BACK));                      // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 5, blockData, faceColor), CUBE_FACE_BACK));            // bottom-right
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_BACK, baseVertex + 0, baseVertex + 1, baseVertex + 2));
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_BACK, baseVertex + 1, baseVertex + 3, baseVertex + 2));
                            }
                            break;

                        case CUBE_FACE_FRONT:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[1][2] + 1) - topBlockSubtractAmount, float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 2, blockData, faceColor), CUBE_FACE_FRONT));                     // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[1][2] + 1) - topBlockSubtractAmount, float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 3, blockData, faceColor), CUBE_FACE_FRONT));           // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 6, blockData, faceColor), CUBE_FACE_FRONT));                                // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[0][2]), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 7, blockData, faceColor), CUBE_FACE_FRONT));                      // bottom-right
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_FRONT, baseVertex + 0, baseVertex + 2, baseVertex + 1));
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_FRONT, baseVertex + 1, baseVertex + 2, baseVertex + 3));
                            }
                            break;

                        case CUBE_FACE_TOP:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1) - topBlockSubtractAmount, float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 0, blockData, faceColor), CUBE_FACE_TOP));            // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1) - topBlockSubtractAmount, float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 1, blockData, faceColor), CUBE_FACE_TOP));  // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[1][2] + 1) - topBlockSubtractAmount, float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 2, blockData, faceColor), CUBE_FACE_TOP));                       // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[1][2] + 1) - topBlockSubtractAmount, float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 3, blockData, faceColor), CUBE_FACE_TOP));             // bottom-right
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_TOP, baseVertex + 0, baseVertex + 2, baseVertex + 1));
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_TOP, baseVertex + 1, baseVertex + 2, baseVertex + 3));
                            }
                            break;

                        case CUBE_FACE_BOTTOM:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 4, blockData, faceColor), CUBE_FACE_BOTTOM));                    // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 5, blockData, faceColor), CUBE_FACE_BOTTOM));          // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 6, blockData, faceColor), CUBE_FACE_BOTTOM));                               // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[0][2]), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), faceAtlasUVRange, CalculateCubeVertexColor(pBlockType, 7, blockData, faceColor), CUBE_FACE_BOTTOM));                     // bottom-right
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_BOTTOM, baseVertex + 0, baseVertex + 1, baseVertex + 2));
                                m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_BOTTOM, baseVertex + 1, baseVertex + 3, baseVertex + 2));
                            }
                            break;
                        }

                        // update bounds
                        outMinBounds = outMinBounds.Min(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]));
                        outMaxBounds = outMaxBounds.Max(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1) - topBlockSubtractAmount);
                    }
                }
                else if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE)
                {
                    const BlockPalette::BlockType::PlaneShape &planeSettings = pBlockType->PlaneShapeSettings;
                    const BlockPalette::BlockType::VisualParameters &visualSettings = pBlockType->PlaneShapeSettings.Visual;
                    float3 faceMinUV = visualSettings.MinUV;
                    float3 faceMaxUV = visualSettings.MaxUV;
                    uint32 faceMaterialIndex = visualSettings.MaterialIndex;

                    // set msb in material index if the block type does not allow shadows
                    if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CAST_SHADOWS)
                        faceMaterialIndex |= MATERIAL_INDEX_SHADOW_BIT_SET;

                    // generate base vertices
                    Vertex baseVertices[8];
                    baseVertices[0].Set(float3(-0.5f, 0.0f, 1.0f), float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), visualSettings.AtlasUVRange, visualSettings.Color, CUBE_FACE_FRONT);      // top-left
                    baseVertices[1].Set(float3(0.5f, 0.0f, 1.0f), float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), visualSettings.AtlasUVRange, visualSettings.Color, CUBE_FACE_FRONT);      // top-right
                    baseVertices[2].Set(float3(-0.5f, 0.0f, 0.0f), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), visualSettings.AtlasUVRange, visualSettings.Color, CUBE_FACE_FRONT);      // bottom-left
                    baseVertices[3].Set(float3(0.5f, 0.0f, 0.0f), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), visualSettings.AtlasUVRange, visualSettings.Color, CUBE_FACE_FRONT);      // bottom-right
                    baseVertices[4].Set(float3(-0.5f, 0.0f, 1.0f), float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), visualSettings.AtlasUVRange, visualSettings.Color, CUBE_FACE_BACK);       // top-left
                    baseVertices[5].Set(float3(0.5f, 0.0f, 1.0f), float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), visualSettings.AtlasUVRange, visualSettings.Color, CUBE_FACE_BACK);       // top-right
                    baseVertices[6].Set(float3(-0.5f, 0.0f, 0.0f), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), visualSettings.AtlasUVRange, visualSettings.Color, CUBE_FACE_BACK);       // bottom-left
                    baseVertices[7].Set(float3(0.5f, 0.0f, 0.0f), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), visualSettings.AtlasUVRange, visualSettings.Color, CUBE_FACE_BACK);       // bottom-right
                    
                    // do plane
                    float currentRotation = pBlockType->PlaneShapeSettings.BaseRotation;
                    for (uint32 i = 0; i < pBlockType->PlaneShapeSettings.RepeatCount; i++, currentRotation += pBlockType->PlaneShapeSettings.RepeatRotation)
                    {
                        // copy base
                        Vertex planeVertices[8];
                        Y_memcpy(planeVertices, baseVertices, sizeof(planeVertices));

                        // create quat for rotation
                        Quaternion planeRotation(Quaternion::FromEulerAngles(0.0f, 0.0f, currentRotation));

                        // rotate the vertices, and add the offsets
                        for (uint32 j = 0; j < countof(planeVertices); j++)
                        {
                            planeVertices[j].Position = planeRotation * planeVertices[j].Position;
                            planeVertices[j].Position.x *= planeSettings.Width;
                            planeVertices[j].Position.z *= planeSettings.Height;
                            planeVertices[j].Position.x += planeSettings.OffsetX;
                            planeVertices[j].Position.y += planeSettings.OffsetY;

                            // move so that the origin is the middle of the block (translation happens after rotation)
                            planeVertices[j].Position.x += 0.5f;
                            planeVertices[j].Position.y += 0.5f;
                        }

                        // move into world space
                        for (uint32 j = 0; j < countof(planeVertices); j++)
                            planeVertices[j].Position = (planeVertices[j].Position * m_scale) + BLOCK_WORLD_COORDINATES(x, y, z);

                        // add to list
                        uint32 baseVertex = m_outputVertices.GetSize();
                        for (uint32 j = 0; j < countof(planeVertices); j++)
                            m_outputVertices.Add(planeVertices[j]);

                        // generate 4 triangles, one for each quad
                        m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_FRONT, baseVertex + 2, baseVertex + 3, baseVertex + 0));
                        m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_FRONT, baseVertex + 0, baseVertex + 3, baseVertex + 1));
                        m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_BACK, baseVertex + 6, baseVertex + 4, baseVertex + 7));
                        m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_BACK, baseVertex + 7, baseVertex + 4, baseVertex + 5));
                    }
                }
            }
        }
    }

    /*for (uint32 i = 0; i < m_outputTriangles.GetSize(); i++)
    {
        const Triangle &t = m_outputTriangles[i];
        float3 ot, ob, on;
        MeshUtilites::CalculateTangentSpaceVectors(m_outputVertices[t.Indices[0]].Position, m_outputVertices[t.Indices[1]].Position, m_outputVertices[t.Indices[2]].Position,
                                                   m_outputVertices[t.Indices[0]].TexCoord.xy(), m_outputVertices[t.Indices[1]].TexCoord.xy(), m_outputVertices[t.Indices[2]].TexCoord.xy(),
                                                   ot, ob, on);
        __asm int 3;
    }*/


    delete[] pBlockDataArray;
    m_outputVertexFactoryFlags = BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXCOORD | BLOCK_MESH_VERTEX_FACTORY_FLAG_VERTEX_COLORS | BLOCK_MESH_VERTEX_FACTORY_FLAG_FACE_INDEX;
}

void BlockMeshBuilder::GenerateCollisionBlocks(float3 &outMinBounds, float3 &outMaxBounds)
{
    static const uint32 ALL_FACES_ALLOCATED_MASK = 0x3F;

    const float blockScale = m_scale;
    const uint32 yStride = m_width;
    const uint32 zStride = yStride * m_length;

    const BlockPalette::BlockType *pBlockType;
    BlockVolumeBlockType blockValue, searchBlockValue;
    uint32 blockData, searchBlockData;
    uint32 x, y, z;

    SIMDVector3f basePosition(m_translation);
    uint32 *pBlockDataArray = new uint32[m_width * m_length * m_height];
    uint32 meshSizes[3] = { m_width, m_length, m_height };
    bool isEdgeZBlockPos, isEdgeZBlockNeg;
    bool isEdgeYBlockPos, isEdgeYBlockNeg;
    bool isEdgeXBlockPos, isEdgeXBlockNeg;

    // strip out faces that have blocks adjacent to them.
    for (z = 0; z < m_height; z++)
    {
        isEdgeZBlockNeg = (z == 0);
        isEdgeZBlockPos = (z == (m_height - 1));

        for (y = 0; y < m_length; y++)
        {
            isEdgeYBlockNeg = (y == 0);
            isEdgeYBlockPos = (y == (m_length - 1));

            for (x = 0; x < m_width; x++)
            {
                isEdgeXBlockNeg = (x == 0);
                isEdgeXBlockPos = (x == (m_width - 1));

                // get block type
                blockValue = BLOCK_VALUE_ARRAY_ACCESS(x, y, z);
                if (blockValue == 0)
                {
                    BLOCK_DATA_ARRAY_ACCESS(x, y, z) = 0;
                    continue;
                }

                pBlockType = m_pPalette->GetBlockType(blockValue);
                if (!(pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_COLLIDABLE))
                {
                    BLOCK_DATA_ARRAY_ACCESS(x, y, z) = 0;
                    continue;
                }

                if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
                {
                    // get block data
                    blockData = ALL_FACES_ALLOCATED_MASK;

                    // Culling values
                    if (!isEdgeXBlockPos && HasCubeVisibilityBlockingBlockAt(pBlockType, x + 1, y + 0, z + 0)) { blockData &= ~(1 << CUBE_FACE_RIGHT); }
                    if (!isEdgeXBlockNeg && HasCubeVisibilityBlockingBlockAt(pBlockType, x - 1, y + 0, z + 0)) { blockData &= ~(1 << CUBE_FACE_LEFT); }
                    if (!isEdgeYBlockPos && HasCubeVisibilityBlockingBlockAt(pBlockType, x + 0, y + 1, z + 0)) { blockData &= ~(1 << CUBE_FACE_BACK); }
                    if (!isEdgeYBlockNeg && HasCubeVisibilityBlockingBlockAt(pBlockType, x + 0, y - 1, z + 0)) { blockData &= ~(1 << CUBE_FACE_FRONT); }
                    if (!isEdgeZBlockPos && HasCubeVisibilityBlockingBlockAt(pBlockType, x + 0, y + 0, z + 1)) { blockData &= ~(1 << CUBE_FACE_TOP); }                    
                    if (!isEdgeZBlockNeg && HasCubeVisibilityBlockingBlockAt(pBlockType, x + 0, y + 0, z - 1)) { blockData &= ~(1 << CUBE_FACE_BOTTOM); }

                    // store face mask
                    BLOCK_DATA_ARRAY_ACCESS(x, y, z) = blockData;
                }
                else if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE)
                {
                    // planes get set to 0 when generated
                    BLOCK_DATA_ARRAY_ACCESS(x, y, z) = 1;
                }
                else
                {
                    // mark all faces as generated, so we don't come back to them
                    BLOCK_DATA_ARRAY_ACCESS(x, y, z) = 0;
                }
            }
        }
    }

    for (z = 0; z < m_height; z++)
    {
        for (y = 0; y < m_length; y++)
        {
            for (x = 0; x < m_width; x++)
            {
                // get block type
                blockValue = BLOCK_VALUE_ARRAY_ACCESS(x, y, z);
                if (blockValue == 0 || (pBlockType = m_pPalette->GetBlockType(blockValue))->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_NONE)
                    continue;

                // get block data
                blockData = BLOCK_DATA_ARRAY_ACCESS(x, y, z);

                // switch shape type
                if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
                {
                    // generate each face
                    for (uint32 faceIndex = 0; faceIndex < CUBE_FACE_COUNT; faceIndex++)
                    {
                        uint32 currentFaceMask = (1 << faceIndex);
                        if ((blockData & currentFaceMask) == 0)
                            continue;

                        // determine which axis to sweep
                        uint32 sliceAxis = 0, sweepAxis0 = 0, sweepAxis1 = 0;
                        switch (faceIndex)
                        {
                        case CUBE_FACE_RIGHT:
                        case CUBE_FACE_LEFT:
                            sliceAxis = 0;
                            sweepAxis0 = 1;
                            sweepAxis1 = 2;
                            break;

                        case CUBE_FACE_BACK:
                        case CUBE_FACE_FRONT:
                            sliceAxis = 1;
                            sweepAxis0 = 0;
                            sweepAxis1 = 2;
                            break;

                        case CUBE_FACE_TOP:
                        case CUBE_FACE_BOTTOM:
                            sliceAxis = 2;
                            sweepAxis0 = 1;
                            sweepAxis1 = 0;
                            break;
                        }

                        // determine the quad boundaries by sweeping each axis, then in reverse
                        uint32 baseCoordinates[3];
                        uint32 blockCoordinates[3];
                        uint32 quadBoundaries[2][3];
                        uint32 i, j, k;

                        // base
                        baseCoordinates[0] = x;
                        baseCoordinates[1] = y;
                        baseCoordinates[2] = z;
                        Y_memcpy(&quadBoundaries[0], baseCoordinates, sizeof(quadBoundaries[0]));
                        Y_memcpy(&quadBoundaries[1], baseCoordinates, sizeof(quadBoundaries[1]));

                        // set slice axis
                        blockCoordinates[sliceAxis] = baseCoordinates[sliceAxis];

                        // forward...
                        {
                            // axis 0
                            for (i = quadBoundaries[0][sweepAxis0] + 1; i < meshSizes[sweepAxis0]; i++)
                            {
                                blockCoordinates[sweepAxis0] = i;
                                blockCoordinates[sweepAxis1] = baseCoordinates[sweepAxis1];
                                searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                searchBlockData = BLOCK_DATA_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                if ((searchBlockValue == blockValue) &&
                                    (searchBlockData & currentFaceMask))
                                {
                                    quadBoundaries[1][sweepAxis0]++;
                                    continue;
                                }
                                else
                                {
                                    break;
                                }
                            }

                            // axis 1
                            for (i = quadBoundaries[0][sweepAxis1] + 1; i < meshSizes[sweepAxis1]; i++)
                            {
                                blockCoordinates[sweepAxis1] = i;

                                // has to match everything on axis 0
                                bool merge = true;
                                for (j = quadBoundaries[0][sweepAxis0]; j <= quadBoundaries[1][sweepAxis0]; j++)
                                {
                                    blockCoordinates[sweepAxis0] = j;
                                    searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                    searchBlockData = BLOCK_DATA_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                    if ((searchBlockValue == blockValue) &&
                                        (searchBlockData & currentFaceMask))
                                    {
                                        continue;
                                    }
                                    else
                                    {
                                        merge = false;
                                        break;
                                    }
                                }

                                // ok?
                                if (merge)
                                    quadBoundaries[1][sweepAxis1]++;
                                else
                                    break;
                            }
                        }


                        // mark all the faces as generated    
                        for (i = quadBoundaries[0][2]; i <= quadBoundaries[1][2]; i++)
                        {
                            for (j = quadBoundaries[0][1]; j <= quadBoundaries[1][1]; j++)
                            {
                                for (k = quadBoundaries[0][0]; k <= quadBoundaries[1][0]; k++)
                                {
                                    // mark as allocated
                                    BLOCK_DATA_ARRAY_ACCESS(k, j, i) &= ~currentFaceMask;
                                }
                            }
                        }

                        // update the cached data too
                        blockData &= ~currentFaceMask;

                        /*
                        0 top back left
                        1 top back right
                        2 top front left
                        3 top front right
                        4 bottom back left
                        5 bottom back right
                        6 bottom front left
                        7 bottom front right

                        x, y, z = 6
                        sx, y, z = 7
                        sx, sy, z = 5
                        sx, sy, sz = 1
                        x, sy, z = 4
                        x, sy, sz = 0
                        x, y, sz = 2
                        sx, y, sz = 3
                        */

                        // generate vertices
                        uint32 baseVertex = m_outputVertices.GetSize();

                        // generate quads
                        switch (faceIndex)
                        {
                        case CUBE_FACE_RIGHT:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_RIGHT));// top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_RIGHT));           // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_RIGHT));           // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_RIGHT));                      // bottom-right
                                m_outputTriangles.Add(Triangle(0, CUBE_FACE_RIGHT, baseVertex + 0, baseVertex + 1, baseVertex + 2));
                                m_outputTriangles.Add(Triangle(0, CUBE_FACE_RIGHT, baseVertex + 1, baseVertex + 3, baseVertex + 2));
                            }
                            break;

                        case CUBE_FACE_LEFT:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_LEFT));           // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_LEFT));                      // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_LEFT));                      // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_LEFT));                                 // bottom-right
                                m_outputTriangles.Add(Triangle(0, CUBE_FACE_LEFT, baseVertex + 0, baseVertex + 2, baseVertex + 1));
                                m_outputTriangles.Add(Triangle(0, CUBE_FACE_LEFT, baseVertex + 1, baseVertex + 2, baseVertex + 3));
                            }
                            break;

                        case CUBE_FACE_BACK:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_BACK));           // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_BACK)); // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_BACK));                      // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_BACK));            // bottom-right
                                m_outputTriangles.Add(Triangle(0, CUBE_FACE_BACK, baseVertex + 0, baseVertex + 1, baseVertex + 2));
                                m_outputTriangles.Add(Triangle(0, CUBE_FACE_BACK, baseVertex + 1, baseVertex + 3, baseVertex + 2));
                            }
                            break;

                        case CUBE_FACE_FRONT:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_FRONT));                     // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_FRONT));           // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_FRONT));                                // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_FRONT));                      // bottom-right
                                m_outputTriangles.Add(Triangle(0, CUBE_FACE_FRONT, baseVertex + 0, baseVertex + 2, baseVertex + 1));
                                m_outputTriangles.Add(Triangle(0, CUBE_FACE_FRONT, baseVertex + 1, baseVertex + 2, baseVertex + 3));
                            }
                            break;

                        case CUBE_FACE_TOP:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_TOP));            // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_TOP));  // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_TOP));                       // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_TOP));             // bottom-right
                                m_outputTriangles.Add(Triangle(0, CUBE_FACE_TOP, baseVertex + 0, baseVertex + 2, baseVertex + 1));
                                m_outputTriangles.Add(Triangle(0, CUBE_FACE_TOP, baseVertex + 1, baseVertex + 2, baseVertex + 3));
                            }
                            break;

                        case CUBE_FACE_BOTTOM:
                            {
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_BOTTOM));                    // top-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_BOTTOM));          // top-right
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_BOTTOM));                               // bottom-left
                                m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_BOTTOM));                     // bottom-right
                                m_outputTriangles.Add(Triangle(0, CUBE_FACE_BOTTOM, baseVertex + 0, baseVertex + 1, baseVertex + 2));
                                m_outputTriangles.Add(Triangle(0, CUBE_FACE_BOTTOM, baseVertex + 1, baseVertex + 3, baseVertex + 2));
                            }
                            break;
                        }

                        // update bounds
                        outMinBounds = outMinBounds.Min(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]));
                        outMaxBounds = outMaxBounds.Max(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1));
                    }
                }
                else if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE)
                {
                    // do plane
                }
            }
        }
    }

    delete[] pBlockDataArray;
}

#undef BLOCK_WORLD_COORDINATES
#undef BLOCK_DATA_ARRAY_ACCESS
#undef BLOCK_VALUE_ARRAY_ACCESS

#define BLOCK_VALUE_ARRAY_ACCESS(x, y, z) m_pBlockData[(z) * zStride + (y) * yStride + (x)]
#define PENDING_FACES_ARRAY_ACCESS(x, y, z) pPendingFacesArray[(z) * zStride + (y) * yStride + (x)]
#define BLOCK_WORLD_COORDINATES(x, y, z) (Vector3((float)x, (float)y, (float)z) * blockScale + basePosition)

void BlockMeshBuilder::GenerateSilhouetteBlocks(float3 &outMinBounds, float3 &outMaxBounds)
{
    /*
    static const byte ALL_FACES_ALLOCATED_MASK = 0x3F;

    const uint32 chunkSize = m_chunkSize;
    const float blockScale = m_blockScale;
    const uint32 yStride = chunkSize;
    const uint32 zStride = yStride * chunkSize;

    const BlockMeshBlockValue *pChunkBlockValues = m_pChunk->GetBlockValues();
    const BlockPalette::BlockType *pBlockType;
    const BlockPalette::BlockType *pSearchBlockType;
    BlockMeshBlockValue blockValue;
    BlockMeshBlockValue searchBlockValue;
    byte blockFaceMask;
    byte currentFaceMask;
    uint32 x, y, z;
    uint32 sx, sy, sz;

    Vector3 basePosition(Vector3((float)m_chunkX, (float)m_chunkY, (float)m_chunkZ) * m_blockScale * (float)m_chunkSize);
    byte *pPendingFacesArray = new byte[chunkSize * chunkSize * chunkSize];
    byte pendingFaceMask;

    // strip out faces that have blocks adjacent to them.
    for (z = 0; z < chunkSize; z++)
    {
        for (y = 0; y < chunkSize; y++)
        {
            for (x = 0; x < chunkSize; x++)
            {
                // get block type
                blockValue = BLOCK_VALUE_ARRAY_ACCESS(x, y, z);
                pBlockType = m_pPalette->GetBlockType(blockValue);

                if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
                {
                    // get block data
                    pendingFaceMask = ALL_FACES_ALLOCATED_MASK;

                    // Culling values
                    if (HasVisibilityBlockingBlockAt(pBlockType, x + 0, y + 0, z + 1)) { pendingFaceMask &= ~(1 << CUBE_FACE_TOP); }
                    if (HasVisibilityBlockingBlockAt(pBlockType, x + 0, y + 1, z + 0)) { pendingFaceMask &= ~(1 << CUBE_FACE_BACK); }
                    if (HasVisibilityBlockingBlockAt(pBlockType, x - 1, y + 0, z + 0)) { pendingFaceMask &= ~(1 << CUBE_FACE_LEFT); }
                    if (HasVisibilityBlockingBlockAt(pBlockType, x + 1, y + 0, z + 0)) { pendingFaceMask &= ~(1 << CUBE_FACE_RIGHT); }
                    if (HasVisibilityBlockingBlockAt(pBlockType, x + 0, y - 1, z + 0)) { pendingFaceMask &= ~(1 << CUBE_FACE_FRONT); }
                    if (HasVisibilityBlockingBlockAt(pBlockType, x + 0, y + 0, z - 1)) { pendingFaceMask &= ~(1 << CUBE_FACE_BOTTOM); }

                    // mask out each direcion
                    if (pendingFaceMask & (1 << CUBE_FACE_RIGHT) && x != (chunkSize - 1))
                    {
                        // look for any other blocks in this direction
                        sx = x + 1;
                        for (;;)
                        {
                            if (sx == chunkSize)
                                break;

                            searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(sx, y, z);
                            if (searchBlockValue == 0 ||
                                !((pSearchBlockType = m_pPalette->GetBlockType(searchBlockValue))->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE) ||
                                (pSearchBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_TRANSPARENT))
                            {
                                break;
                            }

                            sx++;
                        }

                        // found any?
                        if (sx != chunkSize)
                            pendingFaceMask &= ~(1 << CUBE_FACE_RIGHT);
                    }
                    if (pendingFaceMask & (1 << CUBE_FACE_LEFT) && x != 0)
                    {
                        // look for any other blocks in this direction
                        sx = x - 1;
                        for (;;)
                        {
                            if (sx == 0)
                                break;

                            searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(sx, y, z);
                            if (searchBlockValue == 0 ||
                                !((pSearchBlockType = m_pPalette->GetBlockType(searchBlockValue))->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE) ||
                                (pSearchBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_TRANSPARENT))
                            {
                                break;
                            }

                            sx--;
                        }

                        // found any?
                        if (sx != 0)
                            pendingFaceMask &= ~(1 << CUBE_FACE_LEFT);
                    }
                    if (pendingFaceMask & (1 << CUBE_FACE_BACK) && y != (chunkSize - 1))
                    {
                        // look for any other blocks in this direction
                        sy = y + 1;
                        for (;;)
                        {
                            if (sy == chunkSize)
                                break;

                            searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(x, sy, z);
                            if (searchBlockValue == 0 ||
                                !((pSearchBlockType = m_pPalette->GetBlockType(searchBlockValue))->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE) ||
                                (pSearchBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_TRANSPARENT))
                            {
                                break;
                            }

                            sy++;
                        }

                        // found any?
                        if (sy != chunkSize)
                            pendingFaceMask &= ~(1 << CUBE_FACE_BACK);
                    }
                    if (pendingFaceMask & (1 << CUBE_FACE_FRONT) && y != 0)
                    {
                        // look for any other blocks in this direction
                        sy = y - 1;
                        for (;;)
                        {
                            if (sy == 0)
                                break;

                            searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(x, sy, z);
                            if (searchBlockValue == 0 ||
                                !((pSearchBlockType = m_pPalette->GetBlockType(searchBlockValue))->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE) ||
                                (pSearchBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_TRANSPARENT))
                            {
                                break;
                            }

                            sy--;
                        }

                        // found any?
                        if (sy != 0)
                            pendingFaceMask &= ~(1 << CUBE_FACE_FRONT);
                    }
                    if (pendingFaceMask & (1 << CUBE_FACE_TOP) && z != (chunkSize - 1))
                    {
                        // look for any other blocks in this direction
                        sz = z + 1;
                        for (;;)
                        {
                            if (sz == chunkSize)
                                break;

                            searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(x, y, sz);
                            if (searchBlockValue == 0 ||
                                !((pSearchBlockType = m_pPalette->GetBlockType(searchBlockValue))->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE) ||
                                (pSearchBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_TRANSPARENT))
                            {
                                break;
                            }

                            sz++;
                        }

                        // found any?
                        if (sz != chunkSize)
                            pendingFaceMask &= ~(1 << CUBE_FACE_TOP);
                    }
                    if (pendingFaceMask & (1 << CUBE_FACE_BOTTOM) && z != 0)
                    {
                        // look for any other blocks in this direction
                        sz = z - 1;
                        for (;;)
                        {
                            if (sz == 0)
                                break;

                            searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(x, y, sz);
                            if (searchBlockValue == 0 ||
                                !((pSearchBlockType = m_pPalette->GetBlockType(searchBlockValue))->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE) ||
                                (pSearchBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_TRANSPARENT))
                            {
                                break;
                            }

                            sz--;
                        }

                        // found any?
                        if (sz != 0)
                            pendingFaceMask &= ~(1 << CUBE_FACE_BOTTOM);
                    }

                    if (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_TRANSPARENT)
                        pendingFaceMask = 0;

                    // store face mask
                    PENDING_FACES_ARRAY_ACCESS(x, y, z) = pendingFaceMask;
                }
                else
                {
                    // mark all faces as generated, so we don't come back to them
                    PENDING_FACES_ARRAY_ACCESS(x, y, z) = 0;
                }
            }
        }
    }

    for (z = 0; z < chunkSize; z++)
    {
        for (y = 0; y < chunkSize; y++)
        {
            for (x = 0; x < chunkSize; x++)
            {
                // get block type
                blockValue = BLOCK_VALUE_ARRAY_ACCESS(x, y, z);
                if (blockValue == 0 || (pBlockType = m_pPalette->GetBlockType(blockValue))->ShapeType != BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
                    continue;

                // get block data
                blockFaceMask = PENDING_FACES_ARRAY_ACCESS(x, y, z);

                // generate each face
                for (uint32 faceIndex = 0; faceIndex < CUBE_FACE_COUNT; faceIndex++)
                {
                    currentFaceMask = (1 << faceIndex);
                    if ((blockFaceMask & currentFaceMask) == 0)
                        continue;

                    // determine which axis to sweep
                    uint32 sliceAxis = 0, sweepAxis0 = 0, sweepAxis1 = 0;
                    switch (faceIndex)
                    {
                    case CUBE_FACE_RIGHT:
                    case CUBE_FACE_LEFT:
                        sliceAxis = 0;
                        sweepAxis0 = 1;
                        sweepAxis1 = 2;
                        break;

                    case CUBE_FACE_BACK:
                    case CUBE_FACE_FRONT:
                        sliceAxis = 1;
                        sweepAxis0 = 0;
                        sweepAxis1 = 2;
                        break;

                    case CUBE_FACE_TOP:
                    case CUBE_FACE_BOTTOM:
                        sliceAxis = 2;
                        sweepAxis0 = 1;
                        sweepAxis1 = 0;
                        break;
                    }

                    // determine the quad boundaries by sweeping each axis, then in reverse
                    uint32 baseCoordinates[3];
                    uint32 blockCoordinates[3];
                    uint32 quadBoundaries[2][3];
                    uint32 i, j, k;

                    // base
                    baseCoordinates[0] = x;
                    baseCoordinates[1] = y;
                    baseCoordinates[2] = z;
                    Y_memcpy(&quadBoundaries[0], baseCoordinates, sizeof(quadBoundaries[0]));
                    Y_memcpy(&quadBoundaries[1], baseCoordinates, sizeof(quadBoundaries[1]));

                    // set slice axis
                    blockCoordinates[sliceAxis] = baseCoordinates[sliceAxis];

                    // forward...
                    {
                        // axis 0
                        for (i = quadBoundaries[0][sweepAxis0] + 1; i < chunkSize; i++)
                        {
                            blockCoordinates[sweepAxis0] = i;
                            blockCoordinates[sweepAxis1] = baseCoordinates[sweepAxis1];
                            if ((BLOCK_VALUE_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]) == blockValue) &&
                                (PENDING_FACES_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]) & blockFaceMask))
                            {
                                quadBoundaries[1][sweepAxis0]++;
                                continue;
                            }
                            else
                            {
                                break;
                            }
                        }

                        // axis 1
                        for (i = quadBoundaries[0][sweepAxis1] + 1; i < chunkSize; i++)
                        {
                            blockCoordinates[sweepAxis1] = i;

                            // has to match everything on axis 0
                            bool merge = true;
                            for (j = quadBoundaries[0][sweepAxis0]; j <= quadBoundaries[1][sweepAxis0]; j++)
                            {
                                blockCoordinates[sweepAxis0] = j;
                                if ((BLOCK_VALUE_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]) == blockValue) &&
                                    (PENDING_FACES_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]) & blockFaceMask))
                                {
                                    continue;
                                }
                                else
                                {
                                    merge = false;
                                    break;
                                }
                            }

                            // ok?
                            if (merge)
                                quadBoundaries[1][sweepAxis1]++;
                            else
                                break;
                        }
                    }


                    // mark all the faces as generated    
                    for (i = quadBoundaries[0][2]; i <= quadBoundaries[1][2]; i++)
                    {
                        for (j = quadBoundaries[0][1]; j <= quadBoundaries[1][1]; j++)
                        {
                            for (k = quadBoundaries[0][0]; k <= quadBoundaries[1][0]; k++)
                            {
                                // mark as allocated
                                PENDING_FACES_ARRAY_ACCESS(k, j, i) &= ~currentFaceMask;
                            }
                        }
                    }
                    
                    // generate vertices
                    const BlockPalette::BlockType::CubeShapeFace *pFaceDef = &pBlockType->CubeShapeFaces[faceIndex];
                    const uint32 faceMaterialIndex = pFaceDef->SilhouetteMaterialIndex;
                    uint32 baseVertex = m_outputVertices.GetSize();

                    // generate quads
                    Vertex vertices[4];
                    Triangle triangles[2];
                    switch (faceIndex)
                    {
                    case CUBE_FACE_RIGHT:
                        {
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_RIGHT));// top-left
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_RIGHT));           // top-right
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_RIGHT));           // bottom-left
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_RIGHT));                      // bottom-right
                            m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_RIGHT, baseVertex + 0, baseVertex + 1, baseVertex + 2));
                            m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_RIGHT, baseVertex + 1, baseVertex + 3, baseVertex + 2));
                        }
                        break;

                    case CUBE_FACE_LEFT:
                        {
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_LEFT));           // top-left
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_LEFT));                      // top-right
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_LEFT));                      // bottom-left
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_LEFT));                                 // bottom-right
                            m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_LEFT, baseVertex + 0, baseVertex + 2, baseVertex + 1));
                            m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_LEFT, baseVertex + 1, baseVertex + 2, baseVertex + 3));
                        }
                        break;

                    case CUBE_FACE_BACK:
                        {
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_BACK));           // top-left
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_BACK)); // top-right
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_BACK));                      // bottom-left
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_BACK));            // bottom-right
                            m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_BACK, baseVertex + 0, baseVertex + 1, baseVertex + 2));
                            m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_BACK, baseVertex + 1, baseVertex + 3, baseVertex + 2));
                        }
                        break;

                    case CUBE_FACE_FRONT:
                        {
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_FRONT));                     // top-left
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_FRONT));           // top-right
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_FRONT));                                // bottom-left
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_FRONT));                      // bottom-right
                            m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_FRONT, baseVertex + 0, baseVertex + 2, baseVertex + 1));
                            m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_FRONT, baseVertex + 1, baseVertex + 2, baseVertex + 3));
                        }
                        break;

                    case CUBE_FACE_TOP:
                        {
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_TOP));            // top-left
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_TOP));  // top-right
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_TOP));                       // bottom-left
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[1][2] + 1), float3::Zero, float4::Zero, 0, CUBE_FACE_TOP));             // bottom-right
                            m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_TOP, baseVertex + 0, baseVertex + 2, baseVertex + 1));
                            m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_TOP, baseVertex + 1, baseVertex + 2, baseVertex + 3));
                        }
                        break;

                    case CUBE_FACE_BOTTOM:
                        {
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_BOTTOM));                    // top-left
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_BOTTOM));          // top-right
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_BOTTOM));                               // bottom-left
                            m_outputVertices.Add(Vertex(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[0][1], quadBoundaries[0][2]), float3::Zero, float4::Zero, 0, CUBE_FACE_BOTTOM));                     // bottom-right
                            m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_BOTTOM, baseVertex + 0, baseVertex + 1, baseVertex + 2));
                            m_outputTriangles.Add(Triangle(faceMaterialIndex, CUBE_FACE_BOTTOM, baseVertex + 1, baseVertex + 3, baseVertex + 2));
                        }
                        break;
                    }

                    // update bounds
                    outMinBounds = outMinBounds.Min(BLOCK_WORLD_COORDINATES(quadBoundaries[0][0], quadBoundaries[0][1], quadBoundaries[0][2]));
                    outMaxBounds = outMaxBounds.Max(BLOCK_WORLD_COORDINATES(quadBoundaries[1][0] + 1, quadBoundaries[1][1] + 1, quadBoundaries[1][2] + 1));
                }
            }
        }
    }

    delete[] pPendingFacesArray;
    */
}

#undef BLOCK_WORLD_COORDINATES
#undef PENDING_FACES_ARRAY_ACCESS
#undef BLOCK_DATA_ARRAY_ACCESS

void BlockMeshBuilder::GenerateMesh()
{
    // min/max bounds
    float3 minBounds(float3::Infinite);
    float3 maxBounds(float3::NegativeInfinite);

    // generate cube faces
    GenerateBlocks(minBounds, maxBounds);

    // fill bounds
    if (minBounds <= maxBounds)
    {
        m_outputBoundingBox.SetBounds(minBounds, maxBounds);
        m_outputBoundingSphere = Sphere::FromAABox(m_outputBoundingBox);
    }

    // re-order triangles
    OptimizeTriangleOrder();

    // generate batches
    GenerateBatches();
}

void BlockMeshBuilder::GenerateCollisionMesh()
{
    // min/max bounds
    float3 minBounds(float3::Infinite);
    float3 maxBounds(float3::NegativeInfinite);

    // generate cube faces
    GenerateCollisionBlocks(minBounds, maxBounds);

    // fill bounds
    if (minBounds <= maxBounds)
    {
        m_outputBoundingBox.SetBounds(minBounds, maxBounds);
        m_outputBoundingSphere = Sphere::FromAABox(m_outputBoundingBox);
    }
}

void BlockMeshBuilder::GenerateSilhouetteMesh()
{
    // min/max bounds
    float3 minBounds(float3::Infinite);
    float3 maxBounds(float3::NegativeInfinite);

    // generate cube faces
    GenerateSilhouetteBlocks(minBounds, maxBounds);

    // fill bounds
    if (minBounds <= maxBounds)
    {
        m_outputBoundingBox.SetBounds(minBounds, maxBounds);
        m_outputBoundingSphere = Sphere::FromAABox(m_outputBoundingBox);
    }

    // re-order triangles
    OptimizeTriangleOrder();

    // generate batches
    GenerateBatches();
}

static int TriangleOrderSortFunction(const BlockMeshBuilder::Triangle *pLeft, const BlockMeshBuilder::Triangle *pRight)
{
    // sort by material index first
    if (pLeft->MaterialIndex > pRight->MaterialIndex)
        return 1;
    else if (pLeft->MaterialIndex < pRight->MaterialIndex)
        return -1;

    // then by triangle indices for better use of the cache
    if (pLeft->Indices[0] > pRight->Indices[0])
        return 1;
    else if (pLeft->Indices[0] < pRight->Indices[0])
        return -1;

    if (pLeft->Indices[1] > pRight->Indices[1])
        return 1;
    else if (pLeft->Indices[1] < pRight->Indices[1])
        return -1;

    if (pLeft->Indices[2] > pRight->Indices[2])
        return 1;
    else if (pLeft->Indices[2] < pRight->Indices[2])
        return -1;

    return 0;
}

void BlockMeshBuilder::OptimizeTriangleOrder()
{
    if (m_outputTriangles.GetSize() > 0)
    {
        /*
        MeshUtilites::OptimizeIndicesForBatching(reinterpret_cast<byte *>(m_outputTriangles.GetBasePointer()) + offsetof(Triangle, Indices[0]), sizeof(Triangle),
                                                 reinterpret_cast<byte *>(m_outputTriangles.GetBasePointer()) + offsetof(Triangle, MaterialIndex), sizeof(Triangle),
                                                 m_outputTriangles.GetSize() * 3);

        */

        // use sort method instead
        m_outputTriangles.Sort(TriangleOrderSortFunction);
    }
}

void BlockMeshBuilder::GenerateBatches()
{
    uint32 i;
    DebugAssert(m_outputBatches.GetSize() == 0);
    if (m_outputTriangles.GetSize() == 0)
        return;

    const Triangle *pTriangle = m_outputTriangles.GetBasePointer();

    Batch batch;
    batch.StartIndex = 0;
    batch.NumIndices = 0;
    batch.MaterialIndex = pTriangle->MaterialIndex;

    for (i = 0; i < m_outputTriangles.GetSize(); i++, pTriangle++)
    {
        if (pTriangle->MaterialIndex == batch.MaterialIndex)
        {
            batch.NumIndices += 3;
        }
        else
        {
            DebugAssert(batch.NumIndices > 0);
            batch.DrawShadows = (batch.MaterialIndex & MATERIAL_INDEX_SHADOW_BIT_MASK) != 0;
            batch.MaterialIndex &= MATERIAL_INDEX_MASK;
            m_outputBatches.Add(batch);

            batch.StartIndex = i * 3;
            batch.NumIndices = 3;
            batch.MaterialIndex = pTriangle->MaterialIndex;
        }
    }

    if (batch.NumIndices > 0)
    {
        batch.DrawShadows = (batch.MaterialIndex & MATERIAL_INDEX_SHADOW_BIT_MASK) != 0;
        batch.MaterialIndex &= MATERIAL_INDEX_MASK;
        m_outputBatches.Add(batch);
    }
}

bool BlockMeshBuilder::CreateGPUBuffers(VertexBufferBindingArray *pVertexBuffers, uint32 *pVertexFactoryFlags, GPUBuffer **ppIndexBuffer, GPU_INDEX_FORMAT *pIndexFormat, uint32 inVertexFactoryFlags)
{
    // determine vertex factory flags
    uint32 vertexFactoryFlags = m_outputVertexFactoryFlags | inVertexFactoryFlags;
    const RendererCapabilities &rendererCapabilities = g_pRenderer->GetCapabilities();
    if (vertexFactoryFlags & BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXCOORD)
    {
        if (rendererCapabilities.SupportsTextureArrays)
            vertexFactoryFlags |= BLOCK_MESH_VERTEX_FACTORY_FLAG_TEXTURE_ARRAY;
        else
            vertexFactoryFlags |= BLOCK_MESH_VERTEX_FACTORY_FLAG_ATLAS_TEXCOORDS;
    }

    VertexBufferBindingArray vertexBuffers;
    if (!BlockMeshVertexFactory::CreateVerticesBuffer(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), vertexFactoryFlags, m_outputVertices.GetBasePointer(), m_outputVertices.GetSize(), &vertexBuffers))
        return false;

    // generate indices
    uint32 i, n;
    GPUBuffer *pIndexBuffer;
    GPU_INDEX_FORMAT indexFormat;
    if (m_outputVertices.GetSize() <= 0xFFFF)
    {
        uint16 *pIndices = new uint16[m_outputVertices.GetSize() * 3];
        for (i = 0, n = 0; i < m_outputTriangles.GetSize(); i++)
        {
            const Triangle &t = m_outputTriangles[i];
            pIndices[n++] = (uint16)t.Indices[0];
            pIndices[n++] = (uint16)t.Indices[1];
            pIndices[n++] = (uint16)t.Indices[2];
        }

        GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_INDEX_BUFFER, sizeof(uint16)* m_outputTriangles.GetSize() * 3);
        pIndexBuffer = g_pRenderer->CreateBuffer(&bufferDesc, pIndices);
        indexFormat = GPU_INDEX_FORMAT_UINT16;
        delete[] pIndices;
    }
    else
    {
        uint32 *pIndices = new uint32[m_outputTriangles.GetSize() * 3];
        for (i = 0, n = 0; i < m_outputTriangles.GetSize(); i++)
        {
            const Triangle &t = m_outputTriangles[i];
            pIndices[n++] = t.Indices[0];
            pIndices[n++] = t.Indices[1];
            pIndices[n++] = t.Indices[2];
        }

        GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_INDEX_BUFFER, sizeof(uint32)* m_outputTriangles.GetSize() * 3);
        pIndexBuffer = g_pRenderer->CreateBuffer(&bufferDesc, pIndices);
        indexFormat = GPU_INDEX_FORMAT_UINT32;
        delete[] pIndices;
    }

    if (pIndexBuffer == NULL)
        return false;

    BlockMeshVertexFactory::ShareVerticesBuffer(pVertexBuffers, &vertexBuffers);
    *ppIndexBuffer = pIndexBuffer;
    *pIndexFormat = indexFormat;
    *pVertexFactoryFlags = vertexFactoryFlags;
    return true;
}

