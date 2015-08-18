#pragma once
#include "Editor/EditorRendererSwapChainWidget.h"
#include "Editor/ToolMenuWidget.h"
#include "Editor/EditorVectorEditWidget.h"

class Ui_EditorSkeletalAnimationEditor
{
public:
    QMainWindow *mainWindow;

    // actions
    QAction *actionOpen;
    QAction *actionSave;
    QAction *actionSaveAs;
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
    QAction *actionToolBoneTrack;
    QAction *actionToolRootMotion;
    QAction *actionToolClipper;
    QAction *actionToolLightManipulator;

    // menus
    QMenu *menuAnimation;
    QMenu *menuCamera;
    QMenu *menuView;
    QMenu *menuTools;
    QMenu *menuHelp;

    // widgets
    QMenuBar *menubar;
    QToolBar *toolbar;
    QStatusBar *statusBar;

    // left pane
    EditorRendererSwapChainWidget *swapChainWidget;

    // scrubber
    QDockWidget *scrubberDockWidget;
    QSlider *scrubberSlider;

    // hierarchy
    QDockWidget *hierarchyDockWidget;
    QTreeWidget *hierarchyTreeWidget;

    // toolbox
    QDockWidget *toolBoxDockWidget;
    QWidget *toolBoxContainer;

    // mode panel
    ToolMenuWidget *rightToolMenu;

    // stuff for each mode
    QWidget *toolContainer;
    QScrollArea *toolContainerScrollArea;

    // information
    QWidget *toolInformationContainer;
    QLabel *toolInformationSkeletonName;
    QLabel *toolInformationFrameCount;
    QLabel *toolInformationDuration;
    QLabel *toolInformationBoneTrackCount;
    QLabel *toolInformationRootMotion;
    QLabel *toolInformationPreviewMesh;

    // bone tracks
    QWidget *toolBoneTrackContainer;
    QListWidget *toolBoneTrackKeyframeList;
    EditorVectorEditWidget *toolBoneTrackTransformPosition;
    EditorVectorEditWidget *toolBoneTrackTransformRotation;
    EditorVectorEditWidget *toolBoneTrackTransformScale;

    // root motion
    QWidget *toolRootMotionContainer;

    // import
    QWidget *toolClipperContainer;
    QSpinBox *toolClipperStartFrameNumber;
    QPushButton *toolClipperSetStartFrameNumber;
    QSpinBox *toolClipperEndFrameNumber;
    QPushButton *toolClipperSetEndFrameNumber;
    QPushButton *toolClipperClip;

    // light manipulator
    QWidget *toolLightManipulatorContainer;

    void CreateUI(QMainWindow *pMainWindow)
    {
        mainWindow = pMainWindow;
        mainWindow->resize(800, 600);

        actionOpen = new QAction(pMainWindow);
        actionOpen->setIcon(QIcon(QStringLiteral(":/editor/icons/openHS.png")));
        actionOpen->setText(mainWindow->tr("&Open"));

        actionSave = new QAction(pMainWindow);
        actionSave->setIcon(QIcon(QStringLiteral(":/editor/icons/saveHS.png")));
        actionSave->setText(mainWindow->tr("&Save"));

        actionSaveAs = new QAction(pMainWindow);
        actionSaveAs->setText(mainWindow->tr("Save &As"));

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
        actionToolInformation->setText(pMainWindow->tr("Information"));
        actionToolInformation->setCheckable(true);

        actionToolBoneTrack = new QAction(pMainWindow);
        actionToolBoneTrack->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
        actionToolBoneTrack->setText(pMainWindow->tr("Bone Tracks"));
        actionToolBoneTrack->setCheckable(true);

        actionToolRootMotion = new QAction(pMainWindow);
        actionToolRootMotion->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
        actionToolRootMotion->setText(pMainWindow->tr("Root Motion"));
        actionToolRootMotion->setCheckable(true);

        actionToolClipper = new QAction(pMainWindow);
        actionToolClipper->setIcon(QIcon(QStringLiteral(":/editor/icons/EditCodeHS.png")));
        actionToolClipper->setText(pMainWindow->tr("Clipping"));
        actionToolClipper->setCheckable(true);

        actionToolLightManipulator = new QAction(pMainWindow);
        actionToolLightManipulator->setIcon(QIcon(QStringLiteral(":/editor/icons/Alerts.png")));
        actionToolLightManipulator->setText(pMainWindow->tr("Light Manipulator"));
        actionToolLightManipulator->setCheckable(true);

        menuAnimation = new QMenu(pMainWindow);
        menuAnimation->setTitle(mainWindow->tr("&Animation"));
        menuAnimation->addAction(actionOpen);
        menuAnimation->addAction(actionSave);
        menuAnimation->addAction(actionSaveAs);
        menuAnimation->addSeparator();
        menuAnimation->addAction(actionClose);

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
        menuTools->addAction(actionToolBoneTrack);
        menuTools->addAction(actionToolRootMotion);
        menuTools->addAction(actionToolClipper);
        menuTools->addAction(actionToolLightManipulator);

        menuHelp = new QMenu(pMainWindow);
        menuHelp->setTitle(mainWindow->tr("&Help"));

        menubar = new QMenuBar(pMainWindow);
        menubar->addMenu(menuAnimation);
        menubar->addMenu(menuCamera);
        menubar->addMenu(menuView);
        menubar->addMenu(menuTools);
        menubar->addMenu(menuHelp);
        pMainWindow->setMenuBar(menubar);

        toolbar = new QToolBar(pMainWindow);
        toolbar->setIconSize(QSize(16, 16));
        toolbar->addAction(actionOpen);
        toolbar->addAction(actionSave);
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
        toolbar->addAction(actionToolBoneTrack);
        toolbar->addAction(actionToolRootMotion);
        toolbar->addAction(actionToolClipper);
        toolbar->addAction(actionToolLightManipulator);
        pMainWindow->addToolBar(toolbar);

        statusBar = new QStatusBar(pMainWindow);
        pMainWindow->setStatusBar(statusBar);

        // left pane
        swapChainWidget = new EditorRendererSwapChainWidget(pMainWindow);
        swapChainWidget->setFocusPolicy(Qt::FocusPolicy(Qt::TabFocus | Qt::ClickFocus));
        swapChainWidget->setMouseTracking(true);
        pMainWindow->setCentralWidget(swapChainWidget);

        // scrubber
        scrubberDockWidget = new QDockWidget(pMainWindow);
        scrubberDockWidget->setFeatures(0);
        scrubberSlider = new QSlider(Qt::Horizontal, scrubberDockWidget);
        scrubberSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        scrubberSlider->setFixedHeight(32);
        scrubberSlider->setMinimum(0);
        scrubberSlider->setMaximum(16);
        scrubberSlider->setTickInterval(1);
        scrubberSlider->setTickPosition(QSlider::TicksBothSides);
        scrubberDockWidget->setWidget(scrubberSlider);
        pMainWindow->addDockWidget(Qt::BottomDockWidgetArea, scrubberDockWidget);

        // heirarchy
        hierarchyDockWidget = new QDockWidget(pMainWindow);
        hierarchyDockWidget->setWindowTitle("Hierarchy");
        hierarchyTreeWidget = new QTreeWidget(hierarchyDockWidget);
        hierarchyTreeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        hierarchyDockWidget->setWidget(hierarchyTreeWidget);
        pMainWindow->addDockWidget(Qt::RightDockWidgetArea, hierarchyDockWidget);

        // right pane container
        toolBoxDockWidget = new QDockWidget(pMainWindow);
        toolBoxDockWidget->setWindowTitle(pMainWindow->tr("Toolbox"));
        toolBoxDockWidget->setMinimumSize(320, 400);
        toolBoxContainer = new QWidget(toolBoxDockWidget);
        {
            QVBoxLayout *rightDockContainerLayout = new QVBoxLayout(toolBoxContainer);
            rightDockContainerLayout->setContentsMargins(0, 0, 0, 0);

            // mode toolbar
            {
                rightToolMenu = new ToolMenuWidget(toolBoxContainer);
                rightToolMenu->addAction(actionToolInformation);
                rightToolMenu->addAction(actionToolBoneTrack);
                rightToolMenu->addAction(actionToolRootMotion);
                rightToolMenu->addAction(actionToolClipper);
                rightToolMenu->addAction(actionToolLightManipulator);
                rightToolMenu->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                rightDockContainerLayout->addWidget(rightToolMenu);
            }

            // create scroll area for the container
            toolContainerScrollArea = new QScrollArea(toolBoxContainer);
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

                        toolInformationSkeletonName = new QLabel(toolInformationContainer);
                        formLayout->addRow(pMainWindow->tr("Skeleton Name: "), toolInformationSkeletonName);

                        toolInformationFrameCount = new QLabel(toolInformationContainer);
                        formLayout->addRow(pMainWindow->tr("Frame Count: "), toolInformationFrameCount);

                        toolInformationDuration = new QLabel(toolInformationContainer);
                        formLayout->addRow(pMainWindow->tr("Duration: "), toolInformationDuration);

                        toolInformationBoneTrackCount = new QLabel(toolInformationContainer);
                        formLayout->addRow(pMainWindow->tr("Bone Track Count: "), toolInformationBoneTrackCount);

                        toolInformationRootMotion = new QLabel(toolInformationContainer);
                        formLayout->addRow(pMainWindow->tr("Root Motion: "), toolInformationRootMotion);

                        toolInformationPreviewMesh = new QLabel(toolInformationContainer);
                        toolInformationPreviewMesh->setTextFormat(Qt::RichText);
                        formLayout->addRow(pMainWindow->tr("Preview Mesh: "), toolInformationPreviewMesh);

                        toolInformationContainer->setLayout(formLayout);
                    }
                    toolInformationContainer->hide();
                    toolContainerLayout->addWidget(toolInformationContainer);
                }

                // operations tool
                {
                    toolBoneTrackContainer = new QWidget(toolContainer);
                    {
                        QVBoxLayout *boxLayout = new QVBoxLayout(toolBoneTrackContainer);

                        toolBoneTrackKeyframeList = new QListWidget(toolBoneTrackContainer);
                        boxLayout->addWidget(toolBoneTrackKeyframeList, 1);

                        QGroupBox *keyframeInfoGroupBox = new QGroupBox(pMainWindow->tr("Bone Transform"), toolBoneTrackContainer);
                        {
                            QFormLayout *keyframeInfoFormLayout = new QFormLayout(keyframeInfoGroupBox);

                            toolBoneTrackTransformPosition = new EditorVectorEditWidget(keyframeInfoGroupBox);
                            keyframeInfoFormLayout->addRow(pMainWindow->tr("Position: "), toolBoneTrackTransformPosition);

                            toolBoneTrackTransformRotation = new EditorVectorEditWidget(keyframeInfoGroupBox);
                            keyframeInfoFormLayout->addRow(pMainWindow->tr("Rotation: "), toolBoneTrackTransformRotation);

                            toolBoneTrackTransformScale = new EditorVectorEditWidget(keyframeInfoGroupBox);
                            keyframeInfoFormLayout->addRow(pMainWindow->tr("Scale: "), toolBoneTrackTransformScale);

                            keyframeInfoGroupBox->setLayout(keyframeInfoFormLayout);
                        }
                        boxLayout->addWidget(keyframeInfoGroupBox);

                        toolBoneTrackContainer->setLayout(boxLayout);
                    }
                    toolBoneTrackContainer->hide();
                    toolContainerLayout->addWidget(toolBoneTrackContainer);
                }

                // root motion tool
                {
                    toolRootMotionContainer = new QWidget(toolContainer);
                    toolRootMotionContainer->hide();
                    toolContainerLayout->addWidget(toolRootMotionContainer);
                }

                // clipping tool
                {
                    toolClipperContainer = new QWidget(toolContainer);
                    {
                        QFormLayout *formLayout = new QFormLayout(toolClipperContainer);
                        QHBoxLayout *innerBoxLayout;

                        innerBoxLayout = new QHBoxLayout(toolClipperContainer);
                        toolClipperStartFrameNumber = new QSpinBox(toolClipperContainer);
                        innerBoxLayout->addWidget(toolClipperStartFrameNumber, 1);
                        toolClipperSetStartFrameNumber = new QPushButton(pMainWindow->tr("Set"), toolClipperContainer);
                        innerBoxLayout->addWidget(toolClipperSetStartFrameNumber);
                        formLayout->addRow(pMainWindow->tr("Start Frame: "), innerBoxLayout);

                        innerBoxLayout = new QHBoxLayout(toolClipperContainer);
                        toolClipperEndFrameNumber = new QSpinBox(toolClipperContainer);
                        innerBoxLayout->addWidget(toolClipperEndFrameNumber, 1);
                        toolClipperSetEndFrameNumber = new QPushButton(pMainWindow->tr("Set"), toolClipperContainer);
                        innerBoxLayout->addWidget(toolClipperSetEndFrameNumber);
                        formLayout->addRow(pMainWindow->tr("End Frame: "), innerBoxLayout);

                        toolClipperClip = new QPushButton(pMainWindow->tr("Clip"), toolClipperContainer);
                        formLayout->addRow(toolClipperClip);
                    
                        toolClipperContainer->setLayout(formLayout);
                    }

                    toolClipperContainer->hide();
                    toolContainerLayout->addWidget(toolClipperContainer);
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
            toolBoxContainer->setLayout(rightDockContainerLayout);
        }
        toolBoxDockWidget->setWidget(toolBoxContainer);
        pMainWindow->addDockWidget(Qt::RightDockWidgetArea, toolBoxDockWidget);
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

    void UpdateUIForTool(EditorSkeletalAnimationEditor::Tool tool)
    {
        (tool == EditorSkeletalAnimationEditor::Tool_Information) ? toolInformationContainer->show() : toolInformationContainer->hide();
        (tool == EditorSkeletalAnimationEditor::Tool_BoneTrack) ? toolBoneTrackContainer->show() : toolBoneTrackContainer->hide();
        (tool == EditorSkeletalAnimationEditor::Tool_RootMotion) ? toolRootMotionContainer->show() : toolRootMotionContainer->hide();
        (tool == EditorSkeletalAnimationEditor::Tool_Clipper) ? toolClipperContainer->show() : toolClipperContainer->hide();
        (tool == EditorSkeletalAnimationEditor::Tool_LightManipulator) ? toolLightManipulatorContainer->show() : toolLightManipulatorContainer->hide();
        actionToolInformation->setChecked((tool == EditorSkeletalAnimationEditor::Tool_Information));
        actionToolBoneTrack->setChecked((tool == EditorSkeletalAnimationEditor::Tool_BoneTrack));
        actionToolRootMotion->setChecked((tool == EditorSkeletalAnimationEditor::Tool_RootMotion));
        actionToolClipper->setChecked((tool == EditorSkeletalAnimationEditor::Tool_Clipper));
        actionToolLightManipulator->setChecked((tool == EditorSkeletalAnimationEditor::Tool_LightManipulator));
    }
};
