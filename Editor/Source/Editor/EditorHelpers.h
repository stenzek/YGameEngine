#pragma once
#include "Editor/Common.h"

class Image;
class WorldRenderer;

//=================================================================================================================
// helper functions
//=================================================================================================================
namespace EditorHelpers
{
    // grid drawing
    // the grid is generated on the x/y plane, if another plane is desired, provide a matrix parameter.
    // the grid will be translated to the specified base position, so it 'appears' to be infinite.
    // this method assumes that the render target and depth stencil buffer are already set.
    //void DrawGrid(const float &GridWidth, const float &GridHeight, const float &GridStep);

    // convert a qstring to our string class
    void ConvertQStringToString(String &dest, const QString &source);
    String ConvertQStringToString(const QString &source);
    QString ConvertStringToQString(const String &source);

    // convert a image to qimage
    bool ConvertImageToQImage(const Image &image, QImage *pDestinationImage, bool premultiplyAlpha = true);
    bool ConvertImageToQPixmap(const Image &image, QPixmap *pDestinationPixmap, bool premultiplyAlpha = true);

    // convert a qimage to an image
    bool ConvertQImageToImage(const QImage *pSourceImage, Image *pDestinationImage, PIXEL_FORMAT pixelFormat = PIXEL_FORMAT_UNKNOWN);

    // build the full path to an editor resource
    QString GetEditorResourcePath(const char *filename);

    // create a world renderer for the specified view mode
    WorldRenderer *CreateWorldRendererForRenderMode(EDITOR_RENDER_MODE renderMode, GPUContext *pGPUContext, uint32 viewportFlags, uint32 viewportWidth, uint32 viewportHeight);

    // create a picking/entity id world renderer
    WorldRenderer *CreatePickingWorldRenderer(GPUContext *pGPUContext, uint32 viewportWidth, uint32 viewportHeight);

    // create a menu containing entity types
    void CreateEntityTypeMenu(QMenu *pMenu, bool onlyCreatable = true);

    // create a menu containing component types
    void CreateComponentTypeMenu(QMenu *pMenu, bool onlyCreatable = true);
};

// Use shorthand for the string functions
using EditorHelpers::ConvertStringToQString;
using EditorHelpers::ConvertQStringToString;
