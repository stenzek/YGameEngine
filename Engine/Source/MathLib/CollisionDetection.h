#pragma once
#include "MathLib/Common.h"
#include "MathLib/Vectorf.h"

namespace CollisionDetection
{
    // Tests if a point is located inside a triangle. Must provide a point, the first vertex of the triangle, the other 2 edges of the triangle, and a normal.
    bool PointInTriangle(const Vector3f &p, const Vector3f &v0, const Vector3f &e0, const Vector3f &e1, const Vector3f &normal);

    // Ray intersection tests

    // Ray<->Box intersection.
    bool RayIntersectsAABox(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance,
                            const Vector3f &minBounds, const Vector3f &maxBounds);

    // Ray<->Box intersection with contact point.
    bool RayIntersectsAABox(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance,
                            const Vector3f &minBounds, const Vector3f &maxBounds,
                            Vector3f &contactNormal, Vector3f &contactPoint);

    // Ray<->Box intersection with contact face.
    bool RayIntersectsAABox(const Vector3f &rayOrigin, const Vector3f &inverseRayDirection, const float rayDistance,
                            const Vector3f &minBounds, const Vector3f &maxBounds,
                            float *pContactTime, CUBE_FACE *pContactFace);

    // Ray<->Sphere intersection with contact point.
    bool RayIntersectsSphere(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance, 
                             const Vector3f &sphereCenter, const float sphereRadius);

    bool RayIntersectsSphere(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance, 
                             const Vector3f &sphereCenter, const float sphereRadius,
                             Vector3f &contactNormal, Vector3f &contactPoint);

    bool RayIntersectsPlane(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance,
                            const Vector3f &planeNormal, const float planeDistance);

    bool RayIntersectsPlane(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance,
                            const Vector3f &planeNormal, const float planeDistance,
                            Vector3f &contactNormal, Vector3f &contactPoint);

    bool RayIntersectsTriangle(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance,
                               const Vector3f &v0, const Vector3f &v1, const Vector3f &v2,
                               const Vector3f &e0, const Vector3f &e1);

    bool RayIntersectsTriangle(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance,
                               const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1,
                               Vector3f &contactNormal, Vector3f &contactPoint);

    // Axis-aligned box intersection tests
    bool AABoxIntersectsAABox(const Vector3f &AMinBounds, const Vector3f &AMaxBounds, const Vector3f &BMinBounds, const Vector3f &BMaxBounds);
    bool AABoxIntersectsAABox(const Vector3f &AMinBounds, const Vector3f &AMaxBounds, const Vector3f &BMinBounds, const Vector3f &BMaxBounds, Vector3f &contactNormal, Vector3f &contactPoint);
    bool AABoxIntersectsSphere(const Vector3f &minBounds, const Vector3f &maxBounds, const Vector3f &sphereCenter, const float sphereRadius);
    bool AABoxIntersectsSphere(const Vector3f &minBounds, const Vector3f &maxBounds, const Vector3f &sphereCenter, const float sphereRadius, Vector3f &contactNormal, Vector3f &contactPoint);
    bool AABoxIntersectsPlane(const Vector3f &minBounds, const Vector3f &maxBounds, const Vector3f &planeNormal, const float planeDistance);
    bool AABoxIntersectsPlane(const Vector3f &minBounds, const Vector3f &maxBounds, const Vector3f &planeNormal, const float planeDistance, Vector3f &contactNormal, Vector3f &contactPoint);
    bool AABoxIntersectsTriangle(const Vector3f &minBounds, const Vector3f &maxBounds, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1, const Vector3f &normal);
    bool AABoxIntersectsTriangle(const Vector3f &minBounds, const Vector3f &maxBounds, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1, const Vector3f &normal, Vector3f &contactNormal, Vector3f &contactPoint);
    
    // Sphere intersection tests
    bool SphereIntersectsBox(const Vector3f &sphereCenter, const float sphereRadius, const Vector3f &minBounds, const Vector3f &maxBounds);
    bool SphereIntersectsBox(const Vector3f &sphereCenter, const float sphereRadius, const Vector3f &minBounds, const Vector3f &maxBounds, Vector3f &contactNormal, Vector3f &contactPoint);
    bool SphereIntersectsSphere(const Vector3f &ACenter, const float ARadius, const Vector3f &BCenter, const float BRadius);
    bool SphereIntersectsSphere(const Vector3f &ACenter, const float ARadius, const Vector3f &BCenter, const float BRadius, Vector3f &contactNormal, Vector3f &contactPoint);
    bool SphereIntersectsPlane(const Vector3f &sphereCenter, const float sphereRadius, const Vector3f &planeNormal, const float planeDistance);
    bool SphereIntersectsPlane(const Vector3f &sphereCenter, const float sphereRadius, const Vector3f &planeNormal, const float planeDistance, Vector3f &contactNormal, Vector3f &contactPoint);
    bool SphereIntersectsTriangle(const Vector3f &sphereCenter, const float sphereRadius, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1, const Vector3f &normal);
    bool SphereIntersectsTriangle(const Vector3f &sphereCenter, const float sphereRadius, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1, const Vector3f &normal, Vector3f &contactNormal, Vector3f &contactPoint);

    // AABB Sweep
    float AABoxSweep(const Vector3f &movingBoxMinBounds, const Vector3f &movingBoxMaxBounds, const Vector3f &movingBoxDisplacement, const Vector3f &staticBoxMinBounds, const Vector3f &staticBoxMaxBounds);

    // Sphere Sweep
    //TODO
}
