#pragma once
#include "MathLib/Common.h"
#include "MathLib/Vectorf.h"
#include "MathLib/Matrixf.h"
#include "MathLib/SIMDVectorf.h"
#include "MathLib/SIMDMatrixf.h"
#include "MathLib/Angle.h"

struct Quaternion
{
    Quaternion() {}
    Quaternion(const float &x_, const float &y_, const float &z_, const float &w_) : x(x_), y(y_), z(z_), w(w_) {}
    Quaternion(const Vector4f &p) : x(p.x), y(p.y), z(p.z), w(p.w) {}
    Quaternion(const Quaternion &v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

    // load/store
    void Load(const float *v) { x = v[0]; y = v[1]; z = v[2]; w = v[3]; }
    void Store(float *v) const { v[0] = x; v[1] = y; v[2] = z; v[3] = w; }

    // setters
    void Set(const float &x_, const float &y_, const float &z_, const float &w_) { x = x_; y = y_; z = z_; w = w_; }
    void Set(const Vector4f &floatvec) { x = floatvec.x; y = floatvec.y; z = floatvec.z; w = floatvec.w; }
    void SetIdentity() { x = 0.0f; y = 0.0f; z = 0.0f; w = 1.0f; }

    // scalar functions
    float Length() const { return Y_sqrtf(x * x + y * y + z * z + w * w); }
    float Dot(const Quaternion &v) const { return x * x + y * y + z * z + w * w; }

    // access as array
    //const float &operator[](uint32 i) const { return (&x)[i]; }
    //float &operator[](uint32 i) { return (&x)[i]; }
    operator const float *() const { return &x; }
    operator float *() { return &x; }

    // quat functions
    Quaternion Normalize() const;
    Quaternion NormalizeEst() const;
    Quaternion Conjugate() const { return Quaternion(-x, -y, -z, w); }
    Quaternion Inverse() const { return Conjugate(); }

    // in-place functions
    void NormalizeInPlace();
    void NormalizeEstInPlace();
    void ConjugateInPlace();
    void InvertInPlace();

    // creates new quaternion
    Quaternion operator*(const Quaternion &q) const;

    // affects this quaternion
    Quaternion &operator*=(const Quaternion &q);
    Quaternion &operator=(const Quaternion &q) { x = q.x; y = q.y; z = q.z; w = q.w; return *this; }

    // comparitors
    bool operator==(const Quaternion &q) const;
    bool operator!=(const Quaternion &q) const;

    // rotate a vector
    SIMDVector3f operator*(const SIMDVector3f &v) const;
    Vector3f operator*(const Vector3f &v) const;

    // transforming back
    void GetAxisAngle(Vector3f &axis, Angle &angle) const;
    void GetAxes(Vector3f &xAxis, Vector3f &yAxis, Vector3f &zAxis) const;
    Vector3f GetEulerAngles() const;
    Vector4f GetVectorRepresentation() const;
    Matrix3x3f GetMatrix3x3() const;
    Matrix3x4f GetMatrix3x4() const;
    Matrix4x4f GetMatrix4x4() const;
    SIMDMatrix3x3f GetSIMDMatrix3() const;
    //SIMDMatrix3x3 GetSIMDMatrix3x4() const;
    SIMDMatrix4x4f GetSIMDMatrix4() const;

    // creators
    static Quaternion MakeRotationX(Angle angle) { return FromNormalizedAxisAngle(Vector3f::UnitX, angle); }
    static Quaternion MakeRotationY(Angle angle) { return FromNormalizedAxisAngle(Vector3f::UnitY, angle); }
    static Quaternion MakeRotationZ(Angle angle) { return FromNormalizedAxisAngle(Vector3f::UnitZ, angle); }
    static Quaternion FromEulerAngles(const Vector3f &v);
    static Quaternion FromEulerAngles(const Angle x, const Angle y, const Angle z);
    static Quaternion FromNormalizedAxisAngle(const Vector3f &axis, Angle angle);
    static Quaternion FromAxisAngle(const Vector3f &axis, Angle angle);
    static Quaternion FromAxes(const Vector3f &xAxis, const Vector3f &yAxis, const Vector3f &zAxis);
    static Quaternion FromFloat3x3(const Matrix3x3f &rotationMatrix);
    static Quaternion FromFloat3x4(const Matrix3x4f &rotationMatrix);
    static Quaternion FromFloat4x4(const Matrix4x4f &rotationMatrix);
    static Quaternion FromTwoVectors(const Vector3f &u, const Vector3f &v);
    static Quaternion FromTwoUnitVectors(const Vector3f &u, const Vector3f &v);

    // interpolation
    static Quaternion LinearInterpolate(const Quaternion &start, const Quaternion &end, float factor);

    // constants
    static const Quaternion &Identity;

public:
    float x, y, z, w;
};


