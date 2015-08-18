#include "MathLib/SIMDVectori.h"
#include "MathLib/Vectorf.h"
#include "MathLib/Vectori.h"
#include "MathLib/Vectoru.h"

#if Y_CPU_SSE_LEVEL >= 2

// should be the same for all vector implementations
SIMDVector2i::SIMDVector2i(const Vector2i &v) { Set(v.x, v.y); }
SIMDVector3i::SIMDVector3i(const Vector3i &v) { Set(v.x, v.y, v.z); }
SIMDVector4i::SIMDVector4i(const Vector4i &v) { Set(v.x, v.y, v.z, v.w); }
void SIMDVector2i::Set(const Vector2i &v) { Set(v.x, v.y); }
void SIMDVector3i::Set(const Vector3i &v) { Set(v.x, v.y, v.z); }
void SIMDVector4i::Set(const Vector4i &v) { Set(v.x, v.y, v.z, v.w); }
SIMDVector2i &SIMDVector2i::operator=(const Vector2i &v) { Set(v.x, v.y); return *this; }
SIMDVector3i &SIMDVector3i::operator=(const Vector3i &v) { Set(v.x, v.y, v.z); return *this; }
SIMDVector4i &SIMDVector4i::operator=(const Vector4i &v) { Set(v.x, v.y, v.z, v.w); return *this; }

ALIGN_DECL(Y_SSE_ALIGNMENT) static const int32 Vector2iZero[4] = { 0, 0, 0, 0 };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const int32 Vector2iOne[4] = { 1, 1, 0, 0 };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const int32 Vector2iNegativeOne[4] = { -1, -1, 0, 0 };
const SIMDSIMDVector2i &Vector2i::Zero = reinterpret_cast<const SIMDVector2i &>(Vector2iZero);
const SIMDSIMDVector2i &Vector2i::One = reinterpret_cast<const SIMDVector2i &>(Vector2iOne);
const SIMDSIMDVector2i &Vector2i::NegativeOne = reinterpret_cast<const SIMDVector2i &>(Vector2iNegativeOne);

ALIGN_DECL(Y_SSE_ALIGNMENT) static const int32 Vector3iZero[4] = { 0, 0, 0, 0 };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const int32 Vector3iOne[4] = { 1, 1, 1, 0 };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const int32 Vector3iNegativeOne[4] = { -1, -1, -1, 0 };
const SIMDVector3i &SIMDVector3i::Zero = reinterpret_cast<const SIMDVector3i &>(Vector3iZero);
const SIMDVector3i &SIMDVector3i::One = reinterpret_cast<const SIMDVector3i &>(Vector3iOne);
const SIMDVector3i &SIMDVector3i::NegativeOne = reinterpret_cast<const SIMDVector3i &>(Vector3iNegativeOne);

ALIGN_DECL(Y_SSE_ALIGNMENT) static const int32 Vector4iZero[4] = { 0, 0, 0, 0 };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const int32 Vector4iOne[4] = { 1, 1, 1, 1 };
ALIGN_DECL(Y_SSE_ALIGNMENT) static const int32 Vector4iNegativeOne[4] = { -1, -1, -1, -1 };
const SIMDVector4i &SIMDVector4i::Zero = reinterpret_cast<const SIMDVector4i &>(Vector4iZero);
const SIMDVector4i &SIMDVector4i::One = reinterpret_cast<const SIMDVector4i &>(Vector4iOne);
const SIMDVector4i &SIMDVector4i::NegativeOne = reinterpret_cast<const SIMDVector4i &>(Vector4iNegativeOne);

#endif
