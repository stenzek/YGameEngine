#include "BlockEngine/PrecompiledHeader.h"
#include "BlockEngine/BlockWorldMesher.h"
#include "BlockEngine/BlockWorldVertexFactory.h"
#include "Renderer/Renderer.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Core/MeshUtilties.h"
//Log_SetChannel(BlockMeshBuilder);

static const float CUBE_FACE_TANGENTS[CUBE_FACE_COUNT][3] =
{
    { 0, -1, 0 },       // RIGHT
    { 0, -1, 0 },       // LEFT
    { 1, 0, 0 },        // BACK
    { 1, 0, 0 },        // FRONT
    { 1, 0, 0 },        // TOP
    { 1, 0, 0 },        // BOTTOM
};

static const float CUBE_FACE_BINORMALS[CUBE_FACE_COUNT][3] =
{
    { 0, 0, -1 },       // RIGHT
    { 0, 0, -1 },       // LEFT
    { 0, 0, -1 },       // BACK
    { 0, 0, -1 },       // FRONT
    { 0, -1, 0 },       // TOP
    { 0, -1, 0 },       // BOTTOM
};

static const float CUBE_FACE_NORMALS[CUBE_FACE_COUNT][3] =
{
    { 1, 0, 0 },        // RIGHT
    { -1, 0, 0 },       // LEFT
    { 0, 1, 0 },        // BACK
    { 0, -1, 0 },       // FRONT
    { 0, 0, 1 },        // TOP
    { 0, 0, -1 },       // BOTTOM
};

static const uint32 CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_COUNT][2] =
{
    // TANGENT      NORMAL
    { 0x7F008100, 0x0000007F },     // RIGHT
    { 0x81008100, 0x00000081 },     // LEFT
    { 0x7F00007F, 0x00007F00 },     // BACK
    { 0x8100007F, 0x00008100 },     // FRONT
    { 0x8100007F, 0x007F0000 },     // TOP
    { 0x7F00007F, 0x00810000 }      // BOTTOM
};

// rotate the cube faces according to block rotation
// this doesn't work for uvs.. maybe we need some uv transform matrix?
static const CUBE_FACE CUBE_FACE_ROTATION_LUT[NUM_BLOCK_WORLD_BLOCK_ROTATIONS][CUBE_FACE_COUNT] = 
{
    //  Right               Left                Back                Front               Top                 Bottom
    {   CUBE_FACE_RIGHT,    CUBE_FACE_LEFT,     CUBE_FACE_BACK,     CUBE_FACE_FRONT,    CUBE_FACE_TOP,      CUBE_FACE_BOTTOM    },  // North
    {   CUBE_FACE_FRONT,    CUBE_FACE_BACK,     CUBE_FACE_RIGHT,    CUBE_FACE_LEFT,     CUBE_FACE_TOP,      CUBE_FACE_BOTTOM    },  // East
    {   CUBE_FACE_LEFT,     CUBE_FACE_RIGHT,    CUBE_FACE_FRONT,    CUBE_FACE_BACK,     CUBE_FACE_TOP,      CUBE_FACE_BOTTOM    },  // South
    {   CUBE_FACE_BACK,     CUBE_FACE_FRONT,    CUBE_FACE_LEFT,     CUBE_FACE_RIGHT,    CUBE_FACE_TOP,      CUBE_FACE_BOTTOM    },  // West
};

static const float BLOCK_ROTATION_MATRIX_LUT[NUM_BLOCK_WORLD_BLOCK_ROTATIONS][4][4] =
{
    { { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },                                     // North
    //{ { 1.19249e-008f, 1.0f, 0.0f, 0.0f }, { -1.0f, 1.19249e-008f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 0.0f } },                                              // East
    //{ { -1.0f, 8.74228e-008f, 0.0f, 1.0f }, { -8.74228e-008f, -1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 0.0f } },                                            // South
    //{ { -4.37114e-008f, -1.0f, 0.0f, 1.0f }, { 1.0f, -4.37114e-008f, 0.0f, 2.98023e-008f }, { 0.0f, 0.0f, 1.0f, 0.0f } }                                    // West
    { { 0.000000f, 1.000000f, 0.000000f, 0.000000f }, { -1.000000f, 0.000000f, 0.000000f, 1.000000f }, { 0.000000f, 0.000000f, 1.000000f, 0.000000f } },    // East
    { { -1.000000f, 0.000000f, 0.000000f, 1.000000f }, { -0.000000f, -1.000000f, 0.000000f, 1.000000f }, { 0.000000f, 0.000000f, 1.000000f, 0.000000f } },  // South
    { { -0.000000f, -1.000000f, 0.000000f, 1.000000f }, { 1.000000f, -0.000000f, 0.000000f, 0.000000f }, { 0.000000f, 0.000000f, 1.000000f, 0.000000f } }

};

BlockWorldMesher::BlockWorldMesher(const BlockPalette *pPalette, uint32 chunkSize, uint32 lodLevel, const float3 &basePosition, bool generateLightMaps)
    : m_pPalette(pPalette),
      m_chunkSize(chunkSize),
      m_lodLevel(lodLevel),
      m_basePosition(basePosition),
      m_generateLightMaps(generateLightMaps)
{
    m_pBlockValues = new BlockWorldBlockType[chunkSize * chunkSize * chunkSize];
    m_pBlockData = new BlockWorldBlockDataType[chunkSize * chunkSize * chunkSize];
    m_pBlockFaceMasks = new uint8[chunkSize * chunkSize * chunkSize];
    Y_memzero(m_pBlockFaceMasks, sizeof(uint8) * chunkSize * chunkSize * chunkSize);

    m_output.BoundingBox = AABox::Zero;
    m_output.BoundingSphere = Sphere::Zero;
}

BlockWorldMesher::~BlockWorldMesher()
{
    for (MeshInstances *pMeshInstances : m_output.Instances)
        delete pMeshInstances;

    delete[] m_pBlockFaceMasks;
    delete[] m_pBlockData;
    delete[] m_pBlockValues;
}

const BlockWorldBlockType BlockWorldMesher::GetBlockValueAt(uint32 x, uint32 y, uint32 z) const
{
    DebugAssert(x < m_chunkSize && y < m_chunkSize && z < m_chunkSize);
    return m_pBlockValues[(z * m_chunkSize * m_chunkSize) + (y * m_chunkSize) + x];
}

const bool BlockWorldMesher::IsVisibleBlockAt(uint32 x, uint32 y, uint32 z) const
{
    BlockWorldBlockType blockTypeId = GetBlockValueAt(x, y, z);
    const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType(blockTypeId);
    return (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE) != 0;
}

const bool BlockWorldMesher::IsVisibleNonTransparentBlockAt(uint32 x, uint32 y, uint32 z) const
{
    BlockWorldBlockType blockTypeId = GetBlockValueAt(x, y, z);
    const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType(blockTypeId);
    return (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE && !(pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE));
}

const bool BlockWorldMesher::HasCubeLightBlockingBlockAt(uint32 x, uint32 y, uint32 z) const
{
    BlockWorldBlockType blockValue = GetBlockValueAt(x, y, z);
    if (blockValue == 0)
        return false;

    const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType(blockValue);
    if (pBlockType->ShapeType != BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
        return false;

    return (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_VISIBLE) != 0;
}

const bool BlockWorldMesher::CalculateBlockFaceVisibility(uint32 x, uint32 y, uint32 z, BlockWorldBlockType blockValue, CUBE_FACE face) const
{
    BlockWorldBlockType neighbourBlockValue;
    switch (face)
    {
    case CUBE_FACE_RIGHT:   neighbourBlockValue = GetBlockValueAt(x + 1, y, z);     break;
    case CUBE_FACE_LEFT:    neighbourBlockValue = GetBlockValueAt(x - 1, y, z);     break;
    case CUBE_FACE_BACK:    neighbourBlockValue = GetBlockValueAt(x, y + 1, z);     break;
    case CUBE_FACE_FRONT:   neighbourBlockValue = GetBlockValueAt(x, y - 1, z);     break;
    case CUBE_FACE_TOP:     neighbourBlockValue = GetBlockValueAt(x, y, z + 1);     break;
    case CUBE_FACE_BOTTOM:  neighbourBlockValue = GetBlockValueAt(x, y, z - 1);     break;
    default:                neighbourBlockValue = 0;                                break;
    }

    // fastpath for air
    if (neighbourBlockValue == 0)
        return true;

    // coloured blocks are always blocking
    if (neighbourBlockValue & BLOCK_WORLD_BLOCK_VALUE_COLORED_FLAG_BIT)
        return false;

    // lookup block type
    const BlockPalette::BlockType *pNeighbourBlockType = m_pPalette->GetBlockType(neighbourBlockValue);
    switch (pNeighbourBlockType->ShapeType)
    {
    case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE:
        {
            // check for volume blocks here
            if ((pNeighbourBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_VOLUME) && neighbourBlockValue == blockValue)
                return false;

            // transparent blocks can't block visibility, not entirely anyway
            return !(pNeighbourBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCKS_VISIBILITY);
        }
        break;

    case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB:
        {
            // check for volume blocks here
            if ((pNeighbourBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_VOLUME) && neighbourBlockValue == blockValue)
                return false;

            // slabs only block on the bottom face (our top face)
            if (face == CUBE_FACE_TOP)
                return !(pNeighbourBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCKS_VISIBILITY);

            // all other cases they don't block
            return true;       
        }
        break;

    case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS:
        {
            // stairs only block the bottom and back faces which is our top and front faces
            if (face == CUBE_FACE_TOP || face == CUBE_FACE_FRONT)
                return !(pNeighbourBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_BLOCKS_VISIBILITY) ;

            // everything else has to be generated
            return true;
        }
        break;

    case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_MESH:
        {
            // meshes have an unknown shape, therefore they cannot block
            return true;
        }
        break;

    default:
        {
            // everything else.. well there shouldn't be anything else
            return true;
        }
        break;
    }
}

const uint32 BlockWorldMesher::CalculateCubeVertexColor(const BlockPalette::BlockType *pBlockType, CUBE_FACE faceIndex, const uint32 blockValue, const uint32 blockLighting)
{
#if 0
    Vector4 outVertexColor;

    // are we using a typed block?
    uint32 faceColor;
    if (pBlockType != nullptr)
        faceColor = pBlockType->CubeShapeFaces[faceIndex].Visual.Color;
    else
        faceColor = MAKE_COLOR_R8G8B8A8_UNORM(((blockValue >> 10) & 0x1F) * 8, ((blockValue >> 5) & 0x1F) * 8, ((blockValue)& 0x1F) * 8, (blockValue & 0x8000) ? 255 : 0);  // 1555 to 8888, fixme

    // start by taking the face colour
    outVertexColor = Vector4(float(faceColor & 0xFF), float((faceColor >> 8) & 0xFF), float((faceColor >> 16) & 0xFF), float((faceColor >> 24) & 0xFF)) / 255.0f;

    // factor in block lighting
    outVertexColor *= Vector4(Vector3((float)blockLighting / 16.0f) /** 0.5f + 0.5f*/, 1.0f);
    
    // return vertex colour
    {
        uint8 r, g, b, a;
        r = (uint8)Math::Clamp(outVertexColor.r * 255.0f, 0.0f, 255.0f);
        g = (uint8)Math::Clamp(outVertexColor.g * 255.0f, 0.0f, 255.0f);
        b = (uint8)Math::Clamp(outVertexColor.b * 255.0f, 0.0f, 255.0f);
        a = (uint8)Math::Clamp(outVertexColor.a * 255.0f, 0.0f, 255.0f);
        return MAKE_COLOR_R8G8B8A8_UNORM(r, g, b, a);
    }
#else
    // are we using a typed block?
    uint32 faceColor;
    if (pBlockType != nullptr)
        faceColor = pBlockType->CubeShapeFaces[faceIndex].Visual.Color;
    else
        faceColor = MAKE_COLOR_R8G8B8A8_UNORM(((blockValue >> 10) & 0x1F) * 8, ((blockValue >> 5) & 0x1F) * 8, ((blockValue)& 0x1F) * 8, 0);  // 1555 to 8888, fixme

    return (faceColor & 0x00FFFFFF) | ((blockLighting * 10) << 24);
#endif
}

const uint32 BlockWorldMesher::CalculatePlaneVertexColor(const BlockPalette::BlockType *pBlockType, const uint32 blockValue, const uint32 blockLighting)
{
    // are we using a typed block?
    uint32 faceColor;
    if (pBlockType != nullptr)
        faceColor = pBlockType->PlaneShapeSettings.Visual.Color;
    else
        faceColor = MAKE_COLOR_R8G8B8A8_UNORM(((blockValue >> 10) & 0x1F) * 8, ((blockValue >> 5) & 0x1F) * 8, ((blockValue)& 0x1F) * 8, 0);  // 1555 to 8888, fixme

    return (faceColor & 0x00FFFFFF) | ((blockLighting * 10) << 24);
}

#define BLOCK_VALUE_ARRAY_ACCESS(x, y, z) m_pBlockValues[(z) * zStride + (y) * yStride + (x)]
#define BLOCK_DATA_ARRAY_ACCESS(x, y, z) m_pBlockData[(z) * zStride + (y) * yStride + (x)]
#define BLOCK_FACEMASK_ARRAY_ACCESS(x, y, z) m_pBlockFaceMasks[(z) * zStride + (y) * yStride + (x)]
#define BLOCK_WORLD_COORDINATES(x, y, z) (Vector3(static_cast<float>((x - 1) << m_lodLevel), static_cast<float>((y - 1) << m_lodLevel), static_cast<float>((z - 1) << m_lodLevel)) + basePosition)

const uint32 BlockWorldMesher::CalculateCubeFaceLighting(uint32 x, uint32 y, uint32 z, CUBE_FACE faceIndex)
{
    if (!m_generateLightMaps)
        return 15;

    const uint32 yStride = m_chunkSize;
    const uint32 zStride = yStride * m_chunkSize;

    uint8 lightLevel = 0;
    switch (faceIndex)
    {
    case CUBE_FACE_LEFT:    lightLevel = BLOCK_DATA_ARRAY_ACCESS(x - 1, y, z);  break;
    case CUBE_FACE_RIGHT:   lightLevel = BLOCK_DATA_ARRAY_ACCESS(x + 1, y, z);  break;
    case CUBE_FACE_FRONT:   lightLevel = BLOCK_DATA_ARRAY_ACCESS(x, y - 1, z);  break;
    case CUBE_FACE_BACK:    lightLevel = BLOCK_DATA_ARRAY_ACCESS(x, y + 1, z);  break;
    case CUBE_FACE_BOTTOM:  lightLevel = BLOCK_DATA_ARRAY_ACCESS(x, y, z - 1);  break;
    case CUBE_FACE_TOP:     lightLevel = BLOCK_DATA_ARRAY_ACCESS(x, y, z + 1);  break;
    }

    return (uint32)BLOCK_WORLD_BLOCK_DATA_GET_LIGHTING(lightLevel);
}

void BlockWorldMesher::GenerateBlocks(uint3 &minBlockCoordinates, uint3 &maxBlockCoordinates)
{
    const uint32 yStride = m_chunkSize;
    const uint32 zStride = yStride * m_chunkSize;
    const uint32 volumeSizeMinusOne = m_chunkSize - 1;

    // fill in face masks
    for (uint32 z = 1; z < volumeSizeMinusOne; z++)
    {
        for (uint32 y = 1; y < volumeSizeMinusOne; y++)
        {
            for (uint32 x = 1; x < volumeSizeMinusOne; x++)
            {
                // get block type, test for air blocks
                BlockWorldBlockType blockValue = BLOCK_VALUE_ARRAY_ACCESS(x, y, z);
                if (blockValue == 0)
                    continue;

                // lookup rotation
                //uint8 blockRotation = BLOCK_WORLD_BLOCK_DATA_GET_ROTATION(BLOCK_DATA_ARRAY_ACCESS(x, y, z));
                //DebugAssert(blockRotation < NUM_BLOCK_WORLD_BLOCK_ROTATIONS);

                // cull each direction :: todo only need this for cuboid meshes
                uint8 faceMask = 0;
                if (CalculateBlockFaceVisibility(x, y, z, blockValue, CUBE_FACE_LEFT))
                    faceMask |= (1 << CUBE_FACE_LEFT);
                if (CalculateBlockFaceVisibility(x, y, z, blockValue, CUBE_FACE_RIGHT))
                    faceMask |= (1 << CUBE_FACE_RIGHT);
                if (CalculateBlockFaceVisibility(x, y, z, blockValue, CUBE_FACE_FRONT))
                    faceMask |= (1 << CUBE_FACE_FRONT);
                if (CalculateBlockFaceVisibility(x, y, z, blockValue, CUBE_FACE_BACK))
                    faceMask |= (1 << CUBE_FACE_BACK);
                if (CalculateBlockFaceVisibility(x, y, z, blockValue, CUBE_FACE_BOTTOM))
                    faceMask |= (1 << CUBE_FACE_BOTTOM);
                if (CalculateBlockFaceVisibility(x, y, z, blockValue, CUBE_FACE_TOP))
                    faceMask |= (1 << CUBE_FACE_TOP);
//                 // cull each direction :: todo only need this for cuboid meshes
//                 uint8 faceMask = 0;
//                 if (CalculateBlockFaceVisibility(x, y, z, blockValue, CUBE_FACE_ROTATION_LUT[blockRotation][CUBE_FACE_LEFT]))
//                     faceMask |= (1 << CUBE_FACE_LEFT);
//                 if (CalculateBlockFaceVisibility(x, y, z, blockValue, CUBE_FACE_ROTATION_LUT[blockRotation][CUBE_FACE_RIGHT]))
//                     faceMask |= (1 << CUBE_FACE_RIGHT);
//                 if (CalculateBlockFaceVisibility(x, y, z, blockValue, CUBE_FACE_ROTATION_LUT[blockRotation][CUBE_FACE_FRONT]))
//                     faceMask |= (1 << CUBE_FACE_FRONT);
//                 if (CalculateBlockFaceVisibility(x, y, z, blockValue, CUBE_FACE_ROTATION_LUT[blockRotation][CUBE_FACE_BACK]))
//                     faceMask |= (1 << CUBE_FACE_BACK);
//                 if (CalculateBlockFaceVisibility(x, y, z, blockValue, CUBE_FACE_ROTATION_LUT[blockRotation][CUBE_FACE_BOTTOM]))
//                     faceMask |= (1 << CUBE_FACE_BOTTOM);
//                 if (CalculateBlockFaceVisibility(x, y, z, blockValue, CUBE_FACE_ROTATION_LUT[blockRotation][CUBE_FACE_TOP]))
//                     faceMask |= (1 << CUBE_FACE_TOP);


                // handle slabs -- always generate their top face
                if ((blockValue & BLOCK_WORLD_BLOCK_VALUE_COLORED_FLAG_BIT) == 0)
                {
                    const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType(blockValue);
                    if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB && (faceMask & (1 << CUBE_FACE_TOP)) == 0)
                    {
                        if (GetBlockValueAt(x, y, z + 1) != blockValue)
                            faceMask |= (1 << CUBE_FACE_TOP);
                    }

                    // handle stairs - their top faces must always be generated unless they're completely hidden
                    else if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS && faceMask != 0)
                        faceMask |= (1 << CUBE_FACE_TOP);
                }

                // set face mask
                BLOCK_FACEMASK_ARRAY_ACCESS(x, y, z) = faceMask;
            }
        }
    }

    // build mesh
    for (uint32 z = 1; z < volumeSizeMinusOne; z++)
    {
        for (uint32 y = 1; y < volumeSizeMinusOne; y++)
        {
            for (uint32 x = 1; x < volumeSizeMinusOne; x++)
            {
                // get block type, test for air blocks
                BlockWorldBlockType blockValue = BLOCK_VALUE_ARRAY_ACCESS(x, y, z);
                if (blockValue == 0)
                    continue;

                // read data
                BlockWorldBlockDataType blockData = BLOCK_DATA_ARRAY_ACCESS(x, y, z);
                uint8 blockFaceMask = BLOCK_FACEMASK_ARRAY_ACCESS(x, y, z);
                uint8 blockRotation = BLOCK_WORLD_BLOCK_DATA_GET_ROTATION(blockData);
                DebugAssert(blockRotation < 4);
               
                // get the block type, a null blocktype indicates a coloured block
                const BlockPalette::BlockType *pBlockType;
                BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE shapeType;
                if ((blockValue & BLOCK_WORLD_BLOCK_VALUE_COLORED_FLAG_BIT) == 0)
                {
                    // typed block
                    pBlockType = m_pPalette->GetBlockType(blockValue);
                    shapeType = pBlockType->ShapeType;
                }
                else
                {
                    // coloured block
                    pBlockType = nullptr;
                    shapeType = BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE;
                }

                // cube-typed blocks can tile
                if (shapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE || shapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB)// || shapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS)
                {
                    // determine if we can tile in the z direction.            
                    bool canTileZ = false;
                    if (shapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
                    {
                        // all cubes can tile vertically
                        canTileZ = true;
                    }
                    else if (shapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB)
                    {
                        // slabs have two behaviours, depending on if volume bit is set
                        // if volume bit is set, they are generated like cubes, except the very top block
                        // otherwise, they are generated independantly      
                        if (pBlockType->Flags & pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_VOLUME)
                            canTileZ = true;
                    }

                    // generate each face
                    for (uint32 faceIndex = 0; faceIndex < CUBE_FACE_COUNT; faceIndex++)
                    {
                        uint8 currentFaceMask = (1 << faceIndex);
                        if ((blockFaceMask & currentFaceMask) == 0)
                            continue;

                        // calculate light colour for face
                        uint32 blockLighting = CalculateCubeFaceLighting(x, y, z, (CUBE_FACE)faceIndex);

                        // block rotation swaps the faces here!
                        // match on data instead of just lighting will fix tiling too

                        // determine which axis to sweep
                        int32 sliceAxis = 0, sweepAxis0 = 0, sweepAxis1 = 0;
                        switch (faceIndex)
                        {
                        case CUBE_FACE_RIGHT:
                        case CUBE_FACE_LEFT:
                            sliceAxis = 0;
                            sweepAxis0 = 1;
                            sweepAxis1 = (canTileZ) ? 2 : -1;
                            break;

                        case CUBE_FACE_BACK:
                        case CUBE_FACE_FRONT:
                            sliceAxis = 1;
                            sweepAxis0 = 0;
                            sweepAxis1 = (canTileZ) ? 2 : -1;
                            break;

                        case CUBE_FACE_TOP:
                        case CUBE_FACE_BOTTOM:
                            sliceAxis = 2;
                            sweepAxis0 = 1;
                            sweepAxis1 = 0;
                            break;
                        }

                        // hack for stairs since their rotation is dependant on which ways can tile @todo
                        if (shapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS)
                            sweepAxis0 = sweepAxis1 = -1;

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
                        if (sweepAxis0 >= 0 && sweepAxis1 >= 0)
                        {
                            {
                                // axis 0
                                for (i = quadBoundaries[0][sweepAxis0] + 1; i < volumeSizeMinusOne; i++)
                                {
                                    blockCoordinates[sweepAxis0] = i;
                                    blockCoordinates[sweepAxis1] = baseCoordinates[sweepAxis1];
                                    uint32 searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                    uint8 searchBlockRotation = BLOCK_WORLD_BLOCK_DATA_GET_ROTATION(BLOCK_DATA_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]));
                                    uint8 searchFaceMask = BLOCK_FACEMASK_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                    if (searchBlockValue == blockValue && (searchFaceMask & currentFaceMask) && searchBlockRotation == blockRotation &&
                                        CalculateCubeFaceLighting(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2], (CUBE_FACE)faceIndex) == blockLighting)
                                    {
                                        quadBoundaries[1][sweepAxis0]++;
                                        continue;
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                            }

                            // axis 1
                            {
                                for (i = quadBoundaries[0][sweepAxis1] + 1; i < volumeSizeMinusOne; i++)
                                {
                                    blockCoordinates[sweepAxis1] = i;

                                    // has to match everything on axis 0
                                    bool merge = true;
                                    for (j = quadBoundaries[0][sweepAxis0]; j <= quadBoundaries[1][sweepAxis0]; j++)
                                    {
                                        blockCoordinates[sweepAxis0] = j;
                                        uint32 searchBlockValue = BLOCK_VALUE_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                        uint8 searchBlockRotation = BLOCK_WORLD_BLOCK_DATA_GET_ROTATION(BLOCK_DATA_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]));
                                        uint8 searchFaceMask = BLOCK_FACEMASK_ARRAY_ACCESS(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2]);
                                        if (searchBlockValue == blockValue && (searchFaceMask & currentFaceMask) && searchBlockRotation == blockRotation &&
                                            CalculateCubeFaceLighting(blockCoordinates[0], blockCoordinates[1], blockCoordinates[2], (CUBE_FACE)faceIndex) == blockLighting)
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
                                    BLOCK_FACEMASK_ARRAY_ACCESS(k, j, i) &= ~currentFaceMask;
                                }
                            }
                        }

                        // update the cached data too
                        blockFaceMask &= ~currentFaceMask;

                        // generate meshes
                        switch (shapeType)
                        {
                        case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE:
                            AddCubeBlockFace(pBlockType, blockValue, blockLighting, blockRotation, m_basePosition, m_lodLevel, quadBoundaries[0][0] - 1, quadBoundaries[0][1] - 1, quadBoundaries[0][2] - 1, quadBoundaries[1][0] - 1, quadBoundaries[1][1] - 1, quadBoundaries[1][2] - 1, (CUBE_FACE)faceIndex, m_output);
                            break;

                        case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB:
                            AddSlabBlockFace(pBlockType, blockValue, blockLighting, blockRotation, m_basePosition, m_lodLevel, quadBoundaries[0][0] - 1, quadBoundaries[0][1] - 1, quadBoundaries[0][2] - 1, quadBoundaries[1][0] - 1, quadBoundaries[1][1] - 1, quadBoundaries[1][2] - 1, (CUBE_FACE)faceIndex, (GetBlockValueAt(quadBoundaries[1][0], quadBoundaries[1][1], quadBoundaries[1][2] + 1) != blockValue), m_output);
                            break;

                        case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS:
                            AddStairBlockFace(pBlockType, blockValue, blockLighting, blockRotation, m_basePosition, m_lodLevel, quadBoundaries[0][0] - 1, quadBoundaries[0][1] - 1, quadBoundaries[0][2] - 1, quadBoundaries[1][0] - 1, quadBoundaries[1][1] - 1, quadBoundaries[1][2] - 1, (CUBE_FACE)faceIndex, m_output);
                            break;
                        }
                    }
                }
                else if (shapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS)
                {
                    // stair's don't tile at the moment, so just treat them separately for now
                    for (uint32 faceIndex = 0; faceIndex < CUBE_FACE_COUNT; faceIndex++)
                    {
                        //uint8 currentFaceMask = (1 << CUBE_FACE_ROTATION_LUT[blockRotation][faceIndex]);
                        //if ((blockFaceMask & currentFaceMask) == 0)
                            //continue;

                        // calculate light colour for face
                        uint32 blockLighting = CalculateCubeFaceLighting(x, y, z, (CUBE_FACE)faceIndex);

                        // update the cached data too
                        //blockFaceMask &= ~currentFaceMask;
                        AddStairBlockFace(pBlockType, blockValue, blockLighting, blockRotation, m_basePosition, m_lodLevel, x - 1, y - 1, z - 1, x - 1, y - 1, z - 1, (CUBE_FACE)faceIndex, m_output);
                    }
                }
                else if (shapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE ||
                         shapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_MESH)
                {
                    // cull other shapes that are completely enclosed
                    if (blockFaceMask != 0)
                    {
                        // mark as generated
                        blockFaceMask = 0;
                        BLOCK_FACEMASK_ARRAY_ACCESS(x, y, z) = 0;

                        // calculate light colour for face
                        uint32 blockLighting = CalculateCubeFaceLighting(x, y, z, CUBE_FACE_RIGHT);

                        // generate it
                        switch (shapeType)
                        {
                        case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE:
                            AddPlaneBlock(pBlockType, blockValue, blockLighting, blockRotation, m_basePosition, m_lodLevel, x - 1, y - 1, z - 1, m_output);
                            break;

                        case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_MESH:
                            AddMeshBlock(pBlockType, blockValue, blockLighting, blockRotation, m_basePosition, m_lodLevel, x - 1, y - 1, z - 1, m_output);
                            break;
                        }
                    }
                }

                // handle point light emitting blocks, currently only at lod0
                if (pBlockType != nullptr && (pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_POINT_LIGHT_EMITTER) && m_lodLevel == 0)
                    AddLightBlock(pBlockType, m_basePosition, m_lodLevel, x - 1, y - 1, z - 1, m_output);

                // update block bounds
                minBlockCoordinates = minBlockCoordinates.Min(uint3(x - 1, y - 1, z - 1));
                maxBlockCoordinates = maxBlockCoordinates.Max(uint3(x, y, z));
            }
        }
    }
}

#undef BLOCK_VALUE_ARRAY_ACCESS

void BlockWorldMesher::GenerateMesh()
{
    // min/max bounds
    uint3 minBlockCoordinates(Y_UINT32_MAX, Y_UINT32_MAX, Y_UINT32_MAX);
    uint3 maxBlockCoordinates(0, 0, 0);

    // generate cube faces
    GenerateBlocks(minBlockCoordinates, maxBlockCoordinates);

    // fill bounds
    if (minBlockCoordinates <= maxBlockCoordinates)
    {
        float3 minBounds(float3((float)(minBlockCoordinates.x << m_lodLevel), (float)(minBlockCoordinates.y << m_lodLevel), (float)(minBlockCoordinates.z << m_lodLevel)) + m_basePosition);
        float3 maxBounds(float3((float)(maxBlockCoordinates.x << m_lodLevel), (float)(maxBlockCoordinates.y << m_lodLevel), (float)(maxBlockCoordinates.z << m_lodLevel)) + m_basePosition);
        m_output.BoundingBox.SetBounds(minBounds, maxBounds);
        m_output.BoundingSphere = Sphere::FromAABox(m_output.BoundingBox);
    }

    // re-order triangles
    OptimizeTriangleOrder(m_output);

    // generate batches
    GenerateBatches(m_output);
}

static int TriangleOrderSortFunction(const BlockWorldMesher::Triangle *pLeft, const BlockWorldMesher::Triangle *pRight)
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

void BlockWorldMesher::OptimizeTriangleOrder(Output &output)
{
    if (output.Triangles.GetSize() > 0)
    {
        /*
        MeshUtilites::OptimizeIndicesForBatching(reinterpret_cast<byte *>(m_outputTriangles.GetBasePointer()) + offsetof(Triangle, Indices[0]), sizeof(Triangle),
                                                 reinterpret_cast<byte *>(m_outputTriangles.GetBasePointer()) + offsetof(Triangle, MaterialIndex), sizeof(Triangle),
                                                 m_outputTriangles.GetSize() * 3);

        */

        // use sort method instead
        output.Triangles.Sort(TriangleOrderSortFunction);
    }
}

void BlockWorldMesher::GenerateBatches(Output &output)
{
    uint32 i;
    DebugAssert(output.Batches.GetSize() == 0);
    if (output.Triangles.GetSize() > 0)
    {
        const Triangle *pTriangle = output.Triangles.GetBasePointer();

        Batch batch;
        batch.StartIndex = 0;
        batch.NumIndices = 0;
        batch.MaterialIndex = pTriangle->MaterialIndex;

        for (i = 0; i < output.Triangles.GetSize(); i++, pTriangle++)
        {
            if (pTriangle->MaterialIndex == batch.MaterialIndex)
            {
                batch.NumIndices += 3;
            }
            else
            {
                DebugAssert(batch.NumIndices > 0);
                output.Batches.Add(batch);

                batch.StartIndex = i * 3;
                batch.NumIndices = 3;
                batch.MaterialIndex = pTriangle->MaterialIndex;
            }
        }

        if (batch.NumIndices > 0)
            output.Batches.Add(batch);
    }
}

bool BlockWorldMesher::CreateGPUBuffers(const Output &output, VertexBufferBindingArray *pVertexBuffers, GPUBuffer **ppIndexBuffer, GPU_INDEX_FORMAT *pIndexFormat, GPUBuffer **ppInstanceTransformBuffer)
{
    if (!output.Vertices.IsEmpty())
    {
        // create buffer directly from data
        GPU_BUFFER_DESC vertexBufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER, output.Vertices.GetStorageSizeInBytes());
        AutoReleasePtr<GPUBuffer> pVertexBuffer = g_pRenderer->CreateBuffer(&vertexBufferDesc, output.Vertices.GetBasePointer());
        if (pVertexBuffer == nullptr)
            return false;

        pVertexBuffers->SetBuffer(0, pVertexBuffer, 0, sizeof(Vertex));
    }

    if (!output.Triangles.IsEmpty())
    {
        // generate indices
        GPUBuffer *pIndexBuffer;
        GPU_INDEX_FORMAT indexFormat;
        if (output.Vertices.GetSize() <= 0xFFFF)
        {
            uint16 *pIndices = new uint16[output.Vertices.GetSize() * 3];
            for (uint32 i = 0, n = 0; i < output.Triangles.GetSize(); i++)
            {
                const Triangle &t = output.Triangles[i];
                pIndices[n++] = (uint16)t.Indices[0];
                pIndices[n++] = (uint16)t.Indices[1];
                pIndices[n++] = (uint16)t.Indices[2];
            }

            GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_INDEX_BUFFER, sizeof(uint16)* output.Triangles.GetSize() * 3);
            pIndexBuffer = g_pRenderer->CreateBuffer(&bufferDesc, pIndices);
            indexFormat = GPU_INDEX_FORMAT_UINT16;
            delete[] pIndices;
        }
        else
        {
            uint32 *pIndices = new uint32[output.Triangles.GetSize() * 3];
            for (uint32 i = 0, n = 0; i < output.Triangles.GetSize(); i++)
            {
                const Triangle &t = output.Triangles[i];
                pIndices[n++] = t.Indices[0];
                pIndices[n++] = t.Indices[1];
                pIndices[n++] = t.Indices[2];
            }

            GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_INDEX_BUFFER, sizeof(uint32)* output.Triangles.GetSize() * 3);
            pIndexBuffer = g_pRenderer->CreateBuffer(&bufferDesc, pIndices);
            indexFormat = GPU_INDEX_FORMAT_UINT32;
            delete[] pIndices;
        }

        if (pIndexBuffer == nullptr)
            return false;

        *ppIndexBuffer = pIndexBuffer;
        *pIndexFormat = indexFormat;
    }

    // add mesh instances
    if (!output.Instances.IsEmpty())
    {
        // create a merged buffer of all instance transforms
        MemArray<float3x4> allTransformsArray;
        for (MeshInstances *pMeshInstances : output.Instances)
        {
            pMeshInstances->BufferOffset = allTransformsArray.GetStorageSizeInBytes();
            allTransformsArray.AddArray(pMeshInstances->Transforms);
        }

        // create on gpu
        GPU_BUFFER_DESC instanceTransformBufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER, allTransformsArray.GetStorageSizeInBytes());
        GPUBuffer *pInstanceTransformBuffer = g_pRenderer->CreateBuffer(&instanceTransformBufferDesc, allTransformsArray.GetBasePointer());
        if (pInstanceTransformBuffer == nullptr)
            return false;

        *ppInstanceTransformBuffer = pInstanceTransformBuffer;
    }
    return true;
}

bool BlockWorldMesher::MeshSingleBlock(const BlockPalette *pPalette, BlockWorldBlockType blockValue, Output &output)
{
    // get the block type, a null blocktype indicates a coloured block
    const BlockPalette::BlockType *pBlockType;
    BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE shapeType;
    if ((blockValue & BLOCK_WORLD_BLOCK_VALUE_COLORED_FLAG_BIT) == 0)
    {
        // typed block
        pBlockType = pPalette->GetBlockType(blockValue);
        shapeType = pBlockType->ShapeType;
    }
    else
    {
        // coloured block
        pBlockType = nullptr;
        shapeType = BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE;
    }

    // generate meshes
    switch (shapeType)
    {
    case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE:
        {
            for (uint32 face = 0; face < CUBE_FACE_COUNT; face++)
                AddCubeBlockFace(pBlockType, blockValue, 0, 0, float3::Zero, 0, 0, 0, 0, 0, 0, 0, (CUBE_FACE)face, output);
        }
        break;

    case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB:
        {
            for (uint32 face = 0; face < CUBE_FACE_COUNT; face++)
                AddSlabBlockFace(pBlockType, blockValue, 0, 0, float3::Zero, 0, 0, 0, 0, 0, 0, 0, (CUBE_FACE)face, false, output);
        }
        break;

    case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_STAIRS:
        {
            for (uint32 face = 0; face < CUBE_FACE_COUNT; face++)
                AddStairBlockFace(pBlockType, blockValue, 0, 0, float3::Zero, 0, 0, 0, 0, 0, 0, 0, (CUBE_FACE)face, output);
        }
        break;

    case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_PLANE:
        AddPlaneBlock(pBlockType, blockValue, 0, 0, float3::Zero, 0, 0, 0, 0, output);
        break;

    case BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_MESH:
        AddMeshBlock(pBlockType, blockValue, 0, 0, float3::Zero, 0, 0, 0, 0, output);
        break;

    default:
        // not a recognized shape type
        return false;
    }

    // generate everything
    OptimizeTriangleOrder(output);
    GenerateBatches(output);
    return true;
}

void BlockWorldMesher::AddCubeBlockFace(const BlockPalette::BlockType *pBlockType, BlockWorldBlockType blockValue, uint32 blockLighting, uint8 blockRotation, const float3 &basePosition, uint32 lodLevel, uint32 startX, uint32 startY, uint32 startZ, uint32 endX, uint32 endY, uint32 endZ, CUBE_FACE face, Output &output)
{
    // find block counts
    uint32 blockCountX = (endX - startX) + 1;
    uint32 blockCountY = (endY - startY) + 1;
    uint32 blockCountZ = (endZ - startZ) + 1;
    
    // get uvs and color
    uint32 materialIndex;
    float3 uvTopLeft;
    float3 uvTopRight;
    float3 uvBottomRight;
    float3 uvBottomLeft;

    // this depends on the face and rotation
    switch (face)
    {
    case CUBE_FACE_RIGHT:
    case CUBE_FACE_LEFT:
    case CUBE_FACE_BACK:
    case CUBE_FACE_FRONT:
        {
            // R/L/B/F can just use the reorientated face's uvs
            const BlockPalette::BlockType::CubeShapeFace *pFaceDef = &pBlockType->CubeShapeFaces[CUBE_FACE_ROTATION_LUT[blockRotation][face]];
            materialIndex = pFaceDef->Visual.MaterialIndex;
            uvTopLeft = pFaceDef->Visual.MinUV;
            uvTopRight.Set(pFaceDef->Visual.MaxUV.x, pFaceDef->Visual.MinUV.y, pFaceDef->Visual.MinUV.z);
            uvBottomRight = pFaceDef->Visual.MaxUV;
            uvBottomLeft.Set(pFaceDef->Visual.MinUV.x, pFaceDef->Visual.MaxUV.y, pFaceDef->Visual.MinUV.z);

            // repeat the texture
            switch (face)
            {
            case CUBE_FACE_RIGHT:
            case CUBE_FACE_LEFT:
                uvTopRight.x *= (float)(blockCountY << lodLevel);
                uvBottomRight.x *= (float)(blockCountY << lodLevel);
                uvBottomRight.y *= (float)(blockCountZ << lodLevel);
                uvBottomLeft.y *= (float)(blockCountZ << lodLevel);
                break;

            case CUBE_FACE_BACK:
            case CUBE_FACE_FRONT:
                uvTopRight.x *= (float)(blockCountX << lodLevel);
                uvBottomRight.x *= (float)(blockCountX << lodLevel);
                uvBottomRight.y *= (float)(blockCountZ << lodLevel);
                uvBottomLeft.y *= (float)(blockCountZ << lodLevel);
                break;
            }
        }
        break;

    case CUBE_FACE_TOP:
    case CUBE_FACE_BOTTOM:
        {
            // T/B have to have their uvs rotated using the same face
            const BlockPalette::BlockType::CubeShapeFace *pFaceDef = &pBlockType->CubeShapeFaces[face];
            materialIndex = pFaceDef->Visual.MaterialIndex;

            // get the original uvs
            float3 originalUVs[4];
            originalUVs[0] = pFaceDef->Visual.MinUV;
            originalUVs[1].Set(pFaceDef->Visual.MaxUV.x, pFaceDef->Visual.MinUV.y, pFaceDef->Visual.MinUV.z);
            originalUVs[2] = pFaceDef->Visual.MaxUV;
            originalUVs[3].Set(pFaceDef->Visual.MinUV.x, pFaceDef->Visual.MaxUV.y, pFaceDef->Visual.MinUV.z);

            // repeat the texture
            switch (blockRotation)
            {
            case BLOCK_WORLD_BLOCK_ROTATION_NORTH:
            case BLOCK_WORLD_BLOCK_ROTATION_SOUTH:
                originalUVs[1].x *= (float)(blockCountX << lodLevel);
                originalUVs[2].x *= (float)(blockCountX << lodLevel);
                originalUVs[2].y *= (float)(blockCountY << lodLevel);
                originalUVs[3].y *= (float)(blockCountY << lodLevel);
                break;

            case BLOCK_WORLD_BLOCK_ROTATION_EAST:
            case BLOCK_WORLD_BLOCK_ROTATION_WEST:
                originalUVs[1].x *= (float)(blockCountY << lodLevel);
                originalUVs[2].x *= (float)(blockCountY << lodLevel);
                originalUVs[2].y *= (float)(blockCountX << lodLevel);
                originalUVs[3].y *= (float)(blockCountX << lodLevel);
                break;
            }           

            // and reorientate them according to rotation
            uvTopLeft = originalUVs[(0 + blockRotation) % 4];
            uvTopRight = originalUVs[(1 + blockRotation) % 4];
            uvBottomRight = originalUVs[(2 + blockRotation) % 4];
            uvBottomLeft = originalUVs[(3 + blockRotation) % 4];
        }
        break;

    default:
        UnreachableCode();
        return;
    }

    // macro to generate coordinates
#define MAKE_CUBE_VERTEX_POS(x, y, z) ((float3((float)((x) << lodLevel), (float)((y) << lodLevel), (float)((z) << lodLevel))) + basePosition)

    // generate vertices
    uint32 baseVertex = output.Vertices.GetSize();

    // generate quads
    switch (face)
    {
    case CUBE_FACE_RIGHT:
        {
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(endX + 1, endY + 1, endZ + 1), uvTopLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);   // top-left
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(endX + 1, startY, endZ + 1), uvTopRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);       // top-right
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(endX + 1, endY + 1, startZ), uvBottomLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);                                // bottom-left
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(endX + 1, startY, startZ), uvBottomRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);                                    // bottom-right
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 1, baseVertex + 2);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 3, baseVertex + 2);
        }
        break;

    case CUBE_FACE_LEFT:
        {
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(startX, endY + 1, endZ + 1), uvTopLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);           // top-left
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(startX, startY, endZ + 1), uvTopRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);               // top-right
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(startX, endY + 1, startZ), uvBottomLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);                                        // bottom-left
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(startX, startY, startZ), uvBottomRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);                                            // bottom-right
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 2, baseVertex + 1);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 2, baseVertex + 3);
        }
        break;

    case CUBE_FACE_BACK:
        {
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(startX, endY + 1, endZ + 1), uvTopLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BACK, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][1]);           // top-left
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(endX + 1, endY + 1, endZ + 1), uvTopRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BACK, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][1]);       // top-right
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(startX, endY + 1, startZ), uvBottomLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BACK, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][1]);                                        // bottom-left
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(endX + 1, endY + 1, startZ), uvBottomRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BACK, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][1]);                                    // bottom-right
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 1, baseVertex + 2);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 3, baseVertex + 2);
        }
        break;

    case CUBE_FACE_FRONT:
        {
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(startX, startY, endZ + 1), uvTopLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);           // top-left
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(endX + 1, startY, endZ + 1), uvTopRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);       // top-right
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(startX, startY, startZ), uvBottomLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);                                        // bottom-left
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(endX + 1, startY, startZ), uvBottomRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);                                    // bottom-right
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 2, baseVertex + 1);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 2, baseVertex + 3);
        }
        break;

    case CUBE_FACE_TOP:
        {
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(startX, endY + 1, endZ + 1), uvTopLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);               // top-left
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(endX + 1, endY + 1, endZ + 1), uvTopRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);           // top-right
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(startX, startY, endZ + 1), uvBottomLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);                   // bottom-left
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(endX + 1, startY, endZ + 1), uvBottomRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);               // bottom-right
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 2, baseVertex + 1);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 2, baseVertex + 3);
        }
        break;

    case CUBE_FACE_BOTTOM:
        {
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(startX, endY + 1, startZ), uvTopLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BOTTOM, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][1]);                                // top-left
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(endX + 1, endY + 1, startZ), uvTopRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BOTTOM, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][1]);                            // top-right
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(startX, startY, startZ), uvBottomLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BOTTOM, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][1]);                                    // bottom-left
            output.Vertices.Emplace(MAKE_CUBE_VERTEX_POS(endX + 1, startY, startZ), uvBottomRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BOTTOM, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][1]);                                // bottom-right
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 1, baseVertex + 2);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 3, baseVertex + 2);
        }
        break;
    }

#undef MAKE_CUBE_VERTEX_POS
}

void BlockWorldMesher::AddSlabBlockFace(const BlockPalette::BlockType *pBlockType, BlockWorldBlockType blockValue, uint32 blockLighting, uint8 blockRotation, const float3 &basePosition, uint32 lodLevel, uint32 startX, uint32 startY, uint32 startZ, uint32 endX, uint32 endY, uint32 endZ, CUBE_FACE face, bool isTopSlab, Output &output)
{
    // calculate top slab subtract amount
    float3 topBlockSubtractAmount(float3::Zero);
    float sideTopUVAdd = 0.0f;

    // fix up top slabs of volumes
    if (!(pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_CUBE_SHAPE_VOLUME) || isTopSlab)
    {
        // bring the side uvs in
        if (face >= CUBE_FACE_RIGHT && face <= CUBE_FACE_FRONT)
            sideTopUVAdd = 1.0f - pBlockType->SlabShapeSettings.Height;

        // adjust the height of the block
        topBlockSubtractAmount.Set(0.0f, 0.0f, static_cast<float>((uint32)1 << lodLevel) * (1.0f - pBlockType->SlabShapeSettings.Height));
    }

    // find block counts
    uint32 blockCountX = (endX - startX) + 1;
    uint32 blockCountY = (endY - startY) + 1;
    uint32 blockCountZ = (endZ - startZ) + 1;

    // get uvs and color
    uint32 materialIndex;
    float3 uvTopLeft;
    float3 uvTopRight;
    float3 uvBottomRight;
    float3 uvBottomLeft;

    // this depends on the face and rotation
    switch (face)
    {
    case CUBE_FACE_RIGHT:
    case CUBE_FACE_LEFT:
    case CUBE_FACE_BACK:
    case CUBE_FACE_FRONT:
        {
            // R/L/B/F can just use the reorientated face's uvs
            const BlockPalette::BlockType::CubeShapeFace *pFaceDef = &pBlockType->CubeShapeFaces[CUBE_FACE_ROTATION_LUT[blockRotation][face]];
            materialIndex = pFaceDef->Visual.MaterialIndex;
            uvTopLeft.Set(pFaceDef->Visual.MinUV.x, pFaceDef->Visual.MinUV.y - sideTopUVAdd, pFaceDef->Visual.MinUV.z);
            uvTopRight.Set(pFaceDef->Visual.MaxUV.x, pFaceDef->Visual.MinUV.y - sideTopUVAdd, pFaceDef->Visual.MinUV.z);
            uvBottomRight = pFaceDef->Visual.MaxUV;
            uvBottomLeft.Set(pFaceDef->Visual.MinUV.x, pFaceDef->Visual.MaxUV.y, pFaceDef->Visual.MinUV.z);

            // repeat the texture
            switch (face)
            {
            case CUBE_FACE_RIGHT:
            case CUBE_FACE_LEFT:
                uvTopRight.x *= (float)(blockCountY << lodLevel);
                uvBottomRight.x *= (float)(blockCountY << lodLevel);
                uvBottomRight.y *= (float)(blockCountZ << lodLevel);
                uvBottomLeft.y *= (float)(blockCountZ << lodLevel);
                break;

            case CUBE_FACE_BACK:
            case CUBE_FACE_FRONT:
                uvTopRight.x *= (float)(blockCountX << lodLevel);
                uvBottomRight.x *= (float)(blockCountX << lodLevel);
                uvBottomRight.y *= (float)(blockCountZ << lodLevel);
                uvBottomLeft.y *= (float)(blockCountZ << lodLevel);
                break;
            }
        }
        break;

    case CUBE_FACE_TOP:
    case CUBE_FACE_BOTTOM:
        {
            // T/B have to have their uvs rotated using the same face
            const BlockPalette::BlockType::CubeShapeFace *pFaceDef = &pBlockType->CubeShapeFaces[face];
            materialIndex = pFaceDef->Visual.MaterialIndex;

            // get the original uvs
            float3 originalUVs[4];
            originalUVs[0] = pFaceDef->Visual.MinUV;
            originalUVs[1].Set(pFaceDef->Visual.MaxUV.x, pFaceDef->Visual.MinUV.y, pFaceDef->Visual.MinUV.z);
            originalUVs[2] = pFaceDef->Visual.MaxUV;
            originalUVs[3].Set(pFaceDef->Visual.MinUV.x, pFaceDef->Visual.MaxUV.y, pFaceDef->Visual.MinUV.z);

            // repeat the texture
            switch (blockRotation)
            {
            case BLOCK_WORLD_BLOCK_ROTATION_NORTH:
            case BLOCK_WORLD_BLOCK_ROTATION_SOUTH:
                originalUVs[1].x *= (float)(blockCountX << lodLevel);
                originalUVs[2].x *= (float)(blockCountX << lodLevel);
                originalUVs[2].y *= (float)(blockCountY << lodLevel);
                originalUVs[3].y *= (float)(blockCountY << lodLevel);
                break;

            case BLOCK_WORLD_BLOCK_ROTATION_EAST:
            case BLOCK_WORLD_BLOCK_ROTATION_WEST:
                originalUVs[1].x *= (float)(blockCountY << lodLevel);
                originalUVs[2].x *= (float)(blockCountY << lodLevel);
                originalUVs[2].y *= (float)(blockCountX << lodLevel);
                originalUVs[3].y *= (float)(blockCountX << lodLevel);
                break;
            }

            // and reorientate them according to rotation
            uvTopLeft = originalUVs[(0 + blockRotation) % 4];
            uvTopRight = originalUVs[(1 + blockRotation) % 4];
            uvBottomRight = originalUVs[(2 + blockRotation) % 4];
            uvBottomLeft = originalUVs[(3 + blockRotation) % 4];
        }
        break;

    default:
        UnreachableCode();
        return;
    }

    // macro to generate coordinates
#define MAKE_SLAB_VERTEX_POS(x, y, z) (/*transformMatrix.TransformPoint*/(float3((float)((x) << lodLevel), (float)((y) << lodLevel), (float)((z) << lodLevel))) + basePosition)
    
    // generate vertices
    //float4x4 transformMatrix(BLOCK_ROTATION_MATRIX_LUT[blockRotation]);
    uint32 baseVertex = output.Vertices.GetSize();

    // generate quads
    switch (face)
    {
    case CUBE_FACE_RIGHT:
        {
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(endX + 1, endY + 1, endZ + 1) - topBlockSubtractAmount, uvTopLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);   // top-left
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(endX + 1, startY, endZ + 1) - topBlockSubtractAmount, uvTopRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);       // top-right
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(endX + 1, endY + 1, startZ), uvBottomLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);                                // bottom-left
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(endX + 1, startY, startZ), uvBottomRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);                                    // bottom-right
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 1, baseVertex + 2);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 3, baseVertex + 2);
        }
        break;

    case CUBE_FACE_LEFT:
        {
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(startX, endY + 1, endZ + 1) - topBlockSubtractAmount, uvTopLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);           // top-left
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(startX, startY, endZ + 1) - topBlockSubtractAmount, uvTopRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);               // top-right
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(startX, endY + 1, startZ), uvBottomLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);                                        // bottom-left
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(startX, startY, startZ), uvBottomRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);                                            // bottom-right
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 2, baseVertex + 1);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 2, baseVertex + 3);
        }
        break;

    case CUBE_FACE_BACK:
        {
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(startX, endY + 1, endZ + 1) - topBlockSubtractAmount, uvTopLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BACK, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][1]);           // top-left
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(endX + 1, endY + 1, endZ + 1) - topBlockSubtractAmount, uvTopRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BACK, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][1]);       // top-right
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(startX, endY + 1, startZ), uvBottomLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BACK, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][1]);                                        // bottom-left
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(endX + 1, endY + 1, startZ), uvBottomRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BACK, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][1]);                                    // bottom-right
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 1, baseVertex + 2);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 3, baseVertex + 2);
        }
        break;

    case CUBE_FACE_FRONT:
        {
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(startX, startY, endZ + 1) - topBlockSubtractAmount, uvTopLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);           // top-left
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(endX + 1, startY, endZ + 1) - topBlockSubtractAmount, uvTopRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);       // top-right
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(startX, startY, startZ), uvBottomLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);                                        // bottom-left
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(endX + 1, startY, startZ), uvBottomRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);                                    // bottom-right
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 2, baseVertex + 1);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 2, baseVertex + 3);
        }
        break;

    case CUBE_FACE_TOP:
        {
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(startX, endY + 1, endZ + 1) - topBlockSubtractAmount, uvTopLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);               // top-left
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(endX + 1, endY + 1, endZ + 1) - topBlockSubtractAmount, uvTopRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);           // top-right
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(startX, startY, endZ + 1) - topBlockSubtractAmount, uvBottomLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);                   // bottom-left
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(endX + 1, startY, endZ + 1) - topBlockSubtractAmount, uvBottomRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);               // bottom-right
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 2, baseVertex + 1);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 2, baseVertex + 3);
        }
        break;

    case CUBE_FACE_BOTTOM:
        {
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(startX, endY + 1, startZ), uvTopLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BOTTOM, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][1]);                                // top-left
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(endX + 1, endY + 1, startZ), uvTopRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BOTTOM, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][1]);                            // top-right
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(startX, startY, startZ), uvBottomLeft, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BOTTOM, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][1]);                                    // bottom-left
            output.Vertices.Emplace(MAKE_SLAB_VERTEX_POS(endX + 1, startY, startZ), uvBottomRight, CalculateCubeVertexColor(pBlockType, CUBE_FACE_BOTTOM, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][1]);                                // bottom-right
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 1, baseVertex + 2);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 3, baseVertex + 2);
        }
        break;
    }

#undef MAKE_SLAB_VERTEX_POS
}

void BlockWorldMesher::AddStairBlockFace(const BlockPalette::BlockType *pBlockType, BlockWorldBlockType blockValue, uint32 blockLighting, uint8 blockRotation, const float3 &basePosition, uint32 lodLevel, uint32 startX, uint32 startY, uint32 startZ, uint32 endX, uint32 endY, uint32 endZ, CUBE_FACE face, Output &output)
{
    // stairs should always be one block high
    DebugAssert(startZ == endZ);

    // get uvs and color
    const BlockPalette::BlockType::CubeShapeFace *pFaceDef = &pBlockType->CubeShapeFaces[face];
    uint32 materialIndex = pFaceDef->Visual.MaterialIndex;
    float3 minUV = pFaceDef->Visual.MinUV;
    float3 halfUV = pFaceDef->Visual.MaxUV - float3(0.5f, 0.5f, 0.0f);
    float3 maxUV = pFaceDef->Visual.MaxUV;

    // repeat the texture
    uint32 blockCountX = (endX - startX) + 1;
    uint32 blockCountY = (endY - startY) + 1;
    uint32 blockCountZ = (endZ - startZ) + 1;
    switch (face)
    {
    case CUBE_FACE_RIGHT:
    case CUBE_FACE_LEFT:
        halfUV.x *= (float)(blockCountY << lodLevel);
        halfUV.y *= (float)(blockCountZ << lodLevel);
        maxUV.x *= (float)(blockCountY << lodLevel);
        maxUV.y *= (float)(blockCountZ << lodLevel);
        break;

    case CUBE_FACE_BACK:
    case CUBE_FACE_FRONT:
        halfUV.x *= (float)(blockCountX << lodLevel);
        halfUV.y *= (float)(blockCountZ << lodLevel);
        maxUV.x *= (float)(blockCountX << lodLevel);
        maxUV.y *= (float)(blockCountZ << lodLevel);
        break;

    case CUBE_FACE_TOP:
    case CUBE_FACE_BOTTOM:
        halfUV.x *= (float)(blockCountX << lodLevel);
        halfUV.y *= (float)(blockCountY << lodLevel);
        maxUV.x *= (float)(blockCountX << lodLevel);
        maxUV.y *= (float)(blockCountY << lodLevel);
        break;
    }

    // macro to generate coordinates
#define MAKE_STAIR_VERTEX_POS(x, y, z) ((transformMatrix.TransformPoint(float3(x, y, z)) * scale) + startPos)

    // generate vertices
    float4x4 transformMatrix(BLOCK_ROTATION_MATRIX_LUT[blockRotation]);
    uint32 baseVertex = output.Vertices.GetSize();
    float scale = (float)(1 << lodLevel);

    // convert start/end to floats
    float fBlockCountX = (float)blockCountX;
    float fBlockCountY = (float)blockCountY;
    float fBlockCountZ = (float)blockCountZ;
    float3 startPos(float3((float)startX * scale, (float)startY * scale, (float)startZ * scale) + basePosition);

    // generate quads
    switch (CUBE_FACE_ROTATION_LUT[blockRotation][face])
    {
    case CUBE_FACE_RIGHT:
        {
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, fBlockCountY, 0.5f), float3(minUV.x, minUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);    // top-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, 0.0f, 0.5f), float3(halfUV.x, minUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);        // top-right
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, fBlockCountY, 0.0f), float3(minUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);        // bottom-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, 0.0f, 0.0f), float3(halfUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);            // bottom-right

            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, fBlockCountY, fBlockCountZ), float3(halfUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);    // top-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, 0.5f, fBlockCountZ), float3(maxUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);        // top-right
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, fBlockCountY, 0.5f), float3(halfUV.x, maxUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);        // bottom-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, 0.5f, 0.5f), float3(maxUV.x, maxUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_RIGHT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_RIGHT][1]);            // bottom-right
            
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 1, baseVertex + 2);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 3, baseVertex + 2);

            output.Triangles.Emplace(materialIndex, baseVertex + 4, baseVertex + 5, baseVertex + 6);
            output.Triangles.Emplace(materialIndex, baseVertex + 5, baseVertex + 7, baseVertex + 6);
        }
        break;

    case CUBE_FACE_LEFT:
        {
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, fBlockCountY, 0.5f), float3(minUV.x, minUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);            // top-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, 0.0f, 0.5f), float3(halfUV.x, minUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);                // top-right
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, fBlockCountY, 0.0f), float3(minUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);                // bottom-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, 0.0f, 0.0f), float3(halfUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);                    // bottom-right

            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, fBlockCountY, fBlockCountZ), float3(halfUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);            // top-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, 0.5f, fBlockCountZ), float3(maxUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);                // top-right
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, fBlockCountY, 0.5f), float3(halfUV.x, maxUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);                // bottom-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, 0.5f, 0.5f), float3(maxUV.x, maxUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_LEFT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_LEFT][1]);                    // bottom-right

            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 2, baseVertex + 1);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 2, baseVertex + 3);

            output.Triangles.Emplace(materialIndex, baseVertex + 4, baseVertex + 6, baseVertex + 5);
            output.Triangles.Emplace(materialIndex, baseVertex + 5, baseVertex + 6, baseVertex + 7);
        }
        break;

    case CUBE_FACE_BACK:
        {
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, fBlockCountY, fBlockCountZ), float3(minUV.x, minUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_BACK, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][1]);            // top-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, fBlockCountY, fBlockCountZ), float3(maxUV.x, minUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_BACK, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][1]);        // top-right
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, fBlockCountY, 0.0f), float3(minUV.x, maxUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_BACK, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][1]);                // bottom-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, fBlockCountY, 0.0f), float3(maxUV.x, maxUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_BACK, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BACK][1]);            // bottom-right
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 1, baseVertex + 2);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 3, baseVertex + 2);
        }
        break;

    case CUBE_FACE_FRONT:
        {
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, 0.0f, 0.5f), float3(minUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);            // top-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, 0.0f, 0.5f), float3(maxUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);        // top-right
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, 0.0f, 0.0f), float3(minUV.x, minUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);                // bottom-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, 0.0f, 0.0f), float3(maxUV.x, minUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);            // bottom-right

            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, 0.5f, fBlockCountZ), float3(minUV.x, maxUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);            // top-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, 0.5f, fBlockCountZ), float3(maxUV.x, maxUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);        // top-right
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, 0.5f, 0.5f), float3(minUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);                // bottom-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, 0.5f, 0.5f), float3(maxUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_FRONT, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_FRONT][1]);            // bottom-right

            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 2, baseVertex + 1);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 2, baseVertex + 3);

            output.Triangles.Emplace(materialIndex, baseVertex + 4, baseVertex + 6, baseVertex + 5);
            output.Triangles.Emplace(materialIndex, baseVertex + 5, baseVertex + 6, baseVertex + 7);
        }
        break;

    case CUBE_FACE_TOP:
        {
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, 0.5f, 0.5f), float3(minUV.x, minUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);                // top-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, 0.5f, 0.5f), float3(halfUV.x, minUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);            // top-right
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, 0.0f, 0.5f), float3(minUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);                    // bottom-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, 0.0f, 0.5f), float3(halfUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);                // bottom-right

            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, fBlockCountY, fBlockCountZ), float3(halfUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);                // top-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, fBlockCountY, fBlockCountZ), float3(maxUV.x, halfUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);            // top-right
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, 0.5f, fBlockCountZ), float3(halfUV.x, maxUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);                    // bottom-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, 0.5f, fBlockCountZ), float3(maxUV.x, maxUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_TOP, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_TOP][1]);                // bottom-right

            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 2, baseVertex + 1);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 2, baseVertex + 3);

            output.Triangles.Emplace(materialIndex, baseVertex + 4, baseVertex + 6, baseVertex + 5);
            output.Triangles.Emplace(materialIndex, baseVertex + 5, baseVertex + 6, baseVertex + 7);
        }
        break;

    case CUBE_FACE_BOTTOM:
        {
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, fBlockCountY, 0.0f), float3(minUV.x, minUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_BOTTOM, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][1]);        // top-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, fBlockCountY, 0.0f), float3(maxUV.x, minUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_BOTTOM, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][1]);    // top-right
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(0.0f, 0.0f, 0.0f), float3(minUV.x, maxUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_BOTTOM, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][1]);            // bottom-left
            output.Vertices.Emplace(MAKE_STAIR_VERTEX_POS(fBlockCountX, 0.0f, 0.0f), float3(maxUV.x, maxUV.y, minUV.z), CalculateCubeVertexColor(pBlockType, CUBE_FACE_BOTTOM, blockValue, blockLighting), CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][0], CUBE_FACE_PACKED_TANGENT_NORMALS[CUBE_FACE_BOTTOM][1]);        // bottom-right
            output.Triangles.Emplace(materialIndex, baseVertex + 0, baseVertex + 1, baseVertex + 2);
            output.Triangles.Emplace(materialIndex, baseVertex + 1, baseVertex + 3, baseVertex + 2);
        }
        break;
    }

#undef MAKE_STAIR_VERTEX_POS
}

void BlockWorldMesher::AddPlaneBlock(const BlockPalette::BlockType *pBlockType, BlockWorldBlockType blockValue, uint32 blockLighting, uint8 blockRotation, const float3 &basePosition, uint32 lodLevel, uint32 x, uint32 y, uint32 z, Output &output)
{
    // get settings
    const BlockPalette::BlockType::PlaneShape &planeSettings = pBlockType->PlaneShapeSettings;
    const BlockPalette::BlockType::VisualParameters &visualSettings = pBlockType->PlaneShapeSettings.Visual;
    float3 faceMinUV = visualSettings.MinUV;
    float3 faceMaxUV = visualSettings.MaxUV;
    uint32 faceMaterialIndex = visualSettings.MaterialIndex;

    // calculate light colour for face, just use the top
    uint32 vertexColor = CalculatePlaneVertexColor(pBlockType, blockValue, blockLighting);

    // generate base vertices
    FullVertex baseVertices[8];
    baseVertices[0].Set(float3(-0.5f, 0.0f, 1.0f), float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), vertexColor, float3::Zero, float3::Zero, float3::Zero);   // top-left
    baseVertices[1].Set(float3(0.5f, 0.0f, 1.0f), float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), vertexColor, float3::Zero, float3::Zero, float3::Zero);    // top-right
    baseVertices[2].Set(float3(-0.5f, 0.0f, 0.0f), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), vertexColor, float3::Zero, float3::Zero, float3::Zero);   // bottom-left
    baseVertices[3].Set(float3(0.5f, 0.0f, 0.0f), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), vertexColor, float3::Zero, float3::Zero, float3::Zero);    // bottom-right
    baseVertices[4].Set(float3(-0.5f, 0.0f, 1.0f), float3(faceMinUV.x, faceMinUV.y, faceMinUV.z), vertexColor, float3::Zero, float3::Zero, float3::Zero);   // top-left
    baseVertices[5].Set(float3(0.5f, 0.0f, 1.0f), float3(faceMaxUV.x, faceMinUV.y, faceMinUV.z), vertexColor, float3::Zero, float3::Zero, float3::Zero);    // top-right
    baseVertices[6].Set(float3(-0.5f, 0.0f, 0.0f), float3(faceMinUV.x, faceMaxUV.y, faceMinUV.z), vertexColor, float3::Zero, float3::Zero, float3::Zero);   // bottom-left
    baseVertices[7].Set(float3(0.5f, 0.0f, 0.0f), float3(faceMaxUV.x, faceMaxUV.y, faceMinUV.z), vertexColor, float3::Zero, float3::Zero, float3::Zero);    // bottom-right

    // do plane @todo block rotation
    float4x4 transformMatrix(BLOCK_ROTATION_MATRIX_LUT[blockRotation]);
    float currentRotation = pBlockType->PlaneShapeSettings.BaseRotation;
    for (uint32 i = 0; i < pBlockType->PlaneShapeSettings.RepeatCount; i++, currentRotation += pBlockType->PlaneShapeSettings.RepeatRotation)
    {
        // copy base
        FullVertex planeVertices[8];
        Y_memcpy(planeVertices, baseVertices, sizeof(planeVertices));

        // create quat for rotation
        Quaternion planeRotation(Quaternion::FromEulerAngles(0.0f, 0.0f, currentRotation));

        // move into world space
        float scale = (float)(1 << lodLevel);
        float3 blockPos(float3((float)(x << lodLevel), (float)(y << lodLevel), (float)(z << lodLevel)) + basePosition);

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

            // move into world-space
            planeVertices[j].Position = transformMatrix.TransformPoint(planeVertices[j].Position * scale) + blockPos;
        }

        // calculate the tangent space vectors for each triangle
        float3 tangent, binormal, normal;
#define GEN_TANGENTS(v0, v1, v2) MULTI_STATEMENT_MACRO_BEGIN \
                                    MeshUtilites::CalculateTangentSpaceVectors(planeVertices[v0].Position, planeVertices[v1].Position, planeVertices[v2].Position, planeVertices[v0].TexCoord.xy(), planeVertices[v1].TexCoord.xy(), planeVertices[v2].TexCoord.xy(), tangent, binormal, normal); \
                                    planeVertices[v0].Tangent += tangent; planeVertices[v0].Binormal += binormal; planeVertices[v0].Normal += normal; \
                                    planeVertices[v1].Tangent += tangent; planeVertices[v1].Binormal += binormal; planeVertices[v1].Normal += normal; \
                                    planeVertices[v2].Tangent += tangent; planeVertices[v2].Binormal += binormal; planeVertices[v2].Normal += normal; \
                                 MULTI_STATEMENT_MACRO_END

        // for each 4 triangles
        GEN_TANGENTS(2, 3, 0);
        GEN_TANGENTS(0, 3, 1);
        GEN_TANGENTS(6, 4, 7);
        GEN_TANGENTS(7, 4, 5);

        // normalize each tangent space vector
        for (uint32 j = 0; j < countof(planeVertices); j++)
        {
            planeVertices[j].Tangent.SafeNormalizeInPlace();
            planeVertices[j].Binormal.SafeNormalizeInPlace();
            planeVertices[j].Normal.SafeNormalizeInPlace();
        }
#undef GEN_TANGENTS

        // add to list
        uint32 baseVertex = output.Vertices.GetSize();
        for (uint32 j = 0; j < countof(planeVertices); j++)
        {
            const FullVertex &v = planeVertices[j];
            output.Vertices.Emplace(v.Position, v.TexCoord, v.Color, v.Tangent, v.Binormal, v.Normal);
        }

        // generate 4 triangles, one for each quad
        output.Triangles.Emplace(faceMaterialIndex, baseVertex + 2, baseVertex + 3, baseVertex + 0);
        output.Triangles.Emplace(faceMaterialIndex, baseVertex + 0, baseVertex + 3, baseVertex + 1);
        output.Triangles.Emplace(faceMaterialIndex, baseVertex + 6, baseVertex + 4, baseVertex + 7);
        output.Triangles.Emplace(faceMaterialIndex, baseVertex + 7, baseVertex + 4, baseVertex + 5);
    }
}

void BlockWorldMesher::AddMeshBlock(const BlockPalette::BlockType *pBlockType, BlockWorldBlockType blockValue, uint32 blockLighting, uint8 blockRotation, const float3 &basePosition, uint32 lodLevel, uint32 x, uint32 y, uint32 z, Output &output)
{
    // find scale
    float scale = (float)(1 << lodLevel);

    // find translation
    float3 translation(float3((float)(x << lodLevel), (float)(y << lodLevel), (float)(z << lodLevel)) + basePosition);
    translation += float3(0.5f, 0.5f, 0.0f) * scale;

    // calculate transform
    static const float rotationLUT[4] = { 0.0f, 90.0f, 180.0f, 270.0f };
    float3x4 transform(float4x4::MakeTranslationMatrix(translation) *
                       float4x4::MakeRotationMatrixZ(rotationLUT[blockRotation]) * 
                       float4x4::MakeScaleMatrix(pBlockType->MeshShapeSettings.Scale * scale));

    // find the instances for this mesh
    uint32 index = 0;
    for (; index < output.Instances.GetSize(); index++)
    {
        if (output.Instances[index]->MeshIndex == pBlockType->MeshShapeSettings.MeshIndex)
            break;
    }
    if (index == output.Instances.GetSize())
    {
        MeshInstances *pMeshInstances = new MeshInstances;
        pMeshInstances->MeshIndex = pBlockType->MeshShapeSettings.MeshIndex;
        pMeshInstances->BufferOffset = 0;
        output.Instances.Add(pMeshInstances);
    }

    // add to the transform list for this mesh, @todo fixup bounds
    output.Instances[index]->Transforms.Add(transform);
}

void BlockWorldMesher::AddLightBlock(const BlockPalette::BlockType *pBlockType, const float3 &basePosition, uint32 lodLevel, uint32 x, uint32 y, uint32 z, Output &output)
{
    // find scale
    float scale = (float)(1 << lodLevel);

    // find translation
    float3 translation(float3((float)(x << lodLevel), (float)(y << lodLevel), (float)(z << lodLevel)) + basePosition);
    translation += (float3(0.5f, 0.5f, 0.0f) + pBlockType->PointLightEmitterSettings.Offset) * scale;

    RENDER_QUEUE_POINT_LIGHT_ENTRY lightEntry;
    lightEntry.Position = translation;
    lightEntry.Range = pBlockType->PointLightEmitterSettings.Range * scale;
    lightEntry.InverseRange = 1.0f / (pBlockType->PointLightEmitterSettings.Range * scale);
    lightEntry.LightColor = PixelFormatHelpers::ConvertRGBAToFloat4(pBlockType->PointLightEmitterSettings.Color).xyz() * pBlockType->PointLightEmitterSettings.Brightness;
    lightEntry.FalloffExponent = pBlockType->PointLightEmitterSettings.Falloff;
    lightEntry.ShadowFlags = 0;
    lightEntry.ShadowMapIndex = -1;
    lightEntry.Static = false;
    output.Lights.Add(lightEntry);
}

