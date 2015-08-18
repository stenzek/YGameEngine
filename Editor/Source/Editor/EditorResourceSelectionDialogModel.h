#pragma once
#include "Editor/Common.h"

class EditorResourceSelectionDialogModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    class DirectoryNode
    {
    public:
        DirectoryNode(EditorResourceSelectionDialogModel *pModel, const String &rootPath);
        DirectoryNode(EditorResourceSelectionDialogModel *pModel, DirectoryNode *pParent);
        ~DirectoryNode();

        DirectoryNode *GetParent() { return m_pParent; }
        const DirectoryNode *GetParent() const { return m_pParent; }
        const String &GetDisplayName() const { return m_displayName; }
        const String &GetFullName() const { return m_fullName; }
        const ResourceTypeInfo *GetResourceTypeInfo() const { return m_pResourceType; }
        const bool IsDirectory() const { return (m_pResourceType == NULL); }
        const Timestamp &GetModifiedTime() const { return m_modifiedTime; }
        const PODArray<DirectoryNode *> &GetChildren() const { return m_children; }
        const bool IsPopulated() const { return m_populated; }

        void Populate();

    private:
        EditorResourceSelectionDialogModel *m_pModel;
        DirectoryNode *m_pParent;
        String m_displayName;
        String m_fullName;
        const ResourceTypeInfo *m_pResourceType;
        Timestamp m_modifiedTime;
        PODArray<DirectoryNode *> m_children;
        bool m_populated;
    };

public:
    EditorResourceSelectionDialogModel(const String &rootPath, bool includeDirectories = true, bool includeResources = true, uint32 previewIconSize = 0, QWidget *pParent = NULL);
    ~EditorResourceSelectionDialogModel();

    // root path
    const String &GetRootPath() const { return m_rootPath; }
    const bool GetIncludeDirectories() const { return m_includeDirectories; }
    const bool GetIncludeResources() const { return m_includeResources; }

    // resource filters
    void SetResourceFilter(const ResourceTypeInfo **pResourceTypeInfo, uint32 nResourceTypeInfos);
    bool ShouldIncludeResourceType(const ResourceTypeInfo *pResourceTypeInfo);

    // get the directory node for a model index
    const DirectoryNode *GetDirectoryNodeForIndex(const QModelIndex &index) const;

    // implemented functions
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    DirectoryNode *InternalGetDirectoryNode(const QModelIndex &index) const;

    String m_rootPath;
    bool m_includeDirectories;
    bool m_includeResources;

    uint32 m_previewIconSize;

    const ResourceTypeInfo **m_ppResourceTypeInfoFilter;
    uint32 m_nResourceTypeInfoFilters;
    DirectoryNode *m_pRootNode;
};
