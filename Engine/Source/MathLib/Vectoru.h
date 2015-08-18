#pragma once
#include "MathLib/Common.h"
#include "MathLib/VectorShuffles.h"

// interoperability between int/vector types
struct Vector2i;
struct Vector2ui;
struct Vector3i;
struct Vector3ui;
struct Vector4i;
struct Vector4ui;

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct Vector2u
{
    // constructors
    inline Vector2u() {}
    inline Vector2u(uint32 x_, uint32 y_) : x(x_), y(y_) {}
    inline Vector2u(uint32 *p) : x(p[0]), y(p[1]) {}
    inline Vector2u(const Vector2u &v) : x(v.x), y(v.y) {}
    Vector2u(const Vector2ui &v);

    // setters
    void Set(uint32 x_, uint32 y_) { x = x_; y = y_; }
    void Set(const Vector2u &v) { x = v.x; y = v.y; }
    void Set(const Vector2ui &v);
    void SetZero() { x = y = 0; }

    // load/store
    void Load(const uint32 *v) { x = v[0]; y = v[1]; }
    void Store(uint32 *v) const { v[0] = x; v[1] = y; }

    // new vector
    Vector2u operator+(const Vector2u &v) const { return Vector2u(x + v.x, y + v.y); }
    Vector2u operator-(const Vector2u &v) const { return Vector2u(x - v.x, y - v.y); }
    Vector2u operator*(const Vector2u &v) const { return Vector2u(x * v.x, y * v.y); }
    Vector2u operator/(const Vector2u &v) const { return Vector2u(x / v.x, y / v.y); }
    Vector2u operator%(const Vector2u &v) const { return Vector2u(x % v.x, y % v.y); }
    Vector2u operator&(const Vector2u &v) const { return Vector2u(x & v.x, y & v.y); }
    Vector2u operator|(const Vector2u &v) const { return Vector2u(x | v.x, y | v.y); }
    Vector2u operator^(const Vector2u &v) const { return Vector2u(x ^ v.x, y ^ v.y); }

    // scalar operators
    Vector2u operator+(const uint32 v) const { return Vector2u(x + v, y + v); }
    Vector2u operator-(const uint32 v) const { return Vector2u(x - v, y - v); }
    Vector2u operator*(const uint32 v) const { return Vector2u(x * v, y * v); }
    Vector2u operator/(const uint32 v) const { return Vector2u(x / v, y / v); }
    Vector2u operator%(const uint32 v) const { return Vector2u(x % v, y % v); }
    Vector2u operator&(const uint32 v) const { return Vector2u(x & v, y & v); }
    Vector2u operator|(const uint32 v) const { return Vector2u(x | v, y | v); }
    Vector2u operator^(const uint32 v) const { return Vector2u(x ^ v, y ^ v); }

    // no params
    Vector2u operator~() const { return Vector2u(~x, ~y); }

    // comparison operators
    bool operator==(const Vector2u &v) const { return (x == v.x && y == v.y); }
    bool operator!=(const Vector2u &v) const { return (x != v.x || y != v.y); }
    bool operator<=(const Vector2u &v) const { return (x <= v.x && y <= v.y); }
    bool operator>=(const Vector2u &v) const { return (x >= v.x && y >= v.y); }
    bool operator<(const Vector2u &v) const { return (x < v.x && y < v.y); }
    bool operator>(const Vector2u &v) const { return (x > v.x && y > v.y); }

    // modifies this vector
    Vector2u &operator+=(const Vector2u &v) { x += v.x; y += v.y; return *this; }
    Vector2u &operator-=(const Vector2u &v) { x -= v.x; y -= v.y; return *this; }
    Vector2u &operator*=(const Vector2u &v) { x *= v.x; y *= v.y; return *this; }
    Vector2u &operator/=(const Vector2u &v) { x /= v.x; y /= v.y; return *this; }
    Vector2u &operator%=(const Vector2u &v) { x %= v.x; y %= v.y; return *this; }
    Vector2u &operator&=(const Vector2u &v) { x &= v.x; y &= v.y; return *this; }
    Vector2u &operator|=(const Vector2u &v) { x |= v.x; y |= v.y; return *this; }
    Vector2u &operator^=(const Vector2u &v) { x ^= v.x; y ^= v.y; return *this; }
    Vector2u &operator=(const Vector2u &v) { x = v.x; y = v.y; return *this; }

    // scalar operators
    Vector2u &operator+=(const uint32 v) { x += v; y += v; return *this; }
    Vector2u &operator-=(const uint32 v) { x -= v; y -= v; return *this; }
    Vector2u &operator*=(const uint32 v) { x *= v; y *= v; return *this; }
    Vector2u &operator/=(const uint32 v) { x /= v; y /= v; return *this; }
    Vector2u &operator%=(const uint32 v) { x %= v; y %= v; return *this; }
    Vector2u &operator&=(const uint32 v) { x &= v; y &= v; return *this; }
    Vector2u &operator|=(const uint32 v) { x |= v; y |= v; return *this; }
    Vector2u &operator^=(const uint32 v) { x ^= v; y ^= v; return *this; }
    Vector2u &operator=(const uint32 v) { x = v; y = v; return *this; }

    // Vector2ui
    Vector2u &operator=(const Vector2ui &v);

    // index accessors
    const uint32 &operator[](uint32 i) const { return (&x)[i]; }
    uint32 &operator[](uint32 i) { return (&x)[i]; }
    operator const uint32 *() const { return &x; }
    operator uint32 *() { return &x; }

    // to vectorx
    Vector2ui GetVector2ui() const;

    // partial comparisons
    bool AnyLess(const Vector2u &v) const { return (x < v.x || y < v.y); }
    bool AnyGreater(const Vector2u &v) const { return (x > v.x || y > v.y); }
    //bool NearEqual(const uint2 &v, uint32 Epsilon) const { return (abs(x - v.x) <= Epsilon || abs(y - v.y) <= Epsilon); }

    // modification functions
    Vector2u Min(const Vector2u &v) const { return Vector2u((x > v.x) ? v.x : x, (y > v.y) ? v.y : y); }
    Vector2u Max(const Vector2u &v) const { return Vector2u((x < v.x) ? v.x : x, (y < v.y) ? v.y : y); }
    Vector2u Clamp(const Vector2u &lBounds, const Vector2u &uBounds) const { return Min(uBounds).Max(lBounds); }
    Vector2u Saturate() const { return Clamp(Zero, One); }
    Vector2u Snap(const Vector2u &v) const { return Vector2u(Math::Snap(x, v.x), Math::Snap(y, v.y)); }

    // swap
    void Swap(Vector2u &v)
    {
        uint32 tmp;
        tmp = v.x; v.x = x; x = tmp;
        tmp = v.y; v.y = y; y = tmp;
    }

    // maximum of all components
    uint32 MinOfComponents() const { return ::Min(x, y); }
    uint32 MaxOfComponents() const { return ::Max(x, y); }

    // shuffles
    template<int V0, int V1> Vector2u Shuffle2() const
    { 
        const uint32 *pv = (const uint32 *)&x;
        Vector2u result;
        result.x = pv[V0];
        result.y = pv[V1];
        return result;
    }
    
    VECTOR2_SHUFFLE_FUNCTIONS(Vector2u);

    //----------------------------------------------------------------------
    union
    {
        struct { uint32 x, y; };
        struct { uint32 r, g; };
        uint32 ele[2];
    };

    //----------------------------------------------------------------------
    static const Vector2u &Zero, &One;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct Vector3u
{
    // constructors
    inline Vector3u() {}
    inline Vector3u(uint32 x_, uint32 y_, uint32 z_) : x(x_), y(y_), z(z_) {}
    inline Vector3u(uint32 *p) : x(p[0]), y(p[1]), z(p[2]) {}
    inline Vector3u(const Vector2u &v, uint32 z_ = 0) : x(v.x), y(v.y), z(z_) {}
    inline Vector3u(const Vector3u &v) : x(v.x), y(v.y), z(v.z) {}
    Vector3u(const Vector3ui &v);

    // setters
    void Set(uint32 x_, uint32 y_, uint32 z_) { x = x_; y = y_; z = z_; }
    void Set(const Vector2u &v, uint32 z_ = 0) { x = v.x; y = v.y; z = z_; }
    void Set(const Vector3u &v) { x = v.x; y = v.y; z = v.z; }
    void Set(const Vector3ui &v);
    void SetZero() { x = y = z = 0; }

    // load/store
    void Load(const uint32 *v) { x = v[0]; y = v[1]; z = v[2]; }
    void Store(uint32 *v) const { v[0] = x; v[1] = y; v[2] = z;}

    // new vector
    Vector3u operator+(const Vector3u &v) const { return Vector3u(x + v.x, y + v.y, z + v.z); }
    Vector3u operator-(const Vector3u &v) const { return Vector3u(x - v.x, y - v.y, z - v.z); }
    Vector3u operator*(const Vector3u &v) const { return Vector3u(x * v.x, y * v.y, z * v.z); }
    Vector3u operator/(const Vector3u &v) const { return Vector3u(x / v.x, y / v.y, z / v.z); }
    Vector3u operator%(const Vector3u &v) const { return Vector3u(x % v.x, y % v.y, z % v.z); }
    Vector3u operator&(const Vector3u &v) const { return Vector3u(x & v.x, y & v.y, z & v.z); }
    Vector3u operator|(const Vector3u &v) const { return Vector3u(x | v.x, y | v.y, z | v.z); }
    Vector3u operator^(const Vector3u &v) const { return Vector3u(x ^ v.x, y ^ v.y, z ^ v.z); }

    // scalar operators
    Vector3u operator+(const uint32 v) const { return Vector3u(x + v, y + v, z + v); }
    Vector3u operator-(const uint32 v) const { return Vector3u(x - v, y - v, z - v); }
    Vector3u operator*(const uint32 v) const { return Vector3u(x * v, y * v, z * v); }
    Vector3u operator/(const uint32 v) const { return Vector3u(x / v, y / v, z / v); }
    Vector3u operator%(const uint32 v) const { return Vector3u(x % v, y % v, z % v); }
    Vector3u operator&(const uint32 v) const { return Vector3u(x & v, y & v, z & v); }
    Vector3u operator|(const uint32 v) const { return Vector3u(x | v, y | v, z | v); }
    Vector3u operator^(const uint32 v) const { return Vector3u(x ^ v, y ^ v, z ^ v); }

    // no params
    Vector3u operator~() const { return Vector3u(~x, ~y, ~z); }

    // comparison operators
    bool operator==(const Vector3u &v) const { return (x == v.x && y == v.y && z == v.z); }
    bool operator!=(const Vector3u &v) const { return (x != v.x || y != v.y || z != v.z); }
    bool operator<=(const Vector3u &v) const { return (x <= v.x && y <= v.y && z <= v.z); }
    bool operator>=(const Vector3u &v) const { return (x >= v.x && y >= v.y && z >= v.z); }
    bool operator<(const Vector3u &v) const { return (x < v.x && y < v.y && z < v.z); }
    bool operator>(const Vector3u &v) const { return (x > v.x && y > v.y && z > v.z); }

    // modifies this vector
    Vector3u &operator+=(const Vector3u &v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vector3u &operator-=(const Vector3u &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    Vector3u &operator*=(const Vector3u &v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
    Vector3u &operator/=(const Vector3u &v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
    Vector3u &operator%=(const Vector3u &v) { x %= v.x; y %= v.y; z %= v.z; return *this; }
    Vector3u &operator&=(const Vector3u &v) { x &= v.x; y &= v.y; z &= v.z; return *this; }
    Vector3u &operator|=(const Vector3u &v) { x |= v.x; y |= v.y; z |= v.z; return *this; }
    Vector3u &operator^=(const Vector3u &v) { x ^= v.x; y ^= v.y; z ^= v.z; return *this; }
    Vector3u &operator=(const Vector3u &v) { x = v.x; y = v.y; z = v.z; return *this; }

    // scalar operators
    Vector3u &operator+=(const uint32 v) { x += v; y += v; z += v; return *this; }
    Vector3u &operator-=(const uint32 v) { x -= v; y -= v; z -= v; return *this; }
    Vector3u &operator*=(const uint32 v) { x *= v; y *= v; z *= v; return *this; }
    Vector3u &operator/=(const uint32 v) { x /= v; y /= v; z /= v; return *this; }
    Vector3u &operator%=(const uint32 v) { x %= v; y %= v; z %= v; return *this; }
    Vector3u &operator&=(const uint32 v) { x &= v; y &= v; z &= v; return *this; }
    Vector3u &operator|=(const uint32 v) { x |= v; y |= v; z |= v; return *this; }
    Vector3u &operator^=(const uint32 v) { x ^= v; y ^= v; z ^= v; return *this; }
    Vector3u &operator=(const uint32 v) { x = v; y = v; z = v; return *this; }

    // vector3i
    Vector3u &operator=(const Vector3i &v);

    // index accessors
    const uint32 &operator[](uint32 i) const { return (&x)[i]; }
    uint32 &operator[](uint32 i) { return (&x)[i]; }
    operator const uint32 *() const { return &x; }
    operator uint32 *() { return &x; }

    // to vectorx
    Vector3ui GetVector3ui() const;

    // partial comparisons
    bool AnyLess(const Vector3u &v) const { return (x < v.x || y < v.y || z < v.z); }
    bool AnyGreater(const Vector3u &v) const { return (x > v.x || y > v.y || z > v.z); }
    //bool NearEqual(const uint3 &v, uint32 Epsilon) const { return (abs(x - v.x) <= Epsilon || abs(y - v.y) <= Epsilon || abs(z - v.z) <= Epsilon); }

    // modification functions
    Vector3u Min(const Vector3u &v) const { return Vector3u((x > v.x) ? v.x : x, (y > v.y) ? v.y : y, (z > v.z) ? v.z : z); }
    Vector3u Max(const Vector3u &v) const { return Vector3u((x < v.x) ? v.x : x, (y < v.y) ? v.y : y, (z < v.z) ? v.z : z); }
    Vector3u Clamp(const Vector3u &lBounds, const Vector3u &uBounds) const { return Min(uBounds).Max(lBounds); }
    Vector3u Saturate() const { return Clamp(Zero, One); }
    Vector3u Snap(const Vector3u &v) const { return Vector3u(Math::Snap(x, v.x), Math::Snap(y, v.y), Math::Snap(z, v.z)); }

    // swap
    void Swap(Vector3u &v)
    {
        uint32 tmp;
        tmp = v.x; v.x = x; x = tmp;
        tmp = v.y; v.y = y; y = tmp;
        tmp = v.z; v.z = z; z = tmp;
    }

    // maximum of all components
    uint32 MinOfComponents() const { return ::Min(x, ::Min(y, z)); }
    uint32 MaxOfComponents() const { return ::Max(x, ::Max(y, z)); }

    // shuffles
    template<int V0, int V1> Vector2u Shuffle2() const
    { 
        const uint32 *pv = (const uint32 *)&x;
        Vector2u result;
        result.x = pv[V0];
        result.y = pv[V1];
        return result;
    }

    template<int V0, int V1, int V2> Vector3u Shuffle3() const 
    { 
        const uint32 *pv = (const uint32 *)&x;
        Vector3u result;
        result.x = pv[V0];
        result.y = pv[V1];
        result.z = pv[V2];
        return result;
    }

    VECTOR3_SHUFFLE_FUNCTIONS(Vector2u, Vector3u);

    //----------------------------------------------------------------------
    union
    {
        struct { uint32 x, y, z; };
        struct { uint32 r, g, b; };
        uint32 ele[3];
    };

    //----------------------------------------------------------------------
    static const Vector3u &Zero, &One;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct Vector4u
{
    // constructors
    inline Vector4u() {}
    inline Vector4u(uint32 x_, uint32 y_, uint32 z_, uint32 w_) : x(x_), y(y_), z(z_), w(w_) {}
    inline Vector4u(uint32 *p) : x(p[0]), y(p[1]), z(p[2]), w(p[3]) {}
    inline Vector4u(const Vector2u &v, uint32 z_ = 0, uint32 w_ = 0) : x(v.x), y(v.y), z(z_), w(w_) {}
    inline Vector4u(const Vector3u &v, uint32 w_ = 0) : x(v.x), y(v.y), z(v.z), w(w_) {}
    inline Vector4u(const Vector4u &v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
    Vector4u(const Vector4ui &v);

    // setters
    void Set(uint32 x_, uint32 y_, uint32 z_, uint32 w_) { x = x_; y = y_; z = z_; w = w_; }
    void Set(const Vector2u &v, uint32 z_ = 0, uint32 w_ = 0) { x = v.x; y = v.y; z = z_; w = w_; }
    void Set(const Vector3u &v, uint32 w_ = 0) { x = v.x; y = v.y; z = v.z; w = w_; }
    void Set(const Vector4u &v) { x = v.x; y = v.y; z = v.z; w = v.w; }
    void Set(const Vector4ui &v);
    void SetZero() { x = y = z = w = 0; }

    // load/store
    void Load(const uint32 *v) { x = v[0]; y = v[1]; z = v[2]; w = v[3]; }
    void Store(uint32 *v) const { v[0] = x; v[1] = y; v[2] = z; v[3] = w; }

    // new vector
    Vector4u operator+(const Vector4u &v) const { return Vector4u(x + v.x, y + v.y, z + v.z, w + v.w); }
    Vector4u operator-(const Vector4u &v) const { return Vector4u(x - v.x, y - v.y, z - v.z, w - v.w); }
    Vector4u operator*(const Vector4u &v) const { return Vector4u(x * v.x, y * v.y, z * v.z, w * v.w); }
    Vector4u operator/(const Vector4u &v) const { return Vector4u(x / v.x, y / v.y, z / v.z, w / v.w); }
    Vector4u operator%(const Vector4u &v) const { return Vector4u(x % v.x, y % v.y, z % v.z, w % v.w); }
    Vector4u operator&(const Vector4u &v) const { return Vector4u(x & v.x, y & v.y, z & v.z, w & v.w); }
    Vector4u operator|(const Vector4u &v) const { return Vector4u(x | v.x, y | v.y, z | v.z, w | v.w); }
    Vector4u operator^(const Vector4u &v) const { return Vector4u(x ^ v.x, y ^ v.y, z ^ v.z, w ^ v.w); }

    // scalar operators
    Vector4u operator+(const uint32 v) const { return Vector4u(x + v, y + v, z + v, w + v); }
    Vector4u operator-(const uint32 v) const { return Vector4u(x - v, y - v, z - v, w - v); }
    Vector4u operator*(const uint32 v) const { return Vector4u(x * v, y * v, z * v, w * v); }
    Vector4u operator/(const uint32 v) const { return Vector4u(x / v, y / v, z / v, w / v); }
    Vector4u operator%(const uint32 v) const { return Vector4u(x % v, y % v, z % v, w % v); }
    Vector4u operator&(const uint32 v) const { return Vector4u(x & v, y & v, z & v, w & v); }
    Vector4u operator|(const uint32 v) const { return Vector4u(x | v, y | v, z | v, w | v); }
    Vector4u operator^(const uint32 v) const { return Vector4u(x ^ v, y ^ v, z ^ v, w ^ v); }

    // no params
    Vector4u operator~() const { return Vector4u(~x, ~y, ~z, ~w); }

    // comparison operators
    bool operator==(const Vector4u &v) const { return (x == v.x && y == v.y && z == v.z && w == v.w); }
    bool operator!=(const Vector4u &v) const { return (x != v.x || y != v.y || z != v.z || w != v.w); }
    bool operator<=(const Vector4u &v) const { return (x <= v.x && y <= v.y && z <= v.z && w <= v.w); }
    bool operator>=(const Vector4u &v) const { return (x >= v.x && y >= v.y && z >= v.z && w >= v.w); }
    bool operator<(const Vector4u &v) const { return (x < v.x && y < v.y && z < v.z && w < v.w); }
    bool operator>(const Vector4u &v) const { return (x > v.x && y > v.y && z > v.z && w > v.w); }

    // modifies this vector
    Vector4u &operator+=(const Vector4u &v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    Vector4u &operator-=(const Vector4u &v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    Vector4u &operator*=(const Vector4u &v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
    Vector4u &operator/=(const Vector4u &v) { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }
    Vector4u &operator%=(const Vector4u &v) { x %= v.x; y %= v.y; z %= v.z; w %= v.w; return *this; }
    Vector4u &operator&=(const Vector4u &v) { x &= v.x; y &= v.y; z &= v.z; w &= v.w; return *this; }
    Vector4u &operator|=(const Vector4u &v) { x |= v.x; y |= v.y; z |= v.z; w |= v.w; return *this; }
    Vector4u &operator^=(const Vector4u &v) { x ^= v.x; y ^= v.y; z ^= v.z; w ^= v.w; return *this; }
    Vector4u &operator=(const Vector4u &v) { x = v.x; y = v.y; z = v.z; w = v.w; return *this; }

    // scalar operators
    Vector4u &operator+=(const uint32 v) { x += v; y += v; z += v; w += v; return *this; }
    Vector4u &operator-=(const uint32 v) { x -= v; y -= v; z -= v; w -= v; return *this; }
    Vector4u &operator*=(const uint32 v) { x *= v; y *= v; z *= v; w *= v; return *this; }
    Vector4u &operator/=(const uint32 v) { x /= v; y /= v; z /= v; w /= v; return *this; }
    Vector4u &operator%=(const uint32 v) { x %= v; y %= v; z %= v; w %= v; return *this; }
    Vector4u &operator&=(const uint32 v) { x &= v; y &= v; z &= v; w &= v; return *this; }
    Vector4u &operator|=(const uint32 v) { x |= v; y |= v; z |= v; w |= v; return *this; }
    Vector4u &operator^=(const uint32 v) { x ^= v; y ^= v; z ^= v; w ^= v; return *this; }
    Vector4u &operator=(const uint32 v) { x = v; y = v; z = v; w = v; return *this; }

    // vector4i
    Vector4u &operator=(const Vector4i &v);

    // index accessors
    const uint32 &operator[](uint32 i) const { return (&x)[i]; }
    uint32 &operator[](uint32 i) { return (&x)[i]; }
    operator const uint32 *() const { return &x; }
    operator uint32 *() { return &x; }

    // to vectorx
    Vector4ui GetVector4ui() const;

    // partial comparisons
    bool AnyLess(const Vector4u &v) const { return (x < v.x || y < v.y || z < v.z || w < v.w); }
    bool AnyGreater(const Vector4u &v) const { return (x > v.x || y > v.y || z > v.z || w > v.w); }
    //bool NearEqual(const uint4 &v, uint32 Epsilon) const { return (abs(x - v.x) <= Epsilon || abs(y - v.y) <= Epsilon || abs(z - v.z) <= Epsilon || abs(w - v.w) < Epsilon); }

    // modification functions
    Vector4u Min(const Vector4u &v) const { return Vector4u((x > v.x) ? v.x : x, (y > v.y) ? v.y : y, (z > v.z) ? v.z : z, (w > v.w) ? v.w : w); }
    Vector4u Max(const Vector4u &v) const { return Vector4u((x < v.x) ? v.x : x, (y < v.y) ? v.y : y, (z < v.z) ? v.z : z, (w < v.w) ? v.w : w); }
    Vector4u Clamp(const Vector4u &lBounds, const Vector4u &uBounds) const { return Min(uBounds).Max(lBounds); }
    Vector4u Saturate() const { return Clamp(Zero, One); }
    Vector4u Snap(const Vector4u &v) const { return Vector4u(Math::Snap(x, v.x), Math::Snap(y, v.y), Math::Snap(z, v.z), Math::Snap(w, v.w)); }

    // swap
    void Swap(Vector4u &v)
    {
        uint32 tmp;
        tmp = v.x; v.x = x; x = tmp;
        tmp = v.y; v.y = y; y = tmp;
        tmp = v.z; v.z = z; z = tmp;
        tmp = v.w; v.w = w; w = tmp;
    }

    // maximum of all components
    uint32 MinOfComponents() const { return ::Min(x, ::Min(y, ::Min(z, w))); }
    uint32 MaxOfComponents() const { return ::Max(x, ::Max(y, ::Max(z, w))); }

    // shuffles
    template<int V0, int V1> Vector2u Shuffle2() const
    { 
        const uint32 *pv = (const uint32 *)&x;
        Vector2u result;
        result.x = pv[V0];
        result.y = pv[V1];
        return result;
    }

    template<int V0, int V1, int V2> Vector3u Shuffle3() const 
    { 
        const uint32 *pv = (const uint32 *)&x;
        Vector3u result;
        result.x = pv[V0];
        result.y = pv[V1];
        result.z = pv[V2];
        return result;
    }

    template<int V0, int V1, int V2, int V3> Vector4u Shuffle4() const 
    {
        const uint32 *pv = (const uint32 *)&x;
        Vector4u result;
        result.x = pv[V0];
        result.y = pv[V1];
        result.z = pv[V2];
        result.w = pv[V3];
        return result;
    }

    VECTOR4_SHUFFLE_FUNCTIONS(Vector2u, Vector3u, Vector4u);

    //----------------------------------------------------------------------
    union
    {
        struct { uint32 x, y, z, w; };
        struct { uint32 r, g, b, a; };
        uint32 ele[4];
    };

    //----------------------------------------------------------------------
    static const Vector4u &Zero, &One;
};
