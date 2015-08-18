#include "Core/PrecompiledHeader.h"
#include "Core/VirtualFileSystem.h"
#include "Core/Console.h"
#include "YBaseLib/Platform.h"
#include "YBaseLib/Log.h"

VirtualFileSystem s_VirtualFileSystem;
VirtualFileSystem *g_pVirtualFileSystem = &s_VirtualFileSystem;
Log_SetChannel(VirtualFileSystem);

// Storing these here should be okay since nobody outside should be setting them..
namespace CVars {
    CVar vfs_basepath("vfs_basepath", CVAR_FLAG_NO_ARCHIVE | CVAR_FLAG_REQUIRE_APP_RESTART, "", "base path for virtual file system", "string");
    CVar vfs_userpath("vfs_userpath", CVAR_FLAG_NO_ARCHIVE | CVAR_FLAG_REQUIRE_APP_RESTART, "", "base user path for virtual file system", "string");
    CVar vfs_gamedir("vfs_gamedir", CVAR_FLAG_NO_ARCHIVE | CVAR_FLAG_REQUIRE_APP_RESTART, "BlockGame", "virtual file system game directory name", "string");
    CVar vfs_mount_gamedata_rw("vfs_mount_gamedata_rw", CVAR_FLAG_NO_ARCHIVE | CVAR_FLAG_REQUIRE_APP_RESTART, "false", "mount the game data directory read-write (default read-only, only will work with developer directory structure)", "string");
    CVar vfs_gitlayout("vfs_gitlayout", CVAR_FLAG_NO_ARCHIVE | CVAR_FLAG_REQUIRE_APP_RESTART, "false", "use repository-style directory structure", "bool");
}

class LocalVirtualFileSystemArchive : public VirtualFileSystemArchive
{
public:
    LocalVirtualFileSystemArchive(const char *BasePath, bool ReadOnly)
    {
        FileSystem::CanonicalizePath(m_strBasePath, BasePath);
        m_bReadOnly = ReadOnly;
    }

    ~LocalVirtualFileSystemArchive()
    {

    }

    const char *GetArchiveTypeName() const { return "LocalVirtualFileSystemArchive"; }

    inline void BuildFullPath(PathString &fullPath, const char *Path)
    {
       // build full path
        if (Path[0] != '\0')
            fullPath.Format("%s/%s", m_strBasePath.GetCharArray(), Path);
        else
            fullPath = m_strBasePath;
    }

    inline void FixPathString(String &Path)
    {
        DebugAssert(Path.GetLength() >= m_strBasePath.GetLength());
        DebugAssert(Y_strnicmp(Path, m_strBasePath, m_strBasePath.GetLength()) == 0);

        if (Path.GetLength() == m_strBasePath.GetLength())
        {
            // empty path
            Path.Clear();
            return;
        }

        // remove leading backslash
        Path.Erase(0, m_strBasePath.GetLength() + 1);

        // convert all backslashes to forward slashes
#if FS_OSPATH_SEPERATOR_CHARACTER == '\\'
        uint32 i;
        for (i = 0; i < Path.GetLength(); i++)
        {
            if (Path[i] == FS_OSPATH_SEPERATOR_CHARACTER)
                Path[i] = '/';
        }
#endif
    }

    inline void FixPathArray(char *Path)
    {
        uint32 pathLen = Y_strlen(Path);
        DebugAssert(pathLen >= m_strBasePath.GetLength());
        DebugAssert(Y_strnicmp(Path, m_strBasePath, m_strBasePath.GetLength()) == 0);

        if (pathLen == m_strBasePath.GetLength())
        {
            Path[0] = '\0';
            return;
        }

        pathLen -= m_strBasePath.GetLength() + 1;
        Y_memmove(Path, Path + m_strBasePath.GetLength() + 1, pathLen);
        Path[pathLen] = '\0';

        // convert all backslashes to forward slashes
#if FS_OSPATH_SEPERATOR_CHARACTER == '\\'
        uint32 i;
        for (i = 0; i < pathLen; i++)
        {
            if (Path[i] == FS_OSPATH_SEPERATOR_CHARACTER)
                Path[i] = '/';
        }
#endif
    }

    bool FindFiles(const char *Path, const char *Pattern, uint32 Flags, FileSystem::FindResultsArray *pResults)
    {
        // build full path
        PathString fullPath;
        BuildFullPath(fullPath, Path);

        // pass it to filesystem class
        FileSystem::BuildOSPath(fullPath);

        uint32 i;
        uint32 curResultsSize = pResults->GetSize();

        if (!FileSystem::FindFiles(fullPath.GetCharArray(), Pattern, Flags, pResults))
            return false;

        if (!(Flags & FILESYSTEM_FIND_RELATIVE_PATHS))
        {
            for (i = curResultsSize; i < pResults->GetSize(); i++)
                FixPathArray(pResults->GetElement(i).FileName);
        }

        return true;
    }

    bool StatFile(const char *Path, FILESYSTEM_STAT_DATA *pStatData)
    {
        // build full path
        PathString fullPath;
        BuildFullPath(fullPath, Path);

        // pass it to filesystem class
        FileSystem::BuildOSPath(fullPath);
        return FileSystem::StatFile(fullPath.GetCharArray(), pStatData);
    }

    bool GetFileName(String &Destination, const char *FileName)
    {
        PathString fullPath;
        BuildFullPath(fullPath, FileName);

        FileSystem::BuildOSPath(fullPath);
        if (!FileSystem::GetFileName(Destination, fullPath))
            return false;

        FixPathString(Destination);
        return true;
    }

    ByteStream *OpenFile(const char *FileName, uint32 Flags)
    {
        // special behaviour when writing to files
        if (Flags & BYTESTREAM_OPEN_WRITE)
        {
            // if requesting write, outright deny it if we are mounted read-only
            if (m_bReadOnly)
                return NULL;

            // when creating a new folder, if the parent directory structure does not exist,
            // we have to create it ourselves here.
            // not right now.. changed my mind
        }

        // build full path
        PathString fullPath;
        BuildFullPath(fullPath, FileName);

        // pass it to filesystem class
        FileSystem::BuildOSPath(fullPath);
        return FileSystem::OpenFile(fullPath, Flags);
    }

    bool DeleteFile(const char *FileName)
    {
        if (m_bReadOnly)
            return false;

        PathString fullPath;
        BuildFullPath(fullPath, FileName);
        FileSystem::BuildOSPath(fullPath);

        FILESYSTEM_STAT_DATA statData;
        if (FileSystem::StatFile(fullPath, &statData) && !(statData.Attributes & FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY))
            return FileSystem::DeleteFile(fullPath);
        else
            return false;
    }

    bool DeleteDirectory(const char *FileName, bool recursive)
    {
        if (m_bReadOnly)
            return false;

        PathString fullPath;
        BuildFullPath(fullPath, FileName);
        FileSystem::BuildOSPath(fullPath);

        FILESYSTEM_STAT_DATA statData;
        if (FileSystem::StatFile(fullPath, &statData) && statData.Attributes & FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY)
            return FileSystem::DeleteDirectory(fullPath, recursive);
        else
            return false;
    }

    class ChangeNotifierArchive : public FileSystem::ChangeNotifier
    {
    public:
        ChangeNotifierArchive(const String &basePath, const String &filterPath, FileSystem::ChangeNotifier *pRealNotifier)
            : FileSystem::ChangeNotifier(basePath, true),
              m_basePath(basePath),
              m_filterPath(filterPath),
              m_pRealNotifier(pRealNotifier)
        {

        }

        virtual ~ChangeNotifierArchive()
        {
            delete m_pRealNotifier;
        }

        virtual void EnumerateChanges(EnumerateChangesCallback callback, void *pUserData) override
        {
            m_pRealNotifier->EnumerateChanges([this, &callback, pUserData](const ChangeInfo *pChangeInfo)
            {
                uint32 pathLength = Y_strlen(pChangeInfo->Path);
                if (pathLength <= (m_basePath.GetLength() + 1))
                    return;

                // clone the path and copy it, stripping the base path, and convert any \s to /
                PathString virtualPath;
                virtualPath.AppendSubString(pChangeInfo->Path, m_basePath.GetLength() + 1);
                virtualPath.Replace('\\', '/');

                // handle filters
                if (!m_filterPath.IsEmpty() && !virtualPath.StartsWith(m_filterPath, false))
                    return;

                // invoke the callback with the virtual path
                ChangeInfo newChangeInfo;
                newChangeInfo.Path = virtualPath;
                newChangeInfo.Event = pChangeInfo->Event;
                callback(&newChangeInfo, pUserData);
            });
        }

    protected:
        String m_basePath;
        String m_filterPath;
        FileSystem::ChangeNotifier *m_pRealNotifier;
    };

    FileSystem::ChangeNotifier *CreateChangeNotifier(const String &directoryPath)
    {
        // create a change notifier on the base path
        FileSystem::ChangeNotifier *pChangeNotifier = FileSystem::CreateChangeNotifier(m_strBasePath, true);
        if (pChangeNotifier == nullptr)
            return nullptr;

        // create our virtual class
        return new ChangeNotifierArchive(m_strBasePath, directoryPath, pChangeNotifier);
    }
    
protected:
    String m_strBasePath;
    bool m_bReadOnly;
};

class ChangeNotifierVFS : public FileSystem::ChangeNotifier
{
public:
    ChangeNotifierVFS(FileSystem::ChangeNotifier **ppArchiveNotifiers, uint32 nArchiveNotifiers)
        : FileSystem::ChangeNotifier(StaticString("VFS Root"), true)
    {
        m_ppChangeNotifiers = new FileSystem::ChangeNotifier *[nArchiveNotifiers];
        Y_memcpy(m_ppChangeNotifiers, ppArchiveNotifiers, sizeof(FileSystem::ChangeNotifier *) * nArchiveNotifiers);
        m_nChangeNotifiers = nArchiveNotifiers;
    }

    virtual ~ChangeNotifierVFS()
    {
        for (uint32 i = 0; i < m_nChangeNotifiers; i++)
            delete m_ppChangeNotifiers[i];

        delete[] m_ppChangeNotifiers;
    }

    virtual void EnumerateChanges(EnumerateChangesCallback callback, void *pUserData) override
    {
        for (uint32 i = 0; i < m_nChangeNotifiers; i++)
            m_ppChangeNotifiers[i]->EnumerateChanges(callback, pUserData);
    }

private:
    FileSystem::ChangeNotifier **m_ppChangeNotifiers;
    uint32 m_nChangeNotifiers;

};

VirtualFileSystem::VirtualFileSystem()
{

}

VirtualFileSystem::~VirtualFileSystem()
{

}

bool VirtualFileSystem::Initialize()
{
    String executablePath;
    String currentSearchPath;
    FILESYSTEM_STAT_DATA statData;
    
    // parse cvar overrides
    String basePath = CVars::vfs_basepath.GetString();
    String userPath = CVars::vfs_userpath.GetString();
    String gameDirectory = CVars::vfs_gamedir.GetString();
    bool mountDataDirectoriesReadWrite = CVars::vfs_mount_gamedata_rw.GetBool();
    bool shippingDirectoryStructure = true;

    // log start
    Log_InfoPrint("Initializing virtual file system...");

    // get path to binary
    if (!Platform::GetProgramFileName(executablePath))
    {
        Log_ErrorPrintf("VirtualFileSystem initialization error: Could not get program file name.");
        return false;
    }

    // log it
    Log_DevPrintf("Base path: %s", basePath.GetCharArray());
    Log_DevPrintf("User path: %s", userPath.GetCharArray());
    Log_DevPrintf("Game directory: %s", gameDirectory.GetCharArray());
    Log_DevPrintf("Program file name: %s", executablePath.GetCharArray());    

#ifndef Y_PLATFORM_HTML5

    // check for developer directory structure    
    if (basePath.IsEmpty() || CVars::vfs_gitlayout.GetBool())
    {
        // in developer structure, program is located at <root>/Binaries/<platform>
        bool validDeveloperStructure = true;

        // determine base path
        if (basePath.IsEmpty())
            FileSystem::BuildPathRelativeToFile(basePath, executablePath, "../..", true, true);

        // test for <root>/Engine/Data
        currentSearchPath.Format("%s/Engine/Data", basePath.GetCharArray());
        FileSystem::BuildOSPath(currentSearchPath);
        if (!FileSystem::StatFile(currentSearchPath, &statData) || !(statData.Attributes & FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY))
        {
            // try three levels down, for unix systems launching editor/tools
            FileSystem::BuildPathRelativeToFile(basePath, executablePath, "../../..", true, true);

            // test for <root>/Engine/Data again
            currentSearchPath.Format("%s/Engine/Data", basePath.GetCharArray());
            FileSystem::BuildOSPath(currentSearchPath);
            if (!FileSystem::StatFile(currentSearchPath, &statData) || !(statData.Attributes & FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY))
            {
                validDeveloperStructure = false;
            }
        }
            
        // test for <root>/<gamename>/Data
        if (validDeveloperStructure)
        {
            currentSearchPath.Format("%s/%s/Data", basePath.GetCharArray(), gameDirectory.GetCharArray());
            FileSystem::BuildOSPath(currentSearchPath);
            if (!FileSystem::StatFile(currentSearchPath, &statData) || !(statData.Attributes & FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY))
                validDeveloperStructure = false;
        }

        // ok?
        if (validDeveloperStructure)
        {
            // logging
            Log_InfoPrintf("VirtualFileSystem: Developer directory structure detected (base path of '%s').", basePath.GetCharArray());
            shippingDirectoryStructure = false;

            // mount engine data
            currentSearchPath.Format("%s/Engine/Data", basePath.GetCharArray());
            FileSystem::BuildOSPath(currentSearchPath);
            AddDirectory(currentSearchPath, 30, !mountDataDirectoriesReadWrite, false);

            // mount game data
            currentSearchPath.Format("%s/%s/Data", basePath.GetCharArray(), gameDirectory.GetCharArray());
            FileSystem::BuildOSPath(currentSearchPath);
            AddDirectory(currentSearchPath, 20, !mountDataDirectoriesReadWrite, false);
        }
        else
        {
            // clear base path so code below can derive it
            basePath.Clear();
        }
    }

    // load shipping directory structure
    if (shippingDirectoryStructure)
    {
        // find base path
        if (basePath.IsEmpty())
            FileSystem::BuildPathRelativeToFile(basePath, executablePath, "..", true, true);

        // engine data directory should be at <base data path>/Engine
        currentSearchPath.Format("%s/Data/Engine", basePath.GetCharArray());
        FileSystem::BuildOSPath(currentSearchPath);
        if (!AddDirectory(currentSearchPath, 30, true, false))
        {
            Log_ErrorPrintf("VirtualFileSystem: Could not mount engine data directory (detected as '%s')", currentSearchPath.GetCharArray());
            return false;
        }

        // game data directory should be at <base data path>/<game name>
        currentSearchPath.Format("%s/Data/%s", basePath.GetCharArray(), gameDirectory.GetCharArray());
        FileSystem::BuildOSPath(currentSearchPath);
        if (!AddDirectory(currentSearchPath, 20, true, false))
        {
            Log_ErrorPrintf("VirtualFileSystem: Could not mount game data directory (detected as '%s')", currentSearchPath.GetCharArray());
            return false;
        }
    }

    // mount user data
    if (shippingDirectoryStructure || !mountDataDirectoriesReadWrite)
    {
        // both developer and shipping use the same path at <base directory>/UserData/<game name>
        // Otherwise, it'll save to My Documents\\<SavePath>, TODO IMPLEMENT THIS (SHGetSpecialFolderPath(NULL, ModulePath, CSIDL_MYDOCUMENTS, FALSE);)
        bool mountedUserData = false;

        // set userpath
        if (userPath.IsEmpty())
        {
            currentSearchPath.Format("%s/UserData", basePath.GetCharArray());
            FileSystem::BuildOSPath(currentSearchPath);
        }
        else
        {
            currentSearchPath = userPath;
        }

        // check for userdata path presence first
        if (FileSystem::StatFile(currentSearchPath, &statData) && statData.Attributes & FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY)
        {
            // if the game directory does not exist here, we can create it
            currentSearchPath.Format("%s/UserData/%s", basePath.GetCharArray(), gameDirectory.GetCharArray());
            FileSystem::BuildOSPath(currentSearchPath);
            if (!FileSystem::StatFile(currentSearchPath, &statData) || !(statData.Attributes & FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY))
            {
                // create the directory
                Log_InfoPrintf("VirtualFileSystem: Attempting to create user directory for VFS at '%s'...", currentSearchPath.GetCharArray());
                if (!FileSystem::CreateDirectory(currentSearchPath, false))
                    Log_WarningPrintf("VirtualFileSystem: Failed to create user directory in VFS. Path was '%s'.", currentSearchPath.GetCharArray());
            }

            // mount it
            mountedUserData = AddDirectory(currentSearchPath, 0, false, false);                
        }

        // worked?
        if (!mountedUserData)
        {
            Log_WarningPrint("Could not determine user path. Enable developer mode with -vfs_mount_gamedata_rw 1 to save to the game directory, or specify a user path on the commandline with -vfs_userpath.");
            return false;
        }
    }

#else

    // Mount html5 archive as a single archive at /data, /userdata
    AddDirectory("/Data/Engine", 30, true, true);
    currentSearchPath.Format("/Data/%s", gameDirectory.GetCharArray());
    AddDirectory(currentSearchPath, 20, true, true);
    currentSearchPath.Format("/UserData/%s", gameDirectory.GetCharArray());
    AddDirectory(currentSearchPath, 0, false, true);

#endif

    Log_InfoPrintf("VirtualFileSystem initialization completed, %u archives mounted.", m_liArchives.GetSize());
    return true;
}

void VirtualFileSystem::Shutdown()
{
    Log_InfoPrintf("VirtualFileSystem unmounting %u archives...", m_liArchives.GetSize());
    for (ArchiveList::Iterator itr = m_liArchives.Begin(); !itr.AtEnd(); itr.Forward())
    {
        itr->SortKey.Clear();
        delete itr->pArchiveInterface;
    }
}

bool VirtualFileSystem::FindFiles(const char *Path, const char *Pattern, uint32 Flags, FileSystem::FindResultsArray *pResults)
{
    uint32 i, j;
    bool Result = false;

    if (!(Flags & FILESYSTEM_FIND_KEEP_ARRAY))
        pResults->Clear();

    for (ArchiveList::Iterator itr = m_liArchives.Begin(); !itr.AtEnd(); itr.Forward())
    {
        uint32 curResultsSize = pResults->GetSize();

        if (itr->pArchiveInterface->FindFiles(Path, Pattern, Flags | FILESYSTEM_FIND_KEEP_ARRAY, pResults))
        {
            // remove any duplicates that snuck in
            for (i = curResultsSize; i < pResults->GetSize(); )
            {
                for (j = 0; j < curResultsSize; j++)
                {
                    if (Y_stricmp(pResults->GetElement(i).FileName, pResults->GetElement(j).FileName) == 0)
                        break;
                }
                if (j != curResultsSize)
                    pResults->OrderedRemove(i);
                else
                    i++;
            }

            Result = true;
        }
    }

    return Result;
}

bool VirtualFileSystem::StatFile(const char *Path, FILESYSTEM_STAT_DATA *pStatData)
{
    for (ArchiveList::Iterator itr = m_liArchives.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->pArchiveInterface->StatFile(Path, pStatData))
            return true;
    }
    
    return false;
}

bool VirtualFileSystem::FileExists(const char *Path)
{
    FILESYSTEM_STAT_DATA statData;
    for (ArchiveList::Iterator itr = m_liArchives.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->pArchiveInterface->StatFile(Path, &statData) && !(statData.Attributes & FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY))
            return true;
    }

    return false;
}

bool VirtualFileSystem::DirectoryExists(const char *Path)
{
    FILESYSTEM_STAT_DATA statData;
    for (ArchiveList::Iterator itr = m_liArchives.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->pArchiveInterface->StatFile(Path, &statData) && statData.Attributes & FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY)
            return true;
    }

    return false;
}

bool VirtualFileSystem::GetFileName(String &Destination, const char *FileName)
{
    for (ArchiveList::Iterator itr = m_liArchives.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->pArchiveInterface->GetFileName(Destination, FileName))
            return true;
    }

    return false;
}

bool VirtualFileSystem::GetFileName(String &FileName)
{
    return GetFileName(FileName, FileName.GetCharArray());
}

ByteStream *VirtualFileSystem::OpenFile(const char *FileName, uint32 Flags)
{
    ByteStream *pStream;
    for (ArchiveList::Iterator itr = m_liArchives.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if ((pStream = itr->pArchiveInterface->OpenFile(FileName, Flags)) != NULL)
            return pStream;
    }

    return NULL;
}

bool VirtualFileSystem::DeleteFile(const char *FileName)
{
    for (ArchiveList::Iterator itr = m_liArchives.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->pArchiveInterface->DeleteFile(FileName))
            return true;
    }

    return false;
}

bool VirtualFileSystem::DeleteDirectory(const char *FileName, bool recursive)
{
    for (ArchiveList::Iterator itr = m_liArchives.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if (itr->pArchiveInterface->DeleteDirectory(FileName, recursive))
            return true;
    }

    return false;
}

FileSystem::ChangeNotifier *VirtualFileSystem::CreateChangeNotifier(const char *directoryPath /*= nullptr*/)
{
    if (m_liArchives.GetSize() == 0)
        return nullptr;

    FileSystem::ChangeNotifier **ppArchiveNotifiers = (FileSystem::ChangeNotifier **)alloca(sizeof(FileSystem::ChangeNotifier *) * m_liArchives.GetSize());
    uint32 nArchiveNotifiers = 0;

    String directoryPathString;
    if (directoryPath != nullptr)
        directoryPathString = directoryPath;

    for (ArchiveList::Iterator itr = m_liArchives.Begin(); !itr.AtEnd(); itr.Forward())
    {
        if ((ppArchiveNotifiers[nArchiveNotifiers] = itr->pArchiveInterface->CreateChangeNotifier(directoryPathString)) != nullptr)
            nArchiveNotifiers++;
    }

    if (nArchiveNotifiers == 0)
        return nullptr;

    return new ChangeNotifierVFS(ppArchiveNotifiers, nArchiveNotifiers);
}

BinaryBlob *VirtualFileSystem::GetFileContents(const char *filename)
{
    ByteStream *pStream = OpenFile(filename, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
        return nullptr;
    
    BinaryBlob *pReturn = BinaryBlob::CreateFromStream(pStream);
    pStream->Release();
    return pReturn;
}

bool VirtualFileSystem::PutFileContents(const char *filename, const void *pData, uint32 dataSize, bool overwrite /*= true*/, bool createPath /*= true*/)
{
    if (FileExists(filename) && !overwrite)
        return false;

    uint32 openFlags = BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_STREAMED | BYTESTREAM_OPEN_ATOMIC_UPDATE;
    if (createPath)
        openFlags |= BYTESTREAM_OPEN_CREATE_PATH;

    ByteStream *pStream = OpenFile(filename, openFlags);
    if (pStream == nullptr)
        return false;

    if (dataSize > 0 && !pStream->Write2(pData, dataSize))
    {
        pStream->Discard();
        pStream->Release();
        return false;
    }
    else
    {
        pStream->Commit();
        pStream->Release();
        return true;
    }
}

bool VirtualFileSystem::AddDirectory(const char *Path, int32 Priority, bool ReadOnly, bool AddNonexistantDirectories)
{
    // we should be able to stat it, if not, don't bother, as the path most likely does not exist
    FILESYSTEM_STAT_DATA statData;
    if (!FileSystem::StatFile(Path, &statData))
    {
        // skipping directory check?
        if (!AddNonexistantDirectories)
            return false;
    }
    else if (!(statData.Attributes & FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY))
    {
        // can't add it if it's a file
        return false;
    }

    // create archive
    LocalVirtualFileSystemArchive *pArchive = new LocalVirtualFileSystemArchive(Path, ReadOnly);
    AddArchive(pArchive, EmptyString, Priority);
    Log_InfoPrintf("Mounted local directory '%s' into VFS, at priority %d, with %s permissions.", Path, Priority, ReadOnly ? "read-only" : "read-write");
    return true;
}

void VirtualFileSystem::AddArchive(VirtualFileSystemArchive *pArchiveInterface, const String &SortKey, int32 Priority)
{
    VirtualFileSystemArchiveEntry archiveEntry;
    archiveEntry.pArchiveInterface = pArchiveInterface;
    archiveEntry.SortKey = SortKey;
    archiveEntry.Priority = Priority;

    ArchiveList::Iterator itr = m_liArchives.Begin();
    for (; !itr.AtEnd(); itr.Forward())
    {
        VirtualFileSystemArchiveEntry &Entry = *itr;
        if (Entry.Priority < Priority)
            continue;

        // if priority is equal, use sort key, otherwise priority
        if (Entry.Priority == Priority && Entry.SortKey.NumericCompareInsensitive(SortKey) < 0)
            continue;

        // it is before it, insert before this one        
        m_liArchives.InsertBefore(itr, archiveEntry);
        return;
    }

    // not inserted yet, add to tail
    m_liArchives.PushBack(archiveEntry);
}

