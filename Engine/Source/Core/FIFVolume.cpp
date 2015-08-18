#include "Core/PrecompiledHeader.h"
#include "Core/FIFVolume.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/Timestamp.h"
#include "YBaseLib/Log.h"
#include "libfif/fif.h"
Log_SetChannel(FIFVolume);

// FIF IO functions mapped to ByteStream
static int ByteStream_fif_io_read(fif_io_userdata userdata, void *buffer, unsigned int count)
{
    ByteStream *pStream = reinterpret_cast<ByteStream *>(userdata);
    return (int)pStream->Read(buffer, count);
}

static int ByteStream_fif_io_write(fif_io_userdata userdata, const void *buffer, unsigned int count)
{
    ByteStream *pStream = reinterpret_cast<ByteStream *>(userdata);
    return (int)pStream->Write(buffer, count);
}

static int64_t ByteStream_fif_io_seek(fif_io_userdata userdata, fif_offset_t offset, enum FIF_SEEK_MODE mode)
{
    ByteStream *pStream = reinterpret_cast<ByteStream *>(userdata);

    bool res;
    if (mode == FIF_SEEK_MODE_CUR)
        res = pStream->SeekRelative(offset);
    else if (mode == FIF_SEEK_MODE_END)
        res = pStream->SeekToEnd();
    else
        res = pStream->SeekAbsolute(offset);

    return (res) ? (fif_offset_t)pStream->GetPosition() : (fif_offset_t)FIF_ERROR_IO_ERROR;
}

static int ByteStream_fif_io_zero(fif_io_userdata userdata, fif_offset_t offset, unsigned int count)
{
    ByteStream *pStream = reinterpret_cast<ByteStream *>(userdata);
    if (pStream->InErrorState())
        return FIF_ERROR_IO_ERROR;

    uint64 oldOffset = pStream->GetPosition();
    if (!pStream->SeekAbsolute(offset))
        return FIF_ERROR_BAD_OFFSET;
    
    int written = 0;
    while (count > 0)
    {
        static const byte zeroes[128] = { 0 };
        unsigned int writeCount = Min(count, (unsigned int)sizeof(zeroes));
        uint32 writtenBlock = pStream->Write(zeroes, writeCount);
        if (writtenBlock != writeCount)
            break;

        written += writeCount;
        count -= writeCount;
    }

    pStream->SeekAbsolute(oldOffset);
    return written;
}

static int ByteStream_fif_io_ftruncate(fif_io_userdata userdata, fif_offset_t newsize)
{
    // truncate right now will only increase the size
    ByteStream *pStream = reinterpret_cast<ByteStream *>(userdata);
    if (pStream->InErrorState())
        return FIF_ERROR_IO_ERROR;

    if (newsize <= (int64_t)pStream->GetSize())
        return FIF_ERROR_SUCCESS;

    fif_offset_t zeroCount = newsize - (fif_offset_t)pStream->GetSize();
    return ((int64_t)ByteStream_fif_io_zero(userdata, (fif_offset_t)pStream->GetSize(), (unsigned int)zeroCount) == zeroCount) ? FIF_ERROR_SUCCESS : FIF_ERROR_IO_ERROR;
}

static int64_t ByteStream_fif_io_filesize(fif_io_userdata userdata)
{
    ByteStream *pStream = reinterpret_cast<ByteStream *>(userdata);
    return (int64_t)pStream->GetSize();
}

static void ByteStream_fif_io(ByteStream *pStream, fif_io *pIO)
{
    pIO->io_read = ByteStream_fif_io_read;
    pIO->io_write = ByteStream_fif_io_write;
    pIO->io_seek = ByteStream_fif_io_seek;
    pIO->io_zero = ByteStream_fif_io_zero;
    pIO->io_ftruncate = ByteStream_fif_io_ftruncate;
    pIO->io_filesize = ByteStream_fif_io_filesize;
    pIO->userdata = reinterpret_cast<fif_io_userdata>(pStream);
}

class FIFFileStream : public ByteStream
{
public:
    FIFFileStream(FIFVolume *pFIFVolume, fif_mount_handle mountHandle, fif_file_handle fileHandle)
        : m_pFIFVolume(pFIFVolume),
          m_mountHandle(mountHandle),
          m_fileHandle(fileHandle)
    {

    }

    ~FIFFileStream()
    {
        fif_close(m_mountHandle, m_fileHandle);
    }
    
    virtual uint32 Read(void *pDestination, uint32 ByteCount) override
    {
        if (m_errorState)
            return 0;

        int readResult = fif_read(m_mountHandle, m_fileHandle, pDestination, ByteCount);
        if (readResult < 0)
        {
            m_errorState = true;
            return 0;
        }

        return (uint32)readResult;
    }
    
    virtual bool ReadByte(byte *pDestByte) override
    {
        return (Read(reinterpret_cast<void *>(pDestByte), sizeof(byte)) == sizeof(byte));
    }

    virtual bool Read2(void *pDestination, uint32 ByteCount, uint32 *pNumberOfBytesRead = nullptr) override
    {
        uint32 nBytes = Read(pDestination, ByteCount);
        if (pNumberOfBytesRead != nullptr)
            *pNumberOfBytesRead = nBytes;

        return (nBytes == ByteCount);
    }

    virtual uint32 Write(const void *pSource, uint32 ByteCount) override
    {
        if (m_errorState)
            return false;

        int writeResult = fif_write(m_mountHandle, m_fileHandle, pSource, ByteCount);
        if (writeResult < 0)
        {
            Log_ErrorPrintf("fif_write error state: %i", writeResult);
            m_errorState = true;
            return 0;
        }

        return (uint32)writeResult;
    }

    virtual bool WriteByte(byte SourceByte) override
    {
        return (Write(&SourceByte, sizeof(SourceByte)) == sizeof(SourceByte));
    }

    virtual bool Write2(const void *pSource, uint32 ByteCount, uint32 *pNumberOfBytesWritten = nullptr) override
    {
        uint32 nBytes = Write(pSource, ByteCount);
        if (pNumberOfBytesWritten != nullptr)
            *pNumberOfBytesWritten = nBytes;

        return (nBytes == ByteCount);
    }

    virtual bool SeekAbsolute(uint64 Offset) override
    {
        if (m_errorState)
            return false;

        fif_offset_t seekResult = fif_seek(m_mountHandle, m_fileHandle, (fif_offset_t)Offset, FIF_SEEK_MODE_SET);
        if (seekResult != (fif_offset_t)Offset)
        {
            m_errorState = true;
            return false;
        }

        return true;
    }

    virtual bool SeekRelative(int64 Offset) override
    {
        if (m_errorState)
            return false;

        fif_offset_t seekResult = fif_seek(m_mountHandle, m_fileHandle, (fif_offset_t)Offset, FIF_SEEK_MODE_CUR);
        if (seekResult < 0)
        {
            m_errorState = true;
            return false;
        }

        return true;
    }

    virtual bool SeekToEnd() override
    {
        if (m_errorState)
            return false;

        fif_offset_t seekResult = fif_seek(m_mountHandle, m_fileHandle, 0, FIF_SEEK_MODE_END);
        if (seekResult < 0)
        {
            m_errorState = true;
            return false;
        }

        return true;
    }

    virtual uint64 GetPosition() const override
    {
        return (uint32)fif_tell(m_mountHandle, m_fileHandle);
    }

    virtual uint64 GetSize() const override
    {
        fif_fileinfo fileInfo;
        return (fif_fstat(m_mountHandle, m_fileHandle, &fileInfo) != FIF_ERROR_SUCCESS) ? 0 : (uint64)fileInfo.size;
    }

    virtual bool Flush() override
    {
        return true;
    }

    virtual bool Discard() override
    {
        return false;
    }

    virtual bool Commit() override
    {
        return true;
    }

private:
    FIFVolume *m_pFIFVolume;
    fif_mount_handle m_mountHandle;
    fif_file_handle m_fileHandle;
};

FIFVolume::FIFVolume(ByteStream *pStream, ByteStream *pTraceStream, fif_mount_handle mountHandle)
    : m_pStream(pStream),
      m_pTraceStream(pTraceStream),
      m_mountHandle(mountHandle)
{
    if (pStream != nullptr)
        pStream->AddRef();
    if (pTraceStream != nullptr)
        pTraceStream->AddRef();
}

FIFVolume::~FIFVolume()
{
    int unmountResult = fif_unmount_volume(m_mountHandle);
    if (unmountResult != FIF_ERROR_SUCCESS)
        Panic("Failed to unmount volume");

    if (m_pTraceStream != nullptr)
        m_pTraceStream->Release();

    m_pStream->Release();
}

bool FIFVolume::StatFile(const char *filename, FILESYSTEM_STAT_DATA *pStatData) const
{
    // get fileinfo
    fif_fileinfo fileInfo;
    int result = fif_stat(m_mountHandle, filename, &fileInfo);
    if (result != FIF_ERROR_SUCCESS)
    {
        Log_ErrorPrintf("FIFVolume::StatFile: fif_stat() failed: %i", result);
        return false;
    }
        
    // convert attributes
    pStatData->Attributes = 0;
    if (fileInfo.attributes & FIF_FILE_ATTRIBUTE_DIRECTORY)
        pStatData->Attributes |= FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY;
    if (fileInfo.attributes & FIF_FILE_ATTRIBUTE_COMPRESSED)
        pStatData->Attributes |= FILESYSTEM_FILE_ATTRIBUTE_COMPRESSED;
    
    // remaining information
    pStatData->ModificationTime.SetUnixTimestamp(fileInfo.modify_timestamp);
    pStatData->Size = fileInfo.size;
    return true;
}

ByteStream *FIFVolume::OpenFile(const char *filename, uint32 openMode, uint32 compressionLevel /* = 6 */)
{
    // translate open flags
    unsigned int fifOpenMode = 0;
    if (openMode & BYTESTREAM_OPEN_READ)
        fifOpenMode |= FIF_OPEN_MODE_READ;
    if (openMode & BYTESTREAM_OPEN_WRITE)
        fifOpenMode |= FIF_OPEN_MODE_WRITE;
    if (openMode & BYTESTREAM_OPEN_APPEND)
        fifOpenMode |= FIF_OPEN_MODE_APPEND;
    if (openMode & BYTESTREAM_OPEN_TRUNCATE)
        fifOpenMode |= FIF_OPEN_MODE_TRUNCATE;
    if (openMode & BYTESTREAM_OPEN_CREATE)
        fifOpenMode |= FIF_OPEN_MODE_CREATE;
    if (openMode & BYTESTREAM_OPEN_STREAMED)
        fifOpenMode |= FIF_OPEN_MODE_STREAMED;

    // todo: 
    //  - BYTESTREAM_OPEN_ATOMIC_UPDATE
    //  - BYTESTREAM_OPEN_SEEKABLE

    // open file
    fif_file_handle fileHandle;
    int result = fif_open(m_mountHandle, filename, fifOpenMode, &fileHandle);
    if (result != FIF_ERROR_SUCCESS)
    {
        Log_ErrorPrintf("FIFVolume::OpenFile: fif_open() failed: %i", result);
        return nullptr;
    }

    // create stream
    return new FIFFileStream(this, m_mountHandle, fileHandle);
}

bool FIFVolume::DeleteFile(const char *filename)
{
    // forward through
    int result = fif_unlink(m_mountHandle, filename);
    if (result != FIF_ERROR_SUCCESS)
    {
        Log_ErrorPrintf("FIFVolume::DeleteFile: fif_unlink() failed: %i", result);
        return false;
    }

    return true;
}

FIFVolume *FIFVolume::CreateVolume(ByteStream *pStream, ByteStream *pTraceStream /* = nullptr */)
{
    // initialize io
    fif_io io;
    ByteStream_fif_io(pStream, &io);

    // fill default settings
    fif_volume_options volumeOptions;
    fif_mount_options mountOptions;
    fif_set_default_volume_options(&volumeOptions);
    fif_set_default_mount_options(&mountOptions);
    //volumeOptions.block_size = 16384 * 1024;
    mountOptions.new_file_compression_algorithm = FIF_COMPRESSION_ALGORITHM_ZLIB;
    mountOptions.new_file_compression_level = 6;

    // tracing?
    fif_mount_handle mountHandle;
    if (pTraceStream != nullptr)
    {
        // create trace io
        fif_io trace_io;
        ByteStream_fif_io(pTraceStream, &trace_io);

        // create the archive
        int result = fif_trace_create_volume(&mountHandle, &io, NULL, &volumeOptions, &mountOptions, &trace_io);
        if (result != FIF_ERROR_SUCCESS)
        {
            Log_ErrorPrintf("FIFVolume::CreateVolume: fif_trace_create_volume() failed: %i", result);
            return nullptr;
        }
    }
    else
    {
        // create the archive
        int result = fif_create_volume(&mountHandle, &io, NULL, &volumeOptions, &mountOptions);
        if (result != FIF_ERROR_SUCCESS)
        {
            Log_ErrorPrintf("FIFVolume::CreateVolume: fif_create_volume() failed: %i", result);
            return nullptr;
        }
    }

    // create volume
    return new FIFVolume(pStream, pTraceStream, mountHandle);
}

FIFVolume *FIFVolume::OpenVolume(ByteStream *pStream, ByteStream *pTraceStream /* = nullptr */)
{
    // initialize io
    fif_io io;
    ByteStream_fif_io(pStream, &io);

    // fill default settings
    fif_mount_options mountOptions;
    fif_set_default_mount_options(&mountOptions);

    // tracing?
    fif_mount_handle mountHandle;
    if (pTraceStream != nullptr)
    {
        // create trace io
        fif_io trace_io;
        ByteStream_fif_io(pTraceStream, &trace_io);

        // create the archive
        int result = fif_trace_mount_volume(&mountHandle, &io, NULL, &mountOptions, &trace_io);
        if (result != FIF_ERROR_SUCCESS)
        {
            Log_ErrorPrintf("FIFVolume::OpenVolume: fif_trace_mount_volume() failed: %i", result);
            return nullptr;
        }
    }
    else
    {
        // mount volume
        int result = fif_mount_volume(&mountHandle, &io, NULL, &mountOptions);
        if (result != FIF_ERROR_SUCCESS)
        {
            Log_ErrorPrintf("FIFVolume::OpenVolume: fif_mount_volume() failed: %i", result);
            return nullptr;
        }
    }

    // create volume
    return new FIFVolume(pStream, pTraceStream, mountHandle);
}

bool FIFVolume::FindFiles(const char *path, const char *pattern, uint32 flags, FileSystem::FindResultsArray *pResults) const
{
    // has a path
    if (path[0] == '\0')
        return false;

    // clear result array
    if (!(flags & FILESYSTEM_FIND_KEEP_ARRAY))
        pResults->Clear();

    //fif_enumdir()
    return false;
}
