#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/ClassTableGenerator.h"
#include "Engine/DataFormats.h"
#include "Core/PropertyTemplate.h"
#include "Core/ClassTableDataFormat.h"

ClassTableGenerator::Type::Type(uint32 index, const char *typeName)
    : m_index(index),
      m_typeName(typeName)
{

}

ClassTableGenerator::Type::~Type()
{
    for (uint32 i = 0; i < m_properties.GetSize(); i++)
        delete m_properties[i];
}

const ClassTableGenerator::Type::PropertyDeclaration *ClassTableGenerator::Type::GetPropertyDeclarationByName(const char *name) const
{
    for (uint32 i = 0; i < m_properties.GetSize(); i++)
    {
        if (m_properties[i]->Name.CompareInsensitive(name))
            return m_properties[i];
    }

    return nullptr;
}

const ClassTableGenerator::Type::PropertyDeclaration *ClassTableGenerator::Type::AddPropertyDeclaration(const char *name, PROPERTY_TYPE type)
{
    for (uint32 i = 0; i < m_properties.GetSize(); i++)
    {
        if (m_properties[i]->Name.CompareInsensitive(name))
            return nullptr;
    }

    PropertyDeclaration *decl = new PropertyDeclaration();
    decl->Index = m_properties.GetSize();
    decl->Name = name;
    decl->Type = type;
    m_properties.Add(decl);   

    return decl;
}

ClassTableGenerator::ClassTableGenerator()
{

}

ClassTableGenerator::~ClassTableGenerator()
{
    for (uint32 i = 0; i < m_types.GetSize(); i++)
        delete m_types[i];
}

const ClassTableGenerator::Type *ClassTableGenerator::GetTypeByName(const char *name) const
{
    for (uint32 i = 0; i < m_types.GetSize(); i++)
    {
        if (m_types[i]->GetTypeName().CompareInsensitive(name))
            return m_types[i];
    }

    return nullptr;
}

const ClassTableGenerator::Type *ClassTableGenerator::CreateType(const char *name)
{
    for (uint32 i = 0; i < m_types.GetSize(); i++)
    {
        if (m_types[i]->GetTypeName().CompareInsensitive(name))
            return nullptr;
    }

    Type *type = new Type(m_types.GetSize(), name);
    m_types.Add(type);
    return type;
}

const ClassTableGenerator::Type *ClassTableGenerator::CreateTypeFromPropertyTemplate(const char *name, const PropertyTemplate *pPropertyTemplate)
{
    for (uint32 i = 0; i < m_types.GetSize(); i++)
    {
        if (m_types[i]->GetTypeName().CompareInsensitive(name))
            return nullptr;
    }

    Type *type = new Type(m_types.GetSize(), name);
    m_types.Add(type);

    for (uint32 propIndex = 0; propIndex < pPropertyTemplate->GetPropertyDefinitionCount(); propIndex++)
    {
        const PropertyTemplateProperty *pProperty = pPropertyTemplate->GetPropertyDefinition(propIndex);
        type->AddPropertyDeclaration(pProperty->GetName(), pProperty->GetType());
    }

    return type;
}

const ClassTableGenerator::Type *ClassTableGenerator::CreateTypeFromPropertyDeclarationList(const char *name, const PROPERTY_DECLARATION **ppProperties, uint32 nProperties)
{
    for (uint32 i = 0; i < m_types.GetSize(); i++)
    {
        if (m_types[i]->GetTypeName().CompareInsensitive(name))
            return nullptr;
    }

    Type *type = new Type(m_types.GetSize(), name);
    m_types.Add(type);

    for (uint32 propIndex = 0; propIndex < nProperties; propIndex++)
        type->AddPropertyDeclaration(ppProperties[propIndex]->Name, ppProperties[propIndex]->Type);

    return type;
}

bool ClassTableGenerator::Compile(ByteStream *pStream)
{
    // build the header
    uint64 headerOffset = pStream->GetPosition();
    DF_CLASS_TABLE_HEADER classTableHeader;
    classTableHeader.Magic = DF_CLASS_TABLE_HEADER_MAGIC;
    classTableHeader.TotalSize = sizeof(DF_CLASS_TABLE_HEADER);
    classTableHeader.HeaderSize = sizeof(DF_CLASS_TABLE_HEADER);
    classTableHeader.TypeCount = m_types.GetSize();
    if (!pStream->Write2(&classTableHeader, sizeof(classTableHeader)))
        return false;

    // write types
    for (uint32 typeIndex = 0; typeIndex < m_types.GetSize(); typeIndex++)
    {
        const Type *type = m_types[typeIndex];
        uint64 typeStartOffset = pStream->GetPosition();

        // we write the header bits out manually here
        byte typeHeaderBuffer[sizeof(DF_CLASS_TABLE_TYPE)];
        DF_CLASS_TABLE_TYPE *typeHeader = reinterpret_cast<DF_CLASS_TABLE_TYPE *>(typeHeaderBuffer);
        typeHeader->TotalSize = sizeof(DF_CLASS_TABLE_TYPE) + type->GetTypeName().GetLength();
        typeHeader->PropertyCount = type->GetPropertyDeclarationCount();
        typeHeader->TypeNameLength = type->GetTypeName().GetLength();
        if (!pStream->Write2(typeHeader, sizeof(*typeHeader)) || !pStream->Write2(type->GetTypeName().GetCharArray(), typeHeader->TypeNameLength))
            return false;

        // write properties
        for (uint32 propIndex = 0; propIndex < type->GetPropertyDeclarationCount(); propIndex++)
        {
            const Type::PropertyDeclaration *propDeclaration = type->GetPropertyDeclarationByIndex(propIndex);

            byte propHeaderBuffer[sizeof(DF_CLASS_TABLE_TYPE_PROPERTY)];
            DF_CLASS_TABLE_TYPE_PROPERTY *propHeader = reinterpret_cast<DF_CLASS_TABLE_TYPE_PROPERTY *>(propHeaderBuffer);
            propHeader->PropertyType = (uint32)propDeclaration->Type;
            propHeader->PropertyNameLength = propDeclaration->Name.GetLength();
            if (!pStream->Write2(propHeader, sizeof(*propHeader)) || !pStream->Write2(propDeclaration->Name.GetCharArray(), propHeader->PropertyNameLength))
                return false;

            typeHeader->TotalSize += sizeof(*propHeader) + propHeader->PropertyNameLength;
        }

        // update type header size
        uint64 reseekOffset = pStream->GetPosition();
        if (!pStream->SeekAbsolute(typeStartOffset) || !pStream->Write2(typeHeader, sizeof(*typeHeader)) || !pStream->SeekAbsolute(reseekOffset))
            return false;

        // update overall size
        classTableHeader.TotalSize += typeHeader->TotalSize;
    }

    // update header size
    uint64 reseekOffset = pStream->GetPosition();
    if (!pStream->SeekAbsolute(headerOffset) || !pStream->Write2(&classTableHeader, sizeof(classTableHeader)) || !pStream->SeekAbsolute(reseekOffset))
        return false;

    return true;
}

bool ClassTableGenerator::SerializeObjectBinary(ByteStream *pStream, const char *typeName, const PropertyTemplate *pPropertyTemplate, const PropertyTable *pPropertyTable, uint32 *pTypeIndex, uint32 *pWrittenBytes) const
{
    // get template
    const Type *typeInfo = GetTypeByName(typeName);
    if (typeInfo == nullptr)
        return false;

    // create temporary stream
    AutoReleasePtr<ByteStream> pTemporaryStream = ByteStream_CreateGrowableMemoryStream();

    // write the object header
    uint32 writtenBytes = 0;
    //DF_SERIALIZED_OBJECT_HEADER objectHeader;
    //objectHeader.TypeIndex = typeInfo->GetIndex();
    //objectHeader.TotalSize = sizeof(objectHeader);
    //pTemporaryStream->Write2(&objectHeader, sizeof(objectHeader));

    // write properties
    for (uint32 propIndex = 0; propIndex < typeInfo->GetPropertyDeclarationCount(); propIndex++)
    {
        const Type::PropertyDeclaration *propDeclaration = typeInfo->GetPropertyDeclarationByIndex(propIndex);

        // does this property even exist?
        const String *propertyValue = pPropertyTable->GetPropertyValuePointer(propDeclaration->Name);
        if (propertyValue == nullptr)
        {
            // use the default value from the template
            const PropertyTemplateProperty *pTemplateProperty = pPropertyTemplate->GetPropertyDefinitionByName(propDeclaration->Name);
            if (pTemplateProperty == nullptr)
                return false;

            // store it
            propertyValue = &pTemplateProperty->GetDefaultValue();
        }

        uint32 propertySize;
        if (!SerializePropertyValueStringAsNativeType(propDeclaration->Type, *propertyValue, pTemporaryStream, &propertySize))
            return false;

        //objectHeader.TotalSize += propertySize;
        writtenBytes += propertySize;
    }

    // copy back to main stream
    if (pTypeIndex != nullptr)
        *pTypeIndex = typeInfo->GetIndex();

    if (pWrittenBytes != nullptr)
        *pWrittenBytes = writtenBytes;

    return ByteStream_AppendStream(pTemporaryStream, pStream);
}

bool ClassTableGenerator::SerializePropertyValueStringAsNativeType(PROPERTY_TYPE propertyType, const char *propertyValueString, ByteStream *pOutputStream, uint32 *pWrittenBytes)
{
    if (propertyType == PROPERTY_TYPE_STRING)
    {
        // Write out the structure individually
        uint32 stringLength = Y_strlen(propertyValueString) + 1;
        if (pWrittenBytes != nullptr)
            *pWrittenBytes = sizeof(uint32) + stringLength;

        return (pOutputStream->Write2(&stringLength, sizeof(stringLength)) && pOutputStream->Write2(propertyValueString, stringLength));
    }
    else
    {
        // 32 bytes should be enough for the actual value. (largest is currently transform, which is float3 + quat + float)
        byte tempBuffer[4 + 32];
        DF_SERIALIZED_OBJECT_PROPERTY *propData = reinterpret_cast<DF_SERIALIZED_OBJECT_PROPERTY *>(tempBuffer);

        // Un-stringize based on type.
        switch (propertyType)
        {
        case PROPERTY_TYPE_BOOL:
            {
                bool value = StringConverter::StringToBool(propertyValueString);
                propData->DataSize = 1;
                propData->Data[0] = (value ? 1 : 0);
            }
            break;

        case PROPERTY_TYPE_UINT:
            {
                uint32 value = StringConverter::StringToUInt32(propertyValueString);
                uint32 *dest = reinterpret_cast<uint32 *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                *dest = value;
            }
            break;

        case PROPERTY_TYPE_INT:
            {
                int32 value = StringConverter::StringToInt32(propertyValueString);
                int32 *dest = reinterpret_cast<int32 *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                *dest = value;
            }
            break;

        case PROPERTY_TYPE_INT2:
            {
                int2 value(StringConverter::StringToInt2(propertyValueString));
                DF_STRUCT_INT2 *dest = reinterpret_cast<DF_STRUCT_INT2 *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                dest->x = value.x;
                dest->y = value.y;
            }
            break;

        case PROPERTY_TYPE_INT3:
            {
                int3 value(StringConverter::StringToInt3(propertyValueString));
                DF_STRUCT_INT3 *dest = reinterpret_cast<DF_STRUCT_INT3 *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                dest->x = value.x;
                dest->y = value.y;
                dest->z = value.z;
            }
            break;

        case PROPERTY_TYPE_INT4:
            {
                int4 value(StringConverter::StringToInt4(propertyValueString));
                DF_STRUCT_INT4 *dest = reinterpret_cast<DF_STRUCT_INT4 *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                dest->x = value.x;
                dest->y = value.y;
                dest->z = value.z;
                dest->w = value.w;
            }
            break;

        case PROPERTY_TYPE_FLOAT:
            {
                float value = StringConverter::StringToFloat(propertyValueString);
                float *dest = reinterpret_cast<float *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                *dest = value;
            }
            break;

        case PROPERTY_TYPE_FLOAT2:
            {
                float2 value(StringConverter::StringToFloat2(propertyValueString));
                DF_STRUCT_FLOAT2 *dest = reinterpret_cast<DF_STRUCT_FLOAT2 *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                dest->x = value.x;
                dest->y = value.y;
            }
            break;

        case PROPERTY_TYPE_FLOAT3:
            {
                float3 value(StringConverter::StringToFloat3(propertyValueString));
                DF_STRUCT_FLOAT3 *dest = reinterpret_cast<DF_STRUCT_FLOAT3 *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                dest->x = value.x;
                dest->y = value.y;
                dest->z = value.z;
            }
            break;

        case PROPERTY_TYPE_FLOAT4:
            {
                float4 value(StringConverter::StringToFloat4(propertyValueString));
                DF_STRUCT_FLOAT4 *dest = reinterpret_cast<DF_STRUCT_FLOAT4 *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                dest->x = value.x;
                dest->y = value.y;
                dest->z = value.z;
                dest->w = value.w;
            }
            break;

        case PROPERTY_TYPE_QUATERNION:
            {
                Quaternion value(StringConverter::StringToQuaternion(propertyValueString));
                DF_STRUCT_QUATERNION *dest = reinterpret_cast<DF_STRUCT_QUATERNION *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                dest->x = value.x;
                dest->y = value.y;
                dest->z = value.z;
                dest->w = value.w;
            }
            break;

        case PROPERTY_TYPE_TRANSFORM:
            {
                Transform value(StringConverter::StringToTranform(propertyValueString));
                DF_STRUCT_TRANSFORM *dest = reinterpret_cast<DF_STRUCT_TRANSFORM *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                dest->Position.x = value.GetPosition().x;
                dest->Position.y = value.GetPosition().y;
                dest->Position.z = value.GetPosition().z;
                dest->Rotation.x = value.GetRotation().x;
                dest->Rotation.y = value.GetRotation().y;
                dest->Rotation.z = value.GetRotation().z;
                dest->Rotation.w = value.GetRotation().w;
                dest->Scale.x = value.GetScale().x;
                dest->Scale.y = value.GetScale().y;
                dest->Scale.z = value.GetScale().z;
            }
            break;

        case PROPERTY_TYPE_COLOR:
            {
                uint32 value = StringConverter::StringToColor(propertyValueString);
                uint32 *dest = reinterpret_cast<uint32 *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                *dest = value;
            }
            break;

        default:
            UnreachableCode();
            break;
        }

        // Write to the stream
        if (pWrittenBytes != nullptr)
            *pWrittenBytes = sizeof(DF_SERIALIZED_OBJECT_PROPERTY) + propData->DataSize;

        DebugAssert(propData->DataSize < (sizeof(tempBuffer) - sizeof(DF_SERIALIZED_OBJECT_PROPERTY)));
        return pOutputStream->Write2(tempBuffer, sizeof(DF_SERIALIZED_OBJECT_PROPERTY) + propData->DataSize);
    }
}


