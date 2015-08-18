#include "Engine/PrecompiledHeader.h"
#include "Engine/ComponentTypeInfo.h"
#include "Engine/Component.h"

ComponentTypeInfo::ComponentTypeInfo(const char *TypeName, const ObjectTypeInfo *pParentTypeInfo, const PROPERTY_DECLARATION *pPropertyDeclarations, ObjectFactory *pFactory)
    : ObjectTypeInfo(TypeName, pParentTypeInfo, pPropertyDeclarations, pFactory)
{

}

ComponentTypeInfo::~ComponentTypeInfo()
{

}

void ComponentTypeInfo::RegisterType()
{
    if (m_iTypeIndex != INVALID_TYPE_INDEX)
        return;

    ObjectTypeInfo::RegisterType();
}

void ComponentTypeInfo::UnregisterType()
{
    if (m_iTypeIndex != INVALID_TYPE_INDEX)
    {
        ObjectTypeInfo::UnregisterType();
    }
}

