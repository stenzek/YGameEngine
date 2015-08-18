#include "Engine/PrecompiledHeader.h"
#include "Engine/ScriptObjectTypeInfo.h"
#include "Engine/ScriptObject.h"
#include "Engine/ScriptManager.h"

ScriptObjectTypeInfo::ScriptObjectTypeInfo(const char *TypeName, const ObjectTypeInfo *pParentTypeInfo, const PROPERTY_DECLARATION *pPropertyDeclarations, ObjectFactory *pFactory, uint32 scriptFlags, const SCRIPT_FUNCTION_TABLE_ENTRY *pScriptFunctions)
    : ObjectTypeInfo(TypeName, pParentTypeInfo, pPropertyDeclarations, pFactory),
      m_pScriptFunctions(pScriptFunctions),
      m_scriptFlags(scriptFlags),
      m_nScriptFunctions(0),
      m_metaTableReference(INVALID_SCRIPT_REFERENCE)
{
    if (pScriptFunctions != nullptr)
    {
        // count functions
        for (m_nScriptFunctions = 0; m_pScriptFunctions[m_nScriptFunctions].FunctionPointer != nullptr; m_nScriptFunctions++)
        {
        }
    }
}

ScriptObjectTypeInfo::~ScriptObjectTypeInfo()
{

}

bool ScriptObjectTypeInfo::RegisterScriptType()
{
    DebugAssert(m_metaTableReference == INVALID_SCRIPT_REFERENCE);

    // find script flags, and the number of functions
    uint32 scriptFlags = m_scriptFlags;
    uint32 functionCount = 0;
    for (const ScriptObjectTypeInfo *pTypeInfo = this;;)
    {
        // add flags
        if (pTypeInfo->m_scriptFlags & SCRIPT_OBJECT_FLAG_TABLED)
            scriptFlags |= SCRIPT_OBJECT_FLAG_TABLED;

        // add functions
        functionCount += pTypeInfo->m_nScriptFunctions;

        // move to parent
        if (pTypeInfo->m_pParentType != nullptr && pTypeInfo->m_pParentType->IsDerived(OBJECT_TYPEINFO(ScriptObject)))
            pTypeInfo = static_cast<const ScriptObjectTypeInfo *>(pTypeInfo->m_pParentType);
        else
            break;
    }

    // add functions once again
    SCRIPT_FUNCTION_TABLE_ENTRY *pFunctions = (functionCount > 0) ? (SCRIPT_FUNCTION_TABLE_ENTRY *)alloca(sizeof(SCRIPT_FUNCTION_TABLE_ENTRY) * (functionCount + 1)) : nullptr;
    uint32 nFunctions = 0;
    for (const ScriptObjectTypeInfo *pTypeInfo = this;;)
    {
        for (uint32 i = 0; i < pTypeInfo->m_nScriptFunctions; i++)
        {
            DebugAssert(nFunctions < functionCount);
            pFunctions[nFunctions].FunctionName = pTypeInfo->m_pScriptFunctions[i].FunctionName;
            pFunctions[nFunctions].FunctionPointer = pTypeInfo->m_pScriptFunctions[i].FunctionPointer;
            nFunctions++;
        }

        // move to parent
        if (pTypeInfo->m_pParentType != nullptr && pTypeInfo->m_pParentType->IsDerived(OBJECT_TYPEINFO(ScriptObject)))
            pTypeInfo = static_cast<const ScriptObjectTypeInfo *>(pTypeInfo->m_pParentType);
        else
            break;
    }

    // add null entry
    if (nFunctions > 0)
    {
        pFunctions[nFunctions].FunctionName = nullptr;
        pFunctions[nFunctions].FunctionPointer = nullptr;
    }

    // create the userdata type
    if (scriptFlags & SCRIPT_OBJECT_FLAG_TABLED)
        m_metaTableReference = g_pScriptManager->DefineTabledUserDataType(GetTypeName(), pFunctions, nullptr, nullptr, nullptr);
    else
        m_metaTableReference = g_pScriptManager->DefineTabledUserDataType(GetTypeName(), pFunctions, nullptr, nullptr, nullptr);

    return (m_metaTableReference != INVALID_SCRIPT_REFERENCE);
}

void ScriptObjectTypeInfo::UnregisterScriptType()
{
    if (m_metaTableReference == INVALID_SCRIPT_REFERENCE)
        return;

    // drop the reference to the metatable
    g_pScriptManager->ReleaseReference(m_metaTableReference);
    m_metaTableReference = INVALID_SCRIPT_REFERENCE;
}

void ScriptObjectTypeInfo::RegisterType()
{
    // register object stuff
    ObjectTypeInfo::RegisterType();
}

void ScriptObjectTypeInfo::UnregisterType()
{

}

void ScriptObjectTypeInfo::RegisterAllScriptTypes()
{
    // iterate types, looking for those that inherit ScriptObject
    const RegistryType &registry = GetRegistry();
    for (uint32 typeIndex = 0; typeIndex < registry.GetNumTypes(); typeIndex++)
    {
        const ObjectTypeInfo *pObjectTypeInfo = registry.GetTypeInfoByIndex(typeIndex);
        if (pObjectTypeInfo == nullptr || !pObjectTypeInfo->IsDerived(OBJECT_TYPEINFO(ScriptObject)))
            continue;

        // ugh, hack
        ScriptObjectTypeInfo *pScriptObjectTypeInfo = const_cast<ScriptObjectTypeInfo *>(static_cast<const ScriptObjectTypeInfo *>(pObjectTypeInfo));
        pScriptObjectTypeInfo->RegisterScriptType();
    }
}

void ScriptObjectTypeInfo::UnregisterAllScriptTypes()
{
    // iterate types, looking for those that inherit ScriptObject
    const RegistryType &registry = GetRegistry();
    for (uint32 typeIndex = 0; typeIndex < registry.GetNumTypes(); typeIndex++)
    {
        const ObjectTypeInfo *pObjectTypeInfo = registry.GetTypeInfoByIndex(typeIndex);
        if (pObjectTypeInfo == nullptr || !pObjectTypeInfo->IsDerived(OBJECT_TYPEINFO(ScriptObject)))
            continue;

        // ugh, hack
        ScriptObjectTypeInfo *pScriptObjectTypeInfo = const_cast<ScriptObjectTypeInfo *>(static_cast<const ScriptObjectTypeInfo *>(pObjectTypeInfo));
        pScriptObjectTypeInfo->UnregisterScriptType();
    }
}

// void ScriptObjectTypeInfo::PushObject(lua_State *L, const ScriptObject *pObject)
// {
//     ScriptReferenceType argRef = pObject->GetPersistentScriptObjectReference();
//     if (argRef == INVALID_SCRIPT_REFERENCE)
//         lua_rawgeti(L, LUA_REGISTRYINDEX, argRef);
//     else
//         lua_pushnil(L);
// }
// 
// ScriptObject *ScriptObjectTypeInfo::CheckObject(lua_State *L, const ScriptObjectTypeInfo *pTypeInfo, int arg)
// {
//     luaBackupStack(L);
// 
//     // should be a userdata
//     void *pUserData;
//     if (lua_type(L, arg) != LUA_TUSERDATA || (pUserData = lua_touserdata(L, arg)) == nullptr)
//     {
//         ScriptManager::GenerateTypeError(L, arg, pTypeInfo->GetMetaTableReference());
//         return nullptr;
//     }
// 
//     // skip through each class, checking if any of the metatables match
//     const ScriptObjectTypeInfo *pCurrentTypeInfo = pTypeInfo;
//     while (pCurrentTypeInfo)
// 
//     // resolve metatable reference
//     lua_rawgeti(L, LUA_REGISTRYINDEX, metaTableReference);
//     DebugAssert(lua_istable(L, -1));
// 
//     // get the parameter's metatable
//     lua_getmetatable(L, arg);
// 
//     // check they match
//     if (!lua_rawequal(L, -1, -2))
//     {
//         lua_pop(L, 2);
//         GenerateTypeError(L, arg, metaTableReference);
//         return nullptr;
//     }
// 
//     // type is ok, pop metatable + table
//     lua_pop(L, 2);
//     luaVerifyStack(L, 0);
//     return pUserData;
// }

