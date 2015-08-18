#include "Engine/PrecompiledHeader.h"
#include "Engine/InputManager.h"
#include "Engine/SDLHeaders.h"
Log_SetChannel(Input);

static InputManager s_inputManager;
InputManager *g_pInputManager = &s_inputManager;

InputManager::InputManager()
{
    m_eventBlockerCount = 0;
}

InputManager::~InputManager()
{

}

bool InputManager::Startup()
{
    Log_InfoPrint("InputManager::Startup");

    Log_DevPrintf("SDL_InitSubSystem(SDL_INIT_JOYSTICK)...");
    if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
        Log_ErrorPrintf("SDL_InitSubSystem(SDL_INIT_JOYSTICK) failed: %s, joysticks will not be available.", SDL_GetError());
    else
        Log_DevPrintf("  SDL_NumJoysticks() = %i", SDL_NumJoysticks());

    Log_DevPrintf("SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)...");
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0)
        Log_ErrorPrintf("SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) failed: %s, game controllers will not be available.", SDL_GetError());

    Log_DevPrintf("SDL_InitSubSystem(SDL_INIT_HAPTIC)...");
    if (SDL_InitSubSystem(SDL_INIT_HAPTIC) < 0)
        Log_ErrorPrintf("SDL_InitSubSystem(SDL_INIT_HAPTIC) failed: %s, force feedback will not be available.", SDL_GetError());

    return true;
}

void InputManager::Shutdown()
{
    Log_InfoPrint("InputManager::Shutdown");

    for (BindSetTable::Iterator itr = m_bindSets.Begin(); !itr.AtEnd(); itr.Forward())
        delete itr->Value;
    m_pActiveBindSet = nullptr;

    for (EventHashTable::Iterator itr = m_events.Begin(); !itr.AtEnd(); itr.Forward())
        delete itr->Value;
    m_events.Clear();
}

// bool InputManager::OpenJoysticks()
// {
//     int nJoysticks = SDL_NumJoysticks();
//     Log_DevPrintf("SDL_NumJoysticks() = %i", nJoysticks);
//     for (int i = 0; i < nJoysticks; i++)
//     {
//         if (SDL_IsGameController(i))
//         {
//             const char *gameControllerName = SDL_GameControllerNameForIndex(i);
//             Log_DevPrintf("  Opening SDL game controller %i (%s)...", i, gameControllerName);
// 
//             SDL_GameController *pGameController = SDL_GameControllerOpen(i);
//             if (pGameController == nullptr)
//             {
//                 Log_ErrorPrintf("  SDL_GameControllerOpen failed: %s", SDL_GetError());
//                 CloseJoysticks();
//                 return false;
//             }
// 
//             SDL_bool controllerAttached = SDL_GameControllerGetAttached(pGameController);
//             if (!controllerAttached)
//                 Log_WarningPrintf("  Game controller %i is not attached.", i);
// 
//             m_gameControllers.Add(pGameController);
//         }
//         else
//         {
//             const char *joystickName = SDL_JoystickNameForIndex(i);
//             Log_DevPrintf("  Opening SDL joystick %i (%s)...", i, joystickName);
// 
//             SDL_Joystick *pJoystick = SDL_JoystickOpen(i);
//             if (pJoystick == nullptr)
//             {
//                 Log_ErrorPrintf("  SDL_JoystickOpen failed: %s", SDL_GetError());
//                 CloseJoysticks();
//                 return false;
//             }
// 
//             m_joysticks.Add(pJoystick);
//         }
//     }
// 
//     return true;
// }

void InputManager::CloseJoysticks()
{
    for (uint32 i = 0; i < m_controllers.GetSize(); i++)
        SDL_GameControllerClose(m_controllers[i].pSDLGameController);
    m_controllers.Clear();

    for (uint32 i = 0; i < m_joysticks.GetSize(); i++)
        SDL_JoystickClose(m_joysticks[i].pSDLJoystick);
    m_joysticks.Clear();
}

const InputManager::ActionEvent *InputManager::RegisterActionEvent(const void *pOwnerPointer, const char *eventName, const char *displayName, ActionEvent::CallbackType *pCallback)
{
    if (m_events.Find(eventName) != nullptr)
    {
        Log_ErrorPrintf("InputManager::RegisterActionEvent: Event '%s' already exists.", eventName);
        delete pCallback;
        return nullptr;
    }

    ActionEvent *pEvent = new ActionEvent(pOwnerPointer, eventName, displayName, pCallback);
    m_events.Insert(eventName, static_cast<BaseEvent *>(pEvent));
    return pEvent;
}

const InputManager::BinaryStateEvent *InputManager::RegisterBinaryStateEvent(const void *pOwnerPointer, const char *eventName, const char *displayName, BinaryStateEvent::CallbackType *pCallback)
{
    if (m_events.Find(eventName) != nullptr)
    {
        Log_ErrorPrintf("InputManager::RegisterBinaryStateEvent: Event '%s' already exists.", eventName);
        delete pCallback;
        return nullptr;
    }

    BinaryStateEvent *pEvent = new BinaryStateEvent(pOwnerPointer, eventName, displayName, pCallback);
    m_events.Insert(eventName, static_cast<BaseEvent *>(pEvent));
    return pEvent;
}

const InputManager::AxisStateEvent *InputManager::RegisterAxisStateEvent(const void *pOwnerPointer, const char *eventName, const char *displayName, AxisStateEvent::CallbackType *pCallback)
{
    if (m_events.Find(eventName) != nullptr)
    {
        Log_ErrorPrintf("InputManager::RegisterAxisStateEvent: Event '%s' already exists.", eventName);
        delete pCallback;
        return nullptr;
    }

    AxisStateEvent *pEvent = new AxisStateEvent(pOwnerPointer, eventName, displayName, pCallback);
    m_events.Insert(eventName, static_cast<BaseEvent *>(pEvent));
    return pEvent;
}

const InputManager::NormalizedAxisStateEvent *InputManager::RegisterNormalizedAxisStateEvent(const void *pOwnerPointer, const char *eventName, const char *displayName, NormalizedAxisStateEvent::CallbackType *pCallback)
{
    if (m_events.Find(eventName) != nullptr)
    {
        Log_ErrorPrintf("InputManager::RegisterNormalizedAxisStateEvent: Event '%s' already exists.", eventName);
        delete pCallback;
        return nullptr;
    }

    NormalizedAxisStateEvent *pEvent = new NormalizedAxisStateEvent(pOwnerPointer, eventName, displayName, pCallback);
    m_events.Insert(eventName, static_cast<BaseEvent *>(pEvent));
    return pEvent;
}

const InputManager::BaseEvent *InputManager::LookupEventByName(const char *eventName)
{
    EventHashTable::Member *pMember = m_events.Find(eventName);
    return (pMember != nullptr) ? pMember->Value : nullptr;
}

void InputManager::UnregisterEvent(const BaseEvent *pEvent)
{
    EventHashTable::Member *pMember = m_events.Find(pEvent->Name);
    if (pMember != nullptr && pMember->Value == pEvent)
    {
        delete pMember->Value;
        m_events.Remove(pMember);
    }
}

bool InputManager::UnregisterEvent(const char *eventName)
{
    EventHashTable::Member *pMember = m_events.Find(eventName);
    if (pMember != nullptr)
    {
        delete pMember->Value;
        m_events.Remove(pMember);
        return true;
    }

    return false;
}

void InputManager::UnregisterEventsWithOwner(const void *pOwnerPointer)
{
    for (EventHashTable::Iterator itr = m_events.Begin(); !itr.AtEnd(); )
    {
        EventHashTable::Member *pMember = &(*itr);
        itr.Forward();
        
        if (pMember->Value->pOwnerPointer == pOwnerPointer)
            m_events.Remove(pMember);
    }
}

bool InputManager::BindKeyboardKey(const char *bindSetName, const char *keyName, const char *eventName, int32 activateDirection /* = 0 */)
{
    BindSet *pBindSet = (bindSetName != nullptr) ? GetOrCreateBindSetByName(bindSetName) : &m_globalBindSet;
    DebugAssert(pBindSet != nullptr);

    // get sdl scancode
    SDL_Scancode scanCode = SDL_GetScancodeFromName(keyName);
    if (scanCode == SDL_SCANCODE_UNKNOWN)
        return false;

    // search for a bind that already exists
    for (uint32 i = 0; i < pBindSet->KeyboardBinds.GetSize(); i++)
    {
        // overwrite existing bind
        if ((SDL_Scancode)pBindSet->KeyboardBinds[i].SDLScanCode == scanCode)
        {
            // check for command executes
            if (Y_strnicmp(eventName, "exec ", 5) == 0)
            {
                // clip the exec part
                pBindSet->KeyboardBinds[i].CommandString = eventName + 5;
            }
            else
            {
                // write event name
                pBindSet->KeyboardBinds[i].ActivateDirection = activateDirection;
                pBindSet->KeyboardBinds[i].EventName = eventName;
            }

            return true;
        }
    }

    // create new bind
    KeyboardBind bind;
    bind.SDLScanCode = scanCode;

    // check for command executes
    if (Y_strnicmp(eventName, "exec ", 5) == 0)
    {
        // clip the exec part
        bind.CommandString = eventName + 5;
    }
    else
    {
        // write event name
        bind.EventName = eventName;
        bind.ActivateDirection = activateDirection;
    }

    // add to list
    pBindSet->KeyboardBinds.Add(bind);
    return true;
}

bool InputManager::BindMouseAxis(const char *bindSetName, uint32 axisIndex, const char *eventName, int32 bindDirection /* = 0 */)
{
    BindSet *pBindSet = (bindSetName != nullptr) ? GetOrCreateBindSetByName(bindSetName) : &m_globalBindSet;
    DebugAssert(pBindSet != nullptr);

    // search for a bind that already exists
    for (uint32 i = 0; i < pBindSet->MouseAxisBinds.GetSize(); i++)
    {
        if (pBindSet->MouseAxisBinds[i].AxisIndex == axisIndex && pBindSet->MouseAxisBinds[i].BindDirection == bindDirection)
        {
            pBindSet->MouseAxisBinds[i].EventName = eventName;
            return true;
        }
    }

    // create new bind
    MouseAxisBind bind;
    bind.AxisIndex = axisIndex;
    bind.BindDirection = bindDirection;
    bind.EventName = eventName;
    pBindSet->MouseAxisBinds.Add(bind);
    return true;
}

bool InputManager::BindMouseButton(const char *bindSetName, uint32 buttonIndex, const char *eventName, int32 activateDirection /* = 0 */)
{
    BindSet *pBindSet = (bindSetName != nullptr) ? GetOrCreateBindSetByName(bindSetName) : &m_globalBindSet;
    DebugAssert(pBindSet != nullptr);

    // search for a bind that already exists
    for (uint32 i = 0; i < pBindSet->MouseButtonBinds.GetSize(); i++)
    {
        if (pBindSet->MouseButtonBinds[i].ButtonIndex == buttonIndex)
        {
            pBindSet->MouseButtonBinds[i].EventName = eventName;
            pBindSet->MouseButtonBinds[i].ActivateDirection = activateDirection;
            return true;
        }
    }

    // create new bind
    MouseButtonBind bind;
    bind.ButtonIndex = buttonIndex;
    bind.EventName = eventName;
    bind.ActivateDirection = activateDirection;
    pBindSet->MouseButtonBinds.Add(bind);
    return true;
}

bool InputManager::BindControllerAxis(const char *bindSetName, uint32 controllerIndex, ControllerAxis axis, const char *eventName)
{
    BindSet *pBindSet = (bindSetName != nullptr) ? GetOrCreateBindSetByName(bindSetName) : &m_globalBindSet;
    DebugAssert(pBindSet != nullptr);

    // search for a bind that already exists
    for (uint32 i = 0; i < pBindSet->ControllerAxisBinds.GetSize(); i++)
    {
        if (pBindSet->ControllerAxisBinds[i].ControllerIndex == controllerIndex &&
            pBindSet->ControllerAxisBinds[i].Axis == axis)
        {
            pBindSet->ControllerAxisBinds[i].EventName = eventName;
            return true;
        }
    }

    // create new bind
    ControllerAxisBind bind;
    bind.ControllerIndex = controllerIndex;
    bind.Axis = axis;
    bind.EventName = eventName;
    pBindSet->ControllerAxisBinds.Add(bind);
    return true;
}

bool InputManager::BindControllerButton(const char *bindSetName, uint32 controllerIndex, ControllerButton button, const char *eventName, int32 activateDirection /* = 0 */)
{
    BindSet *pBindSet = (bindSetName != nullptr) ? GetOrCreateBindSetByName(bindSetName) : &m_globalBindSet;
    DebugAssert(pBindSet != nullptr);

    // search for a bind that already exists
    for (uint32 i = 0; i < pBindSet->ControllerButtonBinds.GetSize(); i++)
    {
        if (pBindSet->ControllerButtonBinds[i].ControllerIndex == controllerIndex &&
            pBindSet->ControllerButtonBinds[i].Button == button)
        {
            pBindSet->ControllerButtonBinds[i].EventName = eventName;
            pBindSet->ControllerButtonBinds[i].ActivateDirection = activateDirection;
            return true;
        }
    }

    // create new bind
    ControllerButtonBind bind;
    bind.ControllerIndex = controllerIndex;
    bind.Button = button;
    bind.EventName = eventName;
    bind.ActivateDirection = activateDirection;
    pBindSet->ControllerButtonBinds.Add(bind);
    return true;
}

InputManager::BindSet *InputManager::GetOrCreateBindSetByName(const char *bindSetName)
{
    BindSetTable::Member *pMember = m_bindSets.Find(bindSetName);
    if (pMember != nullptr)
        return pMember->Value;
    
    // allocate new
    BindSet *pBindSet = new BindSet();
    pBindSet->Name = bindSetName;
    m_bindSets.Insert(bindSetName, pBindSet);
    return pBindSet;
}

bool InputManager::SwitchBindSet(const char *bindSetName)
{
    BindSetTable::Member *pMember = m_bindSets.Find(bindSetName);
    if (pMember != nullptr)
    {
        Log_DevPrintf("InputManager::SwitchBindSet(%s) success", pMember->Value->Name.GetCharArray());
        m_pActiveBindSet = pMember->Value;
        return true;
    }

    Log_WarningPrintf("InputManager::SwitchBindSet(%s): no bind set with this name", bindSetName);
    m_pActiveBindSet = nullptr;
    return false;
}

bool InputManager::HandleSDLEvent(const void *pEvent)
{
    if (m_eventBlockerCount > 0)
        return false;

    const SDL_Event *pRealEvent = reinterpret_cast<const SDL_Event *>(pEvent);
    switch (pRealEvent->type)
    {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        return HandleKeyboardEvent(pRealEvent);

    case SDL_MOUSEMOTION:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP: 
    case SDL_MOUSEWHEEL:
        return HandleMouseEvent(pRealEvent);

    case SDL_JOYAXISMOTION:
    case SDL_JOYBALLMOTION:
    case SDL_JOYHATMOTION:
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
    case SDL_JOYDEVICEADDED:
    case SDL_JOYDEVICEREMOVED:
        return HandleJoystickEvent(pRealEvent);

    case SDL_CONTROLLERAXISMOTION:
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
    case SDL_CONTROLLERDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED:
    case SDL_CONTROLLERDEVICEREMAPPED:
        return HandleControllerEvent(pRealEvent);  
    }

    return false;
}

bool InputManager::HandleKeyboardEvent(const SDL_Event *pEvent)
{
    // if the key repeat count is >0, skip it
    if (pEvent->key.repeat)
        return false;

    // find the binding
    const KeyboardBind *pKeyboardBind = nullptr;

    // search bind set (if active)
    if (m_pActiveBindSet != nullptr)
    {
        for (const KeyboardBind &keyboardBind : m_pActiveBindSet->KeyboardBinds)
        {
            if ((SDL_Keycode)keyboardBind.SDLScanCode == pEvent->key.keysym.scancode)
            {
                pKeyboardBind = &keyboardBind;
                break;
            }
        }
    }

    // search global bind set if we still didn't find anything
    if (pKeyboardBind == nullptr)
    {
        for (const KeyboardBind &keyboardBind : m_globalBindSet.KeyboardBinds)
        {
            if ((SDL_Keycode)keyboardBind.SDLScanCode == pEvent->key.keysym.scancode)
            {
                pKeyboardBind = &keyboardBind;
                break;
            }
        }
    }

    // if we didn't find anything, it's unhandled
    if (pKeyboardBind == nullptr)
        return false;

    // look up the event
    if (!pKeyboardBind->EventName.IsEmpty())
    {
        const BaseEvent *pBoundEvent = LookupEventByName(pKeyboardBind->EventName);
        if (pBoundEvent != nullptr)
        {
            // handle it
            switch (pBoundEvent->Type)
            {
            case EventType_Action:
                {
                    // invoke action events on key down only
                    if (pEvent->type == SDL_KEYDOWN)
                        static_cast<const ActionEvent *>(pBoundEvent)->pCallback->Invoke();
                }
                break;

            case EventType_BinaryState:
                {
                    // binary state is 1 for down, or 0 for up
                    static_cast<const BinaryStateEvent *>(pBoundEvent)->pCallback->Invoke((pEvent->type == SDL_KEYDOWN));
                }
                break;

            case EventType_AxisState:
                {
                    // axis state is dependant on the bind direction, if it is set
                    if (pKeyboardBind->ActivateDirection != 0)
                        static_cast<const AxisStateEvent *>(pBoundEvent)->pCallback->Invoke(pKeyboardBind->ActivateDirection * ((pEvent->type == SDL_KEYDOWN) ? 1 : 0));
                    else
                        static_cast<const AxisStateEvent *>(pBoundEvent)->pCallback->Invoke(((pEvent->type == SDL_KEYDOWN) ? 1 : 0));
                }
                break;

            case EventType_NormalizedAxisState:
                {
                    // axis state is dependant on the bind direction, if it is set
                    if (pKeyboardBind->ActivateDirection != 0)
                        static_cast<const NormalizedAxisStateEvent *>(pBoundEvent)->pCallback->Invoke(static_cast<float>(pKeyboardBind->ActivateDirection * ((pEvent->type == SDL_KEYDOWN) ? 1 : 0)));
                    else
                        static_cast<const NormalizedAxisStateEvent *>(pBoundEvent)->pCallback->Invoke(static_cast<float>(((pEvent->type == SDL_KEYDOWN) ? 1 : 0)));
                }
                break;
            }
        }
    }
    else if (!pKeyboardBind->CommandString.IsEmpty())
    {
        // invoke command on key down
        if (pEvent->type == SDL_KEYDOWN)
            g_pConsole->ExecuteText(pKeyboardBind->CommandString);
    }

    // handled
    return true;
}

bool InputManager::HandleMouseEvent(const SDL_Event *pEvent)
{
    switch (pEvent->type)
    {
    case SDL_MOUSEMOTION:
        {
            // 
            bool wasHandled = false;
            int32 relativeAmounts[2] = { pEvent->motion.xrel, pEvent->motion.yrel };
            for (uint32 axisIndex = 0; axisIndex < 2; axisIndex++)
            {
                if (relativeAmounts[axisIndex] == 0)
                    continue;

                // get direction
                int32 direction = Math::Sign(relativeAmounts[axisIndex]);

                // search bind set (if active)
                const MouseAxisBind *pAxisBind = nullptr;
                if (m_pActiveBindSet != nullptr)
                {
                    for (const MouseAxisBind &axisBind : m_pActiveBindSet->MouseAxisBinds)
                    {
                        if (axisBind.AxisIndex == axisIndex && (axisBind.BindDirection == 0 || axisBind.BindDirection == direction))
                        {
                            pAxisBind = &axisBind;
                            break;
                        }
                    }
                }

                // search global bindset
                if (pAxisBind == nullptr)
                {
                    for (const MouseAxisBind &axisBind : m_globalBindSet.MouseAxisBinds)
                    {
                        if (axisBind.AxisIndex == axisIndex && (axisBind.BindDirection == 0 || axisBind.BindDirection == direction))
                        {
                            pAxisBind = &axisBind;
                            break;
                        }
                    }
                }

                // skip if nothing was found
                if (pAxisBind == nullptr)
                    continue;

                // look up the event
                const BaseEvent *pBoundEvent = LookupEventByName(pAxisBind->EventName);
                if (pBoundEvent != nullptr)
                {
                    // handle it
                    switch (pBoundEvent->Type)
                    {
                    case EventType_Action:
                        {
                            // trip event
                            static_cast<const ActionEvent *>(pBoundEvent)->pCallback->Invoke();
                            wasHandled = true;
                        }
                        break;

                    case EventType_AxisState:
                        {
                            // calculate relative amount
                            static_cast<const AxisStateEvent *>(pBoundEvent)->pCallback->Invoke(relativeAmounts[axisIndex]);
                            wasHandled = true;
                        }
                        break;

                    case EventType_NormalizedAxisState:
                        {
                            // look up the window size, and calculate the normalized value based on that
                            SDL_Window *pSDLWindow = SDL_GetWindowFromID(pEvent->motion.windowID);
                            if (pSDLWindow != nullptr)
                            {
                                // get window dimensions
                                int32 winWidth, winHeight;
                                SDL_GetWindowSize(pSDLWindow, &winWidth, &winHeight);

                                // calculate normalized value
                                int32 currentPosition = (axisIndex == 0) ? pEvent->motion.x : pEvent->motion.y;
                                int32 axisSize = (axisIndex == 0) ? winWidth : winHeight;
                                float normalizedValue = Math::Clamp((float)currentPosition / (float)axisSize, 0.0f, 1.0f);

                                // pass through
                                static_cast<const NormalizedAxisStateEvent *>(pBoundEvent)->pCallback->Invoke(normalizedValue);
                                wasHandled = true;
                            }
                        }
                        break;
                    }
                }
            }

            // return if a handler was invoked
            return wasHandled;
        }
        break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        {
            // find the binding
            const MouseButtonBind *pButtonBind = nullptr;

            // search bind set (if active)
            if (m_pActiveBindSet != nullptr)
            {
                for (const MouseButtonBind &buttonBind : m_pActiveBindSet->MouseButtonBinds)
                {
                    if (buttonBind.ButtonIndex == pEvent->button.button)
                    {
                        pButtonBind = &buttonBind;
                        break;
                    }
                }
            }

            // search global bind set if we still didn't find anything
            if (pButtonBind == nullptr)
            {
                for (const MouseButtonBind &buttonBind : m_globalBindSet.MouseButtonBinds)
                {
                    if (buttonBind.ButtonIndex == pEvent->button.button)
                    {
                        pButtonBind = &buttonBind;
                        break;
                    }
                }
            }

            // if we didn't find anything, it's unhandled
            if (pButtonBind == nullptr)
                return false;

            // look up the event
            const BaseEvent *pBoundEvent = LookupEventByName(pButtonBind->EventName);
            if (pBoundEvent != nullptr)
            {
                // handle it
                switch (pBoundEvent->Type)
                {
                case EventType_Action:
                    {
                        // invoke action events on key down only
                        if (pEvent->type == SDL_MOUSEBUTTONDOWN)
                            static_cast<const ActionEvent *>(pBoundEvent)->pCallback->Invoke();
                    }
                    break;

                case EventType_BinaryState:
                    {
                        // binary state is 1 for down, or 0 for up
                        static_cast<const BinaryStateEvent *>(pBoundEvent)->pCallback->Invoke((pEvent->type == SDL_MOUSEBUTTONDOWN));
                    }
                    break;

                case EventType_AxisState:
                    {
                        // axis state is dependant on the bind direction, if it is set
                        if (pButtonBind->ActivateDirection != 0)
                            static_cast<const AxisStateEvent *>(pBoundEvent)->pCallback->Invoke(pButtonBind->ActivateDirection * ((pEvent->type == SDL_MOUSEBUTTONDOWN) ? 1 : 0));
                        else
                            static_cast<const AxisStateEvent *>(pBoundEvent)->pCallback->Invoke(((pEvent->type == SDL_MOUSEBUTTONDOWN) ? 1 : 0));
                    }
                    break;

                case EventType_NormalizedAxisState:
                    {
                        // axis state is dependant on the bind direction, if it is set
                        if (pButtonBind->ActivateDirection != 0)
                            static_cast<const NormalizedAxisStateEvent *>(pBoundEvent)->pCallback->Invoke(static_cast<float>(pButtonBind->ActivateDirection * ((pEvent->type == SDL_MOUSEBUTTONDOWN) ? 1 : 0)));
                        else
                            static_cast<const NormalizedAxisStateEvent *>(pBoundEvent)->pCallback->Invoke(static_cast<float>(((pEvent->type == SDL_MOUSEBUTTONDOWN) ? 1 : 0)));
                    }
                    break;
                }
            }

            // handled
            return true;
        }
        break;
 
    case SDL_MOUSEWHEEL:
        {
            // find the binding
            const MouseAxisBind *pAxisBind = nullptr;

            // get x/y directions
            int32 xDirection = Math::Sign(pEvent->wheel.x);
            int32 yDirection = Math::Sign(pEvent->wheel.x);

            // search bind set (if active)
            if (m_pActiveBindSet != nullptr)
            {
                for (const MouseAxisBind &axisBind : m_pActiveBindSet->MouseAxisBinds)
                {
                    if ((axisBind.AxisIndex == 3 && pEvent->wheel.x != 0 && (axisBind.BindDirection == 0 || axisBind.BindDirection == xDirection)) ||
                        (axisBind.AxisIndex == 2 && pEvent->wheel.y != 0 && (axisBind.BindDirection == 0 || axisBind.BindDirection == yDirection)))
                    {
                        pAxisBind = &axisBind;
                        break;
                    }
                }
            }

            // search global bind set if we still didn't find anything
            if (pAxisBind == nullptr)
            {
                for (const MouseAxisBind &axisBind : m_globalBindSet.MouseAxisBinds)
                {
                    if ((axisBind.AxisIndex == 3 && pEvent->wheel.x != 0 && (axisBind.BindDirection == 0 || axisBind.BindDirection == xDirection)) ||
                        (axisBind.AxisIndex == 2 && pEvent->wheel.y != 0 && (axisBind.BindDirection == 0 || axisBind.BindDirection == yDirection)))
                    {
                        pAxisBind = &axisBind;
                        break;
                    }
                }
            }

            // if we didn't find anything, it's unhandled
            if (pAxisBind == nullptr)
                return false;

            // look up the event
            const BaseEvent *pBoundEvent = LookupEventByName(pAxisBind->EventName);
            if (pBoundEvent != nullptr)
            {
                // handle it
                switch (pBoundEvent->Type)
                {
                case EventType_Action:
                    {
                        // invoke on any wheel action
                        static_cast<const ActionEvent *>(pBoundEvent)->pCallback->Invoke();
                    }
                    break;

                case EventType_BinaryState:
                    {
                        // binary state is 1 for up, or 0 for down
                        if (pAxisBind->AxisIndex == 2)
                            static_cast<const BinaryStateEvent *>(pBoundEvent)->pCallback->Invoke((pEvent->wheel.y <= 0) ? 1 : 0);
                        else
                            static_cast<const BinaryStateEvent *>(pBoundEvent)->pCallback->Invoke((pEvent->wheel.x <= 0) ? 0 : 1);
                    }
                    break;

                case EventType_AxisState:
                    {
                        if (pAxisBind->AxisIndex == 2)
                            static_cast<const AxisStateEvent *>(pBoundEvent)->pCallback->Invoke(pEvent->wheel.y);
                        else
                            static_cast<const AxisStateEvent *>(pBoundEvent)->pCallback->Invoke(pEvent->wheel.x);
                    }
                    break;
                }
            }

            // handled
            return true;
        }
        break;    
    }

    return false;
}

InputManager::OpenJoystick *InputManager::GetJoystickBySDLIndex(int32 index) 
{
    for (uint32 i = 0; i < m_joysticks.GetSize(); i++)
    {
        if (m_joysticks[i].SDLJoystickIndex == index)
            return &m_joysticks[i];
    }

    return nullptr;
}

bool InputManager::HandleJoystickEvent(const SDL_Event *pEvent)
{
    // skip anything for game controllers -- can't call this for removed event
    //if (SDL_IsGameController(pEvent->jdevice.which))
        //return false;

    return false;
}

InputManager::OpenController *InputManager::GetControllerBySDLIndex(int32 index)
{
    for (uint32 i = 0; i < m_controllers.GetSize(); i++)
    {
        if (m_controllers[i].SDLJoystickIndex == index)
            return &m_controllers[i];
    }

    return nullptr;
}

bool InputManager::HandleControllerEvent(const SDL_Event *pEvent)
{
    // mapping of sdl controller axis to our axis, buttons to our buttons
    static const ControllerAxis sdlControllerAxisMapping[SDL_CONTROLLER_AXIS_MAX] =
    {
        ControllerAxis_LeftStickX,      // SDL_CONTROLLER_AXIS_LEFTX
        ControllerAxis_LeftStickY,      // SDL_CONTROLLER_AXIS_LEFTY
        ControllerAxis_RightStickX,     // SDL_CONTROLLER_AXIS_RIGHTX
        ControllerAxis_RightStickY,     // SDL_CONTROLLER_AXIS_RIGHTY
        ControllerAxis_LeftTrigger,     // SDL_CONTROLLER_AXIS_TRIGGERLEFT
        ControllerAxis_RightTrigger,    // SDL_CONTROLLER_AXIS_TRIGGERRIGHT
    };
    static const ControllerButton sdlControllerButtonMapping[SDL_CONTROLLER_BUTTON_MAX] =
    {
        ControllerButton_A,             // SDL_CONTROLLER_BUTTON_A
        ControllerButton_B,             // SDL_CONTROLLER_BUTTON_B
        ControllerButton_X,             // SDL_CONTROLLER_BUTTON_X
        ControllerButton_Y,             // SDL_CONTROLLER_BUTTON_Y
        ControllerButton_Back,          // SDL_CONTROLLER_BUTTON_BACK
        ControllerButton_Guide,         // SDL_CONTROLLER_BUTTON_GUIDE
        ControllerButton_Start,         // SDL_CONTROLLER_BUTTON_START
        ControllerButton_LeftStick,     // SDL_CONTROLLER_BUTTON_LEFTSTICK
        ControllerButton_RightStick,    // SDL_CONTROLLER_BUTTON_RIGHTSTICK
        ControllerButton_LeftBumper,    // SDL_CONTROLLER_BUTTON_LEFTSHOULER,
        ControllerButton_RightBumper,   // SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
        ControllerButton_DPadUp,        // SDL_CONTROLLER_BUTTON_DPAD_UP
        ControllerButton_DPadDown,      // SDL_CONTROLLER_BUTTON_DPAD_DOWN
        ControllerButton_DPadLeft,      // SDL_CONTROLLER_BUTTON_DPAD_LEFT
        ControllerButton_DPadRight,     // SDL_CONTROLLER_BUTTON_DPAD_RIGHT
    };

    switch (pEvent->type)
    {
    case SDL_CONTROLLERDEVICEADDED:
        {
            Log_InfoPrintf("InputManager::HandleGameControllerEvent: Controller %i (%s) connected. Opening...", pEvent->cdevice.which, SDL_GameControllerNameForIndex(pEvent->cdevice.which));
            DebugAssert(GetControllerBySDLIndex(pEvent->cdevice.which) == nullptr);

            SDL_GameController *pGameController = SDL_GameControllerOpen(pEvent->cdevice.which);
            if (pGameController == nullptr)
            {
                Log_ErrorPrintf("InputManager::HandleGameControllerEvent: Failed to open controller %i: %s", pEvent->cdevice.which, SDL_GetError());
                return true;
            }

            OpenController gameController;
            gameController.Index = m_controllers.GetSize();
            gameController.SDLJoystickIndex = pEvent->cdevice.which;
            gameController.pSDLGameController = pGameController;

            // read the axis positions for relative movement
            Y_memzero(gameController.LastAxisPositions, sizeof(gameController.LastAxisPositions));
            for (uint32 sdlAxisIndex = 0; sdlAxisIndex < SDL_CONTROLLER_AXIS_MAX; sdlAxisIndex++)
                gameController.LastAxisPositions[sdlControllerAxisMapping[sdlAxisIndex]] = SDL_GameControllerGetAxis(pGameController, (SDL_GameControllerAxis)sdlAxisIndex);

            // add it
            m_controllers.Add(gameController);
            return true;
        }
        break;

    case SDL_CONTROLLERDEVICEREMOVED:
        {
            OpenController *pGameController = GetControllerBySDLIndex(pEvent->cdevice.which);
            DebugAssert(pGameController != nullptr);

            Log_InfoPrintf("InputManager::HandleGameControllerEvent: Controller %i (%s) disconnected.", pEvent->cdevice.which, SDL_GameControllerName(pGameController->pSDLGameController));
            SDL_GameControllerClose(pGameController->pSDLGameController);
            m_controllers.OrderedRemove(pGameController->Index);
            return true;
        }
        break;

    case SDL_CONTROLLERAXISMOTION:
        {
            OpenController *pGameController = GetControllerBySDLIndex(pEvent->caxis.which);
            if (pGameController == nullptr)
                return false;

            DebugAssert(pEvent->caxis.axis < SDL_CONTROLLER_AXIS_MAX);
            uint32 controllerIndex = pGameController->Index;
            ControllerAxis axis = sdlControllerAxisMapping[pEvent->caxis.axis];

            // negate stick axises, so that up is positive
            int32 axisValue = (int32)pEvent->caxis.value;
            if (axis == ControllerAxis_LeftStickY || axis == ControllerAxis_RightStickY)
                axisValue = -axisValue;

            // calculate relative value
            int32 relativeValue = axisValue - (int32)pGameController->LastAxisPositions[axis];
            pGameController->LastAxisPositions[axis] = (int16)axisValue;

            // find the binding
            const ControllerAxisBind *pAxisBind = nullptr;

            // search bind set (if active)
            if (m_pActiveBindSet != nullptr)
            {
                for (const ControllerAxisBind &axisBind : m_pActiveBindSet->ControllerAxisBinds)
                {
                    if (axisBind.ControllerIndex == controllerIndex && axisBind.Axis == axis)
                    {
                        pAxisBind = &axisBind;
                        break;
                    }
                }
            }

            // search global bind set if we still didn't find anything
            if (pAxisBind == nullptr)
            {
                for (const ControllerAxisBind &axisBind : m_globalBindSet.ControllerAxisBinds)
                {
                    if (axisBind.ControllerIndex == controllerIndex && axisBind.Axis == axis)
                    {
                        pAxisBind = &axisBind;
                        break;
                    }
                }
            }

            // if we didn't find anything, it's unhandled
            if (pAxisBind == nullptr)
                return false;

            // look up the event
            const BaseEvent *pBoundEvent = LookupEventByName(pAxisBind->EventName);
            if (pBoundEvent != nullptr)
            {
                // handle it
                switch (pBoundEvent->Type)
                {
                case EventType_Action:
                    {
                        // invoke action events whenever the axis position changes
                        static_cast<const ActionEvent *>(pBoundEvent)->pCallback->Invoke();
                    }
                    break;

                case EventType_BinaryState:
                    {
                        // binary state is off for <= 0, on for >= 0.. use 1/10th deadzone
                        static_cast<const BinaryStateEvent *>(pBoundEvent)->pCallback->Invoke((axisValue >= 3276));
                    }
                    break;

                case EventType_AxisState:
                    {
                        // pass through relative value
                        static_cast<const AxisStateEvent *>(pBoundEvent)->pCallback->Invoke(relativeValue);
                    }
                    break;

                case EventType_NormalizedAxisState:
                    {
                        // normalize it to -1..1
                        float normalizedValue = Math::Clamp((float)axisValue / 32767.0f, -1.0f, 1.0f);

                        // invoke event
                        static_cast<const NormalizedAxisStateEvent *>(pBoundEvent)->pCallback->Invoke(normalizedValue);
                    }
                    break;
                }
            }

            // handled
            return true;
        }
        break;
        
    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP:
        {
            OpenController *pGameController = GetControllerBySDLIndex(pEvent->cbutton.which);
            if (pGameController == nullptr)
                return false;

            DebugAssert(pEvent->cbutton.button < SDL_CONTROLLER_BUTTON_MAX);
            uint32 controllerIndex = pGameController->Index;
            ControllerButton button = sdlControllerButtonMapping[pEvent->cbutton.button];
            bool buttonPressed = (pEvent->cbutton.state == SDL_PRESSED);

            // find the binding
            const ControllerButtonBind *pButtonBind = nullptr;

            // search bind set (if active)
            if (m_pActiveBindSet != nullptr)
            {
                for (const ControllerButtonBind &buttonBind : m_pActiveBindSet->ControllerButtonBinds)
                {
                    if (buttonBind.ControllerIndex == controllerIndex && buttonBind.Button == button)
                    {
                        pButtonBind = &buttonBind;
                        break;
                    }
                }
            }

            // search global bind set if we still didn't find anything
            if (pButtonBind == nullptr)
            {
                for (const ControllerButtonBind &buttonBind : m_globalBindSet.ControllerButtonBinds)
                {
                    if (buttonBind.ControllerIndex == controllerIndex && buttonBind.Button == button)
                    {
                        pButtonBind = &buttonBind;
                        break;
                    }
                }
            }

            // if we didn't find anything, it's unhandled
            if (pButtonBind == nullptr)
                return false;

            // look up the event
            const BaseEvent *pBoundEvent = LookupEventByName(pButtonBind->EventName);
            if (pBoundEvent != nullptr)
            {
                // handle it
                switch (pBoundEvent->Type)
                {
                case EventType_Action:
                    {
                        // invoke action events whenever the button is pushed down
                        if (buttonPressed)
                            static_cast<const ActionEvent *>(pBoundEvent)->pCallback->Invoke();
                    }
                    break;

                case EventType_BinaryState:
                    {
                        // binary state is off for <= 0, on for >= 0
                        static_cast<const BinaryStateEvent *>(pBoundEvent)->pCallback->Invoke(buttonPressed);
                    }
                    break;

                case EventType_AxisState:
                    {
                        // axis state is dependant on the bind direction, if it is set
                        if (pButtonBind->ActivateDirection != 0)
                            static_cast<const AxisStateEvent *>(pBoundEvent)->pCallback->Invoke(pButtonBind->ActivateDirection * ((buttonPressed) ? 1 : 0));
                        else
                            static_cast<const AxisStateEvent *>(pBoundEvent)->pCallback->Invoke(((buttonPressed) ? 1 : 0));
                    }
                    break;

                case EventType_NormalizedAxisState:
                    {
                        // axis state is dependant on the bind direction, if it is set
                        if (pButtonBind->ActivateDirection != 0)
                            static_cast<const NormalizedAxisStateEvent *>(pBoundEvent)->pCallback->Invoke(static_cast<float>(pButtonBind->ActivateDirection * ((buttonPressed) ? 1 : 0)));
                        else
                            static_cast<const NormalizedAxisStateEvent *>(pBoundEvent)->pCallback->Invoke(static_cast<float>(((buttonPressed) ? 1 : 0)));
                    }
                    break;
                }
            }

            // handled
            return true;
        }
        break;
    }

    return false;
}
