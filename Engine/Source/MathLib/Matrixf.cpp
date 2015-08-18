#include "MathLib/Matrixf.h"
#include "MathLib/SIMDMatrixf.h"
#include "MathLib/Quaternion.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/Assert.h"

Matrix3x3f::Matrix3x3f(const float E00, const float E01, const float E02,
                   const float E10, const float E11, const float E12,
                   const float E20, const float E21, const float E22)
{
    float *pv = &m00;
    *pv++ = E00; *pv++ = E01; *pv++ = E02;
    *pv++ = E10; *pv++ = E11; *pv++ = E12;
    *pv++ = E20; *pv++ = E21; *pv = E22;
}

Matrix3x3f::Matrix3x3f(const float Diagonal)
{
    float *pv = &m00;
    *pv++ = Diagonal; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = Diagonal; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv = Diagonal;
}

Matrix3x3f::Matrix3x3f(const float *p)
{
    Y_memcpy(Elements, p, sizeof(Elements));
}

Matrix3x3f::Matrix3x3f(const float p[3][3])
{
    Y_memcpy(Elements, p, sizeof(Elements));
}

Matrix3x3f::Matrix3x3f(const Matrix3x3f &v)
{
    Y_memcpy(Elements, v.Elements, sizeof(Elements));
}

Matrix3x3f::Matrix3x3f(const SIMDMatrix3x3f &v)
{
    Y_memcpy(Elements, v.Elements, sizeof(Elements));
}

Matrix3x3f::Matrix3x3f(const SIMDMatrix4x4f &v)
{
    m00 = v.m00; m01 = v.m01; m02 = v.m02;
    m10 = v.m10; m11 = v.m11; m12 = v.m12;
    m20 = v.m20; m21 = v.m21; m22 = v.m22;
}

Matrix3x3f::Matrix3x3f(const Matrix4x4f &v)
{
    m00 = v.m00; m01 = v.m01; m02 = v.m02;
    m10 = v.m10; m11 = v.m11; m12 = v.m12;
    m20 = v.m20; m21 = v.m21; m22 = v.m22;
}

void Matrix3x3f::Set(const float E00, const float E01, const float E02,
                   const float E10, const float E11, const float E12,
                   const float E20, const float E21, const float E22)
{
    float *pv = &m00;
    *pv++ = E00; *pv++ = E01; *pv++ = E02;
    *pv++ = E10; *pv++ = E11; *pv++ = E12;
    *pv++ = E20; *pv++ = E21; *pv = E22;
}

void Matrix3x3f::SetZero()
{
    float *pv = &m00;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv = 0.0f;
}

void Matrix3x3f::SetIdentity()
{
    float *pv = &m00;
    *pv++ = 1.0f; *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 1.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 1.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f; *pv = 1.0f;
}

Vector3f Matrix3x3f::GetColumn(uint32 j) const
{
    DebugAssert(j < 3);
    return Vector3f(Row[0][j], Row[1][j], Row[2][j]);
}

void Matrix3x3f::SetColumn(uint32 j, const Vector3f &v)
{
    DebugAssert(j < 3);
    Row[0][j] = v.x;
    Row[1][j] = v.y;
    Row[2][j] = v.z;
}

void Matrix3x3f::SetTranspose(const float *pElements)
{
    float *pv = &m00;
    *pv++ = pElements[0]; *pv++ = pElements[3]; *pv++ = pElements[6];
    *pv++ = pElements[1]; *pv++ = pElements[4]; *pv++ = pElements[7];
    *pv++ = pElements[2]; *pv++ = pElements[5]; *pv = pElements[8];
}

void Matrix3x3f::GetTranspose(float *pElements) const
{
    float *pv = pElements;
    *pv++ = Row[0][0]; *pv++ = Row[1][0]; *pv++ = Row[2][0];
    *pv++ = Row[0][1]; *pv++ = Row[1][1]; *pv++ = Row[2][1];
    *pv++ = Row[0][2]; *pv++ = Row[1][2]; *pv = Row[2][2];
}

Matrix3x3f Matrix3x3f::Inverse() const
{
    float invDet = 1.0f / (m00 * (m11 * m22 - m21 * m12) - m01 * (m10 * m22 - m12 * m20) + m02 * (m10 * m21 - m11 * m20));
    
    Matrix3x3f res((m11 * m22 - m12 * m21) * invDet, (m21 * m02 - m01 * m22) * invDet, (m01 * m12 - m11 * m02) * invDet,
                 (m12 * m20 - m10 * m22) * invDet, (m00 * m22 - m20 * m02) * invDet, (m10 * m02 - m00 * m12) * invDet,
                 (m10 * m21 - m20 * m11) * invDet, (m20 * m01 - m00 * m21) * invDet, (m00 * m11 - m01 * m10) * invDet);

    return res;
}

Matrix3x3f Matrix3x3f::Transpose() const
{
    Matrix3x3f res;

    res.m00 = m00;
    res.m01 = m10;
    res.m02 = m20;
    res.m10 = m01;
    res.m11 = m11;
    res.m12 = m21;
    res.m20 = m02;
    res.m21 = m12;
    res.m22 = m22;

    return res;
}

float Matrix3x3f::Determinant() const
{
    return (m00 * (m11 * m22 - m21 * m12) - m01 * (m10 * m22 - m12 * m20) + m02 * (m10 * m21 - m11 * m20));
}

void Matrix3x3f::InvertInPlace()
{
    // rvo should take care of this
    Matrix3x3f temp(*this);
    *this = temp.Inverse();
}

void Matrix3x3f::TransposeInPlace()
{
    uint32 i, j;
    for (i = 0; i < 3; i++)
    {
        for (j = i + 1; j < 3; j++)
        {
            if (i == j)
                continue;

            float tmp = Row[j][i];
            Row[j][i] = Row[i][j];
            Row[i][j] = tmp;
        }
    }
}

SIMDMatrix3x3f Matrix3x3f::GetMatrix3() const
{
    return SIMDMatrix3x3f(&m00);
}

Matrix3x3f &Matrix3x3f::operator=(const Matrix3x3f &v)
{
    Y_memcpy(Elements, v.Elements, sizeof(Elements));
    return *this;
}

Matrix3x3f &Matrix3x3f::operator=(const float Diagonal)
{
    float *pv = &m00;
    *pv++ = Diagonal; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = Diagonal; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv = Diagonal;
    return *this;
}

Matrix3x3f &Matrix3x3f::operator=(const float *p)
{
    Y_memcpy(Elements, p, sizeof(Elements));
    return *this;
}

Matrix3x3f &Matrix3x3f::operator=(const SIMDMatrix3x3f &v)
{
    Y_memcpy(Elements, v.Elements, sizeof(Elements));
    return *this;
}

Matrix3x3f &Matrix3x3f::operator*=(const Matrix3x3f &v)
{
    // again, rvo should fix this up
    Matrix3x3f temp(*this);
    *this = temp * v;
    return *this;
}

Matrix3x3f &Matrix3x3f::operator*=(const float v)
{
    uint32 i;
    for (i = 0; i < 9; i++)
        Elements[i] *= v;

    return *this;
}

Matrix3x3f Matrix3x3f::operator*(const Matrix3x3f &v) const
{
#define mulRC(rw, cl) Row[rw][0] * v.Row[0][cl] + \
                      Row[rw][1] * v.Row[1][cl] + \
                      Row[rw][2] * v.Row[2][cl]


    Matrix3x3f res;
    res.m00 = mulRC(0, 0);  res.m01 = mulRC(0, 1);  res.m02 = mulRC(0, 2);
    res.m10 = mulRC(1, 0);  res.m11 = mulRC(1, 1);  res.m12 = mulRC(1, 2);
    res.m20 = mulRC(2, 0);  res.m21 = mulRC(2, 1);  res.m22 = mulRC(2, 2);
    return res;

#undef mulRC
}

Matrix3x3f Matrix3x3f::operator*(float v) const
{
    uint32 i;
    Matrix3x3f res;

    for (i = 0; i < 9; i++)
        res.Elements[i] = Elements[i] * v;

    return res;
}

Matrix3x3f Matrix3x3f::operator-() const
{
    uint32 i;
    Matrix3x3f res;

    for (i = 0; i < 9; i++)
        res.Elements[i] = -Elements[i];

    return res;
}

Vector3f Matrix3x3f::operator*(const Vector3f &v) const
{
    Vector3f res;

    res.x = Row[0][0] * v.x + Row[0][1] * v.y + Row[0][2] * v.z;
    res.y = Row[1][0] * v.x + Row[1][1] * v.y + Row[1][2] * v.z;
    res.z = Row[2][0] * v.x + Row[2][1] * v.y + Row[2][2] * v.z;

    return res;
}

Matrix3x3f &Matrix3x3f::operator+=(const Matrix3x3f &v)
{
    for (uint32 i = 0; i < countof(Elements); i++)
        Elements[i] += v.Elements[i];
    return *this;
}

Matrix3x3f &Matrix3x3f::operator+=(const float v)
{
    for (uint32 i = 0; i < countof(Elements); i++)
        Elements[i] += v;
    return *this;
}

Matrix3x3f Matrix3x3f::operator+(const Matrix3x3f &v) const
{
    Matrix3x3f ret;
    for (uint32 i = 0; i < countof(Elements); i++)
        ret.Elements[i] = Elements[i] + v.Elements[i];
    return ret;
}

Matrix3x3f Matrix3x3f::operator+(const float v) const
{
    Matrix3x3f ret;
    for (uint32 i = 0; i < countof(Elements); i++)
        ret.Elements[i] = Elements[i] + v;
    return ret;
}

// constants
static const float __float3x3Zero[9] = { 0.0f, 0.0f, 0.0f,
                                          0.0f, 0.0f, 0.0f,
                                          0.0f, 0.0f, 0.0f };
const Matrix3x3f &Matrix3x3f::Zero = reinterpret_cast<const Matrix3x3f &>(__float3x3Zero);
static const float __float3x3Identity[9] = { 1.0f, 0.0f, 0.0f,
                                             0.0f, 1.0f, 0.0f,
                                             0.0f, 0.0f, 1.0f };
const Matrix3x3f &Matrix3x3f::Identity = reinterpret_cast<const Matrix3x3f &>(__float3x3Identity);


//========================================================================================================================================================================================================================

Matrix4x4f::Matrix4x4f(const float E00, const float E01, const float E02, const float E03,
                   const float E10, const float E11, const float E12, const float E13,
                   const float E20, const float E21, const float E22, const float E23,
                   const float E30, const float E31, const float E32, const float E33)
{
    float *pv = &m00;
    *pv++ = E00; *pv++ = E01; *pv++ = E02; *pv++ = E03;
    *pv++ = E10; *pv++ = E11; *pv++ = E12; *pv++ = E13;
    *pv++ = E20; *pv++ = E21; *pv++ = E22; *pv++ = E23;
    *pv++ = E30; *pv++ = E31; *pv++ = E32; *pv = E33;
}

Matrix4x4f::Matrix4x4f(const float Diagonal)
{
    float *pv = &m00;
    *pv++ = Diagonal; *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = Diagonal; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = Diagonal; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = Diagonal;
}

Matrix4x4f::Matrix4x4f(const float *p)
{
    Y_memcpy(Elements, p, sizeof(Elements));
}

Matrix4x4f::Matrix4x4f(const float p[4][4])
{
    Y_memcpy(Elements, p, sizeof(Elements));
}

Matrix4x4f::Matrix4x4f(const Matrix4x4f &v)
{
    Y_memcpy(Elements, v.Elements, sizeof(Elements));
}

Matrix4x4f::Matrix4x4f(const SIMDMatrix4x4f &v)
{
    Y_memcpy(Elements, v.Elements, sizeof(Elements));
}

Matrix4x4f::Matrix4x4f(const Matrix3x4f &v)
{
    Row[0][0] = v.Row[0][0];
    Row[0][1] = v.Row[0][1];
    Row[0][2] = v.Row[0][2];
    Row[0][3] = v.Row[0][3];
    Row[1][0] = v.Row[1][0];
    Row[1][1] = v.Row[1][1];
    Row[1][2] = v.Row[1][2];
    Row[1][3] = v.Row[1][3];
    Row[2][0] = v.Row[2][0];
    Row[2][1] = v.Row[2][1];
    Row[2][2] = v.Row[2][2];
    Row[2][3] = v.Row[2][3];
    Row[3][0] = 0.0f;
    Row[3][1] = 0.0f;
    Row[3][2] = 0.0f;
    Row[3][3] = 1.0f;
}

void Matrix4x4f::Set(const float E00, const float E01, const float E02, const float E03,
                   const float E10, const float E11, const float E12, const float E13,
                   const float E20, const float E21, const float E22, const float E23,
                   const float E30, const float E31, const float E32, const float E33)
{
    float *pv = &m00;
    *pv++ = E00; *pv++ = E01; *pv++ = E02; *pv++ = E03;
    *pv++ = E10; *pv++ = E11; *pv++ = E12; *pv++ = E13;
    *pv++ = E20; *pv++ = E21; *pv++ = E22; *pv++ = E23;
    *pv++ = E30; *pv++ = E31; *pv++ = E32; *pv = E33;
}

void Matrix4x4f::SetZero()
{
    float *pv = &m00;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f; *pv = 0.0f;
}

void Matrix4x4f::SetIdentity()
{
    float *pv = &m00;
    *pv++ = 1.0f; *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 1.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 1.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f; *pv = 1.0f;
}

Vector4f Matrix4x4f::GetColumn(uint32 j) const
{
    DebugAssert(j < 4);
    return Vector4f(Row[0][j], Row[1][j], Row[2][j], Row[3][j]);
}

void Matrix4x4f::SetColumn(uint32 j, const Vector4f &v)
{
    DebugAssert(j < 4);
    Row[0][j] = v.x;
    Row[1][j] = v.y;
    Row[2][j] = v.z;
    Row[3][j] = v.w;
}

void Matrix4x4f::SetTranspose(const float *pElements)
{
    float *pv = &m00;
    *pv++ = pElements[0]; *pv++ = pElements[4]; *pv++ = pElements[8]; *pv++ = pElements[12];
    *pv++ = pElements[1]; *pv++ = pElements[5]; *pv++ = pElements[9]; *pv++ = pElements[13];
    *pv++ = pElements[2]; *pv++ = pElements[6]; *pv++ = pElements[10]; *pv++ = pElements[14];
    *pv++ = pElements[3]; *pv++ = pElements[7]; *pv++ = pElements[11]; *pv = pElements[15];
}

void Matrix4x4f::GetTranspose(float *pElements) const
{
    float *pv = pElements;
    *pv++ = Row[0][0]; *pv++ = Row[1][0]; *pv++ = Row[2][0]; *pv++ = Row[3][0];
    *pv++ = Row[0][1]; *pv++ = Row[1][1]; *pv++ = Row[2][1]; *pv++ = Row[3][1];
    *pv++ = Row[0][2]; *pv++ = Row[1][2]; *pv++ = Row[2][2]; *pv++ = Row[3][2];
    *pv++ = Row[0][3]; *pv++ = Row[1][3]; *pv++ = Row[2][3]; *pv = Row[3][3];
}

Matrix4x4f Matrix4x4f::Inverse() const
{
    Matrix4x4f res;

    float v0 = m20 * m31 - m21 * m30;
    float v1 = m20 * m32 - m22 * m30;
    float v2 = m20 * m33 - m23 * m30;
    float v3 = m21 * m32 - m22 * m31;
    float v4 = m21 * m33 - m23 * m31;
    float v5 = m22 * m33 - m23 * m32;

    float t00 = + (v5 * m11 - v4 * m12 + v3 * m13);
    float t10 = - (v5 * m10 - v2 * m12 + v1 * m13);
    float t20 = + (v4 * m10 - v2 * m11 + v0 * m13);
    float t30 = - (v3 * m10 - v1 * m11 + v0 * m12);

    float invDet = 1 / (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);

    res.m00 = t00 * invDet;
    res.m10 = t10 * invDet;
    res.m20 = t20 * invDet;
    res.m30 = t30 * invDet;

    res.m01 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
    res.m11 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
    res.m21 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
    res.m31 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

    v0 = m10 * m31 - m11 * m30;
    v1 = m10 * m32 - m12 * m30;
    v2 = m10 * m33 - m13 * m30;
    v3 = m11 * m32 - m12 * m31;
    v4 = m11 * m33 - m13 * m31;
    v5 = m12 * m33 - m13 * m32;

    res.m02 = + (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
    res.m12 = - (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
    res.m22 = + (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
    res.m32 = - (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

    v0 = m21 * m10 - m20 * m11;
    v1 = m22 * m10 - m20 * m12;
    v2 = m23 * m10 - m20 * m13;
    v3 = m22 * m11 - m21 * m12;
    v4 = m23 * m11 - m21 * m13;
    v5 = m23 * m12 - m22 * m13;

    res.m03 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
    res.m13 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
    res.m23 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
    res.m33 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

    return res;
}

Matrix4x4f Matrix4x4f::Transpose() const
{
    Matrix4x4f res;

    res.m00 = m00;
    res.m01 = m10;
    res.m02 = m20;
    res.m03 = m30;
    res.m10 = m01;
    res.m11 = m11;
    res.m12 = m21;
    res.m13 = m31;
    res.m20 = m02;
    res.m21 = m12;
    res.m22 = m22;
    res.m23 = m32;
    res.m30 = m03;
    res.m31 = m13;
    res.m32 = m23;
    res.m33 = m33;

    return res;
}

float Matrix4x4f::Determinant() const
{
    float v0 = m20 * m31 - m21 * m30;
    float v1 = m20 * m32 - m22 * m30;
    float v2 = m20 * m33 - m23 * m30;
    float v3 = m21 * m32 - m22 * m31;
    float v4 = m21 * m33 - m23 * m31;
    float v5 = m22 * m33 - m23 * m32;

    float t00 = + (v5 * m11 - v4 * m12 + v3 * m13);
    float t10 = - (v5 * m10 - v2 * m12 + v1 * m13);
    float t20 = + (v4 * m10 - v2 * m11 + v0 * m13);
    float t30 = - (v3 * m10 - v1 * m11 + v0 * m12);

    return (t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03);
}

void Matrix4x4f::InvertInPlace()
{
    // rvo should take care of this
    Matrix4x4f temp(*this);
    *this = temp.Inverse();
}

void Matrix4x4f::TransposeInPlace()
{
    uint32 i, j;
    for (i = 0; i < 4; i++)
    {
        for (j = i + 1; j < 4; j++)
        {
            if (i == j)
                continue;

            float tmp = Row[j][i];
            Row[j][i] = Row[i][j];
            Row[i][j] = tmp;
        }
    }
}

SIMDMatrix4x4f Matrix4x4f::GetMatrix4f() const
{
    return SIMDMatrix4x4f(&m00);
}

Vector3f Matrix4x4f::TransformPoint(const Vector3f &point) const
{
    Vector3f ret;
    ret.x = Vector3f(Row[0]).Dot(point) + Row[0][3];
    ret.y = Vector3f(Row[1]).Dot(point) + Row[1][3];
    ret.z = Vector3f(Row[2]).Dot(point) + Row[2][3];
    return ret;
}

Vector3f Matrix4x4f::TransformNormal(const Vector3f &normal, bool normalize /* = true */) const
{
    Vector3f ret;
    ret.x = Vector3f(Row[0]).Dot(normal);
    ret.y = Vector3f(Row[1]).Dot(normal);
    ret.z = Vector3f(Row[2]).Dot(normal);

    if (normalize && ret.SquaredLength() > 0.0f)
        ret.NormalizeInPlace();

    return ret;
}

void Matrix4x4f::Decompose(Vector3f &translation, Quaternion &rotation, Vector3f &scale) const
{
    // get translation
    translation.x = Row[0][3];
    translation.y = Row[1][3];
    translation.z = Row[2][3];

    // get rotation/scale part
    Vector3f rows3x3[3] = {
        Vector3f(Row[0]),
        Vector3f(Row[1]),
        Vector3f(Row[2])
    };

    // extract scale
    scale.x = rows3x3[0].Length();
    scale.y = rows3x3[1].Length();
    scale.z = rows3x3[2].Length();

    // handle sign
    if (Determinant() < 0)
        scale = -scale;

    // remove all scaling from rotation matrix
    if (scale.x != 0.0f)
        rows3x3[0] /= scale.x;
    if (scale.y != 0.0f)
        rows3x3[1] /= scale.y;
    if (scale.z != 0.0f)
        rows3x3[2] /= scale.z;

    // convert to 3x3 matrix
    Matrix3x3f rot3x3;
    rot3x3.SetRow(0, rows3x3[0]);
    rot3x3.SetRow(1, rows3x3[1]);
    rot3x3.SetRow(2, rows3x3[2]);

    // convert to quaternion
    rotation = Quaternion::FromFloat3x3(rot3x3);
}

Matrix4x4f &Matrix4x4f::operator=(const Matrix4x4f &v)
{
    Y_memcpy(Elements, v.Elements, sizeof(Elements));
    return *this;
}

Matrix4x4f &Matrix4x4f::operator=(const float Diagonal)
{
    float *pv = &m00;
    *pv++ = Diagonal; *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = Diagonal; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = Diagonal; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = Diagonal;
    return *this;
}

Matrix4x4f &Matrix4x4f::operator=(const float *p)
{
    Y_memcpy(Elements, p, sizeof(Elements));
    return *this;
}

Matrix4x4f &Matrix4x4f::operator=(const SIMDMatrix4x4f &v)
{
    Y_memcpy(Elements, v.Elements, sizeof(Elements));
    return *this;
}

Matrix4x4f &Matrix4x4f::operator*=(const Matrix4x4f &v)
{
    // again, rvo should fix this up
    Matrix4x4f temp(*this);
    *this = temp * v;
    return *this;
}

Matrix4x4f &Matrix4x4f::operator*=(const float v)
{
    uint32 i;
    for (i = 0; i < 16; i++)
        Elements[i] *= v;

    return *this;
}



Matrix4x4f Matrix4x4f::operator*(const Matrix4x4f &v) const
{
#define mulRC(rw, cl) Row[rw][0] * v.Row[0][cl] + \
                      Row[rw][1] * v.Row[1][cl] + \
                      Row[rw][2] * v.Row[2][cl] + \
                      Row[rw][3] * v.Row[3][cl]


    Matrix4x4f res;
    res.m00 = mulRC(0, 0);  res.m01 = mulRC(0, 1);  res.m02 = mulRC(0, 2);  res.m03 = mulRC(0, 3);
    res.m10 = mulRC(1, 0);  res.m11 = mulRC(1, 1);  res.m12 = mulRC(1, 2);  res.m13 = mulRC(1, 3);
    res.m20 = mulRC(2, 0);  res.m21 = mulRC(2, 1);  res.m22 = mulRC(2, 2);  res.m23 = mulRC(2, 3);
    res.m30 = mulRC(3, 0);  res.m31 = mulRC(3, 1);  res.m32 = mulRC(3, 2);  res.m33 = mulRC(3, 3);
    return res;

#undef mulRC
}

Matrix4x4f Matrix4x4f::operator*(float v) const
{
    uint32 i;
    Matrix4x4f res;

    for (i = 0; i < 16; i++)
        res.Elements[i] = Elements[i] * v;

    return res;
}

Matrix4x4f Matrix4x4f::operator-() const
{
    uint32 i;
    Matrix4x4f res;

    for (i = 0; i < 16; i++)
        res.Elements[i] = -Elements[i];

    return res;
}

Vector4f Matrix4x4f::operator*(const Vector4f &v) const
{
    Vector4f res;

//     res.x = reinterpret_cast<const float4 &>(row[0]).Dot(v);
//     res.y = reinterpret_cast<const float4 &>(row[1]).Dot(v);
//     res.z = reinterpret_cast<const float4 &>(row[2]).Dot(v);
//     res.w = reinterpret_cast<const float4 &>(row[3]).Dot(v);

    res.x = Row[0][0] * v.x + Row[0][1] * v.y + Row[0][2] * v.z + Row[0][3] * v.w;
    res.y = Row[1][0] * v.x + Row[1][1] * v.y + Row[1][2] * v.z + Row[1][3] * v.w;
    res.z = Row[2][0] * v.x + Row[2][1] * v.y + Row[2][2] * v.z + Row[2][3] * v.w;
    res.w = Row[3][0] * v.x + Row[3][1] * v.y + Row[3][2] * v.z + Row[3][3] * v.w;

    return res;
}

Matrix4x4f &Matrix4x4f::operator+=(const Matrix4x4f &v)
{
    for (uint32 i = 0; i < countof(Elements); i++)
        Elements[i] += v.Elements[i];
    return *this;
}

Matrix4x4f &Matrix4x4f::operator+=(const float v)
{
    for (uint32 i = 0; i < countof(Elements); i++)
        Elements[i] += v;
    return *this;
}

Matrix4x4f Matrix4x4f::operator+(const Matrix4x4f &v) const
{
    Matrix4x4f ret;
    for (uint32 i = 0; i < countof(Elements); i++)
        ret.Elements[i] = Elements[i] + v.Elements[i];
    return ret;
}

Matrix4x4f Matrix4x4f::operator+(const float v) const
{
    Matrix4x4f ret;
    for (uint32 i = 0; i < countof(Elements); i++)
        ret.Elements[i] = Elements[i] + v;
    return ret;
}

Matrix4x4f Matrix4x4f::MakeScaleMatrix(float s)
{
    return MakeScaleMatrix(s, s, s);
}

Matrix4x4f Matrix4x4f::MakeScaleMatrix(const Vector3f &s)
{
    return MakeScaleMatrix(s.x, s.y, s.z);
}

Matrix4x4f Matrix4x4f::MakeScaleMatrix(float x, float y, float z)
{
    return Matrix4x4f(x, 0.0f, 0.0f, 0.0f,
                    0.0f, y, 0.0f, 0.0f,
                    0.0f, 0.0f, z, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix4x4f Matrix4x4f::MakeRotationMatrixX(Angle x)
{
    float sinA, cosA;
    Y_sincosf(x.Radians(), &sinA, &cosA);

    return Matrix4x4f(1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, cosA, -sinA, 0.0f,
                    0.0f, sinA, cosA, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix4x4f Matrix4x4f::MakeRotationMatrixY(Angle y)
{
    float sinA, cosA;
    Y_sincosf(y.Radians(), &sinA, &cosA);

    return Matrix4x4f(cosA, 0.0f, sinA, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    -sinA, 0.0f, cosA, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix4x4f Matrix4x4f::MakeRotationMatrixZ(Angle z)
{
    float sinA, cosA;
    Y_sincosf(z.Radians(), &sinA, &cosA);

    return Matrix4x4f(cosA, -sinA, 0.0f, 0.0f,
                    sinA, cosA, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix4x4f Matrix4x4f::MakeRotationFromEulerAnglesMatrix(const Vector3f &r)
{
    return MakeRotationFromEulerAnglesMatrix(r.x, r.y, r.z);
}

Matrix4x4f Matrix4x4f::MakeRotationFromEulerAnglesMatrix(Angle x, Angle y, Angle z)
{
    Matrix4x4f xRotation(MakeRotationMatrixX(x));
    Matrix4x4f yRotation(MakeRotationMatrixY(y));
    Matrix4x4f zRotation(MakeRotationMatrixZ(z));

    // remember: right to left
    return xRotation * yRotation * zRotation;
}

Matrix4x4f Matrix4x4f::MakeTranslationMatrix(const Vector3f &t)
{
    return MakeTranslationMatrix(t.x, t.y, t.z);
}

Matrix4x4f Matrix4x4f::MakeTranslationMatrix(float x, float y, float z)
{
    return Matrix4x4f(1.0f, 0.0f, 0.0f, x,
                    0.0f, 1.0f, 0.0f, y,
                    0.0f, 0.0f, 1.0f, z,
                    0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix4x4f Matrix4x4f::MakePerspectiveProjectionMatrix(float fovY, float aspect, float zNear, float zFar)
{
    // for future:
    // fovy = 2 * atan(tan(fovw / 2) * (height / width))

    // yScale = cot(fovY / 2)
    float sinHalfFov, cosHalfFov;
    Y_sincosf(fovY / 2.0f, &sinHalfFov, &cosHalfFov);

    float yScale = cosHalfFov / sinHalfFov;
    float xScale = yScale / aspect;

    return Matrix4x4f(xScale, 0.0f, 0.0f, 0.0f,
                    0.0f, yScale, 0.0f, 0.0f,
                    0.0f, 0.0f, zFar / (zNear - zFar), zNear * zFar / (zNear - zFar),
                    0.0f, 0.0f, -1.0f, 0.0f);
}

Matrix4x4f Matrix4x4f::MakeOrthographicProjectionMatrix(float width, float height, float zNear, float zFar)
{
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;

    return MakeOrthographicOffCenterProjectionMatrix(-halfWidth, halfWidth, -halfHeight, halfHeight, zNear, zFar);
}

Matrix4x4f Matrix4x4f::MakeOrthographicOffCenterProjectionMatrix(float left, float right, float bottom, float top, float zNear, float zFar)
{
    return Matrix4x4f(2.0f / (right - left), 0.0f, 0.0f, (left + right) / (left - right),
                      0.0f, 2.0f / (top - bottom), 0.0f, (top + bottom) / (bottom - top),
                      0.0f, 0.0f, 1.0f / (zNear - zFar), zNear / (zNear - zFar),
                      0.0f, 0.0f, 0.0f, 1.0f);
}

Matrix4x4f Matrix4x4f::MakeCoordinateSystemConversionMatrix(COORDINATE_SYSTEM fromSystem, COORDINATE_SYSTEM toSystem, bool *pReverseWinding)
{
    Matrix4x4f res;

    // axischange
    Matrix4x4f AxisChange;
    if ((fromSystem == COORDINATE_SYSTEM_Y_UP_LH || fromSystem == COORDINATE_SYSTEM_Y_UP_RH) &&
        (toSystem == COORDINATE_SYSTEM_Z_UP_LH || toSystem == COORDINATE_SYSTEM_Z_UP_RH))
    {
        // going yup -> zup, rotate 90 degrees forward
        AxisChange = MakeRotationMatrixX(90.0f);
    }
    else if ((fromSystem == COORDINATE_SYSTEM_Z_UP_LH || fromSystem == COORDINATE_SYSTEM_Z_UP_RH) &&
             (toSystem == COORDINATE_SYSTEM_Y_UP_LH || toSystem == COORDINATE_SYSTEM_Y_UP_RH))
    {
        // going zup -> yup, rotate 90 degrees backwards
        AxisChange = MakeRotationMatrixX(-90.0f);
    }
    else
    {
        // no axis change
        AxisChange.SetIdentity();
    }

    // handedness change
    uint32 fromHandedness = (fromSystem == COORDINATE_SYSTEM_Y_UP_LH || fromSystem == COORDINATE_SYSTEM_Z_UP_LH) ? 0 : 1;
    uint32 toHandedness = (toSystem == COORDINATE_SYSTEM_Y_UP_LH || toSystem == COORDINATE_SYSTEM_Z_UP_LH) ? 0 : 1;
    if (fromHandedness != toHandedness)
    {
        static const Matrix4x4f MirrorYAxis(1, 0, 0, 0,
                                            0, -1, 0, 0,
                                            0, 0, 1, 0,
                                            0, 0, 0, 1);

        static const Matrix4x4f MirrorZAxis(1, 0, 0, 0,
                                            0, 1, 0, 0,
                                            0, 0, -1, 0,
                                            0, 0, 0, 1);

        if (toSystem == COORDINATE_SYSTEM_Y_UP_LH || toSystem == COORDINATE_SYSTEM_Y_UP_RH)
            res = MirrorZAxis * AxisChange;
        else
            res = MirrorYAxis * AxisChange;

        if (pReverseWinding != NULL)
            *pReverseWinding = true;
    }
    else
    {
        res = AxisChange;

        if (pReverseWinding != NULL)
            *pReverseWinding = false;
    }

    return res;
}

Matrix4x4f Matrix4x4f::MakeLookAtViewMatrix(const Vector3f &eye, const Vector3f &at, const Vector3f &up)
{
    SIMDVector3f zAxis = (eye - at).SafeNormalize();
    SIMDVector3f xAxis = up.Cross(zAxis).SafeNormalize();
    SIMDVector3f yAxis = zAxis.Cross(xAxis);

    return Matrix4x4f(xAxis.x, xAxis.y, xAxis.z, -xAxis.Dot(eye),
                    yAxis.x, yAxis.y, yAxis.z, -yAxis.Dot(eye),
                    zAxis.x, zAxis.y, zAxis.z, -zAxis.Dot(eye),
                    0.0f, 0.0f, 0.0f, 1.0f);
}

static const float s_CubeViewMatrices[6][16] =
{
    // +x
    { 0.0f, 0.0f, -1.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f },

    // -x
    { 0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      -1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f },

    // +y
    { 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, -1.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f },

    // -y
    { 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, -1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f },

    // +z
    { -1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, -1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f },

    // -z
    { 1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f }
};


Matrix4x4f Matrix4x4f::MakeCubeViewMatrix(uint32 face)
{
    DebugAssert(face < 6);
    return Matrix4x4f(s_CubeViewMatrices[face]).Transpose();
}

Matrix4x4f Matrix4x4f::MakeCubeViewMatrix(uint32 face, const Vector3f &position)
{
    DebugAssert(face < 6);

    Matrix4x4f ret(s_CubeViewMatrices[face]);
    ret.SetColumn(3, Vector4f(-ret.GetRow(0).Dot(position), -ret.GetRow(1).Dot(position), -ret.GetRow(2).Dot(position), 1.0f));
    return ret;
}

// constants
static const float __float4x4Zero[16] = { 0.0f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 0.0f, 0.0f, 0.0f };
const Matrix4x4f &Matrix4x4f::Zero = reinterpret_cast<const Matrix4x4f &>(__float4x4Zero);
static const float __float4x4Identity[16] = { 1.0f, 0.0f, 0.0f, 0.0f,
                                              0.0f, 1.0f, 0.0f, 0.0f,
                                              0.0f, 0.0f, 1.0f, 0.0f,
                                              0.0f, 0.0f, 0.0f, 1.0f };
const Matrix4x4f &Matrix4x4f::Identity = reinterpret_cast<const Matrix4x4f &>(__float4x4Identity);

//========================================================================================================================================================================================================================

Matrix3x4f::Matrix3x4f(const float &E00, const float &E01, const float &E02, const float &E03,
                   const float &E10, const float &E11, const float &E12, const float &E13,
                   const float &E20, const float &E21, const float &E22, const float &E23)
{
    float *pv = &m00;
    *pv++ = E00; *pv++ = E01; *pv++ = E02; *pv++ = E03;
    *pv++ = E10; *pv++ = E11; *pv++ = E12; *pv++ = E13;
    *pv++ = E20; *pv++ = E21; *pv++ = E22; *pv = E23;
}

Matrix3x4f::Matrix3x4f(const float *p)
{
    Y_memcpy(Elements, p, sizeof(Elements));
}

Matrix3x4f::Matrix3x4f(const float p[3][4])
{
    Y_memcpy(Elements, p, sizeof(Elements));
}

Matrix3x4f::Matrix3x4f(const Matrix3x4f &v)
{
    Y_memcpy(Elements, v.Elements, sizeof(Elements));
}

// float3x4::float3x4(const Matrix3x4 &v)
// {
//     Y_memcpy(Elements, v.Elements, sizeof(Elements));
// }

Matrix3x4f::Matrix3x4f(const Matrix4x4f &v)
{
    Y_memcpy(Row[0], v.Row[0], sizeof(Row[0]));
    Y_memcpy(Row[1], v.Row[1], sizeof(Row[1]));
    Y_memcpy(Row[2], v.Row[2], sizeof(Row[2]));
}

void Matrix3x4f::Set(const float &E00, const float &E01, const float &E02, const float &E03,
                   const float &E10, const float &E11, const float &E12, const float &E13,
                   const float &E20, const float &E21, const float &E22, const float &E23)
{
    float *pv = &m00;
    *pv++ = E00; *pv++ = E01; *pv++ = E02; *pv++ = E03;
    *pv++ = E10; *pv++ = E11; *pv++ = E12; *pv++ = E13;
    *pv++ = E20; *pv++ = E21; *pv++ = E22; *pv = E23;
}

void Matrix3x4f::SetZero()
{
    float *pv = &m00;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f; *pv = 0.0f;
}

void Matrix3x4f::SetIdentity()
{
    float *pv = &m00;
    *pv++ = 1.0f; *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 1.0f; *pv++ = 0.0f; *pv++ = 0.0f;
    *pv++ = 0.0f; *pv++ = 0.0f; *pv++ = 1.0f; *pv = 0.0f;
}

Vector3f Matrix3x4f::GetColumn(uint32 j) const
{
    DebugAssert(j < 4);
    return Vector3f(Row[0][j], Row[1][j], Row[2][j]);
}

void Matrix3x4f::SetColumn(uint32 j, const Vector3f &v)
{
    DebugAssert(j < 4);
    Row[0][j] = v.x;
    Row[1][j] = v.y;
    Row[2][j] = v.z;
}

void Matrix3x4f::SetTranspose(const float *pElements)
{
    float *pv = &m00;
    *pv++ = pElements[0]; *pv++ = pElements[3]; *pv++ = pElements[6]; *pv++ = pElements[9];
    *pv++ = pElements[1]; *pv++ = pElements[4]; *pv++ = pElements[7]; *pv++ = pElements[10];
    *pv++ = pElements[2]; *pv++ = pElements[5]; *pv++ = pElements[8]; *pv = pElements[11];
}

void Matrix3x4f::GetTranspose(float *pElements) const
{
    float *pv = pElements;
    *pv++ = Row[0][0]; *pv++ = Row[1][0]; *pv++ = Row[2][0]; *pv++ = Row[3][0];
    *pv++ = Row[0][1]; *pv++ = Row[1][1]; *pv++ = Row[2][1]; *pv++ = Row[3][1];
    *pv++ = Row[0][2]; *pv++ = Row[1][2]; *pv++ = Row[2][2]; *pv = Row[3][2];
}

Vector3f Matrix3x4f::TransformPoint(const Vector3f &point) const
{
    Vector3f ret;
    ret.x = Vector3f(Row[0]).Dot(point) + Row[0][3];
    ret.y = Vector3f(Row[1]).Dot(point) + Row[1][3];
    ret.z = Vector3f(Row[2]).Dot(point) + Row[2][3];
    return ret;
}

Vector3f Matrix3x4f::TransformNormal(const Vector3f &normal, bool normalize /* = true */) const
{
    Vector3f ret;
    ret.x = Vector3f(Row[0]).Dot(normal);
    ret.y = Vector3f(Row[1]).Dot(normal);
    ret.z = Vector3f(Row[2]).Dot(normal);

    if (normalize && ret.SquaredLength() > 0.0f)
        ret.NormalizeInPlace();
    
    return ret;
}

void Matrix3x4f::Load(const float *pValues)
{
    Row[0][0] = pValues[0];
    Row[0][1] = pValues[1];
    Row[0][2] = pValues[2];
    Row[0][3] = pValues[3];
    Row[1][0] = pValues[4];
    Row[1][1] = pValues[5];
    Row[1][2] = pValues[6];
    Row[1][3] = pValues[7];
    Row[2][0] = pValues[8];
    Row[2][1] = pValues[9];
    Row[2][2] = pValues[10];
    Row[2][3] = pValues[11];
}

void Matrix3x4f::Store(float *pValues)
{
    pValues[0] = Row[0][0];
    pValues[1] = Row[0][1];
    pValues[2] = Row[0][2];
    pValues[3] = Row[0][3];
    pValues[4] = Row[1][0];
    pValues[5] = Row[1][1];
    pValues[6] = Row[1][2];
    pValues[7] = Row[1][3];
    pValues[8] = Row[2][0];
    pValues[9] = Row[2][1];
    pValues[10] = Row[2][2];
    pValues[11] = Row[2][3];
}

// Matrix3x4 float3x4::GetMatrix3x4() const
// {
//     return Matrix3x4(&m00);
// }

Matrix3x4f &Matrix3x4f::operator=(const Matrix3x4f &v)
{
    Y_memcpy(Elements, v.Elements, sizeof(Elements));
    return *this;
}

Matrix3x4f &Matrix3x4f::operator=(const float *p)
{
    Y_memcpy(Elements, p, sizeof(Elements));
    return *this;
}

// float3x4 &float3x4::operator=(const Matrix3x4 &v)
// {
//     Y_memcpy(Elements, v.Elements, sizeof(Elements));
//     return *this;
// }

Matrix3x4f &Matrix3x4f::operator*=(const Matrix3x4f &v)
{
    // again, rvo should fix this up
    Matrix3x4f temp(*this);
    *this = temp * v;
    return *this;
}

Matrix3x4f &Matrix3x4f::operator*=(const float v)
{
    uint32 i;
    for (i = 0; i < 16; i++)
        Elements[i] *= v;

    return *this;
}

Matrix3x4f Matrix3x4f::operator*(const Matrix3x4f &v) const
{
    static const float lastRow[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

#define mulRC(rw, cl) Row[rw][0] * v.Row[0][cl] + \
                      Row[rw][1] * v.Row[1][cl] + \
                      Row[rw][2] * v.Row[2][cl] + \
                      Row[rw][3] * lastRow[cl]

    Matrix3x4f res;
    res.m00 = mulRC(0, 0);  res.m01 = mulRC(0, 1);  res.m02 = mulRC(0, 2);  res.m03 = mulRC(0, 3);
    res.m10 = mulRC(1, 0);  res.m11 = mulRC(1, 1);  res.m12 = mulRC(1, 2);  res.m13 = mulRC(1, 3);
    res.m20 = mulRC(2, 0);  res.m21 = mulRC(2, 1);  res.m22 = mulRC(2, 2);  res.m23 = mulRC(2, 3);
    return res;

#undef mulRC
}

Matrix3x4f Matrix3x4f::operator*(float v) const
{
    uint32 i;
    Matrix3x4f res;

    for (i = 0; i < 12; i++)
        res.Elements[i] = Elements[i] * v;

    return res;
}

Matrix3x4f Matrix3x4f::operator-() const
{
    uint32 i;
    Matrix3x4f res;

    for (i = 0; i < 12; i++)
        res.Elements[i] = -Elements[i];

    return res;
}

Vector3f Matrix3x4f::operator*(const Vector3f &v) const
{
    Vector3f res;

    res.x = Row[0][0] * v.x + Row[0][1] * v.y + Row[0][2] * v.z + Row[0][3];
    res.y = Row[1][0] * v.x + Row[1][1] * v.y + Row[1][2] * v.z + Row[1][3];
    res.z = Row[2][0] * v.x + Row[2][1] * v.y + Row[2][2] * v.z + Row[2][3];

    return res;
}

Matrix3x4f &Matrix3x4f::operator+=(const Matrix3x4f &v)
{
    for (uint32 i = 0; i < countof(Elements); i++)
        Elements[i] += v.Elements[i];
    return *this;
}

Matrix3x4f &Matrix3x4f::operator+=(const float v)
{
    for (uint32 i = 0; i < countof(Elements); i++)
        Elements[i] += v;
    return *this;
}

Matrix3x4f Matrix3x4f::operator+(const Matrix3x4f &v) const
{
    Matrix3x4f ret;
    for (uint32 i = 0; i < countof(Elements); i++)
        ret.Elements[i] = Elements[i] + v.Elements[i];
    return ret;
}

Matrix3x4f Matrix3x4f::operator+(const float v) const
{
    Matrix3x4f ret;
    for (uint32 i = 0; i < countof(Elements); i++)
        ret.Elements[i] = Elements[i] + v;
    return ret;
}

// constants
static const float __float3x4Zero[12] = { 0.0f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 0.0f, 0.0f, 0.0f };
const Matrix3x4f &Matrix3x4f::Zero = reinterpret_cast<const Matrix3x4f &>(__float3x4Zero);
static const float __float3x4Identity[12] = { 1.0f, 0.0f, 0.0f, 0.0f,
                                              0.0f, 1.0f, 0.0f, 0.0f,
                                              0.0f, 0.0f, 1.0f, 0.0f };
const Matrix3x4f &Matrix3x4f::Identity = reinterpret_cast<const Matrix3x4f &>(__float3x4Identity);


