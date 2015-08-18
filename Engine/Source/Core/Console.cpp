#include "Core/PrecompiledHeader.h"
#include "Core/Console.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/StringConverter.h"
Log_SetChannel(Console);

Console *g_pConsole = NULL;

Console::Console()
    : m_appStarted(false),
      m_rendererStarted(false),
      m_hasPendingRenderCVars(false)
{
    DebugAssert(g_pConsole == NULL);
    g_pConsole = this;
}

Console::~Console()
{
    DebugAssert(g_pConsole == this);
    g_pConsole = NULL;
}

void Console::RegisterCVar(CVar *pCVar)
{
    // make sure there's no unregistered cvar with this name
    CVar *pUnregisteredCVar = NULL;
    CVarHashTable::Member *pMember = m_hmCVars.Find(pCVar->m_strName.GetCharArray());
    if (pMember != NULL)
    {
        // if it's registered, we have duplicate cvar names somewhere.. this is bad.
        DebugAssert(!(pMember->Value->m_iFlags & CVAR_FLAG_REGISTERED));
        pUnregisteredCVar = pMember->Value;
    }

    // set registered flag
    DebugAssert(!(pCVar->m_iFlags & CVAR_FLAG_REGISTERED));
    pCVar->m_iFlags |= CVAR_FLAG_REGISTERED;

    // store in map
    if (pMember != NULL)
        pMember->Value = pCVar;
    else
        pMember = m_hmCVars.Insert(pCVar->GetName().GetCharArray(), pCVar);

    // if unregistered, set value to what the unregistered cvar was set to
    if (pUnregisteredCVar != NULL)
    {
        pCVar->m_iFlags |= CVAR_FLAG_WAS_UNREGISTERED;
        if (!UpdateCVar(pCVar, pUnregisteredCVar->m_strStrValue.GetCharArray(), false))
            Log_WarningPrintf("Could not set unregistered value for CVar '%s' (value: '%s')", pCVar->m_strName.GetCharArray(), pUnregisteredCVar->m_strStrValue.GetCharArray());

        // and free the unregistered one (shouldn't have any external references, as we never return unregistered vars)
        delete pUnregisteredCVar;
    }
    else
    {
        // set value to default
        WriteCVar(pCVar, pCVar->m_strDefaultValue.GetCharArray());
    }

    // create a command for it
    if (m_commands.Find(pCVar->GetName()) == nullptr)
    {
        RegisteredCommand command;
        command.Name = pCVar->GetName();
        command.ExecuteMethod = CVarViewEditCommandExecuteHandler;
        command.HelpMethod = CVarViewEditCommandHelpHandler;
        command.pUserData = reinterpret_cast<void *>(pCVar);
        m_commands.Insert(command.Name, command);
    }
}

void Console::RemoveCVar(CVar *pCVar)
{
    CVarHashTable::Member *pMember = m_hmCVars.Find(pCVar->m_strName.GetCharArray());
    DebugAssert(pMember != NULL && pMember->Value == pCVar);
    m_hmCVars.Remove(pMember);

    // remove the command
    CommandTable::Member *pCommandTableMember = m_commands.Find(pCVar->GetName());
    if (pCommandTableMember != nullptr && pCommandTableMember->Value.pUserData == pCVar)
        m_commands.Remove(pCommandTableMember);
}

CVar *Console::GetCVar(const char *Name)
{
    CVarHashTable::Member *pMember = m_hmCVars.Find(Name);
    return (pMember != NULL && pMember->Value->m_iFlags & CVAR_FLAG_REGISTERED) ? pMember->Value : NULL;
}

bool Console::SetCVarByName(const char *Name, const char *Value, bool Force /* = false */, bool CreateUnregistered /* = true */)
{
    CVarHashTable::Member *pMember = m_hmCVars.Find(Name);
    if (pMember == NULL)
        return UpdateCVar(new CVar(Name, 0, NULL, NULL, NULL), Value, Force);
    else
        return UpdateCVar(pMember->Value, Value, Force);
}

bool Console::SetCVar(CVar *pCVar, const char *Value, bool Force /* = false */)
{
    DebugAssert(pCVar->m_iFlags & CVAR_FLAG_REGISTERED);
    return UpdateCVar(pCVar, Value, Force);
}

bool Console::SetCVar(CVar *pCVar, uint32 Value, bool Force /* = false */)
{
    char Str[128];
    Y_strfromuint32(Str, countof(Str), Value);

    DebugAssert(pCVar->m_iFlags & CVAR_FLAG_REGISTERED);
    return UpdateCVar(pCVar, Str, Force);
}

bool Console::SetCVar(CVar *pCVar, int32 Value, bool Force /* = false */)
{
    char Str[128];
    Y_strfromint32(Str, countof(Str), Value);

    DebugAssert(pCVar->m_iFlags & CVAR_FLAG_REGISTERED);
    return UpdateCVar(pCVar, Str, Force);
}

bool Console::SetCVar(CVar *pCVar, float Value, bool Force /* = false */)
{
    char Str[128];
    Y_strfromfloat(Str, countof(Str), Value);

    DebugAssert(pCVar->m_iFlags & CVAR_FLAG_REGISTERED);
    return UpdateCVar(pCVar, Str, Force);
}

bool Console::SetCVar(CVar *pCVar, bool Value, bool Force /* = false */)
{
    char Str[128];
    Y_strfrombool(Str, countof(Str), Value);

    DebugAssert(pCVar->m_iFlags & CVAR_FLAG_REGISTERED);
    return UpdateCVar(pCVar, Str, Force);
}

bool Console::ResetCVar(CVar *pCVar)
{
    DebugAssert(pCVar->m_iFlags & CVAR_FLAG_REGISTERED);
    return UpdateCVar(pCVar, pCVar->m_strDefaultValue.GetCharArray(), false);
}

bool Console::UpdateCVar(CVar *pCVar, const char *NewValue, bool Force)
{
    AcquireMutex();

    // todo validation

    // update modified flag
    if (pCVar->m_strDefaultValue.Compare(NewValue))
        pCVar->m_iFlags &= ~CVAR_FLAG_MODIFIED;
    else
        pCVar->m_iFlags |= CVAR_FLAG_MODIFIED;

    // pending set?
    if (pCVar->m_iFlags & CVAR_FLAG_REQUIRE_APP_RESTART && m_appStarted)
    {
        Log_InfoPrintf("\"%s\" will be set upon application restart.", pCVar->m_strName.GetCharArray());
        pCVar->m_strPendingValue = NewValue;
        pCVar->m_bChangePending = true;
        ReleaseMutex();
        return true;
    }
    else if (pCVar->m_iFlags & CVAR_FLAG_REQUIRE_RENDER_RESTART && m_rendererStarted)
    {
        Log_InfoPrintf("\"%s\" will be set upon render restart.", pCVar->m_strName.GetCharArray());
        pCVar->m_strPendingValue = NewValue;
        pCVar->m_bChangePending = true;
        m_hasPendingRenderCVars = true;
        ReleaseMutex();
        return true;
    }

    // update actual value
    WriteCVar(pCVar, NewValue);
    ReleaseMutex();
    return true;
}

void Console::WriteCVar(CVar *pCVar, const char *NewValue)
{
    pCVar->m_strStrValue = NewValue;
    pCVar->m_iIntValue = Y_strtoint32(pCVar->m_strStrValue.GetCharArray(), NULL);
    pCVar->m_fFloatValue = Y_strtofloat(pCVar->m_strStrValue.GetCharArray(), NULL);
    pCVar->m_bBoolValue = Y_strtobool(pCVar->m_strStrValue.GetCharArray(), NULL);

    for (uint32 i = 0; i < pCVar->m_callbacks.GetSize(); i++)
        pCVar->m_callbacks[i]->Invoke();
}

void Console::ApplyPendingAppCVars()
{
    Log_InfoPrintf("Console::ApplyPendingAppCVars()");
    m_appStarted = true;

    for (CVarHashTable::Iterator itr = m_hmCVars.Begin(); !itr.AtEnd(); itr.Forward())
    {
        CVar *pCVar = itr->Value;
        if (pCVar->m_iFlags & CVAR_FLAG_REQUIRE_APP_RESTART && pCVar->m_bChangePending)
        {
            WriteCVar(pCVar, pCVar->m_strPendingValue.GetCharArray());
            pCVar->m_strPendingValue.Obliterate();
            pCVar->m_bChangePending = false;
        }
    }
}

void Console::ApplyPendingRenderCVars()
{
    Log_InfoPrintf("Console::ApplyPendingRenderCVars()");
    m_rendererStarted = true;

    for (CVarHashTable::Iterator itr = m_hmCVars.Begin(); !itr.AtEnd(); itr.Forward())
    {
        CVar *pCVar = itr->Value;
        if (pCVar->m_iFlags & CVAR_FLAG_REQUIRE_RENDER_RESTART && pCVar->m_bChangePending)
        {
            Log_DevPrintf("    %s -> '%s'", pCVar->GetName().GetCharArray(), pCVar->m_strPendingValue.GetCharArray());
            WriteCVar(pCVar, pCVar->m_strPendingValue.GetCharArray());
            pCVar->m_strPendingValue.Obliterate();
            pCVar->m_bChangePending = false;
        }
    }

    m_hasPendingRenderCVars = false;
}

uint32 Console::ParseCommandLine(uint32 argc, const char **argv)
{
        /*
#define CHECK_ARG(str) !Y_strcmp(argv[i], str)
#define CHECK_ARG_PARAM(str) !Y_strcmp(argv[i], str) && ((i + 1) < argc)

    // argv[0] always contains the path to the binary, so we ignore it.
    if (argc < 2)
        return true;
       
    // start reading arguments
    for (int i = 1; i < argc; )
    {
        if (CHECK_ARG_PARAM("-fs_basepath"))        // alter base path
            g_pVirtualFileSystem->SetBasePath(argv[++i]);
        else if (CHECK_ARG_PARAM("-fs_userpath"))   // alter user path
            g_pVirtualFileSystem->SetUserPath(argv[++i]);
        else if (CHECK_ARG_PARAM("-fs_gamedir"))    // alter game name
            g_pVirtualFileSystem->SetGameDirectory(argv[++i]);

        i++;
    }

    return true;

#undef*/

    // argv[0] always contains the path to the binary, so we ignore it.
    if (argc < 2)
        return argc - 1;

    // start reading arguments
    for (uint32 i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if ((i + 1) < argc)
            {
                const char *cvarName = &argv[i][1];
                const char *cvarValue = argv[++i];

                // possible cvar
                CVarHashTable::Iterator itr;
                for (itr = m_hmCVars.Begin(); !itr.AtEnd(); itr.Forward())
                {
                    CVar *pCVar = itr->Value;
                    if (pCVar->GetName().CompareInsensitive(cvarName))
                    {
                        // set to this cvar
                        if (UpdateCVar(pCVar, cvarValue, false))
                            Log_DevPrintf("Console::ParseCommandLine: Set cvar '%s' to '%s'", cvarName, cvarValue);
                        else
                            Log_WarningPrintf("Console::ParseCommandLine: Setting cvar '%s' to value '%s' failed.", pCVar->GetName().GetCharArray(), cvarValue);

                        break;
                    }
                }
                if (itr.AtEnd())
                    Log_WarningPrintf("Console::ParseCommandLine: Did not find a cvar named '%s' in registry.", cvarName);
            }
        }
        else
        {
            return i;
        }
    }

    return argc;
}

bool Console::ExecuteText(const char *text)
{
    uint32 textLength = Y_strlen(text);
    if (textLength == 0)
        return false;

    // echo it
    Log_InfoPrintf("> %s", text);

    // copy the string to local memory
    char *tempString = (char *)alloca(textLength + 1);
    Y_memcpy(tempString, text, textLength + 1);

    // tokenize it
    char *tokens[32];
    uint32 nTokens = Y_strsplit2(tempString, ' ', tokens, countof(tokens));
    DebugAssert(nTokens > 0);

    // lookup registered commands
    CommandTable::Member *pMember = m_commands.Find(tokens[0]);
    if (pMember == nullptr)
    {
        // log an error
        Log_ErrorPrintf("No command or variable named '%s' was found.", tokens[0]);
        return false;
    }

    // execute it
    return pMember->Value.ExecuteMethod(pMember->Value.pUserData, nTokens, tokens);
}

bool Console::HandlePartialHelp(const char *text)
{
    uint32 textLength = Y_strlen(text);
    if (textLength == 0)
        return false;

    // copy the string to local memory
    char *tempString = (char *)alloca(textLength + 1);
    Y_memcpy(tempString, text, textLength + 1);

    // tokenize it
    char *tokens[32];
    uint32 nTokens = Y_strsplit2(tempString, ' ', tokens, countof(tokens));
    DebugAssert(nTokens > 0);

    // lookup registered commands, is there an exact match?
    const CommandTable::Member *pMember = m_commands.Find(tokens[0]);
    if (pMember != nullptr)
    {
        // refer to the command's help handler
        Log_InfoPrintf("> %s", text);
        return pMember->Value.HelpMethod(pMember->Value.pUserData, nTokens, tokens);
    }

    // find the closest match, if there is more than one, dump everything starting with this name
    const CommandTable::Member *pClosestMatch = nullptr;
    for (CommandTable::ConstIterator itr = m_commands.Begin(); !itr.AtEnd(); itr.Forward())
    {
        // begins with it?
        if (itr->Value.Name.StartsWith(tokens[0], false))
        {
            // found one already?
            if (pClosestMatch == nullptr)
            {
                pClosestMatch = &(*itr);
                continue;
            }

            // more than one command starts with this prefix, so find everything starting with it, dump it in a temp array
            PODArray<const RegisteredCommand *> foundCommands;
            for (CommandTable::ConstIterator innerItr = m_commands.Begin(); !innerItr.AtEnd(); innerItr.Forward())
            {
                if (innerItr->Value.Name.StartsWith(tokens[0], false))
                    foundCommands.Add(&innerItr->Value);
            }

            // sort them in alphabetical order
            foundCommands.SortCB([](const RegisteredCommand *pLeft, const RegisteredCommand *pRight) {
                return pLeft->Name.NumericCompareInsensitive(pRight->Name);
            });

            // display found entries
            Log_InfoPrintf("> %s [%u matches listed]:", text, foundCommands.GetSize());
            for (uint32 i = 0; i < foundCommands.GetSize(); i++)
                Log_InfoPrintf("  %s", foundCommands[i]->Name.GetCharArray());

            // done for now
            return true;
        }
    }

    // execute the closest match
    if (pClosestMatch != nullptr)
    {
        // refer to the command's help handler
        Log_InfoPrintf(">%s", text);
        return pClosestMatch->Value.HelpMethod(pMember->Value.pUserData, nTokens, tokens);
    }

    // nope.avi
    Log_InfoPrintf(">%s", text);
    Log_ErrorPrintf("  No matches");
    return false;
}

bool Console::HandleAutoCompletion(String &commandString)
{
    // currently unsupported for arguments
    if (commandString.Find(' ') >= 0)
        return false;

    // lookup registered commands, is there an exact match?
    const CommandTable::Member *pMember = m_commands.Find(commandString);
    if (pMember != nullptr)
    {
        // already done
        return true;
    }

    // find the closest match, if there is more than one, dump everything starting with this name
    const CommandTable::Member *pClosestMatch = nullptr;
    for (CommandTable::ConstIterator itr = m_commands.Begin(); !itr.AtEnd(); itr.Forward())
    {
        // begins with it?
        if (itr->Value.Name.StartsWith(commandString, false))
        {
            // found one already?
            if (pClosestMatch == nullptr)
            {
                pClosestMatch = &(*itr);
                continue;
            }

            // more than one command starts with this prefix, so find everything starting with it, dump it in a temp array
            PODArray<const RegisteredCommand *> foundCommands;
            for (CommandTable::ConstIterator innerItr = m_commands.Begin(); !innerItr.AtEnd(); innerItr.Forward())
            {
                if (innerItr->Value.Name.StartsWith(commandString, false))
                    foundCommands.Add(&innerItr->Value);
            }

            // sort them in alphabetical order
            foundCommands.SortCB([](const RegisteredCommand *pLeft, const RegisteredCommand *pRight) {
                return pLeft->Name.NumericCompareInsensitive(pRight->Name);
            });

            // find where the similarities end
            SmallString matchCommandPart;
            for (uint32 i = 0; i < foundCommands[0]->Name.GetLength(); i++)
            {
                char ch = foundCommands[0]->Name[i];
                for (uint32 j = 1; j < foundCommands.GetSize(); j++)
                {
                    if (i >= foundCommands[j]->Name.GetLength() || foundCommands[j]->Name[i] != ch)
                    {
                        ch = 0;
                        break;
                    }
                }

                if (ch != 0)
                    matchCommandPart.AppendCharacter(ch);
                else
                    break;
            }

            // change to this command
            commandString.Clear();
            commandString.AppendString(matchCommandPart);

            // display found entries
            Log_InfoPrintf("> %s [%u matches listed]:", commandString.GetCharArray(), foundCommands.GetSize());
            for (uint32 i = 0; i < foundCommands.GetSize(); i++)
                Log_InfoPrintf("  %s", foundCommands[i]->Name.GetCharArray());

            // done for now
            return true;
        }
    }

    // replace with closest match
    if (pClosestMatch != nullptr)
    {
        commandString.Format("%s", pClosestMatch->Value.Name.GetCharArray());
        return true;
    }

    // nope
    Log_InfoPrintf(">%s", commandString.GetCharArray());
    Log_ErrorPrintf("  No matches");
    return false;
}

void Console::DumpCVarHelp(const CVar *pCVar)
{
    SmallString flagsString;
    if (pCVar->GetFlags() & CVAR_FLAG_REGISTERED)
        flagsString.AppendFormattedString("%s%s", (flagsString.GetLength() > 0) ? ", " : "", "Registered");
    if (pCVar->GetFlags() & CVAR_FLAG_WAS_UNREGISTERED)
        flagsString.AppendFormattedString("%s%s", (flagsString.GetLength() > 0) ? ", " : "", "Unregistered");
    if (pCVar->GetFlags() & CVAR_FLAG_READ_ONLY)
        flagsString.AppendFormattedString("%s%s", (flagsString.GetLength() > 0) ? ", " : "", "ReadOnly");
    if (pCVar->GetFlags() & CVAR_FLAG_MODIFIED)
        flagsString.AppendFormattedString("%s%s", (flagsString.GetLength() > 0) ? ", " : "", "Modified");
    if (pCVar->GetFlags() & CVAR_FLAG_NO_ARCHIVE)
        flagsString.AppendFormattedString("%s%s", (flagsString.GetLength() > 0) ? ", " : "", "NoArchive");
    if (pCVar->GetFlags() & CVAR_FLAG_REQUIRE_APP_RESTART)
        flagsString.AppendFormattedString("%s%s", (flagsString.GetLength() > 0) ? ", " : "", "RequireAppRestart");
    if (pCVar->GetFlags() & CVAR_FLAG_REQUIRE_RENDER_RESTART)
        flagsString.AppendFormattedString("%s%s", (flagsString.GetLength() > 0) ? ", " : "", "RequireRenderRestart");
    if (pCVar->GetFlags() & CVAR_FLAG_REQUIRE_MAP_RESTART)
        flagsString.AppendFormattedString("%s%s", (flagsString.GetLength() > 0) ? ", " : "", "RequireMapRestart");
    if (pCVar->GetFlags() & CVAR_FLAG_PAUSE_RENDER_THREAD)
        flagsString.AppendFormattedString("%s%s", (flagsString.GetLength() > 0) ? ", " : "", "PauseRenderThread");
    if (pCVar->GetFlags() & CVAR_FLAG_CHEAT)
        flagsString.AppendFormattedString("%s%s", (flagsString.GetLength() > 0) ? ", " : "", "Cheat");

    Log_InfoPrintf("CVar information for %s:", pCVar->GetName().GetCharArray());
    Log_InfoPrintf("  Description: %s", pCVar->GetHelp().GetCharArray());
    Log_InfoPrintf("  Flags: %s", flagsString.GetCharArray());
    Log_InfoPrintf("  Domain: %s", pCVar->GetDomain().GetCharArray());
    Log_InfoPrintf("  Default Value: %s", pCVar->GetDefaultValue().GetCharArray());
    Log_InfoPrintf("  Current Value: %s", pCVar->GetString().GetCharArray());
}

void Console::DumpCVarValue(const CVar *pCVar)
{
    SmallString message;
    
    // 'cvar_name' is 'value'
    message.Format("'%s' is '%s'", pCVar->GetName().GetCharArray(), pCVar->GetString().GetCharArray());

    // , domain is 'bool'
    message.AppendFormattedString(", domain is '%s'", pCVar->GetDomain().GetCharArray());

    // log it
    Log_InfoPrint(message);
}

bool Console::CVarViewEditCommandExecuteHandler(void *userData, uint32 argumentCount, const char *const argumentValues[])
{
    CVar *pCVar = reinterpret_cast<CVar *>(userData);

    // if argument count is one, display information about it
    if (argumentCount == 1)
    {
        g_pConsole->DumpCVarHelp(pCVar);
        return true;
    }
    else if (argumentCount == 2)
    {
        // change the value
        if (g_pConsole->SetCVarByName(argumentValues[0], argumentValues[1], false, false))
        {
            Log_InfoPrintf("  CVar '%s' set to '%s'.", pCVar->GetName().GetCharArray(), argumentValues[1]);
            return true;
        }
        else
        {
            Log_ErrorPrintf("  Failed to modify cvar '%s'", pCVar->GetName().GetCharArray());
            return false;
        }
    }
    else
    {
        Log_ErrorPrint("  Invalid arguments, a single argument, the value, is permitted");
        return false;
    }
}

bool Console::CVarViewEditCommandHelpHandler(void *userData, uint32 argumentCount, const char *const argumentValues[])
{
    CVar *pCVar = reinterpret_cast<CVar *>(userData);
    DebugAssert(argumentCount > 0);

    if (argumentCount == 1)
        Log_InfoPrintf("  <%s> or <CR> (current value is %s)", pCVar->GetDomain().GetCharArray(), pCVar->GetString().GetCharArray());
    else if (argumentCount == 2)
        Log_InfoPrint("  <CR>");
    else
        Log_ErrorPrint("  Invalid arguments, a single argument, the value, is permitted");

    return true;
}

bool Console::RegisterCommand(const char *commandName, CommandHandlerMethod executeMethod, CommandHandlerMethod helpMethod, void *userData, const void *ownerPtr /* = nullptr */)
{
    CommandTable::Member *pMember = m_commands.Find(commandName);
    if (pMember != nullptr)
        return false;

    RegisteredCommand command;
    command.Name = commandName;
    command.ExecuteMethod = executeMethod;
    command.HelpMethod = helpMethod;
    command.pUserData = userData;
    command.pOwnerPtr = ownerPtr;
    m_commands.Insert(command.Name, command);
    return true;
}

bool Console::UnregisterCommand(const char *commandName)
{
    CommandTable::Member *pMember = m_commands.Find(commandName);
    if (pMember == nullptr)
        return false;

    m_commands.Remove(pMember);
    return true;
}

uint32 Console::UnregisterCommandsWithOwner(const void *ownerPtr)
{
    uint32 commandsRemoved = 0;

    for (CommandTable::Iterator itr = m_commands.Begin(); !itr.AtEnd();)
    {
        CommandTable::Member *pMember = &(*itr);
        itr.Forward();

        if (pMember->Value.pOwnerPtr == ownerPtr)
        {
            m_commands.Remove(pMember);
            commandsRemoved++;
        }
    }

    return commandsRemoved;
}

CVar::CVar(const char *Name, uint32 Flags, const char *DefaultValue /* = NULL */, const char *Help /* = NULL */, const char *Domain /* = NULL */)
{
    // set values
    DebugAssert(Name != NULL && *Name != '\0');
    m_strName = Name;
    if (DefaultValue != NULL)
        m_strDefaultValue = DefaultValue;
    if (Help != NULL)
        m_strHelp = Help;
    if (Domain != NULL)
        m_strDomain = Domain;

    // clear flags that shouldn't be getting passed
    m_iFlags = Flags & ~(CVAR_FLAG_MODIFIED | CVAR_FLAG_REGISTERED | CVAR_FLAG_WAS_UNREGISTERED);

    // have to use the singleton accessor here
    Console::GetInstance().RegisterCVar(this);
}

CVar::~CVar()
{
    Console::GetInstance().RemoveCVar(this);

    for (uint32 i = 0; i < m_callbacks.GetSize(); i++)
        delete m_callbacks[i];
}

void CVar::AddChangeCallback(Functor *pCallback)
{
    Console::GetInstance().AcquireMutex();
    
    m_callbacks.Add(pCallback);

    Console::GetInstance().ReleaseMutex();
}

void CVar::RemoveChangeCallback(Functor *pCallback)
{
    Console::GetInstance().AcquireMutex();

    for (uint32 i = 0; i < m_callbacks.GetSize(); i++)
    {
        if (m_callbacks[i] == pCallback)
        {
            m_callbacks.OrderedRemove(i);
            break;
        }
    }

    Console::GetInstance().ReleaseMutex();
}

const String &CVar::GetPendingString() const
{
    return (m_bChangePending) ? m_strPendingValue : m_strStrValue;
}

int32 CVar::GetPendingInt() const
{
    return (m_bChangePending) ? StringConverter::StringToInt32(m_strPendingValue) : m_iIntValue;
}

uint32 CVar::GetPendingUInt() const
{
    return (m_bChangePending) ? (uint32)StringConverter::StringToInt32(m_strPendingValue) : (uint32)m_iIntValue;
}

float CVar::GetPendingFloat() const
{
    return (m_bChangePending) ? StringConverter::StringToFloat(m_strPendingValue) : m_fFloatValue;
}

bool CVar::GetPendingBool() const
{
    return (m_bChangePending) ? StringConverter::StringToBool(m_strPendingValue) : m_bBoolValue;
}
