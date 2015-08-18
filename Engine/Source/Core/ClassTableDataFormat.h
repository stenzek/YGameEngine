#pragma once
#include "Core/Common.h"

#pragma pack(push, 4)

#ifdef Y_COMPILER_MSVC
    #pragma warning(push)
    #pragma warning(disable:4200)   // warning C4200: nonstandard extension used : zero-sized array in struct/union
    #pragma warning(disable:4510)   // warning C4510: 'DF_PROPERTY_MAPPING_HEADER' : default constructor could not be generated
    #pragma warning(disable:4512)   // warning C4512: 'DF_PROPERTY_MAPPING_HEADER' : assignment operator could not be generated
    #pragma warning(disable:4610)   // warning C4610: struct 'DF_PROPERTY_MAPPING_HEADER' can never be instantiated - user defined constructor required
#endif

// Type serialization
union DF_STRUCT_FLOAT2
{
    struct
    {
        float x;
        float y;
    };
    float components[2];
};

union DF_STRUCT_FLOAT3
{
    struct
    {
        float x;
        float y;
        float z;
    };
    float components[3];
};

union DF_STRUCT_FLOAT4
{
    struct
    {
        float x;
        float y;
        float z;
        float w;
    };
    float components[4];
};

union DF_STRUCT_INT2
{
    struct
    {
        int32 x;
        int32 y;
    };
    int32 components[2];
};

union DF_STRUCT_INT3
{
    struct
    {
        int32 x;
        int32 y;
        int32 z;
    };
    int32 components[3];
};

union DF_STRUCT_INT4
{
    struct
    {
        int32 x;
        int32 y;
        int32 z;
        int32 w;
    };
    int32 components[4];
};

union DF_STRUCT_UINT2
{
    struct
    {
        uint32 x;
        uint32 y;
    };
    uint32 components[2];
};

union DF_STRUCT_UINT3
{
    struct
    {
        uint32 x;
        uint32 y;
        uint32 z;
    };
    uint32 components[3];
};

union DF_STRUCT_UINT4
{
    struct
    {
        uint32 x;
        uint32 y;
        uint32 z;
        uint32 w;
    };
    uint32 components[4];
};

union DF_STRUCT_QUATERNION
{
    struct
    {
        float x;
        float y;
        float z;
        float w;
    };
    float components[4];
};

struct DF_STRUCT_TRANSFORM
{
    DF_STRUCT_FLOAT3 Position;
    DF_STRUCT_QUATERNION Rotation;
    DF_STRUCT_FLOAT3 Scale;
};

// ----------------------------------------- object serialization ------------------------------------------
#define DF_CLASS_TABLE_HEADER_MAGIC 0xAB8123DF
struct DF_CLASS_TABLE_HEADER
{
    uint32 Magic;
    uint32 TotalSize;
    uint32 HeaderSize;
    uint32 TypeCount;
};

struct DF_CLASS_TABLE_TYPE
{
    uint32 TotalSize;
    uint32 PropertyCount;
    uint32 TypeNameLength;
    char TypeName[];
};

struct DF_CLASS_TABLE_TYPE_PROPERTY
{
    uint32 PropertyType;
    uint32 PropertyNameLength;
    char PropertyName[];
};

struct DF_SERIALIZED_OBJECT_HEADER
{
    uint32 TotalSize;
    uint32 TypeIndex;
};

struct DF_SERIALIZED_OBJECT_PROPERTY
{
    uint32 DataSize;
    byte Data[];
};

#ifdef Y_COMPILER_MSVC
    #pragma warning(pop)
#endif
#pragma pack(pop)
