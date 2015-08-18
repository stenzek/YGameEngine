#include "Editor/PrecompiledHeader.h"
#include "Editor/SkeletalAnimationEditor/EditorSkeletalAnimationEditor.h"
#include "Editor/SkeletalAnimationEditor/ui_EditorSkeletalAnimationEditor.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/RenderProxies/SkeletalMeshRenderProxy.h"
#include "Renderer/Renderer.h"
#include "Engine/ResourceManager.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalAnimation.h"
#include "Engine/SkeletalMesh.h"
#include "ResourceCompiler/SkeletalAnimationGenerator.h"
#include "ContentConverter/AssimpStaticMeshImporter.h"
#include "Editor/EditorLightSimulator.h"
#include "Editor/EditorHelpers.h"
#include "Editor/EditorProgressDialog.h"
#include "Editor/EditorResourceSaveDialog.h"
#include "Editor/EditorResourceSelectionDialog.h"
#include "Editor/Editor.h"
Log_SetChannel(EditorSkeletalAnimationEditor);

EditorSkeletalAnimationEditor::EditorSkeletalAnimationEditor()
    : m_ui(new Ui_EditorSkeletalAnimationEditor()),
      m_renderMode(EDITOR_RENDER_MODE_FULLBRIGHT),
      m_viewportFlags(EDITOR_VIEWPORT_FLAG_ENABLE_SHADOWS),
      m_pGenerator(nullptr),
      m_pSkeleton(nullptr),
      m_pPreviewMesh(nullptr),
      m_duration(0.0f),
      m_previewFrameNumber(0),
      m_selectedBoneIndex(Y_UINT32_MAX),
      m_pRenderWorld(new RenderWorld()),
      m_pLightSimulator(new EditorLightSimulator(0, m_pRenderWorld)),
      m_pPreviewMeshRenderProxy(nullptr),
      m_pSwapChain(nullptr),
      m_pWorldRenderer(nullptr),
      m_bHardwareResourcesCreated(false),
      m_bRedrawPending(true),
      m_lastMousePosition(int2::Zero),
      m_currentTool(Tool_Information),
      m_currentState(State_None)
{
    // create ui
    m_ui->CreateUI(this);
    m_ui->UpdateUIForCameraMode(m_viewController.GetCameraMode());
    m_ui->UpdateUIForRenderMode(m_renderMode);
    m_ui->UpdateUIForViewportFlags(m_viewportFlags);
    m_ui->UpdateUIForTool(m_currentTool);

    // connect events
    ConnectUIEvents();

    // fixup dimensions
    m_viewController.SetViewportDimensions(m_ui->swapChainWidget->size().width(), m_ui->swapChainWidget->size().height());
    m_guiContext.SetViewportDimensions(m_ui->swapChainWidget->size().width(), m_ui->swapChainWidget->size().height());

    // setup world
    //m_pCollisionShapePreviewRenderProxy->SetVisibility(false);
    //m_pRenderWorld->AddRenderable(m_pMeshPreviewRenderProxy);
    //m_pRenderWorld->AddRenderable(m_pCollisionShapePreviewRenderProxy);
}

EditorSkeletalAnimationEditor::~EditorSkeletalAnimationEditor()
{

}

void EditorSkeletalAnimationEditor::SetCameraMode(EDITOR_CAMERA_MODE cameraMode)
{
    DebugAssert(cameraMode < EDITOR_CAMERA_MODE_COUNT);
    m_viewController.SetCameraMode(cameraMode);
    m_ui->UpdateUIForCameraMode(cameraMode);
}

void EditorSkeletalAnimationEditor::SetRenderMode(EDITOR_RENDER_MODE renderMode)
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
    m_ui->UpdateUIForRenderMode(renderMode);

    delete m_pWorldRenderer;
    m_pWorldRenderer = EditorHelpers::CreateWorldRendererForRenderMode(renderMode, g_pRenderer->GetMainContext(), m_viewportFlags, m_pSwapChain->GetWidth(), m_pSwapChain->GetHeight());
    FlagForRedraw();

    // update ui
}

void EditorSkeletalAnimationEditor::SetViewportFlag(uint32 flag)
{
    flag &= ~m_viewportFlags;
    if (flag == 0)
        return;

    m_viewportFlags |= flag;
    m_ui->UpdateUIForViewportFlags(m_viewportFlags);
    FlagForRedraw();
}

void EditorSkeletalAnimationEditor::ClearViewportFlag(uint32 flag)
{
    flag &= m_viewportFlags;
    if (flag == 0)
        return;

    m_viewportFlags &= ~flag;
    m_ui->UpdateUIForViewportFlags(m_viewportFlags);
    FlagForRedraw();
}

void EditorSkeletalAnimationEditor::SetCurrentTool(Tool tool)
{
    m_ui->UpdateUIForTool(tool);

    if (m_currentTool == tool)
        return;

    m_currentTool = tool;

    // redraw
    FlagForRedraw();
}

bool EditorSkeletalAnimationEditor::Load(const char *animationName, ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    // find the filename, and open it
    PathString fileName;
    fileName.Format("%s.ska.xml", animationName);
    AutoReleasePtr<ByteStream> pStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pStream == nullptr)
    {
        pProgressCallbacks->DisplayFormattedModalError("Could not open file '%s'", fileName.GetCharArray());
        return false;
    }
    
    SkeletalAnimationGenerator *pGenerator = new SkeletalAnimationGenerator();
    if (!pGenerator->LoadFromXML(fileName, pStream))
    {
        pProgressCallbacks->DisplayFormattedModalError("Could not parse file '%s'", fileName.GetCharArray());
        delete pGenerator;
        return false;
    }

    // load skeleton
    m_pSkeleton = g_pResourceManager->GetSkeleton(pGenerator->GetSkeletonName());
    if (m_pSkeleton == nullptr)
    {
        pProgressCallbacks->DisplayFormattedModalError("Failed to load referenced skeleton '%s'", m_pGenerator->GetSkeletonName().GetCharArray());
        return false;
    }

    // allocate transforms
    m_previewBoneTransforms.Resize(m_pSkeleton->GetBoneCount());

    // close anything
    Close();

    // store names
    m_animationName = animationName;
    m_animationFileName = fileName;

    // swap the pointers, and do allocations
    m_pGenerator = pGenerator;
//     if (!LoadMaterials())
//     {
//         Close();
//         return false;
//     }
// 
//     // create the preview mesh
//     RefreshPreviewMesh();
//     RefreshCollisionShapePreview();

    // update everything
    OnFileNameChanged();
    UpdateHierarchy();
    UpdateInformationTab();
    UpdateTimeDependantVariables();
    SetPreviewFrameNumber(0);

    // load up preview mesh if present from the uncompiled skeleton (if it exists)
    const String &previewMeshName = m_pGenerator->GetPreviewMeshName();
    if (!previewMeshName.IsEmpty())
        SetPreviewMeshName(previewMeshName);

    return true;
}

bool EditorSkeletalAnimationEditor::Save(ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    if (!IsValidAnimation())
    {
        pProgressCallbacks->DisplayError("The mesh is not valid (does not contain either vertices, triangles, materials, batches). It cannot be saved.");
        return false;
    }

    // open stream
    AutoReleasePtr<ByteStream> pStream = g_pVirtualFileSystem->OpenFile(m_animationFileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_STREAMED | BYTESTREAM_OPEN_ATOMIC_UPDATE);
    if (pStream == NULL)
        return false;

    if (!m_pGenerator->SaveToXML(pStream))
    {
        pStream->Discard();
        return false;
    }

    return true;
}

bool EditorSkeletalAnimationEditor::SaveAs(const char *animationName, ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    if (!IsValidAnimation())
    {
        pProgressCallbacks->DisplayError("The mesh is not valid (does not contain either vertices, triangles, materials, batches). It cannot be saved.");
        return false;
    }

    PathString newMeshFileName;
    newMeshFileName.Format("%s.ska.xml", animationName);

    // open stream
    AutoReleasePtr<ByteStream> pStream = g_pVirtualFileSystem->OpenFile(newMeshFileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_STREAMED | BYTESTREAM_OPEN_ATOMIC_UPDATE);
    if (pStream == NULL)
        return false;

    if (!m_pGenerator->SaveToXML(pStream))
    {
        pStream->Discard();
        return false;
    }

    // update names
    m_animationName = animationName;
    m_animationFileName = newMeshFileName;
    OnFileNameChanged();
    return true;
}

void EditorSkeletalAnimationEditor::Close()
{
    if (m_pGenerator == nullptr)
        return;

    m_duration = 0.0f;
    m_previewFrameNumber = 0;
    m_selectedBoneIndex = Y_UINT32_MAX;
    m_frameTimes.Obliterate();

    delete m_pGenerator;
    m_pGenerator = nullptr;

    if (m_pSkeleton != nullptr)
    {
        m_pSkeleton->Release();
        m_pSkeleton = nullptr;
    }

    m_previewBoneTransforms.Obliterate();

    //m_pMeshPreviewRenderProxy->DeleteAllObjects();
}

bool EditorSkeletalAnimationEditor::SetPreviewMeshName(const char *meshName)
{
    const SkeletalMesh *pSkeletalMesh = g_pResourceManager->GetSkeletalMesh(meshName);
    if (pSkeletalMesh == nullptr)
        return false;

    SAFE_RELEASE(m_pPreviewMesh);
    m_pPreviewMesh = pSkeletalMesh;
    m_pPreviewMesh->AddRef();

    if (m_pPreviewMeshRenderProxy != nullptr)
    {
        m_pPreviewMeshRenderProxy->SetSkeletalMesh(m_pPreviewMesh);
    }
    else
    {
        m_pPreviewMeshRenderProxy = new SkeletalMeshRenderProxy(0, m_pPreviewMesh, Transform::Identity, 0);
        m_pRenderWorld->AddRenderable(m_pPreviewMeshRenderProxy);
    }

    UpdateInformationTab();
    UpdatePreviewBoneTransforms();
    return true;
}

void EditorSkeletalAnimationEditor::SetPreviewFrameNumber(uint32 frameNumber)
{
    DebugAssert(frameNumber < m_frameTimes.GetSize());
    if (m_previewFrameNumber == frameNumber)
        return;

    m_previewFrameNumber = frameNumber;
    BlockSignalsForCall(m_ui->scrubberSlider)->setValue((int)frameNumber + 1);

    m_ui->scrubberDockWidget->setWindowTitle(ConvertStringToQString(SmallString::FromFormat("Frame %u (%.3fs)", frameNumber + 1, m_frameTimes[frameNumber])));

    UpdatePreviewBoneTransforms();
}

bool EditorSkeletalAnimationEditor::CreateHardwareResources()
{
    Log_DevPrintf("Creating hardware resources for EditorSkeletalAnimationEditor (%u x %u)", (uint32)m_ui->swapChainWidget->size().width(), (uint32)m_ui->swapChainWidget->size().height());

    // create swap chain
    if (m_pSwapChain == NULL)
    {
        if ((m_pSwapChain = m_ui->swapChainWidget->GetSwapChain()) == NULL)
            return false;

        m_pSwapChain->AddRef();
    }

    // get dimensions
    uint32 swapChainWidth = m_pSwapChain->GetWidth();
    uint32 swapChainHeight = m_pSwapChain->GetHeight();

    // update gui context, camera
    m_guiContext.SetViewportDimensions(swapChainWidth, swapChainHeight);
    m_guiContext.SetGPUContext(GPUContext::GetContextForCurrentThread());
    m_viewController.SetViewportDimensions(swapChainWidth, swapChainHeight);

    // create render context
    if (m_pWorldRenderer == NULL)
        m_pWorldRenderer = EditorHelpers::CreateWorldRendererForRenderMode(m_renderMode, g_pRenderer->GetMainContext(), m_viewportFlags, m_pSwapChain->GetWidth(), m_pSwapChain->GetHeight());

    // cancel any pending draws until the windows are arranged and a paint is requested.
    m_bHardwareResourcesCreated = true;
    return true;
}

void EditorSkeletalAnimationEditor::ReleaseHardwareResources()
{
    m_guiContext.ClearState();

    delete m_pWorldRenderer;
    m_pWorldRenderer = NULL;

    SAFE_RELEASE(m_pSwapChain);
    m_ui->swapChainWidget->DestroySwapChain();
    m_bHardwareResourcesCreated = false;
}

bool EditorSkeletalAnimationEditor::IsValidAnimation() const
{
//     if (m_pGenerator->GetVertexCount() == 0 ||
//         m_pGenerator->GetTriangleCount() == 0 ||
//         m_pGenerator->GetMaterialCount() == 0 ||
//         m_pGenerator->GetBatchCount() == 0 ||
//         m_pGenerator->GetLODCount() == 0)
//     {
//         return false;
//     }

    return true;
}

void EditorSkeletalAnimationEditor::OnFileNameChanged()
{
    // set the material directory to the same directory as the mesh
    PathString importDirectory;
    FileSystem::BuildPathRelativeToFile(importDirectory, m_animationFileName, "", false, true);

    // update the window title
    setWindowTitle(ConvertStringToQString(String::FromFormat("Skeletal Animation Editor - %s", m_animationName.GetCharArray())));
}

void EditorSkeletalAnimationEditor::UpdateTimeDependantVariables()
{
    m_frameTimes.Clear();
    m_pGenerator->GenerateKeyFrameTimeList(&m_frameTimes);
    m_duration = (m_frameTimes.GetSize() > 0) ? m_frameTimes.LastElement() : 0.0f;

    m_ui->toolInformationFrameCount->setText(ConvertStringToQString(StringConverter::UInt32ToString(m_frameTimes.GetSize())));
    m_ui->toolInformationDuration->setText(ConvertStringToQString(TinyString::FromFormat("%f s", m_duration)));

    BlockSignalsForCall(m_ui->scrubberSlider)->setMinimum(1);
    BlockSignalsForCall(m_ui->scrubberSlider)->setMaximum(m_frameTimes.GetSize());
    BlockSignalsForCall(m_ui->toolClipperStartFrameNumber)->setMinimum(1);
    BlockSignalsForCall(m_ui->toolClipperStartFrameNumber)->setMaximum(m_frameTimes.GetSize());
    BlockSignalsForCall(m_ui->toolClipperEndFrameNumber)->setMinimum(1);
    BlockSignalsForCall(m_ui->toolClipperEndFrameNumber)->setMaximum(m_frameTimes.GetSize());
    SetPreviewFrameNumber(0);
}

void EditorSkeletalAnimationEditor::UpdateHierarchyRecursive(const Skeleton::Bone *pBone, QTreeWidgetItem *pParent)
{
    QTreeWidgetItem *pItem;
    if (pParent != nullptr)
        pItem = new QTreeWidgetItem(pParent);
    else
        pItem = new QTreeWidgetItem(m_ui->hierarchyTreeWidget);

    pItem->setText(0, EditorHelpers::ConvertStringToQString(pBone->GetName()));

    if (pParent != nullptr)
        pParent->addChild(pItem);
    else
        m_ui->hierarchyTreeWidget->addTopLevelItem(pItem);

    for (uint32 childIndex = 0; childIndex < pBone->GetChildBoneCount(); childIndex++)
        UpdateHierarchyRecursive(pBone->GetChildBone(childIndex), pItem);
}

void EditorSkeletalAnimationEditor::UpdateHierarchy()
{
    DebugAssert(m_pSkeleton != nullptr);

    m_ui->hierarchyTreeWidget->clear();

    const Skeleton::Bone *pRootBone = m_pSkeleton->GetRootBone();
    UpdateHierarchyRecursive(pRootBone, nullptr);
}

void EditorSkeletalAnimationEditor::UpdatePreviewBoneTransformRecursive(float time, const Skeleton::Bone *pBone, const Transform *pParentBoneTransform)
{
    // get base frame transform
    Transform boneRelativeTransform(pBone->GetRelativeBaseFrameTransform());

    // get track transform
    const SkeletalAnimationGenerator::BoneTrack *pBoneTrack = m_pGenerator->GetBoneTrackByName(pBone->GetName());
    if (pBoneTrack != nullptr)
    {
        SkeletalAnimationGenerator::BoneTrack::KeyFrame keyframe;
        pBoneTrack->InterpolateKeyFrameAtTime(time, &keyframe);
        boneRelativeTransform.Set(keyframe.GetPosition(), keyframe.GetRotation(), keyframe.GetScale());
    }

    // factor into account the parent transform
    Transform boneAbsoluteTransform((pParentBoneTransform != nullptr) ? Transform::ConcatenateTransforms(boneRelativeTransform, *pParentBoneTransform) : boneRelativeTransform);

    // store transform
    m_previewBoneTransforms[pBone->GetIndex()] = boneAbsoluteTransform;

    // update preview mesh
    if (m_pPreviewMesh != nullptr)
    {
        for (uint32 meshBoneIndex = 0; meshBoneIndex < m_pPreviewMesh->GetBoneCount(); meshBoneIndex++)
        {
            if (m_pPreviewMesh->GetBone(meshBoneIndex)->SkeletonBoneIndex == pBone->GetIndex())
            {
                m_pPreviewMeshRenderProxy->SetBoneTransforms(meshBoneIndex, 1, &boneAbsoluteTransform);
                break;
            }
        }
    }

    // handle any children
    for (uint32 childIndex = 0; childIndex < pBone->GetChildBoneCount(); childIndex++)
        UpdatePreviewBoneTransformRecursive(time, pBone->GetChildBone(childIndex), &boneAbsoluteTransform);
}

void EditorSkeletalAnimationEditor::UpdatePreviewBoneTransforms()
{
    float time = m_frameTimes[m_previewFrameNumber];

    // calculate from the root bone upwards
    const Skeleton::Bone *pRootBone = m_pSkeleton->GetRootBone();
    UpdatePreviewBoneTransformRecursive(time, pRootBone, nullptr);

    // redraw
    FlagForRedraw();
}

void EditorSkeletalAnimationEditor::UpdateInformationTab()
{
    m_ui->toolInformationSkeletonName->setText(ConvertStringToQString(m_pSkeleton->GetName()));
    m_ui->toolInformationBoneTrackCount->setText(ConvertStringToQString(StringConverter::UInt32ToString(m_pGenerator->GetBoneTrackCount())));
    m_ui->toolInformationRootMotion->setText((m_pGenerator->GetRootMotionTrack() != nullptr) ? tr("Yes") : tr("No"));
    m_ui->toolInformationPreviewMesh->setText(ConvertStringToQString(SmallString::FromFormat("<a href=\"select\">%s</a>", (m_pPreviewMesh != nullptr) ? m_pPreviewMesh->GetName().GetCharArray() : "Select...")));
}

void EditorSkeletalAnimationEditor::Draw()
{
    // create hardware resources
    if (!m_bHardwareResourcesCreated && !CreateHardwareResources())
        return;

    GPUContext *pGPUContext = g_pRenderer->GetMainContext();

    // clear it
    pGPUContext->SetOutputBuffer(m_pSwapChain);
    pGPUContext->SetRenderTargets(0, nullptr, nullptr);
    pGPUContext->SetViewport(m_viewController.GetViewport());
    pGPUContext->ClearTargets(true, true, true);

    // setup 3d state
    pGPUContext->GetConstants()->SetFromCamera(m_viewController.GetCamera(), true);

    // invoke draws
    DrawGrid();
    DrawView();

    // bias the projection matrix slightly FIXME
    pGPUContext->GetConstants()->SetCameraProjectionMatrix(m_viewController.GetCamera().GetBiasedProjectionMatrix(0.01f), true);
    DrawOverlays(pGPUContext, &m_guiContext);

    // clear state
    pGPUContext->ClearState(true, true, true, true);

    // present
    m_pSwapChain->SwapBuffers();

    // clear pending flag
    m_bRedrawPending = false;
}

void EditorSkeletalAnimationEditor::DrawGrid()
{
    // draw a grid of 2*the mesh size
    //float gridSizeX = Max(16.0f, Max(Math::Abs(m_pGenerator->GetBoundingBox().GetMinBounds().x), Math::Abs(m_pGenerator->GetBoundingBox().GetMaxBounds().x)) * 2.0f);
    //float gridSizeY = Max(16.0f, Max(Math::Abs(m_pGenerator->GetBoundingBox().GetMinBounds().y), Math::Abs(m_pGenerator->GetBoundingBox().GetMaxBounds().y)) * 2.0f);
    float gridSizeX = 16.0f;
    float gridSizeY = 16.0f;
    m_guiContext.Draw3DGrid(float3(-gridSizeX, -gridSizeY, 0.0f), float3(gridSizeX, gridSizeY, 0.0f), float3(1.0f, 1.0f, 1.0f), MAKE_COLOR_R8G8B8A8_UNORM(102, 102, 102, 255));
}

void EditorSkeletalAnimationEditor::DrawView()
{
    m_pWorldRenderer->DrawWorld(m_pRenderWorld, m_viewController.GetViewParameters(), NULL, NULL, NULL);
}

void EditorSkeletalAnimationEditor::DrawOverlays(GPUContext *pGPUContext, MiniGUIContext *pGUIContext)
{
    // draw the skeleton tree
    {
        // for each bone
        for (uint32 i = 0; i < m_pSkeleton->GetBoneCount(); i++)
        {
            const Skeleton::Bone *pParentBone = m_pSkeleton->GetBoneByIndex(i);
            const Transform &parentBoneTransform = m_previewBoneTransforms[i];
            float3 lineSource(parentBoneTransform.TransformPoint(float3::Zero));

            // for each bone that has this bone as a parent
            for (uint32 j = 0; j < m_pSkeleton->GetBoneCount(); j++)
            {
                const Skeleton::Bone *pChildBone = m_pSkeleton->GetBoneByIndex(j);
                if (pChildBone->GetParentBone() == pParentBone)
                {
                    const Transform &childBoneTransform = m_previewBoneTransforms[j];

                    // draw a line from source to destination
                    float3 lineDestination(childBoneTransform.TransformPoint(float3::Zero));
                    pGUIContext->Draw3DLineWidth(lineSource, lineDestination, (m_selectedBoneIndex == pChildBone->GetIndex()) ? MAKE_COLOR_R8G8B8A8_UNORM(240, 240, 0, 255) : MAKE_COLOR_R8G8B8A8_UNORM(240, 240, 240, 255), 2.0f);

                    // and the axes
                    float3 axisOrigin(lineDestination);
                    float3 xAxisPos(childBoneTransform.TransformPoint(float3::UnitX * 0.5f));
                    float3 yAxisPos(childBoneTransform.TransformPoint(float3::UnitY * 0.5f));
                    float3 zAxisPos(childBoneTransform.TransformPoint(float3::UnitZ * 0.5f));
                    pGUIContext->Draw3DLineWidth(axisOrigin, xAxisPos, MAKE_COLOR_R8G8B8A8_UNORM(255, 0, 0, 255), 2.0f);
                    pGUIContext->Draw3DLineWidth(axisOrigin, yAxisPos, MAKE_COLOR_R8G8B8A8_UNORM(0, 255, 0, 255), 2.0f);
                    pGUIContext->Draw3DLineWidth(axisOrigin, zAxisPos, MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 255, 255), 2.0f);
                }
            }
        }
    }
}

void EditorSkeletalAnimationEditor::OnFrameExecutionTriggered(float timeSinceLastFrame)
{
    // don't do anything if we haven't loaded
    if (m_pGenerator == nullptr)
        return;

    // update camera
    m_viewController.Update(timeSinceLastFrame);

    // force draw if requested by camera
    if (m_viewController.IsChanged())
        FlagForRedraw();

    // redraw if required
    if (m_bRedrawPending)
        Draw();
}

void EditorSkeletalAnimationEditor::ConnectUIEvents()
{
    // actions
    connect(m_ui->actionOpen, SIGNAL(triggered()), this, SLOT(OnActionOpenMeshClicked()));
    connect(m_ui->actionSave, SIGNAL(triggered()), this, SLOT(OnActionSaveMeshClicked()));
    connect(m_ui->actionSaveAs, SIGNAL(triggered()), this, SLOT(OnActionSaveMeshAsClicked()));
    connect(m_ui->actionClose, SIGNAL(triggered()), this, SLOT(OnActionCloseClicked()));
    connect(m_ui->actionCameraPerspective, SIGNAL(triggered(bool)), this, SLOT(OnActionCameraPerspectiveTriggered(bool)));
    connect(m_ui->actionCameraArcball, SIGNAL(triggered(bool)), this, SLOT(OnActionCameraArcballTriggered(bool)));
    connect(m_ui->actionCameraIsometric, SIGNAL(triggered(bool)), this, SLOT(OnActionCameraIsometricTriggered(bool)));
    connect(m_ui->actionViewWireframe, SIGNAL(triggered(bool)), this, SLOT(OnActionViewWireframeTriggered(bool)));
    connect(m_ui->actionViewUnlit, SIGNAL(triggered(bool)), this, SLOT(OnActionViewUnlitTriggered(bool)));
    connect(m_ui->actionViewLit, SIGNAL(triggered(bool)), this, SLOT(OnActionViewLitTriggered(bool)));
    connect(m_ui->actionViewFlagShadows, SIGNAL(triggered(bool)), this, SLOT(OnActionViewFlagShadowsTriggered(bool)));
    connect(m_ui->actionViewFlagWireframeOverlay, SIGNAL(triggered(bool)), this, SLOT(OnActionViewFlagWireframeOverlayTriggered(bool)));
    connect(m_ui->actionToolInformation, SIGNAL(triggered(bool)), this, SLOT(OnActionToolInformationTriggered(bool)));
    connect(m_ui->actionToolBoneTrack, SIGNAL(triggered(bool)), this, SLOT(OnActionToolBoneTrackTriggered(bool)));
    connect(m_ui->actionToolRootMotion, SIGNAL(triggered(bool)), this, SLOT(OnActionToolRootMotionTriggered(bool)));
    connect(m_ui->actionToolClipper, SIGNAL(triggered(bool)), this, SLOT(OnActionToolClipperTriggered(bool)));
    connect(m_ui->actionToolLightManipulator, SIGNAL(triggered(bool)), this, SLOT(OnActionToolLightManipulatorTriggered(bool)));

    // scrubber
    connect(m_ui->scrubberSlider, SIGNAL(valueChanged(int)), this, SLOT(OnScrubberValueChanged(int)));

    // hierarchy
    connect(m_ui->hierarchyTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(OnHierarchyItemClicked(QTreeWidgetItem *, int)));

    // information
    connect(m_ui->toolInformationPreviewMesh, SIGNAL(linkActivated(const QString &)), this, SLOT(OnInformationPreviewMeshLinkActivated(const QString &)));

    // clipper
    connect(m_ui->toolClipperSetStartFrameNumber, SIGNAL(clicked()), this, SLOT(OnClipperSetStartFrameNumberClicked()));
    connect(m_ui->toolClipperSetEndFrameNumber, SIGNAL(clicked()), this, SLOT(OnClipperSetEndFrameNumberClicked()));
    connect(m_ui->toolClipperClip, SIGNAL(clicked()), this, SLOT(OnClipperClipClicked()));

    // swapchain
    connect(m_ui->swapChainWidget, SIGNAL(ResizedEvent()), this, SLOT(OnSwapChainWidgetResized()));
    connect(m_ui->swapChainWidget, SIGNAL(PaintEvent()), this, SLOT(OnSwapChainWidgetPaint()));
    connect(m_ui->swapChainWidget, SIGNAL(KeyboardEvent(const QKeyEvent *)), this, SLOT(OnSwapChainWidgetKeyboardEvent(const QKeyEvent *)));
    connect(m_ui->swapChainWidget, SIGNAL(MouseEvent(const QMouseEvent *)), this, SLOT(OnSwapChainWidgetMouseEvent(const QMouseEvent *)));
    connect(m_ui->swapChainWidget, SIGNAL(WheelEvent(const QWheelEvent *)), this, SLOT(OnSwapChainWidgetWheelEvent(const QWheelEvent *)));
    connect(m_ui->swapChainWidget, SIGNAL(DropEvent(QDropEvent *)), this, SLOT(OnSwapChainWidgetDropEvent(QDropEvent *)));

    // frame event
    connect(g_pEditor, SIGNAL(OnFrameExecution(float)), this, SLOT(OnFrameExecutionTriggered(float)));
}

void EditorSkeletalAnimationEditor::closeEvent(QCloseEvent *pCloseEvent)
{

}

void EditorSkeletalAnimationEditor::OnActionOpenMeshClicked()
{

}

void EditorSkeletalAnimationEditor::OnActionSaveMeshClicked()
{
    EditorProgressDialog progressDialog(this);
    progressDialog.show();

    Save(&progressDialog);
}

void EditorSkeletalAnimationEditor::OnActionSaveMeshAsClicked()
{
    EditorResourceSaveDialog saveDialog(this, OBJECT_TYPEINFO(SkeletalAnimation));
    if (saveDialog.exec())
    {
        String newMeshName(saveDialog.GetReturnValueResourceName());

        // attempt save
        EditorProgressDialog progressDialog(this);
        progressDialog.show();
        SaveAs(newMeshName, &progressDialog);
    }
}

void EditorSkeletalAnimationEditor::OnActionCloseClicked()
{

}

void EditorSkeletalAnimationEditor::OnInformationPreviewMeshLinkActivated(const QString &link)
{
    EditorResourceSelectionDialog selectionDialog(this);
    selectionDialog.AddResourceFilter(OBJECT_TYPEINFO(SkeletalMesh));

    if (selectionDialog.exec())
        SetPreviewMeshName(selectionDialog.GetReturnValueResourceName());
}

void EditorSkeletalAnimationEditor::OnHierarchyItemClicked(QTreeWidgetItem *pItem, int column)
{
    const Skeleton::Bone *pBone = m_pSkeleton->GetBoneByName(ConvertQStringToString(pItem->text(0)));
    if (pBone == nullptr)
    {
        Log_DevPrintf("Clear selected bone");
        m_selectedBoneIndex = Y_UINT32_MAX;
    }
    else
    {
        Log_DevPrintf("Set selected bone to %u - %s", pBone->GetIndex(), pBone->GetName().GetCharArray());
        m_selectedBoneIndex = pBone->GetIndex();
    }

    FlagForRedraw();
}

void EditorSkeletalAnimationEditor::OnScrubberValueChanged(int value)
{
    DebugAssert(value > 0);
    SetPreviewFrameNumber(value - 1);
}

void EditorSkeletalAnimationEditor::OnClipperSetStartFrameNumberClicked()
{
    m_ui->toolClipperStartFrameNumber->setValue(m_previewFrameNumber + 1);
}

void EditorSkeletalAnimationEditor::OnClipperSetEndFrameNumberClicked()
{
    m_ui->toolClipperEndFrameNumber->setValue(m_previewFrameNumber + 1);
}

void EditorSkeletalAnimationEditor::OnClipperClipClicked()
{
    int32 startFrameNumber = m_ui->toolClipperStartFrameNumber->value() - 1;
    int32 endFrameNumber = m_ui->toolClipperEndFrameNumber->value() - 1;
    if (startFrameNumber < 0 || endFrameNumber < 0 || startFrameNumber > endFrameNumber)
    {
        QMessageBox::critical(this, tr("Invalid range"), tr("Invalid clip range"));
        return;
    }

    m_pGenerator->ClipAnimation(m_frameTimes[startFrameNumber], m_frameTimes[endFrameNumber]);
    UpdateTimeDependantVariables();
}

void EditorSkeletalAnimationEditor::OnSwapChainWidgetResized()
{
    // skip full recreation if we can just do the resize
    SAFE_RELEASE(m_pSwapChain);
    delete m_pWorldRenderer;
    m_pWorldRenderer = nullptr;
    m_bHardwareResourcesCreated = false;

    // flag for redraw
    FlagForRedraw();
}

void EditorSkeletalAnimationEditor::OnSwapChainWidgetPaint()
{
    FlagForRedraw();
}

void EditorSkeletalAnimationEditor::OnSwapChainWidgetKeyboardEvent(const QKeyEvent *pKeyboardEvent)
{
    // pass to camera
    m_viewController.HandleKeyboardEvent(pKeyboardEvent);
}

void EditorSkeletalAnimationEditor::OnSwapChainWidgetMouseEvent(const QMouseEvent *pMouseEvent)
{
    if (pMouseEvent->type() == QEvent::MouseButtonPress && pMouseEvent->button() == Qt::LeftButton)
    {
        if (m_currentState == State_None)
        {

        }
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonRelease && pMouseEvent->button() == Qt::LeftButton)
    {
        //switch (m_currentState)
        //{
        //}
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonPress && pMouseEvent->button() == Qt::RightButton)
    {
        // right mouse button can enter a viewport state. if we are not in a state, determine which state to enter.
        if (m_currentState == State_None)
        {
            // right mouse button enters camera rotation state
            m_currentState = State_RotateCamera;
            return;
        }
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonRelease && pMouseEvent->button() == Qt::RightButton)
    {
        // if in camera rotation state, exit it
        if (m_currentState == State_RotateCamera)
        {
            m_currentState = State_None;
            return;
        }
    }
    else if (pMouseEvent->type() == QEvent::MouseMove)
    {
        int2 currentMousePosition(pMouseEvent->x(), pMouseEvent->y());
        int2 lastMousePosition(m_lastMousePosition);
        int2 mousePositionDiff(currentMousePosition - lastMousePosition);
        m_lastMousePosition = currentMousePosition;

        switch (m_currentState)
        {
        case State_RotateCamera:
            m_viewController.RotateFromMousePosition(mousePositionDiff);
            FlagForRedraw();
            break;
        }
    }
}

void EditorSkeletalAnimationEditor::OnSwapChainWidgetWheelEvent(const QWheelEvent *pWheelEvent)
{

}

void EditorSkeletalAnimationEditor::OnSwapChainWidgetGainedFocusEvent()
{

}
