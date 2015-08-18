#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorShaderGraphEditor.h"
#include "Engine/Entity.h"
#include "Engine/Font.h"
#include "Engine/ResourceManager.h"
#include "Renderer/Renderer.h"
#include "ResourceCompiler/ShaderGraph.h"
Log_SetChannel(EditorShaderGraphEditor);

static const uint32 s_iTextPaddingX = 4;
static const uint32 s_iTextPaddingY = 4;
static const uint32 s_iIOConnectorWidth = 8;

EditorShaderGraphEditor::EditorShaderGraphEditor(EditorShaderGraphEditorCallbacks *pCallbacks, ShaderGraph *pShaderGraph)
{
    m_pCallbacks = pCallbacks;
    m_pShaderGraph = pShaderGraph;
    m_eGraphWindowState = EDITOR_SHADER_GRAPH_EDITOR_STATE_NONE;
    m_LastMousePosition.SetZero();
    m_MouseSelectionRegion[0].SetZero();
    m_MouseSelectionRegion[1].SetZero();
    m_pCreateLinkTarget = NULL;
    m_iCreateLinkTargetInputIndex = 0xFFFFFFFF;
    m_pCreateLinkSource = NULL;
    m_iCreateLinkSourceOutputIndex = 0xFFFFFFFF;
    m_bRendererResourcesCreated = false;
    m_pRenderTarget = NULL;
    m_pRenderTargetView = NULL;
    m_pPickingTexture = NULL;
    m_pPickingTextureView = NULL;
    m_pFontData = g_pResourceManager->GetFont("resources/engine/medium_font");
    m_iTextHeight = 12;
    m_ViewTranslation.SetZero();

    // we manually flush when necessary
    m_mGUICtx.PushManualFlush();
}

EditorShaderGraphEditor::~EditorShaderGraphEditor()
{
    ReleaseRendererResources();

    ClearNodeSelection();
    ClearLinkSelection();

    SAFE_RELEASE(m_pFontData);
}

bool EditorShaderGraphEditor::CreateNode(const ShaderGraphNodeTypeInfo *pTypeInfo, const int2 &Position)
{
    if (pTypeInfo->GetFactory() == nullptr)
        return false;

    // using the factory, create the node
    Object *pNodeObject = pTypeInfo->GetFactory()->CreateObject();
    if (pNodeObject == nullptr)
        return false;

    // cast to node
    ShaderGraphNode *pNode = pNodeObject->Cast<ShaderGraphNode>();

    // calc node bounds
    uint32 nodeBoundsWidth, nodeBoundsHeight;
    CalculateNodeDimensions(pNode, &nodeBoundsWidth, &nodeBoundsHeight);

    // offset the position by half the bounds
    int2 offsetPosition = Position;
    offsetPosition.x -= nodeBoundsWidth / 2;
    offsetPosition.y -= nodeBoundsHeight / 2;

    // set node name, position
    SmallString nodeName;
    nodeName.Format("%s_%u", pTypeInfo->GetTypeName(), m_pShaderGraph->GetNextNodeId());
    pNode->SetName(nodeName);
    pNode->SetGraphEditorPosition(offsetPosition);

    if (!m_pShaderGraph->AddNode(pNode))
    {
        delete pNode;
        return false;
    }

    m_pCallbacks->OnNodeAdded();
    FlagForRedraw();
    return true;
}

bool EditorShaderGraphEditor::CreateLink(ShaderGraphNode *pTargetNode, uint32 InputIndex, const ShaderGraphNode *pSourceNode, uint32 OutputIndex)
{
    //uint32 i;

    if (InputIndex >= pTargetNode->GetInputCount() || OutputIndex >= pSourceNode->GetOutputCount())
        return false;

    const ShaderGraphNodeInput *pInput = pTargetNode->GetInput(InputIndex);
    const ShaderGraphNodeOutput *pOutput = pSourceNode->GetOutput(OutputIndex);

    // check input
    if (pInput->IsLinked())
    {
        Log_ErrorPrint("Could not create link: Input already linked.");
        return false;
    }

    // check the output type
    if (pOutput->GetValueType() == SHADER_PARAMETER_TYPE_COUNT)
    {
        Log_ErrorPrint("Could not create link: Output value type unknown.");
        return false;
    }

//     // find a compatible swizzle
//     SHADER_GRAPH_VALUE_SWIZZLE Swizzle = SHADER_GRAPH_VALUE_SWIZZLE_NONE;
//     if (pInput->GetValueType() != SHADER_GRAPH_VALUE_TYPE_UNKNOWN)
//     {
//         for (i = 0; i < SHADER_GRAPH_VALUE_SWIZZLE_COUNT; i++)
//         {
//             SHADER_GRAPH_VALUE_SWIZZLE curSwizzle = (SHADER_GRAPH_VALUE_SWIZZLE)i;
//             if (ShaderGraphNodeInput::GetTypeAfterSwizzle(pOutput->GetValueType(), curSwizzle))
//             {
//                 Swizzle = curSwizzle;
//                 break;
//             }
//         }
// 
//         if (i == SHADER_GRAPH_VALUE_SWIZZLE_COUNT)
//         {
//             Log_ErrorPrint("Could not create link: Could not find compatible swizzle.");
//             return false;
//         }
//     }

    return CreateLink(pTargetNode, InputIndex, pSourceNode, OutputIndex, SHADER_GRAPH_VALUE_SWIZZLE_NONE);
}

bool EditorShaderGraphEditor::CreateLink(ShaderGraphNode *pTargetNode, uint32 InputIndex, const ShaderGraphNode *pSourceNode, uint32 OutputIndex, SHADER_GRAPH_VALUE_SWIZZLE Swizzle)
{
    if (InputIndex >= pTargetNode->GetInputCount() || OutputIndex >= pSourceNode->GetOutputCount())
        return false;

    ShaderGraphNodeInput *pInput = pTargetNode->GetInput(InputIndex);
    const ShaderGraphNodeOutput *pOutput = pSourceNode->GetOutput(OutputIndex);

    // check input
    if (pInput->IsLinked())
    {
        Log_ErrorPrint("Could not create link: Input already linked.");
        return false;
    }

    // check the output type
    if (pOutput->GetValueType() == SHADER_PARAMETER_TYPE_COUNT)
    {
        Log_ErrorPrint("Could not create link: Output value type unknown.");
        return false;
    }

    // check input type
    if (pInput->GetValueType() != SHADER_PARAMETER_TYPE_COUNT && ShaderGraphNodeInput::GetTypeAfterSwizzle(pOutput->GetValueType(), Swizzle) != pInput->GetValueType())
    {
        Log_ErrorPrint("Could not create link: Swizzle type does not convert to valid input type.");
        return false;
    }

    // do link
    FlagForRedraw();
    return pInput->SetLink(pSourceNode, OutputIndex, Swizzle);
}

// void EditorShaderGraphEditor::SetRenderTarget(GPUTexture2D *pRenderTarget)
// {
//     m_pRenderTarget = pRenderTarget;
//     SAFE_RELEASE(m_pPickingTexture);
//     m_bRendererResourcesCreated = false;
// }

bool EditorShaderGraphEditor::CreateRendererResources()
{
    if (m_bRendererResourcesCreated)
        return true;

    // not going anywhere without a RT
    if (m_pRenderTarget == NULL)
        return false;

    // get RT desc
    const GPU_TEXTURE2D_DESC *pRenderTargetDesc = m_pRenderTarget->GetDesc();

    // picking texture
    if (m_pPickingTexture == NULL)
    {
        GPU_TEXTURE2D_DESC textureDesc;
        textureDesc.Width = pRenderTargetDesc->Width;
        textureDesc.Height = pRenderTargetDesc->Height;
        textureDesc.Format = PIXEL_FORMAT_R8G8B8A8_UNORM;
        textureDesc.Flags = GPU_TEXTURE_FLAG_BIND_RENDER_TARGET | GPU_TEXTURE_FLAG_READABLE;
        textureDesc.MipLevels = 1;
        if ((m_pPickingTexture = g_pRenderer->CreateTexture2D(&textureDesc, NULL, NULL, NULL)) == NULL)
            return false;

        m_PickingTextureCopy.Create(PIXEL_FORMAT_R8G8B8A8_UNORM, pRenderTargetDesc->Width, pRenderTargetDesc->Height, 1);
    }

    // update gui
    m_mGUICtx.SetViewportDimensions(pRenderTargetDesc->Width, pRenderTargetDesc->Height);
    m_bRendererResourcesCreated = true;
    m_bRedrawPending = true;
    return true;
}

void EditorShaderGraphEditor::ReleaseRendererResources()
{
    m_pRenderTarget = NULL;
    SAFE_RELEASE(m_pPickingTexture);
    m_bRendererResourcesCreated = false;
}

bool EditorShaderGraphEditor::IsNodeSelected(const ShaderGraphNode *pNode) const
{
    uint32 i;

    for (i = 0; i < m_SelectedGraphNodes.GetSize(); i++)
    {
        if (m_SelectedGraphNodes[i] == pNode)
            return true;
    }

    return false;
}

bool EditorShaderGraphEditor::IsLinkSelected(const ShaderGraphNodeInput *pNodeInput) const
{
    uint32 i;

    for (i = 0; i < m_SelectedGraphLinks.GetSize(); i++)
    {
        if (m_SelectedGraphLinks[i] == pNodeInput)
            return true;
    }

    return false;
}

void EditorShaderGraphEditor::ClearNodeSelection()
{
    m_SelectedGraphNodes.Clear();
}

void EditorShaderGraphEditor::ClearLinkSelection()
{
    m_SelectedGraphLinks.Clear();
}

Vector2i EditorShaderGraphEditor::GetNodeInputPosition(const ShaderGraphNode *pNode, uint32 InputIndex)
{
    uint32 connectorVerticalSpacing = (m_iTextHeight - (m_iTextHeight / 2));

    // calc node bounds
    uint32 nodeBoundsWidth, nodeBoundsHeight;
    CalculateNodeDimensions(pNode, &nodeBoundsWidth, &nodeBoundsHeight);

    // calc node position
    Vector2i nodePosition = m_ViewTranslation + pNode->GetGraphEditorPosition();
    nodePosition.x += (nodeBoundsWidth - (s_iIOConnectorWidth / 2));
    nodePosition.y += (m_iTextHeight + s_iTextPaddingY + s_iTextPaddingY + (m_iTextHeight - connectorVerticalSpacing) + ((m_iTextHeight + s_iTextPaddingY) * InputIndex));
    return nodePosition;
}

Vector2i EditorShaderGraphEditor::GetNodeOutputPosition(const ShaderGraphNode *pNode, uint32 OutputIndex)
{
    uint32 connectorVerticalSpacing = (m_iTextHeight - (m_iTextHeight / 2));

    Vector2i nodePosition = m_ViewTranslation + pNode->GetGraphEditorPosition();
    nodePosition.x += (s_iIOConnectorWidth / 2) - 1;
    nodePosition.y += (m_iTextHeight + s_iTextPaddingY + s_iTextPaddingY + (m_iTextHeight - connectorVerticalSpacing) + ((m_iTextHeight + s_iTextPaddingY) * OutputIndex));
    return nodePosition;
}

void EditorShaderGraphEditor::DrawNodeVisual(const ShaderGraphNode *pNode)
{
    uint32 i;
    MINIGUI_RECT rect;
    bool nodeIsSelected = IsNodeSelected(pNode);

    // colors
    const uint32 borderColor = nodeIsSelected ? MAKE_COLOR_R8G8B8A8_UNORM(204, 0, 0, 255) : MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 255);
    const uint32 headingBackgroundColor = MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 255);
    const uint32 headingTextColor = MAKE_COLOR_R8G8B8A8_UNORM(230, 230, 26, 255);
    const uint32 detailBackgroundColor = MAKE_COLOR_R8G8B8A8_UNORM(192, 192, 192, 255);
    const uint32 detailTextColor = MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 255);
    const uint32 outputConnectorFillColor = MAKE_COLOR_R8G8B8A8_UNORM(0, 102, 0, 255);
    const uint32 inputConnectorFillColor = MAKE_COLOR_R8G8B8A8_UNORM(102, 0, 0, 255);

    // get bounds
    uint32 nodeBoundsWidth, nodeBoundsHeight;
    CalculateNodeDimensions(pNode, &nodeBoundsWidth, &nodeBoundsHeight);

//     // halve it
//     uint32 nodeBoundsHalfWidth, nodeBoundsHalfHeight;
//     nodeBoundsHalfWidth = (uint32)Y_ceilf((float)nodeBoundsWidth * 0.5f);
//     nodeBoundsHalfHeight = (uint32)Y_ceilf((float)nodeBoundsHeight * 0.5f);
// 
//     // create the rect
//     const Vector2i &graphEditorPosition = pNode->GetGraphEditorPosition();
//     rect.left = graphEditorPosition.x - nodeBoundsHalfWidth;
//     rect.right = graphEditorPosition.x + nodeBoundsHalfWidth;
//     rect.top = graphEditorPosition.y - nodeBoundsHalfHeight;
//     rect.bottom = graphEditorPosition.y + nodeBoundsHalfHeight;

    // create the rect
    int2 graphEditorPosition = pNode->GetGraphEditorPosition() + m_ViewTranslation;
    rect.left = graphEditorPosition.x;
    rect.right = graphEditorPosition.x + nodeBoundsWidth;
    rect.top = graphEditorPosition.y;
    rect.bottom = graphEditorPosition.y + nodeBoundsHeight;

    // calculate connector sizes
    uint32 outputConnectorWidth = (pNode->GetOutputCount() > 0) ? s_iIOConnectorWidth : 0;
    uint32 inputConnectorWidth = (pNode->GetInputCount() > 0) ? s_iIOConnectorWidth : 0;
    uint32 connectorVerticalSpacing = (m_iTextHeight - (m_iTextHeight / 2)) / 2;

    // push it in the gui context
    m_mGUICtx.PushRect(&rect);

    // current Y position
    uint32 curY;

    // heading
    SET_MINIGUI_RECT(&rect, outputConnectorWidth, nodeBoundsWidth - inputConnectorWidth, 0, m_iTextHeight + s_iTextPaddingY);
    m_mGUICtx.DrawFilledRect(&rect, headingBackgroundColor);
    m_mGUICtx.DrawRect(&rect, borderColor);
    m_mGUICtx.DrawText(m_pFontData, m_iTextHeight, &rect, pNode->GetTypeInfo()->GetShortName(), headingTextColor, false, MINIGUI_HORIZONTAL_ALIGNMENT_CENTER, MINIGUI_VERTICAL_ALIGNMENT_CENTER);

    // draw background for text
    curY = m_iTextHeight + s_iTextPaddingY;
    SET_MINIGUI_RECT(&rect, outputConnectorWidth, nodeBoundsWidth - inputConnectorWidth, curY, nodeBoundsHeight);
    m_mGUICtx.DrawFilledRect(&rect, detailBackgroundColor);
    m_mGUICtx.DrawRect(&rect, borderColor);

    // spacing between heading and detail
    curY += s_iTextPaddingY;
    rect.top = curY;

    // outputs
    SET_MINIGUI_RECT(&rect, outputConnectorWidth + s_iTextPaddingX, nodeBoundsWidth - inputConnectorWidth - s_iTextPaddingX, curY, nodeBoundsHeight);
    for (i = 0; i < pNode->GetOutputCount(); i++)
    {
        m_mGUICtx.DrawText(m_pFontData, m_iTextHeight, &rect, pNode->GetOutput(i)->GetOutputDesc()->Name, detailTextColor, false, MINIGUI_HORIZONTAL_ALIGNMENT_LEFT, MINIGUI_VERTICAL_ALIGNMENT_TOP);
        rect.top += m_iTextHeight + s_iTextPaddingY;
    }

    // inputs
    SET_MINIGUI_RECT(&rect, outputConnectorWidth + s_iTextPaddingX, nodeBoundsWidth - inputConnectorWidth - s_iTextPaddingX, curY, nodeBoundsHeight);
    for (i = 0; i < pNode->GetInputCount(); i++)
    {
        m_mGUICtx.DrawText(m_pFontData, m_iTextHeight, &rect, pNode->GetInput(i)->GetInputDesc()->Name, detailTextColor, false, MINIGUI_HORIZONTAL_ALIGNMENT_RIGHT, MINIGUI_VERTICAL_ALIGNMENT_TOP);
        rect.top += m_iTextHeight + s_iTextPaddingY;
    }

    // output connectors
    SET_MINIGUI_RECT(&rect, 0, outputConnectorWidth - 1, curY + connectorVerticalSpacing, curY + (m_iTextHeight - connectorVerticalSpacing));
    for (i = 0; i < pNode->GetOutputCount(); i++)
    {
        m_mGUICtx.DrawFilledRect(&rect, outputConnectorFillColor);
        rect.top += m_iTextHeight + s_iTextPaddingY;
        rect.bottom += m_iTextHeight + s_iTextPaddingY;
    }

    // input connectors
    SET_MINIGUI_RECT(&rect, nodeBoundsWidth - inputConnectorWidth - 1, nodeBoundsWidth, curY + connectorVerticalSpacing, curY + (m_iTextHeight - connectorVerticalSpacing));
    for (i = 0; i < pNode->GetInputCount(); i++)
    {
        m_mGUICtx.DrawFilledRect(&rect, inputConnectorFillColor);
        rect.top += m_iTextHeight + s_iTextPaddingY;
        rect.bottom += m_iTextHeight + s_iTextPaddingY;
    }

    // pop rect
    m_mGUICtx.PopRect();
}

void EditorShaderGraphEditor::DrawNodeID(const ShaderGraphNode *pNode)
{
    uint32 i;
    MINIGUI_RECT rect;
    
    // for the id, we are just drawing the bounds of the node
    uint32 nodeBoundsWidth, nodeBoundsHeight;
    CalculateNodeDimensions(pNode, &nodeBoundsWidth, &nodeBoundsHeight);

//     // halve it
//     uint32 nodeBoundsHalfWidth, nodeBoundsHalfHeight;
//     nodeBoundsHalfWidth = (uint32)Y_ceilf((float)nodeBoundsWidth * 0.5f);
//     nodeBoundsHalfHeight = (uint32)Y_ceilf((float)nodeBoundsHeight * 0.5f);
// 
//     // create the rect
//     const Vector2i &graphEditorPosition = pNode->GetGraphEditorPosition();    
//     rect.left = graphEditorPosition.x - nodeBoundsHalfWidth;
//     rect.right = graphEditorPosition.x + nodeBoundsHalfWidth;
//     rect.top = graphEditorPosition.y - nodeBoundsHalfHeight;
//     rect.bottom = graphEditorPosition.y + nodeBoundsHalfHeight;

    // create the rect
    int2 graphEditorPosition = pNode->GetGraphEditorPosition() + m_ViewTranslation;
    SET_MINIGUI_RECT(&rect, graphEditorPosition.x, graphEditorPosition.x + nodeBoundsWidth, graphEditorPosition.y, graphEditorPosition.y + nodeBoundsHeight);

    // calculate connector sizes
    uint32 outputConnectorWidth = (pNode->GetOutputCount() > 0) ? s_iIOConnectorWidth : 0;
    uint32 inputConnectorWidth = (pNode->GetInputCount() > 0) ? s_iIOConnectorWidth : 0;
    uint32 connectorVerticalSpacing = (m_iTextHeight - (m_iTextHeight / 2)) / 2;

    // push it in the gui context
    m_mGUICtx.PushRect(&rect);

    // draw a filled rect over the detail part with the last byte set to zero
    uint32 encodedValue = ((pNode->GetID() & 0xFFFFFF) << 8);   
    SET_MINIGUI_RECT(&rect, outputConnectorWidth, nodeBoundsWidth - inputConnectorWidth, 0, nodeBoundsHeight);
    m_mGUICtx.DrawFilledRect(&rect, encodedValue);

    // determine detail start
    uint32 curY = m_iTextHeight + s_iTextPaddingY + s_iTextPaddingY;
    uint32 n = 1;

    // draw input connectors
    SET_MINIGUI_RECT(&rect, nodeBoundsWidth - inputConnectorWidth - 1, nodeBoundsWidth, curY + connectorVerticalSpacing, curY + (m_iTextHeight - connectorVerticalSpacing));
    for (i = 0; i < pNode->GetInputCount(); i++)
    {
        m_mGUICtx.DrawFilledRect(&rect, encodedValue | (n & 0x7F));
        rect.top += m_iTextHeight + s_iTextPaddingY;
        rect.bottom += m_iTextHeight + s_iTextPaddingY;
        n++;
    }

    // draw output connectors
    SET_MINIGUI_RECT(&rect, 0, outputConnectorWidth - 1, curY + connectorVerticalSpacing, curY + (m_iTextHeight - connectorVerticalSpacing));
    for (i = 0; i < pNode->GetOutputCount(); i++)
    {
        m_mGUICtx.DrawFilledRect(&rect, encodedValue | (n & 0x7F));
        rect.top += m_iTextHeight + s_iTextPaddingY;
        rect.bottom += m_iTextHeight + s_iTextPaddingY;
        n++;
    }

    // pop rect
    m_mGUICtx.PopRect();
    DebugAssert(n <= 0x7F);
}

void EditorShaderGraphEditor::DrawNodeVisualLink(const ShaderGraphNodeInput *pNodeInput)
{
    const ShaderGraphNode *pInputNode = pNodeInput->GetNode();
    uint32 inputIndex = pNodeInput->GetInputIndex();

    const ShaderGraphNode *pOutputNode = pNodeInput->GetSourceNode();
    uint32 outputIndex = pNodeInput->GetSourceOutputIndex();
    if (pOutputNode == NULL)
        return;

    const uint32 lineColor = IsLinkSelected(pNodeInput) ? MAKE_COLOR_R8G8B8A8_UNORM(204, 0, 0, 255) : MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 255);
    const Vector2i lineVertex1 = GetNodeOutputPosition(pOutputNode, outputIndex);
    const Vector2i lineVertex2 = GetNodeInputPosition(pInputNode, inputIndex);

    // draw line
    m_mGUICtx.DrawLine(lineVertex1, lineVertex2, lineColor);
}

void EditorShaderGraphEditor::DrawNodeIDLink(const ShaderGraphNodeInput *pNodeInput)
{
    const ShaderGraphNode *pInputNode = pNodeInput->GetNode();
    uint32 inputIndex = pNodeInput->GetInputIndex();

    const ShaderGraphNode *pOutputNode = pNodeInput->GetSourceNode();
    uint32 outputIndex = pNodeInput->GetSourceOutputIndex();
    if (pOutputNode == NULL)
        return;

    // get position
    const Vector2i lineVertex1 = GetNodeOutputPosition(pOutputNode, outputIndex);
    const Vector2i lineVertex2 = GetNodeInputPosition(pInputNode, inputIndex);

    // draw line
    m_mGUICtx.DrawLine(lineVertex1, lineVertex2, ((pInputNode->GetID() & 0xFFFFFF) << 8) | (inputIndex | 0x80));
}

void EditorShaderGraphEditor::CalculateNodeDimensions(const ShaderGraphNode *pNode, uint32 *pWidth, uint32 *pHeight)
{
    uint32 i;
    uint32 curWidth = 0;
    uint32 curHeight = 0;
    uint32 inputsWidth = 0;
    uint32 inputsHeight = 0;
    uint32 inputsConnectorWidth = 0;
    uint32 outputsWidth = 0;
    uint32 outputsHeight = 0;
    uint32 outputsConnectorWidth = 0;
    uint32 textWidth;
    float Scale;

    // calc scale
    Scale = (float)m_iTextHeight / (float)m_pFontData->GetHeight();

    // calc heading width
    textWidth = m_pFontData->GetTextWidth(pNode->GetTypeInfo()->GetShortName(), Y_strlen(pNode->GetTypeInfo()->GetShortName()), Scale);
    curWidth = textWidth + s_iTextPaddingX;
    curHeight = m_iTextHeight + s_iTextPaddingY;

    // spacing before input/output text
    curHeight += s_iTextPaddingY;

    // add outputs
    outputsWidth = s_iTextPaddingX;
    outputsConnectorWidth = (pNode->GetOutputCount() > 0) ? s_iIOConnectorWidth : 0;
    for (i = 0; i < pNode->GetOutputCount(); i++)
    {
        textWidth = m_pFontData->GetTextWidth(pNode->GetOutput(i)->GetOutputDesc()->Name, Y_strlen(pNode->GetOutput(i)->GetOutputDesc()->Name), Scale);
        outputsWidth = Max(outputsWidth, textWidth + s_iTextPaddingX);
        outputsHeight += m_iTextHeight + s_iTextPaddingY;
    }

    // add inputs
    inputsWidth = s_iTextPaddingX;
    inputsConnectorWidth = (pNode->GetInputCount() > 0) ? s_iIOConnectorWidth : 0;
    for (i = 0; i < pNode->GetInputCount(); i++)
    {
        textWidth = m_pFontData->GetTextWidth(pNode->GetInput(i)->GetInputDesc()->Name, Y_strlen(pNode->GetInput(i)->GetInputDesc()->Name), Scale);
        inputsWidth = Max(inputsWidth, textWidth + s_iTextPaddingX);
        inputsHeight += m_iTextHeight + s_iTextPaddingY;
    }

    // spacing after inputs/outputs
    curHeight += s_iTextPaddingY;

    // max them
    curWidth = Max(curWidth, outputsConnectorWidth + outputsWidth + inputsWidth + inputsConnectorWidth);
    curHeight += Max(inputsHeight, outputsHeight);

    // write to pointers
    *pWidth = curWidth;
    *pHeight = curHeight;
}

void EditorShaderGraphEditor::Draw(bool forceDrawVisual /* = false */, bool forceDrawPickingTexture /* = false */)
{
    uint32 i, j;
    const ShaderGraphNode *pNode;
    const ShaderGraphNodeInput *pNodeInput;
    uint32 nodeCount;

    if (!(forceDrawVisual | forceDrawPickingTexture | m_bRedrawPending))
        return;

    if (!m_bRendererResourcesCreated && !CreateRendererResources())
        return;

    GPUContext *pGPUDevice = g_pRenderer->GetMainContext();

    if ((forceDrawVisual | m_bRedrawPending))
    {
        // clear RT
        pGPUDevice->SetRenderTargets(1, &m_pRenderTargetView, nullptr);
        pGPUDevice->ClearTargets(true, false, false, float4(0.875f, 0.875f, 0.875f, 1.0f));
        pGPUDevice->SetDefaultViewport(m_pRenderTarget);

        // enable batching of gui draws
        m_mGUICtx.PushManualFlush();

        // draw nodes
        nodeCount = m_pShaderGraph->GetNodeCount();
        for (i = 0; i < nodeCount; i++)
        {
            pNode = m_pShaderGraph->GetNodeByArrayIndex(i);
            DrawNodeVisual(pNode);

            for (j = 0; j < pNode->GetInputCount(); j++)
            {
                pNodeInput = pNode->GetInput(j);
                if (pNodeInput->IsLinked())
                    DrawNodeVisualLink(pNodeInput);
            }
        }

        // draw the selection rect (if present)
        if (m_eGraphWindowState == EDITOR_SHADER_GRAPH_EDITOR_STATE_SELECTING)
        {
            MINIGUI_RECT selectionRect;
            SET_MINIGUI_RECT(&selectionRect, Min(m_MouseSelectionRegion[0].x, m_MouseSelectionRegion[1].x), Max(m_MouseSelectionRegion[0].x, m_MouseSelectionRegion[1].x), 
                                             Min(m_MouseSelectionRegion[0].y, m_MouseSelectionRegion[1].y), Max(m_MouseSelectionRegion[0].y, m_MouseSelectionRegion[1].y));

            m_mGUICtx.DrawRect(&selectionRect, MAKE_COLOR_R8G8B8A8_UNORM(51, 153, 255, 255));
        }
        // draw the creation-in-progress line
        else if (m_eGraphWindowState == EDITOR_SHADER_GRAPH_EDITOR_STATE_CREATING_LINK)
        {
            if (m_pCreateLinkSource != NULL)
                m_mGUICtx.DrawLine(GetNodeOutputPosition(m_pCreateLinkSource, m_iCreateLinkSourceOutputIndex), m_LastMousePosition, MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 255));
            else
                m_mGUICtx.DrawLine(m_LastMousePosition, GetNodeInputPosition(m_pCreateLinkTarget, m_iCreateLinkTargetInputIndex), MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 255));
        }

        // flush gui ctx
        m_mGUICtx.Flush();
        m_mGUICtx.PopManualFlush();
    }

    // update picking texture if requested
    if ((forceDrawPickingTexture | m_bRedrawPending)) 
    {
        pGPUDevice->SetRenderTargets(1, &m_pPickingTextureView, nullptr);
        pGPUDevice->ClearTargets(true, false, false);
        pGPUDevice->SetDefaultViewport(m_pPickingTexture);
        
        // enable batching of gui draws
        m_mGUICtx.PushManualFlush();
        m_mGUICtx.SetAlphaBlendingEnabled(false);

        // draw nodes
        nodeCount = m_pShaderGraph->GetNodeCount();
        for (i = 0; i < nodeCount; i++)
        {
            pNode = m_pShaderGraph->GetNodeByArrayIndex(i);
            DrawNodeID(pNode);
        }

        // draw node links
        for (i = 0; i < nodeCount; i++)
        {
            pNode = m_pShaderGraph->GetNodeByArrayIndex(i);
            for (j = 0; j < pNode->GetInputCount(); j++)
            {
                pNodeInput = pNode->GetInput(j);
                if (pNodeInput->IsLinked())
                    DrawNodeIDLink(pNodeInput);
            }
        }

        // clear RT
        m_mGUICtx.Flush();
        m_mGUICtx.PopManualFlush();
        m_mGUICtx.SetAlphaBlendingEnabled(true);

        // read back picking texture
        DebugAssert(m_PickingTextureCopy.IsValidImage());
        if (!pGPUDevice->ReadTexture(m_pPickingTexture, m_PickingTextureCopy.GetData(), m_PickingTextureCopy.GetDataRowPitch(), m_PickingTextureCopy.GetDataSize(), 0, 0, 0, m_PickingTextureCopy.GetWidth(), m_PickingTextureCopy.GetHeight()))
        {
            Log_WarningPrintf("Failed to read back picking texture.");
            Y_memzero(m_PickingTextureCopy.GetData(), m_PickingTextureCopy.GetDataSize());
        }
    }

    // no longer pending
    m_bRedrawPending = false;
}

bool EditorShaderGraphEditor::GetPickingTextureValues(const int2 &MinCoordinates, const int2 &MaxCoordinates, uint32 *pValues)
{
    DebugAssert((uint32)MinCoordinates.x < m_PickingTextureCopy.GetWidth() && (uint32)MinCoordinates.y < m_PickingTextureCopy.GetHeight());
    DebugAssert((uint32)MaxCoordinates.x < m_PickingTextureCopy.GetWidth() && (uint32)MaxCoordinates.y < m_PickingTextureCopy.GetHeight());

    // get number of values to read
    int2 coordinateRange = MaxCoordinates - MinCoordinates + 1;
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

uint32 EditorShaderGraphEditor::GetPickingTextureValue(const int2 &Position)
{
    uint32 texelValue;
    if (!GetPickingTextureValues(Position, Position, &texelValue))
        texelValue = 0;

    return texelValue;
}

void EditorShaderGraphEditor::ProcessGraphWindowMouseEvent(const QMouseEvent *pEvent)
{
    uint32 i, n;

    if (pEvent->type() == QEvent::MouseButtonPress || pEvent->type() == QEvent::MouseButtonRelease)
    {
        switch (pEvent->button())
        {
        case Qt::LeftButton:
            {
                if (pEvent->type() == QEvent::MouseButtonPress)
                {
                    switch (m_eGraphWindowState)
                    {
                    case EDITOR_SHADER_GRAPH_EDITOR_STATE_NONE:
                        {
                            uint32 pickingTextureValue = GetPickingTextureValue(m_LastMousePosition);
                            if (pickingTextureValue != 0)
                            {
                                // retrieve the node under the cursor
                                ShaderGraphNode *pCursorNode = m_pShaderGraph->GetNodeByID(pickingTextureValue >> 8);
                                if (pCursorNode == NULL)
                                    return;

                                if (pickingTextureValue & 0x80)
                                {
                                    // enter selection mode
                                    m_eGraphWindowState = EDITOR_SHADER_GRAPH_EDITOR_STATE_SELECTING;
                                    m_MouseSelectionRegion[0] = m_MouseSelectionRegion[1] = m_LastMousePosition;
                                    UpdateSelections();
                                }
                                else if (pickingTextureValue & 0x7F)
                                {
                                    // selecting an input/output
                                    ClearNodeSelection();
                                    ClearLinkSelection();

                                    uint32 searchIndex = (pickingTextureValue & 0x7F) - 1;
                                    for (i = 0, n = 0; i < pCursorNode->GetInputCount(); i++, n++)
                                    {
                                        if (n == searchIndex)
                                        {
                                            // mouse is over an input
                                            DebugAssert(m_pCreateLinkSource == NULL && m_pCreateLinkTarget == NULL);

                                            // change state
                                            m_eGraphWindowState = EDITOR_SHADER_GRAPH_EDITOR_STATE_CREATING_LINK;
                                            m_pCreateLinkTarget = pCursorNode;
                                            m_iCreateLinkTargetInputIndex = i;
                                            break;
                                        }
                                    }
                                    if (i == pCursorNode->GetInputCount())
                                    {
                                        for (i = 0; i < pCursorNode->GetOutputCount(); i++, n++)
                                        {
                                            if (n == searchIndex)
                                            {
                                                // mouse is over an output
                                                DebugAssert(m_pCreateLinkSource == NULL && m_pCreateLinkTarget == NULL);

                                                // change state
                                                m_eGraphWindowState = EDITOR_SHADER_GRAPH_EDITOR_STATE_CREATING_LINK;
                                                m_pCreateLinkSource = pCursorNode;
                                                m_iCreateLinkSourceOutputIndex = i;
                                                break;
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    // if it is part of the current selection, move the current selection, otherwise clear it and move the cursor node
                                    if (!IsNodeSelected(pCursorNode))
                                    {
                                        // enter selection mode
                                        m_MouseSelectionRegion[0] = m_MouseSelectionRegion[1] = m_LastMousePosition;
                                        UpdateSelections();
                                    }

                                    m_eGraphWindowState = EDITOR_SHADER_GRAPH_EDITOR_STATE_MOVING_NODE;
                                }

                                FlagForRedraw();
                            }
                            else
                            {
                                // enter selection mode
                                m_eGraphWindowState = EDITOR_SHADER_GRAPH_EDITOR_STATE_SELECTING;
                                m_MouseSelectionRegion[0] = m_MouseSelectionRegion[1] = m_LastMousePosition;
                                UpdateSelections();
                            }
                        }
                        break;
                    }
                }
                else if (pEvent->type() == QEvent::MouseButtonRelease)
                {
                    switch (m_eGraphWindowState)
                    {
                    case EDITOR_SHADER_GRAPH_EDITOR_STATE_SELECTING:
                        {
                            m_eGraphWindowState = EDITOR_SHADER_GRAPH_EDITOR_STATE_NONE;
                            m_MouseSelectionRegion[0].SetZero();
                            m_MouseSelectionRegion[1].SetZero();
                            FlagForRedraw();
                        }
                        break;

                    case EDITOR_SHADER_GRAPH_EDITOR_STATE_SCROLLING:
                        {
                            m_eGraphWindowState = EDITOR_SHADER_GRAPH_EDITOR_STATE_NONE;
                        }
                        break;

                    case EDITOR_SHADER_GRAPH_EDITOR_STATE_MOVING_NODE:
                        {
                            m_eGraphWindowState = EDITOR_SHADER_GRAPH_EDITOR_STATE_NONE;
                        }
                        break;

                    case EDITOR_SHADER_GRAPH_EDITOR_STATE_CREATING_LINK:
                        {
                            m_eGraphWindowState = EDITOR_SHADER_GRAPH_EDITOR_STATE_NONE;

                            uint32 pickingTextureValue = GetPickingTextureValue(m_LastMousePosition);
                            if (pickingTextureValue != 0)
                            {
                                // retrieve the node under the cursor
                                ShaderGraphNode *pCursorNode = m_pShaderGraph->GetNodeByID(pickingTextureValue >> 8);
                                if (pCursorNode != NULL)
                                {
                                    uint32 ioIndex = 0xFFFFFFFF;
                                    bool isInput = false;

                                    uint32 searchIndex = (pickingTextureValue & 0x7F) - 1;
                                    for (i = 0, n = 0; i < pCursorNode->GetInputCount(); i++, n++)
                                    {
                                        if (n == searchIndex)
                                        {
                                            ioIndex = i;
                                            isInput = true;
                                            break;
                                        }
                                    }
                                    if (i == pCursorNode->GetInputCount())
                                    {
                                        for (i = 0; i < pCursorNode->GetOutputCount(); i++, n++)
                                        {
                                            if (n == searchIndex)
                                            {
                                                ioIndex = i;
                                                isInput = false;
                                                break;
                                            }
                                        }
                                    }

                                    // already have a target?
                                    if (m_pCreateLinkTarget != NULL)
                                    {
                                        // node must be source
                                        DebugAssert(m_pCreateLinkSource == NULL);
                                        if (!isInput)
                                        {
                                            DebugAssert(ioIndex < pCursorNode->GetOutputCount());
                                            m_pCreateLinkSource = pCursorNode;
                                            m_iCreateLinkSourceOutputIndex = ioIndex;
                                        }
                                    }
                                    else
                                    {
                                        // node must be target
                                        if (isInput)
                                        {
                                            DebugAssert(ioIndex < pCursorNode->GetInputCount());
                                            m_pCreateLinkTarget = pCursorNode;
                                            m_iCreateLinkTargetInputIndex = ioIndex;
                                        }
                                    }
                                }
                            }

                            if (m_pCreateLinkSource != NULL && m_pCreateLinkTarget != NULL)
                            {
                                CreateLink(m_pCreateLinkTarget, m_iCreateLinkTargetInputIndex, m_pCreateLinkSource, m_iCreateLinkSourceOutputIndex);
                            }

                            m_pCreateLinkSource = m_pCreateLinkTarget = NULL;
                            m_iCreateLinkSourceOutputIndex = m_iCreateLinkTargetInputIndex = 0xFFFFFFFF;
                            FlagForRedraw();
                        }
                        break;
                    }
                }
            }
            break;

        case Qt::MiddleButton:
            {
                if (pEvent->type() == QEvent::MouseButtonRelease)
                {
                    uint32 pickingTextureValue = GetPickingTextureValue(m_LastMousePosition);
                    Log_DevPrintf("Picking texture value: %08X %02X %02X %02X %02X", pickingTextureValue, pickingTextureValue >> 24, (pickingTextureValue >> 16) & 0xFF, (pickingTextureValue >> 8) & 0xFF, pickingTextureValue & 0xFF);
                }
            }
            break;

        case Qt::RightButton:
            {
                if (pEvent->type() == QEvent::MouseButtonPress)
                {
                    if (m_eGraphWindowState == EDITOR_SHADER_GRAPH_EDITOR_STATE_NONE)
                    {
                        m_eGraphWindowState = EDITOR_SHADER_GRAPH_EDITOR_STATE_SCROLLING;
                    }
                }
                else if (pEvent->type() == QEvent::MouseButtonRelease)
                {
                    if (m_eGraphWindowState == EDITOR_SHADER_GRAPH_EDITOR_STATE_SCROLLING)
                    {
                        m_eGraphWindowState = EDITOR_SHADER_GRAPH_EDITOR_STATE_NONE;
                    }
                }
            }
            break;
        }
    }
    else if (pEvent->type() == QEvent::MouseMove)
    {
        Vector2i currentPosition(pEvent->x(), pEvent->y());

        // handle mouse movement based on graph state
        switch (m_eGraphWindowState)
        {
        case EDITOR_SHADER_GRAPH_EDITOR_STATE_NONE:
            break;

        case EDITOR_SHADER_GRAPH_EDITOR_STATE_SELECTING:
            {
                m_MouseSelectionRegion[1] = m_LastMousePosition;
                UpdateSelections();
                FlagForRedraw();
            }
            break;

        case EDITOR_SHADER_GRAPH_EDITOR_STATE_SCROLLING:
            {
                // calculate delta
                Vector2i deltaPosition(currentPosition - m_LastMousePosition);

                // translate graph window
                m_ViewTranslation += deltaPosition;
                FlagForRedraw();
            }
            break;

        case EDITOR_SHADER_GRAPH_EDITOR_STATE_MOVING_NODE:
            {
                // calculate delta
                Vector2i deltaPosition(currentPosition - m_LastMousePosition);

                // translate nodes
                for (i = 0; i < m_SelectedGraphNodes.GetSize(); i++)
                    m_SelectedGraphNodes[i]->SetGraphEditorPosition(m_SelectedGraphNodes[i]->GetGraphEditorPosition() + deltaPosition);

                // redraw window
                FlagForRedraw();
            }
            break;

        case EDITOR_SHADER_GRAPH_EDITOR_STATE_CREATING_LINK:
            {
                // queue redraw
                FlagForRedraw();
            }
            break;
        }

        // update position
        m_LastMousePosition = currentPosition;
    }
}

void EditorShaderGraphEditor::UpdateSelections()
{
    uint32 i;

    // determine min/max coordinates
    int2 minCoordinates = m_MouseSelectionRegion[0].Min(m_MouseSelectionRegion[1]);
    int2 maxCoordinates = m_MouseSelectionRegion[0].Max(m_MouseSelectionRegion[1]);
    //Log_DevPrintf("New selection: [%d-%d] [%d-%d]", m_MouseSelectionRegion[0].x, m_MouseSelectionRegion[1].x, m_MouseSelectionRegion[0].y, m_MouseSelectionRegion[1].y);

    // allocate memory for receiving entity ids
    uint32 nTexelValues = (maxCoordinates.x - minCoordinates.x + 1) * (maxCoordinates.y - minCoordinates.y + 1);
    uint32 *pTexelValues = new uint32[nTexelValues];
    uint32 *pCurrentTexelValue = pTexelValues;

    // read texel values
    GetPickingTextureValues(minCoordinates, maxCoordinates, pTexelValues);

    // clear selections
    ClearNodeSelection();
    ClearLinkSelection();

    // iterate each pixel that is selected, determining which render target was hit on the screen
    int2 currentPosition = minCoordinates;
    for (; currentPosition.y <= maxCoordinates.y; currentPosition.y++)
    {
        for (currentPosition.x = minCoordinates.x; currentPosition.x <= maxCoordinates.x; currentPosition.x++)
        {
            uint32 texelValue = *pCurrentTexelValue++;
            if (texelValue == 0)
                continue;

            ShaderGraphNode *pNode = m_pShaderGraph->GetNodeByID(texelValue >> 8);
            if (pNode == NULL)
                continue;

            if (texelValue & 0x80)
            {
                uint32 inputLinkIndex = (texelValue & 0x7F);
                ShaderGraphNodeInput *pNodeInput = pNode->GetInput(inputLinkIndex);
                DebugAssert(pNodeInput->IsLinked());

                for (i = 0; i < m_SelectedGraphLinks.GetSize(); i++)
                {
                    if (m_SelectedGraphLinks[i] == pNodeInput)
                        break;
                }

                if (i == m_SelectedGraphLinks.GetSize())
                {
                    Log_DevPrintf("add link node %u %u (%s)", pNode->GetID(), inputLinkIndex, pNode->GetName().GetCharArray());
                    m_SelectedGraphLinks.Add(pNodeInput);
                }
            }
            else
            {
                for (i = 0; i < m_SelectedGraphNodes.GetSize(); i++)
                {
                    if (m_SelectedGraphNodes[i] == pNode)
                        break;
                }

                if (i == m_SelectedGraphNodes.GetSize())
                {
                    Log_DevPrintf("add selection node %u (%s)", pNode->GetID(), pNode->GetName().GetCharArray());
                    m_SelectedGraphNodes.Add(pNode);
                }
            }
        }
    }

    // leak, doh
    delete[] pTexelValues;

    // update selections
    m_pCallbacks->OnNodeSelectionChange();
    FlagForRedraw();
}

