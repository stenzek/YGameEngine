#include "Core/PrecompiledHeader.h"
#include "Core/Resource.h"

DEFINE_RESOURCE_TYPE_INFO(Resource);

Resource::Resource(const ResourceTypeInfo *pResourceTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pResourceTypeInfo)
{

}

Resource::~Resource()
{

}

static inline bool ResourceNameCharacterIsSane(char c, bool stripSlashes)
{
    if (!(c >= 'a' && c <= 'z') && 
        !(c >= 'A' && c <= 'Z') &&
        !(c >= '0' && c <= '9') &&
        c != '_' && c != '-')
    {
        if (!stripSlashes && c == '/')
            return true;

        return false;
    }

    return true;
}

void Resource::SanitizeResourceName(String &resourceName, bool stripSlashes /* = true */)
{
    for (uint32 i = 0; i < resourceName.GetLength(); i++)
    {
        if (!ResourceNameCharacterIsSane(resourceName[i], stripSlashes))
            resourceName[i] = '_';
    }
}
