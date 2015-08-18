#pragma once
#include "Renderer/Common.h"
#include "Renderer/RendererTypes.h"
#include "Renderer/RenderQueue.h"
#include "Engine/Camera.h"

class Material;
class MaterialShader;

class RenderWorld;
class RenderProfiler;

class ShaderProgram;

class MiniGUIContext;

struct RENDER_QUEUE_RENDERABLE_ENTRY;
class ShaderComponentTypeInfo;
class VertexFactoryTypeInfo;

class WorldRenderer
{
    DeclareNonCopyable(WorldRenderer);

public:
    struct Options
    {
        Options();
        Options(const Options &options);
        void InitFromCVars();
        void SetRenderResolution(uint32 width, uint32 height);
        void DisableUnsupportedFeatures();

        uint32 EnableDepthPrepass : 1;
        uint32 EnableShadows : 1;
        uint32 EnablePointLightShadows : 1;
        uint32 EnableHardwareShadowFiltering : 1;
        uint32 EnableOcclusionCulling : 1;
        uint32 EnableOcclusionPredication : 1;
        uint32 WaitForOcclusionResults : 1;
        uint32 EnablePostProcessing : 1;
        uint32 EnableSSAO : 1;
        uint32 EnableBloom : 1;
        uint32 ShowDebugInfo : 1;
        uint32 ShowShadowMapCascades : 1;
        uint32 ShowIntermediateBuffers : 1;
        uint32 ShowWireframeOverlay : 1;
        uint32 RenderModeFullbright : 1;
        uint32 RenderModeNormals : 1;
        uint32 RenderModeLightingOnly : 1;
        uint32 EmulateMobile : 1;

        uint32 RenderWidth;
        uint32 RenderHeight;

        uint32 OcclusionCullingObjectsPerBatch;

        PIXEL_FORMAT ShadowMapPixelFormat;
        uint32 DirectionalShadowMapResolution;
        uint32 PointShadowMapResolution;
        uint32 SpotShadowMapResolution;
        RENDERER_SHADOW_FILTER ShadowMapFiltering;
        uint32 ShadowMapCascadeCount;
    };

    struct ViewParameters
    {
        ViewParameters();
        ViewParameters(const ViewParameters &parameters);
        void SetCamera(const Camera *pCamera);

        // to save indirection we copy the camera to here
        Camera ViewCamera;
        float MaximumShadowViewDistance;
        RENDERER_VIEWPORT Viewport;
        float WorldTime;
        RENDERER_FOG_MODE FogMode;
        float FogStartDistance;
        float FogEndDistance;
        float FogDensity;
        float3 FogColor;
        bool EnableManualExposure;
        float ManualExposure;
        float MaximumExposure;
        bool EnableBloom;
        float BloomThreshold;
        float BloomMagnitude;
    };

    struct RenderStats
    {
        uint32 ObjectCount;
        uint32 LightCount;
        uint32 ShadowMapCount;
        uint32 ObjectsCulledByOcclusion;
        uint32 IntermediateBufferCount;
        uint32 IntermediateBufferMemoryUsage;
    };

public:
    WorldRenderer(GPUContext *pGPUContext, const Options *pOptions);
    virtual ~WorldRenderer();

    // options
    const Options *GetOptions() const { return &m_options; }

    // context
    GPUContext *GetGPUContext() const { return m_pGPUContext; }
    void SetGPUContext(GPUContext *pGPUContext) { m_pGPUContext = pGPUContext; }

    // gui context
    MiniGUIContext *GetGUIContext() const { return m_pGUIContext; }
    void SetGUIContext(MiniGUIContext *pGUIContext) { m_pGUIContext = pGUIContext; }

    // create renderer resources
    virtual bool Initialize();

    // Draws the world.
    virtual void DrawWorld(const RenderWorld *pRenderWorld, const ViewParameters *pViewParameters, GPURenderTargetView *pRenderTargetView, GPUDepthStencilBufferView *pDepthStencilBufferView, RenderProfiler *pRenderProfiler);

    // Get render stats.
    virtual void GetRenderStats(RenderStats *pRenderStats) const;

    // On completion of frame.
    virtual void OnFrameComplete();

    // create a world renderer based on cvar state
    static WorldRenderer *Create(GPUContext *pGPUContext, const Options *pCreationParameters);
   
protected:
    // intermediate buffer
    struct IntermediateBuffer
    {
        GPUTexture2D *pTexture;
        GPURenderTargetView *pRTV;
        GPUDepthStencilBufferView *pDSV;
        uint32 Width;
        uint32 Height;
        PIXEL_FORMAT PixelFormat;
        uint32 MipLevels;
    };

    // occlusion culling. the render proxy is used as a key, never dereferenced.
    struct PendingOcclusionCullingQuery
    {
        const RenderProxy *pRenderProxy;
        bool MatchUserData;

        uint32 UserData[4];
        void *UserDataPointer[2];

        GPUQuery *pQuery;
        bool Visible;
    };

    // named intermediate buffers - used for debug drawing
    struct DebugBufferView
    {
        GPUTexture *pTexture;
        IntermediateBuffer *pBuffer;
        const char *Name;
    };

    // helper functions
    static ShaderProgram *GetShaderProgram(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const GPU_VERTEX_ELEMENT_DESC *pAttributes, uint32 nAttributes);
    static ShaderProgram *GetShaderProgram(uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialStaticSwitchMask);
    static ShaderProgram *GetShaderProgram(const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry);
    static ShaderProgram *GetShaderProgram(const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags);
    static void SetBlendingModeForMaterial(GPUContext *pGPUDevice, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry);
    static void SetAdditiveBlendingModeForMaterial(GPUContext *pGPUDevice, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry);

    // queue filling + sorting
    void FillRenderQueue(const Camera *pCamera, const RenderWorld *pRenderWorld);

    // debug drawing
    void DrawDebugInfo(const Camera *pCamera, RenderProfiler *pRenderProfiler);

    // wireframe overlay drawing
    void DrawWireframeOverlay(const Camera *pCamera, const RenderQueue::RenderableArray *pRenderables);

    // upscale/downsample one texture to another, using hardware bilinear filtering
    void ScaleTexture(GPUTexture2D *pSourceTexture, GPURenderTargetView *pDestinationRTV, bool restoreViewport = true, bool restoreTargets = true);

    // intermediate buffer requests/releases
    IntermediateBuffer *RequestIntermediateBuffer(uint32 width, uint32 height, PIXEL_FORMAT pixelFormat, uint32 mipLevels);
    IntermediateBuffer *RequestIntermediateBufferMatching(const IntermediateBuffer *pIntermediateBuffer);
    void ReleaseIntermediateBuffer(IntermediateBuffer *pIntermediateBuffer);

    // occlusion culling, draw method assumes depth buffer is bound
    void DrawOcclusionCullingProxies(const Camera *pCamera);
    void CollectOcclusionCullingResults();
    void BindOcclusionQueriesToQueueEntries();

    // queue a buffer for showing in debug mode
    void AddDebugBufferView(GPUTexture *pTexture, const char *name);
    void AddDebugBufferView(IntermediateBuffer *pBuffer, const char *name, bool autoRelease);

    // draw intermediate buffers
    void DrawIntermediateBuffers();

    // common parameters
    GPUContext *m_pGPUContext;
    MiniGUIContext *m_pGUIContext;
    Options m_options;
    uint32 m_globalShaderFlags;

    // render queue
    RenderQueue m_renderQueue;

    // intermediate buffer variables
    PODArray<IntermediateBuffer *> m_allIntermediateBuffers;
    PODArray<IntermediateBuffer *> m_freeIntermediateBuffers;
    MemArray<DebugBufferView> m_debugBufferViews;

    // occlusion culling variables
    PODArray<GPUQuery *> m_occlusionCullingQueryCacheArray;
    MemArray<PendingOcclusionCullingQuery> m_occlusionCullingPendingQueries;
    GPUBuffer *m_pOcclusionCullingCubeVertexBuffer;
    GPUBuffer *m_pOcclusionCullingCubeIndexBuffer;
    ShaderProgram *m_pOcclusionCullingProgram;
};
