#include "Engine/PrecompiledHeader.h"
#include "Engine/ScriptManager.h"
#include "Engine/ScriptProxyFunctions.h"
#include "lua.hpp"

// Primitive types builtin to lua language
template<> bool ScriptArg<bool>::Get(lua_State *L, int arg) { return (luaL_checkinteger(L, arg) != 0); }
template<> void ScriptArg<bool>::Push(lua_State *L, const bool &arg) { lua_pushboolean(L, (int)arg); }
template<> int8 ScriptArg<int8>::Get(lua_State *L, int arg) { return (int8)luaL_checkinteger(L, arg); }
template<> void ScriptArg<int8>::Push(lua_State *L, const int8 &arg) { lua_pushinteger(L, (lua_Integer)arg); }
template<> uint8 ScriptArg<uint8>::Get(lua_State *L, int arg) { return (uint8)luaL_checkinteger(L, arg); }
template<> void ScriptArg<uint8>::Push(lua_State *L, const uint8 &arg) { lua_pushinteger(L, (lua_Integer)arg); }
template<> int16 ScriptArg<int16>::Get(lua_State *L, int arg) { return (int16)luaL_checkinteger(L, arg); }
template<> void ScriptArg<int16>::Push(lua_State *L, const int16 &arg) { lua_pushinteger(L, (lua_Integer)arg); }
template<> uint16 ScriptArg<uint16>::Get(lua_State *L, int arg) { return (uint16)luaL_checkinteger(L, arg); } 
template<> void ScriptArg<uint16>::Push(lua_State *L, const uint16 &arg) { lua_pushinteger(L, (lua_Integer)arg); }
template<> int32 ScriptArg<int32>::Get(lua_State *L, int arg) { return (int32)luaL_checkinteger(L, arg); } 
template<> void ScriptArg<int32>::Push(lua_State *L, const int32 &arg) { lua_pushinteger(L, (lua_Integer)arg); }
template<> uint32 ScriptArg<uint32>::Get(lua_State *L, int arg) { return (uint32)luaL_checkinteger(L, arg); } 
template<> void ScriptArg<uint32>::Push(lua_State *L, const uint32 &arg) { lua_pushinteger(L, (lua_Integer)arg); }
template<> float ScriptArg<float>::Get(lua_State *L, int arg) { return (float)luaL_checknumber(L, arg); } 
template<> void ScriptArg<float>::Push(lua_State *L, const float &arg) { lua_pushnumber(L, (lua_Number)arg); }
template<> double ScriptArg<double>::Get(lua_State *L, int arg) { return (double)luaL_checknumber(L, arg); } 
template<> void ScriptArg<double>::Push(lua_State *L, const double &arg) { lua_pushnumber(L, (lua_Number)arg); }
template<> const char *ScriptArg<const char *>::Get(lua_State *L, int arg) { return luaL_checkstring(L, arg); }
template<> void ScriptArg<const char *>::Push(lua_State *L, const char *const &arg) { lua_pushstring(L, arg); }
template<> String ScriptArg<String>::Get(lua_State *L, int arg) { return String(luaL_checkstring(L, arg)); }
template<> void ScriptArg<String>::Push(lua_State *L, const String &arg) { lua_pushstring(L, arg); }

// Complex type
template<class T> struct ScriptComplexType { };

// So messy... fix this.
template<> struct ScriptComplexType<bool>
{
    static bool Get(lua_State *L, int arg) { return (luaL_checkinteger(L, arg) != 0); }
    static void Push(lua_State *L, const bool &arg) { lua_pushboolean(L, (int)arg); }
};
template<> struct ScriptComplexType<float>
{
    static float Get(lua_State *L, int arg) { return (float)luaL_checknumber(L, arg); }
    static void Push(lua_State *L, const float &arg) { lua_pushnumber(L, (lua_Number)arg); }
};

// float2
template<>
struct ScriptComplexType<float2>
{
    static inline ScriptReferenceType &MetaTableReference() { static ScriptReferenceType ref = INVALID_SCRIPT_REFERENCE; return ref; }
    static inline const char *Name() { return "float2"; }

    // get/push methods
    static float2 Get(lua_State *L, int arg)
    {
        return float2(*reinterpret_cast<const float2 *>(g_pScriptManager->CheckUserDataTypeByMetaTable(L, MetaTableReference(), arg)));
    }
    static void Push(lua_State *L, const float2 &arg)
    {
        g_pScriptManager->PushNewUserData(L, MetaTableReference(), ScriptUserDataTag_None, sizeof(float2), &arg);
    }

    // get/push ptr methods
    static float2 *GetPtr(lua_State *L, int arg)
    {
        return reinterpret_cast<float2 *>(g_pScriptManager->CheckUserDataTypeByMetaTable(L, MetaTableReference(), arg));
    }
    static void PushPtr(lua_State *L, const float2 *arg)
    {
        g_pScriptManager->PushNewUserData(L, MetaTableReference(), ScriptUserDataTag_None, sizeof(float2), arg);
    }

    // Metamethods
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__add, float2, float2, operator+, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__sub, float2, float2, operator-, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__mul, float2, float2, operator*, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__div, float2, float2, operator*, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__mod, float2, float2, operator%, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__unm, float2, float2, operator-, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__eq, bool, float2, operator==, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__lt, bool, float2, operator<, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__le, bool, float2, operator<=, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__gt, bool, float2, operator>, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__ge, bool, float2, operator>=, float2);

    // Methods
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_P2(Set, float2, Set, float, float);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(SetZero, float2, SetZero);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(AnyLess, bool, float2, AnyLess, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(AnyGreater, bool, float2, AnyGreater, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P2(NearEqual, bool, float2, NearEqual, float2, float);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(IsFinite, bool, float2, IsFinite);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Min, float2, float2, Min, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Max, float2, float2, Max, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P2(Clamp, float2, float2, Clamp, float2, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Abs, float2, float2, Abs);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Saturate, float2, float2, Saturate);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Snap, float2, float2, Snap, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Floor, float2, float2, Floor);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Ceil, float2, float2, Ceil);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Round, float2, float2, Round);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Dot, float, float2, Dot, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P2(Lerp, float2, float2, Lerp, float2, float);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(SquaredLength, float, float2, SquaredLength);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Length, float, float2, Length);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(SquaredDistance, float, float2, SquaredDistance, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Distance, float, float2, Distance, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Normalize, float2, float2, Normalize);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(NormalizeEst, float2, float2, NormalizeEst);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(NormalizeInPlace, float2, NormalizeInPlace);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(NormalizeEstInPlace, float2, NormalizeEstInPlace);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(SafeNormalize, float2, float2, SafeNormalize);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(SafeNormalizeEst, float2, float2, SafeNormalizeEst);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(SafeNormalizeInPlace, float2, SafeNormalizeInPlace);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(SafeNormalizeEstInPlace, float2, SafeNormalizeEstInPlace);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Reciprocal, float2, float2, Reciprocal);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Cross, float2, float2, Cross, float2);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(MinOfComponents, float, float2, MinOfComponents);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(MaxOfComponents, float, float2, MaxOfComponents);

    // Custom-wrapped metamethods
    static int Constructor(lua_State *L)
    {
        // construct val
        float2 val;

        // by args...
        int nargs = lua_gettop(L);
        if (nargs == 0)
        {
            // set to zero
            val.SetZero();
        }
        else if (nargs == 1)
        {
            // splat across all components
            float cval = (float)luaL_checknumber(L, 1);
            val.Set(cval, cval);
        }
        else if (nargs == 2)
        {
            // all components defined
            float x = (float)luaL_checknumber(L, 1);
            float y = (float)luaL_checknumber(L, 2);
            val.Set(x, y);
        }
        else
        {
            luaL_error(L, "invalid constructor parameters");
            return 0;
        }

        g_pScriptManager->PushNewUserData(L, MetaTableReference(), ScriptUserDataTag_None, sizeof(float2), &val);
        return 1;
    }
    static int __index(lua_State *L)
    {
        // should have an argument, always
        const float2 *self = GetPtr(L, 1);

        // check the methods table first
        lua_pushvalue(L, 2);
        lua_gettable(L, lua_upvalueindex(1));
        if (lua_isfunction(L, -1))
            return 1;
        else
            lua_pop(L, 1);

        // getting by-string?
        if (lua_isstring(L, 2))
        {
            // number of components?
            const char *compStr = lua_tostring(L, 2);
            size_t compLen = Y_strlen(compStr);
            if (compLen == 1)
            {
                // access single component
                switch (compStr[0])
                {
                case 'x':
                    lua_pushnumber(L, (lua_Number)self->x);
                    return 1;

                case 'y':
                    lua_pushnumber(L, (lua_Number)self->y);
                    return 1;
                }

                luaL_error(L, "invalid component: %s", compStr);
                return 0;
            }
            else
            {
                // construct new vector using swizzle
                luaL_error(L, "invalid swizzle: %s", compStr);
                return 0;
            }
        }
        else if (lua_isinteger(L, 2))
        {
            // access component
            int component = (int)lua_tointeger(L, 2);
            if (component < 0 || component > 1)
            {
                luaL_error(L, "invalid component index: %d", component);
                return 0;
            }

            lua_pushnumber(L, self->ele[component]);
            return 1;
        }
        else
        {
            ScriptManager::GenerateTypeError(L, 2, "string or integer");
            return 0;
        }
    }
    static int __newindex(lua_State *L)
    {
        // should have an argument, always
        float2 *self = GetPtr(L, 1);
        float val = (float)luaL_checknumber(L, 3);

        // getting by-string?
        if (lua_isstring(L, 2))
        {
            // number of components?
            const char *compStr = lua_tostring(L, 2);
            size_t compLen = Y_strlen(compStr);
            if (compLen == 1)
            {
                // access single component
                switch (compStr[0])
                {
                case 'x':
                    self->x = val;
                    return 0;

                case 'y':
                    self->y = val;
                    return 0;
                }

                luaL_error(L, "invalid component: %s", compStr);
                return 0;
            }
            else
            {
                // construct new vector using swizzle
                luaL_error(L, "vector sets can only be done with a single component");
                return 0;
            }
        }
        else if (lua_isinteger(L, 2))
        {
            // access component
            int component = (int)lua_tointeger(L, 2);
            if (component < 0 || component > 1)
            {
                luaL_error(L, "invalid component index: %d", component);
                return 0;
            }

            self->ele[component] = val;
            return 0;
        }
        else
        {
            ScriptManager::GenerateTypeError(L, 2, "string or integer");
            return 0;
        }
    }

    static inline const SCRIPT_FUNCTION_TABLE_ENTRY *MetaMethodFunctions()
    {
        static const SCRIPT_FUNCTION_TABLE_ENTRY reg[] =
        {
            { "__add", __add },
            { "__sub", __sub },
            { "__mul", __mul },
            { "__div", __div },
            { "__mod", __mod },
            { "__unm", __unm },
            { "__eq", __eq },
            { "__lt", __lt },
            { "__le", __le },
            { "__gt", __gt },
            { "__ge", __ge },
            { "__index", __index },
            { "__newindex", __newindex },
            { nullptr, nullptr }
        };

        return reg;
    }

    static inline const SCRIPT_FUNCTION_TABLE_ENTRY *MethodFunctions()
    {
        static const SCRIPT_FUNCTION_TABLE_ENTRY reg[] =
        {
            { "Set", Set },
            { "SetZero", SetZero },
            { "AnyLess", AnyLess },
            { "AnyGreater", AnyGreater },
            { "IsFinite", IsFinite },
            { "Min", Min },
            { "Max", Max },
            { "Clamp", Clamp },
            { "Abs", Abs },
            { "Saturate", Saturate },
            { "Snap", Snap },
            { "Floor", Floor },
            { "Ceil", Ceil },
            { "Round", Round },
            { "Dot", Dot },
            { "Lerp", Lerp },
            { "SquaredLength", SquaredLength },
            { "Length", Length },
            { "SquaredDistance", SquaredDistance },
            { "Distance", Distance },
            { "Normalize", Normalize },
            { "NormalizeEst", NormalizeEst },
            { "NormalizeInPlace", NormalizeInPlace },
            { "NormalizeEstInPlace", NormalizeEstInPlace },
            { "SafeNormalize", SafeNormalize },
            { "SafeNormalizeEst", SafeNormalizeEst },
            { "SafeNormalizeInPlace", SafeNormalizeInPlace },
            { "SafeNormalizeEstInPlace", SafeNormalizeEstInPlace },
            { "Reciprocal", Reciprocal },
            { "Cross", Cross },
            { "MinOfComponents", MinOfComponents },
            { "MaxOfComponents", MaxOfComponents },
            { nullptr, nullptr }
        };

        return reg;
    }
};

template<>
struct ScriptComplexType<float3>
{
    static inline ScriptReferenceType &MetaTableReference() { static ScriptReferenceType ref = INVALID_SCRIPT_REFERENCE; return ref; }
    static inline const char *Name() { return "float3"; }

    // get/push methods
    static float3 Get(lua_State *L, int arg)
    {
        return float3(*reinterpret_cast<const float3 *>(g_pScriptManager->CheckUserDataTypeByMetaTable(L, MetaTableReference(), arg)));
    }
    static void Push(lua_State *L, const float3 &arg)
    {
        g_pScriptManager->PushNewUserData(L, MetaTableReference(), ScriptUserDataTag_None, sizeof(float3), &arg);
    }

    // get/push ptr methods
    static float3 *GetPtr(lua_State *L, int arg)
    {
        return reinterpret_cast<float3 *>(g_pScriptManager->CheckUserDataTypeByMetaTable(L, MetaTableReference(), arg));
    }
    static void PushPtr(lua_State *L, const float3 *arg)
    {
        g_pScriptManager->PushNewUserData(L, MetaTableReference(), ScriptUserDataTag_None, sizeof(float3), arg);
    }

    // Metamethods
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__add, float3, float3, operator+, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__sub, float3, float3, operator-, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__mul, float3, float3, operator*, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__div, float3, float3, operator*, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__mod, float3, float3, operator%, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__unm, float3, float3, operator-, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__eq, bool, float3, operator==, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__lt, bool, float3, operator<, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__le, bool, float3, operator<=, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__gt, bool, float3, operator>, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__ge, bool, float3, operator>=, float3);

    // Methods
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_P3(Set, float3, Set, float, float, float);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(SetZero, float3, SetZero);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(AnyLess, bool, float3, AnyLess, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(AnyGreater, bool, float3, AnyGreater, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P2(NearEqual, bool, float3, NearEqual, float3, float);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(IsFinite, bool, float3, IsFinite);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Min, float3, float3, Min, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Max, float3, float3, Max, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P2(Clamp, float3, float3, Clamp, float3, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Abs, float3, float3, Abs);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Saturate, float3, float3, Saturate);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Snap, float3, float3, Snap, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Floor, float3, float3, Floor);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Ceil, float3, float3, Ceil);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Round, float3, float3, Round);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Dot, float, float3, Dot, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P2(Lerp, float3, float3, Lerp, float3, float);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(SquaredLength, float, float3, SquaredLength);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Length, float, float3, Length);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(SquaredDistance, float, float3, SquaredDistance, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Distance, float, float3, Distance, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Normalize, float3, float3, Normalize);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(NormalizeEst, float3, float3, NormalizeEst);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(NormalizeInPlace, float3, NormalizeInPlace);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(NormalizeEstInPlace, float3, NormalizeEstInPlace);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(SafeNormalize, float3, float3, SafeNormalize);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(SafeNormalizeEst, float3, float3, SafeNormalizeEst);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(SafeNormalizeInPlace, float3, SafeNormalizeInPlace);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(SafeNormalizeEstInPlace, float3, SafeNormalizeEstInPlace);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Reciprocal, float3, float3, Reciprocal);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Cross, float3, float3, Cross, float3);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(MinOfComponents, float, float3, MinOfComponents);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(MaxOfComponents, float, float3, MaxOfComponents);

    // Custom-wrapped metamethods
    static int Constructor(lua_State *L)
    {
        // construct val
        float3 val;

        // by args...
        int nargs = lua_gettop(L);
        if (nargs == 0)
        {
            // set to zero
            val.SetZero();
        }
        else if (nargs == 1)
        {
            // splat across all components
            float cval = (float)luaL_checknumber(L, 1);
            val.Set(cval, cval, cval);
        }
        else if (nargs == 3)
        {
            // all components defined
            float x = (float)luaL_checknumber(L, 1);
            float y = (float)luaL_checknumber(L, 2);
            float z = (float)luaL_checknumber(L, 3);
            val.Set(x, y, z);
        }
        else
        {
            luaL_error(L, "invalid constructor parameters");
            return 0;
        }

        g_pScriptManager->PushNewUserData(L, MetaTableReference(), ScriptUserDataTag_None, sizeof(float3), &val);
        return 1;
    }
    static int __index(lua_State *L)
    {
        // should have an argument, always
        const float3 *self = GetPtr(L, 1);

        // check the methods table first
        lua_pushvalue(L, 2);
        lua_gettable(L, lua_upvalueindex(1));
        if (lua_isfunction(L, -1))
            return 1;
        else
            lua_pop(L, 1);

        // getting by-string?
        if (lua_isstring(L, 2))
        {
            // number of components?
            const char *compStr = lua_tostring(L, 2);
            size_t compLen = Y_strlen(compStr);
            if (compLen == 1)
            {
                // access single component
                switch (compStr[0])
                {
                case 'x':
                    lua_pushnumber(L, (lua_Number)self->x);
                    return 1;

                case 'y':
                    lua_pushnumber(L, (lua_Number)self->y);
                    return 1;

                case 'z':
                    lua_pushnumber(L, (lua_Number)self->z);
                    return 1;
                }

                luaL_error(L, "invalid component: %s", compStr);
                return 0;
            }
            else
            {
                // construct new vector using swizzle
                luaL_error(L, "invalid swizzle: %s", compStr);
                return 0;
            }
        }
        else if (lua_isinteger(L, 2))
        {
            // access component
            int component = (int)lua_tointeger(L, 2);
            if (component < 0 || component > 2)
            {
                luaL_error(L, "invalid component index: %d", component);
                return 0;
            }

            lua_pushnumber(L, self->ele[component]);
            return 1;
        }
        else
        {
            ScriptManager::GenerateTypeError(L, 2, "string or integer");
            return 0;
        }
    }
    static int __newindex(lua_State *L)
    {
        // should have an argument, always
        float3 *self = GetPtr(L, 1);
        float val = (float)luaL_checknumber(L, 3);

        // getting by-string?
        if (lua_isstring(L, 2))
        {
            // number of components?
            const char *compStr = lua_tostring(L, 2);
            size_t compLen = Y_strlen(compStr);
            if (compLen == 1)
            {
                // access single component
                switch (compStr[0])
                {
                case 'x':
                    self->x = val;
                    return 0;

                case 'y':
                    self->y = val;
                    return 0;

                case 'z':
                    self->z = val;
                    return 0;
                }

                luaL_error(L, "invalid component: %s", compStr);
                return 0;
            }
            else
            {
                // construct new vector using swizzle
                luaL_error(L, "vector sets can only be done with a single component");
                return 0;
            }
        }
        else if (lua_isinteger(L, 2))
        {
            // access component
            int component = (int)lua_tointeger(L, 2);
            if (component < 0 || component > 2)
            {
                luaL_error(L, "invalid component index: %d", component);
                return 0;
            }

            self->ele[component] = val;
            return 0;
        }
        else
        {
            ScriptManager::GenerateTypeError(L, 2, "string or integer");
            return 0;
        }
    }

    static inline const SCRIPT_FUNCTION_TABLE_ENTRY *MetaMethodFunctions()
    {
        static const SCRIPT_FUNCTION_TABLE_ENTRY reg[] =
        {
            { "__add", __add },
            { "__sub", __sub },
            { "__mul", __mul },
            { "__div", __div },
            { "__mod", __mod },
            { "__unm", __unm },
            { "__eq", __eq },
            { "__lt", __lt },
            { "__le", __le },
            { "__gt", __gt },
            { "__ge", __ge },
            { "__index", __index },
            { "__newindex", __newindex },
            { nullptr, nullptr }
        };

        return reg;
    }

    static inline const SCRIPT_FUNCTION_TABLE_ENTRY *MethodFunctions()
    {
        static const SCRIPT_FUNCTION_TABLE_ENTRY reg[] =
        {
            { "Set", Set },
            { "SetZero", SetZero },
            { "AnyLess", AnyLess },
            { "AnyGreater", AnyGreater },
            { "IsFinite", IsFinite },
            { "Min", Min },
            { "Max", Max },
            { "Clamp", Clamp },
            { "Abs", Abs },
            { "Saturate", Saturate },
            { "Snap", Snap },
            { "Floor", Floor },
            { "Ceil", Ceil },
            { "Round", Round },
            { "Dot", Dot },
            { "Lerp", Lerp },
            { "SquaredLength", SquaredLength },
            { "Length", Length },
            { "SquaredDistance", SquaredDistance },
            { "Distance", Distance },
            { "Normalize", Normalize },
            { "NormalizeEst", NormalizeEst },
            { "NormalizeInPlace", NormalizeInPlace },
            { "NormalizeEstInPlace", NormalizeEstInPlace },
            { "SafeNormalize", SafeNormalize },
            { "SafeNormalizeEst", SafeNormalizeEst },
            { "SafeNormalizeInPlace", SafeNormalizeInPlace },
            { "SafeNormalizeEstInPlace", SafeNormalizeEstInPlace },
            { "Reciprocal", Reciprocal },
            { "Cross", Cross },
            { "MinOfComponents", MinOfComponents },
            { "MaxOfComponents", MaxOfComponents },
            { nullptr, nullptr }
        };

        return reg;
    }
};

template<>
struct ScriptComplexType<float4>
{
    static inline ScriptReferenceType &MetaTableReference() { static ScriptReferenceType ref = INVALID_SCRIPT_REFERENCE; return ref; }
    static inline const char *Name() { return "float4"; }

    // get/push methods
    static float4 Get(lua_State *L, int arg)
    {
        return float4(*reinterpret_cast<const float4 *>(g_pScriptManager->CheckUserDataTypeByMetaTable(L, MetaTableReference(), arg)));
    }
    static void Push(lua_State *L, const float4 &arg)
    {
        g_pScriptManager->PushNewUserData(L, MetaTableReference(), ScriptUserDataTag_None, sizeof(float4), &arg);
    }

    // get/push ptr methods
    static float4 *GetPtr(lua_State *L, int arg)
    {
        return reinterpret_cast<float4 *>(g_pScriptManager->CheckUserDataTypeByMetaTable(L, MetaTableReference(), arg));
    }
    static void PushPtr(lua_State *L, const float4 *arg)
    {
        g_pScriptManager->PushNewUserData(L, MetaTableReference(), ScriptUserDataTag_None, sizeof(float4), arg);
    }

    // Metamethods
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__add, float4, float4, operator+, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__sub, float4, float4, operator-, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__mul, float4, float4, operator*, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__div, float4, float4, operator*, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__mod, float4, float4, operator%, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__unm, float4, float4, operator-, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__eq, bool, float4, operator==, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__lt, bool, float4, operator<, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__le, bool, float4, operator<=, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__gt, bool, float4, operator>, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__ge, bool, float4, operator>=, float4);

    // Methods
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_P4(Set, float4, Set, float, float, float, float);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(SetZero, float4, SetZero);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(AnyLess, bool, float4, AnyLess, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(AnyGreater, bool, float4, AnyGreater, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P2(NearEqual, bool, float4, NearEqual, float4, float);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(IsFinite, bool, float4, IsFinite);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Min, float4, float4, Min, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Max, float4, float4, Max, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P2(Clamp, float4, float4, Clamp, float4, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Abs, float4, float4, Abs);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Saturate, float4, float4, Saturate);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Snap, float4, float4, Snap, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Floor, float4, float4, Floor);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Ceil, float4, float4, Ceil);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Round, float4, float4, Round);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Dot, float, float4, Dot, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P2(Lerp, float4, float4, Lerp, float4, float);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(SquaredLength, float, float4, SquaredLength);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Length, float, float4, Length);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(SquaredDistance, float, float4, SquaredDistance, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Distance, float, float4, Distance, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Normalize, float4, float4, Normalize);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(NormalizeEst, float4, float4, NormalizeEst);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(NormalizeInPlace, float4, NormalizeInPlace);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(NormalizeEstInPlace, float4, NormalizeEstInPlace);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(SafeNormalize, float4, float4, SafeNormalize);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(SafeNormalizeEst, float4, float4, SafeNormalizeEst);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(SafeNormalizeInPlace, float4, SafeNormalizeInPlace);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(SafeNormalizeEstInPlace, float4, SafeNormalizeEstInPlace);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Reciprocal, float4, float4, Reciprocal);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P2(Cross, float4, float4, Cross, float4, float4);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(MinOfComponents, float, float4, MinOfComponents);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(MaxOfComponents, float, float4, MaxOfComponents);

    // Custom-wrapped metamethods
    static int Constructor(lua_State *L)
    {
        // construct val
        float4 val;

        // by args...
        int nargs = lua_gettop(L);
        if (nargs == 0)
        {
            // set to zero
            val.SetZero();
        }
        else if (nargs == 1)
        {
            // splat across all components
            float cval = (float)luaL_checknumber(L, 1);
            val.Set(cval, cval, cval, cval);
        }
        else if (nargs == 4)
        {
            // all components defined
            float x = (float)luaL_checknumber(L, 1);
            float y = (float)luaL_checknumber(L, 2);
            float z = (float)luaL_checknumber(L, 3);
            float w = (float)luaL_checknumber(L, 4);
            val.Set(x, y, z, w);
        }
        else
        {
            luaL_error(L, "invalid constructor parameters");
            return 0;
        }

        g_pScriptManager->PushNewUserData(L, MetaTableReference(), ScriptUserDataTag_None, sizeof(float4), &val);
        return 1;
    }
    static int __index(lua_State *L)
    {
        // should have an argument, always
        const float4 *self = GetPtr(L, 1);

        // check the methods table first
        lua_pushvalue(L, 2);
        lua_gettable(L, lua_upvalueindex(1));
        if (lua_isfunction(L, -1))
            return 1;
        else
            lua_pop(L, 1);

        // getting by-string?
        if (lua_isstring(L, 2))
        {
            // number of components?
            const char *compStr = lua_tostring(L, 2);
            size_t compLen = Y_strlen(compStr);
            if (compLen == 1)
            {
                // access single component
                switch (compStr[0])
                {
                case 'x':
                    lua_pushnumber(L, (lua_Number)self->x);
                    return 1;

                case 'y':
                    lua_pushnumber(L, (lua_Number)self->y);
                    return 1;

                case 'z':
                    lua_pushnumber(L, (lua_Number)self->z);
                    return 1;

                case 'w':
                    lua_pushnumber(L, (lua_Number)self->w);
                    return 1;
                }

                luaL_error(L, "invalid component: %s", compStr);
                return 0;
            }
            else
            {
                // construct new vector using swizzle
                luaL_error(L, "invalid swizzle: %s", compStr);
                return 0;
            }
        }
        else if (lua_isinteger(L, 2))
        {
            // access component
            int component = (int)lua_tointeger(L, 2);
            if (component < 0 || component > 3)
            {
                luaL_error(L, "invalid component index: %d", component);
                return 0;
            }

            lua_pushnumber(L, self->ele[component]);
            return 1;
        }
        else
        {
            ScriptManager::GenerateTypeError(L, 2, "string or integer");
            return 0;
        }
    }
    static int __newindex(lua_State *L)
    {
        // should have an argument, always
        float4 *self = GetPtr(L, 1);
        float val = (float)luaL_checknumber(L, 3);

        // getting by-string?
        if (lua_isstring(L, 2))
        {
            // number of components?
            const char *compStr = lua_tostring(L, 2);
            size_t compLen = Y_strlen(compStr);
            if (compLen == 1)
            {
                // access single component
                switch (compStr[0])
                {
                case 'x':
                    self->x = val;
                    return 0;

                case 'y':
                    self->y = val;
                    return 0;

                case 'z':
                    self->z = val;
                    return 0;

                case 'w':
                    self->w = val;
                    return 0;
                }

                luaL_error(L, "invalid component: %s", compStr);
                return 0;
            }
            else
            {
                // construct new vector using swizzle
                luaL_error(L, "vector sets can only be done with a single component");
                return 0;
            }
        }
        else if (lua_isinteger(L, 2))
        {
            // access component
            int component = (int)lua_tointeger(L, 2);
            if (component < 0 || component > 3)
            {
                luaL_error(L, "invalid component index: %d", component);
                return 0;
            }

            self->ele[component] = val;
            return 0;
        }
        else
        {
            ScriptManager::GenerateTypeError(L, 2, "string or integer");
            return 0;
        }
    }

    static inline const SCRIPT_FUNCTION_TABLE_ENTRY *MetaMethodFunctions()
    {
        static const SCRIPT_FUNCTION_TABLE_ENTRY reg[] =
        {
            { "__add", __add },
            { "__sub", __sub },
            { "__mul", __mul },
            { "__div", __div },
            { "__mod", __mod },
            { "__unm", __unm },
            { "__eq", __eq },
            { "__lt", __lt },
            { "__le", __le },
            { "__gt", __gt },
            { "__ge", __ge },
            { "__index", __index },
            { "__newindex", __newindex },
            { nullptr, nullptr }
        };

        return reg;
    }

    static inline const SCRIPT_FUNCTION_TABLE_ENTRY *MethodFunctions()
    {
        static const SCRIPT_FUNCTION_TABLE_ENTRY reg[] =
        {
            { "Set", Set },
            { "SetZero", SetZero },
            { "AnyLess", AnyLess },
            { "AnyGreater", AnyGreater },
            { "IsFinite", IsFinite },
            { "Min", Min },
            { "Max", Max },
            { "Clamp", Clamp },
            { "Abs", Abs },
            { "Saturate", Saturate },
            { "Snap", Snap },
            { "Floor", Floor },
            { "Ceil", Ceil },
            { "Round", Round },
            { "Dot", Dot },
            { "Lerp", Lerp },
            { "SquaredLength", SquaredLength },
            { "Length", Length },
            { "SquaredDistance", SquaredDistance },
            { "Distance", Distance },
            { "Normalize", Normalize },
            { "NormalizeEst", NormalizeEst },
            { "NormalizeInPlace", NormalizeInPlace },
            { "NormalizeEstInPlace", NormalizeEstInPlace },
            { "SafeNormalize", SafeNormalize },
            { "SafeNormalizeEst", SafeNormalizeEst },
            { "SafeNormalizeInPlace", SafeNormalizeInPlace },
            { "SafeNormalizeEstInPlace", SafeNormalizeEstInPlace },
            { "Reciprocal", Reciprocal },
            { "Cross", Cross },
            { "MinOfComponents", MinOfComponents },
            { "MaxOfComponents", MaxOfComponents },
            { nullptr, nullptr }
        };

        return reg;
    }
};

template<>
struct ScriptComplexType<Quaternion>
{
    static inline ScriptReferenceType &MetaTableReference() { static ScriptReferenceType ref = INVALID_SCRIPT_REFERENCE; return ref; }
    static inline const char *Name() { return "Quaternion"; }

    // get/push methods
    static Quaternion Get(lua_State *L, int arg)
    {
        return Quaternion(*reinterpret_cast<const Quaternion *>(ScriptManager::CheckUserDataTypeByMetaTable(L, MetaTableReference(), arg)));
    }
    static void Push(lua_State *L, const Quaternion &arg)
    {
        ScriptManager::PushNewUserData(L, MetaTableReference(), ScriptUserDataTag_None, sizeof(Quaternion), &arg);
    }

    // get/push ptr methods
    static Quaternion *GetPtr(lua_State *L, int arg)
    {
        return reinterpret_cast<Quaternion *>(ScriptManager::CheckUserDataTypeByMetaTable(L, MetaTableReference(), arg));
    }
    static void PushPtr(lua_State *L, const Quaternion *arg)
    {
        ScriptManager::PushNewUserData(L, MetaTableReference(), ScriptUserDataTag_None, sizeof(Quaternion), arg);
    }

    // Metamethods
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__eq, bool, Quaternion, operator==, Quaternion);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(__mul, Quaternion, Quaternion, operator*, Quaternion);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(__unm, Quaternion, Quaternion, Inverse);

    // Methods
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_P4(Set, Quaternion, Set, float, float, float, float);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(SetIdentity, Quaternion, SetIdentity);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Length, float, Quaternion, Length);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET_P1(Dot, float, Quaternion, Dot, Quaternion);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(Normalize, Quaternion, Quaternion, Normalize);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL_RET(NormalizeEst, Quaternion, Quaternion, NormalizeEst);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(NormalizeInPlace, Quaternion, NormalizeInPlace);
    MAKE_COMPLEX_SCRIPT_PROXY_FUNCTION_THISVAL(NormalizeEstInPlace, Quaternion, NormalizeEstInPlace);

    // Custom-wrapped metamethods
    static int Constructor(lua_State *L)
    {
        // construct val
        Quaternion val;

        // by args...
        int nargs = lua_gettop(L);
        if (nargs == 0)
        {
            // set to zero
            val.SetIdentity();
        }
        else if (nargs == 4)
        {
            // all components defined
            float x = (float)luaL_checknumber(L, 1);
            float y = (float)luaL_checknumber(L, 2);
            float z = (float)luaL_checknumber(L, 3);
            float w = (float)luaL_checknumber(L, 4);
            val.Set(x, y, z, w);
        }
        else
        {
            luaL_error(L, "invalid constructor parameters");
            return 0;
        }

        g_pScriptManager->PushNewUserData(L, MetaTableReference(), ScriptUserDataTag_None, sizeof(float4), &val);
        return 1;
    }
    static int __index(lua_State *L)
    {
        // should have an argument, always
        const Quaternion *self = GetPtr(L, 1);

        // check the methods table first
        lua_pushvalue(L, 2);
        lua_gettable(L, lua_upvalueindex(1));
        if (lua_isfunction(L, -1))
            return 1;
        else
            lua_pop(L, 1);

        // getting by-string?
        if (lua_isstring(L, 2))
        {
            // number of components?
            const char *compStr = lua_tostring(L, 2);
            size_t compLen = Y_strlen(compStr);
            if (compLen == 1)
            {
                // access single component
                switch (compStr[0])
                {
                case 'x':
                    lua_pushnumber(L, (lua_Number)self->x);
                    return 1;

                case 'y':
                    lua_pushnumber(L, (lua_Number)self->y);
                    return 1;

                case 'z':
                    lua_pushnumber(L, (lua_Number)self->z);
                    return 1;

                case 'w':
                    lua_pushnumber(L, (lua_Number)self->w);
                    return 1;
                }

                luaL_error(L, "invalid component: %s", compStr);
                return 0;
            }
            else
            {
                // construct new vector using swizzle
                luaL_error(L, "invalid swizzle: %s", compStr);
                return 0;
            }
        }
        else if (lua_isinteger(L, 2))
        {
            // access component
            int component = (int)lua_tointeger(L, 2);
            if (component < 0 || component > 3)
            {
                luaL_error(L, "invalid component index: %d", component);
                return 0;
            }

            lua_pushnumber(L, (&self->x)[component]);
            return 1;
        }
        else
        {
            ScriptManager::GenerateTypeError(L, 2, "string or integer");
            return 0;
        }
    }
    static int __newindex(lua_State *L)
    {
        // should have an argument, always
        Quaternion *self = GetPtr(L, 1);
        float val = (float)luaL_checknumber(L, 3);

        // getting by-string?
        if (lua_isstring(L, 2))
        {
            // number of components?
            const char *compStr = lua_tostring(L, 2);
            size_t compLen = Y_strlen(compStr);
            if (compLen == 1)
            {
                // access single component
                switch (compStr[0])
                {
                case 'x':
                    self->x = val;
                    return 0;

                case 'y':
                    self->y = val;
                    return 0;

                case 'z':
                    self->z = val;
                    return 0;

                case 'w':
                    self->w = val;
                    return 0;
                }

                luaL_error(L, "invalid component: %s", compStr);
                return 0;
            }
            else
            {
                // construct new vector using swizzle
                luaL_error(L, "vector sets can only be done with a single component");
                return 0;
            }
        }
        else if (lua_isinteger(L, 2))
        {
            // access component
            int component = (int)lua_tointeger(L, 2);
            if (component < 0 || component > 3)
            {
                luaL_error(L, "invalid component index: %d", component);
                return 0;
            }

            (&self->x)[component] = val;
            return 0;
        }
        else
        {
            ScriptManager::GenerateTypeError(L, 2, "string or integer");
            return 0;
        }
    }

    static inline const SCRIPT_FUNCTION_TABLE_ENTRY *MetaMethodFunctions()
    {
        static const SCRIPT_FUNCTION_TABLE_ENTRY reg[] =
        {
            { "__eq", __eq },
            { "__mul", __mul },
            { "__unm", __unm },
            { "__index", __index },
            { "__newindex", __newindex },
            { nullptr, nullptr }
        };

        return reg;
    }

    static inline const SCRIPT_FUNCTION_TABLE_ENTRY *MethodFunctions()
    {
        static const SCRIPT_FUNCTION_TABLE_ENTRY reg[] =
        {
            { "Set", Set },
            { "SetIdentity", SetIdentity },
            { "Length", Length },
            { "Dot", Dot },
            { "Normalize", Normalize },
            { "NormalizeEst", NormalizeEst },
            { "NormalizeInPlace", NormalizeInPlace },
            { "NormalizeEstInPlace", NormalizeEstInPlace },
            { nullptr, nullptr }
        };

        return reg;
    }
};

template<typename T>
static void RegisterPrimitiveType(lua_State *L)
{
    // use script manager function
    ScriptReferenceType metaTableReference = g_pScriptManager->DefineUserDataType(ScriptComplexType<T>::Name(), ScriptComplexType<T>::MethodFunctions(), ScriptComplexType<T>::MetaMethodFunctions(), ScriptComplexType<T>::Constructor, nullptr);
    DebugAssert(metaTableReference != INVALID_SCRIPT_REFERENCE);

    // set in primitive type
    ScriptComplexType<T>::MetaTableReference() = metaTableReference;
}

template<typename T>
static void RegisterIndexAccessorsForPrimitiveType(lua_State *L)
{
    luaBackupStack(L);

    // get the metatable for T
    lua_rawgeti(L, LUA_REGISTRYINDEX, ScriptComplexType<T>::MetaTableReference());

    // set __newindex to the appropriate value
    lua_pushcclosure(L, ScriptComplexType<T>::__newindex, 0);
    lua_setfield(L, -2, "__newindex");

    // get method table, set it as an upvalue to the __index function
    lua_getfield(L, -1, "__index");
    lua_pushcclosure(L, ScriptComplexType<T>::__index, 1);
    lua_setfield(L, -2, "__index");

    // drop the metatable
    lua_pop(L, 1);
    luaVerifyStack(L, 0);
}

template<typename T>
static void UnregisterPrimitiveType(lua_State *L)
{
    int &metaTableReference = ScriptComplexType<T>::MetaTableReference();
    if (metaTableReference == INVALID_SCRIPT_REFERENCE)
        return;

    luaL_unref(L, LUA_REGISTRYINDEX, metaTableReference);
    metaTableReference = INVALID_SCRIPT_REFERENCE;
}

void ScriptManager::RegisterPrimitiveTypes()
{
    RegisterPrimitiveType<float2>(m_state);
    RegisterIndexAccessorsForPrimitiveType<float2>(m_state);
    RegisterPrimitiveType<float3>(m_state);
    RegisterIndexAccessorsForPrimitiveType<float3>(m_state);
    RegisterPrimitiveType<float4>(m_state);
    RegisterIndexAccessorsForPrimitiveType<float4>(m_state);
    RegisterPrimitiveType<Quaternion>(m_state);
}

void ScriptManager::UnregisterPrimitiveTypes()
{
    UnregisterPrimitiveType<float2>(m_state);
    UnregisterPrimitiveType<float3>(m_state);
    UnregisterPrimitiveType<float4>(m_state);
    UnregisterPrimitiveType<Quaternion>(m_state);
}

// Redirecting ScriptArg to ScriptComplexType
template<> float2 ScriptArg<float2>::Get(lua_State *L, int arg) { return ScriptComplexType<float2>::Get(L, arg); }
template<> void ScriptArg<float2>::Push(lua_State *L, const float2 &arg) { ScriptComplexType<float2>::Push(L, arg); }
template<> float3 ScriptArg<float3>::Get(lua_State *L, int arg) { return ScriptComplexType<float3>::Get(L, arg); }
template<> void ScriptArg<float3>::Push(lua_State *L, const float3 &arg) { ScriptComplexType<float3>::Push(L, arg); }
template<> float4 ScriptArg<float4>::Get(lua_State *L, int arg) { return ScriptComplexType<float4>::Get(L, arg); }
template<> void ScriptArg<float4>::Push(lua_State *L, const float4 &arg) { ScriptComplexType<float4>::Push(L, arg); }
template<> Quaternion ScriptArg<Quaternion>::Get(lua_State *L, int arg) { return ScriptComplexType<Quaternion>::Get(L, arg); }
template<> void ScriptArg<Quaternion>::Push(lua_State *L, const Quaternion &arg) { ScriptComplexType<Quaternion>::Push(L, arg); }
