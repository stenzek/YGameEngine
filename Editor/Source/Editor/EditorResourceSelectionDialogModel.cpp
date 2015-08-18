#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorResourceSelectionDialogModel.h"
#include "Editor/EditorResourcePreviewGenerator.h"
#include "Editor/Editor.h"
#include "Editor/EditorHelpers.h"
#include "Engine/ResourceManager.h"
#include "Core/Image.h"
Log_SetChannel(EditorResourceSelectionDialog);

EditorResourceSelectionDialogModel::DirectoryNode::DirectoryNode(EditorResourceSelectionDialogModel *pModel, const String &rootPath)
    : m_pModel(pModel),
      m_pParent(NULL),
      m_fullName(rootPath),
      m_pResourceType(NULL),
      m_populated(false)
{

}

EditorResourceSelectionDialogModel::DirectoryNode::DirectoryNode(EditorResourceSelectionDialogModel *pModel, DirectoryNode *pParent)
    : m_pModel(pModel),
      m_pParent(pParent),
      m_pResourceType(NULL),
      m_populated(false)
{

}

EditorResourceSelectionDialogModel::DirectoryNode::~DirectoryNode()
{
    for (uint32 i = 0; i < m_children.GetSize(); i++)
        delete m_children[i];
}

void EditorResourceSelectionDialogModel::DirectoryNode::Populate()
{
    DebugAssert(!m_populated);
    DebugAssert(m_pResourceType == NULL);

    PathString filename;
    PathString resourceName;
    FileSystem::FindResultsArray findResults;
    if (g_pVirtualFileSystem->FindFiles(m_fullName, "*", FILESYSTEM_FIND_FOLDERS | FILESYSTEM_FIND_FILES | FILESYSTEM_FIND_RELATIVE_PATHS | FILESYSTEM_FIND_HIDDEN_FILES, &findResults))
    {
        uint32 i;
        for (i = 0; i < findResults.GetSize(); i++)
        {
            const FILESYSTEM_FIND_DATA *pFindData = &findResults[i];
            if (m_fullName.GetLength() == 0)
                filename = pFindData->FileName;
            else
                filename.Format("%s/%s", m_fullName.GetCharArray(), pFindData->FileName);

            if (pFindData->Attributes & FILESYSTEM_FILE_ATTRIBUTE_DIRECTORY)
            {
                // skip over directories?
                if (!m_pModel->GetIncludeDirectories())
                    continue;

                DirectoryNode *pNode = new DirectoryNode(m_pModel, this);
                pNode->m_displayName = pFindData->FileName;
                pNode->m_fullName = filename;
                pNode->m_modifiedTime = pFindData->ModificationTime;
                m_children.Add(pNode);
            }
            else
            {
                // skip over anything that could be a resource if requested
                if (!m_pModel->GetIncludeResources())
                    continue;

                // determine the type of resource
                ByteStream *pStream = g_pVirtualFileSystem->OpenFile(filename, BYTESTREAM_OPEN_READ);
                if (pStream == NULL)
                    continue;
                    
                const ResourceTypeInfo *pResourceTypeInfo = g_pResourceManager->GetResourceTypeForStream(filename, pStream, resourceName);
                pStream->Release();

                // got resource type?
                if (pResourceTypeInfo == NULL || !m_pModel->ShouldIncludeResourceType(pResourceTypeInfo))
                    continue;

                // ensure it doesn't already exist
                uint32 j;
                for (j = 0; j < m_children.GetSize(); j++)
                {
                    if (m_children[j]->m_fullName.CompareInsensitive(resourceName) && m_children[j]->m_pResourceType == pResourceTypeInfo)
                        break;
                }

                DirectoryNode *pNode;
                if (j != m_children.GetSize())
                {
                    if (m_children[j]->m_modifiedTime >= pFindData->ModificationTime)
                        continue;

                    pNode = m_children[j];
                }
                else
                {
                    pNode = new DirectoryNode(m_pModel, this);
                    m_children.Add(pNode);
                }

                // create node
                pNode->m_displayName = resourceName.SubString(resourceName.RFind('/') + 1);
                pNode->m_fullName = resourceName;
                pNode->m_pResourceType = pResourceTypeInfo;
                pNode->m_modifiedTime = pFindData->ModificationTime;
            }
        }
    }

    m_populated = true;
}

EditorResourceSelectionDialogModel::EditorResourceSelectionDialogModel(const String &rootPath, bool includeDirectories /* = true */, bool includeResources /* = true */, uint32 previewIconSize /* = 0 */, QWidget *pParent /* = NULL */)
    : QAbstractItemModel(pParent),
      m_rootPath(rootPath),
      m_includeDirectories(includeDirectories),
      m_includeResources(includeResources),
      m_previewIconSize(previewIconSize),
      m_ppResourceTypeInfoFilter(NULL),
      m_nResourceTypeInfoFilters(0)
{
    m_pRootNode = new DirectoryNode(this, rootPath);
}

EditorResourceSelectionDialogModel::~EditorResourceSelectionDialogModel()
{
    delete m_pRootNode;
}

EditorResourceSelectionDialogModel::DirectoryNode *EditorResourceSelectionDialogModel::InternalGetDirectoryNode(const QModelIndex &index) const
{
    DirectoryNode *pDirectoryNode = reinterpret_cast<DirectoryNode *>(index.internalPointer());
    DebugAssert(pDirectoryNode != NULL);
    return pDirectoryNode;
}

void EditorResourceSelectionDialogModel::SetResourceFilter(const ResourceTypeInfo **pResourceTypeInfo, uint32 nResourceTypeInfos)
{
    m_ppResourceTypeInfoFilter = pResourceTypeInfo;
    m_nResourceTypeInfoFilters = nResourceTypeInfos;
}

bool EditorResourceSelectionDialogModel::ShouldIncludeResourceType(const ResourceTypeInfo *pResourceTypeInfo)
{
    if (m_nResourceTypeInfoFilters == 0)
        return true;

    for (uint32 i = 0; i < m_nResourceTypeInfoFilters; i++)
    {
        if (m_ppResourceTypeInfoFilter[i] == pResourceTypeInfo)
            return true;
    }

    return false;
}

const EditorResourceSelectionDialogModel::DirectoryNode *EditorResourceSelectionDialogModel::GetDirectoryNodeForIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return NULL;

    return InternalGetDirectoryNode(index);
}

QModelIndex EditorResourceSelectionDialogModel::index(int row, int column, const QModelIndex &parent /*= QModelIndex()*/) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    DirectoryNode *pParentItem;
    if (!parent.isValid())
        pParentItem = m_pRootNode;
    else
        pParentItem = InternalGetDirectoryNode(parent);

    if ((uint32)row >= pParentItem->GetChildren().GetSize())
        return QModelIndex();
    else
        return createIndex(row, column, reinterpret_cast<void *>(pParentItem->GetChildren()[row]));
}

QModelIndex EditorResourceSelectionDialogModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    DirectoryNode *pDirectoryNode = InternalGetDirectoryNode(child);
    DirectoryNode *pParentNode = pDirectoryNode->GetParent();
    if (pParentNode == m_pRootNode)
        return QModelIndex();

    uint32 row;
    const PODArray<DirectoryNode *> &childrenArray = pParentNode->GetChildren();
    for (row = 0; row < childrenArray.GetSize(); row++)
    {
        if (childrenArray[row] == pDirectoryNode)
            break;
    }

    return createIndex(row, 0, reinterpret_cast<void *>(pParentNode));
}

int EditorResourceSelectionDialogModel::rowCount(const QModelIndex &parent /*= QModelIndex()*/) const
{
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
    {
        if (!m_pRootNode->IsPopulated())
            m_pRootNode->Populate();

        return m_pRootNode->GetChildren().GetSize();
    }


    DirectoryNode *pDirectoryNode = InternalGetDirectoryNode(parent);
    if (pDirectoryNode->IsDirectory() && !pDirectoryNode->IsPopulated())
        pDirectoryNode->Populate();

    return pDirectoryNode->GetChildren().GetSize();
}

int EditorResourceSelectionDialogModel::columnCount(const QModelIndex &parent /*= QModelIndex()*/) const
{
    if (parent.column() > 0)
        return 0;

    return 3;
}

QVariant EditorResourceSelectionDialogModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
    if (orientation == Qt::Horizontal)
    {
        if (role != Qt::DisplayRole)
            return QVariant();

        switch (section)
        {
        case 0:     return tr("Name");
        case 1:     return tr("Type");
        case 2:     return tr("Date Modified");
        default:    return QVariant();
        }
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}

bool EditorResourceSelectionDialogModel::hasChildren(const QModelIndex &parent /*= QModelIndex()*/) const
{
    if (parent.column() > 0)
        return false;

    if (!parent.isValid())
        return true;

    DirectoryNode *pDirectoryNode = InternalGetDirectoryNode(parent);
    return pDirectoryNode->IsDirectory();
}

QVariant EditorResourceSelectionDialogModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
{
    if (!index.isValid())
        return QVariant();

    DirectoryNode *pDirectoryNode = InternalGetDirectoryNode(index);
    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
        case 0:     return QVariant(pDirectoryNode->GetDisplayName());
        case 1:     return QVariant((pDirectoryNode->GetResourceTypeInfo() != NULL) ? pDirectoryNode->GetResourceTypeInfo()->GetTypeName() : "Directory");
        case 2:     return QVariant(pDirectoryNode->GetModifiedTime().ToString("%Y-%m-%d %H:%M:%S"));
        default:    return QVariant();
        }
    }
    else if (role == Qt::DecorationRole)
    {
        switch (index.column())
        {
        case 0:
            {
                if (pDirectoryNode->GetResourceTypeInfo() != NULL)
                {
                    if (m_previewIconSize > 0)
                    {
                        // generate a preview icon
                        Image previewImage;
                        previewImage.Create(PIXEL_FORMAT_R8G8B8A8_UNORM, m_previewIconSize, m_previewIconSize, 1);

                        // actually generate it
                        EditorResourcePreviewGenerator generator;
                        generator.SetGPUContext(g_pRenderer->GetMainContext());
                        if (generator.GenerateResourcePreview(&previewImage, pDirectoryNode->GetResourceTypeInfo(), pDirectoryNode->GetFullName()))
                        {
                            // return an icon from this
                            QPixmap pixmap;
                            if (EditorHelpers::ConvertImageToQPixmap(previewImage, &pixmap))
                                return pixmap;
                            else
                                return QVariant();
                        }
                        else
                        {
                            // error...
                            //Y_memzero(previewImage.GetData(), previewImage.GetDataSize());
                            return QVariant();
                        }
                    }
                    else
                    {
                        // use resource icon
                        return g_pEditor->GetIconLibrary()->GetIconForResourceType(pDirectoryNode->GetResourceTypeInfo());
                    }
                }
                else
                {
                    // folders can't have any preview generated...
                    return g_pEditor->GetIconLibrary()->GetListViewDirectoryIcon();
                }
            }
            break;
        }
    }

    return QVariant();
}
