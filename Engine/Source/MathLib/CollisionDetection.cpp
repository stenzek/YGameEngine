#include "MathLib/CollisionDetection.h"
#include "MathLib/SIMDVectorf.h"

bool CollisionDetection::PointInTriangle(const Vector3f &p, const Vector3f &v0, const Vector3f &e0, const Vector3f &e1, const Vector3f &normal)
{
    SIMDVector3f u(e0);
    SIMDVector3f v(e1);
    SIMDVector3f w = p - v0;
    SIMDVector3f vCrossW = v.Cross(w);
    SIMDVector3f vCrossU = v.Cross(u);
    if (vCrossW.Dot(vCrossU) < 0.0f)
        return false;

    SIMDVector3f uCrossW = u.Cross(w);
    SIMDVector3f uCrossV = u.Cross(v);

    if (uCrossW.Dot(uCrossV) < 0.0f)
        return false;

    float denon = uCrossV.Length();
    float invDenom = 1.0f / denon;
    float r = vCrossW.Length() * invDenom;
    float t = uCrossW.Length() * invDenom;

    return (r <= 1.0f && t <= 1.0f && (r + t) <= 1.0f);
}

bool CollisionDetection::RayIntersectsAABox(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance, const Vector3f &minBounds, const Vector3f &maxBounds)
{
    bool inside = true;
    SIMDVector3f maxT(-1.0f, -1.0f, -1.0f);
    Vector3f hitLocation; 

    if (rayOrigin.x < minBounds.x)
    {
        hitLocation.x = minBounds.x;
        inside = false;
        if (rayDirection.x != 0.0f)
            maxT.x = (minBounds.x - rayOrigin.x) / rayDirection.x;
    }
    else if (rayOrigin.x > maxBounds.x)
    {
        hitLocation.x = maxBounds.x;
        inside = false;
        if (rayDirection.x != 0.0f)
            maxT.x = (maxBounds.x - rayOrigin.x) / rayDirection.x;
    }

    if (rayOrigin.y < minBounds.y)
    {
        hitLocation.y = minBounds.y;
        inside = false;
        if (rayDirection.y != 0.0f)
            maxT.y = (minBounds.y - rayOrigin.y) / rayDirection.y;
    }
    else if (rayOrigin.y > maxBounds.y)
    {
        hitLocation.y = maxBounds.y;
        inside = false;
        if (rayDirection.y != 0.0f)
            maxT.y = (maxBounds.y - rayOrigin.y) / rayDirection.y;
    }

    if (rayOrigin.z < minBounds.z)
    {
        hitLocation.z = minBounds.z;
        inside = false;
        if (rayDirection.z != 0.0f)
            maxT.z = (minBounds.z - rayOrigin.z) / rayDirection.z;
    }
    else if (rayOrigin.z > maxBounds.z)
    {
        hitLocation.z = maxBounds.z;
        inside = false;
        if (rayDirection.z != 0.0f)
            maxT.z = (maxBounds.z - rayOrigin.z) / rayDirection.z;
    }

    if (inside)
        return true;

    uint32 plane = 0;
    if (maxT[1] > maxT[plane])
        plane = 1;
    if (maxT[2] > maxT[plane])
        plane = 2;

    if (maxT[plane] < 0.0f)
        return false;

    for (uint32 i = 0; i < 3; ++i)
    {
        if (i != plane)
        {
            hitLocation[i] = rayOrigin[i] + maxT[plane] * rayDirection[i];
            if ((hitLocation[i] < minBounds[i]) ||
                (hitLocation[i] > maxBounds[i]))
            {
                return false;
            }
        }
    }

    return (SIMDVector3f(hitLocation) - SIMDVector3f(rayOrigin)).SquaredLength() <= Math::Square(rayDistance);
}

bool CollisionDetection::RayIntersectsAABox(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance, const Vector3f &minBounds, const Vector3f &maxBounds, Vector3f &contactNormal, Vector3f &contactPoint)
{
    bool inside = true;
    SIMDVector3f maxT(-1.0f, -1.0f, -1.0f);
    Vector3f hitLocation; 

    if (rayOrigin.x < minBounds.x)
    {
        hitLocation.x = minBounds.x;
        inside = false;
        if (rayDirection.x != 0.0f)
            maxT.x = (minBounds.x - rayOrigin.x) / rayDirection.x;
    }
    else if (rayOrigin.x > maxBounds.x)
    {
        hitLocation.x = maxBounds.x;
        inside = false;
        if (rayDirection.x != 0.0f)
            maxT.x = (maxBounds.x - rayOrigin.x) / rayDirection.x;
    }

    if (rayOrigin.y < minBounds.y)
    {
        hitLocation.y = minBounds.y;
        inside = false;
        if (rayDirection.y != 0.0f)
            maxT.y = (minBounds.y - rayOrigin.y) / rayDirection.y;
    }
    else if (rayOrigin.y > maxBounds.y)
    {
        hitLocation.y = maxBounds.y;
        inside = false;
        if (rayDirection.y != 0.0f)
            maxT.y = (maxBounds.y - rayOrigin.y) / rayDirection.y;
    }

    if (rayOrigin.z < minBounds.z)
    {
        hitLocation.z = minBounds.z;
        inside = false;
        if (rayDirection.z != 0.0f)
            maxT.z = (minBounds.z - rayOrigin.z) / rayDirection.z;
    }
    else if (rayOrigin.z > maxBounds.z)
    {
        hitLocation.z = maxBounds.z;
        inside = false;
        if (rayDirection.z != 0.0f)
            maxT.z = (maxBounds.z - rayOrigin.z) / rayDirection.z;
    }

    if (inside)
        return true;

    uint32 plane = 0;
    if (maxT[1] > maxT[plane])
        plane = 1;
    if (maxT[2] > maxT[plane])
        plane = 2;

    if (maxT[plane] < 0.0f)
        return false;

    for (uint32 i = 0; i < 3; ++i)
    {
        if (i != plane)
        {
            hitLocation[i] = rayOrigin[i] + maxT[plane] * rayDirection[i];
            if ((hitLocation[i] < minBounds[i]) ||
                (hitLocation[i] > maxBounds[i]))
            {
                return false;
            }
        }
    }

    if ((SIMDVector3f(hitLocation) - SIMDVector3f(rayOrigin)).SquaredLength() > Math::Square(rayDistance))
        return false;

    contactNormal.SetZero();
    contactNormal[plane] = (rayDirection[plane] > 0.0f) ? -1.0f : 1.0f;
    contactPoint = hitLocation;
    return true;
}

bool CollisionDetection::RayIntersectsAABox(const Vector3f &rayOrigin, const Vector3f &inverseRayDirection, const float rayDistance, const Vector3f &minBounds, const Vector3f &maxBounds, float *pContactTime, CUBE_FACE *pContactFace)
{
    static const uint32 faceIndices[3][2] = { { CUBE_FACE_LEFT, CUBE_FACE_RIGHT }, { CUBE_FACE_FRONT, CUBE_FACE_BACK }, { CUBE_FACE_BOTTOM, CUBE_FACE_TOP } };

    SIMDVector3f vec_rayOrigin(rayOrigin);
    SIMDVector3f vec_inverseRayDirection(inverseRayDirection);
    SIMDVector3f v1 = (SIMDVector3f(minBounds) - rayOrigin) * vec_inverseRayDirection;
    SIMDVector3f v2 = (SIMDVector3f(maxBounds) - rayOrigin) * vec_inverseRayDirection;

    uint32 i;
    float tmin = -Y_FLT_INFINITE, tmax = Y_FLT_INFINITE;
    uint32 minHitFace = 0, maxHitFace = 0;

    for (i = 0; i < 3; i++)
    {
        const uint32 *pFaceIndices = faceIndices[i];
        const float &t1 = v1[i];
        const float &t2 = v2[i];

        if (t1 < t2)
        {
            if (t1 > tmin)
            {
                tmin = t1;
                minHitFace = pFaceIndices[0];
            }

            if (t2 < tmax)
            {
                tmax = t2;
                maxHitFace = pFaceIndices[1];
            }
        }
        else
        {
            if (t2 > tmin)
            {
                tmin = t2;
                minHitFace = pFaceIndices[1];
            }
            if (t1 < tmax)
            {
                tmax = t1;
                maxHitFace = pFaceIndices[0];
            }
        }
    }

    if (tmin < tmax)
    {
        if (tmin > 0)
        {
            if (tmin <= rayDistance)
            {
                *pContactTime = tmin;
                *pContactFace = (CUBE_FACE)minHitFace;
                return true;
            }
        }
        else if (tmax > 0)
        {
            if (tmax <= rayDistance)
            {
                *pContactTime = tmax;
                *pContactFace = (CUBE_FACE)maxHitFace;
                return true;
            }
        }
    }

    return false;
}

bool CollisionDetection::RayIntersectsSphere(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance, const Vector3f &sphereCenter, const float sphereRadius)
{
    // http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection
    SIMDVector3f vec_rayOrigin(rayOrigin);
    SIMDVector3f vec_rayDirection(rayDirection);

    float a = vec_rayDirection.Dot(vec_rayDirection);
    float b = 2.0f * vec_rayDirection.Dot(vec_rayOrigin);
    float c = vec_rayOrigin.Dot(vec_rayOrigin) - Math::Square(sphereRadius);

    float disc = Math::Square(b) - 4.0f * a * c;
    if (disc < 0.0f)
        return false;

    float discSqrt = Math::Sqrt(disc);
    float q;
    if (b < 0.0f)
        q = (-b - discSqrt) / 2.0f;
    else
        q = (-b + discSqrt) / 2.0f;

    float t0 = q / a;
    float t1 = c / q;
    if (t0 > t1)
        Swap(t0, t1);

    if (t1 < 0.0f)
        return false;

    if (t0 < 0.0f)
        return (t1 < rayDistance);
    else
        return (t0 < rayDistance);
}

bool CollisionDetection::RayIntersectsSphere(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance, const Vector3f &sphereCenter, const float sphereRadius, Vector3f &contactNormal, Vector3f &contactPoint)
{
    // http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection
    SIMDVector3f vec_rayOrigin(rayOrigin);
    SIMDVector3f vec_rayDirection(rayDirection);

    float a = vec_rayDirection.Dot(vec_rayDirection);
    float b = 2.0f * vec_rayDirection.Dot(vec_rayOrigin);
    float c = vec_rayOrigin.Dot(vec_rayOrigin) - Math::Square(sphereRadius);

    float disc = Math::Square(b) - 4.0f * a * c;
    if (disc < 0.0f)
        return false;

    float discSqrt = Math::Sqrt(disc);
    float q;
    if (b < 0.0f)
        q = (-b - discSqrt) / 2.0f;
    else
        q = (-b + discSqrt) / 2.0f;

    float t0 = q / a;
    float t1 = c / q;
    if (t0 > t1)
        Swap(t0, t1);

    if (t1 < 0.0f)
        return false;

    float t = (t0 < 0.0f) ? t1 : t0;
    if (t > rayDistance)
        return false;

    contactNormal = -rayDirection;
    contactPoint = vec_rayOrigin + vec_rayDirection * t;
    return true;
}

bool CollisionDetection::RayIntersectsPlane(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance, const Vector3f &planeNormal, const float planeDistance)
{
    SIMDVector3f vec_planeNormal(planeNormal);

    float DdotN = SIMDVector3f(rayDirection).Dot(vec_planeNormal);
    if (DdotN >= 0.0f)
        return false;

    float OdotN = SIMDVector3f(rayOrigin).Dot(vec_planeNormal);
    if (Math::NearEqual(OdotN + planeDistance, 0.0f, Math::Epsilon<float>()))
        return true;

    float t = (planeDistance - OdotN) / DdotN;
    return (t >= 0.0f && t <= rayDistance);
}

bool CollisionDetection::RayIntersectsPlane(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance, const Vector3f &planeNormal, const float planeDistance, Vector3f &contactNormal, Vector3f &contactPoint)
{
    SIMDVector3f vec_planeNormal(planeNormal);
    SIMDVector3f vec_rayOrigin(rayOrigin);
    SIMDVector3f vec_rayDirection(rayDirection);

    float DdotN = vec_rayDirection.Dot(vec_planeNormal);
    if (DdotN >= 0.0f)
        return false;

    float OdotN = vec_rayOrigin.Dot(vec_planeNormal);
    float t = 0.0f;
    if (Math::NearEqual(OdotN + planeDistance, 0.0f, Math::Epsilon<float>()) ||
        (t = (planeDistance - OdotN) / DdotN) >= 0.0f)
    {
        contactNormal = planeNormal;
        contactPoint = vec_rayOrigin + vec_rayDirection * t;
        return true;
    }
    else
    {
        return false;
    }
}

bool CollisionDetection::RayIntersectsTriangle(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1)
{
    SIMDVector3f vec_rayOrigin(rayOrigin);
    SIMDVector3f vec_rayDirection(rayDirection);
    SIMDVector3f vec_e0(e0);
    SIMDVector3f vec_e1(e1);

    SIMDVector3f pvec = vec_rayDirection.Cross(vec_e1);
    float det = vec_e0.Dot(pvec);
    if (det <= 0.0f)
        return false;

    float invDet = 1.0f / det;
    SIMDVector3f tvec = vec_rayOrigin - v0;
    float u = tvec.Dot(pvec) * invDet;
    if (u < 0.0f || u > 1.0f)
        return false;

    SIMDVector3f qvec = tvec.Cross(vec_e0);
    float v = vec_rayDirection.Dot(qvec) * invDet;
    if (v < 0 || (u + v) > 1.0f)
        return false;

    float t = vec_e1.Dot(qvec) * invDet;
    return (t >= 0.0f && t <= rayDistance);
}

bool CollisionDetection::RayIntersectsTriangle(const Vector3f &rayOrigin, const Vector3f &rayDirection, const float rayDistance,
                                               const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1,
                                               Vector3f &contactNormal, Vector3f &contactPoint)
{
    SIMDVector3f vec_rayOrigin(rayOrigin);
    SIMDVector3f vec_rayDirection(rayDirection);
    SIMDVector3f vec_e0(e0);
    SIMDVector3f vec_e1(e1);

    SIMDVector3f pvec = vec_rayDirection.Cross(vec_e1);
    float det = vec_e0.Dot(pvec);
    if (det <= 0.0f)
        return false;

    float invDet = 1.0f / det;
    SIMDVector3f tvec = vec_rayOrigin - v0;
    float u = tvec.Dot(pvec) * invDet;
    if (u < 0.0f || u > 1.0f)
        return false;

    SIMDVector3f qvec = tvec.Cross(vec_e0);
    float v = vec_rayDirection.Dot(qvec) * invDet;
    if (v < 0 || (u + v) > 1.0f)
        return false;

    float t = vec_e1.Dot(qvec) * invDet;
    if (t >= 0.0f && t <= rayDistance)
    {
        contactNormal = vec_e0.Cross(vec_e1).NormalizeEst();
        contactPoint = vec_rayOrigin + vec_rayDirection * t;
        return true;
    }
    else
    {
        return false;
    }
}

bool CollisionDetection::AABoxIntersectsAABox(const Vector3f &AMinBounds, const Vector3f &AMaxBounds, const Vector3f &BMinBounds, const Vector3f &BMaxBounds)
{
    SIMDVector3f M(AMinBounds);
    SIMDVector3f N(AMaxBounds);
    SIMDVector3f O(BMinBounds);
    SIMDVector3f P(BMaxBounds);

    if (M.AnyGreater(P) || O.AnyGreater(N))
        return false;

    return true;
}

bool CollisionDetection::AABoxIntersectsAABox(const Vector3f &AMinBounds, const Vector3f &AMaxBounds, const Vector3f &BMinBounds, const Vector3f &BMaxBounds, Vector3f &contactNormal, Vector3f &contactPoint)
{

    return false;
}

bool CollisionDetection::AABoxIntersectsSphere(const Vector3f &minBounds, const Vector3f &maxBounds, const Vector3f &sphereCenter, const float sphereRadius)
{
    return SphereIntersectsBox(sphereCenter, sphereRadius, minBounds, maxBounds);
}

bool CollisionDetection::AABoxIntersectsSphere(const Vector3f &minBounds, const Vector3f &maxBounds, const Vector3f &sphereCenter, const float sphereRadius, Vector3f &contactNormal, Vector3f &contactPoint)
{
    return false;
}

bool CollisionDetection::AABoxIntersectsPlane(const Vector3f &minBounds, const Vector3f &maxBounds, const Vector3f &planeNormal, const float planeDistance)
{
    return false;
}

bool CollisionDetection::AABoxIntersectsPlane(const Vector3f &minBounds, const Vector3f &maxBounds, const Vector3f &planeNormal, const float planeDistance, Vector3f &contactNormal, Vector3f &contactPoint)
{
    return false;
}

bool CollisionDetection::AABoxIntersectsTriangle(const Vector3f &minBounds, const Vector3f &maxBounds, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1, const Vector3f &normal)
{
    return false;
}

bool CollisionDetection::AABoxIntersectsTriangle(const Vector3f &minBounds, const Vector3f &maxBounds, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1, const Vector3f &normal, Vector3f &contactNormal, Vector3f &contactPoint)
{
    return false;
}

bool CollisionDetection::SphereIntersectsBox(const Vector3f &sphereCenter, const float sphereRadius, const Vector3f &minBounds, const Vector3f &maxBounds)
{
    // http://www.gamasutra.com/view/feature/131790/simple_intersection_tests_for_games.php?page=4
    float d = 0.0f;

    if (sphereCenter.x < minBounds.x)
        d += Math::Square(sphereCenter.x - minBounds.x);
    else if (sphereCenter.x > maxBounds.x)
        d += Math::Square(sphereCenter.x - maxBounds.x);

    if (sphereCenter.y < minBounds.y)
        d += Math::Square(sphereCenter.y - minBounds.y);
    else if (sphereCenter.y > maxBounds.y)
        d += Math::Square(sphereCenter.y - maxBounds.y);

    if (sphereCenter.z < minBounds.z)
        d += Math::Square(sphereCenter.z - minBounds.z);
    else if (sphereCenter.z > maxBounds.z)
        d += Math::Square(sphereCenter.z - maxBounds.z);

    return (d <= Math::Square(sphereRadius));
}

bool CollisionDetection::SphereIntersectsBox(const Vector3f &sphereCenter, const float sphereRadius, const Vector3f &minBounds, const Vector3f &maxBounds, Vector3f &contactNormal, Vector3f &contactPoint)
{
    Vector3f boxExtents(maxBounds - minBounds);
    Vector3f boxHalfExtents(boxExtents * 0.5f);
    Vector3f boxCenter(minBounds + boxHalfExtents);
    
    Vector3f sphereRelPos(sphereCenter - boxCenter);
    Vector3f closestPoint(sphereRelPos);
    closestPoint.x = Min(boxHalfExtents.x, closestPoint.x);
    closestPoint.x = Max(-boxHalfExtents.x, closestPoint.x);
    closestPoint.y = Min(boxHalfExtents.y, closestPoint.y);
    closestPoint.y = Max(-boxHalfExtents.y, closestPoint.y);
    closestPoint.z = Min(boxHalfExtents.z, closestPoint.z);
    closestPoint.z = Max(-boxHalfExtents.z, closestPoint.z);

    Vector3f relPosToPoint(sphereRelPos - closestPoint);
    float squaredDistance = relPosToPoint.SquaredLength();
    if (squaredDistance > Math::Square(sphereRadius))
        return false;

    float distance;
    Vector3f normal;
    if (squaredDistance <= Math::Epsilon<float>())
    {
        float faceDist = boxHalfExtents.x - sphereRelPos.x;
        float minDist = faceDist;
        closestPoint.x = boxHalfExtents.x;
        normal.Set(1.0f, 0.0f, 0.0f);

        faceDist = boxHalfExtents.x + sphereRelPos.x;
        if (faceDist < minDist)
        {
            minDist = faceDist;
            closestPoint = sphereRelPos;
            closestPoint.x = -boxHalfExtents.x;
            normal.Set(-1.0f, 0.0f, 0.0f);
        }

        faceDist = boxHalfExtents.y - sphereRelPos.y;
        if (faceDist < minDist)
        {
            minDist = faceDist;
            closestPoint = sphereRelPos;
            closestPoint.y = boxHalfExtents.y;
            normal.Set(0.0f, 1.0f, 0.0f);
        }

        faceDist = boxHalfExtents.y + sphereRelPos.y;
        if (faceDist < minDist)
        {
            minDist = faceDist;
            closestPoint = sphereRelPos;
            closestPoint.y = -boxHalfExtents.y;
            normal.Set(0.0f, -1.0f, 0.0f);
        }

        faceDist = boxHalfExtents.z - sphereRelPos.z;
        if (faceDist < minDist)
        {
            minDist = faceDist;
            closestPoint = sphereRelPos;
            closestPoint.z = boxHalfExtents.z;
            normal.Set(0.0f, 0.0f, 1.0f);
        }

        faceDist = boxHalfExtents.z + sphereRelPos.z;
        if (faceDist < minDist)
        {
            minDist = faceDist;
            closestPoint = sphereRelPos;
            closestPoint.z = -boxHalfExtents.z;
            normal.Set(0.0f, 0.0f, -1.0f);
        }

        distance = minDist;
    }
    else
    {
        distance = Math::Sqrt(squaredDistance);
        normal = relPosToPoint / distance;
    }

    contactNormal = normal;
    contactPoint = closestPoint + boxCenter;
    return true;
}

bool CollisionDetection::SphereIntersectsSphere(const Vector3f &ACenter, const float ARadius, const Vector3f &BCenter, const float BRadius)
{
    SIMDVector3f vec_ACenter(ACenter);
    SIMDVector3f vec_BCenter(BCenter);

    SIMDVector3f diff(vec_ACenter - vec_BCenter);
    float len = diff.Length();
    
    if (len > (ARadius + BRadius))
        return false;

    return true;
}

bool CollisionDetection::SphereIntersectsSphere(const Vector3f &ACenter, const float ARadius, const Vector3f &BCenter, const float BRadius, Vector3f &contactNormal, Vector3f &contactPoint)
{
    SIMDVector3f vec_ACenter(ACenter);
    SIMDVector3f vec_BCenter(BCenter);

    SIMDVector3f diff(vec_ACenter - vec_BCenter);
    float len = diff.Length();

    if (len > (ARadius + BRadius))
        return false;

    SIMDVector3f normalOnSurfaceB(1.0f, 0.0f, 0.0f);
    if (len > Math::Epsilon<float>())
        normalOnSurfaceB = diff / len;

    contactNormal = normalOnSurfaceB;
    contactPoint = vec_BCenter + normalOnSurfaceB * BRadius;
    return true;
}

bool CollisionDetection::SphereIntersectsPlane(const Vector3f &sphereCenter, const float sphereRadius, const Vector3f &planeNormal, const float planeDistance)
{
    return false;
}

bool CollisionDetection::SphereIntersectsPlane(const Vector3f &sphereCenter, const float sphereRadius, const Vector3f &planeNormal, const float planeDistance, Vector3f &contactNormal, Vector3f &contactPoint)
{
    return false;
}

bool CollisionDetection::SphereIntersectsTriangle(const Vector3f &sphereCenter, const float sphereRadius, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1, const Vector3f &normal)
{
    SIMDVector3f vec_sphereCenter(sphereCenter);
    SIMDVector3f vec_normal(normal);
    SIMDVector3f p1ToCenter(vec_sphereCenter - SIMDVector3f(v0));
    float distanceFromPlane = p1ToCenter.Dot(vec_normal);

    if (distanceFromPlane < 0.0f)
    {
        distanceFromPlane = -distanceFromPlane;
        vec_normal = -vec_normal;
    }

    if (distanceFromPlane <= sphereRadius && PointInTriangle(sphereCenter, v0, e0, e1, vec_normal))
    {
        SIMDVector3f contactPoint(vec_sphereCenter - vec_normal * distanceFromPlane);
        return ((vec_sphereCenter - contactPoint).SquaredLength() <= Math::Square(sphereRadius));
    }

    return false;
}

bool CollisionDetection::SphereIntersectsTriangle(const Vector3f &sphereCenter, const float sphereRadius, const Vector3f &v0, const Vector3f &v1, const Vector3f &v2, const Vector3f &e0, const Vector3f &e1, const Vector3f &normal, Vector3f &contactNormal, Vector3f &contactPoint)
{
    SIMDVector3f vec_sphereCenter(sphereCenter);
    SIMDVector3f vec_normal(normal);
    SIMDVector3f p1ToCenter(vec_sphereCenter - SIMDVector3f(v0));
    float distanceFromPlane = p1ToCenter.Dot(vec_normal);

    if (distanceFromPlane < 0.0f)
    {
        distanceFromPlane = -distanceFromPlane;
        vec_normal = -vec_normal;
    }

    if (distanceFromPlane <= sphereRadius && PointInTriangle(sphereCenter, v0, e0, e1, vec_normal))
    {
        SIMDVector3f vec_contactPoint(vec_sphereCenter - vec_normal * distanceFromPlane);
        SIMDVector3f vec_contactToCenter(vec_sphereCenter - vec_contactPoint);
        float squaredDistance = vec_contactToCenter.SquaredLength();
        if (squaredDistance <= Math::Square(sphereRadius))
        {
            float distance = Math::Sqrt(squaredDistance);
            contactNormal = vec_contactToCenter / distance;
            contactPoint = vec_contactPoint;
            return true;
        }
    }

    return false;
}

float CollisionDetection::AABoxSweep(const Vector3f &movingBoxMinBounds, const Vector3f &movingBoxMaxBounds, const Vector3f &movingBoxDisplacement, const Vector3f &staticBoxMinBounds, const Vector3f &staticBoxMaxBounds)
{
    //float3 AExtents(AMaxBounds - AMinBounds);
    //float3 BExtents(BMaxBounds - BMinBounds);
    //float3 ACenter
    float u0 = -Y_FLT_INFINITE;
    float u1 = Y_FLT_INFINITE;

    for (uint32 axis = 0; axis < 3; axis++)
    {
        if (staticBoxMaxBounds[axis] < movingBoxMinBounds[axis])
        {
            if (movingBoxDisplacement[axis] < 0.0f)
            {
                float t0 = (staticBoxMaxBounds[axis] - movingBoxMinBounds[axis]) / movingBoxDisplacement[axis];
                if (t0 > u0)
                    u0 = t0;
            }
            else
            {
                return Y_FLT_INFINITE;
            }
        }
        else if (movingBoxMaxBounds[axis] < staticBoxMinBounds[axis])
        {
            if (movingBoxDisplacement[axis] > 0.0f)
            {
                float t0 = (staticBoxMinBounds[axis] - movingBoxMaxBounds[axis]) / movingBoxDisplacement[axis];
                if (t0 > u0)
                    u0 = t0;
            }
            else
            {
                return Y_FLT_INFINITE;
            }
        }

        if (movingBoxMaxBounds[axis] > staticBoxMinBounds[axis] && movingBoxDisplacement[axis] < 0)
        {
            float t1 = (staticBoxMinBounds[axis] - movingBoxMaxBounds[axis]) / movingBoxDisplacement[axis];
            if (t1 < u1)
                u1 = t1;
        }
        else if (staticBoxMaxBounds[axis] > movingBoxMinBounds[axis] && movingBoxDisplacement[axis] > 0)
        {
            float t1 = (staticBoxMaxBounds[axis] - movingBoxMinBounds[axis]) / movingBoxDisplacement[axis];
            if (t1 < u1)
                u1 = t1;
        }
    }

    if (u0 <= u1 && u0 >= 0.0f && u0 <= 1.0f)
        return movingBoxDisplacement.Length() * u0;
    else
        return Y_FLT_INFINITE;
}

