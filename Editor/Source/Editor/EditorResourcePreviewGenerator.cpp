#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorResourcePreviewGenerator.h"
#include "Editor/EditorHelpers.h"
#include "Engine/ResourceManager.h"
#include "Engine/Material.h"
#include "Engine/MaterialShader.h"
#include "Engine/Texture.h"
#include "Engine/StaticMesh.h"
#include "Engine/Font.h"
#include "Engine/BlockMeshBuilder.h"
#include "Engine/ArcBallCamera.h"
#include "Renderer/VertexFactories/BlockMeshVertexFactory.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/Shaders/FullBrightShader.h"
#include "Core/Image.h"
Log_SetChannel(EditorResourcePreviewGenerator);

EditorResourcePreviewGenerator::EditorResourcePreviewGenerator()
    : m_targetWidth(0),
      m_targetHeight(0),
      m_pGPUContext(nullptr),
      m_pRenderTarget(nullptr),
      m_pRenderTargetView(nullptr),
      m_pDepthStencilBuffer(nullptr),
      m_pDepthStencilBufferView(nullptr),
      m_pWorldRenderer(nullptr),
      m_bGPUResourcesCreated(false),
      m_pOverlayTextFont(nullptr),
      m_pRenderWorld(new RenderWorld())
{
}

EditorResourcePreviewGenerator::~EditorResourcePreviewGenerator()
{
    ReleaseGPUResources();

    SAFE_RELEASE(m_pOverlayTextFont);
    m_pRenderWorld->Release();
}

bool EditorResourcePreviewGenerator::CreateGPUResources()
{
    if (m_bGPUResourcesCreated)
        return true;

    if (m_pWorldRenderer == nullptr)
        m_pWorldRenderer = EditorHelpers::CreateWorldRendererForRenderMode(EDITOR_RENDER_MODE_FULLBRIGHT, m_pGPUContext, 0, m_targetWidth, m_targetHeight);

    m_bGPUResourcesCreated = true;
    return true;
}

void EditorResourcePreviewGenerator::ReleaseGPUResources()
{
    if (m_pWorldRenderer != NULL)
        delete m_pWorldRenderer;
    
    SAFE_RELEASE(m_pDepthStencilBufferView);
    SAFE_RELEASE(m_pDepthStencilBuffer);
    SAFE_RELEASE(m_pRenderTargetView);
    SAFE_RELEASE(m_pRenderTarget);
    m_bGPUResourcesCreated = false;
}

bool EditorResourcePreviewGenerator::SetupRender(Image *pDestinationImage)
{
    if (!m_bGPUResourcesCreated && !CreateGPUResources())
        return false;

    uint32 targetWidth = pDestinationImage->GetWidth();
    uint32 targetHeight = pDestinationImage->GetHeight();
    DebugAssert(targetWidth > 0 && targetHeight > 0);

    if (m_targetWidth != targetWidth || m_targetHeight != targetHeight)
    {
        SAFE_RELEASE(m_pDepthStencilBufferView);
        SAFE_RELEASE(m_pDepthStencilBuffer);
        SAFE_RELEASE(m_pRenderTargetView);
        SAFE_RELEASE(m_pRenderTarget);
        m_targetWidth = 0;
        m_targetHeight = 0;
    }

    if (m_pRenderTarget == NULL)
    {
        GPU_TEXTURE2D_DESC renderTargetDesc;
        renderTargetDesc.Width = targetWidth;
        renderTargetDesc.Height = targetHeight;
        renderTargetDesc.Format = PIXEL_FORMAT_R8G8B8A8_UNORM;
        renderTargetDesc.Flags = GPU_TEXTURE_FLAG_READABLE | GPU_TEXTURE_FLAG_BIND_RENDER_TARGET;
        renderTargetDesc.MipLevels = 1;

        GPU_DEPTH_TEXTURE_DESC depthStencilBufferDesc;
        depthStencilBufferDesc.Width = targetWidth;
        depthStencilBufferDesc.Height = targetHeight;
        depthStencilBufferDesc.Format = PIXEL_FORMAT_D16_UNORM;
        depthStencilBufferDesc.Flags = GPU_TEXTURE_FLAG_BIND_DEPTH_STENCIL_BUFFER;

        if ((m_pRenderTarget = g_pRenderer->CreateTexture2D(&renderTargetDesc, NULL)) == NULL ||
            (m_pDepthStencilBuffer = g_pRenderer->CreateDepthTexture(&depthStencilBufferDesc)) == NULL)
        {
            Log_ErrorPrintf("EditorResourcePreviewGenerator::SetupRender: Could not create render target.");
            SAFE_RELEASE(m_pDepthStencilBuffer);
            SAFE_RELEASE(m_pRenderTarget);
            return false;
        }

        GPU_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(m_pRenderTarget, 0);
        GPU_DEPTH_STENCIL_BUFFER_VIEW_DESC depthStencilViewDesc(m_pDepthStencilBuffer);
        if ((g_pRenderer->CreateRenderTargetView(m_pRenderTarget, &renderTargetViewDesc)) == nullptr ||
            (g_pRenderer->CreateDepthStencilBufferView(m_pDepthStencilBuffer, &depthStencilViewDesc)) == nullptr)
        {
            Log_ErrorPrintf("EditorResourcePreviewGenerator::SetupRender: Could not create render target view.");
            SAFE_RELEASE(m_pDepthStencilBufferView);
            SAFE_RELEASE(m_pDepthStencilBuffer);
            SAFE_RELEASE(m_pRenderTargetView);
            SAFE_RELEASE(m_pRenderTarget);
            return false;
        }

        // update dimensions
        m_targetWidth = targetWidth;
        m_targetHeight = targetHeight;

        // create viewport
        m_viewParameters.Viewport.Set(0, 0, targetWidth, targetHeight, 0.0f, 1.0f);

        // set in context
        m_ArcBallCamera.SetPerspectiveAspect((float)targetWidth, (float)targetHeight);
        m_GUIContext.SetViewportDimensions(targetWidth, targetHeight);
    }

    // capture current state
    m_stateBlock.Capture(m_pGPUContext, GPU_STATE_BLOCK_CAPTURE_FLAG_RASTERIZER_STATE |
                                       GPU_STATE_BLOCK_CAPTURE_FLAG_DEPTHSTENCIL_STATE |
                                       GPU_STATE_BLOCK_CAPTURE_FLAG_BLEND_STATE |
                                       GPU_STATE_BLOCK_CAPTURE_FLAG_RENDER_TARGETS |
                                       GPU_STATE_BLOCK_CAPTURE_FLAG_VERTEX_BUFFERS |
                                       GPU_STATE_BLOCK_CAPTURE_FLAG_INDEX_BUFFER |
                                       GPU_STATE_BLOCK_CAPTURE_FLAG_DRAW_TOPOLOGY |
                                       GPU_STATE_BLOCK_CAPTURE_FLAG_VIEWPORTS |
                                       GPU_STATE_BLOCK_CAPTURE_FLAG_SCISSOR_RECTS |
                                       GPU_STATE_BLOCK_CAPTURE_FLAG_OBJECT_CONSTANTS |
                                       GPU_STATE_BLOCK_CAPTURE_FLAG_CAMERA_CONSTANTS);

    // setup all targets
    m_pGPUContext->SetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilBufferView);
    m_pGPUContext->SetViewport(&m_viewParameters.Viewport);

    // clear the target
    m_pGPUContext->ClearTargets(true, true, true, float4(0.0f, 0.0f, 0.0f, 1.0f));

    // set camera
    m_viewParameters.ViewCamera = m_ArcBallCamera;
    m_viewParameters.ViewCamera.SetObjectCullDistance(m_ArcBallCamera.GetFarPlaneDistance() - m_ArcBallCamera.GetNearPlaneDistance());
    m_viewParameters.MaximumShadowViewDistance = m_viewParameters.ViewCamera.GetObjectCullDistance();

    // begin scene
    return true;
}

bool EditorResourcePreviewGenerator::CompleteRender(Image *pDestinationImage)
{
    // end scene
    m_pGPUContext->ClearState(true, true, true, true);

    // restore state
    m_stateBlock.Restore();
    m_stateBlock.Clear();

    // do we need to copy to a temporary location first
    Image temporaryImage;
    Image *pReadToImage;
    if (m_pRenderTarget->GetDesc()->Format != pDestinationImage->GetPixelFormat())
    {
        temporaryImage.Create(m_pRenderTarget->GetDesc()->Format, m_targetWidth, m_targetHeight, 1);
        pReadToImage = &temporaryImage;
    }
    else
    {
        // can go direct to destination
        pReadToImage = pDestinationImage;
    }

    if (!m_pGPUContext->ReadTexture(m_pRenderTarget, pReadToImage->GetData(), pReadToImage->GetDataRowPitch(), pReadToImage->GetDataSize(), 0, 0, 0, pReadToImage->GetWidth(), pReadToImage->GetHeight()))
    {
        Log_ErrorPrintf("EditorResourcePreviewGenerator::CompleteRender: Texture readback failed.");
        return false;
    }

    // conversion necessary?
    if (pReadToImage != pDestinationImage)
    {
        if (!pDestinationImage->CopyAndConvertPixelFormat(temporaryImage, pDestinationImage->GetPixelFormat()))
        {
            Log_ErrorPrintf("EditorResourcePreviewGenerator::CompleteRender: Image format conversion failed.");
            return false;
        }
    }

    return true;
}

void EditorResourcePreviewGenerator::SetCameraMatricesFor2D()
{

}

void EditorResourcePreviewGenerator::SetCameraMatricesForArcBall()
{

}

void EditorResourcePreviewGenerator::ResetCamera()
{

}

bool EditorResourcePreviewGenerator::GenerateResourcePreview(Image *pDestinationImage, const ResourceTypeInfo *pResourceTypeInfo, const char *resourceName)
{
    AutoReleasePtr<const Resource> pResource = g_pResourceManager->UncachedGetResource(pResourceTypeInfo, resourceName);
    if (pResource == NULL)
        return false;

    return GenerateResourcePreview(pDestinationImage, pResource);
}

bool EditorResourcePreviewGenerator::GenerateResourcePreview(Image *pDestinationImage, const Resource *pResource)
{
    const ResourceTypeInfo *pResourceTypeInfo = pResource->GetResourceTypeInfo();
    
    if (pResourceTypeInfo == OBJECT_TYPEINFO(Material))
        return GenerateMaterialPreview(pDestinationImage, pResource->Cast<Material>());

    if (pResourceTypeInfo == OBJECT_TYPEINFO(MaterialShader))
        return GenerateMaterialShaderPreview(pDestinationImage, pResource->Cast<MaterialShader>());

    if (pResourceTypeInfo == OBJECT_TYPEINFO(Texture2D))
        return GenerateTexture2DPreview(pDestinationImage, pResource->Cast<Texture2D>());

    if (pResourceTypeInfo == OBJECT_TYPEINFO(StaticMesh))
        return GenerateStaticMeshPreview(pDestinationImage, pResource->Cast<StaticMesh>());

    if (pResourceTypeInfo == OBJECT_TYPEINFO(BlockPalette))
        return GenerateBlockMeshBlockListPreview(pDestinationImage, pResource->Cast<BlockPalette>());

    if (pResourceTypeInfo == OBJECT_TYPEINFO(Font))
        return GenerateFontDataPreview(pDestinationImage, pResource->Cast<Font>());

    return false;
}

bool EditorResourcePreviewGenerator::GenerateMaterialPreview(Image *pDestinationImage, const Material *pMaterial)
{
    return false;
}

bool EditorResourcePreviewGenerator::GenerateMaterialShaderPreview(Image *pDestinationImage, const MaterialShader *pMaterialShader)
{
    return false;
}

bool EditorResourcePreviewGenerator::GenerateTexture2DPreview(Image *pDestinationImage, const Texture2D *pTexture)
{
    if (!SetupRender(pDestinationImage))
        return false;

    // todo: draw checkerboard pattern behind

    // draw it
    MINIGUI_RECT rect(0, pDestinationImage->GetWidth() - 1, 0, pDestinationImage->GetHeight() - 1);
    MINIGUI_UV_RECT uvRect(0.0f, 1.0f, 0.0f, 1.0f);
    m_GUIContext.SetAlphaBlendingEnabled(false);
    m_GUIContext.DrawTexturedRect(&rect, &uvRect, pTexture);

    // complete render
    return CompleteRender(pDestinationImage);
}

bool EditorResourcePreviewGenerator::GenerateStaticMeshPreview(Image *pDestinationImage, const StaticMesh *pStaticMesh)
{
    if (!pStaticMesh->CheckGPUResources())
        return false;

    if (!SetupRender(pDestinationImage))
        return false;

    // find mesh dimensions and maximum component
    float3 meshExtents(pStaticMesh->GetBoundingBox().GetExtents());
    float maxComponent = Max(meshExtents.x, Max(meshExtents.y, meshExtents.z));

    // create an arcball camera
    m_ArcBallCamera.Reset();
    m_ArcBallCamera.SetEyeDistance(-maxComponent * 2.0f);
    m_ArcBallCamera.SetFarPlaneDistance(Min(100.0f, maxComponent * 10.0f));
    m_ArcBallCamera.SetTarget(pStaticMesh->GetBoundingBox().GetCenter());

    // setup renderer
    m_pGPUContext->GetConstants()->SetLocalToWorldMatrix(float4x4::Identity, false);
    m_pGPUContext->GetConstants()->SetFromCamera(m_ArcBallCamera, false);
    m_pGPUContext->GetConstants()->CommitChanges();

    // draw lod 0
    const StaticMesh::LOD *lod = pStaticMesh->GetLOD(0);

    // bind buffers
    m_pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);
    lod->GetVertexBuffers()->BindBuffers(m_pGPUContext);
    m_pGPUContext->SetIndexBuffer(lod->GetIndexBuffer(), lod->GetIndexFormat(), 0);

    // draw each batch
    for (uint32 batchIndex = 0; batchIndex < lod->GetBatchCount(); batchIndex++)
    {
        const StaticMesh::Batch *batch = lod->GetBatch(batchIndex);
        const Material *pMaterial = pStaticMesh->GetMaterial(batch->MaterialIndex);
        ShaderProgram *pShaderProgram = g_pRenderer->GetShaderProgram(0, SHADER_COMPONENT_INFO(FullBrightShader), 0, VERTEX_FACTORY_TYPE_INFO(LocalVertexFactory), pStaticMesh->GetVertexFactoryFlags(), pMaterial->GetShader(), pMaterial->GetShaderStaticSwitchMask());
        if (pShaderProgram != nullptr)
        {
            m_pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());
            if (pMaterial->BindDeviceResources(m_pGPUContext, pShaderProgram))
                m_pGPUContext->DrawIndexed(batch->StartIndex, batch->NumIndices, 0);
        }
    }

    return CompleteRender(pDestinationImage);
}

bool EditorResourcePreviewGenerator::GenerateFontDataPreview(Image *pDestinationImage, const Font *pFontData)
{
    return false;
}

bool EditorResourcePreviewGenerator::GenerateBlockMeshBlockListPreview(Image *pDestinationImage, const BlockPalette *pBlockList)
{
    // render all images, and then merge them to the final image
    //Image *pBlockImages = new Image[BLOCK_MESH_MAX_BLOCK_TYPES];
    //delete[] pBlockImages;

    return false;
}

bool EditorResourcePreviewGenerator::GenerateBlockMeshBlockListBlockPreview(Image *pDestinationImage, const BlockPalette *pBlockList, uint32 blockType)
{
    if (!SetupRender(pDestinationImage))
        return false;

    const BlockPalette::BlockType *pBlockType = pBlockList->GetBlockType(blockType);
    if (pBlockType->IsAllocated)
    {
        // get input layout
        uint32 factoryFlags = BlockMeshVertexFactory::GetVertexFlagsForCurrentRenderer();

        // create a temporary mesh, using a 1x1x1 volume
        BlockVolumeBlockType blockData = (BlockVolumeBlockType)blockType;
        BlockMeshBuilder meshBuilder;
        meshBuilder.SetPalette(pBlockList);
        meshBuilder.SetSize(1, 1, 1);
        meshBuilder.SetBlockData(&blockData);
        meshBuilder.SetScale(1.0f);
        meshBuilder.SetAmbientOcclusionEnabled(false);
        meshBuilder.SetTranslation(float3(-0.5f, 0.5f, 0.0f));
        meshBuilder.GenerateMesh();

        // render it
        if (meshBuilder.GetOutputBatchCount() > 0)
        {
            // static view matrix
            static const float viewMatrixValues[16] = {
                1.0000000f, 3.3378265e-009f, 1.0486073e-008f, 0.0082103405f, 
                -1.0661471e-008f, 0.52991974f, 0.84804773f, -0.97877538f, 
                -2.7261406e-009f, -0.84804773f, 0.52991974f, -1.0200906f, 
                0.0000000f, 0.0000000f, 0.0000000f, 1.0000000
            };
            float4x4 viewMatrix(viewMatrixValues);
            float4x4 projectionMatrix(float4x4::MakePerspectiveProjectionMatrix(Math::DegreesToRadians(65.0f), (float)m_targetWidth / (float)m_targetHeight, 0.1f, 100.0f));

            // set constants
            GPUContext *pGPUDevice = m_pGPUContext;
            GPUContextConstants *pGPUConstants = pGPUDevice->GetConstants();
            pGPUConstants->SetLocalToWorldMatrix(float4x4::Identity, false);
            pGPUConstants->SetCameraViewMatrix(viewMatrix, false);
            pGPUConstants->SetCameraProjectionMatrix(projectionMatrix, false);
            pGPUConstants->SetCameraEyePosition(float3(0.0f, -8.0f, 0.0f), false);
            pGPUConstants->CommitChanges();

            // set states
            pGPUDevice->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK));
            pGPUDevice->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(true, true, GPU_COMPARISON_FUNC_LESS), 0);
            pGPUDevice->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);

            // convert vertices to factory format
            uint32 vertexSize = BlockMeshVertexFactory::GetVertexSize(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), factoryFlags);
            uint32 bufferSize = meshBuilder.GetOutputVertexCount() * vertexSize;
            byte *pFactoryVertices = new byte[bufferSize];
            BlockMeshVertexFactory::FillVertices(g_pRenderer->GetPlatform(), g_pRenderer->GetFeatureLevel(), factoryFlags,
                                                 meshBuilder.GetOutputVertices().GetBasePointer(), meshBuilder.GetOutputVertexCount(), pFactoryVertices, bufferSize);


            // allocate indices
            void *pIndicesData;
            GPU_INDEX_FORMAT indexFormat;
            uint32 nIndices;
            g_pRenderer->BuildIndexBufferContents(meshBuilder.GetOutputVertexCount(), &meshBuilder.GetOutputTriangles().GetElement(0).Indices[0], 3, meshBuilder.GetOutputTriangleCount(),
                                                    sizeof(BlockMeshBuilder::Triangle), &pIndicesData, &indexFormat, &nIndices);

            // draw batches
            for (uint32 batchIndex = 0; batchIndex < meshBuilder.GetOutputBatchCount(); batchIndex++)
            {
                const BlockMeshBuilder::Batch &batch = meshBuilder.GetOutputBatches().GetElement(batchIndex);
//                 const void *pIndices = reinterpret_cast<const byte *>(pIndicesData) + (batch.StartIndex * ((indexFormat == GPU_INDEX_FORMAT_UINT32) ? sizeof(uint32) : sizeof(uint16)));
//                 g_pRenderer->DrawMaterialIndexedUserPointer(pGPUDevice, pBlockList->GetMaterial(batch.MaterialIndex),
//                                                             VERTEX_FACTORY_TYPE_INFO(BlockMeshVertexFactory), factoryFlags,
//                                                             pFactoryVertices, vertexSize, meshBuilder.GetOutputVertexCount(),
//                                                             pIndices,
//                                                             indexFormat,
//                                                             nIndices);
            }

            Y_free(pIndicesData);
            delete[] pFactoryVertices;
        }
    }

    return CompleteRender(pDestinationImage);
}

