#include "Engine/PrecompiledHeader.h"
#include "Engine/FPSCounter.h"
#include "Engine/ScriptManager.h"
#include "Renderer/MiniGUIContext.h"
#include "Renderer/Renderer.h"

FPSCounter::FPSCounter()
    : m_pGPUContext(nullptr),
      m_accumulatedTime(0.0),
      m_framesRendered(0),
      m_fps(0),
      m_lastFrameTime(0),
      m_lastGameFrameTime(0),
      m_lastRenderFrameTime(0),
      m_lastFrameTimesIndex(0),
      m_lastDrawCallCount(0),
      m_lastMemoryUsage(0),
      m_lastScriptMemoryUsage(0)
{
    Y_memzero(m_lastFrameTimes, sizeof(m_lastFrameTimes));
}

FPSCounter::~FPSCounter()
{
    ReleaseGPUResources();
}

bool FPSCounter::CreateGPUResources()
{
    return true;
}

void FPSCounter::ReleaseGPUResources()
{

}

void FPSCounter::GetFrameTimeHistory(float *pFrameTimes, uint32 frameCount) const
{
    uint32 currentIndex = (m_lastFrameTimesIndex == 0) ? KEEP_FRAME_TIME_COUNT - 1 : m_lastFrameTimesIndex - 1;
    for (uint32 i = 0; i < frameCount; i++)
    {
        pFrameTimes[i] = m_lastFrameTimes[currentIndex];
        if (currentIndex == 0)
            currentIndex = KEEP_FRAME_TIME_COUNT - 1;
        else
            currentIndex--;
    }
}

void FPSCounter::BeginFrame()
{
    m_lastFrameTime = (float)m_lastFrameStartTimer.GetTimeSeconds();
    m_lastFrameStartTimer.Reset();
    m_gameThreadTimer.Reset();
    m_renderThreadTimer.Reset();
    //m_pGPUContext->ResetDrawCallCounter();

    // add to history
    m_lastFrameTimesIndex %= KEEP_FRAME_TIME_COUNT;
    m_lastFrameTimes[m_lastFrameTimesIndex++] = m_lastFrameTime;

    // update fps
    // todo: seperate timer for this, between invocations?
    m_accumulatedTime += m_lastFrameTime;
    m_framesRendered++;
    if (m_accumulatedTime > 0.1)
    {
        m_fps = (((float)m_framesRendered) / m_accumulatedTime);
        m_accumulatedTime = 0.0;
        m_framesRendered = 0;
    }

    // update last memory usage
    m_lastMemoryUsage = Platform::GetProgramMemoryUsage();
    m_lastScriptMemoryUsage = g_pScriptManager->GetMemoryUsage();
}

void FPSCounter::EndGameThreadFrame()
{
    m_lastGameFrameTime = (float)m_gameThreadTimer.GetTimeSeconds();
}

void FPSCounter::EndRenderThreadFrame()
{
    m_lastRenderFrameTime = (float)m_renderThreadTimer.GetTimeSeconds();
    //m_lastDrawCallCount = m_pGPUContext->GetDrawCallCounter();
}

void FPSCounter::DrawDetails(const Font *pFont, MiniGUIContext *pGUIContext, int32 startX /* = -100 */, int32 startY /* = 0 */, uint32 fontSize /* = 16 */) const
{
    double minFrameTime = m_lastFrameTimes[0];
    double maxFrameTime = m_lastFrameTimes[0];
    double avgFrameTime = m_lastFrameTimes[0];
    for (uint32 i = 1; i < KEEP_FRAME_TIME_COUNT; i++)
    {
        double frameTime = m_lastFrameTimes[i];
        minFrameTime = Min(minFrameTime, frameTime);
        maxFrameTime = Max(maxFrameTime, frameTime);
        avgFrameTime += frameTime;
    }
    avgFrameTime /= (double)KEEP_FRAME_TIME_COUNT;

    SmallString message;
    message.Format("%.2f fps (%.2f ms) [%.2f-%.2f, avg %.2f]", m_fps, m_lastFrameTime * 1000.0, minFrameTime * 1000.0, maxFrameTime * 1000.0, avgFrameTime * 1000.0);

    // determine color
    uint32 color;
    if (avgFrameTime >= 16.0)
        color = MAKE_COLOR_R8G8B8A8_UNORM(255, 0, 0, 255);
    else if (avgFrameTime >= 8.0)
        color = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 255);
    else 
        color = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255);

    pGUIContext->DrawText(pFont, fontSize, startX, startY, message, color, false, MINIGUI_HORIZONTAL_ALIGNMENT_LEFT, MINIGUI_VERTICAL_ALIGNMENT_TOP);

    // second line
    message.Format("game: %.2f ms render: %.2f ms draws: %u", m_lastGameFrameTime * 1000.0, m_lastRenderFrameTime * 1000.0, m_lastDrawCallCount);
    pGUIContext->DrawText(pFont, fontSize, startX, startY + fontSize, message, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), false, MINIGUI_HORIZONTAL_ALIGNMENT_LEFT, MINIGUI_VERTICAL_ALIGNMENT_TOP);

    message.Format("script: %s mem: %s", StringConverter::SizeToHumanReadableString((uint64)m_lastScriptMemoryUsage).GetCharArray(), StringConverter::SizeToHumanReadableString((uint64)m_lastMemoryUsage).GetCharArray());
    pGUIContext->DrawText(pFont, fontSize, startX, startY + fontSize * 2, message, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), false, MINIGUI_HORIZONTAL_ALIGNMENT_LEFT, MINIGUI_VERTICAL_ALIGNMENT_TOP);
}
