#pragma once
#include "Editor/Common.h"
#include "Renderer/MiniGUIContext.h"
#include "ResourceCompiler/ShaderGraph.h"
#include "Core/Image.h"

class Font;

struct EditorShaderGraphEditorCallbacks;

enum EDITOR_SHADER_GRAPH_EDITOR_STATE
{
    EDITOR_SHADER_GRAPH_EDITOR_STATE_NONE,
    EDITOR_SHADER_GRAPH_EDITOR_STATE_SELECTING,
    EDITOR_SHADER_GRAPH_EDITOR_STATE_SCROLLING,
    EDITOR_SHADER_GRAPH_EDITOR_STATE_MOVING_NODE,
    EDITOR_SHADER_GRAPH_EDITOR_STATE_CREATING_LINK,
    EDITOR_SHADER_GRAPH_EDITOR_STATE_COUNT,
};

class EditorShaderGraphEditor
{
public:
    EditorShaderGraphEditor(EditorShaderGraphEditorCallbacks *pCallbacks, ShaderGraph *pShaderGraph);
    ~EditorShaderGraphEditor();

    // accessors
    const ShaderGraph *GetShaderGraph() const { return m_pShaderGraph; }
    const int2 &GetViewTranslation() const { return m_ViewTranslation; }

    bool CreateNode(const ShaderGraphNodeTypeInfo *pTypeInfo, const int2 &Position);
    bool CreateLink(ShaderGraphNode *pTargetNode, uint32 InputIndex, const ShaderGraphNode *pSourceNode, uint32 OutputIndex);
    bool CreateLink(ShaderGraphNode *pTargetNode, uint32 InputIndex, const ShaderGraphNode *pSourceNode, uint32 OutputIndex, SHADER_GRAPH_VALUE_SWIZZLE Swizzle);

    // renderer resources
    const bool PendingRedraw() const { return m_bRedrawPending; }
    void FlagForRedraw() { m_bRedrawPending = true; }
    //void SetRenderTarget(GPUTexture2D *pRenderTarget);
    bool CreateRendererResources();
    void ReleaseRendererResources();

    // selections
    const ShaderGraphNode *GetNodeSelection(uint32 i) const { return m_SelectedGraphNodes[i]; }
    const ShaderGraphNodeInput *GetLinkSelection(uint32 i) const { return m_SelectedGraphLinks[i]; }
    ShaderGraphNode *GetNodeSelection(uint32 i) { return m_SelectedGraphNodes[i]; }
    ShaderGraphNodeInput *GetLinkSelection(uint32 i) { return m_SelectedGraphLinks[i]; }
    uint32 GetNodeSelectionCount() const { return m_SelectedGraphNodes.GetSize(); }
    uint32 GetLinkSelectionCount() const { return m_SelectedGraphLinks.GetSize(); }
    bool IsNodeSelected(const ShaderGraphNode *pNode) const;
    bool IsLinkSelected(const ShaderGraphNodeInput *pNodeInput) const;
    void ClearNodeSelection();
    void ClearLinkSelection();

    // drawing methods
    void Draw(bool forceDrawVisual = false, bool forceDrawPickingTexture = false);

    // input handlers
    void ProcessGraphWindowMouseEvent(const QMouseEvent *pEvent);

private:
    EditorShaderGraphEditorCallbacks *m_pCallbacks;
    ShaderGraph *m_pShaderGraph;

    // graph window state
    EDITOR_SHADER_GRAPH_EDITOR_STATE m_eGraphWindowState;
    int2 m_LastMousePosition;
    int2 m_MouseSelectionRegion[2];

    // selection
    PODArray<ShaderGraphNode *> m_SelectedGraphNodes;
    PODArray<ShaderGraphNodeInput *> m_SelectedGraphLinks;
    ShaderGraphNode *m_pCreateLinkTarget;
    uint32 m_iCreateLinkTargetInputIndex;
    ShaderGraphNode *m_pCreateLinkSource;
    uint32 m_iCreateLinkSourceOutputIndex;

    // hardware resources
    bool m_bRendererResourcesCreated;
    GPUTexture2D *m_pRenderTarget;
    GPURenderTargetView *m_pRenderTargetView;
    GPUTexture2D *m_pPickingTexture;
    GPURenderTargetView *m_pPickingTextureView;
    Image m_PickingTextureCopy;
    bool m_bRedrawPending;

    // renderer resources not in hardware
    const Font *m_pFontData;
    MiniGUIContext m_mGUICtx;
    uint32 m_iTextHeight;
    int2 m_ViewTranslation;

    // drawing subcalls
    Vector2i GetNodeInputPosition(const ShaderGraphNode *pNode, uint32 InputIndex);
    Vector2i GetNodeOutputPosition(const ShaderGraphNode *pNode, uint32 OutputIndex);
    void DrawNodeVisual(const ShaderGraphNode *pNode);
    void DrawNodeVisualLink(const ShaderGraphNodeInput *pNodeInput);
    void DrawNodeID(const ShaderGraphNode *pNode);
    void DrawNodeIDLink(const ShaderGraphNodeInput *pNodeInput);
    void CalculateNodeDimensions(const ShaderGraphNode *pNode, uint32 *pWidth, uint32 *pHeight);

    // updates the picking texture
    bool GetPickingTextureValues(const int2 &MinCoordinates, const int2 &MaxCoordinates, uint32 *pValues);
    uint32 GetPickingTextureValue(const int2 &Position);
    void UpdateSelections();
};

struct EditorShaderGraphEditorCallbacks
{
    virtual void OnNodeAdded() { }
    virtual void OnNodeRemoved() { }
    virtual void OnNodeSelectionChange() { }
};

