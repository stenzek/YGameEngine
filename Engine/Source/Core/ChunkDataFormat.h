#pragma once
#include "Core/Common.h"

// Common chunk header usable for all types, and ChunkLoader class.
// A ChunkSize of zero will assume that the chunk is not present.
#define DF_CHUNKFILE_HEADER_MAGIC 0x4B484359

struct DF_CHUNKFILE_HEADER
{
    uint32 Magic;
    uint32 HeaderSize;
    uint32 ChunkCount;
    uint64 TotalSize;
    uint64 StringsOffset;
    uint32 StringsSize;
    uint32 StringsCount;
};

struct DF_CHUNKFILE_CHUNK_HEADER
{
    uint64 ChunkOffset;
    uint32 ChunkSize;
};
