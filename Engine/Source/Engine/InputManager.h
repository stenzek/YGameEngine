#pragma once
#include "Engine/Common.h"
// #include "Engine/InputTypes.h"
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <SDL_joystick.h>
#include <SDL_gamecontroller.h>
#include <SDL_events.h>

class InputManager
{
public:
    enum EventType
    {
        EventType_Action,
        EventType_BinaryState,
        EventType_AxisState,
        EventType_NormalizedAxisState,
        EventType_Count,
    };

    // these map to the 360 controller, with a few extras
    enum ControllerAxis
    {
        ControllerAxis_LeftStickX,
        ControllerAxis_LeftStickY,
        ControllerAxis_RightStickX,
        ControllerAxis_RightStickY,
        ControllerAxis_LeftTrigger,
        ControllerAxis_RightTrigger,
        ControllerAxis_Extra1,
        ControllerAxis_Extra2,
        ControllerAxis_Count,
    };

    // same for buttons
    enum ControllerButton
    {
        ControllerButton_A,
        ControllerButton_B,
        ControllerButton_X,
        ControllerButton_Y,
        ControllerButton_LeftBumper,
        ControllerButton_RightBumper,
        ControllerButton_LeftStick,
        ControllerButton_RightStick,
        ControllerButton_DPadLeft,
        ControllerButton_DPadRight,
        ControllerButton_DPadUp,
        ControllerButton_DPadDown,
        ControllerButton_Back,
        ControllerButton_Start,
        ControllerButton_Guide,
        ControllerButton_Extra1,
        ControllerButton_Extra2,
        ControllerButton_Extra3,
        ControllerButton_Extra4,
        ControllerButton_Extra5,
        ControllerButton_Count,
    };

    struct BaseEvent
    {
        BaseEvent(EventType type, const void *pOwner, const char *name, const char *description) : Type(type), pOwnerPointer(pOwner), Name(name), Description(description) {}
        virtual ~BaseEvent() {}

        EventType Type;
        const void *pOwnerPointer;
        String Name;
        String Description;
    };

    struct ActionEvent : public BaseEvent
    {
        typedef Functor CallbackType;

        ActionEvent(const void *pOwner, const char *name, const char *description, CallbackType *callback) : BaseEvent(EventType_Action, pOwner, name, description), pCallback(callback) {}
        virtual ~ActionEvent() { delete pCallback; }

        CallbackType *pCallback;
    };

    struct BinaryStateEvent : public BaseEvent
    {
        typedef FunctorA1<bool> CallbackType;

        BinaryStateEvent(const void *pOwner, const char *name, const char *description, CallbackType *callback) : BaseEvent(EventType_BinaryState, pOwner, name, description), pCallback(callback) {}
        virtual ~BinaryStateEvent() { delete pCallback; }

        CallbackType *pCallback;
    };

    struct AxisStateEvent : public BaseEvent
    {
        typedef FunctorA1<int32> CallbackType;

        AxisStateEvent(const void *pOwner, const char *name, const char *description, CallbackType *callback) : BaseEvent(EventType_AxisState, pOwner, name, description), pCallback(callback) {}
        virtual ~AxisStateEvent() { delete pCallback; }

        CallbackType *pCallback;
    };

    struct NormalizedAxisStateEvent : public BaseEvent
    {
        typedef FunctorA1<float> CallbackType;

        NormalizedAxisStateEvent(const void *pOwner, const char *name, const char *description, CallbackType *callback) : BaseEvent(EventType_NormalizedAxisState, pOwner, name, description), pCallback(callback) {}
        virtual ~NormalizedAxisStateEvent() { delete pCallback; }

        CallbackType *pCallback;
    };

//     // for binds
//     enum MouseAxis
//     {
//         MouseAxis_X,
//         MouseAxis_Y,
//         MouseAxis_Count,
//     };
//     enum MouseButton
//     {
//         MouseButton_Left,
//         MouseButton_Right,
//         MouseButton_Middle,
//         MouseButton_Aux1,
//         MouseButton_Aux2,
//         MouseButton_Aux3,
//         MouseButton_Aux4,
//         MouseButton_Aux5,
//         MouseButton_Aux6,
//         MouseButton_Aux7,
//         MouseButton_Aux8,
//         MouseButton_Aux9,
//         MouseButton_Count,
//     };

public:
    InputManager();
    ~InputManager();

    bool Startup();
    void Shutdown();

    // --- register events. pointer ownership is transferred to input manager. ---

    // Action event, executed once when button is pressed
    const ActionEvent *RegisterActionEvent(const void *pOwnerPointer, const char *eventName, const char *displayName, ActionEvent::CallbackType *pCallback);

    // Binary state event, executed whenever the state changes
    const BinaryStateEvent *RegisterBinaryStateEvent(const void *pOwnerPointer, const char *eventName, const char *displayName, BinaryStateEvent::CallbackType *pCallback);

    // Axis state, executed whenever the state changes
    const AxisStateEvent *RegisterAxisStateEvent(const void *pOwnerPointer, const char *eventName, const char *displayName, AxisStateEvent::CallbackType *pCallback);

    // Normalized state, executed whenever the state changes
    const NormalizedAxisStateEvent *RegisterNormalizedAxisStateEvent(const void *pOwnerPointer, const char *eventName, const char *displayName, NormalizedAxisStateEvent::CallbackType *pCallback);

    // Event lookup
    const BaseEvent *LookupEventByName(const char *eventName);

    // Deregister an event
    void UnregisterEvent(const BaseEvent *pEvent);
    bool UnregisterEvent(const char *eventName);

    // Deregister all events matching an owner
    void UnregisterEventsWithOwner(const void *pOwnerPointer);

    // Bind something to events, set eventName to null to unbind
    bool BindKeyboardKey(const char *bindSetName, const char *keyName, const char *eventName, int32 activateDirection = 0);
    bool BindMouseAxis(const char *bindSetName, uint32 axisIndex, const char *eventName, int32 bindDirection = 0);
    bool BindMouseButton(const char *bindSetName, uint32 buttonIndex, const char *eventName, int32 activateDirection = 0);
    bool BindControllerAxis(const char *bindSetName, uint32 controllerIndex, ControllerAxis axis, const char *eventName);
    bool BindControllerButton(const char *bindSetName, uint32 controllerIndex, ControllerButton button, const char *eventName, int32 activateDirection = 0);

    // Bind something to events, set eventName to null to unbind (operates on global bind set)
    bool BindKeyboardKey(const char *keyName, const char *eventName, int32 activateDirection = 0) { return BindKeyboardKey(nullptr, keyName, eventName, activateDirection); }
    bool BindMouseAxis(uint32 axisIndex, const char *eventName, int32 bindDirection = 0) { return BindMouseAxis(nullptr, axisIndex, eventName, bindDirection); }
    bool BindMouseButton(uint32 buttonIndex, const char *eventName, int32 activateDirection = 0) { return BindMouseButton(nullptr, buttonIndex, eventName, activateDirection); }
    bool BindControllerAxis(uint32 controllerIndex, ControllerAxis axis, const char *eventName) { return BindControllerAxis(nullptr, controllerIndex, axis, eventName); }
    bool BindControllerButton(uint32 controllerIndex, ControllerButton button, const char *eventName, int32 activateDirection = 0) { return BindControllerButton(nullptr, controllerIndex, button, eventName, activateDirection); }

    // Bind set switcher
    bool SwitchBindSet(const char *bindSetName);
    
    // SDL event handlers
    bool HandleSDLEvent(const void *pEvent);

    // Event blocker, prevents input manager from handling events temporarily
    void PushEventBlocker() { m_eventBlockerCount++; }
    void PopEventBlocker() { DebugAssert(m_eventBlockerCount > 0); m_eventBlockerCount--; }
    
private:
    //bool OpenJoysticks();
    void CloseJoysticks();

    // event handlers
    bool HandleKeyboardEvent(const SDL_Event *pEvent);
    bool HandleMouseEvent(const SDL_Event *pEvent);
    bool HandleJoystickEvent(const SDL_Event *pEvent);
    bool HandleControllerEvent(const SDL_Event *pEvent);

    struct OpenJoystick
    {
        uint32 Index;
        int32 SDLJoystickIndex;
        SDL_Joystick *pSDLJoystick;
    };

    struct OpenController
    {
        uint32 Index;
        int32 SDLJoystickIndex;
        SDL_GameController *pSDLGameController;

        // we track the last axis value, that way we can pass a relative value
        int16 LastAxisPositions[ControllerAxis_Count];
    };

    MemArray<OpenJoystick> m_joysticks;
    MemArray<OpenController> m_controllers;

    OpenJoystick *GetJoystickBySDLIndex(int32 index);
    OpenController *GetControllerBySDLIndex(int32 index);

    typedef CIStringHashTable<BaseEvent *> EventHashTable;
    EventHashTable m_events;

    struct KeyboardBind
    {
        uint32 SDLScanCode;
        String EventName;
        int32 ActivateDirection;
        String CommandString;
    };
    struct MouseAxisBind
    {
        uint32 AxisIndex;
        int32 BindDirection;
        String EventName;
    };
    struct MouseButtonBind
    {
        uint32 ButtonIndex;
        String EventName;
        int32 ActivateDirection;
    };
    struct ControllerAxisBind
    {
        uint32 ControllerIndex;
        ControllerAxis Axis;
        String EventName;
    };
    struct ControllerButtonBind
    {
        uint32 ControllerIndex;
        ControllerButton Button;
        String EventName;
        int32 ActivateDirection;
    };

    struct BindSet
    {
        String Name;
        Array<KeyboardBind> KeyboardBinds;
        Array<MouseAxisBind> MouseAxisBinds;
        Array<MouseButtonBind> MouseButtonBinds;
        Array<ControllerAxisBind> ControllerAxisBinds;
        Array<ControllerButtonBind> ControllerButtonBinds;
    };

    // global bind set
    BindSet m_globalBindSet;

    // bind set table
    BindSet *GetOrCreateBindSetByName(const char *bindSetName);
    typedef CIStringHashTable <BindSet *> BindSetTable;
    BindSetTable m_bindSets;

    // currently active bind set
    BindSet *m_pActiveBindSet;

    // blocker
    uint32 m_eventBlockerCount;
};

extern InputManager *g_pInputManager;
