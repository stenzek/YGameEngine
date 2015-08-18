#include "Core/PrecompiledHeader.h"
#include "Core/XDisplay.h"

#if defined(Y_PLATFORM_LINUX)

#include "YBaseLib/Assert.h"
#include "YBaseLib/Log.h"
#include <cstdlib>
Log_SetChannel(XDisplay);

static Display *g_pXDisplay = NULL;

void Y_AtExitCloseXDisplay()
{
    if (g_pXDisplay != NULL)
    {
        XCloseDisplay(g_pXDisplay);
        g_pXDisplay = NULL;
    }
}

void Y_SetXDisplayPointer(Display *pXDisplay)
{
    DebugAssert((g_pXDisplay == NULL && pXDisplay != NULL) || (g_pXDisplay != NULL && pXDisplay == NULL));
    g_pXDisplay = pXDisplay;
}

bool Y_CheckXDisplayPointer()
{
    if (g_pXDisplay == NULL)
    {
        g_pXDisplay = XOpenDisplay(NULL);
        if (g_pXDisplay == NULL)
        {
            Log_ErrorPrintf("Could not connect to X server.");
            return false;
        }

        atexit(Y_AtExitCloseXDisplay);
    }
    
    return true;
}

Display *Y_GetXDisplayPointer()
{
    return g_pXDisplay;
}

#endif      // Y_PLATFORM_LINUX
