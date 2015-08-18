#pragma once
#include "Core/Common.h"

class ByteStream;

// Uses the standard chunk header format to create a straightforward way to load chunked files.
// Only one chunk can be loaded at a time. To avoid memory fragmentation, the size of the largest chunk
// will be found on initialization, and a memory block of this size will be allocated. This will be
// used for further chunk loading.

class ChunkFileReader
{
public:
    ChunkFileReader();
    ~ChunkFileReader();

    bool Initialize(ByteStream *pStream);
    bool InitializeFromMemory(const byte *pData, uint32 DataSize, bool RequiresOwnCopy);
    void Close();

    // Gets the total size of the file, including chunk headers.
    uint64 GetTotalSize() const;

    // Retrieve chunk size.
    uint32 GetChunkSize(uint32 ChunkIndex) const;

    // Loads the requested chunk as the current chunk.
    bool LoadChunk(uint32 ChunkIndex);

    // Get the currently loaded chunk as a pointer.
    const byte *GetCurrentChunkPointer() const;
    uint32 GetCurrentChunkSize() const;

    // Templated version of the above.
    template<typename T> const T *GetCurrentChunkTypePointer() const { return (const T *)GetCurrentChunkPointer(); }
    template<typename T> uint32 GetCurrentChunkTypeCount() const { return (uint32)GetCurrentChunkSize() / sizeof(T); }
    template<typename T> bool CurrentChunkHasValidCountOfType() const { return (GetCurrentChunkSize() % sizeof(T)) == 0; }
    template<typename T> bool LoadTypedChunk(uint32 ChunkIndex, const T **pTypePtr, uint32 *pTypeCount)
    {
        if (!LoadChunk(ChunkIndex) || GetCurrentChunkTypeCount<T>() == 0 || !CurrentChunkHasValidCountOfType<T>())
            return false;

        *pTypePtr = GetCurrentChunkTypePointer<T>();
        *pTypeCount = GetCurrentChunkTypeCount<T>();
        return true;
    }

    uint32 GetStringCount() const;
    const char *GetStringByIndex(uint32 StringIndex) const;

private:
    struct Chunk
    {
        uint64 Offset;
        uint32 Size;
    };

    void Reset();
    bool InternalInitialize();
    bool LoadStrings(uint32 stringDataSize, uint32 expectedStringCount);

    ByteStream *m_pStream;
    bool m_bOwnsStream;
    uint64 m_iBaseOffset;
    uint64 m_iTotalSize;
    Chunk *m_pChunks;
    uint32 m_nChunks;
    uint32 m_uMaximumChunkSize;
    uint32 m_iStringCount;
    char *m_pStringData;
    const char **m_pStringPointers;

    byte *m_pChunkData;
    uint32 m_uCurrentChunkSize;
};
