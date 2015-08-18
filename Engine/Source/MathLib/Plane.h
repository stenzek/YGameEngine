#pragma once
#include "MathLib/Common.h"
#include "MathLib/Vectorf.h"

class Plane
{
public:
    Plane() {}
    Plane(const Vector3f &normal, const float dist) : a(normal.x), b(normal.y), c(normal.z), d(dist) {}
    Plane(const float &a_, const float &b_, const float &c_, const float &d_) : a(a_), b(b_), c(c_), d(d_) {}
    Plane(const float *p) : a(p[0]), b(p[1]), c(p[2]), d(p[3]) {}
    Plane(const Plane &v) : a(v.a), b(v.b), c(v.c), d(v.d) {}

    // convert plane to vector
    const Vector3f &GetNormal() const { return reinterpret_cast<const Vector3f &>(a); }
    const float &GetDistance() const { return d; }

    // setters
    void Set(const float &a_, const float &b_, const float &c_, const float &d_) { a = a_; b = b_; c = c_; d = d_; }

    Plane Normalize();

    Plane NormalizeEst();
    
    void NormalizeInPlace();

    void NormalizeEstInPlace();

    // distance from the plane to the specified point.
    float Distance(const Vector3f &p) const;

    // compare operators
    bool operator==(const Plane &v) const { return a == v.a && b == v.b && c == v.c && d == v.d; }
    bool operator!=(const Plane &v) const { return a != v.a && b != v.b && c != v.c && d != v.d; }  

    // modifies this plane
    Plane &operator=(const Plane &v) { a = v.a; b = v.b; c = v.c; d = v.d; return *this; }
    Plane &operator=(const float *p) { a = p[0]; b = p[1]; c = p[2]; d = p[3]; return *this; }

    // calculate plane from a point and a normal
    static Plane FromPointAndNormal(const Vector3f &Point, const Vector3f &Normal);

    // calculate plane from a triangle
    static Plane FromTriangle(const Vector3f &v0, const Vector3f &v1, const Vector3f &v2);

    // calculate intersection point of three planes
    static Vector3f CalculateThreePlaneIntersectionPoint(const Plane &p1, const Plane &p2, const Plane &p3);

public:
    float a, b, c, d;   
};
