#pragma once
#include "ResourceCompiler/Common.h"
#include "ResourceCompiler/ObjectTemplate.h"

struct ResourceCompilerCallbacks;

class ObjectTemplateManager : public Singleton<ObjectTemplateManager>
{
public:
    ObjectTemplateManager();
    ~ObjectTemplateManager();

    // attempts to load the template if it isn't loaded
    const ObjectTemplate *GetObjectTemplate(const char *typeName);

    // loads an object template using a callback for file i/o
    const ObjectTemplate *GetObjectTemplate(ResourceCompilerCallbacks *pCallbacks, const char *typeName);

    // accessors
    const uint32 GetObjectTemplateCount() const { return m_templates.GetMemberCount(); }

    // load all templates
    bool LoadAllTemplates();

    template<class T>
    void EnumerateObjectTemplates(T callback) const
    {
        for (StorageMap::ConstIterator itr = m_templates.Begin(); !itr.AtEnd(); itr.Forward())
            callback(itr->Value);
    }

private:
    // try loading from disk
    static ObjectTemplate *LoadObjectTemplate(const char *typeName);
    static ObjectTemplate *LoadObjectTemplate(ResourceCompilerCallbacks *pCallbacks, const char *typeName);

    // storage
    typedef CIStringHashTable<ObjectTemplate *> StorageMap;
    StorageMap m_templates;
    bool m_allTemplatesLoaded;
};

