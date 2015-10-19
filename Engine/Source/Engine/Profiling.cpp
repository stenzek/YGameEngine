#include "Engine/PrecompiledHeader.h"
#include "Engine/SDLHeaders.h"
#include "Engine/ResourceManager.h"
#include "Renderer/Renderer.h"
Log_SetChannel(Profiling);

#ifdef WITH_PROFILER

#define MICROPROFILE_IMPL 1
#define MICROPROFILEUI_IMPL 1
#define MICROPROFILE_WEBSERVER 0
#define MICROPROFILE_CONTEXT_SWITCH_TRACE 0

#ifdef Y_COMPILER_MSVC
    #pragma warning(push)
    #pragma warning(disable: 4127)      // warning C4127: conditional expression is constant
    #pragma warning(disable: 4512)      // warning C4512: 'MicroProfileScopeLock' : assignment operator could not be generated
    #pragma warning(disable: 4244)      // warning C4244: 'argument' : conversion from 'float' to 'int', possible loss of data
    #pragma warning(disable: 4245)      // warning C4245: '=' : conversion from 'int' to 'uint32_t', signed/unsigned mismatch
    #pragma warning(disable: 4018)      // warning C4018: '<' : signed/unsigned mismatch
    #pragma warning(disable: 4389)      // warning C4389: '==' : signed/unsigned mismatch
    #pragma warning(disable: 4267)      // warning C4267: 'initializing' : conversion from 'size_t' to 'uint32_t', possible loss of data (Engine\Profiling.cpp)
    #pragma warning(disable: 4456)      // warning C4456: declaration of 'nLen' hides previous local declaration (compiling source file Engine\Profiling.cpp)
    #pragma warning(disable: 4457)      // warning C4457: declaration of 'nWidth' hides function parameter
    #include "microprofile.h"
    #include "microprofileui.h"
    #pragma warning(pop)
#else
    #include "microprofile.h"
    #include "microprofileui.h"
#endif

#include "Engine/Profiling.h"

#define TIMESTAMP_QUERY_COUNT (8<<10) // 8192

static MiniGUIContext s_profilerGUIContext;

static bool s_GPUTimingInitialized = false;
static GPUQuery *s_pGPUTimestampQueries[TIMESTAMP_QUERY_COUNT] = { nullptr };
static GPUQuery *s_pGPUFrequencyQuery = nullptr;
static bool s_GPUTimingAvailable = false;
static uint32 s_lastGPUTimestampQuery = 0;
static uint64 s_lastGPUFrequency = 1;

static void InitializeGPUTiming()
{
    DebugAssert(Renderer::IsOnRenderThread());

    if (!s_GPUTimingInitialized)
    {
        s_GPUTimingInitialized = true;

        // create gpu frequency query
        s_pGPUFrequencyQuery = g_pRenderer->CreateQuery(GPU_QUERY_TYPE_FREQUENCY);
        if (s_pGPUFrequencyQuery == nullptr)
        {
            Log_ErrorPrintf("InitializeGPUTiming: Failed to create GPU frequency query.");
            s_GPUTimingAvailable = false;
            return;
        }

        // create timestamp queries
        for (uint32 i = 0; i < TIMESTAMP_QUERY_COUNT; i++)
        {
            if ((s_pGPUTimestampQueries[i] = g_pRenderer->CreateQuery(GPU_QUERY_TYPE_TIMESTAMP)) == nullptr)
            {
                for (uint32 j = 0; j < i; j++)
                {
                    s_pGPUTimestampQueries[j]->Release();
                    s_pGPUTimestampQueries[j] = nullptr;
                }

                Log_ErrorPrintf("InitializeGPUTiming: Failed to create GPU timestamp queries.");
                s_pGPUFrequencyQuery->Release();
                s_pGPUFrequencyQuery = nullptr;
                s_GPUTimingAvailable = false;
                return;
            }
        }

        s_GPUTimingAvailable = true;

        // kick off frequency query
        g_pRenderer->GetGPUContext()->BeginQuery(s_pGPUFrequencyQuery);
    }
}

void Profiling::Initialize()
{
    MicroProfileInit();
    MicroProfileInitUI();
    MicroProfileSetEnableAllGroups(true);
    MicroProfileSetDisplayMode(MP_DRAW_OFF);
    MicroProfileTogglePause();

    QUEUE_BLOCKING_RENDERER_LAMBA_COMMAND([]() {
        s_profilerGUIContext.SetGPUContext(g_pRenderer->GetGPUContext());
        s_profilerGUIContext.SetAlphaBlendingEnabled(true);
        s_profilerGUIContext.SetAlphaBlendingMode(MiniGUIContext::ALPHABLENDING_MODE_STRAIGHT);
    });
}

void Profiling::StartFrame()
{

}

bool Profiling::HandleSDLEvent(const void *pEvent)
{
    if (!g_MicroProfile.nDisplay)
        return false;

    const SDL_Event *pSDLEvent = reinterpret_cast<const SDL_Event *>(pEvent);
    switch (pSDLEvent->type)
    {
    case SDL_MOUSEMOTION:
        MicroProfileMousePosition(pSDLEvent->motion.x, pSDLEvent->motion.y, 0);
        return true;

    case SDL_MOUSEWHEEL:
        {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            MicroProfileMousePosition(mx, my, pSDLEvent->wheel.y);
            return true;
        }

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        {
            uint32 mask = SDL_GetMouseState(nullptr, nullptr);
            bool left = (mask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
            bool right = (mask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
            MicroProfileMouseButton((uint32_t)left, (uint32_t)right);
            return true;
        }

    case SDL_KEYDOWN:
    case SDL_KEYUP:
        MicroProfileModKey((uint32_t)((SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL)) != 0));
        return false;
    }

    return false;
}

bool Profiling::GetProfilerEnabled()
{
    return (g_MicroProfile.nRunning != 0);
}

void Profiling::SetProfilerEnabled(bool enabled)
{
    //DebugAssert(IsOnMainThread());
    if (g_MicroProfile.nRunning == (uint32_t)enabled)
    {
        if (g_MicroProfile.nToggleRunning)
            g_MicroProfile.nToggleRunning = false;

        return;
    }
    else
    {
        if (g_MicroProfile.nToggleRunning)
            return;

        MicroProfileTogglePause();
    }
}

bool Profiling::IsProfilerDisplayEnabled()
{
    return (g_MicroProfile.nDisplay != MP_DRAW_OFF);
}

bool Profiling::SetProfilerDisplay(uint32 mode)
{
    //DebugAssert(IsOnMainThread());
    MicroProfileSetDisplayMode(mode);
    return (g_MicroProfile.nDisplay == MP_DRAW_DETAILED);
}

bool Profiling::ToggleProfilerDisplay()
{
    //DebugAssert(IsOnMainThread());
    MicroProfileToggleDisplayMode();
    return (g_MicroProfile.nDisplay == MP_DRAW_DETAILED);
}

void Profiling::HideProfilerDisplay()
{
    //DebugAssert(IsOnMainThread());
    MicroProfileSetDisplayMode(MP_DRAW_OFF);
}
void Profiling::DrawDisplay()
{
    DebugAssert(Renderer::IsOnRenderThread());
    GPUContext *pGPUContext = g_pRenderer->GetGPUContext();

    if (g_MicroProfile.nDisplay)
    {
        pGPUContext->SetRenderTargets(0, nullptr, nullptr);
        pGPUContext->SetFullViewport();

        const RENDERER_VIEWPORT *pViewport = pGPUContext->GetViewport();
        s_profilerGUIContext.SetViewportDimensions(pViewport->Width, pViewport->Height);
        s_profilerGUIContext.PushManualFlush();
        MicroProfileDraw(pViewport->Width, pViewport->Height);
        s_profilerGUIContext.PopManualFlush();
    }

    // this is on the render thread, so it's as good time as any to poll the frequency
    if (g_MicroProfile.nRunning && s_GPUTimingAvailable)
    {
        // end frequency query
        pGPUContext->EndQuery(s_pGPUFrequencyQuery);

        // get results
        GPU_QUERY_GETDATA_RESULT result = pGPUContext->GetQueryData(s_pGPUFrequencyQuery, &s_lastGPUFrequency, sizeof(s_lastGPUFrequency), 0);
        while (result == GPU_QUERY_GETDATA_RESULT_NOT_READY)
            result = pGPUContext->GetQueryData(s_pGPUFrequencyQuery, &s_lastGPUFrequency, sizeof(s_lastGPUFrequency), 0);

        if (result != GPU_QUERY_GETDATA_RESULT_OK)
        {
            // error?
            Log_WarningPrintf("Profiling::DrawDisplay: Polling GPU frequency failed.");
            s_lastGPUFrequency = 1;
        }
        else if (s_lastGPUFrequency == 0)
        {
            Log_WarningPrintf("Profiling::DrawDisplay: GPU frequency changed, timestamps will be incorrect.");
            s_lastGPUFrequency = 1;
        }

        // kick off next frequency query
        pGPUContext->BeginQuery(s_pGPUFrequencyQuery);
    }
}

void Profiling::EndFrame()
{
    // activating?
    if (!g_MicroProfile.nRunning && g_MicroProfile.nToggleRunning && !s_GPUTimingInitialized)
    {
        QUEUE_BLOCKING_RENDERER_LAMBA_COMMAND([]() {
            InitializeGPUTiming();
        });
    }

    // pass through - force execution on render thread
    if (g_MicroProfile.nRunning || g_MicroProfile.nToggleRunning)
    {
        QUEUE_BLOCKING_RENDERER_LAMBA_COMMAND([]() {
            MicroProfileFlip(nullptr);
        });
    }
}

//////////////////////////////////////////////////////////////////////////
void MicroProfileDrawText(int nX, int nY, uint32_t nColor, const char* pText, uint32_t nNumCharacters)
{
    static const Font *pFont = g_pResourceManager->GetFont("resources/engine/fonts/microprofile");
    s_profilerGUIContext.DrawTextAt(nX, nY, pFont, 10, nColor, pText);
}

void MicroProfileDrawBox(int nX, int nY, int nX1, int nY1, uint32_t nColor, MicroProfileBoxType type)
{
    MINIGUI_RECT rect(nX, nX1, nY, nY1);
    s_profilerGUIContext.DrawFilledRect(&rect, nColor);
}

void MicroProfileDrawLine2D(uint32_t nVertices, float* pVertices, uint32_t nColor)
{
    DebugAssert(nVertices > 2);

    int2 lastVertex((int32)pVertices[0], (int32)pVertices[1]);
    for (uint32 i = 2; i < nVertices; i += 2)
    {
        // @todo remove float-int conversion
        int2 thisVertex((int32)pVertices[i], (int32)pVertices[i + 1]);
        s_profilerGUIContext.DrawLine(lastVertex, thisVertex, nColor);
        lastVertex = thisVertex;
    }
}

uint32_t MicroProfileGpuInsertTimeStamp(void *pContext)
{
    if (!s_GPUTimingAvailable)
        return (uint32_t)-1;

    // get context
    GPUContext *pGPUContext = g_pRenderer->GetGPUContext();
    DebugAssert(Renderer::IsOnRenderThread());

    // get query
    uint32 queryIndex = s_lastGPUTimestampQuery;
    GPUQuery *pThisQuery = s_pGPUTimestampQueries[queryIndex];
    s_lastGPUTimestampQuery = (s_lastGPUTimestampQuery + 1) % countof(s_pGPUTimestampQueries);

    // issue begin/end
    pGPUContext->BeginQuery(pThisQuery);
    pGPUContext->EndQuery(pThisQuery);
    return queryIndex;
}

uint64_t MicroProfileGpuGetTimeStamp(uint32_t nKey)
{
    if (!s_GPUTimingAvailable || nKey == (uint32_t)-1)
        return 0;

    GPUContext *pGPUContext = g_pRenderer->GetGPUContext();
    DebugAssert(Renderer::IsOnRenderThread());

    // get query
    DebugAssert(nKey < countof(s_pGPUTimestampQueries) && s_pGPUTimestampQueries[nKey] != nullptr);
    GPUQuery *pThisQuery = s_pGPUTimestampQueries[nKey];
    uint64_t timestampValue;

    // read results
    GPU_QUERY_GETDATA_RESULT result = pGPUContext->GetQueryData(pThisQuery, &timestampValue, sizeof(timestampValue), 0);
    while (result == GPU_QUERY_GETDATA_RESULT_NOT_READY)
        result = pGPUContext->GetQueryData(pThisQuery, &timestampValue, sizeof(timestampValue), 0);

    // return result
    return (result == GPU_QUERY_GETDATA_RESULT_OK) ? timestampValue : 0;
}

uint64_t MicroProfileTicksPerSecondGpu()
{
    return s_lastGPUFrequency;
}

int MicroProfileGetGpuTickReference(int64_t* pOutCPU, int64_t* pOutGpu)
{
    return 0;
}

// const char* MicroProfileGetThreadName()
// {
//     return "Thread";
// }

#else       // WITH_PROFILER

#include "Engine/Profiling.h"

void Profiling::Initialize()
{

}

void Profiling::StartFrame()
{

}

bool Profiling::HandleSDLEvent(const void *pEvent)
{
    return false;
}

bool Profiling::GetProfilerEnabled()
{
    return false;
}

void Profiling::SetProfilerEnabled(bool enabled)
{

}

bool Profiling::IsProfilerDisplayEnabled()
{
    return false;
}

bool Profiling::SetProfilerDisplay(uint32 mode)
{
    return false;
}

bool Profiling::ToggleProfilerDisplay()
{
    return false;
}

void Profiling::HideProfilerDisplay()
{

}
void Profiling::DrawDisplay()
{

}

void Profiling::EndFrame()
{

}

#endif      // WITH_PROFILER
