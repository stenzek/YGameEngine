#include "MathLib/Quaternion.h"

// constants
static const float __QuaternionIdentity[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
const Quaternion &Quaternion::Identity = reinterpret_cast<const Quaternion &>(__QuaternionIdentity);

Quaternion Quaternion::Normalize() const
{
    Quaternion ret;
    float Length = Y_sqrtf(x * x + y * y + z * z + w * w);
    ret.x = x / Length; 
    ret.y = y / Length;
    ret.z = z / Length;
    ret.w = w / Length;
    return ret;
}

Quaternion Quaternion::NormalizeEst() const
{
    Quaternion ret;
    float invLength = Y_rsqrtf(x * x + y * y + z * z + w * w);
    ret.x = x * invLength;
    ret.y = y * invLength;
    ret.z = z * invLength;
    ret.w = w * invLength;
    return ret;
}

void Quaternion::NormalizeInPlace()
{
    float Length = Y_sqrtf(x * x + y * y + z * z + w * w);
    x /= Length; y /= Length; z /= Length; w /= Length;
}

void Quaternion::NormalizeEstInPlace()
{
    float invLength = Y_rsqrtf(x * x + y * y + z * z + w * w);
    x *= invLength; y *= invLength; z *= invLength; w *= invLength;
}

void Quaternion::ConjugateInPlace()
{
    x = -x;
    y = -y;
    z = -z;
}

void Quaternion::InvertInPlace()
{
    NormalizeInPlace();
    ConjugateInPlace();
}

Quaternion Quaternion::operator*(const Quaternion &q) const
{
    Quaternion res;
    res.x = w * q.x + x * q.w + y * q.z - z * q.y;
    res.y = w * q.y + y * q.w + z * q.x - x * q.z;
    res.z = w * q.z + z * q.w + x * q.y - y * q.x;
    res.w = w * q.w - x * q.x - y * q.y - z * q.z;
    return res;
}

Quaternion &Quaternion::operator*=(const Quaternion &q)
{
    Quaternion temp(*this);
    *this = temp * q;
    return *this;
}

bool Quaternion::operator==(const Quaternion &q) const
{
    return (x == q.x && y == q.y && z == q.z && w == q.w);
}

bool Quaternion::operator!=(const Quaternion &q) const
{
    return (x != q.x || y != q.y || z != q.z || w != q.w);
}

SIMDVector3f Quaternion::operator*(const SIMDVector3f &v) const
{
    // v' = q * v * v^(-1)
//     Quaternion vRotated(*this * Quaternion(v.x, v.y, v.z, 0.0f));
//     vRotated *= Inverse();
//     return Vector3(vRotated.x, vRotated.y, vRotated.z);

//     // http://molecularmusings.wordpress.com/2013/05/24/a-faster-quaternion-vector-multiplication/
//     Vector3 qAsVec(x, y, z);
//     Vector3 t(qAsVec.Cross(v) * 2.0f);
//     return v + (t * w) + qAsVec.Cross(t);

    SIMDVector3f qAsVec(x, y, z);
    SIMDVector3f uv(qAsVec.Cross(v));
    SIMDVector3f uuv(qAsVec.Cross(uv));

    return v + (uv * (2.0f * w)) + (uuv * 2.0f);
}

Vector3f Quaternion::operator*(const Vector3f &v) const
{
    // v' = q * v * v^(-1)
//     Quaternion vRotated(*this * Quaternion(v.x, v.y, v.z, 0.0f));
//     vRotated *= Inverse();
//     return float3(vRotated.x, vRotated.y, vRotated.z);

//     // http://molecularmusings.wordpress.com/2013/05/24/a-faster-quaternion-vector-multiplication/
//     Vector3 vAsVec(v);
//     Vector3 qAsVec(x, y, z);
//     Vector3 t(qAsVec.Cross(v) * 2.0f);
//     return vAsVec + (t * w) + qAsVec.Cross(t);

    SIMDVector3f vAsVec(v);
    SIMDVector3f qAsVec(x, y, z);
    SIMDVector3f uv(qAsVec.Cross(vAsVec));
    SIMDVector3f uuv(qAsVec.Cross(uv));

    return vAsVec + (uv * (2.0f * w)) + (uuv * 2.0f);
}

void Quaternion::GetAxisAngle(Vector3f &axis, Angle &angle) const
{
    // assume unnormalized input
    Quaternion qNormalized = Normalize();

    float cosA = qNormalized.w;
    float sinA = Y_sqrtf(1.0f - cosA * cosA);
    if (Y_fabs(sinA) < 0.0005f)
        sinA = 1.0f;

    angle.SetRadians(Y_acosf(cosA) * 2.0f);
    axis.Set(qNormalized.x, qNormalized.y, qNormalized.z);
    axis /= sinA;
}

void Quaternion::GetAxes(Vector3f &xAxis, Vector3f &yAxis, Vector3f &zAxis) const
{
    Matrix4x4f Temp = GetMatrix4x4();
    xAxis = Temp.GetColumn(0).xyz();
    yAxis = Temp.GetColumn(1).xyz();
    zAxis = Temp.GetColumn(2).xyz();
}

Vector3f Quaternion::GetEulerAngles() const
{
    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/index.htm

    /*
    float sqx = Math::Square(x);
    float sqy = Math::Square(y);
    float sqz = Math::Square(z);
    float sqw = Math::Square(w);

    float unit = sqx + sqy + sqz + sqw;
    float test = x * y + z * w;
    
    // singularity at north pole
    if (test > 0.499f * unit)
        return float3(Y_HALF_PI, 2.0f * Y_atan2f(x, w), 0.0f);

    // singularity at south pole
    if (test < -0.499f * unit)
        return float3(-Y_HALF_PI, -2.0f * Y_atan2f(x, w), 0.0f);

    return float3(Y_asinf(2.0f * test / unit),
                  Y_atan2f(2.0f * y * w - 2.0f * x * z, sqx - sqy - sqz + sqw),
                  Y_atan2f(2.0f * x * w - 2.0f * y * z, -sqx + sqy - sqz + sqw));*/

    Quaternion n(Normalize());

    float test = n.x * n.y + n.z * n.w;
    if (test > 0.499f)
        return Vector3f(90.0f, 360.0f / Y_PI * Y_atan2f(n.x, n.w), 0.0f);

    if (test < -0.499f)
        return Vector3f(-90.0f, 360.0f / Y_PI * Y_atan2f(n.x, n.w), 0.0f);

    float sqx = Math::Square(n.x);
    float sqy = Math::Square(n.y);
    float sqz = Math::Square(n.z);

    return Vector3f(Math::RadiansToDegrees(2.0f * test),
                  Math::RadiansToDegrees(Y_atan2f(2.0f * n.y * n.w - 2.0f * n.x * n.z, 1.0f - 2.0f * sqy - 2.0f * sqz)),
                  Math::RadiansToDegrees(Y_atan2f(2.0f * n.x * n.w - 2.0f * n.y * n.z, 1.0f - 2.0f * sqx - 2.0f * sqz)));
}

Quaternion Quaternion::FromEulerAngles(const Vector3f &v)
{
    return FromEulerAngles(v.x, v.y, v.z);
}

Quaternion Quaternion::FromEulerAngles(const Angle x, const Angle y, const Angle z)
{
    return (Quaternion::FromNormalizedAxisAngle(Vector3f::UnitZ, z) * 
            Quaternion::FromNormalizedAxisAngle(Vector3f::UnitY, y) * 
            Quaternion::FromNormalizedAxisAngle(Vector3f::UnitX, x));
}

Quaternion Quaternion::FromNormalizedAxisAngle(const Vector3f &axis, Angle angle)
{
    float SinAngle, CosAngle;
    Y_sincosf(angle.Radians() / 2.0f, &SinAngle, &CosAngle);

    SIMDVector3f axisMulSin = axis * SinAngle;
    return Quaternion(axisMulSin.x, axisMulSin.y, axisMulSin.z, CosAngle);
}

Quaternion Quaternion::FromAxisAngle(const Vector3f &axis, Angle angle)
{
    return FromNormalizedAxisAngle(axis.Normalize(), angle);
}

Quaternion Quaternion::FromAxes(const Vector3f &xAxis, const Vector3f &yAxis, const Vector3f &zAxis)
{
    Matrix4x4f Temp;
    Temp.SetColumn(0, Vector4f(xAxis, 0.0f));
    Temp.SetColumn(1, Vector4f(yAxis, 0.0f));
    Temp.SetColumn(2, Vector4f(zAxis, 1.0f));
    return FromFloat4x4(Temp);
}

// assumes matrix class has an operator()
template<typename T>
Quaternion QuaternionFromMatrixHelper(const T &mat)
{
    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm

    float trace = mat(0, 0) + mat(1, 1) + mat(2, 2);
    if (trace > 0.0f)
    {
        float S = 0.5f / Math::Sqrt(trace + 1.0f);
        float W = 0.25f / S;
        float X = (mat(2, 1) - mat(1, 2)) * S;
        float Y = (mat(0, 2) - mat(2, 0)) * S;
        float Z = (mat(1, 0) - mat(0, 1)) * S;

        return Quaternion(X, Y, Z, W);
    }
    else
    {
        if (mat(0, 0) > mat(1, 1) && mat(0, 0) > mat(2, 2))
        {
            float S = Math::Sqrt(1.0f + mat(0, 0) - mat(1, 1) - mat(2, 2)) * 2.0f;

            float Qw = (mat(2, 1) - mat(1, 2)) / S;
            float Qx = 0.25f * S;
            float Qy = (mat(0, 1) + mat(1, 0)) / S;
            float Qz = (mat(0, 2) + mat(2, 0)) / S;

            return Quaternion(Qx, Qy, Qz, Qw);
        }
        else if (mat(1, 1) > mat(0, 0) && mat(1, 1) > mat(2, 2))
        {
            float S = Math::Sqrt(1.0f + mat(1, 1) - mat(0, 0) - mat(2, 2)) * 2.0f;

            float Qw = (mat(0, 2) - mat(2, 0)) / S;
            float Qx = (mat(0, 1) + mat(1, 0)) / S;
            float Qy = 0.25f * S;
            float Qz = (mat(1, 2) + mat(2, 1)) / S;

            return Quaternion(Qx, Qy, Qz, Qw);
        }
        else// if (RotationMatrix(2, 2) > RotationMatrix(0, 0) && RotationMatrix(2, 2) > RotationMatrix(1, 1))
        {
            float S = Math::Sqrt(1.0f + mat(2, 2) - mat(0, 0) - mat(1, 1)) * 2.0f;

            float Qw = (mat(1, 0) - mat(0, 1)) / S;
            float Qx = (mat(0, 2) + mat(2, 0)) / S;
            float Qy = (mat(1, 2) + mat(2, 1)) / S;
            float Qz = 0.25f * S;

            return Quaternion(Qx, Qy, Qz, Qw);
        }
    }
}

Vector4f Quaternion::GetVectorRepresentation() const
{
    return Vector4f(x, y, z, w);
}

template<typename T>
T QuaternionToMatrixHelper(const Quaternion &q)
{
    // http://www.flipcode.com/documents/matrfaq.html#Q54

    T res;

    float xx = q.x * q.x;
    float xy = q.x * q.y;
    float xz = q.x * q.z;
    float xw = q.x * q.w;

    float yy = q.y * q.y;
    float yz = q.y * q.z;
    float yw = q.y * q.w;

    float zz = q.z * q.z;
    float zw = q.z * q.w;

    res(0, 0) = 1 - 2 * (yy + zz);
    res(0, 1) = 2 * (xy - zw);
    res(0, 2) = 2 * (xz + yw);
    res(1, 0) = 2 * (xy + zw);
    res(1, 1) = 1 - 2 * (xx + zz);
    res(1, 2) = 2 * (yz - xw);
    res(2, 0) = 2 * (xz - yw);
    res(2, 1) = 2 * (yz + xw);
    res(2, 2) = 1 - 2 * (xx + yy);

    return res;
}

Quaternion Quaternion::FromFloat3x3(const Matrix3x3f &rotationMatrix)
{
    return QuaternionFromMatrixHelper<Matrix3x3f>(rotationMatrix).Normalize();
}

Quaternion Quaternion::FromFloat3x4(const Matrix3x4f &rotationMatrix)
{
    return QuaternionFromMatrixHelper<Matrix3x4f>(rotationMatrix).Normalize();
}

Quaternion Quaternion::FromFloat4x4(const Matrix4x4f &rotationMatrix)
{
    return QuaternionFromMatrixHelper<Matrix4x4f>(rotationMatrix).Normalize();
}

Matrix3x3f Quaternion::GetMatrix3x3() const
{
    Matrix3x3f res = QuaternionToMatrixHelper<Matrix3x3f>(*this);
    return res;
}

SIMDMatrix3x3f Quaternion::GetSIMDMatrix3() const
{
    SIMDMatrix3x3f res = QuaternionToMatrixHelper<SIMDMatrix3x3f>(*this);
    return res;
}

Matrix3x4f Quaternion::GetMatrix3x4() const
{
    Matrix3x4f res = QuaternionToMatrixHelper<Matrix3x4f>(*this);

    // fill in remaining fields
    res[0][3] = res[1][3] = res[2][3] = res[0][3] = 0.0f;

    return res;
}

Matrix4x4f Quaternion::GetMatrix4x4() const
{
    Matrix4x4f res = QuaternionToMatrixHelper<Matrix4x4f>(*this);

    // fill in remaining fields
    res[0][3] = res[1][3] = res[2][3] = res[0][3] = res[3][0] = res[3][1] = res[3][2] = 0.0f;
    res[3][3] = 1.0f;

    return res;
}

SIMDMatrix4x4f Quaternion::GetSIMDMatrix4() const
{
    SIMDMatrix4x4f res = QuaternionToMatrixHelper<SIMDMatrix4x4f>(*this);

    // fill in remaining fields
    res[0][3] = res[1][3] = res[2][3] = res[0][3] = res[3][0] = res[3][1] = res[3][2] = 0.0f;
    res[3][3] = 1.0f;

    return res;
}

Quaternion Quaternion::FromTwoVectors(const Vector3f &u, const Vector3f &v)
{
    //float cos_theta = u.Normalize().Dot(v.Normalize());
    //float angle = Y_acosf(cos_theta);
    //float3 w(u.Cross(v).Normalize());
    //return Quaternion::FromAxisAngle(w, angle);

    float kCosTheta = u.Dot(v);
    float k = Math::Sqrt(u.SquaredLength() * v.SquaredLength());

    if (Math::NearEqual(kCosTheta / k, -1.0f, Y_FLT_EPSILON))
    {
        Vector3f other((Math::Abs(u.Dot(Vector3f::UnitX)) < 1.0f) ? Vector3f::UnitX : Vector3f::UnitY);
        return Quaternion::FromNormalizedAxisAngle(u.Cross(other).Normalize(), 180.0f);
    }

    return Quaternion(Vector4f(u.Cross(v), kCosTheta + k)).Normalize();
}

Quaternion Quaternion::FromTwoUnitVectors(const Vector3f &u, const Vector3f &v)
{
    Vector3f w(u.Cross(v));
    return Quaternion(w.x, w.y, w.z, 1.0f + u.Dot(v));
}

Quaternion Quaternion::LinearInterpolate(const Quaternion &start, const Quaternion &end, float factor)
{
    // from assimp
    float cosineTheta = start.x * end.x + start.y * end.y + start.z * end.z + start.w * end.w;

    // fix signs
    Quaternion newEnd;
    if (cosineTheta < 0.0f)
    {
        cosineTheta = -cosineTheta;
        newEnd.Set(-end.x, -end.y, -end.z, -end.w);
    }
    else
    {
        newEnd = end;
    }

    // calculate coefficients
    float sclp, sclq;
    if ((1.0f - cosineTheta) > Y_FLT_EPSILON)
    {
        float omega = Y_acosf(cosineTheta);
        float sinTheta = Math::Sin(omega);
        sclp = Math::Sin((1.0f - factor) * omega) / sinTheta;
        sclq = Math::Sin(factor * omega) / sinTheta;
    }
    else
    {
        // do lerp approximation
        sclp = 1.0f - factor;
        sclq = factor;
    }

    Quaternion returnValue;
    returnValue.x = sclp * start.x + sclq * newEnd.x;
    returnValue.y = sclp * start.y + sclq * newEnd.y;
    returnValue.z = sclp * start.z + sclq * newEnd.z;
    returnValue.w = sclp * start.w + sclq * newEnd.w;
    return returnValue;
}

