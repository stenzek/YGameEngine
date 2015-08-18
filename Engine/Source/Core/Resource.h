#pragma once
#include "Core/ResourceTypeInfo.h"
#include "Core/Object.h"

class Resource : public Object
{
    DECLARE_RESOURCE_TYPE_INFO(Resource, Object);
    DECLARE_RESOURCE_NO_FACTORY(Resource);

public:
    Resource(const ResourceTypeInfo *pResourceTypeInfo = &s_TypeInfo);
    virtual ~Resource();

    // Override type info accessors.
    const ResourceTypeInfo *GetResourceTypeInfo() const { return static_cast<const ResourceTypeInfo *>(m_pObjectTypeInfo); }

    // Resource name accessor.
    const String &GetName() const { return m_strName; }

    // Helper function to sanitize a resource name.
    static void SanitizeResourceName(String &resourceName, bool stripSlashes = true);

protected:
    // Resource name.
    String m_strName;
};
