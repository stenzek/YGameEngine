#include "MathLib/Plane.h"
#include "MathLib/SIMDVectorf.h"

Plane Plane::Normalize()
{
    Plane ret;
    float Length = Y_sqrtf(a * a + b * b + c * c);
    ret.a = a / Length; 
    ret.b = b / Length;
    ret.c = c / Length;
    ret.d = d / Length;
    return ret;
}

Plane Plane::NormalizeEst()
{
    Plane ret;
    float invLength = Y_rsqrtf(a * a + b * b + c * c);
    ret.a = a * invLength;
    ret.b = b * invLength;
    ret.c = c * invLength;
    ret.d = d * invLength;
    return ret;
}

void Plane::NormalizeInPlace()
{
    float Length = Y_sqrtf(a * a + b * b + c * c);
    a /= Length; b /= Length; c /= Length; d /= Length;
}

void Plane::NormalizeEstInPlace()
{
    float invLength = Y_rsqrtf(a * a + b * b + c * c);
    a *= invLength; b *= invLength; c *= invLength; d *= invLength;
}

float Plane::Distance(const Vector3f &p) const
{
    return a * p.x + b * p.y + c * p.z + d;
}

Plane Plane::FromPointAndNormal(const Vector3f &Point, const Vector3f &Normal)
{
    Plane ret;

    // normal == point
    ret.a = Point.x; ret.b = Point.y; ret.c = Point.z;

    // d = -dot(normal, point)
    ret.d = -(Normal.x * Point.x + Normal.y * Point.y + Normal.z * Point.z);

    // normalize the plane
    return ret.Normalize();
}

Plane Plane::FromTriangle(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2)
{
    Plane ret;
    ret.Set(v0.y * (v1.z - v2.z) + v1.y * (v2.z - v0.z) + v2.y * (v0.z - v1.z),
            v0.z * (v1.x - v2.x) + v1.z * (v2.x - v0.x) + v2.z * (v0.x - v1.x),
            v0.x * (v1.y - v2.y) + v1.x * (v2.y - v0.y) + v2.x * (v0.y - v1.y),
            -(v0.x * (v1.y * v2.z - v2.y * v1.z) + v1.x * (v2.y * v0.z - v0.y * v2.z) + v2.x * (v0.y * v1.z - v1.y * v0.z)));

    return ret.Normalize();
}

Vector3f Plane::CalculateThreePlaneIntersectionPoint(const Plane &p1, const Plane &p2, const Plane &p3)
{
    SIMDVector3f p1Normal(p1.GetNormal());
    SIMDVector3f p2Normal(p2.GetNormal());
    SIMDVector3f p3Normal(p3.GetNormal());
    float p1Distance = p1.GetDistance();
    float p2Distance = p2.GetDistance();
    float p3Distance = p3.GetDistance();

    return Vector3f(p2Normal.Cross(p3Normal) * -p1Distance / p1Normal.Dot(p2Normal.Cross(p3Normal))
                  - p3Normal.Cross(p1Normal) * p2Distance / p2Normal.Dot(p3Normal.Cross(p1Normal))
                  - p1Normal.Cross(p2Normal) * p3Distance / p3Normal.Dot(p1Normal.Cross(p2Normal)));
}
