#include "MathLib/SIMDVectorf.h"
#include "MathLib/Vectorf.h"
#include "MathLib/Vectori.h"
#include "MathLib/Vectoru.h"

#if Y_CPU_SSE_LEVEL >= 1

// should be the same for all vector implementations
SIMDVector2f::SIMDVector2f(const Vector2f &v) { Set(v.x, v.y); }
SIMDVector3f::SIMDVector3f(const Vector3f &v) { Set(v.x, v.y, v.z); }
SIMDVector4f::SIMDVector4f(const Vector4f &v) { Set(v.x, v.y, v.z, v.w); }
void SIMDVector2f::Set(const Vector2f &v) { Set(v.x, v.y); }
void SIMDVector3f::Set(const Vector3f &v) { Set(v.x, v.y, v.z); }
void SIMDVector4f::Set(const Vector4f &v) { Set(v.x, v.y, v.z, v.w); }
SIMDVector2f &SIMDVector2f::operator=(const Vector2f &v) { Set(v.x, v.y); return *this; }
SIMDVector3f &SIMDVector3f::operator=(const Vector3f &v) { Set(v.x, v.y, v.z); return *this; }
SIMDVector4f &SIMDVector4f::operator=(const Vector4f &v) { Set(v.x, v.y, v.z, v.w); return *this; }

ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __VectorUnitX[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __VectorUnitY[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __VectorUnitZ[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __VectorUnitW[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __VectorNegativeUnitX[4] = { -1.0f, 0.0f, 0.0f, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __VectorNegativeUnitY[4] = { 0.0f, -1.0f, 0.0f, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __VectorNegativeUnitZ[4] = { 0.0f, 0.0f, -1.0f, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __VectorNegativeUnitW[4] = { 0.0f, 0.0f, 0.0f, -1.0f };

ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector2fZero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector2fOne[4] = { 1.0f, 1.0f, 0.0f, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector2fNegativeOne[4] = { -1.0f, -1.0f, 0.0f, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector2fInfinite[4] = { Y_FLT_INFINITE, Y_FLT_INFINITE, 0.0f, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector2fNegativeInfinite[4] = { -Y_FLT_INFINITE, -Y_FLT_INFINITE, 0.0f, 0.0f };
const SIMDVector2f &SIMDVector2f::Zero = reinterpret_cast<const SIMDVector2f &>(__Vector2fZero);
const SIMDVector2f &SIMDVector2f::One = reinterpret_cast<const SIMDVector2f &>(__Vector2fOne);
const SIMDVector2f &SIMDVector2f::NegativeOne = reinterpret_cast<const SIMDVector2f &>(__Vector2fNegativeOne);
const SIMDVector2f &SIMDVector2f::Infinite = reinterpret_cast<const SIMDVector2f &>(__Vector2fInfinite);
const SIMDVector2f &SIMDVector2f::NegativeInfinite = reinterpret_cast<const SIMDVector2f &>(__Vector2fNegativeInfinite);
const SIMDVector2f &SIMDVector2f::UnitX = reinterpret_cast<const SIMDVector2f &>(__VectorUnitX);
const SIMDVector2f &SIMDVector2f::UnitY = reinterpret_cast<const SIMDVector2f &>(__VectorUnitY);
const SIMDVector2f &SIMDVector2f::NegativeUnitX = reinterpret_cast<const SIMDVector2f &>(__VectorNegativeUnitX);
const SIMDVector2f &SIMDVector2f::NegativeUnitY = reinterpret_cast<const SIMDVector2f &>(__VectorNegativeUnitY);

ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector3fZero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector3fOne[4] = { 1.0f, 1.0f, 1.0f, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector3fNegativeOne[4] = { -1.0f, -1.0f, -1.0f, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector3fInfinite[4] = { Y_FLT_INFINITE, Y_FLT_INFINITE, Y_FLT_INFINITE, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector3fNegativeInfinite[4] = { -Y_FLT_INFINITE, -Y_FLT_INFINITE, -Y_FLT_INFINITE, 0.0f };
const SIMDVector3f &SIMDVector3f::Zero = reinterpret_cast<const SIMDVector3f &>(__Vector3fZero);
const SIMDVector3f &SIMDVector3f::One = reinterpret_cast<const SIMDVector3f &>(__Vector3fOne);
const SIMDVector3f &SIMDVector3f::NegativeOne = reinterpret_cast<const SIMDVector3f &>(__Vector3fNegativeOne);
const SIMDVector3f &SIMDVector3f::Infinite = reinterpret_cast<const SIMDVector3f &>(__Vector3fInfinite);
const SIMDVector3f &SIMDVector3f::NegativeInfinite = reinterpret_cast<const SIMDVector3f &>(__Vector3fNegativeInfinite);
const SIMDVector3f &SIMDVector3f::UnitX = reinterpret_cast<const SIMDVector3f &>(__VectorUnitX);
const SIMDVector3f &SIMDVector3f::UnitY = reinterpret_cast<const SIMDVector3f &>(__VectorUnitY);
const SIMDVector3f &SIMDVector3f::UnitZ = reinterpret_cast<const SIMDVector3f &>(__VectorUnitZ);
const SIMDVector3f &SIMDVector3f::NegativeUnitX = reinterpret_cast<const SIMDVector3f &>(__VectorNegativeUnitX);
const SIMDVector3f &SIMDVector3f::NegativeUnitY = reinterpret_cast<const SIMDVector3f &>(__VectorNegativeUnitY);
const SIMDVector3f &SIMDVector3f::NegativeUnitZ = reinterpret_cast<const SIMDVector3f &>(__VectorNegativeUnitZ);

ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector4fZero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector4fOne[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector4fNegativeOne[4] = { -1.0f, -1.0f, -1.0f, -1.0f };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector4fInfinite[4] = { Y_FLT_INFINITE, Y_FLT_INFINITE, Y_FLT_INFINITE, Y_FLT_INFINITE };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const float __Vector4fNegativeInfinite[4] = { -Y_FLT_INFINITE, -Y_FLT_INFINITE, -Y_FLT_INFINITE, -Y_FLT_INFINITE };
const SIMDVector4f &SIMDVector4f::Zero = reinterpret_cast<const SIMDVector4f &>(__Vector4fZero);
const SIMDVector4f &SIMDVector4f::One = reinterpret_cast<const SIMDVector4f &>(__Vector4fOne);
const SIMDVector4f &SIMDVector4f::NegativeOne = reinterpret_cast<const SIMDVector4f &>(__Vector4fNegativeOne);
const SIMDVector4f &SIMDVector4f::Infinite = reinterpret_cast<const SIMDVector4f &>(__Vector4fInfinite);
const SIMDVector4f &SIMDVector4f::NegativeInfinite = reinterpret_cast<const SIMDVector4f &>(__Vector4fNegativeInfinite);
const SIMDVector4f &SIMDVector4f::UnitX = reinterpret_cast<const SIMDVector4f &>(__VectorUnitX);
const SIMDVector4f &SIMDVector4f::UnitY = reinterpret_cast<const SIMDVector4f &>(__VectorUnitY);
const SIMDVector4f &SIMDVector4f::UnitZ = reinterpret_cast<const SIMDVector4f &>(__VectorUnitZ);
const SIMDVector4f &SIMDVector4f::UnitW = reinterpret_cast<const SIMDVector4f &>(__VectorUnitW);
const SIMDVector4f &SIMDVector4f::NegativeUnitX = reinterpret_cast<const SIMDVector4f &>(__VectorNegativeUnitX);
const SIMDVector4f &SIMDVector4f::NegativeUnitY = reinterpret_cast<const SIMDVector4f &>(__VectorNegativeUnitY);
const SIMDVector4f &SIMDVector4f::NegativeUnitZ = reinterpret_cast<const SIMDVector4f &>(__VectorNegativeUnitZ);
const SIMDVector4f &SIMDVector4f::NegativeUnitW = reinterpret_cast<const SIMDVector4f &>(__VectorNegativeUnitW);

#endif
