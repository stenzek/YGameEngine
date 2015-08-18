#include "Editor/PrecompiledHeader.h"
#include "Editor/Editor.h"
#include "Editor/EditorHelpers.h"

EditorIconLibrary::EditorIconLibrary()
{

}

EditorIconLibrary::~EditorIconLibrary()
{

}

const QIcon &EditorIconLibrary::GetIconByName(const char *key, uint32 size /* = 16 */) const
{
    SmallString realKey;
    realKey.Format("%s_%u", key, size);

    const IconTable::Member *pMember = m_iconTable.Find(realKey);
    if (pMember == NULL)
    {
        if (!LoadIcon(key, size))
            pMember = m_iconTable.Insert(realKey, m_defaultIcon);
        else
            pMember = m_iconTable.Find(realKey);
    }

    return pMember->Value;
}

const QIcon &EditorIconLibrary::GetIconForResourceType(const ResourceTypeInfo *pResourceTypeInfo, uint32 size /* = 16 */) const
{
    SmallString tableKey;
    tableKey.Format("Resource_%s_%u", pResourceTypeInfo->GetTypeName(), size);

    const IconTable::Member *pMember = m_iconTable.Find(tableKey);
    if (pMember == NULL)
    {
        SmallString realKey;
        realKey.Format("Resource_%s", pResourceTypeInfo->GetTypeName());

        // replace it with the default resource icon
        if (!LoadIcon(realKey, size))
            pMember = m_iconTable.Insert(tableKey, GetIconByName("Resource", size));
        else
            pMember = m_iconTable.Find(tableKey);
    }

    return pMember->Value;
}

bool EditorIconLibrary::PreloadIcons()
{
    //////////////////////////////////////////////////////////////////////////
    // Fixed Icons
    //////////////////////////////////////////////////////////////////////////
    LoadIcon("Directory", 16);

    //////////////////////////////////////////////////////////////////////////
    // Resource Type Icons
    //////////////////////////////////////////////////////////////////////////
    LoadIcon("Resource", 16);
    LoadIcon("Resource_Texture2D", 16);
    LoadIcon("Resource_Texture2DArray", 16);
    LoadIcon("Resource_TextureCube", 16);
    LoadIcon("Resource_Material", 16);
    LoadIcon("Resource_MaterialShader", 16);
    LoadIcon("Resource_StaticMesh", 16);
    LoadIcon("Resource_StaticBlockMesh", 16);

    //////////////////////////////////////////////////////////////////////////
    return true;
}

bool EditorIconLibrary::LoadIcon(const char *name, uint32 size) const
{
    PathString resourceFileName;
    resourceFileName.Format(":/editor/icons/%s_%ux%u.png", name, size, size);

    QIcon createdIcon(ConvertStringToQString(resourceFileName));
    if (createdIcon.isNull() || createdIcon.availableSizes().size() == 0)
        return false;

    SmallString key;
    key.Format("%s_%u", name, size);
    m_iconTable.Insert(key, createdIcon);
    return true;
}
