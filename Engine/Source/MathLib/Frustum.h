#pragma once
#include "MathLib/Common.h"
#include "MathLib/Vectorf.h"
#include "MathLib/Matrixf.h"
#include "MathLib/Plane.h"
#include "MathLib/AABox.h"
#include "MathLib/Sphere.h"

enum FRUSTUM_PLANE
{
    FRUSTUM_PLANE_LEFT,
    FRUSTUM_PLANE_RIGHT,
    FRUSTUM_PLANE_TOP,
    FRUSTUM_PLANE_BOTTOM,
    FRUSTUM_PLANE_FAR,
    FRUSTUM_PLANE_NEAR,
    FRUSTUM_PLANE_COUNT,
};

class Frustum
{
public:
    enum IntersectionType
    {
        INTERSECTION_TYPE_OUTSIDE,              // completely outside the frustum
        INTERSECTION_TYPE_INTERSECTS,           // one of the planes intersects with the object
        INTERSECTION_TYPE_INSIDE,               // completely inside the frustum
    };

public:
    Frustum() {}
    Frustum(const Frustum &frCopy);

    // Builds a frustum from a concatenated view/projection matrix.
    void SetFromMatrix(const Matrix4x4f &matViewProj);

    // Builds a frustum from the specified AABox.
    void SetFromAABox(const AABox &aabBox);

    // Accesses the planes of the frustum.
    const Plane &GetPlane(FRUSTUM_PLANE PlaneIndex) const { return m_planes[PlaneIndex]; }

    // Tests if the object intersects the frustum, or is contained in the frustum.
    bool AABoxIntersection(const AABox &aabBounds) const;
    bool SphereIntersection(const Sphere &sphere) const;

    // Gets the intersection type for the object
    IntersectionType AABoxIntersectionType(const AABox &box) const;
    IntersectionType SphereIntersectionType(const Sphere &sphere) const;

    // Gets the corner vertices.
    void GetCornerVertices(Vector3f pVertices[8]) const;
    void GetCornerVertices(SIMDVector3f pVertices[8]) const;

    // Gets an AABox containing the frustum.
    AABox GetBoundingAABox() const;

    // Gets a sphere containing the frustum.
    Sphere GetBoundingSphere() const;

    // Operators
    bool operator==(const Frustum &Comp) const;
    bool operator!=(const Frustum &Comp) const;
    Frustum &operator=(const Frustum &Copy);

private:
    // clipping planes
    Plane m_planes[FRUSTUM_PLANE_COUNT];
};

