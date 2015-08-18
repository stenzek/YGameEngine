#pragma once
#include "ResourceCompiler/Common.h"
#include "Engine/Physics/CollisionShape.h"

class XMLReader;
class XMLWriter;
class BinaryBlob;

namespace Physics {

class CollisionShapeGenerator
{
public:
    CollisionShapeGenerator(COLLISION_SHAPE_TYPE type = COLLISION_SHAPE_TYPE_TRIANGLE_MESH);
    ~CollisionShapeGenerator();

    // Accessors
    const COLLISION_SHAPE_TYPE GetType() const { return m_type; }
    void SetType(COLLISION_SHAPE_TYPE type);

    // Box functions
    const float3 &GetBoxCenter() const { DebugAssert(m_type == COLLISION_SHAPE_TYPE_BOX); return m_boxCenter; }
    const float3 &GetBoxExtents() const { DebugAssert(m_type == COLLISION_SHAPE_TYPE_BOX); return m_boxHalfExtents; }
    void SetBoxCenter(const float3 &center) { DebugAssert(m_type == COLLISION_SHAPE_TYPE_BOX); m_boxCenter = center; }
    void SetBoxHalfExtents(const float3 &halfExtents) { DebugAssert(m_type == COLLISION_SHAPE_TYPE_BOX); m_boxHalfExtents = halfExtents; }

    // Sphere functions
    const float3 &GetSphereCenter() const { DebugAssert(m_type == COLLISION_SHAPE_TYPE_SPHERE); return m_sphereCenter; }
    const float GetSphereRadius() const { DebugAssert(m_type == COLLISION_SHAPE_TYPE_SPHERE); return m_sphereRadius; }
    void SetSphereCenter(const float3 &center) { DebugAssert(m_type == COLLISION_SHAPE_TYPE_SPHERE); m_sphereCenter = center; }
    void SetSphereRadius(const float radius) { DebugAssert(m_type == COLLISION_SHAPE_TYPE_SPHERE); m_sphereRadius = radius; }

    // Triangle mesh building functions
    const uint32 GetTriangleCount() const { DebugAssert(m_type == COLLISION_SHAPE_TYPE_TRIANGLE_MESH); return m_triangleMeshTriangles.GetSize(); }
    void AddTriangle(const float3 &v0, const float3 &v1, const float3 &v2);

    // Convex hull building functions
    const uint32 GetConvexHullVertexCount() const { DebugAssert(m_type == COLLISION_SHAPE_TYPE_CONVEX_HULL); return m_convexHullVertices.GetSize(); }
    void AddConvexHullVertex(const float3 &v);

    // Conversion functions
    // These functions all operate on a mesh that is currently a triangle mesh.
    void ConvertToBox();
    void ConvertToSphere();
    void ConvertToConvexHull();

    // Loading/saving to XML
    bool LoadFromXML(XMLReader &xmlReader);
    bool SaveToXML(XMLWriter &xmlWriter) const;

    // Compiling
    bool Compile(BinaryBlob **ppOutputBlob) const;

    // copying
    void Copy(const CollisionShapeGenerator *pGenerator);

    // applying a transform
    bool ApplyTransform(const Transform &transform);
    bool ApplyTransform(const float4x4 &transformMatrix);

private:
    struct TriangleMeshTriangle
    {
        float3 VertexPositions[3];
    };

    typedef MemArray<TriangleMeshTriangle> TriangleMeshTriangleArray;
    typedef MemArray<float3> ConvexHullVertexArray;

    COLLISION_SHAPE_TYPE m_type;

    // BOX
    float3 m_boxCenter;
    float3 m_boxHalfExtents;

    // SPHERE
    float3 m_sphereCenter;
    float m_sphereRadius;

    // TRIANGLE MESH
    TriangleMeshTriangleArray m_triangleMeshTriangles;

    // CONVEX HULL
    ConvexHullVertexArray m_convexHullVertices;

    DeclareNonCopyable(CollisionShapeGenerator);
};

};
