#include "MathLib/Sphere.h"
#include "MathLib/AABox.h"
#include "MathLib/CollisionDetection.h"
#include "MathLib/Transform.h"
#include "MathLib/SIMDVectorf.h"

static const float __SphereZero[] = { 0.0f, 0.0f, 0.0f, 0.0f };
static const float __SphereMaxSize[] = { 0.0f, 0.0f, 0.0f, Y_FLT_MAX };
const Sphere &Sphere::Zero = reinterpret_cast<const Sphere &>(__SphereZero);
const Sphere &Sphere::MaxSize = reinterpret_cast<const Sphere &>(__SphereMaxSize);

Sphere::Sphere(const Vector3f &centre, float radius)
    : m_center(centre),
      m_radius(radius)
{

}

Sphere::Sphere(const Sphere &copy)
    : m_center(copy.m_center),
      m_radius(copy.m_radius)
{

}

void Sphere::Merge(const Vector3f &point)
{
    float dist = (SIMDVector3f(point) - SIMDVector3f(m_center)).Length();
    m_radius = Max(m_radius, dist);
}

void Sphere::Merge(const Sphere &sphere)
{
    SIMDVector3f D(SIMDVector3f(sphere.m_center) - SIMDVector3f(m_center));
    float d = D.Length();

    float i0min = -m_radius;
    float i0max = m_radius;
    float i1min = d - sphere.m_radius;
    float i1max = d + sphere.m_radius;

    if (i1min >= i0min && i1max <= i0max)
    {

    }
    else if (i0min >= i1min && i0max <= i1max)
    {
        m_center = sphere.m_center;
        m_radius = sphere.m_radius;
    }
    else
    {
        m_radius = d + m_radius + sphere.m_radius;
        m_center = SIMDVector3f(m_center) + D * (0.5f * (d + m_radius - sphere.m_radius)) / d;
    }
}

Sphere Sphere::Merge(const Sphere &left, const Sphere &right)
{
    Sphere newSphere(left);
    newSphere.Merge(right);
    return newSphere;
}

bool Sphere::AABoxIntersection(const AABox &box) const
{
    return CollisionDetection::SphereIntersectsBox(m_center, m_radius, box.GetMinBounds(), box.GetMaxBounds());
}

bool Sphere::AABoxIntersection(const AABox &box, Vector3f &contactNormal, Vector3f &contactPoint) const
{
    return CollisionDetection::SphereIntersectsBox(m_center, m_radius, box.GetMinBounds(), box.GetMaxBounds(), contactNormal, contactPoint);
}

bool Sphere::AABoxIntersection(const Vector3f &minBounds, const Vector3f &maxBounds) const
{
    return CollisionDetection::SphereIntersectsBox(m_center, m_radius, minBounds, maxBounds);
}

bool Sphere::AABoxIntersection(const Vector3f &minBounds, const Vector3f &maxBounds, Vector3f &contactNormal, Vector3f &contactPoint) const
{
    return CollisionDetection::SphereIntersectsBox(m_center, m_radius, minBounds, maxBounds, contactNormal, contactPoint);
}

bool Sphere::SphereIntersection(const Sphere &sphere) const
{
    return CollisionDetection::SphereIntersectsSphere(m_center, m_radius, sphere.m_center, sphere.m_radius);
}

bool Sphere::SphereIntersection(const Sphere &sphere, Vector3f &contactNormal, Vector3f &contactPoint) const
{
    return CollisionDetection::SphereIntersectsSphere(m_center, m_radius, sphere.m_center, sphere.m_radius, contactNormal, contactPoint);
}

bool Sphere::TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2) const
{
    SIMDVector3f vec_v0(v0);
    SIMDVector3f vec_e0(SIMDVector3f(v1) - vec_v0);
    SIMDVector3f vec_e1(SIMDVector3f(v2) - vec_v0);
    SIMDVector3f vec_normal(vec_e0.Cross(vec_e1).Normalize());

    return CollisionDetection::SphereIntersectsTriangle(m_center, m_radius, v0, v1, v2, vec_e0, vec_e1, vec_normal);
}

bool Sphere::TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, Vector3f &contactNormal, Vector3f &contactPoint) const
{
    SIMDVector3f vec_v0(v0);
    SIMDVector3f vec_e0(SIMDVector3f(v1) - vec_v0);
    SIMDVector3f vec_e1(SIMDVector3f(v2) - vec_v0);
    SIMDVector3f vec_normal(vec_e0.Cross(vec_e1).Normalize());

    return CollisionDetection::SphereIntersectsTriangle(m_center, m_radius, v0, v1, v2, vec_e0, vec_e1, vec_normal, contactNormal, contactPoint);
}

float Sphere::TriangleIntersectionTime(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2) const
{
    SIMDVector3f vec_v0(v0);
    SIMDVector3f vec_e0(SIMDVector3f(v1) - vec_v0);
    SIMDVector3f vec_e1(SIMDVector3f(v2) - vec_v0);
    SIMDVector3f vec_normal(vec_e0.Cross(vec_e1).Normalize());

    Vector3f contactNormal, contactPoint;
    if (CollisionDetection::SphereIntersectsTriangle(m_center, m_radius, v0, v1, v2, vec_e0, vec_e1, vec_normal, contactNormal, contactPoint))
        return (SIMDVector3f(contactPoint) - SIMDVector3f(m_center)).Length();
    else
        return Y_FLT_INFINITE;
}

bool Sphere::operator==(const Sphere &comp) const
{
    return SIMDVector3f(m_center) == SIMDVector3f(comp.m_center) && 
           m_radius == comp.m_radius;
}

bool Sphere::operator!=(const Sphere &comp) const
{
    return SIMDVector3f(m_center) != SIMDVector3f(comp.m_center) || 
           m_radius != comp.m_radius;
}

Sphere &Sphere::operator=(const Sphere &copy)
{
    m_center = copy.m_center;
    m_radius = copy.m_radius;
    return *this;
}

Sphere Sphere::FromAABox(const AABox &box)
{
    SIMDVector3f center(box.GetCenter());
    float radius = (SIMDVector3f(box.GetMaxBounds()) - center).Length();

    return Sphere(center, radius);    
}

Sphere Sphere::FromPoints(const SIMDVector3f *pPoints, uint32 nPoints)
{
    uint32 i;
    Assert(nPoints > 0);

    SIMDVector3f center(pPoints[0]);
    for (i = 1; i < nPoints; i++)
        center += pPoints[i];
    center /= (float)nPoints;
    
    float squaredRadius = 0.0f;
    for (i = 0; i < nPoints; i++)
        squaredRadius = Max(squaredRadius, (pPoints[i] - center).SquaredLength());

    return Sphere(center, Math::Sqrt(squaredRadius));
}

Sphere Sphere::FromPoints(const Vector3f *pPoints, uint32 nPoints)
{
    uint32 i;
    Assert(nPoints > 0);

    SIMDVector3f center(pPoints[0]);
    for (i = 1; i < nPoints; i++)
        center += SIMDVector3f(pPoints[i]);
    center /= (float)nPoints;

    float squaredRadius = 0.0f;
    for (i = 0; i < nPoints; i++)
        squaredRadius = Max(squaredRadius, (SIMDVector3f(pPoints[i]) - center).SquaredLength());

    return Sphere(center, Math::Sqrt(squaredRadius));
}

void Sphere::ApplyTransform(const Transform &transform)
{
    // get maximum amount of scale
    float maxScaleDimension = Max(transform.GetScale().x, Max(transform.GetScale().y, transform.GetScale().z));
    
    // apply translation
    m_center += transform.GetPosition();

    // apply scale
    m_radius *= maxScaleDimension;
}

void Sphere::ApplyTransform(const Matrix3x3f &transform)
{
    // get maximum amount of scale
    float maxScaleDimension = Max(transform(0, 0), Max(transform(1, 1), transform(2, 2)));

    // apply scale
    m_radius *= maxScaleDimension;
}

void Sphere::ApplyTransform(const Matrix3x4f &transform)
{
    // get maximum amount of scale
    float maxScaleDimension = Max(transform(0, 0), Max(transform(1, 1), transform(2, 2)));

    // apply translation
    m_center += transform.GetColumn(3);

    // apply scale
    m_radius *= maxScaleDimension;
}

void Sphere::ApplyTransform(const Matrix4x4f &transform)
{
    // get maximum amount of scale
    float maxScaleDimension = Max(transform(0, 0), Max(transform(1, 1), transform(2, 2)));

    // apply translation
    m_center += transform.GetColumn(3).xyz();

    // apply scale
    m_radius *= maxScaleDimension;
}

Sphere Sphere::GetTransformed(const Transform &transform) const
{
    Sphere transformed(*this);
    transformed.ApplyTransform(transform);
    return transformed;
}

Sphere Sphere::GetTransformed(const Matrix3x3f &transform) const
{
    Sphere transformed(*this);
    transformed.ApplyTransform(transform);
    return transformed;
}

Sphere Sphere::GetTransformed(const Matrix3x4f &transform) const
{
    Sphere transformed(*this);
    transformed.ApplyTransform(transform);
    return transformed;
}

Sphere Sphere::GetTransformed(const Matrix4x4f &transform) const
{
    Sphere transformed(*this);
    transformed.ApplyTransform(transform);
    return transformed;
}

