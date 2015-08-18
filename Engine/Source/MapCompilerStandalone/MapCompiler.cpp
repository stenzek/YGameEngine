#include "Common.h"
#include "MapCompiler/MapCompiler.h"
#include "Engine/Engine.h"
Log_SetChannel(MapCompiler);

enum MAP_COMPILER_STAGE
{
    MAP_COMPILER_STAGE_REGIONS,
    MAP_COMPILER_STAGE_TERRAIN,
    MAP_COMPILER_STAGE_BLOCK_TERRAIN,
    MAP_COMPILER_STAGE_COUNT,
};

static String s_inputFileName;
static String s_outputFileName;
static uint32 s_overrideLODLevels;
static uint32 s_overrideLoadRadius;
static uint32 s_overrideActivationRadius;

struct BuildOperation
{
    enum OpType
    {
        OpType_GlobalEntities,
        OpType_Region,
        OpType_Entities,
        OpType_Terrain,
        OpType_Count,
    };

    BuildOperation(OpType type, int32 regionX, int32 regionY) : Type(type), RegionX(regionX), RegionY(regionY) {}

    OpType Type;
    int32 RegionX;
    int32 RegionY;
};

static MemArray<BuildOperation> s_buildOperations;

static bool ParseRegionCoordinates(const char *coordStr, int32 *pRegionX, int32 *pRegionY)
{
    uint32 coordStrLength = Y_strlen(coordStr);
    char *tempString = (char *)alloca(coordStrLength + 1);
    uint32 tempStringLength = 0;
    uint32 stringPosition = 0;
    uint32 coordIndex = 0;

    for (; stringPosition < coordStrLength; stringPosition++)
    {
        if (coordStr[stringPosition] == ',')
        {
            if (tempStringLength == 0)
                return false;

            tempString[tempStringLength] = 0;
            *pRegionX = StringConverter::StringToInt32(tempString);
            tempStringLength = 0;
            coordIndex++;
            continue;
        }

        tempString[tempStringLength++] = coordStr[stringPosition];
    }

    if (coordIndex == 0)
        return false;

    tempString[tempStringLength] = 0;
    *pRegionY = StringConverter::StringToInt32(tempString);
    return true;
}

static bool ParseArguments(int argc, char **argv)
{
#define CHECK_ARG(str) !Y_strcmp(argv[i], str)
#define CHECK_ARG_PARAM(str) !Y_strcmp(argv[i], str) && ((i + 1) < argc)

    // first argument should always be the map name
    if (argc < 1)
    {
        Log_ErrorPrintf("Missing map file name");
        return false;
    }

    // read map name
    s_inputFileName = argv[0];
    if (!s_inputFileName.EndsWith(".map", false))
    {
        Log_ErrorPrintf("Invalid map file name: '%s' (should end in .map)", s_inputFileName.GetCharArray());
        return false;
    }

    // build output map name
    s_outputFileName = s_inputFileName.SubString(0, s_inputFileName.GetLength() - 4);
    s_outputFileName.AppendString(".cmap");

    // fix for os
    FileSystem::BuildOSPath(s_inputFileName);
    FileSystem::BuildOSPath(s_outputFileName);

    // parse parameters
    for (int i = 1; i < argc; )
    {
        if (CHECK_ARG_PARAM("-OverrideLODLevels"))
        {
            s_overrideLODLevels = StringConverter::StringToUInt32(argv[++i]);
            if (s_overrideLODLevels == 0 || s_overrideLODLevels > 16)
            {
                Log_ErrorPrintf("Invalid override lod levels: %u", s_overrideLODLevels);
                return false;
            }
        }
        else if (CHECK_ARG_PARAM("-OverrideLoadRadius"))
        {
            s_overrideLoadRadius = StringConverter::StringToUInt32(argv[++i]);
            if (s_overrideLoadRadius == 0 || s_overrideLoadRadius > 16)
            {
                Log_ErrorPrintf("Invalid override load radius: %u", s_overrideLoadRadius);
                return false;
            }
        }
        else if (CHECK_ARG_PARAM("-OverrideActivationRadius"))
        {
            s_overrideActivationRadius = StringConverter::StringToUInt32(argv[++i]);
            if (s_overrideActivationRadius == 0 || s_overrideActivationRadius > 16)
            {
                Log_ErrorPrintf("Invalid override activation radius: %u", s_overrideActivationRadius);
                return false;
            }
        }
        else if (CHECK_ARG("-GlobalEntities"))
        {
            BuildOperation op(BuildOperation::OpType_Region, 0, 0);
            s_buildOperations.Add(op);
        }
        else if (CHECK_ARG_PARAM("-BuildRegion"))
        {
            BuildOperation op(BuildOperation::OpType_Region, 0, 0);
            if (!ParseRegionCoordinates(argv[++i], &op.RegionX, &op.RegionY))
            {
                Log_ErrorPrintf("Invalid region coordinates: '%s'", argv[i]);
                return false;
            }
            s_buildOperations.Add(op);
        }
        else if (CHECK_ARG_PARAM("-BuildRegionTerrain"))
        {
            BuildOperation op(BuildOperation::OpType_Terrain, 0, 0);
            if (!ParseRegionCoordinates(argv[++i], &op.RegionX, &op.RegionY))
            {
                Log_ErrorPrintf("Invalid region coordinates: '%s'", argv[i]);
                return false;
            }
            s_buildOperations.Add(op);
        }
        else
        {
            Log_ErrorPrintf("Invalid option: %s", argv[i]);
            return false;
        }

        i++;
    }

#undef CHECK_ARG
#undef CHECK_ARG_PARAM

    return true;
}

int RunMapCompiler(int argc, char **argv)
{
    MapSource *pMapSource = nullptr;
    MapCompiler *pMapCompiler = nullptr;
    ByteStream *pReadStream = nullptr;
    ByteStream *pWriteStream = nullptr;
    int exitCode = 0;

    Log::GetInstance().SetConsoleOutputParams(true);
    Log::GetInstance().SetDebugOutputParams(true);
    
    // Clear all stages.
    if (!ParseArguments(argc, argv))
    {
        Log_ErrorPrintf("Invalid command line arguments.");
        return false;
    }

    // Check input filename.
    Log_InfoPrintf("Map source file name: %s", s_inputFileName.GetCharArray());
    Log_InfoPrintf("Compiled map file name: %s", s_outputFileName.GetCharArray());

    // All stages require reading of the map file.
    {
        Log_InfoPrint("=============================== Reading Map File ==============================");
        ConsoleProgressCallbacks progressCallbacks;

        pMapSource = new MapSource();
        if (!pMapSource->Load(s_inputFileName, &progressCallbacks))
        {
            Log_ErrorPrintf("Failed to load map file '%s'", s_inputFileName.GetCharArray());
            exitCode = 1;
            goto CLEANUP_ERROR;
        }
    }

    // Create compiler
    pMapCompiler = new MapCompiler(pMapSource);

    // Check if the map file exists
    if (s_buildOperations.GetSize() > 0 && g_pVirtualFileSystem->FileExists(s_outputFileName))
    {
        Log_InfoPrint("================================ Reading Compiled Map ====================================");

        pReadStream = FileSystem::OpenFile(s_outputFileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_SEEKABLE);
        if (pReadStream == nullptr)
        {
            Log_ErrorPrintf("Failed to open compiled map file '%s' for reading", s_outputFileName.GetCharArray());
            exitCode = 2;
            goto CLEANUP_ERROR;
        }

        pWriteStream = FileSystem::OpenFile(s_outputFileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_ATOMIC_UPDATE | BYTESTREAM_OPEN_SEEKABLE);
        if (pWriteStream == nullptr)
        {
            Log_ErrorPrintf("Failed to open compiled map file '%s' for writing", s_outputFileName.GetCharArray());
            exitCode = 2;
            goto CLEANUP_ERROR;
        }

        // init compiler
        ConsoleProgressCallbacks progressCallbacks;
        if (!pMapCompiler->StartReuseCompile(pReadStream, pWriteStream, &progressCallbacks))
        {
            Log_ErrorPrintf("Failed to start reuse compile for map file '%s'", s_outputFileName.GetCharArray());
            exitCode = 2;
            goto CLEANUP_ERROR;
        }
    }
    else
    {
        Log_InfoPrint("================================ Creating Compiled Map ====================================");

        // open file for writing
        pWriteStream = FileSystem::OpenFile(s_outputFileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_ATOMIC_UPDATE | BYTESTREAM_OPEN_SEEKABLE);
        if (pWriteStream == nullptr)
        {
            Log_ErrorPrintf("Failed to open compiled map file '%s' for writing", s_outputFileName.GetCharArray());
            exitCode = 2;
            goto CLEANUP_ERROR;
        }

        // init compiler
        ConsoleProgressCallbacks progressCallbacks;
        if (!pMapCompiler->StartFreshCompile(pWriteStream, &progressCallbacks))
        {
            Log_ErrorPrintf("Failed to start fresh compile for map file '%s'", s_outputFileName.GetCharArray());
            exitCode = 2;
            goto CLEANUP_ERROR;
        }

        // set override lods
        if (s_overrideLODLevels != 0)
        {
            Log_WarningPrintf("Overriding LOD levels: %u. This may take a while.", s_overrideLODLevels);
            pMapCompiler->SetOverrideRegionLODLevels(s_overrideLODLevels);
        }
        if (s_overrideLoadRadius != 0)
        {
            Log_WarningPrintf("Overriding region load radius: %u.", s_overrideLoadRadius);
            pMapCompiler->SetOverrideRegionLoadRadius(s_overrideLoadRadius);
        }
        if (s_overrideActivationRadius != 0)
        {
            Log_WarningPrintf("Overriding region activation radius: %u.", s_overrideActivationRadius);
            pMapCompiler->SetOverrideRegionActivationRadius(s_overrideActivationRadius);
        }
    }

    {
        Log_InfoPrint("============================= Compiling ================================");
        
        ConsoleProgressCallbacks progressCallbacks;
        if (s_buildOperations.GetSize() > 0)
        {
            // issue operations
            for (uint32 i = 0; i < s_buildOperations.GetSize(); i++)
            {
                const BuildOperation &op = s_buildOperations[i];
                if (op.Type == BuildOperation::OpType_GlobalEntities)
                {
                    Log_InfoPrintf("Building global entities...");
                    if (!pMapCompiler->BuildGlobalEntities(&progressCallbacks))
                    {
                        Log_ErrorPrintf("Build global entities failed.");
                        exitCode = 3;
                        goto CLEANUP_ERROR;
                    }
                }
                else if (op.Type == BuildOperation::OpType_Region)
                {
                    Log_InfoPrintf("Building region [%i, %i]...", op.RegionX, op.RegionY);
                    if (!pMapCompiler->BuildRegion(op.RegionX, op.RegionY, &progressCallbacks))
                    {
                        Log_ErrorPrintf("Build region [%i, %i] failed.", op.RegionX, op.RegionY);
                        exitCode = 3;
                        goto CLEANUP_ERROR;
                    }
                }
                else if (op.Type == BuildOperation::OpType_Entities)
                {
                    Log_InfoPrintf("Building region [%i, %i] entities...", op.RegionX, op.RegionY);
                    if (!pMapCompiler->BuildRegionEntities(op.RegionX, op.RegionY, &progressCallbacks))
                    {
                        Log_ErrorPrintf("Build region [%i, %i] entities failed.", op.RegionX, op.RegionY);
                        exitCode = 3;
                        goto CLEANUP_ERROR;
                    }
                }
                else if (op.Type == BuildOperation::OpType_Terrain)
                {
                    Log_InfoPrintf("Building region [%i, %i] terrain...", op.RegionX, op.RegionY);
                    if (!pMapCompiler->BuildRegionTerrain(op.RegionX, op.RegionY, &progressCallbacks))
                    {
                        Log_ErrorPrintf("Build region [%i, %i] terrain failed.", op.RegionX, op.RegionY);
                        exitCode = 3;
                        goto CLEANUP_ERROR;
                    }
                }
            }
        }
        else
        {
            // build the whole thing
            if (!pMapCompiler->BuildAll(-1, &progressCallbacks))
            {
                Log_ErrorPrintf("Build all failed.");
                exitCode = 3;
                goto CLEANUP_ERROR;
            }
        }
    }


    {
        Log_InfoPrint("============================= Finalizing ======================================");
        ConsoleProgressCallbacks progressCallbacks;

        if (!pMapCompiler->FinalizeCompile(&progressCallbacks))
        {
            Log_ErrorPrintf("Failed to run finalize.");
            exitCode = 4;
            goto CLEANUP_ERROR;
        }
    }

    // commit to destination
    Log_InfoPrint("=========================== Cleaning up =======================================");
    delete pMapCompiler;
    delete pMapSource;
    if (pReadStream != nullptr)
        pReadStream->Release();
    if (!pWriteStream->Commit())
    {
        pWriteStream->Release();
        Log_WarningPrintf("Failed to commit output file");
        exitCode = 5;
        goto END;
    }

    pWriteStream->Release();
    goto END;

CLEANUP_ERROR:
    Log_InfoPrint("=========================== Cleaning up =======================================");
    if (pMapCompiler != nullptr)
    {
        pMapCompiler->DiscardCompile();
        delete pMapCompiler;
    }
    delete pMapSource;
    if (pWriteStream != nullptr)
    {
        pWriteStream->Discard();
        pWriteStream->Release();
    }
    if (pReadStream != nullptr)
        pReadStream->Release();

    goto END;

END:
    if (exitCode == 0)
        Log_InfoPrint("Exiting with success.");
    else
        Log_ErrorPrintf("Exiting with error code %d.", exitCode);

    return exitCode;
}

int main(int argc, char *argv[])
{
    // set log flags
    g_pLog->SetConsoleOutputParams(true);
    g_pLog->SetDebugOutputParams(true);

    // parse command line
    uint32 argsStart = g_pConsole->ParseCommandLine(argc, (const char **)argv);

    // adjust pointers
    int newArgc = argc - argsStart;
    char **newArgv = argv + argsStart;

    // initialize VFS
    if (!g_pVirtualFileSystem->Initialize())
    {
        Log_ErrorPrintf("VFS startup failed. Cannot continue.");
        return false;
    }

    // initialize engine
    if (!g_pEngine->Startup())
    {
        Log_ErrorPrintf("Engine startup failed. Cannot continue.");
        g_pVirtualFileSystem->Shutdown();
        return false;
    }

    // register types
    g_pEngine->RegisterEngineTypes();

    // run modules
    int returnCode = RunMapCompiler(newArgc, newArgv);

    // shutdown everything
    g_pEngine->Shutdown();
    g_pEngine->UnregisterTypes();
    g_pVirtualFileSystem->Shutdown();
    return returnCode;
}
