#include "Core/PrecompiledHeader.h"
#include "Core/ChunkFileWriter.h"
#include "Core/ChunkDataFormat.h"
#include "YBaseLib/ByteStream.h"

ChunkFileWriter::ChunkFileWriter()
{
    m_iBaseOffset = 0;
    m_pStream = NULL;
    m_pChunkHeaders = NULL;
    m_nChunks = 0;

    m_iCurrentChunk = 0xFFFFFFFF;
    m_iCurrentChunkOffset = 0;
    m_iCurrentChunkSize = 0;

    m_pStringData = NULL;
    m_iStringDataSize = 0;
    m_iStringDataBufferSize = 0;
    m_iStringCount = 0;
}

ChunkFileWriter::~ChunkFileWriter()
{
    if (m_pStream != NULL)
        Close();
}

bool ChunkFileWriter::Initialize(ByteStream *pStream, uint32 NumChunks)
{
    uint32 i;

    m_iBaseOffset = pStream->GetPosition();
    
    m_pStream = pStream;
    m_pStream->AddRef();

    m_nChunks = NumChunks;
    m_pChunkHeaders = new Chunk[m_nChunks];

    // build empty header
    DF_CHUNKFILE_HEADER emptyHeader;
    Y_memzero(&emptyHeader, sizeof(emptyHeader));
    emptyHeader.Magic = ~(uint32)DF_CHUNKFILE_HEADER_MAGIC;
    m_pStream->Write2(&emptyHeader, sizeof(emptyHeader));

    DF_CHUNKFILE_CHUNK_HEADER emptyChunkHeader;
    emptyChunkHeader.ChunkOffset = 0;
    emptyChunkHeader.ChunkSize = 0;

    for (i = 0; i < m_nChunks; i++)
    {
        // store our version
        m_pChunkHeaders[i].Offset = 0;
        m_pChunkHeaders[i].Size = 0;

        // and write an empty entry to the file
        m_pStream->Write2(&emptyChunkHeader, sizeof(emptyChunkHeader));
    }
    
    return !pStream->InErrorState();
}

bool ChunkFileWriter::Close()
{
    DebugAssert(m_iCurrentChunk == 0xFFFFFFFF);
    uint64 chunksEndOffset = m_pStream->GetPosition();

    // write the strings, if any
    if (m_iStringDataSize > 0)
        m_pStream->Write2(m_pStringData, m_iStringDataSize);

    // build header
    DF_CHUNKFILE_HEADER chunkFileHeader;
    chunkFileHeader.Magic = DF_CHUNKFILE_HEADER_MAGIC;
    chunkFileHeader.HeaderSize = sizeof(chunkFileHeader);
    chunkFileHeader.ChunkCount = m_nChunks;
    chunkFileHeader.TotalSize = (m_pStream->GetPosition() - m_iBaseOffset);
    chunkFileHeader.StringsOffset = (chunksEndOffset - m_iBaseOffset);
    chunkFileHeader.StringsSize = m_iStringDataSize;
    chunkFileHeader.StringsCount = m_iStringCount;

    // rewrite header
    m_pStream->SeekAbsolute(m_iBaseOffset);
    m_pStream->Write2(&chunkFileHeader, sizeof(chunkFileHeader));

    // rewrite chunk headers
    uint32 i;
    for (i = 0; i < m_nChunks; i++)
    {
        DF_CHUNKFILE_CHUNK_HEADER chunkHeader;
        chunkHeader.ChunkOffset = m_pChunkHeaders[i].Offset;
        chunkHeader.ChunkSize = m_pChunkHeaders[i].Size;
        m_pStream->Write2(&chunkHeader, sizeof(chunkHeader));
    }

    // seek back to the end
    m_pStream->SeekAbsolute(m_iBaseOffset + chunkFileHeader.TotalSize);

    // get result
    bool closeResult = !(m_pStream->InErrorState());

    // delete data
    if (m_pStringData != NULL)
    {
        Y_free(m_pStringData);
        m_pStringData = NULL;
    }
    m_iStringDataSize = 0;
    m_iStringDataBufferSize = 0;
    m_iStringCount = 0;
    m_iBaseOffset = 0;
    delete[] m_pChunkHeaders;
    m_nChunks = 0;

    m_pStream->Release();
    m_pStream = NULL;

    return closeResult;
}

uint32 ChunkFileWriter::AddString(const char *str)
{
    uint32 strLength = Y_strlen(str) + 1;
    if ((strLength + m_iStringDataSize) > m_iStringDataBufferSize)
    {
        m_iStringDataBufferSize = Max(m_iStringDataBufferSize * 2, m_iStringDataSize + strLength);
        m_pStringData = (char *)realloc(m_pStringData, m_iStringDataBufferSize);
        DebugAssert(m_pStringData != NULL);
    }

    Y_memcpy(m_pStringData + m_iStringDataSize, str, strLength);
    m_iStringDataSize += strLength;

    return m_iStringCount++;
}

void ChunkFileWriter::BeginChunk(uint32 ChunkIndex)
{
    DebugAssert(m_iCurrentChunk == 0xFFFFFFFF);

    m_iCurrentChunk = ChunkIndex;
    m_iCurrentChunkOffset = m_pStream->GetPosition() - m_iBaseOffset;
    m_iCurrentChunkSize = 0;
}

void ChunkFileWriter::WriteChunkData(const void *pData, uint32 cbData)
{
    m_pStream->Write(pData, cbData);
    m_iCurrentChunkSize += cbData;
}

void ChunkFileWriter::EndChunk()
{
    DebugAssert(m_iCurrentChunk != 0xFFFFFFFF && m_iCurrentChunk < m_nChunks);
    DebugAssert(m_iCurrentChunkSize <= 0xFFFFFFFF);

    m_pChunkHeaders[m_iCurrentChunk].Offset = m_iCurrentChunkOffset;
    m_pChunkHeaders[m_iCurrentChunk].Size = static_cast<uint32>(m_iCurrentChunkSize);

    m_iCurrentChunk = 0xFFFFFFFF;
    m_iCurrentChunkOffset = 0;
    m_iCurrentChunkSize = 0;
}

