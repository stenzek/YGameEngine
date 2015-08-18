#include "MathLib/Vectorh.h"
#include "MathLib/Vectorf.h"

Vector2h::Vector2h(const Vector2f &v) : x((v.x)), y((v.y)) {}
Vector2h::operator Vector2f() const { return Vector2f((x), (y)); }

Vector3h::Vector3h(const Vector3f &v) : x((v.x)), y((v.y)), z((v.z)) {}
Vector3h::operator Vector3f() const { return Vector3f((x), (y), (z)); }

Vector4h::Vector4h(const Vector4f &v) : x((v.x)), y((v.y)), z((v.z)), w((v.w)) {}
Vector4h::operator Vector4f() const { return Vector4f((x), (y), (z), (w)); }
