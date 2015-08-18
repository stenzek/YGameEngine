#pragma once
#include "MathLib/Common.h"
#include "MathLib/VectorShuffles.h"

// interoperability between float/vector types
struct Vector2i;
struct Vector3i;
struct Vector4i;
struct Vector2u;
struct Vector3u;
struct Vector4u;
struct Vector2h;
struct Vector3h;
struct Vector4h;
struct Vector2f;
struct Vector3f;
struct Vector4f;
struct SIMDVector2f;
struct SIMDVector3f;
struct SIMDVector4f;

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct Vector2f
{
    // constructors
    inline Vector2f() {}
    inline Vector2f(const float val) : x(val), y(val) {}
    inline Vector2f(const float x_, const float y_) : x(x_), y(y_) {}
    inline Vector2f(const float *p) : x(p[0]), y(p[1]) {}
    inline Vector2f(const Vector2f &v) : x(v.x), y(v.y) {}
    Vector2f(const Vector2i &v);
    Vector2f(const SIMDVector2f &v);

    // setters
    void Set(const float x_, const float y_) { x = x_; y = y_; }
    void Set(const Vector2f &v) { x = v.x; y = v.y; }
    void Set(const SIMDVector2f &v);
    void SetZero() { x = y = 0.0f; }

    // load/store
    void Load(const float *v) { x = v[0]; y = v[1]; }
    void Store(float *v) const { v[0] = x; v[1] = y; }

    // new vector
    Vector2f operator+(const Vector2f &v) const { return Vector2f(x + v.x, y + v.y); }
    Vector2f operator-(const Vector2f &v) const { return Vector2f(x - v.x, y - v.y); }
    Vector2f operator*(const Vector2f &v) const { return Vector2f(x * v.x, y * v.y); }
    Vector2f operator/(const Vector2f &v) const { return Vector2f(x / v.x, y / v.y); }
	Vector2f operator%(const Vector2f &v) const { return Vector2f(Y_fmodf(x, v.x), Y_fmodf(y, v.y)); }
    Vector2f operator-() const { return Vector2f(-x, -y); }

    // scalar operators
    Vector2f operator+(const float v) const { return Vector2f(x + v, y + v); }
    Vector2f operator-(const float v) const { return Vector2f(x - v, y - v); }
    Vector2f operator*(const float v) const { return Vector2f(x * v, y * v); }
    Vector2f operator/(const float v) const { return Vector2f(x / v, y / v); }
	Vector2f operator%(const float v) const { return Vector2f(Y_fmodf(x, v), Y_fmodf(y, v)); }

    // comparison operators
    bool operator==(const Vector2f &v) const { return x == v.x && y == v.y; }
    bool operator!=(const Vector2f &v) const { return x != v.x || y != v.y; }
    bool operator<=(const Vector2f &v) const { return x <= v.x && y <= v.y; }
    bool operator>=(const Vector2f &v) const { return x >= v.x && y >= v.y; }
    bool operator<(const Vector2f &v) const { return x < v.x && y < v.y; }
    bool operator>(const Vector2f &v) const { return x > v.x && y > v.y; }

    // modifies this vector
    Vector2f &operator+=(const Vector2f &v) { x += v.x; y += v.y; return *this; }
    Vector2f &operator-=(const Vector2f &v) { x -= v.x; y -= v.y; return *this; }
    Vector2f &operator*=(const Vector2f &v) { x *= v.x; y *= v.y; return *this; }
    Vector2f &operator/=(const Vector2f &v) { x /= v.x; y /= v.y; return *this; }
	Vector2f &operator%=(const Vector2f &v) { x = Y_fmodf(x, v.x); y = Y_fmodf(y, v.y); return *this; }
    Vector2f &operator=(const Vector2f &v) { x = v.x; y = v.y; return *this; }
    Vector2f &operator=(const SIMDVector2f &v);

    // scalar operators
    Vector2f &operator+=(const float v) { x += v; y += v; return *this; }
    Vector2f &operator-=(const float v) { x -= v; y -= v; return *this; }
    Vector2f &operator*=(const float v) { x *= v; y *= v; return *this; }
    Vector2f &operator/=(const float v) { x /= v; y /= v; return *this; }
	Vector2f &operator%=(const float v) { x = Y_fmodf(x, v); y = Y_fmodf(y, v); return *this; }

    // index accessors
    //const float &operator[](uint32 i) const { return (&x)[i]; }
    //float &operator[](uint32 i) { return (&x)[i]; }
    operator const float *() const { return &x; }
    operator float *() { return &x; }

    // partial comparisons
    bool AnyLess(const Vector2f &v) const { return (x < v.x || y < v.y); }
    bool AnyGreater(const Vector2f &v) const { return (x > v.x || y > v.y); }
    bool NearEqual(const Vector2f &v, const float fEpsilon) const { return (Y_fabs(x - v.x) <= fEpsilon && Y_fabs(y - v.y) <= fEpsilon); }
    bool IsFinite() const { return (*this != Infinite); }

    // clamps
    Vector2f Min(const Vector2f &v) const { Vector2f result; result.x = (x > v.x) ? v.x : x; result.y = (y > v.y) ? v.y : y; return result; }
    Vector2f Max(const Vector2f &v) const { Vector2f result; result.x = (x < v.x) ? v.x : x; result.y = (y < v.y) ? v.y : y; return result; }
    Vector2f Clamp(const Vector2f &lBounds, const Vector2f &uBounds) const { return Min(uBounds).Max(lBounds); }
    Vector2f Abs() const { Vector2f result; result.x = Y_fabs(x); result.y = Y_fabs(y); return result; }
    Vector2f Saturate() const { return Clamp(Zero, One); }
    Vector2f Snap(const Vector2f &v) const { return Vector2f(Y_fsnap(x, v.x), Y_fsnap(y, v.y)); }
    Vector2f Floor() const { return Vector2f(Y_floorf(x), Y_floorf(y)); }
    Vector2f Ceil() const { return Vector2f(Y_ceilf(x), Y_ceilf(y)); }
    Vector2f Round() const { return Vector2f(Y_roundf(x), Y_roundf(y)); }

    // swap
    void Swap(Vector2f &v)
    {
        float tmp;
        tmp = x;    x = v.x;    v.x = tmp;
        tmp = y;    x = v.y;    v.y = tmp;
    }

    // dot product
    float Dot(const Vector2f &v) const
    {
        return x * v.x + y * v.y;
    }

    // lerp
    Vector2f Lerp(const Vector2f &v, const float f) const
    {
        Vector2f result;
        result.x = x + (v.x - x) * f;
        result.y = y + (v.y - y) * f;
        return result;
    }

    // length
    inline float SquaredLength() const { return Dot(*this); }
    inline float Length() const { return Y_sqrtf(Dot(*this)); }

    // distance
    float SquaredDistance(const Vector2f &to) const { return (to - *this).SquaredLength(); }
    float Distance(const Vector2f &to) const { float sql = (to - *this).SquaredLength(); return (sql != 0.0f) ? Y_sqrtf(sql) : 0.0f; }
    
    // normalize
    Vector2f Normalize() const { return *this / Length(); }
    Vector2f NormalizeEst() const { return *this * (Y_rsqrtf(SquaredLength())); }

    // normalize in place
    void NormalizeInPlace() { *this *= (1.0f / Length()); }
    void NormalizeEstInPlace() { *this *= Y_rsqrtf(SquaredLength()); }

    // safe normalize
    Vector2f SafeNormalize() const
    {
        float sqLength = SquaredLength();
        if (sqLength != 0.0f)
            return *this / Y_sqrtf(sqLength);
        else
            return Zero;
    }
    Vector2f SafeNormalizeEst() const
    {
        float sqLength = SquaredLength();
        if (sqLength != 0.0f)
            return *this * Y_rsqrtf(sqLength);
        else
            return Zero;
    }
    void SafeNormalizeInPlace()
    {
        float sqLength = SquaredLength();
        if (sqLength != 0.0f)
            *this /= Y_sqrtf(sqLength);
    }
    void SafeNormalizeEstInPlace()
    {
        float sqLength = SquaredLength();
        if (sqLength != 0.0f)
            *this *= Y_rsqrtf(SquaredLength());
    }

    // reciprocal
    Vector2f Reciprocal() const
    {
        Vector2f result;
        result.x = 1.0f / x;
        result.y = 1.0f / y;
        return result;
    }

    // cross product
    Vector2f Cross(const Vector2f &v) const
    {
        Vector2f result;
        result.x = (x * v.y) - (y * v.x);
        result.y = (x * v.y) - (y * v.x);
        return result;
    }

    // maximum of all components
    float MinOfComponents() const { return ::Min(x, y); }
    float MaxOfComponents() const { return ::Max(x, y); }

    // shuffles
    template<int V0, int V1> Vector2f Shuffle2() const
    { 
        const float *pv = (const float *)&x;
        Vector2f result;
        result.x = pv[V0];
        result.y = pv[V1];
        return result;
    }
    
    VECTOR2_SHUFFLE_FUNCTIONS(Vector2f);

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    union
    {
        struct { float x; float y; };
        struct { float r; float g; };
        float ele[2];
    };

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    static const Vector2f &Zero, &One, &NegativeOne;
    static const Vector2f &Infinite, &NegativeInfinite;
    static const Vector2f &UnitX, &UnitY, &NegativeUnitX, &NegativeUnitY;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct Vector3f
{
    // constructors
    inline Vector3f() {}
    inline Vector3f(const float val) : x(val), y(val), z(val) {}
    inline Vector3f(const float x_, const float y_, const float z_) : x(x_), y(y_), z(z_) {}
    inline Vector3f(const float *p) : x(p[0]), y(p[1]), z(p[2]) {}
    inline Vector3f(const Vector2f &v, const float z_ = 0.0f) : x(v.x), y(v.y), z(z_) {}
    inline Vector3f(const Vector3f &v) : x(v.x), y(v.y), z(v.z) {}
    Vector3f(const Vector3i &v);
    Vector3f(const SIMDVector3f &v);

    // setters
    void Set(const float x_, const float y_, const float z_) { x = x_; y = y_; z = z_; }
    void Set(const Vector2f &v, const float z_ = 0.0f) { x = v.x; y = v.y; z = z_; }
    void Set(const Vector3f &v) { x = v.x; y = v.y; z = v.z; }
    void Set(const SIMDVector3f &v);
    void SetZero() { x = y = z = 0.0f; }

    // load/store
    void Load(const float *v) { x = v[0]; y = v[1]; z = v[2]; }
    void Store(float *v) const { v[0] = x; v[1] = y; v[2] = z;}

    // new vector
    Vector3f operator+(const Vector3f &v) const { return Vector3f(x + v.x, y + v.y, z + v.z); }
    Vector3f operator-(const Vector3f &v) const { return Vector3f(x - v.x, y - v.y, z - v.z); }
    Vector3f operator*(const Vector3f &v) const { return Vector3f(x * v.x, y * v.y, z * v.z); }
    Vector3f operator/(const Vector3f &v) const { return Vector3f(x / v.x, y / v.y, z / v.z); }
	Vector3f operator%(const Vector3f &v) const { return Vector3f(Y_fmodf(x, v.x), Y_fmodf(y, v.y), Y_fmodf(z, v.z)); }
    Vector3f operator-() const { return Vector3f(-x, -y, -z); }

    // scalar operators
    Vector3f operator+(const float v) const { return Vector3f(x + v, y + v, z + v); }
    Vector3f operator-(const float v) const { return Vector3f(x - v, y - v, z - v); }
    Vector3f operator*(const float v) const { return Vector3f(x * v, y * v, z * v); }
    Vector3f operator/(const float v) const { return Vector3f(x / v, y / v, z / v); }
	Vector3f operator%(const float v) const { return Vector3f(Y_fmodf(x, v), Y_fmodf(y, v), Y_fmodf(z, v)); }

    // comparison operators
    bool operator==(const Vector3f &v) const { return x == v.x && y == v.y && z == v.z; }
    bool operator!=(const Vector3f &v) const { return x != v.x || y != v.y || z != v.z; }
    bool operator<=(const Vector3f &v) const { return x <= v.x && y <= v.y && z <= v.z; }
    bool operator>=(const Vector3f &v) const { return x >= v.x && y >= v.y && z >= v.z; }
    bool operator<(const Vector3f &v) const { return x < v.x && y < v.y && z < v.z; }
    bool operator>(const Vector3f &v) const { return x > v.x && y > v.y && z > v.z; }

    // modifies this vector
    Vector3f &operator+=(const Vector3f &v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vector3f &operator-=(const Vector3f &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    Vector3f &operator*=(const Vector3f &v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
    Vector3f &operator/=(const Vector3f &v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
	Vector3f &operator%=(const Vector3f &v) { x = Y_fmodf(x, v.x); y = Y_fmodf(y, v.y); z = Y_fmodf(z, v.z); return *this; }
    Vector3f &operator=(const Vector3f &v) { x = v.x; y = v.y; z = v.z; return *this; }
    Vector3f &operator=(const SIMDVector3f &v);

    // scalar operators
    Vector3f &operator+=(const float v) { x += v; y += v; z += v; return *this; }
    Vector3f &operator-=(const float v) { x -= v; y -= v; z -= v; return *this; }
    Vector3f &operator*=(const float v) { x *= v; y *= v; z *= v; return *this; }
    Vector3f &operator/=(const float v) { x /= v; y /= v; z /= v; return *this; }
	Vector3f &operator%=(const float v) { x = Y_fmodf(x, v); y = Y_fmodf(y, v); z = Y_fmodf(z, v); return *this; }

    // index accessors
    //const float &operator[](uint32 i) const { return (&x)[i]; }
    //float &operator[](uint32 i) { return (&x)[i]; }
    operator const float *() const { return &x; }
    operator float *() { return &x; }

    // partial comparisons
    bool AnyLess(const Vector3f &v) const { return (x < v.x || y < v.y || z < v.z); }
    bool AnyGreater(const Vector3f &v) const { return (x > v.x || y > v.y || z > v.z); }
    bool NearEqual(const Vector3f &v, const float fEpsilon) const { return (Y_fabs(x - v.x) <= fEpsilon && Y_fabs(y - v.y) <= fEpsilon && Y_fabs(z - v.z) <= fEpsilon); }
    bool IsFinite() const { return (*this != Infinite); }

    // clamps
    Vector3f Min(const Vector3f &v) const { Vector3f result; result.x = (x > v.x) ? v.x : x; result.y = (y > v.y) ? v.y : y; result.z = (z > v.z) ? v.z : z; return result; }
    Vector3f Max(const Vector3f &v) const { Vector3f result; result.x = (x < v.x) ? v.x : x; result.y = (y < v.y) ? v.y : y; result.z = (z < v.z) ? v.z : z; return result; }
    Vector3f Clamp(const Vector3f &lBounds, const Vector3f &uBounds) const { return Min(uBounds).Max(lBounds); }
    Vector3f Abs() const { Vector3f result; result.x = Y_fabs(x); result.y = Y_fabs(y); result.z = Y_fabs(z); return result; }
    Vector3f Saturate() const { return Clamp(Zero, One); }
    Vector3f Snap(const Vector3f &v) const { return Vector3f(Y_fsnap(x, v.x), Y_fsnap(y, v.y), Y_fsnap(z, v.z)); }
    Vector3f Floor() const { return Vector3f(Y_floorf(x), Y_floorf(y), Y_floorf(z)); }
    Vector3f Ceil() const { return Vector3f(Y_ceilf(x), Y_ceilf(y), Y_ceilf(z)); }
    Vector3f Round() const { return Vector3f(Y_roundf(x), Y_roundf(y), Y_roundf(z)); }

    // swap
    void Swap(Vector3f &v)
    {
        float tmp;
        tmp = x;    x = v.x;    v.x = tmp;
        tmp = y;    x = v.y;    v.y = tmp;
        tmp = z;    x = v.z;    v.z = tmp;
    }

    // dot product
    float Dot(const Vector3f &v) const
    {
        return x * v.x + y * v.y + z * v.z;
    }

    // lerp
    Vector3f Lerp(const Vector3f &v, const float f) const
    {
        Vector3f result;
        result.x = x + (v.x - x) * f;
        result.y = y + (v.y - y) * f;
        result.z = z + (v.z - z) * f;
        return result;
    }

    // length
    inline float SquaredLength() const { return Dot(*this); }
    inline float Length() const { return Y_sqrtf(Dot(*this)); }

    // distance
    float SquaredDistance(const Vector3f &to) const { return (to - *this).SquaredLength(); }
    float Distance(const Vector3f &to) const { float sql = (to - *this).SquaredLength(); return (sql != 0.0f) ? Y_sqrtf(sql) : 0.0f; }
    
    // normalize
    Vector3f Normalize() const { return *this / Length(); }
    Vector3f NormalizeEst() const { return *this * (Y_rsqrtf(SquaredLength())); }

    // normalize in place
    void NormalizeInPlace() { *this *= (1.0f / Length()); }
    void NormalizeEstInPlace() { *this *= Y_rsqrtf(SquaredLength()); }

    // safe normalize
    Vector3f SafeNormalize() const
    {
        float sqLength = SquaredLength();
        if (sqLength != 0.0f)
            return *this / Y_sqrtf(sqLength);
        else
            return Zero;
    }
    Vector3f SafeNormalizeEst() const
    {
        float sqLength = SquaredLength();
        if (sqLength != 0.0f)
            return *this * Y_rsqrtf(sqLength);
        else
            return Zero;
    }
    void SafeNormalizeInPlace()
    {
        float sqLength = SquaredLength();
        if (sqLength != 0.0f)
            *this /= Y_sqrtf(sqLength);
    }
    void SafeNormalizeEstInPlace()
    {
        float sqLength = SquaredLength();
        if (sqLength != 0.0f)
            *this *= Y_rsqrtf(SquaredLength());
    }

    // reciprocal
    Vector3f Reciprocal() const
    {
        Vector3f result;
        result.x = 1.0f / x;
        result.y = 1.0f / y;
        result.z = 1.0f / z;
        return result;
    }

    // cross product
    Vector3f Cross(const Vector3f &v) const
    {
        Vector3f result;
        result.x = (y * v.z) - (z * v.y);
        result.y = (z * v.x) - (x * v.z);
        result.z = (x * v.y) - (y * v.x);
        return result;
    }

    // maximum of all components
    float MinOfComponents() const { return ::Min(x, ::Min(y, z)); }
    float MaxOfComponents() const { return ::Max(x, ::Max(y, z)); }

    // shuffles
    template<int V0, int V1> Vector2f Shuffle2() const
    { 
        const float *pv = (const float *)&x;
        Vector2f result;
        result.x = pv[V0];
        result.y = pv[V1];
        return result;
    }

    template<int V0, int V1, int V2> Vector3f Shuffle3() const 
    { 
        const float *pv = (const float *)&x;
        Vector3f result;
        result.x = pv[V0];
        result.y = pv[V1];
        result.z = pv[V2];
        return result;
    }

    VECTOR3_SHUFFLE_FUNCTIONS(Vector2f, Vector3f);

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    union
    {
        struct { float x; float y; float z; };
        struct { float r; float g; float b; };
        float ele[3];
    };

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    static const Vector3f &Zero, &One, &NegativeOne;
    static const Vector3f &Infinite, &NegativeInfinite;
    static const Vector3f &UnitX, &UnitY, &UnitZ, &NegativeUnitX, &NegativeUnitY, &NegativeUnitZ;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

struct Vector4f
{
    // constructors
    inline Vector4f() {}
    inline Vector4f(const float val) : x(val), y(val), z(val), w(val) {}
    inline Vector4f(const float x_, const float y_, const float z_, const float w_) : x(x_), y(y_), z(z_), w(w_) {}
    inline Vector4f(const float *p) : x(p[0]), y(p[1]), z(p[2]), w(p[3]) {}
    inline Vector4f(const Vector2f &v, const float z_ = 0.0f, const float w_ = 0.0f) : x(v.x), y(v.y), z(z_), w(w_) {}
    inline Vector4f(const Vector3f &v, const float w_ = 0.0f) : x(v.x), y(v.y), z(v.z), w(w_) {}
    inline Vector4f(const Vector4f &v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
    Vector4f(const Vector4i &v);
    Vector4f(const SIMDVector4f &v);

    // setters
    void Set(const float x_, const float y_, const float z_, const float w_) { x = x_; y = y_; z = z_; w = w_; }
    void Set(const Vector2f &v, const float z_ = 0.0f, const float w_ = 0.0f) { x = v.x; y = v.y; z = z_; w = w_; }
    void Set(const Vector3f &v, const float w_ = 0.0f) { x = v.x; y = v.y; z = v.z; w = w_; }
    void Set(const Vector4f &v) { x = v.x; y = v.y; z = v.z; w = v.w; }
    void Set(const SIMDVector4f &v);
    void SetZero() { x = y = z = w = 0.0f; }

    // load/store
    void Load(const float *v) { x = v[0]; y = v[1]; z = v[2]; w = v[3]; }
    void Store(float *v) const { v[0] = x; v[1] = y; v[2] = z; v[3] = w; }

    // new vector
    Vector4f operator+(const Vector4f &v) const { return Vector4f(x + v.x, y + v.y, z + v.z, w + v.w); }
    Vector4f operator-(const Vector4f &v) const { return Vector4f(x - v.x, y - v.y, z - v.z, w - v.w); }
    Vector4f operator*(const Vector4f &v) const { return Vector4f(x * v.x, y * v.y, z * v.z, w * v.w); }
    Vector4f operator/(const Vector4f &v) const { return Vector4f(x / v.x, y / v.y, z / v.z, w / v.w); }
	Vector4f operator%(const Vector4f &v) const { return Vector4f(Y_fmodf(x, v.x), Y_fmodf(y, v.y), Y_fmodf(z, v.z), Y_fmodf(w, v.w)); }
    Vector4f operator-() const { return Vector4f(-x, -y, -z, -w); }

    // scalar operators
    Vector4f operator+(const float v) const { return Vector4f(x + v, y + v, z + v, w + v); }
    Vector4f operator-(const float v) const { return Vector4f(x - v, y - v, z - v, w - v); }
    Vector4f operator*(const float v) const { return Vector4f(x * v, y * v, z * v, w * v); }
    Vector4f operator/(const float v) const { return Vector4f(x / v, y / v, z / v, w / v); }
	Vector4f operator%(const float v) const { return Vector4f(Y_fmodf(x, v), Y_fmodf(y, v), Y_fmodf(z, v), Y_fmodf(w, v)); }

    // comparison operators
    bool operator==(const Vector4f &v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    bool operator!=(const Vector4f &v) const { return x != v.x || y != v.y || z != v.z || w != v.w; }
    bool operator<=(const Vector4f &v) const { return x <= v.x && y <= v.y && z <= v.z && w <= v.w; }
    bool operator>=(const Vector4f &v) const { return x >= v.x && y >= v.y && z >= v.z && w >= v.w; }
    bool operator<(const Vector4f &v) const { return x < v.x && y < v.y && z < v.z && w < v.w; }
    bool operator>(const Vector4f &v) const { return x > v.x && y > v.y && z > v.z && w > v.w; }

    // modifies this vector
    Vector4f &operator+=(const Vector4f &v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
    Vector4f &operator-=(const Vector4f &v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
    Vector4f &operator*=(const Vector4f &v) { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
    Vector4f &operator/=(const Vector4f &v) { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }
	Vector4f &operator%=(const Vector4f &v) { x = Y_fmodf(x, v.x); y = Y_fmodf(y, v.y); z = Y_fmodf(z, v.z); w = Y_fmodf(w, v.w); return *this; }
    Vector4f &operator=(const Vector4f &v) { x = v.x; y = v.y; z = v.z; w = v.w; return *this; }
    Vector4f &operator=(const SIMDVector4f &v);

    // scalar operators
    Vector4f &operator+=(const float v) { x += v; y += v; z += v; w += v; return *this; }
    Vector4f &operator-=(const float v) { x -= v; y -= v; z -= v; w -= v; return *this; }
    Vector4f &operator*=(const float v) { x *= v; y *= v; z *= v; w *= v; return *this; }
    Vector4f &operator/=(const float v) { x /= v; y /= v; z /= v; w /= v; return *this; }
	Vector4f &operator%=(const float v) { x = Y_fmodf(x, v); y = Y_fmodf(y, v); z = Y_fmodf(z, v); w = Y_fmodf(w, v); return *this; }

    // index accessors
    //const float &operator[](uint32 i) const { return (&x)[i]; }
    //float &operator[](uint32 i) { return (&x)[i]; }
    operator const float *() const { return &x; }
    operator float *() { return &x; }

    // partial comparisons
    bool AnyLess(const Vector4f &v) const { return (x < v.x || y < v.y || z < v.z || w < v.w); }
    bool AnyGreater(const Vector4f &v) const { return (x > v.x || y > v.y || z > v.z || w > v.w); }
    bool NearEqual(const Vector4f &v, const float fEpsilon) const { return (Y_fabs(x - v.x) <= fEpsilon && Y_fabs(y - v.y) <= fEpsilon && Y_fabs(z - v.z) <= fEpsilon && Y_fabs(w - v.w) < fEpsilon); }
    bool IsFinite() const { return (*this != Infinite); }

    // clamps
    Vector4f Min(const Vector4f &v) const { Vector4f result; result.x = (x > v.x) ? v.x : x; result.y = (y > v.y) ? v.y : y; result.z = (z > v.z) ? v.z : z; result.w = (w > v.w) ? v.w : w; return result; }
    Vector4f Max(const Vector4f &v) const { Vector4f result; result.x = (x < v.x) ? v.x : x; result.y = (y < v.y) ? v.y : y; result.z = (z < v.z) ? v.z : z; result.w = (w < v.w) ? v.w : w; return result; }
    Vector4f Clamp(const Vector4f &lBounds, const Vector4f &uBounds) const { return Min(uBounds).Max(lBounds); }
    Vector4f Abs() const { Vector4f result; result.x = Y_fabs(x); result.y = Y_fabs(y); result.z = Y_fabs(z); result.w = Y_fabs(w); return result; }
    Vector4f Saturate() const { return Clamp(Zero, One); }
    Vector4f Snap(const Vector4f &v) const { return Vector4f(Y_fsnap(x, v.x), Y_fsnap(y, v.y), Y_fsnap(z, v.z), Y_fsnap(z, v.z)); }
    Vector4f Floor() const { return Vector4f(Y_floorf(x), Y_floorf(y), Y_floorf(z), Y_floorf(w)); }
    Vector4f Ceil() const { return Vector4f(Y_ceilf(x), Y_ceilf(y), Y_ceilf(z), Y_roundf(w)); }
    Vector4f Round() const { return Vector4f(Y_roundf(x), Y_roundf(y), Y_roundf(z), Y_roundf(w)); }

    // swap
    void Swap(Vector4f &v)
    {
        float tmp;
        tmp = x;    x = v.x;    v.x = tmp;
        tmp = y;    x = v.y;    v.y = tmp;
        tmp = z;    x = v.z;    v.z = tmp;
        tmp = w;    x = v.w;    v.w = tmp;
    }

    // dot product
    float Dot(const Vector4f &v) const
    {
        return x * v.x + y * v.y + z * v.z + w * v.w;
    }

    // lerp
    Vector4f Lerp(const Vector4f &v, const float f) const
    {
        Vector4f result;
        result.x = x + (v.x - x) * f;
        result.y = y + (v.y - y) * f;
        result.z = z + (v.z - z) * f;
        result.w = w + (v.w - w) * f;
        return result;
    }

    // length
    inline float SquaredLength() const { return Dot(*this); }
    inline float Length() const { return Y_sqrtf(Dot(*this)); }

    // distance
    float SquaredDistance(const Vector4f &to) const { return (to - *this).SquaredLength(); }
    float Distance(const Vector4f &to) const { float sql = (to - *this).SquaredLength(); return (sql != 0.0f) ? Y_sqrtf(sql) : 0.0f; }
    
    // normalize
    Vector4f Normalize() const { return *this / Length(); }
    Vector4f NormalizeEst() const { return *this * (Y_rsqrtf(SquaredLength())); }

    // normalize in place
    void NormalizeInPlace() { *this *= (1.0f / Length()); }
    void NormalizeEstInPlace() { *this *= Y_rsqrtf(SquaredLength()); }

    // safe normalize
    Vector4f SafeNormalize() const
    {
        float sqLength = SquaredLength();
        if (sqLength != 0.0f)
            return *this / Y_sqrtf(sqLength);
        else
            return Zero;
    }
    Vector4f SafeNormalizeEst() const
    {
        float sqLength = SquaredLength();
        if (sqLength != 0.0f)
            return *this * Y_rsqrtf(sqLength);
        else
            return Zero;
    }
    void SafeNormalizeInPlace()
    {
        float sqLength = SquaredLength();
        if (sqLength != 0.0f)
            *this /= Y_sqrtf(sqLength);
    }
    void SafeNormalizeEstInPlace()
    {
        float sqLength = SquaredLength();
        if (sqLength != 0.0f)
            *this *= Y_rsqrtf(SquaredLength());
    }

    // reciprocal
    Vector4f Reciprocal() const
    {
        Vector4f result;
        result.x = 1.0f / x;
        result.y = 1.0f / y;
        result.z = 1.0f / z;
        result.w = 1.0f / w;
        return result;
    }

    // cross product
    Vector4f Cross(const Vector4f &v1, const Vector4f &v2) const
    {
        Vector4f result;
        result.x = (((v1.z * v2.w) - (v1.w * v2.z)) * y) - (((v1.y * v2.w) - (v1.w * v2.y)) * z) + (((v1.y * v2.z) - (v1.z * v2.y)) * w);
        result.y = (((v1.w * v2.z) - (v1.z * v2.w)) * x) - (((v1.w * v2.x) - (v1.x * v2.w)) * z) + (((v1.z * v2.x) - (v1.x * v2.z)) * w);
        result.z = (((v1.y * v2.w) - (v1.w * v2.y)) * x) - (((v1.x * v2.w) - (v1.w * v2.x)) * y) + (((v1.x * v2.y) - (v1.y * v2.x)) * w);
        result.w = (((v1.z * v2.y) - (v1.y * v2.z)) * x) - (((v1.z * v2.x) - (v1.x * v2.z)) * y) + (((v1.y * v2.x) - (v1.x * v2.y)) * z);
        return result;
    }

    // maximum of all components
    float MinOfComponents() const { return ::Min(x, ::Min(y, ::Min(z, w))); }
    float MaxOfComponents() const { return ::Max(x, ::Max(y, ::Max(z, w))); }

    // shuffles
    template<int V0, int V1> Vector2f Shuffle2() const
    { 
        const float *pv = (const float *)&x;
        Vector2f result;
        result.x = pv[V0];
        result.y = pv[V1];
        return result;
    }

    template<int V0, int V1, int V2> Vector3f Shuffle3() const 
    { 
        const float *pv = (const float *)&x;
        Vector3f result;
        result.x = pv[V0];
        result.y = pv[V1];
        result.z = pv[V2];
        return result;
    }

    template<int V0, int V1, int V2, int V3> Vector4f Shuffle4() const 
    { 
        const float *pv = (const float *)&x;
        Vector4f result;
        result.x = pv[V0];
        result.y = pv[V1];
        result.z = pv[V2];
        result.w = pv[V3];
        return result;
    }

    VECTOR4_SHUFFLE_FUNCTIONS(Vector2f, Vector3f, Vector4f);

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    union
    {
        struct { float x; float y; float z; float w; };
        struct { float r; float g; float b; float a; };
        float ele[4];
    };

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    static const Vector4f &Zero, &One, &NegativeOne;
    static const Vector4f &Infinite, &NegativeInfinite;
    static const Vector4f &UnitX, &UnitY, &UnitZ, &UnitW, &NegativeUnitX, &NegativeUnitY, &NegativeUnitZ, &NegativeUnitW;
};

