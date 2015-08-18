#include "Editor/PrecompiledHeader.h"
#include "Editor/Editor.h"
#include "Editor/MapEditor/EditorEntityEditMode.h"
#include "Editor/MapEditor/EditorMap.h"
#include "Editor/MapEditor/EditorMapWindow.h"
#include "Editor/MapEditor/EditorMapViewport.h"
#include "Editor/MapEditor/EditorMapEntity.h"
#include "Editor/MapEditor/ui_EditorMapWindow.h"
#include "Editor/EditorVisual.h"
#include "Editor/EditorPropertyEditorDialog.h"
#include "Editor/EditorHelpers.h"
#include "Editor/ToolMenuWidget.h"
#include "Engine/Entity.h"
#include "Engine/ResourceManager.h"
#include "Engine/StaticMesh.h"
#include "Renderer/Renderer.h"
#include "Renderer/VertexFactories/PlainVertexFactory.h"
#include "MathLib/CollisionDetection.h"
#include "ResourceCompiler/ObjectTemplate.h"
#include "MapCompiler/MapSource.h"
Log_SetChannel(EditorEntityEditMode);

const uint32 EDITOR_ENTITY_ID_WIDGET_AXIS_X = 0xFFFFFFFC;
const uint32 EDITOR_ENTITY_ID_WIDGET_AXIS_Y = 0xFFFFFFFD;
const uint32 EDITOR_ENTITY_ID_WIDGET_AXIS_Z = 0xFFFFFFFE;
const uint32 EDITOR_ENTITY_ID_WIDGET_AXIS_ANY = 0xFFFFFFFF;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct ui_EditorEntityEditMode
{
    QWidget *root;

    QAction *actionWidgetSelect;
    QAction *actionWidgetMove;
    QAction *actionWidgetRotate;
    QAction *actionWidgetScale;

    QAction *actionCreateEntity;
    QAction *actionDeleteEntity;

    QAction *actionFilterSelection;
    QAction *actionDeselectAll;

    QAction *actionSelectionLayers;
    QAction *actionSnapSelectionToGrid;

    QLabel *selectionTitleText;
    QListWidget *selectionComponents;
    QAction *actionAddComponent;
    QAction *actionRemoveComponent;

    QAction *actionPushLeft;
    QAction *actionPushRight;
    QAction *actionPushForward;
    QAction *actionPushBack;
    QAction *actionPushUp;
    QAction *actionPushDown;

    QGroupBox *pushContainer;
    QComboBox *pushShape;
    QDoubleSpinBox *pushMaxDistance;

    void CreateUI(QWidget *parentWidget)
    {
        QVBoxLayout *rootLayout;

        root = new QWidget(parentWidget);
        rootLayout = new QVBoxLayout(root);
        rootLayout->setSizeConstraint(QLayout::SetMaximumSize);
        rootLayout->setSpacing(2);
        rootLayout->setMargin(0);

        // create all actions first
        {
            actionWidgetSelect = new QAction(root);
            actionWidgetSelect->setText(root->tr("Select Entities"));
            actionWidgetSelect->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_Select.png")));
            actionWidgetSelect->setCheckable(true);
            actionWidgetMove = new QAction(root);
            actionWidgetMove->setText(root->tr("Move Entities"));
            actionWidgetMove->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_Move.png")));
            actionWidgetMove->setCheckable(true);
            actionWidgetRotate = new QAction(root);
            actionWidgetRotate->setText(root->tr("Rotate Entities"));
            actionWidgetRotate->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_Rotate.png")));
            actionWidgetRotate->setCheckable(true);
            actionWidgetScale = new QAction(root);
            actionWidgetScale->setText(root->tr("Scale Entities"));
            actionWidgetScale->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_Scale.png")));
            actionWidgetScale->setCheckable(true);

            actionCreateEntity = new QAction(root);
            actionCreateEntity->setText(root->tr("Create Entity"));
            actionCreateEntity->setIcon(QIcon(QStringLiteral(":/editor/icons/NewDocumentHS.png")));
            actionDeleteEntity = new QAction(root);
            actionDeleteEntity->setText(root->tr("Delete Entities"));
            actionDeleteEntity->setIcon(QIcon(QStringLiteral(":/editor/icons/DeleteHS.png")));

            actionFilterSelection = new QAction(root);
            actionFilterSelection->setText(root->tr("Filter Selection"));
            actionFilterSelection->setIcon(QIcon(QStringLiteral(":/editor/icons/FullScreenHH.png")));
            actionDeselectAll = new QAction(root);
            actionDeselectAll->setText(root->tr("Deselect All"));
            actionDeselectAll->setIcon(QIcon(QStringLiteral(":/editor/icons/FullScreenHH.png")));

            actionSnapSelectionToGrid = new QAction(root->tr("Snap To Grid"), root);
            actionSnapSelectionToGrid->setIcon(QIcon(QStringLiteral(":/editor/icons/FullScreenHH.png")));

            actionSelectionLayers = new QAction(root->tr("Entity Layers"), root);
            actionSelectionLayers->setIcon(QIcon(QStringLiteral(":/editor/icons/NewDirectory_16x16.png")));
            actionAddComponent = new QAction(root->tr("Add Component"), root);
            actionAddComponent->setIcon(QIcon(QStringLiteral(":/editor/icons/NewDocumentHS.png")));
            actionRemoveComponent = new QAction(root->tr("Remove Component"), root);
            actionRemoveComponent->setIcon(QIcon(QStringLiteral(":/editor/icons/DeleteHS.png")));

            actionPushLeft = new QAction(root);
            actionPushLeft->setText(root->tr("Push Left"));
            actionPushLeft->setIcon(QIcon(QStringLiteral(":/editor/icons/AlignObjectsLeftHS.png")));
            actionPushRight = new QAction(root);
            actionPushRight->setText(root->tr("Push Right"));
            actionPushRight->setIcon(QIcon(QStringLiteral(":/editor/icons/AlignObjectsRightHS.png")));
            actionPushForward = new QAction(root);
            actionPushForward->setText(root->tr("Push Forward"));
            actionPushForward->setIcon(QIcon(QStringLiteral(":/editor/icons/AlignObjectsCenteredVerticalHS.png")));
            actionPushBack = new QAction(root);
            actionPushBack->setText(root->tr("Push Back"));
            actionPushBack->setIcon(QIcon(QStringLiteral(":/editor/icons/AlignObjectsTopHS.png")));
            actionPushUp = new QAction(root);
            actionPushUp->setText(root->tr("Push Up"));
            actionPushUp->setIcon(QIcon(QStringLiteral(":/editor/icons/AlignObjectsCenteredHorizontalHS.png")));
            actionPushDown = new QAction(root);
            actionPushDown->setText(root->tr("Push Down"));
            actionPushDown->setIcon(QIcon(QStringLiteral(":/editor/icons/AlignObjectsBottomHS.png")));
        }

        // fixed, ie always present buttons
        {
            ToolMenuWidget *fixedToolMenu = new ToolMenuWidget(root);
            fixedToolMenu->setBackgroundRole(QPalette::Background);
            fixedToolMenu->addAction(actionWidgetSelect);
            fixedToolMenu->addAction(actionWidgetMove);
            fixedToolMenu->addAction(actionWidgetRotate);
            fixedToolMenu->addAction(actionWidgetScale);
            fixedToolMenu->addAction(actionSnapSelectionToGrid);
            fixedToolMenu->addAction(actionCreateEntity);
            fixedToolMenu->addAction(actionDeleteEntity);
            fixedToolMenu->addAction(actionFilterSelection);
            fixedToolMenu->addAction(actionDeselectAll);
            rootLayout->addWidget(fixedToolMenu);
        }

        // operation-specific commands
        {
            QScrollArea *scrollArea = new QScrollArea(root);
            QWidget *scrollAreaWidget = new QWidget(scrollArea);
            scrollArea->setBackgroundRole(QPalette::Background);
            scrollArea->setFrameStyle(0);
            scrollArea->setWidgetResizable(true);
            scrollArea->setWidget(scrollAreaWidget);

            QVBoxLayout *scrollAreaLayout = new QVBoxLayout(scrollArea);
            scrollAreaLayout->setMargin(0);
            scrollAreaLayout->setSpacing(2);
            scrollAreaLayout->setContentsMargins(0, 0, 0, 0);

            // common entity menu
            {
                QGroupBox *commonGroupBox = new QGroupBox(root->tr("Selection Info"), scrollAreaWidget);
                commonGroupBox->setMinimumSize(1, 200);
                commonGroupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

                QVBoxLayout *commonLayout = new QVBoxLayout(commonGroupBox);
                commonLayout->setContentsMargins(0, 0, 0, 0);
                commonLayout->setMargin(0);

                // title
                {
                    QHBoxLayout *titleLayout = new QHBoxLayout(commonGroupBox);
                    titleLayout->setContentsMargins(4, 4, 4, 4);
                    titleLayout->setAlignment(Qt::AlignTop);

                    QLabel *titleIcon = new QLabel(commonGroupBox);
                    titleIcon->setPixmap(QPixmap(QStringLiteral(":/editor/icons/EditCodeHS.png")));
                    titleIcon->setFixedSize(32, 32);
                    titleIcon->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                    titleLayout->addWidget(titleIcon);

                    selectionTitleText = new QLabel(commonGroupBox);
                    selectionTitleText->setTextFormat(Qt::RichText);
                    selectionTitleText->setText("<strong>hi</strong><br>Fooooo");
                    titleLayout->addWidget(selectionTitleText);

                    commonLayout->addLayout(titleLayout);
                }

                // components
                {
                    QLabel *selectionComponentsLabel = new QLabel(root->tr("Components:"), commonGroupBox);
                    commonLayout->addWidget(selectionComponentsLabel);

                    selectionComponents = new QListWidget(commonGroupBox);
                    commonLayout->addWidget(selectionComponents, 1);
                }

                // tool menu
                {
                    ToolMenuWidget *layerToolMenu = new ToolMenuWidget(root);
                    layerToolMenu->addAction(actionAddComponent);
                    layerToolMenu->addAction(actionRemoveComponent);
                    layerToolMenu->addAction(actionSelectionLayers);
                    commonLayout->addWidget(layerToolMenu);
                }
                
                commonGroupBox->setLayout(commonLayout);
                scrollAreaLayout->addWidget(commonGroupBox);
            }

            // push menu
            {
                pushContainer = new QGroupBox(root->tr("Push Object"), root);

                QVBoxLayout *pushContainerLayout = new QVBoxLayout(pushContainer);
                pushContainerLayout->setMargin(0);
                pushContainerLayout->setSpacing(0);
                pushContainerLayout->setContentsMargins(4, 4, 4, 4);

                // push shape/distance
                {
                    QFormLayout *formLayout = new QFormLayout(pushContainer);
                    formLayout->setMargin(0);
                    formLayout->setSpacing(0);
                    formLayout->setContentsMargins(0, 0, 0, 0);

                    pushShape = new QComboBox(pushContainer);
                    pushShape->setEditable(false);
                    pushShape->addItem(root->tr("Axis-Aligned Box"));
                    pushShape->addItem(root->tr("Sphere"));
                    formLayout->addRow(root->tr("Shape"), pushShape);

                    pushMaxDistance = new QDoubleSpinBox(pushContainer);
                    pushMaxDistance->setMinimum(0.5);
                    pushMaxDistance->setMaximum(1000.0);
                    pushMaxDistance->setValue(100.0);
                    formLayout->addRow(root->tr("Distance"), pushMaxDistance);
                    pushContainerLayout->addLayout(formLayout);
                }

                QHBoxLayout *toolMenuLayout = new QHBoxLayout(pushContainer);
                toolMenuLayout->setMargin(0);
                toolMenuLayout->setSpacing(0);
                toolMenuLayout->setContentsMargins(0, 0, 0, 0);

                ToolMenuWidget *leftToolMenu = new ToolMenuWidget(pushContainer);
                ToolMenuWidget *rightToolMenu = new ToolMenuWidget(pushContainer);
                leftToolMenu->addAction(actionPushLeft);
                rightToolMenu->addAction(actionPushRight);
                leftToolMenu->addAction(actionPushForward);
                rightToolMenu->addAction(actionPushBack);
                leftToolMenu->addAction(actionPushUp);
                rightToolMenu->addAction(actionPushDown);

                toolMenuLayout->addWidget(leftToolMenu);
                toolMenuLayout->addWidget(rightToolMenu);
                pushContainerLayout->addLayout(toolMenuLayout);

                pushContainer->setLayout(pushContainerLayout);
                scrollAreaLayout->addWidget(pushContainer);

                pushContainer->hide();
            }

            // use remaining space
            scrollAreaLayout->addStretch(1);

            // finalize scroll area
            scrollAreaWidget->setLayout(scrollAreaLayout);
            rootLayout->addWidget(scrollArea, 1);
        }

        root->setLayout(rootLayout);
    }

    void OnEntityEditWidgetChanged(EDITOR_ENTITY_EDIT_MODE_WIDGET currentWidget)
    {
        actionWidgetSelect->setChecked((currentWidget == EDITOR_ENTITY_EDIT_MODE_WIDGET_SELECT));
        actionWidgetMove->setChecked((currentWidget == EDITOR_ENTITY_EDIT_MODE_WIDGET_MOVE));
        actionWidgetRotate->setChecked((currentWidget == EDITOR_ENTITY_EDIT_MODE_WIDGET_ROTATE));
        actionWidgetScale->setChecked((currentWidget == EDITOR_ENTITY_EDIT_MODE_WIDGET_SCALE));

        // only show push box on move
        (currentWidget == EDITOR_ENTITY_EDIT_MODE_WIDGET_MOVE) ? pushContainer->show() : pushContainer->hide();
    }
};

QWidget *EditorEntityEditMode::CreateUI(QWidget *pParentWidget)
{
    DebugAssert(m_ui == nullptr);
    m_ui = new ui_EditorEntityEditMode();
    m_ui->CreateUI(pParentWidget);

    // bind events
    connect(m_ui->actionWidgetSelect, SIGNAL(triggered(bool)), this, SLOT(OnUIWidgetSelectClicked(bool)));
    connect(m_ui->actionWidgetMove, SIGNAL(triggered(bool)), this, SLOT(OnUIWidgetMoveClicked(bool)));
    connect(m_ui->actionWidgetRotate, SIGNAL(triggered(bool)), this, SLOT(OnUIWidgetRotateClicked(bool)));
    connect(m_ui->actionWidgetScale, SIGNAL(triggered(bool)), this, SLOT(OnUIWidgetScaleClicked(bool)));
    connect(m_ui->actionCreateEntity, SIGNAL(triggered()), this, SLOT(OnUIActionCreateEntityTriggered()));
    connect(m_ui->actionDeleteEntity, SIGNAL(triggered()), this, SLOT(OnUIActionDeleteEntityTriggered()));
    connect(m_ui->actionFilterSelection, SIGNAL(triggered()), this, SLOT(OnUIOptionsFilterSelectionClicked()));    
    connect(m_ui->actionSelectionLayers, SIGNAL(triggered()), this, SLOT(OnUIActionEntityLayersTriggered()));
    connect(m_ui->actionDeselectAll, SIGNAL(triggered()), this, SLOT(OnUIActionDeselectAllTriggered()));
    connect(m_ui->actionPushLeft, SIGNAL(triggered()), this, SLOT(OnUIPushLeftClicked()));
    connect(m_ui->actionPushRight, SIGNAL(triggered()), this, SLOT(OnUIPushRightClicked()));
    connect(m_ui->actionPushForward, SIGNAL(triggered()), this, SLOT(OnUIPushForwardClicked()));
    connect(m_ui->actionPushBack, SIGNAL(triggered()), this, SLOT(OnUIPushBackClicked()));
    connect(m_ui->actionPushUp, SIGNAL(triggered()), this, SLOT(OnUIPushUpClicked()));
    connect(m_ui->actionPushDown, SIGNAL(triggered()), this, SLOT(OnUIPushDownClicked()));
    connect(m_ui->actionSnapSelectionToGrid, SIGNAL(triggered()), this, SLOT(OnUIActionSnapSelectionToGridTriggered()));
    connect(m_ui->actionAddComponent, SIGNAL(triggered()), this, SLOT(OnUIActionCreateComponentTriggered()));
    connect(m_ui->actionRemoveComponent, SIGNAL(triggered()), this, SLOT(OnUIActionRemoveComponentTriggered()));
    connect(m_ui->actionSelectionLayers, SIGNAL(triggered()), this, SLOT(OnUIActionSelectionLayersTriggered()));
    connect(m_ui->selectionComponents, SIGNAL(currentRowChanged(int)), this, SLOT(OnUISelectionComponentListCurrentRowChanged(int)));

    // update state
    m_ui->OnEntityEditWidgetChanged(m_activeWidget);

    return m_ui->root;
}

void EditorEntityEditMode::OnUIOptionsFilterSelectionClicked()
{

}

void EditorEntityEditMode::OnUIWidgetSelectClicked(bool checked)
{
    SetActiveWidget(EDITOR_ENTITY_EDIT_MODE_WIDGET_SELECT);
}

void EditorEntityEditMode::OnUIWidgetMoveClicked(bool checked)
{
    SetActiveWidget(EDITOR_ENTITY_EDIT_MODE_WIDGET_MOVE);
}

void EditorEntityEditMode::OnUIWidgetRotateClicked(bool checked)
{
    SetActiveWidget(EDITOR_ENTITY_EDIT_MODE_WIDGET_ROTATE);
}

void EditorEntityEditMode::OnUIWidgetScaleClicked(bool checked)
{
    SetActiveWidget(EDITOR_ENTITY_EDIT_MODE_WIDGET_SCALE);
}

void EditorEntityEditMode::OnUIPushLeftClicked()
{
    PushSelection(float3::NegativeUnitX, (float)m_ui->pushMaxDistance->value());
}

void EditorEntityEditMode::OnUIPushRightClicked()
{
    PushSelection(float3::UnitX, (float)m_ui->pushMaxDistance->value());
}

void EditorEntityEditMode::OnUIPushForwardClicked()
{
    PushSelection(float3::UnitY, (float)m_ui->pushMaxDistance->value());
}

void EditorEntityEditMode::OnUIPushBackClicked()
{
    PushSelection(float3::NegativeUnitY, (float)m_ui->pushMaxDistance->value());
}

void EditorEntityEditMode::OnUIPushUpClicked()
{
    PushSelection(float3::UnitZ, (float)m_ui->pushMaxDistance->value());
}

void EditorEntityEditMode::OnUIPushDownClicked()
{
    PushSelection(float3::NegativeUnitZ, (float)m_ui->pushMaxDistance->value());
}

void EditorEntityEditMode::OnUIActionCreateEntityTriggered()
{
    QMenu entityTypeMenu;
    EditorHelpers::CreateEntityTypeMenu(&entityTypeMenu, true);

    QAction *pChoice = entityTypeMenu.exec(QCursor::pos());
    if (pChoice != nullptr)
    {
        // get type name
        QString typeName(pChoice->data().toString());

        // create this entity type
        EditorMapViewport *pViewport = m_pMap->GetMapWindow()->GetActiveViewport();
        if (pViewport != nullptr)
            CreateEntityUsingViewport(pViewport, ConvertQStringToString(typeName));
        else
            m_pMap->CreateEntity(ConvertQStringToString(typeName));
    }
}

void EditorEntityEditMode::OnUIActionDeleteEntityTriggered()
{
    DeleteSelection();
}

void EditorEntityEditMode::OnUIActionEntityLayersTriggered()
{

}

void EditorEntityEditMode::OnUIActionSnapSelectionToGridTriggered()
{
    SnapSelectionToGrid();
}

void EditorEntityEditMode::OnUIActionCreateComponentTriggered()
{
    // should only have a single selection
    if (m_selectedEntities.GetSize() != 1)
        return;

    // components can only be added to entities, not static objects
    EditorMapEntity *pSelectedEntity = m_selectedEntities[0];
    if (!pSelectedEntity->IsEntity())
        return;

    QMenu componentTypeMenu;
    EditorHelpers::CreateComponentTypeMenu(&componentTypeMenu, true);

    QAction *pChoice = componentTypeMenu.exec(QCursor::pos());
    if (pChoice != nullptr)
    {
        // get type name
        QString typeName(pChoice->data().toString());

        // add it
        int32 componentIndex = pSelectedEntity->CreateComponent(ConvertQStringToString(typeName));
        if (componentIndex >= 0)
        {
            // update ui
            UpdatePropertyEditor();
            SetSelectedComponentIndex(componentIndex);
        }
    }
}

void EditorEntityEditMode::OnUIActionRemoveComponentTriggered()
{

}

void EditorEntityEditMode::OnUIActionSelectionLayersTriggered()
{

}

void EditorEntityEditMode::OnUISelectionComponentListCurrentRowChanged(int currentRow)
{
    SetSelectedComponentIndex(currentRow);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


EditorEntityEditMode::EditorEntityEditMode(EditorMap *pMap)
    : EditorEditMode(pMap),
      m_ui(nullptr),
      m_pRotationGismoTexture(nullptr),
      m_activeWidget(EDITOR_ENTITY_EDIT_MODE_WIDGET_SELECT),
      m_lastMousePositionEntityId(0),
      m_currentState(STATE_NONE)
{
    m_mouseSelectionRegion[0].SetZero();
    m_mouseSelectionRegion[1].SetZero();
    Y_memzero(m_currentStateData, sizeof(m_currentStateData));
    m_currentStateDataVector.SetZero();
    m_selectionBounds.SetZero();
    m_selectedComponentIndex = -1;
    m_selectionAxisPosition.SetZero();
    m_selectionAxisRotation.SetIdentity();

    
}

EditorEntityEditMode::~EditorEntityEditMode()
{
    delete m_ui;
}

bool EditorEntityEditMode::Initialize(ProgressCallbacks *pProgressCallbacks)
{
    // base class init
    if (!EditorEditMode::Initialize(pProgressCallbacks))
        return false;

    // load resources
    if ((m_pRotationGismoTexture = g_pResourceManager->GetTexture2D("resources/editor/textures/rotation_gismo")) == nullptr)
        m_pRotationGismoTexture = g_pResourceManager->GetDefaultTexture2D();
    if ((m_pRotationGismoTransparentTexture = g_pResourceManager->GetTexture2D("resources/editor/textures/rotation_gismo_transparent")) == nullptr)
        m_pRotationGismoTransparentTexture = g_pResourceManager->GetDefaultTexture2D();

    // ok
    return true;
}

void EditorEntityEditMode::Activate()
{
    // fall through
    EditorEditMode::Activate();

    // set the initial properties window caption
    //m_pMap->GetMapWindow()->GetUI()->propertyEditor->SetTitleText("No selection");
}

void EditorEntityEditMode::Deactivate()
{
    // clear any selection
    ClearSelection();

    // clear state variables
    m_lastMousePositionEntityId = 0;
    m_mouseSelectionRegion[0].SetZero();
    m_mouseSelectionRegion[1].SetZero();
    m_currentState = STATE_NONE;
    Y_memzero(m_currentStateData, sizeof(m_currentStateData));

    // fall through
    EditorEditMode::Deactivate();
}

void EditorEntityEditMode::SetActiveWidget(EDITOR_ENTITY_EDIT_MODE_WIDGET widget)
{
    DebugAssert(widget < EDITOR_ENTITY_EDIT_MODE_WIDGET_COUNT);
    if (widget == m_activeWidget)
    {
        m_ui->OnEntityEditWidgetChanged(m_activeWidget);
        return;
    }

    m_activeWidget = widget;
    m_ui->OnEntityEditWidgetChanged(m_activeWidget);
    m_pMap->GetMapWindow()->RedrawAllViewports();
}

void EditorEntityEditMode::AddSelection(EditorMapEntity *pEntity)
{
    for (uint32 i = 0; i < m_selectedEntities.GetSize(); i++)
    {
        if (m_selectedEntities[i] == pEntity)
            return;
    }

    // update selected state
    pEntity->OnSelectedStateChanged(true);

    // shouldn't have a component selected
    m_selectedComponentIndex = -1;

    // add to selection array
    m_selectedEntities.Add(pEntity);

    // update property editor
    UpdatePropertyEditor();

    // update bounds
    UpdateSelectionBounds();
    UpdateSelectionAxis();

    // redraw
    m_pMap->GetMapWindow()->RedrawAllViewports();

    //EditorPropertyEditorDialog *pd = new EditorPropertyEditorDialog(m_pMap->GetMapWindow());
    //pd->PopulateForEntity(m_pMap, pEntity);
    //pd->show();
}

void EditorEntityEditMode::RemoveSelection(EditorMapEntity *pEntity)
{
    for (uint32 i = 0; i < m_selectedEntities.GetSize(); i++)
    {
        if (m_selectedEntities[i] == pEntity)
        {
            pEntity->OnSelectedStateChanged(false);
            m_selectedEntities.OrderedRemove(i);
            break;
        }
    }

    // shouldn't have a component selected
    m_selectedComponentIndex = -1;

    // update property editor
    UpdatePropertyEditor();

    // update bounds
    UpdateSelectionBounds();
    UpdateSelectionAxis();

    // redraw
    m_pMap->GetMapWindow()->RedrawAllViewports();
}

void EditorEntityEditMode::ClearSelection()
{
    for (uint32 i = 0; i < m_selectedEntities.GetSize(); i++)
        m_selectedEntities[i]->OnSelectedStateChanged(false);

    m_selectedComponentIndex = -1;
    m_selectedEntities.Clear();
    m_selectionAxisPosition.SetZero();

    // update property editor
    UpdatePropertyEditor();

    // update bounds
    UpdateSelectionBounds();
    UpdateSelectionAxis();

    // redraw
    m_pMap->GetMapWindow()->RedrawAllViewports();
}

void EditorEntityEditMode::SetSelectedComponentIndex(int32 componentIndex)
{
    if (m_selectedEntities.GetSize() != 1)
    {
        m_selectedComponentIndex = -1;
        return;
    }

    EditorMapEntity *pEntity = m_selectedEntities[0];
    if (!pEntity->IsEntity())
    {
        m_selectedComponentIndex = -1;
        return;
    }

    // deselect current
    if ((uint32)m_selectedComponentIndex < pEntity->GetComponentCount())
        pEntity->OnComponentSelectedStateChanged(m_selectedComponentIndex, false);
    pEntity->OnSelectedStateChanged(true);

    // deselecting?
    if (componentIndex < 0)
    {
        m_selectedComponentIndex = -1;
        m_pMap->RedrawAllViewports();
        return;
    }

    // selecting?
    if ((uint32)componentIndex >= pEntity->GetComponentCount())
    {
        m_selectedComponentIndex = -1;
        return;
    }

    // set it
    pEntity->OnSelectedStateChanged(false);
    pEntity->OnComponentSelectedStateChanged(componentIndex, true);
    m_selectedComponentIndex = componentIndex;
    UpdatePropertyEditorForComponent();
    m_pMap->RedrawAllViewports();
}

bool EditorEntityEditMode::IsSelected(const EditorMapEntity *pEntity) const
{
    for (uint32 i = 0; i < m_selectedEntities.GetSize(); i++)
    {
        if (m_selectedEntities[i] == pEntity)
            return true;
    }

    return false;
}

void EditorEntityEditMode::MoveSelection(const float3 &moveDelta)
{
    if (m_selectedEntities.GetSize() == 0)
        return;

    // handle component case
    if (m_selectedEntities.GetSize() == 1 && m_selectedComponentIndex >= 0)
    {
        EditorMapEntity *pEntity = m_selectedEntities[0];
        float3 currentValue(StringConverter::StringToFloat3(pEntity->GetComponentPropertyValue(m_selectedComponentIndex, "LocalPosition")));
        pEntity->SetComponentPropertyValue(m_selectedComponentIndex, "LocalPosition", StringConverter::Float3ToString(currentValue + moveDelta));
    }
    else
    {
        for (uint32 i = 0; i < m_selectedEntities.GetSize(); i++)
        {
            EditorMapEntity *pEntity = m_selectedEntities[i];

            // we'll use the position from the entity visual, since that should be up-to-date anyway.
            float3 newPosition(pEntity->GetPosition() + moveDelta);

            // commit it to the source
            pEntity->SetPosition(newPosition);
        }
    }
}

void EditorEntityEditMode::RotateSelection(const float3 &rotateDelta)
{
    Quaternion rotationMod(Quaternion::FromEulerAngles(rotateDelta));
    if (m_selectedEntities.GetSize() == 0)
        return;

    for (uint32 i = 0; i < m_selectedEntities.GetSize(); i++)
    {
        EditorMapEntity *pEntity = m_selectedEntities[i];

        // we'll use the position from the entity visual, since that should be up-to-date anyway.
        Quaternion newRotation(pEntity->GetRotation() * rotationMod);

        // commit it to the source
        pEntity->SetRotation(newRotation);
    }
}

void EditorEntityEditMode::ScaleSelection(const float3 &scaleDelta)
{
    if (m_selectedEntities.GetSize() == 0)
        return;

    for (uint32 i = 0; i < m_selectedEntities.GetSize(); i++)
    {
        EditorMapEntity *pEntity = m_selectedEntities[i];

        // we'll use the position from the entity visual, since that should be up-to-date anyway.
        float3 newScale(pEntity->GetScale() + scaleDelta);

        // commit it to the source
        pEntity->SetScale(newScale);
    }
}

void EditorEntityEditMode::PushSelection(const float3 &pushDirection, float maxDistance /*= 100.0f*/)
{
    if (m_selectedEntities.GetSize() == 0)
        return;

    bool useBoundingSpheres = false;
    uint32 mainAxis = (pushDirection.x != 0.0f) ? 0 : ((pushDirection.y != 0.0f) ? 1 : 2);

    for (uint32 i = 0; i < m_selectedEntities.GetSize(); i++)
    {
        EditorMapEntity *pEntity = m_selectedEntities[i];
        float bestContactDistance = Y_FLT_INFINITE;

        if (!useBoundingSpheres)
        {
            // construct a new box with the extent of the axis extended to the max distance
            float3 boxCenter(pEntity->GetBoundingBox().GetCenter());
            float3 boxHalfExtents(pEntity->GetBoundingBox().GetExtents() * 0.5f);
            boxHalfExtents[mainAxis] = Max(boxHalfExtents[mainAxis], maxDistance * 0.5f);
            AABox searchBox(boxCenter - boxHalfExtents, boxCenter + boxHalfExtents);

            // loop over objects
            float3 displacement(pushDirection * maxDistance);
            m_pMap->EnumerateEntitiesInAABox(searchBox, [pEntity, &bestContactDistance, &displacement](const EditorMapEntity *pOtherEntity)
            {
                // skip entities without visuals
                if (pEntity == pOtherEntity || !pOtherEntity->IsVisualCreated())
                    return;

                float contactDistance = CollisionDetection::AABoxSweep(pEntity->GetBoundingBox().GetMinBounds(), pEntity->GetBoundingBox().GetMaxBounds(), displacement,
                                                                       pOtherEntity->GetBoundingBox().GetMinBounds(), pOtherEntity->GetBoundingBox().GetMaxBounds());

                bestContactDistance = Min(bestContactDistance, contactDistance);
            });
        }
        else
        {

        }

        // did we hit anything?
        if (bestContactDistance != Y_FLT_INFINITE)
        {
            // move in the push direction, this distance
            float3 newPosition(pEntity->GetPosition() + pushDirection * bestContactDistance);
            Log_DevPrintf("push contact distance %f", bestContactDistance);

            // commit it to the source
            pEntity->SetPosition(newPosition);
        }
        else
        {
            Log_DevPrintf("no contacts found");
        }
    }
}

void EditorEntityEditMode::SnapSelectionToGrid()
{
    if (m_selectedEntities.GetSize() == 0)
        return;

    float3 snapInterval(m_pMap->GetGridSnapInterval(), m_pMap->GetGridSnapInterval(), m_pMap->GetGridSnapInterval());

    for (uint32 i = 0; i < m_selectedEntities.GetSize(); i++)
    {
        EditorMapEntity *pEntity = m_selectedEntities[i];
        pEntity->SetPosition(pEntity->GetPosition().Snap(snapInterval));
    }
}

void EditorEntityEditMode::DeleteSelection()
{
    if (m_selectedEntities.GetSize() == 0)
        return;

    while (m_selectedEntities.GetSize() > 0)
    {
        EditorMapEntity *pEntity = m_selectedEntities[m_selectedEntities.GetSize() - 1];
        m_selectedEntities.PopBack();

        pEntity->OnSelectedStateChanged(false);
        m_pMap->DeleteEntity(pEntity);
    }

    m_selectionAxisPosition.SetZero();

    // update property editor
    UpdatePropertyEditor();

    // update bounds
    UpdateSelectionBounds();
    UpdateSelectionAxis();

    m_pMap->RedrawAllViewports();
}

void EditorEntityEditMode::CloneSelection(bool addToSelection, bool clearOldSelection)
{
    if (m_selectedEntities.GetSize() == 0)
        return;

    // create new entities
    EditorMapEntity **ppNewEntities = (EditorMapEntity **)alloca(sizeof(EditorMapEntity *) * m_selectedEntities.GetSize());
    uint32 nNewEntities = 0;
    for (uint32 selectionIndex = 0; selectionIndex < m_selectedEntities.GetSize(); selectionIndex++)
    {
        // copy it
        if ((ppNewEntities[nNewEntities] = m_pMap->CopyEntity(m_selectedEntities[selectionIndex])) != nullptr)
            nNewEntities++;
    }

    // clear old selection first
    if (clearOldSelection)
        ClearSelection();

    // then add the new
    if (addToSelection)
    {
        for (uint32 i = 0; i < nNewEntities; i++)
            AddSelection(ppNewEntities[i]);
    }
}

void EditorEntityEditMode::Update(const float timeSinceLastUpdate)
{
    EditorEditMode::Update(timeSinceLastUpdate);
}

void EditorEntityEditMode::OnPropertyEditorPropertyChanged(const char *propertyName, const char *propertyValue)
{
    SetPropertyOnSelection(propertyName, propertyValue);
}

void EditorEntityEditMode::OnActiveViewportChanged(EditorMapViewport *pOldActiveViewport, EditorMapViewport *pNewActiveViewport)
{
    EditorEditMode::OnActiveViewportChanged(pOldActiveViewport, pNewActiveViewport);
    
    // clear state variables
    m_lastMousePositionEntityId = 0;
    m_mouseSelectionRegion[0].SetZero();
    m_mouseSelectionRegion[1].SetZero();
    m_currentState = STATE_NONE;
    Y_memzero(m_currentStateData, sizeof(m_currentStateData));

    // cleanp old viewport, if there is one
    if (pOldActiveViewport != NULL)
    {
        pOldActiveViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_ARROW);
    }

    // setup new viewport, if there is one
    if (pNewActiveViewport != NULL)
    {
        pNewActiveViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_ARROW);
    }
}

bool EditorEntityEditMode::HandleViewportKeyboardInputEvent(EditorMapViewport *pViewport, const QKeyEvent *pKeyboardEvent)
{
    // our handlers
    switch (pKeyboardEvent->type())
    {
    case QEvent::KeyPress:
        {
        }
        break;

    case QEvent::KeyRelease:
        {
            switch (pKeyboardEvent->key())
            {
            case Qt::Key_Escape:
                ClearSelection();
                return true;

            case Qt::Key_Delete:
                DeleteSelection();
                return true;

#if 0
                // temp testing stuff
            case Qt::Key_L:
                {
                    PointLightEntity *pEntity = new PointLightEntity(m_pEditorMap->GetWorld()->AllocateEntityId());
                    pEntity->SetPosition(m_Camera.GetCameraPosition());
                    pEntity->SetRange(512.0f);
                    pEntity->SetCastDynamicShadows(true);
                    pEntity->SetCastStaticShadows(true);
                    m_pEditorMap->GetWorld()->AddEntity(pEntity);
                    pEntity->Release();

                    Log_DevPrintf("spawned test light @ %.3f %.3f %.3f", m_Camera.GetCameraPosition().x, m_Camera.GetCameraPosition().y, m_Camera.GetCameraPosition().z);
                    pViewport->FlagForRedraw();
                }
                break;
#endif
            }
        }
        break;
    }

    // nothing left
    return EditorEditMode::HandleViewportKeyboardInputEvent(pViewport, pKeyboardEvent);
}

bool EditorEntityEditMode::HandleViewportMouseInputEvent(EditorMapViewport *pViewport, const QMouseEvent *pMouseEvent)
{
    if (pMouseEvent->type() == QEvent::MouseButtonPress && pMouseEvent->button() == Qt::LeftButton)
    {
        // if not currently in any state, determine which state to transition to
        if (m_currentState == STATE_NONE)
        {
            // what is under the texel that we are currently sitting on?
            if (m_lastMousePositionEntityId >= EDITOR_ENTITY_ID_WIDGET_AXIS_X && m_lastMousePositionEntityId <= EDITOR_ENTITY_ID_WIDGET_AXIS_ANY)
            {
                // under a selection axis. change to move mode, and set the correct movement axis
                m_currentState = STATE_WIDGET;
                m_currentStateData[0] = m_lastMousePositionEntityId - EDITOR_ENTITY_ID_WIDGET_AXIS_X;
                m_mouseSelectionRegion[0] = pViewport->GetMousePosition();
                m_currentStateDataVector.SetZero();

                // update the cursor
                switch (m_activeWidget)
                {
                case EDITOR_ENTITY_EDIT_MODE_WIDGET_MOVE:
                    pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_SIZING);
                    break;

                case EDITOR_ENTITY_EDIT_MODE_WIDGET_ROTATE:
                    pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_RIGHT_ARROW);
                    break;

                case EDITOR_ENTITY_EDIT_MODE_WIDGET_SCALE:
                    pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_MAGNIFIER);
                    break;
                }

                // redraw the viewport
                //pViewport->LockMouseCursor();
                pViewport->FlagForRedraw();
                return true;
            }
            else
            {
                // under nothing or an entity, so change to selection rect mode
                m_currentState = STATE_SELECTION_RECT;
                m_mouseSelectionRegion[0] = m_mouseSelectionRegion[1] = pViewport->GetMousePosition();
                m_currentStateData[0] = 0;
                UpdateSelection(pViewport);

                // update cursor + redraw
                pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_ARROW);
                pViewport->FlagForRedraw();
                return true;
            }
        }     
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonRelease && pMouseEvent->button() == Qt::LeftButton)
    {
        switch (m_currentState)
        {
        case STATE_SELECTION_RECT:
        case STATE_WIDGET:
            {
                m_mouseSelectionRegion[0].SetZero();
                m_mouseSelectionRegion[1].SetZero();
                m_currentState = STATE_NONE;
                Y_memzero(m_currentStateData, sizeof(m_currentStateData));

                //pViewport->UnlockMouseCursor();
                pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_ARROW);
                pViewport->FlagForRedraw();
                return true;
            }
            break;
        }
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonPress && pMouseEvent->button() == Qt::RightButton)
    {
        // right mouse button can enter a viewport state. if we are not in a state, determine which state to enter.
        if (m_currentState == STATE_NONE)
        {
            // right mouse button enters camera rotation state
            m_currentState = STATE_CAMERA_ROTATION;

            // lock cursor, set cursor, and redraw
            pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_CROSS);
            pViewport->LockMouseCursor();
            pViewport->FlagForRedraw();
            return true;
        }
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonRelease && pMouseEvent->button() == Qt::RightButton)
    {
        // if in camera rotation state, exit it
        if (m_currentState == STATE_CAMERA_ROTATION)
        {
            // exit the state
            m_currentState = STATE_NONE;
            Y_memzero(m_currentStateData, sizeof(m_currentStateData));

            // reset viewport state
            pViewport->UnlockMouseCursor();
            pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_ARROW);
            pViewport->FlagForRedraw();
            return true;
        }
    }
    else if (pMouseEvent->type() == QEvent::MouseMove)
    {
        int2 currentMousePosition(pViewport->GetMousePosition());
        int2 mousePositionDelta(pViewport->GetMouseDelta());
        uint32 lastMousePositionEntityId = m_lastMousePositionEntityId;
        uint32 mousePositionEntityId = pViewport->GetPickingTextureValue(currentMousePosition);
           
        // handle mouse movement based on state
        switch (m_currentState)
        {
        case STATE_NONE:
            {
                // if not in any state, but the entity id under the mouse changes, or is nonzero, we need to redraw it
                // so the hover overlay can be shown
                if (mousePositionEntityId != lastMousePositionEntityId)
                {
                    if (mousePositionEntityId >= EDITOR_ENTITY_ID_RESERVED_BEGIN)
                    {
                        // handle cursor updates
                        if (mousePositionEntityId >= EDITOR_ENTITY_ID_WIDGET_AXIS_X && mousePositionEntityId <= EDITOR_ENTITY_ID_WIDGET_AXIS_ANY)
                        {
                            switch (m_activeWidget)
                            {
                            case EDITOR_ENTITY_EDIT_MODE_WIDGET_MOVE:
                                pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_SIZING);
                                break;

                            case EDITOR_ENTITY_EDIT_MODE_WIDGET_ROTATE:
                                pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_RIGHT_ARROW);
                                break;

                            case EDITOR_ENTITY_EDIT_MODE_WIDGET_SCALE:
                                pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_MAGNIFIER);
                                break;
                            }

                            pViewport->FlagForRedraw();
                        }
                    }
                    else
                    {
                        if (lastMousePositionEntityId >= EDITOR_ENTITY_ID_WIDGET_AXIS_X && lastMousePositionEntityId <= EDITOR_ENTITY_ID_WIDGET_AXIS_ANY)
                        {
                            // reset cursor
                            pViewport->SetMouseCursor(EDITOR_CURSOR_TYPE_ARROW);
                        }

                        pViewport->FlagForRedraw();
                    }
                }
            }
            break;

        case STATE_CAMERA_ROTATION:
            {
                // pass difference through to camera
                pViewport->GetViewController().RotateFromMousePosition(mousePositionDelta);
                pViewport->FlagForRedraw();
            }
            break;

        case STATE_SELECTION_RECT:
            {
                // update selection rect bounds
                m_mouseSelectionRegion[1] = currentMousePosition;
                UpdateSelection(pViewport);
                pViewport->FlagForRedraw();
            }
            break;
                
        case STATE_WIDGET:
            {
                // get the center of the selection bounds, this is our reference point
                float3 selectionBoundsCenter(m_selectionBounds.GetCenter());

                // project this to the screen
                float3 projectedSelectionBoundsCenter(pViewport->GetViewController().Project(selectionBoundsCenter));

                // get the position with the mouse offset
                float3 projectedOffsetSelectionBoundsCenter(projectedSelectionBoundsCenter + float3((float)mousePositionDelta.x, (float)mousePositionDelta.y, 0.0f));

                // unproject back to world space
                float3 offsetSelectionBoundsCenter(pViewport->GetViewController().Unproject(projectedOffsetSelectionBoundsCenter));

                // axis mask
                static const float axisMask[4][3] =
                {
                    { 1.0f, 0.0f, 0.0f },       // move x
                    { 0.0f, 1.0f, 0.0f },       // move y
                    { 0.0f, 0.0f, 1.0f },       // move z
                    { 1.0f, 1.0f, 1.0f },       // move all
                };

                // determine move amount in worldspace on all axises
                float3 entityMoveAmount(offsetSelectionBoundsCenter - selectionBoundsCenter);

                // apply axis mask
                DebugAssert(m_currentStateData[0] < 4);
                entityMoveAmount = entityMoveAmount * float3(axisMask[m_currentStateData[0]]);

                // Logging
                /*Log_DevPrintf("mouseDiff = %d %d, selectionBoundsCenter = %.3f %.3f %.3f, projectedSelectionBoundsCenter = %.3f %.3f %.3f, offsetSelectionBoundsCenter = %.3f %.3f %.3f, entityMoveAmount = %.3f %.3f %.3f",
                                mousePositionDelta.x, mousePositionDelta.y,
                                selectionBoundsCenter.x, selectionBoundsCenter.y, selectionBoundsCenter.z,
                                projectedSelectionBoundsCenter.x, projectedSelectionBoundsCenter.y, projectedSelectionBoundsCenter.z,
                                offsetSelectionBoundsCenter.x, offsetSelectionBoundsCenter.y, offsetSelectionBoundsCenter.z,
                                entityMoveAmount.x, entityMoveAmount.y, entityMoveAmount.z);*/

                // if alt modifier is held down, create a copy of the entity unless it's already been done
                if (pMouseEvent->modifiers() & Qt::AltModifier && m_currentStateData[1] == 0)
                {
                    // copy selection
                    CloneSelection(true, true);

                    // flag as 'copied'
                    m_currentStateData[1] = 1;
                }

                // disable grid snap if shift is held down
                bool snapToGrid = true;
                if (pMouseEvent->modifiers() & Qt::ShiftModifier)
                    snapToGrid = false;

                // snapping to grid?
                if (snapToGrid)
                {
                    float3 lastTotalAmountMoved(m_currentStateDataVector);
                    float3 newTotalAmountMoved(lastTotalAmountMoved + entityMoveAmount);

                    // snap the total amount to the grid interval
                    float3 snapInterval(m_pMap->GetGridSnapInterval(), m_pMap->GetGridSnapInterval(), m_pMap->GetGridSnapInterval());
                    float3 snappedTotalAmountMoved(newTotalAmountMoved.Snap(snapInterval));
                    float3 snappedLastTotalAmountMoved(lastTotalAmountMoved.Snap(snapInterval));

                    // apply changes
                    entityMoveAmount = snappedTotalAmountMoved - snappedLastTotalAmountMoved;
                    m_currentStateDataVector = newTotalAmountMoved;
                }
                else
                {
                    m_currentStateDataVector += entityMoveAmount;
                }

                // Handle based on active widget
                switch (m_activeWidget)
                {
                case EDITOR_ENTITY_EDIT_MODE_WIDGET_MOVE:
                    {
                        // Move the entities
                        // if moving in any direction, don't use the local coordinate system
                        if (m_currentStateData[0] != 3)
                            entityMoveAmount = m_selectionAxisRotation * entityMoveAmount;

                        MoveSelection(entityMoveAmount);
                    }
                    break;

                case EDITOR_ENTITY_EDIT_MODE_WIDGET_ROTATE:
                    {
                        // Rotate the entities
                        RotateSelection(entityMoveAmount * 100.0f);
                    }
                    break;

                case EDITOR_ENTITY_EDIT_MODE_WIDGET_SCALE:
                    {
                        // Scale the entities
                        // special case for scaling, keep the values consistent for uniform scaling
                        if (m_currentStateData[0] == 3)
                        {
                            float uniformScaling = Max(entityMoveAmount.x, Max(entityMoveAmount.y, entityMoveAmount.z));
                            entityMoveAmount.Set(uniformScaling, uniformScaling, uniformScaling);
                        }
                        ScaleSelection(entityMoveAmount);
                    }
                    break;
                }
            }
            break;
        }

        // store values
        m_lastMousePositionEntityId = mousePositionEntityId;
    }

    return EditorEditMode::HandleViewportMouseInputEvent(pViewport, pMouseEvent);
}

bool EditorEntityEditMode::HandleViewportWheelInputEvent(EditorMapViewport *pViewport, const QWheelEvent *pWheelEvent)
{
    return EditorEditMode::HandleViewportWheelInputEvent(pViewport, pWheelEvent);
}

void EditorEntityEditMode::OnViewportDrawAfterWorld(EditorMapViewport *pViewport)
{
    MINIGUI_RECT guiRect;
    MiniGUIContext &guiContext = pViewport->GetGUIContext();

    // fall through
    EditorEditMode::OnViewportDrawAfterWorld(pViewport);

    // draw axis on selection
    if (m_selectedEntities.GetSize() > 0)
    {
        // draw bounding box around selection
        {
            guiContext.PushManualFlush();
            guiContext.SetAlphaBlendingEnabled(false);
            guiContext.SetDepthTestingEnabled(false);

            for (uint32 i = 0; i < m_selectedEntities.GetSize(); i++)
                guiContext.Draw3DWireBox(m_selectedEntities[i]->GetBoundingBox(), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));

            guiContext.PopManualFlush();
            guiContext.Flush();
        }

        // determine axis colors
        uint32 XAxisColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 0, 0, 255);
        uint32 YAxisColor = MAKE_COLOR_R8G8B8A8_UNORM(0, 255, 0, 255);
        uint32 ZAxisColor = MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 255, 255);
        uint32 anyAxisColor = MAKE_COLOR_R8G8B8A8_UNORM(45, 188, 236, 255);
        uint32 XPlaneFillColor = MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 0);
        uint32 YPlaneFillColor = MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 0);
        uint32 ZPlaneFillColor = MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 0);
        int32 onlyAxis = -1;

        // currently using widget?
        if (m_currentState == STATE_WIDGET)
        {
            // nuke the other axises
            switch (m_currentStateData[0])
            {
            case 0:
                XAxisColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 255);
                XPlaneFillColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 128);
                YAxisColor = 0;
                YPlaneFillColor = 0;
                ZAxisColor = 0;
                ZPlaneFillColor = 0;
                onlyAxis = 0;
                break;

            case 1:
                XAxisColor = 0;
                XPlaneFillColor = 0;
                YAxisColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 255);
                YPlaneFillColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 128);
                ZAxisColor = 0;
                ZPlaneFillColor = 0;
                onlyAxis = 1;
                break;

            case 2:
                XAxisColor = 0;
                XPlaneFillColor = 0;
                YAxisColor = 0;
                YPlaneFillColor = 0;
                ZAxisColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 255);
                ZPlaneFillColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 128);
                onlyAxis = 2;
                break;
            }
        }
        else
        {
            // if any are highlighted change the color
            switch (m_lastMousePositionEntityId)
            {
            case EDITOR_ENTITY_ID_WIDGET_AXIS_X:
                XAxisColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 255);
                XPlaneFillColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 128);
                break;

            case EDITOR_ENTITY_ID_WIDGET_AXIS_Y:
                YAxisColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 255);
                YPlaneFillColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 128);
                break;

            case EDITOR_ENTITY_ID_WIDGET_AXIS_Z:
                ZAxisColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 255);
                ZPlaneFillColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 128);
                break;
            }
        }

        // draw appropriate axis
        switch (m_activeWidget)
        {
        case EDITOR_ENTITY_EDIT_MODE_WIDGET_MOVE:
            //DrawSelectionMoveAxis(pViewport, XAxisColor, YAxisColor, ZAxisColor, XAxisColor, YAxisColor, ZAxisColor, XPlaneFillColor, YPlaneFillColor, ZPlaneFillColor);
            DrawMoveGismo(pViewport, XAxisColor, YAxisColor, ZAxisColor, anyAxisColor, XPlaneFillColor, YPlaneFillColor, ZPlaneFillColor, onlyAxis);
            break;

        case EDITOR_ENTITY_EDIT_MODE_WIDGET_ROTATE:
            DrawRotationGismo(pViewport, XAxisColor, YAxisColor, ZAxisColor, true, onlyAxis);
            break;

        case EDITOR_ENTITY_EDIT_MODE_WIDGET_SCALE:
            DrawScaleGismo(pViewport, XAxisColor, YAxisColor, ZAxisColor, anyAxisColor, onlyAxis);
            break;
        }
    }

    // draw the selection rectangle
    if (m_currentState == STATE_SELECTION_RECT)
    {
        // draw outer rect
        guiRect.left = Min(m_mouseSelectionRegion[0].x, m_mouseSelectionRegion[1].x);
        guiRect.right = Max(m_mouseSelectionRegion[0].x, m_mouseSelectionRegion[1].x);
        guiRect.top = Min(m_mouseSelectionRegion[0].y, m_mouseSelectionRegion[1].y);
        guiRect.bottom = Max(m_mouseSelectionRegion[0].y, m_mouseSelectionRegion[1].y);
        guiContext.DrawRect(&guiRect, MAKE_COLOR_R8G8B8A8_UNORM(51, 153, 255, 255));

#if 0
        // if >1 pixel, draw alpha blended inside rect
        if (guiRect.left != guiRect.right && guiRect.top != guiRect.bottom)
        {
            guiRect.left++;
            guiRect.right--;
            guiRect.top++;
            guiRect.bottom--;
            g_pGUISystem->GetRenderer()->DrawFilledRect(&guiRect, Vector4(1.0f, 1.0f, 1.0f, 0.5f), true);
        }
#endif

#if 0
        // draw the selection RT to a small section in the corner
        MINIGUI_UV_RECT uvRect = { 0.0f, 1.0f, 0.0f, 1.0f };
        guiRect.left = (int32)Y_floorf(0.05f * (float)m_RenderTargetDimensions.x);
        guiRect.right = (int32)Y_ceilf(0.4f * (float)m_RenderTargetDimensions.x);
        guiRect.top = (int32)Y_floorf(0.05f * (float)m_RenderTargetDimensions.y);
        guiRect.bottom = (int32)Y_ceilf(float(guiRect.right - guiRect.left) * ((float)m_RenderTargetDimensions.y / (float)m_RenderTargetDimensions.x));        
        m_GUIContext.DrawTexturedRect(&guiRect, &uvRect, m_pPickingRenderTarget);
#endif
    }
    else
    {
//         // draw entity tooltip?
//         if (m_eViewportState == EDITOR_VIEWPORT_STATE_NONE && m_iLastMousePositionEntityId != 0 && m_iLastMousePositionEntityId < EDITOR_ENTITY_ID_RESERVED_BEGIN)
//         {
//             // draw text near the entity, indicating its type and id
//             const Entity *pEntity = g_pEditor->GetWorld()->GetEntityById(m_iLastMousePositionEntityId);
//             if (pEntity != NULL)
//             {
//                 // draw tooltip
//                 tempStr.Format("Entity %u (Map ID %u): %s", pEntity->GetEntityId(), g_pEditor->LookupMapEntityId(pEntity->GetEntityId()), pEntity->GetTypeInfo()->GetName());
// 				m_GUIContext.DrawText(m_pOverlayFont, 16, m_LastMousePosition + 16, tempStr, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));
//                 pEntity->Release();
//             }
//         }
    }
}

void EditorEntityEditMode::OnViewportDrawBeforeWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnViewportDrawBeforeWorld(pViewport);
}

void EditorEntityEditMode::OnViewportDrawAfterPost(EditorMapViewport *pViewport)
{
    EditorEditMode::OnViewportDrawAfterPost(pViewport);
}

void EditorEntityEditMode::OnPickingTextureDrawAfterWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnPickingTextureDrawAfterWorld(pViewport);

    // drawing picking texture
    if (m_selectedEntities.GetSize() > 0)
    {
        // draw selection axis
        const uint32 xColor = EDITOR_ENTITY_ID_WIDGET_AXIS_X;
        const uint32 yColor = EDITOR_ENTITY_ID_WIDGET_AXIS_Y;
        const uint32 zColor = EDITOR_ENTITY_ID_WIDGET_AXIS_Z;
        const uint32 anyAxisColor = EDITOR_ENTITY_ID_WIDGET_AXIS_ANY;

        // draw appropriate axis
        switch (m_activeWidget)
        {
        case EDITOR_ENTITY_EDIT_MODE_WIDGET_MOVE:
            //DrawSelectionMoveAxis(pViewport, xColor, yColor, zColor, xColor, yColor, zColor, xColor, yColor, zColor);
            DrawMoveGismo(pViewport, xColor, yColor, zColor, anyAxisColor, xColor, yColor, zColor, -1);
            break;

        case EDITOR_ENTITY_EDIT_MODE_WIDGET_ROTATE:
            DrawRotationGismo(pViewport, xColor, yColor, zColor, false, -1);
            break;

        case EDITOR_ENTITY_EDIT_MODE_WIDGET_SCALE:
            DrawScaleGismo(pViewport, xColor, yColor, zColor, anyAxisColor, -1);
            break;
        }
    }
}

void EditorEntityEditMode::OnPickingTextureDrawBeforeWorld(EditorMapViewport *pViewport)
{
    EditorEditMode::OnPickingTextureDrawBeforeWorld(pViewport);
}

void EditorEntityEditMode::DrawMoveGismo(EditorMapViewport *pViewport, const uint32 XAxisColor, const uint32 YAxisColor, const uint32 ZAxisColor, const uint32 anyAxisColor, const uint32 XPlaneFillColor, const uint32 YPlaneFillColor, const uint32 ZPlaneFillColor, int32 onlyAxis /* = -1 */)
{
    float lineLength = 2.0f;
    float lineThickness = 2.0f;
    float halfLineLength = lineLength * 0.5f;
    float tipLength = 1.0f;
    float tipRadius = 0.1875f;
    float totalLength = lineLength + tipLength + tipRadius;
    uint32 tipVertexCount = 24;
    float anyAxisBoxExtents = 0.2f;

    // cache device pointers
    GPUContext *pGPUDevice = g_pRenderer->GetMainContext();

    // Build transforms.
    {
        const AABox &entitySelectionBounds = m_selectionBounds;
        float3 boundsDimensions = entitySelectionBounds.GetMaxBounds() - entitySelectionBounds.GetMinBounds();

        // Determine the scale to draw the axis at.
        float largestDimension = Max(boundsDimensions.x, Max(boundsDimensions.y, boundsDimensions.z));
        float desiredSize = largestDimension / 6.0f;
        float scale = Max(0.25f, desiredSize / totalLength);
        //float4x4 scaleMatrix(float4x4::MakeScaleMatrix(scale, scale, scale));

        // scale everything up
        lineLength *= scale;
        halfLineLength *= scale;
        tipLength *= scale;
        tipRadius *= scale;
        totalLength = lineLength + tipLength + tipRadius;
        anyAxisBoxExtents *= scale;

        // Get rotation matrix.
        //float4x4 rotationMatrix(m_selectionAxisRotation.GetFloat4x4());

        // Get translation matrix.
        //float4x4 translationMatrix(float4x4::MakeTranslationMatrix(m_selectionAxisPosition));

        // Generate and set world matrix.
        //float4x4 localToWorldMatrix(translationMatrix * scaleMatrix * rotationMatrix);
        //pGPUDevice->GetGPUConstants()->SetLocalToWorldMatrix(localToWorldMatrix, true);
        pGPUDevice->GetConstants()->SetLocalToWorldMatrix(float4x4::Identity, true);
    }

    // arrow base position
    float3 arrowBasePosition(m_selectionAxisPosition);

    // setup pdi
    MiniGUIContext &guiContext = pViewport->GetGUIContext();
    guiContext.SetAlphaBlendingEnabled(false);
    guiContext.SetDepthTestingEnabled(false);

    // Draw any axis box
    if (onlyAxis < 0)
        guiContext.Draw3DBox(float3(arrowBasePosition) - float3(anyAxisBoxExtents, anyAxisBoxExtents, anyAxisBoxExtents), float3(arrowBasePosition) + float3(anyAxisBoxExtents, anyAxisBoxExtents, anyAxisBoxExtents), anyAxisColor);

    // X axis arrow
    if (onlyAxis < 0 || onlyAxis == 0)
        guiContext.Draw3DArrow(arrowBasePosition, m_selectionAxisRotation * float3::UnitX, lineLength, lineThickness, tipLength, tipRadius, XAxisColor, tipVertexCount);

    // Y axis arrow
    if (onlyAxis < 0 || onlyAxis == 1)
        guiContext.Draw3DArrow(arrowBasePosition, m_selectionAxisRotation * float3::UnitY, lineLength, lineThickness, tipLength, tipRadius, YAxisColor, tipVertexCount);

    // Z axis arrow
    if (onlyAxis < 0 || onlyAxis == 2)
        guiContext.Draw3DArrow(arrowBasePosition, m_selectionAxisRotation * float3::UnitZ, lineLength, lineThickness, tipLength, tipRadius, ZAxisColor, tipVertexCount);

    // Draw plane borders
    guiContext.BeginImmediate3D(MiniGUIContext::IMMEDIATE_PRIMITIVE_LINES);

    // X axis plane border
    if (onlyAxis < 0 || onlyAxis == 0)
    {
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, halfLineLength), XAxisColor);               // top-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, 0.0f), XAxisColor);                         // bottom-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, 0.0f), XAxisColor);                         // bottom-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(halfLineLength, 0.0f, 0.0f), XAxisColor);               // bottom-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(halfLineLength, 0.0f, 0.0f), XAxisColor);               // bottom-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(halfLineLength, 0.0f, halfLineLength), XAxisColor);     // top-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(halfLineLength, 0.0f, halfLineLength), XAxisColor);     // top-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, halfLineLength), XAxisColor);               // top-left
    }

    // Y axis plane
    if (onlyAxis < 0 || onlyAxis == 1)
    {
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, halfLineLength), YAxisColor);               // top-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, 0.0f), YAxisColor);                         // bottom-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, 0.0f), YAxisColor);                         // bottom-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, halfLineLength, 0.0f), YAxisColor);               // bottom-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, halfLineLength, 0.0f), YAxisColor);               // bottom-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, halfLineLength, halfLineLength), YAxisColor);     // top-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, halfLineLength, halfLineLength), YAxisColor);     // top-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, halfLineLength), YAxisColor);               // top-left
    }

    // Z axis plane
    if (onlyAxis < 0 || onlyAxis == 2)
    {
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, halfLineLength, 0.0f), ZAxisColor);               // top-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, 0.0f), ZAxisColor);                         // bottom-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, 0.0f), ZAxisColor);                         // bottom-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(halfLineLength, 0.0f, 0.0f), ZAxisColor);               // bottom-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(halfLineLength, 0.0f, 0.0f), ZAxisColor);               // bottom-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(halfLineLength, halfLineLength, 0.0f), ZAxisColor);     // top-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(halfLineLength, halfLineLength, 0.0f), ZAxisColor);     // top-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, halfLineLength, 0.0f), ZAxisColor);               // top-left
    }

    // End plane borders
    guiContext.EndImmediate();

    // Draw plane
    guiContext.SetAlphaBlendingEnabled(true);
    guiContext.SetAlphaBlendingMode(MiniGUIContext::ALPHABLENDING_MODE_STRAIGHT);
    guiContext.BeginImmediate3D(MiniGUIContext::IMMEDIATE_PRIMITIVE_QUADS);

    // X axis plane
    if (XPlaneFillColor != 0 && (onlyAxis < 0 || onlyAxis == 0))
    {
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, halfLineLength), XPlaneFillColor);               // top-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, 0.0f), XPlaneFillColor);                         // bottom-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(halfLineLength, 0.0f, 0.0f), XPlaneFillColor);               // bottom-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(halfLineLength, 0.0f, halfLineLength), XPlaneFillColor);     // top-right
    }

    // Y axis plane
    if (YPlaneFillColor != 0 && (onlyAxis < 0 || onlyAxis == 1))
    {
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, halfLineLength), YPlaneFillColor);               // top-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, 0.0f), YPlaneFillColor);                         // bottom-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, halfLineLength, 0.0f), YPlaneFillColor);               // bottom-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, halfLineLength, halfLineLength), YPlaneFillColor);     // top-right
    }

    // Z axis plane
    if (ZPlaneFillColor != 0 && (onlyAxis < 0 || onlyAxis == 2))
    {
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, halfLineLength, 0.0f), ZPlaneFillColor);               // top-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(0.0f, 0.0f, 0.0f), ZPlaneFillColor);                         // bottom-left
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(halfLineLength, 0.0f, 0.0f), ZPlaneFillColor);               // bottom-right
        guiContext.ImmediateVertex(arrowBasePosition + m_selectionAxisRotation * float3(halfLineLength, halfLineLength, 0.0f), ZPlaneFillColor);     // top-right
    }

    // end planes
    guiContext.EndImmediate();
}

void EditorEntityEditMode::DrawRotationGismo(EditorMapViewport *pViewport, const uint32 XAxisColor, const uint32 YAxisColor, const uint32 ZAxisColor, bool transparent /* = true */, int32 onlyAxis /* = -1 */)
{
    // roughly the size of the rotator gismo
    float gismoRadius = 0.5f;

    // cache device pointers
    GPUContext *pGPUDevice = g_pRenderer->GetMainContext();

    // Setup renderer.
    pGPUDevice->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_NONE));
    //pGPUDevice->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerStateWireframeNoCull());
    pGPUDevice->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false), 0);
    pGPUDevice->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAlphaBlending());

    // Build transforms.
    {
        const AABox &entitySelectionBounds = m_selectionBounds;
        float3 boundsDimensions = entitySelectionBounds.GetMaxBounds() - entitySelectionBounds.GetMinBounds();

        // Determine the scale to draw the axis at.
        float largestDimension = Max(boundsDimensions.x, Max(boundsDimensions.y, boundsDimensions.z));
        float desiredSize = largestDimension / 6.0f;
        float scale = Max(0.25f, desiredSize / gismoRadius);
        float4x4 scaleMatrix(float4x4::MakeScaleMatrix(scale, scale, scale));

        // if drawing a single axis, scale it by 2
        if (onlyAxis >= 0)
            scale *= 0.5f;

        // Get rotation matrix.
        float4x4 rotationMatrix(m_selectionAxisRotation.GetMatrix4x4());

        // Get translation matrix.
        float4x4 translationMatrix(float4x4::MakeTranslationMatrix(m_selectionAxisPosition));

        // Generate and set world matrix.
        float4x4 localToWorldMatrix(translationMatrix * scaleMatrix * rotationMatrix);
        pGPUDevice->GetConstants()->SetLocalToWorldMatrix(localToWorldMatrix, true);
    }
    
    PlainVertexFactory::Vertex gismoVertices[3 * 2 * 3];
    uint32 nVertices = 0;

    // find the selection center
    float3 selectionCenter(m_selectionBounds.GetCenter());

    // draw single axis?
    if (onlyAxis >= 0)
    {
        switch (onlyAxis)
        {
            // x axis
        case 0:
            gismoVertices[nVertices++].SetUVColor(0.0f, -1.0f, -1.0f, 0.0f, 1.0f, XAxisColor);     // bottom-left
            gismoVertices[nVertices++].SetUVColor(0.0f, -1.0f, 1.0f, 0.0f, 0.0f, XAxisColor);      // top-left
            gismoVertices[nVertices++].SetUVColor(0.0f, 1.0f, -1.0f, 1.0f, 1.0f, XAxisColor);     // bottom-right
            gismoVertices[nVertices++].SetUVColor(0.0f, 1.0f, -1.0f, 1.0f, 1.0f, XAxisColor);     // bottom-right
            gismoVertices[nVertices++].SetUVColor(0.0f, -1.0f, 1.0f, 0.0f, 0.0f, XAxisColor);      // top-left
            gismoVertices[nVertices++].SetUVColor(0.0f, 1.0f, 1.0f, 1.0f, 0.0f, XAxisColor);      // top-right
            break;

            // y axis
        case 1:
            gismoVertices[nVertices++].SetUVColor(-1.0f, 0.0f, -1.0f, 0.0f, 1.0f, YAxisColor);     // bottom-left
            gismoVertices[nVertices++].SetUVColor(-1.0f, 0.0f, 1.0f, 0.0f, 0.0f, YAxisColor);      // top-left
            gismoVertices[nVertices++].SetUVColor(1.0f, 0.0f, -1.0f, 1.0f, 1.0f, YAxisColor);      // bottom-right
            gismoVertices[nVertices++].SetUVColor(1.0f, 0.0f, -1.0f, 1.0f, 1.0f, YAxisColor);      // bottom-right
            gismoVertices[nVertices++].SetUVColor(-1.0f, 0.0f, 1.0f, 0.0f, 0.0f, YAxisColor);      // top-left
            gismoVertices[nVertices++].SetUVColor(1.0f, 0.0f, 1.0f, 1.0f, 0.0f, YAxisColor);       // top-right
            break;

            // z axis
        case 2:
            gismoVertices[nVertices++].SetUVColor(-1.0f, -1.0f, 0.0f, 0.0f, 1.0f, ZAxisColor);     // bottom-left
            gismoVertices[nVertices++].SetUVColor(-1.0f, 1.0f, 0.0f, 0.0f, 0.0f, ZAxisColor);     // top-left
            gismoVertices[nVertices++].SetUVColor(1.0f, -1.0f, 0.0f, 1.0f, 1.0f, ZAxisColor);      // bottom-right
            gismoVertices[nVertices++].SetUVColor(1.0f, -1.0f, 0.0f, 1.0f, 1.0f, ZAxisColor);      // bottom-right
            gismoVertices[nVertices++].SetUVColor(-1.0f, 1.0f, 0.0f, 0.0f, 0.0f, ZAxisColor);     // top-left
            gismoVertices[nVertices++].SetUVColor(1.0f, 1.0f, 0.0f, 1.0f, 0.0f, ZAxisColor);      // top-right
            break;
        }
    }
    // draw all axises
    else
    {
        // x axis - points backwards down y axis
        gismoVertices[nVertices++].SetUVColor(0.0f, -1.0f, 0.0f, 0.0f, 0.5f, XAxisColor);   // bottom-left
        gismoVertices[nVertices++].SetUVColor(0.0f, -1.0f, 1.0f, 0.0f, 0.0f, XAxisColor);   // top-left
        gismoVertices[nVertices++].SetUVColor(0.0f, 0.0f, 0.0f, 0.5f, 0.5f, XAxisColor);    // bottom-right
        gismoVertices[nVertices++].SetUVColor(0.0f, 0.0f, 0.0f, 0.5f, 0.5f, XAxisColor);    // bottom-right
        gismoVertices[nVertices++].SetUVColor(0.0f, -1.0f, 1.0f, 0.0f, 0.0f, XAxisColor);   // top-left
        gismoVertices[nVertices++].SetUVColor(0.0f, 0.0f, 1.0f, 0.0f, 0.5f, XAxisColor);    // top-right

        // y axis - points forwards down x axis
        gismoVertices[nVertices++].SetUVColor(0.0f, 0.0f, 0.0f, 0.5f, 0.5f, YAxisColor);    // bottom-left
        gismoVertices[nVertices++].SetUVColor(0.0f, 0.0f, 1.0f, 0.5f, 0.0f, YAxisColor);    // top-left
        gismoVertices[nVertices++].SetUVColor(1.0f, 0.0f, 0.0f, 1.0f, 0.5f, YAxisColor);    // bottom-right
        gismoVertices[nVertices++].SetUVColor(1.0f, 0.0f, 0.0f, 1.0f, 0.5f, YAxisColor);    // bottom-right
        gismoVertices[nVertices++].SetUVColor(0.0f, 0.0f, 1.0f, 0.5f, 0.0f, YAxisColor);    // top-left
        gismoVertices[nVertices++].SetUVColor(1.0f, 0.0f, 1.0f, 1.0f, 0.0f, YAxisColor);    // top-right

        // z axis - points forwards down x axis
        gismoVertices[nVertices++].SetUVColor(0.0f, -1.0f, 0.0f, 0.5f, 1.0f, ZAxisColor);   // bottom-left
        gismoVertices[nVertices++].SetUVColor(0.0f, 0.0f, 0.0f, 0.5f, 0.5f, ZAxisColor);    // top-left
        gismoVertices[nVertices++].SetUVColor(1.0f, -1.0f, 0.0f, 1.0f, 1.0f, ZAxisColor);   // bottom-right
        gismoVertices[nVertices++].SetUVColor(1.0f, -1.0f, 0.0f, 1.0f, 1.0f, ZAxisColor);   // bottom-right
        gismoVertices[nVertices++].SetUVColor(0.0f, 0.0f, 0.0f, 0.5f, 0.5f, ZAxisColor);    // top-left
        gismoVertices[nVertices++].SetUVColor(1.0f, 0.0f, 0.0f, 1.0f, 0.5f, ZAxisColor);    // top-right
    }

    // draw the gismo
    pGPUDevice->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);
    //g_pRenderer->DrawPlainBlended(pGPUDevice, gismoVertices, nVertices, (transparent) ? m_pRotationGismoTransparentTexture->GetGPUTexture() : m_pRotationGismoTexture->GetGPUTexture());
    //g_pRenderer->DrawPlainColored(pGPUDevice, gismoVertices, nVertices);
}

void EditorEntityEditMode::DrawScaleGismo(EditorMapViewport *pViewport, const uint32 XAxisColor, const uint32 YAxisColor, const uint32 ZAxisColor, const uint32 anyAxisColor, int32 onlyAxis /* = -1 */)
{
    static const float lineLength = 2.0f;
    static const float boxSize = 0.5f;
    static const float boxExtents = boxSize * 0.5f;
    static const float totalLength = lineLength + boxExtents;

    // cache device pointers
    GPUContext *pGPUDevice = g_pRenderer->GetMainContext();

    // Setup renderer.
    pGPUDevice->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_NONE));
    pGPUDevice->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false), 0);
    pGPUDevice->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAlphaBlending());

    // Build transforms.
    {
        const AABox &entitySelectionBounds = m_selectionBounds;
        float3 boundsDimensions = entitySelectionBounds.GetMaxBounds() - entitySelectionBounds.GetMinBounds();

        // Determine the scale to draw the axis at.
        float largestDimension = Max(boundsDimensions.x, Max(boundsDimensions.y, boundsDimensions.z));
        float desiredSize = largestDimension / 6.0f;
        float scale = Max(0.25f, desiredSize / totalLength);
        float4x4 scaleMatrix(float4x4::MakeScaleMatrix(scale, scale, scale));

        // Get rotation matrix.
        float4x4 rotationMatrix(m_selectionAxisRotation.GetMatrix4x4());

        // Get translation matrix.
        float4x4 translationMatrix(float4x4::MakeTranslationMatrix(m_selectionAxisPosition));

        // Generate and set world matrix.
        float4x4 localToWorldMatrix(translationMatrix * scaleMatrix * rotationMatrix);
        pGPUDevice->GetConstants()->SetLocalToWorldMatrix(localToWorldMatrix, true);
    }

    // Determine the position at which to draw the axis.
    float3 axisBasePosition = float3::Zero;

    // setup pdi
    MiniGUIContext &guiContext = pViewport->GetGUIContext();
    guiContext.SetAlphaBlendingEnabled(false);
    guiContext.SetDepthTestingEnabled(false);

    // X axis line
    if (onlyAxis < 0 || onlyAxis == 0)
    {
        guiContext.Draw3DLine(float3::Zero, float2::Zero, XAxisColor);
        guiContext.Draw3DLine(float3(lineLength, 0.0f, 0.0f), float2::Zero, XAxisColor);
    }

    // Y axis line
    if (onlyAxis < 0 || onlyAxis == 1)
    {
        guiContext.Draw3DLine(float3::Zero, float2::Zero, YAxisColor);
        guiContext.Draw3DLine(float3(0.0f, lineLength, 0.0f), float2::Zero, YAxisColor);
    }

    // Z axis line
    if (onlyAxis < 0 || onlyAxis == 2)
    {
        guiContext.Draw3DLine(float3::Zero, float2::Zero, ZAxisColor);
        guiContext.Draw3DLine(float3(0.0f, 0.0f, lineLength), float2::Zero, ZAxisColor);
    }

    // Draw "all axises" box
    if (onlyAxis < 0)
        guiContext.Draw3DBox(float3(-boxExtents, -boxExtents, -boxExtents), float3(boxExtents, boxExtents, boxExtents), anyAxisColor);

    // Draw x axis box
    if (onlyAxis < 0 || onlyAxis == 0)
        guiContext.Draw3DBox(float3(lineLength + -boxExtents, -boxExtents, -boxExtents), float3(lineLength + boxExtents, boxExtents, boxExtents), XAxisColor);

    // Draw y axis box
    if (onlyAxis < 0 || onlyAxis == 1)
        guiContext.Draw3DBox(float3(-boxExtents, lineLength + -boxExtents, -boxExtents), float3(boxExtents, lineLength + boxExtents, boxExtents), YAxisColor);

    // Draw y axis box
    if (onlyAxis < 0 || onlyAxis == 2)
        guiContext.Draw3DBox(float3(-boxExtents, -boxExtents, lineLength + -boxExtents), float3(boxExtents, boxExtents, lineLength + boxExtents), ZAxisColor);
}

void EditorEntityEditMode::UpdateSelection(EditorMapViewport *pViewport)
{
    // determine min/max coordinates
    Vector2i minCoordinates = m_mouseSelectionRegion[0].Min(m_mouseSelectionRegion[1]);
    Vector2i maxCoordinates = m_mouseSelectionRegion[0].Max(m_mouseSelectionRegion[1]);
    Log_DevPrintf("New selection: [%d-%d] [%d-%d]", m_mouseSelectionRegion[0].x, m_mouseSelectionRegion[1].x, m_mouseSelectionRegion[0].y, m_mouseSelectionRegion[1].y);
    
    // allocate memory for receiving entity ids
    Vector2i coordinateRange = maxCoordinates - minCoordinates + 1;
    uint32 *pTexelValues = new uint32[coordinateRange.x * coordinateRange.y];
    uint32 *pCurrentTexelValue = pTexelValues;

    // read texel values
    if (pViewport->GetPickingTextureValues(minCoordinates, maxCoordinates, pTexelValues))
    {
        // clear current selection
        ClearSelection();
    
        // iterate each pixel that is selected, determining which render target was hit on the screen
        Vector2i currentPosition = minCoordinates;
        for (; currentPosition.y <= maxCoordinates.y; currentPosition.y++)
        {
            for (currentPosition.x = minCoordinates.x; currentPosition.x <= maxCoordinates.x; currentPosition.x++)
            {
                uint32 texelValue = *pCurrentTexelValue++;
                if (texelValue == 0 || texelValue >= EDITOR_ENTITY_ID_RESERVED_BEGIN)
                    continue;

                // look up the visual that this texel has
                EditorMapEntity *pEntity = m_pMap->GetEntityByArrayIndex(texelValue - 1);
                if (pEntity == nullptr)
                    continue;

                //Log_DevPrintf("   hit queue id %u : %s", texelValue, (pEntity != NULL) ? pEntity->GetTypeInfo()->GetName() : "NULL");

                // is it a visual entity?
                AddSelection(pEntity);
            }
        }
    }
}

void EditorEntityEditMode::UpdateSelectionBounds()
{
    m_selectionBounds = AABox::Zero;

    for (uint32 i = 0; i < m_selectedEntities.GetSize(); i++)
    {
        if (i == 0)
            m_selectionBounds = m_selectedEntities[i]->GetBoundingBox();
        else
            m_selectionBounds.Merge(m_selectedEntities[i]->GetBoundingBox());
    }
}

void EditorEntityEditMode::UpdateSelectionAxis()
{
    // if we have one selection, we set the axis position to be the position of the entity.
    // if we have more than one, then we use the centre of the merged bounds of the selection.
    if (m_selectedEntities.GetSize() == 1)
    {
        m_selectionAxisPosition = m_selectedEntities[0]->GetPosition();
        if (m_pMap->GetMapWindow()->GetReferenceCoordinateSystem() == EDITOR_REFERENCE_COORDINATE_SYSTEM_LOCAL)
            m_selectionAxisRotation = m_selectedEntities[0]->GetRotation();
        else
            m_selectionAxisRotation.SetIdentity();
    }
    else if (m_selectedEntities.GetSize() > 1)
    {
        m_selectionAxisPosition = m_selectionBounds.GetCenter();
        m_selectionAxisRotation.SetIdentity();
    }
    else
    {
        m_selectionAxisPosition.SetZero();
        m_selectionAxisRotation.SetIdentity();
    }

    // force redraw
    m_pMap->GetMapWindow()->RedrawAllViewports();
}

void EditorEntityEditMode::OnEntityPropertyChanged(const EditorMapEntity *pEntity, const char *propertyName, const char *propertyValue)
{
    EditorMapWindow *pMapWindow = m_pMap->GetMapWindow();
    EditorPropertyEditorWidget *propertyEditor = pMapWindow->GetUI()->propertyEditor;

    // check for entity edit mode
    if (pMapWindow->GetEditMode() == EDITOR_EDIT_MODE_ENTITY && IsSelected(pEntity))
    {
        // we're selected, so update the property manager if it's tracked
        // first check if the value matches the other entities
        const char *propertyEditorValue = propertyValue;
        for (uint32 i = 0; i < m_selectedEntities.GetSize(); i++)
        {
            if (m_selectedEntities[i] == pEntity)
                continue;

            const String *otherValue = m_selectedEntities[i]->GetEntityData()->GetPropertyValuePointer(propertyName);
            if (otherValue == nullptr || otherValue->Compare(propertyValue) != 0)
            {
                propertyEditorValue = "<different values>";
                break;
            }
        }

        // and update the property editor
        propertyEditor->UpdatePropertyValue(propertyName, propertyEditorValue);

        // this may be a position-related property, so update the selection bounds/axis
        UpdateSelectionBounds();
        UpdateSelectionAxis();
    }
}

void EditorEntityEditMode::SetPropertyOnSelection(const char *propertyName, const char *propertyValue)
{
    EditorMapWindow *pMapWindow = m_pMap->GetMapWindow();
    EditorPropertyEditorWidget *propertyEditor = pMapWindow->GetUI()->propertyEditor;
    if (m_selectedEntities.GetSize() == 0)
        return;

    // handle case of components
    if (m_selectedEntities.GetSize() == 1 && m_selectedComponentIndex >= 0)
    {
        m_selectedEntities[0]->SetComponentPropertyValue(m_selectedComponentIndex, propertyName, propertyValue);
    }
    else
    {
        // this makes it easier, applying it to all.
        for (uint32 i = 0; i < m_selectedEntities.GetSize(); i++)
            m_selectedEntities[i]->SetEntityPropertyValue(propertyName, propertyValue);
    }

    // in edit mode? update property editor
    if (pMapWindow->GetEditMode() == EDITOR_EDIT_MODE_ENTITY)
        propertyEditor->UpdatePropertyValue(propertyName, propertyValue);

    // fix up selection bounds and redraw
    UpdateSelectionBounds();
    UpdateSelectionAxis();
    pMapWindow->RedrawAllViewports();
}

EditorMapEntity *EditorEntityEditMode::CreateEntityUsingViewport(EditorMapViewport *pViewport, const char *typeName)
{
    // create the entity
    EditorMapEntity *pEntity = m_pMap->CreateEntity(typeName);
    if (pEntity == nullptr)
        return nullptr;

    // determine position
    // fixme: use proper position
    float3 entityPosition = pViewport->GetViewController().GetCameraPosition() + (pViewport->GetViewController().GetCameraForwardDirection() * 3.0f);

    // set position property
    SmallString positionString;
    StringConverter::Float3ToString(positionString, entityPosition);
    pEntity->SetEntityPropertyValue("Position", positionString);

    // clear & set the selection to this new entity
    ClearSelection();
    AddSelection(pEntity);

    // set focus to that viewport
    pViewport->SetFocus();

    // done
    m_pMap->RedrawAllViewports();
    return pEntity;
}

void EditorEntityEditMode::MoveViewportCameraToEntity(EditorMapViewport *pViewport, const EditorMapEntity *pEntity, const Quaternion cameraRotation /* = Quaternion::FromEulerAngles(Math::DegreesToRadians(-45.0f) , 0.0f, 0.0f)*/)
{
    DebugAssert(pEntity != nullptr);

    // find the maximum dimension from the bounding box
    float3 boxExtents(pEntity->GetBoundingBox().GetExtents());
    float viewDistance = Max(1.0f, Max(boxExtents.x, Max(boxExtents.y, boxExtents.z))) * 2.0f;

    // calculate eye position
    float3 moveVector(cameraRotation * float3::UnitY);
    float3 eyePosition(pEntity->GetBoundingBox().GetCenter() - moveVector * viewDistance);
    
    // set camera position
    pViewport->GetViewController().SetCameraPosition(eyePosition);

    // if perspective, set the rotation
    if (pViewport->GetViewController().GetCameraMode() == EDITOR_CAMERA_MODE_PERSPECTIVE)
        pViewport->GetViewController().SetCameraRotation(cameraRotation);
}

void EditorEntityEditMode::UpdatePropertyEditor()
{
    EditorPropertyEditorWidget *propertyEditor = m_pMap->GetMapWindow()->GetUI()->propertyEditor;

    // clear the property editor
    propertyEditor->ClearProperties();

    // got any selection?
    if (m_selectedEntities.GetSize() == 0)
    {
        m_ui->selectionTitleText->setText(QStringLiteral("No selection"));
        m_ui->selectionComponents->clear();
        m_ui->selectionComponents->setEnabled(false);
        m_ui->actionAddComponent->setEnabled(false);
        m_ui->actionRemoveComponent->setEnabled(false);
        m_ui->actionSelectionLayers->setEnabled(false);
        return;
    }


    // single selection?
    if (m_selectedEntities.GetSize() == 1)
    {
        // just add all properties from this
        const EditorMapEntity *pEntity = m_selectedEntities[0];
        const MapSourceEntityData *pEntityData = pEntity->GetEntityData();
        const ObjectTemplate *pTemplate = pEntity->GetTemplate();

        // and set all the values
        for (uint32 i = 0; i < pTemplate->GetPropertyDefinitionCount(); i++)
        {
            const PropertyTemplateProperty *pProperty = pTemplate->GetPropertyDefinition(i);
            const String *propertyValue = pEntity->GetEntityData()->GetPropertyValuePointer(pProperty->GetName());
            if (propertyValue == nullptr)
                propertyValue = &pProperty->GetDefaultValue();

            propertyEditor->AddProperty(pProperty, *propertyValue);
        }

        // build title text
        LargeString titleText;
        titleText.AppendFormattedString("<strong>%s</strong>&nbsp;(%s)<br>", pEntity->GetEntityData()->GetEntityName().GetCharArray(), pEntity->GetTemplate()->GetTypeName().GetCharArray());
        titleText.AppendString(pEntity->GetTemplate()->GetDescription());

        // update ui
        m_ui->selectionTitleText->setText(ConvertStringToQString(titleText));
        m_ui->selectionComponents->setEnabled(true);
        m_ui->actionAddComponent->setEnabled(true);
        m_ui->actionRemoveComponent->setEnabled(true);
        m_ui->actionSelectionLayers->setEnabled(true);

        // update component list
        m_selectedComponentIndex = -1;
        m_ui->selectionComponents->clear();
        for (uint32 componentIndex = 0; componentIndex < pEntityData->GetComponentCount(); componentIndex++)
        {
            const MapSourceEntityComponent *pComponent = pEntityData->GetComponentByIndex(componentIndex);
            SmallString componentListItemName;
            componentListItemName.Format("%s (%s)", pComponent->GetComponentName().GetCharArray(), pComponent->GetTypeName().GetCharArray());
            /*m_ui->selectionComponents->addItem(*/new QListWidgetItem(ConvertStringToQString(componentListItemName), m_ui->selectionComponents);
        }
    }
    else
    {
        // add all properties from the first entity to a temporary list
        PODArray<const PropertyTemplateProperty *> propertyList;
        {
            const EditorMapEntity *pEntity = m_selectedEntities[0];
            const ObjectTemplate *pTemplate = pEntity->GetTemplate();
            for (uint32 i = 0; i < pTemplate->GetPropertyDefinitionCount(); i++)
                propertyList.Add(pTemplate->GetPropertyDefinition(i));
        }

        // now remove any properties not present across the entire selection
        for (uint32 propertyIndex = 0; propertyIndex < propertyList.GetSize(); )
        {
            const PropertyTemplateProperty *pProperty = propertyList[propertyIndex];

            uint32 selectionIndex;
            for (selectionIndex = 0; selectionIndex < m_selectedEntities.GetSize(); selectionIndex++)
            {
                if (m_selectedEntities[selectionIndex]->GetTemplate()->GetPropertyDefinitionByName(pProperty->GetName()) == nullptr)
                    break;
            }

            if (selectionIndex != m_selectedEntities.GetSize())
            {
                // this property wasn't found, remove it
                propertyList.OrderedRemove(propertyIndex);
                continue;
            }
            else
            {
                // go to next
                propertyIndex++;
                continue;
            }
        }

        // add found properties to the editor, and update their values
        for (uint32 i = 0; i < propertyList.GetSize(); i++)
        {
            const PropertyTemplateProperty *pProperty = propertyList[i];

            // get the value for the first selected entity
            String propertyValue = m_selectedEntities[0]->GetEntityData()->GetPropertyTable()->GetPropertyValueDefaultString(pProperty->GetName(), pProperty->GetDefaultValue());
            
            // confirm that it matches the others
            for (uint32 j = 1; j < m_selectedEntities.GetSize(); j++)
            {
                String otherPropertyValue = m_selectedEntities[j]->GetEntityData()->GetPropertyTable()->GetPropertyValueDefaultString(pProperty->GetName(), pProperty->GetDefaultValue());

                // check match
                if (propertyValue.Compare(otherPropertyValue) != 0)
                {
                    // mismatch
                    propertyValue = "<different values>";
                    break;
                }
            }

            // add it
            propertyEditor->AddProperty(pProperty, propertyValue);
        }

        // build title text
        m_ui->selectionTitleText->setText(ConvertStringToQString(SmallString::FromFormat("<strong>Multiple selection</strong>&nbsp;(%u objects)<br>", m_selectedEntities.GetSize())));
        m_ui->selectionComponents->clear();
        m_ui->selectionComponents->setEnabled(false);
        m_ui->actionAddComponent->setEnabled(false);
        m_ui->actionRemoveComponent->setEnabled(false);
        m_ui->actionSelectionLayers->setEnabled(false);
    }
}

void EditorEntityEditMode::UpdatePropertyEditorForComponent()
{
    if (m_selectedEntities.GetSize() != 1 || m_selectedComponentIndex < 0)
    {
        UpdatePropertyEditor();
        return;
    }

    EditorMapEntity *pEntity = m_selectedEntities[0];
    if ((uint32)m_selectedComponentIndex >= pEntity->GetComponentCount())
    {
        UpdatePropertyEditor();
        return;
    }

    // get stuff
    const ObjectTemplate *pTemplate = pEntity->GetComponentTemplate(m_selectedComponentIndex);
    const MapSourceEntityComponent *pData = pEntity->GetComponentData(m_selectedComponentIndex);
    EditorPropertyEditorWidget *pPropertyEditor = m_pMap->GetMapWindow()->GetUI()->propertyEditor;

    // build title text
    LargeString titleText;
    titleText.AppendFormattedString("<strong>%s</strong>&nbsp;(%s)<br>", pData->GetComponentName().GetCharArray(), pTemplate->GetTypeName().GetCharArray());
    titleText.AppendString(pTemplate->GetDescription());
    m_ui->selectionTitleText->setText(ConvertStringToQString(titleText));

    // add properties
    pPropertyEditor->ClearProperties();
    for (uint32 i = 0; i < pTemplate->GetPropertyDefinitionCount(); i++)
    {
        const PropertyTemplateProperty *pProperty = pTemplate->GetPropertyDefinition(i);
        const String *propertyValue = pData->GetPropertyValuePointer(pProperty->GetName());
        if (propertyValue == nullptr)
            propertyValue = &pProperty->GetDefaultValue();

        pPropertyEditor->AddProperty(pProperty, *propertyValue);
    }
}

bool EditorEntityEditMode::HandleResourceViewResourceActivatedEvent(const ResourceTypeInfo *pResourceTypeInfo, const char *resourceName)
{
    // if we are dropping a static mesh, create an entity
    if (pResourceTypeInfo == OBJECT_TYPEINFO(StaticMesh))
    {
        EditorMapEntity *pEntity = CreateEntityUsingViewport(m_pMap->GetMapWindow()->GetActiveViewport(), "StaticMeshBrush");
        if (pEntity != nullptr)
            pEntity->SetEntityPropertyValue("StaticMeshName", resourceName);
    }

    return EditorEditMode::HandleResourceViewResourceActivatedEvent(pResourceTypeInfo, resourceName);
}

bool EditorEntityEditMode::HandleResourceViewResourceDroppedEvent(const ResourceTypeInfo *pResourceTypeInfo, const char *resourceName, const EditorMapViewport *pViewport, int32 x, int32 y)
{
    return EditorEditMode::HandleResourceViewResourceDroppedEvent(pResourceTypeInfo, resourceName, pViewport, x, y);
}
