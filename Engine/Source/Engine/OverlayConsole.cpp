#include "Engine/PrecompiledHeader.h"
#include "Engine/OverlayConsole.h"
#include "Engine/Font.h"
#include "Engine/SDLHeaders.h"
#include "Renderer/Renderer.h"
#include "Renderer/MiniGUIContext.h"
Log_SetChannel(OverlayConsole);

OverlayConsole::OverlayConsole()
{
    // default settings
    m_pFont = g_pRenderer->GetFixedResources()->GetDebugFont();
    m_pFont->AddRef();
    m_fontSize = 16;

    // colors
    m_titleBackgroundColor = MAKE_COLOR_R8G8B8A8_UNORM(120, 30, 50, 220);
    m_titleForegroundColor = MAKE_COLOR_R8G8B8A8_UNORM(220, 220, 220, 255);
    m_inputBackgroundColor = MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 150);
    m_inputBorderColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 255);
    m_inputForegroundColor = MAKE_COLOR_R8G8B8A8_UNORM(220, 220, 220, 255);
    m_inputCaretColor = MAKE_COLOR_R8G8B8A8_UNORM(220, 220, 220, 220);
    m_bufferBackgroundColor = MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 180);
    m_bufferBorderColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 255);

    // message colors
    Y_memset(m_messageColors, 255, sizeof(m_messageColors));
    m_messageColors[LOGLEVEL_ERROR] = MAKE_COLOR_R8G8B8A8_UNORM(255, 0, 0, 255);        // error
    m_messageColors[LOGLEVEL_WARNING] = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 255);    // warning
    m_messageColors[LOGLEVEL_SUCCESS] = MAKE_COLOR_R8G8B8A8_UNORM(0, 255, 0, 255);      // success
    m_messageColors[LOGLEVEL_INFO] = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255);     // info
    m_messageColors[LOGLEVEL_PERF] = MAKE_COLOR_R8G8B8A8_UNORM(0, 255, 255, 255);       // perf
    m_messageColors[LOGLEVEL_DEV] = MAKE_COLOR_R8G8B8A8_UNORM(200, 200, 200, 255);      // dev
    m_messageColors[LOGLEVEL_PROFILE] = MAKE_COLOR_R8G8B8A8_UNORM(0, 255, 255, 255);    // profile
    m_messageColors[LOGLEVEL_TRACE] = MAKE_COLOR_R8G8B8A8_UNORM(200, 200, 200, 255);    // trace

    // maximum message length
    m_maxMessageLength = 160;
    m_maxCommandHistory = 32;
    
    // buffered level
    m_logBufferLevel = LOGLEVEL_TRACE;
    m_logBufferSize = 1048576 - 1;
    m_logBufferContents.Reserve(m_logBufferSize);

    // display level
    m_logDisplayLevel = LOGLEVEL_DEV;
    m_logDisplayLines = 10;
    m_logDisplayTime = 5.0f;
    m_logFadeOutTime = 2.0f;

    // state
    m_activationState = ACTIVATION_STATE_NONE;
    m_logBufferLinesScrolledVertical = 0;
    m_logBufferCharactersScrolledHorizontal = 0;
    m_logBufferLines = 0;
    m_inputCaretPosition = 0;
    m_currentCommandHistoryIndex = 0;

    // hook into log system
    Log::GetInstance().RegisterCallback(LogCallbackTrampoline, reinterpret_cast<void *>(this));
}

OverlayConsole::~OverlayConsole()
{
    // unhook from log system
    Log::GetInstance().UnregisterCallback(LogCallbackTrampoline, reinterpret_cast<void *>(this));

    // delete display entries
    for (uint32 i = 0; i < m_logDisplayEntries.GetSize(); i++)
        delete m_logDisplayEntries[i];

    // release vars
    SAFE_RELEASE(m_pFont);
}

void OverlayConsole::SetBufferLogLevel(LOGLEVEL level)
{

}

void OverlayConsole::SetDisplayLogLevel(LOGLEVEL level)
{

}

void OverlayConsole::LogCallbackTrampoline(void *pUserParam, const char *channelName, const char *functionName, LOGLEVEL level, const char *message)
{
    OverlayConsole *pThis = reinterpret_cast<OverlayConsole *>(pUserParam);
    pThis->LogCallback(channelName, functionName, level, message);
}

void OverlayConsole::LogCallback(const char *channelName, const char *functionName, LOGLEVEL level, const char *message)
{
    SmallString formattedMessage;
    Log::FormatLogMessageForDisplay(channelName, functionName, level, message, [](const char *text, void *str) {
        ((String *)str)->AppendString(text);
    }, (void *)&formattedMessage);

    uint32 messageLength = Min(formattedMessage.GetLength(), m_maxMessageLength);
    MutexLock lock(m_lock);

    // protect this since it is converted to a char
    if (level >= LOGLEVEL_COUNT)
        level = (LOGLEVEL)(LOGLEVEL_COUNT - 1);

    // append to display buffer
    if (level <= m_logDisplayLevel)
    {
        if (m_logDisplayEntries.GetSize() >= m_logDisplayLines)
        {
            delete m_logDisplayEntries[0];
            m_logDisplayEntries.PopFront();
        }

        LogDisplayEntry *pEntry = new LogDisplayEntry;
        pEntry->Message = formattedMessage;
        pEntry->Color = (level < countof(m_messageColors)) ? m_messageColors[level] : m_messageColors[0];
        pEntry->TimeRemaining = m_logDisplayTime;
        m_logDisplayEntries.Add(pEntry);
    }

    // append to store buffer?
    if (level <= m_logBufferLevel)
    {
        // clear out the earliest lines to make room for this line
        while ((messageLength + m_logBufferContents.GetLength() + 1) >= m_logBufferSize)
        {
            // find the first line terminator
            uint32 linePosition = 0;
            for (; linePosition < m_logBufferContents.GetLength(); linePosition++)
            {
                if (m_logBufferContents[linePosition] < 0)
                    break;
            }

            // nuke everything up to and including this point
            m_logBufferContents.Erase(0, linePosition + 1);
            if (m_logBufferLines > 0)
                m_logBufferLines--;
        }

        // append it to the buffer
        uint32 charactersWritten = 0;
        for (uint32 i = 0; i < messageLength; i++)
        {
            // character range 240-255 are reserved for internal use, so replace these characters with a .
            char ch = formattedMessage[i];
            if (ch <= 0 || ch == '\n')
                continue;

            // append it
            m_logBufferContents.AppendCharacter(ch);
            charactersWritten++;
        }
        
        // written?
        if (charactersWritten > 0)
        {
            m_logBufferContents.AppendCharacter(-((char)level + 1));
            m_logBufferLines++;
        }
    }
}

void OverlayConsole::Update(float timeDifference)
{
    MutexLock lock(m_lock);

    // fade out/delete display entries
    for (uint32 i = 0; i < m_logDisplayEntries.GetSize(); )
    {
        LogDisplayEntry *pEntry = m_logDisplayEntries[i];

        // update the time
        pEntry->TimeRemaining -= timeDifference;
        if (pEntry->TimeRemaining <= 0.0f)
        {
            delete pEntry;
            m_logDisplayEntries.PopFront();
            continue;
        }

        // update the color
        if (pEntry->TimeRemaining <= m_logFadeOutTime)
        {
            float fractionRemaining = pEntry->TimeRemaining / m_logFadeOutTime;
            uint32 alpha = (uint32)Math::Truncate(fractionRemaining * 255.0f);

            // change alpha value
            pEntry->Color = (pEntry->Color & 0x00FFFFFF) | (alpha << 24);
        }

        // move to next entry
        i++;
    }   
}

void OverlayConsole::Draw(MiniGUIContext *pGUIContext) const
{
    // uses manual lock/unlock to allow the flush to occur after the lock is released
    pGUIContext->PushManualFlush();
    m_lock.Lock();

    switch (m_activationState)
    {
    case ACTIVATION_STATE_NONE:
        DrawQueuedMessages(pGUIContext);
        break;

    case ACTIVATION_STATE_INPUT_ONLY:
        DrawQueuedMessages(pGUIContext);
        DrawInputOnly(pGUIContext);
        break;

    case ACTIVATION_STATE_PARTIAL:
        DrawPartial(pGUIContext);
        break;
    }

    m_lock.Unlock();
    pGUIContext->PopManualFlush();
}

void OverlayConsole::DrawQueuedMessages(MiniGUIContext *pGUIContext) const
{
    // draw display entries
    const uint32 X_INDENT = 16;
    const uint32 Y_INDENT = 16;
    const uint32 Y_SPACING = 2;

    uint32 currentY = Y_INDENT;

    for (uint32 i = 0; i < m_logDisplayEntries.GetSize(); i++)
    {
        const LogDisplayEntry *pEntry = m_logDisplayEntries[i];
        DebugAssert(pEntry->TimeRemaining >= 0.0f);

        pGUIContext->DrawText(m_pFont, m_fontSize, X_INDENT, currentY, pEntry->Message, pEntry->Color, false, MINIGUI_HORIZONTAL_ALIGNMENT_LEFT, MINIGUI_VERTICAL_ALIGNMENT_TOP);

        currentY += m_fontSize + Y_SPACING;
    }
}

void OverlayConsole::DrawInputOnly(MiniGUIContext *pGUIContext) const
{
    const int32 INPUTFIELD_HEIGHT = 24;
    const int32 VERTICAL_PADDING = 4;
    
    String displayLine;
    MINIGUI_RECT regionRect = pGUIContext->GetTopRect();
    MINIGUI_RECT rect;

    // push some space around
    SET_MINIGUI_RECT(&rect, regionRect.left, regionRect.right, regionRect.bottom - INPUTFIELD_HEIGHT, regionRect.bottom);
    pGUIContext->PushRect(&rect);

    // draw the input box background
    SET_MINIGUI_RECT(&rect, 0, regionRect.right, 0, INPUTFIELD_HEIGHT);
    pGUIContext->DrawFilledRect(&rect, m_inputBackgroundColor);

    // draw the input box contents
    SET_MINIGUI_RECT(&rect, 0, regionRect.right, VERTICAL_PADDING, VERTICAL_PADDING + m_fontSize);
    displayLine.Format("> %s", m_inputBuffer.GetCharArray());
    pGUIContext->DrawText(m_pFont, m_fontSize, &rect, displayLine, m_inputForegroundColor, false, MINIGUI_HORIZONTAL_ALIGNMENT_LEFT, MINIGUI_VERTICAL_ALIGNMENT_TOP);

    // calculate the text width with the current caret position
    uint32 caretOffset = m_pFont->GetTextWidth("> ", 2, (float)m_fontSize / (float)m_pFont->GetHeight());
    uint32 caretSize = m_pFont->GetTextWidth(" ", 1, (float)m_fontSize / (float)m_pFont->GetHeight());
    if (m_inputBuffer.GetLength() > 0)
    {
        uint32 textWidth = m_pFont->GetTextWidth(m_inputBuffer, Min((int32)m_inputBuffer.GetLength(), m_inputCaretPosition), (float)m_fontSize / (float)m_pFont->GetHeight());
        caretOffset += textWidth;
    }

    // draw the caret
    SET_MINIGUI_RECT(&rect, caretOffset, caretOffset + caretSize, VERTICAL_PADDING, VERTICAL_PADDING + m_fontSize);
    pGUIContext->DrawFilledRect(&rect, m_inputCaretColor);    

    // pop rect stack
    pGUIContext->PopRect();
}

void OverlayConsole::DrawPartial(MiniGUIContext *pGUIContext) const
{
    // config
    const int32 HORIZONTAL_MARGIN = 0;
    const int32 VERTICAL_MARGIN = 0;
    const int32 HORIZONTAL_PADDING = 4;
    const int32 VERTICAL_PADDING = 4;
    const int32 BOX_PADDING = 2;
    const int32 TITLE_START_OFFSET = 0;
    const int32 TITLE_END_OFFSET = 16 + VERTICAL_PADDING;
    const int32 LINE_START_OFFSET = TITLE_END_OFFSET;
    const int32 LINE_END_OFFSET = LINE_START_OFFSET + 320 + BOX_PADDING;
    const int32 INPUT_START_OFFSET = LINE_END_OFFSET + 1;
    const int32 INPUT_END_OFFSET = INPUT_START_OFFSET + 16 + BOX_PADDING;

    // rects
    SmallString displayLine;
    MINIGUI_RECT regionRect = pGUIContext->GetTopRect();
    MINIGUI_RECT rect;

    // adjust region rect for margins
    regionRect.left += HORIZONTAL_MARGIN;
    regionRect.right -= HORIZONTAL_MARGIN;
    regionRect.top += VERTICAL_MARGIN;
    regionRect.bottom -= VERTICAL_MARGIN;

    // draw the title background
    SET_MINIGUI_RECT(&rect, regionRect.left, regionRect.right, regionRect.top + TITLE_START_OFFSET, regionRect.top + TITLE_END_OFFSET);
    pGUIContext->DrawFilledRect(&rect, m_titleBackgroundColor);

    // draw the title text
    SET_MINIGUI_RECT(&rect, regionRect.left + HORIZONTAL_PADDING, regionRect.right - HORIZONTAL_PADDING, regionRect.top + TITLE_START_OFFSET, regionRect.top + TITLE_END_OFFSET);
    pGUIContext->DrawText(m_pFont, m_fontSize, &rect, "Console", m_titleForegroundColor, false, MINIGUI_HORIZONTAL_ALIGNMENT_LEFT, MINIGUI_VERTICAL_ALIGNMENT_TOP);
    pGUIContext->DrawText(m_pFont, m_fontSize, &rect, "press ` to close", m_titleForegroundColor, false, MINIGUI_HORIZONTAL_ALIGNMENT_RIGHT, MINIGUI_VERTICAL_ALIGNMENT_TOP);

    // draw the box background
    SET_MINIGUI_RECT(&rect, regionRect.left, regionRect.right, regionRect.top + LINE_START_OFFSET, regionRect.top + LINE_END_OFFSET);
    pGUIContext->DrawFilledRect(&rect, m_bufferBackgroundColor);

    // and the border
    //SET_MINIGUI_RECT(&rect, regionRect.left + 1, regionRect.right, regionRect.top, regionRect.top + LINE_END_OFFSET);
    //pGUIContext->DrawRect(&rect, m_bufferBorderColor);

    // anything in the buffer?
    if (m_logBufferContents.GetLength() > 1)
    {
        // find the starting point of the buffer
        int32 displayStartPosition = m_logBufferContents.GetLength() - 1;

        // move up to the scroll point
        if (m_logBufferLinesScrolledVertical > 0)
        {
            int32 caretIndicatorPosition = regionRect.top + LINE_END_OFFSET - m_fontSize - BOX_PADDING;
            SET_MINIGUI_RECT(&rect, regionRect.left + HORIZONTAL_PADDING, regionRect.right - HORIZONTAL_PADDING, caretIndicatorPosition, caretIndicatorPosition + m_fontSize + 1);
            pGUIContext->DrawText(m_pFont, m_fontSize, &rect, "^^^^ ", MAKE_COLOR_R8G8B8A8_UNORM(230, 230, 230, 255), false, MINIGUI_HORIZONTAL_ALIGNMENT_RIGHT, MINIGUI_VERTICAL_ALIGNMENT_TOP);

            for (uint32 i = 0; i < m_logBufferLinesScrolledVertical; i++)
            {
                if (displayStartPosition == 0)
                    break;

                // remove the log level
                displayStartPosition--;

                // and each character
                while (displayStartPosition > 0 && m_logBufferContents[displayStartPosition] > 0)
                    displayStartPosition--;
            }
        }

        // display these strings
        int32 currentStartOffset = regionRect.top + LINE_END_OFFSET - m_fontSize - BOX_PADDING;
        while (displayStartPosition > 0 && currentStartOffset >= LINE_START_OFFSET)
        {
            // wipe the last line
            displayLine.Clear();

            // extract the log level
            unsigned char messageLevel = static_cast<unsigned char>(-(m_logBufferContents[displayStartPosition]) - 1);
            DebugAssert(messageLevel < countof(m_messageColors));
            displayStartPosition--;

            // add each character in
            while (displayStartPosition >= 0 && m_logBufferContents[displayStartPosition] > 0)
            {
                displayLine.PrependCharacter(m_logBufferContents[displayStartPosition]);
                displayStartPosition--;
            }

            // draw the line
            if (displayLine.GetLength() > 0)
            {
                // if horizontal scroll is enabled, remove characters
                for (uint32 i = 0; i < m_logBufferCharactersScrolledHorizontal && displayLine.GetLength() > 0; i++)
                    displayLine.Erase(0, 1);

                // set rect and draw
                SET_MINIGUI_RECT(&rect, regionRect.left + HORIZONTAL_PADDING, regionRect.right - HORIZONTAL_PADDING, currentStartOffset, currentStartOffset + m_fontSize + 1);
                pGUIContext->DrawText(m_pFont, m_fontSize, &rect, displayLine, m_messageColors[messageLevel], false, MINIGUI_HORIZONTAL_ALIGNMENT_LEFT, MINIGUI_VERTICAL_ALIGNMENT_TOP);
            }

            // move the cursor
            currentStartOffset -= m_fontSize;
            if (currentStartOffset < 0 || displayStartPosition <= 0)
                break;
        }
    }

    // draw the input box background
    SET_MINIGUI_RECT(&rect, regionRect.left, regionRect.right, regionRect.top + INPUT_START_OFFSET, regionRect.top + INPUT_END_OFFSET);
    pGUIContext->DrawFilledRect(&rect, m_inputBackgroundColor);

    // draw the seperator line
    pGUIContext->DrawLine(int2(regionRect.left, regionRect.top + INPUT_START_OFFSET), int2(regionRect.right, regionRect.top + INPUT_START_OFFSET), m_inputBorderColor);

    // draw the input box contents
    SET_MINIGUI_RECT(&rect, regionRect.left + HORIZONTAL_PADDING, regionRect.right - HORIZONTAL_PADDING, regionRect.top + INPUT_START_OFFSET, regionRect.top + INPUT_END_OFFSET);
    displayLine.Format("> %s", m_inputBuffer.GetCharArray());
    pGUIContext->DrawText(m_pFont, m_fontSize, &rect, displayLine, m_inputForegroundColor, false, MINIGUI_HORIZONTAL_ALIGNMENT_LEFT, MINIGUI_VERTICAL_ALIGNMENT_TOP);

    // calculate the text width with the current caret position
    uint32 caretOffset = regionRect.left + HORIZONTAL_PADDING + m_pFont->GetTextWidth("> ", 2, (float)m_fontSize / (float)m_pFont->GetHeight());
    uint32 caretSize = m_pFont->GetTextWidth(" ", 1, (float)m_fontSize / (float)m_pFont->GetHeight());
    if (m_inputBuffer.GetLength() > 0)
    {
        uint32 textWidth = m_pFont->GetTextWidth(m_inputBuffer, Min((int32)m_inputBuffer.GetLength(), m_inputCaretPosition), (float)m_fontSize / (float)m_pFont->GetHeight());
        caretOffset += textWidth;
    }

    // draw the caret
    SET_MINIGUI_RECT(&rect, caretOffset, caretOffset + caretSize, regionRect.top + INPUT_START_OFFSET, regionRect.top + INPUT_END_OFFSET);
    pGUIContext->DrawFilledRect(&rect, m_inputCaretColor);
}

bool OverlayConsole::OnInputEvent(const SDL_Event *pEvent)
{
    MutexLock lock(m_lock);

    // handle the case that we're not active first
    if (m_activationState == ACTIVATION_STATE_NONE)
    {
        if (pEvent->type == SDL_KEYDOWN && pEvent->key.keysym.sym == SDLK_BACKQUOTE)
        {
            // change activation state
            if (m_activationState == ACTIVATION_STATE_NONE)
                m_activationState = ACTIVATION_STATE_INPUT_ONLY;
            else if (m_activationState == ACTIVATION_STATE_INPUT_ONLY)
                m_activationState = ACTIVATION_STATE_PARTIAL;
            else if (m_activationState == ACTIVATION_STATE_PARTIAL)
                //m_activationState = ACTIVATION_STATE_FULL;
            //else
                m_activationState = ACTIVATION_STATE_NONE;

            // consume it
            return true;
        }

        // not using the event
        return false;
    }

    //
    // we are active!
    //

    // hitting the ` key?
    if (pEvent->type == SDL_KEYDOWN)
    {
        if (pEvent->key.keysym.sym == SDLK_BACKQUOTE)
        {
            // change activation state
            if (m_activationState == ACTIVATION_STATE_NONE)
                m_activationState = ACTIVATION_STATE_INPUT_ONLY;
            else if (m_activationState == ACTIVATION_STATE_INPUT_ONLY)
                m_activationState = ACTIVATION_STATE_PARTIAL;
            //else if (m_activationState == ACTIVATION_STATE_PARTIAL)
            //m_activationState = ACTIVATION_STATE_FULL;
            else
                m_activationState = ACTIVATION_STATE_NONE;

            // consume it
            return true;
        }

        // scroll up?
        if (pEvent->key.keysym.sym == SDLK_PAGEUP)
        {
            if (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL))
            {
                if (m_logBufferCharactersScrolledHorizontal > 0)
                    m_logBufferCharactersScrolledHorizontal--;
            }
            else
            {
                m_logBufferLinesScrolledVertical++;
            }

            return true;
        }

        // scroll down?
        if (pEvent->key.keysym.sym == SDLK_PAGEDOWN)
        {
            if (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL))
            {
                m_logBufferCharactersScrolledHorizontal++;
            }
            else
            {
                if (m_logBufferLinesScrolledVertical > 0)
                    m_logBufferLinesScrolledVertical--;
            }

            return true;
        }

        if (pEvent->key.keysym.sym == SDLK_HOME)
        {
            // scroll to start of display?
            if (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL))
                m_logBufferLinesScrolledVertical = (m_logBufferLines > 0) ? (m_logBufferLines - 1) : 0;
            else
                m_inputCaretPosition = 0;
        }

        // scroll to end?
        if (pEvent->key.keysym.sym == SDLK_END)
        {
            if (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL))
                m_logBufferLinesScrolledVertical = 0;
            else
                m_inputCaretPosition = m_inputBuffer.GetLength();

            return true;
        }

        // left arrow key?
        if (pEvent->key.keysym.sym == SDLK_LEFT)
        {
            // ctrl+left?
            if (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL))
            {
                // find the previous space, if not, the start of the input
                if (m_inputBuffer.GetLength() > 0 && m_inputCaretPosition > 0)
                {
                    m_inputCaretPosition--;

                    // currently on a whitespace?
                    if (m_inputBuffer[m_inputCaretPosition] == ' ')
                    {
                        // back up until we find something not whitespace
                        while (m_inputCaretPosition > 0)
                        {
                            m_inputCaretPosition--;
                            if (m_inputBuffer[m_inputCaretPosition] != ' ')
                                break;
                        }
                    }

                    // find the next whitespace
                    while (m_inputCaretPosition > 0)
                    {
                        if (m_inputBuffer[m_inputCaretPosition - 1] == ' ')
                            break;

                        m_inputCaretPosition--;
                    }
                }
            }
            else
            {
                // normal left
                if (m_inputCaretPosition > 0)
                    m_inputCaretPosition--;
            }

            return true;
        }

        // right arrow key?
        if (pEvent->key.keysym.sym == SDLK_RIGHT)
        {
            // ctrl+right?
            if (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL))
            {
                // find the previous space, if not, the start of the input
                if ((uint32)m_inputCaretPosition < m_inputBuffer.GetLength())
                {
                    m_inputCaretPosition++;

                    while ((uint32)m_inputCaretPosition < m_inputBuffer.GetLength())
                    {
                        if (m_inputBuffer[m_inputCaretPosition] == ' ')
                        {
                            m_inputCaretPosition = Min((int32)m_inputBuffer.GetLength(), m_inputCaretPosition + 1);
                            break;
                        }

                        m_inputCaretPosition++;
                    }

                    // skip over any consecutive whitespace
                    while ((uint32)m_inputCaretPosition < m_inputBuffer.GetLength())
                    {
                        if (m_inputBuffer[m_inputCaretPosition] != ' ')
                            break;

                        m_inputCaretPosition++;
                    }
                }
            }
            else
            {
                // normal
                if (m_inputCaretPosition < (int32)m_inputBuffer.GetLength())
                    m_inputCaretPosition++;
            }

            return true;
        }

        // hitting the enter key? submit the command
        if (pEvent->key.keysym.sym == SDLK_RETURN)
        {
            if (m_inputBuffer.GetLength() > 0)
            {
                // release the lock to execute the command
                lock.Unlock();

                // actually execute it
                bool executeResult = g_pConsole->ExecuteText(m_inputBuffer.GetCharArray());

                // re-lock after execution
                lock.Lock();

                // clear history position
                m_currentCommandHistoryIndex = -1;

                // add to history
                if (m_commandHistory.GetSize() == m_maxCommandHistory)
                    m_commandHistory.OrderedRemove(0);
                m_commandHistory.Add(m_inputBuffer);

                // succeeded?
                if (executeResult)
                {
                    // if execution succeeded and we are in input only mode, immediately hide the console
                    if (m_activationState == ACTIVATION_STATE_INPUT_ONLY)
                        SetActivationState(ACTIVATION_STATE_NONE);
                }

                // wipe out buffer
                m_inputBuffer.Clear();
                m_inputCaretPosition = 0;
            }

            return true;
        }

        // hitting the backspace key?
        if (pEvent->key.keysym.sym == SDLK_BACKSPACE)
        {
            if (m_inputBuffer.GetLength() > 0)
            {
                m_inputBuffer.Erase(-1);
                m_inputCaretPosition = Max(m_inputCaretPosition - 1, (int32)0);
            }

            return true;
        }

        // hitting the autocomplete key?
        if (pEvent->key.keysym.sym == SDLK_TAB)
        {
            // unlock before doing auto-completion
            lock.Unlock();
            g_pConsole->HandleAutoCompletion(m_inputBuffer);

            // re-lock to modify variables
            lock.Lock();
            m_inputCaretPosition = m_inputBuffer.GetLength();
            return true;
        }

        // hitting the up arrow? 
        if (pEvent->key.keysym.sym == SDLK_UP)
        {
            if (m_currentCommandHistoryIndex == -1)
            {
                if (m_commandHistory.GetSize() > 0)
                {
                    m_currentCommandHistoryIndex = m_commandHistory.GetSize() - 1;
                    m_inputBuffer = m_commandHistory[m_currentCommandHistoryIndex];
                    m_inputCaretPosition = m_inputBuffer.GetLength();
                }
            }
            else if (m_currentCommandHistoryIndex > 0)
            {
                m_currentCommandHistoryIndex--;
                m_inputBuffer = m_commandHistory[m_currentCommandHistoryIndex];
                m_inputCaretPosition = m_inputBuffer.GetLength();
            }
        }

        // hitting the down arrow?
        if (pEvent->key.keysym.sym == SDLK_DOWN)
        {
            if (m_currentCommandHistoryIndex >= 0)
            {
                if ((m_currentCommandHistoryIndex + 1) < (int32)m_commandHistory.GetSize())
                {
                    m_currentCommandHistoryIndex++;
                    m_inputBuffer = m_commandHistory[m_currentCommandHistoryIndex];
                    m_inputCaretPosition = m_inputBuffer.GetLength();
                }
                else
                {
                    // clear it
                    m_currentCommandHistoryIndex = -1;
                    m_inputBuffer.Clear();
                    m_inputCaretPosition = 0;
                }
            }
        }

        // ctrl+v paste
        if (pEvent->key.keysym.sym == SDLK_v && SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL))
        {
            // get clipboard text
            char *clipboardText = SDL_GetClipboardText();
            if (clipboardText != nullptr)
            {
                uint32 textLength = Y_strlen(clipboardText);
                m_inputBuffer.InsertString(m_inputCaretPosition, clipboardText, textLength);
                m_inputCaretPosition += textLength;
                SDL_free(clipboardText);
            }

            return true;
        }

        // ctrl+w remove everything, but not including the last space
        if (pEvent->key.keysym.sym == SDLK_w && SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL))
        {
            int32 lastPos = m_inputBuffer.RFind(' ');
            if (lastPos >= 0)
                m_inputBuffer.Erase((lastPos != ((int32)m_inputBuffer.GetLength() - 1)) ? (lastPos + 1) : (lastPos));
            else
                m_inputBuffer.Clear();

            return true;
        }
    }

    // handle text being entered
    if (pEvent->type == SDL_TEXTINPUT)
    {
        // hack: remove the ` keystrokes
        if (pEvent->text.text[0] == '`')
            return true;

        // ignore anything pressed with ctrl
        if (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL))
            return true;

        // hitting the help key?
        if (pEvent->text.text[0] == '?')
        {
            lock.Unlock();
            g_pConsole->HandlePartialHelp(m_inputBuffer);
            return true;
        }

        // clear current history position
        m_currentCommandHistoryIndex = -1;

        // append the text
        uint32 insertStringLength = Y_strlen(pEvent->text.text);
        m_inputBuffer.InsertString(m_inputCaretPosition, pEvent->text.text, insertStringLength);
        m_inputCaretPosition += insertStringLength;
        return true;
    }

    // handle mouse wheel scrolling
    if (pEvent->type == SDL_MOUSEWHEEL)
    {
        m_logBufferCharactersScrolledHorizontal = Max((int32)m_logBufferCharactersScrolledHorizontal + pEvent->wheel.x, 0);
        m_logBufferLinesScrolledVertical = Max((int32)m_logBufferLinesScrolledVertical + pEvent->wheel.y, 0);
        return true;
    }

    // block all other keyboard events while we're active
    if (pEvent->type == SDL_KEYDOWN || pEvent->type == SDL_KEYUP)
        return true;
    else
        return false;
}
