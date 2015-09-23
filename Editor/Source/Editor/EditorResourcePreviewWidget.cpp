#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorResourcePreviewWidget.h"
#include "Editor/Editor.h"
#include "Editor/EditorHelpers.h"
#include "Editor/EditorLightSimulator.h"
#include "Engine/ResourceManager.h"
#include "Engine/Material.h"
#include "Engine/MaterialShader.h"
#include "Engine/Texture.h"
#include "Engine/StaticMesh.h"
#include "Engine/BlockMesh.h"
#include "Engine/Font.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/RenderProxies/BlockMeshRenderProxy.h"
#include "Renderer/RenderProxies/StaticMeshRenderProxy.h"
Log_SetChannel(EditorResourcePreviewWidget);

EditorResourcePreviewWidget::EditorResourcePreviewWidget(QWidget *pParent /* = NULL */)
    : QWidget(pParent),
      m_ui(new Ui_EditorResourcePreviewWidget()),
      m_renderMode(EDITOR_RENDER_MODE_FULLBRIGHT),
      m_viewportFlags(0),
      m_ePreviewType(PREVIEW_TYPE_NONE),
      m_bRedrawPending(true),
      m_renderTargetWidth(1),
      m_renderTargetHeight(1),
      m_bHardwareResourcesCreated(false),
      m_pSwapChain(nullptr),
      m_pWorldRenderer(nullptr),
      m_fZoomScale(1.0f),
      m_pOverlayTextFont(nullptr),
      m_pRenderWorld(nullptr),
      m_pResourceRenderProxy(nullptr),
      m_pLightSimulator(nullptr),
      m_eCurrentState(STATE_NONE),
      m_iStateData(0),
      m_LastMousePosition(int2::Zero)
{
    m_Resource.asResource = NULL;

    // create ui
    m_ui->CreateUI(this);
    m_ui->UpdateUIForRenderMode(m_renderMode);
    m_ui->UpdateUIForViewportFlags(m_viewportFlags);
    ConnectUIEvents();

    // setup camera
    ResetCamera();

    // create world, and light entity
    m_pRenderWorld = new RenderWorld();
    m_pLightSimulator = new EditorLightSimulator(0, m_pRenderWorld);
    m_pLightSimulator->SetShowIndicator(false);
    
    // add to tick manager
    connect(g_pEditor, SIGNAL(OnFrameExecution(float)), this, SLOT(OnFrameExecutionTriggered(float)));
}

EditorResourcePreviewWidget::~EditorResourcePreviewWidget()
{
    ClearPreview();

    delete m_pLightSimulator;
    m_pRenderWorld->Release();
    SAFE_RELEASE(m_pResourceRenderProxy);
    SAFE_RELEASE(m_pOverlayTextFont);

    ReleaseGPUResources();
}

void EditorResourcePreviewWidget::SetRenderMode(EDITOR_RENDER_MODE renderMode)
{
    DebugAssert(renderMode < EDITOR_RENDER_MODE_COUNT);
    if (m_renderMode == renderMode)
    {
        // update ui anyway
        m_ui->UpdateUIForRenderMode(renderMode);
        return;
    }

    m_renderMode = renderMode;
    m_ui->UpdateUIForRenderMode(renderMode);

    delete m_pWorldRenderer;
    m_pWorldRenderer = EditorHelpers::CreateWorldRendererForRenderMode(renderMode, g_pRenderer->GetGPUContext(), m_viewportFlags, m_viewParameters.Viewport.Width, m_viewParameters.Viewport.Height);

    FlagForRedraw();
}

void EditorResourcePreviewWidget::SetViewportFlag(uint32 flag)
{
    flag &= ~m_viewportFlags;
    if (flag == 0)
        return;

    m_viewportFlags |= flag;
    m_ui->UpdateUIForViewportFlags(m_viewportFlags);
    FlagForRedraw();
}

void EditorResourcePreviewWidget::ClearViewportFlag(uint32 flag)
{
    flag &= m_viewportFlags;
    if (flag == 0)
        return;

    m_viewportFlags &= ~flag;
    m_ui->UpdateUIForViewportFlags(m_viewportFlags);
    FlagForRedraw();
}

bool EditorResourcePreviewWidget::CreateGPUResources()
{
    if (m_bHardwareResourcesCreated)
        return true;

    Log_DevPrintf("Creating hardware resources for preview widget (%u x %u)", m_renderTargetWidth, m_renderTargetHeight);

    // create swap chain
    if (m_pSwapChain == NULL)
    {
        if ((m_pSwapChain = m_ui->swapChainWidget->GetSwapChain()) == NULL)
            return false;

        m_pSwapChain->AddRef();
    }

    // create render context
    if (m_pWorldRenderer == NULL)
        m_pWorldRenderer = EditorHelpers::CreateWorldRendererForRenderMode(m_renderMode, g_pRenderer->GetGPUContext(), m_viewportFlags, m_viewParameters.Viewport.Width, m_viewParameters.Viewport.Height);

    // update gui context, camera
    m_ArcBallCamera.SetPerspectiveAspect((float)m_renderTargetWidth, (float)m_renderTargetHeight);
    m_guiContext.SetViewportDimensions(m_renderTargetWidth, m_renderTargetHeight);
    m_guiContext.SetGPUContext(g_pRenderer->GetGPUContext());
    m_viewParameters.Viewport.Set(0, 0, m_renderTargetWidth, m_renderTargetHeight, 0.0f, 1.0f);

    // done
    m_bHardwareResourcesCreated = true;
    return true;
}

void EditorResourcePreviewWidget::ReleaseGPUResources()
{
    m_guiContext.ClearState();

    delete m_pWorldRenderer;
    m_pWorldRenderer = NULL;

    SAFE_RELEASE(m_pSwapChain);
    m_ui->swapChainWidget->DestroySwapChain();
    m_bHardwareResourcesCreated = false;
}

void EditorResourcePreviewWidget::Draw()
{
    GPUContext *pGPUDevice = g_pRenderer->GetGPUContext();
    GPUContextConstants *pGPUConstants = pGPUDevice->GetConstants();

    // create hardware resources
    if (!m_bHardwareResourcesCreated && !CreateGPUResources())
        return;

    // update view parameters
    m_viewParameters.ViewCamera = m_ArcBallCamera;
    m_viewParameters.ViewCamera.SetObjectCullDistance(m_ArcBallCamera.GetFarPlaneDistance() - m_ArcBallCamera.GetNearPlaneDistance());
    m_viewParameters.MaximumShadowViewDistance = m_viewParameters.ViewCamera.GetObjectCullDistance();

    // clear target
    pGPUDevice->SetOutputBuffer(m_pSwapChain);
    pGPUDevice->SetRenderTargets(0, NULL, NULL);
    pGPUDevice->SetViewport(&m_viewParameters.Viewport);
    pGPUDevice->ClearTargets(true, true, true);

    // setup constants
    pGPUConstants->SetLocalToWorldMatrix(float4x4::Identity, false);
    pGPUConstants->SetFromCamera(m_viewParameters.ViewCamera, true);

    // do nothing if preview type is none
    if (m_ePreviewType != PREVIEW_TYPE_NONE)
        DrawPreview();

    // draw overlays
    DrawPreviewOverlays();

    // clear state
    pGPUDevice->ClearState(true, true, true, true);

    // end draw calls, flip buffers
    pGPUDevice->PresentOutputBuffer(GPU_PRESENT_BEHAVIOUR_IMMEDIATE);

    // no draw pending now
    m_bRedrawPending = false;
}

void EditorResourcePreviewWidget::OnFrameExecutionTriggered(float timeSinceLastFrame)
{
    // update camera
    m_ArcBallCamera.Update(timeSinceLastFrame);

    //// force draw if requested by camera
    //if (m_ArcBallCamera.IsDirty())
        //FlagForRedraw();

    // draw
    if (m_bRedrawPending || (m_viewportFlags & EDITOR_VIEWPORT_FLAG_REALTIME))
        Draw();
}

void EditorResourcePreviewWidget::SetPreviewErrorMessage(const char *message)
{
    if (m_pResourceRenderProxy != NULL)
    {
        m_pRenderWorld->RemoveRenderable(m_pResourceRenderProxy);
        m_pResourceRenderProxy->Release();
        m_pResourceRenderProxy = NULL;
    }

    SAFE_RELEASE(m_Resource.asResource);
    m_ePreviewType = PREVIEW_TYPE_NONE;    
    m_errorMessage = message;

    UpdateZoomScale();
    ResetCamera();
    FlagForRedraw();
}

void EditorResourcePreviewWidget::ClearPreview()
{
    if (m_pResourceRenderProxy != NULL)
    {
        m_pRenderWorld->RemoveRenderable(m_pResourceRenderProxy);
        m_pResourceRenderProxy->Release();
        m_pResourceRenderProxy = NULL;
    }

    SAFE_RELEASE(m_Resource.asResource);
    m_ePreviewType = PREVIEW_TYPE_NONE;    
    m_errorMessage = "No resource selected.";

    UpdateZoomScale();
    ResetCamera();
    FlagForRedraw();
}

void EditorResourcePreviewWidget::UpdateZoomScale()
{
    m_fZoomScale = 1.0f;

    if (m_pResourceRenderProxy != NULL)
    {
        // determine size of object
        AABox renderProxyBounds = m_pResourceRenderProxy->GetBoundingBox();
        float3 objectSize(renderProxyBounds.GetExtents());

        // halve it, then tune each mouse wheel click to one fourth of the size
        float maxComponent = Max(objectSize.x, Max(objectSize.y, objectSize.z));
        m_fZoomScale = Max(maxComponent / 8.0f, 1.0f);
        Log_DevPrintf("EditorResourcePreviewWidget::UpdateZoomScale: Max component: %f, zoom scale = %f", maxComponent, m_fZoomScale);
    }

    FlagForRedraw();
}

void EditorResourcePreviewWidget::ResetCamera()
{
    m_ArcBallCamera.Reset();
    m_ArcBallCamera.SetEyeDistance(1.0f);

    if (m_pResourceRenderProxy != NULL)
    {
        // determine size of object
        AABox renderProxyBounds = m_pResourceRenderProxy->GetBoundingBox();
        float3 objectSize(renderProxyBounds.GetExtents());

        // zoom it so that the object fits on the screen
        float maxComponent = Max(objectSize.x, Max(objectSize.y, objectSize.z));
        m_ArcBallCamera.SetEyeDistance(-maxComponent * 3.0f);

        // update far z
        m_ArcBallCamera.SetFarPlaneDistance(Max(100.0f, maxComponent * 10.0f));

        // update target
        m_ArcBallCamera.SetTarget(renderProxyBounds.GetCenter());

        // debugging
        Log_DevPrintf("EditorResourcePreviewWidget::ResetCamera: Max component: %f, starting eye distance = %f, far plane distance = %f, target = %s", maxComponent, m_ArcBallCamera.GetEyeDistance(), m_ArcBallCamera.GetFarPlaneDistance(), StringConverter::Float3ToString(m_ArcBallCamera.GetTarget()).GetCharArray());
    }

    FlagForRedraw();
}

bool EditorResourcePreviewWidget::SetPreviewResource(const Resource *pResource)
{
    if (pResource != NULL)
    {
        if (pResource->GetResourceTypeInfo() == Material::StaticTypeInfo())
            SetPreviewMaterial(pResource->Cast<Material>());
        else if (pResource->GetResourceTypeInfo() == MaterialShader::StaticTypeInfo())
            SetPreviewMaterialShader(pResource->Cast<MaterialShader>());
        else if (pResource->GetResourceTypeInfo()->IsDerived(Texture::StaticTypeInfo()))
            SetPreviewTexture(pResource->Cast<Texture>());
        else if (pResource->GetResourceTypeInfo() == StaticMesh::StaticTypeInfo())
            SetPreviewStaticMesh(pResource->Cast<StaticMesh>());
        else if (pResource->GetResourceTypeInfo() == BlockMesh::StaticTypeInfo())
            SetPreviewStaticBlockMesh(pResource->Cast<BlockMesh>());
        else
        {
            SmallString error;
            error.Format("Error: Unhandled resource type: '%s' for '%s'.", pResource->GetResourceTypeInfo()->GetTypeName(), pResource->GetName().GetCharArray());
            SetPreviewErrorMessage(error);
            return false;
        }
    }
    else
    {
        ClearPreview();
    }

    return true;
}

bool EditorResourcePreviewWidget::SetPreviewResourceByName(const ResourceTypeInfo *pResourceTypeInfo, const char *resourceName)
{
    // load the resource
    AutoReleasePtr<const Resource> pResource = g_pResourceManager->UncachedGetResource(pResourceTypeInfo, resourceName);
    if (pResource == NULL)
    {
        SmallString error;
        error.Format("Could not load '%s' as a '%s'.", resourceName, pResourceTypeInfo->GetTypeName());
        SetPreviewErrorMessage(error);
        return false;
    }

    return SetPreviewResource(pResource);
}

void EditorResourcePreviewWidget::SetPreviewMaterial(const Material *pMaterial)
{
    ClearPreview();

    m_ePreviewType = PREVIEW_TYPE_MATERIAL;
    m_Resource.asMaterial = pMaterial;
    m_Resource.asMaterial->AddRef();

    SetupStaticMesh(NULL, pMaterial);
    UpdateZoomScale();
    ResetCamera();
}

void EditorResourcePreviewWidget::SetPreviewMaterialShader(const MaterialShader *pMaterialShader)
{
    ClearPreview();

    m_ePreviewType = PREVIEW_TYPE_MATERIALSHADER;
    m_Resource.asMaterialShader = pMaterialShader;
    m_Resource.asMaterialShader->AddRef();

    Material *pTempMaterial = new Material();
    pTempMaterial->Create("", pMaterialShader);

    SetupStaticMesh(NULL, pTempMaterial);
    UpdateZoomScale();
    ResetCamera();

    pTempMaterial->Release();
}

void EditorResourcePreviewWidget::SetPreviewTexture(const Texture *pTexture)
{
    ClearPreview();

    m_ePreviewType = PREVIEW_TYPE_TEXTURE2D;
    m_Resource.asTexture = pTexture;
    m_Resource.asTexture->AddRef();

    UpdateZoomScale();
    ResetCamera();
}

void EditorResourcePreviewWidget::SetPreviewStaticMesh(const StaticMesh *pStaticMesh)
{
    ClearPreview();

    m_ePreviewType = PREVIEW_TYPE_STATICMESH;
    m_Resource.asStaticMesh = pStaticMesh;
    m_Resource.asStaticMesh->AddRef();

    SetupStaticMesh(pStaticMesh, NULL);
    UpdateZoomScale();
    ResetCamera();
}

void EditorResourcePreviewWidget::SetPreviewStaticBlockMesh(const BlockMesh *pStaticBlockMesh)
{
    ClearPreview();

    m_ePreviewType = PREVIEW_TYPE_STATICBLOCKMESH;
    m_Resource.asStaticBlockMesh = pStaticBlockMesh;
    m_Resource.asStaticBlockMesh->AddRef();

    // create the render proxy
    {
        BlockMeshRenderProxy *pRenderProxy = new BlockMeshRenderProxy(0, pStaticBlockMesh, Transform::Identity, 0);

        DebugAssert(m_pResourceRenderProxy == NULL);
        m_pResourceRenderProxy = pRenderProxy;

        m_pRenderWorld->AddRenderable(pRenderProxy);
        FlagForRedraw();
    }

    UpdateZoomScale();
    ResetCamera();
}

void EditorResourcePreviewWidget::SetupStaticMesh(const StaticMesh *pStaticMeshToRender, const Material *pMaterialOverride)
{
    //uint32 i;
    if (pStaticMeshToRender == NULL)
    {
        pStaticMeshToRender = g_pResourceManager->GetStaticMesh("models/engine/unit_sphere");
        if (pStaticMeshToRender == NULL)
            pStaticMeshToRender = g_pResourceManager->GetDefaultStaticMesh();
    }

    StaticMeshRenderProxy *pRenderProxy = new StaticMeshRenderProxy(0, pStaticMeshToRender, Transform::Identity, 0);

    if (pMaterialOverride != NULL)
    {
        for (uint32 i = 0; i < pStaticMeshToRender->GetMaterialCount(); i++)
            pRenderProxy->SetMaterial(i, pMaterialOverride);
    }

    DebugAssert(m_pResourceRenderProxy == NULL);
    m_pResourceRenderProxy = pRenderProxy;

    m_pRenderWorld->AddRenderable(pRenderProxy);
    FlagForRedraw();
}

void EditorResourcePreviewWidget::DrawPreview()
{
    switch (m_ePreviewType)
    {
    case PREVIEW_TYPE_MATERIAL:
    case PREVIEW_TYPE_MATERIALSHADER:
    case PREVIEW_TYPE_STATICMESH:
    case PREVIEW_TYPE_STATICBLOCKMESH:
        {
            // all of these use a world
            m_pWorldRenderer->DrawWorld(m_pRenderWorld, &m_viewParameters, NULL, NULL);
        }
        break;

    case PREVIEW_TYPE_TEXTURE2D:
        {
            // handle texture drawing
            const Texture2D *pTexture2D = m_Resource.asTexture->SafeCast<Texture2D>();
            DebugAssert(pTexture2D != NULL);

            // setup rect
            MINIGUI_RECT drawRect;
            SET_MINIGUI_RECT(&drawRect, 0, m_renderTargetWidth, 0, m_renderTargetHeight);
            m_guiContext.SetAlphaBlendingEnabled(false);
            m_guiContext.DrawTexturedRect(&drawRect, &MINIGUI_UV_RECT::FULL_RECT, pTexture2D);
        }
        break;
    }
}

void EditorResourcePreviewWidget::DrawPreviewOverlays()
{
    SmallString tempStr;

    if (m_pOverlayTextFont == NULL)
    {
        m_pOverlayTextFont = g_pResourceManager->GetFont("resources/engine/fonts/fixedsyse_16");
        if (m_pOverlayTextFont == NULL)
            return;
    }

    m_guiContext.PushManualFlush();

    if (m_ePreviewType != PREVIEW_TYPE_NONE)
    {
        tempStr.Format("'%s'", m_Resource.asResource->GetName().GetCharArray());
        m_guiContext.DrawText(m_pOverlayTextFont, 16, 2, 2, tempStr);

        switch (m_ePreviewType)
        {
        case PREVIEW_TYPE_MATERIAL:
            {

            }
            break;

        case PREVIEW_TYPE_MATERIALSHADER:
            {

            }
            break;

        case PREVIEW_TYPE_STATICMESH:
            {
                float3 meshExtents(m_Resource.asStaticMesh->GetBoundingBox().GetExtents());
                tempStr.Format("Size: %f x %f x %f", meshExtents.x, meshExtents.y, meshExtents.z);
                m_guiContext.DrawText(m_pOverlayTextFont, 16, 2, 18, tempStr);

                tempStr.Format("LODs: %u", m_Resource.asStaticMesh->GetLODCount());
                m_guiContext.DrawText(m_pOverlayTextFont, 16, 2, 34, tempStr);

                tempStr.Format("Batches: %u", m_Resource.asStaticMesh->GetLOD(0)->GetBatchCount());
                m_guiContext.DrawText(m_pOverlayTextFont, 16, 2, 50, tempStr);

                uint32 triangleCount = 0;
                for (uint32 i = 0; i < m_Resource.asStaticMesh->GetLOD(0)->GetBatchCount(); i++)
                    triangleCount += m_Resource.asStaticMesh->GetLOD(0)->GetBatch(i)->NumIndices / 3;

                tempStr.Format("Triangles: %u", triangleCount);
                m_guiContext.DrawText(m_pOverlayTextFont, 16, 2, 66, tempStr);
            }
            break;

        case PREVIEW_TYPE_STATICBLOCKMESH:
            {

            }
            break;

        case PREVIEW_TYPE_TEXTURE2D:
            {

            }
            break;
        }
    }
    else
    {
        m_guiContext.DrawText(m_pOverlayTextFont, 16, 2, 2, m_errorMessage);
    }

    m_guiContext.PopManualFlush();
}

void EditorResourcePreviewWidget::ConnectUIEvents()
{
    //connect(this, SIGNAL(), this, SLOT(OnFocusGained()));
    //connect(this, SIGNAL(), this, SLOT(OnFocusLost()));

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
}

void EditorResourcePreviewWidget::OnFocusGained()
{

}

void EditorResourcePreviewWidget::OnFocusLost()
{

}

void EditorResourcePreviewWidget::OnSwapChainWidgetResized()
{
    uint32 newWidth = (uint32)m_ui->swapChainWidget->size().width();
    uint32 newHeight = (uint32)m_ui->swapChainWidget->size().height();

    // update dimensions
    m_renderTargetWidth = newWidth;
    m_renderTargetHeight = newHeight;

    // kill old swapchain refs
    if (m_pSwapChain != NULL)
    {
        m_pSwapChain->Release();
        m_pSwapChain = NULL;
    }

    // skip full recreation if we can just do the resize
    if (m_bHardwareResourcesCreated && (m_pSwapChain = m_ui->swapChainWidget->GetSwapChain()) != NULL)
    {
        m_pSwapChain->AddRef();
        m_ArcBallCamera.SetPerspectiveAspect((float)newWidth, (float)newHeight);
        m_guiContext.SetViewportDimensions(newWidth, newHeight);
        m_viewParameters.Viewport.Set(0, 0, m_renderTargetWidth, m_renderTargetHeight, 0.0f, 1.0f);
        delete m_pWorldRenderer;
        m_pWorldRenderer = nullptr;
        m_bHardwareResourcesCreated = false;
        FlagForRedraw();
    }
    else
    {
        ReleaseGPUResources();
        FlagForRedraw();
    }
}

void EditorResourcePreviewWidget::OnSwapChainWidgetPaint()
{
    FlagForRedraw();
}

void EditorResourcePreviewWidget::OnSwapChainWidgetKeyboardEvent(const QKeyEvent *pKeyboardEvent)
{

}

void EditorResourcePreviewWidget::OnSwapChainWidgetMouseEvent(const QMouseEvent *pMouseEvent)
{
    if ((pMouseEvent->type() == QEvent::MouseButtonPress && pMouseEvent->button() == Qt::LeftButton) ||
        (pMouseEvent->type() == QEvent::MouseButtonPress && pMouseEvent->button() == Qt::RightButton))
    {
        // if not currently in any state, determine which state to transition to
        if (m_eCurrentState == STATE_NONE)
        {
            m_eCurrentState = STATE_ROTATE_CAMERA;
            FlagForRedraw();
        }
    }
    else if ((pMouseEvent->type() == QEvent::MouseButtonRelease && pMouseEvent->button() == Qt::LeftButton) ||
             (pMouseEvent->type() == QEvent::MouseButtonRelease && pMouseEvent->button() == Qt::RightButton))
    {
        switch (m_eCurrentState)
        {
        case STATE_ROTATE_CAMERA:
            {
                m_eCurrentState = STATE_NONE;
                FlagForRedraw();
            }
            break;
        }
    }
    else if (pMouseEvent->type() == QEvent::MouseMove)
    {
        int2 currentMousePosition(pMouseEvent->x(), pMouseEvent->y());
        int2 lastMousePosition(m_LastMousePosition);
        int2 mousePositionDiff(currentMousePosition - lastMousePosition);
        m_LastMousePosition = currentMousePosition;

        // handle mouse movement based on state
        switch (m_eCurrentState)
        {
        case STATE_NONE:
            {
            }
            break;

        case STATE_ROTATE_CAMERA:
            {
                m_ArcBallCamera.RotateFromMouseMovement(mousePositionDiff.x, mousePositionDiff.y);
                FlagForRedraw();
            }
            break;
        }
    }
}

void EditorResourcePreviewWidget::OnSwapChainWidgetWheelEvent(const QWheelEvent *pWheelEvent)
{
    if (pWheelEvent->orientation() == Qt::Vertical)
    {
        float wheelDelta = (float)pWheelEvent->delta() / 120.0f;
        m_ArcBallCamera.SetEyeDistance(m_ArcBallCamera.GetEyeDistance() + m_fZoomScale * -wheelDelta);
        Log_DevPrintf("new eye distance: %s", StringConverter::FloatToString(m_ArcBallCamera.GetEyeDistance()).GetCharArray());
        FlagForRedraw();
    }
}
