#include "Core/PrecompiledHeader.h"
#include "Core/ResourceTypeInfo.h"

ResourceTypeInfo::ResourceTypeInfo(const char *TypeName, const ObjectTypeInfo *pParentTypeInfo, const PROPERTY_DECLARATION *pPropertyDeclarations, ObjectFactory *pFactory)
    : ObjectTypeInfo(TypeName, pParentTypeInfo, pPropertyDeclarations, pFactory)
{

}

ResourceTypeInfo::~ResourceTypeInfo()
{

}

