#pragma once

#pragma pack(push, 4)

#define OPENGL_SHADER_CACHE_ENTRY_HEADER (0x474C455332305348ULL)

enum OPENGL_SHADER_BIND_TARGET
{
    OPENGL_SHADER_BIND_TARGET_UNIFORM,
    OPENGL_SHADER_BIND_TARGET_TEXTURE_UNIT,
    OPENGL_SHADER_BIND_TARGET_UNIFORM_BUFFER,
    OPENGL_SHADER_BIND_TARGET_IMAGE_UNIT,
    OPENGL_SHADER_BIND_TARGET_SHADER_STORAGE_BUFFER,
    OPENGL_SHADER_BIND_TARGET_COUNT,
};

struct OpenGLShaderCacheEntryHeader
{
    uint64 Signature;
    uint32 Platform;
    uint32 FeatureLevel;

    uint32 StageSize[SHADER_PROGRAM_STAGE_COUNT];
    uint32 VertexAttributeCount;
    uint32 UniformBlockCount;
    uint32 ParameterCount;
    uint32 FragmentDataCount;
    uint32 DebugNameLength;

    // <byte> * StageSize for each stage
    // OpenGLShaderCacheEntryVertexAttribute * VertexAttributeCount
    // OpenGLShaderCacheEntryUniformBuffer * UniformBufferCount
    // OpenGLShaderCacheEntryParameter * ParameterCount
    // OpenGLShaderCacheEntryFragmentDataOutput * FragmentDataOutputCount
    // <char> * DebugNameLength

    // Ignored on GLES2: UniformBufferCount, FragmentDataOutputCount
};

struct OpenGLShaderCacheEntryVertexAttribute
{
    uint32 SemanticName;
    uint32 SemanticIndex;
    uint32 NameLength;
    // <char> * NameLength
};

struct OpenGLShaderCacheEntryUniformBlock
{
    uint32 NameLength;
    uint32 Size;
    uint32 ParameterIndex;
    bool IsLocal;
    // <char> * NameLength 
};

struct OpenGLShaderCacheEntryParameter
{
    uint32 NameLength;
    uint32 SamplerNameLength;
    uint32 Type;
    uint32 ArraySize;
    uint32 ArrayStride;
    uint32 BindTarget;
    int32 UniformBlockIndex;
    uint32 UniformBlockOffset;
    // <char> * NameLength 
    // <char> * SamplerNameLength
};

struct OpenGLShaderCacheEntryFragmentData
{
    uint32 NameLength;
    uint32 RenderTargetIndex;
    // <char> * NameLength 
};

#pragma pack(pop)

