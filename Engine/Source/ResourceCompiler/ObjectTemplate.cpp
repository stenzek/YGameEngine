#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/ObjectTemplate.h"
#include "ResourceCompiler/ObjectTemplateManager.h"
#include "Core/VirtualFileSystem.h"
#include "YBaseLib/XMLReader.h"
Log_SetChannel(ObjectTemplate);

ObjectTemplate::ObjectTemplate()
    : m_pBaseClassTemplate(nullptr),
      m_canCreate(false)
{

}

ObjectTemplate::~ObjectTemplate()
{

}

bool ObjectTemplate::IsDerivedFrom(const ObjectTemplate *pTemplate) const
{
    const ObjectTemplate *pCurrent = this;
    while (pCurrent != nullptr)
    {
        if (pCurrent == pTemplate)
            return true;

        pCurrent = pCurrent->GetBaseClassTemplate();
    }

    return false;
}

bool ObjectTemplate::LoadFromStream(const char *FileName, ByteStream *pStream)
{
    uint32 i;

    // create xml reader
    XMLReader xmlReader;
    if (!xmlReader.Create(pStream, FileName))
    {
        xmlReader.PrintError("could not create xml reader");
        return false;
    }

    // skip to entity-definition element
    if (!xmlReader.SkipToElement("object-template"))
    {
        xmlReader.PrintError("could not skip to object-template element");
        return false;
    }

    // read attributes
    const char *typeNameStr = xmlReader.FetchAttribute("name");
    const char *baseTypeNameStr = xmlReader.FetchAttribute("base");
    const char *canCreateStr = xmlReader.FetchAttribute("can-create");
    if (typeNameStr == NULL || canCreateStr == NULL)
    {
        xmlReader.PrintError("incomplete entity definition");
        return false;
    }

    // set name/can create
    m_typeName = typeNameStr;
    m_displayName = m_typeName;
    m_canCreate = StringConverter::StringToBool(canCreateStr);

    // handle base class
    if (baseTypeNameStr != NULL)
    {
        m_pBaseClassTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate(baseTypeNameStr);
        if (m_pBaseClassTemplate == nullptr)
        {
            xmlReader.PrintError("could not get base class definition ('%s')", baseTypeNameStr);
            return false;
        }
    }

    // read elements
    if (!xmlReader.IsEmptyElement())
    {
        for (; ;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 entityDefinitionSelection = xmlReader.Select("displayname|category|description|properties");
                if (entityDefinitionSelection < 0)
                    return false;

                switch (entityDefinitionSelection)
                {
                    // displayname
                case 0:
                    {
                        if (xmlReader.IsEmptyElement())
                            continue;

                        // get text
                        m_displayName = xmlReader.GetElementText(true);
                    }
                    break;

                    // category
                case 1:
                    {
                        if (xmlReader.IsEmptyElement())
                            continue;

                        // get text
                        m_categoryName = xmlReader.GetElementText(true);
                    }
                    break;

                    // description
                case 2:
                    {
                        if (xmlReader.IsEmptyElement())
                            continue;

                        // get text
                        m_description = xmlReader.GetElementText(true);
                    }
                    break;

                    // properties
                case 3:
                    {
                        if (!xmlReader.IsEmptyElement())
                        {
                            if (!m_propertyTableTemplate.LoadFromXML(xmlReader))
                                return false;

                            DebugAssert(Y_strcmp(xmlReader.GetNodeName(), "properties") == 0);
                        }
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "object-template") == 0);
                break;
            }
            else
            {
                UnreachableCode();
                break;
            }
        }
    }

    // copy stuff across from base class
    if (m_pBaseClassTemplate != nullptr)
    {
        // copy properties across
        for (i = 0; i < m_pBaseClassTemplate->GetPropertyDefinitionCount(); i++)
        {
            const PropertyTemplateProperty *pBaseClassProperty = m_pBaseClassTemplate->GetPropertyDefinition(i);
            if (m_propertyTableTemplate.GetPropertyDefinitionByName(pBaseClassProperty->GetName()))
            {
                // property is overriden by child, ignore it
                continue;
            }

            PropertyTemplateProperty *pClonedPropertyDefinition = pBaseClassProperty->Clone();
            DebugAssert(pClonedPropertyDefinition != NULL);
            m_propertyTableTemplate.AddPropertyDefinition(pClonedPropertyDefinition);
        }
    }

    Log_DevPrintf("Object template loaded: '%s' (base class '%s', %u properties)", m_typeName.GetCharArray(), (m_pBaseClassTemplate != nullptr) ? m_pBaseClassTemplate->GetTypeName().GetCharArray() : "none", m_propertyTableTemplate.GetPropertyDefinitionCount());
    return true;
}
