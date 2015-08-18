#pragma once

// profile setup
// todo move this elsewhere....

// define to log loading times of resources
#define PROFILE_RESOURCEMANAGER_LOAD_TIMES 1

// define to log texture upload times
#define PROFILE_TEXTURE_UPLOAD_TIMES 1

// define to log compilation times of shaders
#define PROFILE_SHADER_COMPILE_TIMES 1

// define to log terrain quadtree rebuild times
#define PROFILE_TERRAIN_QUADTREE_REBUILD_TIMES 1

// BaseLib includes
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/CString.h"
#include "YBaseLib/NameTable.h"
#include "YBaseLib/Timer.h"
#include "YBaseLib/Pair.h"
#include "YBaseLib/KeyValuePair.h"
#include "YBaseLib/Array.h"
#include "YBaseLib/MemArray.h"
#include "YBaseLib/AlignedMemArray.h"
#include "YBaseLib/PODArray.h"
#include "YBaseLib/String.h"
#include "YBaseLib/ReferenceCounted.h"
#include "YBaseLib/ReferenceCountedHolder.h"
#include "YBaseLib/AutoReleasePtr.h"
#include "YBaseLib/NonCopyable.h"
#include "YBaseLib/LinkedList.h"
#include "YBaseLib/HashTable.h"
#include "YBaseLib/CIStringHashTable.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/TextReader.h"
#include "YBaseLib/TextWriter.h"
#include "YBaseLib/Singleton.h"
#include "YBaseLib/StringConverter.h"
#include "YBaseLib/Thread.h"
#include "YBaseLib/Mutex.h"
#include "YBaseLib/MutexLock.h"
#include "YBaseLib/RecursiveMutex.h"
#include "YBaseLib/RecursiveMutexLock.h"
#include "YBaseLib/ReadWriteLock.h"
#include "YBaseLib/Barrier.h"
#include "YBaseLib/ThreadPool.h"
#include "YBaseLib/Timestamp.h"
#include "YBaseLib/BinaryBlob.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/Functor.h"
#include "YBaseLib/ProgressCallbacks.h"
#include "YBaseLib/Platform.h"
#include "YBaseLib/FileSystem.h"
#include "YBaseLib/BitSet.h"

// MathLib includes
#include "MathLib/AABox.h"
#include "MathLib/Sphere.h"
#include "MathLib/Vectorh.h"
#include "MathLib/Vectorf.h"
#include "MathLib/Vectori.h"
#include "MathLib/Vectoru.h"
#include "MathLib/Matrixf.h"
#include "MathLib/SIMDVectorf.h"
#include "MathLib/SIMDVectori.h"
#include "MathLib/SIMDMatrixf.h"
#include "MathLib/SIMDMatrixi.h"
#include "MathLib/Ray.h"
#include "MathLib/Plane.h"
#include "MathLib/Quaternion.h"
#include "MathLib/Frustum.h"
#include "MathLib/Transform.h"
#include "MathLib/Interpolator.h"
#include "MathLib/StreamOperators.h"
#include "MathLib/StringConverters.h"
#include "MathLib/HashTraits.h"

// Core includes
#include "Core/Property.h"
#include "Core/PixelFormat.h"
#include "Core/Console.h"
#include "Core/VirtualFileSystem.h"
#include "Core/Object.h"
#include "Core/Resource.h"

// Alias mathlib types, temporary
typedef Vector2f float2;
typedef Vector3f float3;
typedef Vector4f float4;
typedef Vector2i int2;
typedef Vector3i int3;
typedef Vector4i int4;
typedef Vector2u uint2;
typedef Vector3u uint3;
typedef Vector4u uint4;
typedef Matrix3x3f float3x3;
typedef Matrix3x4f float3x4;
typedef Matrix4x4f float4x4;

// Alias in stringconverter
namespace StringConverter {
    static inline float2 StringToFloat2(const char *Source) { return StringToVector2f(Source); }
    static inline float3 StringToFloat3(const char *Source) { return StringToVector3f(Source); }
    static inline float4 StringToFloat4(const char *Source) { return StringToVector4f(Source); }
    static inline void Float2ToString(String &Destination, const float2 &Source) { return Vector2fToString(Destination, Source); }
    static inline void Float3ToString(String &Destination, const float3 &Source) { return Vector3fToString(Destination, Source); }
    static inline void Float4ToString(String &Destination, const float4 &Source) { return Vector4fToString(Destination, Source); }
    static inline int2 StringToInt2(const char *Source) { return StringToVector2i(Source); }
    static inline int3 StringToInt3(const char *Source) { return StringToVector3i(Source); }
    static inline int4 StringToInt4(const char *Source) { return StringToVector4i(Source); }
    static inline uint2 StringToUInt2(const char *Source) { return StringToVector2u(Source); }
    static inline uint3 StringToUInt3(const char *Source) { return StringToVector3u(Source); }
    static inline uint4 StringToUInt4(const char *Source) { return StringToVector4u(Source); }
    static inline void Int2ToString(String &Destination, const int2 &Source) { return Vector2iToString(Destination, Source); }
    static inline void Int3ToString(String &Destination, const int3 &Source) { return Vector3iToString(Destination, Source); }
    static inline void Int4ToString(String &Destination, const int4 &Source) { return Vector4iToString(Destination, Source); }
    static inline void UInt2ToString(String &Destination, const uint2 &Source) { return Vector2uToString(Destination, Source); }
    static inline void UInt3ToString(String &Destination, const uint3 &Source) { return Vector3uToString(Destination, Source); }
    static inline void UInt4ToString(String &Destination, const uint4 &Source) { return Vector4uToString(Destination, Source); }
    static inline TinyString Float2ToString(const float2 &Source) { return Vector2fToString(Source); }
    static inline TinyString Float3ToString(const float3 &Source) { return Vector3fToString(Source); }
    static inline TinyString Float4ToString(const float4 &Source) { return Vector4fToString(Source); }
    static inline TinyString Int2ToString(const int2 &Source) { return Vector2iToString(Source); }
    static inline TinyString Int3ToString(const int3 &Source) { return Vector3iToString(Source); }
    static inline TinyString Int4ToString(const int4 &Source) { return Vector4iToString(Source); }
    static inline TinyString UInt2ToString(const uint2 &Source) { return Vector2uToString(Source); }
    static inline TinyString UInt3ToString(const uint3 &Source) { return Vector3uToString(Source); }
    static inline TinyString UInt4ToString(const uint4 &Source) { return Vector4uToString(Source); }
}    

// engine common includes
#include "Engine/Defines.h"
