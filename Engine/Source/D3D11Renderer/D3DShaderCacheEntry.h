#pragma once

enum D3D_SHADER_BIND_TARGET
{
    D3D_SHADER_BIND_TARGET_CONSTANT_BUFFER,
    D3D_SHADER_BIND_TARGET_RESOURCE,
    D3D_SHADER_BIND_TARGET_SAMPLER,
    D3D_SHADER_BIND_TARGET_UNORDERED_ACCESS_VIEW,
    D3D_SHADER_BIND_TARGET_COUNT,
};

#pragma pack(push, 1)

#define D3D_SHADER_CACHE_ENTRY_HEADER ((uint32)'D3DS')
#define D3D_SHADER_CACHE_ENTRY_MAX_NAME_LENGTH 256
#define D3D_GLOBAL_CONSTANT_BUFFER_SIZE (65536)       // 64KiB

struct D3DShaderCacheEntryHeader
{
    uint32 Signature;
    uint32 FeatureLevel;

    uint32 StageSize[SHADER_PROGRAM_STAGE_COUNT];

    uint32 VertexAttributeCount;
    uint32 ConstantBufferCount;
    uint32 ParameterCount;
};

struct D3DShaderCacheEntryVertexAttribute
{
    uint32 SemanticName;
    uint32 SemanticIndex;
};

struct D3DShaderCacheEntryConstantBuffer
{
    uint32 NameLength;
    uint32 MinimumSize;
    uint32 ParameterIndex;
    // <char> * NameLength follows
};

struct D3DShaderCacheEntryParameter
{
    uint32 NameLength;
    SHADER_PARAMETER_TYPE Type;
    int32 ConstantBufferIndex;
    uint32 ConstantBufferOffset;
    uint32 ArraySize;
    uint32 ArrayStride;
    uint32 BindTarget; 
    int32 BindPoint[SHADER_PROGRAM_STAGE_COUNT];
    int32 LinkedSamplerIndex;
    // <char> * NameLength follows
};

#pragma pack(pop)

