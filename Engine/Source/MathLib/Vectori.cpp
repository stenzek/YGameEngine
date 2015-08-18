#include "MathLib/Vectori.h"
#include "MathLib/SIMDVectori.h"

Vector2i::Vector2i(const SIMDVector2i &v) { Set(v.x, v.y); }
Vector3i::Vector3i(const SIMDVector3i &v) { Set(v.x, v.y, v.z); }
Vector4i::Vector4i(const SIMDVector4i &v) { Set(v.x, v.y, v.z, v.w); }
void Vector2i::Set(const SIMDVector2i &v) { Set(v.x, v.y); }
void Vector3i::Set(const SIMDVector3i &v) { Set(v.x, v.y, v.z); }
void Vector4i::Set(const SIMDVector4i &v) { Set(v.x, v.y, v.z, v.w); }
Vector2i &Vector2i::operator=(const SIMDVector2i &v) { Set(v.x, v.y); return *this; }
Vector3i &Vector3i::operator=(const SIMDVector3i &v) { Set(v.x, v.y, v.z); return *this; }
Vector4i &Vector4i::operator=(const SIMDVector4i &v) { Set(v.x, v.y, v.z, v.w); return *this; }

static const int32 __int2Zero[2] = { 0, 0 };
static const int32 __int2One[2] = { 1, 1 };
static const int32 __int2NegativeOne[2] = { -1, -1 };
const Vector2i &Vector2i::Zero = reinterpret_cast<const Vector2i &>(__int2Zero);
const Vector2i &Vector2i::One = reinterpret_cast<const Vector2i &>(__int2One);
const Vector2i &Vector2i::NegativeOne = reinterpret_cast<const Vector2i &>(__int2NegativeOne);

static const int32 __int3Zero[3] = { 0, 0, 0 };
static const int32 __int3One[3] = { 1, 1, 1 };
static const int32 __int3NegativeOne[3] = { -1, -1, -1 };
const Vector3i &Vector3i::Zero = reinterpret_cast<const Vector3i &>(__int3Zero);
const Vector3i &Vector3i::One = reinterpret_cast<const Vector3i &>(__int3One);
const Vector3i &Vector3i::NegativeOne = reinterpret_cast<const Vector3i &>(__int3NegativeOne);

static const int32 __int4Zero[4] = { 0, 0, 0, 0 };
static const int32 __int4One[4] = { 1, 1, 1, 1 };
static const int32 __int4NegativeOne[4] = { -1, -1, -1, -1 };
const Vector4i &Vector4i::Zero = reinterpret_cast<const Vector4i &>(__int4Zero);
const Vector4i &Vector4i::One = reinterpret_cast<const Vector4i &>(__int4One);
const Vector4i &Vector4i::NegativeOne = reinterpret_cast<const Vector4i &>(__int4NegativeOne);
