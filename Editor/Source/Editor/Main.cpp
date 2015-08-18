#include "Editor/PrecompiledHeader.h"
#include "Editor/Editor.h"
Log_SetChannel(EditorMain);

#if defined(Y_PLATFORM_LINUX) && 0
    
    #include <qpa/qplatformnativeinterface.h>
    #include "Core/XDisplay.h"
    
    static bool UpdateXDisplayPointer(QApplication *pApplication)
    {
        QPlatformNativeInterface *pNativeInterface = pApplication->platformNativeInterface();
        if (pNativeInterface == NULL)
        {
            Log_ErrorPrintf("UpdateXDisplayPointer(): Could not get platform native interface.");
            return false;
        }

        Display *pX11Display = reinterpret_cast<Display *>(pNativeInterface->nativeResourceForScreen(QByteArray("display"), QGuiApplication::primaryScreen()));
        if (pX11Display == NULL)
        {
            Log_ErrorPrintf("UpdateXDisplayPointer(): Could not get X11 connection from Qt.");
            return false;
        }

        Y_SetXDisplayPointer(pX11Display);
        return true;
    }

#endif


int main(int argc, char *argv[])
{
    // set log flags
    g_pLog->SetConsoleOutputParams(true);
    g_pLog->SetDebugOutputParams(true);

    // create application
    Editor editor(argc, argv);

    // platform-specific initializtion
#if defined(Y_PLATFORM_LINUX) && 0
    UpdateXDisplayPointer(&editor);
#endif

    // pass control over
    return editor.Execute();
}

