#include "Core/PrecompiledHeader.h"
#include "Core/PropertyTemplate.h"
#include "YBaseLib/StringConverter.h"
#include "YBaseLib/XMLReader.h"

PropertyTemplate::PropertyTemplate()
{

}

PropertyTemplate::PropertyTemplate(const PropertyTemplate &from)
{
    for (uint32 i = 0; i < from.m_propertyDefinitions.GetSize(); i++)
    {
        PropertyTemplateProperty *pPropertyClone = from.m_propertyDefinitions[i]->Clone();
        DebugAssert(pPropertyClone != nullptr);
        m_propertyDefinitions.Add(pPropertyClone);
    }
}

PropertyTemplate::~PropertyTemplate()
{
    for (uint32 i = 0; i < m_propertyDefinitions.GetSize(); i++)
        delete m_propertyDefinitions[i];
}

const PropertyTemplateProperty *PropertyTemplate::GetPropertyDefinitionByName(const char *propertyName) const
{
    for (uint32 i = 0; i < m_propertyDefinitions.GetSize(); i++)
    {
        if (m_propertyDefinitions[i]->GetName().CompareInsensitive(propertyName))
            return m_propertyDefinitions[i];
    }

    return NULL;
}

void PropertyTemplate::AddPropertyDefinition(PropertyTemplateProperty *pProperty)
{
    DebugAssert(GetPropertyDefinitionByName(pProperty->GetName()) == nullptr);
    m_propertyDefinitions.Add(pProperty);
}

#ifdef HAVE_LIBXML2

bool PropertyTemplate::LoadFromStream(const char *FileName, ByteStream *pStream)
{
    // create xml reader
    XMLReader xmlReader;
    if (!xmlReader.Create(pStream, FileName))
    {
        xmlReader.PrintError("could not create xml reader");
        return false;
    }

    // skip to entity-definition element
    if (!xmlReader.SkipToElement("propertytemplate"))
    {
        xmlReader.PrintError("could not skip to propertytemplate element");
        return false;
    }

    if (!LoadFromXML(xmlReader))
        return false;

    DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "propertytemplate") == 0);
    return true;
}

bool PropertyTemplate::LoadFromXML(XMLReader &xmlReader)
{
    // read elements
    if (!xmlReader.IsEmptyElement())
    {
        for (; ;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 propertiesSelection = xmlReader.Select("property");
                if (propertiesSelection < 0)
                    return false;

                switch (propertiesSelection)
                {
                    // property
                case 0:
                    {
                        PropertyTemplateProperty *pPropertyDefinition = new PropertyTemplateProperty();
                        if (!pPropertyDefinition->ParseXML(xmlReader))
                        {
                            delete pPropertyDefinition;
                            return false;
                        }

                        // if it already exists, allow the override
                        for (uint32 i = 0; i < m_propertyDefinitions.GetSize(); i++)
                        {
                            if (m_propertyDefinitions[i]->GetName() == pPropertyDefinition->GetName())
                            {
                                xmlReader.PrintError("property already defined: '%s'", pPropertyDefinition->GetName().GetCharArray());
                                delete pPropertyDefinition;
                                return false;
                            }
                        }

                        m_propertyDefinitions.Add(pPropertyDefinition);
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
            else
            {
                UnreachableCode();
                break;
            }
        }
    }

    return true;
}

#endif

PropertyTemplateProperty::PropertyTemplateProperty()
    : m_eType(PROPERTY_TYPE_COUNT),
      m_flags(0),
      m_pSelector(NULL)
{

}

PropertyTemplateProperty::~PropertyTemplateProperty()
{
    if (m_pSelector != NULL)
        delete m_pSelector;
}

#ifdef HAVE_LIBXML2

bool PropertyTemplateProperty::ParseXML(XMLReader &xmlReader)
{
    // we should be on a property element.
    DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "property") == 0);

    // read attributes
    const char *typeStr = xmlReader.FetchAttribute("type");
    const char *nameStr = xmlReader.FetchAttribute("name");
    if (typeStr == NULL || nameStr == NULL)
    {
        xmlReader.PrintError("incomplete property declaration");
        return false;
    }

    // convert type, set name
    m_strName = nameStr;
    if (!NameTable_TranslateType(NameTables::PropertyType, typeStr, &m_eType, true))
    {
        xmlReader.PrintError("invalid property type: '%s'", typeStr);
        return false;
    }

    // parse fields
    if (!xmlReader.IsEmptyElement())
    {
        for (; ;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 propertySelection = xmlReader.Select("flags|category|label|description|default|selector");
                if (propertySelection < 0)
                    return false;

                switch (propertySelection)
                {
                    // flags
                case 0:
                    {
                        uint32 flags = 0;
                        if (!xmlReader.IsEmptyElement())
                        {
                            for (;;)
                            {
                                if (!xmlReader.NextToken())
                                    return false;

                                if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                {
                                    int32 flagsSelection = xmlReader.Select("hidden|readonly");
                                    if (flagsSelection < 0)
                                        return false;

                                    switch (flagsSelection)
                                    {
                                        // hidden
                                    case 0:
                                        flags |= PROPERTY_TEMPLATE_PROPERTY_FLAG_HIDDEN;
                                        break;

                                        // readonly
                                    case 1:
                                        flags |= PROPERTY_TEMPLATE_PROPERTY_FLAG_READONLY;
                                        break;
                                    }
                                }
                                else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                {
                                    DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "flags") == 0);
                                    break;
                                }
                                else
                                {
                                    UnreachableCode();
                                    break;
                                }
                            }
                        }

                        m_flags = flags;
                    }
                    break;

                    // category
                case 1:
                    m_strCategory = xmlReader.GetElementText(true);
                    break;

                    // label
                case 2:
                    m_strLabel = xmlReader.GetElementText(true);
                    break;

                    // description
                case 3:
                    m_strDescription = xmlReader.GetElementText(true);
                    break;

                    // default
                case 4:
                    m_strDefaultValue = xmlReader.GetElementText(true);
                    break;

                    // selector
                case 5:
                    {
                        if (m_pSelector != NULL)
                        {
                            xmlReader.PrintError("selector already defined");
                            return false;
                        }

                        int32 selectorSelection = xmlReader.SelectAttribute("type", "resource|color|slider|spinner|choice|euler-angles|flags");
                        if (selectorSelection < 0)
                            return false;

                        PropertyTemplateValueSelector *pSelector = NULL;
                        switch (selectorSelection)
                        {
                            // resource
                        case 0:
                            pSelector = new PropertyTemplateValueSelector_Resource(this);
                            break;

                            // color
                        case 1:
                            pSelector = new PropertyTemplateValueSelector_Color(this);
                            break;

                            // slider
                        case 2:
                            pSelector = new PropertyTemplateValueSelector_Slider(this);
                            break;

                            // spinner
                        case 3:
                            pSelector = new PropertyTemplateValueSelector_Spinner(this);
                            break;

                            // choice
                        case 4:
                            pSelector = new PropertyTemplateValueSelector_Choice(this);
                            break;

                            // euler-angles
                        case 5:
                            pSelector = new PropertyTemplateValueSelector_EulerAngles(this);
                            break;

                            // flags
                        case 6:
                            pSelector = new PropertyTemplateValueSelector_Flags(this);
                            break;

                        default:
                            UnreachableCode();
                            break;
                        }

                        // parse it
                        DebugAssert(pSelector != NULL);
                        if (!pSelector->ParseXML(xmlReader))
                        {
                            delete pSelector;
                            return false;
                        }

                        // set selector
                        m_pSelector = pSelector;
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "property") == 0);
                break;
            }
            else
            {
                UnreachableCode();
                break;
            }
        }
    }

    return true;
}

#endif

PropertyTemplateProperty *PropertyTemplateProperty::Clone() const
{
    PropertyTemplateProperty *pClone = new PropertyTemplateProperty();

    pClone->m_eType = m_eType;
    pClone->m_strName = m_strName;
    pClone->m_strCategory = m_strCategory;
    pClone->m_strLabel = m_strLabel;
    pClone->m_strDescription = m_strDescription;
    pClone->m_strDefaultValue = m_strDefaultValue;

    if (m_pSelector != NULL)
    {
        pClone->m_pSelector = m_pSelector->Clone(pClone);
        DebugAssert(pClone->m_pSelector != NULL);
    }

    return pClone;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PropertyTemplateValueSelector::PropertyTemplateValueSelector(const PropertyTemplateProperty *pOwner, PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE type)
    : m_pOwner(pOwner),
      m_eType(type)
{

}

PropertyTemplateValueSelector::~PropertyTemplateValueSelector()
{

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PropertyTemplateValueSelector_Resource::PropertyTemplateValueSelector_Resource(const PropertyTemplateProperty *pOwner)
    : PropertyTemplateValueSelector(pOwner, PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_RESOURCE)
{

}

PropertyTemplateValueSelector_Resource::~PropertyTemplateValueSelector_Resource()
{

}

#ifdef HAVE_LIBXML2

bool PropertyTemplateValueSelector_Resource::ParseXML(XMLReader &xmlReader)
{
    const char *resourceTypeStr = xmlReader.FetchAttribute("resource-type");
    if (resourceTypeStr == NULL)
    {
        xmlReader.PrintError("incomplete resource selector definition");
        return false;
    }

    m_strResourceTypeName = resourceTypeStr;

    return xmlReader.SkipCurrentElement();
}

#endif

PropertyTemplateValueSelector *PropertyTemplateValueSelector_Resource::Clone(const PropertyTemplateProperty *pOwner)
{
    PropertyTemplateValueSelector_Resource *pClone = new PropertyTemplateValueSelector_Resource(pOwner);
    
    pClone->m_strResourceTypeName = m_strResourceTypeName;

    return pClone;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PropertyTemplateValueSelector_Color::PropertyTemplateValueSelector_Color(const PropertyTemplateProperty *pOwner)
    : PropertyTemplateValueSelector(pOwner, PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_COLOR)
{

}

PropertyTemplateValueSelector_Color::~PropertyTemplateValueSelector_Color()
{

}

#ifdef HAVE_LIBXML2

bool PropertyTemplateValueSelector_Color::ParseXML(XMLReader &xmlReader)
{
    return xmlReader.SkipCurrentElement();
}

#endif

PropertyTemplateValueSelector *PropertyTemplateValueSelector_Color::Clone(const PropertyTemplateProperty *pOwner)
{
    PropertyTemplateValueSelector_Color *pClone = new PropertyTemplateValueSelector_Color(pOwner);

    return pClone;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PropertyTemplateValueSelector_Slider::PropertyTemplateValueSelector_Slider(const PropertyTemplateProperty *pOwner)
    : PropertyTemplateValueSelector(pOwner, PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_SLIDER)
{

}

PropertyTemplateValueSelector_Slider::~PropertyTemplateValueSelector_Slider()
{

}

#ifdef HAVE_LIBXML2

bool PropertyTemplateValueSelector_Slider::ParseXML(XMLReader &xmlReader)
{
    const char *minStr = xmlReader.FetchAttribute("min");
    const char *maxStr = xmlReader.FetchAttribute("max");
    if (minStr == NULL || maxStr == NULL)
    {
        xmlReader.PrintError("incomplete slider selector definition");
        return false;
    }

    m_fMinValue = StringConverter::StringToFloat(minStr);
    m_fMaxValue = StringConverter::StringToFloat(maxStr);

    return xmlReader.SkipCurrentElement();
}

#endif

PropertyTemplateValueSelector *PropertyTemplateValueSelector_Slider::Clone(const PropertyTemplateProperty *pOwner)
{
    PropertyTemplateValueSelector_Slider *pClone = new PropertyTemplateValueSelector_Slider(pOwner);

    pClone->m_fMinValue = m_fMinValue;
    pClone->m_fMaxValue = m_fMaxValue;

    return pClone;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PropertyTemplateValueSelector_Spinner::PropertyTemplateValueSelector_Spinner(const PropertyTemplateProperty *pOwner)
    : PropertyTemplateValueSelector(pOwner, PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_SPINNER)
{

}

PropertyTemplateValueSelector_Spinner::~PropertyTemplateValueSelector_Spinner()
{

}

#ifdef HAVE_LIBXML2

bool PropertyTemplateValueSelector_Spinner::ParseXML(XMLReader &xmlReader)
{
    const char *minStr = xmlReader.FetchAttribute("min");
    const char *incrementStr = xmlReader.FetchAttribute("increment");
    const char *maxStr = xmlReader.FetchAttribute("max");

    m_fMinValue = (minStr != NULL) ? StringConverter::StringToFloat(minStr) : -Y_FLT_MAX;
    m_fIncrement = (incrementStr != NULL) ? StringConverter::StringToFloat(incrementStr) : 1.0f;
    m_fMaxValue = (maxStr != NULL) ? StringConverter::StringToFloat(maxStr) : Y_FLT_MAX;

    return xmlReader.SkipCurrentElement();
}

#endif

PropertyTemplateValueSelector *PropertyTemplateValueSelector_Spinner::Clone(const PropertyTemplateProperty *pOwner)
{
    PropertyTemplateValueSelector_Spinner *pClone = new PropertyTemplateValueSelector_Spinner(pOwner);

    pClone->m_fMinValue = m_fMinValue;
    pClone->m_fIncrement = m_fIncrement;
    pClone->m_fMaxValue = m_fMaxValue;

    return pClone;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PropertyTemplateValueSelector_Choice::PropertyTemplateValueSelector_Choice(const PropertyTemplateProperty *pOwner)
    : PropertyTemplateValueSelector(pOwner, PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_CHOICE)
{

}

PropertyTemplateValueSelector_Choice::~PropertyTemplateValueSelector_Choice()
{

}

#ifdef HAVE_LIBXML2

bool PropertyTemplateValueSelector_Choice::ParseXML(XMLReader &xmlReader)
{
    if (!xmlReader.IsEmptyElement())
    {
        for (; ;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 selectorSelection = xmlReader.Select("choice");
                if (selectorSelection < 0)
                    return false;

                switch (selectorSelection)
                {
                    // choice
                case 0:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        if (valueStr == NULL)
                        {
                            xmlReader.PrintError("incomplete choice declaration");
                            return false;
                        }

                        Choice choice;
                        choice.Left = valueStr;
                        choice.Right = xmlReader.GetElementText(true);
                        m_Choices.Add(choice);
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "selector") == 0);
                break;
            }
            else
            {
                UnreachableCode();
                break;
            }
        }
    }

    m_Choices.Shrink();
    return true;
}

#endif

PropertyTemplateValueSelector *PropertyTemplateValueSelector_Choice::Clone(const PropertyTemplateProperty *pOwner)
{
    PropertyTemplateValueSelector_Choice *pClone = new PropertyTemplateValueSelector_Choice(pOwner);

    if (m_Choices.GetSize() > 0)
    {
        pClone->m_Choices.Reserve(m_Choices.GetSize());

        uint32 i;
        for (i = 0; i < m_Choices.GetSize(); i++)
            pClone->m_Choices.Add(Choice(m_Choices[i]));
    }

    return pClone;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PropertyTemplateValueSelector_EulerAngles::PropertyTemplateValueSelector_EulerAngles(const PropertyTemplateProperty *pOwner)
    : PropertyTemplateValueSelector(pOwner, PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_EULER_ANGLES)
{

}

PropertyTemplateValueSelector_EulerAngles::~PropertyTemplateValueSelector_EulerAngles()
{

}

#ifdef HAVE_LIBXML2

bool PropertyTemplateValueSelector_EulerAngles::ParseXML(XMLReader &xmlReader)
{
    return xmlReader.SkipCurrentElement();
}

#endif

PropertyTemplateValueSelector *PropertyTemplateValueSelector_EulerAngles::Clone(const PropertyTemplateProperty *pOwner)
{
    PropertyTemplateValueSelector_EulerAngles *pClone = new PropertyTemplateValueSelector_EulerAngles(pOwner);

    return pClone;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PropertyTemplateValueSelector_Flags::PropertyTemplateValueSelector_Flags(const PropertyTemplateProperty *pOwner)
    : PropertyTemplateValueSelector(pOwner, PROPERTY_TEMPLATE_VALUE_SELECTOR_TYPE_FLAGS)
{

}

PropertyTemplateValueSelector_Flags::~PropertyTemplateValueSelector_Flags()
{

}

#ifdef HAVE_LIBXML2

bool PropertyTemplateValueSelector_Flags::ParseXML(XMLReader &xmlReader)
{
    if (!xmlReader.IsEmptyElement())
    {
        for (; ;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 selectorSelection = xmlReader.Select("flag");
                if (selectorSelection < 0)
                    return false;

                switch (selectorSelection)
                {
                    // choice
                case 0:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        if (valueStr == NULL)
                        {
                            xmlReader.PrintError("incomplete flag declaration");
                            return false;
                        }

                        Flag flag;
                        flag.Value = StringConverter::StringToUInt32(valueStr);

                        if (!xmlReader.IsEmptyElement())
                        {
                            for (; ;)
                            {
                                if (!xmlReader.NextToken())
                                    return false;

                                if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                {
                                    int32 propertySelection = xmlReader.Select("label|description");
                                    if (propertySelection < 0)
                                        return false;

                                    switch (propertySelection)
                                    {
                                        // label
                                    case 0:
                                        flag.Label = xmlReader.GetElementText(true);
                                        break;

                                        // description
                                    case 1:
                                        flag.Description = xmlReader.GetElementText(true);
                                        break;

                                    default:
                                        UnreachableCode();
                                        break;
                                    }
                                }
                                else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                {
                                    DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "flag") == 0);
                                    break;
                                }
                                else
                                {
                                    UnreachableCode();
                                    break;
                                }
                            }
                        }

                        m_Flags.Add(flag);
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "selector") == 0);
                break;
            }
            else
            {
                UnreachableCode();
                break;
            }
        }
    }

    m_Flags.Shrink();
    return true;
}

#endif

PropertyTemplateValueSelector *PropertyTemplateValueSelector_Flags::Clone(const PropertyTemplateProperty *pOwner)
{
    PropertyTemplateValueSelector_Flags *pClone = new PropertyTemplateValueSelector_Flags(pOwner);

    if (m_Flags.GetSize() > 0)
    {
        pClone->m_Flags.Reserve(m_Flags.GetSize());

        uint32 i;
        for (i = 0; i < m_Flags.GetSize(); i++)
            pClone->m_Flags.Add(Flag(m_Flags[i]));
    }

    return pClone;
}
