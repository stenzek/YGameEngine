#pragma once
#include "Core/Common.h"
#include "YBaseLib/ByteStream.h"
#include "YBaseLib/FileSystem.h"
#include "YBaseLib/BinaryBlob.h"
#include "YBaseLib/LinkedList.h"

class VirtualFileSystemArchive
{
public:
    virtual ~VirtualFileSystemArchive() {}
    virtual const char *GetArchiveTypeName() const = 0;
    virtual bool FindFiles(const char *Path, const char *Pattern, uint32 Flags, FileSystem::FindResultsArray *pResults) = 0;
    virtual bool StatFile(const char *Path, FILESYSTEM_STAT_DATA *pStatData) = 0;
    virtual bool GetFileName(String &Destination, const char *FileName) = 0;
    virtual ByteStream *OpenFile(const char *FileName, uint32 Flags) = 0;
    virtual bool DeleteFile(const char *FileName) = 0;
    virtual bool DeleteDirectory(const char *FileName, bool recursive) = 0;
    virtual FileSystem::ChangeNotifier *CreateChangeNotifier(const String &directoryPath) = 0;
};

class VirtualFileSystem
{
public:
    VirtualFileSystem();
    ~VirtualFileSystem();

    // initialization
    bool Initialize();
    void Shutdown();

    // search for files
    bool FindFiles(const char *Path, const char *Pattern, uint32 Flags, FileSystem::FindResultsArray *pResults);    

    // stat file
    bool StatFile(const char *Path, FILESYSTEM_STAT_DATA *pStatData);

    // file exists?
    bool FileExists(const char *Path);

    // directory exists?
    bool DirectoryExists(const char *Path);

    // get filename on disk, fixes case-sensitivity
    bool GetFileName(String &Destination, const char *FileName);
    bool GetFileName(String &FileName);

    // open files
    ByteStream *OpenFile(const char *FileName, uint32 Flags);

    // delete files
    bool DeleteFile(const char *FileName);

    // delete directory, optionally recursive, if not recursive, will fail if files exist
    bool DeleteDirectory(const char *FileName, bool recursive);

    // create change notifier interface
    virtual FileSystem::ChangeNotifier *CreateChangeNotifier(const char *directoryPath = nullptr);

    // get file contents helper
    BinaryBlob *GetFileContents(const char *filename);

    // put file contents helper
    bool PutFileContents(const char *filename, const void *pData, uint32 dataSize, bool overwrite = true, bool createPath = true);

private:
    // searches the path for any pack files, and mounts them at the specified priority
    //void AddPackFiles(const char *Path, int32 Priority);

    // mounts a local file system at this path
    bool AddDirectory(const char *Path, int32 Priority, bool ReadOnly, bool AddNonexistantDirectories);

    // adds a precreated VFS archive at the specified priority.
    void AddArchive(VirtualFileSystemArchive *pArchiveInterface, const String &SortKey, int32 Priority);
    
    struct VirtualFileSystemArchiveEntry
    {
        VirtualFileSystemArchive *pArchiveInterface;
        String SortKey;
        int32 Priority;
    };

    // TODO: Convert to array
    typedef LinkedList<VirtualFileSystemArchiveEntry> ArchiveList;
    ArchiveList m_liArchives;
};

extern VirtualFileSystem *g_pVirtualFileSystem;
