#pragma once

class Ui_EditorResourcePreviewWidget
{
public:
    QGridLayout *gridLayout;

    // toolbar
    QToolButton *toolbarViewWireframe;
    QToolButton *toolbarViewUnlit;
    QToolButton *toolbarViewLit;
    QToolButton *toolbarViewLightingOnly;
    QToolButton *toolbarFlagShadows;
    QToolButton *toolbarFlagWireframeOverlay;

    // swap chain
    EditorRendererSwapChainWidget *swapChainWidget;

    void CreateUI(QWidget *pWidget)
    {
        //static const char *TRANSLATION_CONTEXT = "EditorResourcePreviewWidget";
        QHBoxLayout *horizontalLayout;
        QSpacerItem *horizontalSpacer;

        gridLayout = new QGridLayout(pWidget);
        gridLayout->setSpacing(0);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        gridLayout->setMargin(0);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(0);
        
        toolbarViewWireframe = new QToolButton(pWidget);
        toolbarViewWireframe->setMinimumSize(QSize(24, 24));
        toolbarViewWireframe->setMaximumSize(QSize(24, 24));
        toolbarViewWireframe->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderMode_Wireframe.png")));
        toolbarViewWireframe->setCheckable(true);
        toolbarViewWireframe->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarViewWireframe);

        toolbarViewUnlit = new QToolButton(pWidget);
        toolbarViewUnlit->setMinimumSize(QSize(24, 24));
        toolbarViewUnlit->setMaximumSize(QSize(24, 24));
        toolbarViewUnlit->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderMode_Unlit.png")));
        toolbarViewUnlit->setCheckable(true);
        toolbarViewUnlit->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarViewUnlit);

        toolbarViewLit = new QToolButton(pWidget);
        toolbarViewLit->setMinimumSize(QSize(24, 24));
        toolbarViewLit->setMaximumSize(QSize(24, 24));
        toolbarViewLit->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderMode_Lit.png")));
        toolbarViewLit->setCheckable(true);
        toolbarViewLit->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarViewLit);

        toolbarViewLightingOnly = new QToolButton(pWidget);
        toolbarViewLightingOnly->setMinimumSize(QSize(24, 24));
        toolbarViewLightingOnly->setMaximumSize(QSize(24, 24));
        toolbarViewLightingOnly->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderMode_LightingOnly.png")));
        toolbarViewLightingOnly->setCheckable(true);
        toolbarViewLightingOnly->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarViewLightingOnly);

        horizontalSpacer = new QSpacerItem(4, 24, QSizePolicy::Fixed, QSizePolicy::Minimum);
        horizontalLayout->addItem(horizontalSpacer);

        toolbarFlagShadows = new QToolButton(pWidget);
        toolbarFlagShadows->setMinimumSize(QSize(24, 24));
        toolbarFlagShadows->setMaximumSize(QSize(24, 24));
        toolbarFlagShadows->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderFlag_Shadows.png")));
        toolbarFlagShadows->setCheckable(true);
        toolbarFlagShadows->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarFlagShadows);

        toolbarFlagWireframeOverlay = new QToolButton(pWidget);
        toolbarFlagWireframeOverlay->setMinimumSize(QSize(24, 24));
        toolbarFlagWireframeOverlay->setMaximumSize(QSize(24, 24));
        toolbarFlagWireframeOverlay->setIcon(QIcon(QStringLiteral(":/editor/icons/Viewport_RenderFlag_WireframeOverlay.png")));
        toolbarFlagWireframeOverlay->setCheckable(true);
        toolbarFlagWireframeOverlay->setAutoRaise(true);
        horizontalLayout->addWidget(toolbarFlagWireframeOverlay);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        horizontalLayout->addItem(horizontalSpacer);

        gridLayout->addLayout(horizontalLayout, 0, 0, 1, 1);

        swapChainWidget = new EditorRendererSwapChainWidget(pWidget);
        swapChainWidget->setObjectName(QStringLiteral("swapChainWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(swapChainWidget->sizePolicy().hasHeightForWidth());
        swapChainWidget->setSizePolicy(sizePolicy);
        swapChainWidget->setMaximumSize(QSize(16777215, 16777215));
        swapChainWidget->setMouseTracking(true);
        swapChainWidget->setFocusPolicy(Qt::FocusPolicy(Qt::TabFocus | Qt::ClickFocus));

        gridLayout->addWidget(swapChainWidget, 1, 0, 1, 1);
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

