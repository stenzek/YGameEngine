#pragma once
#include "MathLib/Common.h"
#include "MathLib/Vectorf.h"
#include "MathLib/SIMDVectorf.h"
#include "MathLib/Quaternion.h"
#include "MathLib/AABox.h"
#include "MathLib/Sphere.h"
#include "MathLib/Ray.h"

class Transform
{
public:
    Transform() {}
    Transform(const Transform &transform);
    Transform(const Vector3f &translation, const Quaternion &rotation, const Vector3f &scale);
    Transform(const Matrix4x4f &transformMatrix);

    const Vector3f &GetPosition() const { return m_position; }
    const Quaternion &GetRotation() const { return m_rotation; }
    const Vector3f &GetScale() const { return m_scale; }

    void Set(const Vector3f &position, const Quaternion &rotation, const Vector3f &scale) { m_position = position; m_rotation = rotation; m_scale = scale; }
    void SetPosition(const Vector3f &position) { m_position = position; }
    void SetRotation(const Quaternion &rotation) { m_rotation = rotation; }
    void SetScale(const Vector3f &scale) { m_scale = scale; }
    void SetIdentity();

    Vector3f TransformPoint(const Vector3f &v) const;
    SIMDVector3f TransformPoint(const SIMDVector3f &v) const;
    Vector3f TransformNormal(const Vector3f &v) const;
    SIMDVector3f TransformNormal(const SIMDVector3f &v) const;
    Ray TransformRay(const Ray &v) const;

    Vector3f UntransformPoint(const Vector3f &v) const;
    SIMDVector3f UntransformPoint(const SIMDVector3f &v) const;
    Vector3f UntransformNormal(const Vector3f &v) const;
    SIMDVector3f UntransformNormal(const SIMDVector3f &v) const;
    Ray UntransformRay(const Ray &v) const;

    AABox TransformBoundingBox(const AABox &boundingBox) const;
    Sphere TransformBoundingSphere(const Sphere &boundingSphere) const;

    // inverse transform
    Transform Inverse() const;
    void InvertInPlace();

    Matrix3x4f GetTransformMatrix3x4() const;
    Matrix4x4f GetTransformMatrix4x4() const;

    Matrix3x4f GetInverseTransformMatrix3x4() const;
    Matrix4x4f GetInverseTransformMatrix4x4() const;

    bool operator==(const Transform &transform) const;
    bool operator!=(const Transform &transform) const;
    Transform operator*(const Transform &rightSide) const;
    Transform &operator=(const Transform &transform);
    Transform &operator*=(const Transform &rightSide);

    // transform * vector
    Vector3f operator*(const Vector3f &rhs) const;
    SIMDVector3f operator*(const SIMDVector3f &rhs) const;

    Transform LinearInterpolate(const Transform &end, const float factor);

    // linear interpolation of transforms
    static Transform LinearInterpolate(const Transform &start, const Transform &end, const float factor);

    // transform concatenation. left side will happen before the right side. (reverse of matrix multiplication)
    static Transform ConcatenateTransforms(const Transform &leftSide, const Transform &rightSide);

    static const Transform &Identity;
   
private:
    Vector3f m_position;
    Quaternion m_rotation;
    Vector3f m_scale;
};

