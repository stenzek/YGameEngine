#include "MathLib/Transform.h"

Transform::Transform(const Transform &transform)
    : m_position(transform.m_position),
      m_rotation(transform.m_rotation),
      m_scale(transform.m_scale)
{

}

Transform::Transform(const Vector3f &translation, const Quaternion &rotation, const Vector3f &scale)
    : m_position(translation),
      m_rotation(rotation),
      m_scale(scale)
{

}

Transform::Transform(const Matrix4x4f &transformMatrix)
{
    transformMatrix.Decompose(m_position, m_rotation, m_scale);
}

void Transform::SetIdentity()
{
    m_position.SetZero();
    m_rotation.SetIdentity();
    m_scale = Vector3f::One;
}

Vector3f Transform::TransformPoint(const Vector3f &v) const
{
    Vector3f ret;

    ret = v * m_scale;
    ret = m_rotation * ret;
    ret += m_position;

    return ret;
}

SIMDVector3f Transform::TransformPoint(const SIMDVector3f &v) const
{
    SIMDVector3f ret;

    ret = v * m_scale;
    ret = m_rotation * ret;
    ret += SIMDVector3f(m_position);

    return ret;
}

Vector3f Transform::TransformNormal(const Vector3f &v) const
{
    return (m_rotation * v).Normalize();
}

SIMDVector3f Transform::TransformNormal(const SIMDVector3f &v) const
{
    return (m_rotation * v).Normalize();
}

Ray Transform::TransformRay(const Ray &v) const
{
    Vector3f newOrigin(TransformPoint(v.GetOrigin()));
    Vector3f newEnd(TransformPoint(v.GetEnd()));

    return Ray(newOrigin, newEnd);
}

Vector3f Transform::UntransformPoint(const Vector3f &v) const
{
    Vector3f ret;

    ret = v - m_position;
    ret = m_rotation.Inverse() * ret;
    ret /= m_scale;
    
    return ret;
}

SIMDVector3f Transform::UntransformPoint(const SIMDVector3f &v) const
{
    SIMDVector3f ret;

    ret = v - SIMDVector3f(m_position);
    ret = m_rotation.Inverse() * ret;
    ret /= m_scale;

    return ret;
}

Vector3f Transform::UntransformNormal(const Vector3f &v) const
{
    return (m_rotation.Inverse() * v).Normalize();
}

SIMDVector3f Transform::UntransformNormal(const SIMDVector3f &v) const
{
    return (m_rotation.Inverse() * v).Normalize();
}

Ray Transform::UntransformRay(const Ray &v) const
{
    Vector3f newOrigin(UntransformPoint(v.GetOrigin()));
    Vector3f newEnd(UntransformPoint(v.GetEnd()));

    return Ray(newOrigin, newEnd);
}

Transform Transform::Inverse() const
{
    return Transform(-m_position, m_rotation.Inverse(), Vector3f::One / m_scale);
}

void Transform::InvertInPlace()
{
    m_position = -m_position;
    m_rotation.InvertInPlace();
    m_scale = Vector3f::One / m_scale;
}

Matrix3x4f Transform::GetTransformMatrix3x4() const
{
    return Matrix3x4f(GetTransformMatrix4x4());
}

Matrix4x4f Transform::GetTransformMatrix4x4() const
{
    return Matrix4x4f::MakeTranslationMatrix(m_position) * m_rotation.GetMatrix4x4() * Matrix4x4f::MakeScaleMatrix(m_scale);
}

Matrix3x4f Transform::GetInverseTransformMatrix3x4() const
{
    return Matrix3x4f(GetInverseTransformMatrix4x4());
}

Matrix4x4f Transform::GetInverseTransformMatrix4x4() const
{
    return Matrix4x4f::MakeScaleMatrix(Vector3f::One / m_scale) * m_rotation.Inverse().GetMatrix4x4() * Matrix4x4f::MakeTranslationMatrix(-m_position);
}

bool Transform::operator==(const Transform &transform) const
{
    return (m_position == transform.m_position &&
            m_rotation == transform.m_rotation &&
            m_scale == transform.m_scale);
}

bool Transform::operator!=(const Transform &transform) const
{
    return (m_position != transform.m_position ||
            m_rotation != transform.m_rotation ||
            m_scale != transform.m_scale);
}

Transform &Transform::operator=(const Transform &transform)
{
    m_position = transform.m_position;
    m_rotation = transform.m_rotation;
    m_scale = transform.m_scale;
    return *this;
}

Transform Transform::operator*(const Transform &rightSide) const
{
    return ConcatenateTransforms(*this, rightSide);
}

Vector3f Transform::operator*(const Vector3f &rhs) const
{
    return TransformPoint(rhs);
}

SIMDVector3f Transform::operator*(const SIMDVector3f &rhs) const
{
    return TransformPoint(rhs);
}

Transform &Transform::operator*=(const Transform &rightSide)
{
    Transform temp(*this);
    *this = ConcatenateTransforms(temp, rightSide);
    return *this;
}

AABox Transform::TransformBoundingBox(const AABox &boundingBox) const
{
    SIMDVector3f cornerPoints[8];
    boundingBox.GetCornerPoints(cornerPoints);

    AABox newBoundingBox(TransformPoint(cornerPoints[0]));
    for (uint32 i = 1; i < 8; i++)
        newBoundingBox.Merge(TransformPoint(cornerPoints[i]));

    return newBoundingBox;
}

Sphere Transform::TransformBoundingSphere(const Sphere &boundingSphere) const
{
    Sphere newBoundingSphere;

    newBoundingSphere.SetCenter(TransformPoint(boundingSphere.GetCenter()));
    newBoundingSphere.SetRadius(boundingSphere.GetRadius() * Max(m_scale.x, Max(m_scale.y, m_scale.z)));

    return newBoundingSphere;
}

Transform Transform::LinearInterpolate(const Transform &end, const float factor)
{
    return Transform::LinearInterpolate(*this, end, factor);
}

Transform Transform::LinearInterpolate(const Transform &start, const Transform &end, const float factor)
{
    Transform returnValue;

    returnValue.m_position = SIMDVector3f(start.m_position).Lerp(SIMDVector3f(end.m_position), factor);
    returnValue.m_rotation = Quaternion::LinearInterpolate(start.m_rotation, end.m_rotation, factor);
    returnValue.m_scale = SIMDVector3f(start.m_scale).Lerp(SIMDVector3f(end.m_scale), factor);

    return returnValue;
}

Transform Transform::ConcatenateTransforms(const Transform &leftSide, const Transform &rightSide)
{
#if 1
    // left then right
    //float3 newTranslation(rightSide.TransformPoint(leftSide.GetPosition()));
    Vector3f newTranslation(rightSide.m_rotation * (leftSide.m_position * rightSide.m_scale) + rightSide.GetPosition());
    Quaternion newRotation((rightSide.m_rotation * leftSide.m_rotation).Normalize());
    Vector3f newScale(rightSide.m_scale * leftSide.m_scale);

    return Transform(newTranslation, newRotation, newScale);
#else
    return Transform(rightSide.GetTransformMatrix3x4() * leftSide.GetTransformMatrix3x4());
#endif
}


static const float IdentityTransform[10] = { 0.0f, 0.0f, 0.0f,          // translation
                                             0.0f, 0.0f, 0.0f, 1.0f,    // rotation
                                             1.0f, 1.0f, 1.0f };        // scale

const Transform& Transform::Identity = reinterpret_cast<const Transform &>(IdentityTransform);
