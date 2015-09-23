#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorHelpers.h"
#include "Editor/MapEditor/EditorMapWindow.h"
#include "Editor/MapEditor/EditorMap.h"
#include "Editor/MapEditor/EditorMapViewport.h"
#include "Editor/MapEditor/EditorEditMode.h"
#include "Editor/MapEditor/EditorMapEditMode.h"
#include "Editor/MapEditor/EditorEntityEditMode.h"
#include "Editor/MapEditor/EditorGeometryEditMode.h"
#include "Editor/MapEditor/EditorTerrainEditMode.h"
#include "Editor/MapEditor/EditorCreateTerrainDialog.h"
#include "Editor/MapEditor/EditorWorldOutlinerWidget.h"
#include "Editor/EditorProgressDialog.h"
#include "Editor/EditorCVars.h"
#include "Editor/EditorPropertyEditorDialog.h"
#include "Editor/EditorPropertyEditorWidget.h"
#include "Editor/EditorHelpers.h"
#include "Engine/Entity.h"
#include "MapCompiler/MapSource.h"
#include "MapCompiler/MapSourceTerrainData.h"
#include "Editor/MapEditor/ui_EditorMapWindow.h"
Log_SetChannel(EditorMapWindow);

EditorMapWindow::EditorMapWindow()
    : QMainWindow(),
      m_ui(new Ui_EditorMapWindow()),
      m_pOpenMap(NULL),
      m_referenceCoordinateSystem(EDITOR_REFERENCE_COORDINATE_SYSTEM_WORLD),
      m_editMode(EDITOR_EDIT_MODE_NONE),
      m_pActiveEditModePointer(NULL),
      m_viewportLayout(EDITOR_VIEWPORT_LAYOUT_COUNT),
      m_pActiveViewport(NULL)
{
    m_ui->CreateUI(this);
    ConnectUIEvents();

    g_pEditor->AddMapWindow(this);

    // add log callback
    Log::GetInstance().RegisterCallback(LogCallback, reinterpret_cast<void *>(this));
}

EditorMapWindow::~EditorMapWindow()
{
    // remove log callback
    Log::GetInstance().UnregisterCallback(LogCallback, reinterpret_cast<void *>(this));

    g_pEditor->RemoveMapWindow(this);

    DebugAssert(m_pOpenMap == NULL);

    delete m_ui;
}

const EditorWorldOutlinerWidget *EditorMapWindow::GetWorldOutlinerWidget() const
{
    return m_ui->worldOutliner;
}

const EditorResourceBrowserWidget *EditorMapWindow::GetResourceBrowserWidget() const
{
    return m_ui->resourceBrowser;
}

const EditorPropertyEditorWidget *EditorMapWindow::GetPropertyEditorWidget() const
{
    return m_ui->propertyEditor;
}

EditorWorldOutlinerWidget *EditorMapWindow::GetWorldOutlinerWidget()
{
    return m_ui->worldOutliner;
}

EditorResourceBrowserWidget * EditorMapWindow::GetResourceBrowserWidget()
{
    return m_ui->resourceBrowser;
}

EditorPropertyEditorWidget *EditorMapWindow::GetPropertyEditorWidget()
{
    return m_ui->propertyEditor;
}

bool EditorMapWindow::NewMap()
{
    DebugAssert(m_pOpenMap == NULL);
    g_pEditor->BlockFrameExecution();

    m_pOpenMap = new EditorMap(this);
    if (!m_pOpenMap->CreateMap() || !OnMapOpened())
    {
        delete m_pOpenMap;
        m_pOpenMap = NULL;
        g_pEditor->UnblockFrameExecution();
        return false;
    }

    g_pEditor->UnblockFrameExecution();
    return true;
}

bool EditorMapWindow::OpenMap(const char *fileName)
{
    DebugAssert(m_pOpenMap == NULL);
    g_pEditor->BlockFrameExecution();

    // setup progress dialog
    EditorProgressDialog progressDialog(this);
    progressDialog.show();
    
    m_pOpenMap = new EditorMap(this);
    if (!m_pOpenMap->OpenMap(fileName) || !OnMapOpened(&progressDialog))
    {
        delete m_pOpenMap;
        m_pOpenMap = NULL;
        g_pEditor->UnblockFrameExecution();
        return false;
    }

    g_pEditor->UnblockFrameExecution();
    return true;
}

bool EditorMapWindow::SaveMap()
{
    g_pEditor->BlockFrameExecution();

    // setup progress dialog
    EditorProgressDialog progressDialog(this);
    progressDialog.show();

    bool result = m_pOpenMap->SaveMap(NULL, &progressDialog);

    g_pEditor->UnblockFrameExecution();
    return result;
}

bool EditorMapWindow::SaveMapAs(const char *newFileName)
{
    // setup progress dialog
    EditorProgressDialog progressDialog(this);
    progressDialog.show();

    if (!m_pOpenMap->SaveMap(newFileName, &progressDialog))
        return false;

    m_ui->OnMapFileNameChanged(m_pOpenMap);
    return true;
}

void EditorMapWindow::CloseMap()
{
    DebugAssert(m_pOpenMap != NULL);
    g_pEditor->BlockFrameExecution();

    DeleteAllViewports();
    m_viewportLayout = EDITOR_VIEWPORT_LAYOUT_COUNT;
    
    m_pActiveEditModePointer->Deactivate();
    m_pActiveEditModePointer = NULL;

    // update ui
    {
        m_ui->propertyEditor->ClearProperties();
        m_ui->worldOutliner->Clear();

        m_ui->actionFileSaveMap->setEnabled(false);
        m_ui->actionFileSaveMapAs->setEnabled(false);
        m_ui->actionFileCloseMap->setEnabled(false);
        m_ui->actionEditUndo->setEnabled(false);
        m_ui->actionEditRedo->setEnabled(false);
        m_ui->actionEditReferenceCoordinateSystemLocal->setEnabled(false);
        m_ui->actionEditReferenceCoordinateSystemWorld->setEnabled(false);
        m_ui->actionViewWorldOutliner->setEnabled(false);
        m_ui->actionViewToolbox->setEnabled(false);
        m_ui->actionViewPropertyEditor->setEnabled(false);
        m_ui->menuViewViewportLayout->setEnabled(false);
        m_ui->menuInsert->setEnabled(false);
        m_ui->menuInsertEntity->setEnabled(false);
        m_ui->actionToolsCreateTerrain->setEnabled(false);
        m_ui->actionToolsDeleteTerrain->setEnabled(false);
        m_ui->actionToolsMapEditor->setEnabled(false);
        m_ui->actionToolsEntityEditor->setEnabled(false);
        m_ui->actionToolsGeometryEditor->setEnabled(false);
        m_ui->actionToolsTerrainEditor->setEnabled(false);

        // clear out everything in the tool window
        while (m_ui->toolboxTabWidget->count() > 0)
        {
            int index = m_ui->toolboxTabWidget->count() - 1;
            QWidget *widget = m_ui->toolboxTabWidget->widget(index);
            BlockSignalsForCall(m_ui->toolboxTabWidget)->removeTab(index);
            delete widget;
        }

        // hide windows
        m_ui->ToggleWorldOutliner(false);
        m_ui->ToggleToolbox(false);
        m_ui->TogglePropertyEditor(false);

        if (m_ui->closedMapBlankPanel == NULL)
        {
            m_ui->closedMapBlankPanel = new QWidget(m_ui->mainWindow, 0);
            m_ui->mainWindow->setCentralWidget(m_ui->closedMapBlankPanel);
        }

        m_ui->OnMapFileNameChanged(NULL);
    }

    delete m_pOpenMap;
    m_pOpenMap = NULL;

    g_pEditor->UnblockFrameExecution();
}

void EditorMapWindow::SetReferenceCoordinateSystem(EDITOR_REFERENCE_COORDINATE_SYSTEM referenceCoordinateSystem)
{
    DebugAssert(m_pOpenMap != NULL);
    DebugAssert(referenceCoordinateSystem < EDITOR_REFERENCE_COORDINATE_SYSTEM_COUNT);
    if (m_referenceCoordinateSystem != referenceCoordinateSystem)
    {
        m_referenceCoordinateSystem = referenceCoordinateSystem;

        // update entity edit mode
        if (m_editMode == EDITOR_EDIT_MODE_ENTITY)
            m_pOpenMap->GetEntityEditMode()->UpdateSelectionAxis();
    }

    // update ui
    m_ui->OnReferenceCoordinateSystemChanged(referenceCoordinateSystem);
}

void EditorMapWindow::SetEditMode(EDITOR_EDIT_MODE mode)
{
    DebugAssert(m_pOpenMap != NULL);
    if (mode == m_editMode)
    {
        m_ui->UpdateUIForEditMode(mode);
        return;
    }

    // get new mode pointer
    EditorEditMode *pNewEditModePointer;
    switch (mode)
    {
    case EDITOR_EDIT_MODE_MAP:                  pNewEditModePointer = m_pOpenMap->GetMapEditMode();             break;
    case EDITOR_EDIT_MODE_ENTITY:               pNewEditModePointer = m_pOpenMap->GetEntityEditMode();          break;
    case EDITOR_EDIT_MODE_GEOMETRY:             pNewEditModePointer = m_pOpenMap->GetGeometryEditMode();        break;
    case EDITOR_EDIT_MODE_TERRAIN:              pNewEditModePointer = m_pOpenMap->GetTerrainEditMode();         break;
    default:                                    pNewEditModePointer = NULL;                                     break;
    }

    // not created?
    if (pNewEditModePointer == NULL)
    {
        // revert mode
        m_ui->UpdateUIForEditMode(m_editMode);
        return;
    }

    // update edit mode var
    m_editMode = mode;

    // clear stuff from current edit mode
    m_pActiveEditModePointer->Deactivate();

    // clear property editor
    m_ui->propertyEditor->ClearProperties();

    // set pointer, and activate it
    m_pActiveEditModePointer = pNewEditModePointer;
    m_pActiveEditModePointer->Activate();

    // update ui
    m_ui->UpdateUIForEditMode(mode);

    // redraw all viewports
    RedrawAllViewports();
}

EditorMapViewport *EditorMapWindow::CreateViewport(EDITOR_CAMERA_MODE cameraMode, EDITOR_RENDER_MODE renderMode, uint32 flags)
{
    DebugAssert(m_pOpenMap != NULL);

    // find a free slot
    uint32 viewportId;
    for (viewportId = 0; viewportId < m_viewports.GetSize(); viewportId++)
    {
        if (m_viewports[viewportId] == NULL)
            break;
    }

    // create it
    EditorMapViewport *pViewport = new EditorMapViewport(this, m_pOpenMap, viewportId, cameraMode, renderMode, flags);

    // add to list
    if (viewportId >= m_viewports.GetSize())
        m_viewports.Resize(viewportId + 1);
    m_viewports[viewportId] = pViewport;

    // update streaming
    OnViewportCameraChange(pViewport);

    // return it
    return pViewport;
}

void EditorMapWindow::SetActiveViewport(EditorMapViewport *pViewport)
{
    m_pActiveViewport = pViewport;
    
    // update status bar
    UpdateStatusBarCameraFields();
}

void EditorMapWindow::SetViewportLayout(EDITOR_VIEWPORT_LAYOUT layout)
{
    if (m_viewportLayout == layout)
        return;

    m_viewportLayout = layout;
    DeleteAllViewports();

    EditorMapViewport *pViewport;
    switch (layout)
    {
    case EDITOR_VIEWPORT_LAYOUT_1X1:
        {
            pViewport = CreateViewport(EDITOR_CAMERA_MODE_PERSPECTIVE, EDITOR_RENDER_MODE_FULLBRIGHT, EDITOR_VIEWPORT_FLAG_ENABLE_SHADOWS | EDITOR_VIEWPORT_FLAG_ENABLE_DEBUG_INFO);
            setCentralWidget(pViewport);
        }
        break;

    case EDITOR_VIEWPORT_LAYOUT_2X2:
        {

        }
        break;
    }
}

void EditorMapWindow::RedrawAllViewports()
{
    for (uint32 i = 0; i < m_viewports.GetSize(); i++)
    {
        EditorMapViewport *pViewport = m_viewports[i];
        if (pViewport != NULL)
            pViewport->FlagForRedraw();
    }
}

void EditorMapWindow::DeleteAllViewports()
{
    setCentralWidget(NULL);

    for (uint32 i = 0; i < m_viewports.GetSize(); i++)
        delete m_viewports[i];

    m_viewports.Clear();
}

void EditorMapWindow::OnViewportCameraChange(EditorMapViewport *pViewport)
{
    m_pOpenMap->OnViewportCameraChange();

    if (m_pActiveViewport == pViewport)
        UpdateStatusBarCameraFields();
}

bool EditorMapWindow::OnMapOpened(ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    // set none edit mode
    m_editMode = EDITOR_EDIT_MODE_MAP;
    m_pActiveEditModePointer = m_pOpenMap->GetMapEditMode();
    m_pActiveEditModePointer->Activate();

    // create viewports
    SetViewportLayout(EDITOR_VIEWPORT_LAYOUT_1X1);

    // update ui
    {
        m_ui->actionFileSaveMap->setEnabled(true);
        m_ui->actionFileSaveMapAs->setEnabled(true);
        m_ui->actionFileCloseMap->setEnabled(true);
        m_ui->actionEditUndo->setEnabled(true);
        m_ui->actionEditRedo->setEnabled(true);
        m_ui->actionEditReferenceCoordinateSystemLocal->setEnabled(true);
        m_ui->actionEditReferenceCoordinateSystemWorld->setEnabled(true);
        m_ui->actionViewWorldOutliner->setEnabled(true);
        m_ui->actionViewToolbox->setEnabled(true);
        m_ui->actionViewPropertyEditor->setEnabled(true);
        m_ui->menuViewViewportLayout->setEnabled(true);
        m_ui->menuInsert->setEnabled(true);
        m_ui->menuInsertEntity->setEnabled(true);
        m_ui->actionToolsMapEditor->setEnabled(true);
        m_ui->actionToolsEntityEditor->setEnabled(true);
        m_ui->actionToolsGeometryEditor->setEnabled(true);

        if (m_ui->closedMapBlankPanel != NULL)
        {
            delete m_ui->closedMapBlankPanel;
            m_ui->closedMapBlankPanel = NULL;
        }

        m_ui->OnMapFileNameChanged(m_pOpenMap);

        // update grid snap options in status bar
        BlockSignalsForCall(m_ui->statusBarGridSnapEnabled)->setChecked(m_pOpenMap->GetGridSnapEnabled());
        BlockSignalsForCall(m_ui->statusBarGridSnapInterval)->setValue(m_pOpenMap->GetGridSnapInterval());
        BlockSignalsForCall(m_ui->statusBarGridLinesVisible)->setChecked(m_pOpenMap->GetGridLinesVisible());
        BlockSignalsForCall(m_ui->statusBarGridLinesInterval)->setValue(m_pOpenMap->GetGridLinesInterval());

        // create edit mode uis
        m_ui->toolboxTabWidget->addTab(m_pOpenMap->GetMapEditMode()->CreateUI(m_ui->toolboxTabWidget), QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")), tr("Map"));
        m_ui->toolboxTabWidget->addTab(m_pOpenMap->GetEntityEditMode()->CreateUI(m_ui->toolboxTabWidget), QIcon(QStringLiteral(":/editor/icons/Toolbar_Select.png")), tr("Entity"));
        m_ui->toolboxTabWidget->addTab(m_pOpenMap->GetGeometryEditMode()->CreateUI(m_ui->toolboxTabWidget), QIcon(QStringLiteral(":/editor/icons/HighlightHH.png")), tr("Geometry"));

        // has terrain?
        if (m_pOpenMap->HasTerrain())
        {
            m_ui->toolboxTabWidget->addTab(m_pOpenMap->GetTerrainEditMode()->CreateUI(m_ui->toolboxTabWidget), QIcon(QStringLiteral(":/editor/icons/BreakpointHS.png")), tr("Terrain"));
            
            m_ui->actionToolsTerrainEditor->setEnabled(true);
            m_ui->actionToolsCreateTerrain->setEnabled(false);
            m_ui->actionToolsDeleteTerrain->setEnabled(true);
        }
        else
        {
            m_ui->toolboxTabWidget->addTab(new QWidget(m_ui->toolboxTabWidget), QIcon(QStringLiteral(":/editor/icons/BreakpointHS.png")), tr("Terrain"));
            m_ui->toolboxTabWidget->setTabEnabled(3, false);

            m_ui->actionToolsTerrainEditor->setEnabled(false);
            m_ui->actionToolsCreateTerrain->setEnabled(true);
            m_ui->actionToolsDeleteTerrain->setEnabled(false);
        }
        
        m_ui->UpdateUIForEditMode(EDITOR_EDIT_MODE_MAP);
        m_ui->ToggleWorldOutliner(true);
        m_ui->ToggleToolbox(true);
        m_ui->TogglePropertyEditor(true);
    }

    // set focus to our only viewport
    DebugAssert(m_viewports.GetSize() > 0);
    m_viewports[0]->SetFocus();

    // set reference coordinate system
    SetReferenceCoordinateSystem(EDITOR_REFERENCE_COORDINATE_SYSTEM_WORLD);

    //EditorPropertyEditorDialog *w = new EditorPropertyEditorDialog(this);
    //w->PopulateForTable(g_pEditor->GetPropertyTemplates()->MapPropertyTemplate, m_pOpenMap->GetMapSource()->GetProperties());
    //w->show();

    // done
    return true;
}

void EditorMapWindow::LogCallback(void *pUserParam, const char *channelName, const char *functionName, LOGLEVEL level, const char *message)
{
    static bool reentrant = false;
    if (!reentrant)
    {
        reentrant = true;

        // hackfix to prevent the stupid thing from making the window massive
        QString tempString(message);
        if (tempString.length() > 100)
            tempString.truncate(100);

        reinterpret_cast<EditorMapWindow *>(pUserParam)->m_ui->statusBarMainText->setText(tempString);
        reentrant = false;
    }
}

void EditorMapWindow::closeEvent(QCloseEvent *pCloseEvent)
{
    if (IsMapOpen())
    {
        OnActionFileCloseMapTriggered();
        if (IsMapOpen())
        {
            // use selected cancel
            pCloseEvent->ignore();
            return;
        }
    }

    // accept event
    QMainWindow::closeEvent(pCloseEvent);
}

void EditorMapWindow::ConnectUIEvents()
{
    connect(m_ui->actionFileNewMap, SIGNAL(triggered()), this, SLOT(OnActionFileNewMapTriggered()));
    connect(m_ui->actionFileOpenMap, SIGNAL(triggered()), this, SLOT(OnActionFileOpenMapTriggered()));
    connect(m_ui->actionFileSaveMap, SIGNAL(triggered()), this, SLOT(OnActionFileSaveMapTriggered()));
    connect(m_ui->actionFileSaveMapAs, SIGNAL(triggered()), this, SLOT(OnActionFileSaveMapAsTriggered()));
    connect(m_ui->actionFileCloseMap, SIGNAL(triggered()), this, SLOT(OnActionFileCloseMapTriggered()));
    connect(m_ui->actionFileExit, SIGNAL(triggered()), this, SLOT(OnActionFileExitTriggered()));

    connect(m_ui->actionEditReferenceCoordinateSystemLocal, SIGNAL(triggered(bool)), this, SLOT(OnActionEditReferenceCoordinateSystemLocalTriggered(bool)));
    connect(m_ui->actionEditReferenceCoordinateSystemWorld, SIGNAL(triggered(bool)), this, SLOT(OnActionEditReferenceCoordinateSystemWorldTriggered(bool)));

    connect(m_ui->actionToolsCreateTerrain, SIGNAL(triggered()), this, SLOT(OnActionToolsCreateTerrainTriggered()));
    connect(m_ui->actionToolsDeleteTerrain, SIGNAL(triggered()), this, SLOT(OnActionToolsDeleteTerrainTriggered()));

    connect(m_ui->actionViewWorldOutliner, SIGNAL(triggered(bool)), this, SLOT(OnActionViewWorldOutlinerTriggered(bool)));
    connect(m_ui->worldOutlinerDock, SIGNAL(visibilityChanged(bool)), this, SLOT(OnActionViewWorldOutlinerTriggered(bool)));
    connect(m_ui->actionViewResourceBrowser, SIGNAL(triggered(bool)), this, SLOT(OnActionViewResourceBrowserTriggered(bool)));
    connect(m_ui->resourceBrowserDock, SIGNAL(visibilityChanged(bool)), this, SLOT(OnActionViewResourceBrowserTriggered(bool)));
    connect(m_ui->actionViewToolbox, SIGNAL(triggered(bool)), this, SLOT(OnActionViewToolboxTriggered(bool)));
    connect(m_ui->toolboxTabWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(OnActionViewToolboxTriggered(bool)));
    connect(m_ui->actionViewPropertyEditor, SIGNAL(triggered(bool)), this, SLOT(OnActionViewPropertyEditorTriggered(bool)));
    connect(m_ui->propertyEditorDockWidget, SIGNAL(visibilityChanged(bool)), this, SLOT(OnActionViewPropertyEditorTriggered(bool)));
    connect(m_ui->actionViewThemeNone, SIGNAL(triggered()), this, SLOT(OnActionViewThemeNone()));
    connect(m_ui->actionViewThemeDark, SIGNAL(triggered()), this, SLOT(OnActionViewThemeDark()));
    connect(m_ui->actionViewThemeDarkOther, SIGNAL(triggered()), this, SLOT(OnActionViewThemeDarkOther()));

    connect(m_ui->actionToolsMapEditor, SIGNAL(triggered(bool)), this, SLOT(OnActionToolsMapEditModeTriggered(bool)));
    connect(m_ui->actionToolsEntityEditor, SIGNAL(triggered(bool)), this, SLOT(OnActionToolsEntityEditModeTriggered(bool)));
    connect(m_ui->actionToolsGeometryEditor, SIGNAL(triggered(bool)), this, SLOT(OnActionToolsGeometryEditModeTriggered(bool)));
    connect(m_ui->actionToolsTerrainEditor, SIGNAL(triggered(bool)), this, SLOT(OnActionToolsHeightfieldTerrainEditModeTriggered(bool)));

    connect(m_ui->statusBarGridSnapEnabled, SIGNAL(toggled(bool)), this, SLOT(OnStatusBarGridSnapEnabledToggled(bool)));
    connect(m_ui->statusBarGridSnapInterval, SIGNAL(valueChanged(double)), this, SLOT(OnStatusBarGridSnapIntervalChanged(double)));
    connect(m_ui->statusBarGridLinesVisible, SIGNAL(toggled(bool)), this, SLOT(OnStatusBarGridLinesVisibleToggled(bool)));
    connect(m_ui->statusBarGridLinesInterval, SIGNAL(valueChanged(double)), this, SLOT(OnStatusBarGridLinesIntervalChanged(double)));

    connect(m_ui->worldOutliner, SIGNAL(OnEntitySelected(const EditorMapEntity *)), this, SLOT(OnWorldOutlinerEntitySelected(const EditorMapEntity *)));
    connect(m_ui->worldOutliner, SIGNAL(OnEntityActivated(const EditorMapEntity *)), this, SLOT(OnWorldOutlinerEntityActivated(const EditorMapEntity *)));

    connect(m_ui->toolboxTabWidget, SIGNAL(currentChanged(int)), this, SLOT(OnToolboxTabWidgetCurrentChanged(int)));

    connect(m_ui->propertyEditor, SIGNAL(OnPropertyValueChange(const char *, const char *)), this, SLOT(OnPropertyEditorPropertyChanged(const char *, const char *)));

    connect(g_pEditor, SIGNAL(OnFrameExecution(float)), this, SLOT(OnFrameExecutionTriggered(float)));
}

void EditorMapWindow::UpdateStatusBarCameraFields()
{
    float3 cameraPosition((m_pActiveViewport != nullptr) ? m_pActiveViewport->GetViewController().GetCameraPosition() : float3::Zero);
    float3 cameraDirection((m_pActiveViewport != nullptr) ? m_pActiveViewport->GetViewController().GetCameraForwardDirection() : float3::Zero);
    SmallString str;

    str.Format("Camera Position: %.5f %.5f %.5f", cameraPosition.x, cameraPosition.y, cameraPosition.z);
    m_ui->statusBarCameraPosition->setText(EditorHelpers::ConvertStringToQString(str));

    str.Format("Camera Direction: %.5f %.5f %.5f", cameraDirection.x, cameraDirection.y, cameraDirection.z);
    m_ui->statusBarCameraDirection->setText(EditorHelpers::ConvertStringToQString(str));
}

void EditorMapWindow::UpdateStatusBarFPSField()
{
    TinyString str;
    str.Format("%.4f FPS", g_pEditor->GetEditorFPS());
    m_ui->statusBarFPS->setText(EditorHelpers::ConvertStringToQString(str));
}

void EditorMapWindow::OnActionFileNewMapTriggered()
{
    if (IsMapOpen())
    {
        OnActionFileCloseMapTriggered();
        if (IsMapOpen())
        {
            // use selected cancel, since the map is still open
            return;
        }
    }

    NewMap();
}

void EditorMapWindow::OnActionFileOpenMapTriggered()
{
    g_pEditor->BlockFrameExecution();

    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Open Map..."),
                                                    QStringLiteral(""),
                                                    tr("Map Files (*.map)"),
                                                    NULL,
                                                    0);

    if (filename.isEmpty())
    {
        g_pEditor->UnblockFrameExecution();
        return;
    }

    if (IsMapOpen())
    {
        OnActionFileCloseMapTriggered();
        if (IsMapOpen())
        {
            // use selected cancel, since the map is still open
            g_pEditor->UnblockFrameExecution();
            return;
        }
    }

    OpenMap(ConvertQStringToString(filename));
    g_pEditor->UnblockFrameExecution();
}

void EditorMapWindow::OnActionFileSaveMapTriggered()
{
    if (!IsMapOpen())
        return;

    if (m_pOpenMap->GetMapFileName().GetLength() == 0)
    {
        // map has never been saved, so assume save as behaviour instead
        OnActionFileSaveMapAsTriggered();
        return;
    }

    SaveMap();
}

void EditorMapWindow::OnActionFileSaveMapAsTriggered()
{
    if (!IsMapOpen())
        return;

    g_pEditor->BlockFrameExecution();

    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save Map As..."),
                                                    QStringLiteral(""),
                                                    tr("Map Files (*.map)"),
                                                    NULL,
                                                    0);

    if (filename.isEmpty())
    {
        g_pEditor->UnblockFrameExecution();
        return;
    }

    SaveMapAs(ConvertQStringToString(filename));
    g_pEditor->UnblockFrameExecution();
}

void EditorMapWindow::OnActionFileCloseMapTriggered()
{
    if (!IsMapOpen())
        return;

    if (m_pOpenMap->GetMapSource()->IsChanged())
    {
        g_pEditor->BlockFrameExecution();

        int result = QMessageBox::question(this,
            tr("Save open map?"),
            tr("The current open map is changed, do you wish to save the changes made to it?"),
            (QMessageBox::StandardButtons)(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel),
            QMessageBox::Yes);

        g_pEditor->UnblockFrameExecution();

        if (result == QMessageBox::Yes)
            OnActionFileSaveMapTriggered();
        else if (result == QMessageBox::Cancel)
            return;
    }

    CloseMap();
}

void EditorMapWindow::OnActionFileExitTriggered()
{
    if (IsMapOpen())
    {
        OnActionFileCloseMapTriggered();
        if (IsMapOpen())
        {
            // cancel pressed if we are here
            return;
        }
    }

    g_pEditor->Exit();
}


void EditorMapWindow::OnActionViewWorldOutlinerTriggered(bool checked)
{
    m_ui->ToggleWorldOutliner(checked);
}

void EditorMapWindow::OnActionViewResourceBrowserTriggered(bool checked)
{
    m_ui->ToggleResourceBrowser(checked);
}

void EditorMapWindow::OnActionViewToolboxTriggered(bool checked)
{
    m_ui->ToggleToolbox(checked);
}

void EditorMapWindow::OnActionViewPropertyEditorTriggered(bool checked)
{
    m_ui->TogglePropertyEditor(checked);
}

void EditorMapWindow::OnActionToolsCreateTerrainTriggered()
{
    //DebugAssert(!m_pOpenMap->GetMapSource()->HasTerrain());
    
    EditorCreateTerrainDialog createDialog(this);
    if (!createDialog.exec())
        return;

    // create it
    /*
    m_pOpenMap->CreateTerrain(createDialog.GetLayerList(), createDialog.GetSectionSize(), createDialog.GetUnitsPerPoint(), createDialog.GetQuadTreeNodeSize(),
                              createDialog.GetTextureRepeatInterval(), createDialog.GetStorageFormat(), createDialog.GetMinHeight(), createDialog.GetMaxHeight(),
                              createDialog.GetBaseHeight());

    // create initial section?
    if (createDialog.GetCreateCenterSection())
        m_pOpenMap->CreateTerrainSection(0, 0);
    */
}

void EditorMapWindow::OnActionToolsDeleteTerrainTriggered()
{
//     DebugAssert(m_pOpenMap->GetMapSource()->HasTerrain());
//     if (QMessageBox::question(this, tr("Delete Terrain"), tr("Are you sure you wish to delete the terrain for this map?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
//         return;
// 
//     // delete it
//     m_pOpenMap->DeleteTerrain();
}

void EditorMapWindow::OnActionToolsCreateBlockTerrainTriggered()
{

}

void EditorMapWindow::OnActionToolsDeleteBlockTerrainTriggered()
{

}

void EditorMapWindow::OnStatusBarGridSnapEnabledToggled(bool checked)
{
    if (m_pOpenMap != nullptr)
        m_pOpenMap->SetGridSnapEnabled(checked);
}

void EditorMapWindow::OnStatusBarGridSnapIntervalChanged(double value)
{
    if (m_pOpenMap != nullptr)
        m_pOpenMap->SetGridSnapInterval((float)value);
}

void EditorMapWindow::OnStatusBarGridLinesVisibleToggled(bool checked)
{
    if (m_pOpenMap != nullptr)
        m_pOpenMap->SetGridLinesVisible(checked);
}

void EditorMapWindow::OnStatusBarGridLinesIntervalChanged(double value)
{
    if (m_pOpenMap != nullptr)
        m_pOpenMap->SetGridLinesInterval((float)value);
}

void EditorMapWindow::OnWorldOutlinerEntitySelected(const EditorMapEntity *pEntity)
{
    if (pEntity != nullptr)
    {
        if (m_editMode != EDITOR_EDIT_MODE_ENTITY)
            SetEditMode(EDITOR_EDIT_MODE_ENTITY);

        m_pOpenMap->GetEntityEditMode()->ClearSelection();
        m_pOpenMap->GetEntityEditMode()->AddSelection(const_cast<EditorMapEntity *>(pEntity));
        RedrawAllViewports();
    }
}

void EditorMapWindow::OnWorldOutlinerEntityActivated(const EditorMapEntity *pEntity)
{
    if (pEntity != nullptr)
    {
        if (m_editMode != EDITOR_EDIT_MODE_ENTITY)
            SetEditMode(EDITOR_EDIT_MODE_ENTITY);

        m_pOpenMap->GetEntityEditMode()->ClearSelection();
        m_pOpenMap->GetEntityEditMode()->AddSelection(const_cast<EditorMapEntity *>(pEntity));

        if (m_pActiveViewport != nullptr)
            m_pOpenMap->GetEntityEditMode()->MoveViewportCameraToEntity(m_pActiveViewport, pEntity);

        RedrawAllViewports();
    }
}

void EditorMapWindow::OnToolboxTabWidgetCurrentChanged(int index)
{
    if (index < 0)
        return;

    // set new edit mode
    EDITOR_EDIT_MODE newEditMode = (EDITOR_EDIT_MODE)(index + 1);
    DebugAssert(newEditMode < EDITOR_EDIT_MODE_COUNT);
    SetEditMode(newEditMode);
}

void EditorMapWindow::OnPropertyEditorPropertyChanged(const char *propertyName, const char *propertyValue)
{
    m_pActiveEditModePointer->OnPropertyEditorPropertyChanged(propertyName, propertyValue);
}

void EditorMapWindow::OnFrameExecutionTriggered(float timeSinceLastFrame)
{
    if (m_pOpenMap != nullptr)
    {
        bool viewportDrawn = false;

        // update map
        m_pOpenMap->Update(timeSinceLastFrame);

        // draw viewports
        for (uint32 i = 0; i < m_viewports.GetSize(); i++)
            viewportDrawn |= m_viewports[i]->Draw(timeSinceLastFrame);

        // update fps
        if (viewportDrawn)
            UpdateStatusBarFPSField();
    }
}

