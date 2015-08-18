#pragma once
#include "Engine/TerrainRenderer.h"
#include "Engine/TerrainQuadTree.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/RendererTypes.h"
#include "Renderer/VertexBufferBindingArray.h"
#include "Renderer/VertexFactory.h"

class ShaderProgram;
class TerrainRendererCDLOD;
class TerrainSectionRenderProxyCDLOD;

class TerrainRendererCDLOD_VertexFactory : public VertexFactory
{
    DECLARE_VERTEX_FACTORY_TYPE_INFO(TerrainRendererCDLOD_VertexFactory, VertexFactory);

public:
    static bool IsValidPermutation(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags);
    static bool FillShaderCompilerParameters(uint32 globalShaderFlags, uint32 baseShaderFlags, uint32 vertexFactoryFlags, ShaderCompilerParameters *pParameters);

    static uint32 GetVertexElementsDesc(RENDERER_PLATFORM platform, RENDERER_FEATURE_LEVEL featureLevel, uint32 flags, GPU_VERTEX_ELEMENT_DESC pElementsDesc[GPU_INPUT_LAYOUT_MAX_ELEMENTS]);

    // setup for drawing
    //static void SetShaderUniforms(GPUShaderProgram *pShaderProgram, const TerrainManager *pTerrainManager, const TerrainSection *pSection, int32 sectionX, int32 sectionY, uint32 nodeOffset[2], uint32 nodeSize);
};

class TerrainRendererCDLOD : public TerrainRenderer
{
public:
    TerrainRendererCDLOD(const TerrainParameters *pParameters, const TerrainLayerList *pLayerList);
    virtual ~TerrainRendererCDLOD();

    // create a render proxy for a section
    virtual TerrainSectionRenderProxy *CreateSectionRenderProxy(uint32 entityId, const TerrainSection *pSection) override;

    // gpu resources
    virtual bool CreateGPUResources() override;
    virtual void ReleaseGPUResources() override;

    // readers for renderproxy
    bool BindGridBuffers(GPUContext *pGPUDevice) const;
    uint32 GetGridSize() const { return m_gridSize; }
    uint32 GetGridIndexCount() const { return m_indexCount; }
    uint32 GetGridIndexCountPerSubQuad() const { return m_indexCountPerSubQuad; }
    uint32 GetGridIndexCountSingleQuad() const { return m_indexCountSingleQuad; }
    uint32 GetGridIndexEndTL() const { return m_indexEndTL; }
    uint32 GetGridIndexEndTR() const { return m_indexEndTR; }
    uint32 GetGridIndexEndBL() const { return m_indexEndBL; }
    uint32 GetGridIndexEndBR() const { return m_indexEndBR; }

protected:
    bool CreateGridBuffers();
    void OnSettingsChangedCallback();

    // callback to update settings
    Functor *m_pSettingsUpdateCallback;
    volatile bool m_settingsUpdatedFlag;

    // gpu resources
    uint32 m_gridSize;
    GPUBuffer *m_pVertexBuffer;
    GPUBuffer *m_pIndexBuffer;
    GPU_INDEX_FORMAT m_indexFormat;
    uint32 m_indexCount;
    uint32 m_indexCountPerSubQuad;
    uint32 m_indexCountSingleQuad;
    uint32 m_indexEndTL;
    uint32 m_indexEndTR;
    uint32 m_indexEndBL;
    uint32 m_indexEndBR;
    bool m_GPUResourcesCreated;
};

class TerrainSectionRenderProxyCDLOD : public TerrainSectionRenderProxy
{
    friend class TerrainRendererCDLOD;

public:
    TerrainSectionRenderProxyCDLOD(uint32 entityID, const TerrainRendererCDLOD *pRenderer, const TerrainSection *pSection);
    ~TerrainSectionRenderProxyCDLOD();

    // create textures
    bool CreateGPUResources();

    // virtual update methods for TerrainSectionRenderProxy
    virtual void OnLayersModified() override;
    virtual void OnPointHeightModified(uint32 x, uint32 y) override;
    virtual void OnPointLayersModified(uint32 x, uint32 y) override;

    // resources accessors
    GPUTexture2D *GetHeightMapTexture() const { return m_pHeightMapTexture; }
    GPUTexture2D *GetNormalMapTexture() const { return m_pNormalMapTexture; }
    GPUTexture *GetAlphaMapTexture() const { return m_pAlphaMapTexture; }
    const Material *GetRenderMaterial() const { return m_pRenderMaterial; }

    // RenderProxy methods
    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override;
    virtual void SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const override;
    virtual void DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUContext *pGPUContext) const override;
    virtual void DrawDebugInfo(const Camera *pCamera, GPUContext *pGPUContext, MiniGUIContext *pGUIContext) const override;

private:
    // texture creators
    bool CreateHeightMapTexture();
    bool CreateNormalMapTexture();
    bool CreateCombinedHeightNormalMapTexture();
    bool CreateAlphaMapTexture();
    bool CreateRenderMaterial();
    bool CreateDetailInstanceBuffer();

    // helpers
    void CalculateMorphConstants(float cameraNearDistance, float cameraFarDistance) const;
    void SetupTerrainDraw(const TerrainQuadTreeNode *pNode, GPUContext *pDevice, ShaderProgram *pShaderProgram) const;
    void InvokeTerrainDraw(GPUContext *pDevice, uint32 drawFlags) const;
    bool BuildDetailBatches(GPUContext *pGPUContext, const float3 &eyePosition) const;
    void SetupDetailBatchDraw(uint32 detailBatchIndex, uint32 meshLODIndex, uint32 meshBatchIndex, GPUContext *pGPUContext, ShaderProgram *pShaderProgram) const;
    void InvokeDetailBatchDraw(uint32 detailBatchIndex, uint32 meshLODIndex, uint32 meshBatchIndex, GPUContext *pGPUContext) const;

    // vars
    const TerrainRendererCDLOD *m_pRenderer;

    GPUTexture2D *m_pHeightMapTexture;
    GPUTexture2D *m_pNormalMapTexture;
    GPUTexture *m_pAlphaMapTexture;
    const Material *m_pRenderMaterial;
    bool m_GPUResourcesCreated;
    //bool m_layersChanged;
    //bool m_heightChanged;
    //bool m_weightChanged;

    // detail buffer
    GPUBuffer *m_pDetailInstanceBuffer;
    struct DetailBatch
    {
        uint32 MeshIndex;
        uint32 InstanceBufferOffset;
        uint32 InstanceCount;
    };
    mutable MemArray<DetailBatch> m_detailBatches;

    // guaranteed to be valid for the current context draw only
    // morphing constants
    mutable float m_visibilityRanges[TERRAIN_MAX_RENDER_LODS];
    mutable float m_morphStart[TERRAIN_MAX_RENDER_LODS];
    mutable float m_morphEnd[TERRAIN_MAX_RENDER_LODS];
    mutable float4 m_morphConstants[TERRAIN_MAX_RENDER_LODS];
};

