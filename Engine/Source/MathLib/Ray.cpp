#include "MathLib/Ray.h"
#include "MathLib/Plane.h"
#include "MathLib/AABox.h"
#include "MathLib/Sphere.h"
#include "MathLib/CollisionDetection.h"
#include "MathLib/SIMDVectorf.h"
#include "YBaseLib/Assert.h"

Ray::Ray(const Ray &copyRay)
    : m_Origin(copyRay.m_Origin), 
      m_End(copyRay.m_End),
      m_Direction(copyRay.m_Direction), 
      m_InverseDirection(copyRay.m_InverseDirection),
      m_Distance(copyRay.m_Distance)
{

}

Ray::Ray(const Vector3f &orgin, const Vector3f &direction, float maxDistance)
{
    SIMDVector3f vec_direction(direction);

    m_Origin = orgin;
    m_End = SIMDVector3f(orgin) + vec_direction * maxDistance;
    m_Direction = direction;
    m_InverseDirection = SIMDVector3f::One / vec_direction;
    m_Distance = maxDistance;
}

Ray::Ray(const Vector3f &start, const Vector3f &end)
{
    m_Origin = start;
    m_End = end;

    SIMDVector3f rayVector(SIMDVector3f(end) - SIMDVector3f(start));
    DebugAssert(rayVector.SquaredLength() > 0.0f);

    m_Direction = rayVector.Normalize();
    m_InverseDirection = SIMDVector3f::One / rayVector;
    m_Distance = rayVector.Length();
}

AABox Ray::GetAABox() const
{
    SIMDVector3f vec_origin(m_Origin);
    SIMDVector3f vec_end(m_End);

    return AABox(vec_origin.Min(vec_end), vec_origin.Max(vec_end));
}

bool Ray::AABoxIntersection(const AABox &aaBox) const
{
    return CollisionDetection::RayIntersectsAABox(m_Origin, m_Direction, m_Distance, aaBox.GetMinBounds(), aaBox.GetMaxBounds());
}

bool Ray::AABoxIntersection(const AABox &aaBox, Vector3f &contactNormal, Vector3f &contactPoint) const
{
    return CollisionDetection::RayIntersectsAABox(m_Origin, m_Direction, m_Distance, aaBox.GetMinBounds(), aaBox.GetMaxBounds(), contactNormal, contactPoint);
}

bool Ray::AABoxIntersection(const Vector3f &minBounds, const Vector3f &maxBounds) const
{
    return CollisionDetection::RayIntersectsAABox(m_Origin, m_Direction, m_Distance, minBounds, maxBounds);
}

bool Ray::AABoxIntersection(const Vector3f &minBounds, const Vector3f &maxBounds, Vector3f &contactNormal, Vector3f &contactPoint) const
{
    return CollisionDetection::RayIntersectsAABox(m_Origin, m_Direction, m_Distance, minBounds, maxBounds, contactNormal, contactPoint);
}

float Ray::AABoxIntersectionTime(const AABox &aaBox) const
{
    Vector3f contactNormal, contactPoint;
    if (CollisionDetection::RayIntersectsAABox(m_Origin, m_Direction, m_Distance, aaBox.GetMinBounds(), aaBox.GetMaxBounds(), contactNormal, contactPoint))
        return (contactPoint - m_Origin).Length();
    else
        return Y_FLT_INFINITE;
}

float Ray::AABoxIntersectionTime(const Vector3f &minBounds, const Vector3f &maxBounds) const
{
    Vector3f contactNormal, contactPoint;
    if (CollisionDetection::RayIntersectsAABox(m_Origin, m_Direction, m_Distance, minBounds, maxBounds, contactNormal, contactPoint))
        return (contactPoint - m_Origin).Length();
    else
        return Y_FLT_INFINITE;
}

bool Ray::AABoxIntersectionTimeFace(const AABox &aaBox, float *pContactTime, CUBE_FACE *pContactFace) const
{
    return CollisionDetection::RayIntersectsAABox(m_Origin, m_InverseDirection, m_Distance, aaBox.GetMinBounds(), aaBox.GetMaxBounds(), pContactTime, pContactFace);
}

bool Ray::AABoxIntersectionTimeFace(const Vector3f &minBounds, const Vector3f &maxBounds, float *pContactTime, CUBE_FACE *pContactFace) const
{
    return CollisionDetection::RayIntersectsAABox(m_Origin, m_InverseDirection, m_Distance, minBounds, maxBounds, pContactTime, pContactFace);
}

bool Ray::PlaneIntersection(const Plane &intersectPlane)
{
    return CollisionDetection::RayIntersectsPlane(m_Origin, m_Direction, m_Distance, intersectPlane.GetNormal(), intersectPlane.GetDistance());
}

bool Ray::PlaneIntersection(const Plane &intersectPlane, Vector3f &contactNormal, Vector3f &contactPoint)
{
    return CollisionDetection::RayIntersectsPlane(m_Origin, m_Direction, m_Distance, intersectPlane.GetNormal(), intersectPlane.GetDistance(), contactNormal, contactPoint);
}

float Ray::PlaneIntersectionTime(const Plane &intersectPlane)
{
    Vector3f contactNormal, contactPoint;
    if (CollisionDetection::RayIntersectsPlane(m_Origin, m_Direction, m_Distance, intersectPlane.GetNormal(), intersectPlane.GetDistance(), contactNormal, contactPoint))
        return (contactPoint - m_Origin).Length();
    else
        return Y_FLT_INFINITE;
}

bool Ray::TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2) const
{
    SIMDVector3f vec_v0(v0);
    SIMDVector3f vec_e0 = SIMDVector3f(v1) - vec_v0;
    SIMDVector3f vec_e1 = SIMDVector3f(v2) - vec_v0;

    return CollisionDetection::RayIntersectsTriangle(m_Origin, m_Direction, m_Distance, v0, v1, v2, vec_e0, vec_e1);
}

bool Ray::TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, Vector3f &contactNormal, Vector3f &contactPoint) const
{
    SIMDVector3f vec_v0(v0);
    SIMDVector3f vec_e0 = SIMDVector3f(v1) - vec_v0;
    SIMDVector3f vec_e1 = SIMDVector3f(v2) - vec_v0;

    return CollisionDetection::RayIntersectsTriangle(m_Origin, m_Direction, m_Distance, v0, v1, v2, vec_e0, vec_e1, contactNormal, contactPoint);
}

bool Ray::TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1) const
{
    return CollisionDetection::RayIntersectsTriangle(m_Origin, m_Direction, m_Distance, v0, v1, v2, e0, e1);
}

bool Ray::TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1, Vector3f &contactNormal, Vector3f &contactPoint) const
{
    return CollisionDetection::RayIntersectsTriangle(m_Origin, m_Direction, m_Distance, v0, v1, v2, e0, e1, contactNormal, contactPoint);
}

float Ray::TriangleIntersectionTime(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2)
{
    SIMDVector3f vec_v0(v0);
    SIMDVector3f vec_e0 = SIMDVector3f(v1) - vec_v0;
    SIMDVector3f vec_e1 = SIMDVector3f(v2) - vec_v0;

    Vector3f contactNormal, contactPoint;
    if (CollisionDetection::RayIntersectsTriangle(m_Origin, m_Direction, m_Distance, v0, v1, v2, vec_e0, vec_e1, contactNormal, contactPoint))
        return (contactPoint - m_Origin).Length();
    else
        return Y_FLT_INFINITE;
}

float Ray::TriangleIntersectionTime(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1)
{
    Vector3f contactNormal, contactPoint;
    if (CollisionDetection::RayIntersectsTriangle(m_Origin, m_Direction, m_Distance, v0, v1, v2, e0, e1, contactNormal, contactPoint))
        return (contactPoint - m_Origin).Length();
    else
        return Y_FLT_INFINITE;
}

bool Ray::SphereIntersection(const Sphere &sphere)
{
    return CollisionDetection::RayIntersectsSphere(m_Origin, m_Direction, m_Distance, sphere.GetCenter(), sphere.GetRadius());
}

bool Ray::SphereIntersection(const Sphere &sphere, Vector3f &contactNormal, Vector3f &contactPoint)
{
    return CollisionDetection::RayIntersectsSphere(m_Origin, m_Direction, m_Distance, sphere.GetCenter(), sphere.GetRadius(), contactNormal, contactPoint);
}

float Ray::SphereIntersectionTime(const Sphere &sphere)
{
    Vector3f contactNormal, contactPoint;
    if (CollisionDetection::RayIntersectsSphere(m_Origin, m_Direction, m_Distance, sphere.GetCenter(), sphere.GetRadius(), contactNormal, contactPoint))
        return (contactPoint - m_Origin).Length();
    else
        return Y_FLT_INFINITE;
}
