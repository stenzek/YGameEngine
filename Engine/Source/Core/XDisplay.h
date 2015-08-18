#pragma once
#include "Core/Common.h"

#if defined(Y_PLATFORM_LINUX)

#include <X11/X.h>
#include <X11/Xlib.h>

void Y_SetXDisplayPointer(Display *pXDisplay);
bool Y_CheckXDisplayPointer();
Display *Y_GetXDisplayPointer();

#endif      // Y_PLATFORM_LINUX
