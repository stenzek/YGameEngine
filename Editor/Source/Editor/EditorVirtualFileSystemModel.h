#pragma once
#include "Editor/Common.h"

class EditorVirtualFileSystemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    EditorVirtualFileSystemModel(QWidget *pParent = NULL);
    ~EditorVirtualFileSystemModel();

    virtual QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex( ) ) const;

    virtual QModelIndex parent( const QModelIndex &child ) const;

    virtual int rowCount( const QModelIndex &parent = QModelIndex( ) ) const;

    virtual int columnCount( const QModelIndex &parent = QModelIndex( ) ) const;

    virtual bool hasChildren( const QModelIndex &parent = QModelIndex( ) ) const;

    virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

};

