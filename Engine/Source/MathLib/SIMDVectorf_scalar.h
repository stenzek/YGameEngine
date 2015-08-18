#pragma once
#include "MathLib/Vectorf.h"

// basically a massive hack, since we forward declare the simd types, we can't typedef them
// so we just inherit them instead..
struct SIMDVector2i;
struct SIMDVector3i;
struct SIMDVector4i;

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct SIMDVector2f : public Vector2f
{
    // constructors
    inline SIMDVector2f() {}
    inline SIMDVector2f(const float val) : Vector2f(val) {}
    inline SIMDVector2f(const float x_, const float y_) : Vector2f(x_, y_) {}
    inline SIMDVector2f(const float *p) : Vector2f(p) {}
    inline SIMDVector2f(const SIMDVector2f &v) : Vector2f(v) {}
    inline SIMDVector2f(const Vector2f &v) : Vector2f(v) {}
    inline SIMDVector2f(const Vector2i &v) : Vector2f(v) {}

    // new vector
    inline SIMDVector2f operator+(const SIMDVector2f &v) const { return Vector2f::operator+(v); }
    inline SIMDVector2f operator-(const SIMDVector2f &v) const { return Vector2f::operator-(v); }
    inline SIMDVector2f operator*(const SIMDVector2f &v) const { return Vector2f::operator*(v); }
    inline SIMDVector2f operator/(const SIMDVector2f &v) const { return Vector2f::operator/(v); }
    inline SIMDVector2f operator%(const SIMDVector2f &v) const { return Vector2f::operator%(v); }
    inline SIMDVector2f operator-() const { return Vector2f::operator-(); }

    // scalar operators
    inline SIMDVector2f operator+(const float v) const { return Vector2f::operator+(v); }
    inline SIMDVector2f operator-(const float v) const { return Vector2f::operator-(v); }
    inline SIMDVector2f operator*(const float v) const { return Vector2f::operator*(v); }
    inline SIMDVector2f operator/(const float v) const { return Vector2f::operator/(v); }
    inline SIMDVector2f operator%(const float v) const { return Vector2f::operator%(v); }

    // comparison operators
    inline bool operator==(const SIMDVector2f &v) const { return Vector2f::operator==(v); }
    inline bool operator!=(const SIMDVector2f &v) const { return Vector2f::operator!=(v); }
    inline bool operator<=(const SIMDVector2f &v) const { return Vector2f::operator<=(v); }
    inline bool operator>=(const SIMDVector2f &v) const { return Vector2f::operator>=(v); }
    inline bool operator<(const SIMDVector2f &v) const { return Vector2f::operator<(v); }
    inline bool operator>(const SIMDVector2f &v) const { return Vector2f::operator>(v); }

    // modifies this vector
    inline SIMDVector2f &operator+=(const SIMDVector2f &v) { Vector2f::operator+=(v); return *this; }
    inline SIMDVector2f &operator-=(const SIMDVector2f &v) { Vector2f::operator-=(v); return *this; }
    inline SIMDVector2f &operator*=(const SIMDVector2f &v) { Vector2f::operator*=(v); return *this; }
    inline SIMDVector2f &operator/=(const SIMDVector2f &v) { Vector2f::operator/=(v); return *this; }
    inline SIMDVector2f &operator%=(const SIMDVector2f &v) { Vector2f::operator%=(v); return *this; }
    inline SIMDVector2f &operator=(const SIMDVector2f &v) { Vector2f::operator=(v); return *this; }
    inline SIMDVector2f &operator=(const Vector2f &v) { Vector2f::operator=(v); return *this; }

    // scalar operators
    inline SIMDVector2f &operator+=(const float v) { Vector2f::operator+=(v); return *this; }
    inline SIMDVector2f &operator-=(const float v) { Vector2f::operator-=(v); return *this; }
    inline SIMDVector2f &operator*=(const float v) { Vector2f::operator*=(v); return *this; }
    inline SIMDVector2f &operator/=(const float v) { Vector2f::operator/=(v); return *this; }
    inline SIMDVector2f &operator%=(const float v) { Vector2f::operator%=(v); return *this; }
    
    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    static const SIMDVector2f &Zero, &One, &NegativeOne;
    static const SIMDVector2f &Infinite, &NegativeInfinite;
    static const SIMDVector2f &UnitX, &UnitY, &NegativeUnitX, &NegativeUnitY;
    static const SIMDVector2f &Epsilon;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct SIMDVector3f : public Vector3f
{
    // constructors
    inline SIMDVector3f() {}
    inline SIMDVector3f(const float val) : Vector3f(val) {}
    inline SIMDVector3f(const float x_, const float y_, const float z_) : Vector3f(x_, y_, z_) {}
    inline SIMDVector3f(const float *p) : Vector3f(p) {}
    inline SIMDVector3f(const SIMDVector2f &v, const float &z_ = 0.0f) : Vector3f(v, z_) {}
    inline SIMDVector3f(const SIMDVector3f &v) : Vector3f(v) {}
    inline SIMDVector3f(const Vector3i &v) : Vector3f(v) {}
    inline SIMDVector3f(const Vector3f &v) : Vector3f(v) {}

    // new vector
    inline SIMDVector3f operator+(const SIMDVector3f &v) const { return Vector3f::operator+(v); }
    inline SIMDVector3f operator-(const SIMDVector3f &v) const { return Vector3f::operator-(v); }
    inline SIMDVector3f operator*(const SIMDVector3f &v) const { return Vector3f::operator*(v); }
    inline SIMDVector3f operator/(const SIMDVector3f &v) const { return Vector3f::operator/(v); }
    inline SIMDVector3f operator%(const SIMDVector3f &v) const { return Vector3f::operator%(v); }
    inline SIMDVector3f operator-() const { return Vector3f::operator-(); }

    // scalar operators
    inline SIMDVector3f operator+(const float v) const { return Vector3f::operator+(v); }
    inline SIMDVector3f operator-(const float v) const { return Vector3f::operator-(v); }
    inline SIMDVector3f operator*(const float v) const { return Vector3f::operator*(v); }
    inline SIMDVector3f operator/(const float v) const { return Vector3f::operator/(v); }
    inline SIMDVector3f operator%(const float v) const { return Vector3f::operator%(v); }

    // comparison operators
    inline bool operator==(const SIMDVector3f &v) const { return Vector3f::operator==(v); }
    inline bool operator!=(const SIMDVector3f &v) const { return Vector3f::operator!=(v); }
    inline bool operator<=(const SIMDVector3f &v) const { return Vector3f::operator<=(v); }
    inline bool operator>=(const SIMDVector3f &v) const { return Vector3f::operator>=(v); }
    inline bool operator<(const SIMDVector3f &v) const { return Vector3f::operator<(v); }
    inline bool operator>(const SIMDVector3f &v) const { return Vector3f::operator>(v); }

    // modifies this vector
    inline SIMDVector3f &operator+=(const SIMDVector3f &v) { Vector3f::operator+=(v); return *this; }
    inline SIMDVector3f &operator-=(const SIMDVector3f &v) { Vector3f::operator-=(v); return *this; }
    inline SIMDVector3f &operator*=(const SIMDVector3f &v) { Vector3f::operator*=(v); return *this; }
    inline SIMDVector3f &operator/=(const SIMDVector3f &v) { Vector3f::operator/=(v); return *this; }
    inline SIMDVector3f &operator%=(const SIMDVector3f &v) { Vector3f::operator%=(v); return *this; }
    inline SIMDVector3f &operator=(const SIMDVector3f &v) { Vector3f::operator=(v); return *this; }
    inline SIMDVector3f &operator=(const Vector3f &v) { Vector3f::operator=(v); return *this; }

    // scalar operators
    inline SIMDVector3f &operator+=(const float v) { Vector3f::operator+=(v); return *this; }
    inline SIMDVector3f &operator-=(const float v) { Vector3f::operator-=(v); return *this; }
    inline SIMDVector3f &operator*=(const float v) { Vector3f::operator*=(v); return *this; }
    inline SIMDVector3f &operator/=(const float v) { Vector3f::operator/=(v); return *this; }
    inline SIMDVector3f &operator%=(const float v) { Vector3f::operator%=(v); return *this; }

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    static const SIMDVector3f &Zero, &One, &NegativeOne;
    static const SIMDVector3f &Infinite, &NegativeInfinite;
    static const SIMDVector3f &UnitX, &UnitY, &UnitZ, &NegativeUnitX, &NegativeUnitY, &NegativeUnitZ;
    static const SIMDVector3f &Epsilon;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct SIMDVector4f : public Vector4f
{
    // constructors
    inline SIMDVector4f() {}
    inline SIMDVector4f(const float val) : Vector4f(val) {}
    inline SIMDVector4f(const float x_, const float y_, const float z_, const float w_) : Vector4f(x_, y_, z_, w_) {}
    inline SIMDVector4f(const float *p) : Vector4f(p) {}
    inline SIMDVector4f(const SIMDVector2f &v, const float z_ = 0.0f, const float w_ = 0.0f) : Vector4f(v, z_, w_) {}
    inline SIMDVector4f(const SIMDVector3f &v, const float w_ = 0.0f) : Vector4f(v, w_) {}
    inline SIMDVector4f(const SIMDVector4f &v) : Vector4f(v) {}
    inline SIMDVector4f(const Vector4i &v) : Vector4f(v) {}
    inline SIMDVector4f(const Vector4f &v) : Vector4f(v) {}

    // new vector
    inline SIMDVector4f operator+(const SIMDVector4f &v) const { return Vector4f::operator+(v); }
    inline SIMDVector4f operator-(const SIMDVector4f &v) const { return Vector4f::operator-(v); }
    inline SIMDVector4f operator*(const SIMDVector4f &v) const { return Vector4f::operator*(v); }
    inline SIMDVector4f operator/(const SIMDVector4f &v) const { return Vector4f::operator/(v); }
    inline SIMDVector4f operator%(const SIMDVector4f &v) const { return Vector4f::operator%(v); }
    inline SIMDVector4f operator-() const { return Vector4f::operator-(); }

    // scalar operators
    inline SIMDVector4f operator+(const float v) const { return Vector4f::operator+(v); }
    inline SIMDVector4f operator-(const float v) const { return Vector4f::operator-(v); }
    inline SIMDVector4f operator*(const float v) const { return Vector4f::operator*(v); }
    inline SIMDVector4f operator/(const float v) const { return Vector4f::operator/(v); }
    inline SIMDVector4f operator%(const float v) const { return Vector4f::operator%(v); }

    // comparison operators
    inline bool operator==(const SIMDVector4f &v) const { return Vector4f::operator==(v); }
    inline bool operator!=(const SIMDVector4f &v) const { return Vector4f::operator!=(v); }
    inline bool operator<=(const SIMDVector4f &v) const { return Vector4f::operator<=(v); }
    inline bool operator>=(const SIMDVector4f &v) const { return Vector4f::operator>=(v); }
    inline bool operator<(const SIMDVector4f &v) const { return Vector4f::operator<(v); }
    inline bool operator>(const SIMDVector4f &v) const { return Vector4f::operator>(v); }

    // modifies this vector
    inline SIMDVector4f &operator+=(const SIMDVector4f &v) { Vector4f::operator+=(v); return *this; }
    inline SIMDVector4f &operator-=(const SIMDVector4f &v) { Vector4f::operator-=(v); return *this; }
    inline SIMDVector4f &operator*=(const SIMDVector4f &v) { Vector4f::operator*=(v); return *this; }
    inline SIMDVector4f &operator/=(const SIMDVector4f &v) { Vector4f::operator/=(v); return *this; }
    inline SIMDVector4f &operator%=(const SIMDVector4f &v) { Vector4f::operator%=(v); return *this; }
    inline SIMDVector4f &operator=(const SIMDVector4f &v) { Vector4f::operator=(v); return *this; }
    inline SIMDVector4f &operator=(const Vector4f &v) { Vector4f::operator=(v); return *this; }

    // scalar operators
    inline SIMDVector4f &operator+=(const float v) { Vector4f::operator+=(v); return *this; }
    inline SIMDVector4f &operator-=(const float v) { Vector4f::operator-=(v); return *this; }
    inline SIMDVector4f &operator*=(const float v) { Vector4f::operator*=(v); return *this; }
    inline SIMDVector4f &operator/=(const float v) { Vector4f::operator/=(v); return *this; }
    inline SIMDVector4f &operator%=(const float v) { Vector4f::operator%=(v); return *this; }
    
    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    static const SIMDVector4f &Zero, &One, &NegativeOne;
    static const SIMDVector4f &Infinite, &NegativeInfinite;
    static const SIMDVector4f &UnitX, &UnitY, &UnitZ, &UnitW, &NegativeUnitX, &NegativeUnitY, &NegativeUnitZ, &NegativeUnitW;
    static const SIMDVector4f &Epsilon;
};

