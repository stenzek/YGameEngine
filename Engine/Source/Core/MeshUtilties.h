#pragma once
#include "Core/Common.h"
#include "MathLib/Vectorf.h"
#include "MathLib/Matrixf.h"
#include "MathLib/Plane.h"

namespace MeshUtilites
{
    // Generate normal information for the specified vertices.
    // Vertices are assumed to be packed tightly and have three elements (x, y, z)
    // Normals are packed the same.
    // Indices are assumed to be 32-bit integers.
    // If indices is NULL, it is assumed that it is a non-indexed mesh.
    void CalculateNormals(const void *pInVertices, uint32 uVertexStride, uint32 nVertices, const void *pInTriangles, uint32 uTriangleStride, uint32 nTriangles, void *pOutNormals, uint32 uNormalStride);

    // Calculate the normal of a triangle face.
    Vector3f CalculateFaceNormal(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2);

    // Calculate the plane of a triangle.
    Plane CalculateTrianglePlane(const Vector3f &p0, const Vector3f &p1, const Vector3f &p2);
    Plane CalculateTrianglePlane(const Vector3f &TriangleNormal, const Vector3f &FirstVertex);

    // Calculate tangent space vectors for specified vertices and texture coordinates.
    // Resultant vectors are not guaranteed to be unit length.
    void CalculateTangentSpaceVectors(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector2f &uv0, const Vector2f &uv1, const Vector2f &uv2, Vector3f &OutTangent, Vector3f &OutBinormal, Vector3f &OutNormal);

    // Generate indices for a triangle-strip based mesh.
    // pOutIndices should be equal to nVertices - 2, and 32-bit integer.
    // Returns the number of triangles (indices / 3) for the mesh.
    uint32 GenerateTriangleStripIndices(const void *pInVertices, uint32 uVertexStride, uint32 nVertices, void *pOutTriangles, uint32 uTriangleStride);

    // Reverse the winding order for the specified set of indices.
    void ReverseTriangleWinding(void *pInOutTriangles, uint32 uTriangleStride, uint32 nTriangles);

    // Optimize the specified indices so that all the indices that use the same material are grouped together.
    void OptimizeIndicesForBatching(void *pInIndices, uint32 IndexStride, const void *pInMaterialIndices, uint32 MaterialIndexStride, uint32 nIndices);

    // Creates a sphere. The returned memory should be freed with Y_free.
    void CreateSphere(Vector3f **ppVertices, uint32 *pNumVertices, uint32 SubDivLevel = 3, float Scale = 1.0f);

    float InterpolateVector(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const float &c0, const float &c1, const float &c2, const Vector3f &p);

    bool PointInTriangle(const Vector3f &p, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &normal);

    void OrthogonalizeTangent(const Vector3f &inTangent, const Vector3f &inBinormal, const Vector3f &inNormal, Vector3f &outTangent, float &outBinormalSign);
}

