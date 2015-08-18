#pragma once
#include "Core/Common.h"
#include "YBaseLib/NameTable.h"
#include "YBaseLib/String.h"
#include "MathLib/Vectorf.h"
#include "MathLib/Vectori.h"
#include "MathLib/Vectoru.h"
#include "MathLib/Quaternion.h"
#include "MathLib/Transform.h"

class BinaryReader;
class BinaryWriter;

#define MAX_PROPERTY_TABLE_NAME_LENGTH 128
#define MAX_PROPERTY_NAME_LENGTH 128

enum PROPERTY_TYPE
{
    PROPERTY_TYPE_BOOL,
    PROPERTY_TYPE_UINT,
    PROPERTY_TYPE_INT,
    PROPERTY_TYPE_INT2,
    PROPERTY_TYPE_INT3,
    PROPERTY_TYPE_INT4,
    PROPERTY_TYPE_FLOAT,
    PROPERTY_TYPE_FLOAT2,
    PROPERTY_TYPE_FLOAT3,
    PROPERTY_TYPE_FLOAT4,
    PROPERTY_TYPE_QUATERNION,
    PROPERTY_TYPE_TRANSFORM,
    PROPERTY_TYPE_STRING,
    PROPERTY_TYPE_COLOR,
    PROPERTY_TYPE_COUNT,
};

namespace NameTables {
    Y_Declare_NameTable(PropertyType);
}

enum PROPERTY_FLAG
{
    PROPERTY_FLAG_READ_ONLY                         = (1 << 0),         // Property cannot be modified by user. Engine can still modify it, however.
    PROPERTY_FLAG_INVOKE_CHANGE_CALLBACK_ON_CREATE  = (1 << 1),         // Property change callback will be invoked when the object is being created. By default it is not.
};

struct PROPERTY_DECLARATION
{
    typedef bool(*GET_PROPERTY_CALLBACK)(const void *pObjectPtr, const void *pUserData, void *pValuePtr);
    typedef bool(*SET_PROPERTY_CALLBACK)(void *pObjectPtr, const void *pUserData, const void *pValuePtr);
    typedef void(*PROPERTY_CHANGED_CALLBACK)(void *pObjectPtr, const void *pUserData);

    const char *Name;
    PROPERTY_TYPE Type;
    uint32 Flags;

    GET_PROPERTY_CALLBACK GetPropertyCallback;
    const void *pGetPropertyCallbackUserData;
    SET_PROPERTY_CALLBACK SetPropertyCallback;
    const void *pSetPropertyCallbackUserData;
    PROPERTY_CHANGED_CALLBACK PropertyChangedCallback;
    const void *pPropertyChangedCallbackUserData;
};

bool GetPropertyValueAsString(const void *pObject, const PROPERTY_DECLARATION *pProperty, String &StrValue);
bool SetPropertyValueFromString(void *pObject, const PROPERTY_DECLARATION *pProperty, const char *szValue);
bool WritePropertyValueToBuffer(const void *pObject, const PROPERTY_DECLARATION *pProperty, BinaryWriter &binaryWriter);
bool ReadPropertyValueFromBuffer(void *pObject, const PROPERTY_DECLARATION *pProperty, BinaryReader &binaryReader);
bool EncodePropertyTypeToBuffer(PROPERTY_TYPE propertyType, const char *valueString, BinaryWriter &binaryWriter);

namespace DefaultPropertyTableCallbacks
{
    // builtin functions
    bool GetBool(const void *pObjectPtr, const void *pUserData, bool *pValuePtr);
    bool SetBool(void *pObjectPtr, const void *pUserData, const bool *pValuePtr);
    bool GetUInt(const void *pObjectPtr, const void *pUserData, uint32 *pValuePtr);
    bool SetUInt(void *pObjectPtr, const void *pUserData, const uint32 *pValuePtr);
    bool GetInt(const void *pObjectPtr, const void *pUserData, int32 *pValuePtr);
    bool SetInt(void *pObjectPtr, const void *pUserData, const int32 *pValuePtr);
    bool GetInt2(const void *pObjectPtr, const void *pUserData, Vector2i *pValuePtr);
    bool SetInt2(void *pObjectPtr, const void *pUserData, const Vector2i *pValuePtr);
    bool GetInt3(const void *pObjectPtr, const void *pUserData, Vector3i *pValuePtr);
    bool SetInt3(void *pObjectPtr, const void *pUserData, const Vector3i *pValuePtr);
    bool GetInt4(const void *pObjectPtr, const void *pUserData, Vector4i *pValuePtr);
    bool SetInt4(void *pObjectPtr, const void *pUserData, const Vector4i *pValuePtr);
    bool GetFloat(const void *pObjectPtr, const void *pUserData, float *pValuePtr);
    bool SetFloat(void *pObjectPtr, const void *pUserData, const float *pValuePtr);
    bool GetFloat2(const void *pObjectPtr, const void *pUserData, Vector2f *pValuePtr);
    bool SetFloat2(void *pObjectPtr, const void *pUserData, const Vector2f *pValuePtr);
    bool GetFloat3(const void *pObjectPtr, const void *pUserData, Vector3f *pValuePtr);
    bool SetFloat3(void *pObjectPtr, const void *pUserData, const Vector3f *pValuePtr);
    bool GetFloat4(const void *pObjectPtr, const void *pUserData, Vector4f *pValuePtr);
    bool SetFloat4(void *pObjectPtr, const void *pUserData, const Vector4f *pValuePtr);
    bool GetQuaternion(const void *pObjectPtr, const void *pUserData, Quaternion *pValuePtr);
    bool SetQuaternion(void *pObjectPtr, const void *pUserData, const Quaternion *pValuePtr);
    bool GetTransform(const void *pObjectPtr, const void *pUserData, Transform *pValuePtr);
    bool SetTransform(void *pObjectPtr, const void *pUserData, const Transform *pValuePtr);
    bool GetString(const void *pObjectPtr, const void *pUserData, String *pValuePtr);
    bool SetString(void *pObjectPtr, const void *pUserData, const String *pValuePtr);
    bool GetColor(const void *pObjectPtr, const void *pUserData, uint32 *pValuePtr);
    bool SetColor(const void *pObjectPtr, const void *pUserData, const uint32 *pValuePtr);

    // static bool value
    bool GetConstBool(const void *pObjectPtr, const void *pUserData, bool *pValuePtr);
}

#define PROPERTY_TABLE_MEMBER(Name, Type, Flags, \
                              GetPropertyCallback, GetPropertyCallbackUserData, \
                              SetPropertyCallback, SetPropertyCallbackUserData, \
                              PropertyChangedCallback, PropertyChangedCallbackUserData) \
            { \
              Name, Type, Flags, \
              (PROPERTY_DECLARATION::GET_PROPERTY_CALLBACK)(GetPropertyCallback), (const void *)(GetPropertyCallbackUserData), \
              (PROPERTY_DECLARATION::SET_PROPERTY_CALLBACK)(SetPropertyCallback), (const void *)(SetPropertyCallbackUserData), \
              (PROPERTY_DECLARATION::PROPERTY_CHANGED_CALLBACK)(PropertyChangedCallback), (const void *)(PropertyChangedCallbackUserData) \
            },

#define PROPERTY_TABLE_MEMBER_BOOL(Name, Flags, Offset, ChangedFunc, ChangedFuncUserData)  \
                                   PROPERTY_TABLE_MEMBER(Name, PROPERTY_TYPE_BOOL, Flags, \
                                                         DefaultPropertyTableCallbacks::GetBool, (Offset), \
                                                         DefaultPropertyTableCallbacks::SetBool, (Offset), \
                                                         ChangedFunc, ChangedFuncUserData)

#define PROPERTY_TABLE_MEMBER_UINT(Name, Flags, Offset, ChangedFunc, ChangedFuncUserData)  \
                                   PROPERTY_TABLE_MEMBER(Name, PROPERTY_TYPE_INT, Flags, \
                                                                DefaultPropertyTableCallbacks::GetUInt, (Offset), \
                                                                DefaultPropertyTableCallbacks::SetUInt, (Offset), \
                                                                ChangedFunc, ChangedFuncUserData)

#define PROPERTY_TABLE_MEMBER_INT(Name, Flags, Offset, ChangedFunc, ChangedFuncUserData)  \
                                  PROPERTY_TABLE_MEMBER(Name, PROPERTY_TYPE_INT, Flags, \
                                                                DefaultPropertyTableCallbacks::GetInt, (Offset), \
                                                                DefaultPropertyTableCallbacks::SetInt, (Offset), \
                                                                ChangedFunc, ChangedFuncUserData)

#define PROPERTY_TABLE_MEMBER_INT2(Name, Flags, Offset, ChangedFunc, ChangedFuncUserData)  \
                                   PROPERTY_TABLE_MEMBER(Name, PROPERTY_TYPE_INT2, Flags, \
                                                                DefaultPropertyTableCallbacks::GetInt2, (Offset), \
                                                                DefaultPropertyTableCallbacks::SetInt2, (Offset), \
                                                                ChangedFunc, ChangedFuncUserData)

#define PROPERTY_TABLE_MEMBER_INT3(Name, Flags, Offset, ChangedFunc, ChangedFuncUserData)  \
                                   PROPERTY_TABLE_MEMBER(Name, PROPERTY_TYPE_INT3, Flags, \
                                                                DefaultPropertyTableCallbacks::GetInt3, (Offset), \
                                                                DefaultPropertyTableCallbacks::SetInt3, (Offset), \
                                                                ChangedFunc, ChangedFuncUserData)

#define PROPERTY_TABLE_MEMBER_INT4(Name, Flags, Offset, ChangedFunc, ChangedFuncUserData)  \
                                   PROPERTY_TABLE_MEMBER(Name, PROPERTY_TYPE_INT4, Flags, \
                                                                DefaultPropertyTableCallbacks::GetInt4, (Offset), \
                                                                DefaultPropertyTableCallbacks::SetInt4, (Offset), \
                                                                ChangedFunc, ChangedFuncUserData)

#define PROPERTY_TABLE_MEMBER_FLOAT(Name, Flags, Offset, ChangedFunc, ChangedFuncUserData)  \
                                    PROPERTY_TABLE_MEMBER(Name, PROPERTY_TYPE_FLOAT, Flags, \
                                                          DefaultPropertyTableCallbacks::GetFloat, (Offset), \
                                                          DefaultPropertyTableCallbacks::SetFloat, (Offset), \
                                                          ChangedFunc, ChangedFuncUserData)

#define PROPERTY_TABLE_MEMBER_FLOAT2(Name, Flags, Offset, ChangedFunc, ChangedFuncUserData)  \
                                     PROPERTY_TABLE_MEMBER(Name, PROPERTY_TYPE_FLOAT2, Flags, \
                                                          DefaultPropertyTableCallbacks::GetFloat2, (Offset), \
                                                          DefaultPropertyTableCallbacks::SetFloat2, (Offset), \
                                                          ChangedFunc, ChangedFuncUserData)

#define PROPERTY_TABLE_MEMBER_FLOAT3(Name, Flags, Offset, ChangedFunc, ChangedFuncUserData)  \
                                     PROPERTY_TABLE_MEMBER(Name, PROPERTY_TYPE_FLOAT3, Flags, \
                                                          DefaultPropertyTableCallbacks::GetFloat3, (Offset), \
                                                          DefaultPropertyTableCallbacks::SetFloat3, (Offset), \
                                                          ChangedFunc, ChangedFuncUserData)

#define PROPERTY_TABLE_MEMBER_FLOAT4(Name, Flags, Offset, ChangedFunc, ChangedFuncUserData)  \
                                     PROPERTY_TABLE_MEMBER(Name, PROPERTY_TYPE_FLOAT4, Flags, \
                                                          DefaultPropertyTableCallbacks::GetFloat4, (Offset), \
                                                          DefaultPropertyTableCallbacks::SetFloat4, (Offset), \
                                                          ChangedFunc, ChangedFuncUserData)

#define PROPERTY_TABLE_MEMBER_QUATERNION(Name, Flags, Offset, ChangedFunc, ChangedFuncUserData)  \
                                         PROPERTY_TABLE_MEMBER(Name, PROPERTY_TYPE_QUATERNION, Flags, \
                                                           DefaultPropertyTableCallbacks::GetQuaternion, (Offset), \
                                                           DefaultPropertyTableCallbacks::SetQuaternion, (Offset), \
                                                           ChangedFunc, ChangedFuncUserData)

#define PROPERTY_TABLE_MEMBER_TRANSFORM(Name, Flags, Offset, ChangedFunc, ChangedFuncUserData)  \
                                        PROPERTY_TABLE_MEMBER(Name, PROPERTY_TYPE_TRANSFORM, Flags, \
                                                              DefaultPropertyTableCallbacks::GetTransform, (Offset), \
                                                              DefaultPropertyTableCallbacks::SetTransform, (Offset), \
                                                              ChangedFunc, ChangedFuncUserData)

#define PROPERTY_TABLE_MEMBER_COLOR(Name, Flags, Offset, ChangedFunc, ChangedFuncUserData)  \
                                    PROPERTY_TABLE_MEMBER(Name, PROPERTY_TYPE_COLOR, Flags, \
                                                          DefaultPropertyTableCallbacks::GetColor, (Offset), \
                                                          DefaultPropertyTableCallbacks::SetColor, (Offset), \
                                                          ChangedFunc, ChangedFuncUserData)



