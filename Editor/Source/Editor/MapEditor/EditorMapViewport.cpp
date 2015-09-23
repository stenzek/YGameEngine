#include "Editor/PrecompiledHeader.h"
#include "Editor/MapEditor/EditorMapViewport.h"
#include "Editor/MapEditor/EditorMapWindow.h"
#include "Editor/MapEditor/EditorMap.h"
#include "Editor/MapEditor/EditorEditMode.h"
#include "Editor/Editor.h"
#include "Editor/EditorHelpers.h"
#include "Editor/EditorCVars.h"
#include "Engine/ResourceManager.h"
#include "Engine/Entity.h"
#include "Engine/Font.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderProfiler.h"
Log_SetChannel(EditorMapViewport);

static const PIXEL_FORMAT SCENE_PIXEL_FORMAT = PIXEL_FORMAT_R8G8B8A8_UNORM;
static const PIXEL_FORMAT SCENE_DEPTH_STENCIL_PIXEL_FORMAT = PIXEL_FORMAT_D24_UNORM_S8_UINT;
static const PIXEL_FORMAT PICKING_TEXTURE_PIXEL_FORMAT = PIXEL_FORMAT_R8G8B8A8_UNORM;
static const PIXEL_FORMAT PICKING_DEPTH_STENCIL_PIXEL_FORMAT = PIXEL_FORMAT_D24_UNORM_S8_UINT;

static EditorMapViewport *s_pViewportWithMouseGrab = nullptr;

EditorMapViewport::EditorMapViewport(EditorMapWindow *pMapWindow, EditorMap *pMap, uint32 viewportId, EDITOR_CAMERA_MODE cameraMode, EDITOR_RENDER_MODE renderMode, uint32 viewportFlags)
    : QWidget(pMapWindow),
      m_ui(new Ui_EditorMapViewport()),
      m_pMapWindow(pMapWindow),
      m_pMap(pMap),
      m_viewportId(viewportId),
      m_renderMode(renderMode),
      m_viewportFlags(viewportFlags),
      m_worldTime(0.0f),
      m_mousePosition(int2::Zero),
      m_mouseDelta(int2::Zero),
      m_mouseGrabbed(false),
      m_mouseGrabPosition(int2::Zero),
      m_cursorType(EDITOR_CURSOR_TYPE_ARROW),
      m_redrawPending(false),
      m_hardwareResourcesCreated(false),
      m_pSwapChain(nullptr),
      m_pPickingRenderTarget(nullptr),
      m_pPickingRenderTargetView(nullptr),
      m_pPickingDepthStencilBuffer(nullptr),
      m_pPickingDepthStencilBufferView(nullptr),
      m_pWorldRenderer(nullptr),
      m_pPickingWorldRenderer(nullptr)
{
    // update view ccontroller
    m_viewController.SetCameraMode(cameraMode);

    // create ui
    m_ui->CreateUI(this);
    m_ui->UpdateUIForCameraMode(cameraMode);
    m_ui->UpdateUIForRenderMode(renderMode);
    m_ui->UpdateUIForViewportFlags(viewportFlags);

    // connect ui events
    ConnectUIEvents();

    // queue a draw
    FlagForRedraw();
}

EditorMapViewport::~EditorMapViewport()
{
    if (s_pViewportWithMouseGrab == this)
        UnlockMouseCursor();

    // if active, unset
    if (IsActiveViewport())
        m_pMapWindow->SetActiveViewport(nullptr);

    // free everything else
    ReleaseHardwareResources();

    // kill ui
    delete m_ui;
}

void EditorMapViewport::SetCameraMode(EDITOR_CAMERA_MODE cameraMode)
{
    m_viewController.SetCameraMode(cameraMode);
    m_ui->UpdateUIForCameraMode(cameraMode);
}

void EditorMapViewport::SetRenderMode(EDITOR_RENDER_MODE renderMode)
{
    DebugAssert(renderMode < EDITOR_RENDER_MODE_COUNT);
    if (m_renderMode == renderMode)
    {
        // update ui anyway
        m_ui->UpdateUIForRenderMode(renderMode);
        return;
    }

    // update state
    m_renderMode = renderMode;
    delete m_pWorldRenderer;
    m_pWorldRenderer = EditorHelpers::CreateWorldRendererForRenderMode(renderMode, g_pRenderer->GetGPUContext(), m_viewportFlags, m_pSwapChain->GetWidth(), m_pSwapChain->GetHeight());
    FlagForRedraw();

    // update ui
    m_ui->UpdateUIForRenderMode(renderMode);
}

void EditorMapViewport::SetViewportFlag(uint32 flag)
{
    flag &= ~m_viewportFlags;
    if (flag == 0)
        return;

    m_viewportFlags |= flag;
    m_ui->UpdateUIForViewportFlags(m_viewportFlags);
    FlagForRedraw();
}

void EditorMapViewport::ClearViewportFlag(uint32 flag)
{
    flag &= m_viewportFlags;
    if (flag == 0)
        return;

    m_viewportFlags &= ~flag;
    m_ui->UpdateUIForViewportFlags(m_viewportFlags);
    FlagForRedraw();
}

bool EditorMapViewport::CreateHardwareResources()
{
    GPUContext *pGPUContext = g_pRenderer->GetGPUContext();

    // create swap chain
    if (m_pSwapChain == NULL)
    {
        if ((m_pSwapChain = m_ui->swapChainWidget->GetSwapChain()) == nullptr)
            return false;

        m_pSwapChain->AddRef();
    }

    uint32 swapChainWidth = m_pSwapChain->GetWidth();
    uint32 swapChainHeight = m_pSwapChain->GetHeight();
    //Log_DevPrintf("Creating hardware resources for viewport (%u x %u)", swapChainWidth, swapChainHeight);

    // create picking render target
    if (m_pPickingRenderTarget == NULL)
    {
        GPU_TEXTURE2D_DESC textureDesc;
        textureDesc.Width = swapChainWidth;
        textureDesc.Height = swapChainHeight;
        textureDesc.Format = PICKING_TEXTURE_PIXEL_FORMAT;
        textureDesc.Flags = GPU_TEXTURE_FLAG_READABLE | GPU_TEXTURE_FLAG_BIND_RENDER_TARGET;
        textureDesc.MipLevels = 1;

        GPU_DEPTH_TEXTURE_DESC depthStencilBufferDesc;
        depthStencilBufferDesc.Width = swapChainWidth;
        depthStencilBufferDesc.Height = swapChainHeight;
        depthStencilBufferDesc.Format = PIXEL_FORMAT_D16_UNORM;
        depthStencilBufferDesc.Flags = GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER;

        if ((m_pPickingRenderTarget = g_pRenderer->CreateTexture2D(&textureDesc, nullptr)) == nullptr ||
            (m_pPickingDepthStencilBuffer = g_pRenderer->CreateDepthTexture(&depthStencilBufferDesc)) == nullptr)
        {
            Log_ErrorPrintf("Could not create selection render target.");
            SAFE_RELEASE(m_pPickingDepthStencilBuffer);
            SAFE_RELEASE(m_pPickingRenderTarget);
            return false;
        }

        GPU_RENDER_TARGET_VIEW_DESC rtvDesc(m_pPickingRenderTarget, 0);
        GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC dsvDesc(m_pPickingDepthStencilBuffer);
        if ((m_pPickingRenderTargetView = g_pRenderer->CreateRenderTargetView(m_pPickingRenderTarget, &rtvDesc)) == nullptr ||
            (m_pPickingDepthStencilBufferView = g_pRenderer->CreateDepthStencilBufferView(m_pPickingDepthStencilBuffer, &dsvDesc)) == nullptr)
        {
            Log_ErrorPrintf("Could not create selection render target view.");
            SAFE_RELEASE(m_pPickingDepthStencilBufferView);
            SAFE_RELEASE(m_pPickingDepthStencilBuffer);
            SAFE_RELEASE(m_pPickingRenderTargetView);
            SAFE_RELEASE(m_pPickingRenderTarget);
            return false;
        }

        // create picking texture local copy
        if (!m_PickingTextureCopy.IsValidImage())
        {
            m_PickingTextureCopy.Create(PICKING_TEXTURE_PIXEL_FORMAT, swapChainWidth, swapChainHeight, 1);
            Y_memzero(m_PickingTextureCopy.GetData(), m_PickingTextureCopy.GetDataSize());
        }
    }

    // create render context
    if (m_pWorldRenderer == nullptr)
        m_pWorldRenderer = EditorHelpers::CreateWorldRendererForRenderMode(m_renderMode, g_pRenderer->GetGPUContext(), m_viewportFlags, swapChainWidth, swapChainHeight);

    // create picking texture render context
    if (m_pPickingWorldRenderer == nullptr)
        m_pPickingWorldRenderer = EditorHelpers::CreatePickingWorldRenderer(pGPUContext, swapChainWidth, swapChainHeight);
    
    // update gui context, camera
    m_guiContext.SetViewportDimensions(swapChainWidth, swapChainHeight);
    m_guiContext.SetGPUContext(g_pRenderer->GetGPUContext());
    m_viewController.SetViewportDimensions(swapChainWidth, swapChainHeight);

#ifdef Y_BUILD_CONFIG_DEBUG
    m_pPickingRenderTarget->SetDebugName("Picking Render Target");
    m_pPickingDepthStencilBuffer->SetDebugName("Picking Depth Stencil Buffer");
#endif

    // cancel any pending draws until the windows are arranged and a paint is requested.
    m_hardwareResourcesCreated = true;
    return true;
}

void EditorMapViewport::ReleaseHardwareResources()
{
    m_guiContext.ClearState();
    delete m_pWorldRenderer;
    delete m_pPickingWorldRenderer;

    m_PickingTextureCopy.Delete();
    SAFE_RELEASE(m_pPickingDepthStencilBufferView);
    SAFE_RELEASE(m_pPickingDepthStencilBuffer);
    SAFE_RELEASE(m_pPickingRenderTargetView);
    SAFE_RELEASE(m_pPickingRenderTarget);

    SAFE_RELEASE(m_pSwapChain);
    m_ui->swapChainWidget->DestroySwapChain();
    m_hardwareResourcesCreated = false;
}

const bool EditorMapViewport::IsActiveViewport() const
{
    return (m_pMapWindow->GetActiveViewport() == this);
}

void EditorMapViewport::SetFocus()
{
    m_pMapWindow->SetActiveViewport(this);
    m_ui->swapChainWidget->setFocus();
}

void EditorMapViewport::SetMouseCursor(EDITOR_CURSOR_TYPE cursorType)
{
    //m_ui->swapChainWidget->setCursor()
    m_cursorType = cursorType;
}

void EditorMapViewport::LockMouseCursor()
{
    if (m_mouseGrabbed)
        return;

    if (s_pViewportWithMouseGrab != nullptr)
        s_pViewportWithMouseGrab->UnlockMouseCursor();

    // grab mouse and set an empty cursor
    m_ui->swapChainWidget->grabMouse(QCursor(Qt::BlankCursor));

    // get current global mouse position
    QPoint currentMousePosition(QCursor::pos());

    // init grab state
    m_mouseGrabbed = true;
    m_mouseGrabPosition.Set(currentMousePosition.x(), currentMousePosition.y());
    s_pViewportWithMouseGrab = this;

    FlagForRedraw();
}

void EditorMapViewport::UnlockMouseCursor()
{
    DebugAssert(s_pViewportWithMouseGrab == this);

    // restore old position
    QCursor::setPos(m_mouseGrabPosition.x, m_mouseGrabPosition.y);

    // release grab and cursor at the same time
    m_ui->swapChainWidget->releaseMouse();

    m_mouseGrabbed = false;
    s_pViewportWithMouseGrab = nullptr;

    // redraw to remove the crosshairs
    FlagForRedraw();
}

void EditorMapViewport::FlagForRedraw()
{
    if (!m_redrawPending)
    {
        m_redrawPending = true;
        g_pEditor->QueueFrameExecution();
    }
}

bool EditorMapViewport::Draw(const float timeDiff)
{
    // update camera
    m_viewController.Update(timeDiff);

    // update time
    if (HasViewportFlag(EDITOR_VIEWPORT_FLAG_REALTIME))
        m_worldTime += timeDiff;

    // force draw if requested by camera
    if (m_viewController.IsChanged())
    {
        m_pMapWindow->OnViewportCameraChange(this);
        if (!HasViewportFlag(EDITOR_VIEWPORT_FLAG_REALTIME))
            FlagForRedraw();
    }

    // do draw
    if (!m_redrawPending && !(m_viewportFlags & EDITOR_VIEWPORT_FLAG_REALTIME))
        return false;
    
    GPUContext *pGPUContext = g_pRenderer->GetGPUContext();

    // create hardware resources
    if (!m_hardwareResourcesCreated && !CreateHardwareResources())
        return false;

    // bind output buffer
    pGPUContext->SetOutputBuffer(m_pSwapChain);

    // for debugging, we draw the view first, but for release and perf optimization, we draw the view last
    DrawView();
    DrawPickTexture();

    // swap swapchain buffers
    pGPUContext->PresentOutputBuffer(GPU_PRESENT_BEHAVIOUR_IMMEDIATE);

    // now read the picking texture back.
    DebugAssert(m_PickingTextureCopy.IsValidImage());
    if (!pGPUContext->ReadTexture(m_pPickingRenderTarget, m_PickingTextureCopy.GetData(), m_PickingTextureCopy.GetDataRowPitch(), m_PickingTextureCopy.GetDataSize(), 0, 0, 0, m_PickingTextureCopy.GetWidth(), m_PickingTextureCopy.GetHeight()))
    {
        Log_WarningPrintf("Failed to read back picking texture.");
        Y_memzero(m_PickingTextureCopy.GetData(), m_PickingTextureCopy.GetDataSize());
    }

    // clear context state
    pGPUContext->ClearState(true, true, true, true);

    // no draw pending now
    m_redrawPending = false;

    return true;
}

bool EditorMapViewport::GetPickingTextureValues(const int2 &MinCoordinates, const int2 &MaxCoordinates, uint32 *pValues) const
{
    if (m_pPickingRenderTarget == NULL ||
        (uint32)MinCoordinates.x >= m_PickingTextureCopy.GetWidth() ||
        (uint32)MinCoordinates.y >= m_PickingTextureCopy.GetHeight() ||
        (uint32)MaxCoordinates.x >= m_PickingTextureCopy.GetWidth() ||
        (uint32)MaxCoordinates.y >= m_PickingTextureCopy.GetHeight() ||
        MinCoordinates.AnyGreater(MaxCoordinates))
    {
        return false;
    }

    // get number of values to read
    Vector2i coordinateRange = Vector2i(MaxCoordinates) - Vector2i(MinCoordinates) + 1;
    int32 nValues = (coordinateRange.x) * (coordinateRange.y);
    DebugAssert(nValues > 0);

    // read the pixels from the image copy
    if (!m_PickingTextureCopy.ReadPixels(pValues, sizeof(uint32) * nValues, MinCoordinates.x, MinCoordinates.y, coordinateRange.x, coordinateRange.y))
    {
        // not sure why this would fail
        return false;
    }

    return true;
}

uint32 EditorMapViewport::GetPickingTextureValue(const int2 &Position) const
{
    uint32 texelValue;
    if (!GetPickingTextureValues(Position, Position, &texelValue))
        return 0;

    return texelValue;
}

Ray EditorMapViewport::GetPickRay() const
{
    return m_viewController.GetPickRay(m_mousePosition.x, m_mousePosition.y);
}

void EditorMapViewport::DrawView()
{
    GPUContext *pGPUContext = g_pRenderer->GetGPUContext();
    GPUContextConstants *pGPUConstants = pGPUContext->GetConstants();
    SmallString tempStr;

    // set targets
    pGPUContext->SetOutputBuffer(m_pSwapChain);
    pGPUContext->SetRenderTargets(0, nullptr, nullptr);
    pGPUContext->SetViewport(m_viewController.GetViewport());
    pGPUContext->ClearTargets(true, true, true);

    // setup 3d state
    pGPUConstants->SetFromCamera(m_viewController.GetCamera(), true);

    // draw grid
    if (m_pMap->GetGridLinesVisible())
    {
        static const float gridWidth = 32768.0f * 0.5f;
        static const float gridHeight = 32768.0f * 0.5f;
        m_guiContext.Draw3DGrid(Plane(0.0f, 0.0f, 1.0f, 0.0f), float3::Zero, gridWidth, gridHeight, m_pMap->GetGridLinesInterval());
    }
    
    // callbacks before world
    m_pMapWindow->GetActiveEditModePointer()->OnViewportDrawBeforeWorld(this);
    
    // draw world
    m_pWorldRenderer->DrawWorld(m_pMap->GetRenderWorld(), m_viewController.GetViewParameters(), nullptr, nullptr);

    // world renderer can reset the render targets, so set them back again
    pGPUContext->SetRenderTargets(0, nullptr, nullptr);
    pGPUContext->SetViewport(m_viewController.GetViewport());

    // callbacks after world
    m_pMapWindow->GetActiveEditModePointer()->OnViewportDrawAfterWorld(this);

    // draw overlays
    m_pMapWindow->GetActiveEditModePointer()->OnViewportDrawAfterPost(this);
    DrawAfterPost();

    // clear state of everything
    m_guiContext.ClearState();
}

void EditorMapViewport::DrawAfterPost()
{
    m_guiContext.PushManualFlush();

    // draw crosshair in widget if grabbed
    if (m_mouseGrabbed && m_cursorType == EDITOR_CURSOR_TYPE_CROSS)
    {
        const RENDERER_VIEWPORT *pRenderViewport = m_viewController.GetViewport();
        int32 startX = pRenderViewport->Width / 2;
        int32 startY = pRenderViewport->Height / 2;
            
        m_guiContext.DrawLine(int2(startX - 5, startY), int2(startX + 4, startY), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 150));
        m_guiContext.DrawLine(int2(startX, startY - 5), int2(startX, startY + 4), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 150));
    }

    m_guiContext.PopManualFlush();
}

void EditorMapViewport::DrawPickTexture()
{
    // setup render targets and constants
    GPUContext *pGPUContext = g_pRenderer->GetGPUContext();
    pGPUContext->GetConstants()->SetFromCamera(m_viewController.GetCamera(), true);

    // callbacks before world
    m_pMapWindow->GetActiveEditModePointer()->OnPickingTextureDrawBeforeWorld(this);

    // draw world
    m_pPickingWorldRenderer->DrawWorld(m_pMap->GetRenderWorld(), m_viewController.GetViewParameters(), m_pPickingRenderTargetView, m_pPickingDepthStencilBufferView);

    // world renderer can reset the render targets, so set them back again
    pGPUContext->SetRenderTargets(1, &m_pPickingRenderTargetView, m_pPickingDepthStencilBufferView);
    pGPUContext->SetViewport(m_viewController.GetViewport());

    // callbacks after world
    m_pMapWindow->GetActiveEditModePointer()->OnPickingTextureDrawAfterWorld(this);

    // clear render targets
    pGPUContext->SetRenderTargets(0, NULL, NULL);
}

void EditorMapViewport::ConnectUIEvents()
{
    connect(m_ui->toolbarMoreOptions, SIGNAL(clicked()), this, SLOT(OnToolbarMoreOptionsClicked()));
    connect(m_ui->toolbarRealTime, SIGNAL(clicked(bool)), this, SLOT(OnToolbarRealtimeClicked(bool)));
    connect(m_ui->toolbarGameView, SIGNAL(clicked(bool)), this, SLOT(OnToolbarGameViewClicked(bool)));

    connect(m_ui->toolbarCameraFront, SIGNAL(clicked(bool)), this, SLOT(OnToolbarCameraFrontClicked(bool)));
    connect(m_ui->toolbarCameraSide, SIGNAL(clicked(bool)), this, SLOT(OnToolbarCameraSideClicked(bool)));
    connect(m_ui->toolbarCameraTop, SIGNAL(clicked(bool)), this, SLOT(OnToolbarCameraTopClicked(bool)));
    connect(m_ui->toolbarCameraIsometric, SIGNAL(clicked(bool)), this, SLOT(OnToolbarCameraIsometricClicked(bool)));
    connect(m_ui->toolbarCameraPerspective, SIGNAL(clicked(bool)), this, SLOT(OnToolbarCameraPerspectiveClicked(bool)));
    connect(m_ui->toolbarViewWireframe, SIGNAL(clicked(bool)), this, SLOT(OnToolbarViewWireframeClicked(bool)));
    connect(m_ui->toolbarViewUnlit, SIGNAL(clicked(bool)), this, SLOT(OnToolbarViewUnlitClicked(bool)));
    connect(m_ui->toolbarViewLit, SIGNAL(clicked(bool)), this, SLOT(OnToolbarViewLitClicked(bool)));
    connect(m_ui->toolbarViewLightingOnly, SIGNAL(clicked(bool)), this, SLOT(OnToolbarViewLightingOnlyClicked(bool)));
    connect(m_ui->toolbarFlagShadows, SIGNAL(clicked(bool)), this, SLOT(OnToolbarFlagShadowsClicked(bool)));
    connect(m_ui->toolbarFlagWireframeOverlay, SIGNAL(clicked(bool)), this, SLOT(OnToolbarFlagWireframeOverlayClicked(bool)));

    connect(m_ui->swapChainWidget, SIGNAL(ResizedEvent()), this, SLOT(OnSwapChainWidgetResized()));
    connect(m_ui->swapChainWidget, SIGNAL(PaintEvent()), this, SLOT(OnSwapChainWidgetPaint()));
    connect(m_ui->swapChainWidget, SIGNAL(KeyboardEvent(const QKeyEvent *)), this, SLOT(OnSwapChainWidgetKeyboardEvent(const QKeyEvent *)));
    connect(m_ui->swapChainWidget, SIGNAL(MouseEvent(const QMouseEvent *)), this, SLOT(OnSwapChainWidgetMouseEvent(const QMouseEvent *)));
    connect(m_ui->swapChainWidget, SIGNAL(WheelEvent(const QWheelEvent *)), this, SLOT(OnSwapChainWidgetWheelEvent(const QWheelEvent *)));
    connect(m_ui->swapChainWidget, SIGNAL(DropEvent(QDropEvent *)), this, SLOT(OnSwapChainWidgetDropEvent(QDropEvent *)));
    connect(m_ui->swapChainWidget, SIGNAL(GainedFocusEvent()), this, SLOT(OnSwapChainWidgetGainedFocusEvent()));
}

void EditorMapViewport::focusInEvent(QFocusEvent *pFocusEvent)
{
    if (!IsActiveViewport())
        m_pMapWindow->SetActiveViewport(this);
}

void EditorMapViewport::OnSwapChainWidgetGainedFocusEvent()
{
    if (!IsActiveViewport())
        m_pMapWindow->SetActiveViewport(this);
}

void EditorMapViewport::OnToolbarMoreOptionsClicked()
{

}

void EditorMapViewport::OnSwapChainWidgetResized()
{
    // kill old swapchain refs
    SAFE_RELEASE(m_pSwapChain);

    // kill the picking buffers, these will have to be re-created
    SAFE_RELEASE(m_pPickingRenderTarget);
    SAFE_RELEASE(m_pPickingRenderTargetView);
    SAFE_RELEASE(m_pPickingDepthStencilBuffer);
    SAFE_RELEASE(m_pPickingDepthStencilBufferView);
    m_PickingTextureCopy.Delete();

    // kill renderers
    delete m_pWorldRenderer;
    m_pWorldRenderer = nullptr;
    delete m_pPickingWorldRenderer;
    m_pPickingWorldRenderer = nullptr;

    // force recreation of remaining resources
    m_hardwareResourcesCreated = false;
    FlagForRedraw();
}

void EditorMapViewport::OnSwapChainWidgetPaint()
{
    FlagForRedraw();
}

void EditorMapViewport::OnSwapChainWidgetKeyboardEvent(const QKeyEvent *pKeyboardEvent)
{
    // pass through to edit mode
    if (!m_pMapWindow->GetActiveEditModePointer()->HandleViewportKeyboardInputEvent(this, pKeyboardEvent))
    {

    }
}

void EditorMapViewport::OnSwapChainWidgetMouseEvent(const QMouseEvent *pMouseEvent)
{
    // update the mouse position from the event
    if (pMouseEvent->type() == QEvent::MouseMove)
    {
        // calculate the delta
        int2 newMousePosition(pMouseEvent->x(), pMouseEvent->y());
        m_mouseDelta = newMousePosition - m_mousePosition;
        m_mousePosition = newMousePosition;

        //Log_DevPrintf("mouse pos: %i %i, delta: %i %i", pMouseEvent->x(), pMouseEvent->y(), m_mouseDelta.x, m_mouseDelta.y);
    }

    // pass through to edit mode
    if (!m_pMapWindow->GetActiveEditModePointer()->HandleViewportMouseInputEvent(this, pMouseEvent))
    {
    }
}

void EditorMapViewport::OnSwapChainWidgetWheelEvent(const QWheelEvent *pWheelEvent)
{

}

void EditorMapViewport::OnSwapChainWidgetDropEvent(QDropEvent *pDropEvent)
{
    Log_DevPrintf("");
}
