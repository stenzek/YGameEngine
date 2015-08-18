#pragma once
#include "Engine/ScriptObjectTypeInfo.h"

class EntityTypeInfo : public ScriptObjectTypeInfo
{
public:
    EntityTypeInfo(const char *TypeName, const ObjectTypeInfo *pParentTypeInfo, const PROPERTY_DECLARATION *pPropertyDeclarations, ObjectFactory *pFactory, uint32 scriptFlags, const SCRIPT_FUNCTION_TABLE_ENTRY *pScriptFunctions);
    virtual ~EntityTypeInfo();

    // type registration
    virtual void RegisterType() override;
    virtual void UnregisterType() override;
};

// Macros
#define DECLARE_ENTITY_TYPEINFO(Type, ParentType) \
            private: \
            static EntityTypeInfo s_typeInfo; \
            static const PROPERTY_DECLARATION s_propertyDeclarations[]; \
            static const SCRIPT_FUNCTION_TABLE_ENTRY *__TypeScriptFunctionTable(); \
            public: \
            typedef Type ThisClass; \
            typedef ParentType BaseClass; \
            static const EntityTypeInfo *StaticTypeInfo() { return &s_typeInfo; } \
            static EntityTypeInfo *StaticMutableTypeInfo() { return &s_typeInfo; }

#define DECLARE_ENTITY_GENERIC_FACTORY(Type) DECLARE_OBJECT_GENERIC_FACTORY(Type)

#define DECLARE_ENTITY_NO_FACTORY(Type) DECLARE_OBJECT_NO_FACTORY(Type)

#define DEFINE_ENTITY_TYPEINFO(Type, ScriptFlags) \
    EntityTypeInfo Type::s_typeInfo = EntityTypeInfo(#Type, Type::BaseClass::StaticTypeInfo(), Type::s_propertyDeclarations, Type::StaticFactory(), ScriptFlags, Type::__TypeScriptFunctionTable()); \
    DEFINE_SCRIPT_ARG_WRAPPERS(Type);

#define DEFINE_ENTITY_TYPEINFO_NOSCRIPT(Type) \
    EntityTypeInfo Type::s_typeInfo = EntityTypeInfo(#Type, Type::BaseClass::StaticTypeInfo(), Type::s_propertyDeclarations, Type::StaticFactory(), SCRIPT_OBJECT_FLAG_UNEXPOSED, nullptr); \
    const SCRIPT_FUNCTION_TABLE_ENTRY *Type::__TypeScriptFunctionTable() { return nullptr; }

#define DEFINE_ENTITY_GENERIC_FACTORY(Type) DEFINE_OBJECT_GENERIC_FACTORY(Type)

#define BEGIN_ENTITY_PROPERTIES(Type) \
    const PROPERTY_DECLARATION Type::s_propertyDeclarations[] = {

#define END_ENTITY_PROPERTIES() \
        PROPERTY_TABLE_MEMBER(NULL, PROPERTY_TYPE_COUNT, 0, NULL, NULL, NULL, NULL, NULL, NULL) \
    };

#define BEGIN_ENTITY_SCRIPT_FUNCTIONS(Type) BEGIN_SCRIPT_OBJECT_FUNCTIONS(Type)
#define DEFINE_ENTITY_SCRIPT_FUNCTION(FunctionName, FunctionPointer) DEFINE_SCRIPT_OBJECT_FUNCTION(FunctionName, FunctionPointer)
#define END_ENTITY_SCRIPT_FUNCTIONS() END_SCRIPT_OBJECT_FUNCTIONS()

#define ENTITY_TYPEINFO(Type) OBJECT_TYPEINFO(Type)
#define ENTITY_TYPEINFO_PTR(Ptr) OBJECT_TYPEINFO_PTR(Type)

#define ENTITY_MUTABLE_TYPEINFO(Type) OBJECT_MUTABLE_TYPEINFO(Type)
#define ENTITY_MUTABLE_TYPEINFO_PTR(Type) OBJECT_MUTABLE_TYPEINFO_PTR(Type)
