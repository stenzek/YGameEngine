#pragma once
#include "Editor/Common.h"
#include "Editor/SimpleTreeModel.h"

class EditorMap;
class EditorMapEntity;
class EditorWorldOutlinerModel;

class EditorWorldOutlinerWidget : public QWidget
{
    Q_OBJECT

public:
    EditorWorldOutlinerWidget(QWidget *pParent);
    virtual ~EditorWorldOutlinerWidget();

    // called when map is removed
    void Clear();

    // entity interface
    void AddEntity(const EditorMapEntity *pEntity);
    void OnEntityPositionChanged(const EditorMapEntity *pEntity);
    void RemoveEntity(const EditorMapEntity *pEntity);

Q_SIGNALS:
    void OnEntitySelected(const EditorMapEntity *pEntity);
    void OnEntityActivated(const EditorMapEntity *pEntity);

private Q_SLOTS:
    void OnTreeViewItemClicked(const QModelIndex &index);
    void OnTreeViewItemActivated(const QModelIndex &index);

private:
    // ui elements
    QLineEdit *m_pSearchBox;
    QTreeView *m_pTreeView;

    // lazy, so we copy to this model
    SimpleTreeModel m_model;

    // various headers
    SimpleTreeModel::Container *m_pEntityContainer;
    SimpleTreeModel::Container *m_pVolumesContainer;
};
