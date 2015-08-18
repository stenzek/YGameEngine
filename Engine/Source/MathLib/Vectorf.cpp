#include "MathLib/Vectorf.h"
#include "MathLib/Vectorh.h"
#include "MathLib/Vectori.h"
#include "MathLib/SIMDVectorf.h"
#include "MathLib/SIMDVectori.h"

Vector2f::Vector2f(const SIMDVector2f &v) { Set(v.x, v.y); }
Vector2f::Vector2f(const Vector2i &v) : x((float)v.x), y((float)v.y) {}
void Vector2f::Set(const SIMDVector2f &v) { Set(v.x, v.y); }
Vector2f &Vector2f::operator=(const SIMDVector2f &v) { Set(v.x, v.y); return *this; }

Vector3f::Vector3f(const SIMDVector3f &v) { Set(v.x, v.y, v.z); }
Vector3f::Vector3f(const Vector3i &v) : x((float)v.x), y((float)v.y), z((float)v.z) {}
void Vector3f::Set(const SIMDVector3f &v) { Set(v.x, v.y, v.z); }
Vector3f &Vector3f::operator=(const SIMDVector3f &v) { Set(v.x, v.y, v.z); return *this; }

Vector4f::Vector4f(const SIMDVector4f &v) { Set(v.x, v.y, v.z, v.w); }
Vector4f::Vector4f(const Vector4i &v) : x((float)v.x), y((float)v.y), z((float)v.z), w((float)v.w) {}
void Vector4f::Set(const SIMDVector4f &v) { Set(v.x, v.y, v.z, v.w); }
Vector4f &Vector4f::operator=(const SIMDVector4f &v) { Set(v.x, v.y, v.z, v.w); return *this; }

static const float __floatUnitX[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
static const float __floatUnitY[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
static const float __floatUnitZ[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
static const float __floatUnitW[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const float __floatNegativeUnitX[4] = { -1.0f, 0.0f, 0.0f, 0.0f };
static const float __floatNegativeUnitY[4] = { 0.0f, -1.0f, 0.0f, 0.0f };
static const float __floatNegativeUnitZ[4] = { 0.0f, 0.0f, -1.0f, 0.0f };
static const float __floatNegativeUnitW[4] = { 0.0f, 0.0f, 0.0f, -1.0f };

static const float __float2Zero[2] = { 0.0f, 0.0f };
static const float __float2One[2] = { 1.0f, 1.0f };
static const float __float2NegativeOne[2] = { -1.0f, -1.0f };
static const float __float2Infinite[2] = { Y_FLT_INFINITE, Y_FLT_INFINITE };
static const float __float2NegativeInfinite[2] = { -Y_FLT_INFINITE, -Y_FLT_INFINITE };
const Vector2f &Vector2f::Zero = reinterpret_cast<const Vector2f &>(__float2Zero);
const Vector2f &Vector2f::One = reinterpret_cast<const Vector2f &>(__float2One);
const Vector2f &Vector2f::NegativeOne = reinterpret_cast<const Vector2f &>(__float2NegativeOne);
const Vector2f &Vector2f::Infinite = reinterpret_cast<const Vector2f &>(__float2Infinite);
const Vector2f &Vector2f::NegativeInfinite = reinterpret_cast<const Vector2f &>(__float2NegativeInfinite);
const Vector2f &Vector2f::UnitX = reinterpret_cast<const Vector2f &>(__floatUnitX);
const Vector2f &Vector2f::UnitY = reinterpret_cast<const Vector2f &>(__floatUnitY);
const Vector2f &Vector2f::NegativeUnitX = reinterpret_cast<const Vector2f &>(__floatNegativeUnitX);
const Vector2f &Vector2f::NegativeUnitY = reinterpret_cast<const Vector2f &>(__floatNegativeUnitY);

static const float __float3Zero[3] = { 0.0f, 0.0f, 0.0f };
static const float __float3One[3] = { 1.0f, 1.0f, 1.0f };
static const float __float3NegativeOne[3] = { -1.0f, -1.0f, -1.0f };
static const float __float3Infinite[3] = { Y_FLT_INFINITE, Y_FLT_INFINITE, Y_FLT_INFINITE };
static const float __float3NegativeInfinite[3] = { -Y_FLT_INFINITE, -Y_FLT_INFINITE, -Y_FLT_INFINITE };
const Vector3f &Vector3f::Zero = reinterpret_cast<const Vector3f &>(__float3Zero);
const Vector3f &Vector3f::One = reinterpret_cast<const Vector3f &>(__float3One);
const Vector3f &Vector3f::NegativeOne = reinterpret_cast<const Vector3f &>(__float3NegativeOne);
const Vector3f &Vector3f::Infinite = reinterpret_cast<const Vector3f &>(__float3Infinite);
const Vector3f &Vector3f::NegativeInfinite = reinterpret_cast<const Vector3f &>(__float3NegativeInfinite);
const Vector3f &Vector3f::UnitX = reinterpret_cast<const Vector3f &>(__floatUnitX);
const Vector3f &Vector3f::UnitY = reinterpret_cast<const Vector3f &>(__floatUnitY);
const Vector3f &Vector3f::UnitZ = reinterpret_cast<const Vector3f &>(__floatUnitZ);
const Vector3f &Vector3f::NegativeUnitX = reinterpret_cast<const Vector3f &>(__floatNegativeUnitX);
const Vector3f &Vector3f::NegativeUnitY = reinterpret_cast<const Vector3f &>(__floatNegativeUnitY);
const Vector3f &Vector3f::NegativeUnitZ = reinterpret_cast<const Vector3f &>(__floatNegativeUnitZ);

static const float __float4Zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
static const float __float4One[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const float __float4NegativeOne[4] = { -1.0f, -1.0f, -1.0f, -1.0f };
static const float __float4Infinite[4] = { Y_FLT_INFINITE, Y_FLT_INFINITE, Y_FLT_INFINITE, Y_FLT_INFINITE };
static const float __float4NegativeInfinite[4] = { -Y_FLT_INFINITE, -Y_FLT_INFINITE, -Y_FLT_INFINITE, -Y_FLT_INFINITE };
const Vector4f &Vector4f::Zero = reinterpret_cast<const Vector4f &>(__float4Zero);
const Vector4f &Vector4f::One = reinterpret_cast<const Vector4f &>(__float4One);
const Vector4f &Vector4f::NegativeOne = reinterpret_cast<const Vector4f &>(__float4NegativeOne);
const Vector4f &Vector4f::Infinite = reinterpret_cast<const Vector4f &>(__float4Infinite);
const Vector4f &Vector4f::NegativeInfinite = reinterpret_cast<const Vector4f &>(__float4NegativeInfinite);
const Vector4f &Vector4f::UnitX = reinterpret_cast<const Vector4f &>(__floatUnitX);
const Vector4f &Vector4f::UnitY = reinterpret_cast<const Vector4f &>(__floatUnitY);
const Vector4f &Vector4f::UnitZ = reinterpret_cast<const Vector4f &>(__floatUnitZ);
const Vector4f &Vector4f::UnitW = reinterpret_cast<const Vector4f &>(__floatUnitW);
const Vector4f &Vector4f::NegativeUnitX = reinterpret_cast<const Vector4f &>(__floatNegativeUnitX);
const Vector4f &Vector4f::NegativeUnitY = reinterpret_cast<const Vector4f &>(__floatNegativeUnitY);
const Vector4f &Vector4f::NegativeUnitZ = reinterpret_cast<const Vector4f &>(__floatNegativeUnitZ);
const Vector4f &Vector4f::NegativeUnitW = reinterpret_cast<const Vector4f &>(__floatNegativeUnitW);
