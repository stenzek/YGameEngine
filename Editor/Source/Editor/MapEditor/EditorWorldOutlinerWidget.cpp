#include "Editor/PrecompiledHeader.h"
#include "Editor/MapEditor/EditorWorldOutlinerWidget.h"
#include "Editor/MapEditor/EditorMap.h"
#include "Editor/MapEditor/EditorMapEntity.h"
#include "ResourceCompiler/ObjectTemplate.h"
#include "Editor/EditorHelpers.h"

EditorWorldOutlinerWidget::EditorWorldOutlinerWidget(QWidget *pParent)
    : QWidget(pParent),
      m_pSearchBox(nullptr),
      m_pTreeView(nullptr),
      m_model(3),
      m_pEntityContainer(nullptr),
      m_pVolumesContainer(nullptr)
{
    // create ui
    {
        QSizePolicy expandAndVerticalStretchSizePolicy;
        expandAndVerticalStretchSizePolicy.setHorizontalPolicy(QSizePolicy::Expanding);
        expandAndVerticalStretchSizePolicy.setVerticalPolicy(QSizePolicy::Expanding);
        expandAndVerticalStretchSizePolicy.setVerticalStretch(1);

        QFormLayout *formLayout = new QFormLayout(this);
        formLayout->setContentsMargins(2, 2, 2, 2);

        m_pSearchBox = new QLineEdit(this);
        m_pSearchBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_pSearchBox->setFixedHeight(20);
        formLayout->addRow(tr("Search: "), m_pSearchBox);

        m_pTreeView = new QTreeView(this);
        m_pTreeView->setSizePolicy(expandAndVerticalStretchSizePolicy);
        formLayout->addRow(m_pTreeView);

        setLayout(formLayout);
    }

    // initialize model columns
    m_model.SetColumnTitle(0, tr("Name"));
    m_model.SetColumnTitle(1, tr("Type"));
    m_model.SetColumnTitle(2, tr("Location"));

    // initialize model containers
    m_pEntityContainer = m_model.CreateContainer(tr("Entities"));
    m_pVolumesContainer = m_model.CreateContainer(tr("Volumes"));

    // set the model
    m_pTreeView->setModel(&m_model);

    // connect events
    connect(m_pTreeView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(OnTreeViewItemClicked(const QModelIndex &)));
    connect(m_pTreeView, SIGNAL(activated(const QModelIndex &)), this, SLOT(OnTreeViewItemActivated(const QModelIndex &)));
}

EditorWorldOutlinerWidget::~EditorWorldOutlinerWidget()
{
    m_pTreeView->setModel(nullptr);
}

void EditorWorldOutlinerWidget::Clear()
{
    m_pEntityContainer->RemoveAllItems();
    m_pVolumesContainer->RemoveAllItems();
}

void EditorWorldOutlinerWidget::AddEntity(const EditorMapEntity *pEntity)
{
    SimpleTreeModel::Item *pItem = m_pEntityContainer->CreateItem();
    pItem->SetText(0, ConvertStringToQString(pEntity->GetEntityData()->GetEntityName()));
    pItem->SetText(1, ConvertStringToQString(pEntity->GetTemplate()->GetTypeName()));
    pItem->SetText(2, ConvertStringToQString(StringConverter::Float3ToString(pEntity->GetPosition())));
    pItem->SetUserData(const_cast<void *>(reinterpret_cast<const void *>(pEntity)));
}

void EditorWorldOutlinerWidget::OnEntityPositionChanged(const EditorMapEntity *pEntity)
{
    int itemIndex = m_pEntityContainer->FindItemIndexByUserData(const_cast<void *>(reinterpret_cast<const void *>(pEntity)));
    DebugAssert(itemIndex >= 0);

    SimpleTreeModel::Item *pItem = m_pEntityContainer->GetItem(itemIndex);
    pItem->SetText(2, ConvertStringToQString(StringConverter::Float3ToString(pEntity->GetPosition())));
}

void EditorWorldOutlinerWidget::RemoveEntity(const EditorMapEntity *pEntity)
{
    int itemIndex = m_pEntityContainer->FindItemIndexByUserData(const_cast<void *>(reinterpret_cast<const void *>(pEntity)));
    DebugAssert(itemIndex >= 0);

    m_pEntityContainer->RemoveItem(itemIndex);
}

void EditorWorldOutlinerWidget::OnTreeViewItemClicked(const QModelIndex &index)
{
    const SimpleTreeModel::Item *pItem = reinterpret_cast<SimpleTreeModel::Item *>(index.internalPointer());
    if (pItem == nullptr)
        return;

    if (pItem->GetUserData() != nullptr)
    {
        if (pItem->GetParent() == m_pEntityContainer)
            OnEntitySelected(reinterpret_cast<EditorMapEntity *>(pItem->GetUserData()));
        //else if (pItem->GetParent() == m_pVolumesContainer)
            //OnVolumeSelected();
    }
}

void EditorWorldOutlinerWidget::OnTreeViewItemActivated(const QModelIndex &index)
{
    const SimpleTreeModel::Item *pItem = reinterpret_cast<SimpleTreeModel::Item *>(index.internalPointer());
    if (pItem == nullptr)
        return;

    if (pItem->GetUserData() != nullptr)
    {
        if (pItem->GetParent() == m_pEntityContainer)
            OnEntityActivated(reinterpret_cast<EditorMapEntity *>(pItem->GetUserData()));
        //else if (pItem->GetParent() == m_pVolumesContainer)
            //OnVolumeActivated();
    }
}
