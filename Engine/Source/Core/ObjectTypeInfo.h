#pragma once
#include "Core/Common.h"
#include "Core/TypeRegistry.h"
#include "Core/Property.h"

// Constants.
#define INVALID_OBJECT_TYPE_INDEX (0xFFFFFFFF)

// Forward declare the factory type.
class Object;
struct ObjectFactory;

//
// ObjectTypeInfo
//
class ObjectTypeInfo
{
public:
    // constructors
    ObjectTypeInfo(const char *TypeName, const ObjectTypeInfo *pParentTypeInfo, const PROPERTY_DECLARATION *pPropertyDeclarations, ObjectFactory *pFactory);
    virtual ~ObjectTypeInfo();

    // accessors
    const uint32 GetTypeIndex() const { return m_iTypeIndex; }
    const uint32 GetInheritanceDepth() const { return m_iInheritanceDepth; }
    const char *GetTypeName() const { return m_strTypeName; }
    const ObjectTypeInfo *GetParentType() const { return m_pParentType; }
    ObjectFactory *GetFactory() const { return m_pFactory; }

    // type information
    // currently only does single inheritance
    bool IsDerived(const ObjectTypeInfo *pTypeInfo) const;

    // properties
    const PROPERTY_DECLARATION *GetPropertyDeclarationByName(const char *PropertyName) const;
    const PROPERTY_DECLARATION *GetPropertyDeclarationByIndex(uint32 Index) const { DebugAssert(Index < m_nPropertyDeclarations); return m_ppPropertyDeclarations[Index]; }
    uint32 GetPropertyCount() const { return m_nPropertyDeclarations; }

    // only called once.
    virtual void RegisterType();
    virtual void UnregisterType();

protected:
    uint32 m_iTypeIndex;
    uint32 m_iInheritanceDepth;
    const char *m_strTypeName;
    const ObjectTypeInfo *m_pParentType;
    ObjectFactory *m_pFactory;

    // properties
    const PROPERTY_DECLARATION *m_pSourcePropertyDeclarations;
    const PROPERTY_DECLARATION **m_ppPropertyDeclarations;
    uint32 m_nPropertyDeclarations;

    // TYPE REGISTRY
public:
    typedef TypeRegistry<ObjectTypeInfo> RegistryType;
    static RegistryType &GetRegistry();
    // END TYPE REGISTRY
};

//
// ObjectFactory
//
struct ObjectFactory
{
    virtual Object *CreateObject() = 0;
    virtual void DeleteObject(Object *pObject) = 0;
};

// Macros
#define DECLARE_OBJECT_TYPE_INFO(Type, ParentType) \
    private: \
    static ObjectTypeInfo s_typeInfo; \
    public: \
    typedef Type ThisClass; \
    typedef ParentType BaseClass; \
    static const ObjectTypeInfo *StaticTypeInfo() { return &s_typeInfo; } \
    static ObjectTypeInfo *StaticMutableTypeInfo() { return &s_typeInfo; }

#define DECLARE_OBJECT_PROPERTY_MAP(Type) \
    private: \
    static const PROPERTY_DECLARATION s_propertyDeclarations[]; \
    static const PROPERTY_DECLARATION *StaticPropertyMap() { return s_propertyDeclarations; }

#define DECLARE_OBJECT_NO_PROPERTIES(Type) \
    private: \
    static const PROPERTY_DECLARATION *StaticPropertyMap() { return nullptr; }

#define DEFINE_OBJECT_TYPE_INFO(Type) \
    ObjectTypeInfo Type::s_typeInfo(#Type, Type::BaseClass::StaticTypeInfo(), Type::StaticPropertyMap(), Type::StaticFactory())

#define DECLARE_OBJECT_NO_FACTORY(Type) \
    public: \
    static ObjectFactory *StaticFactory() { return nullptr; }

#define BEGIN_OBJECT_PROPERTY_MAP(Type) \
    const PROPERTY_DECLARATION Type::s_propertyDeclarations[] = {

#define END_OBJECT_PROPERTY_MAP() \
        PROPERTY_TABLE_MEMBER(NULL, PROPERTY_TYPE_COUNT, 0, NULL, NULL, NULL, NULL, NULL, NULL) \
    };

#define OBJECT_TYPEINFO(Type) Type::StaticTypeInfo()
#define OBJECT_TYPEINFO_PTR(Ptr) Ptr->StaticTypeInfo()

#define OBJECT_MUTABLE_TYPEINFO(Type) Type::StaticMutableTypeInfo()
#define OBJECT_MUTABLE_TYPEINFO_PTR(Type) Type->StaticMutableTypeInfo()
