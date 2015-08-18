#include "Engine/PrecompiledHeader.h"
#include "Engine/ScriptObject.h"
#include "Engine/ScriptManager.h"
Log_SetChannel(ScriptObject);

DEFINE_UNEXPOSED_SCRIPT_OBJECT_TYPEINFO(ScriptObject, 0);
BEGIN_OBJECT_PROPERTY_MAP(ScriptObject)
END_OBJECT_PROPERTY_MAP()

ScriptObject::ScriptObject(const ScriptObjectTypeInfo *pTypeInfo /*= &s_typeInfo*/)
    : BaseClass(pTypeInfo),
      m_persistentScriptObjectReference(INVALID_SCRIPT_REFERENCE)
{

}

ScriptObject::~ScriptObject()
{
    if (m_persistentScriptObjectReference != INVALID_SCRIPT_REFERENCE)
    {
        g_pScriptManager->SetObjectReferencePointer(m_persistentScriptObjectReference, nullptr);
        g_pScriptManager->ReleaseReference(m_persistentScriptObjectReference);
    }
}

ScriptReferenceType ScriptObject::GetPersistentScriptObjectReference() const
{
    if (m_persistentScriptObjectReference == INVALID_SCRIPT_REFERENCE)
    {
        const_cast<ScriptObject *>(this)->CreateScriptObject();
        return m_persistentScriptObjectReference;
    }

    return m_persistentScriptObjectReference;
}

bool ScriptObject::CreateScriptObject()
{
    DebugAssert(m_persistentScriptObjectReference == INVALID_SCRIPT_REFERENCE);

    // unexposed classes can't be created
    if (GetScriptObjectTypeInfo()->GetMetaTableReference() == INVALID_SCRIPT_REFERENCE)
        return false;

    // this one will always succeed, it's rather simple
    m_persistentScriptObjectReference = g_pScriptManager->AllocateAndReferenceObject(GetScriptObjectTypeInfo()->GetMetaTableReference(), ScriptUserDataTag_ScriptObject, this);
    return (m_persistentScriptObjectReference != INVALID_SCRIPT_REFERENCE);
}

bool ScriptObject::CreateScriptObject(const byte *pScript, uint32 scriptLength, const char *source /* = "text chunk" */)
{
    // create the script object first
    if (!CreateScriptObject())
        return false;

    // run the object script
    ScriptCallResult res = g_pScriptManager->RunObjectScript(m_persistentScriptObjectReference, pScript, scriptLength, source);
    if (res != ScriptCallResult_Success)
    {
        Log_ErrorPrintf("ScriptObject::CreateScriptObject: Running of environment script '%s' failed with %s.", source, NameTable_GetNameString(NameTables::ScriptCallResults, res));
        return false;
    }

    // ran ok
    return true;
}

bool ScriptObject::CreateScriptObject(ScriptReferenceType environmentReference)
{
    // create the script object first
    if (!CreateScriptObject())
        return false;

    // todo: copy functions from environment reference to this object
    return false;
}

