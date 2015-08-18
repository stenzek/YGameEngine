#pragma once
#include "Editor/EditorPropertyEditorWidget.h"
#include "Editor/MapEditor/EditorWorldOutlinerWidget.h"
#include "Editor/ResourceBrowser/EditorResourceBrowserWidget.h"

struct Ui_EditorMapWindow
{
    QMainWindow *mainWindow;

    // actions
    QAction *actionFileNewMap;
    QAction *actionFileOpenMap;
    QAction *actionFileSaveMap;
    QAction *actionFileSaveMapAs;
    QAction *actionFileCloseMap;
    QAction *actionFileExit;
    QAction *actionEditUndo;
    QAction *actionEditRedo;
    QAction *actionEditReferenceCoordinateSystemLocal;
    QAction *actionEditReferenceCoordinateSystemWorld;
    QAction *actionViewWorldOutliner;
    QAction *actionViewResourceBrowser;
    QAction *actionViewToolbox;
    QAction *actionViewPropertyEditor;
    QAction *actionViewViewportLayout1x1;
    QAction *actionViewViewportLayout2x2;
    QAction *actionViewThemeNone;
    QAction *actionViewThemeDark;
    QAction *actionViewThemeDarkOther;
    QAction *actionViewVisibleLayers;
    QAction *actionToolsMapEditor;
    QAction *actionToolsEntityEditor;
    QAction *actionToolsGeometryEditor;
    QAction *actionToolsTerrainEditor;
    QAction *actionToolsCreateTerrain;
    QAction *actionToolsDeleteTerrain;
    
    // menus
    QMenu *menuFile;
    QMenu *menuEdit;
    QMenu *menuEditReferenceCoordinateSystem;
    QMenu *menuView;
    QMenu *menuViewViewportLayout;
    QMenu *menuViewTheme;
    QMenu *menuInsert;
    QMenu *menuInsertEntity;
    QMenu *menuTools;
    QMenu *menuHelp;

    // left pane
    QWidget *closedMapBlankPanel;

    // world browser edit dock
    QDockWidget *worldOutlinerDock;
    EditorWorldOutlinerWidget *worldOutliner;

    // resource browser edit dock
    QDockWidget *resourceBrowserDock;
    EditorResourceBrowserWidget *resourceBrowser;

    // tool dock
    QDockWidget *toolboxDockWidget;
    QTabWidget *toolboxTabWidget;

    // properties dock
    QDockWidget *propertyEditorDockWidget;
    EditorPropertyEditorWidget *propertyEditor;

    // widgets
    QMenuBar *menubar;
    QToolBar *toolbar;
    
    QStatusBar *statusBar;
    QLabel *statusBarMainText;
    QCheckBox *statusBarGridSnapEnabled;
    QDoubleSpinBox *statusBarGridSnapInterval;
    QCheckBox *statusBarGridLinesVisible;
    QDoubleSpinBox *statusBarGridLinesInterval;
    QLabel *statusBarCameraPosition;
    QLabel *statusBarCameraDirection;
    QLabel *statusBarFPS;
    
    void CreateUI(EditorMapWindow *pMainWindow)
    {
        // resize window
        mainWindow = pMainWindow;
        //mainWindow->resize(1280, 720);
        mainWindow->resize(1366, 768);

        actionFileNewMap = new QAction(pMainWindow);
        actionFileNewMap->setIcon(QIcon(QStringLiteral(":/editor/icons/NewDocumentHS.png")));
        actionFileNewMap->setText(pMainWindow->tr("&New Map"));

        actionFileOpenMap = new QAction(pMainWindow);
        actionFileOpenMap->setIcon(QIcon(QStringLiteral(":/editor/icons/openHS.png")));
        actionFileOpenMap->setText(pMainWindow->tr("&Open Map"));

        actionFileSaveMap = new QAction(pMainWindow);
        actionFileSaveMap->setIcon(QIcon(QStringLiteral(":/editor/icons/saveHS.png")));
        actionFileSaveMap->setText(pMainWindow->tr("&Save Map"));

        actionFileSaveMapAs = new QAction(pMainWindow);
        actionFileSaveMapAs->setText(pMainWindow->tr("Save Map &As..."));

        actionFileCloseMap = new QAction(pMainWindow);
        actionFileCloseMap->setText(pMainWindow->tr("&Close Map"));

        actionFileExit = new QAction(pMainWindow);
        actionFileExit->setText(pMainWindow->tr("E&xit"));

        actionEditUndo = new QAction(pMainWindow);
        actionEditUndo->setIcon(QIcon(QStringLiteral(":/editor/icons/Edit_UndoHS.png")));
        actionEditUndo->setText(pMainWindow->tr("&Undo"));

        actionEditRedo = new QAction(pMainWindow);
        actionEditRedo->setIcon(QIcon(QStringLiteral(":/editor/icons/Edit_RedoHS.png")));
        actionEditRedo->setText(pMainWindow->tr("&Undo"));

        actionEditReferenceCoordinateSystemLocal = new QAction(pMainWindow);
        actionEditReferenceCoordinateSystemLocal->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_LocalCoordSystem.png")));
        actionEditReferenceCoordinateSystemLocal->setText(pMainWindow->tr("&Local"));
        actionEditReferenceCoordinateSystemLocal->setCheckable(true);

        actionEditReferenceCoordinateSystemWorld = new QAction(pMainWindow);
        actionEditReferenceCoordinateSystemWorld->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_WorldCoordSystem.png")));
        actionEditReferenceCoordinateSystemWorld->setText(pMainWindow->tr("&World"));
        actionEditReferenceCoordinateSystemWorld->setCheckable(true);
       
        actionViewWorldOutliner = new QAction(pMainWindow);
        actionViewWorldOutliner->setText(pMainWindow->tr("&World Outliner"));
        actionViewWorldOutliner->setIcon(QIcon(QStringLiteral(":/editor/icons/camera.png")));
        actionViewWorldOutliner->setCheckable(true);

        actionViewResourceBrowser = new QAction(pMainWindow);
        actionViewResourceBrowser->setText(pMainWindow->tr("&Resource Browser"));
        actionViewResourceBrowser->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_Select.png")));
        actionViewResourceBrowser->setCheckable(true);

        actionViewToolbox = new QAction(pMainWindow);
        actionViewToolbox->setText(pMainWindow->tr("&Toolbox"));
        actionViewToolbox->setIcon(QIcon(QStringLiteral(":/editor/icons/HighlightHH.png")));
        actionViewToolbox->setCheckable(true);

        actionViewPropertyEditor = new QAction(pMainWindow);
        actionViewPropertyEditor->setText(pMainWindow->tr("&Property Editor"));
        actionViewPropertyEditor->setIcon(QIcon(QStringLiteral(":/editor/icons/HighlightHH.png")));
        actionViewPropertyEditor->setCheckable(true);

        actionViewViewportLayout1x1 = new QAction(pMainWindow);
        actionViewViewportLayout1x1->setText(pMainWindow->tr("1x1"));
        actionViewViewportLayout1x1->setCheckable(true);

        actionViewViewportLayout2x2 = new QAction(pMainWindow);
        actionViewViewportLayout2x2->setText(pMainWindow->tr("2x2"));
        actionViewViewportLayout2x2->setCheckable(true);

        actionViewThemeNone = new QAction(pMainWindow);
        actionViewThemeNone->setText(pMainWindow->tr("&None"));

        actionViewThemeDark = new QAction(pMainWindow);
        actionViewThemeDark->setText(pMainWindow->tr("&Dark"));

        actionViewThemeDarkOther = new QAction(pMainWindow);
        actionViewThemeDarkOther->setText(pMainWindow->tr("Dark &Other"));

        actionViewVisibleLayers = new QAction(pMainWindow->tr("Visible &Layers"), pMainWindow);
        actionViewVisibleLayers->setIcon(QIcon(QStringLiteral(":/editor/icons/Directory_16x16.png")));

        actionToolsMapEditor = new QAction(pMainWindow);
        actionToolsMapEditor->setText(pMainWindow->tr("&Map Editor"));
        actionToolsMapEditor->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
        actionToolsMapEditor->setCheckable(true);

        actionToolsEntityEditor = new QAction(pMainWindow);
        actionToolsEntityEditor->setText(pMainWindow->tr("&Entity Editor"));
        actionToolsEntityEditor->setIcon(QIcon(QStringLiteral(":/editor/icons/Toolbar_Select.png")));
        actionToolsEntityEditor->setCheckable(true);

        actionToolsGeometryEditor = new QAction(pMainWindow);
        actionToolsGeometryEditor->setText(pMainWindow->tr("&Geometry Editor"));
        actionToolsGeometryEditor->setIcon(QIcon(QStringLiteral(":/editor/icons/HighlightHH.png")));
        actionToolsGeometryEditor->setCheckable(true);

        actionToolsTerrainEditor = new QAction(pMainWindow);
        actionToolsTerrainEditor->setText(pMainWindow->tr("&Terrain Editor"));
        actionToolsTerrainEditor->setIcon(QIcon(QStringLiteral(":/editor/icons/BreakpointHS.png")));
        actionToolsTerrainEditor->setCheckable(true);

        actionToolsCreateTerrain = new QAction(pMainWindow);
        actionToolsCreateTerrain->setText(pMainWindow->tr("Create Terrain"));

        actionToolsDeleteTerrain = new QAction(pMainWindow);
        actionToolsDeleteTerrain->setText(pMainWindow->tr("Delete Terrain"));

        menuFile = new QMenu(pMainWindow);
        menuFile->setTitle(pMainWindow->tr("&File"));
        menuFile->addAction(actionFileNewMap);
        menuFile->addAction(actionFileOpenMap);
        menuFile->addAction(actionFileSaveMap);
        menuFile->addAction(actionFileSaveMapAs);
        menuFile->addAction(actionFileCloseMap);
        menuFile->addSeparator();
        menuFile->addAction(actionFileExit);

        menuEdit = new QMenu(pMainWindow);
        menuEdit->setTitle(pMainWindow->tr("&Edit"));
        menuEdit->addAction(actionEditUndo);
        menuEdit->addAction(actionEditRedo);
        menuEdit->addSeparator();
        menuEditReferenceCoordinateSystem = new QMenu(pMainWindow);
        menuEditReferenceCoordinateSystem->setTitle(pMainWindow->tr("&Reference Coordinate System"));
        menuEditReferenceCoordinateSystem->addAction(actionEditReferenceCoordinateSystemLocal);
        menuEditReferenceCoordinateSystem->addAction(actionEditReferenceCoordinateSystemWorld);
        menuEdit->addMenu(menuEditReferenceCoordinateSystem);

        menuView = new QMenu(pMainWindow);
        menuView->setTitle(pMainWindow->tr("&View"));
        menuView->addAction(actionViewWorldOutliner);
        menuView->addAction(actionViewResourceBrowser);
        menuView->addAction(actionViewToolbox);
        menuView->addAction(actionViewPropertyEditor);
        menuView->addSeparator();
        menuView->addAction(actionViewVisibleLayers);
        menuView->addSeparator();

        menuViewViewportLayout = new QMenu(menuView);
        menuViewViewportLayout->setTitle(pMainWindow->tr("&Viewport Layout"));
        menuViewViewportLayout->addAction(actionViewViewportLayout1x1);
        menuViewViewportLayout->addAction(actionViewViewportLayout2x2);
        menuView->addMenu(menuViewViewportLayout);
        menuView->addSeparator();

        menuViewTheme = new QMenu(menuView);
        menuViewTheme->setTitle(pMainWindow->tr("&Theme"));
        menuViewTheme->addAction(actionViewThemeNone);
        menuViewTheme->addAction(actionViewThemeDark);
        menuViewTheme->addAction(actionViewThemeDarkOther);
        menuView->addMenu(menuViewTheme);

        menuInsertEntity = new QMenu(pMainWindow);
        menuInsertEntity->setTitle(pMainWindow->tr("&Entity"));

        menuInsert = new QMenu(pMainWindow);
        menuInsert->setTitle(pMainWindow->tr("&Insert"));
        menuInsert->addMenu(menuInsertEntity);
        menuInsert->addSeparator();

        menuTools = new QMenu(pMainWindow);
        menuTools->setTitle(pMainWindow->tr("&Tools"));
        menuTools->addAction(actionToolsMapEditor);
        menuTools->addAction(actionToolsEntityEditor);
        menuTools->addAction(actionToolsGeometryEditor);
        menuTools->addAction(actionToolsTerrainEditor);
        menuTools->addSeparator();
        menuTools->addAction(actionToolsCreateTerrain);
        menuTools->addAction(actionToolsDeleteTerrain);

        menuHelp = new QMenu(pMainWindow);
        menuHelp->setTitle(pMainWindow->tr("&Help"));

        menubar = new QMenuBar(pMainWindow);
        {
            menubar->addMenu(menuFile);
            menubar->addMenu(menuEdit);
            menubar->addMenu(menuView);
            menubar->addMenu(menuInsert);
            menubar->addMenu(menuTools);
            menubar->addMenu(menuHelp);
        }
        pMainWindow->setMenuBar(menubar);

        toolbar = new QToolBar(pMainWindow);
        {
            toolbar->setIconSize(QSize(16, 16));
            toolbar->addAction(actionFileNewMap);
            toolbar->addAction(actionFileOpenMap);
            toolbar->addAction(actionFileSaveMap);
            toolbar->addSeparator();
            toolbar->addAction(actionEditUndo);
            toolbar->addAction(actionEditRedo);
            toolbar->addSeparator();
            toolbar->addAction(actionEditReferenceCoordinateSystemLocal);
            toolbar->addAction(actionEditReferenceCoordinateSystemWorld);
            toolbar->addSeparator();
            toolbar->addAction(actionViewVisibleLayers);
            toolbar->addSeparator();
            toolbar->addAction(actionViewWorldOutliner);
            toolbar->addAction(actionViewResourceBrowser);
            toolbar->addAction(actionViewToolbox);
            toolbar->addAction(actionViewPropertyEditor);
            toolbar->addSeparator();
            toolbar->addAction(actionToolsMapEditor);
            toolbar->addAction(actionToolsEntityEditor);
            toolbar->addAction(actionToolsGeometryEditor);
            toolbar->addAction(actionToolsTerrainEditor);
        }
        pMainWindow->addToolBar(toolbar);

        statusBar = new QStatusBar(pMainWindow);
        {
            statusBarMainText = new QLabel(statusBar);
            statusBar->addWidget(statusBarMainText, 1);

            // grid snap
            {
                QWidget *gridSnapContainer = new QWidget(statusBar);
                QHBoxLayout *horizontalLayout = new QHBoxLayout(gridSnapContainer);
                horizontalLayout->setMargin(0);
                horizontalLayout->setSpacing(0);
                horizontalLayout->setContentsMargins(0, 0, 0, 0);

                statusBarGridSnapEnabled = new QCheckBox(gridSnapContainer);
                horizontalLayout->addWidget(statusBarGridSnapEnabled);

                statusBarGridSnapInterval = new QDoubleSpinBox(gridSnapContainer);
                statusBarGridSnapInterval->setMinimum(0.1);
                statusBarGridSnapInterval->setMaximum(100.0);
                statusBarGridSnapInterval->setSingleStep(0.1);
                horizontalLayout->addWidget(statusBarGridSnapInterval);

                gridSnapContainer->setLayout(horizontalLayout);
                statusBar->addWidget(gridSnapContainer);
            }

            // grid lines
            {
                QWidget *gridLinesContainer = new QWidget(statusBar);
                QHBoxLayout *horizontalLayout = new QHBoxLayout(gridLinesContainer);
                horizontalLayout->setMargin(0);
                horizontalLayout->setSpacing(0);
                horizontalLayout->setContentsMargins(0, 0, 0, 0);

                statusBarGridLinesVisible = new QCheckBox(gridLinesContainer);
                horizontalLayout->addWidget(statusBarGridLinesVisible);

                statusBarGridLinesInterval = new QDoubleSpinBox(gridLinesContainer);
                statusBarGridLinesInterval->setMinimum(0.1);
                statusBarGridLinesInterval->setMaximum(100.0);
                statusBarGridLinesInterval->setSingleStep(0.1);
                horizontalLayout->addWidget(statusBarGridLinesInterval);

                gridLinesContainer->setLayout(horizontalLayout);
                statusBar->addWidget(gridLinesContainer);
            }

            statusBarCameraPosition = new QLabel(statusBar);
            statusBar->addWidget(statusBarCameraPosition);

            statusBarCameraDirection = new QLabel(statusBar);
            statusBar->addWidget(statusBarCameraDirection);

            statusBarFPS = new QLabel(statusBar);
            statusBar->addWidget(statusBarFPS);
        }
        pMainWindow->setStatusBar(statusBar);

        closedMapBlankPanel = new QWidget(pMainWindow);
        pMainWindow->setCentralWidget(closedMapBlankPanel);

        worldOutlinerDock = new QDockWidget(pMainWindow);
        worldOutlinerDock->setWindowTitle(pMainWindow->tr("World Outliner"));
        worldOutlinerDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
        worldOutliner = new EditorWorldOutlinerWidget(worldOutlinerDock);
        worldOutlinerDock->setWidget(worldOutliner);
        pMainWindow->addDockWidget(Qt::LeftDockWidgetArea, worldOutlinerDock);

        resourceBrowserDock = new QDockWidget(pMainWindow);
        resourceBrowserDock->setWindowTitle(pMainWindow->tr("Resource Browser"));
        resourceBrowserDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
        resourceBrowser = new EditorResourceBrowserWidget(pMainWindow, resourceBrowserDock);
        resourceBrowserDock->setWidget(resourceBrowser);
        pMainWindow->addDockWidget(Qt::LeftDockWidgetArea, resourceBrowserDock);

        toolboxDockWidget = new QDockWidget(pMainWindow);
        toolboxDockWidget->setWindowTitle(pMainWindow->tr("Tool"));
        toolboxDockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
        toolboxTabWidget = new QTabWidget(toolboxDockWidget);
        toolboxTabWidget->setTabPosition(QTabWidget::East);
        toolboxDockWidget->setWidget(toolboxTabWidget);
        pMainWindow->addDockWidget(Qt::RightDockWidgetArea, toolboxDockWidget);

        propertyEditorDockWidget = new QDockWidget(pMainWindow);
        propertyEditorDockWidget->setWindowTitle(pMainWindow->tr("Property Editor"));
        propertyEditorDockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
        propertyEditor = new EditorPropertyEditorWidget(propertyEditorDockWidget);
        propertyEditorDockWidget->setWidget(propertyEditor);
        pMainWindow->addDockWidget(Qt::RightDockWidgetArea, propertyEditorDockWidget);

        // set the initial state of everything
        actionFileSaveMap->setEnabled(false);
        actionFileSaveMapAs->setEnabled(false);
        actionFileCloseMap->setEnabled(false);
        actionEditUndo->setEnabled(false);
        actionEditRedo->setEnabled(false);
        actionEditReferenceCoordinateSystemLocal->setEnabled(false);
        actionEditReferenceCoordinateSystemWorld->setEnabled(false);
        actionEditReferenceCoordinateSystemWorld->setCheckable(true);
        actionToolsMapEditor->setEnabled(false);
        actionToolsEntityEditor->setEnabled(false);
        actionToolsGeometryEditor->setEnabled(false);
        actionToolsTerrainEditor->setEnabled(false);
        actionToolsCreateTerrain->setEnabled(false);
        actionToolsDeleteTerrain->setEnabled(false);
        actionViewWorldOutliner->setEnabled(false);
        actionViewResourceBrowser->setEnabled(true);
        actionViewResourceBrowser->setChecked(true);
        actionViewToolbox->setEnabled(false);
        actionViewPropertyEditor->setEnabled(false);
        menuViewViewportLayout->setEnabled(false);
        menuInsert->setEnabled(false);
        menuInsertEntity->setEnabled(false);
        menuViewViewportLayout->setEnabled(false);

        // and hide the windows
        ToggleWorldOutliner(false);
        ToggleResourceBrowser(true);
        ToggleToolbox(false);
        TogglePropertyEditor(false);
    }

    void ToggleWorldOutliner(bool visible)
    {
        (visible) ? worldOutlinerDock->show() : worldOutlinerDock->hide();
        actionViewWorldOutliner->setChecked(visible);
    }

    void ToggleResourceBrowser(bool visible)
    {
        (visible) ? resourceBrowserDock->show() : resourceBrowserDock->hide();
        actionViewResourceBrowser->setChecked(visible);
    }

    void ToggleToolbox(bool visible)
    {
        (visible) ? toolboxDockWidget->show() : toolboxDockWidget->hide();
        actionViewToolbox->setChecked(visible);
    }

    void TogglePropertyEditor(bool visible)
    {
        (visible) ? propertyEditorDockWidget->show() : propertyEditorDockWidget->hide();
        actionViewPropertyEditor->setChecked(visible);
    }

    void OnReferenceCoordinateSystemChanged(EDITOR_REFERENCE_COORDINATE_SYSTEM coordinateSystem)
    {
        actionEditReferenceCoordinateSystemLocal->setChecked((coordinateSystem == EDITOR_REFERENCE_COORDINATE_SYSTEM_LOCAL));
        actionEditReferenceCoordinateSystemWorld->setChecked((coordinateSystem == EDITOR_REFERENCE_COORDINATE_SYSTEM_WORLD));
    }
    
    void OnMapFileNameChanged(EditorMap *pEditorMap)
    {
        QString windowTitle = mainWindow->tr("Map Editor");

        if (pEditorMap != NULL)
        {
            windowTitle.append(" - ");
            if (pEditorMap->GetMapFileName().GetLength() == 0)
                windowTitle.append("<unsaved map>");
            else
                windowTitle.append(pEditorMap->GetMapFileName().GetCharArray());
        }

        mainWindow->setWindowTitle(windowTitle);
    }

    void UpdateUIForEditMode(EDITOR_EDIT_MODE editMode)
    {
        DebugAssert(editMode < EDITOR_EDIT_MODE_COUNT);
        if (editMode != EDITOR_EDIT_MODE_NONE)
            toolboxTabWidget->setCurrentIndex((int)editMode - 1);

        actionToolsMapEditor->setChecked((editMode == EDITOR_EDIT_MODE_MAP));
        actionToolsEntityEditor->setChecked((editMode == EDITOR_EDIT_MODE_ENTITY));
        actionToolsGeometryEditor->setChecked((editMode == EDITOR_EDIT_MODE_GEOMETRY));
        actionToolsTerrainEditor->setChecked((editMode == EDITOR_EDIT_MODE_TERRAIN));

        ToggleToolbox(true);
    }
};

