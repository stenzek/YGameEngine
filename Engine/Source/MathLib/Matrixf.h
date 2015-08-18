#pragma once
#include "MathLib/Vectorf.h"
#include "MathLib/Angle.h"
#include "YBaseLib/Assert.h"

struct Quaternion;
struct Matrix3x3f;
struct Matrix3x4f;
struct Matrix4x4f;
struct Matrix2x2f;
struct SIMDMatrix3x3f;
struct SIMDMatrix3x4f;
struct SIMDMatrix4x4f;

struct Matrix3x3f
{
    Matrix3x3f() {}

    // set everything at once
    Matrix3x3f(const float E00, const float E01, const float E02,
             const float E10, const float E11, const float E12,
             const float E20, const float E21, const float E22);

    // sets the diagonal to the specified value (1 == identity)
    Matrix3x3f(const float Diagonal);

    // assumes row-major packing order with vectors in columns
    Matrix3x3f(const float *p);
    Matrix3x3f(const float p[3][3]);

    // copy
    Matrix3x3f(const Matrix3x3f &v);

    // downscaling/from floatx
    Matrix3x3f(const SIMDMatrix3x3f &v);
    Matrix3x3f(const SIMDMatrix4x4f &v);
    Matrix3x3f(const Matrix4x4f &v);
   
    // setters
    void Set(const float E00, const float E01, const float E02,
             const float E10, const float E11, const float E12,
             const float E20, const float E21, const float E22);
    void SetIdentity();
    void SetZero();

    // column accessors
    Vector3f GetColumn(uint32 j) const;
    void SetColumn(uint32 j, const Vector3f &v);

    // for converting to/from opengl matrices
    void SetTranspose(const float *pElements);
    void GetTranspose(float *pElements) const;

    // new matrix
    Matrix3x3f Inverse() const;
    Matrix3x3f Transpose() const;
    float Determinant() const;

    // in-place
    void InvertInPlace();
    void TransposeInPlace();

    // to floatx
    SIMDMatrix3x3f GetMatrix3() const;

    // row/field accessors
    const Vector3f &operator[](uint32 i) const { DebugAssert(i < 3); return reinterpret_cast<const Vector3f &>(Row[i]); }
    const Vector3f &GetRow(uint32 i) const { DebugAssert(i < 3); return reinterpret_cast<const Vector3f &>(Row[i]); }
    const float operator()(uint32 i, uint32 j) const { DebugAssert(i < 3 && j < 3); return Row[i][j]; }
    Vector3f &operator[](uint32 i) { DebugAssert(i < 3); return reinterpret_cast<Vector3f &>(Row[i]); }
    void SetRow(uint32 i, const Vector3f &v) { DebugAssert(i < 3); reinterpret_cast<Vector3f &>(Row[i]) = v; }
    float &operator()(uint32 i, uint32 j) { DebugAssert(i < 3 && j < 3); return Row[i][j]; }

    // assignment operators
    Matrix3x3f &operator=(const Matrix3x3f &v);
    Matrix3x3f &operator=(const float Diagonal);
    Matrix3x3f &operator=(const float *p);
    Matrix3x3f &operator=(const SIMDMatrix3x3f &v);

    // various operators
    Matrix3x3f &operator+=(const Matrix3x3f &v);
    Matrix3x3f &operator+=(const float v);
    Matrix3x3f &operator*=(const Matrix3x3f &v);
    Matrix3x3f &operator*=(const float v);
    Matrix3x3f operator+(const Matrix3x3f &v) const;
    Matrix3x3f operator+(const float v) const;
    Matrix3x3f operator*(const Matrix3x3f &v) const;
    Matrix3x3f operator*(float v) const;
    Matrix3x3f operator-() const;

    // matrix * vector
    Vector3f operator*(const Vector3f &v) const;

    // constants
    static const Matrix3x3f &Zero, &Identity;

public:
    union
    {
        float Elements[9];
        float Row[3][3];
        struct
        {
            float m00, m01, m02;
            float m10, m11, m12;
            float m20, m21, m22;
        };
    };
};

struct Matrix4x4f
{
    Matrix4x4f() {}

    // set everything at once
    Matrix4x4f(const float E00, const float E01, const float E02, const float E03,
             const float E10, const float E11, const float E12, const float E13,
             const float E20, const float E21, const float E22, const float E23,
             const float E30, const float E31, const float E32, const float E33);

    // sets the diagonal to the specified value (1 == identity)
    Matrix4x4f(const float Diagonal);

    // assumes row-major packing order with vectors in columns
    Matrix4x4f(const float *p);
    Matrix4x4f(const float p[4][4]);

    // copy
    Matrix4x4f(const Matrix4x4f &v);

    // downscaling/from floatx
    Matrix4x4f(const SIMDMatrix4x4f &v);

    // import from float3x4
    Matrix4x4f(const Matrix3x4f &v);
   
    // setters
    void Set(const float E00, const float E01, const float E02, const float E03,
             const float E10, const float E11, const float E12, const float E13,
             const float E20, const float E21, const float E22, const float E23,
             const float E30, const float E31, const float E32, const float E33);
    void SetIdentity();
    void SetZero();

    // column accessors
    Vector4f GetColumn(uint32 j) const;
    void SetColumn(uint32 j, const Vector4f &v);

    // for converting to/from opengl matrices
    void SetTranspose(const float *pElements);
    void GetTranspose(float *pElements) const;

    // new matrix
    Matrix4x4f Inverse() const;
    Matrix4x4f Transpose() const;
    float Determinant() const;

    // in-place
    void InvertInPlace();
    void TransposeInPlace();

    // to floatx
    SIMDMatrix4x4f GetMatrix4f() const;

    // transform vec3
    Vector3f TransformPoint(const Vector3f &point) const;
    Vector3f TransformNormal(const Vector3f &normal, bool normalize = true) const;

    // decompose
    void Decompose(Vector3f &translation, Quaternion &rotation, Vector3f &scale) const;

    // row/field accessors
    const Vector4f &operator[](uint32 i) const { DebugAssert(i < 4); return reinterpret_cast<const Vector4f &>(Row[i]); }
    const Vector4f &GetRow(uint32 i) const { DebugAssert(i < 4); return reinterpret_cast<const Vector4f &>(Row[i]); }
    const float operator()(uint32 i, uint32 j) const { DebugAssert(i < 4 && j < 4); return Row[i][j]; }
    Vector4f &operator[](uint32 i) { DebugAssert(i < 4); return reinterpret_cast<Vector4f &>(Row[i]); }
    void SetRow(uint32 i, const Vector4f &v) { DebugAssert(i < 4); reinterpret_cast<Vector4f &>(Row[i]) = v; }
    float &operator()(uint32 i, uint32 j) { DebugAssert(i < 4 && j < 4); return Row[i][j]; }

    // assignment operators
    Matrix4x4f &operator=(const Matrix4x4f &v);
    Matrix4x4f &operator=(const float Diagonal);
    Matrix4x4f &operator=(const float *p);
    Matrix4x4f &operator=(const SIMDMatrix4x4f &v);

    // various operators
    Matrix4x4f &operator+=(const Matrix4x4f &v);
    Matrix4x4f &operator+=(const float v);
    Matrix4x4f &operator*=(const Matrix4x4f &v);
    Matrix4x4f &operator*=(const float v);
    Matrix4x4f operator+(const Matrix4x4f &v) const;
    Matrix4x4f operator+(const float v) const;
    Matrix4x4f operator*(const Matrix4x4f &v) const;
    Matrix4x4f operator*(float v) const;
    Matrix4x4f operator-() const;

    // matrix * vector
    Vector4f operator*(const Vector4f &v) const;

    // constants
    static const Matrix4x4f &Zero, &Identity;

    // matrix constructors
    static Matrix4x4f MakeScaleMatrix(float s);
    static Matrix4x4f MakeScaleMatrix(const Vector3f &s);
    static Matrix4x4f MakeScaleMatrix(float x, float y, float z);
    static Matrix4x4f MakeRotationMatrixX(Angle angle);
    static Matrix4x4f MakeRotationMatrixY(Angle angle);
    static Matrix4x4f MakeRotationMatrixZ(Angle angle);
    static Matrix4x4f MakeRotationFromEulerAnglesMatrix(const Vector3f &r);
    static Matrix4x4f MakeRotationFromEulerAnglesMatrix(Angle x, Angle y, Angle z);
    static Matrix4x4f MakeTranslationMatrix(const Vector3f &t);
    static Matrix4x4f MakeTranslationMatrix(float x, float y, float z);
    static Matrix4x4f MakePerspectiveProjectionMatrix(float fovY, float aspect, float zNear, float zFar);
    static Matrix4x4f MakeOrthographicProjectionMatrix(float width, float height, float zNear, float zFar);
    static Matrix4x4f MakeOrthographicOffCenterProjectionMatrix(float left, float right, float bottom, float top, float zNear, float zFar);
    static Matrix4x4f MakeCoordinateSystemConversionMatrix(COORDINATE_SYSTEM fromSystem, COORDINATE_SYSTEM toSystem, bool *pReverseWinding);
    static Matrix4x4f MakeLookAtViewMatrix(const Vector3f &eye, const Vector3f &at, const Vector3f &up);
    static Matrix4x4f MakeCubeViewMatrix(uint32 face);
    static Matrix4x4f MakeCubeViewMatrix(uint32 face, const Vector3f &position);

public:
    union
    {
        float Elements[16];
        float Row[4][4];
        struct
        {
            float m00, m01, m02, m03;
            float m10, m11, m12, m13;
            float m20, m21, m22, m23;
            float m30, m31, m32, m33;
        };
    };
};

// float3x4 matrix has special behaviour when doing multiplication, there is an implicit { 0, 0, 0, 1 } fourth row
struct Matrix3x4f
{
    Matrix3x4f() {}

    // set everything at once
    Matrix3x4f(const float &E00, const float &E01, const float &E02, const float &E03,
             const float &E10, const float &E11, const float &E12, const float &E13,
             const float &E20, const float &E21, const float &E22, const float &E23);

    // assumes row-major packing order with vectors in columns
    Matrix3x4f(const float *p);
    Matrix3x4f(const float p[3][4]);

    // copy
    Matrix3x4f(const Matrix3x4f &v);

    // downscaling/from floatx
    Matrix3x4f(const SIMDMatrix3x4f &v);

    // from 4x4 matrix
    Matrix3x4f(const Matrix4x4f &m);
   
    // setters
    void Set(const float &E00, const float &E01, const float &E02, const float &E03,
             const float &E10, const float &E11, const float &E12, const float &E13,
             const float &E20, const float &E21, const float &E22, const float &E23);
    void SetIdentity();
    void SetZero();

    // column accessors
    Vector3f GetColumn(uint32 j) const;
    void SetColumn(uint32 j, const Vector3f &v);

    // for converting to/from opengl matrices
    void SetTranspose(const float *pElements);
    void GetTranspose(float *pElements) const;

    // to floatx
    SIMDMatrix3x4f GetMatrix3x4() const;

    // transform vec3
    Vector3f TransformPoint(const Vector3f &point) const;
    Vector3f TransformNormal(const Vector3f &normal, bool normalize = true) const;

    // load/store
    void Load(const float *pValues);
    void Store(float *pValues);

    // row/field accessors
    const Vector4f &operator[](uint32 i) const { DebugAssert(i < 3); return reinterpret_cast<const Vector4f &>(Row[i]); }
    const Vector4f &GetRow(uint32 i) const { DebugAssert(i < 3); return reinterpret_cast<const Vector4f &>(Row[i]); }
    const float &operator()(uint32 i, uint32 j) const { DebugAssert(i < 3 && j < 4); return Row[i][j]; }
    Vector4f &operator[](uint32 i) { DebugAssert(i < 3); return reinterpret_cast<Vector4f &>(Row[i]); }
    void SetRow(uint32 i, const Vector4f &v) { DebugAssert(i < 3); reinterpret_cast<Vector4f &>(Row[i]) = v; }
    float &operator()(uint32 i, uint32 j) { DebugAssert(i < 3 && j < 4); return Row[i][j]; }

    // assignment operators
    Matrix3x4f &operator=(const Matrix3x4f &v);
    Matrix3x4f &operator=(const float *p);
    Matrix3x4f &operator=(const SIMDMatrix3x4f &v);

    // various operators
    Matrix3x4f &operator+=(const Matrix3x4f &v);
    Matrix3x4f &operator+=(const float v);
    Matrix3x4f &operator*=(const Matrix3x4f &v);
    Matrix3x4f &operator*=(const float v);
    Matrix3x4f operator+(const Matrix3x4f &v) const;
    Matrix3x4f operator+(const float v) const;
    Matrix3x4f operator*(const Matrix3x4f &v) const;
    Matrix3x4f operator*(float v) const;
    Matrix3x4f operator-() const;

    // matrix * vector
    Vector3f operator*(const Vector3f &v) const;

    // constants
    static const Matrix3x4f &Zero, &Identity;

public:
    union
    {
        float Elements[12];
        float Row[3][4];
        struct
        {
            float m00, m01, m02, m03;
            float m10, m11, m12, m13;
            float m20, m21, m22, m23;
        };
    };
};

#define DEFINE_CONST_MATRIX4X4F(name, m00, m01, m02, m03, \
                                           m10, m11, m12, m13, \
                                           m20, m21, m22, m23, \
                                           m30, m31, m32, m33) \
                                     static const float __data_##name[16] = { \
                                        m00, m01, m02, m03, \
                                        m10, m11, m12, m13, \
                                        m20, m21, m22, m23, \
                                        m30, m31, m32, m33 \
                                     }; \
                                     const float4x4 &name = reinterpret_cast<const float4x4 &>(__data_##name);

#define DECLARE_CONST_MATRIX4X4F(name) const float4x4 &name;
