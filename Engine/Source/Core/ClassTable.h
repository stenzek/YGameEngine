#pragma once
#include "Core/Common.h"
#include "Core/ObjectTypeInfo.h"
#include "Core/Property.h"
#include "Core/PropertyTable.h"

class ByteStream;

class ClassTable
{
public:
    class TypeMapping
    {
        friend class ClassTable;

    public:
        const ObjectTypeInfo *GetTypeInfo() const { return m_pTypeInfo; }
        const PROPERTY_DECLARATION *GetPropertyMapping(uint32 serializedIndex) const { DebugAssert(serializedIndex < m_propertyCount); return m_ppPropertyMapping[serializedIndex]; }
        const uint32 GetPropertyCount() const { return m_propertyCount; }

    private:
        const ObjectTypeInfo *m_pTypeInfo;
        const PROPERTY_DECLARATION **m_ppPropertyMapping;
        uint32 m_propertyCount;
    };
    
public:
    ClassTable();
    ~ClassTable();

    // load a class table
    bool LoadFromStream(ByteStream *pStream, bool abortOnError = false);
    bool LoadFromBuffer(const void *pBuffer, uint32 bufferSize, bool abortOnError = false);
    
    // save a class table
    bool SaveToStream(ByteStream *pStream) const;

    // runtime accessors
    const TypeMapping *GetTypeMapping(uint32 serializedIndex) const { DebugAssert(serializedIndex < m_typeCount); return &m_pTypeMappings[serializedIndex]; }
    const int32 GetTypeMappingForType(const ObjectTypeInfo *pTypeInfo) const;
    const uint32 GetTypeCount() const { return m_typeCount; }

    // add a type to this class table
    uint32 AddType(const ObjectTypeInfo *pTypeInfo);

    // save (serialize) an object
    bool SerializeObject(const Object *pObject, ByteStream *pStream);

    // construct (deserialize) an object
    Object *UnserializeObject(ByteStream *pStream, uint32 typeIndex);

private:
    TypeMapping *m_pTypeMappings;
    uint32 m_typeCount;
};
