#pragma once
#include "Engine/Common.h"
#include "Engine/ScriptManager.h"
#include "Engine/ScriptProxyFunctions.h"

enum SCRIPT_OBJECT_FLAG
{
    SCRIPT_OBJECT_FLAG_UNEXPOSED    = (1 << 0),         // Don't expose the object to script.
    SCRIPT_OBJECT_FLAG_TABLED       = (1 << 1),         // Declare this type as a tabled type, allowing custom script properties to be set.
};

class ScriptObjectTypeInfo : public ObjectTypeInfo
{
public:
    ScriptObjectTypeInfo(const char *TypeName, const ObjectTypeInfo *pParentTypeInfo, const PROPERTY_DECLARATION *pPropertyDeclarations, ObjectFactory *pFactory, uint32 scriptFlags, const SCRIPT_FUNCTION_TABLE_ENTRY *pScriptFunctions);
    virtual ~ScriptObjectTypeInfo();

    const uint32 GetScriptFlags() const { return m_scriptFlags; }
    const ScriptReferenceType GetMetaTableReference() const { return m_metaTableReference; }

    // script type registration
    bool RegisterScriptType();
    void UnregisterScriptType();

    // type registration
    virtual void RegisterType() override;
    virtual void UnregisterType() override;

    // register all types in script engine
    static void RegisterAllScriptTypes();
    static void UnregisterAllScriptTypes();

//     // pushes object from arguments
//     static void PushObject(lua_State *L, const ScriptObject *pObject);
// 
//     // gets typed object from arguments
//     template<typename T>
//     static T *CheckObject(lua_State *L, int arg)
//     {
//         ScriptObject *pObject = CheckObject(L, T::StaticTypeInfo(), arg);
//         return (pObject != nullptr) ? pObject->Cast<T>() : nullptr;
//     }
// 
//     // gets the object from arguments
//     static ScriptObject *CheckObject(lua_State *L, const ScriptObjectTypeInfo *pTypeInfo, int arg);

private:
    const SCRIPT_FUNCTION_TABLE_ENTRY *m_pScriptFunctions;
    uint32 m_scriptFlags;

    uint32 m_nScriptFunctions;
    ScriptReferenceType m_metaTableReference;
};

// Macros
#define DECLARE_SCRIPT_OBJECT_TYPEINFO(Type, ParentType) \
            private: \
            static ScriptObjectTypeInfo s_typeInfo; \
            static const PROPERTY_DECLARATION s_propertyDeclarations[]; \
            static const SCRIPT_FUNCTION_TABLE_ENTRY *__TypeScriptFunctionTable(); \
            public: \
            typedef Type ThisClass; \
            typedef ParentType BaseClass; \
            static const ScriptObjectTypeInfo *StaticTypeInfo() { return &s_typeInfo; } \
            static ScriptObjectTypeInfo *StaticMutableTypeInfo() { return &s_typeInfo; }

/*
// #define DEFINE_SCRIPT_ARG_WRAPPERS(Type) \
//     template<> struct ScriptArg<Type *> { \
//         static Type *Get(lua_State *L, int arg) { \
//             return (lua_isnil(L, arg)) ? nullptr : (Type *)ScriptManager::CheckObjectType(L, arg, OBJECT_TYPEINFO(Type)->GetMetaTableReference()); \
//         } \
//         static void Push(lua_State *L, const Type *&arg) { \
//             if (arg->HasPersistentScriptObjectReference()) { \
//                 lua_rawgeti(L, LUA_REGISTRYINDEX, arg->GetPersistentScriptObjectReference()); \
//                                     } else { \
//                 ScriptManager::PushNewObjectReference(L, OBJECT_TYPEINFO(Type)->GetMetaTableReference(), arg); \
//                                     } \
//         } \
//     };
*/

#define DEFINE_SCRIPT_ARG_WRAPPERS(Type) \
    template<> const Type *ScriptArg<const Type *>::Get(lua_State *L, int arg) { \
        const ScriptObject *pScriptObject = (lua_isnil(L, arg)) ? nullptr : reinterpret_cast<const ScriptObject *>(ScriptManager::CheckObjectTypeByTag(L, ScriptUserDataTag_ScriptObject, arg)); \
        return (pScriptObject != nullptr) ? pScriptObject->SafeCast<Type>() : nullptr; \
    } \
    template<> Type *ScriptArg<Type *>::Get(lua_State *L, int arg) { \
        ScriptObject *pScriptObject = (lua_isnil(L, arg)) ? nullptr : reinterpret_cast<ScriptObject *>(ScriptManager::CheckObjectTypeByTag(L, ScriptUserDataTag_ScriptObject, arg)); \
        return (pScriptObject != nullptr) ? pScriptObject->SafeCast<Type>() : nullptr; \
    } \
    template<> void ScriptArg<const Type *>::Push(lua_State *L, const Type *const &arg) { \
        ScriptReferenceType argRef = arg->GetPersistentScriptObjectReference(); \
        if (argRef != INVALID_SCRIPT_REFERENCE) \
            lua_rawgeti(L, LUA_REGISTRYINDEX, arg->GetPersistentScriptObjectReference()); \
        else \
            lua_pushnil(L); \
    } \
    template<> void ScriptArg<Type *>::Push(lua_State *L, Type *const &arg) { \
        ScriptReferenceType argRef = arg->GetPersistentScriptObjectReference(); \
        if (argRef != INVALID_SCRIPT_REFERENCE) \
            lua_rawgeti(L, LUA_REGISTRYINDEX, arg->GetPersistentScriptObjectReference()); \
        else \
            lua_pushnil(L); \
    }

#define DEFINE_SCRIPT_OBJECT_TYPEINFO(Type, ScriptFlags) \
    ScriptObjectTypeInfo Type::s_typeInfo = ScriptObjectTypeInfo(#Type, Type::BaseClass::StaticTypeInfo(), Type::s_propertyDeclarations, Type::StaticFactory(), ScriptFlags, Type::__TypeScriptFunctionTable()); \
    DEFINE_SCRIPT_ARG_WRAPPERS(type)

#define DEFINE_UNEXPOSED_SCRIPT_OBJECT_TYPEINFO(Type, ScriptFlags) \
    ScriptObjectTypeInfo Type::s_typeInfo = ScriptObjectTypeInfo(#Type, Type::BaseClass::StaticTypeInfo(), Type::s_propertyDeclarations, Type::StaticFactory(), ScriptFlags | SCRIPT_OBJECT_FLAG_UNEXPOSED, nullptr); \
    const SCRIPT_FUNCTION_TABLE_ENTRY *Type::__TypeScriptFunctionTable() { return nullptr; }

#define BEGIN_SCRIPT_OBJECT_FUNCTIONS(Type) const SCRIPT_FUNCTION_TABLE_ENTRY *Type::__TypeScriptFunctionTable() { static const SCRIPT_FUNCTION_TABLE_ENTRY table[] = {
#define DEFINE_SCRIPT_OBJECT_FUNCTION(FunctionName, FunctionPointer) { FunctionName, FunctionPointer },
#define END_SCRIPT_OBJECT_FUNCTIONS() { nullptr, nullptr } }; return table; }
