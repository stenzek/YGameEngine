#pragma once
#include "Engine/Common.h"
#include "Engine/ScriptObjectTypeInfo.h"

class ScriptObject : public Object
{
    DECLARE_SCRIPT_OBJECT_TYPEINFO(ScriptObject, Object);
    DECLARE_OBJECT_NO_FACTORY(ScriptObject);

public:
    ScriptObject(const ScriptObjectTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~ScriptObject();

    // Script object type info accessor
    const ScriptObjectTypeInfo *GetScriptObjectTypeInfo() const { return static_cast<const ScriptObjectTypeInfo *>(m_pObjectTypeInfo); }

    // Do we have a persistent script object reference?
    bool HasPersistentScriptObjectReference() const { return (m_persistentScriptObjectReference != INVALID_SCRIPT_REFERENCE); }

    // Gets the persistent script object reference, if it doesn't exist, allocates it.
    ScriptReferenceType GetPersistentScriptObjectReference() const;

    // Creates and initializes this object's script environment.
    bool CreateScriptObject();

    // Creates and initializes this object's script environment, running the provided script.
    bool CreateScriptObject(const byte *pScript, uint32 scriptLength, const char *source = "text chunk");

    // Creates and initializes this object's script environment, using an existing environment.
    bool CreateScriptObject(ScriptReferenceType environmentReference);

    // Method invoker
    inline ScriptCallResult CallScriptMethod(const char *methodName)
    {
        return (HasPersistentScriptObjectReference()) ? g_pScriptManager->CallObjectMethod(m_persistentScriptObjectReference, methodName, false) : ScriptCallResult_NoObject;
    }
    inline ScriptCallResult CallScriptMethodResults(const char *methodName)
    {
        return (HasPersistentScriptObjectReference()) ? g_pScriptManager->CallObjectMethod(m_persistentScriptObjectReference, methodName, true) : ScriptCallResult_NoObject;
    }
    template<typename... Args>
    inline ScriptCallResult CallScriptMethod(const char *methodName, Args... args)
    {
        return (HasPersistentScriptObjectReference()) ? g_pScriptManager->CallObjectMethod(m_persistentScriptObjectReference, methodName, args..., false) : ScriptCallResult_NoObject;
    }
    template<typename... Args>
    inline ScriptCallResult CallScriptMethodResults(const char *methodName, Args... args)
    {
        return (HasPersistentScriptObjectReference()) ? g_pScriptManager->CallObjectMethod(m_persistentScriptObjectReference, methodName, args..., true) : ScriptCallResult_NoObject;
    }

    // Threaded method invoker
    inline ScriptCallResult CallThreadedScriptMethod(ScriptThread **ppThread, const char *methodName)
    {
        return (HasPersistentScriptObjectReference()) ? g_pScriptManager->CallThreadedObjectMethod(ppThread, m_persistentScriptObjectReference, methodName, false) : ScriptCallResult_NoObject;
    }
    inline ScriptCallResult CallThreadedScriptMethodResults(ScriptThread **ppThread, const char *methodName)
    {
        return (HasPersistentScriptObjectReference()) ? g_pScriptManager->CallThreadedObjectMethod(ppThread, m_persistentScriptObjectReference, methodName, true) : ScriptCallResult_NoObject;
    }
    template<typename... Args>
    inline ScriptCallResult CallThreadedScriptMethod(ScriptThread **ppThread, const char *methodName, Args... args)
    {
        return (HasPersistentScriptObjectReference()) ? g_pScriptManager->CallThreadedObjectMethod(ppThread, m_persistentScriptObjectReference, methodName, args..., false) : ScriptCallResult_NoObject;
    }
    template<typename... Args>
    inline ScriptCallResult CallThreadedScriptMethodResults(ScriptThread **ppThread, const char *methodName, Args... args)
    {
        return (HasPersistentScriptObjectReference()) ? g_pScriptManager->CallThreadedObjectMethod(ppThread, m_persistentScriptObjectReference, methodName, args..., true) : ScriptCallResult_NoObject;
    }

private:
    ScriptReferenceType m_persistentScriptObjectReference;
};
