#pragma once
#include "MathLib/Vectori.h"

// interoperability between int/vector types
struct Vector2i;
struct Vector3i;
struct Vector4i;

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct SIMDVector2i : public Vector2i
{
    // constructors
    inline SIMDVector2i() {}
    inline SIMDVector2i(int32 x_, int32 y_) : Vector2i(x_, y_) {}
    inline SIMDVector2i(const int32 *p) : Vector2i(p) {}
    inline SIMDVector2i(const SIMDVector2i &v) : Vector2i(v) {}
    inline SIMDVector2i(const Vector2i &v) : Vector2i(v) {}

    //----------------------------------------------------------------------
    static const SIMDVector2i &Zero, &One, &NegativeOne;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct SIMDVector3i : public Vector3i
{
    // constructors
    inline SIMDVector3i() {}
    inline SIMDVector3i(int32 x_, int32 y_, int32 z_) : Vector3i(x_, y_, z_) {}
    inline SIMDVector3i(const int32 *p) : Vector3i(p) {}
    inline SIMDVector3i(const SIMDVector2i &v, int32 z_ = 0) : Vector3i(v, z_) {}
    inline SIMDVector3i(const SIMDVector3i &v) : Vector3i(v) {}
    inline SIMDVector3i(const Vector3i &v) : Vector3i(v) {}

    //----------------------------------------------------------------------
    static const SIMDVector3i &Zero, &One, &NegativeOne;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct SIMDVector4i : public Vector4i
{
    // constructors
    inline SIMDVector4i() {}
    inline SIMDVector4i(int32 x_, int32 y_, int32 z_, int32 w_) : Vector4i(x_, y_, z_, w_) {}
    inline SIMDVector4i(const int32 *p) : Vector4i(p) {}
    inline SIMDVector4i(const SIMDVector2i &v, int32 z_ = 0, int32 w_ = 0) : Vector4i(v, z_) {}
    inline SIMDVector4i(const SIMDVector3i &v, int32 w_ = 0) : Vector4i(v, w_) {}
    inline SIMDVector4i(const SIMDVector4i &v) : Vector4i(v) {}
    inline SIMDVector4i(const Vector4i &v) : Vector4i(v) {}

    //----------------------------------------------------------------------
    static const SIMDVector4i &Zero, &One, &NegativeOne;
};
