#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/ObjectTemplateManager.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "YBaseLib/XMLReader.h"
Log_SetChannel(ObjectTemplate);

static const char *OBJECT_TEMPLATE_BASE_PATH = "resources/engine/object_templates";

ObjectTemplateManager::ObjectTemplateManager()
    : m_allTemplatesLoaded(false)
{

}

ObjectTemplateManager::~ObjectTemplateManager()
{
    for (StorageMap::Iterator itr = m_templates.Begin(); !itr.AtEnd(); itr.Forward())
        delete itr->Value;
}

const ObjectTemplate *ObjectTemplateManager::GetObjectTemplate(const char *typeName)
{
    const StorageMap::Member *pMember = m_templates.Find(typeName);
    if (pMember != nullptr)
        return pMember->Value;

    // skip loading if everything is already loaded
    if (m_allTemplatesLoaded)
        return nullptr;

    // try a load first
    ObjectTemplate *pTemplate = LoadObjectTemplate(typeName);
    if (pTemplate == nullptr)
    {
        // load failed, insert a null reference
        Log_ErrorPrintf("ObjectTemplateManager::GetObjectTemplate: Failed to load template for type '%s'", typeName);
        m_templates.Insert(typeName, pTemplate);
        return nullptr;
    }

    // store it first
    // this assert is here in case the child class reloaded this class
    DebugAssert(m_templates.Find(pTemplate->GetTypeName()) == nullptr);
    m_templates.Insert(pTemplate->GetTypeName(), pTemplate);
    return pTemplate;
}

const ObjectTemplate *ObjectTemplateManager::GetObjectTemplate(ResourceCompilerCallbacks *pCallbacks, const char *typeName)
{
    const StorageMap::Member *pMember = m_templates.Find(typeName);
    if (pMember != nullptr)
        return pMember->Value;

    // skip loading if everything is already loaded
    if (m_allTemplatesLoaded)
        return nullptr;

    // try a load first
    ObjectTemplate *pTemplate = LoadObjectTemplate(pCallbacks, typeName);
    if (pTemplate == nullptr)
    {
        // load failed, insert a null reference
        Log_ErrorPrintf("ObjectTemplateManager::GetObjectTemplate: Failed to load template for type '%s'", typeName);
        m_templates.Insert(typeName, pTemplate);
        return nullptr;
    }

    // store it first
    // this assert is here in case the child class reloaded this class
    DebugAssert(m_templates.Find(pTemplate->GetTypeName()) == nullptr);
    m_templates.Insert(pTemplate->GetTypeName(), pTemplate);
    return pTemplate;
}

ObjectTemplate *ObjectTemplateManager::LoadObjectTemplate(const char *typeName)
{
    PathString fileName;
    fileName.Format("%s/%s.xml", OBJECT_TEMPLATE_BASE_PATH, typeName);

    ByteStream *pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
        return nullptr;

    ObjectTemplate *pTemplate = new ObjectTemplate();
    if (!pTemplate->LoadFromStream(fileName, pStream))
    {        
        delete pTemplate;
        pStream->Release();
        return nullptr;
    }

    // this assert is here in case the child class reloaded this class
    pStream->Release();
    return pTemplate;
}

ObjectTemplate *ObjectTemplateManager::LoadObjectTemplate(ResourceCompilerCallbacks *pCallbacks, const char *typeName)
{
    PathString fileName;
    fileName.Format("%s/%s.xml", OBJECT_TEMPLATE_BASE_PATH, typeName);

    AutoReleasePtr<BinaryBlob> pBlob = pCallbacks->GetFileContents(fileName);
    if (pBlob == nullptr)
        return nullptr;
    
    AutoReleasePtr<ByteStream> pStream = pBlob->CreateReadOnlyStream();

    ObjectTemplate *pTemplate = new ObjectTemplate();
    if (!pTemplate->LoadFromStream(fileName, pStream))
    {
        delete pTemplate;
        pStream->Release();
        return nullptr;
    }

    // this assert is here in case the child class reloaded this class
    pStream->Release();
    return pTemplate;
}

bool ObjectTemplateManager::LoadAllTemplates()
{
    SmallString entityTypeName;

    // search for templates
    FileSystem::FindResultsArray findResults;
    g_pVirtualFileSystem->FindFiles(OBJECT_TEMPLATE_BASE_PATH, "*.xml", FILESYSTEM_FIND_FILES, &findResults);

    // no templates?
    if (findResults.GetSize() == 0)
    {
        Log_WarningPrintf("ObjectTemplate::LoadEntityTemplates: No templates found!");
        return false;
    }

    // go through find results
    for (uint32 i = 0; i < findResults.GetSize(); i++)
    {
        FILESYSTEM_FIND_DATA &findData = findResults[i];

        // derive the type name from the filename
        const char *posExtensionStart = Y_strrchr(findData.FileName, '.');
        const char *posFileNameStart = Y_strrchr(findData.FileName, '/');
        if (posExtensionStart == nullptr || posFileNameStart == nullptr || posExtensionStart <= posFileNameStart)
        {
            Log_ErrorPrintf("ObjectTemplate::LoadEntityTemplates: Could not open derive type name from file name '%s'", findData.FileName);
            return false;
        }

        // work out entity type name
        entityTypeName.Clear();
        entityTypeName.AppendSubString(posFileNameStart + 1, 0, (int32)(posExtensionStart - posFileNameStart - 1));

        // does it already exist?
        StorageMap::Member *pExistingMember = m_templates.Find(entityTypeName);
        if (pExistingMember != nullptr)
        {
            // if it was loaded as a base class, and failed, we should've bailed out at the below branch.
            DebugAssert(pExistingMember->Value != nullptr);
            continue;
        }

        // open stream
        ByteStream *pStream = g_pVirtualFileSystem->OpenFile(findData.FileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
        if (pStream == nullptr)
        {
            Log_WarningPrintf("ObjectTemplate::LoadEntityTemplates: Could not open template file '%s'", findData.FileName);
            continue;
        }

        ObjectTemplate *pTemplate = new ObjectTemplate();
        if (!pTemplate->LoadFromStream(findData.FileName, pStream))
        {
            Log_ErrorPrintf("ObjectTemplate::LoadEntityTemplates: Failed to load template '%s'", findData.FileName);
            delete pTemplate;
            pStream->Release();
            return false;
        }

        // can lose the strema now
        pStream->Release();

        // should match the filename
        DebugAssert(entityTypeName.CompareInsensitive(pTemplate->GetTypeName()));

        // insert into map
        m_templates.Insert(pTemplate->GetTypeName(), pTemplate);
    }

    Log_InfoPrintf("ObjectTemplateManager::LoadAllTemplates: Loaded %u templates.", m_templates.GetMemberCount());
    m_allTemplatesLoaded = true;
    return true;
}



