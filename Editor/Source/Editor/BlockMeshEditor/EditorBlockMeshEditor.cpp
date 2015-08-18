#include "Editor/PrecompiledHeader.h"
#include "Editor/BlockMeshEditor/EditorBlockMeshEditor.h"
#include "Editor/EditorRendererSwapChainWidget.h"
#include "Editor/EditorResourcePreviewGenerator.h"
#include "Editor/EditorProgressDialog.h"
#include "Editor/EditorResourceSaveDialog.h"
#include "Editor/Editor.h"
#include "Editor/EditorHelpers.h"
#include "Editor/EditorBlockVolumeRenderProxy.h"
#include "Editor/EditorLightSimulator.h"
#include "Engine/BlockMeshBuilder.h"
#include "Engine/BlockMeshUtilities.h"
#include "Renderer/VertexFactories/BlockMeshVertexFactory.h"
#include "Renderer/RenderWorld.h"
Log_SetChannel(EditorBlockMeshEditor);

// ui - must come last
#include "Editor/BlockMeshEditor/ui_EditorBlockMeshEditor.h"

static const uint32 BLOCK_TYPE_ICON_SIZE = 32;
static const uint32 SELECTED_BLOCKS_TINT_COLOR = MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 100, 100);
static const uint32 PREVIEW_BLOCK_TINT_COLOR = MAKE_COLOR_R8G8B8A8_UNORM(200, 200, 200, 200);

EditorBlockMeshEditor::EditorBlockMeshEditor()
    : QMainWindow(nullptr, 0),
      m_ui(new Ui_EditorBlockMeshEditor()),
      m_pBlockList(nullptr),
      m_pGenerator(nullptr),
      m_pAvailableBlockTypes(nullptr),
      m_nAvailableBlockTypes(0),
      m_renderMode(EDITOR_RENDER_MODE_FULLBRIGHT),
      m_viewportFlags(EDITOR_VIEWPORT_FLAG_ENABLE_SHADOWS),
      m_pSwapChain(nullptr),
      m_pWorldRenderer(nullptr),
      m_bHardwareResourcesCreated(false),
      m_bRedrawPending(false),
      m_pRenderWorld(nullptr),
      m_pLightSimulator(nullptr),
      m_pMeshRenderProxy(nullptr),
      m_pSelectedRenderProxy(nullptr),
      m_pPreviewRenderProxy(nullptr),
      m_currentLOD(Y_UINT32_MAX),
      m_eCurrentWidget(WIDGET_COUNT),
      m_MouseOverBlock(int3::Zero),
      m_bMouseOverBlockSet(false),
      m_iMouseOverBlockFaceIndex(CUBE_FACE_COUNT),
      m_bMouseOverPlaceBlockSet(false),
      m_MouseOverPlaceBlock(int3::Zero),
      m_iPlaceBlockToolBlockType(0),
      m_iSelectedAreaCount(0),
      m_eCurrentState(STATE_NONE),
      m_iStateData(0),
      m_lastMousePosition(int2::Zero)
{
    // init view controller
    m_viewController.SetCameraMode(EDITOR_CAMERA_MODE_PERSPECTIVE);

    // create ui
    m_ui->CreateUI(this);
    m_ui->UpdateUIForCameraMode(m_viewController.GetCameraMode());
    m_ui->UpdateUIForRenderMode(m_renderMode);
    m_ui->UpdateUIForViewportFlags(m_viewportFlags);

    // add this as a tracked main window
    g_pEditor->AddMainWindow(this);
}

EditorBlockMeshEditor::~EditorBlockMeshEditor()
{
    // force close can bypass this
    if (m_pGenerator != NULL)
        Deinitialize();

    // remove tracked main window
    g_pEditor->RemoveMainWindow(this);
    
    // delete ui
    delete m_ui;
}

void EditorBlockMeshEditor::SetCameraMode(EDITOR_CAMERA_MODE cameraMode)
{
    DebugAssert(cameraMode < EDITOR_CAMERA_MODE_COUNT);
    m_viewController.SetCameraMode(cameraMode);
    m_ui->UpdateUIForCameraMode(cameraMode);
}

void EditorBlockMeshEditor::SetRenderMode(EDITOR_RENDER_MODE renderMode)
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

void EditorBlockMeshEditor::SetViewportFlag(uint32 flag)
{
    flag &= ~m_viewportFlags;
    if (flag == 0)
        return;

    m_viewportFlags |= flag;
    m_ui->UpdateUIForViewportFlags(m_viewportFlags);
    FlagForRedraw();
}

void EditorBlockMeshEditor::ClearViewportFlag(uint32 flag)
{
    flag &= m_viewportFlags;
    if (flag == 0)
        return;

    m_viewportFlags &= ~flag;
    m_ui->UpdateUIForViewportFlags(m_viewportFlags);
    FlagForRedraw();
}

bool EditorBlockMeshEditor::Create(const char *meshName, const BlockPalette *pBlockList, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    DebugAssert(m_pGenerator == NULL);

    m_meshName = meshName;
    m_meshFileName.Format("%s.blm.xml", meshName);

    // create the generator
    m_pGenerator = new BlockMeshGenerator();
    if (!m_pGenerator->Create(pBlockList))
        return false;

    m_pBlockList = pBlockList;
    m_pBlockList->AddRef();

    return Initialize(pProgressCallbacks);
}

bool EditorBlockMeshEditor::Load(const char *meshName, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    DebugAssert(m_pGenerator == NULL);

    m_meshFileName.Format("%s.blm.xml", meshName);
    if (!g_pVirtualFileSystem->GetFileName(m_meshFileName))
        return false;

    AutoReleasePtr<ByteStream> pStream = g_pVirtualFileSystem->OpenFile(m_meshFileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pStream == NULL)
        return false;

    // update mesh name
    m_meshName = m_meshFileName.SubString(0, -8);

    // create the generator
    m_pGenerator = new BlockMeshGenerator();
    if (!m_pGenerator->LoadFromXML(m_meshFileName, pStream))
        return false;

    // init blocklist
    m_pBlockList = m_pGenerator->GetBlockList();
    m_pBlockList->AddRef();

    // init everything else
    return Initialize(pProgressCallbacks);
}

bool EditorBlockMeshEditor::Save(ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    DebugAssert(m_pGenerator != NULL);

    // open stream
    AutoReleasePtr<ByteStream> pStream = g_pVirtualFileSystem->OpenFile(m_meshFileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_CREATE_PATH | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_STREAMED | BYTESTREAM_OPEN_ATOMIC_UPDATE);
    if (pStream == NULL)
        return false;

    if (!m_pGenerator->SaveToXML(pStream))
    {
        pStream->Discard();
        return false;
    }

    return true;
}

bool EditorBlockMeshEditor::SaveAs(const char *meshName, ProgressCallbacks *pProgressCallbacks /* = ProgressCallbacks::NullProgressCallback */)
{
    DebugAssert(m_pGenerator != NULL);

    PathString newMeshFileName;
    newMeshFileName.Format("%s.blm.xml", meshName);

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
    m_meshName = meshName;
    m_meshFileName = newMeshFileName;

    // update ui
    m_ui->UpdateUIForOpenMesh(m_meshName);
    return true;
}

bool EditorBlockMeshEditor::IsBlockSelected(const int3 &coords)
{
    uint32 i;
    for (i = 0; i < m_SelectedBlocks.GetSize(); i++)
    {
        if (m_SelectedBlocks[i] == coords)
            return true;
    }

    return false;
}

void EditorBlockMeshEditor::SelectBlock(const int3 &coords)
{
    // has to be in range
    if (coords.AnyLess(m_pGenerator->GetMeshMinCoordinates(m_currentLOD)) || coords.AnyGreater(m_pGenerator->GetMeshMaxCoordinates(m_currentLOD)))
        return;

    // does it actually exist?
    BlockVolumeBlockType blockType = m_pGenerator->GetBlock(m_currentLOD, coords);
    if (blockType == 0)
        return;

    // already selected?
    uint32 i;
    for (i = 0; i < m_SelectedBlocks.GetSize(); i++)
    {
        if (m_SelectedBlocks[i] == coords)
            return;
    }

    // add to list
    m_SelectedBlocks.Add(coords);

    // remove blocks from the volume mesh
    // add blocks to the selected mesh
    m_meshVolume.SetBlock(coords, 0);
    m_selectedVolume.SetBlock(coords, blockType);
    UpdateMeshView();
    UpdateSelectedBlocksView();
}

void EditorBlockMeshEditor::DeselectBlock(const int3 &coords)
{
    uint32 i;
    for (i = 0; i < m_SelectedBlocks.GetSize(); i++)
    {
        if (m_SelectedBlocks[i] == coords)
            break;
    }
    if (i == m_SelectedBlocks.GetSize())
        return;

    m_SelectedBlocks.OrderedRemove(i);

    // get value
    BlockVolumeBlockType blockType = m_pGenerator->GetBlock(m_currentLOD, coords);

    // update meshes
    m_meshVolume.SetBlock(coords, blockType);
    m_selectedVolume.SetBlock(coords, 0);
    UpdateMeshView();
    UpdateSelectedBlocksView();
}

void EditorBlockMeshEditor::DeselectAllBlocks()
{
    m_SelectedBlocks.Clear();

    // fast path for this, just reset the view mesh
    m_meshVolume = m_pGenerator->GetMeshVolume(m_currentLOD);
    
    // clear the selection mesh
    m_selectedVolume.Clear();

    // update views
    UpdateMeshView();
    UpdateSelectedBlocksView();
}

void EditorBlockMeshEditor::ExpandSelectionToHeight()
{
    /*
    if (m_iSelectedAreaCount == 2)
    {
        m_SelectedArea[0].z = m_pGenerator->GetMinCoordinates().z;
        m_SelectedArea[1].z = m_pGenerator->GetMaxCoordinates().z;
        FlagForRedraw();
    }
    */
}

void EditorBlockMeshEditor::ExpandSelection(const int3 &direction)
{
    /*
    if (m_iSelectedAreaCount == 2)
    {
        if (direction.x < 0)
            m_SelectedArea[0].x += direction.x;
        else if (direction.x > 0)
            m_SelectedArea[1].x += direction.x;

        if (direction.y < 0)
            m_SelectedArea[0].y += direction.y;
        else if (direction.y > 0)
            m_SelectedArea[1].y += direction.y;

        if (direction.z < 0)
            m_SelectedArea[0].z += direction.z;
        else if (direction.z > 0)
            m_SelectedArea[1].z += direction.z;

        m_SelectedArea[0] = m_SelectedArea[0].Max(m_pGenerator->GetMinCoordinates());
        m_SelectedArea[1] = m_SelectedArea[1].Min(m_pGenerator->GetMaxCoordinates());
        FlagForRedraw();
    }*/
}

void EditorBlockMeshEditor::ClearSelectedArea()
{
    m_SelectedArea[0].SetZero();
    m_SelectedArea[1].SetZero();
    m_iSelectedAreaCount = 0;
    FlagForRedraw();
}

void EditorBlockMeshEditor::MoveSelectedBlocks(const int3 &direction)
{
    /*
    int3 oldMinCoordinates = m_pGenerator->GetMinCoordinates();
    int3 oldMaxCoordinates = m_pGenerator->GetMaxCoordinates();

    if (m_eCurrentWidget == WIDGET_SELECT_BLOCKS)
    {
        uint32 i;
        for (i = 0; i < m_SelectedBlocks.GetSize(); i++)
            m_pGenerator->MoveBlock(m_SelectedBlocks[i], direction);
    }
    else if (m_eCurrentWidget == WIDGET_SELECT_AREA && m_iSelectedAreaCount == 2)
    {
        m_pGenerator->MoveBlocks(m_SelectedArea[0], m_SelectedArea[1], direction);
    }

    int3 newMinCoordinates = m_pGenerator->GetMinCoordinates();
    int3 newMaxCoordinates = m_pGenerator->GetMaxCoordinates();

    if (newMinCoordinates != oldMinCoordinates || newMaxCoordinates != oldMaxCoordinates)
    {
        m_pMeshPreviewEntity->Resize(m_pGenerator->GetMinCoordinates(), m_pGenerator->GetMaxCoordinates());
        m_pCallbacks->OnMeshResized(newMinCoordinates, newMaxCoordinates);
    }

    m_pMeshPreviewEntity->CopyBlockData(m_pGenerator->GetBlockData());
    FlagForRedraw();*/
}

void EditorBlockMeshEditor::SetCurrentLOD(uint32 LOD)
{
    if (m_currentLOD == LOD)
        return;

    if (m_SelectedBlocks.GetSize() > 0)
        DeselectAllBlocks();

    DebugAssert(LOD < m_pGenerator->GetLODCount());
    m_currentLOD = LOD;
    m_ui->UpdateUIForCurrentLOD(LOD);

    // copy lod to current mesh
    m_meshVolume = m_pGenerator->GetMeshVolume(LOD);
    m_selectedVolume.Resize(m_meshVolume.GetMinCoordinates(), m_meshVolume.GetMaxCoordinates());
    m_selectedVolume.Clear();
    m_ui->UpdateUIForMeshSize(m_meshVolume.GetWidth(), m_meshVolume.GetLength(), m_meshVolume.GetHeight());

    // update views
    UpdateMeshView();
}

void EditorBlockMeshEditor::SetWidget(WIDGET t)
{
    if (m_eCurrentWidget == t)
        return;

    // remove anything from current
    switch (m_eCurrentWidget)
    {
    case WIDGET_PLACE_BLOCKS:
        m_pPreviewRenderProxy->ClearMesh();
        break;
    }

    // set new
    m_eCurrentWidget = t;
    m_ui->UpdateUIForWidget(t);

    // update new
    switch (m_eCurrentWidget)
    {
    case WIDGET_SELECT_BLOCKS:
        UpdateMouseBlockCoordinates();
        break;

    case WIDGET_PLACE_BLOCKS:
        {
            UpdateMouseBlockCoordinates();
            
            // refresh all the ui junk
            BlockVolumeBlockType oldPlaceValue = m_iPlaceBlockToolBlockType;
            m_iPlaceBlockToolBlockType = BLOCK_MESH_MAX_BLOCK_TYPES;
            SetPlaceBlockWidgetBlockType(oldPlaceValue);
        }
        break;

    case WIDGET_DELETE_BLOCKS:
        UpdateMouseBlockCoordinates();
        break;
    }
    
    FlagForRedraw();
}

void EditorBlockMeshEditor::SetPlaceBlockWidgetBlockType(BlockVolumeBlockType t)
{
    if (m_iPlaceBlockToolBlockType == t)
        return;

    m_iPlaceBlockToolBlockType = t; 
    UpdatePreviewMesh(t);

    for (uint32 i = 0; i < m_nAvailableBlockTypes; i++)
    {
        if (m_pAvailableBlockTypes[i].pBlockType->BlockTypeIndex == (uint32)t)
        {
            m_ui->UpdateUISelectedPlaceBlockType(m_pAvailableBlockTypes, m_nAvailableBlockTypes, i, m_eCurrentWidget);
            break;
        }
    }
}

void EditorBlockMeshEditor::SetNextPlaceBlockWidgetBlockType()
{
    uint32 currentIndex;
    for (currentIndex = 0; currentIndex < m_nAvailableBlockTypes; currentIndex++)
    {
        if (m_pAvailableBlockTypes[currentIndex].pBlockType->BlockTypeIndex == (uint32)m_iPlaceBlockToolBlockType)
            break;
    }

    if (currentIndex == m_nAvailableBlockTypes)
        return;

    currentIndex++;
    currentIndex %= m_nAvailableBlockTypes;
    SetPlaceBlockWidgetBlockType((BlockVolumeBlockType)m_pAvailableBlockTypes[currentIndex].pBlockType->BlockTypeIndex);
}

void EditorBlockMeshEditor::SetPreviousPlaceBlockWidgetBlockType()
{
    uint32 currentIndex;
    for (currentIndex = 0; currentIndex < m_nAvailableBlockTypes; currentIndex++)
    {
        if (m_pAvailableBlockTypes[currentIndex].pBlockType->BlockTypeIndex == (uint32)m_iPlaceBlockToolBlockType)
            break;
    }

    if (currentIndex == m_nAvailableBlockTypes)
        return;

    if (currentIndex == 0)
        currentIndex = m_nAvailableBlockTypes - 1;
    else
        currentIndex--;

    SetPlaceBlockWidgetBlockType((BlockVolumeBlockType)m_pAvailableBlockTypes[currentIndex].pBlockType->BlockTypeIndex);
}

bool EditorBlockMeshEditor::CreateHardwareResources()
{
    Log_DevPrintf("Creating hardware resources for EditorStaticBlockMeshEditor (%u x %u)", (uint32)m_ui->swapChainWidget->size().width(), (uint32)m_ui->swapChainWidget->size().height());

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
    m_viewController.SetViewportDimensions(swapChainWidth, swapChainHeight);
    m_guiContext.SetViewportDimensions(swapChainWidth, swapChainHeight);

    // create render context
    if (m_pWorldRenderer == NULL)
        m_pWorldRenderer = EditorHelpers::CreateWorldRendererForRenderMode(m_renderMode, g_pRenderer->GetMainContext(), m_viewportFlags, m_pSwapChain->GetWidth(), m_pSwapChain->GetHeight());
    
    // cancel any pending draws until the windows are arranged and a paint is requested.
    m_bHardwareResourcesCreated = true;
    return true;
}

void EditorBlockMeshEditor::ReleaseHardwareResources()
{
    m_guiContext.ClearState();

    delete m_pWorldRenderer;
    m_pWorldRenderer = NULL;

    SAFE_RELEASE(m_pSwapChain);
    m_ui->swapChainWidget->DestroySwapChain();
    m_bHardwareResourcesCreated = false;
}

void EditorBlockMeshEditor::OnFrameExecutionTriggered(float timeSinceLastFrame)
{
    // update camera
    m_viewController.Update(timeSinceLastFrame);

    // force draw if requested by camera
    if (m_viewController.IsChanged())
        FlagForRedraw();

    // redraw if required
    if (m_bRedrawPending)
        Draw();
}

void EditorBlockMeshEditor::Draw()
{
    // create hardware resources
    if (!m_bHardwareResourcesCreated && !CreateHardwareResources())
        return;

    GPUContext *pGPUContext = g_pRenderer->GetMainContext();

    // clear it
    pGPUContext->SetOutputBuffer(m_pSwapChain);
    pGPUContext->SetRenderTargets(0, NULL, NULL);
    pGPUContext->SetViewport(m_viewController.GetViewport());
    pGPUContext->ClearTargets(true, true, true);

    // setup 3d state
    pGPUContext->GetConstants()->SetFromCamera(m_viewController.GetCamera(), true);

    // invoke draws
    DrawGrid();
    DrawView();

    // bias the projection matrix slightly FIXME
    //pGPUConstants->SetCameraProjectionMatrix(m_viewController.GetBiasedProjectionMatrix(0.01f), true);
    Draw3DOverlays();

    // clear state
    pGPUContext->ClearState(true, true, true, true);

    // present
    m_pSwapChain->SwapBuffers();

    // clear pending flag
    m_bRedrawPending = false;
}

void EditorBlockMeshEditor::DrawGrid()
{
    const int3 &minCoordinates = m_pGenerator->GetMeshMinCoordinates(m_currentLOD);
    const int3 &maxCoordinates = m_pGenerator->GetMeshMaxCoordinates(m_currentLOD);
    float scale = m_pGenerator->GetMeshVolume(m_currentLOD).GetScale();

    float3 minWorldCoordinates = float3(float(minCoordinates.x - 1), float(minCoordinates.y - 1), 0.0f) * scale;
    float3 maxWorldCoordinates = float3(float(maxCoordinates.x + 2), float(maxCoordinates.y + 2), 0.0f) * scale;
    m_guiContext.Draw3DGrid(minWorldCoordinates, maxWorldCoordinates, float3(scale, scale, scale), MAKE_COLOR_R8G8B8A8_UNORM(102, 102, 102, 255));
}

void EditorBlockMeshEditor::DrawView()
{
    m_pWorldRenderer->DrawWorld(m_pRenderWorld, m_viewController.GetViewParameters(), NULL, NULL, NULL);
}

void EditorBlockMeshEditor::Draw3DOverlays()
{
    // begin batching
    m_guiContext.SetDepthTestingEnabled(false);
    m_guiContext.SetAlphaBlendingEnabled(false);
    m_guiContext.PushManualFlush();

    switch (m_eCurrentWidget)
    {
    case WIDGET_SELECT_BLOCKS:
        {
            // draw box around the currently highlighted block
            if (m_bMouseOverBlockSet)
            {
                float3 minCoordinates(float3((float)m_MouseOverBlock.x, (float)m_MouseOverBlock.y, (float)m_MouseOverBlock.z) * m_meshVolume.GetScale());
                float3 maxCoordinates(minCoordinates + m_meshVolume.GetScale());
                m_guiContext.Draw3DWireBox(minCoordinates, maxCoordinates, MAKE_COLOR_R8G8B8A8_UNORM(102, 102, 102, 255));
            }

            // draw boxes around each selected block
            for (uint32 i = 0; i < m_SelectedBlocks.GetSize(); i++)
            {
                const int3 &blockCoordinates = m_SelectedBlocks[i];
                float3 minCoordinates(float3((float)blockCoordinates.x, (float)blockCoordinates.y, (float)blockCoordinates.z) * m_meshVolume.GetScale());
                float3 maxCoordinates(minCoordinates + m_meshVolume.GetScale());
                m_guiContext.Draw3DWireBox(minCoordinates, maxCoordinates, MAKE_COLOR_R8G8B8A8_UNORM(200, 200, 200, 255));
            }
        }
        break;

        /*
    case WIDGET_SELECT_AREA:
        {
            // do we have an area yet?
            if (m_iSelectedAreaCount == 2)
            {
                // yup, so draw a box around the whole area
                minCoordinates = Vector3(Vector3i(m_SelectedArea[0])) * blockSize;
                maxCoordinates = Vector3(Vector3i(m_SelectedArea[1])) * blockSize + blockSize;
                m_guiContext.Draw3DWireBox(minCoordinates, maxCoordinates, MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 150, 255));
            }
            else
            {
                // do we have anything?
                if (m_iSelectedAreaCount == 1)
                {
                    // draw a green box on the starting position
                    minCoordinates = Vector3(Vector3i(m_SelectedArea[0])) * blockSize;
                    maxCoordinates = minCoordinates + blockSize;
                    m_guiContext.Draw3DWireBox(minCoordinates, maxCoordinates, MAKE_COLOR_R8G8B8A8_UNORM(0, 255, 0, 255));
                }

                // draw boxes around the spot where the block will be placed
                if (m_bMouseOverBlockSet || m_bMouseOverPlaceBlockSet)
                {
                    minCoordinates = Vector3(Vector3i((m_bMouseOverBlockSet) ? m_MouseOverBlock : m_MouseOverPlaceBlock)) * blockSize;
                    maxCoordinates = minCoordinates + blockSize;
                    m_guiContext.Draw3DWireBox(minCoordinates, maxCoordinates, MAKE_COLOR_R8G8B8A8_UNORM(200, 200, 200, 255));
                }
            }
        }
        break;*/

    case WIDGET_PLACE_BLOCKS:
        {
            // draw boxes around the spot where the block will be placed
            if (m_bMouseOverPlaceBlockSet)
            {
                float3 minCoordinates(float3((float)m_MouseOverPlaceBlock.x, (float)m_MouseOverPlaceBlock.y, (float)m_MouseOverPlaceBlock.z) * m_meshVolume.GetScale());
                float3 maxCoordinates(minCoordinates + m_meshVolume.GetScale());
                m_guiContext.Draw3DWireBox(minCoordinates, maxCoordinates, MAKE_COLOR_R8G8B8A8_UNORM(200, 200, 200, 255));
            }
        }
        break;

    case WIDGET_DELETE_BLOCKS:
        {
            // draw boxes around the block which will be deleted
            if (m_bMouseOverBlockSet)
            {
                float3 minCoordinates(float3((float)m_MouseOverBlock.x, (float)m_MouseOverBlock.y, (float)m_MouseOverBlock.z) * m_meshVolume.GetScale());
                float3 maxCoordinates(minCoordinates + m_meshVolume.GetScale());
                m_guiContext.Draw3DWireBox(minCoordinates, maxCoordinates, MAKE_COLOR_R8G8B8A8_UNORM(200, 200, 200, 255));
            }
        }
        break;
    }

    m_guiContext.Flush();
    m_guiContext.PopManualFlush();
}

bool EditorBlockMeshEditor::Initialize(ProgressCallbacks *pProgressCallbacks)
{
    // generate block types
    {
        Timer generationTimer;

        pProgressCallbacks->SetStatusText("Building available block types...");
        if (!GenerateAvailableBlockTypes(pProgressCallbacks))
            return false;

        Log_PerfPrintf("EditorStaticBlockMeshEditor::Initialize: Available block type generation took %.4f msec", generationTimer.GetTimeMilliseconds());
    }

    // create preview volume
    m_meshVolume.SetPalette(m_pBlockList);
    m_selectedVolume.SetPalette(m_pBlockList);
    m_previewVolume.SetPalette(m_pBlockList);
    m_previewVolume.Resize(int3::Zero, int3::Zero);

    // create world
    m_pRenderWorld = new RenderWorld();

    // create light
    m_pLightSimulator = new EditorLightSimulator(0, m_pRenderWorld);

    // create composite render proxy
    m_pMeshRenderProxy = new EditorBlockVolumeRenderProxy(0);
    m_pRenderWorld->AddRenderable(m_pMeshRenderProxy);

    // creat selected render proxy
    m_pSelectedRenderProxy = new EditorBlockVolumeRenderProxy(0);
    m_pRenderWorld->AddRenderable(m_pSelectedRenderProxy);

    // create preview composite render proxy
    m_pPreviewRenderProxy = new EditorBlockVolumeRenderProxy(0);
    m_pRenderWorld->AddRenderable(m_pPreviewRenderProxy);

    // update ui for mesh
    m_ui->UpdateUIForOpenMesh(m_meshName);
    m_ui->UpdateUIForLODCount(m_pGenerator->GetLODCount(), false);

    // update camera
    m_viewController.SetCameraMode(EDITOR_CAMERA_MODE_PERSPECTIVE);
    m_viewController.SetPerspectiveAcceleration(10.0f * m_pGenerator->GetScale());
    m_viewController.SetPerspectiveMaxSpeed(2.0f * m_pGenerator->GetScale());
    m_viewController.Reset();
    
    // set modes
    SetWidget(WIDGET_CAMERA);
    SetCurrentLOD(0);

    // connect ui events
    ConnectUIEvents();

    // ok
    return true;
}

void EditorBlockMeshEditor::Deinitialize()
{
    // kill gpu resources
    ReleaseHardwareResources();

    // kill world and entities
    SAFE_RELEASE(m_pMeshRenderProxy);
    SAFE_RELEASE(m_pPreviewRenderProxy);
    delete m_pLightSimulator;
    delete m_pRenderWorld;
    m_pRenderWorld = nullptr;

    delete m_pGenerator;
    m_pGenerator = nullptr;
    SAFE_RELEASE(m_pBlockList);

    // ensure the frame event dies
    disconnect(g_pEditor, SIGNAL(OnFrameExecution(float)), this, SLOT(OnFrameExecutionTriggered(float)));
}

bool EditorBlockMeshEditor::GenerateAvailableBlockTypes(ProgressCallbacks *pProgressCallbacks)
{
    uint32 nAvailableBlocks = 0;
    for (uint32 i = 1; i < BLOCK_MESH_MAX_BLOCK_TYPES; i++)
    {
        if (m_pBlockList->GetBlockType(i)->IsAllocated)
            nAvailableBlocks++;
    }

    m_pAvailableBlockTypes = new AvailableBlockType[nAvailableBlocks + 1];
    m_nAvailableBlockTypes = nAvailableBlocks;
    nAvailableBlocks = 0;

    // add air
    m_pAvailableBlockTypes[nAvailableBlocks].pBlockType = m_pBlockList->GetBlockType(0);
    m_pAvailableBlockTypes[nAvailableBlocks].BlockName = "None";
    nAvailableBlocks++;

    // prepare the icon
    Image renderedBlockTypeIcon;
    renderedBlockTypeIcon.Create(PIXEL_FORMAT_R8G8B8A8_UNORM, BLOCK_TYPE_ICON_SIZE, BLOCK_TYPE_ICON_SIZE, 1);

    // init preview generator
    EditorResourcePreviewGenerator previewGenerator;
    previewGenerator.SetGPUContext(g_pRenderer->GetMainContext());

    // setup progress
    pProgressCallbacks->SetCancellable(false);
    pProgressCallbacks->SetProgressRange(m_nAvailableBlockTypes);
    pProgressCallbacks->SetProgressValue(0);

    // generate icons
    for (uint32 i = 1; i < BLOCK_MESH_MAX_BLOCK_TYPES; i++)
    {
        const BlockPalette::BlockType *pBlockType = m_pBlockList->GetBlockType(i);
        if (!pBlockType->IsAllocated)
            continue;

        m_pAvailableBlockTypes[nAvailableBlocks].pBlockType = pBlockType;
        m_pAvailableBlockTypes[nAvailableBlocks].BlockName = pBlockType->Name;

        if (!previewGenerator.GenerateBlockMeshBlockListBlockPreview(&renderedBlockTypeIcon, m_pBlockList, (BlockVolumeBlockType)pBlockType->BlockTypeIndex))
        {
            Log_WarningPrintf("EditorStaticBlockMeshEditor::GenerateAvailableBlockTypes: Generate icon for %u (%s) failed.", i, pBlockType->Name.GetCharArray());
            Y_memset(renderedBlockTypeIcon.GetData(), 0xFF, renderedBlockTypeIcon.GetDataSize());
        }

        QPixmap qPixmap;
        if (!EditorHelpers::ConvertImageToQPixmap(renderedBlockTypeIcon, &qPixmap))
            Log_WarningPrintf("EditorStaticBlockMeshEditor::GenerateAvailableBlockTypes: Conversion to image for %u (%s) failed.", i, pBlockType->Name.GetCharArray());
        else
            m_pAvailableBlockTypes[nAvailableBlocks].BlockTypeIcon.addPixmap(qPixmap);

        pProgressCallbacks->SetProgressValue(nAvailableBlocks);
        nAvailableBlocks++;
    }

    // set the various combo boxes up
    m_ui->UpdateUIAvailableBlockTypes(BLOCK_TYPE_ICON_SIZE, m_pAvailableBlockTypes, m_nAvailableBlockTypes);
    pProgressCallbacks->SetProgressValue(m_nAvailableBlockTypes);
    return true;
}

bool EditorBlockMeshEditor::OnCloseAttempt()
{
    if (m_pGenerator != NULL && m_pGenerator->IsChanged())
    {
        int result = QMessageBox::question(this,
            tr("Save open mesh?"),
            tr("The current open mesh is changed, do you wish to save the changes made to it?"),
            (QMessageBox::StandardButtons)(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel),
            QMessageBox::Yes);

        if (result == QMessageBox::Yes)
            OnActionSaveMeshClicked();
        else if (result == QMessageBox::Cancel)
            return false;
    }

    return true;
}

bool EditorBlockMeshEditor::GetBlockCoordinatesForMousePosition(int3 *pBlockCoordinates, CUBE_FACE *pFaceIndex, const int2 &mousePosition) const
{
    Ray hitRay = m_viewController.GetPickRay(mousePosition.x, mousePosition.y);
    //Log_DevPrintf("[%i, %i] -> %s --- %s (%s) (%f)", mousePosition.x, mousePosition.y, StringConverter::Float3ToString(hitRay.GetOrigin()).GetCharArray(), StringConverter::Float3ToString(hitRay.GetEnd()).GetCharArray(), StringConverter::Float3ToString(hitRay.GetDirection()).GetCharArray(), hitRay.GetDistance());
    
    float intersectionTime;
    return m_pGenerator->GetMeshVolume(m_currentLOD).RayCastTimeFace(hitRay, pBlockCoordinates, &intersectionTime, pFaceIndex, false);
}

bool EditorBlockMeshEditor::GetPlaceBlockCoordinatesForMousePosition(int3 *pBlockCoordinates, const int2 &mousePosition) const
{
    Ray hitRay = m_viewController.GetPickRay(mousePosition.x, mousePosition.y);
    int3 meshHitBlock;
    float meshHitDistance;
    CUBE_FACE meshHitFace;
    
    // hit against the mesh
    if (!m_pGenerator->GetMeshVolume(m_currentLOD).RayCastTimeFace(hitRay, &meshHitBlock, &meshHitDistance, &meshHitFace, false))
        meshHitDistance = Y_FLT_INFINITE;

    // hit against the plane
    Plane groundPlane(float3::UnitZ, 0.0f);
    float3 planeHitLocation;
    float3 planeHitNormal;
    float planeHitDistance;
    if (hitRay.PlaneIntersection(groundPlane, planeHitNormal, planeHitLocation))
        planeHitDistance = (planeHitLocation - hitRay.GetOrigin()).Length();
    else
        planeHitDistance = Y_FLT_INFINITE;

    // perfer mesh over plane
    if (meshHitDistance <= planeHitDistance)
    {
        // cater for both == inf case
        if (meshHitDistance == Y_FLT_INFINITE)
            return false;

        // we need to offset the block, depending on which face was hit
        switch (meshHitFace)
        {
        case CUBE_FACE_RIGHT:   meshHitBlock.x++;   break;
        case CUBE_FACE_LEFT:    meshHitBlock.x--;   break;
        case CUBE_FACE_BACK:    meshHitBlock.y++;   break;
        case CUBE_FACE_FRONT:   meshHitBlock.y--;   break;
        case CUBE_FACE_TOP:     meshHitBlock.z++;   break;
        case CUBE_FACE_BOTTOM:  meshHitBlock.z--;   break;
        }

        *pBlockCoordinates = meshHitBlock;
        return true;
    }

    // work out which block on the plane the ray is colliding with
    float3 hitBlockCoordsF = planeHitLocation / (float)m_meshVolume.GetScale();
    int3 hitBlockCoords(Math::Truncate(Math::Floor(hitBlockCoordsF.x)), Math::Truncate(Math::Floor(hitBlockCoordsF.y)), 0);
    //Log_DevPrintf("hitloc: %f %f %f, block: %i %i %i", planeHitLocation.x, planeHitLocation.y, planeHitLocation.z, hitBlockCoords.x, hitBlockCoords.y, hitBlockCoords.z);
    
    // make sure that block isn't occupied by anything currently. additionally, if it's +/- 1
    // from the mesh boundaries, we'll allow it, otherwise ignore it (out of range)
    if (((hitBlockCoords >= m_pGenerator->GetMeshMinCoordinates(m_currentLOD) && hitBlockCoords <= m_pGenerator->GetMeshMaxCoordinates(m_currentLOD)) && m_pGenerator->GetBlock(m_currentLOD, hitBlockCoords) != 0) ||
        ((hitBlockCoords + int3::One) >= m_pGenerator->GetMeshMinCoordinates(m_currentLOD) && (hitBlockCoords - int3::One) <= m_pGenerator->GetMeshMaxCoordinates(m_currentLOD)))
    {
        *pBlockCoordinates = hitBlockCoords;
        return true;
    }

    // nothing
    return false;
}

void EditorBlockMeshEditor::SetBlock(const int3 &blockPosition, BlockVolumeBlockType value)
{
    Log_DevPrintf("EditorStaticBlockMeshEditor::SetBlock(%i, %i, %i, %u)", blockPosition.x, blockPosition.y, blockPosition.z, (uint32)value);

    m_pGenerator->SetBlock(m_currentLOD, blockPosition, value);
    CheckForMeshSizeChanges();
    m_meshVolume.SetBlock(blockPosition, value);
    UpdateMeshView();
}

void EditorBlockMeshEditor::CheckForMeshSizeChanges()
{
    const BlockMeshVolume &realVolume = m_pGenerator->GetMeshVolume(m_currentLOD);
    if (m_meshVolume.GetMinCoordinates() != realVolume.GetMinCoordinates() ||
        m_meshVolume.GetMaxCoordinates() != realVolume.GetMaxCoordinates())
    {
        m_meshVolume.Resize(realVolume.GetMinCoordinates(), realVolume.GetMaxCoordinates());
        m_selectedVolume.Resize(realVolume.GetMinCoordinates(), realVolume.GetMaxCoordinates());

        m_ui->UpdateUIForMeshSize(realVolume.GetWidth(), realVolume.GetLength(), realVolume.GetHeight());
    }
}

void EditorBlockMeshEditor::UpdateMouseBlockCoordinates()
{
    int3 mouseOverBlock;
    CUBE_FACE mouseOverBlockFaceIndex;
    bool validMouseOverBlock = GetBlockCoordinatesForMousePosition(&mouseOverBlock, &mouseOverBlockFaceIndex, m_lastMousePosition);
    if (validMouseOverBlock)
    {
        if (!m_bMouseOverBlockSet || (m_MouseOverBlock != mouseOverBlock))
        {
            Log_DevPrintf("block under mouse set: %i,%i,%i", m_MouseOverBlock.x, m_MouseOverBlock.y, m_MouseOverBlock.z);
            m_MouseOverBlock = mouseOverBlock;
            m_iMouseOverBlockFaceIndex = mouseOverBlockFaceIndex;
            m_bMouseOverBlockSet = true;

            if (m_eCurrentWidget == WIDGET_SELECT_BLOCKS || m_eCurrentWidget == WIDGET_SELECT_AREA || m_eCurrentWidget == WIDGET_DELETE_BLOCKS)
                FlagForRedraw();
        }
    }
    else if (m_bMouseOverBlockSet)
    {
        Log_DevPrintf("block under mouse set: NONE");
        m_bMouseOverBlockSet = false;
        m_MouseOverBlock.SetZero();

        if (m_eCurrentWidget == WIDGET_SELECT_BLOCKS || m_eCurrentWidget == WIDGET_SELECT_AREA || m_eCurrentWidget == WIDGET_DELETE_BLOCKS)
            FlagForRedraw();
    }

    // are we in a placeable block position?
    int3 mouseOverPlaceBlock;
    bool validMouseOverPlaceBlock = GetPlaceBlockCoordinatesForMousePosition(&mouseOverPlaceBlock, m_lastMousePosition);
    if (validMouseOverPlaceBlock)
    {
        if (!m_bMouseOverPlaceBlockSet || (mouseOverPlaceBlock != m_MouseOverPlaceBlock))
        {
            Log_DevPrintf("place block position set: %i,%i,%i", mouseOverPlaceBlock.x, mouseOverPlaceBlock.y, mouseOverPlaceBlock.z);
            m_MouseOverPlaceBlock = mouseOverPlaceBlock;
            m_bMouseOverPlaceBlockSet = true;

            if (m_eCurrentWidget == WIDGET_PLACE_BLOCKS)
            {
                // work out new base offset of the place block mesh
                float3 placeMeshOffset(float3((float)mouseOverPlaceBlock.x, (float)mouseOverPlaceBlock.y, (float)mouseOverPlaceBlock.z) * m_meshVolume.GetScale());
                m_pPreviewRenderProxy->SetTransform(Transform(placeMeshOffset, Quaternion::Identity, float3::One).GetTransformMatrix4x4());
                m_pPreviewRenderProxy->SetVisibility(true);
                FlagForRedraw();
            }
        }
    }
    else if (m_bMouseOverPlaceBlockSet)
    {
        Log_DevPrintf("place block position set: NONE");

        m_bMouseOverPlaceBlockSet = false;
        m_MouseOverPlaceBlock.SetZero();

        if (m_eCurrentWidget == WIDGET_PLACE_BLOCKS)
        {
            // clear the preview mesh
            m_pPreviewRenderProxy->SetVisibility(false);
            FlagForRedraw();
        }
    }
}

void EditorBlockMeshEditor::UpdateMeshView()
{
    m_pMeshRenderProxy->BuildMesh(&m_meshVolume, m_pGenerator->GetUseAmbientOcclusion());
    FlagForRedraw();
}

void EditorBlockMeshEditor::UpdateSelectedBlocksView()
{
    m_pSelectedRenderProxy->BuildMesh(&m_selectedVolume, m_pGenerator->GetUseAmbientOcclusion());
    FlagForRedraw();
}

void EditorBlockMeshEditor::UpdatePreviewMesh(BlockVolumeBlockType blockType)
{
    m_previewVolume.SetBlock(0, 0, 0, blockType);

    if (blockType != 0)
        m_pPreviewRenderProxy->BuildMesh(&m_previewVolume, m_pGenerator->GetUseAmbientOcclusion());
    else
        m_pPreviewRenderProxy->ClearMesh();

    if (m_eCurrentWidget == WIDGET_PLACE_BLOCKS)
        FlagForRedraw();
}

void EditorBlockMeshEditor::ConnectUIEvents()
{
    // actions
    connect(m_ui->actionSaveMesh, SIGNAL(triggered()), this, SLOT(OnActionSaveMeshClicked()));
    connect(m_ui->actionSaveMeshAs, SIGNAL(triggered()), this, SLOT(OnActionSaveMeshAsClicked()));
    connect(m_ui->actionClose, SIGNAL(triggered()), this, SLOT(OnActionCloseClicked()));
    connect(m_ui->actionEditUndo, SIGNAL(triggered()), this, SLOT(OnActionEditUndoClicked()));
    connect(m_ui->actionEditRedo, SIGNAL(triggered()), this, SLOT(OnActionEditRedoClicked()));
    connect(m_ui->actionCameraPerspective, SIGNAL(triggered(bool)), this, SLOT(OnActionCameraPerspectiveTriggered(bool)));
    connect(m_ui->actionCameraArcball, SIGNAL(triggered(bool)), this, SLOT(OnActionCameraArcballTriggered(bool)));
    connect(m_ui->actionCameraIsometric, SIGNAL(triggered(bool)), this, SLOT(OnActionCameraIsometricTriggered(bool)));
    connect(m_ui->actionViewWireframe, SIGNAL(triggered(bool)), this, SLOT(OnActionViewWireframeTriggered(bool)));
    connect(m_ui->actionViewUnlit, SIGNAL(triggered(bool)), this, SLOT(OnActionViewUnlitTriggered(bool)));
    connect(m_ui->actionViewLit, SIGNAL(triggered(bool)), this, SLOT(OnActionViewLitTriggered(bool)));
    connect(m_ui->actionViewFlagShadows, SIGNAL(triggered(bool)), this, SLOT(OnActionViewFlagShadowsTriggered(bool)));
    connect(m_ui->actionViewFlagWireframeOverlay, SIGNAL(triggered(bool)), this, SLOT(OnActionViewFlagWireframeOverlayTriggered(bool)));
    connect(m_ui->actionWidgetCamera, SIGNAL(triggered(bool)), this, SLOT(OnActionWidgetCameraTriggered(bool)));
    connect(m_ui->actionWidgetSelectBlocks, SIGNAL(triggered(bool)), this, SLOT(OnActionWidgetSelectBlocksTriggered(bool)));
    connect(m_ui->actionWidgetSelectArea, SIGNAL(triggered(bool)), this, SLOT(OnActionWidgetSelectAreaTriggered(bool)));
    connect(m_ui->actionWidgetPlaceBlocks, SIGNAL(triggered(bool)), this, SLOT(OnActionWidgetPlaceBlocksTriggered(bool)));
    connect(m_ui->actionWidgetDeleteBlocks, SIGNAL(triggered(bool)), this, SLOT(OnActionWidgetDeleteBlocksTriggered(bool)));
    connect(m_ui->actionWidgetLightManipulator, SIGNAL(triggered(bool)), this, SLOT(OnActionWidgetLightManipulatorTriggered(bool)));

    // place widget
    connect(m_ui->placeBlocksWidgetPaletteList, SIGNAL(itemClicked(QListWidgetItem *)), this, SLOT(OnPlaceBlockWidgetListWidgetItemClicked(QListWidgetItem *)));

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

void EditorBlockMeshEditor::closeEvent(QCloseEvent *pCloseEvent)
{
    if (!OnCloseAttempt())
    {
        pCloseEvent->ignore();
        return;
    }

    QMainWindow::closeEvent(pCloseEvent);

    Deinitialize();
    deleteLater();
}

void EditorBlockMeshEditor::OnActionSaveMeshClicked()
{
    EditorProgressDialog progressDialog(this);
    progressDialog.show();

    if (!Save(&progressDialog))
        QMessageBox::critical(this, tr("Save mesh operation failed"), tr("Save mesh operation failed. Any changes on-disk have been reverted."), QMessageBox::Ok);
}

void EditorBlockMeshEditor::OnActionSaveMeshAsClicked()
{
    PathString tempPath;

    for (;;)
    {
        EditorResourceSaveDialog saveDialog(this, OBJECT_TYPEINFO(BlockMesh));
        int res = saveDialog.exec();
        if (res == 0)
            return;

        // construct temporary path
        tempPath.Format("%s.staticblockmesh.xml", saveDialog.GetReturnValueResourceName().GetCharArray());
        if (g_pVirtualFileSystem->FileExists(tempPath))
        {
            res = QMessageBox::question(this, tr("Question"), tr("The specified file already exists. Do you wish to overwrite it?"), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
            if (res == QMessageBox::Cancel)
                return;

            if (res == QMessageBox::No)
                continue;
        }

        tempPath.Clear();
        tempPath.AppendString(saveDialog.GetReturnValueResourceName());
        break;
    }

    EditorProgressDialog progressDialog(this);
    progressDialog.show();

    if (!SaveAs(tempPath, &progressDialog))
        QMessageBox::critical(this, tr("Save mesh operation failed"), tr("Save mesh operation failed. Any changes on-disk have been reverted."), QMessageBox::Ok);
}

void EditorBlockMeshEditor::OnActionCloseClicked()
{
    close();
}

void EditorBlockMeshEditor::OnActionEditUndoClicked()
{

}

void EditorBlockMeshEditor::OnActionEditRedoClicked()
{

}

void EditorBlockMeshEditor::OnSwapChainWidgetResized()
{
    uint32 newWidth = (uint32)m_ui->swapChainWidget->size().width();
    uint32 newHeight = (uint32)m_ui->swapChainWidget->size().height();

    // kill old swapchain refs
    if (m_pSwapChain != NULL)
    {
        m_pSwapChain->Release();
        m_pSwapChain = NULL;
    }

    // skip full recreation if we can just do the resize
    if (m_bHardwareResourcesCreated && (m_pSwapChain = m_ui->swapChainWidget->GetSwapChain()) != NULL)
        m_pSwapChain->AddRef();
    else
        m_bHardwareResourcesCreated = false;

    m_viewController.SetViewportDimensions(newWidth, newHeight);
    m_guiContext.SetViewportDimensions(newWidth, newHeight);

    // flag for redraw
    FlagForRedraw();
}

void EditorBlockMeshEditor::OnSwapChainWidgetPaint()
{
    FlagForRedraw();
}

void EditorBlockMeshEditor::OnSwapChainWidgetKeyboardEvent(const QKeyEvent *pKeyboardEvent)
{
//     // we handle camera movement via the keyboard.
//     bool isKeyDownEvent = (pKeyboardEvent->Event == INPUT_EVENT_TYPE_KEYBOARD_KEY_DOWN);
//     bool isKeyUpEvent = (pKeyboardEvent->Event == INPUT_EVENT_TYPE_KEYBOARD_KEY_UP);
//     if ((isKeyDownEvent | isKeyUpEvent))
//     {
// 
//     }

    // pass to camera
    m_viewController.HandleKeyboardEvent(pKeyboardEvent);
}

void EditorBlockMeshEditor::OnSwapChainWidgetMouseEvent(const QMouseEvent *pMouseEvent)
{
    if (pMouseEvent->type() == QEvent::MouseButtonPress && pMouseEvent->button() == Qt::LeftButton)
    {
        // if not currently in any state, determine which state to transition to
        if (m_eCurrentState == STATE_NONE)
        {
            switch (m_eCurrentWidget)
            {
            case WIDGET_CAMERA:
                m_eCurrentState = STATE_MOVE_CAMERA;
                return;

            case WIDGET_SELECT_BLOCKS:
                {
                    if (m_bMouseOverBlockSet)
                    {
                        if (!IsBlockSelected(m_MouseOverBlock))
                            SelectBlock(m_MouseOverBlock);
                        else
                            DeselectBlock(m_MouseOverBlock);

                        FlagForRedraw();
                        return;
                    }
                }
                break;

            case WIDGET_PLACE_BLOCKS:
                {
                    // set the block
                    if (m_bMouseOverPlaceBlockSet)
                    {
                        SetBlock(m_MouseOverPlaceBlock, m_iPlaceBlockToolBlockType);
                        UpdateMouseBlockCoordinates();
                        FlagForRedraw();
                    }
                }
                break;

            case WIDGET_SELECT_AREA:
                {
                    // have a selection?
                    if (m_iSelectedAreaCount < 2 && (m_bMouseOverBlockSet || m_bMouseOverPlaceBlockSet))
                    {
                        m_SelectedArea[m_iSelectedAreaCount++] = (m_bMouseOverBlockSet) ? m_MouseOverBlock : m_MouseOverPlaceBlock;
                            
                        if (m_iSelectedAreaCount == 2)
                        {
                            int3 fixedMinCoords = m_SelectedArea[0].Min(m_SelectedArea[1]);
                            int3 fixedMaxCoords = m_SelectedArea[0].Max(m_SelectedArea[1]);
                            m_SelectedArea[0] = fixedMinCoords;
                            m_SelectedArea[1] = fixedMaxCoords;
                        }

                        FlagForRedraw();
                    }
                }
                break;

            case WIDGET_DELETE_BLOCKS:
                {
                    // delete the block the mouse is over
                    if (m_bMouseOverBlockSet)
                    {
                        SetBlock(m_MouseOverBlock, 0);
                        UpdateMouseBlockCoordinates();
                        FlagForRedraw();
                    }
                }
                break;

            case WIDGET_LIGHT_MANIPULATOR:
                {
                    m_eCurrentState = STATE_MANIPULATE_LIGHT;
                }
                break;
            }
        }     
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonRelease && pMouseEvent->button() == Qt::LeftButton)
    {
        switch (m_eCurrentState)
        {
        case STATE_MOVE_CAMERA:
        case STATE_ROTATE_CAMERA:
        case STATE_MANIPULATE_LIGHT:
            {
                m_eCurrentState = STATE_NONE;
                return;
            }
            break;
        }
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonPress && pMouseEvent->button() == Qt::RightButton)
    {
        // right mouse button can enter a viewport state. if we are not in a state, determine which state to enter.
        if (m_eCurrentState == STATE_NONE)
        {
            // right mouse button enters camera rotation state
            m_eCurrentState = STATE_ROTATE_CAMERA;
            return;
        }
    }
    else if (pMouseEvent->type() == QEvent::MouseButtonRelease && pMouseEvent->button() == Qt::RightButton)
    {
        // if in camera rotation state, exit it
        if (m_eCurrentState == STATE_ROTATE_CAMERA)
        {
            m_eCurrentState = STATE_NONE;
            return;
        }
    }
    else if (pMouseEvent->type() == QEvent::MouseMove)
    {
        int2 currentMousePosition(pMouseEvent->x(), pMouseEvent->y());
        int2 lastMousePosition(m_lastMousePosition);
        int2 mousePositionDiff(currentMousePosition - lastMousePosition);
        m_lastMousePosition = currentMousePosition;

        // update the block position that we are under
        UpdateMouseBlockCoordinates();

        // handle mouse movement based on state
        switch (m_eCurrentState)
        {
        case STATE_NONE:
            {
            }
            break;

        case STATE_MOVE_CAMERA:
            {
                m_viewController.MoveFromMousePosition(mousePositionDiff);
                FlagForRedraw();
            }
            break;

        case STATE_ROTATE_CAMERA:
            {
                m_viewController.RotateFromMousePosition(mousePositionDiff);
                FlagForRedraw();
            }
            break;

        case STATE_MANIPULATE_LIGHT:
            {
                m_pLightSimulator->ManipulateFromMouseMovement(mousePositionDiff);
                FlagForRedraw();
            }
            break;
        }
    }
}

void EditorBlockMeshEditor::OnSwapChainWidgetWheelEvent(const QWheelEvent *pWheelEvent)
{
    if (m_eCurrentWidget == WIDGET_PLACE_BLOCKS && pWheelEvent->orientation() == Qt::Vertical)
        (pWheelEvent->delta() > 0) ? SetPreviousPlaceBlockWidgetBlockType() : SetNextPlaceBlockWidgetBlockType();
}

void EditorBlockMeshEditor::OnSwapChainWidgetGainedFocusEvent()
{

}

void EditorBlockMeshEditor::OnPlaceBlockWidgetToolButtonClicked(bool checked)
{
    // find the index of the sender
    QObject *pSender = sender();
    uint32 index;
    for (index = 0; index < m_nAvailableBlockTypes; index++)
    {
        if (m_ui->placeBlocksWidgetPaletteIcons[index] == pSender)
            break;
    }
    if (index == m_nAvailableBlockTypes)
        return;

    SetPlaceBlockWidgetBlockType((BlockVolumeBlockType)m_pAvailableBlockTypes[index].pBlockType->BlockTypeIndex);
}

void EditorBlockMeshEditor::OnPlaceBlockWidgetListWidgetItemClicked(QListWidgetItem *pItem)
{
    int row = m_ui->placeBlocksWidgetPaletteList->currentRow();
    if ((uint32)row < m_nAvailableBlockTypes)
        SetPlaceBlockWidgetBlockType((BlockVolumeBlockType)m_pAvailableBlockTypes[row].pBlockType->BlockTypeIndex);
}
