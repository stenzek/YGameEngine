#include "MathLib/Frustum.h"
#include "MathLib/SIMDVectorf.h"

Frustum::Frustum(const Frustum &frCopy)
{
    uint32 i;
    for (i = 0; i < FRUSTUM_PLANE_COUNT; i++)
        m_planes[i] = frCopy.m_planes[i];
}

void Frustum::SetFromMatrix(const Matrix4x4f &matViewProj)
{
    m_planes[FRUSTUM_PLANE_LEFT].a = matViewProj[3][0] + matViewProj[0][0];
    m_planes[FRUSTUM_PLANE_LEFT].b = matViewProj[3][1] + matViewProj[0][1];
    m_planes[FRUSTUM_PLANE_LEFT].c = matViewProj[3][2] + matViewProj[0][2];
    m_planes[FRUSTUM_PLANE_LEFT].d = matViewProj[3][3] + matViewProj[0][3];
    m_planes[FRUSTUM_PLANE_LEFT].NormalizeInPlace();

    m_planes[FRUSTUM_PLANE_RIGHT].a = matViewProj[3][0] - matViewProj[0][0];
    m_planes[FRUSTUM_PLANE_RIGHT].b = matViewProj[3][1] - matViewProj[0][1];
    m_planes[FRUSTUM_PLANE_RIGHT].c = matViewProj[3][2] - matViewProj[0][2];
    m_planes[FRUSTUM_PLANE_RIGHT].d = matViewProj[3][3] - matViewProj[0][3];
    m_planes[FRUSTUM_PLANE_RIGHT].NormalizeInPlace();

    m_planes[FRUSTUM_PLANE_TOP].a = matViewProj[3][0] - matViewProj[1][0];
    m_planes[FRUSTUM_PLANE_TOP].b = matViewProj[3][1] - matViewProj[1][1];
    m_planes[FRUSTUM_PLANE_TOP].c = matViewProj[3][2] - matViewProj[1][2];
    m_planes[FRUSTUM_PLANE_TOP].d = matViewProj[3][3] - matViewProj[1][3];
    m_planes[FRUSTUM_PLANE_TOP].NormalizeInPlace();

    m_planes[FRUSTUM_PLANE_BOTTOM].a = matViewProj[3][0] + matViewProj[1][0];
    m_planes[FRUSTUM_PLANE_BOTTOM].b = matViewProj[3][1] + matViewProj[1][1];
    m_planes[FRUSTUM_PLANE_BOTTOM].c = matViewProj[3][2] + matViewProj[1][2];
    m_planes[FRUSTUM_PLANE_BOTTOM].d = matViewProj[3][3] + matViewProj[1][3];
    m_planes[FRUSTUM_PLANE_BOTTOM].NormalizeInPlace();

    m_planes[FRUSTUM_PLANE_NEAR].a = matViewProj[3][0] + matViewProj[2][0];
    m_planes[FRUSTUM_PLANE_NEAR].b = matViewProj[3][1] + matViewProj[2][1];
    m_planes[FRUSTUM_PLANE_NEAR].c = matViewProj[3][2] + matViewProj[2][2];
    m_planes[FRUSTUM_PLANE_NEAR].d = matViewProj[3][3] + matViewProj[2][3];
    m_planes[FRUSTUM_PLANE_NEAR].NormalizeInPlace();

    m_planes[FRUSTUM_PLANE_FAR].a = matViewProj[3][0] - matViewProj[2][0];
    m_planes[FRUSTUM_PLANE_FAR].b = matViewProj[3][1] - matViewProj[2][1];
    m_planes[FRUSTUM_PLANE_FAR].c = matViewProj[3][2] - matViewProj[2][2];
    m_planes[FRUSTUM_PLANE_FAR].d = matViewProj[3][3] - matViewProj[2][3];
    m_planes[FRUSTUM_PLANE_FAR].NormalizeInPlace();
}

void Frustum::SetFromAABox(const AABox &aabBox)
{
    const Vector3f &Low = aabBox.GetMinBounds();
    const Vector3f &High = aabBox.GetMaxBounds();

    m_planes[FRUSTUM_PLANE_LEFT].a = -1.0f;
    m_planes[FRUSTUM_PLANE_LEFT].b = 0.0f;
    m_planes[FRUSTUM_PLANE_LEFT].c = 0.0f;
    m_planes[FRUSTUM_PLANE_LEFT].d = -Low.x;

    m_planes[FRUSTUM_PLANE_RIGHT].a = 1.0f;
    m_planes[FRUSTUM_PLANE_RIGHT].b = 0.0f;
    m_planes[FRUSTUM_PLANE_RIGHT].c = 0.0f;
    m_planes[FRUSTUM_PLANE_RIGHT].d = High.x;

    m_planes[FRUSTUM_PLANE_BOTTOM].a = 0.0f;
    m_planes[FRUSTUM_PLANE_BOTTOM].b = -1.0f;
    m_planes[FRUSTUM_PLANE_BOTTOM].c = 0.0f;
    m_planes[FRUSTUM_PLANE_BOTTOM].d = -Low.y;

    m_planes[FRUSTUM_PLANE_TOP].a = 0.0f;
    m_planes[FRUSTUM_PLANE_TOP].b = 1.0f;
    m_planes[FRUSTUM_PLANE_TOP].c = 0.0f;
    m_planes[FRUSTUM_PLANE_TOP].d = High.y;

    m_planes[FRUSTUM_PLANE_FAR].a = 0.0f;
    m_planes[FRUSTUM_PLANE_FAR].b = 0.0f;
    m_planes[FRUSTUM_PLANE_FAR].c = -1.0f;
    m_planes[FRUSTUM_PLANE_FAR].d = -Low.z;

    m_planes[FRUSTUM_PLANE_NEAR].a = 0.0f;
    m_planes[FRUSTUM_PLANE_NEAR].b = 0.0f;
    m_planes[FRUSTUM_PLANE_NEAR].c = 1.0f;
    m_planes[FRUSTUM_PLANE_NEAR].d = High.z;
}

bool Frustum::AABoxIntersection(const AABox &aabBounds) const
{
    SIMDVector3f cornerVertices[8];
    aabBounds.GetCornerPoints(cornerVertices);

    // check if the points fit in the frustum. if no points fit, toss it out
    uint32 i, j;
    for (i = 0; i < FRUSTUM_PLANE_COUNT; i++)
    {
        SIMDVector3f planeNormal(m_planes[i].GetNormal());
        float planeDistance = m_planes[i].GetDistance();
        uint32 pointsIn = 8;

        for (j = 0; j < 8; j++)
        {
            float hitDistance = planeNormal.Dot(cornerVertices[j]) + planeDistance;
            if (hitDistance < 0)
                pointsIn--;
        }

        if (pointsIn == 0)
            return false;
    }

    return true;
}

bool Frustum::SphereIntersection(const Sphere &sphere) const
{
    SIMDVector3f sphereCenter(sphere.GetCenter());
    float sphereRadius = sphere.GetRadius();
    float negSphereRadius = -sphere.GetRadius();

    for (uint32 i = 0; i < FRUSTUM_PLANE_COUNT; i++)
    {
        float hitDistance = SIMDVector3f(m_planes[i].GetNormal()).Dot(sphereCenter) + m_planes[i].GetDistance();

        // Outside
        if (hitDistance < negSphereRadius)
            return false;

        // Intersection
        if (Math::Abs(hitDistance) < sphereRadius)
            return true;
    }

    return true;
}

Frustum::IntersectionType Frustum::AABoxIntersectionType(const AABox &box) const
{
    SIMDVector3f cornerVertices[8];
    box.GetCornerPoints(cornerVertices);

    // check if the points fit in the frustum. if no points fit, toss it out
    IntersectionType intersectionType = INTERSECTION_TYPE_INSIDE;
    for (uint32 i = 0; i < FRUSTUM_PLANE_COUNT; i++)
    {
        SIMDVector3f planeNormal(m_planes[i].GetNormal());
        float planeDistance = m_planes[i].GetDistance();
        uint32 pointsIn = 8;

        for (uint32 j = 0; j < 8; j++)
        {
            float hitDistance = planeNormal.Dot(cornerVertices[j]) + planeDistance;
            if (hitDistance < 0)
                pointsIn--;
        }

        if (pointsIn == 0)
            return INTERSECTION_TYPE_OUTSIDE;
        else if (pointsIn != 8)
            intersectionType = INTERSECTION_TYPE_INTERSECTS;
    }

    return intersectionType;
}

Frustum::IntersectionType Frustum::SphereIntersectionType(const Sphere &sphere) const
{
    SIMDVector3f sphereCenter(sphere.GetCenter());
    float sphereRadius = sphere.GetRadius();
    float negSphereRadius = -sphereRadius;

    for (uint32 i = 0; i < FRUSTUM_PLANE_COUNT; i++)
    {
        float hitDistance = SIMDVector3f(m_planes[i].GetNormal()).Dot(sphereCenter) + m_planes[i].GetDistance();
        
        // if distance < -sphereRadius, outside
        if (hitDistance < negSphereRadius)
            return INTERSECTION_TYPE_OUTSIDE;

        // if abs(distance) < sphereRadius, intersect
        if (Math::Abs(hitDistance) < sphereRadius)
            return INTERSECTION_TYPE_INTERSECTS;
    }

    return INTERSECTION_TYPE_INSIDE;
}

bool Frustum::operator==(const Frustum &Comp) const
{
    return m_planes[FRUSTUM_PLANE_LEFT] == Comp.m_planes[FRUSTUM_PLANE_LEFT] &&
           m_planes[FRUSTUM_PLANE_RIGHT] == Comp.m_planes[FRUSTUM_PLANE_RIGHT] &&
           m_planes[FRUSTUM_PLANE_NEAR] == Comp.m_planes[FRUSTUM_PLANE_NEAR] &&
           m_planes[FRUSTUM_PLANE_FAR] == Comp.m_planes[FRUSTUM_PLANE_FAR] &&
           m_planes[FRUSTUM_PLANE_TOP] == Comp.m_planes[FRUSTUM_PLANE_TOP] &&
           m_planes[FRUSTUM_PLANE_BOTTOM] == Comp.m_planes[FRUSTUM_PLANE_BOTTOM];
}

bool Frustum::operator!=(const Frustum &Comp) const
{
    return m_planes[FRUSTUM_PLANE_LEFT] != Comp.m_planes[FRUSTUM_PLANE_LEFT] ||
           m_planes[FRUSTUM_PLANE_RIGHT] != Comp.m_planes[FRUSTUM_PLANE_RIGHT] ||
           m_planes[FRUSTUM_PLANE_NEAR] != Comp.m_planes[FRUSTUM_PLANE_NEAR] ||
           m_planes[FRUSTUM_PLANE_FAR] != Comp.m_planes[FRUSTUM_PLANE_FAR] ||
           m_planes[FRUSTUM_PLANE_TOP] != Comp.m_planes[FRUSTUM_PLANE_TOP] ||
           m_planes[FRUSTUM_PLANE_BOTTOM] != Comp.m_planes[FRUSTUM_PLANE_BOTTOM];
}

Frustum &Frustum::operator=(const Frustum &Copy)
{
    uint32 i;
    for (i = 0; i < FRUSTUM_PLANE_COUNT; i++)
        m_planes[i] = Copy.m_planes[i];
    
    return *this;
}

void Frustum::GetCornerVertices(Vector3f pVertices[8]) const
{
    SIMDVector3f out[8];
    GetCornerVertices(out);

    for (uint32 i = 0; i < 8; i++)
        pVertices[i] = out[i];
}

void Frustum::GetCornerVertices(SIMDVector3f pVertices[8]) const
{
#if 0
    Matrix4 inverseViewProjectionMatrix(m_ViewProjectionMatrix);
    inverseViewProjectionMatrix.InvertInPlace();

    static const float unitCubeVertices[8][4] =
    {
        { -1.0f, -1.0f, 0.0f, 1.0f },
        { 1.0f, -1.0f, 0.0f, 1.0f },
        { -1.0f, 1.0f, 0.0f, 1.0f },
        { 1.0f, 1.0f, 0.0f, 1.0f },
        { -1.0f, -1.0f, 1.0f, 1.0f },
        { 1.0f, -1.0f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f, 1.0f },
    };

    for (uint32 i = 0; i < 8; i++)
    {
        Vector4 transformed = (inverseViewProjectionMatrix * Vector4(unitCubeVertices[i]));
        pVertices[i] = transformed.xyz() / transformed.w;
    }
#else
    
    pVertices[0] = Plane::CalculateThreePlaneIntersectionPoint(m_planes[FRUSTUM_PLANE_NEAR], m_planes[FRUSTUM_PLANE_BOTTOM], m_planes[FRUSTUM_PLANE_RIGHT]);
    pVertices[1] = Plane::CalculateThreePlaneIntersectionPoint(m_planes[FRUSTUM_PLANE_NEAR], m_planes[FRUSTUM_PLANE_TOP], m_planes[FRUSTUM_PLANE_RIGHT]);
    pVertices[2] = Plane::CalculateThreePlaneIntersectionPoint(m_planes[FRUSTUM_PLANE_NEAR], m_planes[FRUSTUM_PLANE_TOP], m_planes[FRUSTUM_PLANE_LEFT]);
    pVertices[3] = Plane::CalculateThreePlaneIntersectionPoint(m_planes[FRUSTUM_PLANE_NEAR], m_planes[FRUSTUM_PLANE_BOTTOM], m_planes[FRUSTUM_PLANE_LEFT]);
    pVertices[4] = Plane::CalculateThreePlaneIntersectionPoint(m_planes[FRUSTUM_PLANE_FAR], m_planes[FRUSTUM_PLANE_BOTTOM], m_planes[FRUSTUM_PLANE_RIGHT]);
    pVertices[5] = Plane::CalculateThreePlaneIntersectionPoint(m_planes[FRUSTUM_PLANE_FAR], m_planes[FRUSTUM_PLANE_TOP], m_planes[FRUSTUM_PLANE_RIGHT]);
    pVertices[6] = Plane::CalculateThreePlaneIntersectionPoint(m_planes[FRUSTUM_PLANE_FAR], m_planes[FRUSTUM_PLANE_TOP], m_planes[FRUSTUM_PLANE_LEFT]);
    pVertices[7] = Plane::CalculateThreePlaneIntersectionPoint(m_planes[FRUSTUM_PLANE_FAR], m_planes[FRUSTUM_PLANE_BOTTOM], m_planes[FRUSTUM_PLANE_LEFT]);

#endif
}

AABox Frustum::GetBoundingAABox() const
{
    SIMDVector3f cornerVertices[8];
    GetCornerVertices(cornerVertices);
    return AABox::FromPoints(cornerVertices, countof(cornerVertices));
}

Sphere Frustum::GetBoundingSphere() const
{
    SIMDVector3f cornerVertices[8];
    GetCornerVertices(cornerVertices);
    return Sphere::FromPoints(cornerVertices, countof(cornerVertices));
}

