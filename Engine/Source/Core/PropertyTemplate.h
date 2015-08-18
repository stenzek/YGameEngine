#pragma once
#include "Core/Common.h"
#include "Core/Property.h"
#include "YBaseLib/Pair.h"
#include "YBaseLib/Array.h"
#include "YBaseLib/PODArray.h"

class ByteStream;
class XMLReader;

class PropertyTemplate;
class PropertyTemplateProperty;
class PropertyTemplateValueSelector;

enum PROPERTY_TEMPLATE_PROPERTY_FLAGS
{
    PROPERTY_TEMPLATE_PROPERTY_FLAG_HIDDEN                       = (1 << 0),
    PROPERTY_TEMPLATE_PROPERTY_FLAG_READONLY                     = (1 << 1),
};

enum PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE
{
    PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_RESOURCE,
    PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_COLOR,
    PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_SLIDER,
    PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_SPINNER,
    PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_CHOICE,
    PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_EULER_ANGLES,
    PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_FLAGS,
    PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_COUNT,
};

class PropertyTemplate
{
public:
    PropertyTemplate();
    PropertyTemplate(const PropertyTemplate &from);
    ~PropertyTemplate();

    void AddPropertyDefinition(PropertyTemplateProperty *pProperty);
    const PropertyTemplateProperty *GetPropertyDefinition(uint32 i) const { DebugAssert(i < m_propertyDefinitions.GetSize()); return m_propertyDefinitions[i]; }
    const PropertyTemplateProperty *GetPropertyDefinitionByName(const char *propertyName) const;
    const uint32 GetPropertyDefinitionCount() const { return m_propertyDefinitions.GetSize(); }
    
    template<class T>
    void EnumerateProperties(T callback) const
    {
        for (uint32 i = 0; i < m_propertyDefinitions.GetSize(); i++)
            callback(m_propertyDefinitions[i]);
    }

#ifdef HAVE_LIBXML2
    bool LoadFromStream(const char *FileName, ByteStream *pStream);
    bool LoadFromXML(XMLReader &xmlReader);
#endif

private:
    typedef PODArray<PropertyTemplateProperty *> PropertyDefinitionArray;
    PropertyDefinitionArray m_propertyDefinitions;
};

class PropertyTemplateProperty
{
public:
    PropertyTemplateProperty();
    ~PropertyTemplateProperty();

    const PROPERTY_TYPE GetType() const { return m_eType; }
    const String &GetName() const { return m_strName; }

    const uint32 GetFlags() const { return m_flags; }
    
    const String &GetCategory() const { return m_strCategory; }
    const String &GetLabel() const { return m_strLabel; }
    const String &GetDescription() const { return m_strDescription; }
    const String &GetDefaultValue() const { return m_strDefaultValue; }

    const PropertyTemplateValueSelector *GetSelector() const { return m_pSelector; }

    PropertyTemplateProperty *Clone() const;

#ifdef HAVE_LIBXML2
    bool ParseXML(XMLReader &xmlReader);
#endif

private:
    PROPERTY_TYPE m_eType;
    String m_strName;
    uint32 m_flags;

    String m_strCategory;
    String m_strLabel;
    String m_strDescription;
    String m_strDefaultValue;
    
    PropertyTemplateValueSelector *m_pSelector;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PropertyTemplateValueSelector
{
public:
    PropertyTemplateValueSelector(const PropertyTemplateProperty *pOwner, PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE type);
    virtual ~PropertyTemplateValueSelector();

    const PropertyTemplateProperty *GetOwner() const { return m_pOwner; }
    const PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE GetType() const { return m_eType; }

    virtual PropertyTemplateValueSelector *Clone(const PropertyTemplateProperty *pOwner) = 0;

#ifdef HAVE_LIBXML2
    virtual bool ParseXML(XMLReader &xmlReader) = 0;
#endif

private:
    const PropertyTemplateProperty *m_pOwner;
    PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE m_eType;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PropertyTemplateValueSelector_Resource : public PropertyTemplateValueSelector
{
public:
    PropertyTemplateValueSelector_Resource(const PropertyTemplateProperty *pOwner);
    ~PropertyTemplateValueSelector_Resource();

    const String &GetResourceTypeName() const { return m_strResourceTypeName; }

    virtual PropertyTemplateValueSelector *Clone(const PropertyTemplateProperty *pOwner) override final;

#ifdef HAVE_LIBXML2
    virtual bool ParseXML(XMLReader &xmlReader) override final;
#endif

private:
    String m_strResourceTypeName;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PropertyTemplateValueSelector_Color : public PropertyTemplateValueSelector
{
public:
    PropertyTemplateValueSelector_Color(const PropertyTemplateProperty *pOwner);
    ~PropertyTemplateValueSelector_Color();

    virtual PropertyTemplateValueSelector *Clone(const PropertyTemplateProperty *pOwner) override final;

#ifdef HAVE_LIBXML2
    virtual bool ParseXML(XMLReader &xmlReader) override final;
#endif
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PropertyTemplateValueSelector_Slider : public PropertyTemplateValueSelector
{
public:
    PropertyTemplateValueSelector_Slider(const PropertyTemplateProperty *pOwner);
    ~PropertyTemplateValueSelector_Slider();

    const float GetMinValue() const { return m_fMinValue; }
    const float GetMaxValue() const { return m_fMaxValue; }

    virtual PropertyTemplateValueSelector *Clone(const PropertyTemplateProperty *pOwner) override final;

#ifdef HAVE_LIBXML2
    virtual bool ParseXML(XMLReader &xmlReader) override final;
#endif

private:
    float m_fMinValue;
    float m_fMaxValue;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PropertyTemplateValueSelector_Spinner : public PropertyTemplateValueSelector
{
public:
    PropertyTemplateValueSelector_Spinner(const PropertyTemplateProperty *pOwner);
    ~PropertyTemplateValueSelector_Spinner();

    const float GetMinValue() const { return m_fMinValue; }
    const float GetIncrement() const { return m_fIncrement; }
    const float GetMaxValue() const { return m_fMaxValue; }

    virtual PropertyTemplateValueSelector *Clone(const PropertyTemplateProperty *pOwner) override final;

#ifdef HAVE_LIBXML2
    virtual bool ParseXML(XMLReader &xmlReader) override final;
#endif

private:
    float m_fMinValue;
    float m_fIncrement;
    float m_fMaxValue;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PropertyTemplateValueSelector_Choice : public PropertyTemplateValueSelector
{
public:
    typedef Pair<String, String> Choice;

public:
    PropertyTemplateValueSelector_Choice(const PropertyTemplateProperty *pOwner);
    ~PropertyTemplateValueSelector_Choice();

    const Choice &GetChoice(uint32 i) const { DebugAssert(i < m_Choices.GetSize()); return m_Choices[i]; }
    const uint32 GetChoiceCount() const { return m_Choices.GetSize(); }

    virtual PropertyTemplateValueSelector *Clone(const PropertyTemplateProperty *pOwner) override final;

#ifdef HAVE_LIBXML2
    virtual bool ParseXML(XMLReader &xmlReader) override final;
#endif

private:
    Array<Choice> m_Choices;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PropertyTemplateValueSelector_EulerAngles : public PropertyTemplateValueSelector
{
public:
    PropertyTemplateValueSelector_EulerAngles(const PropertyTemplateProperty *pOwner);
    ~PropertyTemplateValueSelector_EulerAngles();

    virtual PropertyTemplateValueSelector *Clone(const PropertyTemplateProperty *pOwner) override final;

#ifdef HAVE_LIBXML2
    virtual bool ParseXML(XMLReader &xmlReader) override final;
#endif
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PropertyTemplateValueSelector_Flags : public PropertyTemplateValueSelector
{
public:
    struct Flag
    {
        uint32 Value;
        String Label;
        String Description;
    };

public:
    PropertyTemplateValueSelector_Flags(const PropertyTemplateProperty *pOwner);
    ~PropertyTemplateValueSelector_Flags();

    const Flag &GetFlag(uint32 i) const { DebugAssert(i < m_Flags.GetSize()); return m_Flags[i]; }
    const uint32 GetFlagCount() const { return m_Flags.GetSize(); }

    virtual PropertyTemplateValueSelector *Clone(const PropertyTemplateProperty *pOwner) override final;

#ifdef HAVE_LIBXML2
    virtual bool ParseXML(XMLReader &xmlReader) override final;
#endif

private:
    Array<Flag> m_Flags;
};
