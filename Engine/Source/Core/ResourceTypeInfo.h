#pragma once
#include "Core/ObjectTypeInfo.h"

class ResourceTypeInfo : public ObjectTypeInfo
{
public:
    ResourceTypeInfo(const char *TypeName, const ObjectTypeInfo *pParentTypeInfo, const PROPERTY_DECLARATION *pPropertyDeclarations, ObjectFactory *pFactory);
    virtual ~ResourceTypeInfo();
};

// Macros
#define DECLARE_RESOURCE_TYPE_INFO(Type, ParentType) \
    private: \
    static ResourceTypeInfo s_TypeInfo; \
    static const PROPERTY_DECLARATION *StaticPropertyMap() { return nullptr; } \
    public: \
    typedef Type ThisClass; \
    typedef ParentType BaseClass; \
    static const ResourceTypeInfo *StaticTypeInfo() { return &s_TypeInfo; } \
    static ResourceTypeInfo *StaticMutableTypeInfo() { return &s_TypeInfo; }

#define DECLARE_RESOURCE_GENERIC_FACTORY(Type) DECLARE_OBJECT_GENERIC_FACTORY(Type)

#define DECLARE_RESOURCE_NO_FACTORY(Type) DECLARE_OBJECT_NO_FACTORY(Type)

#define DEFINE_RESOURCE_TYPE_INFO(Type) \
    ResourceTypeInfo Type::s_TypeInfo(#Type, Type::BaseClass::StaticTypeInfo(), Type::StaticPropertyMap(), Type::StaticFactory())

#define DEFINE_RESOURCE_GENERIC_FACTORY(Type) DEFINE_OBJECT_GENERIC_FACTORY(Type)

#define BEGIN_RESOURCE_PROPERTIES(Type) \
    const PROPERTY_DECLARATION Type::s_propertyDeclarations[] = {

#define END_RESOURCE_PROPERTIES() \
        PROPERTY_TABLE_MEMBER(NULL, PROPERTY_TYPE_COUNT, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL) \
    };
