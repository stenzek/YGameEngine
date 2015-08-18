#include "DemoGame/PrecompiledHeader.h"
#include "DemoGame/DemoGame.h"
Log_SetChannel(Main);

// Pretty much copied from the SDL header
#if defined(__WIN32__) || defined(__WINRT__) || defined(__IPHONEOS__) || defined(__ANDROID__)
    #define main SDL_main
#endif

// SDL requires the entry point declared without c++ decoration
extern "C" int main(int argc, char *argv[])
{
    // change gamename
    g_pConsole->SetCVarByName("vfs_gamedir", "DemoGame", true);
    g_pConsole->ApplyPendingAppCVars();

    // set log flags
    g_pLog->SetConsoleOutputParams(true);
    g_pLog->SetDebugOutputParams(true);

#if defined(__WIN32__)
    // fix up stdout/stderr on win32
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#endif

    // parse command line
    g_pConsole->ParseCommandLine(argc, (const char **)argv);
    g_pConsole->ApplyPendingAppCVars();

    // create game instance, and run it
    DemoGame *pDemoGame = new DemoGame();
    int32 result = pDemoGame->MainEntryPoint();

    // game has ended, so cleanup
    delete pDemoGame;
    return result;
}
