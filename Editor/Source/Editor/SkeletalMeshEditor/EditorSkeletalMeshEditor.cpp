#include "Editor/PrecompiledHeader.h"
#include "Editor/SkeletalMeshEditor/EditorSkeletalMeshEditor.h"
#include "Editor/SkeletalMeshEditor/ui_EditorSkeletalMeshEditor.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/RenderProxies/CompositeRenderProxy.h"
#include "Renderer/Renderer.h"
#include "Renderer/VertexFactories/LocalVertexFactory.h"
#include "Engine/ResourceManager.h"
#include "Engine/Engine.h"
#include "ResourceCompiler/CollisionShapeGenerator.h"
#include "ResourceCompiler/StaticMeshGenerator.h"
#include "ContentConverter/AssimpStaticMeshImporter.h"
#include "Editor/EditorLightSimulator.h"
#include "Editor/EditorHelpers.h"
#include "Editor/EditorProgressDialog.h"
#include "Editor/EditorResourceSaveDialog.h"
#include "Editor/Editor.h"
Log_SetChannel(EditorSkeletalMeshEditor);

EditorSkeletalMeshEditor::EditorSkeletalMeshEditor()
    : m_ui(new Ui_EditorSkeletalMeshEditor()),
      m_renderMode(EDITOR_RENDER_MODE_FULLBRIGHT),
      m_viewportFlags(EDITOR_VIEWPORT_FLAG_ENABLE_SHADOWS),
      m_pGenerator(nullptr),
      m_pRenderWorld(new RenderWorld()),
      m_pLightSimulator(new EditorLightSimulator(0, m_pRenderWorld)),
      m_pMeshPreviewRenderProxy(new CompositeRenderProxy(0, Transform::Identity)),
      m_pCollisionShapePreviewRenderProxy(new CompositeRenderProxy(0, Transform::Identity)),
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
    m_pCollisionShapePreviewRenderProxy->SetVisibility(false);
    m_pRenderWorld->AddRenderable(m_pMeshPreviewRenderProxy);
    m_pRenderWorld->AddRenderable(m_pCollisionShapePreviewRenderProxy);
}

EditorSkeletalMeshEditor::~EditorSkeletalMeshEditor()
{

}

void EditorSkeletalMeshEditor::SetCameraMode(EDITOR_CAMERA_MODE cameraMode)
{
    DebugAssert(cameraMode < EDITOR_CAMERA_MODE_COUNT);
    m_viewController.SetCameraMode(cameraMode);
    m_ui->UpdateUIForCameraMode(cameraMode);
}

void EditorSkeletalMeshEditor::SetRenderMode(EDITOR_RENDER_MODE renderMode)
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

void EditorSkeletalMeshEditor::SetViewportFlag(uint32 flag)
{
    flag &= ~m_viewportFlags;
    if (flag == 0)
        return;

    m_viewportFlags |= flag;
    m_ui->UpdateUIForViewportFlags(m_viewportFlags);
    FlagForRedraw();
}

void EditorSkeletalMeshEditor::ClearViewportFlag(uint32 flag)
{
    flag &= m_viewportFlags;
    if (flag == 0)
        return;

    m_viewportFlags &= ~flag;
    m_ui->UpdateUIForViewportFlags(m_viewportFlags);
    FlagForRedraw();
}

void EditorSkeletalMeshEditor::SetCurrentTool(Tool tool)
{
    m_ui->UpdateUIForTool(tool);

    if (m_currentTool == tool)
        return;

    m_currentTool = tool;

    // adjust collision mesh visibility
    m_pCollisionShapePreviewRenderProxy->SetVisibility((tool == Tool_Collision));

    // redraw
    FlagForRedraw();
}

void EditorSkeletalMeshEditor::Create(const char *meshName, ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    m_meshName = meshName;
    m_meshFileName.Format("%s.skm.xml", meshName);

    // allocate a new generator
    m_pGenerator = new StaticMeshGenerator();
    m_pGenerator->Create(false, false);

    // update everything
    OnFileNameChanged();
    UpdateInformationTab();
}

bool EditorSkeletalMeshEditor::Load(const char *meshName, ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    // find the filename, and open it
    PathString fileName;
    fileName.Format("%s.staticmesh.xml", meshName);
    AutoReleasePtr<ByteStream> pMeshStream = g_pVirtualFileSystem->OpenFile(fileName, BYTESTREAM_OPEN_READ | BYTESTREAM_OPEN_STREAMED);
    if (pMeshStream == nullptr)
    {
        Log_ErrorPrintf("Could not open file '%s'", fileName.GetCharArray());
        return false;
    }
    
    StaticMeshGenerator *pGenerator = new StaticMeshGenerator();
    if (!pGenerator->LoadFromXML(fileName, pMeshStream))
    {
        Log_ErrorPrintf("Could not parse file '%s'", fileName.GetCharArray());
        delete pGenerator;
        return false;
    }

    // close anything
    Close();

    // store names
    m_meshName = meshName;
    m_meshFileName = fileName;

    // swap the pointers, and do allocations
    m_pGenerator = pGenerator;
    if (!LoadMaterials())
    {
        Close();
        return false;
    }

    // create the preview mesh
    RefreshPreviewMesh();
    RefreshCollisionShapePreview();

    // update everything
    OnFileNameChanged();
    UpdateInformationTab();
    return true;
}

bool EditorSkeletalMeshEditor::Save(ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    if (!m_pGenerator->IsCompleteMesh())
    {
        pProgressCallbacks->DisplayError("The mesh is not valid (does not contain either vertices, triangles, materials, batches). It cannot be saved.");
        return false;
    }

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

bool EditorSkeletalMeshEditor::SaveAs(const char *meshName, ProgressCallbacks *pProgressCallbacks /*= ProgressCallbacks::NullProgressCallback*/)
{
    if (!m_pGenerator->IsCompleteMesh())
    {
        pProgressCallbacks->DisplayError("The mesh is not valid (does not contain either vertices, triangles, materials, batches). It cannot be saved.");
        return false;
    }

    PathString newMeshFileName;
    newMeshFileName.Format("%s.staticmesh.xml", meshName);

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
    OnFileNameChanged();
    return true;
}

void EditorSkeletalMeshEditor::Close()
{
    if (m_pGenerator == nullptr)
        return;

    delete m_pGenerator;
    m_pGenerator = nullptr;

    m_pMeshPreviewRenderProxy->DeleteAllObjects();
}

bool EditorSkeletalMeshEditor::CreateHardwareResources()
{
    Log_DevPrintf("Creating hardware resources for EditorSkeletalMeshEditor (%u x %u)", (uint32)m_ui->swapChainWidget->size().width(), (uint32)m_ui->swapChainWidget->size().height());

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

void EditorSkeletalMeshEditor::ReleaseHardwareResources()
{
    m_guiContext.ClearState();

    delete m_pWorldRenderer;
    m_pWorldRenderer = NULL;

    SAFE_RELEASE(m_pSwapChain);
    m_ui->swapChainWidget->DestroySwapChain();
    m_bHardwareResourcesCreated = false;
}

void EditorSkeletalMeshEditor::OnFileNameChanged()
{
    // set the material directory to the same directory as the mesh
    PathString importDirectory;
    FileSystem::BuildPathRelativeToFile(importDirectory, m_meshFileName, "", false, true);
    m_ui->toolImportMaterialDirectory->setText(ConvertStringToQString(importDirectory));

    // update the window title
    setWindowTitle(ConvertStringToQString(String::FromFormat("Static Mesh Editor - %s", m_meshName.GetCharArray())));
}

bool EditorSkeletalMeshEditor::LoadMaterials()
{
#if 0
    // allocate a new array
    PODArray<const Material *> materials;
    for (uint32 i = 0; i < m_pGenerator->GetMaterialCount(); i++)
    {
        const Material *pMaterial = g_pResourceManager->GetMaterial(m_pGenerator->GetMaterialName(i));
        if (pMaterial == nullptr)
            pMaterial = g_pResourceManager->GetDefaultMaterial();

        materials.Add(pMaterial);
    }

    // swap with the current array
    m_meshMaterials.Swap(materials);

    // release the old materials
    for (uint32 i = 0; i < materials.GetSize(); i++)
        materials[i]->Release();
#endif
    return true;
}

void EditorSkeletalMeshEditor::RefreshPreviewMesh()
{
#if 0
    DebugAssert(m_pGenerator != nullptr && m_pGenerator->GetVertexCount() > 0 && m_pGenerator->GetBatchCount() > 0);

    // find vertex factory flags
    uint32 vertexFactoryFlags = 0;
    if (m_pGenerator->GetVertexTextureCoordinatesEnabled())
        vertexFactoryFlags |= LOCAL_VERTEX_FACTORY_FLAG_VERTEX_TEXCOORDS;
    if (m_pGenerator->GetVertexColorsEnabled())
        vertexFactoryFlags |= LOCAL_VERTEX_FACTORY_FLAG_VERTEX_COLORS;

    // allocate vertices
    LocalVertexFactory::Vertex *pVertices = new LocalVertexFactory::Vertex[m_pGenerator->GetVertexCount()];
    for (uint32 i = 0; i < m_pGenerator->GetVertexCount(); i++)
    {
        const StaticMeshGenerator::Vertex *pSourceVertex = m_pGenerator->GetVertex(i);
        LocalVertexFactory::Vertex *pDestinationVertex = &pVertices[i];

        pDestinationVertex->Position = pSourceVertex->Position;
        pDestinationVertex->TangentX = pSourceVertex->Tangent;
        pDestinationVertex->TangentY = pSourceVertex->Binormal;
        pDestinationVertex->TangentZ = pSourceVertex->Normal;
        pDestinationVertex->TexCoord = pSourceVertex->TexCoord;
        pDestinationVertex->Color = pSourceVertex->Color;
    }

    // convert vertices to render format
    uint32 vertexSize = LocalVertexFactory::GetVertexSize(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), vertexFactoryFlags);
    byte *pGPUVertices = new byte[vertexSize * m_pGenerator->GetVertexCount()];
    LocalVertexFactory::FillVerticesBuffer(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), vertexFactoryFlags, pVertices, m_pGenerator->GetVertexCount(), pGPUVertices, vertexSize * m_pGenerator->GetVertexCount());

    // allocate indices
    uint32 *pIndices = new uint32[m_pGenerator->GetTriangleCount() * 3];
    for (uint32 i = 0; i < m_pGenerator->GetTriangleCount(); i++)
        Y_memcpy(&pIndices[i * 3], m_pGenerator->GetTriangle(i)->Indices, sizeof(uint32) * 3);

    // allocate batches
    CompositeRenderProxy::Batch *pBatches = new CompositeRenderProxy::Batch[m_pGenerator->GetBatchCount()];
    for (uint32 i = 0; i < m_pGenerator->GetBatchCount(); i++)
    {
        const StaticMeshGenerator::Batch *pSourceBatch = m_pGenerator->GetBatch(i);
        CompositeRenderProxy::Batch *pDestinationBatch = &pBatches[i];
        pDestinationBatch->FirstIndex = pSourceBatch->StartIndex;
        pDestinationBatch->IndexCount = pSourceBatch->NumIndices;
        pDestinationBatch->MaterialIndex = pSourceBatch->MaterialIndex;
    }

    // delete all preview objects
    m_pMeshPreviewRenderProxy->DeleteAllObjects();

    // allocate a new one
    uint32 objectID = m_pMeshPreviewRenderProxy->AddObject();
    m_pMeshPreviewRenderProxy->UpdateObject(objectID, VERTEX_FACTORY_TYPE_INFO(LocalVertexFactory), vertexFactoryFlags,
                                            pGPUVertices, vertexSize, m_pGenerator->GetVertexCount(),
                                            pIndices, GPU_INDEX_FORMAT_UINT32, m_pGenerator->GetTriangleCount() * 3,
                                            m_meshMaterials.GetBasePointer(), m_meshMaterials.GetSize(),
                                            pBatches, m_pGenerator->GetBatchCount(),
                                            m_pGenerator->GetBoundingBox(), m_pGenerator->GetBoundingSphere());

    // free up memory
    delete[] pBatches;
    delete[] pIndices;
    delete[] pGPUVertices;
    delete[] pVertices;
#endif
}

void EditorSkeletalMeshEditor::RefreshCollisionShapePreview()
{
    // delete all preview objects
    m_pCollisionShapePreviewRenderProxy->DeleteAllObjects();

//     // depending on the type
//     const Physics::CollisionShapeGenerator *pCollisionShapeGenerator = m_pGenerator->GetCollisionShape();
//     if (pCollisionShapeGenerator != nullptr)
//     {
//         switch (pCollisionShapeGenerator->GetType())
//         {
//         case Physics::COLLISION_SHAPE_TYPE_TRIANGLE_MESH:
//             {
// 
//             }
//             break;
//         }
//     }
}

void EditorSkeletalMeshEditor::UpdateInformationTab()
{
#if 0
    m_ui->toolInformationMinBounds->setText(ConvertStringToQString(StringConverter::Float3ToString(m_pGenerator->GetBoundingBox().GetMinBounds())));
    m_ui->toolInformationMaxBounds->setText(ConvertStringToQString(StringConverter::Float3ToString(m_pGenerator->GetBoundingBox().GetMaxBounds())));
    m_ui->toolInformationVertexCount->setText(ConvertStringToQString(StringConverter::UInt32ToString(m_pGenerator->GetVertexCount())));
    m_ui->toolInformationTriangleCount->setText(ConvertStringToQString(StringConverter::UInt32ToString(m_pGenerator->GetTriangleCount())));
    m_ui->toolInformationMaterialCount->setText(ConvertStringToQString(StringConverter::UInt32ToString(m_pGenerator->GetMaterialCount())));
    m_ui->toolInformationBatchCount->setText(ConvertStringToQString(StringConverter::UInt32ToString(m_pGenerator->GetBatchCount())));
    m_ui->toolInformationEnableTextureCoordinates->setChecked(m_pGenerator->GetVertexTextureCoordinatesEnabled());
    m_ui->toolInformationEnableVertexColors->setChecked(m_pGenerator->GetVertexColorsEnabled());

    // collision shape information
    SmallString collisionShapeInformation;
    collisionShapeInformation.AppendString("Collision shape: ");
    if (m_pGenerator->GetCollisionShape() == nullptr)
    {
        // no shape
        collisionShapeInformation.AppendString("None");
        m_ui->toolCollisionGenerateTriangleMesh->setChecked(true);
    }
    else
    {
        const Physics::CollisionShapeGenerator *pCollisionShapeGenerator = m_pGenerator->GetCollisionShape();
        switch (pCollisionShapeGenerator->GetType())
        {
        case Physics::COLLISION_SHAPE_TYPE_BOX:
            collisionShapeInformation.AppendFormattedString("Box, Extents: %s", StringConverter::Float3ToString(pCollisionShapeGenerator->GetBoxExtents()).GetCharArray());
            m_ui->toolCollisionGenerateBox->setChecked(true);
            break;

        case Physics::COLLISION_SHAPE_TYPE_SPHERE:
            collisionShapeInformation.AppendFormattedString("Sphere, radius: %s", StringConverter::FloatToString(pCollisionShapeGenerator->GetSphereRadius()).GetCharArray());
            m_ui->toolCollisionGenerateSphere->setChecked(true);
            break;

        case Physics::COLLISION_SHAPE_TYPE_TRIANGLE_MESH:
            collisionShapeInformation.AppendFormattedString("Triangle Mesh, %u triangles", pCollisionShapeGenerator->GetTriangleCount());
            m_ui->toolCollisionGenerateTriangleMesh->setChecked(true);
            break;

        case Physics::COLLISION_SHAPE_TYPE_CONVEX_HULL:
            collisionShapeInformation.AppendFormattedString("Convex hull, %u vertices", pCollisionShapeGenerator->GetConvexHullVertexCount());
            m_ui->toolCollisionGenerateConvexHull->setChecked(true);
            break;
        }
    }

    m_ui->toolCollisionInformation->setText(ConvertStringToQString(collisionShapeInformation));
#endif
}

void EditorSkeletalMeshEditor::Draw()
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
    DrawOverlays(pGPUContext);

    // clear state
    pGPUContext->ClearState(true, true, true, true);

    // present
    m_pSwapChain->SwapBuffers();

    // clear pending flag
    m_bRedrawPending = false;
}

void EditorSkeletalMeshEditor::DrawGrid()
{
    // draw a grid of 2*the mesh size
    float gridSizeX = Max(16.0f, Max(Math::Abs(m_pGenerator->GetBoundingBox().GetMinBounds().x), Math::Abs(m_pGenerator->GetBoundingBox().GetMaxBounds().x)) * 2.0f);
    float gridSizeY = Max(16.0f, Max(Math::Abs(m_pGenerator->GetBoundingBox().GetMinBounds().y), Math::Abs(m_pGenerator->GetBoundingBox().GetMaxBounds().y)) * 2.0f);
    m_guiContext.Draw3DGrid(float3(-gridSizeX, -gridSizeY, 0.0f), float3(gridSizeX, gridSizeY, 0.0f), float3(1.0f, 1.0f, 1.0f), MAKE_COLOR_R8G8B8A8_UNORM(102, 102, 102, 255));
}

void EditorSkeletalMeshEditor::DrawView()
{
    m_pWorldRenderer->DrawWorld(m_pRenderWorld, m_viewController.GetViewParameters(), NULL, NULL, NULL);
}

void EditorSkeletalMeshEditor::DrawOverlays(GPUContext *pGPUContext)
{
    //m_guiContext.DrawText(g_pRenderer->GetFixedResources()->GetDebugFont(), 16, 0, 0, "Hi");

    // draw the collision mesh
    if (m_currentTool == Tool_Collision)
    {
        const Physics::CollisionShapeGenerator *pCollisionShape = m_pGenerator->GetCollisionShape();
        if (pCollisionShape != nullptr)
        {
            switch (pCollisionShape->GetType())
            {
            case Physics::COLLISION_SHAPE_TYPE_BOX:
                m_guiContext.Draw3DWireBox(AABox(-pCollisionShape->GetBoxExtents(), pCollisionShape->GetBoxExtents()), MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 0, 255));
                break;
            }
        }
    }
}

void EditorSkeletalMeshEditor::OnFrameExecutionTriggered(float timeSinceLastFrame)
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

void EditorSkeletalMeshEditor::ConnectUIEvents()
{
    // actions
    connect(m_ui->actionOpenMesh, SIGNAL(triggered()), this, SLOT(OnActionOpenMeshClicked()));
    connect(m_ui->actionSaveMesh, SIGNAL(triggered()), this, SLOT(OnActionSaveMeshClicked()));
    connect(m_ui->actionSaveMeshAs, SIGNAL(triggered()), this, SLOT(OnActionSaveMeshAsClicked()));
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
    connect(m_ui->actionToolOperations, SIGNAL(triggered(bool)), this, SLOT(OnActionToolOperationsTriggered(bool)));
    connect(m_ui->actionToolMaterials, SIGNAL(triggered(bool)), this, SLOT(OnActionToolMaterialsTriggered(bool)));
    connect(m_ui->actionToolLOD, SIGNAL(triggered(bool)), this, SLOT(OnActionToolLODTriggered(bool)));
    connect(m_ui->actionToolCollision, SIGNAL(triggered(bool)), this, SLOT(OnActionToolCollisionTriggered(bool)));
    connect(m_ui->actionToolImport, SIGNAL(triggered(bool)), this, SLOT(OnActionToolImportTriggered(bool)));
    connect(m_ui->actionToolLightManipulator, SIGNAL(triggered(bool)), this, SLOT(OnActionToolLightManipulatorTriggered(bool)));

    // swapchain
    connect(m_ui->swapChainWidget, SIGNAL(ResizedEvent()), this, SLOT(OnSwapChainWidgetResized()));
    connect(m_ui->swapChainWidget, SIGNAL(PaintEvent()), this, SLOT(OnSwapChainWidgetPaint()));
    connect(m_ui->swapChainWidget, SIGNAL(KeyboardEvent(const QKeyEvent *)), this, SLOT(OnSwapChainWidgetKeyboardEvent(const QKeyEvent *)));
    connect(m_ui->swapChainWidget, SIGNAL(MouseEvent(const QMouseEvent *)), this, SLOT(OnSwapChainWidgetMouseEvent(const QMouseEvent *)));
    connect(m_ui->swapChainWidget, SIGNAL(WheelEvent(const QWheelEvent *)), this, SLOT(OnSwapChainWidgetWheelEvent(const QWheelEvent *)));
    connect(m_ui->swapChainWidget, SIGNAL(DropEvent(QDropEvent *)), this, SLOT(OnSwapChainWidgetDropEvent(QDropEvent *)));

    // frame event
    connect(g_pEditor, SIGNAL(OnFrameExecution(float)), this, SLOT(OnFrameExecutionTriggered(float)));

    // operations
    connect(m_ui->toolOperationsGenerateTangents, SIGNAL(clicked()), this, SLOT(OnOperationsGenerateTangentsClicked()));
    connect(m_ui->toolOperationsCenterMesh, SIGNAL(clicked()), this, SLOT(OnOperationsCenterMeshClicked()));
    connect(m_ui->toolOperationsRemoveUnusedVertices, SIGNAL(clicked()), this, SLOT(OnOperationsRemoveUnusedVerticesClicked()));
    connect(m_ui->toolOperationsRemoveUnusedTriangles, SIGNAL(clicked()), this, SLOT(OnOperationsRemoveUnusedTrianglesClicked()));

    // importer
    connect(m_ui->toolImportSelectFileName, SIGNAL(clicked()), this, SLOT(OnToolImportSelectFileNameClicked()));
    connect(m_ui->toolImportImport, SIGNAL(clicked()), this, SLOT(OnToolImportImportClicked()));
}

void EditorSkeletalMeshEditor::closeEvent(QCloseEvent *pCloseEvent)
{

}

void EditorSkeletalMeshEditor::OnActionOpenMeshClicked()
{

}

void EditorSkeletalMeshEditor::OnActionSaveMeshClicked()
{
    EditorProgressDialog progressDialog(this);
    progressDialog.show();

    Save(&progressDialog);
}

void EditorSkeletalMeshEditor::OnActionSaveMeshAsClicked()
{
    EditorResourceSaveDialog saveDialog(this, OBJECT_TYPEINFO(StaticMesh));
    if (saveDialog.exec())
    {
        String newMeshName(saveDialog.GetReturnValueResourceName());

        // attempt save
        EditorProgressDialog progressDialog(this);
        progressDialog.show();
        SaveAs(newMeshName, &progressDialog);
    }
}

void EditorSkeletalMeshEditor::OnActionCloseClicked()
{

}

void EditorSkeletalMeshEditor::OnSwapChainWidgetResized()
{
    uint32 newWidth = (uint32)m_ui->swapChainWidget->size().width();
    uint32 newHeight = (uint32)m_ui->swapChainWidget->size().height();

    // kill old swapchain refs
    if (m_pSwapChain != nullptr)
    {
        m_pSwapChain->Release();
        m_pSwapChain = NULL;
    }

    // skip full recreation if we can just do the resize
    if (m_bHardwareResourcesCreated && (m_pSwapChain = m_ui->swapChainWidget->GetSwapChain()) != nullptr)
        m_pSwapChain->AddRef();
    else
        m_bHardwareResourcesCreated = false;

    m_viewController.SetViewportDimensions(newWidth, newHeight);
    m_guiContext.SetViewportDimensions(newWidth, newHeight);

    // flag for redraw
    FlagForRedraw();
}

void EditorSkeletalMeshEditor::OnSwapChainWidgetPaint()
{
    FlagForRedraw();
}

void EditorSkeletalMeshEditor::OnSwapChainWidgetKeyboardEvent(const QKeyEvent *pKeyboardEvent)
{
    // pass to camera
    m_viewController.HandleKeyboardEvent(pKeyboardEvent);
}

void EditorSkeletalMeshEditor::OnSwapChainWidgetMouseEvent(const QMouseEvent *pMouseEvent)
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

void EditorSkeletalMeshEditor::OnSwapChainWidgetWheelEvent(const QWheelEvent *pWheelEvent)
{

}

void EditorSkeletalMeshEditor::OnSwapChainWidgetGainedFocusEvent()
{

}

void EditorSkeletalMeshEditor::OnOperationsGenerateTangentsClicked()
{
    for (uint32 i = 0; i < m_pGenerator->GetLODCount(); i++)
        m_pGenerator->GenerateTangents(i);
}

void EditorSkeletalMeshEditor::OnOperationsCenterMeshClicked()
{
    QMenu popupMenu;
    popupMenu.addAction(tr("Origin: Center"));
    popupMenu.addAction(tr("Origin: Bottom-middle-center"));

    QAction *action = popupMenu.exec(QCursor::pos());
    if (action != nullptr)
    {
        if (action->text() == tr("Origin: Center"))
            m_pGenerator->CenterMesh(StaticMeshGenerator::CenterOrigin_Center);
        else if (action->text() == tr("Origin: Bottom-middle-center"))
            m_pGenerator->CenterMesh(StaticMeshGenerator::CenterOrigin_CenterBottom);
        else
            return;

        UpdateInformationTab();
        RefreshPreviewMesh();
        FlagForRedraw();
    }
}

void EditorSkeletalMeshEditor::OnOperationsRemoveUnusedVerticesClicked()
{

}

void EditorSkeletalMeshEditor::OnOperationsRemoveUnusedTrianglesClicked()
{

}

void EditorSkeletalMeshEditor::OnToolImportSelectFileNameClicked()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    tr("Select Mesh File..."),
                                                    QStringLiteral(""),
                                                    tr("Static Mesh Formats (*.obj *.fbx *.ase *.3ds *.bsp *.md3 *.dae *.blend)"),
                                                    NULL,
                                                    0);

    if (filename.isEmpty())
        return;

    // create a progress dialog
    EditorProgressDialog progressDialog(this);
    progressDialog.show();

    // attempt to find the mesh names
    Array<String> meshNames;
    if (!AssimpStaticMeshImporter::GetFileSubMeshNames(ConvertQStringToString(filename), meshNames, &progressDialog))
        return;

    // update the combo box
    m_ui->toolImportSubMeshName->clear();
    for (uint32 i = 0; i < meshNames.GetSize(); i++)
        m_ui->toolImportSubMeshName->addItem(ConvertStringToQString(meshNames[i]));
    m_ui->toolImportSubMeshName->addItem(tr("* All Meshes"), QVariant(1u));

    // enable the import button
    m_ui->toolImportSelectFileName->setText(filename);
    m_ui->toolImportImport->setEnabled(true);
}

void EditorSkeletalMeshEditor::OnToolImportImportClicked()
{
    AssimpStaticMeshImporter::Options options;
    AssimpStaticMeshImporter::SetDefaultOptions(&options);
    options.SourcePath = ConvertQStringToString(m_ui->toolImportSelectFileName->text());
    options.ImportMaterials = m_ui->toolImportMaterials->isChecked();
    options.MaterialDirectory = ConvertQStringToString(m_ui->toolImportMaterialDirectory->text());
    options.MaterialPrefix = ConvertQStringToString(m_ui->toolImportMaterialPrefix->text());
    options.DefaultMaterialName = g_pEngine->GetDefaultMaterialName();
    options.TransformMatrix = float4x4::MakeScaleMatrix(m_ui->toolImportScale->GetValueFloat3());

    // create a progress dialog
    EditorProgressDialog progressDialog(this);
    progressDialog.show();

    // create and run importer
    AssimpStaticMeshImporter *pImporter = new AssimpStaticMeshImporter(&options, &progressDialog);
    if (!pImporter->Execute())
    {
        progressDialog.DisplayError("Import failed. The error log may contain more information.");
        delete pImporter;
        return;
    }

    // valid?
    if (!m_pGenerator->IsCompleteMesh())
    {
        progressDialog.DisplayError("Import did not produce any usable meshes.");
        delete pImporter;
        return;
    }

    // clone the mesh
    m_pGenerator->Copy(pImporter->GetOutputGenerator());

    // and cleanup
    delete pImporter;

    // refresh the stuff
    LoadMaterials();
    UpdateInformationTab();
    RefreshPreviewMesh();
    RefreshCollisionShapePreview();
}

