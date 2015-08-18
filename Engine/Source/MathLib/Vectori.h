#pragma once
#include "MathLib/Common.h"
#include "MathLib/VectorShuffles.h"

// interoperability between int/vector types
struct Vector2i;
struct Vector3i;
struct Vector4i;
struct SIMDVector2i;
struct SIMDVector3i;
struct SIMDVector4i;

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct Vector2i
{
    // constructors
    inline Vector2i() {}
    inline Vector2i(int32 x_, int32 y_) : x(x_), y(y_) {}
    inline Vector2i(int32 *p) : x(p[0]), y(p[1]) {}
    inline Vector2i(const Vector2i &v) : x(v.x), y(v.y) {}
    Vector2i(const SIMDVector2i &v);

    // setters
    void Set(int32 x_, int32 y_) { x = x_; y = y_; }
    void Set(const Vector2i &v) { x = v.x; y = v.y; }
    void Set(const SIMDVector2i &v);
    void SetZero() { x = y = 0; }

    // load/store
    void Load(const int32 *v) { x = v[0]; y = v[1]; }
    void Store(int32 *v) const { v[0] = x; v[1] = y; }

    // new vector
    Vector2i operator+(const Vector2i &v) const { return Vector2i(x + v.x, y + v.y); }
    Vector2i operator-(const Vector2i &v) const { return Vector2i(x - v.x, y - v.y); }
    Vector2i operator*(const Vector2i &v) const { return Vector2i(x * v.x, y * v.y); }
    Vector2i operator/(const Vector2i &v) const { return Vector2i(x / v.x, y / v.y); }
    Vector2i operator%(const Vector2i &v) const { return Vector2i(x % v.x, y % v.y); }
    Vector2i operator&(const Vector2i &v) const { return Vector2i(x & v.x, y & v.y); }
    Vector2i operator|(const Vector2i &v) const { return Vector2i(x | v.x, y | v.y); }
    Vector2i operator^(const Vector2i &v) const { return Vector2i(x ^ v.x, y ^ v.y); }

    // scalar operators
    Vector2i operator+(const int32 v) const { return Vector2i(x + v, y + v); }
    Vector2i operator-(const int32 v) const { return Vector2i(x - v, y - v); }
    Vector2i operator*(const int32 v) const { return Vector2i(x * v, y * v); }
    Vector2i operator/(const int32 v) const { return Vector2i(x / v, y / v); }
    Vector2i operator%(const int32 v) const { return Vector2i(x % v, y % v); }
    Vector2i operator&(const int32 v) const { return Vector2i(x & v, y & v); }
    Vector2i operator|(const int32 v) const { return Vector2i(x | v, y | v); }
    Vector2i operator^(const int32 v) const { return Vector2i(x ^ v, y ^ v); }

    // no params
    Vector2i operator~() const { return Vector2i(~x, ~y); }
    Vector2i operator-() const { return Vector2i(-x, -y); }

    // comparison operators
    bool operator==(const Vector2i &v) const { return (x == v.x && y == v.y); }
    bool operator!=(const Vector2i &v) const { return (x != v.x || y != v.y); }
    bool operator<=(const Vector2i &v) const { return (x <= v.x && y <= v.y); }
    bool operator>=(const Vector2i &v) const { return (x >= v.x && y >= v.y); }
    bool operator<(const Vector2i &v) const { return (x < v.x && y < v.y); }
    bool operator>(const Vector2i &v) const { return (x > v.x && y > v.y); }

    // modifies this vector
    Vector2i &operator+=(const Vector2i &v) { x += v.x; y += v.y; return *this; }
    Vector2i &operator-=(const Vector2i &v) { x -= v.x; y -= v.y; return *this; }
    Vector2i &operator*=(const Vector2i &v) { x *= v.x; y *= v.y; return *this; }
    Vector2i &operator/=(const Vector2i &v) { x /= v.x; y /= v.y; return *this; }
    Vector2i &operator%=(const Vector2i &v) { x %= v.x; y %= v.y; return *this; }
    Vector2i &operator&=(const Vector2i &v) { x &= v.x; y &= v.y; return *this; }
    Vector2i &operator|=(const Vector2i &v) { x |= v.x; y |= v.y; return *this; }
    Vector2i &operator^=(const Vector2i &v) { x ^= v.x; y ^= v.y; return *this; }
    Vector2i &operator=(const Vector2i &v) { x = v.x; y = v.y; return *this; }

    // scalar operators
    Vector2i &operator+=(const int32 v) { x += v; y += v; return *this; }
    Vector2i &operator-=(const int32 v) { x -= v; y -= v; return *this; }
    Vector2i &operator*=(const int32 v) { x *= v; y *= v; return *this; }
    Vector2i &operator/=(const int32 v) { x /= v; y /= v; return *this; }
    Vector2i &operator%=(const int32 v) { x %= v; y %= v; return *this; }
    Vector2i &operator&=(const int32 v) { x &= v; y &= v; return *this; }
    Vector2i &operator|=(const int32 v) { x |= v; y |= v; return *this; }
    Vector2i &operator^=(const int32 v) { x ^= v; y ^= v; return *this; }
    Vector2i &operator=(const int32 v) { x = v; y = v; return *this; }

    // vector2i
    Vector2i &operator=(const SIMDVector2i &v);

    // index accessors
    const int32 &operator[](uint32 i) const { return (&x)[i]; }
    int32 &operator[](uint32 i) { return (&x)[i]; }
    operator const int32 *() const { return &x; }
    operator int32 *() { return &x; }

    // partial comparisons
    bool AnyLess(const Vector2i &v) const { return (x < v.x || y < v.y); }
    bool AnyLessEqual(const Vector2i &v) const { return (x <= v.x || y <= v.y); }
    bool AnyGreater(const Vector2i &v) const { return (x > v.x || y > v.y); }
    bool AnyGreaterEqual(const Vector2i &v) const { return (x >= v.x || y >= v.y); }
    //bool NearEqual(const int2 &v, int32 Epsilon) const { return (abs(x - v.x) <= Epsilon || abs(y - v.y) <= Epsilon); }

    // modification functions
    Vector2i Min(const Vector2i &v) const { return Vector2i((x > v.x) ? v.x : x, (y > v.y) ? v.y : y); }
    Vector2i Max(const Vector2i &v) const { return Vector2i((x < v.x) ? v.x : x, (y < v.y) ? v.y : y); }
    Vector2i Clamp(const Vector2i &lBounds, const Vector2i &uBounds) const { return Min(uBounds).Max(lBounds); }
    Vector2i Abs() const { return Vector2i((x < 0) ? -x : x, (y < 0) ? -y : y); }
    Vector2i Saturate() const { return Clamp(Zero, One); }
    Vector2i Snap(const Vector2i &v) const { return Vector2i(Math::Snap(x, v.x), Math::Snap(y, v.y)); }

    // swap
    void Swap(Vector2i &v)
    {
        int32 tmp;
        tmp = v.x; v.x = x; x = tmp;
        tmp = v.y; v.y = y; y = tmp;
    }

    // maximum of all components
    int32 MinOfComponents() const { return ::Min(x, y); }
    int32 MaxOfComponents() const { return ::Max(x, y); }

    // shuffles
    template<int V0, int V1> Vector2i Shuffle2() const
    { 
        const int32 *pv = (const int32 *)&x;
        Vector2i result;
        result.x = pv[V0];
        result.y = pv[V1];
        return result;
    }
    
    VECTOR2_SHUFFLE_FUNCTIONS(Vector2i);

    //----------------------------------------------------------------------
    union
    {
        struct { int32 x, y; };
        struct { int32 r, g; };
        int32 ele[2];
    };

    //----------------------------------------------------------------------
    static const Vector2i &Zero, &One, &NegativeOne;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct Vector3i
{
    // constructors
    inline Vector3i() {}
    inline Vector3i(int32 x_, int32 y_, int32 z_) : x(x_), y(y_), z(z_) {}
    inline Vector3i(int32 *p) : x(p[0]), y(p[1]), z(p[2]) {}
    inline Vector3i(const Vector2i &v, int32 z_ = 0) : x(v.x), y(v.y), z(z_) {}
    inline Vector3i(const Vector3i &v) : x(v.x), y(v.y), z(v.z) {}
    Vector3i(const SIMDVector3i &v);

    // setters
    void Set(int32 x_, int32 y_, int32 z_) { x = x_; y = y_; z = z_; }
    void Set(const Vector2i &v, int32 z_ = 0) { x = v.x; y = v.y; z = z_; }
    void Set(const Vector3i &v) { x = v.x; y = v.y; z = v.z; }
    void Set(const SIMDVector3i &v);
    void SetZero() { x = y = z = 0; }

    // load/store
    void Load(const int32 *v) { x = v[0]; y = v[1]; z = v[2]; }
    void Store(int32 *v) const { v[0] = x; v[1] = y; v[2] = z;}

    // new vector
    Vector3i operator+(const Vector3i &v) const { return Vector3i(x + v.x, y + v.y, z + v.z); }
    Vector3i operator-(const Vector3i &v) const { return Vector3i(x - v.x, y - v.y, z - v.z); }
    Vector3i operator*(const Vector3i &v) const { return Vector3i(x * v.x, y * v.y, z * v.z); }
    Vector3i operator/(const Vector3i &v) const { return Vector3i(x / v.x, y / v.y, z / v.z); }
    Vector3i operator%(const Vector3i &v) const { return Vector3i(x % v.x, y % v.y, z % v.z); }
    Vector3i operator&(const Vector3i &v) const { return Vector3i(x & v.x, y & v.y, z & v.z); }
    Vector3i operator|(const Vector3i &v) const { return Vector3i(x | v.x, y | v.y, z | v.z); }
    Vector3i operator^(const Vector3i &v) const { return Vector3i(x ^ v.x, y ^ v.y, z ^ v.z); }

    // scalar operators
    Vector3i operator+(const int32 v) const { return Vector3i(x + v, y + v, z + v); }
    Vector3i operator-(const int32 v) const { return Vector3i(x - v, y - v, z - v); }
    Vector3i operator*(const int32 v) const { return Vector3i(x * v, y * v, z * v); }
    Vector3i operator/(const int32 v) const { return Vector3i(x / v, y / v, z / v); }
    Vector3i operator%(const int32 v) const { return Vector3i(x % v, y % v, z % v); }
    Vector3i operator&(const int32 v) const { return Vector3i(x & v, y & v, z & v); }
    Vector3i operator|(const int32 v) const { return Vector3i(x | v, y | v, z | v); }
    Vector3i operator^(const int32 v) const { return Vector3i(x ^ v, y ^ v, z ^ v); }

    // no params
    Vector3i operator~() const { return Vector3i(~x, ~y, ~z); }
    Vector3i operator-() const { return Vector3i(-x, -y, -z); }

    // comparison operators
    bool operator==(const Vector3i &v) const { return (x == v.x && y == v.y && z == v.z); }
    bool operator!=(const Vector3i &v) const { return (x != v.x || y != v.y || z != v.z); }
    bool operator<=(const Vector3i &v) const { return (x <= v.x && y <= v.y && z <= v.z); }
    bool operator>=(const Vector3i &v) const { return (x >= v.x && y >= v.y && z >= v.z); }
    bool operator<(const Vector3i &v) const { return (x < v.x && y < v.y && z < v.z); }
    bool operator>(const Vector3i &v) const { return (x > v.x && y > v.y && z > v.z); }

    // modifies this vector
    Vector3i &operator+=(const Vector3i &v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vector3i &operator-=(const Vector3i &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    Vector3i &operator*=(const Vector3i &v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
    Vector3i &operator/=(const Vector3i &v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
    Vector3i &operator%=(const Vector3i &v) { x %= v.x; y %= v.y; z %= v.z; return *this; }
    Vector3i &operator&=(const Vector3i &v) { x &= v.x; y &= v.y; z &= v.z; return *this; }
    Vector3i &operator|=(const Vector3i &v) { x |= v.x; y |= v.y; z |= v.z; return *this; }
    Vector3i &operator^=(const Vector3i &v) { x ^= v.x; y ^= v.y; z ^= v.z; return *this; }
    Vector3i &operator=(const Vector3i &v) { x = v.x; y = v.y; z = v.z; return *this; }

    // scalar operators
    Vector3i &operator+=(const int32 v) { x += v; y += v; z += v; return *this; }
    Vector3i &operator-=(const int32 v) { x -= v; y -= v; z -= v; return *this; }
    Vector3i &operator*=(const int32 v) { x *= v; y *= v; z *= v; return *this; }
    Vector3i &operator/=(const int32 v) { x /= v; y /= v; z /= v; return *this; }
    Vector3i &operator%=(const int32 v) { x %= v; y %= v; z %= v; return *this; }
    Vector3i &operator&=(const int32 v) { x &= v; y &= v; z &= v; return *this; }
    Vector3i &operator|=(const int32 v) { x |= v; y |= v; z |= v; return *this; }
    Vector3i &operator^=(const int32 v) { x ^= v; y ^= v; z ^= v; return *this; }
    Vector3i &operator=(const int32 v) { x = v; y = v; z = v; return *this; }

    // vector3i
    Vector3i &operator=(const SIMDVector3i &v);

    // index accessors
    const int32 &operator[](uint32 i) const { return (&x)[i]; }
    int32 &operator[](uint32 i) { return (&x)[i]; }
    operator const int32 *() const { return &x; }
    operator int32 *() { return &x; }

    // partial comparisons
    bool AnyLess(const Vector3i &v) const { return (x < v.x || y < v.y || z < v.z); }
    bool AnyLessEqual(const Vector3i &v) const { return (x <= v.x || y <= v.y || z <= v.z); }
    bool AnyGreater(const Vector3i &v) const { return (x > v.x || y > v.y || z > v.z); }
    bool AnyGreaterEqual(const Vector3i &v) const { return (x >= v.x || y >= v.y || z >= v.z); }
    //bool NearEqual(const int3 &v, int32 Epsilon) const { return (abs(x - v.x) <= Epsilon || abs(y - v.y) <= Epsilon || abs(z - v.z) <= Epsilon); }

    // modification functions
    Vector3i Min(const Vector3i &v) const { return Vector3i((x > v.x) ? v.x : x, (y > v.y) ? v.y : y, (z > v.z) ? v.z : z); }
    Vector3i Max(const Vector3i &v) const { return Vector3i((x < v.x) ? v.x : x, (y < v.y) ? v.y : y, (z < v.z) ? v.z : z); }
    Vector3i Clamp(const Vector3i &lBounds, const Vector3i &uBounds) const { return Min(uBounds).Max(lBounds); }
    Vector3i Abs() const { return Vector3i((x < 0) ? -x : x, (y < 0) ? -y : y, (z < 0) ? -z : z); }
    Vector3i Saturate() const { return Clamp(Zero, One); }
    Vector3i Snap(const Vector3i &v) const { return Vector3i(Math::Snap(x, v.x), Math::Snap(y, v.y), Math::Snap(z, v.z)); }

    // swap
    void Swap(Vector3i &v)
    {
        int32 tmp;
        tmp = v.x; v.x = x; x = tmp;
        tmp = v.y; v.y = y; y = tmp;
        tmp = v.z; v.z = z; z = tmp;
    }

    // maximum of all components
    int32 MinOfComponents() const { return ::Min(x, ::Min(y, z)); }
    int32 MaxOfComponents() const { return ::Max(x, ::Max(y, z)); }

    // shuffles
    template<int V0, int V1> Vector2i Shuffle2() const
    { 
        const int32 *pv = (const int32 *)&x;
        Vector2i result;
        result.x = pv[V0];
        result.y = pv[V1];
        return result;
    }

    template<int V0, int V1, int V2> Vector3i Shuffle3() const 
    { 
        const int32 *pv = (const int32 *)&x;
        Vector3i result;
        result.x = pv[V0];
        result.y = pv[V1];
        result.z = pv[V2];
        return result;
    }

    VECTOR3_SHUFFLE_FUNCTIONS(Vector2i, Vector3i);

    //----------------------------------------------------------------------
    union
    {
        struct { int32 x, y, z; };
        struct { int32 r, g, b; };
        int32 ele[3];
    };

    //----------------------------------------------------------------------
    static const Vector3i &Zero, &One, &NegativeOne;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct Vector4i
{
    // constructors
    inline Vector4i() {}
    inline Vector4i(int32 x_, int32 y_, int32 z_, int32 w_) : x(x_), y(y_), z(z_), w(w_) {}
    inline Vector4i(int32 *p) : x(p[0]), y(p[1]), z(p[2]), w(p[3]) {}
    inline Vector4i(const Vector2i &v, int32 z_ = 0, int32 w_ = 0) : x(v.x), y(v.y), z(z_), w(w_) {}
    inline Vector4i(const Vector3i &v, int32 w_ = 0) : x(v.x), y(v.y), z(v.z), w(w_) {}
    inline Vector4i(const Vector4i &v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
    Vector4i(const SIMDVector4i &v);

    // setters
    void Set(int32 x_, int32 y_, int32 z_, int32 w_) { x = x_; y = y_; z = z_; w = w_; }
    void Set(const Vector2i &v, int32 z_ = 0, int32 w_ = 0) { x = v.x; y = v.y; z = z_; w = w_; }
    void Set(const Vector3i &v, int32 w_ = 0) { x = v.x; y = v.y; z = v.z; w = w_; }
    void Set(const Vector4i &v) { x = v.x; y = v.y; z = v.z; w = v.w; }
    void Set(const SIMDVector4i &v);
    void SetZero() { x = y = z = w = 0; }

    // load/store
    void Load(const int32 *v) { x = v[0]; y = v[1]; z = v[2]; w = v[3]; }
    void Store(int32 *v) const { v[0] = x; v[1] = y; v[2] = z; v[3] = w; }

    // new vector
    Vector4i operator+(const Vector4i &v) const { return Vector4i(x + v.x, y + v.y, z + v.z, w + v.w); }
    Vector4i operator-(const Vector4i &v) const { return Vector4i(x - v.x, y - v.y, z - v.z, w - v.w); }
    Vector4i operator*(const Vector4i &v) const { return Vector4i(x * v.x, y * v.y, z * v.z, w * v.w); }
    Vector4i operator/(const Vector4i &v) const { return Vector4i(x / v.x, y / v.y, z / v.z, w / v.w); }
    Vector4i operator%(const Vector4i &v) const { return Vector4i(x % v.x, y % v.y, z % v.z, w % v.w); }
    Vector4i operator&(const Vector4i &v) const { return Vector4i(x & v.x, y & v.y, z & v.z, w & v.w); }
    Vector4i operator|(const Vector4i &v) const { return Vector4i(x | v.x, y | v.y, z | v.z, w | v.w); }
    Vector4i operator^(const Vector4i &v) const { return Vector4i(x ^ v.x, y ^ v.y, z ^ v.z, w ^ v.w); }

    // scalar operators
    Vector4i operator+(const int32 v) const { return Vector4i(x + v, y + v, z + v, w + v); }
    Vector4i operator-(const int32 v) const { return Vector4i(x - v, y - v, z - v, w - v); }
    Vector4i operator*(const int32 v) const { return Vector4i(x * v, y * v, z * v, w * v); }
    Vector4i operator/(const int32 v) const { return Vector4i(x / v, y / v, z / v, w / v); }
    Vector4i operator%(const int32 v) const { return Vector4i(x % v, y % v, z % v, w % v); }
    Vector4i operator&(const int32 v) const { return Vector4i(x & v, y & v, z & v, w & v); }
    Vector4i operator|(const int32 v) const { return Vector4i(x | v, y | v, z | v, w | v); }
    Vector4i operator^(const int32 v) const { return Vector4i(x ^ v, y ^ v, z ^ v, w ^ v); }

    // no params
    Vector4i operator~() const { return Vector4i(~x, ~y, ~z, ~w); }
    Vector4i operator-() const { return Vector4i(-x, -y, -z, ~w); }

    // comparison operators
    bool operator==(const Vector4i &v) const { return (x == v.x && y == v.y && z == v.z && w == v.w); }
    bool operator!=(const Vector4i &v) const { return (x != v.x || y != v.y || z != v.z || w != v.w); }
    bool operator<=(const Vector4i &v) const { return (x <= v.x && y <= v.y && z <= v.z && w <= v.w); }
    bool operator>=(const Vector4i &v) const { return (x >= v.x && y >= v.y && z >= v.z && w >= v.w); }
    bool operator<(const Vector4i &v) const { return (x < v.x && y < v.y && z < v.z && w < v.w); }
    bool operator>(const Vector4i &v) const { return (x > v.x && y > v.y && z > v.z && w > v.w); }

    // modifies this vector
    Vector4i &operator+=(const Vector4i &v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    Vector4i &operator-=(const Vector4i &v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    Vector4i &operator*=(const Vector4i &v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
    Vector4i &operator/=(const Vector4i &v) { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }
    Vector4i &operator%=(const Vector4i &v) { x %= v.x; y %= v.y; z %= v.z; w %= v.w; return *this; }
    Vector4i &operator&=(const Vector4i &v) { x &= v.x; y &= v.y; z &= v.z; w &= v.w; return *this; }
    Vector4i &operator|=(const Vector4i &v) { x |= v.x; y |= v.y; z |= v.z; w |= v.w; return *this; }
    Vector4i &operator^=(const Vector4i &v) { x ^= v.x; y ^= v.y; z ^= v.z; w ^= v.w; return *this; }
    Vector4i &operator=(const Vector4i &v) { x = v.x; y = v.y; z = v.z; w = v.w; return *this; }

    // scalar operators
    Vector4i &operator+=(const int32 v) { x += v; y += v; z += v; w += v; return *this; }
    Vector4i &operator-=(const int32 v) { x -= v; y -= v; z -= v; w -= v; return *this; }
    Vector4i &operator*=(const int32 v) { x *= v; y *= v; z *= v; w *= v; return *this; }
    Vector4i &operator/=(const int32 v) { x /= v; y /= v; z /= v; w /= v; return *this; }
    Vector4i &operator%=(const int32 v) { x %= v; y %= v; z %= v; w %= v; return *this; }
    Vector4i &operator&=(const int32 v) { x &= v; y &= v; z &= v; w &= v; return *this; }
    Vector4i &operator|=(const int32 v) { x |= v; y |= v; z |= v; w |= v; return *this; }
    Vector4i &operator^=(const int32 v) { x ^= v; y ^= v; z ^= v; w ^= v; return *this; }
    Vector4i &operator=(const int32 v) { x = v; y = v; z = v; w = v; return *this; }

    // vector4i
    Vector4i &operator=(const SIMDVector4i &v);

    // index accessors
    const int32 &operator[](uint32 i) const { return (&x)[i]; }
    int32 &operator[](uint32 i) { return (&x)[i]; }
    operator const int32 *() const { return &x; }
    operator int32 *() { return &x; }

    // partial comparisons
    bool AnyLess(const Vector4i &v) const { return (x < v.x || y < v.y || z < v.z || w < v.w); }
    bool AnyLessEqual(const Vector4i &v) const { return (x <= v.x || y <= v.y || z <= v.z || w <= v.w); }
    bool AnyGreater(const Vector4i &v) const { return (x > v.x || y > v.y || z > v.z || w > v.w); }
    bool AnyGreaterEqual(const Vector4i &v) const { return (x >= v.x || y >= v.y || z >= v.z || w >= v.w); }
    //bool NearEqual(const int4 &v, int32 Epsilon) const { return (abs(x - v.x) <= Epsilon || abs(y - v.y) <= Epsilon || abs(z - v.z) <= Epsilon || abs(w - v.w) < Epsilon); }

    // modification functions
    Vector4i Min(const Vector4i &v) const { return Vector4i((x > v.x) ? v.x : x, (y > v.y) ? v.y : y, (z > v.z) ? v.z : z, (w > v.w) ? v.w : w); }
    Vector4i Max(const Vector4i &v) const { return Vector4i((x < v.x) ? v.x : x, (y < v.y) ? v.y : y, (z < v.z) ? v.z : z, (w < v.w) ? v.w : w); }
    Vector4i Clamp(const Vector4i &lBounds, const Vector4i &uBounds) const { return Min(uBounds).Max(lBounds); }
    Vector4i Abs() const { return Vector4i((x < 0) ? -x : x, (y < 0) ? -y : y, (z < 0) ? -z : z, (w < 0) ? -w : w); }
    Vector4i Saturate() const { return Clamp(Zero, One); }
    Vector4i Snap(const Vector4i &v) const { return Vector4i(Math::Snap(x, v.x), Math::Snap(y, v.y), Math::Snap(z, v.z), Math::Snap(w, v.w)); }

    // swap
    void Swap(Vector4i &v)
    {
        int32 tmp;
        tmp = v.x; v.x = x; x = tmp;
        tmp = v.y; v.y = y; y = tmp;
        tmp = v.z; v.z = z; z = tmp;
        tmp = v.w; v.w = w; w = tmp;
    }

    // maximum of all components
    int32 MinOfComponents() const { return ::Min(x, ::Min(y, ::Min(z, w))); }
    int32 MaxOfComponents() const { return ::Max(x, ::Max(y, ::Max(z, w))); }

    // shuffles
    template<int V0, int V1> Vector2i Shuffle2() const
    { 
        const int32 *pv = (const int32 *)&x;
        Vector2i result;
        result.x = pv[V0];
        result.y = pv[V1];
        return result;
    }

    template<int V0, int V1, int V2> Vector3i Shuffle3() const 
    { 
        const int32 *pv = (const int32 *)&x;
        Vector3i result;
        result.x = pv[V0];
        result.y = pv[V1];
        result.z = pv[V2];
        return result;
    }

    template<int V0, int V1, int V2, int V3> Vector4i Shuffle4() const 
    {
        const int32 *pv = (const int32 *)&x;
        Vector4i result;
        result.x = pv[V0];
        result.y = pv[V1];
        result.z = pv[V2];
        result.w = pv[V3];
        return result;
    }

    VECTOR4_SHUFFLE_FUNCTIONS(Vector2i, Vector3i, Vector4i);

    //----------------------------------------------------------------------
    union
    {
        struct { int32 x, y, z, w; };
        struct { int32 r, g, b, a; };
        int32 ele[4];
    };

    //----------------------------------------------------------------------
    static const Vector4i &Zero, &One, &NegativeOne;
};
