#pragma once
#include "MathLib/Common.h"
#include "MathLib/Vectorf.h"
#include "MathLib/Vectori.h"
#include "MathLib/Vectoru.h"
#include "MathLib/Quaternion.h"
#include "MathLib/Transform.h"
#include "MathLib/SIMDVectorf.h"
#include "MathLib/SIMDVectori.h"
#include "YBaseLib/String.h"

namespace StringConverter
{
    Vector2f StringToVector2f(const char *Source);
    Vector3f StringToVector3f(const char *Source);
    Vector4f StringToVector4f(const char *Source);

    void Vector2fToString(String &Destination, const Vector2f &Source);
    void Vector3fToString(String &Destination, const Vector3f &Source);
    void Vector4fToString(String &Destination, const Vector4f &Source);

    Vector2i StringToVector2i(const char *Source);
    Vector3i StringToVector3i(const char *Source);
    Vector4i StringToVector4i(const char *Source);
    Vector2u StringToVector2u(const char *Source);
    Vector3u StringToVector3u(const char *Source);
    Vector4u StringToVector4u(const char *Source);

    void Vector2iToString(String &Destination, const Vector2i &Source);
    void Vector3iToString(String &Destination, const Vector3i &Source);
    void Vector4iToString(String &Destination, const Vector4i &Source);
    void Vector2uToString(String &Destination, const Vector2u &Source);
    void Vector3uToString(String &Destination, const Vector3u &Source);
    void Vector4uToString(String &Destination, const Vector4u &Source);

    SIMDVector2f StringToSIMDVector2f(const char *Source);
    SIMDVector3f StringToSIMDVector3f(const char *Source);
    SIMDVector4f StringToSIMDVector4f(const char *Source);

    void SIMDVector2fToString(String &Destination, const SIMDVector2f &Source);
    void SIMDVector3fToString(String &Destination, const SIMDVector3f &Source);
    void SIMDVector4fToString(String &Destination, const SIMDVector4f &Source);

    SIMDVector2i StringToSIMDVector2i(const char *Source);
    SIMDVector3i StringToSIMDVector3i(const char *Source);
    SIMDVector4i StringToSIMDVector4i(const char *Source);

    void SIMDVector2iToString(String &Destination, const SIMDVector2i &Source);
    void SIMDVector3iToString(String &Destination, const SIMDVector3i &Source);
    void SIMDVector4iToString(String &Destination, const SIMDVector4i &Source);

    Quaternion StringToQuaternion(const char *Source);
    void QuaternionToString(String &Destination, const Quaternion &Source);

    void TransformToString(String &dest, const Transform &transform);
    Transform StringToTranform(const String &str);

    TinyString Vector2fToString(const Vector2f &Source);
    TinyString Vector3fToString(const Vector3f &Source);
    TinyString Vector4fToString(const Vector4f &Source);
    TinyString Vector2iToString(const Vector2i &Source);
    TinyString Vector3iToString(const Vector3i &Source);
    TinyString Vector4iToString(const Vector4i &Source);
    TinyString Vector2uToString(const Vector2u &Source);
    TinyString Vector3uToString(const Vector3u &Source);
    TinyString Vector4uToString(const Vector4u &Source);
    TinyString SIMDVector2fToString(const SIMDVector2f &Source);
    TinyString SIMDVector3fToString(const SIMDVector3f &Source);
    TinyString SIMDVector4fToString(const SIMDVector4f &Source);
    TinyString SIMDVector2iToString(const SIMDVector2i &Source);
    TinyString SIMDVector3iToString(const SIMDVector3i &Source);
    TinyString SIMDVector4iToString(const SIMDVector4i &Source);
    TinyString QuaternionToString(const Quaternion &Source);
    SmallString TransformToString(const Transform &Source);
}

