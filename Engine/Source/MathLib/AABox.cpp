#include "MathLib/AABox.h"
#include "MathLib/Matrixf.h"
#include "MathLib/CollisionDetection.h"
#include "MathLib/Transform.h"
#include "MathLib/Sphere.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"

static const float __AABoxZero[] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
static const float __AABoxMaxSize[] = { -Y_FLT_MAX, -Y_FLT_MAX, -Y_FLT_MAX, Y_FLT_MAX, Y_FLT_MAX, Y_FLT_MAX };
const AABox &AABox::Zero = reinterpret_cast<const AABox &>(__AABoxZero);
const AABox &AABox::MaxSize = reinterpret_cast<const AABox &>(__AABoxMaxSize);

AABox::AABox(const Vector3f &vLow, const Vector3f &vHigh)
{
    DebugAssert(vLow <= vHigh);
    m_minBounds = vLow;
    m_maxBounds = vHigh;
}

AABox::AABox(const AABox &rCopy)
{
    DebugAssert(rCopy.m_minBounds <= rCopy.m_maxBounds);
    m_minBounds = rCopy.GetMinBounds();
    m_maxBounds = rCopy.GetMaxBounds();
}

AABox::AABox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
{
    DebugAssert(minX <= maxX && minY <= maxY && minZ <= maxZ);
    m_minBounds.Set(minX, minY, minZ);
    m_maxBounds.Set(maxX, maxY, maxZ);
}

AABox::AABox(const Vector3f &position)
{
    m_minBounds = position;
    m_maxBounds = position;
}

void AABox::SetBounds(const AABox &bounds)
{
    m_minBounds = bounds.m_minBounds;
    m_maxBounds = bounds.m_maxBounds;
}

void AABox::SetBounds(const Vector3f &position)
{
    m_minBounds = position;
    m_maxBounds = position;
}

void AABox::SetBounds(const Vector3f &vLow, const Vector3f &vHigh)
{
    DebugAssert(vLow <= vHigh);
    m_minBounds = vLow;
    m_maxBounds = vHigh;
}

void AABox::SetBounds(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
{
    DebugAssert(minX <= maxX && minY <= maxY && minZ <= maxZ);
    m_minBounds.Set(minX, minY, minZ);
    m_maxBounds.Set(maxX, maxY, maxZ);
}

void AABox::SetZero()
{
    m_minBounds.SetZero();
    m_maxBounds.SetZero();
}

void AABox::Merge(float minX, float minY, float minZ, float maxX, float maxY, float maxZ)
{
    SIMDVector3f v_minBounds(minX, minY, minZ);
    SIMDVector3f v_maxBounds(maxX, maxY, maxZ);

    m_minBounds = SIMDVector3f(m_minBounds).Min(v_minBounds);
    m_minBounds = SIMDVector3f(m_minBounds).Min(v_maxBounds);
    m_maxBounds = SIMDVector3f(m_maxBounds).Max(v_minBounds);
    m_maxBounds = SIMDVector3f(m_maxBounds).Max(v_maxBounds);
}

void AABox::Merge(const Vector3f &minBounds, const Vector3f &maxBounds)
{
    SIMDVector3f v_minBounds(minBounds);
    SIMDVector3f v_maxBounds(maxBounds);

    m_minBounds = SIMDVector3f(m_minBounds).Min(v_minBounds);
    m_minBounds = SIMDVector3f(m_minBounds).Min(v_maxBounds);
    m_maxBounds = SIMDVector3f(m_maxBounds).Max(v_minBounds);
    m_maxBounds = SIMDVector3f(m_maxBounds).Max(v_maxBounds);
}

void AABox::Merge(const Vector3f &vPoint)
{
    m_minBounds = SIMDVector3f(m_minBounds).Min(vPoint);
    m_maxBounds = SIMDVector3f(m_maxBounds).Max(vPoint);
}

void AABox::Merge(const AABox &box)
{
    Merge(box.GetMinBounds());
    Merge(box.GetMaxBounds());
}

AABox AABox::Merge(const AABox &left, const AABox &right)
{
    AABox newBox(left);
    newBox.Merge(right);
    return newBox;
}

int32 AABox::ContainsAABox(const AABox &rOtherBox) const
{
    SIMDVector3f vecMinBounds(m_minBounds);
    SIMDVector3f vecMaxBounds(m_maxBounds);
    SIMDVector3f vecOtherMinBounds(rOtherBox.m_minBounds);
    SIMDVector3f vecOtherMaxBounds(rOtherBox.m_maxBounds);
    if (vecMinBounds <= vecOtherMinBounds && vecMaxBounds >= vecOtherMaxBounds)
        return 1;
    else if (vecOtherMinBounds <= vecMinBounds && vecOtherMaxBounds >= vecMaxBounds)
        return -1;
    else
        return 0;
}

bool AABox::ContainsPoint(const Vector3f &Point) const
{
    return (Point >= m_minBounds && Point <= m_maxBounds);
}

bool AABox::AABoxIntersection(const AABox &aabOther) const
{
    return CollisionDetection::AABoxIntersectsAABox(m_minBounds, m_maxBounds, aabOther.m_minBounds, aabOther.m_maxBounds);
}

bool AABox::AABoxIntersection(const AABox &aabOther, Vector3f &contactNormal, Vector3f &contactPoint) const
{
    return CollisionDetection::AABoxIntersectsAABox(m_minBounds, m_maxBounds, aabOther.m_minBounds, aabOther.m_maxBounds, contactNormal, contactPoint);
}

bool AABox::AABoxIntersection(const Vector3f &minBounds, const Vector3f &maxBounds) const
{
    return CollisionDetection::AABoxIntersectsAABox(m_minBounds, m_maxBounds, minBounds, maxBounds);
}

bool AABox::AABoxIntersection(const Vector3f &minBounds, const Vector3f &maxBounds, Vector3f &contactNormal, Vector3f &contactPoint) const
{
    return CollisionDetection::AABoxIntersectsAABox(m_minBounds, m_maxBounds, minBounds, maxBounds, contactNormal, contactPoint);
}

bool AABox::SphereIntersection(const Sphere &sphOther) const
{
    return CollisionDetection::AABoxIntersectsSphere(m_minBounds, m_maxBounds, sphOther.GetCenter(), sphOther.GetRadius());
}

bool AABox::SphereIntersection(const Sphere &sphOther, Vector3f &contactNormal, Vector3f &contactPoint) const
{
    return CollisionDetection::AABoxIntersectsSphere(m_minBounds, m_maxBounds, sphOther.GetCenter(), sphOther.GetRadius(), contactNormal, contactPoint);
}

bool AABox::TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2) const
{
    SIMDVector3f vec_v0(v0);
    SIMDVector3f vec_e0(SIMDVector3f(v1) - vec_v0);
    SIMDVector3f vec_e1(SIMDVector3f(v2) - vec_v0);
    SIMDVector3f vec_normal(vec_e0.Cross(vec_e1).Normalize());

    return CollisionDetection::AABoxIntersectsTriangle(m_minBounds, m_maxBounds, v0, v1, v2, vec_e0, vec_e1, vec_normal);
}

bool AABox::TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, Vector3f &contactNormal, Vector3f &contactPoint) const
{
    SIMDVector3f vec_v0(v0);
    SIMDVector3f vec_e0(SIMDVector3f(v1) - vec_v0);
    SIMDVector3f vec_e1(SIMDVector3f(v2) - vec_v0);
    SIMDVector3f vec_normal(vec_e0.Cross(vec_e1).Normalize());

    return CollisionDetection::AABoxIntersectsTriangle(m_minBounds, m_maxBounds, v0, v1, v2, vec_e0, vec_e1, vec_normal, contactNormal, contactPoint);
}

float AABox::TriangleIntersectionTime(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2) const
{
    SIMDVector3f vec_v0(v0);
    SIMDVector3f vec_e0(SIMDVector3f(v1) - vec_v0);
    SIMDVector3f vec_e1(SIMDVector3f(v2) - vec_v0);
    SIMDVector3f vec_normal(vec_e0.Cross(vec_e1).Normalize());

    Vector3f contactNormal, contactPoint;
    if (CollisionDetection::AABoxIntersectsTriangle(m_minBounds, m_maxBounds, v0, v1, v2, vec_e0, vec_e1, vec_normal, contactNormal, contactPoint))
        return (SIMDVector3f(contactPoint) - GetCenter()).Length();
    else
        return Y_FLT_INFINITE;
}

void AABox::GetCornerPoints(Vector3f *pVertices) const
{
    pVertices[0].Set(m_minBounds.x, m_minBounds.y, m_minBounds.z);
    pVertices[1].Set(m_minBounds.x, m_minBounds.y, m_maxBounds.z);
    pVertices[2].Set(m_minBounds.x, m_maxBounds.y, m_minBounds.z);
    pVertices[3].Set(m_minBounds.x, m_maxBounds.y, m_maxBounds.z);
    pVertices[4].Set(m_maxBounds.x, m_minBounds.y, m_minBounds.z);
    pVertices[5].Set(m_maxBounds.x, m_minBounds.y, m_maxBounds.z);
    pVertices[6].Set(m_maxBounds.x, m_maxBounds.y, m_minBounds.z);
    pVertices[7].Set(m_maxBounds.x, m_maxBounds.y, m_maxBounds.z);
}

void AABox::GetCornerPoints(SIMDVector3f *pVertices) const
{
    pVertices[0].Set(m_minBounds.x, m_minBounds.y, m_minBounds.z);
    pVertices[1].Set(m_minBounds.x, m_minBounds.y, m_maxBounds.z);
    pVertices[2].Set(m_minBounds.x, m_maxBounds.y, m_minBounds.z);
    pVertices[3].Set(m_minBounds.x, m_maxBounds.y, m_maxBounds.z);
    pVertices[4].Set(m_maxBounds.x, m_minBounds.y, m_minBounds.z);
    pVertices[5].Set(m_maxBounds.x, m_minBounds.y, m_maxBounds.z);
    pVertices[6].Set(m_maxBounds.x, m_maxBounds.y, m_minBounds.z);
    pVertices[7].Set(m_maxBounds.x, m_maxBounds.y, m_maxBounds.z);
}

void AABox::ApplyTransform(const Transform &transform)
{
    Vector3f cornerPoints[8];
    GetCornerPoints(cornerPoints);
    for (uint32 i = 0; i < 8; i++)
        cornerPoints[i] = transform.TransformPoint(cornerPoints[i]);

    SetBounds(cornerPoints[0], cornerPoints[0]);
    for (uint32 i = 1; i < 8; i++)
        Merge(cornerPoints[i]);
}

void AABox::ApplyTransform(const Matrix3x3f &transform)
{
    Vector3f cornerPoints[8];
    GetCornerPoints(cornerPoints);
    for (uint32 i = 0; i < 8; i++)
        cornerPoints[i] = transform * cornerPoints[i];

    SetBounds(cornerPoints[0], cornerPoints[0]);
    for (uint32 i = 1; i < 8; i++)
        Merge(cornerPoints[i]);
}

void AABox::ApplyTransform(const Matrix3x4f &transform)
{
    Vector3f cornerPoints[8];
    GetCornerPoints(cornerPoints);
    for (uint32 i = 0; i < 8; i++)
        cornerPoints[i] = transform.TransformPoint(cornerPoints[i]);

    SetBounds(cornerPoints[0], cornerPoints[0]);
    for (uint32 i = 1; i < 8; i++)
        Merge(cornerPoints[i]);
}

void AABox::ApplyTransform(const Matrix4x4f &transform)
{
    SIMDMatrix4x4f vecTransformMatrix(transform);

    // fastpath
//     if (TransformMatrix == Matrix4::Identity)
//         return AABox(*this);

    // slowpath
    SIMDVector3f cornerPoints[8];
    GetCornerPoints(cornerPoints);
    for (uint32 i = 0; i < 8; i++)
        cornerPoints[i] = vecTransformMatrix.TransformPoint(cornerPoints[i]);

    SetBounds(cornerPoints[0], cornerPoints[0]);
    for (uint32 i = 1; i < 8; i++)
        Merge(cornerPoints[i]);
}

AABox AABox::GetTransformed(const Transform &transform) const
{
    AABox transformed(*this);
    transformed.ApplyTransform(transform);
    return transformed;
}

AABox AABox::GetTransformed(const Matrix3x3f &transform) const
{
    AABox transformed(*this);
    transformed.ApplyTransform(transform);
    return transformed;
}

AABox AABox::GetTransformed(const Matrix3x4f &transform) const
{
    AABox transformed(*this);
    transformed.ApplyTransform(transform);
    return transformed;
}

AABox AABox::GetTransformed(const Matrix4x4f &transform) const
{
    AABox transformed(*this);
    transformed.ApplyTransform(transform);
    return transformed;
}

SIMDVector3f AABox::GetCenter() const
{
    //return Vector3(((MaxBounds - MinBounds) * 0.5f + MinBounds);
    
    // optimized case
    SIMDVector3f lmin = m_minBounds;
    SIMDVector3f lmax = m_maxBounds;

    SIMDVector3f ret = lmax;
    ret -= lmin;
    ret *= 0.5f;
    ret += lmin;
    
    return ret;
}

SIMDVector3f AABox::GetExtents() const
{
    return (SIMDVector3f(m_maxBounds) - SIMDVector3f(m_minBounds));
}

bool AABox::operator==(const AABox &Other) const
{
    return SIMDVector3f(m_minBounds) == SIMDVector3f(Other.m_minBounds) &&
           SIMDVector3f(m_maxBounds) == SIMDVector3f(Other.m_maxBounds);
}

bool AABox::operator!=(const AABox &Other) const
{
    return SIMDVector3f(m_minBounds) != SIMDVector3f(Other.m_minBounds) ||
           SIMDVector3f(m_maxBounds) != SIMDVector3f(Other.m_maxBounds);
}

AABox &AABox::operator=(const AABox &rCopy)
{
    DebugAssert(rCopy.GetMinBounds() <= rCopy.GetMaxBounds());
    m_minBounds = rCopy.GetMinBounds();
    m_maxBounds = rCopy.GetMaxBounds();
    return *this;
}

AABox AABox::FromPoints(const SIMDVector3f *pPoints, uint32 nPoints)
{
    Assert(nPoints > 0);

    SIMDVector3f minBounds(pPoints[0]);
    SIMDVector3f maxBounds(minBounds);

    uint32 i;
    for (i = 1; i < nPoints; i++)
    {
        const SIMDVector3f point(pPoints[i]);
        minBounds = minBounds.Min(point);
        maxBounds = maxBounds.Max(point);
    }

    return AABox(minBounds, maxBounds);
}

AABox AABox::FromPoints(const Vector3f *pPoints, uint32 nPoints)
{
    Assert(nPoints > 0);

    SIMDVector3f minBounds(pPoints[0]);
    SIMDVector3f maxBounds(minBounds);

    uint32 i;
    for (i = 1; i < nPoints; i++)
    {
        const SIMDVector3f point(pPoints[i]);
        minBounds = minBounds.Min(point);
        maxBounds = maxBounds.Max(point);
    }

    return AABox(minBounds, maxBounds);
}

AABox AABox::FromSphere(const Sphere &sphere)
{
    SIMDVector3f center(SIMDVector3f(sphere.GetCenter()));
    return AABox(center - sphere.GetRadius(), center + sphere.GetRadius());
}

Sphere AABox::GetSphere() const
{
    return Sphere::FromAABox(*this);
}
