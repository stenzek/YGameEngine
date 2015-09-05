#include "Renderer/PrecompiledHeader.h"
#include "Renderer/ShaderConstantBuffer.h"
#include "Renderer/Renderer.h"
Log_SetChannel(ShaderConstantBuffer);

ShaderConstantBuffer::RegistryType *ShaderConstantBuffer::GetRegistry()
{
    static RegistryType registry;
    return &registry;
}

ShaderConstantBuffer::ShaderConstantBuffer(const char *bufferName, const char *instanceName, SHADER_CONSTANT_BUFFER_FIELD_DECLARATION *pFieldDeclarations, RENDERER_PLATFORM platformRequirement, RENDERER_FEATURE_LEVEL minimumFeatureLevel, SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY updateFrequency)
    : m_index(0xFFFFFFFF),
      m_bufferName(bufferName),
      m_instanceName(instanceName),
      m_platformRequirement(platformRequirement),
      m_pFields(pFieldDeclarations),
      m_nFields(0)
{
    // count fields
    for (; m_pFields[m_nFields].FieldName != nullptr; m_nFields++);
    CalculateBufferSize();

    // add to registry
    m_index = GetRegistry()->RegisterTypeInfo(this, m_bufferName, 0);
}

ShaderConstantBuffer::ShaderConstantBuffer(const char *bufferName, const char *instanceName, uint32 structSize, RENDERER_PLATFORM platformRequirement, RENDERER_FEATURE_LEVEL minimumFeatureLevel, SHADER_CONSTANT_BUFFER_UPDATE_FREQUENCY updateFrequency)
    : m_index(0xFFFFFFFF),
      m_bufferName(bufferName),
      m_instanceName(instanceName),
      m_platformRequirement(platformRequirement),
      m_pFields(nullptr),
      m_nFields(0),
      m_bufferSize(structSize)
{
    // add to registry
    m_index = GetRegistry()->RegisterTypeInfo(this, m_bufferName, 0);
}

ShaderConstantBuffer::~ShaderConstantBuffer()
{
    // remove from registry
    GetRegistry()->UnregisterTypeInfo(this);
}

void ShaderConstantBuffer::CalculateBufferSize()
{
    // packing rules for both d3d constant buffers and opengl uniform buffers:
    // if each field fits into the remainder of a vec4, store it there
    // otherwise, overflow into the next vec4. array elements are each
    // stored in their own vec4.

    static const uint32 REGISTER_SIZE_BYTES = 16;
    uint32 currentRegisterIndex = 0;
    uint32 currentRegisterOffset = 0;

    for (uint32 fieldIndex = 0; fieldIndex < m_nFields; fieldIndex++)
    {
        SHADER_CONSTANT_BUFFER_FIELD_DECLARATION *field = &m_pFields[fieldIndex];
        uint32 valueSize = (field->FieldType == SHADER_PARAMETER_TYPE_STRUCT) ? field->BufferArrayStride : ShaderParameterValueTypeSize(field->FieldType);
        uint32 registersRequired = valueSize / REGISTER_SIZE_BYTES;

        // if there is an array
        if (field->ArraySize > 1)
        {
            // start a new vec4
            if (currentRegisterOffset != 0)
            {
                currentRegisterIndex++;
                currentRegisterOffset = 0;
            }

            // store starting offset
            field->BufferOffset = currentRegisterIndex * REGISTER_SIZE_BYTES;

            // update the alignment of each array element
            field->BufferArrayStride = valueSize;
            uint32 valueStrideRem = field->BufferArrayStride % REGISTER_SIZE_BYTES;
            if (valueStrideRem != 0)
                field->BufferArrayStride += (REGISTER_SIZE_BYTES - valueStrideRem);

            // align each array element to a register
            if (registersRequired == 0)
                currentRegisterIndex += field->ArraySize;
            else
                currentRegisterIndex += registersRequired * field->ArraySize;

            // if there are leftover bytes in the last register, save it
            currentRegisterOffset = valueStrideRem;
        }
        else
        {
            // can it fit in the current register?
            if (currentRegisterOffset != 0 && (valueSize + currentRegisterOffset) <= REGISTER_SIZE_BYTES)
            {
                // store it
                field->BufferOffset = currentRegisterIndex * REGISTER_SIZE_BYTES + currentRegisterOffset;
                field->BufferArrayStride = valueSize;

                // increment offset in current register
                currentRegisterOffset += valueSize;
            }
            else
            {
                // spill into next register
                if (currentRegisterOffset != 0)
                {
                    currentRegisterIndex++;
                    currentRegisterOffset = 0;
                }

                // store it
                field->BufferOffset = currentRegisterIndex * REGISTER_SIZE_BYTES;
                field->BufferArrayStride = valueSize;

                // leave space after the field in case another can be packed into this register
                currentRegisterIndex += registersRequired;
                currentRegisterOffset = valueSize % REGISTER_SIZE_BYTES;
            }
        }

        //Log_DevPrintf("constant buffer '%s' field %u (%s) offset %u stride %u", m_name, fieldIndex, field->FieldName, field->BufferOffset, field->BufferArrayStride);
    }

    // align up to the next register, opengl seems to behave this way, d3d doesn't
    if (currentRegisterOffset != 0)
    {
        currentRegisterIndex++;
        currentRegisterOffset = 0;
    }

    // calculate the overall size of the buffer
    m_bufferSize = currentRegisterIndex * REGISTER_SIZE_BYTES + currentRegisterOffset;
    //Log_DevPrintf("constant buffer '%s' buffer size %u", m_name, m_bufferSize);
}

// void ShaderConstantBuffer::SetContantLibraryIndex(uint32 index)
// {
//     DebugAssert((m_index == 0xFFFFFFFF && index != 0xFFFFFFFF) || (m_index != 0xFFFFFFFF && index == 0xFFFFFFFF));
//     m_index = index;
// }

void ShaderConstantBuffer::SetField(GPUContext *pContext, uint32 field, SHADER_PARAMETER_TYPE type, const void *pValue, bool commitChanges /* = false */) const
{
    DebugAssert(field < m_nFields);
    
    const SHADER_CONSTANT_BUFFER_FIELD_DECLARATION *fieldDeclaration = &m_pFields[field];
    DebugAssert(fieldDeclaration->FieldType == type);

    uint32 valueSize = ShaderParameterValueTypeSize(fieldDeclaration->FieldType);
    pContext->WriteConstantBuffer(m_index, field, fieldDeclaration->BufferOffset, valueSize, pValue, commitChanges);
}

void ShaderConstantBuffer::SetFieldArray(GPUContext *pContext, uint32 field, SHADER_PARAMETER_TYPE type, uint32 firstElement, uint32 numElements, const void *pValues, bool commitChanges /* = false */) const
{
    DebugAssert(field < m_nFields);

    const SHADER_CONSTANT_BUFFER_FIELD_DECLARATION *fieldDeclaration = &m_pFields[field];
    DebugAssert(fieldDeclaration->FieldType == type);
    DebugAssert((firstElement + numElements) <= fieldDeclaration->ArraySize);

    uint32 valueSize = ShaderParameterValueTypeSize(fieldDeclaration->FieldType);
    
    // do strided write?
    if (valueSize != fieldDeclaration->BufferArrayStride)
        pContext->WriteConstantBufferStrided(m_index, field, fieldDeclaration->BufferOffset + (firstElement * fieldDeclaration->BufferArrayStride), fieldDeclaration->BufferArrayStride, valueSize, numElements, pValues, commitChanges);
    else
        pContext->WriteConstantBuffer(m_index, field, fieldDeclaration->BufferOffset + (firstElement * valueSize), valueSize * numElements, pValues, commitChanges);
}

void ShaderConstantBuffer::SetFieldStruct(GPUContext *pContext, uint32 field, const void *pValue, uint32 valueSize, bool commitChanges) const
{
    DebugAssert(field < m_nFields);

    const SHADER_CONSTANT_BUFFER_FIELD_DECLARATION *fieldDeclaration = &m_pFields[field];
    DebugAssert(fieldDeclaration->FieldType == SHADER_PARAMETER_TYPE_STRUCT);
    DebugAssert(valueSize <= fieldDeclaration->BufferArrayStride);

    pContext->WriteConstantBuffer(m_index, 0xFFFFFFFF, fieldDeclaration->BufferOffset, valueSize, pValue, commitChanges);
}

void ShaderConstantBuffer::SetFieldStructArray(GPUContext *pContext, uint32 field, const void *pValues, uint32 valueSize, uint32 firstElement, uint32 numElements, bool commitChanges) const
{
    DebugAssert(field < m_nFields);

    const SHADER_CONSTANT_BUFFER_FIELD_DECLARATION *fieldDeclaration = &m_pFields[field];
    DebugAssert(fieldDeclaration->FieldType == SHADER_PARAMETER_TYPE_STRUCT);
    DebugAssert((firstElement + numElements) <= fieldDeclaration->ArraySize);

    // do strided write?
    if (valueSize != fieldDeclaration->BufferArrayStride)
        pContext->WriteConstantBufferStrided(m_index, 0xFFFFFFFF, fieldDeclaration->BufferOffset + (firstElement * fieldDeclaration->BufferArrayStride), fieldDeclaration->BufferArrayStride, valueSize, numElements, pValues, commitChanges);
    else
        pContext->WriteConstantBuffer(m_index, 0xFFFFFFFF, fieldDeclaration->BufferOffset + (firstElement * valueSize), valueSize * numElements, pValues, commitChanges);
}

void ShaderConstantBuffer::SetRawData(GPUContext *pContext, uint32 offset, uint32 size, const void *data, bool commitChanges /* = false */)
{
    DebugAssert((offset + size) <= m_bufferSize);
    pContext->WriteConstantBuffer(m_index, 0xFFFFFFFF, offset, size, data, commitChanges);
}

void ShaderConstantBuffer::CommitChanges(GPUContext *pContext)
{
    // just forward through
    pContext->CommitConstantBuffer(m_index);
}

const ShaderConstantBuffer *ShaderConstantBuffer::GetShaderConstantBufferByName(const char *name, RENDERER_PLATFORM platform /*= RENDERER_PLATFORM_COUNT*/, RENDERER_FEATURE_LEVEL featureLevel /*= RENDERER_FEATURE_LEVEL_COUNT*/)
{
    const ShaderConstantBuffer::RegistryType *registry = GetRegistry();
    for (uint32 index = 0; index < registry->GetNumTypes(); index++)
    {
        const ShaderConstantBuffer *pBuffer = registry->GetTypeInfoByIndex(index);
        if (pBuffer == nullptr)
            continue;
        
        // check platform requirement
        if (platform != RENDERER_PLATFORM_COUNT && pBuffer->GetPlatformRequirement() != RENDERER_PLATFORM_COUNT && pBuffer->GetPlatformRequirement() != platform)
            continue;

        // check feature level requirement
        if (featureLevel != RENDERER_FEATURE_LEVEL_COUNT && pBuffer->GetMinimumFeatureLevel() != RENDERER_FEATURE_LEVEL_COUNT && pBuffer->GetMinimumFeatureLevel() > featureLevel)
            continue;

        // check name
        if (Y_strcmp(pBuffer->GetBufferName(), name) == 0)
            return pBuffer;
    }

    return nullptr;
}
