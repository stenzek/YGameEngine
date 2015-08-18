#pragma once
#include "Core/Math.h"
#include "Core/SIMD/Vector_shuffles.h"
#include "Core/Memory.h"
#include <intrin.h>

#if Y_CPU_SSE_LEVEL < 1
    #error SSE must be enabled.
#endif

struct Vector2f;
struct Vector3f;
struct Vector4f;

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

ALIGN_DECL(Y_SSE_ALIGNMENT) struct SIMDVector2f
{
    // constructors
    inline SIMDVector2f() {}
    inline SIMDVector2f(const float x_, const float y_) { m128 = _mm_set_ps(0.0f, 0.0f, y_, x_); }
    inline SIMDVector2f(const float *p) { m128 = _mm_set_ps(0.0f, 0.0f, p[1], p[0]); }
    inline SIMDVector2f(const SIMDVector2f &v) : m128(v.m128) {}
    inline SIMDVector2f(const Vector2i v) { m128 = _mm_set_ps(0.0f, 0.0f, (float)v.y, (float)v.x); }
    SIMDVector2f(const Vector2f &v);

    // new/delete, we overload these so that vectors allocated on the heap are guaranteed to be aligned correctly
    void *operator new[](size_t c) { return Y_aligned_malloc(c, Y_SSE_ALIGNMENT); }
    void *operator new(size_t c) { return Y_aligned_malloc(c, Y_SSE_ALIGNMENT); }
    void operator delete[](void *pMemory) { return Y_aligned_free(pMemory); }
    void operator delete(void *pMemory) { return Y_aligned_free(pMemory); }

    // setters
    void Set(const float x_, const float y_) { m128 = _mm_set_ps(0.0f, 0.0f, y_, x_); }
    void Set(const SIMDVector2f v) { m128 = v.m128; }
    void Set(const Vector2f &v);
    void SetZero() { m128 = _mm_setzero_ps(); }

    // load/store
    void Load(const float *v) { m128 = _mm_set_ps(0.0f, 0.0f, v[1], v[0]); }
    void Store(float *v) const { v[0] = m128.m128_f32[0]; v[1] = m128.m128_f32[1]; v[2] = m128.m128_f32[2]; }

    // new vector
    inline SIMDVector2f operator+(const SIMDVector2f v) const { SIMDVector2f r; r.m128 = _mm_add_ps(m128, v.m128); return r; }
    inline SIMDVector2f operator-(const SIMDVector2f v) const { SIMDVector2f r; r.m128 = _mm_sub_ps(m128, v.m128); return r; }
    inline SIMDVector2f operator*(const SIMDVector2f v) const { SIMDVector2f r; r.m128 = _mm_mul_ps(m128, v.m128); return r; }
    inline SIMDVector2f operator/(const SIMDVector2f v) const { SIMDVector2f r; r.m128 = _mm_div_ps(m128, v.m128); return r; }
	inline SIMDVector2f operator%(const SIMDVector2f v) const { return SIMDVector2f(Y_fmodf(x, v.x), Y_fmodf(y, v.y)); }
    inline SIMDVector2f operator-() const { SIMDVector2f r; r.m128 = _mm_sub_ps(_mm_setzero_ps(), m128); return r; }

    // scalar operators
    inline SIMDVector2f operator+(const float v) const { SIMDVector2f r; r.m128 = _mm_add_ps(m128, _mm_set_ps1(v)); return r; }
    inline SIMDVector2f operator-(const float v) const { SIMDVector2f r; r.m128 = _mm_sub_ps(m128, _mm_set_ps1(v)); return r; }
    inline SIMDVector2f operator*(const float v) const { SIMDVector2f r; r.m128 = _mm_mul_ps(m128, _mm_set_ps1(v)); return r; }
    inline SIMDVector2f operator/(const float v) const { SIMDVector2f r; r.m128 = _mm_div_ps(m128, _mm_set_ps1(v)); return r; }
	inline SIMDVector2f operator%(const float v) const { return SIMDVector2f(Y_fmodf(x, v), Y_fmodf(y, v)); }

    // comparison operators
    inline bool operator==(const SIMDVector2f v) const { return (_mm_movemask_ps(_mm_cmpeq_ps(m128, v.m128)) & 0x3) == 0x3; }
    inline bool operator!=(const SIMDVector2f v) const { return (_mm_movemask_ps(_mm_cmpneq_ps(m128, v.m128)) & 0x3) != 0x0; }
    inline bool operator<=(const SIMDVector2f v) const { return (_mm_movemask_ps(_mm_cmple_ps(m128, v.m128)) & 0x3) == 0x3; }
    inline bool operator>=(const SIMDVector2f v) const { return (_mm_movemask_ps(_mm_cmpge_ps(m128, v.m128)) & 0x3) == 0x3; }
    inline bool operator<(const SIMDVector2f v) const { return (_mm_movemask_ps(_mm_cmplt_ps(m128, v.m128)) & 0x3) == 0x3; }
    inline bool operator>(const SIMDVector2f v) const { return (_mm_movemask_ps(_mm_cmpgt_ps(m128, v.m128)) & 0x3) == 0x3; }

    // modifies this vector
    inline SIMDVector2f &operator+=(const SIMDVector2f v) { m128 = _mm_add_ps(m128, v.m128); return *this; }
    inline SIMDVector2f &operator-=(const SIMDVector2f v) { m128 = _mm_sub_ps(m128, v.m128); return *this; }
    inline SIMDVector2f &operator*=(const SIMDVector2f v) { m128 = _mm_mul_ps(m128, v.m128); return *this; }
    inline SIMDVector2f &operator/=(const SIMDVector2f v) { m128 = _mm_div_ps(m128, v.m128); return *this; }
	inline SIMDVector2f &operator%=(const SIMDVector2f v) { x = Y_fmodf(x, v.x); y = Y_fmodf(y, v.y); return *this; }
    inline SIMDVector2f &operator=(const SIMDVector2f v) { m128 = v.m128; return *this; }
    SIMDVector2f &operator=(const Vector2f &v);

    // scalar operators
    inline SIMDVector2f &operator+=(const float v) { m128 = _mm_add_ps(m128, _mm_set_ps1(v)); return *this; }
    inline SIMDVector2f &operator-=(const float v) { m128 = _mm_sub_ps(m128, _mm_set_ps1(v)); return *this; }
    inline SIMDVector2f &operator*=(const float v) { m128 = _mm_mul_ps(m128, _mm_set_ps1(v)); return *this; }
    inline SIMDVector2f &operator/=(const float v) { m128 = _mm_div_ps(m128, _mm_set_ps1(v)); return *this; }
	inline SIMDVector2f &operator%=(const float v) { x = Y_fmodf(x, v); y = Y_fmodf(y, v); return *this; }

    // index accessors
    //const float &operator[](uint32 i) const { return (&x)[i]; }
    //float &operator[](uint32 i) { return (&x)[i]; }
    operator const float *() const { return &x; }
    operator float *() { return &x; }

    // to floatx
    const Vector2f &GetFloat2() const { return reinterpret_cast<const Vector2f &>(*this); }
    Vector2f &GetFloat2() { return reinterpret_cast<Vector2f &>(*this); }
    operator const Vector2f &() const { return reinterpret_cast<const Vector2f &>(*this); }
    operator Vector2f &() { return reinterpret_cast<Vector2f &>(*this); }

    // partial comparisons
    bool AnyLess(const SIMDVector2f v) const { return (_mm_movemask_ps(_mm_cmplt_ps(m128, v.m128)) & 0x3) != 0x0; }
    bool AnyLessEqual(const SIMDVector2f v) const { return (_mm_movemask_ps(_mm_cmple_ps(m128, v.m128)) & 0x3) != 0x0; }
    bool AnyGreater(const SIMDVector2f v) const { return (_mm_movemask_ps(_mm_cmpgt_ps(m128, v.m128)) & 0x3) != 0x0; }
    bool AnyGreaterEqual(const SIMDVector2f v) const { return (_mm_movemask_ps(_mm_cmpge_ps(m128, v.m128)) & 0x3) != 0x0; }
    bool NearEqual(const SIMDVector2f v) const { return (*this - v).Abs() < Epsilon; }
    bool NearEqual(const SIMDVector2f v, const float fEpsilon) const { return (*this - v).Abs() < SIMDVector2f(fEpsilon, fEpsilon); }
    bool IsFinite() const { return (*this != Infinite); }

    // clamps
    SIMDVector2f Min(const SIMDVector2f v) const { SIMDVector2f r; r.m128 = _mm_min_ps(m128, v.m128); return r; }
    SIMDVector2f Max(const SIMDVector2f v) const { SIMDVector2f r; r.m128 = _mm_max_ps(m128, v.m128); return r; }
    SIMDVector2f Clamp(const SIMDVector2f lBounds, const SIMDVector2f uBounds) const { SIMDVector2f r; r.m128 = _mm_max_ps(lBounds.m128, _mm_min_ps(uBounds.m128, m128)); return r; }
    SIMDVector2f Abs() const { SIMDVector2f r; r.m128 = _mm_max_ps(m128, _mm_sub_ps(_mm_setzero_ps(), m128)); return r; }
    SIMDVector2f Saturate() const { SIMDVector2f r; r.m128 = _mm_max_ps(One.m128, _mm_min_ps(_mm_setzero_ps(), m128)); return r; }

    // swap
    void Swap(SIMDVector2f v) { __m128 temp = m128; m128 = v.m128; v.m128 = temp; }

    // internal dot product, uses dp on sse4, hadd on sse3, shuffle+add on sse
#if Y_CPU_SSE_LEVEL >= 4
    inline __m128 __Dot(const SIMDVector2f v) const
    { 
        return _mm_dp_ps(m128, v.m128, 0xC1);
    }
#elif Y_CPU_SSE_LEVEL >= 3
    __m128 __Dot(const SIMDVector2f v) const
    {
        __m128 tmp = _mm_mul_ps(m128, v.m128);
        tmp = _mm_hadd_ps(tmp, tmp);
        return tmp;
    }
#else
    __m128 __Dot(const SIMDVector2f v) const
    {
        // wastes registers, can be optimized still, least the 3 shuffles can be executed in parallel
        __m128 m, tmp1;
        m = _mm_mul_ps(m128, v.m128);
        tmp1 = _mm_shuffle_ps(m, m, _MM_SHUFFLE(1, 1, 1, 1));
        return _mm_add_ss(m, tmp1);
    }
#endif

    // actual dot product
    float Dot(const SIMDVector2f v) const { return _mm_cvtss_f32(__Dot(v)); }

    // lerp
    SIMDVector2f Lerp(const SIMDVector2f v, const float f) const
    {
        SIMDVector2f r;
        r.m128 = _mm_add_ps(m128, _mm_mul_ps(_mm_sub_ps(v.m128, m128), _mm_set_ps1(f)));
        return r;
    }

    // length
    inline float SquaredLength() const { return _mm_cvtss_f32(__Dot(*this)); }
    float Length() const { return _mm_cvtss_f32(_mm_sqrt_ss(__Dot(*this))); }
    
    // normalize
    SIMDVector2f Normalize() const
    {
        SIMDVector2f r;
        __m128 length = _mm_sqrt_ss(__Dot(*this));
        r.m128 = _mm_div_ps(m128, _mm_shuffle_ps(length, length, _MM_SHUFFLE(0, 0, 0, 0)));
        return r;
    }

    // fast normalize
    SIMDVector2f NormalizeEst() const
    {
        SIMDVector2f r;
        __m128 invLength = _mm_rsqrt_ss(__Dot(*this));
        r.m128 = _mm_mul_ps(m128, _mm_shuffle_ps(invLength, invLength, _MM_SHUFFLE(0, 0, 0, 0)));
        return r;
    }

    // in-place normalize
    void NormalizeInPlace()
    {
        __m128 length = _mm_sqrt_ss(__Dot(*this));
        m128 = _mm_div_ps(m128, _mm_shuffle_ps(length, length, _MM_SHUFFLE(0, 0, 0, 0)));
    }

    // fast normalize
    void NormalizeEstInPlace()
    {
        __m128 invLength = _mm_rsqrt_ss(__Dot(*this));
        m128 = _mm_mul_ps(m128, _mm_shuffle_ps(invLength, invLength, _MM_SHUFFLE(0, 0, 0, 0)));
    }

    // safe normalize
    SIMDVector2f SafeNormalize() const
    {
        SIMDVector2f r;
        __m128 len = __Dot(*this);
        if ((_mm_movemask_ps(_mm_cmpneq_ss(len, Zero.m128)) & 0x1))
        {
            len = _mm_sqrt_ss(len);
            r.m128 = _mm_div_ps(m128, _mm_shuffle_ps(len, len, _MM_SHUFFLE(0, 0, 0, 0)));
        }
        else
        {
            r.m128 = m128;
        }
        return r;
    }
    SIMDVector2f SafeNormalizeEst() const
    {
        SIMDVector2f r;
        __m128 len = __Dot(*this);
        if ((_mm_movemask_ps(_mm_cmpneq_ss(len, Zero.m128)) & 0x1))
        {
            len = _mm_rsqrt_ss(len);
            r.m128 = _mm_mul_ps(m128, _mm_shuffle_ps(len, len, _MM_SHUFFLE(0, 0, 0, 0)));
        }
        else
        {
            r.m128 = m128;
        }
        return r;
    }
    void SafeNormalizeInPlace()
    {
        __m128 len = __Dot(*this);
        if ((_mm_movemask_ps(_mm_cmpneq_ss(len, Zero.m128)) & 0x1))
        {
            len = _mm_sqrt_ss(len);
            m128 = _mm_div_ps(m128, _mm_shuffle_ps(len, len, _MM_SHUFFLE(0, 0, 0, 0)));
        }
    }
    void SafeNormalizeEstInPlace()
    {
        __m128 len = __Dot(*this);
        if ((_mm_movemask_ps(_mm_cmpneq_ss(len, Zero.m128)) & 0x1))
        {
            len = _mm_rsqrt_ss(len);
            m128 = _mm_mul_ps(m128, _mm_shuffle_ps(len, len, _MM_SHUFFLE(0, 0, 0, 0)));
        }
    }

    // reciprocal
    SIMDVector2f Reciprocal() const
    {
        SIMDVector2f r;
        r.m128 = _mm_div_ps(One.m128, m128);
        return r;
    }

    // not sse yet
    SIMDVector2f Cross(const SIMDVector2f v) const
    {
        SIMDVector2f r;
        r.x = (x * v.y) - (y * v.x);
        r.y = (x * v.y) - (y * v.x);
        return r;
    }

    // shuffles - todo fix return types on these
    template<int V0, int V1> SIMDVector2f Shuffle2() const { SIMDVector2f r; r.m128 = _mm_shuffle_ps(m128, m128, _MM_SHUFFLE(0, 0, V1, V0)); return r; }
    VECTOR2_SHUFFLE_FUNCTIONS(SIMDVector2f);

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    union
    {
        __m128 m128;
        struct { float x; float y; };
        struct { float r; float g; };
    };

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    static const SIMDVector2f &Zero, &One, &NegativeOne;
    static const SIMDVector2f &Infinite, &NegativeInfinite;
    static const SIMDVector2f &UnitX, &UnitY, &UnitZ, &UnitW, &NegativeUnitX, &NegativeUnitY, &NegativeUnitZ, &NegativeUnitW;
    static const SIMDVector2f &Epsilon;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

ALIGN_DECL(Y_SSE_ALIGNMENT) struct SIMDVector3f
{
    // constructors
    inline SIMDVector3f() {}
    inline SIMDVector3f(const float x_, const float y_, const float z_) { m128 = _mm_set_ps(0.0f, z_, y_, x_); }
    inline SIMDVector3f(const float *p) { m128 = _mm_set_ps(0.0f, p[2], p[1], p[0]); }
    inline SIMDVector3f(const SIMDVector2f v, const float z_ = 0.0f) { m128 = _mm_set_ps(0.0f, z_, v.y, v.x); }
    inline SIMDVector3f(const SIMDVector3f &v) : m128(v.m128) {}
    inline SIMDVector3f(const Vector3i v) { m128 = _mm_set_ps(0.0f, (float)v.z, (float)v.y, (float)v.x); }
    SIMDVector3f(const Vector3f &v);

    // new/delete, we overload these so that vectors allocated on the heap are guaranteed to be aligned correctly
    void *operator new[](size_t c) { return Y_aligned_malloc(c, Y_SSE_ALIGNMENT); }
    void *operator new(size_t c) { return Y_aligned_malloc(c, Y_SSE_ALIGNMENT); }
    void operator delete[](void *pMemory) { return Y_aligned_free(pMemory); }
    void operator delete(void *pMemory) { return Y_aligned_free(pMemory); }

    // setters
    void Set(const float x_, const float y_, const float z_) { m128 = _mm_set_ps(0.0f, z_, y_, x_); }
    void Set(const SIMDVector2f v, const float z_ = 0.0f) { m128 = _mm_set_ps(0.0f, z_, v.y, v.x); }
    void Set(const SIMDVector3f v) { m128 = v.m128; }
    void Set(const Vector3f &v);
    void SetZero() { m128 = _mm_setzero_ps(); }

    // load/store
    void Load(const float *v) { m128 = _mm_set_ps(0.0f, v[2], v[1], v[0]); }
    void Store(float *v) const { v[0] = m128.m128_f32[0]; v[1] = m128.m128_f32[1]; v[2] = m128.m128_f32[2]; v[3] = m128.m128_f32[3]; }

    // new vector
    inline SIMDVector3f operator+(const SIMDVector3f v) const { SIMDVector3f r; r.m128 = _mm_add_ps(m128, v.m128); return r; }
    inline SIMDVector3f operator-(const SIMDVector3f v) const { SIMDVector3f r; r.m128 = _mm_sub_ps(m128, v.m128); return r; }
    inline SIMDVector3f operator*(const SIMDVector3f v) const { SIMDVector3f r; r.m128 = _mm_mul_ps(m128, v.m128); return r; }
    inline SIMDVector3f operator/(const SIMDVector3f v) const { SIMDVector3f r; r.m128 = _mm_div_ps(m128, v.m128); return r; }
	inline SIMDVector3f operator%(const SIMDVector3f v) const { return SIMDVector3f(Y_fmodf(x, v.x), Y_fmodf(y, v.y), Y_fmodf(z, v.z)); }
    inline SIMDVector3f operator-() const { SIMDVector3f r; r.m128 = _mm_sub_ps(_mm_setzero_ps(), m128); return r; }

    // scalar operators
    inline SIMDVector3f operator+(const float v) const { SIMDVector3f r; r.m128 = _mm_add_ps(m128, _mm_set_ps1(v)); return r; }
    inline SIMDVector3f operator-(const float v) const { SIMDVector3f r; r.m128 = _mm_sub_ps(m128, _mm_set_ps1(v)); return r; }
    inline SIMDVector3f operator*(const float v) const { SIMDVector3f r; r.m128 = _mm_mul_ps(m128, _mm_set_ps1(v)); return r; }
    inline SIMDVector3f operator/(const float v) const { SIMDVector3f r; r.m128 = _mm_div_ps(m128, _mm_set_ps1(v)); return r; }
	inline SIMDVector3f operator%(const float v) const { return SIMDVector3f(Y_fmodf(x, v), Y_fmodf(y, v), Y_fmodf(z, v)); }

    // comparison operators
    inline bool operator==(const SIMDVector3f v) const { return (_mm_movemask_ps(_mm_cmpeq_ps(m128, v.m128)) & 0x7) == 0x7; }
    inline bool operator!=(const SIMDVector3f v) const { return (_mm_movemask_ps(_mm_cmpneq_ps(m128, v.m128)) & 0x7) != 0x0; }
    inline bool operator<=(const SIMDVector3f v) const { return (_mm_movemask_ps(_mm_cmple_ps(m128, v.m128)) & 0x7) == 0x7; }
    inline bool operator>=(const SIMDVector3f v) const { return (_mm_movemask_ps(_mm_cmpge_ps(m128, v.m128)) & 0x7) == 0x7; }
    inline bool operator<(const SIMDVector3f v) const { return (_mm_movemask_ps(_mm_cmplt_ps(m128, v.m128)) & 0x7) == 0x7; }
    inline bool operator>(const SIMDVector3f v) const { return (_mm_movemask_ps(_mm_cmpgt_ps(m128, v.m128)) & 0x7) == 0x7; }

    // modifies this vector
    inline SIMDVector3f &operator+=(const SIMDVector3f v) { m128 = _mm_add_ps(m128, v.m128); return *this; }
    inline SIMDVector3f &operator-=(const SIMDVector3f v) { m128 = _mm_sub_ps(m128, v.m128); return *this; }
    inline SIMDVector3f &operator*=(const SIMDVector3f v) { m128 = _mm_mul_ps(m128, v.m128); return *this; }
    inline SIMDVector3f &operator/=(const SIMDVector3f v) { m128 = _mm_div_ps(m128, v.m128); return *this; }
	inline SIMDVector3f &operator%=(const SIMDVector3f v) { x = Y_fmodf(x, v.x); y = Y_fmodf(y, v.y); z = Y_fmodf(z, v.z); return *this; }
    inline SIMDVector3f &operator=(const SIMDVector3f v) { m128 = v.m128; return *this; }
    SIMDVector3f &operator=(const Vector3f &v);

    // scalar operators
    inline SIMDVector3f &operator+=(const float v) { m128 = _mm_add_ps(m128, _mm_set_ps1(v)); return *this; }
    inline SIMDVector3f &operator-=(const float v) { m128 = _mm_sub_ps(m128, _mm_set_ps1(v)); return *this; }
    inline SIMDVector3f &operator*=(const float v) { m128 = _mm_mul_ps(m128, _mm_set_ps1(v)); return *this; }
    inline SIMDVector3f &operator/=(const float v) { m128 = _mm_div_ps(m128, _mm_set_ps1(v)); return *this; }
	inline SIMDVector3f &operator%=(const float v) { x = Y_fmodf(x, v); y = Y_fmodf(y, v); z = Y_fmodf(z, v); return *this; }

    // index accessors
    //const float &operator[](uint32 i) const { return (&x)[i]; }
    //float &operator[](uint32 i) { return (&x)[i]; }
    operator const float *() const { return &x; }
    operator float *() { return &x; }

    // to floatx
    const Vector3f &GetFloat3() const { return reinterpret_cast<const Vector3f &>(*this); }
    Vector3f &GetFloat3() { return reinterpret_cast<Vector3f &>(*this); }
    operator const Vector3f &() const { return reinterpret_cast<const Vector3f &>(*this); }
    operator Vector3f &() { return reinterpret_cast<Vector3f &>(*this); }

    // partial comparisons
    bool AnyLess(const SIMDVector3f v) const { return (_mm_movemask_ps(_mm_cmplt_ps(m128, v.m128)) & 0x7) != 0x0; }
    bool AnyLessEqual(const SIMDVector3f v) const { return (_mm_movemask_ps(_mm_cmple_ps(m128, v.m128)) & 0x7) != 0x0; }
    bool AnyGreater(const SIMDVector3f v) const { return (_mm_movemask_ps(_mm_cmpgt_ps(m128, v.m128)) & 0x7) != 0x0; }
    bool AnyGreaterEqual(const SIMDVector3f v) const { return (_mm_movemask_ps(_mm_cmpge_ps(m128, v.m128)) & 0x7) != 0x0; }
    bool NearEqual(const SIMDVector3f v) const { return (*this - v).Abs() < Epsilon; }
    bool NearEqual(const SIMDVector3f v, const float fEpsilon) const { return (*this - v).Abs() < SIMDVector3f(fEpsilon, fEpsilon, fEpsilon); }
    bool IsFinite() const { return (*this != Infinite); }

    // clamps
    SIMDVector3f Min(const SIMDVector3f v) const { SIMDVector3f r; r.m128 = _mm_min_ps(m128, v.m128); return r; }
    SIMDVector3f Max(const SIMDVector3f v) const { SIMDVector3f r; r.m128 = _mm_max_ps(m128, v.m128); return r; }
    SIMDVector3f Clamp(const SIMDVector3f lBounds, const SIMDVector3f uBounds) const { SIMDVector3f r; r.m128 = _mm_max_ps(lBounds.m128, _mm_min_ps(uBounds.m128, m128)); return r; }
    SIMDVector3f Abs() const { SIMDVector3f r; r.m128 = _mm_max_ps(m128, _mm_sub_ps(_mm_setzero_ps(), m128)); return r; }
    SIMDVector3f Saturate() const { static const __m128 ones = _mm_set_ps1(1.0f); SIMDVector3f r; r.m128 = _mm_max_ps(ones, _mm_min_ps(_mm_setzero_ps(), m128)); return r; }

    // swap
    void Swap(SIMDVector3f v) { __m128 temp = m128; m128 = v.m128; v.m128 = temp; }

    // internal dot product, uses dp on sse4, hadd on sse3, shuffle+add on sse
#if Y_CPU_SSE_LEVEL >= 4
    inline __m128 __Dot(const SIMDVector3f v) const
    { 
        return _mm_dp_ps(m128, v.m128, 0xF1);
    }
#elif Y_CPU_SSE_LEVEL >= 3
    __m128 __Dot(const SIMDVector3f v) const
    {
        __m128 tmp = _mm_mul_ps(m128, v.m128);  // w1*w2, z1*z2, y1*y2, x1*x2
        tmp = _mm_hadd_ps(tmp, tmp);            // w1*w2+z1*z2, y1*y2+x1*x2, w1*w2+z1*z2, y1*y2+x1*x2
        tmp = _mm_hadd_ps(tmp, tmp);            // w1*w2+z1*z2+y1*y2+x1*x2, w1*w2+z1*z2+y1*y2+x1*x2, w1*w2+z1*z2+y1*y2+x1*x2, w1*w2+z1*z2+y1*y2+x1*x2
        return tmp;
    }
#else
    __m128 __Dot(const SIMDVector3f v) const
    {
        // wastes registers, can be optimized still, least the 3 shuffles can be executed in parallel
        __m128 m, tmp1, tmp2;
        m = _mm_mul_ps(m128, v.m128);
        tmp1 = _mm_shuffle_ps(m, m, _MM_SHUFFLE(1, 1, 1, 1));
        tmp2 = _mm_shuffle_ps(m, m, _MM_SHUFFLE(2, 2, 2, 2));
        return _mm_add_ss(m, _mm_add_ss(tmp1, tmp2));
    }
#endif

    // actual dot product
    float Dot(const SIMDVector3f v) const { return _mm_cvtss_f32(__Dot(v)); }

    // lerp
    SIMDVector3f Lerp(const SIMDVector3f v, const float f) const
    {
        SIMDVector3f r;
        r.m128 = _mm_add_ps(m128, _mm_mul_ps(_mm_sub_ps(v.m128, m128), _mm_set_ps1(f)));
        return r;
    }

    // length
    inline float SquaredLength() const { return _mm_cvtss_f32(__Dot(*this)); }
    float Length() const { return _mm_cvtss_f32(_mm_sqrt_ss(__Dot(*this))); }
    
    // normalize
    SIMDVector3f Normalize() const
    {
        SIMDVector3f r;
        __m128 length = _mm_sqrt_ss(__Dot(*this));
        r.m128 = _mm_div_ps(m128, _mm_shuffle_ps(length, length, _MM_SHUFFLE(0, 0, 0, 0)));
        return r;
    }

    // fast normalize
    SIMDVector3f NormalizeEst() const
    {
        SIMDVector3f r;
        __m128 invLength = _mm_rsqrt_ss(__Dot(*this));
        r.m128 = _mm_mul_ps(m128, _mm_shuffle_ps(invLength, invLength, _MM_SHUFFLE(0, 0, 0, 0)));
        return r;
    }

    // in-place normalize
    void NormalizeInPlace()
    {
        __m128 length = _mm_sqrt_ss(__Dot(*this));
        m128 = _mm_div_ps(m128, _mm_shuffle_ps(length, length, _MM_SHUFFLE(0, 0, 0, 0)));
    }

    // fast normalize
    void NormalizeEstInPlace()
    {
        __m128 invLength = _mm_rsqrt_ss(__Dot(*this));
        m128 = _mm_mul_ps(m128, _mm_shuffle_ps(invLength, invLength, _MM_SHUFFLE(0, 0, 0, 0)));
    }

    // safe normalize
    SIMDVector3f SafeNormalize() const
    {
        SIMDVector3f r;
        __m128 len = __Dot(*this);
        if ((_mm_movemask_ps(_mm_cmpneq_ss(len, Zero.m128)) & 0x1))
        {
            len = _mm_sqrt_ss(len);
            r.m128 = _mm_div_ps(m128, _mm_shuffle_ps(len, len, _MM_SHUFFLE(0, 0, 0, 0)));
        }
        else
        {
            r.m128 = m128;
        }
        return r;
    }
    SIMDVector3f SafeNormalizeEst() const
    {
        SIMDVector3f r;
        __m128 len = __Dot(*this);
        if ((_mm_movemask_ps(_mm_cmpneq_ss(len, Zero.m128)) & 0x1))
        {
            len = _mm_rsqrt_ss(len);
            r.m128 = _mm_mul_ps(m128, _mm_shuffle_ps(len, len, _MM_SHUFFLE(0, 0, 0, 0)));
        }
        else
        {
            r.m128 = m128;
        }
        return r;
    }
    void SafeNormalizeInPlace()
    {
        __m128 len = __Dot(*this);
        if ((_mm_movemask_ps(_mm_cmpneq_ss(len, Zero.m128)) & 0x1))
        {
            len = _mm_sqrt_ss(len);
            m128 = _mm_div_ps(m128, _mm_shuffle_ps(len, len, _MM_SHUFFLE(0, 0, 0, 0)));
        }
    }
    void SafeNormalizeEstInPlace()
    {
        __m128 len = __Dot(*this);
        if ((_mm_movemask_ps(_mm_cmpneq_ss(len, Zero.m128)) & 0x1))
        {
            len = _mm_rsqrt_ss(len);
            m128 = _mm_mul_ps(m128, _mm_shuffle_ps(len, len, _MM_SHUFFLE(0, 0, 0, 0)));
        }
    }

    // reciprocal
    SIMDVector3f Reciprocal() const
    {
        SIMDVector3f r;
        r.m128 = _mm_div_ps(One.m128, m128);
        return r;
    }

    // not sse yet
    SIMDVector3f Cross(const SIMDVector3f &v) const
    {
        SIMDVector3f r;
        r.x = (y * v.z) - (z * v.y);
        r.y = (z * v.x) - (x * v.z);
        r.z = (x * v.y) - (y * v.x);
        return r;
    }

    // shuffles - todo fix return types on these
    template<int V0, int V1> SIMDVector2f Shuffle2() const { SIMDVector2f r; r.m128 = _mm_shuffle_ps(m128, m128, _MM_SHUFFLE(0, 0, V1, V0)); return r; }
    template<int V0, int V1, int V2> SIMDVector3f Shuffle3() const { SIMDVector3f r; r.m128 = _mm_shuffle_ps(m128, m128, _MM_SHUFFLE(0, V2, V1, V0)); return r; }
    VECTOR3_SHUFFLE_FUNCTIONS(SIMDVector2f, SIMDVector3f);

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    union
    {
        __m128 m128;
        struct { float x; float y; float z; };
        struct { float r; float g; float b; };
    };

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    static const SIMDVector3f &Zero, &One, &NegativeOne;
    static const SIMDVector3f &Infinite, &NegativeInfinite;
    static const SIMDVector3f &UnitX, &UnitY, &UnitZ, &UnitW, &NegativeUnitX, &NegativeUnitY, &NegativeUnitZ, &NegativeUnitW;
    static const SIMDVector3f &Epsilon;
};

//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

ALIGN_DECL(Y_SSE_ALIGNMENT) struct SIMDVector4f
{
    // constructors
    inline SIMDVector4f() {}
    inline SIMDVector4f(const float x_, const float y_, const float z_, const float w_) { m128 = _mm_set_ps(w_, z_, y_, x_); }
    inline SIMDVector4f(const float *p) { m128 = _mm_loadu_ps(p); }
    inline SIMDVector4f(const SIMDVector2f &v, const float z_ = 0.0f, const float w_ = 0.0f) { m128 = _mm_set_ps(w_, z_, v.y, v.x); }
    inline SIMDVector4f(const SIMDVector3f &v, const float w_ = 0.0f) { m128 = _mm_set_ps(w_, v.z, v.y, v.x); }
    inline SIMDVector4f(const SIMDVector4f &v) : m128(v.m128) {}
    inline SIMDVector4f(const Vector4i &v) { m128 = _mm_set_ps((float)v.w, (float)v.z, (float)v.y, (float)v.x); }
    SIMDVector4f(const Vector4f &v);

    // new/delete, we overload these so that vectors allocated on the heap are guaranteed to be aligned correctly
    void *operator new[](size_t c) { return Y_aligned_malloc(c, Y_SSE_ALIGNMENT); }
    void *operator new(size_t c) { return Y_aligned_malloc(c, Y_SSE_ALIGNMENT); }
    void operator delete[](void *pMemory) { return Y_aligned_free(pMemory); }
    void operator delete(void *pMemory) { return Y_aligned_free(pMemory); }

    // setters
    void Set(const float x_, const float y_, const float z_, const float w_) { m128 = _mm_set_ps(w_, z_, y_, x_); }
    void Set(const SIMDVector2f &v, const float z_ = 0.0f, const float w_ = 0.0f) { m128 = _mm_set_ps(w_, z_, v.y, v.x); }
    void Set(const SIMDVector3f &v, const float w_ = 0.0f) { m128 = _mm_set_ps(w_, v.z, v.y, v.x); }
    void Set(const SIMDVector4f &v) { m128 = v.m128; }
    void Set(const Vector4f &v);
    void SetZero() { m128 = _mm_setzero_ps(); }

    // load/store
    void Load(const float *v) { m128 = _mm_loadu_ps(v); }
    void Store(float *v) const { _mm_storeu_ps(v, m128); }

    // new vector
    inline SIMDVector4f operator+(const SIMDVector4f &v) const { SIMDVector4f r; r.m128 = _mm_add_ps(m128, v.m128); return r; }
    inline SIMDVector4f operator-(const SIMDVector4f &v) const { SIMDVector4f r; r.m128 = _mm_sub_ps(m128, v.m128); return r; }
    inline SIMDVector4f operator*(const SIMDVector4f &v) const { SIMDVector4f r; r.m128 = _mm_mul_ps(m128, v.m128); return r; }
    inline SIMDVector4f operator/(const SIMDVector4f &v) const { SIMDVector4f r; r.m128 = _mm_div_ps(m128, v.m128); return r; }
	inline SIMDVector4f operator%(const SIMDVector4f &v) const { return SIMDVector4f(Y_fmodf(x, v.x), Y_fmodf(y, v.y), Y_fmodf(z, v.z), Y_fmodf(w, v.w)); }
    inline SIMDVector4f operator-() const { SIMDVector4f r; r.m128 = _mm_sub_ps(_mm_setzero_ps(), m128); return r; }

    // scalar operators
    inline SIMDVector4f operator+(const float v) const { SIMDVector4f r; r.m128 = _mm_add_ps(m128, _mm_set_ps1(v)); return r; }
    inline SIMDVector4f operator-(const float v) const { SIMDVector4f r; r.m128 = _mm_sub_ps(m128, _mm_set_ps1(v)); return r; }
    inline SIMDVector4f operator*(const float v) const { SIMDVector4f r; r.m128 = _mm_mul_ps(m128, _mm_set_ps1(v)); return r; }
    inline SIMDVector4f operator/(const float v) const { SIMDVector4f r; r.m128 = _mm_div_ps(m128, _mm_set_ps1(v)); return r; }
	inline SIMDVector4f operator%(const float v) const { return SIMDVector4f(Y_fmodf(x, v), Y_fmodf(y, v), Y_fmodf(z, v), Y_fmodf(w, v)); }

    // comparison operators
    inline bool operator==(const SIMDVector4f &v) const { return _mm_movemask_ps(_mm_cmpeq_ps(m128, v.m128)) == 0xF; }
    inline bool operator!=(const SIMDVector4f &v) const { return _mm_movemask_ps(_mm_cmpneq_ps(m128, v.m128)) != 0x0; }
    inline bool operator<=(const SIMDVector4f &v) const { return _mm_movemask_ps(_mm_cmple_ps(m128, v.m128)) == 0xF; }
    inline bool operator>=(const SIMDVector4f &v) const { return _mm_movemask_ps(_mm_cmpge_ps(m128, v.m128)) == 0xF; }
    inline bool operator<(const SIMDVector4f &v) const { return _mm_movemask_ps(_mm_cmplt_ps(m128, v.m128)) == 0xF; }
    inline bool operator>(const SIMDVector4f &v) const { return _mm_movemask_ps(_mm_cmpgt_ps(m128, v.m128)) == 0xF; }

    // modifies this vector
    inline SIMDVector4f &operator+=(const SIMDVector4f &v) { m128 = _mm_add_ps(m128, v.m128); return *this; }
    inline SIMDVector4f &operator-=(const SIMDVector4f &v) { m128 = _mm_sub_ps(m128, v.m128); return *this; }
    inline SIMDVector4f &operator*=(const SIMDVector4f &v) { m128 = _mm_mul_ps(m128, v.m128); return *this; }
    inline SIMDVector4f &operator/=(const SIMDVector4f &v) { m128 = _mm_div_ps(m128, v.m128); return *this; }
	inline SIMDVector4f &operator%=(const SIMDVector4f &v) { x = Y_fmodf(x, v.x); y = Y_fmodf(y, v.y); z = Y_fmodf(z, v.z); w = Y_fmodf(w, v.w); return *this; }
    inline SIMDVector4f &operator=(const SIMDVector4f &v) { m128 = v.m128; return *this; }
    SIMDVector4f &operator=(const Vector4f &v);

    // scalar operators
    inline SIMDVector4f &operator+=(const float v) { m128 = _mm_add_ps(m128, _mm_set_ps1(v)); return *this; }
    inline SIMDVector4f &operator-=(const float v) { m128 = _mm_sub_ps(m128, _mm_set_ps1(v)); return *this; }
    inline SIMDVector4f &operator*=(const float v) { m128 = _mm_mul_ps(m128, _mm_set_ps1(v)); return *this; }
    inline SIMDVector4f &operator/=(const float v) { m128 = _mm_div_ps(m128, _mm_set_ps1(v)); return *this; }
	inline SIMDVector4f &operator%=(const float v) { x = Y_fmodf(x, v); y = Y_fmodf(y, v); z = Y_fmodf(z, v); w = Y_fmodf(w, v); return *this; }

    // index accessors
    //const float &operator[](uint32 i) const { return (&x)[i]; }
    //float &operator[](uint32 i) { return (&x)[i]; }
    operator const float *() const { return &x; }
    operator float *() { return &x; }

    // to floatx
    const Vector4f &GetFloat4() const { return reinterpret_cast<const Vector4f &>(*this); }
    Vector4f &GetFloat4() { return reinterpret_cast<Vector4f &>(*this); }
    operator const Vector4f &() const { return reinterpret_cast<const Vector4f &>(*this); }
    operator Vector4f &() { return reinterpret_cast<Vector4f &>(*this); }

    // partial comparisons
    bool AnyLess(const SIMDVector4f &v) const { return _mm_movemask_ps(_mm_cmplt_ps(m128, v.m128)) != 0x00; }
    bool AnyLessEqual(const SIMDVector4f &v) const { return _mm_movemask_ps(_mm_cmple_ps(m128, v.m128)) != 0x00; }
    bool AnyGreater(const SIMDVector4f &v) const { return _mm_movemask_ps(_mm_cmpgt_ps(m128, v.m128)) != 0x00; }
    bool AnyGreaterEqual(const SIMDVector4f &v) const { return _mm_movemask_ps(_mm_cmpge_ps(m128, v.m128)) != 0x00; }
    bool NearEqual(const SIMDVector4f &v) const { return (*this - v).Abs() < Epsilon; }
    bool NearEqual(const SIMDVector4f &v, const float &fEpsilon) const { SIMDVector4f tmp = (*this - v).Abs(); return (tmp.x <= fEpsilon && tmp.y <= fEpsilon && tmp.z <= fEpsilon && tmp.w <= fEpsilon); }
    bool IsFinite() const { return (*this != Infinite); }

    // clamps
    SIMDVector4f Min(const SIMDVector4f &v) const { SIMDVector4f r; r.m128 = _mm_min_ps(m128, v.m128); return r; }
    SIMDVector4f Max(const SIMDVector4f &v) const { SIMDVector4f r; r.m128 = _mm_max_ps(m128, v.m128); return r; }
    SIMDVector4f Clamp(const SIMDVector4f &lBounds, const SIMDVector4f &uBounds) const { SIMDVector4f r; r.m128 = _mm_max_ps(lBounds.m128, _mm_min_ps(uBounds.m128, m128)); return r; }
    SIMDVector4f Abs() const { SIMDVector4f r; r.m128 = _mm_max_ps(m128, _mm_sub_ps(_mm_setzero_ps(), m128)); return r; }
    SIMDVector4f Saturate() const { static const __m128 ones = _mm_set_ps1(1.0f); SIMDVector4f r; r.m128 = _mm_max_ps(ones, _mm_min_ps(_mm_setzero_ps(), m128)); return r; }

    // swap
    void Swap(SIMDVector4f &v) { __m128 temp = m128; m128 = v.m128; v.m128 = temp; }

    // internal dot product, uses dp on sse4, hadd on sse3, shuffle+add on sse
#if Y_CPU_SSE_LEVEL >= 4
    inline __m128 __Dot(const SIMDVector4f &v) const
    { 
        return _mm_dp_ps(m128, v.m128, 0xF1);
    }
#elif Y_CPU_SSE_LEVEL >= 3
    __m128 __Dot(const SIMDVector4f &v) const
    {
        __m128 tmp = _mm_mul_ps(m128, v.m128);  // w1*w2, z1*z2, y1*y2, x1*x2
        tmp = _mm_hadd_ps(tmp, tmp);            // w1*w2+z1*z2, y1*y2+x1*x2, w1*w2+z1*z2, y1*y2+x1*x2
        tmp = _mm_hadd_ps(tmp, tmp);            // w1*w2+z1*z2+y1*y2+x1*x2, w1*w2+z1*z2+y1*y2+x1*x2, w1*w2+z1*z2+y1*y2+x1*x2, w1*w2+z1*z2+y1*y2+x1*x2
        return tmp;
    }
#else
    __m128 __Dot(const SIMDVector4f &v) const
    {
        // wastes registers, can be optimized still, least the 3 shuffles can be executed in parallel
        //__m128 m, tmp1, tmp2, tmp3;
        //m = _mm_mul_ps(m128, v.m128);
        //tmp1 = _mm_shuffle_ps(m, m, _MM_SHUFFLE(1, 1, 1, 1));
        //tmp2 = _mm_shuffle_ps(m, m, _MM_SHUFFLE(2, 2, 2, 2));
        //tmp3 = _mm_shuffle_ps(m, m, _MM_SHUFFLE(3, 3, 3, 3));
        //return _mm_add_ss(m, _mm_add_ss(tmp1, _mm_add_ss(tmp2, tmp3)));
        __m128 m, tmp1;
        m = _mm_mul_ps(m128, v.m128);
        tmp1 = _mm_movehl_ps(m, m);
        tmp1 = _mm_add_ps(tmp1, m);
        m = _mm_shuffle_ps(m, tmp1, _MM_SHUFFLE(0, 0, 0, 1));
        return _mm_add_ss(m, tmp1);
    }
#endif

    // actual dot product
    float Dot(const SIMDVector4f &v) const { return _mm_cvtss_f32(__Dot(v)); }

    // lerp
    SIMDVector4f Lerp(const SIMDVector4f &v, const float f) const
    {
        SIMDVector4f r;
        r.m128 = _mm_add_ps(m128, _mm_mul_ps(_mm_sub_ps(v.m128, m128), _mm_set_ps1(f)));
        return r;
    }

    // length
    inline float SquaredLength() const { return _mm_cvtss_f32(__Dot(*this)); }
    float Length() const { return _mm_cvtss_f32(_mm_sqrt_ss(__Dot(*this))); }
    
    // normalize
    SIMDVector4f Normalize() const
    {
        SIMDVector4f r;
        __m128 length = _mm_sqrt_ss(__Dot(*this));
        r.m128 = _mm_div_ps(m128, _mm_shuffle_ps(length, length, _MM_SHUFFLE(0, 0, 0, 0)));
        return r;
    }

    // fast normalize
    SIMDVector4f NormalizeEst() const
    {
        SIMDVector4f r;
        __m128 invLength = _mm_rsqrt_ss(__Dot(*this));
        r.m128 = _mm_mul_ps(m128, _mm_shuffle_ps(invLength, invLength, _MM_SHUFFLE(0, 0, 0, 0)));
        return r;
    }

    // in-place normalize
    void NormalizeInPlace()
    {
        __m128 length = _mm_sqrt_ss(__Dot(*this));
        m128 = _mm_div_ps(m128, _mm_shuffle_ps(length, length, _MM_SHUFFLE(0, 0, 0, 0)));
    }

    // fast normalize
    void NormalizeEstInPlace()
    {
        __m128 invLength = _mm_rsqrt_ss(__Dot(*this));
        m128 = _mm_mul_ps(m128, _mm_shuffle_ps(invLength, invLength, _MM_SHUFFLE(0, 0, 0, 0)));
    }

    // safe normalize
    SIMDVector4f SafeNormalize() const
    {
        SIMDVector4f r;
        __m128 len = __Dot(*this);
        if ((_mm_movemask_ps(_mm_cmpneq_ss(len, Zero.m128)) & 0x1))
        {
            len = _mm_sqrt_ss(len);
            r.m128 = _mm_div_ps(m128, _mm_shuffle_ps(len, len, _MM_SHUFFLE(0, 0, 0, 0)));
        }
        else
        {
            r.m128 = m128;
        }
        return r;
    }
    SIMDVector4f SafeNormalizeEst() const
    {
        SIMDVector4f r;
        __m128 len = __Dot(*this);
        if ((_mm_movemask_ps(_mm_cmpneq_ss(len, Zero.m128)) & 0x1))
        {
            len = _mm_rsqrt_ss(len);
            r.m128 = _mm_mul_ps(m128, _mm_shuffle_ps(len, len, _MM_SHUFFLE(0, 0, 0, 0)));
        }
        else
        {
            r.m128 = m128;
        }
        return r;
    }
    void SafeNormalizeInPlace()
    {
        __m128 len = __Dot(*this);
        if ((_mm_movemask_ps(_mm_cmpneq_ss(len, Zero.m128)) & 0x1))
        {
            len = _mm_sqrt_ss(len);
            m128 = _mm_div_ps(m128, _mm_shuffle_ps(len, len, _MM_SHUFFLE(0, 0, 0, 0)));
        }
    }
    void SafeNormalizeEstInPlace()
    {
        __m128 len = __Dot(*this);
        if ((_mm_movemask_ps(_mm_cmpneq_ss(len, Zero.m128)) & 0x1))
        {
            len = _mm_rsqrt_ss(len);
            m128 = _mm_mul_ps(m128, _mm_shuffle_ps(len, len, _MM_SHUFFLE(0, 0, 0, 0)));
        }
    }

    // reciprocal
    SIMDVector4f Reciprocal() const
    {
        SIMDVector4f r;
        r.m128 = _mm_div_ps(One.m128, m128);
        return r;
    }

    // not sse yet
    SIMDVector4f Cross(const SIMDVector4f &v1, const SIMDVector4f &v2) const
    {
        SIMDVector4f r;
        r.x = (((v1.z * v2.w) - (v1.w * v2.z)) * y) - (((v1.y * v2.w) - (v1.w * v2.y)) * z) + (((v1.y * v2.z) - (v1.z * v2.y)) * w);
        r.y = (((v1.w * v2.z) - (v1.z * v2.w)) * x) - (((v1.w * v2.x) - (v1.x * v2.w)) * z) + (((v1.z * v2.x) - (v1.x * v2.z)) * w);
        r.z = (((v1.y * v2.w) - (v1.w * v2.y)) * x) - (((v1.x * v2.w) - (v1.w * v2.x)) * y) + (((v1.x * v2.y) - (v1.y * v2.x)) * w);
        r.w = (((v1.z * v2.y) - (v1.y * v2.z)) * x) - (((v1.z * v2.x) - (v1.x * v2.z)) * y) + (((v1.y * v2.x) - (v1.x * v2.y)) * z);
        return r;
    }

    // shuffles - todo fix return types on these
    template<int V0, int V1> SIMDVector2f Shuffle2() const { SIMDVector2f r; r.m128 = _mm_shuffle_ps(m128, m128, _MM_SHUFFLE(0, 0, V1, V0)); return r; }
    template<int V0, int V1, int V2> SIMDVector3f Shuffle3() const { SIMDVector3f r; r.m128 = _mm_shuffle_ps(m128, m128, _MM_SHUFFLE(0, V2, V1, V0)); return r; }
    template<int V0, int V1, int V2, int V3> SIMDVector4f Shuffle4() const { SIMDVector4f r; r.m128 = _mm_shuffle_ps(m128, m128, _MM_SHUFFLE(V3, V2, V1, V0)); return r; }
    VECTOR4_SHUFFLE_FUNCTIONS(SIMDVector2f, SIMDVector3f, SIMDVector4f);

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    union
    {
        __m128 m128;
        struct { float x; float y; float z; float w; };
        struct { float r; float g; float b; float a; };
    };

    //----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    static const SIMDVector4f &Zero, &One, &NegativeOne;
    static const SIMDVector4f &Infinite, &NegativeInfinite;
    static const SIMDVector4f &UnitX, &UnitY, &UnitZ, &UnitW, &NegativeUnitX, &NegativeUnitY, &NegativeUnitZ, &NegativeUnitW;
    static const SIMDVector4f &Epsilon;
};

