#pragma once

class Ui_EditorMapViewport
{
public:
    QGridLayout *gridLayout;

    // toolbar
    QToolButton *toolbarMoreOptions;
    QToolButton *toolbarRealTime;
    QToolButton *toolbarGameView;
    QToolButton *toolbarCameraFront;
    QToolButton *toolbarCameraSide;
    QToolButton *toolbarCameraTop;
    QToolButton *toolbarCameraIsometric;
    QToolButton *toolbarCameraPerspective;
    QToolButton *toolbarViewWireframe;
    QToolButton *toolbarViewUnlit;
    QToolButton *toolbarViewLit;
    QToolButton *toolbarViewLightingOnly;
    QToolButton *toolbarFlagShadows;
    QToolButton *toolbarFlagWireframeOverlay;

    // swap chain
    EditorRendererSwapChainWidget *swapChainWidget;

    void CreateUI(QWidget *pViewport)
    {
        QHBoxLayout *horizontalLayout;
        QSpacerItem *horizontalSpacer;

        gridLayout = new QGridLayout(pViewport);
        gridLayout->setSpacing(0);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setMargin(0);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(0);
        
        toolbarMoreOptions = new QToolButton(pViewport);
        toolbarMoreOptions->setMinimumSize(QSize(24, 24));
        toolbarMoreOptions->setMaximumSize(QSize(24, 24));
        toolbarMoreOptions->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_OtherOptions.png")));
        toolbarMoreOptions->setAutoRaise(false);
        horizontalLayout->addWidget(toolbarMoreOptions);

        horizontalSpacer = new QSpacerItem(4, 24, QSizePolicy::Fixed, QSizePolicy::Minimum);
        horizontalLayout->addItem(horizontalSpacer);

        toolbarRealTime = new QToolButton(pViewport);
        toolbarRealTime->setMinimumSize(QSize(24, 24));
        toolbarRealTime->setMaximumSize(QSize(24, 24));
        toolbarRealTime->setIcon(QIcon(QStringLiteral(":/editor/icons/PlayHS.png")));
        toolbarRealTime->setCheckable(true);
        toolbarRealTime->setChecked(false);
        toolbarRealTime->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarRealTime);

        toolbarGameView = new QToolButton(pViewport);
        toolbarGameView->setMinimumSize(QSize(24, 24));
        toolbarGameView->setMaximumSize(QSize(24, 24));
        toolbarGameView->setIcon(QIcon(QStringLiteral(":/editor/icons/AnimateHS.png")));
        toolbarGameView->setCheckable(true);
        toolbarGameView->setChecked(false);
        toolbarGameView->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarGameView);

        horizontalSpacer = new QSpacerItem(4, 24, QSizePolicy::Fixed, QSizePolicy::Minimum);
        horizontalLayout->addItem(horizontalSpacer);

        toolbarCameraFront = new QToolButton(pViewport);
        toolbarCameraFront->setMinimumSize(QSize(24, 24));
        toolbarCameraFront->setMaximumSize(QSize(24, 24));
        toolbarCameraFront->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_View_Front.png")));
        toolbarCameraFront->setCheckable(true);
        toolbarCameraFront->setChecked(false);
        toolbarCameraFront->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarCameraFront);

        toolbarCameraSide = new QToolButton(pViewport);
        toolbarCameraSide->setMinimumSize(QSize(24, 24));
        toolbarCameraSide->setMaximumSize(QSize(24, 24));
        toolbarCameraSide->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_View_Side.png")));
        toolbarCameraSide->setCheckable(true);
        toolbarCameraSide->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarCameraSide);

        toolbarCameraTop = new QToolButton(pViewport);
        toolbarCameraTop->setMinimumSize(QSize(24, 24));
        toolbarCameraTop->setMaximumSize(QSize(24, 24));
        toolbarCameraTop->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_View_Top.png")));
        toolbarCameraTop->setCheckable(true);
        toolbarCameraTop->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarCameraTop);

        toolbarCameraIsometric = new QToolButton(pViewport);
        toolbarCameraIsometric->setMinimumSize(QSize(24, 24));
        toolbarCameraIsometric->setMaximumSize(QSize(24, 24));
        toolbarCameraIsometric->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_View_Isometric.png")));
        toolbarCameraIsometric->setCheckable(true);
        toolbarCameraIsometric->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarCameraIsometric);

        toolbarCameraPerspective = new QToolButton(pViewport);
        toolbarCameraPerspective->setMinimumSize(QSize(24, 24));
        toolbarCameraPerspective->setMaximumSize(QSize(24, 24));
        toolbarCameraPerspective->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_View_Perspective.png")));
        toolbarCameraPerspective->setCheckable(true);
        toolbarCameraPerspective->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarCameraPerspective);

        horizontalSpacer = new QSpacerItem(4, 24, QSizePolicy::Fixed, QSizePolicy::Minimum);
        horizontalLayout->addItem(horizontalSpacer);

        toolbarViewWireframe = new QToolButton(pViewport);
        toolbarViewWireframe->setMinimumSize(QSize(24, 24));
        toolbarViewWireframe->setMaximumSize(QSize(24, 24));
        toolbarViewWireframe->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderMode_Wireframe.png")));
        toolbarViewWireframe->setCheckable(true);
        toolbarViewWireframe->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarViewWireframe);

        toolbarViewUnlit = new QToolButton(pViewport);
        toolbarViewUnlit->setMinimumSize(QSize(24, 24));
        toolbarViewUnlit->setMaximumSize(QSize(24, 24));
        toolbarViewUnlit->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderMode_Unlit.png")));
        toolbarViewUnlit->setCheckable(true);
        toolbarViewUnlit->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarViewUnlit);

        toolbarViewLit = new QToolButton(pViewport);
        toolbarViewLit->setMinimumSize(QSize(24, 24));
        toolbarViewLit->setMaximumSize(QSize(24, 24));
        toolbarViewLit->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderMode_Lit.png")));
        toolbarViewLit->setCheckable(true);
        toolbarViewLit->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarViewLit);

        toolbarViewLightingOnly = new QToolButton(pViewport);
        toolbarViewLightingOnly->setMinimumSize(QSize(24, 24));
        toolbarViewLightingOnly->setMaximumSize(QSize(24, 24));
        toolbarViewLightingOnly->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderMode_LightingOnly.png")));
        toolbarViewLightingOnly->setCheckable(true);
        toolbarViewLightingOnly->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarViewLightingOnly);

        horizontalSpacer = new QSpacerItem(4, 24, QSizePolicy::Fixed, QSizePolicy::Minimum);
        horizontalLayout->addItem(horizontalSpacer);

        toolbarFlagShadows = new QToolButton(pViewport);
        toolbarFlagShadows->setMinimumSize(QSize(24, 24));
        toolbarFlagShadows->setMaximumSize(QSize(24, 24));
        toolbarFlagShadows->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderFlag_Shadows.png")));
        toolbarFlagShadows->setCheckable(true);
        toolbarFlagShadows->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarFlagShadows);

        toolbarFlagWireframeOverlay = new QToolButton(pViewport);
        toolbarFlagWireframeOverlay->setMinimumSize(QSize(24, 24));
        toolbarFlagWireframeOverlay->setMaximumSize(QSize(24, 24));
        toolbarFlagWireframeOverlay->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderFlag_WireframeOverlay.png")));
        toolbarFlagWireframeOverlay->setCheckable(true);
        toolbarFlagWireframeOverlay->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarFlagWireframeOverlay);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        horizontalLayout->addItem(horizontalSpacer);

        gridLayout->addLayout(horizontalLayout, 0, 0, 1, 1);

        swapChainWidget = new EditorRendererSwapChainWidget(pViewport);
        swapChainWidget->setObjectName(QStringLiteral("swapChainWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(swapChainWidget->sizePolicy().hasHeightForWidth());
        swapChainWidget->setSizePolicy(sizePolicy);
        swapChainWidget->setMaximumSize(QSize(16777215, 16777215));
        swapChainWidget->setMouseTracking(true);
        swapChainWidget->setFocusPolicy(Qt::FocusPolicy(Qt::TabFocus | Qt::ClickFocus));
        swapChainWidget->setAcceptDrops(true);

        gridLayout->addWidget(swapChainWidget, 1, 0, 1, 1);
    }

    void UpdateUIForCameraMode(EDITOR_CAMERA_MODE cameraMode)
    {
        toolbarCameraFront->setChecked((cameraMode == EDITOR_CAMERA_MODE_ORTHOGRAPHIC_FRONT));
        toolbarCameraSide->setChecked((cameraMode == EDITOR_CAMERA_MODE_ORTHOGRAPHIC_SIDE));
        toolbarCameraTop->setChecked((cameraMode == EDITOR_CAMERA_MODE_ORTHOGRAPHIC_TOP));
        toolbarCameraIsometric->setChecked((cameraMode == EDITOR_CAMERA_MODE_ISOMETRIC));
        toolbarCameraPerspective->setChecked((cameraMode == EDITOR_CAMERA_MODE_PERSPECTIVE));
    }

    void UpdateUIForRenderMode(EDITOR_RENDER_MODE renderMode)
    {
        toolbarViewWireframe->setChecked((renderMode == EDITOR_RENDER_MODE_WIREFRAME));
        toolbarViewUnlit->setChecked((renderMode == EDITOR_RENDER_MODE_FULLBRIGHT));
        toolbarViewLit->setChecked((renderMode == EDITOR_RENDER_MODE_LIT));
        toolbarViewLightingOnly->setChecked((renderMode == EDITOR_RENDER_MODE_LIGHTING_ONLY));
    }

    void UpdateUIForViewportFlags(uint32 viewportFlags)
    {
        toolbarFlagShadows->setChecked((viewportFlags & EDITOR_VIEWPORT_FLAG_ENABLE_SHADOWS) != 0);
        toolbarFlagWireframeOverlay->setChecked((viewportFlags & EDITOR_VIEWPORT_FLAG_WIREFRAME_OVERLAY) != 0);
    }
};

