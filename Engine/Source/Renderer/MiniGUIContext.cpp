#include "Renderer/PrecompiledHeader.h"
#include "Renderer/MiniGUIContext.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderProgram.h"
#include "Engine/Camera.h"
#include "Engine/Font.h"
#include "Engine/Texture.h"
Log_SetChannel(MiniGUIContext);

const MINIGUI_UV_RECT MINIGUI_UV_RECT::FULL_RECT = { 0.0f, 1.0f, 0.0f, 1.0f };

// todo: investigate using strips instead and degenerates inbetween

//#define MINIGUI_TRACING 1
#ifdef MINIGUI_TRACING
    #define MiniGUIContext_Log_TracePrint(x) Log_TracePrint(x)
    #define MiniGUIContext_Log_TracePrintf(x, ...) Log_TracePrintf(x, __VA_ARGS__)
#else
    #define MiniGUIContext_Log_TracePrint(x)
    #define MiniGUIContext_Log_TracePrintf(x, ...)
#endif

MiniGUIContext::MiniGUIContext(GPUContext *pGPUContext /* = nullptr */, uint32 viewportWidth /* = 1 */, uint32 viewportHeight /* = 1 */)
    : m_pGPUContext(pGPUContext),
      m_viewportWidth(viewportWidth),
      m_viewportHeight(viewportHeight),
      m_depthTestingEnabled(false),
      m_alphaBlendingMode(ALPHABLENDING_MODE_NONE),
      m_manualFlushCount(0),
      m_immediatePrimitive(IMMEDIATE_PRIMITIVE_COUNT),
      m_immediateStartVertex(0),
      m_textCaretPositionX(0),
      m_textCaretPositionY(0),
      m_textHorizontalAlign(MINIGUI_HORIZONTAL_ALIGNMENT_LEFT),
      m_textVerticalAlign(MINIGUI_VERTICAL_ALIGNMENT_TOP),
      m_textWordWrap(false),
      m_batchType(BATCH_TYPE_NONE),
      m_pBatchTexture(nullptr)
{
    SET_MINIGUI_RECT(&m_topRect, 0, viewportWidth, 0, viewportHeight);
}

MiniGUIContext::~MiniGUIContext()
{
    ClearState();

    DebugAssert(m_rectStack.GetSize() == 0);
    DebugAssert(m_pBatchTexture == nullptr);
}

void MiniGUIContext::SetViewportDimensions(uint32 width, uint32 height)
{
    // rect stack should be empty before changing RT
    DebugAssert(m_rectStack.GetSize() == 0);

    // clear quuee
    if (m_batchType != BATCH_TYPE_NONE)
        Flush();

    if (m_viewportWidth != width || m_viewportHeight != height)
    {
        // update vars
        m_viewportWidth = width;
        m_viewportHeight = height;
    }

    // fix rects
    SET_MINIGUI_RECT(&m_topRect, 0, width, 0, height);
}

void MiniGUIContext::SetViewportDimensions(const RENDERER_VIEWPORT *pViewport)
{
    SetViewportDimensions(pViewport->Width, pViewport->Height);
}

void MiniGUIContext::SetDepthTestingEnabled(bool enabled)
{
    if (enabled == m_depthTestingEnabled)
        return;

    Flush();
    m_depthTestingEnabled = enabled;
}

void MiniGUIContext::SetAlphaBlendingEnabled(bool enabled)
{
    SetAlphaBlendingMode((enabled) ? ALPHABLENDING_MODE_STRAIGHT : ALPHABLENDING_MODE_NONE);
}

void MiniGUIContext::SetAlphaBlendingMode(ALPHABLENDING_MODE mode)
{
    DebugAssert(mode < ALPHABLENDING_MODE_COUNT);
    if (m_alphaBlendingMode == mode)
        return;

    Flush();
    m_alphaBlendingMode = mode;
}

void MiniGUIContext::PushManualFlush()
{
    m_manualFlushCount++;
}

void MiniGUIContext::PopManualFlush()
{
    DebugAssert(m_manualFlushCount > 0);
    m_manualFlushCount--;
    if (m_manualFlushCount == 0)
        Flush();
}

void MiniGUIContext::ClearState()
{
    DebugAssert(m_manualFlushCount == 0);
    m_depthTestingEnabled = false;
    m_alphaBlendingMode = ALPHABLENDING_MODE_NONE;

    SET_MINIGUI_RECT(&m_topRect, 0, m_viewportWidth, 0, m_viewportHeight);

    m_batchType = BATCH_TYPE_NONE;
    SAFE_RELEASE(m_pBatchTexture);
    m_batchVertices2D.Clear();
    m_batchVertices3D.Clear();
}

void MiniGUIContext::PushRect(const MINIGUI_RECT *pRect)
{
    MINIGUI_RECT translatedRect;
    TranslateRect(&translatedRect, pRect);

    m_rectStack.Add(translatedRect);
    Y_memcpy(&m_topRect, &translatedRect, sizeof(m_topRect));

    MiniGUIContext_Log_TracePrintf("MiniGUIContext::PushRect(%i, %i, %i, %i)", pRect->left, pRect->right, pRect->top, pRect->bottom);
}

void MiniGUIContext::PopRect()
{
    DebugAssert(m_rectStack.GetSize() > 0);
    m_rectStack.RemoveBack();
    
    if (m_rectStack.GetSize() > 0)
        Y_memcpy(&m_topRect, &m_rectStack[m_rectStack.GetSize() - 1], sizeof(m_topRect));
    else
        SET_MINIGUI_RECT(&m_topRect, 0, m_viewportWidth, 0, m_viewportHeight);

    MiniGUIContext_Log_TracePrint("MiniGUIContext::PopRect()");
}

bool MiniGUIContext::ValidateRect(const MINIGUI_RECT *pRect)
{
    if (pRect->left > pRect->right || pRect->top > pRect->bottom)
    {
        //Log_WarningPrintf("MiniGUIContext: RECT FAILED VALIDATION: left=%i right=%i top=%i bottom=%i", pRect->left, pRect->right, pRect->top, pRect->bottom);
        MiniGUIContext_Log_TracePrintf("MiniGUIContext: RECT FAILED VALIDATION: left=%i right=%i top=%i bottom=%i", pRect->left, pRect->right, pRect->top, pRect->bottom);
        return false;
    }

    return true;
}

void MiniGUIContext::TranslateRect(MINIGUI_RECT *pDestinationRect, const MINIGUI_RECT *pSourceRect)
{
    if (m_rectStack.GetSize() > 0)
    {
        pDestinationRect->left = pSourceRect->left + m_topRect.left;
        pDestinationRect->right = pSourceRect->right + m_topRect.left;
        pDestinationRect->top = pSourceRect->top + m_topRect.top;
        pDestinationRect->bottom = pSourceRect->bottom + m_topRect.top;
    }
    else if (pSourceRect != pDestinationRect)
    {
        pDestinationRect->left = pSourceRect->left;
        pDestinationRect->right = pSourceRect->right;
        pDestinationRect->top = pSourceRect->top;
        pDestinationRect->bottom = pSourceRect->bottom;
    }
}

void MiniGUIContext::UntranslateRect(MINIGUI_RECT *pDestinationRect, const MINIGUI_RECT *pSourceRect)
{
    if (m_rectStack.GetSize() > 0)
    {
        pDestinationRect->left = pSourceRect->left - m_topRect.left;
        pDestinationRect->right = pSourceRect->right - m_topRect.left;
        pDestinationRect->top = pSourceRect->top - m_topRect.top;
        pDestinationRect->bottom = pSourceRect->bottom - m_topRect.top;
    }
    else if (pSourceRect != pDestinationRect)
    {
        pDestinationRect->left = pSourceRect->left;
        pDestinationRect->right = pSourceRect->right;
        pDestinationRect->top = pSourceRect->top;
        pDestinationRect->bottom = pSourceRect->bottom;
    }
}

void MiniGUIContext::ClipRect(MINIGUI_RECT *pDestinationRect, const MINIGUI_RECT *pSourceRect)
{
    if (m_rectStack.GetSize() > 0)
    {
        pDestinationRect->left = Max(pSourceRect->left, m_topRect.left);
        pDestinationRect->right = Min(pSourceRect->right, m_topRect.right);
        pDestinationRect->top = Max(pSourceRect->top, m_topRect.top);
        pDestinationRect->bottom = Min(pSourceRect->bottom, m_topRect.bottom);
    }
    else if (pDestinationRect != pSourceRect)
    {
        pDestinationRect->left = pSourceRect->left;
        pDestinationRect->right = pSourceRect->right;
        pDestinationRect->top = pSourceRect->top;
        pDestinationRect->bottom = pSourceRect->bottom;
    }
}

void MiniGUIContext::TranslateAndClipRect(MINIGUI_RECT *pDestinationRect, const MINIGUI_RECT *pSourceRect)
{
    MINIGUI_RECT tempRect;
    TranslateRect(&tempRect, pSourceRect);
    ClipRect(pDestinationRect, &tempRect);
}

void MiniGUIContext::ClipUVRect(MINIGUI_UV_RECT *pDestinationUVRect, const MINIGUI_UV_RECT *pSourceUVRect, const MINIGUI_RECT *pClippedRect, const MINIGUI_RECT *pOriginalRect)
{
    int32 totalWidth = pOriginalRect->right - pOriginalRect->left;
    int32 totalHeight = pOriginalRect->bottom - pOriginalRect->top;
    float fTotalWidth = (float)totalWidth;
    float fTotalHeight = (float)totalHeight;
    float fUVWidth = pSourceUVRect->right - pSourceUVRect->left;
    float fUVHeight = pSourceUVRect->bottom - pSourceUVRect->top;
    float f;

    // left clipped?
    if (pClippedRect->left > pOriginalRect->left)
    {
        f = (float)(pClippedRect->left - pOriginalRect->left) / fTotalWidth;
        pDestinationUVRect->left = pSourceUVRect->left + fUVWidth * f;
    }
    else
    {
        pDestinationUVRect->left = pSourceUVRect->left;
    }

    // top clipped?
    if (pClippedRect->top > pOriginalRect->top)
    {
        f = (float)(pClippedRect->top - pOriginalRect->top) / fTotalHeight;
        pDestinationUVRect->top = pSourceUVRect->top + fUVHeight * f;
    }
    else
    {
        pDestinationUVRect->top = pSourceUVRect->top;
    }

    // right clipped?
    if (pClippedRect->right > pOriginalRect->right)
    {
        f = (float)(pClippedRect->right - pOriginalRect->right) / fTotalWidth;
        pDestinationUVRect->right = pSourceUVRect->right + fUVWidth * f;
    }
    else
    {
        pDestinationUVRect->right = pSourceUVRect->right;
    }

    // bottom clipped?
    if (pClippedRect->bottom > pOriginalRect->bottom)
    {
        f = (float)(pClippedRect->bottom - pOriginalRect->bottom) / fTotalHeight;
        pDestinationUVRect->bottom = pSourceUVRect->bottom + fUVHeight * f;
    }
    else
    {
        pDestinationUVRect->bottom = pSourceUVRect->bottom;
    }
}

int2 MiniGUIContext::TranslateAndClipPoint(const int2 &Point)
{
    int2 ret = Point;

//     if (m_rectStack.GetSize() > 0)
//     {
//         ret.x += m_topRect.left;
//         ret.y += m_topRect.top;
//     }

    if (m_rectStack.GetSize() > 0)
    {
        ret.x = Max(Min(m_topRect.left + Point.x, m_topRect.right), m_topRect.left);
        ret.y = Max(Min(m_topRect.top + Point.y, m_topRect.bottom), m_topRect.top);
    }

    return ret;
}

void MiniGUIContext::InternalDrawText(const Font *pFontData, float scale, uint32 color, const MINIGUI_RECT *pRect, const char *text, uint32 nCharacters, bool skipFlushCheck)
{
    if (!ValidateRect(pRect))
        return;

    // last bound texture
    int32 lastTextureIndex = -1;

    // is it one of ours?
    if (m_batchType == BATCH_TYPE_2D_TEXT)
    {
        for (uint32 i = 0; i < pFontData->GetTextureCount(); i++)
        {
            if (pFontData->GetTexture(i)->GetGPUTexture() == m_pBatchTexture)
            {
                lastTextureIndex = (uint32)i;
                break;
            }
        }
    }

    // current pos
    int32 curX = pRect->left;
    int32 curY = pRect->top;

    // premultiply the alpha in color
    if ((color >> 24) != 0xFF)
    {
        float fraction = static_cast<float>(color >> 24) / 255.0f;
        color = (color & 0xFF000000) |
            static_cast<uint32>(static_cast<float>(color & 0xFF) / 255.0f * fraction * 255.0f) |
            static_cast<uint32>(static_cast<float>((color >> 8) & 0xFF) / 255.0f * fraction * 255.0f) << 8 |
            static_cast<uint32>(static_cast<float>((color >> 16) & 0xFF) / 255.0f * fraction * 255.0f) << 16;
    }

    // prep vertex fields that the font function doesn't fill in
    OverlayShader::Vertex2D Vertices[6];
    for (uint32 i = 0; i < countof(Vertices); i++)
        Vertices[i].Set(float2::Zero, float2::Zero, color);

    // generate vertices
    for (uint32 i = 0; i < nCharacters; i++)
    {
        char ch = text[i];

        // some that we skip
        if (ch == '\r' || ch == '\n' || ch == '\t')
            continue;

        // get char info
        const Font::CharacterData *pCharacterData = pFontData->GetCharacterDataForCodePoint(ch);
        if (pCharacterData == NULL)
            continue;

        // generate vertex for it
        if (pFontData->GetVertex<OverlayShader::Vertex2D>(pCharacterData, curX, curY, scale, pRect->right, pRect->bottom, Vertices))
        {
            if ((int32)pCharacterData->TextureIndex != lastTextureIndex)
            {
                Flush();

                // get texture ptr
                GPUTexture2D *pGPUTexture = static_cast<GPUTexture2D *>(pFontData->GetTexture(pCharacterData->TextureIndex)->GetGPUTexture());
                if (pGPUTexture != NULL)
                {
                    // setup batch
                    m_batchType = BATCH_TYPE_2D_TEXT;
                    m_pBatchTexture = pGPUTexture;
                    m_pBatchTexture->AddRef();

                    // set last texture
                    lastTextureIndex = pCharacterData->TextureIndex;

                    // add vertices
                    AddVertices(Vertices, countof(Vertices));
                }
            }
            else
            {
                // add vertices
                AddVertices(Vertices, countof(Vertices));
            }
        }
    }

    if (!m_manualFlushCount && !skipFlushCheck)
        Flush();
}

void MiniGUIContext::SetBatchType(BATCH_TYPE type)
{
    if (m_batchType != type && m_batchType != BATCH_TYPE_NONE)
        Flush();

    m_batchType = type;
}

void MiniGUIContext::Flush()
{
    Renderer::FixedResources *pStateManager = g_pRenderer->GetFixedResources();
    if (m_batchType == BATCH_TYPE_NONE)
        return;
       
    // rasterizer state
    m_pGPUContext->SetRasterizerState(pStateManager->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_NONE));

    // depth state -- we never test depth for 2d drawing
    if (m_batchType >= BATCH_TYPE_3D_COLORED_LINES)
        m_pGPUContext->SetDepthStencilState(pStateManager->GetDepthStencilState(m_depthTestingEnabled, false, GPU_COMPARISON_FUNC_LESS_EQUAL), 0);
    else
        m_pGPUContext->SetDepthStencilState(pStateManager->GetDepthStencilState(false, false, GPU_COMPARISON_FUNC_LESS_EQUAL), 0);
        
    // alphablending state
    if (m_batchType == BATCH_TYPE_2D_TEXT || m_batchType == BATCH_TYPE_3D_TEXT)
    {
        // force premultipled alpha for text
        m_pGPUContext->SetBlendState(pStateManager->GetBlendStatePremultipliedAlpha());
    }
    else
    {
        // use saved mode
        switch (m_alphaBlendingMode)
        {
        case ALPHABLENDING_MODE_NONE:
            m_pGPUContext->SetBlendState(pStateManager->GetBlendStateNoBlending());
            break;

        case ALPHABLENDING_MODE_STRAIGHT:
            m_pGPUContext->SetBlendState(pStateManager->GetBlendStateAlphaBlending());
            break;

        case ALPHABLENDING_MODE_PREMULTIPLIED:
            m_pGPUContext->SetBlendState(pStateManager->GetBlendStatePremultipliedAlpha());
            break;

        default:
            UnreachableCode();
            break;
        }
    }

    // select shader
    switch (m_batchType)
    {
    case BATCH_TYPE_2D_COLORED_LINES:
    case BATCH_TYPE_2D_COLORED_TRIANGLES:
        m_pGPUContext->SetShaderProgram(pStateManager->GetOverlayShaderColoredScreen()->GetGPUProgram());
        break;

    case BATCH_TYPE_2D_TEXTURED_TRIANGLES:
    case BATCH_TYPE_2D_TEXT:
        m_pGPUContext->SetShaderProgram(pStateManager->GetOverlayShaderTexturedScreen()->GetGPUProgram());
        OverlayShader::SetTexture(m_pGPUContext, pStateManager->GetOverlayShaderTexturedScreen(), m_pBatchTexture);
        break;

    case BATCH_TYPE_3D_COLORED_LINES:
    case BATCH_TYPE_3D_COLORED_TRIANGLES:
        m_pGPUContext->GetConstants()->SetLocalToWorldMatrix(float4x4::Identity, true);
        m_pGPUContext->SetShaderProgram(pStateManager->GetOverlayShaderColoredWorld()->GetGPUProgram());
        break;

    case BATCH_TYPE_3D_TEXTURED_TRIANGLES:
    case BATCH_TYPE_3D_TEXT:
        m_pGPUContext->GetConstants()->SetLocalToWorldMatrix(float4x4::Identity, true);
        m_pGPUContext->SetShaderProgram(pStateManager->GetOverlayShaderTexturedWorld()->GetGPUProgram());
        OverlayShader::SetTexture(m_pGPUContext, pStateManager->GetOverlayShaderTexturedWorld(), m_pBatchTexture);
        break;

    default:
        UnreachableCode();
        break;
    }

    // set topology
    switch (m_batchType)
    {
    case BATCH_TYPE_2D_COLORED_LINES:
    case BATCH_TYPE_3D_COLORED_LINES:
        m_pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_LINE_LIST);
        break;

    case BATCH_TYPE_2D_COLORED_TRIANGLES:
    case BATCH_TYPE_2D_TEXTURED_TRIANGLES:
    case BATCH_TYPE_2D_TEXT:
    case BATCH_TYPE_3D_COLORED_TRIANGLES:
    case BATCH_TYPE_3D_TEXTURED_TRIANGLES:
    case BATCH_TYPE_3D_TEXT:
        m_pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);
        break;

    default:
        UnreachableCode();
        break;
    }

    // invoke correct draw
    switch (m_batchType)
    {
    case BATCH_TYPE_2D_COLORED_LINES:
    case BATCH_TYPE_2D_COLORED_TRIANGLES:
    case BATCH_TYPE_2D_TEXTURED_TRIANGLES:
    case BATCH_TYPE_2D_TEXT:
        m_pGPUContext->DrawUserPointer(m_batchVertices2D.GetBasePointer(), sizeof(OverlayShader::Vertex2D), m_batchVertices2D.GetSize());
        break;

    case BATCH_TYPE_3D_COLORED_LINES:
    case BATCH_TYPE_3D_COLORED_TRIANGLES:
    case BATCH_TYPE_3D_TEXTURED_TRIANGLES:
    case BATCH_TYPE_3D_TEXT:
        m_pGPUContext->DrawUserPointer(m_batchVertices3D.GetBasePointer(), sizeof(OverlayShader::Vertex3D), m_batchVertices3D.GetSize());
        break;

    default:
        UnreachableCode();
        break;
    }

    // restore state
    m_batchType = BATCH_TYPE_NONE;
    SAFE_RELEASE(m_pBatchTexture);
    m_batchVertices2D.Clear();
    m_batchVertices3D.Clear();
}

void MiniGUIContext::BeginImmediate2D(IMMEDIATE_PRIMITIVE primitive)
{
    DebugAssertMsg(m_immediatePrimitive == IMMEDIATE_PRIMITIVE_COUNT, "Immediate draw not begun");

    switch (primitive)
    {
    case IMMEDIATE_PRIMITIVE_LINES:
    case IMMEDIATE_PRIMITIVE_LINE_STRIP:
    case IMMEDIATE_PRIMITIVE_LINE_LOOP:
        SetBatchType(BATCH_TYPE_2D_COLORED_LINES);
        break;

    case IMMEDIATE_PRIMITIVE_TRIANGLES:
    case IMMEDIATE_PRIMITIVE_TRIANGLE_STRIP:
    case IMMEDIATE_PRIMITIVE_TRIANGLE_FAN:
    case IMMEDIATE_PRIMITIVE_QUADS:
    case IMMEDIATE_PRIMITIVE_QUAD_STRIP:
        SetBatchType(BATCH_TYPE_2D_COLORED_TRIANGLES);
        break;
    }

    m_immediatePrimitive = primitive;
    m_immediateStartVertex = m_batchVertices2D.GetSize();
}

void MiniGUIContext::BeginImmediate2D(IMMEDIATE_PRIMITIVE primitive, const Texture2D *pTexture)
{
    GPUTexture *pGPUTexture = pTexture->GetGPUTexture();
    if (pGPUTexture != nullptr)
        BeginImmediate2D(primitive, pGPUTexture);
    else
        BeginImmediate2D(primitive);
}

void MiniGUIContext::BeginImmediate2D(IMMEDIATE_PRIMITIVE primitive, GPUTexture *pGPUTexture)
{
    DebugAssertMsg(m_immediatePrimitive == IMMEDIATE_PRIMITIVE_COUNT, "Immediate draw not begun");

    if (m_pBatchTexture != pGPUTexture)
        Flush();

    switch (primitive)
    {
    case IMMEDIATE_PRIMITIVE_LINES:
    case IMMEDIATE_PRIMITIVE_LINE_STRIP:
    case IMMEDIATE_PRIMITIVE_LINE_LOOP:
        SetBatchType(BATCH_TYPE_2D_COLORED_LINES);
        break;

    case IMMEDIATE_PRIMITIVE_TRIANGLES:
    case IMMEDIATE_PRIMITIVE_TRIANGLE_STRIP:
    case IMMEDIATE_PRIMITIVE_TRIANGLE_FAN:
    case IMMEDIATE_PRIMITIVE_QUADS:
    case IMMEDIATE_PRIMITIVE_QUAD_STRIP:
        SetBatchType(BATCH_TYPE_2D_TEXTURED_TRIANGLES);
        break;
    }

    DebugAssert(m_pBatchTexture == nullptr);
    m_pBatchTexture = pGPUTexture;
    m_pBatchTexture->AddRef();
    
    m_immediatePrimitive = primitive;
    m_immediateStartVertex = m_batchVertices2D.GetSize();
}

void MiniGUIContext::BeginImmediate3D(IMMEDIATE_PRIMITIVE primitive)
{
    DebugAssertMsg(m_immediatePrimitive == IMMEDIATE_PRIMITIVE_COUNT, "Immediate draw not begun");

    switch (primitive)
    {
    case IMMEDIATE_PRIMITIVE_LINES:
    case IMMEDIATE_PRIMITIVE_LINE_STRIP:
    case IMMEDIATE_PRIMITIVE_LINE_LOOP:
        SetBatchType(BATCH_TYPE_3D_COLORED_LINES);
        break;

    case IMMEDIATE_PRIMITIVE_TRIANGLES:
    case IMMEDIATE_PRIMITIVE_TRIANGLE_STRIP:
    case IMMEDIATE_PRIMITIVE_TRIANGLE_FAN:
    case IMMEDIATE_PRIMITIVE_QUADS:
    case IMMEDIATE_PRIMITIVE_QUAD_STRIP:
        SetBatchType(BATCH_TYPE_3D_COLORED_TRIANGLES);
        break;
    }

    m_immediatePrimitive = primitive;
    m_immediateStartVertex = m_batchVertices3D.GetSize();
}

void MiniGUIContext::BeginImmediate3D(IMMEDIATE_PRIMITIVE primitive, const Texture2D *pTexture)
{
    GPUTexture *pGPUTexture = pTexture->GetGPUTexture();
    if (pGPUTexture != nullptr)
        BeginImmediate3D(primitive, pGPUTexture);
    else
        BeginImmediate3D(primitive);
}

void MiniGUIContext::BeginImmediate3D(IMMEDIATE_PRIMITIVE primitive, GPUTexture *pGPUTexture)
{
    DebugAssertMsg(m_immediatePrimitive == IMMEDIATE_PRIMITIVE_COUNT, "Immediate draw not begun");

    if (m_pBatchTexture != pGPUTexture)
        Flush();

    switch (primitive)
    {
    case IMMEDIATE_PRIMITIVE_LINES:
    case IMMEDIATE_PRIMITIVE_LINE_STRIP:
    case IMMEDIATE_PRIMITIVE_LINE_LOOP:
        SetBatchType(BATCH_TYPE_3D_COLORED_LINES);
        break;

    case IMMEDIATE_PRIMITIVE_TRIANGLES:
    case IMMEDIATE_PRIMITIVE_TRIANGLE_STRIP:
    case IMMEDIATE_PRIMITIVE_TRIANGLE_FAN:
    case IMMEDIATE_PRIMITIVE_QUADS:
    case IMMEDIATE_PRIMITIVE_QUAD_STRIP:
        SetBatchType(BATCH_TYPE_3D_TEXTURED_TRIANGLES);
        break;
    }

    DebugAssert(m_pBatchTexture == nullptr);
    m_pBatchTexture = pGPUTexture;
    m_pBatchTexture->AddRef();

    m_immediatePrimitive = primitive;
    m_immediateStartVertex = m_batchVertices3D.GetSize();
}

void MiniGUIContext::AddVertices(const OverlayShader::Vertex2D *pVertices, uint32 nVertices)
{
    DebugAssert(m_batchType < BATCH_TYPE_3D_COLORED_LINES);
    m_batchVertices2D.AddRange(pVertices, nVertices);
}

void MiniGUIContext::AddVertices(const OverlayShader::Vertex3D *pVertices, uint32 nVertices)
{
    DebugAssert(m_batchType >= BATCH_TYPE_3D_COLORED_LINES);
    m_batchVertices3D.AddRange(pVertices, nVertices);
}

void MiniGUIContext::ImmediateVertex(const OverlayShader::Vertex2D &vertex)
{
    DebugAssert(m_batchType < BATCH_TYPE_3D_COLORED_LINES);

    // number of immediate vertices
    uint32 vertexCount = m_batchVertices2D.GetSize();
    uint32 immVertexPos = vertexCount - m_immediateStartVertex;
    uint32 immVertexCount = immVertexPos + 1;

    // insert extra vertices where necessary
    switch (m_immediatePrimitive)
    {
    //case IMMEDIATE_PRIMITIVE_POINTS:
    case IMMEDIATE_PRIMITIVE_LINES:
    case IMMEDIATE_PRIMITIVE_TRIANGLES:
        {
            // add vertex as normal
            m_batchVertices2D.Add(vertex);
        }
        break;

    case IMMEDIATE_PRIMITIVE_LINE_STRIP:
    case IMMEDIATE_PRIMITIVE_LINE_LOOP:
        {
            // add last vertex if not the first
            m_batchVertices2D.EnsureReserve(2);
            if (immVertexPos > 0)
                m_batchVertices2D.Add(m_batchVertices2D[vertexCount - 1]);

            // add vertex
            m_batchVertices2D.Add(vertex);
        }
        break;

    case IMMEDIATE_PRIMITIVE_TRIANGLE_STRIP:
        {
            // add the last two vertices, flipping the winding on even vertices
            if (immVertexCount >= 3)
            {
                m_batchVertices2D.EnsureReserve(3);
                if ((immVertexCount % 2) == 0)
                {
                    m_batchVertices2D.Add(m_batchVertices2D[vertexCount - 2]);
                    m_batchVertices2D.Add(m_batchVertices2D[vertexCount - 1]);
                }
                else
                {
                    m_batchVertices2D.Add(m_batchVertices2D[vertexCount - 1]);
                    m_batchVertices2D.Add(m_batchVertices2D[vertexCount - 2]);
                }
            }

            // add vertex
            m_batchVertices2D.Add(vertex);
        }
        break;

    case IMMEDIATE_PRIMITIVE_TRIANGLE_FAN:
        {
            // create triangle fan
            m_batchVertices2D.EnsureReserve(3);
            m_batchVertices2D.Add(vertex);
            m_batchVertices2D.Add(m_batchVertices2D[vertexCount - 1]);
            m_batchVertices2D.Add(m_batchVertices2D[m_immediateStartVertex]);
        }
        break;

    case IMMEDIATE_PRIMITIVE_QUADS:
        {
            // we have to add the last 2 vertices in reverse order
            uint32 currentIndex = immVertexPos % 6;
            if (currentIndex == 3)
            {
                // ensure it is large enough so the array doesn't get moved in memory
                m_batchVertices2D.EnsureReserve(3);

                // copy in case array gets resized and pointers invalidated
                uint32 baseIndex = vertexCount - 3;
                OverlayShader::Vertex2D topLeftVertex(m_batchVertices2D[baseIndex + 0]);
                OverlayShader::Vertex2D bottomLeftVertex(m_batchVertices2D[baseIndex + 1]);
                OverlayShader::Vertex2D bottomRightVertex(m_batchVertices2D[baseIndex + 2]);
                OverlayShader::Vertex2D topRightVertex(vertex);

                // reorder first triangle
                m_batchVertices2D[baseIndex + 0] = bottomLeftVertex;
                m_batchVertices2D[baseIndex + 1] = topLeftVertex;
                m_batchVertices2D[baseIndex + 2] = bottomRightVertex;

                // create second triangle
                m_batchVertices2D.Add(bottomRightVertex);
                m_batchVertices2D.Add(topLeftVertex);
                m_batchVertices2D.Add(topRightVertex);
            }
            else
            {
                // add vertex as-is
                m_batchVertices2D.Add(vertex);
            }                
        }
        break;
    }
}

void MiniGUIContext::ImmediateVertex(const OverlayShader::Vertex3D &vertex)
{
    DebugAssert(m_batchType >= BATCH_TYPE_3D_COLORED_LINES);

    // number of immediate vertices
    uint32 vertexCount = m_batchVertices3D.GetSize();
    uint32 immVertexPos = vertexCount - m_immediateStartVertex;
    uint32 immVertexCount = immVertexPos + 1;

    // insert extra vertices where necessary
    switch (m_immediatePrimitive)
    {
    //case IMMEDIATE_PRIMITIVE_POINTS:
    case IMMEDIATE_PRIMITIVE_LINES:
    case IMMEDIATE_PRIMITIVE_TRIANGLES:
        {
            // add vertex as normal
            m_batchVertices3D.Add(vertex);
        }
        break;

    case IMMEDIATE_PRIMITIVE_LINE_STRIP:
    case IMMEDIATE_PRIMITIVE_LINE_LOOP:
        {
            // add last vertex if not the first
            m_batchVertices3D.EnsureReserve(2);
            if (immVertexPos > 0)
                m_batchVertices3D.Add(m_batchVertices3D[vertexCount - 1]);

            // add vertex
            m_batchVertices3D.Add(vertex);
        }
        break;

    case IMMEDIATE_PRIMITIVE_TRIANGLE_STRIP:
        {
            // add the last two vertices, flipping the winding on even vertices
            if (immVertexCount >= 3)
            {
                m_batchVertices3D.EnsureReserve(3);
                if ((immVertexCount % 2) == 0)
                {
                    m_batchVertices3D.Add(m_batchVertices3D[vertexCount - 2]);
                    m_batchVertices3D.Add(m_batchVertices3D[vertexCount - 1]);
                }
                else
                {
                    m_batchVertices3D.Add(m_batchVertices3D[vertexCount - 1]);
                    m_batchVertices3D.Add(m_batchVertices3D[vertexCount - 2]);
                }
            }

            // add vertex
            m_batchVertices3D.Add(vertex);
        }
        break;

    case IMMEDIATE_PRIMITIVE_TRIANGLE_FAN:
        {
            // create triangle fan
            m_batchVertices3D.EnsureReserve(3);
            m_batchVertices3D.Add(vertex);
            m_batchVertices3D.Add(m_batchVertices3D[vertexCount - 1]);
            m_batchVertices3D.Add(m_batchVertices3D[m_immediateStartVertex]);
        }
        break;

    case IMMEDIATE_PRIMITIVE_QUADS:
        {
            // we have to add the last 2 vertices in reverse order
            uint32 currentIndex = immVertexPos % 6;
            if (currentIndex == 3)
            {
                // ensure it is large enough so the array doesn't get moved in memory
                m_batchVertices3D.EnsureReserve(3);

                // copy in case array gets resized and pointers invalidated
                uint32 baseIndex = vertexCount - 3;
                OverlayShader::Vertex3D topLeftVertex(m_batchVertices3D[baseIndex + 0]);
                OverlayShader::Vertex3D bottomLeftVertex(m_batchVertices3D[baseIndex + 1]);
                OverlayShader::Vertex3D bottomRightVertex(m_batchVertices3D[baseIndex + 2]);
                OverlayShader::Vertex3D topRightVertex(vertex);

                // reorder first triangle
                m_batchVertices3D[baseIndex + 0] = bottomLeftVertex;
                m_batchVertices3D[baseIndex + 1] = topLeftVertex;
                m_batchVertices3D[baseIndex + 2] = bottomRightVertex;

                // create second triangle
                m_batchVertices3D.Add(bottomRightVertex);
                m_batchVertices3D.Add(topLeftVertex);
                m_batchVertices3D.Add(topRightVertex);
            }
            else
            {
                // add vertex as-is
                m_batchVertices3D.Add(vertex);
            }                
        }
        break;
    }
}

void MiniGUIContext::EndImmediate()
{
    bool is3D = (m_batchType >= BATCH_TYPE_3D_COLORED_LINES);
    uint32 immVertexCount = (is3D ? m_batchVertices3D.GetSize() : m_batchVertices2D.GetSize()) - m_immediateStartVertex;
    if (immVertexCount > 0)
    {
        // handle primitive-specific stuff
        switch (m_immediatePrimitive)
        {
        case IMMEDIATE_PRIMITIVE_LINE_LOOP:
            {
                // line loops have to be closed
                DebugAssert(immVertexCount > 2);
                if (is3D)
                {
                    m_batchVertices2D.EnsureReserve(2);
                    m_batchVertices2D.Add(m_batchVertices2D[m_batchVertices2D.GetSize() - 1]);
                    m_batchVertices2D.Add(m_batchVertices2D[m_immediateStartVertex]);
                }
                else
                {
                    m_batchVertices3D.EnsureReserve(2);
                    m_batchVertices3D.Add(m_batchVertices3D[m_batchVertices3D.GetSize() - 1]);
                    m_batchVertices3D.Add(m_batchVertices3D[m_immediateStartVertex]);
                }
            }
            break;
        }

        // clear immediate state
        m_immediatePrimitive = IMMEDIATE_PRIMITIVE_COUNT;
        m_immediateStartVertex = 0;

        // invoke draw if not manually flushing
        if (!m_manualFlushCount)
            Flush();
    }
    else
    {
        // clear immediate state
        m_immediatePrimitive = IMMEDIATE_PRIMITIVE_COUNT;
        m_immediateStartVertex = 0;
    }
}

void MiniGUIContext::ImmediateVertex(const float2 &position)
{
    OverlayShader::Vertex2D vertex;
    vertex.Set(position, float2::Zero, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));
    ImmediateVertex(vertex);
}

void MiniGUIContext::ImmediateVertex(const float2 &position, const float2 &texCoord)
{
    OverlayShader::Vertex2D vertex;
    vertex.Set(position, texCoord, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));
    ImmediateVertex(vertex);
}

void MiniGUIContext::ImmediateVertex(const float2 &position, const uint32 color)
{
    OverlayShader::Vertex2D vertex;
    vertex.Set(position, float2::Zero, color);
    ImmediateVertex(vertex);
}

void MiniGUIContext::ImmediateVertex(const float3 &position)
{
    OverlayShader::Vertex3D vertex;
    vertex.Set(position, float2::Zero, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));
    ImmediateVertex(vertex);
}

void MiniGUIContext::ImmediateVertex(const float3 &position, const float2 &texCoord)
{
    OverlayShader::Vertex3D vertex;
    vertex.Set(position, texCoord, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));
    ImmediateVertex(vertex);
}

void MiniGUIContext::ImmediateVertex(const float3 &position, const uint32 color)
{
    OverlayShader::Vertex3D vertex;
    vertex.Set(position, float2::Zero, color);
    ImmediateVertex(vertex);
}

void MiniGUIContext::ImmediateVertex(float x, float y)
{
    OverlayShader::Vertex2D vertex;
    vertex.Set(x, y, 0.0f, 0.0f, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));
    ImmediateVertex(vertex);
}

void MiniGUIContext::ImmediateVertex(float x, float y, float u, float v)
{
    OverlayShader::Vertex2D vertex;
    vertex.Set(x, y, u, v, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));
    ImmediateVertex(vertex);
}

void MiniGUIContext::ImmediateVertex(float x, float y, uint32 color)
{
    OverlayShader::Vertex2D vertex;
    vertex.Set(x, y, 0.0f, 0.0f, color);
    ImmediateVertex(vertex);
}

void MiniGUIContext::ImmediateVertex(float x, float y, float z)
{
    OverlayShader::Vertex3D vertex;
    vertex.Set(x, y, z, 0.0f, 0.0f, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));
    ImmediateVertex(vertex);
}

void MiniGUIContext::ImmediateVertex(float x, float y, float z, float u, float v)
{
    OverlayShader::Vertex3D vertex;
    vertex.Set(x, y, z, u, v, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));
    ImmediateVertex(vertex);
}

void MiniGUIContext::ImmediateVertex(float x, float y, float z, uint32 color)
{
    OverlayShader::Vertex3D vertex;
    vertex.Set(x, y, z, 0.0f, 0.0f, color);
    ImmediateVertex(vertex);
}

void MiniGUIContext::DrawLine(const int2 &LineVertex1, const int2 &LineVertex2, const uint32 Color)
{
    int2 clippedVertex1 = TranslateAndClipPoint(LineVertex1);
    int2 clippedVertex2 = TranslateAndClipPoint(LineVertex2);

    if (m_batchType != BATCH_TYPE_2D_COLORED_LINES)
    {
        Flush();
        m_batchType = BATCH_TYPE_2D_COLORED_LINES;
    }

    OverlayShader::Vertex2D Vertices[2];
    Vertices[0].SetColor((float)clippedVertex1.x, (float)clippedVertex1.y, Color);
    Vertices[1].SetColor((float)clippedVertex2.x, (float)clippedVertex2.y, Color);
    AddVertices(Vertices, countof(Vertices));

    if (!m_manualFlushCount)
        Flush();
}

void MiniGUIContext::DrawLine(const int2 &LineVertex1, const uint32 Color1, const int2 &LineVertex2, const uint32 Color2)
{
    int2 clippedVertex1 = TranslateAndClipPoint(LineVertex1);
    int2 clippedVertex2 = TranslateAndClipPoint(LineVertex2);

    if (m_batchType != BATCH_TYPE_2D_COLORED_LINES)
    {
        Flush();
        m_batchType = BATCH_TYPE_2D_COLORED_LINES;
    }

    OverlayShader::Vertex2D Vertices[2];
    Vertices[0].SetColor((float)clippedVertex1.x, (float)clippedVertex1.y, Color1);
    Vertices[1].SetColor((float)clippedVertex2.x, (float)clippedVertex2.y, Color2);
    AddVertices(Vertices, countof(Vertices));

    if (!m_manualFlushCount)
        Flush();
}

void MiniGUIContext::DrawRect(const MINIGUI_RECT *pRect, const uint32 Color)
{
    MINIGUI_RECT realRect;
    TranslateAndClipRect(&realRect, pRect);
    if (!ValidateRect(&realRect))
        return;

    SetBatchType(BATCH_TYPE_2D_COLORED_LINES);

    OverlayShader::Vertex2D Vertices[8];
    Vertices[0].SetColor((float)realRect.left, (float)realRect.top, Color);     // top
    Vertices[1].SetColor((float)realRect.right, (float)realRect.top, Color);    // top
    Vertices[2].SetColor((float)realRect.right, (float)realRect.top, Color);    // right
    Vertices[3].SetColor((float)realRect.right, (float)realRect.bottom, Color); // right
    Vertices[4].SetColor((float)realRect.right, (float)realRect.bottom, Color); // bottom
    Vertices[5].SetColor((float)realRect.left, (float)realRect.bottom, Color);  // bottom
    Vertices[6].SetColor((float)realRect.left, (float)realRect.bottom, Color);  // left
    Vertices[7].SetColor((float)realRect.left, (float)realRect.top, Color);     // left
    AddVertices(Vertices, countof(Vertices));

    if (!m_manualFlushCount)
        Flush();

    MiniGUIContext_Log_TracePrintf("MiniGUIContext::DrawRect(%i, %i, %i, %i)", pRect->left, pRect->right, pRect->top, pRect->bottom);
}

void MiniGUIContext::DrawFilledRect(const MINIGUI_RECT *pRect, const uint32 Color)
{
    DrawFilledRect(pRect, Color, Color, Color, Color);
}

void MiniGUIContext::DrawFilledRect(const MINIGUI_RECT *pRect, const uint32 Color1, const uint32 Color2, const uint32 Color3, const uint32 Color4)
{
    // check state
    SetBatchType(BATCH_TYPE_2D_COLORED_TRIANGLES);

    // clip rect
    MINIGUI_RECT clippedRect;
    TranslateAndClipRect(&clippedRect, pRect);
    if (!ValidateRect(&clippedRect))
        return;

    // find points
    float2 p1, p2, p3, p4;
    p1.Set((float)clippedRect.left, (float)clippedRect.top);
    p2.Set((float)clippedRect.right, (float)clippedRect.top);
    p3.Set((float)clippedRect.right, (float)clippedRect.bottom);
    p4.Set((float)clippedRect.left, (float)clippedRect.bottom);

    // generate vertices
    OverlayShader::Vertex2D Vertices[6];
    Vertices[0].SetColor(p1, Color1);
    Vertices[1].SetColor(p4, Color4);
    Vertices[2].SetColor(p2, Color2);
    Vertices[3].SetColor(p2, Color2);
    Vertices[4].SetColor(p4, Color4);
    Vertices[5].SetColor(p3, Color3);
    AddVertices(Vertices, countof(Vertices));

    // flush
    if (!m_manualFlushCount)
        Flush();

    MiniGUIContext_Log_TracePrintf("MiniGUIContext::DrawFilledRect(%i, %i, %i, %i)", pRect->left, pRect->right, pRect->top, pRect->bottom);
}

void MiniGUIContext::DrawGradientRect(const MINIGUI_RECT *pRect, const uint32 Color1, const uint32 Color2, bool Horizontal /*= false*/)
{
    if (Horizontal)
        DrawFilledRect(pRect, Color1, Color2, Color2, Color1);
    else
        DrawFilledRect(pRect, Color1, Color1, Color2, Color2);
}

void MiniGUIContext::DrawTexturedRect(const MINIGUI_RECT *pRect, const MINIGUI_UV_RECT *pUVRect, const Texture2D *pTexture)
{
    GPUTexture2D *pGPUTexture = static_cast<GPUTexture2D *>(pTexture->GetGPUTexture());
    if (pGPUTexture != NULL)
        DrawTexturedRect(pRect, pUVRect, pGPUTexture);
}

void MiniGUIContext::DrawTexturedRect(const MINIGUI_RECT *pRect, const MINIGUI_UV_RECT *pUVRect, GPUTexture *pDeviceTexture)
{
    // check state
    if (m_batchType != BATCH_TYPE_2D_TEXTURED_TRIANGLES || m_pBatchTexture != pDeviceTexture)
    {
        Flush();
        m_batchType = BATCH_TYPE_2D_TEXTURED_TRIANGLES;
        m_pBatchTexture = pDeviceTexture;
        m_pBatchTexture->AddRef();
    }

    // clip rect
    MINIGUI_RECT realRect;
    MINIGUI_RECT translatedRect;
    MINIGUI_UV_RECT clippedUVRect;
    TranslateRect(&translatedRect, pRect);
    TranslateAndClipRect(&realRect, pRect);
    ClipUVRect(&clippedUVRect, pUVRect, &realRect, &translatedRect);    
    if (!ValidateRect(&realRect))
        return;

    // due to batching, we have to use triangle lists.
    OverlayShader::Vertex2D Vertices[6];
    Vertices[0].SetUV((float)realRect.left, (float)realRect.top, clippedUVRect.left, clippedUVRect.top);              // topleft
    Vertices[1].SetUV((float)realRect.left, (float)realRect.bottom, clippedUVRect.left, clippedUVRect.bottom);        // bottomleft
    Vertices[2].SetUV((float)realRect.right, (float)realRect.top, clippedUVRect.right, clippedUVRect.top);            // topright
    Vertices[3].SetUV((float)realRect.right, (float)realRect.top, clippedUVRect.right, clippedUVRect.top);            // topright
    Vertices[4].SetUV((float)realRect.left, (float)realRect.bottom, clippedUVRect.left, clippedUVRect.bottom);        // bottomleft    
    Vertices[5].SetUV((float)realRect.right, (float)realRect.bottom, clippedUVRect.right, clippedUVRect.bottom);      // bottomright
    AddVertices(Vertices, countof(Vertices));

    if (!m_manualFlushCount)
        Flush();

    MiniGUIContext_Log_TracePrintf("MiniGUIContext::DrawTexturedRect(%i, %i, %i, %i)", pRect->left, pRect->right, pRect->top, pRect->bottom);
}

void MiniGUIContext::DrawEmissiveRect(const MINIGUI_RECT *pRect, const Material *pMaterial)
{
    MiniGUIContext_Log_TracePrintf("MiniGUIContext::DrawEmissiveRect(%i, %i, %i, %i, %s)", pRect->left, pRect->right, pRect->top, pRect->bottom, pMaterial->GetName().GetCharArray());
}

void MiniGUIContext::DrawText(const Font *pFontData, int32 Size, const MINIGUI_RECT *pRect, const char *Text, const uint32 Color /* = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255) */, bool AllowFormatting /* = false */, 
                              MINIGUI_HORIZONTAL_ALIGNMENT hAlign /* = MINIGUI_HORIZONTAL_ALIGNMENT_LEFT */, MINIGUI_VERTICAL_ALIGNMENT vAlign /* = MINIGUI_VERTICAL_ALIGNMENT_TOP */)
{
    if (pFontData == NULL || !ValidateRect(pRect))
        return;

    // determine the number of lines of text, and the maximum number of characters in a line
    const char *pLineStart = Text;
    const char *pTextPointer = Text;
    uint32 maxCharactersPerLine = 0;
    uint32 nCharacters = 0;
    uint32 nLines = 1;
    for (; *pTextPointer != '\0'; pTextPointer++)
    {
        char ch = *pTextPointer;
        if (ch == '\n')
        {
            maxCharactersPerLine = Max(maxCharactersPerLine, nCharacters);
            nCharacters = 0;
            nLines++;
        }
        else
        {
            nCharacters++;
        }
    }

    // calc scale
    float Scale = (float)Size / (float)pFontData->GetHeight();

    // calculate y start position
    int32 curY;
    if (vAlign == MINIGUI_VERTICAL_ALIGNMENT_TOP)
        curY = pRect->top;
    else if (vAlign == MINIGUI_VERTICAL_ALIGNMENT_CENTER)
        curY = (int32)((float)(pRect->bottom - pRect->top) * 0.5f) - (int32)((float)Size * 0.5f) - ((nLines - 1) * Size);
    else
        curY = pRect->bottom - (nLines * Size);
    
    // draw each line of the text
    // everything will get batched together, hopefully :)
    pLineStart = Text;
    pTextPointer = pLineStart;
    for (; *pTextPointer != '\0'; )
    {
        nCharacters = 0;
        pLineStart = pTextPointer;
        while (*pTextPointer != '\n' && *pTextPointer != '\0')
        {
            nCharacters++;
            pTextPointer++;
        }

        // increment the text pointer for the next line
        if (*pTextPointer != '\0')
            pTextPointer++;

        // special case: no characters and newline, just increment the y position
        if (nCharacters == 0 && *pLineStart == '\n')
        {
            curY += Size;
            continue;
        }

        // calculate the width of the text
        int32 lineWidth = (int32)pFontData->GetTextWidth(pLineStart, nCharacters, Scale);
        if (lineWidth == 0)
        {
            curY += Size;
            continue;
        }
        
        // calculate line rect
        MINIGUI_RECT lineRect;
        lineRect.top = curY;
        lineRect.bottom = curY + (Size * 2);

        // horizontal alignment
        if (hAlign == MINIGUI_HORIZONTAL_ALIGNMENT_LEFT)
        {
            lineRect.left = pRect->left;
            lineRect.right = lineRect.left + lineWidth;
        }
        else if (hAlign == MINIGUI_HORIZONTAL_ALIGNMENT_CENTER)
        {
            float halfWidth = (float)(pRect->right - pRect->left) * 0.5f;
            float halfTextWidth = (float)(lineWidth) * 0.5f;
            lineRect.left = pRect->left + (int32)Y_floorf(halfWidth - halfTextWidth);
            lineRect.right = pRect->left + (int32)Y_ceilf(halfWidth + halfTextWidth);
        }
        else    // MINIGUI_HORIZONTAL_ALIGNMENT_RIGHT
        {
            lineRect.left = pRect->right - lineWidth;
            lineRect.right = lineRect.left + lineWidth;
        }
        
        // clip to provided bounds
        lineRect.left = Max(lineRect.left, pRect->left);
        lineRect.right = Min(lineRect.right, pRect->right);
        lineRect.top = Max(lineRect.top, pRect->top);
        lineRect.bottom = Min(lineRect.bottom, pRect->bottom);

        // translate and clip to stack rect
        MINIGUI_RECT realRect;
        TranslateAndClipRect(&realRect, &lineRect);

        // draw text
        InternalDrawText(pFontData, Scale, Color, &realRect, pLineStart, nCharacters, true);

        // increment y position
        curY += Size;
    }

    if (!m_manualFlushCount)
        Flush();

    MiniGUIContext_Log_TracePrintf("MiniGUIContext::DrawText(%i, %i, %i, %i, %s)", pRect->left, pRect->right, pRect->top, pRect->bottom, Text);
}

void MiniGUIContext::DrawText(const Font *pFontData, int32 Size, const int2 &Position, const char *Text, const uint32 Color /* = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255) */, bool AllowFormatting /* = false */,
                              MINIGUI_HORIZONTAL_ALIGNMENT hAlign /* = MINIGUI_HORIZONTAL_ALIGNMENT_LEFT */, MINIGUI_VERTICAL_ALIGNMENT vAlign /* = MINIGUI_VERTICAL_ALIGNMENT_TOP */)
{
    MINIGUI_RECT drawRect;

    if (Position.x < 0)
    {
        drawRect.left = m_topRect.right + Position.x;
        drawRect.right = m_topRect.right;
    }
    else
    {
        drawRect.left = m_topRect.left + Position.x;
        drawRect.right = m_topRect.right;
    }

    if (Position.y < 0)
    {
        drawRect.top = m_topRect.bottom + Position.y;
        drawRect.bottom = m_topRect.bottom;
    }
    else
    {
        drawRect.top = m_topRect.top + Position.y;
        drawRect.bottom = m_topRect.bottom;
    }

    UntranslateRect(&drawRect, &drawRect);
    DrawText(pFontData, Size, &drawRect, Text, Color, AllowFormatting, hAlign, vAlign);
}

void MiniGUIContext::DrawText(const Font *pFontData, int32 Size, int32 x, int32 y, const char *Text, const uint32 Color /* = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255) */, bool AllowFormatting /* = false */,
                              MINIGUI_HORIZONTAL_ALIGNMENT hAlign /* = MINIGUI_HORIZONTAL_ALIGNMENT_LEFT */, MINIGUI_VERTICAL_ALIGNMENT vAlign /* = MINIGUI_VERTICAL_ALIGNMENT_TOP */)
{
    DrawText(pFontData, Size, int2(x, y), Text, Color, AllowFormatting, hAlign, vAlign);
}

void MiniGUIContext::Draw3DLine(const float3 &p0, const float3 &p1, const uint32 color)
{
    Draw3DLine(p0, color, p1, color);
}

void MiniGUIContext::Draw3DLine(const float3 &p0, const uint32 color1, const float3 &p1, const uint32 color2)
{
    SetBatchType(BATCH_TYPE_3D_COLORED_LINES);

    OverlayShader::Vertex3D vertices[2];
    vertices[0].SetColor(p0.x, p0.y, p0.z, color1);
    vertices[1].SetColor(p1.x, p1.y, p1.z, color2);
    AddVertices(vertices, countof(vertices));

    if (!m_manualFlushCount)
        Flush();
}

void MiniGUIContext::Draw3DLineWidth(const float3 &p0, const float3 &p1, const uint32 color, float lineWidth, bool widthScale /* = true */)
{
    Draw3DLineWidth(p0, color, p1, color, lineWidth, widthScale);
}

static float ComputeConstantScale(const float3 &pos, const float4x4 &viewMatrix, const float4x4 &projectionMatrix, uint32 vpwidth)
{
    float3 ppcam0(viewMatrix.TransformPoint(pos));
    float3 ppcam1(ppcam0);
    ppcam1.x += 1.0f;

    float l1 = 1.0f/(ppcam0.x*projectionMatrix[3][0] + ppcam0.y*projectionMatrix[3][1] + ppcam0.z*projectionMatrix[3][2] + projectionMatrix[3][3]);
    float c1 = (ppcam0.x*projectionMatrix[0][0] + ppcam0.y*projectionMatrix[0][1] + ppcam0.z*projectionMatrix[0][2] + projectionMatrix[0][3])*l1;
    float l2 = 1.0f/(ppcam1.x*projectionMatrix[3][0] + ppcam1.y*projectionMatrix[3][1] + ppcam1.z*projectionMatrix[3][2] + projectionMatrix[3][3]);
    float c2 = (ppcam1.x*projectionMatrix[0][0] + ppcam1.y*projectionMatrix[0][1] + ppcam1.z*projectionMatrix[0][2] + projectionMatrix[0][3])*l2;
    float CorrectScale = 1.0f/(c2 - c1);
    return CorrectScale / float(vpwidth);
}

static float3 ComputePoint(float x, float y, const float4x4 &inverseView, const float3 &trans)
{
    return float3(trans.x + x * inverseView[0][0] + y * inverseView[0][1],
                  trans.y + x * inverseView[1][0] + y * inverseView[1][1],
                  trans.z + x * inverseView[2][0] + y * inverseView[2][1]);
}

void MiniGUIContext::Draw3DLineWidth(const float3 &p0, const uint32 color1, const float3 &p1, const uint32 color2, float lineWidth, bool widthScale /* = true */)
{
    // OPTIMIZE ME!
    // http://www.flipcode.com/archives/Textured_Lines_In_D3D.shtml
    const float4x4 &viewMatrix = g_pRenderer->GetGPUContext()->GetConstants()->GetCameraViewMatrix();
    const float4x4 &projectionMatrix = g_pRenderer->GetGPUContext()->GetConstants()->GetCameraProjectionMatrix();
    float4x4 inverseViewMatrix(viewMatrix.Transpose());

    // transform the delta by the view matrix rotation component
    float3 deltaDiff(p1 - p0);
    float3 delta(deltaDiff.Dot(viewMatrix.GetRow(0).xyz()),
                 deltaDiff.Dot(viewMatrix.GetRow(1).xyz()),
                 deltaDiff.Dot(viewMatrix.GetRow(2).xyz()));

    // calculate the sizes at each end of the line
    float sizep0 = lineWidth;
    float sizep1 = lineWidth;
    if (widthScale)
    {
        sizep0 *= ComputeConstantScale(p0, viewMatrix, projectionMatrix, m_viewportWidth);
        sizep1 *= ComputeConstantScale(p1, viewMatrix, projectionMatrix, m_viewportWidth);
    }

    // compute quad vertices
    float theta0 = Math::ArcTan(-delta.x, -delta.y);
    float c0 = sizep0 * Math::Cos(theta0);
    float s0 = sizep0 * Math::Sin(theta0);
    float3 q_v0(ComputePoint(c0, -s0, inverseViewMatrix, p0));
    float3 q_v1(ComputePoint(-c0, s0, inverseViewMatrix, p0));

    float theta1 = Math::ArcTan(delta.x, delta.y);
    float c1 = sizep1 * Math::Cos(theta1);
    float s1 = sizep1 * Math::Sin(theta1);
    float3 q_v2(ComputePoint(-c1, s1, inverseViewMatrix, p1));
    float3 q_v3(ComputePoint(c1, -s1, inverseViewMatrix, p1));

    SetBatchType(BATCH_TYPE_3D_COLORED_TRIANGLES);

    OverlayShader::Vertex3D vertices[6];
    vertices[0].SetColor(q_v0.x, q_v0.y, q_v0.z, color1);
    vertices[1].SetColor(q_v1.x, q_v1.y, q_v1.z, color1);
    vertices[2].SetColor(q_v2.x, q_v2.y, q_v2.z, color2);
    vertices[3].SetColor(q_v2.x, q_v2.y, q_v2.z, color2);
    vertices[4].SetColor(q_v1.x, q_v1.y, q_v1.z, color1);
    vertices[5].SetColor(q_v3.x, q_v3.y, q_v3.z, color2);
    AddVertices(vertices, countof(vertices));

    if (!m_manualFlushCount)
        Flush();
}

void MiniGUIContext::Draw3DRect(const float3 &p0, const float3 &p1, const uint32 color)
{
    float3 pmin(p0.Min(p1));
    float3 pmax(p0.Max(p1));

    // go ccw
    Draw3DRect(float3(pmin.x, pmax.y, pmin.z),
               float3(pmin.x, pmin.y, pmax.z),
               float3(pmax.x, pmin.y, pmax.z),
               float3(pmax.x, pmax.y, pmin.z),
               color);
}

void MiniGUIContext::Draw3DRect(const float3 &p0, const float3 &p1, const float3 &p2, const float3 &p3, const uint32 color)
{
    SetBatchType(BATCH_TYPE_3D_COLORED_LINES);

    OverlayShader::Vertex3D Vertices[8];
    Vertices[0].Set(p0, float2::Zero, color);
    Vertices[1].Set(p1, float2::Zero, color);
    Vertices[2].Set(p1, float2::Zero, color);
    Vertices[3].Set(p2, float2::Zero, color);
    Vertices[4].Set(p2, float2::Zero, color);
    Vertices[5].Set(p3, float2::Zero, color);
    Vertices[6].Set(p3, float2::Zero, color);
    Vertices[7].Set(p0, float2::Zero, color);
    AddVertices(Vertices, countof(Vertices));

    if (!m_manualFlushCount)
        Flush();
}

void MiniGUIContext::Draw3DRectWidth(const float3 &p0, const float3 &p1, const uint32 color, float lineWidth, bool widthScale /*= true*/)
{
    float3 pmin(p0.Min(p1));
    float3 pmax(p0.Max(p1));

    // go ccw
    Draw3DRectWidth(float3(pmin.x, pmax.y, pmin.z),
                    float3(pmin.x, pmin.y, pmax.z),
                    float3(pmax.x, pmin.y, pmax.z),
                    float3(pmax.x, pmax.y, pmin.z),
                    color, lineWidth, widthScale);
}

void MiniGUIContext::Draw3DRectWidth(const float3 &p0, const float3 &p1, const float3 &p2, const float3 &p3, const uint32 color, float lineWidth, bool widthScale /*= true*/)
{
    // force auto flush since we're going to call the same routine a bunch of times
    PushManualFlush();

    // invoke the line draws
    Draw3DLineWidth(p0, color, p1, color, lineWidth, widthScale);
    Draw3DLineWidth(p1, color, p2, color, lineWidth, widthScale);
    Draw3DLineWidth(p2, color, p3, color, lineWidth, widthScale);
    Draw3DLineWidth(p3, color, p0, color, lineWidth, widthScale);

    // restore flush state
    PopManualFlush();
}

void MiniGUIContext::Draw3DFilledRect(const float3 &p0, const float3 &p1, const uint32 color)
{
    float3 pmin(p0.Min(p1));
    float3 pmax(p0.Max(p1));

    // go ccw
    Draw3DFilledRect(float3(pmin.x, pmax.y, pmin.z),
                     float3(pmin.x, pmin.y, pmax.z),
                     float3(pmax.x, pmin.y, pmax.z),
                     float3(pmax.x, pmax.y, pmin.z),
                     color);
}

void MiniGUIContext::Draw3DFilledRect(const float3 &p0, const uint32 color0, const float3 &p1, const uint32 color1)
{
    float3 pmin(p0.Min(p1));
    float3 pmax(p0.Max(p1));

    // go ccw
    Draw3DFilledRect(float3(pmin.x, pmax.y, pmin.z), color1,
                     float3(pmin.x, pmin.y, pmax.z), color0,
                     float3(pmax.x, pmin.y, pmax.z), color0,
                     float3(pmax.x, pmax.y, pmin.z), color1);
}

void MiniGUIContext::Draw3DFilledRect(const float3 &p0, const float3 &p1, const float3 &p2, const float3 &p3, const uint32 color)
{
    Draw3DFilledRect(p0, color, p1, color, p2, color, p3, color);
}

void MiniGUIContext::Draw3DFilledRect(const float3 &p0, const uint32 color0, const float3 &p1, const uint32 color1, const float3 &p2, const uint32 color2, const float3 &p3, const uint32 color3)
{
    // check state
    SetBatchType(BATCH_TYPE_3D_COLORED_TRIANGLES);

    // generate vertices
    OverlayShader::Vertex3D Vertices[6];
    Vertices[0].Set(p0, float2::Zero, color0);
    Vertices[1].Set(p1, float2::Zero, color1);
    Vertices[2].Set(p3, float2::Zero, color2);
    Vertices[3].Set(p3, float2::Zero, color2);
    Vertices[4].Set(p1, float2::Zero, color1);
    Vertices[5].Set(p2, float2::Zero, color3);
    AddVertices(Vertices, countof(Vertices));

    // flush
    if (!m_manualFlushCount)
        Flush();
}

void MiniGUIContext::Draw3DGradientRect(const float3 &p0, const float3 &p1, const uint32 fromColor, const uint32 toColor, bool horizontal /* = false */)
{
    float3 pmin(p0.Min(p1));
    float3 pmax(p0.Max(p1));

    // go ccw
    Draw3DFilledRect(float3(pmin.x, pmax.y, pmin.z), horizontal ? fromColor : toColor,
                     float3(pmin.x, pmin.y, pmax.z), horizontal ? fromColor : fromColor,
                     float3(pmax.x, pmin.y, pmax.z), horizontal ? toColor : fromColor,
                     float3(pmax.x, pmax.y, pmin.z), horizontal ? toColor : toColor);
}

void MiniGUIContext::Draw3DTexturedRect(const float3 &p0, const float3 &p1, const MINIGUI_UV_RECT *pUVRect, const Texture2D *pTexture)
{
    float3 pmin(p0.Min(p1));
    float3 pmax(p0.Max(p1));

    // go ccw
    Draw3DTexturedRect(float3(pmin.x, pmax.y, pmin.z),
                       float3(pmin.x, pmin.y, pmax.z),
                       float3(pmax.x, pmin.y, pmax.z),
                       float3(pmax.x, pmax.y, pmin.z),
                       pUVRect, pTexture);
}

void MiniGUIContext::Draw3DTexturedRect(const float3 &p0, const float3 &p1, const MINIGUI_UV_RECT *pUVRect, GPUTexture *pDeviceTexture)
{
    float3 pmin(p0.Min(p1));
    float3 pmax(p0.Max(p1));

    // go ccw
    Draw3DTexturedRect(float3(pmin.x, pmax.y, pmin.z),
                       float3(pmin.x, pmin.y, pmax.z),
                       float3(pmax.x, pmin.y, pmax.z),
                       float3(pmax.x, pmax.y, pmin.z),
                       pUVRect, pDeviceTexture);
}

void MiniGUIContext::Draw3DTexturedRect(const float3 &p0, const float3 &p1, const float3 &p2, const float3 &p3, const MINIGUI_UV_RECT *pUVRect, const Texture2D *pTexture)
{
    GPUTexture2D *pGPUTexture = static_cast<GPUTexture2D *>(pTexture->GetGPUTexture());
    if (pGPUTexture != NULL)
        Draw3DTexturedRect(p0, p1, p2, p3, pUVRect, pGPUTexture);
}

void MiniGUIContext::Draw3DTexturedRect(const float3 &p0, const float3 &p1, const float3 &p2, const float3 &p3, const MINIGUI_UV_RECT *pUVRect, GPUTexture *pDeviceTexture)
{
    if (m_batchType != BATCH_TYPE_3D_TEXTURED_TRIANGLES ||
        m_pBatchTexture != pDeviceTexture)
    {
        Flush();
        m_batchType = BATCH_TYPE_3D_TEXTURED_TRIANGLES;
        m_pBatchTexture = pDeviceTexture;
        m_pBatchTexture->AddRef();
    }

    OverlayShader::Vertex3D vertices[6];
    vertices[0].SetUV(p0.x, p0.y, p0.z, pUVRect->left, pUVRect->top);               // topleft
    vertices[1].SetUV(p1.x, p1.y, p1.z, pUVRect->left, pUVRect->bottom);            // bottomleft
    vertices[2].SetUV(p3.x, p3.y, p3.z, pUVRect->right, pUVRect->top);              // topright
    vertices[3].SetUV(p3.x, p3.y, p3.z, pUVRect->right, pUVRect->top);              // topright
    vertices[4].SetUV(p1.x, p1.y, p1.z, pUVRect->left, pUVRect->bottom);            // bottomleft    
    vertices[5].SetUV(p2.x, p2.y, p2.z, pUVRect->right, pUVRect->bottom);           // bottomright
    AddVertices(vertices, countof(vertices));

    if (!m_manualFlushCount)
        Flush();
}

void MiniGUIContext::Draw3DEmissiveRect(const float3 &p0, const float3 &p1, const float3 &p2, const float3 &p3, const Material *pMaterial)
{

}

void MiniGUIContext::Draw3DGrid(const Plane &groundPlane, const float3 &origin, const float gridWidth, const float gridHeight, const float lineStep /* = 0.0f */, const uint32 lineColor /* = 0 */)
{
    SetBatchType(BATCH_TYPE_3D_COLORED_LINES);

    const float gridRangeX = gridWidth * 0.5f;
    const float gridRangeY = gridHeight * 0.5f;
    float3 vOrigin(origin);

    OverlayShader::Vertex3D vertices[2];
    uint32 drawRows;
    uint32 drawColumns;
    float x, y;
    uint32 i;

    if (lineStep > 0.0f)
    {
        drawRows = (uint32)Y_ceilf(gridRangeX * 2.0f / lineStep);
        drawColumns = (uint32)Y_ceilf(gridRangeY * 2.0f / lineStep);

        // draw columns
        for (i = 0, x = 0.0f; i <= drawColumns; i++, x += lineStep)
        {
            vertices[0].Set(vOrigin + float3(-gridRangeX + x, gridRangeY, 0.0f), float2::Zero, lineColor);
            vertices[1].Set(vOrigin + float3(-gridRangeX + x, -gridRangeY, 0.0f), float2::Zero, lineColor);
            AddVertices(vertices, countof(vertices));
        }

        // draw rows
        for (i = 0, y = 0.0f; i <= drawRows; i++, y += lineStep)
        {
            vertices[0].Set(vOrigin + float3(-gridRangeX, -gridRangeY + y, 0.0f), float2::Zero, lineColor);
            vertices[1].Set(vOrigin + float3(gridRangeX, -gridRangeY + y, 0.0f), float2::Zero, lineColor);
            AddVertices(vertices, countof(vertices));
        }
    }

    if (!m_manualFlushCount)
        Flush();
}

void MiniGUIContext::Draw3DGrid(const float3 &minCoordinates, const float3 &maxCoordinates, const float3 &gridStep, const uint32 lineColor)
{
    SetBatchType(BATCH_TYPE_3D_COLORED_LINES);

    OverlayShader::Vertex3D vertices[2];
    float x, y, z;
    uint32 i;

    float3 gridRange = float3(maxCoordinates) - float3(minCoordinates);
    uint32 drawLinesX = (gridStep.x > 0.0f) ? (uint32)Y_ceilf(gridRange.x / gridStep.x) : 0;
    uint32 drawLinesY = (gridStep.y > 0.0f) ? (uint32)Y_ceilf(gridRange.y / gridStep.y) : 0;
    uint32 drawLinesZ = (gridStep.z > 0.0f) ? (uint32)Y_ceilf(gridRange.z / gridStep.z) : 0;

    if (drawLinesX > 0)
    {
        for (i = 0, x = 0.0f; i <= drawLinesX; i++, x += gridStep.x)
        {
            vertices[0].Set(float3(minCoordinates.x + x, minCoordinates.y, minCoordinates.z), float2::Zero, lineColor);
            vertices[1].Set(float3(minCoordinates.x + x, maxCoordinates.y, maxCoordinates.z), float2::Zero, lineColor);
            AddVertices(vertices, countof(vertices));
        }
    }

    if (drawLinesY > 0)
    {
        for (i = 0, y = 0.0f; i <= drawLinesY; i++, y += gridStep.y)
        {
            vertices[0].Set(float3(minCoordinates.x, minCoordinates.y + y, minCoordinates.z), float2::Zero, lineColor);
            vertices[1].Set(float3(maxCoordinates.x, minCoordinates.y + y, maxCoordinates.z), float2::Zero, lineColor);
            AddVertices(vertices, countof(vertices));
        }
    }

    if (drawLinesZ > 0)
    {
        for (i = 0, z = 0.0f; i <= drawLinesZ; i++, z += gridStep.z)
        {
            vertices[0].Set(float3(minCoordinates.x, minCoordinates.y, minCoordinates.z + z), float2::Zero, lineColor);
            vertices[1].Set(float3(maxCoordinates.x, maxCoordinates.y, minCoordinates.z + z), float2::Zero, lineColor);
            AddVertices(vertices, countof(vertices));
        }
    }

    if (!m_manualFlushCount)
        Flush();
}

void MiniGUIContext::Draw3DWireBox(const float3 &minBounds, const float3 &maxBounds, const uint32 color)
{
    SetBatchType(BATCH_TYPE_3D_COLORED_LINES);

    OverlayShader::Vertex3D Vertices[24];
    OverlayShader::Vertex3D *pVertex = Vertices;
    (pVertex++)->SetColor(minBounds.x, minBounds.y, minBounds.z, color);
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, minBounds.z, color);
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, minBounds.z, color);
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, maxBounds.z, color);
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, maxBounds.z, color);
    (pVertex++)->SetColor(minBounds.x, minBounds.y, maxBounds.z, color);
    (pVertex++)->SetColor(minBounds.x, minBounds.y, maxBounds.z, color);
    (pVertex++)->SetColor(minBounds.x, minBounds.y, minBounds.z, color);
    (pVertex++)->SetColor(minBounds.x, minBounds.y, minBounds.z, color);
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, minBounds.z, color);
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, minBounds.z, color);
    (pVertex++)->SetColor(maxBounds.x, maxBounds.y, minBounds.z, color);
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, maxBounds.z, color);
    (pVertex++)->SetColor(maxBounds.x, maxBounds.y, maxBounds.z, color);
    (pVertex++)->SetColor(minBounds.x, minBounds.y, maxBounds.z, color);
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, maxBounds.z, color);
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, minBounds.z, color);
    (pVertex++)->SetColor(maxBounds.x, maxBounds.y, minBounds.z, color);
    (pVertex++)->SetColor(maxBounds.x, maxBounds.y, minBounds.z, color);
    (pVertex++)->SetColor(maxBounds.x, maxBounds.y, maxBounds.z, color);
    (pVertex++)->SetColor(maxBounds.x, maxBounds.y, maxBounds.z, color);
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, maxBounds.z, color);
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, maxBounds.z, color);
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, minBounds.z, color);
    AddVertices(Vertices, countof(Vertices));

    if (!m_manualFlushCount)
        Flush();
}

static void MakeSinCosTable(float *sinTable, float *cosTable, uint32 n, int32 a)
{
    DebugAssert(n > 0);

    float angle = 2.0f * Y_PI / (float)a;

    // first
    sinTable[0] = 0.0f;
    cosTable[0] = 1.0f;

    // entries
    for (uint32 i = 1; i < n; i++)
        Math::SinCos(angle * (float)i, &sinTable[i], &cosTable[i]);

    // last
    sinTable[n] = 0.0f;
    cosTable[n] = 1.0f;
}

void MiniGUIContext::Draw3DWireSphere(const float3 &sphereCenter, float radius, uint32 color, uint32 slices /* = 16 */, uint32 stacks /* = 16 */)
{
    // based on freeglut code
    DebugAssert(slices > 0 && stacks > 0);
    SetBatchType(BATCH_TYPE_3D_COLORED_LINES);

    // allocate storage for sin/cos table
    uint32 table1Size = slices + 1;
    uint32 table2Size = stacks * 2 + 1;
    float *sinTable1 = (float *)alloca(sizeof(float) * table1Size);
    float *cosTable1 = (float *)alloca(sizeof(float) * table1Size);
    float *sinTable2 = (float *)alloca(sizeof(float) * table2Size);
    float *cosTable2 = (float *)alloca(sizeof(float) * table2Size);

    // initialize sin/cos table for stacks
    MakeSinCosTable(sinTable1, cosTable1, stacks, -(int32)slices);
    MakeSinCosTable(sinTable2, cosTable2, stacks * 2, stacks * 2);

    // draw slices lines for each stack
    for (uint32 i = 1; i < stacks; i++)
    {
        float z = cosTable2[i] * radius;
        float r = sinTable2[i];

        // create first vertex
        OverlayShader::Vertex3D firstVertex;
        OverlayShader::Vertex3D lastVertex;
        firstVertex.Set(float3(r * radius, 0.0f, z) + sphereCenter, float2::Zero, color);
        Y_memcpy(&lastVertex, &firstVertex, sizeof(lastVertex));

        for (uint32 j = 1; j <= slices; j++)
        {
            float x = cosTable1[j];
            float y = sinTable1[j];

            // create vertex
            OverlayShader::Vertex3D vertex;
            vertex.Set(float3(x * r * radius, y * r * radius, z) + sphereCenter, float2::Zero, color);

            // create line
            m_batchVertices3D.Add(lastVertex);
            m_batchVertices3D.Add(vertex);

            // store as last
            Y_memcpy(&lastVertex, &vertex, sizeof(lastVertex));
        }

        // and from last to first
        m_batchVertices3D.Add(lastVertex);
        m_batchVertices3D.Add(firstVertex);
    }

    // draw stacks lines for each slice
    for (uint32 i = 0; i < slices; i++)
    {
        // create first vertex
        OverlayShader::Vertex3D lastVertex;
        lastVertex.Set(float3(0.0f, 0.0f, radius) + sphereCenter, float2::Zero, color);

        // for each stack
        for (uint32 j = 1; j <= stacks; j++)
        {
            float x = cosTable1[i] * sinTable2[j];
            float y = sinTable1[i] * sinTable2[j];
            float z = cosTable2[j];

            // create vertex
            OverlayShader::Vertex3D vertex;
            vertex.Set(float3(x, y, z) * radius + sphereCenter, float2::Zero, color);

            // create line
            m_batchVertices3D.Add(lastVertex);
            m_batchVertices3D.Add(vertex);

            // store as last
            Y_memcpy(&lastVertex, &vertex, sizeof(lastVertex));
        }
    }

    if (!m_manualFlushCount)
        Flush();
}

void MiniGUIContext::Draw3DBox(const float3 &minBounds, const float3 &maxBounds, const uint32 color)
{
    SetBatchType(BATCH_TYPE_3D_COLORED_TRIANGLES);

    OverlayShader::Vertex3D Vertices[36];
    OverlayShader::Vertex3D *pVertex = Vertices;

    // front face
    (pVertex++)->SetColor(minBounds.x, minBounds.y, minBounds.z, color);    // bottom-front-left
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, minBounds.z, color);    // bottom-front-right
    (pVertex++)->SetColor(minBounds.x, minBounds.y, maxBounds.z, color);    // top-front-left
    (pVertex++)->SetColor(minBounds.x, minBounds.y, maxBounds.z, color);    // top-front-left
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, minBounds.z, color);    // bottom-front-right
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, maxBounds.z, color);    // top-front-right

    // back face
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, minBounds.z, color);    // bottom-back-left
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, maxBounds.z, color);    // top-back-left
    (pVertex++)->SetColor(maxBounds.x, maxBounds.y, minBounds.z, color);    // bottom-back-right
    (pVertex++)->SetColor(maxBounds.x, maxBounds.y, minBounds.z, color);    // bottom-back-right
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, maxBounds.z, color);    // top-back-left
    (pVertex++)->SetColor(maxBounds.x, maxBounds.y, maxBounds.z, color);    // top-back-right

    // left face
    (pVertex++)->SetColor(minBounds.x, minBounds.y, minBounds.z, color);    // bottom-back-left
    (pVertex++)->SetColor(minBounds.x, minBounds.y, maxBounds.z, color);    // top-back-left
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, minBounds.z, color);    // bottom-front-left
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, minBounds.z, color);    // bottom-front-left
    (pVertex++)->SetColor(minBounds.x, minBounds.y, maxBounds.z, color);    // top-back-left
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, maxBounds.z, color);    // top-front-left

    // right face
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, minBounds.z, color);    // bottom-back-right
    (pVertex++)->SetColor(maxBounds.x, maxBounds.y, minBounds.z, color);    // bottom-front-right
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, maxBounds.z, color);    // top-back-right
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, maxBounds.z, color);    // top-back-right
    (pVertex++)->SetColor(maxBounds.x, maxBounds.y, minBounds.z, color);    // bottom-front-right
    (pVertex++)->SetColor(maxBounds.x, maxBounds.y, maxBounds.z, color);    // top-front-right

    // top face
    (pVertex++)->SetColor(minBounds.x, minBounds.y, maxBounds.z, color);    // top-front-left
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, maxBounds.z, color);    // top-front-right
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, maxBounds.z, color);    // top-back-left
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, maxBounds.z, color);    // top-back-left
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, maxBounds.z, color);    // top-front-right
    (pVertex++)->SetColor(maxBounds.x, maxBounds.y, maxBounds.z, color);    // top-back-right

    // bottom face
    (pVertex++)->SetColor(minBounds.x, minBounds.y, minBounds.z, color);    // bottom-front-left
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, minBounds.z, color);    // bottom-back-left
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, minBounds.z, color);    // bottom-front-right
    (pVertex++)->SetColor(maxBounds.x, minBounds.y, minBounds.z, color);    // bottom-front-right
    (pVertex++)->SetColor(minBounds.x, maxBounds.y, minBounds.z, color);    // bottom-back-left
    (pVertex++)->SetColor(maxBounds.x, maxBounds.y, minBounds.z, color);    // bottom-back-right
    AddVertices(Vertices, countof(Vertices));

    if (!m_manualFlushCount)
        Flush();
}

void MiniGUIContext::Draw3DSphere(const float3 &sphereCenter, float radius, uint32 color, uint32 slices /* = 16 */, uint32 stacks /* = 16 */)
{
    if (m_batchType != BATCH_TYPE_3D_COLORED_TRIANGLES)
    {
        Flush();
        m_batchType = BATCH_TYPE_3D_COLORED_TRIANGLES;
    }

    if (!m_manualFlushCount)
        Flush();
}

void MiniGUIContext::Draw3DArrow(const float3 &arrowStart, const float3 &arrowDirection, float lineLength, float lineThickness, float tipLength, float tipRadius, uint32 color, uint32 tipVertexCount /* = 24 */)
{
    // multiple draw calls, so save flush state and force manual flush
    PushManualFlush();

    // http://www.freemancw.com/2012/06/opengl-cone-function/#code

    // Draw line
    if (lineLength > 0.0f && lineThickness > 0.0f)
        Draw3DLineWidth(arrowStart, arrowStart + arrowDirection * lineLength, color, lineThickness);

    // Draw tip
    if (tipLength > 0.0f && tipRadius > 0.0f)
    {
        // ensure we're drawing triangles
        SetBatchType(BATCH_TYPE_3D_COLORED_TRIANGLES);

        // get axis
        float3 absDirection(arrowDirection.Abs());
        float3 e0;
        if (absDirection.x <= absDirection.y && absDirection.x <= absDirection.z)
            e0 = arrowDirection.Cross(float3::UnitX);
        else if (absDirection.y <= absDirection.x && absDirection.y <= absDirection.z)
            e0 = arrowDirection.Cross(float3::UnitY);
        else
            e0 = arrowDirection.Cross(float3::UnitZ);

        float3 e1(e0.Cross(arrowDirection));

        // determine vars
        uint32 arrowVertexStep = 360 / tipVertexCount;
        float3 tipStart(arrowStart + arrowDirection * lineLength);
        float3 tipEnd(tipStart + arrowDirection * tipLength);

        // determine vertices
        for (uint32 i = 0; i < tipVertexCount; i++)
        {
            float sinAngle, cosAngle;
            float sinNextAngle, cosNextAngle;
            Math::SinCos(Math::DegreesToRadians((float)(arrowVertexStep * i)), &sinAngle, &cosAngle);
            Math::SinCos(Math::DegreesToRadians((float)(arrowVertexStep * (i + 1))), &sinNextAngle, &cosNextAngle);

            // vertex
            float3 p0(tipStart + (((e0 * cosAngle) + (e1 * sinAngle)) * tipRadius));
            float3 p1(tipStart + (((e0 * cosNextAngle) + (e1 * sinNextAngle)) * tipRadius));

            // build vertices
            OverlayShader::Vertex3D vertices[6];

            // bottom part
            vertices[0].SetColor(tipStart.x, tipStart.y, tipStart.z, color);
            vertices[1].SetColor(p0.x, p0.y, p0.z, color);
            vertices[2].SetColor(p1.x, p1.y, p1.z, color);

            // top part
            vertices[3].SetColor(p0.x, p0.y, p0.z, color);
            vertices[4].SetColor(tipEnd.x, tipEnd.y, tipEnd.z, color);
            vertices[5].SetColor(p1.x, p1.y, p1.z, color);

            // add vertices
            AddVertices(vertices, countof(vertices));
        }
    }

    PopManualFlush();
}

void MiniGUIContext::DrawText(const Font *pFont, int32 size, uint32 color, const char *text)
{
    // work out font scale
    float textScale = (float)size / (float)pFont->GetHeight();

    // work out how many pixels we have to work with
    int32 widthAvailable = m_topRect.right - m_topRect.left - m_textCaretPositionX;
    int32 heightAvailable = m_topRect.bottom - m_topRect.top - m_textCaretPositionY;
    if (widthAvailable <= 0 || heightAvailable <= 0)
        return;

    // keep adding characters until we either hit the end of the line (word wrap), or finish the string
    int32 lineStart = 0;
    int32 lineLength = 0;
    int32 lineWidth = 0;
    int32 textLength = (int32)Y_strlen(text);
    for (int32 i = 0; i < textLength; i++)
    {
        int32 characterWidth = (int32)pFont->GetTextWidth(&text[i], 1, textScale);
        if ((lineWidth + characterWidth) > widthAvailable)
        {
            // draw this line
            if (lineLength > 0)
            {
                MINIGUI_RECT drawRect(m_topRect.left + m_textCaretPositionX, m_topRect.right, m_topRect.top + m_textCaretPositionY, m_topRect.bottom);
                InternalDrawText(pFont, textScale, color, &drawRect, text + lineStart, lineLength, false);
                lineStart += lineLength;
                lineLength = 0;
                lineWidth = 0;
            }

            // move to next line?
            if (m_textWordWrap)
            {
                m_textCaretPositionX = 0;
                m_textCaretPositionY += size;
                widthAvailable = m_topRect.right - m_topRect.left - m_textCaretPositionX;
                heightAvailable = m_topRect.bottom - m_topRect.top - m_textCaretPositionY;
                if (widthAvailable <= 0 || heightAvailable <= 0)
                    return;
            }
            else
            {
                // don't draw any more
                return;
            }
        }

        // add this character
        lineLength++;
        lineWidth += characterWidth;
    }

    // anything left-over?
    if (lineLength > 0)
    {
        MINIGUI_RECT drawRect(m_topRect.left + m_textCaretPositionX, m_topRect.right, m_topRect.top + m_textCaretPositionY, m_topRect.bottom);
        InternalDrawText(pFont, textScale, color, &drawRect, text + lineStart, lineLength, false);
    }
}

void MiniGUIContext::DrawFormattedText(const Font *pFont, int32 size, uint32 color, const char *format, ...)
{
    SmallString str;

    va_list ap;
    va_start(ap, format);
    str.FormatVA(format, ap);
    DrawText(pFont, size, color, str);
    va_end(ap);
}

void MiniGUIContext::DrawTextAt(int32 x, int32 y, const Font *pFont, int32 size, uint32 color, const char *text)
{
    // backup the old caret position
    int32 oldCaretPositionX = m_textCaretPositionX;
    int32 oldCaretPositionY = m_textCaretPositionY;

    // set new caret position to x/y
    m_textCaretPositionX = x;
    m_textCaretPositionY = y;

    // draw the text
    DrawText(pFont, size, color, text);

    // restore caret positio
    m_textCaretPositionX = oldCaretPositionX;
    m_textCaretPositionY = oldCaretPositionY;
}

void MiniGUIContext::DrawFormattedTextAt(int32 x, int32 y, const Font *pFont, int32 size, uint32 color, const char *format, ...)
{
    SmallString str;

    va_list ap;
    va_start(ap, format);
    str.FormatVA(format, ap);
    DrawTextAt(x, y, pFont, size, color, str);
    va_end(ap);
}
