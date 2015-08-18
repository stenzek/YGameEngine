#include "Engine/ScriptManager.h"
#include "lua.hpp"

//////////////////////////////////////////////////////////////////////////
// Wrapper macros - this as a value type
// TODO: REPLACE ALL THIS WITH VARIADIC TEMPLATES
//////////////////////////////////////////////////////////////////////////

#define MAKE_SCRIPT_PROXY_FUNCTION_THISVAL(wrapperName, thisType, function) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        thisVal.function(); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISVAL_P1(wrapperName, thisType, function, p1Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        thisVal.function(p1Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISVAL_P2(wrapperName, thisType, function, p1Type, p2Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        thisVal.function(p1Val, p2Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISVAL_P3(wrapperName, thisType, function, p1Type, p2Type, p3Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        thisVal.function(p1Val, p2Val, p3Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISVAL_P4(wrapperName, thisType, function, p1Type, p2Type, p3Type, p4Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptArg<p4Type>::Get(L, 5); \
        thisVal.function(p1Val, p2Val, p3Val, p4Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISVAL_RET(wrapperName, retType, thisType, function) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        retType retVal = thisVal.function(); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(wrapperName, retType, thisType, function, p1Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        retType retVal = thisVal.function(p1Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P2(wrapperName, retType, thisType, function, p1Type, p2Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        retType retVal = thisVal.function(p1Val, p2Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P3(wrapperName, retType, thisType, function, p1Type, p2Type, p3Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        retType retVal = thisVal.function(p1Val, p2Val, p3Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P4(wrapperName, retType, thisType, function, p1Type, p2Type, p3Type, p4Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptArg<p4Type>::Get(L, 5); \
        retType retVal = thisVal.function(p1Val, p2Val, p3Val, p4Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISVAL(thisType, function) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        thisVal.function(); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISVAL_P1(thisType, function, p1Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        thisVal.function(p1Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISVAL_P2(thisType, function, p1Type, p2Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        thisVal.function(p1Val, p2Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISVAL_P3(thisType, function, p1Type, p2Type, p3Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        thisVal.function(p1Val, p2Val, p3Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISVAL_P4(thisType, function, p1Type, p2Type, p3Type, p4Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptArg<p4Type>::Get(L, 5); \
        thisVal.function(p1Val, p2Val, p3Val, p4Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISVAL_RET(retType, thisType, function) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        retType retVal = thisVal.function(); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISVAL_RET_P1(retType, thisType, function, p1Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        retType retVal = thisVal.function(p1Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISVAL_RET_P2(retType, thisType, function, p1Type, p2Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        retType retVal = thisVal.function(p1Val, p2Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISVAL_RET_P3(retType, thisType, function, p1Type, p2Type, p3Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        retType retVal = thisVal.function(p1Val, p2Val, p3Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISVAL_RET_P4(retType, thisType, function, p1Type, p2Type, p3Type, p4Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptArg<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptArg<p4Type>::Get(L, 5); \
        retType retVal = thisVal.function(p1Val, p2Val, p3Val, p4Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

//////////////////////////////////////////////////////////////////////////
// Wrapper macros -- this value as pointer type with checks
//////////////////////////////////////////////////////////////////////////

#define MAKE_SCRIPT_PROXY_FUNCTION_THISPTR(thisType, function) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        thisVal->function(); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISPTR_P1(thisType, function, p1Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        thisVal->function(p1Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISPTR_P2(thisType, function, p1Type, p2Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        thisVal->function(p1Val, p2Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISPTR_P3(thisType, function, p1Type, p2Type, p3Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        thisVal->function(p1Val, p2Val, p3Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISPTR_P4(thisType, function, p1Type, p2Type, p3Type, p4Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptArg<p4Type>::Get(L, 5); \
        thisVal->function(p1Val, p2Val, p3Val, p4Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISPTR_RET(retType, thisType, function) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        retType retVal = thisVal->function(); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISPTR_RET_P1(retType, thisType, function, p1Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        retType retVal = thisVal->function(p1Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISPTR_RET_P2(retType, thisType, function, p1Type, p2Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        retType retVal = thisVal->function(p1Val, p2Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISPTR_RET_P3(retType, thisType, function, p1Type, p2Type, p3Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        retType retVal = thisVal->function(p1Val, p2Val, p3Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_FUNCTION_THISPTR_RET_P4(retType, thisType, function, p1Type, p2Type, p3Type, p4Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptArg<p4Type>::Get(L, 5); \
        retType retVal = thisVal->function(p1Val, p2Val, p3Val, p4Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISPTR(thisType, function) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        thisVal->function(); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_P1(thisType, function, p1Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        thisVal->function(p1Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_P2(thisType, function, p1Type, p2Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        thisVal->function(p1Val, p2Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_P3(thisType, function, p1Type, p2Type, p3Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        thisVal->function(p1Val, p2Val, p3Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_P4(thisType, function, p1Type, p2Type, p3Type, p4Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptArg<p4Type>::Get(L, 5); \
        thisVal->function(p1Val, p2Val, p3Val, p4Val); \
        return 0; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_RET(retType, thisType, function) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        retType retVal = thisVal->function(); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_RET_P1(retType, thisType, function, p1Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        retType retVal = thisVal->function(p1Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_RET_P2(retType, thisType, function, p1Type, p2Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        retType retVal = thisVal->function(p1Val, p2Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_RET_P3(retType, thisType, function, p1Type, p2Type, p3Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        retType retVal = thisVal->function(p1Val, p2Val, p3Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_SCRIPT_PROXY_LAMBDA_THISPTR_RET_P4(retType, thisType, function, p1Type, p2Type, p3Type, p4Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptArg<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptArg<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptArg<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptArg<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptArg<p4Type>::Get(L, 5); \
        retType retVal = thisVal->function(p1Val, p2Val, p3Val, p4Val); \
        ScriptArg<retType>::Push(L, retVal); \
        return 1; \
    }

//////////////////////////////////////////////////////////////////////////
// Wrapper macros - this as a value type
// TODO: REPLACE ALL THIS WITH VARIADIC TEMPLATES
//////////////////////////////////////////////////////////////////////////

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(wrapperName, thisType, function) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        thisVal.function(); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_P1(wrapperName, thisType, function, p1Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        thisVal.function(p1Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_P2(wrapperName, thisType, function, p1Type, p2Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        thisVal.function(p1Val, p2Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_P3(wrapperName, thisType, function, p1Type, p2Type, p3Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        thisVal.function(p1Val, p2Val, p3Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_P4(wrapperName, thisType, function, p1Type, p2Type, p3Type, p4Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptComplexType<p4Type>::Get(L, 5); \
        thisVal.function(p1Val, p2Val, p3Val, p4Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(wrapperName, retType, thisType, function) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        retType retVal = thisVal.function(); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(wrapperName, retType, thisType, function, p1Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        retType retVal = thisVal.function(p1Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P2(wrapperName, retType, thisType, function, p1Type, p2Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        retType retVal = thisVal.function(p1Val, p2Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P3(wrapperName, retType, thisType, function, p1Type, p2Type, p3Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        retType retVal = thisVal.function(p1Val, p2Val, p3Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P4(wrapperName, retType, thisType, function, p1Type, p2Type, p3Type, p4Type) \
    static int wrapperName(lua_State *L) { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptComplexType<p4Type>::Get(L, 5); \
        retType retVal = thisVal.function(p1Val, p2Val, p3Val, p4Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISVAL(thisType, function) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        thisVal.function(); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISVAL_P1(thisType, function, p1Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        thisVal.function(p1Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISVAL_P2(thisType, function, p1Type, p2Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        thisVal.function(p1Val, p2Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISVAL_P3(thisType, function, p1Type, p2Type, p3Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        thisVal.function(p1Val, p2Val, p3Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISVAL_P4(thisType, function, p1Type, p2Type, p3Type, p4Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptComplexType<p4Type>::Get(L, 5); \
        thisVal.function(p1Val, p2Val, p3Val, p4Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISVAL_RET(retType, thisType, function) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        retType retVal = thisVal.function(); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISVAL_RET_P1(retType, thisType, function, p1Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        retType retVal = thisVal.function(p1Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISVAL_RET_P2(retType, thisType, function, p1Type, p2Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        retType retVal = thisVal.function(p1Val, p2Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISVAL_RET_P3(retType, thisType, function, p1Type, p2Type, p3Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        retType retVal = thisVal.function(p1Val, p2Val, p3Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISVAL_RET_P4(retType, thisType, function, p1Type, p2Type, p3Type, p4Type) \
    [](lua_State *L) -> int { \
        thisType thisVal = ScriptComplexType<thisType>::Get(L, 1); \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptComplexType<p4Type>::Get(L, 5); \
        retType retVal = thisVal.function(p1Val, p2Val, p3Val, p4Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

//////////////////////////////////////////////////////////////////////////
// Wrapper macros -- this value as pointer type with checks
//////////////////////////////////////////////////////////////////////////

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISPTR(thisType, function) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        thisVal->function(); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISPTR_P1(thisType, function, p1Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        thisVal->function(p1Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISPTR_P2(thisType, function, p1Type, p2Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        thisVal->function(p1Val, p2Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISPTR_P3(thisType, function, p1Type, p2Type, p3Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        thisVal->function(p1Val, p2Val, p3Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISPTR_P4(thisType, function, p1Type, p2Type, p3Type, p4Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptComplexType<p4Type>::Get(L, 5); \
        thisVal->function(p1Val, p2Val, p3Val, p4Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISPTR_RET(retType, thisType, function) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        retType retVal = thisVal->function(); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISPTR_RET_P1(retType, thisType, function, p1Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        retType retVal = thisVal->function(p1Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISPTR_RET_P2(retType, thisType, function, p1Type, p2Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        retType retVal = thisVal->function(p1Val, p2Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISPTR_RET_P3(retType, thisType, function, p1Type, p2Type, p3Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        retType retVal = thisVal->function(p1Val, p2Val, p3Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISPTR_RET_P4(retType, thisType, function, p1Type, p2Type, p3Type, p4Type) \
    static int wrapperName(lua_State *L) { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptComplexType<p4Type>::Get(L, 5); \
        retType retVal = thisVal->function(p1Val, p2Val, p3Val, p4Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISPTR(thisType, function) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        thisVal->function(); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISPTR_P1(thisType, function, p1Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        thisVal->function(p1Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISPTR_P2(thisType, function, p1Type, p2Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        thisVal->function(p1Val, p2Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISPTR_P3(thisType, function, p1Type, p2Type, p3Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        thisVal->function(p1Val, p2Val, p3Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISPTR_P4(thisType, function, p1Type, p2Type, p3Type, p4Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptComplexType<p4Type>::Get(L, 5); \
        thisVal->function(p1Val, p2Val, p3Val, p4Val); \
        return 0; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISPTR_RET(retType, thisType, function) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        retType retVal = thisVal->function(); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISPTR_RET_P1(retType, thisType, function, p1Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        retType retVal = thisVal->function(p1Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISPTR_RET_P2(retType, thisType, function, p1Type, p2Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        retType retVal = thisVal->function(p1Val, p2Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISPTR_RET_P3(retType, thisType, function, p1Type, p2Type, p3Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        retType retVal = thisVal->function(p1Val, p2Val, p3Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }

#define MAKE_COMPLEX_SCRIPT_PROXY_LAMBDA_THISPTR_RET_P4(retType, thisType, function, p1Type, p2Type, p3Type, p4Type) \
    [](lua_State *L) -> int { \
        thisType *thisVal = ScriptComplexType<thisType *>::Get(L, 1); \
        if (thisVal == nullptr) { luaL_argerror(L, 1, "attempt to call method on null object"); } \
        p1Type p1Val = ScriptComplexType<p1Type>::Get(L, 2); \
        p2Type p2Val = ScriptComplexType<p2Type>::Get(L, 3); \
        p3Type p3Val = ScriptComplexType<p3Type>::Get(L, 4); \
        p4Type p4Val = ScriptComplexType<p4Type>::Get(L, 5); \
        retType retVal = thisVal->function(p1Val, p2Val, p3Val, p4Val); \
        ScriptComplexType<retType>::Push(L, retVal); \
        return 1; \
    }
