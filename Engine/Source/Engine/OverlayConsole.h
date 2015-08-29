#pragma once
#include "Engine/Common.h"

class Font;
class MiniGUIContext;
union SDL_Event;

class OverlayConsole
{
public:
    enum ACTIVATION_STATE
    {
        ACTIVATION_STATE_NONE,
        ACTIVATION_STATE_INPUT_ONLY,
        ACTIVATION_STATE_PARTIAL,
        ACTIVATION_STATE_FULL,
    };

//     struct ColorConfig
//     {
//         uint32 InputBackgroundColor;
//         uint32 InputBorderColor;
//         uint32 InputForegroundColor;
// 
//         uint32 BufferBackgroundColor;
//         uint32 BufferBorderColor;
// 
//         uint32 LogErrorColor;
//         uint32 LogWarningColor;
//         uint32 LogInfoColor;
//         uint32 LogPerfColor;
//         uint32 LogDevColor;
//         uint32 LogProfileColor;
//         uint32 LogTraceColor;
// 
//         void SetDefault();
//     };

public:
    OverlayConsole();
    ~OverlayConsole();

    // change the activation state
    ACTIVATION_STATE GetActivationState() const { return m_activationState; }
    void SetActivationState(ACTIVATION_STATE activationState) { m_activationState = activationState; }

    // logging level customization
    void SetBufferLogLevel(LOGLEVEL level);
    void SetDisplayLogLevel(LOGLEVEL level);

    // call every frame
    void Update(float timeDifference);

    // call from render thread
    void Draw(MiniGUIContext *pGUIContext) const;

    // call when an input event occurs. if it is consumed by the console, true will be returned
    bool OnInputEvent(const SDL_Event *pEvent);

private:
    // append a message to the log buffer contents, wiping out anything that has to be
    void AppendMessageToBuffer(const char *message, uint32 length, uint32 color);

    // append a message to the display
    void AppendMessageToDisplay(const char *message, uint32 length, float time, uint32 color);

    // update messagecolors array
    //void UpdateMessageColors(const ColorConfig *pConfig);

    // draw messages only
    void DrawQueuedMessages(MiniGUIContext *pGUIContext) const;
    void DrawInputOnly(MiniGUIContext *pGUIContext) const;
    void DrawPartial(MiniGUIContext *pGUIContext) const;

    // log callback
    static void LogCallbackTrampoline(void *pUserParam, const char *channelName, const char *functionName, LOGLEVEL level, const char *message);
    void LogCallback(const char *channelName, const char *functionName, LOGLEVEL level, const char *message);

    // anything is pretty much locked
    mutable Mutex m_lock;

    // font info
    const Font *m_pFont;
    uint32 m_fontSize;

    // color config
    uint32 m_titleBackgroundColor;
    uint32 m_titleForegroundColor;
    uint32 m_inputBackgroundColor;
    uint32 m_inputBorderColor;
    uint32 m_inputForegroundColor;
    uint32 m_inputCaretColor;
    uint32 m_bufferBackgroundColor;
    uint32 m_bufferBorderColor;
    uint32 m_messageColors[LOGLEVEL_COUNT];
    uint32 m_maxMessageLength;
    uint32 m_maxCommandHistory;
    
    // buffered text
    LOGLEVEL m_logBufferLevel;
    uint32 m_logBufferSize;

    // displayed text
    LOGLEVEL m_logDisplayLevel;
    uint32 m_logDisplayLines;
    float m_logDisplayTime;
    float m_logFadeOutTime;

    // state
    ACTIVATION_STATE m_activationState;
    uint32 m_logBufferLinesScrolledVertical;
    uint32 m_logBufferCharactersScrolledHorizontal;
    String m_inputBuffer;
    int32 m_inputCaretPosition;
    Array<String> m_commandHistory;
    int32 m_currentCommandHistoryIndex;

    // log buffer
    String m_logBufferContents;
    uint32 m_logBufferLines;

    // display buffer
    struct LogDisplayEntry
    {
        SmallString Message;
        uint32 Color;
        float TimeRemaining;
    };
    PODArray<LogDisplayEntry *> m_logDisplayEntries;

};

