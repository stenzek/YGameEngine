#include "Core/PrecompiledHeader.h"
#include "Core/ChunkFileReader.h"
#include "Core/ChunkDataFormat.h"
#include "YBaseLib/ByteStream.h"

ChunkFileReader::ChunkFileReader()
    : m_pStream(NULL),
      m_bOwnsStream(false),
      m_iBaseOffset(0),
      m_iTotalSize(0),
      m_pChunks(NULL),
      m_nChunks(0),
      m_uMaximumChunkSize(0),
      m_iStringCount(0),
      m_pStringData(NULL),
      m_pStringPointers(NULL),
      m_pChunkData(NULL),
      m_uCurrentChunkSize(0)
{

}

ChunkFileReader::~ChunkFileReader()
{
    Reset();
}

void ChunkFileReader::Reset()
{
    if (m_bOwnsStream)
        m_pStream->Release();

    m_pStream = NULL;
    m_bOwnsStream = false;
    m_iBaseOffset = 0;
    m_iTotalSize = 0;

    if (m_pChunks != NULL)
    {
        delete[] m_pChunks;
        m_pChunks = NULL;
    }

    m_nChunks = 0;
    m_uMaximumChunkSize = 0;

    m_iStringCount = 0;
    if (m_pStringPointers != NULL)
    {
        delete[] m_pStringPointers;
        m_pStringPointers = NULL;
    }
    if (m_pStringData != NULL)
    {
        Y_free(m_pStringData);
        m_pStringData = NULL;
    }
    
    if (m_pChunkData != NULL)
    {
        delete[] m_pChunkData;
        m_pChunkData = NULL;
    }
    m_uCurrentChunkSize = 0;
}

bool ChunkFileReader::Initialize(ByteStream *pStream)
{
    Reset();
    m_pStream = pStream;
    return InternalInitialize();
}

bool ChunkFileReader::InitializeFromMemory(const byte *pData, uint32 DataSize, bool RequiresOwnCopy)
{
    Reset();

    if (RequiresOwnCopy)
    {
        m_pStream = ByteStream_CreateGrowableMemoryStream(NULL, DataSize);
        m_pStream->Write(pData, DataSize);
        m_pStream->SeekAbsolute(0);
        m_bOwnsStream = true;
    }
    else
    {
        m_pStream = ByteStream_CreateMemoryStream((void *)pData, DataSize);
        m_bOwnsStream = true;
    }

    return InternalInitialize();        
}

void ChunkFileReader::Close()
{
    Reset();
}

bool ChunkFileReader::InternalInitialize()
{
    uint32 i;

    // work out base offset
    m_iBaseOffset = m_pStream->GetPosition();

    // read in chunk header
    DF_CHUNKFILE_HEADER chunkFileHeader;
    if (!m_pStream->Read2(&chunkFileHeader, sizeof(chunkFileHeader)))
        goto FAILURE;

    // validate header
    if (chunkFileHeader.Magic != DF_CHUNKFILE_HEADER_MAGIC ||
        chunkFileHeader.HeaderSize != sizeof(chunkFileHeader))
    {
        goto FAILURE;
    }

    // work out offset to chunks
    m_nChunks = chunkFileHeader.ChunkCount;
    m_iTotalSize = chunkFileHeader.TotalSize;

    // read in chunk headers
    m_pChunks = new Chunk[m_nChunks];
    for (i = 0; i < m_nChunks; i++)
    {
        DF_CHUNKFILE_CHUNK_HEADER chunkHeader;
        if (!m_pStream->Read2(&chunkHeader, sizeof(chunkHeader)))
            goto FAILURE;

        m_pChunks[i].Offset = m_iBaseOffset + chunkHeader.ChunkOffset;
        m_pChunks[i].Size = chunkHeader.ChunkSize;
    }

    // alloc space
    for (i = 0; i < m_nChunks; i++)
        m_uMaximumChunkSize = Max(m_uMaximumChunkSize, m_pChunks[i].Size);
    if (m_uMaximumChunkSize > 0)
        m_pChunkData = new byte[(size_t)m_uMaximumChunkSize];

    // read in strings
    if (chunkFileHeader.StringsSize > 0)
    {
        if (!m_pStream->SeekAbsolute(m_iBaseOffset + chunkFileHeader.StringsOffset))
            goto FAILURE;

        if (!LoadStrings(chunkFileHeader.StringsSize, chunkFileHeader.StringsCount))
            goto FAILURE;
    }

    return true;

FAILURE:
    Reset();
    return false;
}

uint64 ChunkFileReader::GetTotalSize() const
{
    return m_iTotalSize;
}

uint32 ChunkFileReader::GetChunkSize(uint32 ChunkIndex) const
{
    if (ChunkIndex >= m_nChunks)
        return 0;

    return m_pChunks[ChunkIndex].Size;
}

bool ChunkFileReader::LoadChunk(uint32 ChunkIndex)
{
    if (ChunkIndex >= m_nChunks || m_pChunks[ChunkIndex].Size == 0)
        return false;

    m_uCurrentChunkSize = m_pChunks[ChunkIndex].Size;
    if (!m_pStream->SeekAbsolute(m_pChunks[ChunkIndex].Offset) || !m_pStream->Read2(m_pChunkData, (size_t)m_uCurrentChunkSize))
        return false;

    return true;
}

const byte *ChunkFileReader::GetCurrentChunkPointer() const
{
    DebugAssert(m_uCurrentChunkSize > 0);
    return m_pChunkData;
}

uint32 ChunkFileReader::GetCurrentChunkSize() const
{
    DebugAssert(m_uCurrentChunkSize > 0);
    return m_uCurrentChunkSize;
}

bool ChunkFileReader::LoadStrings(uint32 stringDataSize, uint32 expectedStringCount)
{
    uint32 i, j;
    DebugAssert(stringDataSize > 0);

    m_pStringData = (char *)malloc(stringDataSize);
    if (!m_pStream->Read2(m_pStringData, stringDataSize))
        return false;

    // last character should be a zero. if not, bad data
    if (m_pStringData[stringDataSize - 1] != 0)
        return false;

    // first pass: determine count of strings
    for (i = 0; i < stringDataSize; i++)
    {
        if (m_pStringData[i] == 0)
            m_iStringCount++;
    }

    // should be >0 due to check above
    DebugAssert(m_iStringCount > 0);
    if (m_iStringCount != expectedStringCount)
        return false;

    // second pass: allocate string count, store pointers
    m_pStringPointers = new const char *[m_iStringCount];
    const char *pStringStart = (const char *)m_pStringData;
    for (i = 0, j = 0; i < stringDataSize; i++)
    {
        if (m_pStringData[i] == 0)
        {
            DebugAssert(j < m_iStringCount);
            m_pStringPointers[j++] = pStringStart;
            pStringStart = ((const char *)m_pStringData) + i + 1;
        }
    }

    // should be equal
    DebugAssert(m_iStringCount == j);
    return true;
}

uint32 ChunkFileReader::GetStringCount() const
{
    return m_iStringCount;
}

const char *ChunkFileReader::GetStringByIndex(uint32 StringIndex) const
{
    if (StringIndex >= m_iStringCount)
        return NULL;

    return m_pStringPointers[StringIndex];
}
