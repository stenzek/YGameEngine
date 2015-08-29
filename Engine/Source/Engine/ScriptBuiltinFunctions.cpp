#include "Engine/PrecompiledHeader.h"
#include "Engine/ScriptManager.h"
Log_SetChannel(ScriptManager);

static void lua_Log_Write(lua_State *L, LOGLEVEL level)
{
    const char *msg = lua_tostring(L, 1);
    const char *funcname = "";      // @TODO
    Log::GetInstance().Write("Script", "", level, (msg != nullptr) ? msg : "(null)");
}

static int lua_Log_Error(lua_State *L)
{
    lua_Log_Write(L, LOGLEVEL_ERROR);
    return 0;
}

static int lua_Log_Warning(lua_State *L)
{
    lua_Log_Write(L, LOGLEVEL_WARNING);
    return 0;
}

static int lua_Log_Info(lua_State *L)
{
    lua_Log_Write(L, LOGLEVEL_INFO);
    return 0;
}

static int lua_Log_Perf(lua_State *L)
{
    lua_Log_Write(L, LOGLEVEL_PERF);
    return 0;
}

static int lua_Log_Dev(lua_State *L)
{
    lua_Log_Write(L, LOGLEVEL_DEV);
    return 0;
}

static int lua_Log_Profile(lua_State *L)
{
    lua_Log_Write(L, LOGLEVEL_PROFILE);
    return 0;
}

static int lua_Log_Trace(lua_State *L)
{
    lua_Log_Write(L, LOGLEVEL_TRACE);
    return 0;
}

static const luaL_Reg lua_Log[] =
{
    { "Error", lua_Log_Error },
    { "Warning", lua_Log_Warning },
    { "Info", lua_Log_Info },
    { "Perf", lua_Log_Perf },
    { "Dev", lua_Log_Dev },
    { "Profile", lua_Log_Profile },
    { "Trace", lua_Log_Trace },
    { nullptr, nullptr }
};

static void RegisterFunctionLibrary(lua_State *L, const char *name, const luaL_Reg *reg)
{
    luaBackupStack(L);

    luaL_newlib(L, reg);
    lua_setglobal(L, name);

    luaVerifyStack(L, 0);
}

void ScriptManager::RegisterBuiltinFunctions()
{
    luaBackupStack(m_state);

    // Log
    RegisterFunctionLibrary(m_state, "Log", lua_Log);

    luaVerifyStack(m_state, 0);
}
