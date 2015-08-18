#pragma once
#include "Engine/Common.h"
#include "Core/ObjectTypeInfo.h"

class ComponentTypeInfo : public ObjectTypeInfo
{
public:
    ComponentTypeInfo(const char *TypeName, const ObjectTypeInfo *pParentTypeInfo, const PROPERTY_DECLARATION *pPropertyDeclarations, ObjectFactory *pFactory);
    virtual ~ComponentTypeInfo();

    // only called once.
    virtual void RegisterType() override;
    virtual void UnregisterType() override;
};

#define DECLARE_COMPONENT_TYPEINFO(Type, ParentType) \
            private: \
            static ComponentTypeInfo s_typeInfo; \
            static const PROPERTY_DECLARATION s_propertyDeclarations[]; \
            public: \
            typedef Type ThisClass; \
            typedef ParentType BaseClass; \
            static const ComponentTypeInfo *StaticTypeInfo() { return &s_typeInfo; }  \
            static ComponentTypeInfo *StaticMutableTypeInfo() { return &s_typeInfo; }

#define DECLARE_COMPONENT_GENERIC_FACTORY(Type) DECLARE_OBJECT_GENERIC_FACTORY(Type)
#define DECLARE_COMPONENT_NO_FACTORY(Type) DECLARE_OBJECT_NO_FACTORY(Type)

#define DEFINE_COMPONENT_TYPEINFO(Type) \
    ComponentTypeInfo Type::s_typeInfo = ComponentTypeInfo(#Type, Type::BaseClass::StaticTypeInfo(), Type::s_propertyDeclarations, Type::StaticFactory());

#define DEFINE_COMPONENT_GENERIC_FACTORY(Type) DEFINE_OBJECT_GENERIC_FACTORY(Type)

#define BEGIN_COMPONENT_PROPERTIES(Type) \
    const PROPERTY_DECLARATION Type::s_propertyDeclarations[] = {

#define END_COMPONENT_PROPERTIES() \
        PROPERTY_TABLE_MEMBER(NULL, PROPERTY_TYPE_COUNT, 0, NULL, NULL, NULL, NULL, NULL, NULL) \
    };

#define COMPONENT_TYPEINFO(Type) OBJECT_TYPEINFO(Type)
#define COMPONENT_TYPEINFO_PTR(Ptr) OBJECT_TYPEINFO_PTR(Type)

#define COMPONENT_MUTABLE_TYPEINFO(Type) OBJECT_MUTABLE_TYPEINFO(Type)
#define COMPONENT_MUTABLE_TYPEINFO_PTR(Type) OBJECT_MUTABLE_TYPEINFO_PTR(Type)
