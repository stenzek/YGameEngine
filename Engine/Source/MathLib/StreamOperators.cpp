#include "MathLib/StreamOperators.h"
#include "MathLib/Vectorf.h"
#include "MathLib/Vectori.h"
#include "MathLib/Vectoru.h"
#include "MathLib/Quaternion.h"
#include "MathLib/AABox.h"
#include "MathLib/Sphere.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"

BinaryReader &operator>>(BinaryReader &binaryReader, Vector2f &value)
{
    return binaryReader >> value.x >> value.y;
}

BinaryReader &operator>>(BinaryReader &binaryReader, Vector3f &value)
{
    return binaryReader >> value.x >> value.y >> value.z;
}

BinaryReader &operator>>(BinaryReader &binaryReader, Vector4f &value)
{
    return binaryReader >> value.x >> value.y >> value.z >> value.w;
}

BinaryReader &operator>>(BinaryReader &binaryReader, Vector2i &value)
{
    return binaryReader >> value.x >> value.y;
}

BinaryReader &operator>>(BinaryReader &binaryReader, Vector3i &value)
{
    return binaryReader >> value.x >> value.y >> value.z;
}

BinaryReader &operator>>(BinaryReader &binaryReader, Vector4i &value)
{
    return binaryReader >> value.x >> value.y >> value.z >> value.w;
}

BinaryReader &operator>>(BinaryReader &binaryReader, Vector2u &value)
{
    return binaryReader >> value.x >> value.y;
}

BinaryReader &operator>>(BinaryReader &binaryReader, Vector3u &value)
{
    return binaryReader >> value.x >> value.y >> value.z;
}

BinaryReader &operator>>(BinaryReader &binaryReader, Vector4u &value)
{
    return binaryReader >> value.x >> value.y >> value.z >> value.w;
}

BinaryReader &operator>>(BinaryReader &binaryReader, Quaternion &value)
{
    float x, y, z, w;
    BinaryReader &ret = binaryReader >> x >> y >> z >> w;
    value.Set(x, y, z, w);
    return ret;
}

BinaryReader &operator>>(BinaryReader &binaryReader, AABox &value)
{
    Vector3f minBounds, maxBounds;
    BinaryReader &ret = binaryReader >> minBounds >> maxBounds;
    value.SetBounds(minBounds, maxBounds);
    return ret;
}

BinaryReader &operator>>(BinaryReader &binaryReader, Sphere &value)
{
    Vector3f center;
    float radius;
    BinaryReader &ret = binaryReader >> center >> radius;
    value.SetCenterAndRadius(center, radius);
    return ret;
}

BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector2f &value)
{
    return binaryWriter << value.x << value.y;
}

BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector3f &value)
{
    return binaryWriter << value.x << value.y << value.z;
}

BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector4f &value)
{
    return binaryWriter << value.x << value.y << value.z << value.w;
}

BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector2i &value)
{
    return binaryWriter << value.x << value.y;
}

BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector3i &value)
{
    return binaryWriter << value.x << value.y << value.z;
}

BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector4i &value)
{
    return binaryWriter << value.x << value.y << value.z << value.w;
}

BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector2u &value)
{
    return binaryWriter << value.x << value.y;
}

BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector3u &value)
{
    return binaryWriter << value.x << value.y << value.z;
}

BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector4u &value)
{
    return binaryWriter << value.x << value.y << value.z << value.w;
}

BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Quaternion &value)
{
    return binaryWriter << value.x << value.y << value.z << value.w;
}

BinaryWriter &operator<<(BinaryWriter &binaryWriter, const AABox &value)
{
    return binaryWriter << value.GetMinBounds() << value.GetMaxBounds();
}

BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Sphere &value)
{
    return binaryWriter << value.GetCenter() << value.GetRadius();
}
