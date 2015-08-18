#include "Engine/PrecompiledHeader.h"
#include "Engine/Physics/CollisionShape.h"
#include "Engine/Physics/BoxCollisionShape.h"
#include "Engine/Physics/ConvexHullCollisionShape.h"
#include "Engine/Physics/SphereCollisionShape.h"
#include "Engine/Physics/TriangleMeshCollisionShape.h"

namespace Physics {

Y_Define_NameTable(NameTables::CollisionShapeType)
    Y_NameTable_VEntry(COLLISION_SHAPE_TYPE_NONE, "None")
    Y_NameTable_VEntry(COLLISION_SHAPE_TYPE_BOX, "Box")
    Y_NameTable_VEntry(COLLISION_SHAPE_TYPE_SPHERE, "Sphere")
    Y_NameTable_VEntry(COLLISION_SHAPE_TYPE_TRIANGLE_MESH, "TriangleMesh")
    Y_NameTable_VEntry(COLLISION_SHAPE_TYPE_SCALED_TRIANGLE_MESH, "ScaledTriangleMesh")
    Y_NameTable_VEntry(COLLISION_SHAPE_TYPE_CONVEX_HULL, "ConvexHull")
    Y_NameTable_VEntry(COLLISION_SHAPE_TYPE_TERRAIN, "Terrain")
    Y_NameTable_VEntry(COLLISION_SHAPE_TYPE_BLOCK_MESH, "BlockMesh")
    Y_NameTable_VEntry(COLLISION_SHAPE_TYPE_BLOCK_TERRAIN, "BlockTerrain")
Y_NameTable_End()

static CollisionShape *CreateCollisionShape(uint8 shapeType)
{
    switch (shapeType)
    {
    case COLLISION_SHAPE_TYPE_BOX:
        return new BoxCollisionShape();
        
    case COLLISION_SHAPE_TYPE_SPHERE:
        return new SphereCollisionShape();

    case COLLISION_SHAPE_TYPE_CONVEX_HULL:
        return nullptr;

    case COLLISION_SHAPE_TYPE_TRIANGLE_MESH:
        return new TriangleMeshCollisionShape();
    }

    return nullptr;
}

CollisionShape *CollisionShape::CreateFromData(const void *pData, uint32 dataSize)
{
    if (dataSize < sizeof(uint8))
        return nullptr;

    const uint8 shapeType = *(const uint8 *)pData;
    const byte *pShapeData = reinterpret_cast<const byte *>(pData) + sizeof(uint8);
    uint32 shapeDataSize = dataSize - sizeof(uint8);

    CollisionShape *pReturnShape = CreateCollisionShape(shapeType);
    if (pReturnShape == nullptr || !pReturnShape->LoadFromData(pShapeData, shapeDataSize))
    {
        pReturnShape->Release();
        return nullptr;
    }

    return pReturnShape;
}

CollisionShape *CollisionShape::CreateFromStream(ByteStream *pStream, uint32 dataSize)
{
    if (dataSize < sizeof(uint8))
        return nullptr;

    uint8 shapeType;
    uint32 shapeDataSize = dataSize - sizeof(uint8);
    if (!pStream->ReadByte(&shapeType))
        return nullptr;    

    CollisionShape *pReturnShape = CreateCollisionShape(shapeType);
    if (pReturnShape == nullptr || !pReturnShape->LoadFromStream(pStream, shapeDataSize))
    {
        pReturnShape->Release();
        return nullptr;
    }

    return pReturnShape;
}

}       // namespace Physics
