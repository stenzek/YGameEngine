#pragma once
#include "MathLib/Common.h"

// interoperability between float/vector types
struct Vector2f;
struct Vector3f;
struct Vector4f;

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#pragma pack(push, 2)

struct half
{
    half() {}
    half(const half &v) : value(v.value) {}
    half(uint16 v) : value(v) {}
    half(const float f) : value(Math::FloatToHalf(f)) {}

    operator uint16() const { return value; }
    operator float() const { return Math::HalfToFloat(value); }

    half &operator=(const half &h) { value = h.value; return *this; }

private:
    uint16 value;
};

struct Vector2h
{
    Vector2h() {}
    Vector2h(const Vector2h &v) : x(v.x) , y(v.y) {}
    Vector2h(const float x_, const float y_) : x((x_)), y((y_)) {}
    Vector2h(const Vector2f &v);

    operator Vector2f() const;

private:
    half x, y;
};

struct Vector3h
{
    Vector3h() {}
    Vector3h(const Vector3h &v) : x(v.x) , y(v.y), z(v.z) {}
    Vector3h(const float x_, const float y_, const float z_) : x((x_)), y((y_)), z((z_)) {}
    Vector3h(const Vector3f &v);

    operator Vector3f() const;

private:
    half x, y, z;
};

struct Vector4h
{
    Vector4h() {}
    Vector4h(const Vector4h &v) : x(v.x) , y(v.y), z(v.z), w(v.w) {}
    Vector4h(const float x_, const float y_, const float z_, const float w_) : x((x_)), y((y_)), z((z_)), w((w_)) {}
    Vector4h(const Vector4f &v);

    operator Vector4f() const;

private:
    half x, y, z, w;
};

#pragma pack(pop)


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
