#pragma once
#include "MathLib/Vectorf.h"
#include "MathLib/Plane.h"

// Line is a line in 3D space, with a point, spanning infinite distance in +/- directions of Direction.
struct Line
{
    Line() {}
    Line(const Vector3f &Point_, const Vector3f &Direction_) : Point(Point_), Direction(Direction_) {}
    Line(const Line &v) : Point(v.Point), Direction(v.Direction) {}
    
    // Set from a point and direction.
    void Set(const Vector3f &Point_, const Vector3f &Direction_) { Point = Point_; Direction = Direction_; }

    // Set from two points.
    void SetFromTwoPoints(const Vector3f &Point1, const Vector3f &Point2)
    {
        Point = Point1;
        Direction = (Point2 - Point1).SafeNormalize();
    }

    // Compute line<->line intersection. If an intersection cannot be found, an infinite vector is returned.
    Vector3f LineIntersection(const Line &LineB, const float epsilon = Y_FLT_EPSILON) const
    {
        const Vector3f &dir1 = this->Direction;
        const Vector3f &dir2 = LineB.Direction;
        const Vector3f &point1 = this->Point;
        const Vector3f &point2 = LineB.Point;
        
        float t;

        if (Y_fabs(dir1.y * dir2.x - dir1.x * dir2.y) > epsilon)
        {
            t = (-point1.y * dir2.x + point2.y * dir2.x + dir2.y * point1.x-dir2.y * point2.x) / (dir1.y * dir2.x - dir1.x * dir2.y);
        }
        else if (Y_fabs(-dir1.x * dir2.z + dir1.z * dir2.x) > epsilon)
        {
            t=-(-dir2.z * point1.x + dir2.z * point2.x + dir2.x * point1.z - dir2.x * point2.z) / (-dir1.x * dir2.z + dir1.z * dir2.x);
        }
        else if (Y_fabs(-dir1.z * dir2.y + dir1.y * dir2.z) > epsilon)
        {
            t = (point1.z * dir2.y - point2.z * dir2.y - dir2.z * point1.y + dir2.z * point2.y) / (-dir1.z * dir2.y + dir1.y * dir2.z);
        }
        else
        {
            return Vector3f::Infinite;
        }

        return dir1 * t + point1;
    }

    // Compute line<->plane intersection. If an intersection cannot be found, an infinite vector is returned.
    Vector3f PlaneIntersection(const Plane &plane, const float epsilon = Y_FLT_EPSILON) const
    {
        const float &A = plane.a;
        const float &B = plane.b;
        const float &C = plane.c;
        const float &D = plane.d;

        float numerator = A * this->Point.x + B * this->Point.y + C * this->Point.z + D;
        float denominator = A * this->Direction.x + B * this->Direction.y + C * this->Direction.z;

        //if line is parallel to the plane...
        if (Y_fabs(denominator) < epsilon)
        {
            // if line is contained in the plane...
            if (Y_fabs(numerator) < epsilon)
                return this->Point;
            else
                return Vector3f::Infinite;
        }
        else
        {
            // if line intercepts the plane
            float t = -numerator / denominator;
            return this->Direction * t + Point;
        }
    }

    // Compute point to point distance.
    float PointToPointDistance(const Line &l) const { return PointToPointDistance(l.Point); }
    float PointToPointDistance(const Vector3f &v) const
    {
        Vector3f temp = v - this->Point;
        float length = temp.Length();
        if (length == 0.0f)
            return 0.0f;
        
        // normalize temp
        temp /= length;
        if (temp.Dot(this->Direction) < 0.0f)
            return -length;
        else
            return length;
    }

    // Finds closest point from line to point.
    //Vector3 ClosestPointOnLine(const MLVECTOR3 &Point) const;

    // Finds the distance from point to line.
    //float DistanceFromPoint(const MLVECTOR3 &Point) const;

    // operators
    bool operator==(const Line &v) const { return (Point == v.Point && Direction == v.Direction); }
    bool operator!=(const Line &v) const { return (Point != v.Point || Direction != v.Direction); }
    Line &operator=(const Line &v) { Point = v.Point; Direction = v.Direction;  return *this; }

public:
    Vector3f Point;
    Vector3f Direction;
};

