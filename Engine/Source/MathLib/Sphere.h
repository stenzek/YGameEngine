#pragma once
#include "MathLib/Common.h"
#include "MathLib/Vectorf.h"

class AABox;
class Transform;
struct Matrix3x3f;
struct Matrix3x4f;
struct Matrix4x4f;

class Sphere
{
public:
    Sphere() {}
    Sphere(const Vector3f &centre, float radius);
    Sphere(const Sphere &copy);

    void SetCenter(const Vector3f &centre) { m_center = centre; }
    void SetRadius(float radius) { m_radius = radius; }
    void SetCenterAndRadius(const Vector3f &centre, float radius) { m_center = centre; m_radius = radius; }

    const Vector3f &GetCenter() const { return m_center; }
    float GetRadius() const { return m_radius; }

    void Merge(const Vector3f &point);
    void Merge(const Sphere &sphere);    

    bool AABoxIntersection(const AABox &box) const;
    bool AABoxIntersection(const AABox &box, Vector3f &contactNormal, Vector3f &contactPoint) const;
    bool AABoxIntersection(const Vector3f &minBounds, const Vector3f &maxBounds) const;
    bool AABoxIntersection(const Vector3f &minBounds, const Vector3f &maxBounds, Vector3f &contactNormal, Vector3f &contactPoint) const;

    bool SphereIntersection(const Sphere &sphere) const;
    bool SphereIntersection(const Sphere &sphere, Vector3f &contactNormal, Vector3f &contactPoint) const;

    bool TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2) const;
    bool TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, Vector3f &contactNormal, Vector3f &contactPoint) const;
    float TriangleIntersectionTime(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2) const;

    // transform
    void ApplyTransform(const Transform &transform);
    void ApplyTransform(const Matrix3x3f &transform);
    void ApplyTransform(const Matrix3x4f &transform);
    void ApplyTransform(const Matrix4x4f &transform);

    // get transformed
    Sphere GetTransformed(const Transform &transform) const;
    Sphere GetTransformed(const Matrix3x3f &transform) const;
    Sphere GetTransformed(const Matrix3x4f &transform) const;
    Sphere GetTransformed(const Matrix4x4f &transform) const;

    bool operator==(const Sphere &comp) const;
    bool operator!=(const Sphere &comp) const;
    Sphere &operator=(const Sphere &copy);

    static Sphere FromAABox(const AABox &box);
    static Sphere FromPoints(const SIMDVector3f *pPoints, uint32 nPoints);
    static Sphere FromPoints(const Vector3f *pPoints, uint32 nPoints);
    static Sphere Merge(const Sphere &left, const Sphere &right);

    static const Sphere &Zero;
    static const Sphere &MaxSize;

private:
    Vector3f m_center;
    float m_radius;
};
