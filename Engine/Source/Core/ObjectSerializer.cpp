#include "Core/PrecompiledHeader.h"
#include "Core/ObjectSerializer.h"
#include "Core/ClassTableDataFormat.h"
#include "YBaseLib/StringConverter.h"
#include "MathLib/Vectorf.h"
#include "MathLib/Vectori.h"
#include "MathLib/Quaternion.h"
#include "MathLib/Transform.h"
#include "MathLib/StringConverters.h"

bool ObjectSerializer::SerializePropertyValueString(PROPERTY_TYPE propertyType, const char *propertyValueString, ByteStream *pOutputStream, uint32 *pWrittenBytes)
{
    if (propertyType == PROPERTY_TYPE_STRING)
    {
        // Write out the structure individually
        uint32 stringLength = Y_strlen(propertyValueString);
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
                Vector2i value(StringConverter::StringToVector2i(propertyValueString));
                DF_STRUCT_INT2 *dest = reinterpret_cast<DF_STRUCT_INT2 *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                dest->x = value.x;
                dest->y = value.y;
            }
            break;

        case PROPERTY_TYPE_INT3:
            {
                Vector3i value(StringConverter::StringToVector3i(propertyValueString));
                DF_STRUCT_INT3 *dest = reinterpret_cast<DF_STRUCT_INT3 *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                dest->x = value.x;
                dest->y = value.y;
                dest->z = value.z;
            }
            break;

        case PROPERTY_TYPE_INT4:
            {
                Vector4i value(StringConverter::StringToVector4i(propertyValueString));
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
                Vector2f value(StringConverter::StringToVector2f(propertyValueString));
                DF_STRUCT_FLOAT2 *dest = reinterpret_cast<DF_STRUCT_FLOAT2 *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                dest->x = value.x;
                dest->y = value.y;
            }
            break;

        case PROPERTY_TYPE_FLOAT3:
            {
                Vector3f value(StringConverter::StringToVector3f(propertyValueString));
                DF_STRUCT_FLOAT3 *dest = reinterpret_cast<DF_STRUCT_FLOAT3 *>(propData->Data);
                propData->DataSize = sizeof(*dest);
                dest->x = value.x;
                dest->y = value.y;
                dest->z = value.z;
            }
            break;

        case PROPERTY_TYPE_FLOAT4:
            {
                Vector4f value(StringConverter::StringToVector4f(propertyValueString));
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

