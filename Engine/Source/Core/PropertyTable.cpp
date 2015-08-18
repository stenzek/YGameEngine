#include "Core/PrecompiledHeader.h"
#include "Core/PropertyTable.h"
#include "Core/PropertyTemplate.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
#include "YBaseLib/StringConverter.h"
#include "MathLib/StringConverters.h"

PropertyTable::PropertyTable()
{

}

PropertyTable::PropertyTable(const PropertyTable &copy)
{
    for (PropertyHashTable::ConstIterator itr = copy.m_properties.Begin(); !itr.AtEnd(); itr.Forward())
        m_properties.Insert(itr->Key, itr->Value);
}

PropertyTable::~PropertyTable()
{

}

bool PropertyTable::GetPropertyValue(const char *propertyName, String *pValue) const
{
    const PropertyHashTable::Member *pMember = m_properties.Find(propertyName);
    if (pMember != nullptr)
    {
        pValue->Assign(pMember->Value);
        return true;
    }

    return false;
}

const String *PropertyTable::GetPropertyValuePointer(const char *propertyName) const
{
    const PropertyHashTable::Member *pMember = m_properties.Find(propertyName);
    return (pMember != nullptr) ? &pMember->Value : nullptr;
}

const char *PropertyTable::GetPropertyValueDefault(const char *propertyName, const char *defaultValue /*= ""*/) const
{
    const PropertyHashTable::Member *pMember = m_properties.Find(propertyName);
    return (pMember != nullptr) ? pMember->Value.GetCharArray() : defaultValue;
}

const String &PropertyTable::GetPropertyValueDefaultString(const char *propertyName, const String &defaultValue /*= EmptyString*/) const
{
    const PropertyHashTable::Member *pMember = m_properties.Find(propertyName);
    return (pMember != nullptr) ? pMember->Value : defaultValue;
}

const char *PropertyTable::InternalGetPropertyValue(const char *PropertyName) const
{
    const PropertyHashTable::Member *pMember = m_properties.Find(PropertyName);
    return (pMember != nullptr) ? pMember->Value.GetCharArray() : nullptr;
}

String PropertyTable::InternalGetPropertyValueString(const char *PropertyName) const
{
    const PropertyHashTable::Member *pMember = m_properties.Find(PropertyName);
    return (pMember != nullptr) ? pMember->Value : EmptyString;
}

void PropertyTable::SetPropertyValue(const char *PropertyName, const char *Value)
{
    PropertyHashTable::Member *pMember = m_properties.Find(PropertyName);
    if (pMember != nullptr)
    {
        pMember->Value = Value;
        return;
    }

    m_properties.Insert(PropertyName, Value);
}

void PropertyTable::SetPropertyValueString(const char *PropertyName, const String &Value)
{
    PropertyHashTable::Member *pMember = m_properties.Find(PropertyName);
    if (pMember != nullptr)
    {
        pMember->Value = Value;
        return;
    }

    m_properties.Insert(PropertyName, Value);
}

void PropertyTable::Create()
{

}

void PropertyTable::CreateFromTemplate(const PropertyTemplate *pTemplate)
{
    for (uint32 i = 0; i < pTemplate->GetPropertyDefinitionCount(); i++)
    {
        const PropertyTemplateProperty *pProperty = pTemplate->GetPropertyDefinition(i);
        SetPropertyValueString(pProperty->GetName(), pProperty->GetDefaultValue());
    }
}

void PropertyTable::CopyProperties(const PropertyTable *pTable)
{
    for (PropertyHashTable::ConstIterator itr = pTable->m_properties.Begin(); !itr.AtEnd(); itr.Forward())
        SetPropertyValueString(itr->Key, itr->Value);
}

void PropertyTable::Clear()
{
    m_properties.Clear();
}

#if defined(HAVE_LIBXML2)

bool PropertyTable::LoadFromXML(XMLReader &xmlReader)
{
    // read fields
    if (!xmlReader.IsEmptyElement())
    {
        for (;;)
        {
            if (!xmlReader.NextToken())
                return false;

            int32 objectSelection;
            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                objectSelection = xmlReader.Select("property");
                if (objectSelection < 0)
                    return false;

                switch (objectSelection)
                {
                    // property
                case 0:
                    {
                        const char *propertyName = xmlReader.FetchAttribute("name");
                        const char *propertyValue = xmlReader.FetchAttribute("value");
                        if (propertyName == NULL || propertyValue == NULL)
                        {
                            xmlReader.PrintError("incomplete property definition");
                            return false;
                        }

                        SetPropertyValue(propertyName, propertyValue);
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                break;
            }
        }
    }

    return true;
}

void PropertyTable::SaveToXML(XMLWriter &xmlWriter) const
{
    // write properties
    for (PropertyHashTable::ConstIterator pitr = m_properties.Begin(); !pitr.AtEnd(); pitr.Forward())
    {
        xmlWriter.StartElement("property");
        xmlWriter.WriteAttribute("name", pitr->Key);
        xmlWriter.WriteAttribute("value", pitr->Value);
        xmlWriter.EndElement();
    }
}

#endif       // WITH_LIBXML2

bool PropertyTable::GetPropertyValueBool(const char *propertyName, bool *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToBool(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueInt8(const char *propertyName, int8 *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToInt8(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueInt16(const char *propertyName, int16 *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToInt16(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueInt32(const char *propertyName, int32 *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToInt32(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueInt64(const char *propertyName, int64 *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToInt64(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueUInt8(const char *propertyName, uint8 *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToUInt8(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueUInt16(const char *propertyName, uint16 *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToUInt16(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueUInt32(const char *propertyName, uint32 *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToUInt32(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueUInt64(const char *propertyName, uint64 *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToUInt64(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueFloat(const char *propertyName, float *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToFloat(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueDouble(const char *propertyName, double *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToDouble(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueColor(const char *propertyName, uint32 *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToColor(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueInt2(const char *propertyName, Vector2i *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToVector2i(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueInt3(const char *propertyName, Vector3i *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToVector3i(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueInt4(const char *propertyName, Vector4i *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToVector4i(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueUInt2(const char *propertyName, Vector2u *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToVector2u(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueUInt3(const char *propertyName, Vector3u *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToVector3u(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueUInt4(const char *propertyName, Vector4u *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToVector4u(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueFloat2(const char *propertyName, Vector2f *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToVector2f(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueFloat3(const char *propertyName, Vector3f *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToVector3f(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueFloat4(const char *propertyName, Vector4f *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToVector4f(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueQuaternion(const char *propertyName, Quaternion *pValue) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    if (propertyValue != NULL)
    {
        *pValue = StringConverter::StringToQuaternion(propertyValue);
        return true;
    }

    return false;
}

bool PropertyTable::GetPropertyValueDefaultBool(const char *propertyName, bool defaultValue /* = false */) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToBool(propertyValue) : defaultValue;
}

int8 PropertyTable::GetPropertyValueDefaultInt8(const char *propertyName, int8 defaultValue /*= 0*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToInt8(propertyValue) : defaultValue;
}

int16 PropertyTable::GetPropertyValueDefaultInt16(const char *propertyName, int16 defaultValue /*= 0*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToInt16(propertyValue) : defaultValue;
}

int32 PropertyTable::GetPropertyValueDefaultInt32(const char *propertyName, int32 defaultValue /*= 0*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToInt32(propertyValue) : defaultValue;
}

int64 PropertyTable::GetPropertyValueDefaultInt64(const char *propertyName, int64 defaultValue /*= 0*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToInt64(propertyValue) : defaultValue;
}

uint8 PropertyTable::GetPropertyValueDefaultUInt8(const char *propertyName, uint8 defaultValue /*= 0*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToUInt8(propertyValue) : defaultValue;
}

uint16 PropertyTable::GetPropertyValueDefaultUInt16(const char *propertyName, uint16 defaultValue /*= 0*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToUInt16(propertyValue) : defaultValue;
}

uint32 PropertyTable::GetPropertyValueDefaultUInt32(const char *propertyName, uint32 defaultValue /*= 0*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToUInt32(propertyValue) : defaultValue;
}

uint64 PropertyTable::GetPropertyValueDefaultUInt64(const char *propertyName, uint64 defaultValue /*= 0*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToUInt64(propertyValue) : defaultValue;
}

float PropertyTable::GetPropertyValueDefaultFloat(const char *propertyName, float defaultValue /*= 0*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToFloat(propertyValue) : defaultValue;
}

double PropertyTable::GetPropertyValueDefaultDouble(const char *propertyName, double defaultValue /*= 0*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToDouble(propertyValue) : defaultValue;
}

uint32 PropertyTable::GetPropertyValueDefaultColor(const char *propertyName, uint32 defaultValue /*= 0*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToColor(propertyValue) : defaultValue;
}

Vector2i PropertyTable::GetPropertyValueDefaultInt2(const char *propertyName, const Vector2i &defaultValue /*= int2::Zero*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToVector2i(propertyValue) : defaultValue;
}

Vector3i PropertyTable::GetPropertyValueDefaultInt3(const char *propertyName, const Vector3i &defaultValue /*= int3::Zero*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToVector3i(propertyValue) : defaultValue;
}

Vector4i PropertyTable::GetPropertyValueDefaultInt4(const char *propertyName, const Vector4i &defaultValue /*= int4::Zero*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToVector4i(propertyValue) : defaultValue;
}

Vector2u PropertyTable::GetPropertyValueDefaultUInt2(const char *propertyName, const Vector2u &defaultValue /*= uint2::Zero*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToVector2u(propertyValue) : defaultValue;
}

Vector3u PropertyTable::GetPropertyValueDefaultUInt3(const char *propertyName, const Vector3u &defaultValue /*= uint3::Zero*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToVector3u(propertyValue) : defaultValue;
}

Vector4u PropertyTable::GetPropertyValueDefaultUInt4(const char *propertyName, const Vector4u &defaultValue /*= uint4::Zero*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToVector4u(propertyValue) : defaultValue;
}

Vector2f PropertyTable::GetPropertyValueDefaultFloat2(const char *propertyName, const Vector2f &defaultValue /*= float2::Zero*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToVector2f(propertyValue) : defaultValue;
}

Vector3f PropertyTable::GetPropertyValueDefaultFloat3(const char *propertyName, const Vector3f &defaultValue /*= float3::Zero*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToVector3f(propertyValue) : defaultValue;
}

Vector4f PropertyTable::GetPropertyValueDefaultFloat4(const char *propertyName, const Vector4f &defaultValue /*= float4::Zero*/) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToVector4f(propertyValue) : defaultValue;
}

Quaternion PropertyTable::GetPropertyValueDefaultQuaternion(const char *propertyName, const Quaternion &defaultValue /* = Quaternion::Identity */) const
{
    const char *propertyValue = InternalGetPropertyValue(propertyName);
    return (propertyValue != NULL) ? StringConverter::StringToQuaternion(propertyValue) : defaultValue;
}

// typed property writers
void PropertyTable::SetPropertyValueBool(const char *propertyName, bool propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::BoolToString(propertyValue));
}

void PropertyTable::SetPropertyValueInt8(const char *propertyName, int8 propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::Int8ToString(propertyValue));
}

void PropertyTable::SetPropertyValueInt16(const char *propertyName, int16 propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::Int16ToString(propertyValue));
}

void PropertyTable::SetPropertyValueInt32(const char *propertyName, int32 propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::Int32ToString(propertyValue));
}

void PropertyTable::SetPropertyValueInt64(const char *propertyName, int64 propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::Int64ToString(propertyValue));
}

void PropertyTable::SetPropertyValueUInt8(const char *propertyName, uint8 propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::UInt8ToString(propertyValue));
}

void PropertyTable::SetPropertyValueUInt16(const char *propertyName, uint16 propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::UInt16ToString(propertyValue));
}

void PropertyTable::SetPropertyValueUInt32(const char *propertyName, uint32 propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::UInt32ToString(propertyValue));
}

void PropertyTable::SetPropertyValueUInt64(const char *propertyName, uint64 propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::UInt64ToString(propertyValue));
}

void PropertyTable::SetPropertyValueFloat(const char *propertyName, float propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::FloatToString(propertyValue));
}

void PropertyTable::SetPropertyValueDouble(const char *propertyName, double propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::DoubleToString(propertyValue));
}

void PropertyTable::SetPropertyValueColor(const char *propertyName, uint32 propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::ColorToString(propertyValue));
}

void PropertyTable::SetPropertyValueInt2(const char *propertyName, const Vector2i &propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::Vector2iToString(propertyValue));
}

void PropertyTable::SetPropertyValueInt3(const char *propertyName, const Vector3i &propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::Vector3iToString(propertyValue));
}

void PropertyTable::SetPropertyValueInt4(const char *propertyName, const Vector4i &propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::Vector4iToString(propertyValue));
}

void PropertyTable::SetPropertyValueUInt2(const char *propertyName, const Vector2u &propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::Vector2uToString(propertyValue));
}

void PropertyTable::SetPropertyValueUInt3(const char *propertyName, const Vector3u &propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::Vector3uToString(propertyValue));
}

void PropertyTable::SetPropertyValueUInt4(const char *propertyName, const Vector4u &propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::Vector4uToString(propertyValue));
}

void PropertyTable::SetPropertyValueFloat2(const char *propertyName, const Vector2f &propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::Vector2fToString(propertyValue));
}

void PropertyTable::SetPropertyValueFloat3(const char *propertyName, const Vector3f &propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::Vector3fToString(propertyValue));
}

void PropertyTable::SetPropertyValueFloat4(const char *propertyName, const Vector4f &propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::Vector4fToString(propertyValue));
}

void PropertyTable::SetPropertyValueQuaternion(const char *propertyName, const Quaternion &propertyValue)
{
    SetPropertyValue(propertyName, StringConverter::QuaternionToString(propertyValue));
}

PropertyTable &PropertyTable::operator=(const PropertyTable &copy)
{
    m_properties.Clear();

    for (PropertyHashTable::ConstIterator itr = copy.m_properties.Begin(); !itr.AtEnd(); itr.Forward())
        m_properties.Insert(itr->Key, itr->Value);

    return *this;
}

static PropertyTable s_emptyPropertyList;
const PropertyTable &PropertyTable::EmptyPropertyList = s_emptyPropertyList;
