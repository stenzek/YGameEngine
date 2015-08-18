#include "Core/PrecompiledHeader.h"
#include "Core/Property.h"
#include "YBaseLib/BinaryReader.h"
#include "YBaseLib/BinaryWriter.h"
#include "YBaseLib/StringConverter.h"
#include "MathLib/StringConverters.h"
#include "MathLib/StreamOperators.h"

Y_Define_NameTable(NameTables::PropertyType)
    Y_NameTable_Entry("bool", PROPERTY_TYPE_BOOL)
    Y_NameTable_Entry("uint", PROPERTY_TYPE_UINT)
    Y_NameTable_Entry("int", PROPERTY_TYPE_INT)
    Y_NameTable_Entry("int2", PROPERTY_TYPE_INT2)
    Y_NameTable_Entry("int3", PROPERTY_TYPE_INT3)
    Y_NameTable_Entry("int4", PROPERTY_TYPE_INT4)
    Y_NameTable_Entry("float", PROPERTY_TYPE_FLOAT)
    Y_NameTable_Entry("float2", PROPERTY_TYPE_FLOAT2)
    Y_NameTable_Entry("float3", PROPERTY_TYPE_FLOAT3)
    Y_NameTable_Entry("float4", PROPERTY_TYPE_FLOAT4)
    Y_NameTable_Entry("Color", PROPERTY_TYPE_COLOR)
    Y_NameTable_Entry("Quaternion", PROPERTY_TYPE_QUATERNION)
    Y_NameTable_Entry("Transform", PROPERTY_TYPE_TRANSFORM)
    Y_NameTable_Entry("String", PROPERTY_TYPE_STRING)
Y_NameTable_End()

bool GetPropertyValueAsString(const void *pObject, const PROPERTY_DECLARATION *pProperty, String &StrValue)
{
    if (pProperty->GetPropertyCallback == NULL)
        return false;

    // Strings handled seperately.
    if (pProperty->Type == PROPERTY_TYPE_STRING)
    {
        // We can pass StrValue directly across.
        return pProperty->GetPropertyCallback(pObject, pProperty->pGetPropertyCallbackUserData, &StrValue);
    }
    else
    {
        // 32 bytes should be enough for the actual value. (largest is currently transform, which is float3 + quat + float)
        byte TempValue[32];

        // Call the function.
        if (!pProperty->GetPropertyCallback(pObject, pProperty->pGetPropertyCallbackUserData, &TempValue))
            return false;

        // Now stringize it based on type.
        switch (pProperty->Type)
        {
        case PROPERTY_TYPE_BOOL:
            StringConverter::BoolToString(StrValue, reinterpret_cast<const bool &>(TempValue));
            break;

        case PROPERTY_TYPE_UINT:
            StringConverter::UInt32ToString(StrValue, reinterpret_cast<const uint32 &>(TempValue));
            break;

        case PROPERTY_TYPE_INT:
            StringConverter::Int32ToString(StrValue, reinterpret_cast<const int32 &>(TempValue));
            break;

        case PROPERTY_TYPE_INT2:
            StringConverter::Vector2iToString(StrValue, reinterpret_cast<const Vector2i &>(TempValue));
            break;

        case PROPERTY_TYPE_INT3:
            StringConverter::Vector3iToString(StrValue, reinterpret_cast<const Vector3i &>(TempValue));
            break;

        case PROPERTY_TYPE_INT4:
            StringConverter::Vector4iToString(StrValue, reinterpret_cast<const Vector4i &>(TempValue));
            break;

        case PROPERTY_TYPE_FLOAT:
            StringConverter::FloatToString(StrValue, reinterpret_cast<const float &>(TempValue));
            break;

        case PROPERTY_TYPE_FLOAT2:
            StringConverter::Vector2fToString(StrValue, reinterpret_cast<const Vector2f &>(TempValue));
            break;

        case PROPERTY_TYPE_FLOAT3:
            StringConverter::Vector3fToString(StrValue, reinterpret_cast<const Vector3f &>(TempValue));
            break;

        case PROPERTY_TYPE_FLOAT4:
            StringConverter::Vector4fToString(StrValue, reinterpret_cast<const Vector4f &>(TempValue));
            break;

        case PROPERTY_TYPE_QUATERNION:
            StringConverter::QuaternionToString(StrValue, reinterpret_cast<const Quaternion &>(TempValue));
            break;

        case PROPERTY_TYPE_TRANSFORM:
            StringConverter::TransformToString(StrValue, reinterpret_cast<const Transform &>(TempValue));
            break;

        case PROPERTY_TYPE_COLOR:
            StringConverter::ColorToString(StrValue, reinterpret_cast<const uint32 &>(TempValue));
            break;

        default:
            UnreachableCode();
            break;
        }

        return true;
    }
}

bool SetPropertyValueFromString(void *pObject, const PROPERTY_DECLARATION *pProperty, const char *szValue)
{
    if (pProperty->SetPropertyCallback == NULL)
        return false;

    // Strings handled seperately.
    if (pProperty->Type == PROPERTY_TYPE_STRING)
    {
        // Create a constant string.
        StaticString StringRef(szValue);
        if (!pProperty->SetPropertyCallback(pObject, pProperty->pSetPropertyCallbackUserData, &StringRef))
            return false;
    }
    else
    {
        // 32 bytes should be enough for the actual value. (largest is currently transform, which is float3 + quat + float)
        byte TempValue[32];

        // Un-stringize based on type.
        switch (pProperty->Type)
        {
        case PROPERTY_TYPE_BOOL:
            reinterpret_cast<bool &>(TempValue) = StringConverter::StringToBool(szValue);
            break;

        case PROPERTY_TYPE_UINT:
            reinterpret_cast<uint32 &>(TempValue) = StringConverter::StringToUInt32(szValue);
            break;

        case PROPERTY_TYPE_INT:
            reinterpret_cast<int32 &>(TempValue) = StringConverter::StringToInt32(szValue);
            break;

        case PROPERTY_TYPE_INT2:
            reinterpret_cast<Vector2i &>(TempValue) = StringConverter::StringToVector2i(szValue);
            break;

        case PROPERTY_TYPE_INT3:
            reinterpret_cast<Vector3i &>(TempValue) = StringConverter::StringToVector3i(szValue);
            break;

        case PROPERTY_TYPE_INT4:
            reinterpret_cast<Vector4i &>(TempValue) = StringConverter::StringToVector4i(szValue);
            break;

        case PROPERTY_TYPE_FLOAT:
            reinterpret_cast<float &>(TempValue) = StringConverter::StringToFloat(szValue);
            break;

        case PROPERTY_TYPE_FLOAT2:
            reinterpret_cast<Vector2f &>(TempValue) = StringConverter::StringToVector2f(szValue);
            break;

        case PROPERTY_TYPE_FLOAT3:
            reinterpret_cast<Vector3f &>(TempValue) = StringConverter::StringToVector3f(szValue);
            break;

        case PROPERTY_TYPE_FLOAT4:
            reinterpret_cast<Vector4f &>(TempValue) = StringConverter::StringToVector4f(szValue);
            break;

        case PROPERTY_TYPE_QUATERNION:
            reinterpret_cast<Quaternion &>(TempValue) = StringConverter::StringToQuaternion(szValue);
            break;

        case PROPERTY_TYPE_TRANSFORM:
            reinterpret_cast<Transform &>(TempValue) = StringConverter::StringToTranform(szValue);
            break;

        case PROPERTY_TYPE_COLOR:
            reinterpret_cast<uint32 &>(TempValue) = StringConverter::StringToColor(szValue);
            break;

        default:
            UnreachableCode();
            break;
        }

        // Call the function.
        if (!pProperty->SetPropertyCallback(pObject, pProperty->pSetPropertyCallbackUserData, TempValue))
            return false;
    }

    // Notify updater if needed.
    //if (pProperty->PropertyChangedCallback != NULL)
        //pProperty->PropertyChangedCallback(pObject, pProperty->pPropertyChangedCallbackUserData);

    return true;
}

bool WritePropertyValueToBuffer(const void *pObject, const PROPERTY_DECLARATION *pProperty, BinaryWriter &binaryWriter)
{
    if (pProperty->GetPropertyCallback == NULL)
        return false;

    // Strings handled seperately.
    if (pProperty->Type == PROPERTY_TYPE_STRING)
    {
        // We can pass StrValue directly across.
        SmallString stringValue;
        if (!pProperty->GetPropertyCallback(pObject, pProperty->pGetPropertyCallbackUserData, &stringValue))
            return false;

        binaryWriter.WriteUInt32(stringValue.GetLength() + 1);
        binaryWriter.WriteCString(stringValue);
        return true;
    }
    else
    {
        // 32 bytes should be enough for the actual value. (largest is currently transform, which is float3 + quat + float)
        byte TempValue[32];

        // Call the function.
        if (!pProperty->GetPropertyCallback(pObject, pProperty->pGetPropertyCallbackUserData, &TempValue))
            return false;

        // Now stringize it based on type.
        switch (pProperty->Type)
        {
        case PROPERTY_TYPE_BOOL:
            binaryWriter.WriteUInt32(1);
            binaryWriter.WriteBool(reinterpret_cast<const bool &>(TempValue));
            break;

        case PROPERTY_TYPE_UINT:
            binaryWriter.WriteUInt32(4);
            binaryWriter.WriteUInt32(reinterpret_cast<const uint32 &>(TempValue));
            break;

        case PROPERTY_TYPE_INT:
            binaryWriter.WriteUInt32(4);
            binaryWriter.WriteInt32(reinterpret_cast<const int32 &>(TempValue));
            break;

        case PROPERTY_TYPE_INT2:
            binaryWriter.WriteUInt32(8);
            binaryWriter << (reinterpret_cast<const Vector2i &>(TempValue));
            break;

        case PROPERTY_TYPE_INT3:
            binaryWriter.WriteUInt32(12);
            binaryWriter << (reinterpret_cast<const Vector3i &>(TempValue));
            break;

        case PROPERTY_TYPE_INT4:
            binaryWriter.WriteUInt32(16);
            binaryWriter << (reinterpret_cast<const Vector4i &>(TempValue));
            break;

        case PROPERTY_TYPE_FLOAT:
            binaryWriter.WriteUInt32(4);
            binaryWriter.WriteFloat(reinterpret_cast<const float &>(TempValue));
            break;

        case PROPERTY_TYPE_FLOAT2:
            binaryWriter.WriteUInt32(8);
            binaryWriter << (reinterpret_cast<const Vector2f &>(TempValue));
            break;

        case PROPERTY_TYPE_FLOAT3:
            binaryWriter.WriteUInt32(12);
            binaryWriter << (reinterpret_cast<const Vector3f &>(TempValue));
            break;

        case PROPERTY_TYPE_FLOAT4:
            binaryWriter.WriteUInt32(16);
            binaryWriter << (reinterpret_cast<const Vector4f &>(TempValue));
            break;

        case PROPERTY_TYPE_QUATERNION:
            binaryWriter.WriteUInt32(16);
            binaryWriter << (reinterpret_cast<const Quaternion &>(TempValue).GetVectorRepresentation());
            break;

        case PROPERTY_TYPE_TRANSFORM:
            binaryWriter.WriteUInt32(12 + 16 + 4);
            binaryWriter << (reinterpret_cast<const Transform &>(TempValue).GetPosition());
            binaryWriter << (reinterpret_cast<const Transform &>(TempValue).GetRotation().GetVectorRepresentation());
            binaryWriter << (reinterpret_cast<const Transform &>(TempValue).GetScale());
            break;

        case PROPERTY_TYPE_COLOR:
            binaryWriter.WriteUInt32(4);
            binaryWriter.WriteUInt32(reinterpret_cast<const uint32 &>(TempValue));
            break;

        default:
            UnreachableCode();
            break;
        }

        return true;
    }
}

bool ReadPropertyValueFromBuffer(void *pObject, const PROPERTY_DECLARATION *pProperty, BinaryReader &binaryReader)
{
    if (pProperty->SetPropertyCallback == NULL)
        return false;

    // Strings handled seperately.
    if (pProperty->Type == PROPERTY_TYPE_STRING)
    {
        uint32 stringLength = binaryReader.ReadUInt32();

        SmallString stringValue;
        binaryReader.ReadCString(stringValue);        
        if (stringValue.GetLength() != (stringLength - 1) || !pProperty->SetPropertyCallback(pObject, pProperty->pSetPropertyCallbackUserData, &stringValue))
            return false;
    }
    else
    {
        // 32 bytes should be enough for the actual value. (largest is currently transform, which is float3 + quat + float)
        byte TempValue[32];

        // Un-stringize based on type.
        switch (pProperty->Type)
        {
        case PROPERTY_TYPE_BOOL:
            if (binaryReader.ReadUInt32() != 1) { return false; }
            reinterpret_cast<bool &>(TempValue) = binaryReader.ReadBool();
            break;

        case PROPERTY_TYPE_UINT:
            if (binaryReader.ReadUInt32() != 4) { return false; }
            reinterpret_cast<uint32 &>(TempValue) = binaryReader.ReadUInt32();
            break;

        case PROPERTY_TYPE_INT:
            if (binaryReader.ReadUInt32() != 4) { return false; }
            reinterpret_cast<int32 &>(TempValue) = binaryReader.ReadInt32();
            break;

        case PROPERTY_TYPE_INT2:
            if (binaryReader.ReadUInt32() != 8) { return false; }
            binaryReader >> reinterpret_cast<Vector2i &>(TempValue);
            break;

        case PROPERTY_TYPE_INT3:
            if (binaryReader.ReadUInt32() != 12) { return false; }
            binaryReader >> reinterpret_cast<Vector3i &>(TempValue);
            break;

        case PROPERTY_TYPE_INT4:
            if (binaryReader.ReadUInt32() != 16) { return false; }
            binaryReader >> reinterpret_cast<Vector4i &>(TempValue);
            break;

        case PROPERTY_TYPE_FLOAT:
            if (binaryReader.ReadUInt32() != 4) { return false; }
            reinterpret_cast<float &>(TempValue) = binaryReader.ReadFloat();
            break;

        case PROPERTY_TYPE_FLOAT2:
            if (binaryReader.ReadUInt32() != 8) { return false; }
            binaryReader >> reinterpret_cast<Vector2f &>(TempValue);
            break;

        case PROPERTY_TYPE_FLOAT3:
            if (binaryReader.ReadUInt32() != 12) { return false; }
            binaryReader >> reinterpret_cast<Vector3f &>(TempValue);
            break;

        case PROPERTY_TYPE_FLOAT4:
            if (binaryReader.ReadUInt32() != 16) { return false; }
            binaryReader >> reinterpret_cast<Vector4f &>(TempValue);
            break;

        case PROPERTY_TYPE_QUATERNION:
            if (binaryReader.ReadUInt32() != 16) { return false; }
            binaryReader >> reinterpret_cast<Quaternion &>(TempValue);
            break;

        case PROPERTY_TYPE_TRANSFORM:
            {
                if (binaryReader.ReadUInt32() != (12 + 16 + 4)) { return false; }
                Vector3f position; Quaternion rotation; Vector3f scale;
                binaryReader >> position >> rotation >> scale;
                reinterpret_cast<Transform &>(TempValue).SetPosition(position);
                reinterpret_cast<Transform &>(TempValue).SetRotation(rotation);
                reinterpret_cast<Transform &>(TempValue).SetScale(scale);
            }
            break;

        case PROPERTY_TYPE_COLOR:
            if (binaryReader.ReadUInt32() != 4) { return false; }
            reinterpret_cast<uint32 &>(TempValue) = binaryReader.ReadUInt32();
            break;

        default:
            UnreachableCode();
            break;
        }

        // Call the function.
        if (!pProperty->SetPropertyCallback(pObject, pProperty->pSetPropertyCallbackUserData, TempValue))
            return false;
    }

    // Notify updater if needed.
    //if (pProperty->PropertyChangedCallback != NULL)
        //pProperty->PropertyChangedCallback(pObject, pProperty->pPropertyChangedCallbackUserData);

    return true;
}

bool EncodePropertyTypeToBuffer(PROPERTY_TYPE propertyType, const char *valueString, BinaryWriter &binaryWriter)
{
    // Strings handled seperately.
    if (propertyType == PROPERTY_TYPE_STRING)
    {
        // We can pass StrValue directly across.
        binaryWriter.WriteUInt32(Y_strlen(valueString) + 1);
        binaryWriter.WriteCString(valueString);
        return true;
    }
    else
    {
        // Now stringize it based on type.
        switch (propertyType)
        {
        case PROPERTY_TYPE_BOOL:
            binaryWriter.WriteUInt32(1);
            binaryWriter.WriteBool(StringConverter::StringToBool(valueString));
            break;

        case PROPERTY_TYPE_UINT:
            binaryWriter.WriteUInt32(4);
            binaryWriter.WriteUInt32(StringConverter::StringToUInt32(valueString));
            break;

        case PROPERTY_TYPE_INT:
            binaryWriter.WriteUInt32(4);
            binaryWriter.WriteInt32(StringConverter::StringToInt32(valueString));
            break;

        case PROPERTY_TYPE_INT2:
            binaryWriter.WriteUInt32(8);
            binaryWriter << (StringConverter::StringToVector2i(valueString));
            break;

        case PROPERTY_TYPE_INT3:
            binaryWriter.WriteUInt32(12);
            binaryWriter << (StringConverter::StringToVector3i(valueString));
            break;

        case PROPERTY_TYPE_INT4:
            binaryWriter.WriteUInt32(16);
            binaryWriter << (StringConverter::StringToVector4i(valueString));
            break;

        case PROPERTY_TYPE_FLOAT:
            binaryWriter.WriteUInt32(4);
            binaryWriter.WriteFloat(StringConverter::StringToFloat(valueString));
            break;

        case PROPERTY_TYPE_FLOAT2:
            binaryWriter.WriteUInt32(8);
            binaryWriter << (StringConverter::StringToVector2f(valueString));
            break;

        case PROPERTY_TYPE_FLOAT3:
            binaryWriter.WriteUInt32(12);
            binaryWriter << (StringConverter::StringToVector3f(valueString));
            break;

        case PROPERTY_TYPE_FLOAT4:
            binaryWriter.WriteUInt32(16);
            binaryWriter << (StringConverter::StringToVector4f(valueString));
            break;

        case PROPERTY_TYPE_QUATERNION:
            binaryWriter.WriteUInt32(16);
            binaryWriter << (StringConverter::StringToQuaternion(valueString));
            break;

        case PROPERTY_TYPE_TRANSFORM:
            {
                Transform transform(StringConverter::StringToTranform(valueString));
                binaryWriter.WriteUInt32(12 + 16 + 4);
                binaryWriter << (transform.GetPosition());
                binaryWriter << (transform.GetRotation());
                binaryWriter << (transform.GetScale());
            }
            break;

        case PROPERTY_TYPE_COLOR:
            binaryWriter.WriteUInt32(4);
            binaryWriter.WriteUInt32(StringConverter::StringToColor(valueString));;
            break;

        default:
            UnreachableCode();
            break;
        }

        return true;
    }
}

// default property callbacks
bool DefaultPropertyTableCallbacks::GetBool(const void *pObjectPtr, const void *pUserData, bool *pValuePtr)
{
    *pValuePtr = *((const bool *)((((const byte *)pObjectPtr) + (*(int *)&pUserData))));
    return true;
}

bool DefaultPropertyTableCallbacks::SetBool(void *pObjectPtr, const void *pUserData, const bool *pValuePtr)
{
    *((bool *)((((byte *)pObjectPtr) + (*(int *)&pUserData)))) = *pValuePtr;
    return true;
}

bool DefaultPropertyTableCallbacks::GetUInt(const void *pObjectPtr, const void *pUserData, uint32 *pValuePtr)
{
    *pValuePtr = *((const uint32 *)((((const byte *)pObjectPtr) + (*(uint32 *)&pUserData))));
    return true;
}

bool DefaultPropertyTableCallbacks::SetUInt(void *pObjectPtr, const void *pUserData, const uint32 *pValuePtr)
{
    *((uint32 *)((((byte *)pObjectPtr) + (*(uint32 *)&pUserData)))) = *pValuePtr;
    return true;
}

bool DefaultPropertyTableCallbacks::GetInt(const void *pObjectPtr, const void *pUserData, int32 *pValuePtr)
{
    *pValuePtr = *((const int32 *)((((const byte *)pObjectPtr) + (*(int32 *)&pUserData))));
    return true;
}

bool DefaultPropertyTableCallbacks::SetInt(void *pObjectPtr, const void *pUserData, const int32 *pValuePtr)
{
    *((int32 *)((((byte *)pObjectPtr) + (*(int32 *)&pUserData)))) = *pValuePtr;
    return true;
}

bool DefaultPropertyTableCallbacks::GetInt2(const void *pObjectPtr, const void *pUserData, Vector2i *pValuePtr)
{
    *pValuePtr = *((const Vector2i *)((((const byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData))));
    return true;
}

bool DefaultPropertyTableCallbacks::SetInt2(void *pObjectPtr, const void *pUserData, const Vector2i *pValuePtr)
{
    *((Vector2i *)((((byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData)))) = *pValuePtr;
    return true;
}

bool DefaultPropertyTableCallbacks::GetInt3(const void *pObjectPtr, const void *pUserData, Vector3i *pValuePtr)
{
    *pValuePtr = *((const Vector3i *)((((const byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData))));
    return true;
}

bool DefaultPropertyTableCallbacks::SetInt3(void *pObjectPtr, const void *pUserData, const Vector3i *pValuePtr)
{
    *((Vector3i *)((((byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData)))) = *pValuePtr;
    return true;
}

bool DefaultPropertyTableCallbacks::GetInt4(const void *pObjectPtr, const void *pUserData, Vector4i *pValuePtr)
{
    *pValuePtr = *((const Vector4i *)((((const byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData))));
    return true;
}

bool DefaultPropertyTableCallbacks::SetInt4(void *pObjectPtr, const void *pUserData, const Vector4i *pValuePtr)
{
    *((Vector4i *)((((byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData)))) = *pValuePtr;
    return true;
}

bool DefaultPropertyTableCallbacks::GetFloat(const void *pObjectPtr, const void *pUserData, float *pValuePtr)
{
    *pValuePtr = *((const float *)((((const byte *)pObjectPtr) + (*(int *)&pUserData))));
    return true;
}

bool DefaultPropertyTableCallbacks::SetFloat(void *pObjectPtr, const void *pUserData, const float *pValuePtr)
{
    *((float *)((((byte *)pObjectPtr) + (*(int *)&pUserData)))) = *pValuePtr;
    return true;
}

bool DefaultPropertyTableCallbacks::GetFloat2(const void *pObjectPtr, const void *pUserData, Vector2f *pValuePtr)
{
    *pValuePtr = *((const Vector2f *)((((const byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData))));
    return true;
}

bool DefaultPropertyTableCallbacks::SetFloat2(void *pObjectPtr, const void *pUserData, const Vector2f *pValuePtr)
{
    *((Vector2f *)((((byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData)))) = *pValuePtr;
    return true;
}

bool DefaultPropertyTableCallbacks::GetFloat3(const void *pObjectPtr, const void *pUserData, Vector3f *pValuePtr)
{
    *pValuePtr = *((const Vector3f *)((((const byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData))));
    return true;
}

bool DefaultPropertyTableCallbacks::SetFloat3(void *pObjectPtr, const void *pUserData, const Vector3f *pValuePtr)
{
    *((Vector3f *)((((byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData)))) = *pValuePtr;
    return true;
}

bool DefaultPropertyTableCallbacks::GetFloat4(const void *pObjectPtr, const void *pUserData, Vector4f *pValuePtr)
{
    *pValuePtr = *((const Vector4f *)((((const byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData))));
    return true;
}

bool DefaultPropertyTableCallbacks::SetFloat4(void *pObjectPtr, const void *pUserData, const Vector4f *pValuePtr)
{
    *((Vector4f *)((((byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData)))) = *pValuePtr;
    return true;
}

bool DefaultPropertyTableCallbacks::SetString(void *pObjectPtr, const void *pUserData, const String *pValuePtr)
{
    ((String *)((((byte *)pObjectPtr) + (*(int *)&pUserData))))->Assign(*pValuePtr);
    return true;
}

bool DefaultPropertyTableCallbacks::GetQuaternion(const void *pObjectPtr, const void *pUserData, Quaternion *pValuePtr)
{
    *pValuePtr = *((const Quaternion *)((((const byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData))));
    return true;
}

bool DefaultPropertyTableCallbacks::SetQuaternion(void *pObjectPtr, const void *pUserData, const Quaternion *pValuePtr)
{
    *((Quaternion *)((((byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData)))) = *pValuePtr;
    return true;
}

bool DefaultPropertyTableCallbacks::GetTransform(const void *pObjectPtr, const void *pUserData, Transform *pValuePtr)
{
    *pValuePtr = *((const Transform *)((((const byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData))));
    return true;
}

bool DefaultPropertyTableCallbacks::SetTransform(void *pObjectPtr, const void *pUserData, const Transform *pValuePtr)
{
    *((Transform *)((((byte *)pObjectPtr) + (*(ptrdiff_t *)&pUserData)))) = *pValuePtr;
    return true;
}

bool DefaultPropertyTableCallbacks::GetString(const void *pObjectPtr, const void *pUserData, String *pValuePtr)
{
    pValuePtr->Assign(*((const String *)((((const byte *)pObjectPtr) + (*(int *)&pUserData)))));
    return true;
}

bool DefaultPropertyTableCallbacks::GetColor(const void *pObjectPtr, const void *pUserData, uint32 *pValuePtr)
{
    *pValuePtr = *((const uint32 *)((((const byte *)pObjectPtr) + (*(uint32 *)&pUserData))));
    return true;
}

bool DefaultPropertyTableCallbacks::SetColor(const void *pObjectPtr, const void *pUserData, const uint32 *pValuePtr)
{
    *((uint32 *)((((byte *)pObjectPtr) + (*(uint32 *)&pUserData)))) = *pValuePtr;
    return true;
}

bool DefaultPropertyTableCallbacks::GetConstBool(const void *pObjectPtr, const void *pUserData, bool *pValuePtr)
{
    bool Value = (pUserData != 0) ? true : false;
    *pValuePtr = Value;
    return true;
}
