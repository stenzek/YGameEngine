#include "Core/PrecompiledHeader.h"
#include "Core/MeshUtilties.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/Assert.h"
#include "MathLib/SIMDVectorf.h"

namespace MeshUtilites
{
    void CalculateNormals(const void *pInVertices, uint32 uVertexStride, uint32 nVertices, const void *pInTriangles, uint32 uTriangleStride, uint32 nTriangles, void *pOutNormals, uint32 uNormalStride)
    {
        uint32 i, j;
        const byte *pInVerticesBytePtr = (const byte *)pInVertices;
        const byte *pInTrianglesBytePtr = (const byte *)pInTriangles;
        byte *pOutNormalsBytePtr = (byte *)pOutNormals;

        // initialize all normals to zero
        for (i = 0; i < nVertices; i++)
            reinterpret_cast<Vector3f *>(pOutNormalsBytePtr + i * uNormalStride)->SetZero();


        // get number of triangles
        uint32 nActualTriangles = (pInTriangles != NULL) ? nTriangles : (nVertices / 3);

        // calculate face normals and add them to the vertex normals
        for (i = 0; i < nActualTriangles; i++)
        {
            Vector3f faceVertices[3];
            Vector3f *pFaceNormals[3];

            if (pInTriangles != NULL)
            {
                // using indices
                for (j = 0; j < 3; j++)
                {
                    uint32 VertexIndex = *(uint32 *)(pInTrianglesBytePtr + i * uTriangleStride + j * sizeof(uint32));
                    faceVertices[j] = *reinterpret_cast<const Vector3f *>(pInVerticesBytePtr + VertexIndex * uVertexStride);
                    pFaceNormals[j] = reinterpret_cast<Vector3f *>(pOutNormalsBytePtr + VertexIndex * uNormalStride);
                }
            }
            else
            {
                // using vertices
                for (j = 0; j < 3; j++)
                {
                    faceVertices[j] = *reinterpret_cast<const Vector3f *>(pInVerticesBytePtr + (i * 3 + j) * uTriangleStride);
                    pFaceNormals[j] = reinterpret_cast<Vector3f *>(pOutNormalsBytePtr + (i * 3 + j) * uNormalStride);
                }
            }

//             // calc face normal
//             MLVECTOR3 V2MinusV1;
//             MLVECTOR3 V3MinusV1;
//             mlVector3Subtract(&V2MinusV1, pFaceVertices[1], pFaceVertices[0]);
//             mlVector3Subtract(&V3MinusV1, pFaceVertices[2], pFaceVertices[0]);
// 
//             MLVECTOR3 FaceNormal;
//             mlVector3Cross(&FaceNormal, &V2MinusV1, &V3MinusV1);
// 
//             // add it to the vertex normals
//             for (j = 0; j < 3; j++)
//                 mlVector3Add(pFaceNormals[j], pFaceNormals[j], &FaceNormal);

            Vector3f dv0 = faceVertices[1] - faceVertices[0];
            Vector3f dv1 = faceVertices[2] - faceVertices[0];
            Vector3f normal = dv0.Cross(dv1);

            for (j = 0; j < 3; j++)
                *pFaceNormals[j] = normal;
        }

        // normalize all the vertex normals
//         for (i = 0; i < nVertices; i++)
//         {
//             MLVECTOR3 *pNormal = (MLVECTOR3 *)(pOutNormalsBytePtr + i * uNormalStride);
//             if (mlVector3LengthSq(pNormal) < Y_FLT_EPSILON)
//                 continue;
// 
//             mlVector3Normalize(pNormal, pNormal);
//         }
    }

    Vector3f CalculateFaceNormal(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2)
    {
        Vector3f temp = (v1 - v0).Cross(v2 - v0);
        return temp.Normalize();
    }

    Plane CalculateTrianglePlane(const Vector3f &TriangleNormal, const Vector3f &FirstVertex)
    {
        Plane p;
        p.a = TriangleNormal.x;
        p.b = TriangleNormal.y;
        p.c = TriangleNormal.z;
        p.d = -(TriangleNormal.Dot(FirstVertex));
        p.Normalize();
        return p;
    }

    void CalculateTangentSpaceVectors(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector2f &uv0, const Vector2f &uv1, const Vector2f &uv2, Vector3f &OutTangent, Vector3f &OutBinormal, Vector3f &OutNormal)
    {
        SIMDVector3f V0(v0);
        SIMDVector3f dv0 = SIMDVector3f(v1) - V0;
        SIMDVector3f dv1 = SIMDVector3f(v2) - V0;
        
        SIMDVector2f UV0(uv0);
        SIMDVector2f dt0 = SIMDVector2f(uv1) - uv0;
        SIMDVector2f dt1 = SIMDVector2f(uv2) - uv0;

        float tmp = (dt0.x * dt1.y - dt1.x * dt0.y);
        float r = (tmp == 0.0f) ? 0.0f : 1.0f / tmp;

        SIMDVector3f tangent(SIMDVector3f(dt1.y * dv0.x - dt0.y * dv1.x, dt1.y * dv0.y - dt0.y * dv1.y, dt1.y * dv0.z - dt0.y * dv1.z) * r);
        SIMDVector3f binormal(SIMDVector3f(dt0.x * dv1.x - dt1.x * dv0.x, dt0.x * dv1.y - dt1.x * dv0.y, dt0.x * dv1.z - dt1.x * dv0.z) * r);
        SIMDVector3f normal(dv0.Cross(dv1));

        OutTangent = (tangent.SquaredLength() > 0.0f) ? tangent.Normalize() : tangent;
        OutBinormal = (binormal.SquaredLength() > 0.0f) ? binormal.Normalize() : binormal;
        OutNormal = (normal.SquaredLength() > 0.0f) ? normal.Normalize() : normal;
    }

    uint32 GenerateTriangleStripIndices(const void *pInVertices, uint32 uVertexStride, uint32 nVertices, void *pOutTriangles, uint32 uTriangleStride)
    {
        uint32 i, n;
        //const byte *pInVerticesBytePtr = (const byte *)pInVertices;
        const byte *pOutTrianglesBytePtr = (const byte *)pOutTriangles;
        
        // generate the actual indices
        DebugAssert(nVertices > 2);
        uint32 nTriangles = (nVertices - 2);
        bool Flip = true;
        uint32 *pTriIndices = (uint32 *)(pOutTrianglesBytePtr);

        // first triangle is generated outside the loop
        pTriIndices[0] = 0;
        pTriIndices[1] = 1;
        pTriIndices[2] = 2;
        n = 1;

        // generate the rest of the triangles
        for (i = 3; i < nVertices; i++)
        {
            pTriIndices = (uint32 *)(pOutTrianglesBytePtr + (uVertexStride * n));
            n++;

            // http://en.wikipedia.org/wiki/File:Triangle_Strip_In_OpenGL.gif
            if (Flip)
            {
                pTriIndices[0] = i - 1;
                pTriIndices[1] = i - 2;
                Flip = false;
            }
            else
            {
                pTriIndices[0] = i - 2;
                pTriIndices[1] = i - 1;
                Flip = true;
            }
            // and the last vertex
            pTriIndices[2] = i;
        }

        DebugAssert(n == nTriangles);
        return nTriangles;
    }

    void ReverseTriangleWinding(void *pInOutTriangles, uint32 uTriangleStride, uint32 nTriangles)
    {
        uint32 i;
        const byte *pInOutTrianglesBytePtr = (const byte *)pInOutTriangles;

        for (i = 0; i < nTriangles; i++)
        {
            uint32 *pTriIndices = (uint32 *)(pInOutTrianglesBytePtr + (i * uTriangleStride));

            uint32 tmp = pTriIndices[0];
            pTriIndices[0] = pTriIndices[2];
            pTriIndices[2] = tmp;
        }
    }

    void OptimizeIndicesForBatching(void *pInIndices, uint32 IndexStride, const void *pInMaterialIndices, uint32 MaterialIndexStride, uint32 nIndices)
    {
        uint32 i;
        int32 j;
        byte *pOutPtrIndices;
        byte *pOutPtrMaterials;
        int32 MinMaterialId;
        int32 MaxMaterialId;

        uint32 nTriangles = nIndices / 3;
        DebugAssert((nIndices % 3) == 0);

        uint32 *pSrcIndices = Y_mallocT<uint32>(nIndices);
        int32 *pSrcMaterials = Y_mallocT<int32>(nTriangles);
        Y_memcpy_stride(pSrcIndices, sizeof(uint32) * 3, pInIndices, IndexStride, sizeof(uint32) * 3, nTriangles);

        // initial pass to grab the maxes
        pOutPtrMaterials = (byte *)pInMaterialIndices;
        MinMaterialId = MaxMaterialId = pSrcMaterials[0] = *(uint32 *)pOutPtrMaterials;
        pOutPtrMaterials += MaterialIndexStride;
        for (i = 1; i < nTriangles; i++)
        {
            pSrcMaterials[i] = *(uint32 *)pOutPtrMaterials;
            pOutPtrMaterials += MaterialIndexStride;
            MinMaterialId = Min(MinMaterialId, pSrcMaterials[i]);
            MaxMaterialId = Max(MaxMaterialId, pSrcMaterials[i]);
        }

        // now loop through the material ids and add the indices
        uint32 NumOutputTriangles = 0;
        pOutPtrIndices = (byte *)pInIndices;
        pOutPtrMaterials = (byte *)pInMaterialIndices;
        for (j = MinMaterialId; j <= MaxMaterialId; j++)
        {
            for (i = 0; i < nTriangles; i++)
            {
                if (pSrcMaterials[i] == j)
                {
                    // this one goes in
                    ((uint32 *)pOutPtrIndices)[0] = pSrcIndices[i * 3 + 0];
                    ((uint32 *)pOutPtrIndices)[1] = pSrcIndices[i * 3 + 1];
                    ((uint32 *)pOutPtrIndices)[2] = pSrcIndices[i * 3 + 2];
                    *(uint32 *)pOutPtrMaterials = j;
                    pOutPtrIndices += IndexStride;
                    pOutPtrMaterials += MaterialIndexStride;
                    NumOutputTriangles++;
                }
            }
        }

        // should be equal
        DebugAssert(NumOutputTriangles == nTriangles);

        // free temp memory
        Y_free(pSrcMaterials);
        Y_free(pSrcIndices);
    }

    static void CreateSphereSubDivide(Vector3f *&pCurrentVertex, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, uint32 Level, const float &Scale)
    {
        if (Level > 0)
        {
            Level--;
            Vector3f v3 = (v0 + v1).Normalize();
            Vector3f v4 = (v1 + v2).Normalize();
            Vector3f v5 = (v2 + v0).Normalize();

            CreateSphereSubDivide(pCurrentVertex, v0, v3, v5, Level, Scale);
            CreateSphereSubDivide(pCurrentVertex, v3, v4, v5, Level, Scale);
            CreateSphereSubDivide(pCurrentVertex, v3, v1, v4, Level, Scale);
            CreateSphereSubDivide(pCurrentVertex, v5, v4, v2, Level, Scale);
        }
        else
        {
            *pCurrentVertex++ = v0 * Scale;
            *pCurrentVertex++ = v1 * Scale;
            *pCurrentVertex++ = v2 * Scale;
        }
    }

    void CreateSphere(Vector3f **ppVertices, uint32 *pNumVertices, uint32 SubDivLevel /* = 3 */, float Scale /* = 1.0f */)
    {
        uint32 nVertices = 8 * 3 * (1 << (2 * SubDivLevel));
        Vector3f *pVertices = new Vector3f[nVertices];

        // Tessellate a octahedron
        Vector3f px0(-1,  0,  0);
        Vector3f px1(1, 0, 0);
        Vector3f py0(0, -1, 0);
        Vector3f py1(0, 1, 0);
        Vector3f pz0(0, 0, -1);
        Vector3f pz1(0, 0, 1);

        Vector3f *pCurrentVertex = pVertices;
        CreateSphereSubDivide(pCurrentVertex, py0, px0, pz0, SubDivLevel, Scale);
        CreateSphereSubDivide(pCurrentVertex, py0, pz0, px1, SubDivLevel, Scale);
        CreateSphereSubDivide(pCurrentVertex, py0, px1, pz1, SubDivLevel, Scale);
        CreateSphereSubDivide(pCurrentVertex, py0, pz1, px0, SubDivLevel, Scale);
        CreateSphereSubDivide(pCurrentVertex, py1, pz0, px0, SubDivLevel, Scale);
        CreateSphereSubDivide(pCurrentVertex, py1, px0, pz1, SubDivLevel, Scale);
        CreateSphereSubDivide(pCurrentVertex, py1, pz1, px1, SubDivLevel, Scale);
        CreateSphereSubDivide(pCurrentVertex, py1, px1, pz0, SubDivLevel, Scale);

        DebugAssert((pCurrentVertex - pVertices) == (int32)nVertices);
        *ppVertices = pVertices;
        *pNumVertices = nVertices;
    }

    float InterpolateVector(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const float &c0, const float &c1, const float &c2, const Vector3f &p)
    {
        // determine barycentric coordinates of p
        float f01 = (v1.x - v0.x) * (p.y - v0.y) - (p.x - v0.x) * (v1.y - v0.y);
        float f12 = (v2.x - v1.x) * (p.y - v1.y) - (p.x - v1.x) * (v2.y - v1.y);
        float f20 = (v0.x - v2.x) * (p.y - v2.y) - (p.x - v2.x) * (v0.y - v2.y);
        float S = f01 + f12 + f20;

        float bca = f12 / S;
        float bcb = f20 / S;
        float bcy = f01 / S;

        return bca * c0 + bcb * c1 + bcy * c2;
    }

    bool PointInTriangle(const Vector3f &p, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &normal)
    {
#if 0
        float3 edge0(v1 - v0);
        float3 edge1(v2 - v0);
        float3 edge2(v0 - v2);
        float3 v0_to_p(p - v0);
        float3 v1_to_p(p - v1);
        float3 v2_to_p(p - v2);
        float3 edge0_normal(edge0.Cross(normal));
        float3 edge1_normal(edge1.Cross(normal));
        float3 edge2_normal(edge2.Cross(normal));

        float r0 = edge0_normal.Dot(v0_to_p);
        float r1 = edge1_normal.Dot(v1_to_p);
        float r2 = edge2_normal.Dot(v2_to_p);

        return (r0 > 0.0f && r1 > 0.0f && r2 > 0.0f) || (r0 <= 0.0f && r1 <= 0.0f && r2 <= 0.0f);
#else
        Vector3f u = v1 - v0;
        Vector3f v = v2 - v0;
        Vector3f w = p - v0;
        Vector3f vCrossW = v.Cross(w);
        Vector3f vCrossU = v.Cross(u);
        if (vCrossW.Dot(vCrossU) < 0.0f)
            return false;

        Vector3f uCrossW = u.Cross(w);
        Vector3f uCrossV = u.Cross(v);

        if (uCrossW.Dot(uCrossV) < 0.0f)
            return false;

        float denon = uCrossV.Length();
        float invDenom = 1.0f / denon;
        float r = vCrossW.Length() * invDenom;
        float t = uCrossW.Length() * invDenom;

        return (r <= 1.0f && t <= 1.0f && (r + t) <= 1.0f);
#endif
    }

    void OrthogonalizeTangent(const Vector3f &inTangent, const Vector3f &inBinormal, const Vector3f &inNormal, Vector3f &outTangent, float &outBinormalSign)
    {
        Vector3f tangent(inTangent);
        Vector3f binormal(inBinormal);
        Vector3f normal(inNormal);

        float NdotT = normal.Dot(tangent);
        Vector3f newTangent(tangent.x - NdotT * normal.x,
                          tangent.y - NdotT * normal.y,
                          tangent.z - NdotT * normal.z);

        float magT = newTangent.Length();
        newTangent.NormalizeInPlace();

        float NdotB = normal.Dot(binormal);
        float TdotB = newTangent.Dot(normal) * magT;

        Vector3f newBinormal(binormal.x - NdotB * normal.x - TdotB * newTangent.x,
                           binormal.y - NdotB * normal.y - TdotB * newTangent.y,
                           binormal.z - NdotB * normal.z - TdotB * newTangent.z);

        float magB = newBinormal.Length();
        newBinormal.NormalizeInPlace();

        if (magT <= 1e-6f || magB <= 1e-6f)
        {
            Vector3f axis1;
            Vector3f axis2;

            float dpXN = Math::Abs(Vector3f::UnitX.Dot(normal));
            float dpYN = Math::Abs(Vector3f::UnitY.Dot(normal));
            float dpZN = Math::Abs(Vector3f::UnitZ.Dot(normal));

            if (dpXN <= dpYN && dpXN <= dpZN)
            {
                axis1 = Vector3f::UnitX;
                axis2 = (dpYN <= dpZN) ? Vector3f::UnitY : Vector3f::UnitZ;
            }
            else if (dpYN <= dpXN && dpYN <= dpZN)
            {
                axis1 = Vector3f::UnitY;
                axis2 = (dpYN <= dpZN) ? Vector3f::UnitX : Vector3f::UnitZ;
            }
            else
            {
                axis1 = Vector3f::UnitZ;
                axis2 = (dpYN <= dpZN) ? Vector3f::UnitX : Vector3f::UnitY;
            }

            newTangent = axis1 - (normal * normal.Dot(axis1));
            newBinormal = axis2 - (normal * normal.Dot(axis2)) - (newTangent.SafeNormalize() * newTangent.Dot(axis2));

            newTangent.SafeNormalizeInPlace();
            newBinormal.SafeNormalizeInPlace();
        }

        float dp = normal.Cross(newTangent).Dot(newBinormal);
        outTangent = newTangent;
        outBinormalSign = (dp < 0.0f) ? -1.0f : 1.0f;
    }
}
