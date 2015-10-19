#pragma once
#include "Engine/Common.h"

#ifdef WITH_PROFILER

#ifdef Y_COMPILER_MSVC
    #pragma warning(push)
    #pragma warning(disable: 4127)      // warning C4127: conditional expression is constant
    #include "microprofile.h"
    #pragma warning(pop)
#else
    #include "microprofile.h"
#endif

#define MICROPROFILE_COLOR(r, g, b) ((uint32)0xFF000000 | ((uint32)(r) << 16) | ((uint32)(g) << 8) | ((uint32)(b)) )

#endif      // WITH_PROFILER

namespace Profiling
{
    bool GetProfilerEnabled();
    void SetProfilerEnabled(bool enabled);

    bool IsProfilerDisplayEnabled();
    bool SetProfilerDisplay(uint32 mode);
    bool ToggleProfilerDisplay();
    void HideProfilerDisplay();

    void Initialize();
    void StartFrame();

    // Returns true if the event was used, false otherwise.
    bool HandleSDLEvent(const void *pEvent);

    void DrawDisplay();
    void EndFrame();
}

