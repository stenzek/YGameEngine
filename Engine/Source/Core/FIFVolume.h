#pragma once
#include "Core/Common.h"
#include "YBaseLib/String.h"
#include "YBaseLib/Timestamp.h"
#include "YBaseLib/FileSystem.h"

typedef struct fif_mount_s *fif_mount_handle;


class FIFVolume
{
public:
    ~FIFVolume();

    // archive operations
    bool StatFile(const char *filename, FILESYSTEM_STAT_DATA *pStatData) const;
    bool FindFiles(const char *path, const char *pattern, uint32 flags, FileSystem::FindResultsArray *pResults) const;
    ByteStream *OpenFile(const char *filename, uint32 openMode, uint32 compressionLevel = 6);
    bool DeleteFile(const char *filename);
    
    // open a new volume
    static FIFVolume *CreateVolume(ByteStream *pStream, ByteStream *pTraceStream = nullptr);

    // open an existing volume.
    static FIFVolume *OpenVolume(ByteStream *pStream, ByteStream *pTraceStream = nullptr);

private:
    FIFVolume(ByteStream *pStream, ByteStream *pTraceStream, fif_mount_handle mountHandle);
    bool ParseZip();

    ByteStream *m_pStream;
    ByteStream *m_pTraceStream;
    fif_mount_handle m_mountHandle;
};

