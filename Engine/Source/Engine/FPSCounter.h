#pragma once
#include "Engine/Common.h"

class Font;
class MiniGUIContext;
class GPUContext;

class FPSCounter
{
public:
    static const uint32 KEEP_FRAME_TIME_COUNT = 200;

public:
    FPSCounter();
    ~FPSCounter();
    
    // overall average fps
    const float GetFPS() const { return m_fps; }

    // frame times are behind one frame
    const float GetFrameTime() const { return m_lastFrameTime; }
    
    // call when starting the frame
    void BeginFrame();

    // call when the game thread finishes and is waiting for the render thread
    void EndGameThreadFrame();

    // call when the render thread finishes and is waiting for the game thread
    void EndRenderThreadFrame();

    // draw a debug message of the current fps to the screen
    void DrawDetails(const Font *pFont, MiniGUIContext *pGUIContext, int32 startX = -400, int32 startY = 0, uint32 fontSize = 16) const;

    // resources
    GPUContext *GetGPUContext() { return m_pGPUContext; }
    void SetGPUContext(GPUContext *pGPUContext) { m_pGPUContext = pGPUContext; }
    bool CreateGPUResources();
    void ReleaseGPUResources();

    // access last frame time
    void GetFrameTimeHistory(float *pFrameTimes, uint32 frameCount) const;

private:
    GPUContext *m_pGPUContext;

    Timer m_lastFrameStartTimer;
    Timer m_gameThreadTimer;
    Timer m_renderThreadTimer;

    float m_accumulatedTime;
    uint32 m_framesRendered;
    float m_fps;
    float m_lastFrameTime;
    float m_lastGameFrameTime;
    float m_lastRenderFrameTime;

    float m_lastFrameTimes[KEEP_FRAME_TIME_COUNT];
    uint32 m_lastFrameTimesIndex;

    uint32 m_lastDrawCallCount;

    size_t m_lastMemoryUsage;
    size_t m_lastScriptMemoryUsage;
};
