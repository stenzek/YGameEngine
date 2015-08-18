#pragma once
#include "Engine/Common.h"
#include "Engine/ScriptTypes.h"
#include <lua.hpp>

// Assert setup
#ifdef Y_BUILD_CONFIG_DEBUG
    #define luaCheckOnMainThread() Assert(1)
    #define luaBackupStack(__state) int __stack_top_backup__ = lua_gettop(__state)
    //#define luaVerifyStack(__state, __offset) Assert(lua_gettop(__state) == (__stack_top_backup__) + (__offset))
    #define luaVerifyStack(__state, __offset) if (lua_gettop(__state) != (__stack_top_backup__) + (__offset)) { ScriptManager::DumpScriptStack(__state); Panic("lua stack validation failed"); }
    #define luaVerifyUserType(__state, __offset, __metaTableReference)  lua_getmetatable((__state), (__offset)); \
                                                                        lua_rawgeti((__state), LUA_REGISTRYINDEX, (__metaTableReference)); \
                                                                        Assert(lua_rawequal((__state), -1, -2)); \
                                                                        lua_pop((__state), 2);
#else
    #define luaCheckOnMainThread()
    #define luaBackupStack(__state) 
    #define luaVerifyStack( __state, __offset) 
    #define luaVerifyUserType(__state, __offset, __metaTableReference)
#endif

// Script args wrapper
template<typename T>
struct ScriptArg
{
    static T Get(lua_State *L, int arg);
    static void Push(lua_State *L, const T &arg);
};

// Script manager
class ScriptManager
{
public:
    ScriptManager();
    ~ScriptManager();

    //==========================================================================================================================================================================================================
    // Initialization
    //==========================================================================================================================================================================================================
    bool Startup();
    void Shutdown();
    
    //==========================================================================================================================================================================================================
    // Helper Methods
    //==========================================================================================================================================================================================================

    // access the lua state
    lua_State *GetGlobalState() const { return m_state; }

    // generate a type error
    static int GenerateTypeError(lua_State *L, int narg, const char *type);
    static int GenerateTypeError(lua_State *L, int narg, ScriptReferenceType metaTableReference);

    // dump the current stack, top-to-bottom, to the log
    static int DumpScriptStack(lua_State *L);

    // dump a script traceback to the error log
    static int DumpScriptTraceback(lua_State *L);

    // Create an object reference, taking the object from the top of the stack.
    ScriptReferenceType CreateReference(lua_State *L);

    // Release an object reference, deleting it if no scripts are using it.
    void ReleaseReference(ScriptReferenceType objectReference);

    // Find memory usage of script state.
    size_t GetMemoryUsage() const;

    // Run a garbage collection step.
    void RunGCStep();

    // Run a full garbage collection.
    void RunGCFull();

    //==========================================================================================================================================================================================================
    // UserData Type Management
    //==========================================================================================================================================================================================================

    // Define a new user data type. If the type name is already in use, INVALID_SCRIPT_REFERENCE will be returned. An optional null-terminated list of preassigned functions can be provided.
    // A reference to the user data type's metatable will be returned, which is usable for the CheckUserDataType / PushNewUserDataType functions. Using the DefineTabledUserDataType
    // method will install wrappers so that the object can have arbritrary values set on it, with the table being allocated on-demand.
    ScriptReferenceType DefineUserDataType(const char *typeName, const SCRIPT_FUNCTION_TABLE_ENTRY *pMethods = nullptr, const SCRIPT_FUNCTION_TABLE_ENTRY *pMetaMethods = nullptr, ScriptNativeFunctionType constructor = nullptr, ScriptNativeFunctionType destructor = nullptr);
    ScriptReferenceType DefineTabledUserDataType(const char *typeName, const SCRIPT_FUNCTION_TABLE_ENTRY *pMethods = nullptr, const SCRIPT_FUNCTION_TABLE_ENTRY *pMetaMethods = nullptr, ScriptNativeFunctionType constructor = nullptr, ScriptNativeFunctionType destructor = nullptr);

    // Check whether the user data type matches the specified metatable reference, and returns the userdata pointer if so.
    static void *CheckUserDataTypeByMetaTable(lua_State *L, ScriptReferenceType metaTableReference, int arg);
    static void *CheckUserDataTypeByTag(lua_State *L, ScriptUserDataTag tag, int arg);

    // Pushes a new instance of the specified user data metatable.
    static void PushNewUserData(lua_State *L, ScriptReferenceType metaTableReference, ScriptUserDataTag tag, uint32 size, const void *data);
    ScriptReferenceType AllocateAndReferenceNewUserData(ScriptReferenceType metaTableReference, ScriptUserDataTag tag, uint32 size, const void *data);

    // Change the tag of an existing referenced userdata
    void SetUserDataReferenceTag(ScriptReferenceType reference, ScriptUserDataTag tag);

    //==========================================================================================================================================================================================================
    // Script Invocation
    //==========================================================================================================================================================================================================

    // Script runner
    ScriptCallResult RunScript(const byte *script, uint32 scriptLength, const char *source = "text chunk");
    
    // Retrieves the number of return values after script execution.
    uint32 GetCallResultCount();

    // Ends the script, cleaning up any return values.
    void EndCall();

    // Call from a script native function, execution of the LUA thread will be paused and resumable later if it is a resumable call. Forward this return value back from the native function.
    int YieldThreadedCall(lua_State *L, const char *waitChannel = "", float timeout = DEFAULT_SCRIPT_TIMEOUT, ScriptThreadTimeoutAction timeoutAction = ScriptThreadTimeoutAction_Abort);

    // Retrieves the number of return values from a resumable script.
    uint32 GetThreadedCallResultCount(ScriptThread *pThread);

    // Prematurely ends the threaded call, cleaning up.
    void AbortThreadedCall(ScriptThread *pThread);

    // Ends the resumable script, cleaning up any return values.
    void EndThreadedCall(ScriptThread *pThread);

    //==========================================================================================================================================================================================================
    // Script Thread Management
    //==========================================================================================================================================================================================================///
    const uint32 GetPauedThreadCount() const { return m_pausedThreads.GetSize(); }
    const ScriptThread *GetPausedThread(uint32 i) const { return m_pausedThreads[i]; }
    void CheckPausedThreadTimeout(float deltaTime);

    //==========================================================================================================================================================================================================
    // Script Object Management
    //==========================================================================================================================================================================================================///

    // Pushes/creates a new instance of an object.
    static void PushNewObjectReference(lua_State *L, ScriptReferenceType metaTableReference, ScriptUserDataTag tag, const void *pObjectPointer);
    ScriptReferenceType AllocateAndReferenceObject(ScriptReferenceType metaTableReference, ScriptUserDataTag tag, const void *pObjectPointer);
    void SetObjectReferencePointer(ScriptReferenceType objectReference, const void *pObject);
    static void *CheckObjectTypeByMetaTable(lua_State *L, ScriptReferenceType metaTableReference, int arg);
    static void *CheckObjectTypeByTag(lua_State *L, ScriptUserDataTag tag, int arg);

    // Create an object's script environment, and hooks up any methods to the object's internal table.
    ScriptCallResult RunObjectScript(ScriptReferenceType objectReference, const byte *script, uint32 scriptLength, const char *source = "text chunk");

    // Prepares an entity table object call.
    ScriptCallResult BeginObjectMethodCall(ScriptReferenceType objectReference, const char *methodName);

    // Invokes an entity table object call.
    ScriptCallResult InvokeObjectMethodCall(bool saveResults = false);

    // Prepares a resumable entity table object call.
    ScriptCallResult BeginThreadedObjectMethodCall(ScriptThread **ppThread, ScriptReferenceType objectReference, const char *methodName);

    // Invokes a resumable entity table object call. This same function is called when resuming a paused thread.
    ScriptCallResult ResumeThreadedObjectMethodCall(ScriptThread *pThread, bool saveResults = false);

    // Templated versions of the above for ease-of-use
    ScriptCallResult CallObjectMethod(ScriptReferenceType objectReference, const char *methodName, bool saveResults = false);
    template<typename P1> ScriptCallResult CallObjectMethod(ScriptReferenceType objectReference, const char *methodName, P1 arg1, bool saveResults = false);
    template<typename P1, typename P2> ScriptCallResult CallObjectMethod(ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, bool saveResults = false);
    template<typename P1, typename P2, typename P3> ScriptCallResult CallObjectMethod(ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, P3 arg3, bool saveResults = false);
    template<typename P1, typename P2, typename P3, typename P4> ScriptCallResult CallObjectMethod(ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, P3 arg3, P4 arg4, bool saveResults = false);
    template<typename P1, typename P2, typename P3, typename P4, typename P5> ScriptCallResult CallObjectMethod(ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, bool saveResults = false);

    // Templated versions of above for threaded calls
    ScriptCallResult CallThreadedObjectMethod(ScriptThread **ppThread, ScriptReferenceType objectReference, const char *methodName, bool saveResults = false);
    template<typename P1> ScriptCallResult CallThreadedObjectMethod(ScriptThread **ppThread, ScriptReferenceType objectReference, const char *methodName, P1 arg1, bool saveResults = false);
    template<typename P1, typename P2> ScriptCallResult CallThreadedObjectMethod(ScriptThread **ppThread, ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, bool saveResults = false);
    template<typename P1, typename P2, typename P3> ScriptCallResult CallThreadedObjectMethod(ScriptThread **ppThread, ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, P3 arg3, bool saveResults = false);
    template<typename P1, typename P2, typename P3, typename P4> ScriptCallResult CallThreadedObjectMethod(ScriptThread **ppThread, ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, P3 arg3, P4 arg4, bool saveResults = false);
    template<typename P1, typename P2, typename P3, typename P4, typename P5> ScriptCallResult CallThreadedObjectMethod(ScriptThread **ppThread, ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, bool saveResults = false);

private:
    //////////////////////////////////////////////////////////////////////////

    // Register known entity types as lua types
    void RegisterBuiltinFunctions();
    void RegisterPrimitiveTypes();
    void UnregisterPrimitiveTypes();

    // Lock global variables from being modified
    void LockGlobals();
    void UnlockGlobals();

    //////////////////////////////////////////////////////////////////////////

    // lua state
    lua_State *m_state;

    //////////////////////////////////////////////////////////////////////////

    // paused threads
    PODArray<ScriptThread *> m_pausedThreads;
    
private:
    DeclareNonCopyable(ScriptManager);
};

extern ScriptManager *g_pScriptManager;

//////////////////////////////////////////////////////////////////////////

// Wrapper around a pending coroutine
class ScriptThread
{
    friend ScriptManager;

public:
    ScriptThread(lua_State *threadState, ScriptReferenceType threadReference);
    ~ScriptThread();

    // Retreives the state of the thread.
    lua_State *GetThreadState() const { return m_pThreadState; }

    // Retreives the wait channel of the thread.
    const String &GetWaitChannel() const { return m_waitChannel; }

    // Retreives the timeout of the thread.
    float GetTimeout() const { return m_timeout; }

    // Resets the timeout of the thread.
    void ResetTimeout(float timeout = DEFAULT_SCRIPT_TIMEOUT) { m_timeout = timeout; }

    // Change the timeout action of the thread.
    ScriptThreadTimeoutAction GetTimeoutAction() const { return m_timeoutAction; }
    void SetTimeoutAction(ScriptThreadTimeoutAction timeoutAction) { m_timeoutAction = timeoutAction; }

    // Number of times this thread has been paused.
    uint32 GetYieldCount() const { return m_yieldCount; }

private:
    // Pointer to thread state object
    lua_State *m_pThreadState;

    // Reference to the thread state object. This is necessary so that the LuA GC doesn't kill the state object.
    ScriptReferenceType m_threadReference;

    // Thread wait channel.
    String m_waitChannel;

    // Thread wait timeout. After this amount of time, the thread will perform the TimeoutAction action.
    float m_timeout;
    ScriptThreadTimeoutAction m_timeoutAction;

    // Number of times this thread has been paused.
    uint32 m_yieldCount;

    // noncopyable
    DeclareNonCopyable(ScriptThread);
};

//////////////////////////////////////////////////////////////////////////

// Inlined versions of the above
inline ScriptCallResult ScriptManager::CallObjectMethod(ScriptReferenceType objectReference, const char *methodName, bool saveResults /* = false */)
{
    ScriptCallResult res = BeginObjectMethodCall(objectReference, methodName);
    if (res == ScriptCallResult_Success)
        res = InvokeObjectMethodCall(saveResults);

    return res;
}

template<typename P1>
inline ScriptCallResult ScriptManager::CallObjectMethod(ScriptReferenceType objectReference, const char *methodName, P1 arg1, bool saveResults /* = false */)
{
    ScriptCallResult res = BeginObjectMethodCall(objectReference, methodName);
    if (res == ScriptCallResult_Success)
    {
        ScriptArg<P1>::Push(m_state, arg1);
        res = InvokeObjectMethodCall(saveResults);
    }
    return res;
}

template<typename P1, typename P2>
inline ScriptCallResult ScriptManager::CallObjectMethod(ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, bool saveResults /* = false */)
{
    ScriptCallResult res = BeginObjectMethodCall(objectReference, methodName);
    if (res == ScriptCallResult_Success)
    {
        ScriptArg<P1>::Push(m_state, arg1);
        ScriptArg<P2>::Push(m_state, arg2);
        res = InvokeObjectMethodCall(saveResults);
    }
    return res;
}

template<typename P1, typename P2, typename P3>
inline ScriptCallResult ScriptManager::CallObjectMethod(ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, P3 arg3, bool saveResults /* = false */)
{
    ScriptCallResult res = BeginObjectMethodCall(objectReference, methodName);
    if (res == ScriptCallResult_Success)
    {
        ScriptArg<P1>::Push(m_state, arg1);
        ScriptArg<P2>::Push(m_state, arg2);
        ScriptArg<P3>::Push(m_state, arg3);
        res = InvokeObjectMethodCall(saveResults);
    }
    return res;
}

template<typename P1, typename P2, typename P3, typename P4>
inline ScriptCallResult ScriptManager::CallObjectMethod(ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, P3 arg3, P4 arg4, bool saveResults /* = false */)
{
    ScriptCallResult res = BeginObjectMethodCall(objectReference, methodName);
    if (res == ScriptCallResult_Success)
    {
        ScriptArg<P1>::Push(m_state, arg1);
        ScriptArg<P2>::Push(m_state, arg2);
        ScriptArg<P3>::Push(m_state, arg3);
        ScriptArg<P4>::Push(m_state, arg4);
        res = InvokeObjectMethodCall(saveResults);
    }
    return res;
}

template<typename P1, typename P2, typename P3, typename P4, typename P5>
inline ScriptCallResult ScriptManager::CallObjectMethod(ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, bool saveResults /* = false */)
{
    ScriptCallResult res = BeginObjectMethodCall(objectReference, methodName);
    if (res == ScriptCallResult_Success)
    {
        ScriptArg<P1>::Push(m_state, arg1);
        ScriptArg<P2>::Push(m_state, arg2);
        ScriptArg<P3>::Push(m_state, arg3);
        ScriptArg<P4>::Push(m_state, arg4);
        ScriptArg<P5>::Push(m_state, arg5);
        res = InvokeObjectMethodCall(saveResults);
    }
    return res;
}

inline ScriptCallResult ScriptManager::CallThreadedObjectMethod(ScriptThread **ppThread, ScriptReferenceType objectReference, const char *methodName, bool saveResults /* = false */)
{
    ScriptCallResult res = BeginThreadedObjectMethodCall(ppThread, objectReference, methodName);
    if (res == ScriptCallResult_Success)
        res = ResumeThreadedObjectMethodCall(*ppThread, saveResults);

    return res;
}

template<typename P1>
inline ScriptCallResult ScriptManager::CallThreadedObjectMethod(ScriptThread **ppThread, ScriptReferenceType objectReference, const char *methodName, P1 arg1, bool saveResults /* = false */)
{
    ScriptCallResult res = BeginThreadedObjectMethodCall(ppThread, objectReference, methodName);
    if (res == ScriptCallResult_Success)
    {
        ScriptArg<P1>::Push(m_state, arg1);
        res = ResumeThreadedObjectMethodCall(*ppThread, saveResults);
    }
    return res;
}

template<typename P1, typename P2>
inline ScriptCallResult ScriptManager::CallThreadedObjectMethod(ScriptThread **ppThread, ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, bool saveResults /* = false */)
{
    ScriptCallResult res = BeginThreadedObjectMethodCall(ppThread, objectReference, methodName);
    if (res == ScriptCallResult_Success)
    {
        ScriptArg<P1>::Push(m_state, arg1);
        ScriptArg<P2>::Push(m_state, arg2);
        res = ResumeThreadedObjectMethodCall(*ppThread, saveResults);
    }
    return res;
}

template<typename P1, typename P2, typename P3>
inline ScriptCallResult ScriptManager::CallThreadedObjectMethod(ScriptThread **ppThread, ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, P3 arg3, bool saveResults /* = false */)
{
    ScriptCallResult res = BeginThreadedObjectMethodCall(ppThread, objectReference, methodName);
    if (res == ScriptCallResult_Success)
    {
        ScriptArg<P1>::Push(m_state, arg1);
        ScriptArg<P2>::Push(m_state, arg2);
        ScriptArg<P3>::Push(m_state, arg3);
        res = ResumeThreadedObjectMethodCall(*ppThread, saveResults);
    }
    return res;
}

template<typename P1, typename P2, typename P3, typename P4>
inline ScriptCallResult ScriptManager::CallThreadedObjectMethod(ScriptThread **ppThread, ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, P3 arg3, P4 arg4, bool saveResults /* = false */)
{
    ScriptCallResult res = BeginThreadedObjectMethodCall(ppThread, objectReference, methodName);
    if (res == ScriptCallResult_Success)
    {
        ScriptArg<P1>::Push(m_state, arg1);
        ScriptArg<P2>::Push(m_state, arg2);
        ScriptArg<P3>::Push(m_state, arg3);
        ScriptArg<P4>::Push(m_state, arg4);
        res = ResumeThreadedObjectMethodCall(*ppThread, saveResults);
    }
    return res;
}

template<typename P1, typename P2, typename P3, typename P4, typename P5>
inline ScriptCallResult ScriptManager::CallThreadedObjectMethod(ScriptThread **ppThread, ScriptReferenceType objectReference, const char *methodName, P1 arg1, P2 arg2, P3 arg3, P4 arg4, P5 arg5, bool saveResults /* = false */)
{
    ScriptCallResult res = BeginThreadedObjectMethodCall(ppThread, objectReference, methodName);
    if (res == ScriptCallResult_Success)
    {
        ScriptArg<P1>::Push(m_state, arg1);
        ScriptArg<P2>::Push(m_state, arg2);
        ScriptArg<P3>::Push(m_state, arg3);
        ScriptArg<P4>::Push(m_state, arg4);
        ScriptArg<P5>::Push(m_state, arg5);
        res = ResumeThreadedObjectMethodCall(*ppThread, saveResults);
    }
    return res;
}

