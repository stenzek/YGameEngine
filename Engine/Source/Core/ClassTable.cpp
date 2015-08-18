#include "Core/PrecompiledHeader.h"
#include "Core/ClassTable.h"
#include "Core/ClassTableDataFormat.h"
#include "Core/ObjectTypeInfo.h"
#include "Core/Object.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
Log_SetChannel(ClassTable);

static const char *MISSING_PROPERTY_NAME_STRING = "___MISSING___";

ClassTable::ClassTable()
    : m_pTypeMappings(nullptr),
      m_typeCount(0)
{

}

ClassTable::~ClassTable()
{
    for (uint32 i = 0; i < m_typeCount; i++)
        delete[] m_pTypeMappings[i].m_ppPropertyMapping;

    delete[] m_pTypeMappings;
}

const int32 ClassTable::GetTypeMappingForType(const ObjectTypeInfo *pTypeInfo) const
{
    for (uint32 typeIndex = 0; typeIndex < m_typeCount; typeIndex++)
    {
        if (m_pTypeMappings[typeIndex].m_pTypeInfo == pTypeInfo)
            return (int32)typeIndex;
    }

    return -1;
}


bool ClassTable::LoadFromStream(ByteStream *pStream, bool abortOnError /* = false */)
{
#if 0
    uint32 bufferSize = (uint32)(pStream->GetSize() - pStream->GetPosition());
    byte *tempBuffer = new byte[bufferSize];
    if (!pStream->Read2(tempBuffer, bufferSize))
    {
        delete[] tempBuffer;
        return false;
    }

    bool result = LoadFromBuffer(tempBuffer, bufferSize, abortOnError);
    delete[] tempBuffer;
    return result;
#else
    BinaryReader binaryReader(pStream);
    SmallString extractedString;

    // read/validate header
    DF_CLASS_TABLE_HEADER header;
    if (!binaryReader.SafeReadBytes(&header, sizeof(header)) ||
        header.Magic != DF_CLASS_TABLE_HEADER_MAGIC ||
        header.HeaderSize != sizeof(DF_CLASS_TABLE_HEADER))
    {
        return false;
    }

    // allocate types
    m_pTypeMappings = (TypeMapping *)Y_realloc(m_pTypeMappings, sizeof(TypeMapping) * header.TypeCount);
    Y_memzero(m_pTypeMappings, sizeof(TypeMapping) * header.TypeCount);
    m_typeCount = header.TypeCount;

    // read types
    for (uint32 typeIndex = 0; typeIndex < m_typeCount; typeIndex++)
    {
        // read header
        DF_CLASS_TABLE_TYPE typeHeader;
        if (!binaryReader.SafeReadBytes(&typeHeader, sizeof(typeHeader)) || typeHeader.TotalSize < (sizeof(DF_CLASS_TABLE_TYPE) + typeHeader.TypeNameLength))
            return false;

        // work out offset to next type in case we have to skip it
        uint64 nextTypeOffset = binaryReader.GetStreamPosition() + (typeHeader.TotalSize - sizeof(DF_CLASS_TABLE_TYPE));

        // read type name
        if (!binaryReader.SafeReadFixedString(typeHeader.TypeNameLength, &extractedString))
            return false;

        // try to map this to a type we know
        const ObjectTypeInfo *pTypeInfo = ObjectTypeInfo::GetRegistry().GetTypeInfoByName(extractedString);
        if (pTypeInfo == nullptr)
        {
            Log_WarningPrintf("ClassTable::LoadFromStream: Table contains unknown type '%s'", extractedString.GetCharArray());
            if (abortOnError)
                return false;

            // skip this type
            if (!binaryReader.SafeSeekAbsolute(nextTypeOffset))
                return false;
        }

        // allocate properties
        TypeMapping *destMapping = &m_pTypeMappings[typeIndex];
        destMapping->m_pTypeInfo = pTypeInfo;
        destMapping->m_ppPropertyMapping = new const PROPERTY_DECLARATION *[typeHeader.PropertyCount];
        destMapping->m_propertyCount = typeHeader.PropertyCount;
        Y_memzero(destMapping->m_ppPropertyMapping, sizeof(const PROPERTY_DECLARATION *) * typeHeader.PropertyCount);

        // read properties
        for (uint32 propertyIndex = 0; propertyIndex < destMapping->m_propertyCount; propertyIndex++)
        {
            // extract and increment pointer
            DF_CLASS_TABLE_TYPE_PROPERTY propertyHeader;
            if (!binaryReader.SafeReadBytes(&propertyHeader, sizeof(propertyHeader)))
                return false;

            // read the property name
            if (!binaryReader.SafeReadFixedString(propertyHeader.PropertyNameLength, &extractedString))
                return false;

            // find the mapping
            if ((destMapping->m_ppPropertyMapping[propertyIndex] = pTypeInfo->GetPropertyDeclarationByName(extractedString)) == nullptr)
            {
                Log_WarningPrintf("ClassTable::LoadFromStream: Table for type '%s' contains unknown property '%s'", pTypeInfo->GetTypeName(), extractedString.GetCharArray());
                if (abortOnError)
                    return false;
            }
        }

        // should be in the correct position
        if (pStream->GetPosition() != nextTypeOffset)
            return false;
    }

    return true;
#endif
}

bool ClassTable::LoadFromBuffer(const void *pBuffer, uint32 bufferSize, bool abortOnError /* = false */)
{
    const DF_CLASS_TABLE_HEADER *pHeader = reinterpret_cast<const DF_CLASS_TABLE_HEADER *>(pBuffer);
    if (pHeader->Magic != DF_CLASS_TABLE_HEADER_MAGIC ||
        pHeader->HeaderSize != sizeof(DF_CLASS_TABLE_HEADER) ||
        pHeader->TotalSize > bufferSize)
    {
        return false;
    }

    const byte *pDataPointer = reinterpret_cast<const byte *>(pHeader) + pHeader->HeaderSize;
    SmallString extractedString;

    // allocate types
    m_pTypeMappings = (TypeMapping *)Y_realloc(m_pTypeMappings, sizeof(TypeMapping) * pHeader->TypeCount);
    Y_memzero(m_pTypeMappings, sizeof(TypeMapping) * pHeader->TypeCount);
    m_typeCount = pHeader->TypeCount;

    // read types
    for (uint32 typeIndex = 0; typeIndex < m_typeCount; typeIndex++)
    {
        TypeMapping *destMapping = &m_pTypeMappings[typeIndex];

        // get header
        const DF_CLASS_TABLE_TYPE *sourceType = reinterpret_cast<const DF_CLASS_TABLE_TYPE *>(pDataPointer);

        // extract name
        extractedString.Clear();
        extractedString.AppendString(sourceType->TypeName, sourceType->TypeNameLength);

        // try to map this to a type we know
        const ObjectTypeInfo *pTypeInfo = ObjectTypeInfo::GetRegistry().GetTypeInfoByName(extractedString);
        if (pTypeInfo == nullptr)
        {
            Log_WarningPrintf("ClassTable::LoadFromBuffer: Table contains unknown type '%s'", extractedString.GetCharArray());
            if (abortOnError)
                return false;

            // skip this type
            pDataPointer += sourceType->TotalSize;
            continue;
        }

        // allocate properties
        destMapping->m_pTypeInfo = pTypeInfo;
        destMapping->m_ppPropertyMapping = new const PROPERTY_DECLARATION *[sourceType->PropertyCount];
        destMapping->m_propertyCount = sourceType->PropertyCount;
        Y_memzero(destMapping->m_ppPropertyMapping, sizeof(const PROPERTY_DECLARATION *) * sourceType->PropertyCount);

        // start on properties
        const byte *pPropertyPointer = pDataPointer + sizeof(DF_CLASS_TABLE_TYPE) + sourceType->TypeNameLength;
        for (uint32 propertyIndex = 0; propertyIndex < sourceType->PropertyCount; propertyIndex++)
        {
            // extract and increment pointer
            const DF_CLASS_TABLE_TYPE_PROPERTY *sourceProperty = reinterpret_cast<const DF_CLASS_TABLE_TYPE_PROPERTY *>(pPropertyPointer);
            pPropertyPointer += sizeof(DF_CLASS_TABLE_TYPE_PROPERTY) + sourceProperty->PropertyNameLength;

            // find the mapping
            extractedString.Clear();
            extractedString.AppendString(sourceProperty->PropertyName, sourceProperty->PropertyNameLength);
            if ((destMapping->m_ppPropertyMapping[propertyIndex] = pTypeInfo->GetPropertyDeclarationByName(extractedString)) == nullptr)
            {
                Log_WarningPrintf("ClassTable::LoadFromBuffer: Table for type '%s' contains unknown property '%s'", pTypeInfo->GetTypeName(), extractedString.GetCharArray());
                if (abortOnError)
                    return false;
            }
        }

        // increment data pointer
        pDataPointer += sourceType->TotalSize;
    }

    // done
    return true;
}

bool ClassTable::SaveToStream(ByteStream *pStream) const
{
    BinaryWriter binaryWriter(pStream);
    uint32 *pTypeSizes = (uint32 *)alloca(sizeof(uint32) * m_typeCount);

    // avoiding seeking so we can push this straight to the output stream, but we need to know the total size
    uint32 totalSize = sizeof(DF_CLASS_TABLE_HEADER);
    for (uint32 typeIndex = 0; typeIndex < m_typeCount; typeIndex++)
    {
        const TypeMapping *pTypeMapping = &m_pTypeMappings[typeIndex];
        uint32 typeSize = sizeof(DF_CLASS_TABLE_TYPE);
        if (pTypeMapping->m_pTypeInfo == nullptr)
            typeSize += Y_strlen(MISSING_PROPERTY_NAME_STRING);
        else
            typeSize += Y_strlen(pTypeMapping->GetTypeInfo()->GetTypeName());

        for (uint32 propertyIndex = 0; propertyIndex < pTypeMapping->GetPropertyCount(); propertyIndex++)
        {
            typeSize += sizeof(DF_CLASS_TABLE_TYPE_PROPERTY);
            if (pTypeMapping->m_ppPropertyMapping[propertyIndex] == nullptr)
                typeSize += Y_strlen(MISSING_PROPERTY_NAME_STRING);
            else
                typeSize += Y_strlen(pTypeMapping->m_ppPropertyMapping[propertyIndex]->Name);
        }

        pTypeSizes[typeIndex] = typeSize;
        totalSize += typeSize;
    }

    // write header
    DF_CLASS_TABLE_HEADER header;
    header.Magic = DF_CLASS_TABLE_HEADER_MAGIC;
    header.TotalSize = totalSize;
    header.HeaderSize = sizeof(header);
    header.TypeCount = m_typeCount;
    if (!binaryWriter.SafeWriteBytes(&header, sizeof(header)))
        return false;

    // write types
    for (uint32 typeIndex = 0; typeIndex < m_typeCount; typeIndex++)
    {
        const TypeMapping *pTypeMapping = &m_pTypeMappings[typeIndex];
        DF_CLASS_TABLE_TYPE typeHeader;
        typeHeader.TotalSize = pTypeSizes[typeIndex];
        typeHeader.PropertyCount = pTypeMapping->GetPropertyCount();
        if (pTypeMapping->m_pTypeInfo != nullptr)
        {
            typeHeader.TypeNameLength = Y_strlen(pTypeMapping->m_pTypeInfo->GetTypeName());
            if (!binaryWriter.SafeWriteBytes(&typeHeader, sizeof(typeHeader)) || !binaryWriter.SafeWriteBytes(pTypeMapping->m_pTypeInfo->GetTypeName(), typeHeader.TypeNameLength))
                return false;
        }
        else
        {
            typeHeader.TypeNameLength = Y_strlen(MISSING_PROPERTY_NAME_STRING);
            if (!binaryWriter.SafeWriteBytes(&typeHeader, sizeof(typeHeader)) || !binaryWriter.SafeWriteBytes(MISSING_PROPERTY_NAME_STRING, typeHeader.TypeNameLength))
                return false;
        }

        // write properties
        for (uint32 propertyIndex = 0; propertyIndex < pTypeMapping->GetPropertyCount(); propertyIndex++)
        {
            const PROPERTY_DECLARATION *pProperty = pTypeMapping->GetPropertyMapping(propertyIndex);
            DF_CLASS_TABLE_TYPE_PROPERTY propertyHeader;
            if (pProperty != nullptr)
            {
                propertyHeader.PropertyType = pProperty->Type;
                propertyHeader.PropertyNameLength = Y_strlen(pProperty->Name);
                if (!binaryWriter.SafeWriteBytes(&propertyHeader, sizeof(propertyHeader)) || !binaryWriter.SafeWriteBytes(pProperty->Name, propertyHeader.PropertyNameLength))
                    return false;
            }
            else
            {
                propertyHeader.PropertyType = PROPERTY_TYPE_STRING;
                propertyHeader.PropertyNameLength = Y_strlen(MISSING_PROPERTY_NAME_STRING);
                if (!binaryWriter.SafeWriteBytes(&propertyHeader, sizeof(propertyHeader)) || !binaryWriter.SafeWriteBytes(MISSING_PROPERTY_NAME_STRING, propertyHeader.PropertyNameLength))
                    return false;
            }
        }
    }

    return true;
}

uint32 ClassTable::AddType(const ObjectTypeInfo *pTypeInfo)
{
    DebugAssert(GetTypeMappingForType(pTypeInfo) < 0);

    // reallocate for space
    uint32 typeIndex = m_typeCount;
    m_pTypeMappings = (TypeMapping *)Y_realloc(m_pTypeMappings, sizeof(TypeMapping) * (m_typeCount + 1));
    DebugAssert(m_pTypeMappings != nullptr);
    m_typeCount++;

    // add it
    TypeMapping *pTypeMapping = &m_pTypeMappings[typeIndex];
    pTypeMapping->m_pTypeInfo = pTypeInfo;
    pTypeMapping->m_ppPropertyMapping = new const PROPERTY_DECLARATION *[pTypeInfo->GetPropertyCount()];
    pTypeMapping->m_propertyCount = pTypeInfo->GetPropertyCount();
    for (uint32 i = 0; i < pTypeMapping->m_propertyCount; i++)
        pTypeMapping->m_ppPropertyMapping[i] = pTypeInfo->GetPropertyDeclarationByIndex(i);

    // return index
    return typeIndex;
}

bool ClassTable::SerializeObject(const Object *pObject, ByteStream *pStream)
{
    BinaryWriter binaryWriter(pStream);

    // in list?
    int32 typeIndex = GetTypeMappingForType(pObject->GetObjectTypeInfo());
    if (typeIndex < 0)
    {
        // add it
        typeIndex = (int32)AddType(pObject->GetObjectTypeInfo());
    }

    // serialize the object's properties
    const TypeMapping *pTypeMapping = &m_pTypeMappings[typeIndex];
    for (uint32 propertyIndex = 0; propertyIndex < pTypeMapping->GetPropertyCount(); propertyIndex++)
    {
        // write the property value to the buffer
        if (!WritePropertyValueToBuffer(pObject, pTypeMapping->GetPropertyMapping(propertyIndex), binaryWriter))
            return false;
    }

    return true;
}

Object *ClassTable::UnserializeObject(ByteStream *pStream, uint32 typeIndex)
{
    // should be a valid type index
    const TypeMapping *pTypeMapping = &m_pTypeMappings[typeIndex];
    DebugAssert(typeIndex < m_typeCount);

    // is it mapped
    const ObjectTypeInfo *pTypeInfo = pTypeMapping->GetTypeInfo();
    if (pTypeInfo == nullptr)
    {
        Log_ErrorPrintf("ClassTable::UnserializeObject: Missing mapping for type %u", typeIndex);
        return nullptr;
    }

    // create the object
    ObjectFactory *pFactory = pTypeInfo->GetFactory();
    Object *pObject = (pFactory != nullptr) ? pFactory->CreateObject() : nullptr;
    if (pObject == nullptr)
    {
        Log_ErrorPrintf("ClassTable::UnserializeObject: Failed to create instance of type '%s'", pTypeMapping->GetTypeInfo()->GetTypeName());
        return nullptr;
    }

    // import properties
    for (uint32 propertyIndex = 0; propertyIndex < pTypeMapping->GetPropertyCount(); propertyIndex++)
    {
//         // read property info
//         uint32 propertySize;
//         if (!pStream->Read2(&propertySize, sizeof(propertySize)))
//             return nullptr;

        // mapped?
        const PROPERTY_DECLARATION *pMappedProperty = pTypeMapping->GetPropertyMapping(propertyIndex);
        if (pMappedProperty == nullptr)
        {
            // skip the property
            uint32 propertySize;
            if (!pStream->Read2(&propertySize, sizeof(propertySize)) ||
                !pStream->SeekRelative((int64)propertySize))
            {
                pFactory->DeleteObject(pObject);
                return nullptr;
            }

            continue;
        }

        // FIXME
        BinaryReader binaryReader(pStream);
        if (!ReadPropertyValueFromBuffer(pObject, pMappedProperty, binaryReader))
        {
            pFactory->DeleteObject(pObject);
            return nullptr;
        }
    }

    // all done
    return pObject;
}
