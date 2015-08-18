#include "MathLib/SIMDMatrixf_scalar.h"

const SIMDMatrix3x3f &SIMDMatrix3x3f::Zero = reinterpret_cast<const SIMDMatrix3x3f &>(Matrix3x3f::Zero);
const SIMDMatrix3x3f &SIMDMatrix3x3f::Identity = reinterpret_cast<const SIMDMatrix3x3f &>(Matrix3x3f::Identity);
const SIMDMatrix4x4f &SIMDMatrix4x4f::Zero = reinterpret_cast<const SIMDMatrix4x4f &>(Matrix4x4f::Zero);
const SIMDMatrix4x4f &SIMDMatrix4x4f::Identity = reinterpret_cast<const SIMDMatrix4x4f &>(Matrix4x4f::Identity);
