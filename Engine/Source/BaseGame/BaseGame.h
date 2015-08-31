#pragma once
#include "BaseGame/Common.h"
#include "BaseGame/GameState.h"
#include "Renderer/Renderer.h"
#include "Renderer/WorldRenderer.h"
#include "Renderer/RenderProfiler.h"
#include "Renderer/MiniGUIContext.h"
#include "Engine/FPSCounter.h"
#include "Engine/OverlayConsole.h"
#include "Engine/SDLHeaders.h"

class BaseGame
{
public:
    BaseGame();
    virtual ~BaseGame();

    //=================================================================================================================================================================================================
    // Startup/shutdown
    //=================================================================================================================================================================================================
    bool IsQuitPending() const { return m_quitFlag; }
    void CriticalError(const char *format, ...);
    int32 MainEntryPoint();
    void Quit();

    //=================================================================================================================================================================================================
    // GameState functions
    //=================================================================================================================================================================================================
    
    // Access the current game state
    GameState *GetGameState() const { return m_pGameState; }

    // Queue a game state for execution, deleting the current game beforehand.
    void SetNextGameState(GameState *pGameState);

    // Start a modal game state, saving the current state beforehand, and returning after EndModalGameState.
    void BeginModalGameState(GameState *pGameState);

    // End a modal game state, restoring the previous state.
    void EndModalGameState();

    //=================================================================================================================================================================================================
    // Render functions, all these pointers are owned by the render thread
    //=================================================================================================================================================================================================
    GPUContext *GetGPUContext() { return m_pGPUContext; }
    RendererOutputWindow *GetOutputWindow() { return m_pOutputWindow; }
    WorldRenderer *GetWorldRenderer() { return m_pWorldRenderer; }
    RenderProfiler *GetRenderProfiler() { return m_pRenderProfiler; }
    MiniGUIContext *GetGUIContext() { return &m_guiContext; }
    void QueueRendererRestart() { m_restartRendererFlag = true; }
    void SetRelativeMouseMovement(bool enabled);
    void PushForcedAbsoluteMouseMovement();
    void PopForcedAbsoluteMouseMovement();

protected:
    //=================================================================================================================================================================================================
    // Events
    //=================================================================================================================================================================================================
    virtual void OnRegisterTypes();
    virtual bool OnStart();
    virtual void OnWindowResized(uint32 width, uint32 height);
    virtual void OnWindowFocusGained();
    virtual void OnWindowFocusLost();
    virtual void OnMainThreadPreFrame(float deltaTime);
    virtual void OnMainThreadBeginFrame(float deltaTime);
    virtual void OnMainThreadAsyncTick(float deltaTime);
    virtual void OnMainThreadTick(float deltaTime);
    virtual void OnMainThreadEndFrame(float deltaTime);
    virtual void OnRenderThreadBeginFrame(float deltaTime);
    virtual void OnRenderThreadDraw(float deltaTime);
    virtual void OnRenderThreadEndFrame(float deltaTime);
    virtual void OnExit();

protected:
    //=================================================================================================================================================================================================
    // Utility/subsystem-independent
    //=================================================================================================================================================================================================
    FPSCounter m_fpsCounter;
    OverlayConsole *m_pOverlayConsole;

    // flags
    bool m_quitFlag;
    bool m_restartRendererFlag;
    bool m_relativeMouseMovement;
    uint32 m_forcedAbsoluteMouseMovement;

    //=================================================================================================================================================================================================
    // Renderer Subsystem
    //=================================================================================================================================================================================================
    GPUContext *m_pGPUContext;
    RendererOutputWindow *m_pOutputWindow;
    WorldRenderer *m_pWorldRenderer;
    RenderProfiler *m_pRenderProfiler;
    MiniGUIContext m_guiContext;
    Event m_renderThreadEventsReadyEvent;
    Event m_renderThreadFrameCompleteEvent;

    //=================================================================================================================================================================================================
    // GameState
    //=================================================================================================================================================================================================
    GameState *m_pGameState;
    GameState *m_pNextGameState;
    bool m_nextGameStateIsModal;
    bool m_gameStateEndModal;

private:
    //=================================================================================================================================================================================================
    // Commands
    //=================================================================================================================================================================================================
    void RegisterBaseCommands();

    // command entry points
    static bool Command_Quit_ExecuteHandler(void *userData, uint32 argumentCount, const char *const argumentValues[]);
    static bool Command_Quit_HelpHandler(void *userData, uint32 argumentCount, const char *const argumentValues[]);
    static bool Command_GC_ExecuteHandler(void *userData, uint32 argumentCount, const char *const argumentValues[]);
    static bool Command_GC_HelpHandler(void *userData, uint32 argumentCount, const char *const argumentValues[]);
    static bool Command_OpenDebugRenderWindow_ExecuteHandler(void *userData, uint32 argumentCount, const char *const argumentValues[]);
    static bool Command_OpenDebugRenderWindow_HelpHandler(void *userData, uint32 argumentCount, const char *const argumentValues[]);
    static bool Command_Profiler_ExecuteHandler(void *userData, uint32 argumentCount, const char *const argumentValues[]);
    static bool Command_Profiler_HelpHandler(void *userData, uint32 argumentCount, const char *const argumentValues[]);
    static bool Command_ProfilerDisplay_ExecuteHandler(void *userData, uint32 argumentCount, const char *const argumentValues[]);
    static bool Command_ProfilerDisplay_HelpHandler(void *userData, uint32 argumentCount, const char *const argumentValues[]);

    //=================================================================================================================================================================================================
    // Input subsystem
    //=================================================================================================================================================================================================
    void RegisterBaseInputEvents();
    void BindBaseInputEvents();

    // input events
    void InputActionHandler_PreviousDebugCamera();
    void InputActionHandler_NextDebugCamera();

    // input variables
    MemArray<SDL_Event> m_pendingEvents;

    // game state stack
    PODArray<GameState *> m_gameStateStack;

    // time between frames
    Timer m_frameTimer;

    //=================================================================================================================================================================================================
    // Tasks
    //=================================================================================================================================================================================================
    // main thread tasks
    void MainThreadGameLoop(bool isModal);
    void MainThreadGameLoopIteration();
    void MainThreadProcessInputEvents(float deltaTime);
    void MainThreadFrame(float deltaTime);

    // render thread tasks
    bool RendererStart();
    void RenderThreadRestartRenderer();
    void RenderThreadCollectEvents(float deltaTime);
    void RenderThreadFrame(float deltaTime);
    void RenderThreadDrawOverlays(float deltaTime);
    void RendererShutdown();

#ifdef Y_PLATFORM_HTML5
    // frame trampoline callback for html5
    static void HTML5FrameTrampoline(void *pParam);
#endif

#ifdef WITH_IMGUI
    //=================================================================================================================================================================================================
    // ImGui Stuff
    //=================================================================================================================================================================================================
public:
    bool IsImGuiActivated() const { return (m_imGuiEnabled != 0); }
    void OpenDebugRenderMenu();
    void ActivateImGui();
    void DeactivateImGui();

private:
    // imgui overlay drawer
    void RenderThreadDrawImGuiOverlays();

    // imgui vars
    uint32 m_imGuiEnabled;
    bool m_showDebugRenderMenu;
#else
    // stubs when compiling without imgui
    bool IsImGuiActivated() const { return false; }
    void OpenDebugRenderMenu() {}
    void ActivateImGui() {}
    void DeactivateImGui() {}
#endif

#ifdef WITH_PROFILER
    //=================================================================================================================================================================================================
    // Profiler Stuff
    //=================================================================================================================================================================================================
public:
    void SetProfilerDisplayMode(uint32 mode);
    void ToggleProfilerDisplay();
    void HideProfilerDisplay();
    
private:
    bool m_profilerInterceptMouseEvents;
#else
    // stubs when compiling without profiler
    void SetProfilerDisplayMode(uint32 mode) {}
    void ToggleProfilerDisplay() {}
    void HideProfilerDisplay() {}
#endif

public:
    //=================================================================================================================================================================================================
    // Helpers
    //=================================================================================================================================================================================================
};
