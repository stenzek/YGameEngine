#include "BlockEngine/PrecompiledHeader.h"
#include "BlockEngine/BlockWorldChunkCollisionShape.h"
#include "BlockEngine/BlockWorldChunk.h"
#include "Engine/BlockPalette.h"
#include "Engine/Physics/BulletHeaders.h"
Log_SetChannel(BlockWorldChunkCollisionShape);

ATTRIBUTE_ALIGNED16(class) btBlockWorldChunkCollisionShape : public btConcaveShape
{
public:
    BT_DECLARE_ALIGNED_ALLOCATOR();

    btBlockWorldChunkCollisionShape(const BlockPalette *pPalette, uint32 chunkSize, const BlockWorldChunk *pChunk)
        : btConcaveShape(),
          m_pPalette(pPalette),
          m_chunkSize(chunkSize),
          m_pChunk(pChunk),
          m_aabbMin(0.0f, 0.0f, 0.0f),
          m_aabbMax((float)chunkSize, (float)chunkSize, (float)chunkSize),
          m_localScaling(1.0f, 1.0f, 1.0f)
    {
        m_shapeType = CUSTOM_CONCAVE_SHAPE_TYPE;
    }

    virtual ~btBlockWorldChunkCollisionShape()
    {

    }

    virtual void processAllTriangles(btTriangleCallback* callback,const btVector3& aabbMin,const btVector3& aabbMax) const;

    virtual void getAabb(const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
    {
        aabbMin = t(m_aabbMin);
        aabbMax = t(m_aabbMax);
    }

    virtual void setLocalScaling(const btVector3& scaling) { m_localScaling = scaling; }
    virtual const btVector3& getLocalScaling() const { return m_localScaling; }

    virtual void calculateLocalInertia(btScalar mass,btVector3& inertia) const;

    virtual const char* getName() const { return "btBlockTerrainCollisionShape"; }

private:
    const BlockPalette *m_pPalette;
    uint32 m_chunkSize;
    const BlockWorldChunk *m_pChunk;

    btVector3 m_aabbMin;
    btVector3 m_aabbMax;
    btVector3 m_localScaling;
};

BlockWorldChunkCollisionShape::BlockWorldChunkCollisionShape(const BlockPalette *pPalette, uint32 chunkSize, const BlockWorldChunk *pChunk)
{
    // create bullet shape
    m_pBulletShape = new btBlockWorldChunkCollisionShape(pPalette, chunkSize, pChunk);
    m_boundingBox.SetBounds(float3::Zero, float3((float)chunkSize, (float)chunkSize, (float)chunkSize));
}

BlockWorldChunkCollisionShape::~BlockWorldChunkCollisionShape()
{
    delete m_pBulletShape;
}

const Physics::COLLISION_SHAPE_TYPE BlockWorldChunkCollisionShape::GetType() const
{
    return Physics::COLLISION_SHAPE_TYPE_CUSTOM;
}

const AABox &BlockWorldChunkCollisionShape::GetLocalBoundingBox() const
{
    return m_boundingBox;
}

bool BlockWorldChunkCollisionShape::LoadFromData(const void *pData, uint32 dataSize)
{
    return false;
}

bool BlockWorldChunkCollisionShape::LoadFromStream(ByteStream *pStream, uint32 dataSize)
{
    return false;
}

Physics::CollisionShape *BlockWorldChunkCollisionShape::CreateScaledShape(const float3 &scale) const
{
    Panic("Should never be called.");
    return NULL;
}

void BlockWorldChunkCollisionShape::ApplyShapeTransform(btTransform &worldTransform) const
{

}

void BlockWorldChunkCollisionShape::ApplyInverseShapeTransform(btTransform &worldTransform) const
{

}

btCollisionShape *BlockWorldChunkCollisionShape::GetBulletShape() const
{
    return static_cast<btCollisionShape *>(m_pBulletShape);
}

void btBlockWorldChunkCollisionShape::calculateLocalInertia(btScalar mass, btVector3& inertia) const
{
    //moving concave objects not supported
    inertia.setValue(btScalar(0.),btScalar(0.),btScalar(0.));
}

static inline btVector3 GetBlockVertexPosition(const btVector3 &baseTranslation, const float blockSizeInWorldUnits, int32 x, int32 y, int32 z) 
{
    return btVector3((float)x, (float)y, (float)z) * blockSizeInWorldUnits + baseTranslation;
}

void btBlockWorldChunkCollisionShape::processAllTriangles(btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax) const
{
    // can't handle != lod 0 atm
    if (m_pChunk->GetLoadedLODLevel() != 0)
        return;

    // get chunk data
    const BlockWorldBlockType *pBlockValues = m_pChunk->GetBlockValues(0);
    const uint32 yStride = m_chunkSize;
    const uint32 zStride = m_chunkSize * yStride;
    const int32 chunkSizeMinusOne = (int32)m_chunkSize - 1;

    // clip the chunk to the specified aabb
    uint32 minBlockX = (uint32)Math::Clamp((int32)Math::Floor(aabbMin.x()), 0, chunkSizeMinusOne);
    uint32 minBlockY = (uint32)Math::Clamp((int32)Math::Floor(aabbMin.y()), 0, chunkSizeMinusOne);
    uint32 minBlockZ = (uint32)Math::Clamp((int32)Math::Floor(aabbMin.z()), 0, chunkSizeMinusOne);
    uint32 maxBlockX = (uint32)Math::Clamp((int32)Math::Floor(aabbMax.x()), 0, chunkSizeMinusOne);
    uint32 maxBlockY = (uint32)Math::Clamp((int32)Math::Floor(aabbMax.y()), 0, chunkSizeMinusOne);
    uint32 maxBlockZ = (uint32)Math::Clamp((int32)Math::Floor(aabbMax.z()), 0, chunkSizeMinusOne);

    // iterate over matched blocks
    for (uint32 blockZ = minBlockZ; blockZ <= maxBlockZ; blockZ++)
    {
        for (uint32 blockY = minBlockY; blockY <= maxBlockY; blockY++)
        {
            for (uint32 blockX = minBlockX; blockX <= maxBlockX; blockX++)
            {
                // get block type
                BlockWorldBlockType blockValue = pBlockValues[blockZ * zStride + blockY * yStride + blockX];
                if (blockValue == 0)
                    continue;

                // get type
                float blockHeight = 1.0f;
                if ((blockValue & BLOCK_WORLD_BLOCK_VALUE_COLORED_FLAG_BIT) == 0)
                {
                    const BlockPalette::BlockType *pBlockType = m_pPalette->GetBlockType(blockValue);
                    if ((pBlockType->Flags & BLOCK_MESH_BLOCK_TYPE_FLAG_COLLIDABLE) == 0)
                        continue;

                    if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_SLAB)
                    {
                        // adjust height
                        blockHeight = pBlockType->SlabShapeSettings.Height;
                    }
                    else if (pBlockType->ShapeType == BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_MESH)
                    {
                        // lookup the mesh
                        const StaticMesh *pStaticMesh = m_pPalette->GetMesh(pBlockType->MeshShapeSettings.MeshIndex);
                        const Physics::CollisionShape *pCollisionShape = pStaticMesh->GetCollisionShape();
                        if (pCollisionShape == nullptr)
                            continue;

                        // only works for concave
                        if (!pCollisionShape->GetBulletShape()->isConcave())
                            continue;

                        // create a temporary transform
                        btQuaternion rotation;
                        rotation.setEuler(0.0f, 0.0f, 90.0f * (float)m_pChunk->GetBlockRotation(0, blockX, blockY, blockZ));
                        btTransform meshTransform(rotation, btVector3((float)blockX + 0.5f, (float)blockY + 0.5f, (float)blockZ) + Physics::Float3ToBulletVector(m_pChunk->GetBasePosition()));

                        // create a temporary callback
                        class MeshCallback : public btTriangleCallback
                        {
                            btTransform &meshTransform;
                            btTriangleCallback *callback;

                        public:
                            MeshCallback(btTransform &meshTransform, btTriangleCallback *callback) 
                                : meshTransform(meshTransform), callback(callback) {}

                            // these have to be here to shut up the stupid warnings, even deleting them still throws warnings..
                            // 1>BlockEngine\BlockWorldChunkCollisionShape.cpp(202): warning C4822: 'btBlockWorldChunkCollisionShape::processAllTriangles::MeshCallback::MeshCallback' : local class member function does not have a body
                            MeshCallback(const MeshCallback &m) : meshTransform(m.meshTransform), callback(m.callback) {}
                            MeshCallback &operator=(const MeshCallback &m) { meshTransform = m.meshTransform; callback = m.callback; return *this; }

                            virtual void processTriangle(btVector3* triangle, int partId, int triangleIndex)
                            {
                                btVector3 newTriangle[3];
                                newTriangle[0] = meshTransform(triangle[0]);
                                newTriangle[1] = meshTransform(triangle[1]);
                                newTriangle[2] = meshTransform(triangle[2]);
                                callback->processTriangle(newTriangle, partId, triangleIndex);
                            }
                        };
                        MeshCallback meshCallback(meshTransform, callback);

                        // invoke the mesh's collision shape process function
                        static_cast<const btConcaveShape *>(pStaticMesh->GetCollisionShape()->GetBulletShape())->processAllTriangles(&meshCallback, meshTransform.invXform(aabbMin), meshTransform.invXform(aabbMax));

                    }
                    else if (pBlockType->ShapeType != BLOCK_MESH_BLOCK_TYPE_SHAPE_TYPE_CUBE)
                    {
                        // can't handle yet
                        continue;
                    }
                }
                
                // is cube
                {
                    // generate the vertex positions for this block
                    float baseX = (float)blockX;
                    float baseY = (float)blockY;
                    float baseZ = (float)blockZ;

                    // generate vertices
                    btVector3 blockVertices[8] =
                    {
                        btVector3(baseX, baseY, baseZ),                             // bottom-front-left
                        btVector3(baseX + 1.0f, baseY, baseZ),                      // bottom-front-right
                        btVector3(baseX, baseY + 1.0f, baseZ),                      // bottom-back-left
                        btVector3(baseX + 1.0f, baseY + 1.0f, baseZ),               // bottom-back-right
                        btVector3(baseX, baseY, baseZ + blockHeight),               // top-front-left
                        btVector3(baseX + 1.0f, baseY, baseZ + blockHeight),        // top-front-right
                        btVector3(baseX, baseY + 1.0f, baseZ + blockHeight),        // top-back-left
                        btVector3(baseX + 1.0f, baseY + 1.0f, baseZ + blockHeight)  // top-back-right
                    };

                    btVector3 triangleVerticesA[3];
                    btVector3 triangleVerticesB[3];

                    // right face
                    triangleVerticesA[0] = blockVertices[5]; triangleVerticesA[1] = blockVertices[1]; triangleVerticesA[2] = blockVertices[3];
                    triangleVerticesB[0] = blockVertices[5]; triangleVerticesB[1] = blockVertices[3]; triangleVerticesB[2] = blockVertices[7];
                    callback->processTriangle(triangleVerticesA, CUBE_FACE_RIGHT, 0);
                    callback->processTriangle(triangleVerticesB, CUBE_FACE_RIGHT, 1);

                    // left face
                    triangleVerticesA[0] = blockVertices[4]; triangleVerticesA[1] = blockVertices[0]; triangleVerticesA[2] = blockVertices[2];
                    triangleVerticesB[0] = blockVertices[2]; triangleVerticesB[1] = blockVertices[4]; triangleVerticesB[2] = blockVertices[6];
                    callback->processTriangle(triangleVerticesA, CUBE_FACE_LEFT, 0);
                    callback->processTriangle(triangleVerticesB, CUBE_FACE_LEFT, 1);

                    // back face
                    triangleVerticesA[0] = blockVertices[6]; triangleVerticesA[1] = blockVertices[7]; triangleVerticesA[2] = blockVertices[2];
                    triangleVerticesB[0] = blockVertices[7]; triangleVerticesB[1] = blockVertices[3]; triangleVerticesB[2] = blockVertices[2];
                    callback->processTriangle(triangleVerticesA, CUBE_FACE_BACK, 0);
                    callback->processTriangle(triangleVerticesB, CUBE_FACE_BACK, 1);

                    // front face
                    triangleVerticesA[0] = blockVertices[0]; triangleVerticesA[1] = blockVertices[5]; triangleVerticesA[2] = blockVertices[4];
                    triangleVerticesB[0] = blockVertices[4]; triangleVerticesB[1] = blockVertices[1]; triangleVerticesB[2] = blockVertices[5];
                    callback->processTriangle(triangleVerticesA, CUBE_FACE_FRONT, 0);
                    callback->processTriangle(triangleVerticesB, CUBE_FACE_FRONT, 1);

                    // top face
                    triangleVerticesA[0] = blockVertices[6]; triangleVerticesA[1] = blockVertices[5]; triangleVerticesA[2] = blockVertices[7];
                    triangleVerticesB[0] = blockVertices[6]; triangleVerticesB[1] = blockVertices[4]; triangleVerticesB[2] = blockVertices[5];
                    callback->processTriangle(triangleVerticesA, CUBE_FACE_TOP, 0);
                    callback->processTriangle(triangleVerticesB, CUBE_FACE_TOP, 1);

                    // bottom face
                    triangleVerticesA[0] = blockVertices[0]; triangleVerticesA[1] = blockVertices[2]; triangleVerticesA[2] = blockVertices[3];
                    triangleVerticesB[0] = blockVertices[2]; triangleVerticesB[1] = blockVertices[3]; triangleVerticesB[2] = blockVertices[1];
                    callback->processTriangle(triangleVerticesA, CUBE_FACE_BOTTOM, 0);
                    callback->processTriangle(triangleVerticesB, CUBE_FACE_BOTTOM, 1);

                    /*
                    if (searchBounds.AABoxIntersection(AABox(float3(baseX, baseY, baseZ), float3(baseX + fBlockSizeInWorldUnits, baseY + fBlockSizeInWorldUnits, baseZ + fBlockSizeInWorldUnits))))
                        Log_DevPrintf("PASS! sect %i %i chunk %i %i %i block %u %u %u", pSection->GetSectionX(), pSection->GetSectionY(), pChunk->GetChunkX(), pChunk->GetChunkY(), pChunk->GetChunkZ(), blockX, blockY, blockZ);
                    else
                        Log_DevPrintf("FAIL! sect %i %i chunk %i %i %i block %u %u %u", pSection->GetSectionX(), pSection->GetSectionY(), pChunk->GetChunkX(), pChunk->GetChunkY(), pChunk->GetChunkZ(), blockX, blockY, blockZ);
                    */
                }
            }
        }
    }
}
