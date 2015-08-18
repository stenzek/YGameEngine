#include "MathLib/Common.h"
#include "YBaseLib/Assert.h"

#if 0

struct int2x2;
struct int3x3;
struct int3x4;
struct int4x4;

struct SIMDMatrix2i
{
    SIMDMatrix2i() {}

    // set everything at once
    SIMDMatrix2i(const int32 E00, const int32 E01,
             const int32 E10, const int32 E11);

    // sets the diagonal to the specified value (1 == identity)
    SIMDMatrix2i(const int32 Diagonal);

    // assumes row-major packing order with vectors in columns
    SIMDMatrix2i(const int32 *p);

    // copy
    SIMDMatrix2i(const SIMDMatrix2i &v);

    // downscaling/from floatx
    SIMDMatrix2i(const int2x2 &v);
   
    // setters
    void Set(const int32 E00, const int32 E01,
             const int32 E10, const int32 E11);
    void SetIdentity();
    void SetZero();

    // column accessors
    Vector2i GetColumn(uint32 j) const;
    void SetColumn(uint32 j, const Vector2i &v);

    // for converting to/from opengl matrices
    void SetTranspose(const int32 *pElements);
    void GetTranspose(int32 *pElements) const;

    // new matrix
    SIMDMatrix2i Transpose() const;

    // in-place
    void TransposeInPlace();

    // to intx
    const int2x2 &GetInt2x2() const { return reinterpret_cast<const int2x2 &>(*this); }
    int2x2 &GetInt2x2() { return reinterpret_cast<int2x2 &>(*this); }
    const int2 &GetRowInt2(uint32 i) const { return reinterpret_cast<const int2 &>(Row[i]); }
    int2 GetColumnInt2(uint32 i) const;
    operator const int2x2 &() const { return reinterpret_cast<const int2x2 &>(*this); }
    operator int2x2 &() { return reinterpret_cast<int2x2 &>(*this); }

    // row/field accessors
    const Vector2i &operator[](uint32 i) const { DebugAssert(i < 4); return reinterpret_cast<const Vector2i &>(Row[i]); }
    const Vector2i &GetRow(uint32 i) const { DebugAssert(i < 4); return reinterpret_cast<const Vector2i &>(Row[i]); }
    const int32 &operator()(uint32 i, uint32 j) const { DebugAssert(i < 4 && j < 4); return Row[i][j]; }
    Vector2i &operator[](uint32 i) { DebugAssert(i < 4); return reinterpret_cast<Vector2i &>(Row[i]); }
    void SetRow(uint32 i, const Vector2i &v) { DebugAssert(i < 4); reinterpret_cast<Vector2i &>(Row[i]) = v; }
    int32 &operator()(uint32 i, uint32 j) { DebugAssert(i < 4 && j < 4); return Row[i][j]; }

    // assignment operators
    SIMDMatrix2i &operator=(const SIMDMatrix2i &v);
    SIMDMatrix2i &operator=(const int32 Diagonal);
    SIMDMatrix2i &operator=(const int32 *p);
    SIMDMatrix2i &operator=(const int2x2 &v);

    // various operators
    SIMDMatrix2i &operator*=(const SIMDMatrix2i &v);
    SIMDMatrix2i &operator*=(const int32 v);
    SIMDMatrix2i operator*(const SIMDMatrix2i &v) const;
    SIMDMatrix2i operator*(int32 v) const;
    SIMDMatrix2i operator-() const;

    // matrix * vector
    Vector2i operator*(const Vector2i &v) const;

    // constants
    static const SIMDMatrix2i &Zero, &Identity;

public:
    union
    {
        int32 Elements[4];
        int32 Row[2][2];
        struct
        {
            int32 m00, m01;
            int32 m10, m11;
        };
    };
};

#endif
