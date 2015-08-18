#include "Editor/PrecompiledHeader.h"
#include "Editor/SimpleTreeModel.h"

SimpleTreeModel::Item::Item(SimpleTreeModel *pModel, const Container *pParent, int parentIndex)
    : m_pModel(pModel),
      m_pParent(pParent),
      m_parentIndex(parentIndex),
      m_pFields(new QString[pModel->GetColumnCount()]),
      m_nColumns(pModel->GetColumnCount()),
      m_pUserData(nullptr)

{

}

SimpleTreeModel::Item::~Item()
{
    delete[] m_pFields;
}

QModelIndex SimpleTreeModel::Item::CreateIndexRelativeToParent(int column /* = 0 */) const
{
    // work out the parent to emit from, make second level the top level
    const Item *pEmitFrom = this;
    if (m_pParent != nullptr && m_pParent->GetParent() == nullptr)
        pEmitFrom = nullptr;

    return m_pModel->createIndex(m_parentIndex, column, const_cast<Item *>(pEmitFrom));
}

QModelIndex SimpleTreeModel::Item::CreateIndex(int column /* = 0 */) const
{
    return m_pModel->createIndex(0, column, const_cast<Item *>(this));
}

const QString SimpleTreeModel::Item::GetText(int column) const
{
    Q_ASSERT(column >= 0 && column < m_nColumns);
    return m_pFields[column];
}

void SimpleTreeModel::Item::SetText(int column, const QString text)
{
    Q_ASSERT(column >= 0 && column < m_nColumns);
    m_pFields[column] = text;

    m_pModel->dataChanged(CreateIndexRelativeToParent(column), CreateIndexRelativeToParent(column));
}

void SimpleTreeModel::Item::SetUserData(void *pUserData)
{
    m_pUserData = pUserData;
}


SimpleTreeModel::Container::Container(SimpleTreeModel *pModel, Container *pParent, int parentIndex)
    : Item(pModel, pParent, parentIndex)
{

}

SimpleTreeModel::Container::~Container()
{
    for (int i = 0; i < m_items.size(); i++)
        delete m_items[i];
}

SimpleTreeModel::Item *SimpleTreeModel::Container::CreateItem()
{
    Item *pItem = new Item(m_pModel, this, m_items.size());

    m_pModel->beginInsertRows(CreateIndex(), pItem->GetParentIndex(), pItem->GetParentIndex());
    m_items.append(pItem);
    m_pModel->endInsertRows();

    return pItem;
}

SimpleTreeModel::Container *SimpleTreeModel::Container::CreateContainer(const QString titleText)
{
    Container *pContainer = new Container(m_pModel, this, m_items.size());
    pContainer->SetTitle(titleText);

    m_pModel->beginInsertRows(CreateIndex(), pContainer->GetParentIndex(), pContainer->GetParentIndex());
    m_items.append(pContainer);
    m_pModel->endInsertRows();

    return pContainer;
}

const SimpleTreeModel::Item *SimpleTreeModel::Container::GetItem(int index) const
{
    return (index >= 0 && index < m_items.size()) ? m_items[index] : nullptr;
}

SimpleTreeModel::Item * SimpleTreeModel::Container::GetItem(int index)
{
    return (index >= 0 && index < m_items.size()) ? m_items[index] : nullptr;
}

int SimpleTreeModel::Container::FindItemIndexByPointer(const Item *pItem) const
{
    for (int i = 0; i < m_items.size(); i++)
    {
        if (m_items[i] == pItem)
            return i;
    }

    return -1;
}

int SimpleTreeModel::Container::FindItemIndexByUserData(void *pUserData) const
{
    for (int i = 0; i < m_items.size(); i++)
    {
        if (m_items[i]->GetUserData() == pUserData)
            return i;
    }

    return -1;
}

void SimpleTreeModel::Container::RemoveItem(Item *pItem)
{
    int index = FindItemIndexByPointer(pItem);
    Q_ASSERT(index >= 0);
    return RemoveItem(index);
}

void SimpleTreeModel::Container::RemoveItem(int index)
{
    Q_ASSERT(index >= 0 && index < m_items.size());

    m_pModel->beginRemoveRows(CreateIndex(), index, index);

    Item *pItem = m_items[index];
    m_items.removeAt(index);
    delete pItem;

    m_pModel->endRemoveRows();
}

void SimpleTreeModel::Container::RemoveAllItems()
{
    if (m_items.size() > 0)
    {
        m_pModel->beginRemoveRows(CreateIndex(), 0, m_items.size() - 1);

        for (int i = 0; i < m_items.size(); i++)
            delete m_items[i];

        m_items.clear();

        m_pModel->endRemoveRows();
    }
}

SimpleTreeModel::SimpleTreeModel(int nColumns, QObject *pParent /*= nullptr*/)
    : QAbstractItemModel(pParent),
      m_nColumns(nColumns),
      m_pColumnNames(new QString[nColumns]),
      m_rootItem(this, nullptr, 0)
{
    Q_ASSERT(nColumns > 0);
}

SimpleTreeModel::~SimpleTreeModel()
{
    delete[] m_pColumnNames;
}

void SimpleTreeModel::SetColumnTitle(int i, const QString name)
{
    Q_ASSERT(i < m_nColumns);
    m_pColumnNames[i] = name;
}

SimpleTreeModel::Container *SimpleTreeModel::CreateContainer(const QString titleText, Container *pContainer /*= nullptr*/)
{
    if (pContainer != nullptr)
        return pContainer->CreateContainer(titleText);
    else
        return m_rootItem.CreateContainer(titleText);
}

SimpleTreeModel::Item *SimpleTreeModel::CreateItem(Container *pContainer /*= nullptr*/)
{
    if (pContainer != nullptr)
        return pContainer->CreateItem();
    else
        return m_rootItem.CreateItem();
}

QModelIndex SimpleTreeModel::index(int row, int column, const QModelIndex &parent /*= QModelIndex()*/) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    // root item?
    const Item *pItem;
    if (!parent.isValid())
        pItem = &m_rootItem;
    else
        pItem = reinterpret_cast<Item *>(parent.internalPointer());

    // should point to an item/container    
    Q_ASSERT(pItem != nullptr && pItem->IsContainer());

    // access this row index
    Q_ASSERT(row >= 0 && row < static_cast<const Container *>(pItem)->m_items.size());
    return createIndex(row, column, static_cast<const Container *>(pItem)->m_items[row]);
}

QModelIndex SimpleTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    // should point to an item/container
    const Item *pItem = reinterpret_cast<Item *>(child.internalPointer());
    Q_ASSERT(pItem != nullptr);

    // look up parent, 'top-level', ie one level below the root items should not have a parent
    if (pItem->GetParent() == nullptr || pItem->GetParent() == &m_rootItem)
        return QModelIndex();

    // create row
    return createIndex(pItem->GetParentIndex(), 0, const_cast<void *>(reinterpret_cast<const void *>(pItem->GetParent())));
}

int SimpleTreeModel::rowCount(const QModelIndex &parent /*= QModelIndex()*/) const
{
    if (parent.column() > 0)
        return 0;

    // root item?
    const Item *pItem;
    if (!parent.isValid())
        pItem = &m_rootItem;
    else
        pItem = reinterpret_cast<Item *>(parent.internalPointer());

    // should point to an item/container    
    Q_ASSERT(pItem != nullptr && pItem->IsContainer());

    // return row count
    return static_cast<const Container *>(pItem)->m_items.size();
}

int SimpleTreeModel::columnCount(const QModelIndex &parent /*= QModelIndex()*/) const
{
    if (parent.column() > 0)
        return 0;

    return m_nColumns;
}

bool SimpleTreeModel::hasChildren(const QModelIndex &parent /*= QModelIndex()*/) const
{
    if (parent.column() > 0)
        return false;

    // root node
    if (!parent.isValid())
        return true;

    // root item?
    const Item *pItem = reinterpret_cast<Item *>(parent.internalPointer());

    // should point to an item/container    
    Q_ASSERT(pItem != nullptr);

    // return true if it is a container
    return pItem->IsContainer();
}

QVariant SimpleTreeModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
    if (orientation == Qt::Horizontal)
    {
        if (role != Qt::DisplayRole)
            return QVariant();

        if (section >= 0 && section < m_nColumns)
            return QVariant(m_pColumnNames[section]);
        else
            return QVariant();
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}

QVariant SimpleTreeModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
{
    if (!index.isValid())
        return QVariant();

    // should point to an item/container
    const Item *pItem = reinterpret_cast<Item *>(index.internalPointer());
    Q_ASSERT(pItem != nullptr);

    // role?
    int column = index.column();
    if (role == Qt::DisplayRole)
    {
        if (column >= 0 && column < m_nColumns)
            return pItem->m_pFields[column];
        else
            return QVariant();
    }

    return QVariant();
}
