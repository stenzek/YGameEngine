#pragma once
#include "Core/ObjectTypeInfo.h"

//
// Object
// Object class does its own reference counting and lifetime management, as the factories can do pooling.
//
class Object
{
    // OBJECT TYPE STUFF
private:
    static ObjectTypeInfo s_typeInfo;
public:
    typedef Object ThisClass;
    static const ObjectTypeInfo *StaticTypeInfo() { return &s_typeInfo; }
    static ObjectTypeInfo *StaticMutableTypeInfo() { return &s_typeInfo; }
    static const PROPERTY_DECLARATION *StaticPropertyMap() { return nullptr; }
    static ObjectFactory *StaticFactory() { return nullptr; }
    // END OBJECT TYPE STUFF

public:
    Object(const ObjectTypeInfo *pObjectTypeInfo = &s_typeInfo);
    virtual ~Object();

    // Retrieves the type information for this object.
    const ObjectTypeInfo *GetObjectTypeInfo() const { return m_pObjectTypeInfo; }

    // Cast from one object type to another, unchecked.
    template<class T> const T *Cast() const { DebugAssert(m_pObjectTypeInfo->IsDerived(T::StaticTypeInfo())); return static_cast<const T *>(this); }
    template<class T> T *Cast() { DebugAssert(m_pObjectTypeInfo->IsDerived(T::StaticTypeInfo())); return static_cast<T *>(this); }

    // Cast from one object type to another, checked.
    template<class T> const T *SafeCast() const { return (m_pObjectTypeInfo->IsDerived(T::StaticTypeInfo())) ? static_cast<const T *>(this) : NULL; }
    template<class T> T *SafeCast() { return (m_pObjectTypeInfo->IsDerived(T::StaticTypeInfo())) ? static_cast<T *>(this) : NULL; }

    // Test if one object type is derived from another.
    template<class T> bool IsDerived() const { return (m_pObjectTypeInfo->IsDerived(T::StaticTypeInfo())); }
    bool IsDerived(const ObjectTypeInfo *pTypeInfo) const { return (m_pObjectTypeInfo->IsDerived(pTypeInfo)); }

    // Life management.
    void AddRef() const;
    uint32 Release() const;

protected:
    // Type info pointer. Set by subclasses.
    const ObjectTypeInfo *m_pObjectTypeInfo;

    // Life management. The user *must* use the AddRef/Release methods to modify these,
    // hence why they are private.
    mutable Y_ATOMIC_DECL uint32 m_iReferenceCount;
    mutable uint32 m_iReferenceCountValid;

    // Disable copy constructor and assignment operators, since that will copy the reference count value.
    DeclareNonCopyable(Object);
};

//
// GenericObjectFactory<T>
//
template<class T>
struct GenericObjectFactory : public ObjectFactory
{
    Object *CreateObject() { return new T(); }
    void DeleteObject(Object *pObject) { delete pObject; }
};

#define DECLARE_OBJECT_GENERIC_FACTORY(Type) \
    private: \
    static GenericObjectFactory<Type> s_GenericFactory; \
    public: \
    static ObjectFactory *StaticFactory() { return &s_GenericFactory; }

#define DEFINE_OBJECT_GENERIC_FACTORY(Type) \
    GenericObjectFactory<Type> Type::s_GenericFactory = GenericObjectFactory<Type>();
