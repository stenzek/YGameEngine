#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorVirtualFileSystemModel.h"

EditorVirtualFileSystemModel::EditorVirtualFileSystemModel(QWidget *pParent /*= NULL*/)
    : QAbstractItemModel(pParent)
{

}

EditorVirtualFileSystemModel::~EditorVirtualFileSystemModel()
{

}


QModelIndex EditorVirtualFileSystemModel::index(int row, int column, const QModelIndex &parent /*= QModelIndex( ) */) const
{
    return QModelIndex();
}

QModelIndex EditorVirtualFileSystemModel::parent(const QModelIndex &child) const
{
    return QModelIndex();
}

int EditorVirtualFileSystemModel::rowCount(const QModelIndex &parent /*= QModelIndex( ) */) const
{
    return 0;
}

int EditorVirtualFileSystemModel::columnCount(const QModelIndex &parent /*= QModelIndex( ) */) const
{
    return 0;
}

bool EditorVirtualFileSystemModel::hasChildren(const QModelIndex &parent /*= QModelIndex( ) */) const
{
    return false;
}

QVariant EditorVirtualFileSystemModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole */) const
{
    return QVariant();
}

QVariant EditorVirtualFileSystemModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole */) const
{
    return QVariant();
}
