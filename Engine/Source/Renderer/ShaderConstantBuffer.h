#pragma once
#include "Renderer/Common.h"
#include "Renderer/RendererTypes.h"
#include "Core/TypeRegistry.h"

enum SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY
{
    SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY_PER_DRAW,
    SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY_PER_PROGRAM,
    SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY_PER_VIEW,
    SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY_PER_FRAME,
    NUM_SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCIES
};

struct SHADER_CONSTANT_BUFFER_FIELD_DECLARATION
{
    const char *FieldName;
    SHADER_PARAMETER_TYPE FieldType;
    uint32 ArraySize;

    uint32 BufferOffset;
    uint32 BufferArrayStride;
};

class ShaderConstantBuffer
{
public:
    ShaderConstantBuffer(const char *bufferName, const char *instanceName, SHADER_CONSTANT_BUFFER_FIELD_DECLARATION *pFieldDeclarations, RENDERER_PLATFORM platformRequirement, RENDERER_FEATURE_LEVEL minimumFeatureLevel, SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY updateFrequency);
    ShaderConstantBuffer(const char *bufferName, const char *instanceName, uint32 structSize, RENDERER_PLATFORM platformRequirement, RENDERER_FEATURE_LEVEL minimumFeatureLevel, SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY updateFrequency);
    ~ShaderConstantBuffer();

    const uint32 GetIndex() const { return m_index; }
    const char *GetBufferName() const { return m_bufferName; }
    const char *GetInstanceName() const { return m_instanceName; }
    const RENDERER_PLATFORM GetPlatformRequirement() const { return m_platformRequirement; }
    const RENDERER_FEATURE_LEVEL GetMinimumFeatureLevel() const { return m_minimumFeatureLevel; }

    const SHADER_CONSTANT_BUFFER_FIELD_DECLARATION *GetFieldDeclaration(uint32 field) const { DebugAssert(field < m_nFields); return &m_pFields[field]; }
    const uint32 GetFieldCount() const { return m_nFields; }

    const uint32 GetBufferSize() const { return m_bufferSize; }

    void SetField(GPUContext *pContext, uint32 field, SHADER_PARAMETER_TYPE type, const void *pValue, bool commitChanges = false) const;
    void SetFieldArray(GPUContext *pContext, uint32 field, SHADER_PARAMETER_TYPE type, uint32 firstElement, uint32 numElements, const void *pValues, bool commitChanges = false) const;
    void SetFieldStruct(GPUContext *pContext, uint32 field, const void *pValue, uint32 valueSize, bool commitChanges = false) const;
    void SetFieldStructArray(GPUContext *pContext, uint32 field, const void *pValues, uint32 valueSize, uint32 firstElement, uint32 numElements, bool commitChanges = false) const;
    void SetRawData(GPUContext *pContext, uint32 offset, uint32 size, const void *data, bool commitChanges = false);

    void SetFieldBool(GPUContext *pContext, uint32 field, bool value, bool commitChanges = false) const { uint32 bval = (value) ? 0 : 1; SetField(pContext, field, SHADER_PARAMETER_TYPE_BOOL, &bval, commitChanges); }
    void SetFieldInt(GPUContext *pContext, uint32 field, int32 value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_INT, &value, commitChanges); }
    void SetFieldInt2(GPUContext *pContext, uint32 field, const int2 &value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_INT2, &value, commitChanges); }
    void SetFieldInt3(GPUContext *pContext, uint32 field, const int3 &value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_INT3, &value, commitChanges); }
    void SetFieldInt4(GPUContext *pContext, uint32 field, const int4 &value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_INT4, &value, commitChanges); }
    void SetFieldUInt(GPUContext *pContext, uint32 field, uint32 value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_UINT, &value, commitChanges); }
    void SetFieldUInt2(GPUContext *pContext, uint32 field, const uint2 &value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_UINT2, &value, commitChanges); }
    void SetFieldUInt3(GPUContext *pContext, uint32 field, const uint3 &value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_UINT3, &value, commitChanges); }
    void SetFieldUInt4(GPUContext *pContext, uint32 field, const uint4 &value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_UINT4, &value, commitChanges); }
    void SetFieldFloat(GPUContext *pContext, uint32 field, float value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_FLOAT, &value, commitChanges); }
    void SetFieldFloat2(GPUContext *pContext, uint32 field, const float2 &value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_FLOAT2, &value, commitChanges); }
    void SetFieldFloat3(GPUContext *pContext, uint32 field, const float3 &value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_FLOAT3, &value, commitChanges); }
    void SetFieldFloat4(GPUContext *pContext, uint32 field, const float4 &value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_FLOAT4, &value, commitChanges); }
    void SetFieldFloat3x3(GPUContext *pContext, uint32 field, const float3x3 &value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_FLOAT3X3, &value, commitChanges); }
    void SetFieldFloat3x4(GPUContext *pContext, uint32 field, const float3x4 &value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_FLOAT3X4, &value, commitChanges); }
    void SetFieldFloat4x4(GPUContext *pContext, uint32 field, const float4x4 &value, bool commitChanges = false) const { SetField(pContext, field, SHADER_PARAMETER_TYPE_FLOAT4X4, &value, commitChanges); }

    void SetFieldBoolArray(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const bool *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_BOOL, firstElement, numElements, values, commitChanges); }
    void SetFieldIntArray(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const int32 *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_INT, firstElement, numElements, values, commitChanges); }
    void SetFieldInt2Array(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const int2 *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_INT2, firstElement, numElements, values, commitChanges); }
    void SetFieldInt3Array(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const int3 *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_INT3, firstElement, numElements, values, commitChanges); }
    void SetFieldInt4Array(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const int4 *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_INT4, firstElement, numElements, values, commitChanges); }
    void SetFieldUIntArray(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const uint32 *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_UINT, firstElement, numElements, values, commitChanges); }
    void SetFieldUInt2Array(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const uint2 *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_UINT2, firstElement, numElements, values, commitChanges); }
    void SetFieldUInt3Array(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const uint3 *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_UINT3, firstElement, numElements, values, commitChanges); }
    void SetFieldUInt4Array(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const uint4 *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_UINT4, firstElement, numElements, values, commitChanges); }
    void SetFieldFloatArray(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const float *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_FLOAT, firstElement, numElements, values, commitChanges); }
    void SetFieldFloat2Array(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const float2 *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_FLOAT2, firstElement, numElements, values, commitChanges); }
    void SetFieldFloat3Array(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const float3 *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_FLOAT3, firstElement, numElements, values, commitChanges); }
    void SetFieldFloat4Array(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const float4 *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_FLOAT4, firstElement, numElements, values, commitChanges); }
    void SetFieldFloat3x3Array(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const float3x3 *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_FLOAT3X3, firstElement, numElements, values, commitChanges); }
    void SetFieldFloat3x4Array(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const float3x4 *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_FLOAT3X4, firstElement, numElements, values, commitChanges); }
    void SetFieldFloat4x4Array(GPUContext *pContext, uint32 field, uint32 firstElement, uint32 numElements, const float4x4 *values, bool commitChanges = false) const { SetFieldArray(pContext, field, SHADER_PARAMETER_TYPE_FLOAT4X4, firstElement, numElements, values, commitChanges); }

    void CommitChanges(GPUContext *pContext);

private:
    // determine the buffer size
    void CalculateBufferSize();

    // vars
    uint32 m_index;
    const char *m_bufferName;
    const char *m_instanceName;
    RENDERER_PLATFORM m_platformRequirement;
    RENDERER_FEATURE_LEVEL m_minimumFeatureLevel;
    SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY m_updateFrequency;

    // field data
    SHADER_CONSTANT_BUFFER_FIELD_DECLARATION *m_pFields;
    uint32 m_nFields;
    
    // calculated buffer size
    uint32 m_bufferSize;

public:
    // Type registry for lookups
    typedef TypeRegistry<ShaderConstantBuffer> RegistryType;
    static RegistryType *GetRegistry();
    static const ShaderConstantBuffer *GetShaderConstantBufferByName(const char *name, RENDERER_PLATFORM platform = RENDERER_PLATFORM_COUNT, RENDERER_FEATURE_LEVEL featureLevel = RENDERER_FEATURE_LEVEL_COUNT);
};

#define DECLARE_SHADER_CONSTANT_BUFFER(BufferVariableName) extern ShaderConstantBuffer BufferVariableName; extern SHADER_CONSTANT_BUFFER_FIELD_DECLARATION __SCBFields_##BufferVariableName[];
#define DECLARE_SHADER_CONSTANT_BUFFER_INCLASS(BufferVariableName) static ShaderConstantBuffer BufferVariableName; static SHADER_CONSTANT_BUFFER_FIELD_DECLARATION __SCBFields_##BufferVariableName[];

#define BEGIN_SHADER_CONSTANT_BUFFER(BufferVariableName, ShaderBufferName, ShaderInstanceName, PlatformRequirement, MinimumFeatureLevel, UpdateFrequency) const char *__SCBBufferName_##BufferVariableName = ShaderBufferName; const char *__SCBInstanceName_##BufferVariableName = ShaderInstanceName; RENDERER_PLATFORM __SCBPlatformRequirement_##BufferVariableName = PlatformRequirement; RENDERER_FEATURE_LEVEL __SCBMinimumFeatureLevel_##BufferVariableName = MinimumFeatureLevel; SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY __SCBUpdateFrequency##BufferVariableName = UpdateFrequency; SHADER_CONSTANT_BUFFER_FIELD_DECLARATION __SCBFields_##BufferVariableName[] = {
#define SHADER_CONSTANT_BUFFER_FIELD(FieldName, Type, ArraySize) { FieldName, Type, ArraySize, 0, 0 },
#define SHADER_CONSTANT_BUFFER_FIELD_STRUCT(FieldName, StructSize, ArraySize) { FieldName, SHADER_PARAMETER_TYPE_STRUCT, ArraySize, 0, StructSize },
#define END_SHADER_CONSTANT_BUFFER(BufferVariableName) { nullptr, SHADER_PARAMETER_TYPE_COUNT, 0, 0, 0 } }; ShaderConstantBuffer BufferVariableName(__SCBBufferName_##BufferVariableName, __SCBInstanceName_##BufferVariableName, __SCBFields_##BufferVariableName, __SCBPlatformRequirement_##BufferVariableName, __SCBMinimumFeatureLevel_##BufferVariableName, __SCBUpdateFrequency##BufferVariableName);

#define DECLARE_RAW_SHADER_CONSTANT_BUFFER(BufferVariableName) extern ShaderConstantBuffer BufferVariableName;
#define DECLARE_RAW_SHADER_CONSTANT_BUFFER_INCLASS(BufferVariableName) static ShaderConstantBuffer BufferVariableName;
#define DEFINE_RAW_SHADER_CONSTANT_BUFFER(BufferVariableName, ShaderBufferName, ShaderInstanceName, BufferSize, PlatformRequirement, MinimumFeatureLevel, UpdateFrequency) ShaderConstantBuffer BufferVariableName(ShaderBufferName, ShaderInstanceName, BufferSize, PlatformRequirement, MinimumFeatureLevel, UpdateFrequency);
