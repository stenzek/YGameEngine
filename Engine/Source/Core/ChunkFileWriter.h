#pragma once
#include "Core/Common.h"

class ByteStream;

class ChunkFileWriter
{
public:
    ChunkFileWriter();
    ~ChunkFileWriter();

    bool Initialize(ByteStream *pStream, uint32 NumChunks);
    bool Close();

    uint32 AddString(const char *str);

    void BeginChunk(uint32 ChunkIndex);
    void WriteChunkData(const void *pData, uint32 cbData);
    void EndChunk();

private:
    struct Chunk
    {
        uint64 Offset;
        uint32 Size;
    };

    ByteStream *m_pStream;
    uint64 m_iBaseOffset;

    Chunk *m_pChunkHeaders;
    uint32 m_nChunks;

    uint32 m_iCurrentChunk;
    uint64 m_iCurrentChunkOffset;
    uint64 m_iCurrentChunkSize;

    char *m_pStringData;
    uint32 m_iStringDataSize;
    uint32 m_iStringDataBufferSize;
    uint32 m_iStringCount;
};