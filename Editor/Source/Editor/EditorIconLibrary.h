#pragma once
#include "Editor/Common.h"

class EditorIconLibrary
{
public:
    EditorIconLibrary();
    ~EditorIconLibrary();

    // load icons
    bool PreloadIcons();

    // keyed icons
    const QIcon &GetIconByName(const char *key, uint32 size = 16) const;

    // gets a QIcon for the specified resource type
    const QIcon &GetIconForResourceType(const ResourceTypeInfo *pResourceTypeInfo, uint32 size = 16) const;

    // helper functions
    const QIcon &GetListViewDirectoryIcon(uint32 size = 16) const { return GetIconByName("Directory", size); }

private:
    bool LoadIcon(const char *name, uint32 size) const;

    typedef CIStringHashTable<QIcon> IconTable;
    mutable IconTable m_iconTable;
    QIcon m_defaultIcon;
};

