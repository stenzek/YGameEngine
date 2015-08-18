#include "Editor/PrecompiledHeader.h"
#include "Editor/ToolMenuWidget.h"
#include "Editor/ResourceBrowser/EditorResourceBrowserWidget.h"
#include "Editor/ResourceBrowser/EditorStaticMeshImportDialog.h"
#include "Editor/MapEditor/EditorMapWindow.h"
#include "Editor/MapEditor/EditorEditMode.h"
#include "Editor/EditorResourceSelectionDialogModel.h"
#include "Editor/EditorProgressDialog.h"
#include "Editor/Editor.h"
#include "Engine/BlockMesh.h"
#include "Editor/BlockMeshEditor/EditorBlockMeshEditor.h"
#include "Engine/SkeletalAnimation.h"
#include "Editor/SkeletalAnimationEditor/EditorSkeletalAnimationEditor.h"
#include "Engine/SkeletalMesh.h"
#include "Editor/SkeletalMeshEditor/EditorSkeletalMeshEditor.h"
#include "Engine/StaticMesh.h"
#include "Editor/StaticMeshEditor/EditorStaticMeshEditor.h"

static const uint32 PREVIEW_ICON_SIZE = 96;

struct Ui_EditorResourceBrowserWidget
{
    ToolMenuWidget *toolbar;

    QAction *actionDirectoryBack;
    QAction *actionDirectoryUp;

    QAction *actionNewDirectory;
    QAction *actionDeleteResource;

    QAction *actionImportStaticMesh;

    QAction *actionInsertResource;
    QAction *actionEditBlockMesh;
    QAction *actionEditSkeletalAnimation;
    QAction *actionEditSkeletalMesh;
    QAction *actionEditStaticMesh;

    EditorResourceSelectionDialogModel *directoryTreeModel;
    EditorResourceSelectionDialogModel *resourceListModel;

    QTreeView *directoryTree;
    QListView *resourceList;

    void CreateUI(QWidget *root)
    {
        QVBoxLayout *rootLayout = new QVBoxLayout();

        root->setMinimumSize(300, 200);
        root->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

        // actions
        {
            actionDirectoryBack = new QAction(root->tr("Back"), root);
            actionDirectoryBack->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
            actionDirectoryUp = new QAction(root->tr("Up"), root);
            actionDirectoryUp->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
            
            actionNewDirectory = new QAction(root->tr("New Directory"), root);
            actionNewDirectory->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
            actionDeleteResource = new QAction(root->tr("Delete Resource"), root);
            actionDeleteResource->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));

            actionImportStaticMesh = new QAction(root->tr("Import Static Mesh..."), root);
            actionImportStaticMesh->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));

            actionInsertResource = new QAction(root->tr("Insert Resource"), root);
            actionEditBlockMesh = new QAction(root->tr("Edit Block Mesh..."), root);
            actionEditSkeletalAnimation = new QAction(root->tr("Edit Skeletal Animation..."), root);
            actionEditSkeletalMesh = new QAction(root->tr("Edit Skeletal Mesh..."), root);
            actionEditStaticMesh = new QAction(root->tr("Edit Static Mesh..."), root);
        }

        // toolbar
        {
            toolbar = new ToolMenuWidget(root, Qt::Horizontal, false);
            toolbar->addAction(actionDirectoryBack);
            toolbar->addAction(actionDirectoryUp);
            toolbar->addSeperator();
            toolbar->addAction(actionNewDirectory);
            toolbar->addAction(actionDeleteResource);
            toolbar->addSeperator();
            toolbar->addAction(actionImportStaticMesh);
            rootLayout->addWidget(toolbar);
        }

        // models
        {
            directoryTreeModel = new EditorResourceSelectionDialogModel(EmptyString, true, false, PREVIEW_ICON_SIZE, root);
            resourceListModel = new EditorResourceSelectionDialogModel(EmptyString, false, true, PREVIEW_ICON_SIZE, root);
        }

        // bottom half - splitter
        {
            QSplitter *bottomSplitter = new QSplitter(root);

            directoryTree = new QTreeView(bottomSplitter);
            directoryTree->setMaximumWidth(150);
            directoryTree->setModel(directoryTreeModel);
            directoryTree->hideColumn(1);
            directoryTree->hideColumn(2);
            directoryTree->setHeaderHidden(true);
            bottomSplitter->addWidget(directoryTree);

            resourceList = new QListView(bottomSplitter);
            resourceList->setModel(resourceListModel);
            resourceList->setViewMode(QListView::ListMode);
            resourceList->setIconSize(QSize(PREVIEW_ICON_SIZE, PREVIEW_ICON_SIZE));
            resourceList->setContextMenuPolicy(Qt::ActionsContextMenu);
            bottomSplitter->addWidget(resourceList);

            rootLayout->addWidget(bottomSplitter, 1);
        }

        root->setLayout(rootLayout);
    }

    void SetResourceListActionsForResourceType(const ResourceTypeInfo *pTypeInfo)
    {
        // clear everything out
        while (resourceList->actions().size() > 0)
            resourceList->removeAction(resourceList->actions().at(0));

        // don't bother checking if nothing is specified
        if (pTypeInfo == nullptr)
            return;

        // type specific
        if (pTypeInfo == OBJECT_TYPEINFO(BlockMesh))
        {
            resourceList->addAction(actionInsertResource);
            resourceList->addAction(actionEditBlockMesh);
        }
        else if (pTypeInfo == OBJECT_TYPEINFO(SkeletalAnimation))
        {
            resourceList->addAction(actionEditSkeletalAnimation);
        }
        else if (pTypeInfo == OBJECT_TYPEINFO(SkeletalMesh))
        {
            resourceList->addAction(actionInsertResource);
            resourceList->addAction(actionEditSkeletalMesh);
        }
        else if (pTypeInfo == OBJECT_TYPEINFO(StaticMesh))
        {
            resourceList->addAction(actionInsertResource);
            resourceList->addAction(actionEditStaticMesh);
        }
    }
};

void EditorResourceBrowserWidget::ConnectUIEvents()
{
    connect(m_ui->actionDirectoryBack, SIGNAL(triggered()), this, SLOT(OnActionDirectoryBackTriggered()));
    connect(m_ui->actionDirectoryUp, SIGNAL(triggered()), this, SLOT(OnActionDirectoryUpTriggered()));
    connect(m_ui->actionNewDirectory, SIGNAL(triggered()), this, SLOT(OnActionNewDirectoryTriggered()));
    connect(m_ui->actionDeleteResource, SIGNAL(triggered()), this, SLOT(OnActionDeleteResourceTriggered()));
    connect(m_ui->actionImportStaticMesh, SIGNAL(triggered()), this, SLOT(OnActionImportStaticMeshTriggered()));
    connect(m_ui->actionInsertResource, SIGNAL(triggered()), this, SLOT(OnActionInsertResourceTriggered()));
    connect(m_ui->actionEditBlockMesh, SIGNAL(triggered()), this, SLOT(OnActionEditBlockMeshTriggered()));
    connect(m_ui->actionEditSkeletalAnimation, SIGNAL(triggered()), this, SLOT(OnActionEditSkeletalAnimationTriggered()));
    connect(m_ui->actionEditSkeletalMesh, SIGNAL(triggered()), this, SLOT(OnActionEditSkeletalMeshTriggered()));
    connect(m_ui->actionEditStaticMesh, SIGNAL(triggered()), this, SLOT(OnActionEditStaticMeshTriggered()));
    connect(m_ui->directoryTree, SIGNAL(clicked(const QModelIndex &)), this, SLOT(OnDirectoryTreeItemClickedOrActivated(const QModelIndex &)));
    connect(m_ui->directoryTree, SIGNAL(activated(const QModelIndex &)), this, SLOT(OnDirectoryTreeItemClickedOrActivated(const QModelIndex &)));
    connect(m_ui->resourceList, SIGNAL(clicked(const QModelIndex &)), this, SLOT(OnResourceListItemClicked(const QModelIndex &)));
    connect(m_ui->resourceList, SIGNAL(activated(const QModelIndex &)), this, SLOT(OnResourceListItemActivated(const QModelIndex &)));
}

EditorResourceBrowserWidget::EditorResourceBrowserWidget(EditorMapWindow *pMapWindow, QWidget *parent)
    : QWidget(parent),
      m_pMapWindow(pMapWindow),
      m_ui(new Ui_EditorResourceBrowserWidget())
{
    m_ui->CreateUI(this);

    ConnectUIEvents();
}

EditorResourceBrowserWidget::~EditorResourceBrowserWidget()
{
    delete m_ui;
}

void EditorResourceBrowserWidget::NavigateToDirectory(const char *path)
{
    if (path == nullptr)
    {
        NavigateToDirectory("");
        return;
    }

    if (m_currentDirectory == path)
        return;

    // set us to disabled, this will grey everything out temporarily
    setEnabled(false);
    g_pEditor->ProcessBackgroundEvents();

    // actually change the path
    m_currentDirectory = path;
    m_ui->resourceList->setModel(nullptr);
    delete m_ui->resourceListModel;
    m_ui->resourceListModel = new EditorResourceSelectionDialogModel(m_currentDirectory, false, true, PREVIEW_ICON_SIZE, this);
    m_ui->resourceList->setModel(m_ui->resourceListModel);
    m_selectedResourceName.Clear();
    m_pSelectedResourceType = nullptr;

    // re-enable everything
    setEnabled(true);
}

void EditorResourceBrowserWidget::OnActionDirectoryBackTriggered()
{

}

void EditorResourceBrowserWidget::OnActionDirectoryUpTriggered()
{

}

void EditorResourceBrowserWidget::OnActionNewDirectoryTriggered()
{

}

void EditorResourceBrowserWidget::OnActionDeleteResourceTriggered()
{

}

void EditorResourceBrowserWidget::OnActionImportStaticMeshTriggered()
{
    // create a static mesh import dialog
    EditorStaticMeshImportDialog *staticMeshImportDialog = new EditorStaticMeshImportDialog(nullptr);
    staticMeshImportDialog->show();
}

void EditorResourceBrowserWidget::OnActionInsertResourceTriggered()
{
    if (m_pSelectedResourceType == nullptr)
        return;

    // pass to active edit mode
    if (m_pMapWindow->IsMapOpen())
        m_pMapWindow->GetActiveEditModePointer()->HandleResourceViewResourceActivatedEvent(m_pSelectedResourceType, m_selectedResourceName);
}

void EditorResourceBrowserWidget::OnActionEditBlockMeshTriggered()
{
    if (m_pSelectedResourceType != OBJECT_TYPEINFO(BlockMesh))
        return;

    // open progress dialog
    EditorProgressDialog progressDialog(this);
    progressDialog.show();

    // open a static mesh editor dialog
    EditorBlockMeshEditor *editor = new EditorBlockMeshEditor();
    if (!editor->Load(m_selectedResourceName, &progressDialog))
    {
        progressDialog.ModalError("Failed to open block mesh editor.");
        delete editor;
        return;
    }

    progressDialog.close();
    editor->show();
}

void EditorResourceBrowserWidget::OnActionEditSkeletalMeshTriggered()
{
    if (m_pSelectedResourceType != OBJECT_TYPEINFO(SkeletalMesh))
        return;

    // open progress dialog
    EditorProgressDialog progressDialog(this);
    progressDialog.show();

    // open a static mesh editor dialog
    EditorSkeletalMeshEditor *editor = new EditorSkeletalMeshEditor();
    if (!editor->Load(m_selectedResourceName, &progressDialog))
    {
        progressDialog.ModalError("Failed to open skeletal mesh editor.");
        delete editor;
        return;
    }

    progressDialog.close();
    editor->show();
}

void EditorResourceBrowserWidget::OnActionEditSkeletalAnimationTriggered()
{
    if (m_pSelectedResourceType != OBJECT_TYPEINFO(SkeletalAnimation))
        return;

    // open progress dialog
    EditorProgressDialog progressDialog(this);
    progressDialog.show();

    // open a static mesh editor dialog
    EditorSkeletalAnimationEditor *editor = new EditorSkeletalAnimationEditor();
    if (!editor->Load(m_selectedResourceName, &progressDialog))
    {
        progressDialog.ModalError("Failed to open skeletal animation editor.");
        delete editor;
        return;
    }

    progressDialog.close();
    editor->show();
}

void EditorResourceBrowserWidget::OnActionEditStaticMeshTriggered()
{
    if (m_pSelectedResourceType != OBJECT_TYPEINFO(StaticMesh))
        return;

    // open progress dialog
    EditorProgressDialog progressDialog(this);
    progressDialog.show();

    // open a static mesh editor dialog
    EditorStaticMeshEditor *editor = new EditorStaticMeshEditor();
    if (!editor->Load(m_selectedResourceName, &progressDialog))
    {
        progressDialog.ModalError("Failed to open static mesh editor.");
        delete editor;
        return;
    }

    progressDialog.close();
    editor->show();
}

void EditorResourceBrowserWidget::OnDirectoryTreeItemClickedOrActivated(const QModelIndex &index)
{
    const EditorResourceSelectionDialogModel::DirectoryNode *pDirectoryNode = m_ui->directoryTreeModel->GetDirectoryNodeForIndex(index);
    if (pDirectoryNode == nullptr)
        return;

    if (pDirectoryNode->IsDirectory())
    {
        // enter this directory, have to temporarily copy the string though, as navigating will destroy the node
        String directoryPathCopy(pDirectoryNode->GetFullName());
        NavigateToDirectory(directoryPathCopy);
    }
}

void EditorResourceBrowserWidget::OnResourceListItemClicked(const QModelIndex &index)
{
    const EditorResourceSelectionDialogModel::DirectoryNode *pDirectoryNode = m_ui->directoryTreeModel->GetDirectoryNodeForIndex(index);
    if (pDirectoryNode == nullptr || pDirectoryNode->IsDirectory() || pDirectoryNode->GetResourceTypeInfo() == nullptr)
    {
        m_selectedResourceName.Clear();
        m_pSelectedResourceType = nullptr;
        m_ui->SetResourceListActionsForResourceType(nullptr);
        return;
    }

    m_selectedResourceName = pDirectoryNode->GetFullName();
    m_pSelectedResourceType = pDirectoryNode->GetResourceTypeInfo();
    m_ui->SetResourceListActionsForResourceType(m_pSelectedResourceType);
}

void EditorResourceBrowserWidget::OnResourceListItemActivated(const QModelIndex &index)
{
    const EditorResourceSelectionDialogModel::DirectoryNode *pDirectoryNode = m_ui->directoryTreeModel->GetDirectoryNodeForIndex(index);
    if (pDirectoryNode == nullptr || pDirectoryNode->IsDirectory() || pDirectoryNode->GetResourceTypeInfo() == nullptr)
        return;

    // pass to active edit mode
    if (m_pMapWindow->IsMapOpen())
        m_pMapWindow->GetActiveEditModePointer()->HandleResourceViewResourceActivatedEvent(pDirectoryNode->GetResourceTypeInfo(), pDirectoryNode->GetFullName());
}
