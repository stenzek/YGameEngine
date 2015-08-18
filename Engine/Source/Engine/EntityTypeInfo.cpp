#include "Engine/PrecompiledHeader.h"
#include "Engine/EntityTypeInfo.h"
#include "Engine/Entity.h"

EntityTypeInfo::EntityTypeInfo(const char *TypeName, const ObjectTypeInfo *pParentTypeInfo, const PROPERTY_DECLARATION *pPropertyDeclarations, ObjectFactory *pFactory, uint32 scriptFlags, const SCRIPT_FUNCTION_TABLE_ENTRY *pScriptFunctions)
    : ScriptObjectTypeInfo(TypeName, pParentTypeInfo, pPropertyDeclarations, pFactory, scriptFlags, pScriptFunctions)
{

}

EntityTypeInfo::~EntityTypeInfo()
{

}

void EntityTypeInfo::RegisterType()
{
    if (m_iTypeIndex != INVALID_TYPE_INDEX)
        return;

    // register object stuff
    ObjectTypeInfo::RegisterType();
}

void EntityTypeInfo::UnregisterType()
{
    if (m_iTypeIndex != INVALID_TYPE_INDEX)
    {
        ObjectTypeInfo::UnregisterType();
    }
}

