#include "BaseGame/PrecompiledHeader.h"
#include "BaseGame/LoadingScreenProgressCallbacks.h"
#include "Renderer/Renderer.h"
#include "Renderer/MiniGUIContext.h"
Log_SetChannel(LoadingScreenProgressCallbacks);

LoadingScreenProgressCallbacks::LoadingScreenProgressCallbacks(uint32 offsetX /* = 0 */, uint32 offsetY /* = 0 */, uint32 fontHeight /* = 16 */, uint32 barWidth /* = 300 */, uint32 barHeight /* = 20 */)
    : m_offsetX(offsetX)
    , m_offsetY(offsetY)
    , m_fontHeight(fontHeight)
    , m_barWidth(barWidth)
    , m_barHeight(barHeight)
{

}

LoadingScreenProgressCallbacks::~LoadingScreenProgressCallbacks()
{

}

void LoadingScreenProgressCallbacks::PushState()
{
    BaseProgressCallbacks::PushState();
}

void LoadingScreenProgressCallbacks::PopState()
{
    BaseProgressCallbacks::PopState();
    Redraw();
}

void LoadingScreenProgressCallbacks::SetCancellable(bool cancellable)
{
    BaseProgressCallbacks::SetCancellable(cancellable);
    Redraw();
}

void LoadingScreenProgressCallbacks::SetStatusText(const char *statusText)
{
    BaseProgressCallbacks::SetStatusText(statusText);
    Redraw();
}

void LoadingScreenProgressCallbacks::SetProgressRange(uint32 range)
{
    uint32 lastRange = m_progressRange;

    BaseProgressCallbacks::SetProgressRange(range);

    if (m_progressRange != lastRange)
        Redraw();
}

void LoadingScreenProgressCallbacks::SetProgressValue(uint32 value)
{
    uint32 lastValue = m_progressValue;

    BaseProgressCallbacks::SetProgressValue(value);

    if (m_progressValue != lastValue && m_timeSinceLastDraw.GetTimeSeconds() >= 0.01f)
    {
        m_timeSinceLastDraw.Reset();
        Redraw();
    }
}

void LoadingScreenProgressCallbacks::DisplayError(const char *message)
{
    Log_ErrorPrint(message);
}

void LoadingScreenProgressCallbacks::DisplayWarning(const char *message)
{
    Log_WarningPrint(message);
}

void LoadingScreenProgressCallbacks::DisplayInformation(const char *message)
{
    Log_InfoPrint(message);
}

void LoadingScreenProgressCallbacks::DisplayDebugMessage(const char *message)
{
    Log_DevPrint(message);
}

void LoadingScreenProgressCallbacks::ModalError(const char *message)
{
    Log_ErrorPrint(message);
}

bool LoadingScreenProgressCallbacks::ModalConfirmation(const char *message)
{
    Log_InfoPrint(message);
    return false;
}

uint32 LoadingScreenProgressCallbacks::ModalPrompt(const char *message, uint32 nOptions, ...)
{
    DebugAssert(nOptions > 0);
    Log_InfoPrint(message);
    return 0;
}

void LoadingScreenProgressCallbacks::Redraw()
{
    // singlethreaded rendering?
    if (Renderer::IsOnRenderThread())
    {
        RedrawRenderThread();
        return;
    }

    // multithreaded rendering
    QUEUE_BLOCKING_RENDERER_LAMBA_COMMAND([this]() {
        RedrawRenderThread();
    });
}

void LoadingScreenProgressCallbacks::RedrawRenderThread()
{
    GPUContext *pGPUContext = g_pRenderer->GetGPUContext();
    MiniGUIContext *pGUIContext = g_pRenderer->GetGUIContext();

    // assuming correct targets are already set
    MINIGUI_RECT rect;
    SmallString text;

    // calc percent
    float percentComplete = (m_progressRange > 0) ? ((float)m_progressValue / (float)m_progressRange) * 100.0f : 0.0f;
    if (percentComplete > 100.0f)
        percentComplete = 100.0f;

    // clear screen
    pGPUContext->SetRenderTargets(0, nullptr, nullptr);
    pGPUContext->SetFullViewport();
    pGPUContext->ClearTargets(true, true, true, float4(0.0f, 0.0f, 0.2f, 1.0f));
    pGUIContext->SetViewportDimensions(pGPUContext->GetViewport());

    // start batching
    pGUIContext->PushManualFlush();

    // draw the status text
    pGUIContext->DrawText(g_pRenderer->GetFixedResources()->GetDebugFont(), m_fontHeight, m_offsetX, m_offsetY, m_statusText);

    // draw percent next to it for now
    text.Format("%.2f%% complete", percentComplete);
    pGUIContext->DrawText(g_pRenderer->GetFixedResources()->GetDebugFont(), m_fontHeight, m_offsetX + m_barWidth + 2 + 20, m_offsetY + (m_fontHeight / 2) + 4 + 1 + (m_barHeight / 2), text);

    // draw the progress bar outline
    SET_MINIGUI_RECT(&rect, m_offsetX, m_offsetX + m_barWidth + 2, m_offsetY + m_fontHeight + 4, m_offsetY + m_fontHeight + 4 + m_barHeight + 2);
    pGUIContext->DrawRect(&rect, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));

    // bar background
    rect.left++; rect.right--; rect.top++; rect.bottom--;
    pGUIContext->DrawFilledRect(&rect, MAKE_COLOR_R8G8B8A8_UNORM(112, 146, 190, 255));

    // draw the bar itself
    if (percentComplete > 0.0f)
    {
        SET_MINIGUI_RECT(&rect, m_offsetX + 1, m_offsetX + (uint32)Math::Truncate(percentComplete * 0.01f * (float)m_barWidth), m_offsetY + m_fontHeight + 4 + 1, m_offsetY + m_fontHeight + 4 + m_barHeight + 1);
        pGUIContext->DrawFilledRect(&rect, MAKE_COLOR_R8G8B8A8_UNORM(255, 201, 14, 255));
    }

    // flush ops
    pGUIContext->PopManualFlush();
    pGUIContext->Flush();

    // swap buffers
    pGPUContext->PresentOutputBuffer(GPU_PRESENT_BEHAVIOUR_IMMEDIATE);
}

