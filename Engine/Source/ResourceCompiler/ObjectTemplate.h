#pragma once
#include "ResourceCompiler/Common.h"
#include "Core/PropertyTemplate.h"

class ByteStream;

// this class mirrors EditorEntityDefinition, without a lot of the extra editor-specific stuff

class ObjectTemplate
{
public:
    ObjectTemplate();
    ~ObjectTemplate();

    const String &GetTypeName() const { return m_typeName; }
    const String &GetDisplayName() const { return m_displayName; }
    const String &GetCategoryName() const { return m_categoryName; }
    const String &GetDescription() const { return m_description; }
    const ObjectTemplate *GetBaseClassTemplate() const { return m_pBaseClassTemplate; }
    const bool CanCreate() const { return m_canCreate; }

    const PropertyTemplate *GetPropertyTemplate() const { return &m_propertyTableTemplate; }
    const PropertyTemplateProperty *GetPropertyDefinition(uint32 i) const { return m_propertyTableTemplate.GetPropertyDefinition(i); }
    const PropertyTemplateProperty *GetPropertyDefinitionByName(const char *propertyName) const { return m_propertyTableTemplate.GetPropertyDefinitionByName(propertyName); }
    const uint32 GetPropertyDefinitionCount() const { return m_propertyTableTemplate.GetPropertyDefinitionCount(); }

    bool LoadFromStream(const char *FileName, ByteStream *pStream);

    // inheritance helper
    bool IsDerivedFrom(const ObjectTemplate *pTemplate) const;

private:
    String m_typeName;
    String m_displayName;
    String m_categoryName;
    String m_description;
    const ObjectTemplate *m_pBaseClassTemplate;
    bool m_canCreate;

    PropertyTemplate m_propertyTableTemplate;
};

