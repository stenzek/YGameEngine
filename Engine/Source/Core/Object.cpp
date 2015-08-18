#include "Core/PrecompiledHeader.h"
#include "Core/Object.h"

// Have to define this manually as Object has no parent class.
ObjectTypeInfo Object::s_typeInfo("Object", nullptr, nullptr, nullptr);

// "borrowed" from ReferenceCounted.cpp
static const uint32 ReferenceCountValidValue = 0x11C0FFEE;
static const uint32 ReferenceCountInvalidValue = 0xDEADC0DE;

Object::Object(const ObjectTypeInfo *pObjectTypeInfo /* = &s_typeInfo */)
    : m_pObjectTypeInfo(pObjectTypeInfo),
      m_iReferenceCount(1),
      m_iReferenceCountValid(ReferenceCountValidValue)
{

}

Object::~Object()
{
    // Flag reference count as invalid, that way any methods on bad pointers will fail in debug builds.
    m_iReferenceCountValid = ReferenceCountInvalidValue;
}

void Object::AddRef() const
{
    Y_AtomicIncrement(m_iReferenceCount);
}

uint32 Object::Release() const
{
    DebugAssert(m_iReferenceCount > 0 && m_iReferenceCountValid == ReferenceCountValidValue);
    uint32 NewRefCount = Y_AtomicDecrement(m_iReferenceCount);
    if (NewRefCount == 0)
        delete this;
        //m_pObjectTypeInfo->GetFactory()->DeleteObject(const_cast<Object *>(this));

    return NewRefCount;
}

