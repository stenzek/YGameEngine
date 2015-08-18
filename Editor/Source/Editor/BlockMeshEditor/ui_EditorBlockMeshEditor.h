#pragma once

class Ui_EditorBlockMeshEditor
{
public:
    QMainWindow *mainWindow;

    // actions
    QAction *actionSaveMesh;
    QAction *actionSaveMeshAs;
    QAction *actionClose;
    QAction *actionEditUndo;
    QAction *actionEditRedo;
    QAction *actionCameraPerspective;
    QAction *actionCameraArcball;
    QAction *actionCameraIsometric;
    QAction *actionViewWireframe;
    QAction *actionViewUnlit;
    QAction *actionViewLit;
    QAction *actionViewFlagShadows;
    QAction *actionViewFlagWireframeOverlay;
    QAction *actionWidgetCamera;
    QAction *actionWidgetSelectBlocks;
    QAction *actionWidgetSelectArea;
    QAction *actionWidgetPlaceBlocks;
    QAction *actionWidgetDeleteBlocks;
    QAction *actionWidgetLightManipulator;
        
    // menus
    QMenu *menuMesh;
    QMenu *menuEdit;
    QMenu *menuCamera;
    QMenu *menuView;
    QMenu *menuWidget;
    QMenu *menuHelp;

    // toolbar items
    QComboBox *activeLODComboBox;

    // left pane
    EditorRendererSwapChainWidget *swapChainWidget;

    // right pane
    // camera mode
    QDockWidget *cameraDock;
    
    // select blocks mode
    QDockWidget *selectBlocksAreaDock;

    // place blocks mode
    QDockWidget *placeBlocksDock;
    QTabWidget *placeBlocksWidget;
    QWidget *placeBlocksWidgetPaletteIconsWidget;
    FlowLayout *placeBlocksWidgetPaletteIconsFlowLayout;
    QToolButton *placeBlocksWidgetPaletteIcons[BLOCK_MESH_MAX_BLOCK_TYPES];
    QListWidget *placeBlocksWidgetPaletteList;

    // delete blocks mode
    QDockWidget *deleteBlocksDock;

    // light manipulator mode
    QDockWidget *lightManipulatorDock;
    
    // widgets
    QMenuBar *menubar;
    QToolBar *toolbar;
    
    QStatusBar *statusBar;
    QLabel *statusBarMainText;
    QLabel *statusBarMeshSize;
    QProgressBar *statusBarProgressBar;
    QLabel *statusBarFPS;
    
    void CreateUI(QMainWindow *pMainWindow)
    {
        static const char *TRANSLATION_CONTEXT = "EditorBlockMeshEditor";
        //QVBoxLayout *verticalLayout;

        mainWindow = pMainWindow;
        mainWindow->resize(800, 600);

        actionSaveMesh = new QAction(pMainWindow);
        actionSaveMesh->setIcon(QIcon(QStringLiteral(":/editor/icons/saveHS.png")));
        actionSaveMesh->setText(QApplication::translate(TRANSLATION_CONTEXT, "&Save Mesh"));

        actionSaveMeshAs = new QAction(pMainWindow);
        actionSaveMeshAs->setIcon(QIcon(QStringLiteral(":/editor/icons/saveHS.png")));
        actionSaveMeshAs->setText(QApplication::translate(TRANSLATION_CONTEXT, "Save Mesh &As"));

        actionClose = new QAction(pMainWindow);
        actionClose->setText(QApplication::translate(TRANSLATION_CONTEXT, "&Close"));

        actionEditUndo = new QAction(pMainWindow);
        actionEditUndo->setIcon(QIcon(QStringLiteral(":/editor/icons/Edit_UndoHS.png")));
        actionEditUndo->setText(QApplication::translate(TRANSLATION_CONTEXT, "&Undo"));

        actionEditRedo = new QAction(pMainWindow);
        actionEditRedo->setIcon(QIcon(QStringLiteral(":/editor/icons/Edit_RedoHS.png")));
        actionEditRedo->setText(QApplication::translate(TRANSLATION_CONTEXT, "&Redo"));

        actionCameraPerspective = new QAction(pMainWindow);
        actionCameraPerspective->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_View_Perspective.png")));
        actionCameraPerspective->setText(QApplication::translate(TRANSLATION_CONTEXT, "&Perspective Camera"));
        actionCameraPerspective->setCheckable(true);

        actionCameraArcball = new QAction(pMainWindow);
        actionCameraArcball->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_View_Side.png")));
        actionCameraArcball->setText(QApplication::translate(TRANSLATION_CONTEXT, "&Arcball Camera"));
        actionCameraArcball->setCheckable(true);
        
        actionCameraIsometric = new QAction(pMainWindow);
        actionCameraIsometric->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_View_Isometric.png")));
        actionCameraIsometric->setText(QApplication::translate(TRANSLATION_CONTEXT, "&Isometric Camera"));
        actionCameraIsometric->setCheckable(true);

        actionViewWireframe = new QAction(pMainWindow);
        actionViewWireframe->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderMode_Wireframe.png")));
        actionViewWireframe->setText(QApplication::translate(TRANSLATION_CONTEXT, "&Wireframe View"));
        actionViewWireframe->setCheckable(true);

        actionViewUnlit = new QAction(pMainWindow);
        actionViewUnlit->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderMode_Unlit.png")));
        actionViewUnlit->setText(QApplication::translate(TRANSLATION_CONTEXT, "&Unlit View"));
        actionViewUnlit->setCheckable(true);

        actionViewLit = new QAction(pMainWindow);
        actionViewLit->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderMode_Lit.png")));
        actionViewLit->setText(QApplication::translate(TRANSLATION_CONTEXT, "&Lit View"));
        actionViewLit->setCheckable(true);
        
        actionViewFlagShadows = new QAction(pMainWindow);
        actionViewFlagShadows->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderFlag_Shadows.png")));
        actionViewFlagShadows->setText(QApplication::translate(TRANSLATION_CONTEXT, "Enable &Shadows"));
        actionViewFlagShadows->setCheckable(true);

        actionViewFlagWireframeOverlay = new QAction(pMainWindow);
        actionViewFlagWireframeOverlay->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderFlag_WireframeOverlay.png")));
        actionViewFlagWireframeOverlay->setText(QApplication::translate(TRANSLATION_CONTEXT, "Enable Wireframe &Overlay"));
        actionViewFlagWireframeOverlay->setCheckable(true);

        actionWidgetCamera = new QAction(pMainWindow);
        actionWidgetCamera->setIcon(QIcon(QStringLiteral(":/editor/icons/camera.png")));
        actionWidgetCamera->setText(QApplication::translate(TRANSLATION_CONTEXT, "&Camera Widget"));
        actionWidgetCamera->setCheckable(true);

        actionWidgetSelectBlocks = new QAction(pMainWindow);
        actionWidgetSelectBlocks->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
        actionWidgetSelectBlocks->setText(QApplication::translate(TRANSLATION_CONTEXT, "Select &Blocks Widget"));
        actionWidgetSelectBlocks->setCheckable(true);

        actionWidgetSelectArea = new QAction(pMainWindow);
        actionWidgetSelectArea->setIcon(QIcon(QStringLiteral(":/editor/icons/MultiplePagesHH.png")));
        actionWidgetSelectArea->setText(QApplication::translate(TRANSLATION_CONTEXT, "Select &Area Widget"));
        actionWidgetSelectArea->setCheckable(true);

        actionWidgetPlaceBlocks = new QAction(pMainWindow);
        actionWidgetPlaceBlocks->setIcon(QIcon(QStringLiteral(":/editor/icons/BreakpointHS.png")));
        actionWidgetPlaceBlocks->setText(QApplication::translate(TRANSLATION_CONTEXT, "&Place Blocks Widget"));
        actionWidgetPlaceBlocks->setCheckable(true);

        actionWidgetDeleteBlocks = new QAction(pMainWindow);
        actionWidgetDeleteBlocks->setIcon(QIcon(QStringLiteral(":/editor/icons/DeleteHS.png")));
        actionWidgetDeleteBlocks->setText(QApplication::translate(TRANSLATION_CONTEXT, "&Delete Blocks Widget"));
        actionWidgetDeleteBlocks->setCheckable(true);

        actionWidgetLightManipulator = new QAction(pMainWindow);
        actionWidgetLightManipulator->setIcon(QIcon(QStringLiteral(":/editor/icons/Alerts.png")));
        actionWidgetLightManipulator->setText(QApplication::translate(TRANSLATION_CONTEXT, "&Light Manipulator Widget"));
        actionWidgetLightManipulator->setCheckable(true);
        
        menuMesh = new QMenu(pMainWindow);
        menuMesh->setTitle(QApplication::translate(TRANSLATION_CONTEXT, "&Mesh"));
        menuMesh->addAction(actionSaveMesh);
        menuMesh->addAction(actionSaveMeshAs);
        menuMesh->addSeparator();
        menuMesh->addAction(actionClose);

        menuEdit = new QMenu(pMainWindow);
        menuEdit->setTitle(QApplication::translate(TRANSLATION_CONTEXT, "&Edit"));
        menuEdit->addAction(actionEditUndo);
        menuEdit->addAction(actionEditRedo);

        menuCamera = new QMenu(pMainWindow);
        menuCamera->setTitle(QApplication::translate(TRANSLATION_CONTEXT, "&Camera"));
        menuCamera->addAction(actionCameraPerspective);
        menuCamera->addAction(actionCameraArcball);
        menuCamera->addAction(actionCameraIsometric);

        menuView = new QMenu(pMainWindow);
        menuView->setTitle(QApplication::translate(TRANSLATION_CONTEXT, "&View"));
        menuView->addAction(actionViewWireframe);
        menuView->addAction(actionViewUnlit);
        menuView->addAction(actionViewLit);
        menuView->addSeparator();
        menuView->addAction(actionViewFlagShadows);
        menuView->addAction(actionViewFlagWireframeOverlay);

        menuWidget = new QMenu(pMainWindow);
        menuWidget->setTitle(QApplication::translate(TRANSLATION_CONTEXT, "&Widget"));
        menuWidget->addAction(actionWidgetCamera);
        menuWidget->addAction(actionWidgetSelectBlocks);
        menuWidget->addAction(actionWidgetSelectArea);
        menuWidget->addAction(actionWidgetPlaceBlocks);
        menuWidget->addAction(actionWidgetDeleteBlocks);
        menuWidget->addAction(actionWidgetLightManipulator);

        menuHelp = new QMenu(pMainWindow);
        menuHelp->setTitle(QApplication::translate(TRANSLATION_CONTEXT, "&Help"));

        menubar = new QMenuBar(pMainWindow);
        menubar->addMenu(menuMesh);
        menubar->addMenu(menuEdit);
        menubar->addMenu(menuCamera);
        menubar->addMenu(menuView);
        menubar->addMenu(menuWidget);
        menubar->addMenu(menuHelp);

        activeLODComboBox = new QComboBox(pMainWindow);
        activeLODComboBox->setEditable(false);
        
        toolbar = new QToolBar(pMainWindow);
        toolbar->setIconSize(QSize(16, 16));
        toolbar->addAction(actionSaveMesh);
        toolbar->addSeparator();
        toolbar->addAction(actionEditUndo);
        toolbar->addAction(actionEditRedo);
        toolbar->addSeparator();
        toolbar->addWidget(activeLODComboBox);
        toolbar->addSeparator();
        toolbar->addAction(actionCameraPerspective);
        toolbar->addAction(actionCameraArcball);
        toolbar->addAction(actionCameraIsometric);
        toolbar->addSeparator();
        toolbar->addAction(actionViewWireframe);
        toolbar->addAction(actionViewUnlit);
        toolbar->addAction(actionViewLit);
        toolbar->addSeparator();
        toolbar->addAction(actionViewFlagShadows);
        toolbar->addAction(actionViewFlagWireframeOverlay);
        toolbar->addSeparator();
        toolbar->addAction(actionWidgetCamera);
        toolbar->addAction(actionWidgetSelectBlocks);
        toolbar->addAction(actionWidgetSelectArea);
        toolbar->addAction(actionWidgetPlaceBlocks);
        toolbar->addAction(actionWidgetDeleteBlocks);
        toolbar->addAction(actionWidgetLightManipulator);

        statusBar = new QStatusBar(pMainWindow);
        {
            statusBarMainText = new QLabel(statusBar);
            statusBar->addWidget(statusBarMainText, 1);

            statusBarMeshSize = new QLabel(statusBar);
            statusBar->addWidget(statusBarMeshSize);

            statusBarProgressBar = new QProgressBar(statusBar);
            statusBarProgressBar->setTextVisible(false);
            statusBarProgressBar->setFixedWidth(100);
            statusBarProgressBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            statusBarProgressBar->setRange(0, 1);
            statusBarProgressBar->setValue(1);
            statusBar->addWidget(statusBarProgressBar);

            statusBarFPS = new QLabel(statusBar);
            statusBar->addWidget(statusBarFPS);
        }

        swapChainWidget = new EditorRendererSwapChainWidget(pMainWindow);
        swapChainWidget->setFocusPolicy(Qt::FocusPolicy(Qt::TabFocus | Qt::ClickFocus));
        swapChainWidget->setMouseTracking(true);

        cameraDock = new QDockWidget(pMainWindow);
        cameraDock->setWindowTitle(QApplication::translate(TRANSLATION_CONTEXT, "Camera Settings"));
        cameraDock->hide();

        selectBlocksAreaDock = new QDockWidget(pMainWindow);
        selectBlocksAreaDock->setWindowTitle(QApplication::translate(TRANSLATION_CONTEXT, "Select Blocks/Area Settings"));
        selectBlocksAreaDock->hide();

        placeBlocksDock = new QDockWidget(pMainWindow);
        placeBlocksWidget = new QTabWidget(placeBlocksDock);
        placeBlocksWidget->setTabsClosable(false);
        placeBlocksWidget->setTabPosition(QTabWidget::South);
        placeBlocksWidget->setTabShape(QTabWidget::Rounded);
        {
            placeBlocksWidgetPaletteIconsWidget = new QWidget(placeBlocksWidget);
            //placeBlocksWidgetPaletteIconsWidget->setStyleSheet(QStringLiteral("background-color: black;"));
            {
                placeBlocksWidgetPaletteIconsFlowLayout = new FlowLayout(placeBlocksWidgetPaletteIconsWidget, 0, 0, 0);
                Y_memzero(placeBlocksWidgetPaletteIcons, sizeof(placeBlocksWidgetPaletteIcons));
            }
            placeBlocksWidgetPaletteIconsWidget->setLayout(placeBlocksWidgetPaletteIconsFlowLayout);
            placeBlocksWidget->addTab(placeBlocksWidgetPaletteIconsWidget, QApplication::translate(TRANSLATION_CONTEXT, "Icons"));

            placeBlocksWidgetPaletteList = new QListWidget(placeBlocksWidget);
            placeBlocksWidget->addTab(placeBlocksWidgetPaletteList, QApplication::translate(TRANSLATION_CONTEXT, "List"));
        }
        placeBlocksDock->setWindowTitle(QApplication::translate(TRANSLATION_CONTEXT, "Place Blocks Settings"));
        placeBlocksDock->setWidget(placeBlocksWidget);
        placeBlocksDock->hide();

        deleteBlocksDock = new QDockWidget(pMainWindow);
        deleteBlocksDock->setWindowTitle(QApplication::translate(TRANSLATION_CONTEXT, "Delete Blocks Settings"));
        deleteBlocksDock->hide();

        lightManipulatorDock = new QDockWidget(pMainWindow);
        lightManipulatorDock->setWindowTitle(QApplication::translate(TRANSLATION_CONTEXT, "Light Manipulator Settings"));
        lightManipulatorDock->hide();

        pMainWindow->setMenuBar(menubar);
        pMainWindow->addToolBar(toolbar);
        pMainWindow->setCentralWidget(swapChainWidget);
        pMainWindow->setStatusBar(statusBar);
        pMainWindow->addDockWidget(Qt::RightDockWidgetArea, cameraDock);
        pMainWindow->addDockWidget(Qt::RightDockWidgetArea, selectBlocksAreaDock);
        pMainWindow->addDockWidget(Qt::RightDockWidgetArea, placeBlocksDock);
        pMainWindow->addDockWidget(Qt::RightDockWidgetArea, deleteBlocksDock);
        pMainWindow->addDockWidget(Qt::RightDockWidgetArea, lightManipulatorDock);

        QMetaObject::connectSlotsByName(pMainWindow);
    }

    void UpdateUIForOpenMesh(const char *meshName)
    {
        SmallString title;
        title.Format("Block Mesh Editor - %s", meshName);
        mainWindow->setWindowTitle(ConvertStringToQString(title));
    }

    void UpdateUIForCameraMode(EDITOR_CAMERA_MODE cameraMode)
    {
        actionCameraPerspective->setChecked((cameraMode == EDITOR_CAMERA_MODE_PERSPECTIVE));
        actionCameraArcball->setChecked((cameraMode == EDITOR_CAMERA_MODE_ARCBALL));
        actionCameraIsometric->setChecked((cameraMode == EDITOR_CAMERA_MODE_ISOMETRIC));
    }

    void UpdateUIForRenderMode(EDITOR_RENDER_MODE renderMode)
    {
        actionViewWireframe->setChecked((renderMode == EDITOR_RENDER_MODE_WIREFRAME));
        actionViewUnlit->setChecked((renderMode == EDITOR_RENDER_MODE_FULLBRIGHT));
        actionViewLit->setChecked((renderMode == EDITOR_RENDER_MODE_LIT));
    }

    void UpdateUIForViewportFlags(uint32 viewportFlags)
    {
        actionViewFlagShadows->setChecked((viewportFlags & EDITOR_VIEWPORT_FLAG_ENABLE_SHADOWS) != 0);
        actionViewFlagWireframeOverlay->setChecked((viewportFlags & EDITOR_VIEWPORT_FLAG_WIREFRAME_OVERLAY) != 0);
    }

    void UpdateUIForWidget(EditorBlockMeshEditor::WIDGET widget)
    {
        (widget == EditorBlockMeshEditor::WIDGET_CAMERA) ? cameraDock->show() : cameraDock->hide();
        (widget == EditorBlockMeshEditor::WIDGET_SELECT_BLOCKS || widget == EditorBlockMeshEditor::WIDGET_SELECT_AREA) ? selectBlocksAreaDock->show() : selectBlocksAreaDock->hide();
        (widget == EditorBlockMeshEditor::WIDGET_PLACE_BLOCKS) ? placeBlocksDock->show() : placeBlocksDock->hide();
        (widget == EditorBlockMeshEditor::WIDGET_DELETE_BLOCKS) ? deleteBlocksDock->show() : deleteBlocksDock->hide();
        (widget == EditorBlockMeshEditor::WIDGET_LIGHT_MANIPULATOR) ? lightManipulatorDock->show() : lightManipulatorDock->hide();
        actionWidgetCamera->setChecked((widget == EditorBlockMeshEditor::WIDGET_CAMERA));
        actionWidgetSelectBlocks->setChecked((widget == EditorBlockMeshEditor::WIDGET_SELECT_BLOCKS));
        actionWidgetSelectArea->setChecked((widget == EditorBlockMeshEditor::WIDGET_SELECT_AREA));
        actionWidgetPlaceBlocks->setChecked((widget == EditorBlockMeshEditor::WIDGET_PLACE_BLOCKS));
        actionWidgetDeleteBlocks->setChecked((widget == EditorBlockMeshEditor::WIDGET_DELETE_BLOCKS));
        actionWidgetLightManipulator->setChecked((widget == EditorBlockMeshEditor::WIDGET_LIGHT_MANIPULATOR));
    }

    void UpdateUIForLODCount(uint32 newLODCount, bool lastIsImposter)
    {
        SmallString caption;

        activeLODComboBox->clear();
        for (uint32 i = 0; i < newLODCount; i++)
        {
            if (lastIsImposter && (i == newLODCount - 1))
                caption.Format("Imposter LOD");
            else
                caption.Format("LOD %u", i);

            activeLODComboBox->addItem(ConvertStringToQString(caption));
        }
    }

    void UpdateUIForCurrentLOD(uint32 currentLOD)
    {
        DebugAssert(currentLOD < (uint32)activeLODComboBox->count());
        activeLODComboBox->setCurrentIndex((int)currentLOD);
    }

    void UpdateUIAvailableBlockTypes(uint32 iconSize, const EditorBlockMeshEditor::AvailableBlockType *pAvailableBlockTypes, uint32 nAvailableBlockTypes)
    {
        // place blocks
        {
            // icons
            {
                // remove everything
                {
                    QLayoutItem *pLayoutItem;
                    while ((pLayoutItem = placeBlocksWidgetPaletteIconsFlowLayout->takeAt(0)) != NULL)
                        placeBlocksWidgetPaletteIconsFlowLayout->removeItem(pLayoutItem);

                    for (uint32 i = 0; i < BLOCK_MESH_MAX_BLOCK_TYPES; i++)
                    {
                        delete placeBlocksWidgetPaletteIcons[i];
                        placeBlocksWidgetPaletteIcons[i] = NULL;
                    }
                }

                for (uint32 i = 0; i < nAvailableBlockTypes; i++)
                {
                    QToolButton *toolButton = new QToolButton(placeBlocksWidgetPaletteIconsWidget);
                    toolButton->setFixedSize(iconSize + 2, iconSize + 2);
                    toolButton->setIconSize(QSize(iconSize, iconSize));
                    toolButton->setIcon(pAvailableBlockTypes[i].BlockTypeIcon);
                    toolButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                    toolButton->setToolTip(ConvertStringToQString(pAvailableBlockTypes[i].BlockName));
                    toolButton->setCheckable(true);
                    toolButton->setChecked(false);
                    toolButton->setAutoRaise(true);
                
                    placeBlocksWidgetPaletteIcons[i] = toolButton;
                    placeBlocksWidgetPaletteIconsFlowLayout->addWidget(toolButton);

                    QObject::connect(toolButton, SIGNAL(clicked(bool)), mainWindow, SLOT(OnPlaceBlockWidgetToolButtonClicked(bool)));
                }
            }

            // list
            {
                // remove everything
                placeBlocksWidgetPaletteList->clear();
                placeBlocksWidgetPaletteList->setIconSize(QSize(iconSize, iconSize));

                // add them
                for (uint32 i = 0; i < nAvailableBlockTypes; i++)
                    placeBlocksWidgetPaletteList->addItem(new QListWidgetItem(pAvailableBlockTypes[i].BlockTypeIcon, ConvertStringToQString(pAvailableBlockTypes[i].BlockName)));
            }
        }
        /*
        placeBlocksDockWidgetPlaceBlockTypeComboBox->clear();
        placeBlocksDockWidgetPlaceBlockTypeComboBox->setIconSize(QSize(iconSize, iconSize));

        for (uint32 i = 0; i < nAvailableBlockTypes; i++)
            placeBlocksDockWidgetPlaceBlockTypeComboBox->addItem(pAvailableBlockTypes[i].BlockTypeIcon, Editor::ConvertStringToQString(pAvailableBlockTypes[i].BlockName));
        */
    }

    void UpdateUISelectedPlaceBlockType(const EditorBlockMeshEditor::AvailableBlockType *pAvailableBlockTypes, uint32 nAvailableBlockTypes, uint32 selectedIndex, EditorBlockMeshEditor::WIDGET currentWidget)
    {
        for (uint32 i = 0; i < nAvailableBlockTypes; i++)
            placeBlocksWidgetPaletteIcons[i]->setChecked(false);

        placeBlocksWidgetPaletteIcons[selectedIndex]->setChecked(true);
        placeBlocksWidgetPaletteList->setCurrentRow(selectedIndex);

        if (currentWidget == EditorBlockMeshEditor::WIDGET_PLACE_BLOCKS)
        {
            SmallString message;
            message.Format("Placing block: %s", pAvailableBlockTypes[selectedIndex].BlockName.GetCharArray());
            statusBarMainText->setText(ConvertStringToQString(message));
        }
    }

    void UpdateUIForMeshSize(uint32 meshWidth, uint32 meshLength, uint32 meshHeight)
    {
        SmallString sizeString;
        StringConverter::SizeToHumanReadableString(sizeString, sizeof(BlockVolumeBlockType) * meshWidth * meshLength * meshHeight);
        
        SmallString message;
        message.Format("Mesh Size: %u x %u x %u (%s)", meshWidth, meshLength, meshHeight, sizeString.GetCharArray());

        statusBarMeshSize->setText(ConvertStringToQString(message));
    }

    void UpdateUIForFPS(float fps)
    {
        SmallString message;
        message.Format("FPS: %.2f", fps);
        statusBarFPS->setText(ConvertStringToQString(message));
    }
};

