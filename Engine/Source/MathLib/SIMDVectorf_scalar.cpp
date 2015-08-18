#include "MathLib/SIMDVectorf.h"

#if Y_CPU_SSE_LEVEL == 0

const SIMDVector2f &SIMDVector2f::Zero = reinterpret_cast<const SIMDVector2f &>(Vector2f::Zero);
const SIMDVector2f &SIMDVector2f::One = reinterpret_cast<const SIMDVector2f &>(Vector2f::One);
const SIMDVector2f &SIMDVector2f::NegativeOne = reinterpret_cast<const SIMDVector2f &>(Vector2f::NegativeOne);
const SIMDVector2f &SIMDVector2f::Infinite = reinterpret_cast<const SIMDVector2f &>(Vector2f::Infinite);
const SIMDVector2f &SIMDVector2f::NegativeInfinite = reinterpret_cast<const SIMDVector2f &>(Vector2f::NegativeInfinite);
const SIMDVector2f &SIMDVector2f::UnitX = reinterpret_cast<const SIMDVector2f &>(Vector2f::UnitX);
const SIMDVector2f &SIMDVector2f::UnitY = reinterpret_cast<const SIMDVector2f &>(Vector2f::UnitY);
const SIMDVector2f &SIMDVector2f::NegativeUnitX = reinterpret_cast<const SIMDVector2f &>(Vector2f::NegativeUnitX);
const SIMDVector2f &SIMDVector2f::NegativeUnitY = reinterpret_cast<const SIMDVector2f &>(Vector2f::NegativeUnitY);


const SIMDVector3f &SIMDVector3f::Zero = reinterpret_cast<const SIMDVector3f &>(Vector3f::Zero);
const SIMDVector3f &SIMDVector3f::One = reinterpret_cast<const SIMDVector3f &>(Vector3f::One);
const SIMDVector3f &SIMDVector3f::NegativeOne = reinterpret_cast<const SIMDVector3f &>(Vector3f::NegativeOne);
const SIMDVector3f &SIMDVector3f::Infinite = reinterpret_cast<const SIMDVector3f &>(Vector3f::Infinite);
const SIMDVector3f &SIMDVector3f::NegativeInfinite = reinterpret_cast<const SIMDVector3f &>(Vector3f::NegativeInfinite);
const SIMDVector3f &SIMDVector3f::UnitX = reinterpret_cast<const SIMDVector3f &>(Vector3f::UnitX);
const SIMDVector3f &SIMDVector3f::UnitY = reinterpret_cast<const SIMDVector3f &>(Vector3f::UnitY);
const SIMDVector3f &SIMDVector3f::UnitZ = reinterpret_cast<const SIMDVector3f &>(Vector3f::UnitZ);
const SIMDVector3f &SIMDVector3f::NegativeUnitX = reinterpret_cast<const SIMDVector3f &>(Vector3f::NegativeUnitX);
const SIMDVector3f &SIMDVector3f::NegativeUnitY = reinterpret_cast<const SIMDVector3f &>(Vector3f::NegativeUnitY);
const SIMDVector3f &SIMDVector3f::NegativeUnitZ = reinterpret_cast<const SIMDVector3f &>(Vector3f::NegativeUnitZ);

const SIMDVector4f &SIMDVector4f::Zero = reinterpret_cast<const SIMDVector4f &>(Vector4f::Zero);
const SIMDVector4f &SIMDVector4f::One = reinterpret_cast<const SIMDVector4f &>(Vector4f::One);
const SIMDVector4f &SIMDVector4f::NegativeOne = reinterpret_cast<const SIMDVector4f &>(Vector4f::NegativeOne);
const SIMDVector4f &SIMDVector4f::Infinite = reinterpret_cast<const SIMDVector4f &>(Vector4f::Infinite);
const SIMDVector4f &SIMDVector4f::NegativeInfinite = reinterpret_cast<const SIMDVector4f &>(Vector4f::NegativeInfinite);
const SIMDVector4f &SIMDVector4f::UnitX = reinterpret_cast<const SIMDVector4f &>(Vector4f::UnitX);
const SIMDVector4f &SIMDVector4f::UnitY = reinterpret_cast<const SIMDVector4f &>(Vector4f::UnitY);
const SIMDVector4f &SIMDVector4f::UnitZ = reinterpret_cast<const SIMDVector4f &>(Vector4f::UnitZ);
const SIMDVector4f &SIMDVector4f::UnitW = reinterpret_cast<const SIMDVector4f &>(Vector4f::UnitW);
const SIMDVector4f &SIMDVector4f::NegativeUnitX = reinterpret_cast<const SIMDVector4f &>(Vector4f::NegativeUnitX);
const SIMDVector4f &SIMDVector4f::NegativeUnitY = reinterpret_cast<const SIMDVector4f &>(Vector4f::NegativeUnitY);
const SIMDVector4f &SIMDVector4f::NegativeUnitZ = reinterpret_cast<const SIMDVector4f &>(Vector4f::NegativeUnitZ);
const SIMDVector4f &SIMDVector4f::NegativeUnitW = reinterpret_cast<const SIMDVector4f &>(Vector4f::NegativeUnitW);

#endif
