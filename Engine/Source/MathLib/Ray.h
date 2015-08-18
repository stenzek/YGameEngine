#pragma once
#include "MathLib/Common.h"
#include "MathLib/Vectorf.h"

class AABox;
class AABoxi;
class Plane;
class Sphere;

class Ray
{
public:
    Ray() {}
    Ray(const Ray &copyRay);
    Ray(const Vector3f &orgin, const Vector3f &direction, float maxDistance);
    Ray(const Vector3f &start, const Vector3f &end);

    const Vector3f &GetOrigin() const { return m_Origin; }
    const Vector3f &GetEnd() const { return m_End; }
    const Vector3f &GetDirection() const { return m_Direction; }
    const Vector3f &GetInverseDirection() const { return m_InverseDirection; }
    const float GetDistance() const { return m_Distance; }

    //void SetOrigin(const float3 &newOrigin) { m_Origin = newOrigin; }
    //void SetDirection(const float3 &newDirection) { m_Direction = newDirection; m_InverseDirection = float3::One / newDirection; }

    AABox GetAABox() const;

    bool AABoxIntersection(const AABox &aaBox) const;
    bool AABoxIntersection(const AABox &aaBox, Vector3f &contactNormal, Vector3f &contactPoint) const;
    bool AABoxIntersection(const Vector3f &minBounds, const Vector3f &maxBounds) const;
    bool AABoxIntersection(const Vector3f &minBounds, const Vector3f &maxBounds, Vector3f &contactNormal, Vector3f &contactPoint) const;
    float AABoxIntersectionTime(const AABox &aaBox) const;
    float AABoxIntersectionTime(const Vector3f &minBounds, const Vector3f &maxBounds) const;
    bool AABoxIntersectionTimeFace(const AABox &aaBox, float *pContactTime, CUBE_FACE *pContactFace) const;
    bool AABoxIntersectionTimeFace(const Vector3f &minBounds, const Vector3f &maxBounds, float *pContactTime, CUBE_FACE *pContactFace) const;

    bool PlaneIntersection(const Plane &intersectPlane);
    bool PlaneIntersection(const Plane &intersectPlane, Vector3f &contactNormal, Vector3f &contactPoint);
    float PlaneIntersectionTime(const Plane &intersectPlane);

    bool TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2) const;
    bool TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, Vector3f &contactNormal, Vector3f &contactPoint) const;
    bool TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1) const;
    bool TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1, Vector3f &contactNormal, Vector3f &contactPoint) const;
    float TriangleIntersectionTime(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2);
    float TriangleIntersectionTime(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1);
    
    bool SphereIntersection(const Sphere &sphere);
    bool SphereIntersection(const Sphere &sphere, Vector3f &contactNormal, Vector3f &contactPoint);
    float SphereIntersectionTime(const Sphere &sphere);

private:
    Vector3f m_Origin;
    Vector3f m_End;
    Vector3f m_Direction;
    Vector3f m_InverseDirection;
    float m_Distance;
};


