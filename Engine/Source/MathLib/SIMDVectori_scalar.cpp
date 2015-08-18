#include "MathLib/SIMDVectori.h"

#if Y_CPU_SSE_LEVEL == 0

const SIMDVector2i &SIMDVector2i::Zero = reinterpret_cast<const SIMDVector2i &>(Vector2i::Zero);
const SIMDVector2i &SIMDVector2i::One = reinterpret_cast<const SIMDVector2i &>(Vector2i::One);
const SIMDVector2i &SIMDVector2i::NegativeOne = reinterpret_cast<const SIMDVector2i &>(Vector2i::NegativeOne);

const SIMDVector3i &SIMDVector3i::Zero = reinterpret_cast<const SIMDVector3i &>(Vector3i::Zero);
const SIMDVector3i &SIMDVector3i::One = reinterpret_cast<const SIMDVector3i &>(Vector3i::One);
const SIMDVector3i &SIMDVector3i::NegativeOne = reinterpret_cast<const SIMDVector3i &>(Vector3i::NegativeOne);

const SIMDVector4i &SIMDVector4i::Zero = reinterpret_cast<const SIMDVector4i &>(Vector4i::Zero);
const SIMDVector4i &SIMDVector4i::One = reinterpret_cast<const SIMDVector4i &>(Vector4i::One);
const SIMDVector4i &SIMDVector4i::NegativeOne = reinterpret_cast<const SIMDVector4i &>(Vector4i::NegativeOne);

#endif
