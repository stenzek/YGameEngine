#pragma once
#include "BaseGame/Common.h"
#include "YBaseLib/ProgressCallbacks.h"

class GPUContext;
class MiniGUIContext;

class LoadingScreenProgressCallbacks : public BaseProgressCallbacks
{
public:
    LoadingScreenProgressCallbacks(uint32 offsetX = 0, uint32 offsetY = 0, uint32 fontHeight = 16, uint32 barWidth = 300, uint32 barHeight = 20);
    ~LoadingScreenProgressCallbacks();

    virtual void PushState();
    virtual void PopState();

    virtual void SetCancellable(bool cancellable);
    virtual void SetStatusText(const char *statusText);
    virtual void SetProgressRange(uint32 range);
    virtual void SetProgressValue(uint32 value);

    virtual void DisplayError(const char *message);
    virtual void DisplayWarning(const char *message);
    virtual void DisplayInformation(const char *message);
    virtual void DisplayDebugMessage(const char *message);

    virtual void ModalError(const char *message);
    virtual bool ModalConfirmation(const char *message);
    virtual uint32 ModalPrompt(const char *message, uint32 nOptions, ...);

private:
    void Redraw();

    virtual void RedrawRenderThread();

    uint32 m_offsetX;
    uint32 m_offsetY;
    uint32 m_fontHeight;
    uint32 m_barWidth;
    uint32 m_barHeight;

    float m_lastPercentComplete;
    uint32 m_lastBarLength;
};
