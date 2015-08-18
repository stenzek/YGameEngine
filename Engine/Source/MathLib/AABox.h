#pragma once
#include "MathLib/Common.h"
#include "MathLib/Sphere.h"
#include "MathLib/Vectorf.h"

class Sphere;
class Transform;
struct Matrix3x3f;
struct Matrix3x4f;
struct Matrix4x4f;

// this class represents an axis-aligned bounding box
class AABox
{
public:
    AABox() {}
    AABox(const Vector3f &position);
    AABox(const Vector3f &vecLow, const Vector3f &vecHigh);
    AABox(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
    AABox(const AABox &rCopy);

    // changes low+high values regardless of existing value
    void SetBounds(const AABox &bounds);
    void SetBounds(const Vector3f &position);
    void SetBounds(const Vector3f &vecLow, const Vector3f &vecHigh);
    void SetBounds(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
    void SetZero();

    // changes low value if this value is lower than it, high value if it is higher
    void Merge(float minX, float minY, float minZ, float maxX, float maxY, float maxZ);
    void Merge(const Vector3f &minBounds, const Vector3f &maxBounds);
    void Merge(const Vector3f &vecPoint);
    void Merge(const AABox &box);

    // accessors
    inline const Vector3f &GetMinBounds() const { return m_minBounds; }
    inline const Vector3f &GetMaxBounds() const { return m_maxBounds; }

    // tests whether one AABox contains another
    // 1 - this contains OtherBox, 0 - neither contain, -1 - OtherBox contains this
    int32 ContainsAABox(const AABox &rOtherBox) const;

    // tests whether a point is inside a box
    bool ContainsPoint(const Vector3f &Point) const;

    // tests whether two aaboxes instersect each other
    bool AABoxIntersection(const AABox &aabOther) const;
    bool AABoxIntersection(const AABox &aabOther, Vector3f &contactNormal, Vector3f &contactPoint) const;
    bool AABoxIntersection(const Vector3f &minBounds, const Vector3f &maxBounds) const;
    bool AABoxIntersection(const Vector3f &minBounds, const Vector3f &maxBounds, Vector3f &contactNormal, Vector3f &contactPoint) const;

    // tests whether an aabox and a sphere intersect each other
    bool SphereIntersection(const Sphere &sphOther) const;
    bool SphereIntersection(const Sphere &sphOther, Vector3f &contactNormal, Vector3f &contactPoint) const;

    // tests whether an aabox and a triangle intersect each other
    bool TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2) const;
    bool TriangleIntersection(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, Vector3f &contactNormal, Vector3f &contactPoint) const;
    float TriangleIntersectionTime(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2) const;

    // Get vertices for this box.
    void GetCornerPoints(Vector3f *pVertices) const;
    void GetCornerPoints(SIMDVector3f *pVertices) const;

    // transforms
    void ApplyTransform(const Transform &transform);
    void ApplyTransform(const Matrix3x3f &transform);
    void ApplyTransform(const Matrix3x4f &transform);
    void ApplyTransform(const Matrix4x4f &transform);

    // Get transformed bounds.
    AABox GetTransformed(const Transform &transform) const;
    AABox GetTransformed(const Matrix3x3f &transform) const;
    AABox GetTransformed(const Matrix3x4f &transform) const;
    AABox GetTransformed(const Matrix4x4f &transform) const;

    // Gets a vector at the centre of the the box.
    SIMDVector3f GetCenter() const;

    // Determines the extents of the box. (the size of the box)
    SIMDVector3f GetExtents() const;

    // Gets a bounding sphere encompassing this box.
    Sphere GetSphere() const;

    // comparison operator
    bool operator==(const AABox &Other) const;
    bool operator!=(const AABox &Other) const;

    // assignment operator
    AABox &operator=(const AABox &rCopy);

    // box builders
    static AABox FromSphere(const Sphere &sphere);
    static AABox FromPoints(const SIMDVector3f *pPoints, uint32 nPoints);
    static AABox FromPoints(const Vector3f *pPoints, uint32 nPoints);
    static AABox Merge(const AABox &left, const AABox &right);

    // constants
    static const AABox &Zero;
    static const AABox &MaxSize;
    
private:
    Vector3f m_minBounds;
    Vector3f m_maxBounds;
};
