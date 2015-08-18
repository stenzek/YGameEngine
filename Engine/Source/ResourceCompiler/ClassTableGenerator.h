#pragma once
#include "ResourceCompiler/Common.h"
#include "Core/Property.h"
#include "Core/PropertyTable.h"

class PropertyTemplate;

class ClassTableGenerator
{
public:
    class Type
    {
        friend class ClassTableGenerator;

    public:
        struct PropertyDeclaration
        {
            uint32 Index;
            String Name;
            PROPERTY_TYPE Type;
        };

    public:
        Type(uint32 index, const char *typeName);
        ~Type();

        const uint32 GetIndex() const { return m_index; }
        const String &GetTypeName() const { return m_typeName; }

        const uint32 GetPropertyDeclarationCount() const { return m_properties.GetSize(); }
        const PropertyDeclaration *GetPropertyDeclarationByIndex(uint32 index) const { return m_properties[index]; }
        const PropertyDeclaration *GetPropertyDeclarationByName(const char *name) const;
        const PropertyDeclaration *AddPropertyDeclaration(const char *name, PROPERTY_TYPE type);

    private:
        uint32 m_index;
        String m_typeName;
        PODArray<PropertyDeclaration *> m_properties;
    };

public:
    ClassTableGenerator();
    ~ClassTableGenerator();

    const uint32 GetTypeCount() const { return m_types.GetSize(); }
    const Type *GetTypeByIndex(uint32 typeIndex) const { return m_types[typeIndex]; }
    const Type *GetTypeByName(const char *name) const;

    // new types
    const Type *CreateType(const char *name);

    // new type from property template
    const Type *CreateTypeFromPropertyTemplate(const char *name, const PropertyTemplate *pPropertyTemplate);

    // new type from property declaration list
    const Type *CreateTypeFromPropertyDeclarationList(const char *name, const PROPERTY_DECLARATION **ppProperties, uint32 nProperties);

    // compile
    bool Compile(ByteStream *pStream);

    // using the current state of this class table, serialize an object with a property table
    // the template is necessary due to any missing values being unknown
    bool SerializeObjectBinary(ByteStream *pStream, const char *typeName, const PropertyTemplate *pPropertyTemplate, const PropertyTable *pPropertyTable, uint32 *pTypeIndex, uint32 *pWrittenBytes) const;

    // serialize an individual object property string, converting it to its native type
    static bool SerializePropertyValueStringAsNativeType(PROPERTY_TYPE propertyType, const char *propertyValueString, ByteStream *pOutputStream, uint32 *pWrittenBytes);

private:
    PODArray<Type *> m_types;
};

