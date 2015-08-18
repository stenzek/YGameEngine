#include "MathLib/Matrixf.h"
#include "MathLib/SIMDVectorf.h"

struct Matrix2x2f;
struct Matrix3x3f;
struct Matrix3x4f;
struct Matrix4x4f;

struct SIMDMatrix3x3f : public Matrix3x3f
{
    SIMDMatrix3x3f() {}

    // set everything at once
    SIMDMatrix3x3f(const float E00, const float E01, const float E02,
                 const float E10, const float E11, const float E12,
                 const float E20, const float E21, const float E22)
        : Matrix3x3f(E00, E01, E02,
                     E10, E11, E12,
                     E20, E21, E22) {}

    // sets the diagonal to the specified value (1 == identity)
    SIMDMatrix3x3f(const float Diagonal) : Matrix3x3f(Diagonal) {}

    // assumes row-major packing order with vectors in columns
    SIMDMatrix3x3f(const float *p) : Matrix3x3f(p) {}

    // copy
    SIMDMatrix3x3f(const SIMDMatrix3x3f &v) : Matrix3x3f(v) {}

    // downscaling/from floatx
    SIMDMatrix3x3f(const Matrix3x3f &v) : Matrix3x3f(v) {}
   
    // row/field accessors
    const float operator()(uint32 i, uint32 j) const { return Matrix3x3f::operator()(i, j); }
    float &operator()(uint32 i, uint32 j) { return Matrix3x3f::operator()(i, j); }

    // assignment operators
    SIMDMatrix3x3f &operator=(const SIMDMatrix3x3f &v) { Matrix3x3f::operator=(v); return *this; }
    SIMDMatrix3x3f &operator=(const float Diagonal) { Matrix3x3f::operator=(Diagonal); return *this; }
    SIMDMatrix3x3f &operator=(const float *p) { Matrix3x3f::operator=(p); return *this; }
    SIMDMatrix3x3f &operator=(const Matrix3x3f &v) { Matrix3x3f::operator=(v); return *this; }

    // various operators
    SIMDMatrix3x3f &operator*=(const SIMDMatrix3x3f &v) { Matrix3x3f::operator*=(v); return *this; }
    SIMDMatrix3x3f &operator*=(const float v) { Matrix3x3f::operator*=(v); return *this; }
    SIMDMatrix3x3f operator*(const SIMDMatrix3x3f &v) const { return Matrix3x3f::operator*(v); }
    SIMDMatrix3x3f operator*(float v) const { return Matrix3x3f::operator*(v); }
    SIMDMatrix3x3f operator-() const { return Matrix3x3f::operator-(); }

    // matrix * vector
    SIMDVector3f operator*(const SIMDVector3f &v) const { return Matrix3x3f::operator*(v); }

    // constants
    static const SIMDMatrix3x3f &Zero, &Identity;
};

struct SIMDMatrix4x4f : public Matrix4x4f
{
    SIMDMatrix4x4f() {}

    // set everything at once
    SIMDMatrix4x4f(const float E00, const float E01, const float E02, const float E03,
                 const float E10, const float E11, const float E12, const float E13,
                 const float E20, const float E21, const float E22, const float E23,
                 const float E30, const float E31, const float E32, const float E33)
        : Matrix4x4f(E00, E01, E02, E03,
                     E10, E11, E12, E13,
                     E20, E21, E22, E23,
                     E30, E31, E32, E33) {}

    // sets the diagonal to the specified value (1 == identity)
    SIMDMatrix4x4f(const float Diagonal) : Matrix4x4f(Diagonal) {}

    // assumes row-major packing order with vectors in columns
    SIMDMatrix4x4f(const float *p) : Matrix4x4f(p) {}

    // copy
    SIMDMatrix4x4f(const SIMDMatrix4x4f &v) : Matrix4x4f(v) {}

    // downscaling/from floatx
    SIMDMatrix4x4f(const Matrix4x4f &v) : Matrix4x4f(v) {}
   
    // row/field accessors
    const float operator()(uint32 i, uint32 j) const { return Matrix4x4f::operator()(i, j); }
    float &operator()(uint32 i, uint32 j) { return Matrix4x4f::operator()(i, j); }

    // assignment operators
    SIMDMatrix4x4f &operator=(const SIMDMatrix4x4f &v) { Matrix4x4f::operator=(v); return *this; }
    SIMDMatrix4x4f &operator=(const float Diagonal) { Matrix4x4f::operator=(Diagonal); return *this; }
    SIMDMatrix4x4f &operator=(const float *p) { Matrix4x4f::operator=(p); return *this; }
    SIMDMatrix4x4f &operator=(const Matrix4x4f &v) { Matrix4x4f::operator=(v); return *this; }

    // various operators
    SIMDMatrix4x4f &operator*=(const SIMDMatrix4x4f &v) { Matrix4x4f::operator*=(v); return *this; }
    SIMDMatrix4x4f &operator*=(const float v) { Matrix4x4f::operator*=(v); return *this; }
    SIMDMatrix4x4f operator*(const SIMDMatrix4x4f &v) const { return Matrix4x4f::operator*(v); }
    SIMDMatrix4x4f operator*(float v) const { return Matrix4x4f::operator*(v); }
    SIMDMatrix4x4f operator-() const { return Matrix4x4f::operator-(); }

    // matrix * vector
    SIMDVector4f operator*(const SIMDVector4f &v) const { return Matrix4x4f::operator*(v); }

    // constants
    static const SIMDMatrix4x4f &Zero, &Identity;
};
