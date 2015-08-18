#pragma once
#include "Core/Common.h"
#include "YBaseLib/String.h"
#include "YBaseLib/Functor.h"
#include "YBaseLib/PODArray.h"
#include "YBaseLib/Singleton.h"
#include "YBaseLib/CIStringHashTable.h"
#include "YBaseLib/Mutex.h"

// Console.h -> CVar.h ?

enum CVAR_FLAG
{
    CVAR_FLAG_REGISTERED                = (1 << 0),
    CVAR_FLAG_WAS_UNREGISTERED          = (1 << 1),
    CVAR_FLAG_READ_ONLY                 = (1 << 2),
    CVAR_FLAG_MODIFIED                  = (1 << 3),
    CVAR_FLAG_NO_ARCHIVE                = (1 << 4),
    CVAR_FLAG_REQUIRE_APP_RESTART       = (1 << 5),
    CVAR_FLAG_REQUIRE_RENDER_RESTART    = (1 << 6),
    CVAR_FLAG_REQUIRE_MAP_RESTART       = (1 << 7),
    CVAR_FLAG_PAUSE_RENDER_THREAD       = (1 << 8),
    CVAR_FLAG_CHEAT                     = (1 << 9),
};

class CVar
{
    friend class Console;

public:
    CVar(const char *Name, uint32 Flags, const char *DefaultValue = NULL, const char *Help = NULL, const char *Domain = NULL);
    ~CVar();

    inline const String &GetName() const { return m_strName; }
    inline uint32 GetFlags() const { return m_iFlags; }
    inline const String &GetDefaultValue() const { return m_strDefaultValue; }
    inline const String &GetHelp() const { return m_strHelp; }
    inline const String &GetDomain() const { return m_strDomain; }

    inline const String &GetString() const { return m_strStrValue; }
    inline int32 GetInt() const { return m_iIntValue; }
    inline uint32 GetUInt() const { return (uint32)m_iIntValue; }
    inline float GetFloat() const { return m_fFloatValue; }
    inline bool GetBool() const { return m_bBoolValue; }

    inline bool IsChangePending() const { return m_bChangePending; }

    void AddChangeCallback(Functor *pCallback);
    void RemoveChangeCallback(Functor *pCallback);

    const String &GetPendingString() const;
    int32 GetPendingInt() const;
    uint32 GetPendingUInt() const;
    float GetPendingFloat() const;
    bool GetPendingBool() const;

private:
    String m_strName;
    uint32 m_iFlags;
    String m_strDefaultValue;
    String m_strHelp;
    String m_strDomain;
    
    String m_strStrValue;
    int32 m_iIntValue;
    float m_fFloatValue;
    bool m_bBoolValue;

    bool m_bChangePending;
    String m_strPendingValue;

    PODArray<Functor *> m_callbacks;
};

class Console : public Singleton<Console>
{
    friend class CVar;

public:
    typedef bool(*CommandHandlerMethod)(void *userData, uint32 argumentCount, const char *const argumentValues[]);

public:
    Console();
    ~Console();

    ////////////////////////////////////////////////////////////////////////////////////
    // CVar Management
    ////////////////////////////////////////////////////////////////////////////////////
    CVar *GetCVar(const char *Name);
    bool SetCVar(CVar *pCVar, const char *Value, bool Force = false);
    bool SetCVar(CVar *pCVar, uint32 Value, bool Force = false);
    bool SetCVar(CVar *pCVar, int32 Value, bool Force = false);
    bool SetCVar(CVar *pCVar, float Value, bool Force = false);
    bool SetCVar(CVar *pCVar, bool Value, bool Force = false);
    bool ResetCVar(CVar *pCVar);

    // this function will create unregistered cvars if one is not present
    bool SetCVarByName(const char *Name, const char *Value, bool Force = false, bool CreateUnregistered = true);

    // Called when app is started
    void ApplyPendingAppCVars();

    // Called when the renderer is restarted.
    void ApplyPendingRenderCVars();

    // command line parser for any fs switches
    uint32 ParseCommandLine(uint32 argc, const char **argv);

    ///////////////////////////////////////////////////////////////////////////////////
    // Command Management
    ///////////////////////////////////////////////////////////////////////////////////
    bool RegisterCommand(const char *commandName, CommandHandlerMethod executeMethod, CommandHandlerMethod helpMethod, void *userData, const void *ownerPtr = nullptr);
    bool UnregisterCommand(const char *commandName);
    uint32 UnregisterCommandsWithOwner(const void *ownerPtr);

    // execute text
    bool ExecuteText(const char *text);

    // handle autocompletion
    bool HandleAutoCompletion(String &commandString);

    // handle help
    bool HandlePartialHelp(const char *text);

private:
    // Console lock
    void AcquireMutex() { m_consoleLock.Lock(); }
    void ReleaseMutex() { m_consoleLock.Unlock(); }

    // CVar manangement
    void RegisterCVar(CVar *pCVar);
    void RemoveCVar(CVar *pCVar);
    void CreateUnregisteredCVar(const char *Name, const char *Value);
    bool ValidateCVar(CVar *pCVar, const char *NewValue);
    bool UpdateCVar(CVar *pCVar, const char *NewValue, bool Force);
    void WriteCVar(CVar *pCVar, const char *NewValue);

    // log the cvar information
    void DumpCVarHelp(const CVar *pCVar);
    void DumpCVarValue(const CVar *pCVar);

    // command handler for a cvar, expects the cvar pointer in userData
    static bool CVarViewEditCommandExecuteHandler(void *userData, uint32 argumentCount, const char *const argumentValues[]);
    static bool CVarViewEditCommandHelpHandler(void *userData, uint32 argumentCount, const char *const argumentValues[]);

private:
    typedef CIStringHashTable<CVar *> CVarHashTable;
    CVarHashTable m_hmCVars;
    Mutex m_consoleLock;

    struct RegisteredCommand
    {
        String Name;
        CommandHandlerMethod ExecuteMethod;
        CommandHandlerMethod HelpMethod;
        void *pUserData;
        const void *pOwnerPtr;
    };
    typedef CIStringHashTable<RegisteredCommand> CommandTable;
    CommandTable m_commands;

    bool m_appStarted;
    bool m_rendererStarted;
    bool m_hasPendingRenderCVars;
};

extern Console *g_pConsole;
