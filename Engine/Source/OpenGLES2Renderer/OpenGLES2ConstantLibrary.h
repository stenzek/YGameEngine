#pragma once
#include "OpenGLES2Renderer/OpenGLES2Common.h"

class OpenGLES2ConstantLibrary
{
    DeclareNonCopyable(OpenGLES2ConstantLibrary);

public:
    typedef uint32 ConstantID;
    static const ConstantID ConstantIndexInvalid;
    static const uint32 BufferOffsetInvalid;

    struct ConstantInfo
    {
        uint32 BufferIndex;
        uint32 FieldIndex;
        uint32 ArraySize;
        SHADER_PARAMETER_TYPE Type;
        uint32 LocalBufferOffset;
        uint32 GlobalBufferOffset;
    };

public:
    OpenGLES2ConstantLibrary(RENDERER_FEATURE_LEVEL featureLevel);
    ~OpenGLES2ConstantLibrary();

    // Size of global constant storage.
    const uint32 GetConstantStorageBufferSize() const { return m_allConstantsSize; }

    // Find the constant id for a given fully-qualified name (InstanceName.VariableName)
    ConstantID LookupConstantID(const char *fullyQualifiedName) const;

    // Finds the constant id for a buffer and field index.
    ConstantID LookupConstantID(uint32 bufferIndex, uint32 fieldIndex) const;

    // Finds the constant located at buffer and offset.
    bool FindConstantsAtOffset(uint32 bufferIndex, uint32 startOffset, uint32 endOffset, ConstantID *pFirstConstant, ConstantID *pLastConstant) const;

    // Get the starting offset for a constant buffer in the global buffer.
    uint32 GetBufferStartOffset(uint32 bufferIndex) const;

    // Get constant information.
    const ConstantInfo *GetConstantInfo(ConstantID constantID) const;

    // Update the currently-bound program's copy of a constant
    void UpdateProgramConstant(const byte *pGlobalBuffer, ConstantID constantID, GLuint programLocation) const;

private:
    // generate constant data, starting offsets, and total buffer size
    void GenerateConstantData(RENDERER_FEATURE_LEVEL featureLevel);

    // allocate a block of memory of this size per-context
    uint32 m_allConstantsSize;

    // constant data
    MemArray<ConstantInfo> m_constantData;

    // starting indices of each constant buffer
    PODArray<ConstantID> m_constantBufferStartingIndices;

    // starting offset in the global buffer of each constant buffer
    PODArray<uint32> m_constantBufferStartingOffsets;

    // hash table mapping fully-qualified names to constant ids
    CIStringHashTable<ConstantID> m_nameToIDMap;
};

