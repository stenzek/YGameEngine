#include "MathLib/Vectoru.h"

static const uint32 __uint2Zero[2] = { 0, 0 };
static const uint32 __uint2One[2] = { 1, 1 };
const Vector2u &Vector2u::Zero = reinterpret_cast<const Vector2u &>(__uint2Zero);
const Vector2u &Vector2u::One = reinterpret_cast<const Vector2u &>(__uint2One);

static const uint32 __uint3Zero[3] = { 0, 0, 0 };
static const uint32 __uint3One[3] = { 1, 1, 1 };
const Vector3u &Vector3u::Zero = reinterpret_cast<const Vector3u &>(__uint3Zero);
const Vector3u &Vector3u::One = reinterpret_cast<const Vector3u &>(__uint3One);

static const uint32 __uint4Zero[4] = { 0, 0, 0, 0 };
static const uint32 __uint4One[4] = { 1, 1, 1, 1 };
const Vector4u &Vector4u::Zero = reinterpret_cast<const Vector4u &>(__uint4Zero);
const Vector4u &Vector4u::One = reinterpret_cast<const Vector4u &>(__uint4One);
