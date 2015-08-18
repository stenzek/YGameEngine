#pragma once
#include "Core/Math.h"
#include "Core/Memory.h"
#include <intrin.h>

#if Y_CPU_SSE_LEVEL < 2
    #error SSE2 must be enabled.
#endif

// interoperability between int/vector types
struct Vector2i;
struct Vector3i;
struct Vector4i;

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

ALIGN_DECL(Y_SSE_ALIGNMENT) struct SIMDVector2i
{
    // constructors
    inline SIMDVector2i() {}
    inline SIMDVector2i(int32 x_, int32 y_) { m128i = _mm_set_epi32(0, 0, y_, x_); }
    inline SIMDVector2i(const int32 *p) { m128i = _mm_set_epi32(0, 0, p[1], p[0]); }
    inline SIMDVector2i(const SIMDVector2i &v) : m128i(v.m128i) {}
    SIMDVector2i(const Vector2i &v);

    // new/delete, we overload these so that vectors allocated on the heap are guaranteed to be aligned correctly
    void *operator new[](size_t c) { return Y_aligned_malloc(c, Y_SSE_ALIGNMENT); }
    void *operator new(size_t c) { return Y_aligned_malloc(c, Y_SSE_ALIGNMENT); }
    void operator delete[](void *pMemory) { return Y_aligned_free(pMemory); }
    void operator delete(void *pMemory) { return Y_aligned_free(pMemory); }

    // setters
    void Set(int32 x_, int32 y_) { m128i = _mm_set_epi32(0, 0, y_, x_); }
    void Set(const SIMDVector2i &v) { m128i = v.m128i; }
    void Set(const Vector2i &v);
    void SetZero() { m128i = _mm_setzero_si128(); }

    // new vector
    inline SIMDVector2i operator+(const SIMDVector2i &v) const { SIMDVector2i r; r.m128i = _mm_add_epi32(m128i, v.m128i); return r; }
    inline SIMDVector2i operator-(const SIMDVector2i &v) const { SIMDVector2i r; r.m128i = _mm_sub_epi32(m128i, v.m128i); return r; }
    inline SIMDVector2i operator*(const SIMDVector2i &v) const { SIMDVector2i r; r.m128i = _mm_mul_epi32(m128i, v.m128i); return r; }
    inline SIMDVector2i operator/(const SIMDVector2i &v) const { SIMDVector2i r; r.m128i = _mm_set_epi32(0, 0, y / v.y, x / v.x); return r; }
    inline SIMDVector2i operator%(const SIMDVector2i &v) const { SIMDVector2i r; r.m128i = _mm_set_epi32(0, 0, y % v.y, x % v.x); return r; }
    inline SIMDVector2i operator&(const SIMDVector2i &v) const { SIMDVector2i r; r.m128i = _mm_and_si128(m128i, v.m128i); return r; }
    inline SIMDVector2i operator|(const SIMDVector2i &v) const { SIMDVector2i r; r.m128i = _mm_or_si128(m128i, v.m128i); return r; }
    inline SIMDVector2i operator^(const SIMDVector2i &v) const { SIMDVector2i r; r.m128i = _mm_xor_si128(m128i, v.m128i); return r; }

    // scalar operators
    inline SIMDVector2i operator+(const int32 v) const { SIMDVector2i r; r.m128i = _mm_add_epi32(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector2i operator-(const int32 v) const { SIMDVector2i r; r.m128i = _mm_sub_epi32(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector2i operator*(const int32 v) const { SIMDVector2i r; r.m128i = _mm_mul_epi32(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector2i operator/(const int32 v) const { SIMDVector2i r; r.m128i = _mm_set_epi32(0, 0, y / v, x / v); return r; }
    inline SIMDVector2i operator%(const int32 v) const { SIMDVector2i r; r.m128i = _mm_set_epi32(0, 0, y % v, x % v); return r; }
    inline SIMDVector2i operator&(const int32 v) const { SIMDVector2i r; r.m128i = _mm_and_si128(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector2i operator|(const int32 v) const { SIMDVector2i r; r.m128i = _mm_or_si128(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector2i operator^(const int32 v) const { SIMDVector2i r; r.m128i = _mm_xor_si128(m128i, _mm_set1_epi32(v)); return r; }

    // no params
    inline SIMDVector2i operator~() const { SIMDVector2i r; r.m128i = _mm_andnot_si128(m128i, m128i); return r; }
    inline SIMDVector2i operator-() const { SIMDVector2i r; r.m128i = _mm_xor_si128(m128i, _mm_set1_epi32(0x7FFFFFFF)); return r; }

    // comparison operators
    inline bool operator==(const SIMDVector2i &v) const { return (_mm_movemask_epi8(_mm_cmpeq_epi32(m128i, v.m128i)) & 0xFF) == 0xFF; }
    inline bool operator!=(const SIMDVector2i &v) const { return (_mm_movemask_epi8(_mm_cmpeq_epi32(m128i, v.m128i)) & 0xFF) != 0xFF; }
    inline bool operator<=(const SIMDVector2i &v) const { return (_mm_movemask_epi8(_mm_xor_si128(_mm_cmplt_epi32(m128i, v.m128i), _mm_cmpeq_epi32(m128i, v.m128i))) & 0x33) == 0x33; }
    inline bool operator>=(const SIMDVector2i &v) const { return (_mm_movemask_epi8(_mm_xor_si128(_mm_cmpgt_epi32(m128i, v.m128i), _mm_cmpeq_epi32(m128i, v.m128i))) & 0x33) == 0x33; }
    inline bool operator<(const SIMDVector2i &v) const { return (_mm_movemask_epi8(_mm_cmplt_epi32(m128i, v.m128i)) & 0x33) == 0x33; }
    inline bool operator>(const SIMDVector2i &v) const { return (_mm_movemask_epi8(_mm_cmpgt_epi32(m128i, v.m128i)) & 0x33) == 0x33; }

    // modifies this vector
    inline SIMDVector2i &operator+=(const SIMDVector2i &v) { m128i = _mm_add_epi32(m128i, v.m128i); return *this; }
    inline SIMDVector2i &operator-=(const SIMDVector2i &v) { m128i = _mm_sub_epi32(m128i, v.m128i); return *this; }
    inline SIMDVector2i &operator*=(const SIMDVector2i &v) { m128i = _mm_mul_epi32(m128i, v.m128i); return *this; }
    inline SIMDVector2i &operator/=(const SIMDVector2i &v) { x /= v.x; y /= v.y; return *this; }
    inline SIMDVector2i &operator%=(const SIMDVector2i &v) { x %= v.x; y %= v.y; return *this; }
    inline SIMDVector2i &operator&=(const SIMDVector2i &v) { m128i = _mm_and_si128(m128i, v.m128i); return *this; }
    inline SIMDVector2i &operator|=(const SIMDVector2i &v) { m128i = _mm_or_si128(m128i, v.m128i); return *this; }
    inline SIMDVector2i &operator^=(const SIMDVector2i &v) { m128i = _mm_xor_si128(m128i, v.m128i); return *this; }

    // scalar operators
    inline SIMDVector2i &operator+=(const int32 v) { m128i = _mm_add_epi32(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector2i &operator-=(const int32 v) { m128i = _mm_sub_epi32(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector2i &operator*=(const int32 v) { m128i = _mm_mul_epi32(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector2i &operator/=(const int32 v) { x /= v; y /= v; return *this; }
    inline SIMDVector2i &operator%=(const int32 v) { x %= v; y %= v; return *this; }
    inline SIMDVector2i &operator&=(const int32 v) { m128i = _mm_and_si128(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector2i &operator|=(const int32 v) { m128i = _mm_or_si128(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector2i &operator^=(const int32 v) { m128i = _mm_xor_si128(m128i, _mm_set1_epi32(v)); return *this; }

    inline SIMDVector2i &operator=(const SIMDVector2i &v) { m128i = v.m128i; return *this; }
    SIMDVector2i &operator=(const Vector2i &v);

    // index accessors
    //const int32 &operator[](uint32 i) const { return (&x)[i]; }
    //int32 &operator[](uint32 i) { return (&x)[i]; }
    operator const int32 *() const { return &x; }
    operator int32 *() { return &x; }

    // to intx
    const Vector2i &GetInt2() const { return reinterpret_cast<const Vector2i &>(*this); }
    Vector2i &GetInt2() { return reinterpret_cast<Vector2i &>(*this); }
    operator const Vector2i &() const { return reinterpret_cast<const Vector2i &>(*this); }
    operator Vector2i &() { return reinterpret_cast<Vector2i &>(*this); }

    // partial comparisons
    bool AnyLess(const SIMDVector2i &v) const { return (_mm_movemask_epi8(_mm_cmplt_epi32(m128i, v.m128i)) & 0x3) != 0x0; }
    bool AnyLessEqual(const SIMDVector2i &v) const { return (x <= v.x || y <= v.y); }
    bool AnyGreater(const SIMDVector2i &v) const { return (_mm_movemask_epi8(_mm_cmpgt_epi32(m128i, v.m128i)) & 0x3) != 0x0; }
    bool AnyGreaterEqual(const SIMDVector2i &v) const { return (x >= v.x || y >= v.y); }

    // modification functions
    SIMDVector2i Min(const SIMDVector2i &v) const { SIMDVector2i r; r.m128i = _mm_min_epi32(m128i, v.m128i); return r; }
    SIMDVector2i Max(const SIMDVector2i &v) const { SIMDVector2i r; r.m128i = _mm_max_epi32(m128i, v.m128i); return r; }
    SIMDVector2i Clamp(const SIMDVector2i &lBounds, const SIMDVector2i &uBounds) const { SIMDVector2i r; r.m128i = _mm_min_epi32(uBounds.m128i, _mm_max_epi32(lBounds.m128i, m128i)); return r; }
    SIMDVector2i Abs() const { SIMDVector2i r; r.m128i = _mm_abs_epi32(m128i); return r; }
    SIMDVector2i Saturate() const { SIMDVector2i r; r.m128i = _mm_min_epi32(_mm_set1_epi32(1), _mm_max_epi32(_mm_setzero_si128(), m128i)); return r; }
    SIMDVector2i Snap(const SIMDVector2i &v) const { return SIMDVector2i(Math::Snap(x, v.x), Math::Snap(y, v.y)); }


    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    union
    {
        __m128i m128i;
        struct { int32 x, y; };
        struct { int32 r, g; };
    };

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    static const SIMDVector2i &Zero, &One, &NegativeOne;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

ALIGN_DECL(Y_SSE_ALIGNMENT) struct SIMDVector3i
{
    // constructors
    inline SIMDVector3i() {}
    inline SIMDVector3i(int32 x_, int32 y_, int32 z_) { m128i = _mm_set_epi32(0, z_, y_, x_); }
    inline SIMDVector3i(const int32 *p) { m128i = _mm_set_epi32(0, p[2], p[1], p[0]); }
    inline SIMDVector3i(const SIMDVector3i &v) : m128i(v.m128i) {}
    SIMDVector3i(const Vector3i &v);

    // new/delete, we overload these so that vectors allocated on the heap are guaranteed to be aligned correctly
    void *operator new[](size_t c) { return Y_aligned_malloc(c, Y_SSE_ALIGNMENT); }
    void *operator new(size_t c) { return Y_aligned_malloc(c, Y_SSE_ALIGNMENT); }
    void operator delete[](void *pMemory) { return Y_aligned_free(pMemory); }
    void operator delete(void *pMemory) { return Y_aligned_free(pMemory); }

    // setters
    void Set(int32 x_, int32 y_, int32 z_) { m128i = _mm_set_epi32(0, z_, y_, x_); }
    void Set(const SIMDVector2i &v) { m128i = v.m128i; }
    void Set(const Vector3i &v);
    void SetZero() { m128i = _mm_setzero_si128(); }

    // new vector
    inline SIMDVector3i operator+(const SIMDVector3i &v) const { SIMDVector3i r; r.m128i = _mm_add_epi32(m128i, v.m128i); return r; }
    inline SIMDVector3i operator-(const SIMDVector3i &v) const { SIMDVector3i r; r.m128i = _mm_sub_epi32(m128i, v.m128i); return r; }
    inline SIMDVector3i operator*(const SIMDVector3i &v) const { SIMDVector3i r; r.m128i = _mm_mul_epi32(m128i, v.m128i); return r; }
    inline SIMDVector3i operator/(const SIMDVector3i &v) const { SIMDVector3i r; r.m128i = _mm_set_epi32(x / v.x, y / v.y, z / v.z, 0); return r; }
    inline SIMDVector3i operator%(const SIMDVector3i &v) const { SIMDVector3i r; r.m128i = _mm_set_epi32(x % v.x, y % v.y, z % v.z, 0); return r; }
    inline SIMDVector3i operator&(const SIMDVector3i &v) const { SIMDVector3i r; r.m128i = _mm_and_si128(m128i, v.m128i); return r; }
    inline SIMDVector3i operator|(const SIMDVector3i &v) const { SIMDVector3i r; r.m128i = _mm_or_si128(m128i, v.m128i); return r; }
    inline SIMDVector3i operator^(const SIMDVector3i &v) const { SIMDVector3i r; r.m128i = _mm_xor_si128(m128i, v.m128i); return r; }

    // scalar operators
    inline SIMDVector3i operator+(const int32 v) const { SIMDVector3i r; r.m128i = _mm_add_epi32(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector3i operator-(const int32 v) const { SIMDVector3i r; r.m128i = _mm_sub_epi32(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector3i operator*(const int32 v) const { SIMDVector3i r; r.m128i = _mm_mul_epi32(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector3i operator/(const int32 v) const { SIMDVector3i r; r.m128i = _mm_set_epi32(0, z / v, y / v, x / v); return r; }
    inline SIMDVector3i operator%(const int32 v) const { SIMDVector3i r; r.m128i = _mm_set_epi32(0, z % v, y % v, x % v); return r; }
    inline SIMDVector3i operator&(const int32 v) const { SIMDVector3i r; r.m128i = _mm_and_si128(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector3i operator|(const int32 v) const { SIMDVector3i r; r.m128i = _mm_or_si128(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector3i operator^(const int32 v) const { SIMDVector3i r; r.m128i = _mm_xor_si128(m128i, _mm_set1_epi32(v)); return r; }

    // no params
    inline SIMDVector3i operator~() const { SIMDVector3i r; r.m128i = _mm_andnot_si128(m128i, m128i); return r; }
    inline SIMDVector3i operator-() const { SIMDVector3i r; r.m128i = _mm_xor_si128(m128i, _mm_set1_epi32(0x7FFFFFFF)); return r; }

    // comparison operators
    inline bool operator==(const SIMDVector3i &v) const { return (_mm_movemask_epi8(_mm_cmpeq_epi32(m128i, v.m128i)) & 0xFFF) == 0xFFF; }
    inline bool operator!=(const SIMDVector3i &v) const { return (_mm_movemask_epi8(_mm_cmpeq_epi32(m128i, v.m128i)) & 0xFFF) != 0xFFF; }
    inline bool operator<=(const SIMDVector3i &v) const { return (_mm_movemask_epi8(_mm_xor_si128(_mm_cmplt_epi32(m128i, v.m128i), _mm_cmpeq_epi32(m128i, v.m128i))) & 0x333) == 0x333; }
    inline bool operator>=(const SIMDVector3i &v) const { return (_mm_movemask_epi8(_mm_xor_si128(_mm_cmpgt_epi32(m128i, v.m128i), _mm_cmpeq_epi32(m128i, v.m128i))) & 0x333) == 0x333; }
    inline bool operator<(const SIMDVector3i &v) const { return (_mm_movemask_epi8(_mm_cmplt_epi32(m128i, v.m128i)) & 0x333) == 0x333; }
    inline bool operator>(const SIMDVector3i &v) const { return (_mm_movemask_epi8(_mm_cmpgt_epi32(m128i, v.m128i)) & 0x333) == 0x333; }

    // modifies this vector
    inline SIMDVector3i &operator+=(const SIMDVector3i &v) { m128i = _mm_add_epi32(m128i, v.m128i); return *this; }
    inline SIMDVector3i &operator-=(const SIMDVector3i &v) { m128i = _mm_sub_epi32(m128i, v.m128i); return *this; }
    inline SIMDVector3i &operator*=(const SIMDVector3i &v) { m128i = _mm_mul_epi32(m128i, v.m128i); return *this; }
    inline SIMDVector3i &operator/=(const SIMDVector3i &v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
    inline SIMDVector3i &operator%=(const SIMDVector3i &v) { x %= v.x; y %= v.y; z %= v.z; return *this; }
    inline SIMDVector3i &operator&=(const SIMDVector3i &v) { m128i = _mm_and_si128(m128i, v.m128i); return *this; }
    inline SIMDVector3i &operator|=(const SIMDVector3i &v) { m128i = _mm_or_si128(m128i, v.m128i); return *this; }
    inline SIMDVector3i &operator^=(const SIMDVector3i &v) { m128i = _mm_xor_si128(m128i, v.m128i); return *this; }

    // scalar operators
    inline SIMDVector3i &operator+=(const int32 v) { m128i = _mm_add_epi32(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector3i &operator-=(const int32 v) { m128i = _mm_sub_epi32(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector3i &operator*=(const int32 v) { m128i = _mm_mul_epi32(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector3i &operator/=(const int32 v) { x /= v; y /= v; z /= v; return *this; }
    inline SIMDVector3i &operator%=(const int32 v) { x %= v; y %= v; z %= v; return *this; }
    inline SIMDVector3i &operator&=(const int32 v) { m128i = _mm_and_si128(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector3i &operator|=(const int32 v) { m128i = _mm_or_si128(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector3i &operator^=(const int32 v) { m128i = _mm_xor_si128(m128i, _mm_set1_epi32(v)); return *this; }

    inline SIMDVector3i &operator=(const SIMDVector3i &v) { m128i = v.m128i; return *this; }
    SIMDVector3i &operator=(const Vector3i &v);

    // index accessors
    //const int32 &operator[](uint32 i) const { return (&x)[i]; }
    //int32 &operator[](uint32 i) { return (&x)[i]; }
    operator const int32 *() const { return &x; }
    operator int32 *() { return &x; }

    // to intx
    const Vector3i &GetInt3() const { return reinterpret_cast<const Vector3i &>(*this); }
    Vector3i &GetInt3() { return reinterpret_cast<Vector3i &>(*this); }
    operator const Vector3i &() const { return reinterpret_cast<const Vector3i &>(*this); }
    operator Vector3i &() { return reinterpret_cast<Vector3i &>(*this); }

    // partial comparisons
    bool AnyLess(const SIMDVector3i &v) const { return (_mm_movemask_epi8(_mm_cmplt_epi32(m128i, v.m128i)) & 0x7) != 0x0; }
    bool AnyLessEqual(const SIMDVector3i &v) const { return (x <= v.x || y <= v.y || z <= v.z); }
    bool AnyGreater(const SIMDVector3i &v) const { return (_mm_movemask_epi8(_mm_cmpgt_epi32(m128i, v.m128i)) & 0x7) != 0x0; }
    bool AnyGreaterEqual(const SIMDVector3i &v) const { return (x >= v.x || y >= v.y || z <= v.z); }

    // modification functions
    SIMDVector3i Min(const SIMDVector3i &v) const { SIMDVector3i r; r.m128i = _mm_min_epi32(m128i, v.m128i); return r; }
    SIMDVector3i Max(const SIMDVector3i &v) const { SIMDVector3i r; r.m128i = _mm_max_epi32(m128i, v.m128i); return r; }
    SIMDVector3i Clamp(const SIMDVector3i &lBounds, const SIMDVector3i &uBounds) const { SIMDVector3i r; r.m128i = _mm_min_epi32(uBounds.m128i, _mm_max_epi32(lBounds.m128i, m128i)); return r; }
    SIMDVector3i Abs() const { SIMDVector3i r; r.m128i = _mm_abs_epi32(m128i); return r; }
    SIMDVector3i Saturate() const { SIMDVector3i r; r.m128i = _mm_min_epi32(_mm_set1_epi32(1), _mm_max_epi32(_mm_setzero_si128(), m128i)); return r; }
    SIMDVector3i Snap(const SIMDVector3i &v) const { return SIMDVector3i(Math::Snap(x, v.x), Math::Snap(y, v.y), Math::Snap(z, v.z)); }

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    union
    {
        __m128i m128i;
        struct { int32 x, y, z; };
        struct { int32 r, g, b; };
    };

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    static const SIMDVector3i &Zero, &One, &NegativeOne;
};


//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

ALIGN_DECL(Y_SSE_ALIGNMENT) struct SIMDVector4i
{
    // constructors
    inline SIMDVector4i() {}
    inline SIMDVector4i(int32 x_, int32 y_, int32 z_, int32 w_) { m128i = _mm_set_epi32(w_, z_, y_, x_); }
    inline SIMDVector4i(const int32 *p) { m128i = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p)); }
    inline SIMDVector4i(const SIMDVector4i &v) : m128i(v.m128i) {}
    SIMDVector4i(const Vector4i &v);

    // new/delete, we overload these so that vectors allocated on the heap are guaranteed to be aligned correctly
    void *operator new[](size_t c) { return Y_aligned_malloc(c, Y_SSE_ALIGNMENT); }
    void *operator new(size_t c) { return Y_aligned_malloc(c, Y_SSE_ALIGNMENT); }
    void operator delete[](void *pMemory) { return Y_aligned_free(pMemory); }
    void operator delete(void *pMemory) { return Y_aligned_free(pMemory); }

    // setters
    void Set(int32 x_, int32 y_, int32 z_, int32 w_) { m128i = _mm_set_epi32(w_, z_, y_, x_); }
    void Set(const SIMDVector2i &v) { m128i = v.m128i; }
    void Set(const Vector4i &v);
    void SetZero() { m128i = _mm_setzero_si128(); }

    // new vector
    inline SIMDVector4i operator+(const SIMDVector4i &v) const { SIMDVector4i r; r.m128i = _mm_add_epi32(m128i, v.m128i); return r; }
    inline SIMDVector4i operator-(const SIMDVector4i &v) const { SIMDVector4i r; r.m128i = _mm_sub_epi32(m128i, v.m128i); return r; }
    inline SIMDVector4i operator*(const SIMDVector4i &v) const { SIMDVector4i r; r.m128i = _mm_mul_epi32(m128i, v.m128i); return r; }
    inline SIMDVector4i operator/(const SIMDVector4i &v) const { SIMDVector4i r; r.m128i = _mm_set_epi32(x / v.x, y / v.y, z / v.z, w / v.w); return r; }
    inline SIMDVector4i operator%(const SIMDVector4i &v) const { SIMDVector4i r; r.m128i = _mm_set_epi32(x % v.x, y % v.y, z % v.z, w / v.w); return r; }
    inline SIMDVector4i operator&(const SIMDVector4i &v) const { SIMDVector4i r; r.m128i = _mm_and_si128(m128i, v.m128i); return r; }
    inline SIMDVector4i operator|(const SIMDVector4i &v) const { SIMDVector4i r; r.m128i = _mm_or_si128(m128i, v.m128i); return r; }
    inline SIMDVector4i operator^(const SIMDVector4i &v) const { SIMDVector4i r; r.m128i = _mm_xor_si128(m128i, v.m128i); return r; }

    // scalar operators
    inline SIMDVector4i operator+(const int32 v) const { SIMDVector4i r; r.m128i = _mm_add_epi32(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector4i operator-(const int32 v) const { SIMDVector4i r; r.m128i = _mm_sub_epi32(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector4i operator*(const int32 v) const { SIMDVector4i r; r.m128i = _mm_mul_epi32(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector4i operator/(const int32 v) const { SIMDVector4i r; r.m128i = _mm_set_epi32(w / v, z / v, y / v, x / v); return r; }
    inline SIMDVector4i operator%(const int32 v) const { SIMDVector4i r; r.m128i = _mm_set_epi32(w % v, z % v, y % v, x % v); return r; }
    inline SIMDVector4i operator&(const int32 v) const { SIMDVector4i r; r.m128i = _mm_and_si128(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector4i operator|(const int32 v) const { SIMDVector4i r; r.m128i = _mm_or_si128(m128i, _mm_set1_epi32(v)); return r; }
    inline SIMDVector4i operator^(const int32 v) const { SIMDVector4i r; r.m128i = _mm_xor_si128(m128i, _mm_set1_epi32(v)); return r; }

    // no params
    inline SIMDVector4i operator~() const { SIMDVector4i r; r.m128i = _mm_andnot_si128(m128i, m128i); return r; }
    inline SIMDVector4i operator-() const { SIMDVector4i r; r.m128i = _mm_xor_si128(m128i, _mm_set1_epi32(0x7FFFFFFF)); return r; }

    // comparison operators
    inline bool operator==(const SIMDVector4i &v) const { return (_mm_movemask_epi8(_mm_cmpeq_epi32(m128i, v.m128i)) & 0xFFFF) == 0xFFFF; }
    inline bool operator!=(const SIMDVector4i &v) const { return (_mm_movemask_epi8(_mm_cmpeq_epi32(m128i, v.m128i)) & 0xFFFF) != 0xFFFF; }
    inline bool operator<=(const SIMDVector4i &v) const { return (_mm_movemask_epi8(_mm_xor_si128(_mm_cmplt_epi32(m128i, v.m128i), _mm_cmpeq_epi32(m128i, v.m128i))) & 0x3333) == 0x3333; }
    inline bool operator>=(const SIMDVector4i &v) const { return (_mm_movemask_epi8(_mm_xor_si128(_mm_cmpgt_epi32(m128i, v.m128i), _mm_cmpeq_epi32(m128i, v.m128i))) & 0x3333) == 0x3333; }
    inline bool operator<(const SIMDVector4i &v) const { return (_mm_movemask_epi8(_mm_cmplt_epi32(m128i, v.m128i)) & 0x3333) == 0x3333; }
    inline bool operator>(const SIMDVector4i &v) const { return (_mm_movemask_epi8(_mm_cmpgt_epi32(m128i, v.m128i)) & 0x3333) == 0x3333; }

    // modifies this vector
    inline SIMDVector4i &operator+=(const SIMDVector4i &v) { m128i = _mm_add_epi32(m128i, v.m128i); return *this; }
    inline SIMDVector4i &operator-=(const SIMDVector4i &v) { m128i = _mm_sub_epi32(m128i, v.m128i); return *this; }
    inline SIMDVector4i &operator*=(const SIMDVector4i &v) { m128i = _mm_mul_epi32(m128i, v.m128i); return *this; }
    inline SIMDVector4i &operator/=(const SIMDVector4i &v) { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }
    inline SIMDVector4i &operator%=(const SIMDVector4i &v) { x %= v.x; y %= v.y; z %= v.z; w %= v.w; return *this; }
    inline SIMDVector4i &operator&=(const SIMDVector4i &v) { m128i = _mm_and_si128(m128i, v.m128i); return *this; }
    inline SIMDVector4i &operator|=(const SIMDVector4i &v) { m128i = _mm_or_si128(m128i, v.m128i); return *this; }
    inline SIMDVector4i &operator^=(const SIMDVector4i &v) { m128i = _mm_xor_si128(m128i, v.m128i); return *this; }

    // scalar operators
    inline SIMDVector4i &operator+=(const int32 v) { m128i = _mm_add_epi32(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector4i &operator-=(const int32 v) { m128i = _mm_sub_epi32(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector4i &operator*=(const int32 v) { m128i = _mm_mul_epi32(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector4i &operator/=(const int32 v) { x /= v; y /= v; z /= v; w /= v; return *this; }
    inline SIMDVector4i &operator%=(const int32 v) { x %= v; y %= v; z %= v; w %= v; return *this; }
    inline SIMDVector4i &operator&=(const int32 v) { m128i = _mm_and_si128(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector4i &operator|=(const int32 v) { m128i = _mm_or_si128(m128i, _mm_set1_epi32(v)); return *this; }
    inline SIMDVector4i &operator^=(const int32 v) { m128i = _mm_xor_si128(m128i, _mm_set1_epi32(v)); return *this; }

    inline SIMDVector4i &operator=(const SIMDVector4i &v) { m128i = v.m128i; return *this; }
    SIMDVector4i &operator=(const Vector4i &v);

    // index accessors
    //const int32 &operator[](int32 i) const { return (&x)[i]; }
    //int32 &operator[](int32 i) { return (&x)[i]; }
    operator const int32 *() const { return &x; }
    operator int32 *() { return &x; }

    // to floatx
    const Vector4i &GetFloat4() const { return reinterpret_cast<const Vector4i &>(*this); }
    Vector4i &GetFloat4() { return reinterpret_cast<Vector4i &>(*this); }
    operator const Vector4i &() const { return reinterpret_cast<const Vector4i &>(*this); }
    operator Vector4i &() { return reinterpret_cast<Vector4i &>(*this); }

    // partial comparisons
    bool AnyLess(const SIMDVector4i &v) const { return _mm_movemask_epi8(_mm_cmplt_epi32(m128i, v.m128i)) != 0x00; }
    bool AnyLessEqual(const SIMDVector4i &v) const { return (x <= v.x || y <= v.y || z <= v.z || w <= v.w); }
    bool AnyGreater(const SIMDVector4i &v) const { return _mm_movemask_epi8(_mm_cmpgt_epi32(m128i, v.m128i)) != 0x00; }
    bool AnyGreaterEqual(const SIMDVector4i &v) const { return (x >= v.x || y >= v.y || z >= v.z || w >= v.w); }

    // modification functions
    SIMDVector4i Min(const SIMDVector4i &v) const { SIMDVector4i r; r.m128i = _mm_min_epi32(m128i, v.m128i); return r; }
    SIMDVector4i Max(const SIMDVector4i &v) const { SIMDVector4i r; r.m128i = _mm_max_epi32(m128i, v.m128i); return r; }
    SIMDVector4i Clamp(const SIMDVector4i &lBounds, const SIMDVector4i &uBounds) const { SIMDVector4i r; r.m128i = _mm_min_epi32(uBounds.m128i, _mm_max_epi32(lBounds.m128i, m128i)); return r; }
    SIMDVector4i Abs() const { SIMDVector4i r; r.m128i = _mm_abs_epi32(m128i); return r; }
    SIMDVector4i Saturate() const { SIMDVector4i r; r.m128i = _mm_min_epi32(_mm_set1_epi32(1), _mm_max_epi32(_mm_setzero_si128(), m128i)); return r; }
    SIMDVector4i Snap(const SIMDVector4i &v) const { return SIMDVector4i(Math::Snap(x, v.x), Math::Snap(y, v.y), Math::Snap(z, v.z), Math::Snap(w, v.w)); }

    // swap
    void Swap(SIMDVector4i &v) { __m128i temp = m128i; m128i = v.m128i; v.m128i = temp; }

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    union
    {
        __m128i m128i;
        struct { int32 x, y, z, w; };
        struct { int32 r, g, b, a; };
    };

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    static const SIMDVector4i &Zero, &One, &NegativeOne;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------