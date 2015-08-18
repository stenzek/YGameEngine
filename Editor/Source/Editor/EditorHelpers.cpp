#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Editor/MapEditor/EditorEntityIdWorldRenderer.h"
#include "Editor/EditorVisual.h"
#include "Editor/Editor.h"
#include "Renderer/Renderer.h"
#include "Renderer/WorldRenderer.h"
#include "ResourceCompiler/ObjectTemplate.h"
#include "ResourceCompiler/ObjectTemplateManager.h"
#include "Core/Image.h"
#include "Core/PixelFormat.h"
Log_SetChannel(EditorHelpers);

// void EditorHelpers::DrawGrid(const float &GridWidth, const float &GridHeight, const float &GridStep)
// {
//     static const uint32 verticesPerDraw = 16384;
//     PlainVertexFactory::Vertex gridVertices[verticesPerDraw];
//     float gridBoundsX = Min(65536.0f, GridWidth) * 0.5f;
//     float gridBoundsY = Min(65536.0f, GridHeight) * 0.5f;
//     float x, y;
//     uint32 i;
//     uint32 nVertices = 0;
// 
//     // cache device pointers
//     GPUContext *pGPUDevice = g_pRenderer->GetMainContext();
//     GPUContextConstants *pGPUConstants = pGPUDevice->GetConstants();
// 
//     // set constants
//     pGPUConstants->SetLocalToWorldMatrix(Matrix4::Identity, false);
//     pGPUConstants->CommitChanges();
// 
//     // setup renderer
//     pGPUDevice->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_BACK));
//     pGPUDevice->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false), 0);
//     pGPUDevice->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateNoBlending());
//     pGPUDevice->SetDrawTopology(DRAW_TOPOLOGY_LINE_LIST);
// 
//     // calc number of rows, columns
//     uint32 drawRows = (uint32)Y_ceilf(gridBoundsX * 2.0f / GridStep);
//     uint32 drawColumns = (uint32)Y_ceilf(gridBoundsY * 2.0f / GridStep);
// 
//     // fill in vertices common data
//     uint32 verticesToInitialize = Min(verticesPerDraw, (drawRows + drawColumns + 2) * 2);
//     for (i = 0; i < verticesToInitialize; i++)
//     {
//         gridVertices[i].TexCoord.SetZero();
//         gridVertices[i].Color = MAKE_COLOR_R8G8B8A8_UNORM(102, 102, 102, 255);
//     }
// 
//     // draw columns
//     for (i = 0, x = 0.0f; i <= drawColumns; i++, x += GridStep)
//     {
//         gridVertices[nVertices++].Position.Set(-gridBoundsX + x, gridBoundsY, 0.0f);
//         gridVertices[nVertices++].Position.Set(-gridBoundsX + x, -gridBoundsY, 0.0f);
// 
//         if (nVertices == verticesPerDraw)
//         {
//             g_pRenderer->DrawPlainColored(pGPUDevice, gridVertices, nVertices);
//             nVertices = 0;
//         }
//     }
// 
//     // draw rows
//     for (i = 0, y = 0.0f; i <= drawRows; i++, y += GridStep)
//     {
//         gridVertices[nVertices++].Position.Set(-gridBoundsX, -gridBoundsY + y, 0.0f);
//         gridVertices[nVertices++].Position.Set(gridBoundsX, -gridBoundsY + y, 0.0f);
// 
//         if (nVertices == verticesPerDraw)
//         {
//             g_pRenderer->DrawPlainColored(pGPUDevice, gridVertices, nVertices);
//             nVertices = 0;
//         }
//     }
// 
//     // vertices still to draw?
//     if (nVertices > 0)
//         g_pRenderer->DrawPlainColored(pGPUDevice, gridVertices, nVertices);
// }

void EditorHelpers::ConvertQStringToString(String &dest, const QString &source)
{
    dest.Clear();
    
    uint32 sourceLength = (uint32)source.length();
    if (sourceLength > 0)
    {
        QByteArray sourceUtf8(source.toUtf8());
        dest.Resize(sourceLength);
        Y_memcpy(dest.GetWriteableCharArray(), sourceUtf8.data(), sourceLength);
        dest.GetWriteableCharArray()[sourceLength] = 0;
    }
}

String EditorHelpers::ConvertQStringToString(const QString &source)
{
    String returnValue;
    ConvertQStringToString(returnValue, source);
    return returnValue;
}

QString EditorHelpers::ConvertStringToQString(const String &source)
{
    QString returnValue(QString::fromUtf8(source.GetCharArray(), source.GetLength()));
    return returnValue;
}

bool EditorHelpers::ConvertImageToQImage(const Image &image, QImage *pDestinationImage, bool premultiplyAlpha /* = true */)
{
    const PIXEL_FORMAT_INFO *pImagePixelFormatInfo = PixelFormat_GetPixelFormatInfo(image.GetPixelFormat());
    if (pImagePixelFormatInfo->HasAlpha)
    {
        Image tempImage;
        const Image *pCopyImage = &image;
        if (image.GetPixelFormat() != PIXEL_FORMAT_B8G8R8A8_UNORM)
        {
            if (!tempImage.CopyAndConvertPixelFormat(image, PIXEL_FORMAT_B8G8R8A8_UNORM))
                return false;

            pCopyImage = &tempImage;
        }

        QImage::Format qImageFormat = QImage::Format_ARGB32;
        if (premultiplyAlpha)
        {
            uint32 nPixels = pCopyImage->GetWidth() * pCopyImage->GetHeight();
            uint32 *pPixelData = (uint32 *)pCopyImage->GetData();
            for (uint32 i = 0; i < nPixels; i++)
            {
                byte *pPixelDataBytes = (byte *)pPixelData;
                
                // this doesn't actually match up with the ordering in the pixel data, but since 
                // we are pulling and saving in the same order it doesn't actually matter
                float3 color((float)pPixelDataBytes[0], (float)pPixelDataBytes[1], (float)pPixelDataBytes[2]);
                color *= (float)pPixelDataBytes[3] / 255.0f;                
                pPixelDataBytes[0] = (byte)Math::Clamp(Math::Truncate(color.r), 0, 255);
                pPixelDataBytes[1] = (byte)Math::Clamp(Math::Truncate(color.g), 0, 255);
                pPixelDataBytes[2] = (byte)Math::Clamp(Math::Truncate(color.b), 0, 255);

                pPixelData++;
            }

            qImageFormat = QImage::Format_ARGB32_Premultiplied;
        }

        *pDestinationImage = QImage(pCopyImage->GetData(), pCopyImage->GetWidth(), pCopyImage->GetHeight(), qImageFormat).copy();
        return true;
    }
    else
    {
        Image tempImage;
        const Image *pCopyImage = &image;
        if (image.GetPixelFormat() != PIXEL_FORMAT_B8G8R8X8_UNORM)
        {
            if (!tempImage.CopyAndConvertPixelFormat(image, PIXEL_FORMAT_B8G8R8X8_UNORM))
                return false;

            pCopyImage = &tempImage;
        }

        *pDestinationImage = QImage(pCopyImage->GetData(), pCopyImage->GetWidth(), pCopyImage->GetHeight(), QImage::Format_RGB32).copy();
        return true;
    }
}

bool EditorHelpers::ConvertImageToQPixmap(const Image &image, QPixmap *pDestinationPixmap, bool premultiplyAlpha /* = true */)
{
    QImage qImage;
    if (!ConvertImageToQImage(image, &qImage, premultiplyAlpha))
        return false;

    *pDestinationPixmap = QPixmap::fromImage(qImage);
    return true;
}

bool EditorHelpers::ConvertQImageToImage(const QImage *pSourceImage, Image *pDestinationImage, PIXEL_FORMAT pixelFormat /*= PIXEL_FORMAT_UNKNOWN*/)
{
    /*switch (pSourceImage->format())
    {
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        {
            pDestinationImage->Create(PIXEL_FORMAT_R8G8B8A8_UNORM, pSourceImage->width(), pSourceImage->height(), 1);
            
            byte *pDataPointer = pDestinationImage->GetData();
            for (uint32 y = 0; y < pDestinationImage->GetHeight(); y++)
            {
                byte *pDataRowPointer = pDataPointer;

                for (uint32 x = 0; x < pDestinationImage->GetWidth(); x++)
                {
                    QRgb sourcePixelValue = pSourceImage->pixel(x, y);
                    
                    *(pDataRowPointer++) = qRed(sourcePixelValue);
                    *(pDataRowPointer++) = qBlue(sourcePixelValue);
                    *(pDataRowPointer++) = qGreen(sourcePixelValue);
                    *(pDataRowPointer++) = qAlpha(sourcePixelValue);
                }

                pDataPointer += pDestinationImage->GetDataRowPitch();
            }
        }
        break;
    }*/

    if (pSourceImage->hasAlphaChannel())
    {
        if (pSourceImage->format() != QImage::Format_ARGB32)
        {
            QImage imageCopy(pSourceImage->convertToFormat(QImage::Format_ARGB32));
            pDestinationImage->Create(PIXEL_FORMAT_R8G8B8A8_UNORM, imageCopy.width(), imageCopy.height(), 1);
            
            byte *pDestinationPointer = pDestinationImage->GetData();
            for (int scanline = 0; scanline < imageCopy.height(); scanline++)
            {
                Y_memcpy(pDestinationPointer, imageCopy.scanLine(scanline), Min(pDestinationImage->GetDataRowPitch(), (uint32)imageCopy.bytesPerLine()));
                pDestinationPointer += pDestinationImage->GetDataRowPitch();
            }
        }
        else
        {
            pDestinationImage->Create(PIXEL_FORMAT_R8G8B8A8_UNORM, pSourceImage->width(), pSourceImage->height(), 1);

            byte *pDestinationPointer = pDestinationImage->GetData();
            for (int scanline = 0; scanline < pSourceImage->height(); scanline++)
            {
                Y_memcpy(pDestinationPointer, pSourceImage->scanLine(scanline), Min(pDestinationImage->GetDataRowPitch(), (uint32)pSourceImage->bytesPerLine()));
                pDestinationPointer += pDestinationImage->GetDataRowPitch();
            }
        }
    }
    else
    {
        if (pSourceImage->format() != QImage::Format_RGB32)
        {
            QImage imageCopy(pSourceImage->convertToFormat(QImage::Format_RGB32));
            pDestinationImage->Create(PIXEL_FORMAT_R8G8B8A8_UNORM, imageCopy.width(), imageCopy.height(), 1);

            byte *pDestinationPointer = pDestinationImage->GetData();
            for (int scanline = 0; scanline < imageCopy.height(); scanline++)
            {
                Y_memcpy(pDestinationPointer, imageCopy.scanLine(scanline), Min(pDestinationImage->GetDataRowPitch(), (uint32)imageCopy.bytesPerLine()));
                pDestinationPointer += pDestinationImage->GetDataRowPitch();
            }
        }
        else
        {
            pDestinationImage->Create(PIXEL_FORMAT_R8G8B8A8_UNORM, pSourceImage->width(), pSourceImage->height(), 1);

            byte *pDestinationPointer = pDestinationImage->GetData();
            for (int scanline = 0; scanline < pSourceImage->height(); scanline++)
            {
                Y_memcpy(pDestinationPointer, pSourceImage->scanLine(scanline), Min(pDestinationImage->GetDataRowPitch(), (uint32)pSourceImage->bytesPerLine()));
                pDestinationPointer += pDestinationImage->GetDataRowPitch();
            }
        }
    }

    if (pixelFormat != PIXEL_FORMAT_UNKNOWN && !pDestinationImage->ConvertPixelFormat(pixelFormat))
        return false;

    return true;
}

QString EditorHelpers::GetEditorResourcePath(const char *filename)
{
    PathString sstr;
    sstr.Format("engine/resources/editor/%s", filename);
    FileSystem::BuildOSPath(sstr);

    return ConvertStringToQString(sstr);
}

WorldRenderer *EditorHelpers::CreateWorldRendererForRenderMode(EDITOR_RENDER_MODE renderMode, GPUContext *pGPUContext, uint32 viewportFlags, uint32 viewportWidth, uint32 viewportHeight)
{
    WorldRenderer::Options options;
    options.InitFromCVars();
    options.EnableShadows = (viewportFlags & EDITOR_VIEWPORT_FLAG_ENABLE_SHADOWS);
    options.ShowDebugInfo = (viewportFlags & EDITOR_VIEWPORT_FLAG_ENABLE_DEBUG_INFO);
    options.ShowWireframeOverlay = (viewportFlags & EDITOR_VIEWPORT_FLAG_WIREFRAME_OVERLAY);
    options.RenderWidth = viewportWidth;
    options.RenderHeight = viewportHeight;

    switch (renderMode)
    {
    case EDITOR_RENDER_MODE_LIT:
        break;

    case EDITOR_RENDER_MODE_FULLBRIGHT:
        options.RenderModeFullbright = true;
        break;

    case EDITOR_RENDER_MODE_WIREFRAME:
        options.RenderModeNormals = true;
        break;

    case EDITOR_RENDER_MODE_LIGHTING_ONLY:
        options.RenderModeLightingOnly = true;
        break;
    }

    WorldRenderer *pWorldRenderer = WorldRenderer::Create(pGPUContext, &options);
    if (pWorldRenderer == nullptr)
        Panic("Failed to create world renderer");

    return pWorldRenderer;
}

WorldRenderer *EditorHelpers::CreatePickingWorldRenderer(GPUContext *pGPUContext, uint32 viewportWidth, uint32 viewportHeight)
{
    // use defaults, since most of them will be ignored anyway
    WorldRenderer::Options options;
    options.RenderWidth = viewportWidth;
    options.RenderHeight = viewportHeight;

    WorldRenderer *pWorldRenderer = new EditorEntityIdWorldRenderer(pGPUContext, &options);
    if (!pWorldRenderer->Initialize())
        Panic("Failed to create picking world renderer");

    return pWorldRenderer;
}

void EditorHelpers::CreateEntityTypeMenu(QMenu *pMenu, bool onlyCreatable /*= true*/)
{
    // get the entity and static object types
    const ObjectTemplate *pStaticObjectTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate("StaticObject");
    const ObjectTemplate *pEntityTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate("Entity");
    DebugAssert(pStaticObjectTemplate != nullptr && pEntityTemplate != nullptr);

    PODArray<const ObjectTemplate *> items;
    ObjectTemplateManager::GetInstance().EnumerateObjectTemplates([onlyCreatable, pStaticObjectTemplate, pEntityTemplate, &items](const ObjectTemplate *pTemplate)
    {
        if (onlyCreatable && !pTemplate->CanCreate())
            return;

        // has to either be parented to entity or static object
        if (!pTemplate->IsDerivedFrom(pStaticObjectTemplate) && !pTemplate->IsDerivedFrom(pEntityTemplate))
            return;

        items.Add(pTemplate);
    });

    // sort alphabetically
    items.SortCB([](const ObjectTemplate *pLeft, const ObjectTemplate *pRight) {
        return pLeft->GetTypeName().NumericCompare(pRight->GetTypeName());
    });

    // create menu
    for (uint32 i = 0; i < items.GetSize(); i++)
    {
        QAction *pAction = pMenu->addAction(ConvertStringToQString(items[i]->GetDisplayName()));
        pAction->setToolTip(ConvertStringToQString(items[i]->GetDescription()));
        pAction->setData(ConvertStringToQString(items[i]->GetTypeName()));
    }
}

void EditorHelpers::CreateComponentTypeMenu(QMenu *pMenu, bool onlyCreatable /*= true*/)
{
    // get the entity and static object types
    const ObjectTemplate *pComponentTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate("Component");
    DebugAssert(pComponentTemplate != nullptr);

    PODArray<const ObjectTemplate *> items;
    ObjectTemplateManager::GetInstance().EnumerateObjectTemplates([onlyCreatable, pComponentTemplate, &items](const ObjectTemplate *pTemplate)
    {
        if (onlyCreatable && !pTemplate->CanCreate() || !pTemplate->IsDerivedFrom(pComponentTemplate))
            return;

        items.Add(pTemplate);
    });

    // sort alphabetically
    items.SortCB([](const ObjectTemplate *pLeft, const ObjectTemplate *pRight) {
        return pLeft->GetTypeName().NumericCompare(pRight->GetTypeName());
    });

    // create menu
    for (uint32 i = 0; i < items.GetSize(); i++)
    {
        QAction *pAction = pMenu->addAction(ConvertStringToQString(items[i]->GetDisplayName()));
        pAction->setToolTip(ConvertStringToQString(items[i]->GetDescription()));
        pAction->setData(ConvertStringToQString(items[i]->GetTypeName()));
    }
}
