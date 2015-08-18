#pragma once
#include "Renderer/Common.h"
#include "Renderer/RendererTypes.h"
#include "Engine/Camera.h"

class Font;
class MiniGUIContext;

// helper macros
#define RENDER_PROFILER_BEGIN_SECTION(profilerVariable, sectionName, enableGPUStats) MULTI_STATEMENT_MACRO_BEGIN \
                                                                                        if (profilerVariable != nullptr) { \
                                                                                            profilerVariable->BeginSection(sectionName, enableGPUStats); \
                                                                                        } \
                                                                                     MULTI_STATEMENT_MACRO_END

#define RENDER_PROFILER_END_SECTION(profilerVariable) MULTI_STATEMENT_MACRO_BEGIN \
                                                                        if (profilerVariable != nullptr) { \
                                                                            profilerVariable->EndSection(); \
                                                                        } \
                                                                   MULTI_STATEMENT_MACRO_END

#define RENDER_PROFILER_ADD_CAMERA(profilerVariable, cameraPtr, cameraName) MULTI_STATEMENT_MACRO_BEGIN \
                                                                        if (profilerVariable != nullptr) { \
                                                                            profilerVariable->AddCamera(cameraPtr, cameraName); \
                                                                                        } \
                                                                   MULTI_STATEMENT_MACRO_END

                               

class RenderProfiler
{
public:
    class Section
    {
        friend class RenderProfiler;

    public:
        Section();
        ~Section();

        const String &GetSectionName() const { return m_sectionName; }

        const float GetElapsedTime() const { return m_elapsedTime; }
        const bool HasGPUStats() const { return m_hasGPUStats; }
        const float GetCPUTime() const { return (m_elapsedTime - m_gpuWaitTime); }
        const float GetGPUTime() const { return m_gpuTime; }
        const float GetGPUWaitTime() const { return m_gpuWaitTime; }
        const uint32 GetDrawCallCount() const { return m_drawCallCount; }

        const Section *GetParent() const { return m_pParent; }
        const Section *GetChild(uint32 i) const { return m_children[i]; }
        const uint32 GetChildCount() const { return m_children.GetSize(); }

    private:
        void Clear();
        void Begin(Section *pParentSection, const char *sectionName, bool enableGPUStats, GPUContext *pGPUContext, GPUQuery *pGPUTimeQuery);
        void End(GPUContext *pGPUContext);

        String m_sectionName;
        float m_elapsedTime;
        bool m_hasGPUStats;
        float m_gpuTime;
        float m_gpuWaitTime;
        uint32 m_drawCallCount;
        uint32 m_startingDrawCallCount;
        Timer m_timer;
        GPUQuery *m_pGPUTimeQuery;

        Section *m_pParent;
        PODArray<Section *> m_children;
    };

    struct DrawSummaryOptions
    {
        float RedThresholdMs;
        float YellowThresholdMs;
        const Font *pFont;
        uint32 FontSize;
        uint32 DrawAreaStartX;
        uint32 DrawAreaStartY;
        uint32 DrawAreaWidth;
        uint32 DrawAreaHeight;
        uint32 DrawAreaBackgroundColor;
        uint32 DrawAreaBorderColor;
    };

public:
    RenderProfiler(GPUContext *pGPUContext);
    ~RenderProfiler();

    // enable/disable frame timing
    bool GetGPUStatsEnabled() const { return m_gpuTimingEnabled; }
    void SetGPUStatsEnabled(bool enabled) { m_gpuTimingEnabled = enabled; }

    // begin section
    void BeginSection(const char *sectionName, bool enableGPUStats);
    void EndSection();

    // begin camera
    const uint32 GetCameraCount() const { return m_cameraArray.GetSize(); }
    const Camera *GetCamera(uint32 index) { return &m_cameraArray[index].Value; }
    const String &GetCameraName(uint32 index) { return m_cameraArray[index].Key; }
    void AddCamera(const Camera *pCamera, const char *cameraName);

    // camera override, used by renderers
    const bool HasCameraOverride() const { return (m_cameraOverrideIndex >= 0 && (uint32)m_cameraOverrideIndex < m_cameraArray.GetSize()); }
    const Camera *GetCameraOverrideCamera() const { return (m_cameraOverrideIndex >= 0 && (uint32)m_cameraOverrideIndex < m_cameraArray.GetSize()) ? &m_cameraArray[m_cameraOverrideIndex].Value : nullptr; }
    const char *GetCameraOverrideName() const { return (m_cameraOverrideIndex >= 0 && (uint32)m_cameraOverrideIndex < m_cameraArray.GetSize()) ? m_cameraArray[m_cameraOverrideIndex].Key.GetCharArray() : nullptr; }
    const int32 GetCameraOverrideIndex() const { return m_cameraOverrideIndex; }
    void SetCameraOverride(uint32 cameraOverrideIndex) { m_cameraOverrideIndex = (int32)cameraOverrideIndex; }
    void ClearCameraOverride() { m_cameraOverrideIndex = -1; }
   
    // call when starting/ending the frame
    void BeginFrame();
    void EndFrame();

    // gpu resources
    void SetGPUContext(GPUContext *pGPUContext) { m_pGPUContext = pGPUContext; }
    bool CreateRendererResources();
    void ReleaseRendererResources();

    // draw
    void DrawPreviousFrameSummary(MiniGUIContext *pGUIContext, const DrawSummaryOptions *pDrawOptions);
    void DrawThisFrameSummary(MiniGUIContext *pGUIContext, const DrawSummaryOptions *pDrawOptions);

private:
    // get a new section object
    Section *GetNewSection();

    // get a gpu time query object
    GPUQuery *GetNewGPUTimeQuery();

    // recursive cleanup of section
    void CleanupSectionAndChildren(Section *pSection);

    // draw a frame summary recursively
    void DrawFrameSummary(const Section *pRootSection, MiniGUIContext *pGUIContext, const DrawSummaryOptions *pDrawOptions);
    uint32 DrawFrameSummaryRecursive(const Section *pCurrentSection, MiniGUIContext *pGUIContext, const DrawSummaryOptions *pDrawOptions);

    // vars
    GPUContext *m_pGPUContext;
    bool m_gpuTimingEnabled;

    // resources
    bool m_rendererResourcesCreated;

    // cache of gpu time query objects
    PODArray<GPUQuery *> m_gpuTimeQueryCache;
    PODArray<GPUQuery *> m_freeGPUTimeQueries;

    // cache of section objects
    PODArray<Section *> m_sectionCache;
    PODArray<Section *> m_freeSections;

    // root section for this frame
    Section *m_pRootSection;

    // current section for this frame
    Section *m_pCurrentSection;

    // root section for previous frame
    Section *m_pPreviousFrameRootSection;

    // camera list, only accessible until the end of the frame
    Array< KeyValuePair<String, Camera> > m_cameraArray;
    int32 m_cameraOverrideIndex;
};

