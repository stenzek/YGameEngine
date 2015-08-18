#pragma once

class BinaryReader;
class BinaryWriter;
struct Vector2f;
struct Vector3f;
struct Vector4f;
struct Vector2i;
struct Vector3i;
struct Vector4i;
struct Vector2u;
struct Vector3u;
struct Vector4u;
struct Quaternion;
class AABox;
class Sphere;

BinaryReader &operator>>(BinaryReader &binaryReader, Vector2f &value);
BinaryReader &operator>>(BinaryReader &binaryReader, Vector3f &value);
BinaryReader &operator>>(BinaryReader &binaryReader, Vector4f &value);
BinaryReader &operator>>(BinaryReader &binaryReader, Vector2i &value);
BinaryReader &operator>>(BinaryReader &binaryReader, Vector3i &value);
BinaryReader &operator>>(BinaryReader &binaryReader, Vector4i &value);
BinaryReader &operator>>(BinaryReader &binaryReader, Vector2u &value);
BinaryReader &operator>>(BinaryReader &binaryReader, Vector3u &value);
BinaryReader &operator>>(BinaryReader &binaryReader, Vector4u &value);
BinaryReader &operator>>(BinaryReader &binaryReader, Quaternion &value);
BinaryReader &operator>>(BinaryReader &binaryReader, AABox &value);
BinaryReader &operator>>(BinaryReader &binaryReader, Sphere &value);

BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector2f &value);
BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector3f &value);
BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector4f &value);
BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector2i &value);
BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector3i &value);
BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector4i &value);
BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector2u &value);
BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector3u &value);
BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Vector4u &value);
BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Quaternion &value);
BinaryWriter &operator<<(BinaryWriter &binaryWriter, const AABox &value);
BinaryWriter &operator<<(BinaryWriter &binaryWriter, const Sphere &value);