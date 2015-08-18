#include "OpenGLES2Renderer/PrecompiledHeader.h"
#include "OpenGLES2Renderer/OpenGLES2ConstantLibrary.h"
#include "OpenGLES2Renderer/OpenGLES2GPUContext.h"
#include "Renderer/ShaderConstantBuffer.h"
Log_SetChannel(OpenGLES2ConstantLibrary);

const OpenGLES2ConstantLibrary::ConstantID OpenGLES2ConstantLibrary::ConstantIndexInvalid = 0xFFFFFFFF;
const uint32 OpenGLES2ConstantLibrary::BufferOffsetInvalid = 0xFFFFFFFF;

OpenGLES2ConstantLibrary::OpenGLES2ConstantLibrary(RENDERER_FEATURE_LEVEL featureLevel)
    : m_allConstantsSize(0)
{
    GenerateConstantData(featureLevel);
}

OpenGLES2ConstantLibrary::~OpenGLES2ConstantLibrary()
{

}

OpenGLES2ConstantLibrary::ConstantID OpenGLES2ConstantLibrary::LookupConstantID(const char *fullyQualifiedName) const
{
    const CIStringHashTable<ConstantID>::Member *pMember = m_nameToIDMap.Find(fullyQualifiedName);
    if (pMember == nullptr)
        return ConstantIndexInvalid;

    return pMember->Value;
}

OpenGLES2ConstantLibrary::ConstantID OpenGLES2ConstantLibrary::LookupConstantID(uint32 bufferIndex, uint32 fieldIndex) const
{
    ConstantID startingConstantID = m_constantBufferStartingIndices[bufferIndex];
    if (startingConstantID == ConstantIndexInvalid)
        return ConstantIndexInvalid;

    return startingConstantID + fieldIndex;
}

bool OpenGLES2ConstantLibrary::FindConstantsAtOffset(uint32 bufferIndex, uint32 startOffset, uint32 endOffset, ConstantID *pFirstConstant, ConstantID *pLastConstant) const
{
    ConstantID bufferStartingConstantID = m_constantBufferStartingOffsets[bufferIndex];
    ConstantID firstConstantID = ConstantIndexInvalid;
    ConstantID lastConstantID = ConstantIndexInvalid;
    for (ConstantID currentConstantID = bufferStartingConstantID; currentConstantID < m_constantData.GetSize(); currentConstantID++)
    {
        const ConstantInfo &constantInfo = m_constantData[currentConstantID];
        if (constantInfo.BufferIndex != bufferIndex)
            break;

        if (startOffset >= constantInfo.LocalBufferOffset)
        {
            if (firstConstantID == ConstantIndexInvalid)
            {
                firstConstantID = currentConstantID;
                lastConstantID = currentConstantID;
            }
        }

        if (endOffset > constantInfo.LocalBufferOffset)
            lastConstantID = currentConstantID;
        else
            break;
    }

    if (firstConstantID != ConstantIndexInvalid)
    {
        *pFirstConstant = firstConstantID;
        *pLastConstant = lastConstantID;
        return true;
    }
    else
    {
        return false;
    }
}

uint32 OpenGLES2ConstantLibrary::GetBufferStartOffset(uint32 bufferIndex) const
{
    return m_constantBufferStartingOffsets[bufferIndex];
}

const OpenGLES2ConstantLibrary::ConstantInfo *OpenGLES2ConstantLibrary::GetConstantInfo(ConstantID constantID) const
{
    return &m_constantData[constantID];
}

void OpenGLES2ConstantLibrary::UpdateProgramConstant(const byte *pGlobalBuffer, ConstantID constantID, GLuint programLocation) const
{
    const ConstantInfo &info = m_constantData[constantID];
    OpenGLES2Helpers::GLUniformWrapper(info.Type, programLocation, info.ArraySize, pGlobalBuffer + info.GlobalBufferOffset);
}

void OpenGLES2ConstantLibrary::GenerateConstantData(RENDERER_FEATURE_LEVEL featureLevel)
{
    const ShaderConstantBuffer::RegistryType *pRegistry = ShaderConstantBuffer::GetRegistry();

    // reserve space
    m_constantBufferStartingIndices.Reserve(pRegistry->GetNumTypes());
    m_constantBufferStartingOffsets.Reserve(pRegistry->GetNumTypes());

    // iterate buffers
    uint32 totalBufferSize = 0;
    for (uint32 registryIndex = 0; registryIndex < pRegistry->GetNumTypes(); registryIndex++)
    {
        // lookup
        const ShaderConstantBuffer *pBuffer = pRegistry->GetTypeInfoByIndex(registryIndex);
        if (pBuffer == nullptr)
        {
            m_constantBufferStartingIndices.Add(ConstantIndexInvalid);
            m_constantBufferStartingOffsets.Add(BufferOffsetInvalid);
            continue;
        }

        // check platform requirement
        if (pBuffer->GetPlatformRequirement() != RENDERER_PLATFORM_COUNT && pBuffer->GetPlatformRequirement() != RENDERER_PLATFORM_OPENGLES2)
        {
            m_constantBufferStartingIndices.Add(ConstantIndexInvalid);
            m_constantBufferStartingOffsets.Add(BufferOffsetInvalid);
            continue;
        }

        // check feature level requirement
        if (pBuffer->GetMinimumFeatureLevel() != RENDERER_FEATURE_LEVEL_COUNT && pBuffer->GetMinimumFeatureLevel() > featureLevel)
        {
            m_constantBufferStartingIndices.Add(ConstantIndexInvalid);
            m_constantBufferStartingOffsets.Add(BufferOffsetInvalid);
            continue;
        }

        // we can't handle struct buffers right now
        if (pBuffer->GetFieldCount() == 0)
        {
            m_constantBufferStartingIndices.Add(ConstantIndexInvalid);
            m_constantBufferStartingOffsets.Add(BufferOffsetInvalid);
            continue;
        }

        // add our starting constant index
        m_constantBufferStartingIndices.Add(m_constantData.GetSize());

        // add our starting offset
        m_constantBufferStartingOffsets.Add(totalBufferSize);

        // retrieve constants
        for (uint32 fieldIndex = 0; fieldIndex < pBuffer->GetFieldCount(); fieldIndex++)
        {
            const SHADER_CONSTANT_BUFFER_FIELD_DECLARATION *pDeclaration = pBuffer->GetFieldDeclaration(fieldIndex);

            // generate fully-qualified name
            SmallString fullyQualifiedName;
            fullyQualifiedName.Format("%s.%s", pBuffer->GetInstanceName(), pDeclaration->FieldName);

            // insert to name map, this shouldn't exist
            if (m_nameToIDMap.Find(fullyQualifiedName) != nullptr)
                Log_WarningPrintf("OpenGLES2ConstantLibrary::GenerateConstantData: Duplicate fully-qualified name: %s", fullyQualifiedName.GetCharArray());
            else
                m_nameToIDMap.Insert(fullyQualifiedName, m_constantData.GetSize());

            // generate constant info
            ConstantInfo data;
            data.BufferIndex = registryIndex;
            data.FieldIndex = fieldIndex;
            data.ArraySize = pDeclaration->ArraySize;
            data.Type = pDeclaration->FieldType;
            data.LocalBufferOffset = pDeclaration->BufferOffset;
            //data.GlobalBufferOffset = totalBufferSize;            
            data.GlobalBufferOffset = totalBufferSize + pDeclaration->BufferOffset;
            m_constantData.Add(data);

            // add our field size, ES data is tightly packed ignoring register alignment
            DebugAssert(pDeclaration->ArraySize > 0);
            //totalBufferSize += ShaderParameterValueTypeSize(pDeclaration->FieldType) * pDeclaration->ArraySize;

            // can't pack, because we're updated via WriteConstantBuffer which uses the GPU-register offset
            // if we wrote by constant id this would be different, though
            //totalBufferSize += pDeclaration->ArraySize * pDeclaration->BufferArrayStride;
        }

        totalBufferSize += pBuffer->GetBufferSize();
    }

    // allocate the constant storage
    Log_DevPrintf("OpenGLES2ConstantLibrary::GenerateConstantData: Total constant memory usage: %u bytes", totalBufferSize);
    DebugAssert(totalBufferSize > 0);
    m_allConstantsSize = totalBufferSize;
}

