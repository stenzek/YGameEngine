#include "Engine/PrecompiledHeader.h"
#include "Engine/ScriptManager.h"
#include "Engine/ScriptObjectTypeInfo.h"
#include "Engine/Entity.h"
Log_SetChannel(ScriptManager);

// Stack dump function
int ScriptManager::DumpScriptStack(lua_State *L)
{
    Log_InfoPrint("Script stack dump: (top-down)");

    int idx = lua_gettop(L);
    for (; idx > 0; idx--)
    {
        int t = lua_type(L, idx);
        if (t == LUA_TSTRING)
            Log_InfoPrintf("  string(%s)", lua_tostring(L, idx));
        else if (t == LUA_TNUMBER)
            Log_InfoPrintf("  number(%f)", lua_tonumber(L, idx));
        else if (t == LUA_TFUNCTION && lua_iscfunction(L, idx))
            Log_InfoPrintf("  cfunction(%p)", lua_tocfunction(L, idx));
        else if (t == LUA_TFUNCTION)
            Log_InfoPrintf("  function");
        else
        {
            if (!luaL_callmeta(L, idx, "__type"))
                lua_pushstring(L, luaL_typename(L, idx));

            Log_InfoPrintf("  %s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }

    return 0;
}

// Traceback function
int ScriptManager::DumpScriptTraceback(lua_State *L)
{
    luaBackupStack(L);

    Log_ErrorPrint("Traceback: (most recent call first)");

    int level = 0;          // we don't want to include this error function
    lua_Debug dbg;
    while (lua_getstack(L, level++, &dbg) == 1)
    {
        // get function info
        lua_getinfo(L, "Snl", &dbg);

        // build stack trace line
        char line[512];
        line[0] = 0;
        if (*dbg.source == '=')
        {
            Y_snprintf(&line[Y_strlen(line)], countof(line) - Y_strlen(line), "<native code>:");
        }
        else
        {
            Y_snprintf(&line[Y_strlen(line)], countof(line) - Y_strlen(line), "%s:", dbg.short_src);
            if (dbg.currentline > 0)
                Y_snprintf(&line[Y_strlen(line)], countof(line) - Y_strlen(line), "%d:", dbg.currentline);
        }

        if (level == 1)
            Y_strncat(line, sizeof(line), " <error handler called>");
        else if (*dbg.namewhat != '\0')
            Y_snprintf(&line[Y_strlen(line)], countof(line) - Y_strlen(line), " in '%s' function '%s'", dbg.namewhat, dbg.name);
        else
        {
            if (*dbg.what == 'm')
                Y_snprintf(&line[Y_strlen(line)], countof(line) - Y_strlen(line), " in main chunk");
            else
                Y_snprintf(&line[Y_strlen(line)], countof(line) - Y_strlen(line), " unknown");
        }

        // output line
        Log_ErrorPrint(line);
    }

    luaVerifyStack(L, 0);
    return 0;
}

// Error handler function
static int ErrorHandler(lua_State *L)
{
    const char *msg = (lua_isstring(L, 1)) ? lua_tostring(L, 1) : nullptr;
    Log_ErrorPrintf("Script error: %s", (msg != nullptr) ? msg : "(null)");

    ScriptManager::DumpScriptTraceback(L);
    return 0;
}

// Panic handler function
static int PanicHandler(lua_State *L)
{
    const char *str = (lua_isstring(L, -1)) ? lua_tostring(L, -1) : nullptr;

    char msg[200];
    Y_snprintf(msg, countof(msg), "Script engine panic: %s", str);

    Y_OnPanicReached(msg, __FUNCTION__, __FILE__, __LINE__);
    return 0;
}

// Redirector method for our userdata type to handle getting members.
static int UserDataAttribute_Index(lua_State *L)
{
    luaBackupStack(L);

    // check if the userdata has a uservalue (data table)
    lua_getuservalue(L, 1);
    if (lua_istable(L, -1))
    {
        // search the uservalue table for the key, raw method will work fine here since there shouldn't be any metamethods on this table
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        if (!lua_isnil(L, -1))
        {
            // value found, so drop out and clean up the stack (replace the uservalue table with the value itself
            lua_replace(L, -2);
            luaVerifyStack(L, 1);
            return 1;
        }

        // drop the nil, usertable off the stack
        lua_pop(L, 2);
    }
    else
    {
        // drop the usertable off the stack
        lua_pop(L, 1);
    }

    // search the table as normal, this is the methods table, so there won't be any metamethods here either
    lua_pushvalue(L, 2);
    lua_rawget(L, lua_upvalueindex(1));
    luaVerifyStack(L, 1);
    return 1;
}

// Redirector method for our userdata type to handle setting members.
static int UserDataAttribute_NewIndex(lua_State *L)
{
    luaBackupStack(L);

    // does this userdata currently have a uservalue? 
    lua_getuservalue(L, 1);
    if (lua_isnil(L, -1))
    {
        // nil, so create the table, and bind it to the userdata
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setuservalue(L, 1);
    }

    // assign it to the table, use raw set here though
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, -3);
    lua_pop(L, 1);

    luaVerifyStack(L, 0);
    return 0;
}

// Static error method when trying to allocate a non-script-allocatable user type, expects the type name in upvalue#1
static int UserData_AllocateUnallocatableTypeError(lua_State *L)
{
    const char *typeName = lua_tostring(L, lua_upvalueindex(1));
    luaL_error(L, "attempt to allocate unallocatable type %s", typeName);
    return 0;
}

static ScriptManager s_scriptManager;
ScriptManager *g_pScriptManager = &s_scriptManager;

ScriptManager::ScriptManager()
    : m_state(nullptr)
{

}

ScriptManager::~ScriptManager()
{
    DebugAssert(m_state == nullptr);
}

static int UserData_GetMetaTableName(lua_State *L, int idx)
{
    luaBackupStack(L);

    lua_getfield(L, idx, "__name");
    if (lua_isstring(L, -1))
    {
        // remove the metatable
        luaVerifyStack(L, 1);
        return 1;
    }

    lua_pop(L, 1);
    lua_pushstring(L, "<<unknown>>");
    luaVerifyStack(L, 1);
    return 1;
}

static int UserData_GetTypeName(lua_State *L, int idx)
{
    luaBackupStack(L);

    if (lua_isuserdata(L, idx))
    {
        if (!luaL_callmeta(L, idx, "__type"))
        {
            // get __name from metatable
            lua_getmetatable(L, idx);
            if (lua_istable(L, -1))
            {
                lua_getfield(L, -1, "__name");
                if (lua_isstring(L, -1))
                {
                    // remove the metatable
                    lua_remove(L, -2);
                    luaVerifyStack(L, 1);
                    return 1;
                }
                lua_pop(L, 1);
            }
            lua_pop(L, 1);
        }
    }

    lua_pushstring(L, luaL_typename(L, idx));
    luaVerifyStack(L, 1);
    return 1;
}

int ScriptManager::GenerateTypeError(lua_State *L, int narg, const char *type)
{
    UserData_GetTypeName(L, narg);

    const char *msg = lua_pushfstring(L, "%s expected, got %s", type, lua_tostring(L, -1));
    return luaL_argerror(L, narg, msg);
}

int ScriptManager::GenerateTypeError(lua_State *L, int narg, ScriptReferenceType metaTableReference)
{
    UserData_GetTypeName(L, narg);

    // resolve metatable
    lua_rawgeti(L, LUA_REGISTRYINDEX, metaTableReference);
    UserData_GetMetaTableName(L, -1);

    const char *msg = lua_pushfstring(L, "%s expected, got %s", lua_tostring(L, -1), lua_tostring(L, -3));
    return luaL_argerror(L, narg, msg);
}

ScriptReferenceType ScriptManager::CreateReference(lua_State *L)
{
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

void ScriptManager::ReleaseReference(ScriptReferenceType objectReference)
{
    luaL_unref(m_state, LUA_REGISTRYINDEX, objectReference);
}

size_t ScriptManager::GetMemoryUsage() const
{
    return (size_t)lua_gc(m_state, LUA_GCCOUNT, 0) << 10 | (size_t)lua_gc(m_state, LUA_GCCOUNTB, 0);
}

void ScriptManager::RunGCStep()
{
    lua_gc(m_state, LUA_GCSTEP, 0);
}

void ScriptManager::RunGCFull()
{
    lua_gc(m_state, LUA_GCCOLLECT, 0);
}

bool ScriptManager::Startup()
{
    Log_InfoPrint("ScriptManager::Startup()");
    Timer tmr;
    Timer ttmr;

    // allocate state
    m_state = luaL_newstate();
    Log_PerfPrintf("newstate took %.4f msec", tmr.GetTimeMilliseconds()); tmr.Reset();
    lua_atpanic(m_state, PanicHandler);

    // load libs
    luaopen_base(m_state);
    luaopen_table(m_state);
    luaopen_string(m_state);
    luaopen_utf8(m_state);
    luaopen_math(m_state);
    luaopen_debug(m_state);
    lua_pop(m_state, 6);
    Log_PerfPrintf("lib funcs took %.4f msec", tmr.GetTimeMilliseconds()); tmr.Reset();

    // create types
    RegisterBuiltinFunctions();
    Log_PerfPrintf("builtin funcs took %.4f msec", tmr.GetTimeMilliseconds()); tmr.Reset();
    RegisterPrimitiveTypes();
    Log_PerfPrintf("primitive funcs took %.4f msec", tmr.GetTimeMilliseconds()); tmr.Reset();
    ScriptObjectTypeInfo::RegisterAllScriptTypes();
    Log_PerfPrintf("object funcs took %.4f msec", tmr.GetTimeMilliseconds()); tmr.Reset();

    Log_PerfPrintf("total took %.4f msec", ttmr.GetTimeMilliseconds()); 
    // done
    return true;
}

void ScriptManager::Shutdown()
{
    Log_InfoPrint("ScriptManager::Shutdown()");

    // unregister object types
    ScriptObjectTypeInfo::UnregisterAllScriptTypes();

    // unregister all primitive types
    UnregisterPrimitiveTypes();

    // finally delete state
    lua_close(m_state);
    m_state = nullptr;
}

ScriptReferenceType ScriptManager::DefineTabledUserDataType(const char *typeName, const SCRIPT_FUNCTION_TABLE_ENTRY *pMethods /* = nullptr */, const SCRIPT_FUNCTION_TABLE_ENTRY *pMetaMethods /* = nullptr */, ScriptNativeFunctionType constructor /* = nullptr */, ScriptNativeFunctionType destructor /* = nullptr */)
{
    luaBackupStack(m_state);

    // allocate metatable
    if (!luaL_newmetatable(m_state, typeName))
    {
        lua_pop(m_state, 1);
        luaVerifyStack(m_state, 0);
        return INVALID_SCRIPT_REFERENCE;
    }

    // fill method table
    lua_newtable(m_state);
    if (pMethods != nullptr)
    {
        for (const SCRIPT_FUNCTION_TABLE_ENTRY *pMethod = pMethods; pMethod->FunctionPointer != nullptr; pMethod++)
        {
            lua_pushcclosure(m_state, pMethod->FunctionPointer, 0);
            lua_setfield(m_state, -2, pMethod->FunctionName);
        }
    }

    // set __index of the metatable to our redirector function, using the method table as an upvalue
    lua_pushcclosure(m_state, UserDataAttribute_Index, 1);
    lua_setfield(m_state, -2, "__index");

    // set __newindex of the metatable to our redirector function
    lua_pushcclosure(m_state, UserDataAttribute_NewIndex, 0);
    lua_setfield(m_state, -2, "__newindex");

    // create constructor if provided (TypeName() method)
    if (constructor != nullptr)
    {
        lua_pushcclosure(m_state, constructor, 0);
        lua_setglobal(m_state, typeName);
    }
    else
    {
        // create a stub method that errors out when trying to allocate the type, that way the name is reserved
        lua_pushstring(m_state, typeName);
        lua_pushcclosure(m_state, UserData_AllocateUnallocatableTypeError, 1);
        lua_setglobal(m_state, typeName);
    }

    // create destructor if provided
    if (destructor != nullptr)
    {
        lua_pushcclosure(m_state, destructor, 0);
        lua_setfield(m_state, -2, "__gc");
    }

    // add any metamethods
    if (pMetaMethods != nullptr)
    {
        for (const SCRIPT_FUNCTION_TABLE_ENTRY *pMethod = pMethods; pMethod->FunctionPointer != nullptr; pMethod++)
        {
            lua_pushcclosure(m_state, pMethod->FunctionPointer, 0);
            lua_setfield(m_state, -2, pMethod->FunctionName);
        }
    }

    // reference the metatable type
    ScriptReferenceType ref = luaL_ref(m_state, LUA_REGISTRYINDEX);

    // verify stack and return
    luaVerifyStack(m_state, 0);
    return ref;
}

ScriptReferenceType ScriptManager::DefineUserDataType(const char *typeName, const SCRIPT_FUNCTION_TABLE_ENTRY *pMethods /* = nullptr */, const SCRIPT_FUNCTION_TABLE_ENTRY *pMetaMethods /* = nullptr */, ScriptNativeFunctionType constructor /* = nullptr */, ScriptNativeFunctionType destructor /* = nullptr */)
{
    luaBackupStack(m_state);

    // allocate metatable
    if (!luaL_newmetatable(m_state, typeName))
    {
        luaVerifyStack(m_state, 0);
        return INVALID_SCRIPT_REFERENCE;
    }

    // if methods are provided, allocate and fill the method table
    if (pMethods != nullptr)
    {
        // todo: create sized table lua_createtable
        lua_newtable(m_state);
        for (const SCRIPT_FUNCTION_TABLE_ENTRY *pMethod = pMethods; pMethod->FunctionPointer != nullptr; pMethod++)
        {
            lua_pushcclosure(m_state, pMethod->FunctionPointer, 0);
            lua_setfield(m_state, -2, pMethod->FunctionName);
        }

        // set __index of the metatable to this table
        lua_setfield(m_state, -2, "__index");
    }

    // create constructor if provided (TypeName() method)
    if (constructor != nullptr)
    {
        lua_pushcclosure(m_state, constructor, 0);
        lua_setglobal(m_state, typeName);
    }
    else
    {
        // create a stub method that errors out when trying to allocate the type, that way the name is reserved
        lua_pushstring(m_state, typeName);
        lua_pushcclosure(m_state, UserData_AllocateUnallocatableTypeError, 1);
        lua_setglobal(m_state, typeName);
    }

    // create destructor if provided
    if (destructor != nullptr)
    {
        lua_pushcclosure(m_state, destructor, 0);
        lua_setfield(m_state, -2, "__gc");
    }

    // add any metamethods
    if (pMetaMethods != nullptr)
    {
        for (const SCRIPT_FUNCTION_TABLE_ENTRY *pMethod = pMethods; pMethod->FunctionPointer != nullptr; pMethod++)
        {
            lua_pushcclosure(m_state, pMethod->FunctionPointer, 0);
            lua_setfield(m_state, -2, pMethod->FunctionName);
        }
    }

    // reference the metatable type
    ScriptReferenceType ref = luaL_ref(m_state, LUA_REGISTRYINDEX);
    
    // verify stack and return
    luaVerifyStack(m_state, 0);
    return ref;
}

void *ScriptManager::CheckUserDataTypeByMetaTable(lua_State *L, ScriptReferenceType metaTableReference, int arg)
{
    void *pUserData;
    luaBackupStack(L);

    // should be a userdata
    if (lua_type(L, arg) != LUA_TUSERDATA || (pUserData = lua_touserdata(L, arg)) == nullptr)
    {
        GenerateTypeError(L, arg, metaTableReference);
        return nullptr;
    }

    // resolve metatable reference
    lua_rawgeti(L, LUA_REGISTRYINDEX, metaTableReference);
    DebugAssert(lua_istable(L, -1));

    // get the parameter's metatable
    lua_getmetatable(L, arg);

    // check they match
    if (!lua_rawequal(L, -1, -2))
    {
        lua_pop(L, 2);
        GenerateTypeError(L, arg, metaTableReference);
        return nullptr;
    }

    // type is ok, pop metatable + table
    lua_pop(L, 2);
    luaVerifyStack(L, 0);
    return reinterpret_cast<ScriptUserDataTag *>(pUserData) + 1;
}

void *ScriptManager::CheckUserDataTypeByTag(lua_State *L, ScriptUserDataTag tag, int arg)
{
    void *pUserData;
    luaBackupStack(L);

    // should be a userdata
    if (lua_type(L, arg) != LUA_TUSERDATA || (pUserData = lua_touserdata(L, arg)) == nullptr)
    {
        luaL_argerror(L, arg, "expected userdata");
        return nullptr;
    }

    // read the tag of the userdata
    ScriptUserDataTag udTag = *reinterpret_cast<ScriptUserDataTag *>(pUserData);
    if (udTag != tag)
    {
        lua_pushfstring(L, "expected userdata tag %u, got %u", tag, udTag);
        luaL_argerror(L, arg, lua_tostring(L, -1));
        return nullptr;
    }

    // type is ok, pop metatable + table
    luaVerifyStack(L, 0);
    return reinterpret_cast<ScriptUserDataTag *>(pUserData) + 1;
}

void ScriptManager::PushNewUserData(lua_State *L, ScriptReferenceType metaTableReference, ScriptUserDataTag tag, uint32 size, const void *data)
{
    luaBackupStack(L);

    // create user data and copy it in
    void *pUserData = lua_newuserdata(L, size + sizeof(ScriptUserDataTag));
    *reinterpret_cast<ScriptUserDataTag *>(pUserData) = tag;
    Y_memcpy(reinterpret_cast<ScriptUserDataTag *>(pUserData) + 1, data, size);

    // resolve metatable reference
    lua_rawgeti(L, LUA_REGISTRYINDEX, metaTableReference);
    DebugAssert(lua_istable(L, -1));

    // set the metatable of the newly generated user data to it
    lua_setmetatable(L, -2);

    luaVerifyStack(L, 1);
}

ScriptReferenceType ScriptManager::AllocateAndReferenceNewUserData(ScriptReferenceType metaTableReference, ScriptUserDataTag tag, uint32 size, const void *data)
{
    luaBackupStack(m_state);

    PushNewUserData(m_state, metaTableReference, tag, size, data);

    // reference the object
    ScriptReferenceType ref = luaL_ref(m_state, LUA_REGISTRYINDEX);

    luaVerifyStack(m_state, 0);
    return ref;
}

void ScriptManager::SetUserDataReferenceTag(ScriptReferenceType reference, ScriptUserDataTag tag)
{
    luaBackupStack(m_state);

    lua_rawgeti(m_state, LUA_REGISTRYINDEX, reference);
    DebugAssert(lua_isuserdata(m_state, -1));

    ScriptUserDataTag *pUserDataTag = reinterpret_cast<ScriptUserDataTag *>(lua_touserdata(m_state, -1));
    *pUserDataTag = tag;

    lua_pop(m_state, 1);
    luaVerifyStack(m_state, 0);
}

ScriptCallResult ScriptManager::RunScript(const byte *script, uint32 scriptLength, const char *source /* = "text chunk" */)
{
    // stack should be empty
    DebugAssert(lua_gettop(m_state) == 0);
    luaBackupStack(m_state);

    // push error handler
    lua_pushcfunction(m_state, ErrorHandler);

    // parse string
    if (luaL_loadbuffer(m_state, (const char *)script, scriptLength, source) != 0)
    {
        lua_pop(m_state, 1);
        luaVerifyStack(m_state, 0);
        return ScriptCallResult_ParseError;
    }

    // run string
    int r = lua_pcall(m_state, 0, 0, -2);
    if (r != 0)
    {
        const char *errorMessage = lua_tostring(m_state, -1);
        if (errorMessage != nullptr)
            Log_ErrorPrintf("Script error: %s", errorMessage);

        // pop error message and handler
        lua_pop(m_state, 2);
        luaVerifyStack(m_state, 0);
        return ScriptCallResult_RuntimeError;
    }

    // pop error handler
    lua_pop(m_state, 1);
    luaVerifyStack(m_state, 0);
    return ScriptCallResult_Success;
}

void ScriptManager::PushNewObjectReference(lua_State *L, ScriptReferenceType metaTableReference, ScriptUserDataTag tag, const void *pObjectPointer)
{
    DebugAssert(metaTableReference != INVALID_SCRIPT_REFERENCE);
    luaBackupStack(L);

    // allocate userdata of pointer-size
    void *pUserData = lua_newuserdata(L, sizeof(void *) + sizeof(ScriptUserDataTag));
    *reinterpret_cast<ScriptUserDataTag *>(pUserData) = tag;
    *reinterpret_cast<void **>(reinterpret_cast<ScriptUserDataTag *>(pUserData) + 1) = const_cast<void *>(pObjectPointer);

    // set metatable
    lua_rawgeti(L, LUA_REGISTRYINDEX, metaTableReference);
    lua_setmetatable(L, -2);
    luaVerifyStack(L, 1);
}

ScriptReferenceType ScriptManager::AllocateAndReferenceObject(ScriptReferenceType metaTableReference, ScriptUserDataTag tag, const void *pObjectPointer)
{
    DebugAssert(metaTableReference != INVALID_SCRIPT_REFERENCE);
    luaBackupStack(m_state);

    // push it
    PushNewObjectReference(m_state, metaTableReference, tag, pObjectPointer);

    // reference it
    ScriptReferenceType ref = luaL_ref(m_state, LUA_REGISTRYINDEX);
    luaVerifyStack(m_state, 0);
    return ref;
}

void ScriptManager::SetObjectReferencePointer(ScriptReferenceType objectReference, const void *pObject)
{
    DebugAssert(objectReference != INVALID_SCRIPT_REFERENCE);

    // push the reference
    lua_rawgeti(m_state, LUA_REGISTRYINDEX, objectReference);

    // replace the userdata
    void **ppUserData = (void **)lua_touserdata(m_state, -1);
    *ppUserData = const_cast<void *>(pObject);
    lua_pop(m_state, 1);
}

void *ScriptManager::CheckObjectTypeByMetaTable(lua_State *L, ScriptReferenceType metaTableReference, int arg)
{
    void **ppUserData = (void **)CheckUserDataTypeByMetaTable(L, metaTableReference, arg);
    return *ppUserData;
}

void *ScriptManager::CheckObjectTypeByTag(lua_State *L, ScriptUserDataTag tag, int arg)
{
    void **ppUserData = (void **)CheckUserDataTypeByTag(L, tag, arg);
    return *ppUserData;
}

ScriptCallResult ScriptManager::RunObjectScript(ScriptReferenceType objectReference, const byte *script, uint32 scriptLength, const char *source /* = "text chunk" */)
{
    // stack should be empty
    DebugAssert(lua_gettop(m_state) == 0);
    luaBackupStack(m_state);

    // dereference the userdata object
    lua_rawgeti(m_state, LUA_REGISTRYINDEX, objectReference);

    // create a new table for the environment of the running script
    lua_newtable(m_state);

    // alias self to the entity's table
    lua_pushvalue(m_state, -2);
    lua_setfield(m_state, -2, "self");

    // create a temporary metatable, redirect __index so that it first checks the local environment before the global
    lua_newtable(m_state);
    lua_pushglobaltable(m_state);
    lua_setfield(m_state, -2, "__index");
    lua_setmetatable(m_state, -2);

    // push error handler
    lua_pushcfunction(m_state, ErrorHandler);

    // load the script
    if (luaL_loadbuffer(m_state, (const char *)script, scriptLength, source) != 0)
    {
        // todo: reorder this
        const char *errorMessage = lua_tostring(m_state, -1);
        if (errorMessage != nullptr)
            Log_ErrorPrintf("Script parse error: %s", errorMessage);

        // failed, pop both error handler, and the table
        lua_pop(m_state, 4);
        luaVerifyStack(m_state, 0);
        return ScriptCallResult_ParseError;
    }

    // set environment for execution to the environment for this script
    lua_pushvalue(m_state, -3);
    lua_setupvalue(m_state, -2, 1);

    // run the script
    int r = lua_pcall(m_state, 0, 0, -2);
    if (r != 0)
    {
        const char *errorMessage = lua_tostring(m_state, -1);
        if (errorMessage != nullptr)
            Log_ErrorPrintf("Script error: %s", errorMessage);

        // execution failed
        lua_pop(m_state, 4);
        luaVerifyStack(m_state, 0);
        return ScriptCallResult_RuntimeError;
    }

    // pop the error handler off, this should leave us with the environment table, object reference
    lua_pop(m_state, 1);

    // drop self from the environment table
    lua_pushnil(m_state);
    lua_setfield(m_state, -2, "self");

    // copy everything out of the environment table into the entity's table
    lua_pushnil(m_state);
    while (lua_next(m_state, -2) != 0)
    {
        const char *key = lua_tostring(m_state, -2);
        Log_TracePrintf("ScriptManager::RunEntityScript: found %s - %s", key, lua_typename(m_state, lua_type(m_state, -1)));

        // skip non-functions
        if (!lua_isfunction(m_state, -1))
        {
            lua_pop(m_state, 1);
            continue;
        }

        // set the key to the value, this will drop the value from the stack too
        // todo: optimize this to a lua_settable to avoid string allocation
        lua_setfield(m_state, -4, key);
    }

    // drop the environment table, and the dereferenced object
    lua_pop(m_state, 2);
    luaVerifyStack(m_state, 0);
    return ScriptCallResult_Success;
}

ScriptCallResult ScriptManager::BeginObjectMethodCall(ScriptReferenceType objectReference, const char *methodName)
{
    // stack should be empty
    DebugAssert(lua_gettop(m_state) == 0);

    // todo: optimize me
    luaBackupStack(m_state);

    // dereference object
    lua_rawgeti(m_state, LUA_REGISTRYINDEX, objectReference);

    // do we have a method by this name?
    lua_getfield(m_state, -1, methodName);
    if (!lua_isfunction(m_state, -1))
    {
        // nope, bail out
        lua_pop(m_state, 2);
        luaVerifyStack(m_state, 0);
        return ScriptCallResult_MethodNotFound;
    }

    // swap the object{self} and function around so that the function comes first
    lua_insert(m_state, -2);

    // push the error handler, rotate so that stack = errorhandler, function, self
    lua_pushcfunction(m_state, ErrorHandler);
    lua_insert(m_state, -3);

    luaVerifyStack(m_state, 3);
    return ScriptCallResult_Success;
}

ScriptCallResult ScriptManager::InvokeObjectMethodCall(bool saveResults /* = false */)
{
    // error handler, function, self, args...
    int argCount = lua_gettop(m_state) - 2;
    DebugAssert(argCount > 0);

    // invoke pcall, pretty straightforward
    int r = lua_pcall(m_state, argCount, (saveResults) ? LUA_MULTRET : 0, 1);
    if (r != 0)
    {
        const char *errorMessage = lua_tostring(m_state, -1);
        if (errorMessage != nullptr)
            Log_ErrorPrintf("Script error: %s", errorMessage);

        // pop error message and handler
        lua_settop(m_state, 0);
        return ScriptCallResult_RuntimeError;
    }

    // saving results?
    if (!saveResults)
    {
        // empty the stack
        lua_settop(m_state, 0);
    }
    else
    {
        // remove error handler
        DebugAssert(lua_gettop(m_state) > 0);
        lua_remove(m_state, 0);
    }

    return ScriptCallResult_Success;
}

uint32 ScriptManager::GetCallResultCount()
{
    int nResults = lua_gettop(m_state);
    DebugAssert(nResults >= 0);
    return (uint32)nResults;
}

void ScriptManager::EndCall()
{
    // drop results
    lua_settop(m_state, 0);
}

ScriptThread::ScriptThread(lua_State *threadState, ScriptReferenceType threadReference)
    : m_pThreadState(threadState),
      m_threadReference(threadReference),
      m_waitChannel(""),
      m_timeout(DEFAULT_SCRIPT_TIMEOUT),
      m_timeoutAction(ScriptThreadTimeoutAction_Abort),
      m_yieldCount(0)
{

}

ScriptThread::~ScriptThread()
{
    g_pScriptManager->ReleaseReference(m_threadReference);
}

ScriptCallResult ScriptManager::BeginThreadedObjectMethodCall(ScriptThread **ppThread, ScriptReferenceType objectReference, const char *methodName)
{
    // stack should be empty
    DebugAssert(lua_gettop(m_state) == 0);
    luaBackupStack(m_state);

    // dereference the object
    lua_rawgeti(m_state, LUA_REGISTRYINDEX, objectReference);

    // do we have a method by this name?
    //lua_getfield(m_state, -1, methodName);
    lua_pushstring(m_state, methodName);
    lua_gettable(m_state, -2);
    if (!lua_isfunction(m_state, -1))
    {
        // nope, bail out
        lua_pop(m_state, 2);
        luaVerifyStack(m_state, 0);
        return ScriptCallResult_MethodNotFound;
    }

    // swap the object{self} and function around so that the function comes first
    lua_insert(m_state, -2);

    // allocate thread wrapper, and dereference the thread state. this is necessary to pin it and prevent gc
    lua_State *pThreadState = lua_newthread(m_state);
    ScriptReferenceType threadReference = CreateReference(m_state);
    ScriptThread *pThread = new ScriptThread(pThreadState, threadReference);
    *ppThread = pThread;

    // move the 2 stack elements from the global state to the thread state
    lua_xmove(m_state, pThread->m_pThreadState, 2);
    DebugAssert(lua_gettop(pThread->m_pThreadState) == 2);

    luaVerifyStack(m_state, 0);
    return ScriptCallResult_Success;
}

ScriptCallResult ScriptManager::ResumeThreadedObjectMethodCall(ScriptThread *pThread, bool saveResults /* = false */)
{
    int argCount = lua_gettop(pThread->m_pThreadState);
    
    // if this is the initial call
    if (pThread->m_yieldCount == 0)
    {
        // function and self are provided in the initial call
        DebugAssert(argCount >= 2);
        argCount -= 2;
    }
    
    // use lua_resume instead of lua_pcall
    int r = lua_resume(pThread->m_pThreadState, nullptr, argCount);
    if (r == LUA_YIELD)
    {
        // we should have the three wait setup parameters
        DebugAssert(lua_gettop(pThread->m_pThreadState) == 3);

        // extract wait channel
        const char *waitChannel = lua_tostring(pThread->m_pThreadState, 1);
        if (waitChannel != nullptr && *waitChannel != '\0')
            pThread->m_waitChannel = waitChannel;
        else
            pThread->m_waitChannel = EmptyString;

        // extract timeout and action
        pThread->m_timeout = (float)lua_tonumber(pThread->m_pThreadState, 2);
        pThread->m_timeoutAction = (ScriptThreadTimeoutAction)lua_tointeger(pThread->m_pThreadState, 3);

        // pop yield args
        lua_pop(pThread->m_pThreadState, 3);

        // if this is the first yield, place the thread into the paused threads list
        if (pThread->m_yieldCount == 0)
            m_pausedThreads.Add(pThread);

        // increment yield count
        pThread->m_yieldCount++;
        return ScriptCallResult_Yielded;
    }

    // error case
    else if (r != 0)
    {
        const char *errorMessage = lua_tostring(pThread->m_pThreadState, -1);
        if (errorMessage != nullptr)
            Log_ErrorPrintf("Threaded script execution exception: %s", errorMessage);

        // the stack is remaining in its current state, it is not unwound. so we can execute a traceback from here.
        ScriptManager::DumpScriptTraceback(pThread->m_pThreadState);

        // remove from paused thread list
        if (pThread->m_yieldCount != 0)
            m_pausedThreads.OrderedRemove((uint32)m_pausedThreads.IndexOf(pThread));

        // lose everything on the stack, and cleanup the thread
        lua_pop(pThread->m_pThreadState, lua_gettop(pThread->m_pThreadState));
        delete pThread;

        // error
        return ScriptCallResult_RuntimeError;
    }

    // remove from paused thread list
    if (pThread->m_yieldCount != 0)
        m_pausedThreads.OrderedRemove((uint32)m_pausedThreads.IndexOf(pThread));

    // if we're not interested in the results, cleanup the thread now
    if (!saveResults)
    {
        //DumpScriptStack(pThread->m_pThreadState);
        lua_settop(pThread->m_pThreadState, 0);
        delete pThread;
    }
    else
    {
        // seems to keep the self pointer on the stack in position 1.. dunno why?
        lua_remove(m_state, 1);
    }

    // execution ok
    return ScriptCallResult_Success;
}

int ScriptManager::YieldThreadedCall(lua_State *L, const char *waitChannel /* = "" */, float timeout /* = DEFAULT_SCRIPT_TIMEOUT */, ScriptThreadTimeoutAction timeoutAction /* = ScriptThreadTimeoutAction_Abort */)
{
    lua_pushstring(L, waitChannel);
    lua_pushnumber(L, (lua_Number)timeout);
    lua_pushinteger(L, (lua_Integer)timeoutAction);
    return lua_yieldk(L, 3, 0, nullptr);
}

uint32 ScriptManager::GetThreadedCallResultCount(ScriptThread *pThread)
{
    int nResults = lua_gettop(pThread->m_pThreadState);
    DebugAssert(nResults > 0);
    return (uint32)nResults;
}

void ScriptManager::AbortThreadedCall(ScriptThread *pThread)
{
    // remove from list
    int32 index = m_pausedThreads.IndexOf(pThread);
    DebugAssert(index >= 0);
    m_pausedThreads.OrderedRemove((uint32)index);

    // clean up the thread
    delete pThread;
}

void ScriptManager::EndThreadedCall(ScriptThread *pThread)
{
    // drop results
    lua_settop(pThread->m_pThreadState, 0);
}

void ScriptManager::CheckPausedThreadTimeout(float deltaTime)
{
    for (uint32 i = 0; i < m_pausedThreads.GetSize(); )
    {
        ScriptThread *pThread = m_pausedThreads[i];
        if (deltaTime < pThread->m_timeout)
        {
            // timeout not elapsed yet
            pThread->m_timeout -= deltaTime;
            i++;
            continue;
        }

        // what's the timeout action?
        switch (pThread->m_timeoutAction)
        {
        case ScriptThreadTimeoutAction_ReturnNil:
            {
                // first return nil, then fall through and resume
                lua_pushnil(pThread->m_pThreadState);
            }

        case ScriptThreadTimeoutAction_Resume:
            {
                // resume thread execution
                ScriptCallResult executeResult = ResumeThreadedObjectMethodCall(pThread, false);
                if (executeResult == ScriptCallResult_Yielded)
                {
                    // thread has yielded, it will still be in the list
                    i++;
                }
                
                // thread has ended or thrown an error
                continue;
            }
        }

        // kill the thread, and remove it from the list
        Log_WarningPrintf("ScriptManager::CheckPausedThreadTimeout: Aborting thread %p due to timeout", pThread);
        m_pausedThreads.OrderedRemove(i);
        delete pThread;
    }
}
