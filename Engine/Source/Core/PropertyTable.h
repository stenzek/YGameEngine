#pragma once
#include "Core/Common.h"
#include "YBaseLib/String.h"
#include "YBaseLib/CIStringHashTable.h"
#include "MathLib/Vectorf.h"
#include "MathLib/Vectori.h"
#include "MathLib/Vectoru.h"
#include "MathLib/Quaternion.h"

class PropertyTemplate;
class XMLReader;
class XMLWriter;

class PropertyTable
{
public:
    typedef CIStringHashTable<String> PropertyHashTable;

public:
    PropertyTable();
    PropertyTable(const PropertyTable &copy);
    ~PropertyTable();

    const PropertyHashTable &GetPropertyHashTable() const { return m_properties; }

    bool GetPropertyValue(const char *propertyName, String *pValue) const;
    const String *GetPropertyValuePointer(const char *propertyName) const;
    const char *GetPropertyValueDefault(const char *propertyName, const char *defaultValue = "") const;
    const String &GetPropertyValueDefaultString(const char *propertyName, const String &defaultValue = EmptyString) const;

    void SetPropertyValue(const char *PropertyName, const char *Value);
    void SetPropertyValueString(const char *PropertyName, const String &Value);

    void Create();
    void CreateFromTemplate(const PropertyTemplate *pTemplate);
    void CopyProperties(const PropertyTable *pTable);
    void Clear();

#ifdef HAVE_LIBXML2
    bool LoadFromXML(XMLReader &xmlReader);
    void SaveToXML(XMLWriter &xmlWriter) const;
#endif

    // typed property readers
    bool GetPropertyValueBool(const char *propertyName, bool *pValue) const;
    bool GetPropertyValueInt8(const char *propertyName, int8 *pValue) const;
    bool GetPropertyValueInt16(const char *propertyName, int16 *pValue) const;
    bool GetPropertyValueInt32(const char *propertyName, int32 *pValue) const;
    bool GetPropertyValueInt64(const char *propertyName, int64 *pValue) const;
    bool GetPropertyValueUInt8(const char *propertyName, uint8 *pValue) const;
    bool GetPropertyValueUInt16(const char *propertyName, uint16 *pValue) const;
    bool GetPropertyValueUInt32(const char *propertyName, uint32 *pValue) const;
    bool GetPropertyValueUInt64(const char *propertyName, uint64 *pValue) const;
    bool GetPropertyValueFloat(const char *propertyName, float *pValue) const;
    bool GetPropertyValueDouble(const char *propertyName, double *pValue) const;
    bool GetPropertyValueColor(const char *propertyName, uint32 *pValue) const;
    bool GetPropertyValueInt2(const char *propertyName, Vector2i *pValue) const;
    bool GetPropertyValueInt3(const char *propertyName, Vector3i *pValue) const;
    bool GetPropertyValueInt4(const char *propertyName, Vector4i *pValue) const;
    bool GetPropertyValueUInt2(const char *propertyName, Vector2u *pValue) const;
    bool GetPropertyValueUInt3(const char *propertyName, Vector3u *pValue) const;
    bool GetPropertyValueUInt4(const char *propertyName, Vector4u *pValue) const;
    bool GetPropertyValueFloat2(const char *propertyName, Vector2f *pValue) const;
    bool GetPropertyValueFloat3(const char *propertyName, Vector3f *pValue) const;
    bool GetPropertyValueFloat4(const char *propertyName, Vector4f *pValue) const;
    bool GetPropertyValueQuaternion(const char *propertyName, Quaternion *pValue) const;

    // typed property readers with fallback values
    bool GetPropertyValueDefaultBool(const char *propertyName, bool defaultValue = false) const;
    int8 GetPropertyValueDefaultInt8(const char *propertyName, int8 defaultValue = 0) const;
    int16 GetPropertyValueDefaultInt16(const char *propertyName, int16 defaultValue = 0) const;
    int32 GetPropertyValueDefaultInt32(const char *propertyName, int32 defaultValue = 0) const;
    int64 GetPropertyValueDefaultInt64(const char *propertyName, int64 defaultValue = 0) const;
    uint8 GetPropertyValueDefaultUInt8(const char *propertyName, uint8 defaultValue = 0) const;
    uint16 GetPropertyValueDefaultUInt16(const char *propertyName, uint16 defaultValue = 0) const;
    uint32 GetPropertyValueDefaultUInt32(const char *propertyName, uint32 defaultValue = 0) const;
    uint64 GetPropertyValueDefaultUInt64(const char *propertyName, uint64 defaultValue = 0) const;
    float GetPropertyValueDefaultFloat(const char *propertyName, float defaultValue = 0) const;
    double GetPropertyValueDefaultDouble(const char *propertyName, double defaultValue = 0) const;
    uint32 GetPropertyValueDefaultColor(const char *propertyName, uint32 defaultValue = 0) const;
    Vector2i GetPropertyValueDefaultInt2(const char *propertyName, const Vector2i &defaultValue = Vector2i::Zero) const;
    Vector3i GetPropertyValueDefaultInt3(const char *propertyName, const Vector3i &defaultValue = Vector3i::Zero) const;
    Vector4i GetPropertyValueDefaultInt4(const char *propertyName, const Vector4i &defaultValue = Vector4i::Zero) const;
    Vector2u GetPropertyValueDefaultUInt2(const char *propertyName, const Vector2u &defaultValue = Vector2u::Zero) const;
    Vector3u GetPropertyValueDefaultUInt3(const char *propertyName, const Vector3u &defaultValue = Vector3u::Zero) const;
    Vector4u GetPropertyValueDefaultUInt4(const char *propertyName, const Vector4u &defaultValue = Vector4u::Zero) const;
    Vector2f GetPropertyValueDefaultFloat2(const char *propertyName, const Vector2f &defaultValue = Vector2f::Zero) const;
    Vector3f GetPropertyValueDefaultFloat3(const char *propertyName, const Vector3f &defaultValue = Vector3f::Zero) const;
    Vector4f GetPropertyValueDefaultFloat4(const char *propertyName, const Vector4f &defaultValue = Vector4f::Zero) const;
    Quaternion GetPropertyValueDefaultQuaternion(const char *propertyName, const Quaternion &defaultValue = Quaternion::Identity) const;

    // typed property writers
    void SetPropertyValueBool(const char *propertyName, bool propertyValue);
    void SetPropertyValueInt8(const char *propertyName, int8 propertyValue);
    void SetPropertyValueInt16(const char *propertyName, int16 propertyValue);
    void SetPropertyValueInt32(const char *propertyName, int32 propertyValue);
    void SetPropertyValueInt64(const char *propertyName, int64 propertyValue);
    void SetPropertyValueUInt8(const char *propertyName, uint8 propertyValue);
    void SetPropertyValueUInt16(const char *propertyName, uint16 propertyValue);
    void SetPropertyValueUInt32(const char *propertyName, uint32 propertyValue);
    void SetPropertyValueUInt64(const char *propertyName, uint64 propertyValue);
    void SetPropertyValueFloat(const char *propertyName, float propertyValue);
    void SetPropertyValueDouble(const char *propertyName, double propertyValue);
    void SetPropertyValueColor(const char *propertyName, uint32 propertyValue);
    void SetPropertyValueInt2(const char *propertyName, const Vector2i &propertyValue);
    void SetPropertyValueInt3(const char *propertyName, const Vector3i &propertyValue);
    void SetPropertyValueInt4(const char *propertyName, const Vector4i &propertyValue);
    void SetPropertyValueUInt2(const char *propertyName, const Vector2u &propertyValue);
    void SetPropertyValueUInt3(const char *propertyName, const Vector3u &propertyValue);
    void SetPropertyValueUInt4(const char *propertyName, const Vector4u &propertyValue);
    void SetPropertyValueFloat2(const char *propertyName, const Vector2f &propertyValue);
    void SetPropertyValueFloat3(const char *propertyName, const Vector3f &propertyValue);
    void SetPropertyValueFloat4(const char *propertyName, const Vector4f &propertyValue);
    void SetPropertyValueQuaternion(const char *propertyName, const Quaternion &propertyValue);

    // property enumerator, callback in form of func(const String &, const String &), or func(const char *, const char *)
    template<class T>
    void EnumerateProperties(T callback) const
    {
        for (PropertyHashTable::ConstIterator itr = m_properties.Begin(); !itr.AtEnd(); itr.Forward())
            callback(itr->Key, itr->Value);
    }

    // assignment operator
    PropertyTable &operator=(const PropertyTable &copy);

public:
    static const PropertyTable &EmptyPropertyList;

private:
    const char *InternalGetPropertyValue(const char *PropertyName) const;
    String InternalGetPropertyValueString(const char *PropertyName) const;

    PropertyHashTable m_properties;
};
