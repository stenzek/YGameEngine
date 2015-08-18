#pragma once
#include "Renderer/Common.h"
#include "Renderer/Shaders/OverlayShader.h"

enum MINIGUI_HORIZONTAL_ALIGNMENT
{
    MINIGUI_HORIZONTAL_ALIGNMENT_LEFT,
    MINIGUI_HORIZONTAL_ALIGNMENT_CENTER,
    MINIGUI_HORIZONTAL_ALIGNMENT_RIGHT,
};

enum MINIGUI_VERTICAL_ALIGNMENT
{
    MINIGUI_VERTICAL_ALIGNMENT_TOP,
    MINIGUI_VERTICAL_ALIGNMENT_CENTER,
    MINIGUI_VERTICAL_ALIGNMENT_BOTTOM,
};

struct MINIGUI_RECT
{
    int32 left, right;
    int32 top, bottom;

    MINIGUI_RECT() {}
    MINIGUI_RECT(int32 left_, int32 right_, int32 top_, int32 bottom_) : left(left_) , right(right_), top(top_), bottom(bottom_) { }
    void Set(int32 left_, int32 right_, int32 top_, int32 bottom_) { left = left_; right = right_; top = top_; bottom = bottom_; }
    int32 GetHeight() const { return (bottom - top); }
    int32 GetWidth() const { return (right - left); }
};

#define SET_MINIGUI_RECT(rectPtr, leftVal, rightVal, topVal, bottomVal) { (rectPtr)->left = (leftVal); (rectPtr)->right = (rightVal); (rectPtr)->top = (topVal); (rectPtr)->bottom = (bottomVal); }

struct MINIGUI_UV_RECT
{
    float left, right;      // u direction
    float top, bottom;      // v direction

    MINIGUI_UV_RECT() {}
    MINIGUI_UV_RECT(float left_, float right_, float top_, float bottom_) : left(left_), right(right_), top(top_), bottom(bottom_) { }
    void Set(float left_, float right_, float top_, float bottom_) { left = left_; right = right_; top = top_; bottom = bottom_; }

    static const MINIGUI_UV_RECT FULL_RECT;
};

class Camera;
class Font;
class Material;
class Texture2D;
class GPUTexture;
class GPUDepthTexture;

class MiniGUIContext
{
public:
    enum BATCH_TYPE
    {
        BATCH_TYPE_NONE,
        BATCH_TYPE_2D_COLORED_LINES,
        BATCH_TYPE_2D_COLORED_TRIANGLES,
        BATCH_TYPE_2D_TEXTURED_TRIANGLES,
        BATCH_TYPE_2D_TEXT,
        BATCH_TYPE_3D_COLORED_LINES,
        BATCH_TYPE_3D_COLORED_TRIANGLES,
        BATCH_TYPE_3D_TEXTURED_TRIANGLES,
        BATCH_TYPE_3D_TEXT,
    };

    enum ALPHABLENDING_MODE
    {
        ALPHABLENDING_MODE_NONE,
        ALPHABLENDING_MODE_STRAIGHT,
        ALPHABLENDING_MODE_PREMULTIPLIED,
        ALPHABLENDING_MODE_COUNT,
    };

    enum IMMEDIATE_PRIMITIVE
    {
        //IMMEDIATE_PRIMITIVE_POINTS,
        IMMEDIATE_PRIMITIVE_LINES,
        IMMEDIATE_PRIMITIVE_LINE_STRIP,
        IMMEDIATE_PRIMITIVE_LINE_LOOP,
        IMMEDIATE_PRIMITIVE_TRIANGLES,
        IMMEDIATE_PRIMITIVE_TRIANGLE_STRIP,
        IMMEDIATE_PRIMITIVE_TRIANGLE_FAN,
        IMMEDIATE_PRIMITIVE_QUADS,
        IMMEDIATE_PRIMITIVE_QUAD_STRIP,
        IMMEDIATE_PRIMITIVE_COUNT,
    };

public:
    MiniGUIContext(GPUContext *pGPUContext = nullptr, uint32 viewportWidth = 1, uint32 viewportHeight = 1);
    ~MiniGUIContext();

    // context
    GPUContext *GetGPUContext() const { return m_pGPUContext; }
    void SetGPUContext(GPUContext *pGPUContext) { m_pGPUContext = pGPUContext; }

    // state
    const uint32 GetViewportWidth() const { return m_viewportWidth; }
    const uint32 GetViewportHeight() const { return m_viewportHeight; }
    const bool GetDepthTestingEnabled() const { return m_depthTestingEnabled; }
    const bool GetAlphaBlendingEnabled() const { return (m_alphaBlendingMode != ALPHABLENDING_MODE_NONE); }
    const ALPHABLENDING_MODE GetAlphaBlendingMode() const { return m_alphaBlendingMode; }
    const bool GetManualFlushEnabled() const { return (m_manualFlushCount > 0); }
    void SetViewportDimensions(uint32 width, uint32 height);
    void SetViewportDimensions(const RENDERER_VIEWPORT *pViewport);
    void SetDepthTestingEnabled(bool enabled);
    void SetAlphaBlendingEnabled(bool enabled);
    void SetAlphaBlendingMode(ALPHABLENDING_MODE mode);
    void PushManualFlush();
    void PopManualFlush();
    void ClearState();

    // rect manipulation
    const MINIGUI_RECT GetTopRect() const { return m_topRect; }
    void PushRect(const MINIGUI_RECT *pRect);
    void PopRect();

    // immediate mode drawing
    void BeginImmediate2D(IMMEDIATE_PRIMITIVE primitive);
    void BeginImmediate2D(IMMEDIATE_PRIMITIVE primitive, const Texture2D *pTexture);
    void BeginImmediate2D(IMMEDIATE_PRIMITIVE primitive, GPUTexture *pGPUTexture);
    void BeginImmediate3D(IMMEDIATE_PRIMITIVE primitive);
    void BeginImmediate3D(IMMEDIATE_PRIMITIVE primitive, const Texture2D *pTexture);
    void BeginImmediate3D(IMMEDIATE_PRIMITIVE primitive, GPUTexture *pGPUTexture);
    void ImmediateVertex(const float2 &position);
    void ImmediateVertex(const float2 &position, const float2 &texCoord);
    void ImmediateVertex(const float2 &position, const uint32 color);
    void ImmediateVertex(const float3 &position);
    void ImmediateVertex(const float3 &position, const float2 &texCoord);
    void ImmediateVertex(const float3 &position, const uint32 color);
    void ImmediateVertex(float x, float y);
    void ImmediateVertex(float x, float y, float u, float v);
    void ImmediateVertex(float x, float y, uint32 color);
    void ImmediateVertex(float x, float y, float z);
    void ImmediateVertex(float x, float y, float z, float u, float v);
    void ImmediateVertex(float x, float y, float z, uint32 color);
    void EndImmediate();

    // 2D drawing
    void DrawLine(const int2 &LineVertex1, const int2 &LineVertex2, const uint32 Color);
    void DrawLine(const int2 &LineVertex1, const uint32 Color1, const int2 &LineVertex2, const uint32 Color2);
    void DrawRect(const MINIGUI_RECT *pRect, const uint32 Color);
    void DrawFilledRect(const MINIGUI_RECT *pRect, const uint32 Color);
    void DrawFilledRect(const MINIGUI_RECT *pRect, const uint32 Color1, const uint32 Color2, const uint32 Color3, const uint32 Color4);
    void DrawGradientRect(const MINIGUI_RECT *pRect, const uint32 Color1, const uint32 Color2, bool Horizontal = false);    
    void DrawTexturedRect(const MINIGUI_RECT *pRect, const MINIGUI_UV_RECT *pUVRect, const Texture2D *pTexture);
    void DrawTexturedRect(const MINIGUI_RECT *pRect, const MINIGUI_UV_RECT *pUVRect, GPUTexture *pDeviceTexture);
    void DrawEmissiveRect(const MINIGUI_RECT *pRect, const Material *pMaterial);

    void DrawText(const Font *pFontData, int32 Size, const MINIGUI_RECT *pRect, const char *Text, const uint32 Color = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), bool AllowFormatting = false,
                  MINIGUI_HORIZONTAL_ALIGNMENT hAlign = MINIGUI_HORIZONTAL_ALIGNMENT_LEFT, MINIGUI_VERTICAL_ALIGNMENT vAlign = MINIGUI_VERTICAL_ALIGNMENT_TOP);

    void DrawText(const Font *pFontData, int32 Size, const int2 &Position, const char *Text, const uint32 Color = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), bool AllowFormatting = false,
                  MINIGUI_HORIZONTAL_ALIGNMENT hAlign = MINIGUI_HORIZONTAL_ALIGNMENT_LEFT, MINIGUI_VERTICAL_ALIGNMENT vAlign = MINIGUI_VERTICAL_ALIGNMENT_TOP);

    void DrawText(const Font *pFontData, int32 Size, int32 x, int32 y, const char *Text, const uint32 Color = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), bool AllowFormatting = false,
                  MINIGUI_HORIZONTAL_ALIGNMENT hAlign = MINIGUI_HORIZONTAL_ALIGNMENT_LEFT, MINIGUI_VERTICAL_ALIGNMENT vAlign = MINIGUI_VERTICAL_ALIGNMENT_TOP);

    // 3D drawing
    void Draw3DLine(const float3 &p0, const float3 &p1, const uint32 color);
    void Draw3DLine(const float3 &p0, const uint32 color1, const float3 &p1, const uint32 color2);
    void Draw3DLineWidth(const float3 &p0, const float3 &p1, const uint32 color, float lineWidth, bool widthScale = true);
    void Draw3DLineWidth(const float3 &p0, const uint32 color1, const float3 &p1, const uint32 color2, float lineWidth, bool widthScale = true);
    void Draw3DRect(const float3 &p0, const float3 &p1, const uint32 color);
    void Draw3DRect(const float3 &p0, const float3 &p1, const float3 &p2, const float3 &p3, const uint32 color);
    void Draw3DRectWidth(const float3 &p0, const float3 &p1, const uint32 color, float lineWidth, bool widthScale = true);
    void Draw3DRectWidth(const float3 &p0, const float3 &p1, const float3 &p2, const float3 &p3, const uint32 color, float lineWidth, bool widthScale = true);
    void Draw3DFilledRect(const float3 &p0, const float3 &p1, const uint32 color);
    void Draw3DFilledRect(const float3 &p0, const uint32 color0, const float3 &p1, const uint32 color1);
    void Draw3DFilledRect(const float3 &p0, const float3 &p1, const float3 &p2, const float3 &p3, const uint32 color);
    void Draw3DFilledRect(const float3 &p0, const uint32 color0, const float3 &p1, const uint32 color1, const float3 &p2, const uint32 color2, const float3 &p3, const uint32 color3);
    void Draw3DGradientRect(const float3 &p0, const float3 &p1, const uint32 fromColor, const uint32 toColor, bool horizontal = false);    
    void Draw3DTexturedRect(const float3 &p0, const float3 &p1, const MINIGUI_UV_RECT *pUVRect, const Texture2D *pTexture);
    void Draw3DTexturedRect(const float3 &p0, const float3 &p1, const MINIGUI_UV_RECT *pUVRect, GPUTexture *pDeviceTexture);
    void Draw3DTexturedRect(const float3 &p0, const float3 &p1, const float3 &p2, const float3 &p3, const MINIGUI_UV_RECT *pUVRect, const Texture2D *pTexture);
    void Draw3DTexturedRect(const float3 &p0, const float3 &p1, const float3 &p2, const float3 &p3, const MINIGUI_UV_RECT *pUVRect, GPUTexture *pDeviceTexture);
    void Draw3DEmissiveRect(const float3 &p0, const float3 &p1, const float3 &p2, const float3 &p3, const Material *pMaterial);
    void Draw3DGrid(const Plane &groundPlane, const float3 &origin, const float gridWidth, const float gridHeight, const float lineStep = 0.0f, const uint32 lineColor = 0);
    void Draw3DGrid(const float3 &minCoordinates, const float3 &maxCoordinates, const float3 &gridStep, const uint32 lineColor);
    void Draw3DWireBox(const float3 &minBounds, const float3 &maxBounds, const uint32 color);
    void Draw3DWireBox(const AABox &box, const uint32 color) { Draw3DWireBox(box.GetMinBounds(), box.GetMaxBounds(), color); }
    void Draw3DWireSphere(const float3 &sphereCenter, float radius, uint32 color, uint32 slices = 16, uint32 stacks = 16);
    void Draw3DBox(const float3 &minBounds, const float3 &maxBounds, const uint32 color);
    void Draw3DBox(const AABox &box, const uint32 color) { Draw3DBox(box.GetMinBounds(), box.GetMaxBounds(), color); }
    void Draw3DSphere(const float3 &sphereCenter, float radius, uint32 color, uint32 slices = 16, uint32 stacks = 16);
    void Draw3DArrow(const float3 &arrowStart, const float3 &arrowDirection, float lineLength, float lineThickness, float tipLength, float tipRadius, uint32 color, uint32 tipVertexCount = 24);

    // text drawing interface

    // caret position
    const int32 GetTextCaretPositionX() const { return m_textCaretPositionX; }
    const int32 GetTextCaretPositionY() const { return m_textCaretPositionY; }
    void SetTextCaretPosition(int32 x, int32 y) { m_textCaretPositionX = x; m_textCaretPositionY = y; }
    
    // word wrap
    const bool GetTextWordWrap() const { return m_textWordWrap; }
    void SetTextWordWrap(bool wordWrap) { m_textWordWrap = wordWrap; }

    // text alignment
    const MINIGUI_HORIZONTAL_ALIGNMENT GetTextHorizontalAlign() const { return m_textHorizontalAlign;}
    const MINIGUI_VERTICAL_ALIGNMENT GetTextVerticalAlign() const { return m_textVerticalAlign; }
    void SetTextHorizontalAlign(MINIGUI_HORIZONTAL_ALIGNMENT horizontalAlign) { m_textHorizontalAlign = horizontalAlign; }
    void SetTextVerticalAlign(MINIGUI_VERTICAL_ALIGNMENT verticalAlign) { m_textVerticalAlign = verticalAlign; }

    // draw text using current state
    void DrawText(const Font *pFont, int32 size, uint32 color, const char *text);
    void DrawFormattedText(const Font *pFont, int32 size, uint32 color, const char *format, ...);

    // draw text directly to point
    void DrawTextAt(int32 x, int32 y, const Font *pFont, int32 size, uint32 color, const char *text);
    void DrawFormattedTextAt(int32 x, int32 y, const Font *pFont, int32 size, uint32 color, const char *format, ...);

    // 
    void Flush();

private:
    typedef MemArray<MINIGUI_RECT> RectStack;
    typedef MemArray<OverlayShader::Vertex2D> Vertex2DArray;
    typedef MemArray<OverlayShader::Vertex3D> Vertex3DArray;

    // target
    GPUContext *m_pGPUContext;
    uint32 m_viewportWidth, m_viewportHeight;

    // state
    bool m_depthTestingEnabled;
    ALPHABLENDING_MODE m_alphaBlendingMode;
    uint32 m_manualFlushCount;

    // rect stack
    RectStack m_rectStack;
    MINIGUI_RECT m_topRect;

    // immediate mode properties
    IMMEDIATE_PRIMITIVE m_immediatePrimitive;
    uint32 m_immediateStartVertex;

    // text
    int32 m_textCaretPositionX;
    int32 m_textCaretPositionY;
    MINIGUI_HORIZONTAL_ALIGNMENT m_textHorizontalAlign;
    MINIGUI_VERTICAL_ALIGNMENT m_textVerticalAlign;
    bool m_textWordWrap;
   
    // batch
    BATCH_TYPE m_batchType;
    GPUTexture *m_pBatchTexture;
    Vertex2DArray m_batchVertices2D;
    Vertex3DArray m_batchVertices3D;

    // shaders


    // rect functions
    bool ValidateRect(const MINIGUI_RECT *pRect);
    void TranslateRect(MINIGUI_RECT *pDestinationRect, const MINIGUI_RECT *pSourceRect);
    void UntranslateRect(MINIGUI_RECT *pDestinationRect, const MINIGUI_RECT *pSourceRect);
    void ClipRect(MINIGUI_RECT *pDestinationRect, const MINIGUI_RECT *pSourceRect);
    void TranslateAndClipRect(MINIGUI_RECT *pDestinationRect, const MINIGUI_RECT *pSourceRect);
    void ClipUVRect(MINIGUI_UV_RECT *pDestinationUVRect, const MINIGUI_UV_RECT *pSourceUVRect, const MINIGUI_RECT *pClippedRect, const MINIGUI_RECT *pOriginalRect);

    // point functions
    int2 TranslateAndClipPoint(const int2 &Point);

    void InternalDrawText(const Font *pFontData, float scale, uint32 color, const MINIGUI_RECT *pRect, const char *text, uint32 nCharacters, bool skipFlushCheck);
    void SetBatchType(BATCH_TYPE type);
    void AddVertices(const OverlayShader::Vertex2D *pVertices, uint32 nVertices);
    void AddVertices(const OverlayShader::Vertex3D *pVertices, uint32 nVertices);
    void ImmediateVertex(const OverlayShader::Vertex2D &vertex);
    void ImmediateVertex(const OverlayShader::Vertex3D &vertex);
};
