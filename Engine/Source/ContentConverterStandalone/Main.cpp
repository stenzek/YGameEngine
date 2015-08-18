#include "PrecompiledHeader.h"
#include "ContentConverter.h"
#include "Engine/Common.h"
#include "Engine/Engine.h"
Log_SetChannel(ContentConverter);

int ModuleRedirector(int argc, char **argv)
{
    if (argc < 1)
    {
        Log_ErrorPrintf("Expected at least a module name.");
        return 2;
    }

    // restore log level
    g_pLog->SetConsoleOutputParams(true, NULL, LOGLEVEL_PROFILE);

    // parse first arg
    char *moduleName = argv[0];
    argv = ((--argc) > 0) ? argv + 1 : NULL;

    if (!Y_stricmp(moduleName, "OBJImporter"))
    {
        Log_InfoPrintf("Using OBJImporter module.");
        return RunOBJImporter(argc, argv);
    }
    else if (!Y_stricmp(moduleName, "TextureImporter"))
    {
        Log_InfoPrintf("Using TextureImporter module.");
        return RunTextureImporter(argc, argv);
    }
    else if (!Y_stricmp(moduleName, "FontImporter"))
    {
        Log_InfoPrintf("Using FontImporter module.");
        return RunFontImporter(argc, argv);
    }
    else if (!Y_stricmp(moduleName, "AssimpSkeletonImporter"))
    {
        Log_InfoPrintf("Using AssimpSkeletonImporter module.");
        return RunAssimpSkeletonImporter(argc, argv);
    }
    else if (!Y_stricmp(moduleName, "AssimpSkeletalMeshImporter"))
    {
        Log_InfoPrintf("Using AssimpSkeletalMeshImporter module.");
        return RunAssimpSkeletalMeshImporter(argc, argv);
    }
    else if (!Y_stricmp(moduleName, "AssimpSkeletalAnimationImporter"))
    {
        Log_InfoPrintf("Using AssimpSkeletalAnimationImporter module.");
        return RunAssimpSkeletalAnimationImporter(argc, argv);
    }
    else if (!Y_stricmp(moduleName, "AssimpStaticMeshImporter"))
    {
        Log_InfoPrintf("Using AssimpStaticMeshImporter module.");
        return RunAssimpStaticMeshImporter(argc, argv);
    }
    else if (!Y_stricmp(moduleName, "AssimpSceneImporter"))
    {
        Log_InfoPrintf("Using AssimpSceneImporter module.");
        return RunAssimpSceneImporter(argc, argv);
    }
    else
    {
        Log_ErrorPrintf("Invalid module name specified.");
        return 2;
    }
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
    int returnCode = ModuleRedirector(newArgc, newArgv);

    // shutdown everything
    g_pEngine->Shutdown();
    g_pEngine->UnregisterTypes();
    g_pVirtualFileSystem->Shutdown();
    return returnCode;
}
