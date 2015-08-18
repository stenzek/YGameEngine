#pragma once
#include "Engine/Common.h"

class btCollisionShape;
class btTransform;

namespace Physics {

enum COLLISION_SHAPE_TYPE
{
    COLLISION_SHAPE_TYPE_NONE,
    COLLISION_SHAPE_TYPE_BOX,
    COLLISION_SHAPE_TYPE_SPHERE,
    COLLISION_SHAPE_TYPE_TRIANGLE_MESH,
    COLLISION_SHAPE_TYPE_SCALED_TRIANGLE_MESH,
    COLLISION_SHAPE_TYPE_CONVEX_HULL,
    COLLISION_SHAPE_TYPE_TERRAIN,
    COLLISION_SHAPE_TYPE_BLOCK_MESH,
    COLLISION_SHAPE_TYPE_BLOCK_TERRAIN,
    COLLISION_SHAPE_TYPE_CUSTOM,
    COLLISION_SHAPE_TYPE_COUNT,
};

namespace NameTables {
    Y_Declare_NameTable(CollisionShapeType);
}

class CollisionShape : public ReferenceCounted
{
public:
    CollisionShape() {}
    virtual ~CollisionShape() {}

    // Gets type of collision shape.
    virtual const COLLISION_SHAPE_TYPE GetType() const = 0;

    // Bounds in local space
    virtual const AABox &GetLocalBoundingBox() const = 0;

    // Loading the shape from precompiled data.
    virtual bool LoadFromData(const void *pData, uint32 dataSize) = 0;
    virtual bool LoadFromStream(ByteStream *pStream, uint32 dataSize) = 0;

    // Creates a scaled version of this collision shape, sharing as much data as possible.
    virtual CollisionShape *CreateScaledShape(const float3 &scale) const = 0;

    // Applies any local transforms contained in the shape itself, before passing it to bullet.
    virtual void ApplyShapeTransform(btTransform &worldTransform) const = 0;
    virtual void ApplyInverseShapeTransform(btTransform &worldTransform) const = 0;

    // Bullet object handle
    virtual btCollisionShape *GetBulletShape() const = 0;

public:
    // Creates a collision shape from precompiled data.
    static CollisionShape *CreateFromData(const void *pData, uint32 dataSize);
    static CollisionShape *CreateFromStream(ByteStream *pStream, uint32 dataSize);
};

}       // namespace Physics
