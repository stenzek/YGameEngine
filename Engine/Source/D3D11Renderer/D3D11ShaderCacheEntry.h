#pragma once

enum D3D11_SHADER_BIND_TARGET
{
    D3D11_SHADER_BIND_TARGET_CONSTANT_BUFFER,
    D3D11_SHADER_BIND_TARGET_RESOURCE,
    D3D11_SHADER_BIND_TARGET_SAMPLER,
    D3D11_SHADER_BIND_TARGET_UNORDERED_ACCESS_VIEW,
    D3D11_SHADER_BIND_TARGET_COUNT,
};

#pragma pack(push, 1)

#define D3D11_SHADER_CACHE_ENTRY_HEADER ((uint32)'D11E')
#define D3D11_SHADER_CACHE_ENTRY_MAX_NAME_LENGTH 256

struct D3D11ShaderCacheEntryHeader
{
    uint32 Signature;
    uint32 FeatureLevel;

    uint32 StageSize[SHADER_PROGRAM_STAGE_COUNT];

    uint32 VertexAttributeCount;
    uint32 ConstantBufferCount;
    uint32 ParameterCount;
};

struct D3D11ShaderCacheEntryVertexAttribute
{
    uint32 SemanticName;
    uint32 SemanticIndex;
};

struct D3D11ShaderCacheEntryConstantBuffer
{
    char Name[D3D11_SHADER_CACHE_ENTRY_MAX_NAME_LENGTH];
    uint32 Size;
    uint32 ParameterIndex;
    bool IsLocal;
};

struct D3D11ShaderCacheEntryParameter
{
    char Name[D3D11_SHADER_CACHE_ENTRY_MAX_NAME_LENGTH];
    SHADER_PARAMETER_TYPE Type;
    int32 ConstantBufferIndex;
    uint32 ConstantBufferOffset;
    uint32 ArraySize;
    uint32 ArrayStride;
    uint32 BindTarget; 
    int32 BindPoint[SHADER_PROGRAM_STAGE_COUNT];
    int32 LinkedSamplerIndex;
};

#pragma pack(pop)

