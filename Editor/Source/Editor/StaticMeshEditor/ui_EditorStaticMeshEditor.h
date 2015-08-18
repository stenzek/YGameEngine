#pragma once
#include "Editor/EditorRendererSwapChainWidget.h"
#include "Editor/ToolMenuWidget.h"
#include "Editor/EditorVectorEditWidget.h"

class Ui_EditorStaticMeshEditor
{
public:
    QMainWindow *mainWindow;

    // actions
    QAction *actionOpenMesh;
    QAction *actionSaveMesh;
    QAction *actionSaveMeshAs;
    QAction *actionClose;
    QAction *actionCameraPerspective;
    QAction *actionCameraArcball;
    QAction *actionCameraIsometric;
    QAction *actionViewWireframe;
    QAction *actionViewUnlit;
    QAction *actionViewLit;
    QAction *actionViewFlagShadows;
    QAction *actionViewFlagWireframeOverlay;
    QAction *actionToolInformation;
    QAction *actionToolOperations;
    QAction *actionToolMaterials;
    QAction *actionToolLOD;
    QAction *actionToolCollision;
    QAction *actionToolLightManipulator;

    // menus
    QMenu *menuMesh;
    QMenu *menuCamera;
    QMenu *menuView;
    QMenu *menuTools;
    QMenu *menuHelp;

    // widgets
    QMenuBar *menubar;
    QToolBar *toolbar;
    QStatusBar *statusBar;
    QLabel *statusBarStatusLabel;

    // left pane
    EditorRendererSwapChainWidget *swapChainWidget;

    // right pane
    QDockWidget *rightDockWidget;
    QWidget *rightDockContainer;

    // mode panel
    ToolMenuWidget *rightToolMenu;

    // stuff for each mode
    QWidget *toolContainer;
    QScrollArea *toolContainerScrollArea;

    // information
    QWidget *toolInformationContainer;
    QLabel *toolInformationMinBounds;
    QLabel *toolInformationMaxBounds;
    QLabel *toolInformationVertexCount;
    QLabel *toolInformationTriangleCount;
    QLabel *toolInformationMaterialCount;
    QLabel *toolInformationBatchCount;
    QCheckBox *toolInformationEnableTextureCoordinates;
    QCheckBox *toolInformationEnableVertexColors;

    // operations
    QWidget *toolOperationsContainer;
    QPushButton *toolOperationsGenerateTangents;
    QPushButton *toolOperationsCenterMesh;
    QPushButton *toolOperationsRemoveUnusedVertices;
    QPushButton *toolOperationsRemoveUnusedTriangles;

    // materials
    QWidget *toolMaterialsContainer;

    // lods
    QWidget *toolLODContainer;

    // collision
    QWidget *toolCollisionContainer;
    QLabel *toolCollisionInformation;
    QRadioButton *toolCollisionGenerateBox;
    QRadioButton *toolCollisionGenerateSphere;
    QRadioButton *toolCollisionGenerateTriangleMesh;
    QRadioButton *toolCollisionGenerateConvexHull;
    QPushButton *toolCollisionGenerate;

    // light manipulator
    QWidget *toolLightManipulatorContainer;

    void CreateUI(QMainWindow *pMainWindow)
    {
        mainWindow = pMainWindow;
        mainWindow->resize(800, 600);

        actionOpenMesh = new QAction(pMainWindow);
        actionOpenMesh->setIcon(QIcon(QStringLiteral(":/editor/icons/openHS.png")));
        actionOpenMesh->setText(mainWindow->tr("&Open Mesh"));

        actionSaveMesh = new QAction(pMainWindow);
        actionSaveMesh->setIcon(QIcon(QStringLiteral(":/editor/icons/saveHS.png")));
        actionSaveMesh->setText(mainWindow->tr("&Save Mesh"));

        actionSaveMeshAs = new QAction(pMainWindow);
        actionSaveMeshAs->setText(mainWindow->tr("Save Mesh &As"));

        actionClose = new QAction(pMainWindow);
        actionClose->setText(mainWindow->tr("&Close"));

        actionCameraPerspective = new QAction(pMainWindow);
        actionCameraPerspective->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_View_Perspective.png")));
        actionCameraPerspective->setText(mainWindow->tr("&Perspective Camera"));
        actionCameraPerspective->setCheckable(true);

        actionCameraArcball = new QAction(pMainWindow);
        actionCameraArcball->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_View_Side.png")));
        actionCameraArcball->setText(mainWindow->tr("&Arcball Camera"));
        actionCameraArcball->setCheckable(true);

        actionCameraIsometric = new QAction(pMainWindow);
        actionCameraIsometric->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_View_Isometric.png")));
        actionCameraIsometric->setText(mainWindow->tr("&Isometric Camera"));
        actionCameraIsometric->setCheckable(true);

        actionViewWireframe = new QAction(pMainWindow);
        actionViewWireframe->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderMode_Wireframe.png")));
        actionViewWireframe->setText(mainWindow->tr("&Wireframe View"));
        actionViewWireframe->setCheckable(true);

        actionViewUnlit = new QAction(pMainWindow);
        actionViewUnlit->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderMode_Unlit.png")));
        actionViewUnlit->setText(mainWindow->tr("&Unlit View"));
        actionViewUnlit->setCheckable(true);

        actionViewLit = new QAction(pMainWindow);
        actionViewLit->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderMode_Lit.png")));
        actionViewLit->setText(mainWindow->tr("&Lit View"));
        actionViewLit->setCheckable(true);

        actionViewFlagShadows = new QAction(pMainWindow);
        actionViewFlagShadows->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderFlag_Shadows.png")));
        actionViewFlagShadows->setText(mainWindow->tr("Enable &Shadows"));
        actionViewFlagShadows->setCheckable(true);

        actionViewFlagWireframeOverlay = new QAction(pMainWindow);
        actionViewFlagWireframeOverlay->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderFlag_WireframeOverlay.png")));
        actionViewFlagWireframeOverlay->setText(mainWindow->tr("Enable Wireframe &Overlay"));
        actionViewFlagWireframeOverlay->setCheckable(true);

        actionToolInformation = new QAction(pMainWindow);
        actionToolInformation->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
        actionToolInformation->setText(pMainWindow->tr("Mesh Information"));
        actionToolInformation->setCheckable(true);

        actionToolOperations = new QAction(pMainWindow);
        actionToolOperations->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
        actionToolOperations->setText(pMainWindow->tr("Operations"));
        actionToolOperations->setCheckable(true);

        actionToolMaterials = new QAction(pMainWindow);
        actionToolMaterials->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
        actionToolMaterials->setText(pMainWindow->tr("Materials"));
        actionToolMaterials->setCheckable(true);

        actionToolLOD = new QAction(pMainWindow);
        actionToolLOD->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
        actionToolLOD->setText(pMainWindow->tr("LOD Generator"));
        actionToolLOD->setCheckable(true);

        actionToolCollision = new QAction(pMainWindow);
        actionToolCollision->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
        actionToolCollision->setText(pMainWindow->tr("Collision Mesh"));
        actionToolCollision->setCheckable(true);

        actionToolLightManipulator = new QAction(pMainWindow);
        actionToolLightManipulator->setIcon(QIcon(QStringLiteral(":/editor/icons/Alerts.png")));
        actionToolLightManipulator->setText(pMainWindow->tr("Light Manipulator"));
        actionToolLightManipulator->setCheckable(true);

        menuMesh = new QMenu(pMainWindow);
        menuMesh->setTitle(mainWindow->tr("&Mesh"));
        menuMesh->addAction(actionOpenMesh);
        menuMesh->addAction(actionSaveMesh);
        menuMesh->addAction(actionSaveMeshAs);
        menuMesh->addSeparator();
        menuMesh->addAction(actionClose);

        menuCamera = new QMenu(pMainWindow);
        menuCamera->setTitle(mainWindow->tr("&Camera"));
        menuCamera->addAction(actionCameraPerspective);
        menuCamera->addAction(actionCameraArcball);
        menuCamera->addAction(actionCameraIsometric);

        menuView = new QMenu(pMainWindow);
        menuView->setTitle(mainWindow->tr("&View"));
        menuView->addAction(actionViewWireframe);
        menuView->addAction(actionViewUnlit);
        menuView->addAction(actionViewLit);
        menuView->addSeparator();
        menuView->addAction(actionViewFlagShadows);
        menuView->addAction(actionViewFlagWireframeOverlay);

        menuTools = new QMenu(pMainWindow);
        menuTools->setTitle(mainWindow->tr("&Tools"));
        menuTools->addAction(actionToolInformation);
        menuTools->addAction(actionToolOperations);
        menuTools->addAction(actionToolMaterials);
        menuTools->addAction(actionToolLOD);
        menuTools->addAction(actionToolCollision);
        menuTools->addAction(actionToolLightManipulator);

        menuHelp = new QMenu(pMainWindow);
        menuHelp->setTitle(mainWindow->tr("&Help"));

        menubar = new QMenuBar(pMainWindow);
        menubar->addMenu(menuMesh);
        menubar->addMenu(menuCamera);
        menubar->addMenu(menuView);
        menubar->addMenu(menuTools);
        menubar->addMenu(menuHelp);
        pMainWindow->setMenuBar(menubar);

        toolbar = new QToolBar(pMainWindow);
        toolbar->setIconSize(QSize(16, 16));
        toolbar->addAction(actionOpenMesh);
        toolbar->addAction(actionSaveMesh);
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
        toolbar->addAction(actionToolInformation);
        toolbar->addAction(actionToolOperations);
        toolbar->addAction(actionToolMaterials);
        toolbar->addAction(actionToolLOD);
        toolbar->addAction(actionToolCollision);
        toolbar->addAction(actionToolLightManipulator);
        pMainWindow->addToolBar(toolbar);

        statusBar = new QStatusBar(pMainWindow);
        statusBarStatusLabel = new QLabel(statusBar);
        statusBar->addWidget(statusBarStatusLabel, 1);
        pMainWindow->setStatusBar(statusBar);

        // left pane
        swapChainWidget = new EditorRendererSwapChainWidget(pMainWindow);
        swapChainWidget->setFocusPolicy(Qt::FocusPolicy(Qt::TabFocus | Qt::ClickFocus));
        swapChainWidget->setMouseTracking(true);
        pMainWindow->setCentralWidget(swapChainWidget);

        // right pane container
        rightDockWidget = new QDockWidget(pMainWindow);
        rightDockWidget->setWindowTitle(pMainWindow->tr("Toolbox"));
        rightDockWidget->setMinimumSize(320, 400);
        rightDockContainer = new QWidget(rightDockWidget);
        {
            QVBoxLayout *rightDockContainerLayout = new QVBoxLayout(rightDockContainer);
            rightDockContainerLayout->setContentsMargins(0, 0, 0, 0);

            // mode toolbar
            {
                //rightToolMenu = new QToolBar(rightDockContainer);
                //rightToolBar->setMovable(false);
                //rightToolBar->setFloatable(false);
                //rightToolBar->setOrientation(Qt::Vertical);
                //rightToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                rightToolMenu = new ToolMenuWidget(rightDockContainer);
                rightToolMenu->addAction(actionToolInformation);
                rightToolMenu->addAction(actionToolOperations);
                rightToolMenu->addAction(actionToolMaterials);
                rightToolMenu->addAction(actionToolLOD);
                rightToolMenu->addAction(actionToolCollision);
                rightToolMenu->addAction(actionToolLightManipulator);
                rightToolMenu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                rightDockContainerLayout->addWidget(rightToolMenu);
            }

            // create scroll area for the container
            toolContainerScrollArea = new QScrollArea(rightDockContainer);
            toolContainerScrollArea->setBackgroundRole(QPalette::Background);
            toolContainerScrollArea->setFrameStyle(0);
            toolContainerScrollArea->setWidgetResizable(true);

            // tool container
            toolContainer = new QWidget(toolContainerScrollArea);
            {
                QVBoxLayout *toolContainerLayout = new QVBoxLayout(toolContainer);
                toolContainerLayout->setContentsMargins(0, 0, 0, 0);
                toolContainerLayout->setMargin(0);
                toolContainerLayout->setSpacing(0);

                // information tool
                {
                    toolInformationContainer = new QWidget(toolContainer);
                    {
                        QFormLayout *formLayout = new QFormLayout(toolInformationContainer);
                        //formLayout->setContentsMargins(2, 2, 2, 2);
                        //formLayout->setMargin(0);
                        //formLayout->setSpacing(0);

                        toolInformationMinBounds = new QLabel(toolInformationContainer);
                        formLayout->addRow(pMainWindow->tr("Min Bounds: "), toolInformationMinBounds);

                        toolInformationMaxBounds = new QLabel(toolInformationContainer);
                        formLayout->addRow(pMainWindow->tr("Max Bounds: "), toolInformationMaxBounds);

                        toolInformationVertexCount = new QLabel(toolInformationContainer);
                        formLayout->addRow(pMainWindow->tr("Vertex Count: "), toolInformationVertexCount);

                        toolInformationTriangleCount = new QLabel(toolInformationContainer);
                        formLayout->addRow(pMainWindow->tr("Triangle Count: "), toolInformationTriangleCount);

                        toolInformationMaterialCount = new QLabel(toolInformationContainer);
                        formLayout->addRow(pMainWindow->tr("Material Count: "), toolInformationMaterialCount);

                        toolInformationBatchCount = new QLabel(toolInformationContainer);
                        formLayout->addRow(pMainWindow->tr("Batch Count: "), toolInformationBatchCount);

                        toolInformationEnableTextureCoordinates = new QCheckBox(pMainWindow->tr("Enable Texture Coordinates"), toolInformationContainer);
                        formLayout->addRow(toolInformationEnableTextureCoordinates);

                        toolInformationEnableVertexColors = new QCheckBox(pMainWindow->tr("Enable Vertex Colors"), toolInformationContainer);
                        formLayout->addRow(toolInformationEnableVertexColors);

                        toolInformationContainer->setLayout(formLayout);
                    }
                    toolInformationContainer->hide();
                    toolContainerLayout->addWidget(toolInformationContainer);
                }

                // operations tool
                {
                    toolOperationsContainer = new QWidget(toolContainer);
                    {
                        QVBoxLayout *boxLayout = new QVBoxLayout(toolOperationsContainer);

                        toolOperationsGenerateTangents = new QPushButton(pMainWindow->tr("Generate Tangents"), toolOperationsContainer);
                        boxLayout->addWidget(toolOperationsGenerateTangents);

                        toolOperationsCenterMesh = new QPushButton(pMainWindow->tr("Center Mesh..."), toolOperationsContainer);
                        boxLayout->addWidget(toolOperationsCenterMesh);

                        toolOperationsRemoveUnusedVertices = new QPushButton(pMainWindow->tr("Remove Unused Vertices"), toolOperationsContainer);
                        boxLayout->addWidget(toolOperationsRemoveUnusedVertices);

                        toolOperationsRemoveUnusedTriangles = new QPushButton(pMainWindow->tr("Remove Unused Triangles"), toolOperationsContainer);
                        boxLayout->addWidget(toolOperationsRemoveUnusedTriangles);

                        toolOperationsContainer->setLayout(boxLayout);
                    }
                    toolOperationsContainer->hide();
                    toolContainerLayout->addWidget(toolOperationsContainer);
                }

                // materials tool
                {
                    toolMaterialsContainer = new QWidget(toolContainer);
                    toolMaterialsContainer->hide();
                    toolContainerLayout->addWidget(toolMaterialsContainer);
                }

                // lods tool
                {
                    toolLODContainer = new QWidget(toolContainer);
                    toolLODContainer->hide();
                    toolContainerLayout->addWidget(toolLODContainer);
                }

                // collision tool
                {
                    toolCollisionContainer = new QWidget(toolContainer);
                    {
                        QVBoxLayout *boxLayout = new QVBoxLayout(toolCollisionContainer);

                        toolCollisionInformation = new QLabel(toolCollisionContainer);
                        boxLayout->addWidget(toolCollisionInformation);

                        {
                            QGroupBox *groupBox = new QGroupBox(pMainWindow->tr("Build Shape"), toolCollisionContainer);
                            QVBoxLayout *groupBoxLayout = new QVBoxLayout(groupBox);

                            toolCollisionGenerateBox = new QRadioButton(pMainWindow->tr("Box"), groupBox);
                            groupBoxLayout->addWidget(toolCollisionGenerateBox);

                            toolCollisionGenerateSphere = new QRadioButton(pMainWindow->tr("Sphere"), groupBox);
                            groupBoxLayout->addWidget(toolCollisionGenerateSphere);

                            toolCollisionGenerateTriangleMesh = new QRadioButton(pMainWindow->tr("Triangle Mesh"), groupBox);
                            groupBoxLayout->addWidget(toolCollisionGenerateTriangleMesh);

                            toolCollisionGenerateConvexHull = new QRadioButton(pMainWindow->tr("Convex Hull"), groupBox);
                            groupBoxLayout->addWidget(toolCollisionGenerateConvexHull);

                            toolCollisionGenerate = new QPushButton(pMainWindow->tr("Rebuild..."), groupBox);
                            groupBoxLayout->addWidget(toolCollisionGenerate);

                            boxLayout->addWidget(groupBox);
                        }

                        toolCollisionContainer->setLayout(boxLayout);
                    }
                    toolCollisionContainer->hide();
                    toolContainerLayout->addWidget(toolCollisionContainer);
                }
                
                // light manipulator tool
                {
                    toolLightManipulatorContainer = new QWidget(toolContainer);
                    {

                    }
                    toolLightManipulatorContainer->hide();
                    toolContainerLayout->addWidget(toolLightManipulatorContainer);
                }

                toolContainerLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Expanding));
                toolContainer->setLayout(toolContainerLayout);
            }
            toolContainerScrollArea->setWidget(toolContainer);

            // complete container setup
            rightDockContainerLayout->addWidget(toolContainerScrollArea, 1);
            rightDockContainer->setLayout(rightDockContainerLayout);
        }
        rightDockWidget->setWidget(rightDockContainer);
        pMainWindow->addDockWidget(Qt::RightDockWidgetArea, rightDockWidget);
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

    void UpdateUIForTool(EditorStaticMeshEditor::Tool tool)
    {
        (tool == EditorStaticMeshEditor::Tool_Information) ? toolInformationContainer->show() : toolInformationContainer->hide();
        (tool == EditorStaticMeshEditor::Tool_Operations) ? toolOperationsContainer->show() : toolOperationsContainer->hide();
        (tool == EditorStaticMeshEditor::Tool_Materials) ? toolMaterialsContainer->show() : toolMaterialsContainer->hide();
        (tool == EditorStaticMeshEditor::Tool_LOD) ? toolLODContainer->show() : toolLODContainer->hide();
        (tool == EditorStaticMeshEditor::Tool_Collision) ? toolCollisionContainer->show() : toolCollisionContainer->hide();
        (tool == EditorStaticMeshEditor::Tool_LightManipulator) ? toolLightManipulatorContainer->show() : toolLightManipulatorContainer->hide();
        actionToolInformation->setChecked((tool == EditorStaticMeshEditor::Tool_Information));
        actionToolOperations->setChecked((tool == EditorStaticMeshEditor::Tool_Operations));
        actionToolMaterials->setChecked((tool == EditorStaticMeshEditor::Tool_Materials));
        actionToolLOD->setChecked((tool == EditorStaticMeshEditor::Tool_LOD));
        actionToolCollision->setChecked((tool == EditorStaticMeshEditor::Tool_Collision));
        actionToolLightManipulator->setChecked((tool == EditorStaticMeshEditor::Tool_LightManipulator));
    }
};
