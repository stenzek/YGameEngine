#pragma once
#include <QtCore/QAbstractItemModel>

class SimpleTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    class Container;
    class Item;

    class Item
    {
        friend class SimpleTreeModel;
        Item(SimpleTreeModel *pModel, const Container *pParent, int parentIndex);
        virtual ~Item();

        virtual const bool IsContainer() const { return false; }

    public:
        const Container *GetParent() const { return m_pParent; }
        int GetParentIndex() const { return m_parentIndex; }
        const QString GetText(int column) const;
        void *GetUserData() const { return m_pUserData; }

        void SetText(int column, const QString text);
        void SetUserData(void *pUserData);

        QModelIndex CreateIndexRelativeToParent(int column = 0) const;
        QModelIndex CreateIndex(int column = 0) const;

    protected:
        SimpleTreeModel *m_pModel;
        const Container *m_pParent;
        int m_parentIndex;
        QString *m_pFields;
        int m_nColumns;
        void *m_pUserData;
    };

    class Container : public Item
    {
        friend class SimpleTreeModel;
        Container(SimpleTreeModel *pModel, Container *pParent, int parentIndex);
        virtual ~Container();

        virtual const bool IsContainer() const { return true; }

    public:
        // title
        const QString GetTitle() const { return GetText(0); }
        void SetTitle(const QString title) { SetText(0, title); }

        // create new item
        Item *CreateItem();
        Container *CreateContainer(const QString titleText);

        // item access
        const Item *GetItem(int index) const;
        Item *GetItem(int index);

        // get an item by userdata
        int FindItemIndexByPointer(const Item *pItem) const;
        int FindItemIndexByUserData(void *pUserData) const;

        // remove item
        void RemoveItem(Item *pItem);
        void RemoveItem(int index);

        // remove all items
        void RemoveAllItems();

    private:
        QList<Item *> m_items;
    };

public:
    SimpleTreeModel(int nColumns, QObject *pParent = nullptr);
    virtual ~SimpleTreeModel();

    // access
    int GetColumnCount() const { return m_nColumns; }
    QString GetColumnTitle(int i) const { Q_ASSERT(i >= 0 && i < m_nColumns); return m_pColumnNames[i]; }
    void SetColumnTitle(int i, const QString name);

    // item creation
    Container *CreateContainer(const QString titleText, Container *pContainer = nullptr);
    Item *CreateItem(Container *pContainer = nullptr);

    // root item access
    const Item *GetRootItem() const { return &m_rootItem; }
    Item *GetRootItem() { return &m_rootItem; }

    // pushes through to root item
    const Item *GetItem(int index) const { return m_rootItem.GetItem(index); }
    Item *GetItem(int index) { return m_rootItem.GetItem(index); }
    int FindItemIndexByPointer(const Item *pItem) const { return m_rootItem.FindItemIndexByPointer(pItem); }
    int FindItemIndexByUserData(void *pUserData) const { return m_rootItem.FindItemIndexByUserData(pUserData); }
    void RemoveItem(Item *pItem) { m_rootItem.RemoveItem(pItem); }
    void RemoveItem(int index) { m_rootItem.RemoveItem(index); }

    // virtual interface methods
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private:
    int m_nColumns;
    QString *m_pColumnNames;
    Container m_rootItem;
};
