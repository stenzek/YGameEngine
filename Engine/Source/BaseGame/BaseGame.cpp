#include "BaseGame/PrecompiledHeader.h"
#include "BaseGame/BaseGame.h"
#include "Engine/InputManager.h"
#include "Engine/EngineCVars.h"
#include "Engine/World.h"
#include "Engine/ScriptManager.h"
#include "Engine/Profiling.h"
#include "Renderer/WorldRenderer.h"
#include "Renderer/ImGuiBridge.h"
#include "YBaseLib/CPUID.h"
Log_SetChannel(BaseGame);

BaseGame::BaseGame()
    : m_pOverlayConsole(nullptr)
    , m_quitFlag(false)
    , m_restartRendererFlag(false)
    , m_relativeMouseMovement(false)
    , m_forcedAbsoluteMouseMovement(0)
    , m_pGPUContext(nullptr)
    , m_pWorldRenderer(nullptr)
    , m_pOutputWindow(nullptr)
    , m_pRenderProfiler(nullptr)
    , m_renderThreadEventsReadyEvent(true)
    , m_renderThreadFrameCompleteEvent(true)
    , m_pGameState(nullptr)
    , m_pNextGameState(nullptr)
    , m_nextGameStateIsModal(false)
    , m_gameStateEndModal(false)
#ifdef WITH_IMGUI
    , m_imGuiEnabled(0)
    , m_showDebugRenderMenu(false)
#endif
#ifdef WITH_PROFILER
    , m_profilerInterceptMouseEvents(false)
#endif
{

}

BaseGame::~BaseGame()
{
    DebugAssert(m_pNextGameState == nullptr);
    DebugAssert(m_pGameState == nullptr);
    DebugAssert(m_pRenderProfiler == nullptr);
    DebugAssert(m_pWorldRenderer == nullptr);
    DebugAssert(m_pOutputWindow == nullptr);
    DebugAssert(m_pGPUContext == nullptr);
}

void BaseGame::Quit()
{
    m_quitFlag = true;
}

void BaseGame::OnRegisterTypes()
{

}

bool BaseGame::OnStart()
{
    // enable resource modification tracking
    g_pResourceManager->SetResourceModificationDetectionEnabled(true);

    // create overlay console
    m_pOverlayConsole = new OverlayConsole();

    // register stuff
    RegisterBaseCommands();
    RegisterBaseInputEvents();
    BindBaseInputEvents();
    return true;
}

void BaseGame::OnExit()
{
    g_pInputManager->UnregisterEventsWithOwner(this);
    g_pConsole->UnregisterCommandsWithOwner(this);

    delete m_pOverlayConsole;
    m_pOverlayConsole = nullptr;
}

void BaseGame::OnWindowResized(uint32 width, uint32 height)
{
    Log_DevPrintf("Game window resized: %ux%u", width, height);

    // check if we lost exclusive fullscreen
    if (m_pOutputWindow->GetFullscreenState() == RENDERER_FULLSCREEN_STATE_FULLSCREEN && !m_pGPUContext->GetExclusiveFullScreen())
    {
        Log_WarningPrintf("BaseGame::OnWindowResized: Fullscreen state lost outside our control. Updating internal state.");
        m_pOutputWindow->SetFullscreenState(RENDERER_FULLSCREEN_STATE_WINDOWED);
        g_pConsole->SetCVar(&CVars::r_fullscreen, false);
        QueueRendererRestart();
    }

    // resize the output window and buffer
    m_pOutputWindow->SetDimensions(width, height);
    m_pGPUContext->ResizeOutputBuffer(width, height);

    // pass to game state
    m_pGameState->OnWindowResized(width, height);
}

void BaseGame::OnWindowFocusGained()
{
    Log_DevPrintf("Game window focus gained.");

    // pass to game state
    m_pGameState->OnWindowFocusGained();
}

void BaseGame::OnWindowFocusLost()
{
    Log_DevPrintf("Game window focus lost.");

    // pass to game state
    m_pGameState->OnWindowFocusLost();
}

void BaseGame::OnMainThreadPreFrame(float deltaTime)
{
    // detect any changed resources
    g_pResourceManager->Update();

    // pass to game state
    m_pGameState->OnMainThreadPreFrame(deltaTime);
}

void BaseGame::OnMainThreadBeginFrame(float deltaTime)
{
    // pass to game state
    m_pGameState->OnMainThreadBeginFrame(deltaTime);
}

void BaseGame::OnMainThreadAsyncTick(float deltaTime)
{
    // pass to game state
    m_pGameState->OnMainThreadAsyncTick(deltaTime);
}

void BaseGame::OnMainThreadTick(float deltaTime)
{
    // pass to game state
    m_pGameState->OnMainThreadTick(deltaTime);
}

void BaseGame::OnMainThreadEndFrame(float deltaTime)
{
    // pass to game state
    m_pGameState->OnMainThreadEndFrame(deltaTime);
}

void BaseGame::OnRenderThreadBeginFrame(float deltaTime)
{
    // pass to game state
    m_pGameState->OnRenderThreadBeginFrame(deltaTime);
}

void BaseGame::OnRenderThreadDraw(float deltaTime)
{
    // pass to game state
    m_pGameState->OnRenderThreadDraw(deltaTime);
}

void BaseGame::OnRenderThreadEndFrame(float deltaTime)
{
    // pass to game state
    m_pGameState->OnRenderThreadEndFrame(deltaTime);
}

void BaseGame::RegisterBaseCommands()
{
    // todo: move console as an argument to command handler
    g_pConsole->RegisterCommand("quit", Command_Quit_ExecuteHandler, Command_Quit_HelpHandler, this, this);
    g_pConsole->RegisterCommand("gc", Command_GC_ExecuteHandler, Command_GC_HelpHandler, this, this);
    g_pConsole->RegisterCommand("debugrendermenu", Command_OpenDebugRenderWindow_ExecuteHandler, Command_OpenDebugRenderWindow_HelpHandler, this, this);
    g_pConsole->RegisterCommand("profiler", Command_Profiler_ExecuteHandler, Command_Profiler_HelpHandler, this, this);
    g_pConsole->RegisterCommand("profilerdisplay", Command_ProfilerDisplay_ExecuteHandler, Command_ProfilerDisplay_HelpHandler, this, this);
}

bool BaseGame::Command_Quit_ExecuteHandler(void *userData, uint32 argumentCount, const char *const argumentValues[])
{
    reinterpret_cast<BaseGame *>(userData)->Quit();
    return true;
}

bool BaseGame::Command_Quit_HelpHandler(void *userData, uint32 argumentCount, const char *const argumentValues[])
{
    Log_InfoPrint("  <CR>");
    return true;
}

bool BaseGame::Command_GC_ExecuteHandler(void *userData, uint32 argumentCount, const char *const argumentValues[])
{
    if (argumentCount > 1)
    {
        if (Y_stricmp(argumentValues[1], "step") == 0)
        {
            Log_InfoPrint("Running GC step...");
            g_pScriptManager->RunGCStep();
            return true;
        }
    }

    Log_InfoPrint("Running GC full collect...");
    g_pScriptManager->RunGCFull();
    return true;
}

bool BaseGame::Command_GC_HelpHandler(void *userData, uint32 argumentCount, const char *const argumentValues[])
{
    if (argumentCount == 1)
        Log_InfoPrint("  [step|full] <CR>");
    else
        Log_InfoPrint("  <CR>");

    return true;
}

bool BaseGame::Command_OpenDebugRenderWindow_ExecuteHandler(void *userData, uint32 argumentCount, const char *const argumentValues[])
{
    reinterpret_cast<BaseGame *>(userData)->OpenDebugRenderMenu();
    return true;
}

bool BaseGame::Command_OpenDebugRenderWindow_HelpHandler(void *userData, uint32 argumentCount, const char *const argumentValues[])
{
    Log_InfoPrint("  <CR>");
    return true;
}

bool BaseGame::Command_Profiler_ExecuteHandler(void *userData, uint32 argumentCount, const char *const argumentValues[])
{
#ifdef WITH_PROFILER
    bool currentState = Profiling::GetProfilerEnabled();
    bool newState = currentState;
    if (argumentCount > 1)
    {
        if (Y_stricmp(argumentValues[1], "on") == 0)
            newState = true;
        else
            newState = false;
    }

    if (currentState != newState)
        Profiling::SetProfilerEnabled(newState);

    Log_InfoPrintf("Profiler is %s%s.", (currentState != newState) ? "now " : "", (newState) ? "enabled" : "disabled");
    return true;

#else
    Log_ErrorPrint("Not compiled with profiler support.");
    return true;
#endif
}

bool BaseGame::Command_Profiler_HelpHandler(void *userData, uint32 argumentCount, const char *const argumentValues[])
{
    if (argumentCount == 1)
        Log_InfoPrint("  [off|on] <CR>");
    else
        Log_InfoPrint("  <CR>");

    return true;
}

bool BaseGame::Command_ProfilerDisplay_ExecuteHandler(void *userData, uint32 argumentCount, const char *const argumentValues[])
{
#ifdef WITH_PROFILER
    if (argumentCount > 1)
    {
        static const char *optionsLUT[] = { "off", "bars", "detail", "hidden" };
        for (uint32 i = 0; i < countof(optionsLUT); i++)
        {
            if (Y_stricmp(optionsLUT[i], argumentValues[1]) == 0)
            {
                reinterpret_cast<BaseGame *>(userData)->SetProfilerDisplayMode(i);
                return true;
            }
        }

        Log_ErrorPrintf("Invalid profiler display mode: '%s'", argumentValues[1]);
        return true;
    }

    reinterpret_cast<BaseGame *>(userData)->ToggleProfilerDisplay();
    return true;

#else
    Log_ErrorPrint("Not compiled with profiler support.");
    return true;
#endif
}

bool BaseGame::Command_ProfilerDisplay_HelpHandler(void *userData, uint32 argumentCount, const char *const argumentValues[])
{
    if (argumentCount == 1)
        Log_InfoPrint("  [off|bars|detail|hidden] <CR>");
    else
        Log_InfoPrint("  <CR>");

    return true;
}

void BaseGame::RegisterBaseInputEvents()
{
    g_pInputManager->RegisterActionEvent(this, "PreviousDebugCamera", "Toggle previous debug camera", MakeFunctorClass(this, &BaseGame::InputActionHandler_PreviousDebugCamera));
    g_pInputManager->RegisterActionEvent(this, "NextDebugCamera", "Toggle next debug camera", MakeFunctorClass(this, &BaseGame::InputActionHandler_NextDebugCamera));
}

void BaseGame::BindBaseInputEvents()
{
    g_pInputManager->BindKeyboardKey("ESC", "exec quit");
    g_pInputManager->BindKeyboardKey("F1", "exec debugrendermenu");
    g_pInputManager->BindKeyboardKey("F2", "exec profilerdisplay");
}

void BaseGame::InputActionHandler_PreviousDebugCamera()
{
    if (m_pRenderProfiler == nullptr)
        return;

    if (m_pRenderProfiler->GetCameraOverrideIndex() <= 0)
        m_pRenderProfiler->ClearCameraOverride();
    else
        m_pRenderProfiler->SetCameraOverride(m_pRenderProfiler->GetCameraOverrideIndex() - 1);
}

void BaseGame::InputActionHandler_NextDebugCamera()
{
    if (m_pRenderProfiler == nullptr)
        return;

    m_pRenderProfiler->SetCameraOverride(m_pRenderProfiler->GetCameraOverrideIndex() + 1);
}

void BaseGame::SetNextGameState(GameState *pGameState)
{
    Log_DevPrintf("BaseGame::SetNextGameState(%p)", pGameState);

    // queue next game state
    if (m_pNextGameState != nullptr)
    {
        Log_WarningPrintf("BaseGame::SetNextGameState: More than one next game state set in a frame. Deleting previous next game state.");
        delete m_pNextGameState;
    }

    // should not have any modals active
    Assert(m_gameStateStack.IsEmpty());

    m_pNextGameState = pGameState;
    m_nextGameStateIsModal = false;
}

void BaseGame::BeginModalGameState(GameState *pGameState)
{
    Log_DevPrintf("BaseGame::BeginModalGameState(%p)", pGameState);

    // queue next game state
    if (m_pNextGameState != nullptr)
    {
        Log_WarningPrintf("BaseGame::BeginModalGameState: More than one next game state set in a frame. Deleting previous next game state.");
        delete m_pNextGameState;
    }

    m_pNextGameState = pGameState;
    m_nextGameStateIsModal = true;
}

void BaseGame::EndModalGameState()
{
    Log_DevPrintf("BaseGame::EndModalGameState()");
    Assert(m_gameStateStack.GetSize() > 0);
    m_gameStateEndModal = true;
}

void BaseGame::CriticalError(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    SmallString message;
    message.FormatVA(format, ap);
    va_end(ap);

    Log_ErrorPrint("*** CRITICAL ERROR ***");
    Log_ErrorPrint(message);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Critical error", message, nullptr);
}

void BaseGame::SetRelativeMouseMovement(bool enabled)
{
    QUEUE_RENDERER_LAMBDA_COMMAND([this, enabled]()
    {
        if (m_relativeMouseMovement == enabled)
            return;

        m_relativeMouseMovement = enabled;
        if (enabled)
        {
            if (!m_forcedAbsoluteMouseMovement)
                m_pOutputWindow->SetMouseRelativeMovement(enabled);
        }
        else
        {
            if (!m_forcedAbsoluteMouseMovement)
                m_pOutputWindow->SetMouseRelativeMovement(enabled);
        }
    });
}

void BaseGame::PushForcedAbsoluteMouseMovement()
{
    QUEUE_RENDERER_LAMBDA_COMMAND([this]() 
    {
        if ((m_forcedAbsoluteMouseMovement++) == 0 && m_relativeMouseMovement)
            m_pOutputWindow->SetMouseRelativeMovement(false);
    });
}

void BaseGame::PopForcedAbsoluteMouseMovement()
{
    QUEUE_RENDERER_LAMBDA_COMMAND([this]()
    {
        DebugAssert(m_forcedAbsoluteMouseMovement > 0);
        if ((--m_forcedAbsoluteMouseMovement) == 0 && m_relativeMouseMovement)
            m_pOutputWindow->SetMouseRelativeMovement(true);
    });
}

void BaseGame::RenderThreadCollectEvents(float deltaTime)
{
    MICROPROFILE_SCOPEI("BaseGame", "RenderThreadCollectEvents", MAKE_COLOR_R8G8B8_UNORM(100, 255, 255));

    // get new messages from the underlying subsystem
    SDL_PumpEvents();

    // loop until we have everything
    for (;;)
    {
        // get events
        SDL_Event events[128];
        int nEvents = SDL_PeepEvents(events, countof(events), SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
        if (nEvents <= 0)
            break;

        // process them
        for (int i = 0; i < nEvents; i++)
        {
            // there are only a few messages we are interested in
            const SDL_Event *pEvent = &events[i];
            switch (pEvent->type)
            {
            case SDL_WINDOWEVENT:
                {
                    // the window associated with this event should be our output window, if not, skip it
                    if (SDL_GetWindowFromID(pEvent->window.windowID) != m_pOutputWindow->GetSDLWindow())
                        continue;

                    // handle the event
                    switch (pEvent->window.event)
                    {
                    case SDL_WINDOWEVENT_RESIZED:
                        OnWindowResized(pEvent->window.data1, pEvent->window.data2);
                        m_restartRendererFlag = true;
                        break;

                    case SDL_WINDOWEVENT_CLOSE:
                        m_quitFlag = true;
                        break;

                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        OnWindowFocusGained();
                        break;

                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        OnWindowFocusLost();
                        break;
                    }

                    // don't add it to the main thread's queue, these sorts of messages are of no use to it
                    continue;
                }
                break;

            case SDL_SYSWMEVENT:
                {
                    // syswmevents are of no use to the main thread
                    continue;
                }
                break;

                // handle alt+enter
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                {
                    if (pEvent->key.keysym.scancode == SDL_SCANCODE_RETURN && SDL_GetModState() & (KMOD_LALT | KMOD_RALT))
                    {
                        // only toggle on key up
                        if (pEvent->type == SDL_KEYUP)
                        {
                            Log_DevPrintf("Toggling fullscreen.");
                            g_pConsole->SetCVar(&CVars::r_fullscreen, !CVars::r_fullscreen.GetPendingBool());
                            QueueRendererRestart();
                        }

                        // skip passing to game
                        continue;
                    }
                }
                break;
            }

#ifdef WITH_PROFILER
            // pass to profiler
            if (m_profilerInterceptMouseEvents && Profiling::HandleSDLEvent(pEvent))
                continue;
#endif

#ifdef WITH_IMGUI
            // pass to imgui
            if (m_imGuiEnabled && ImGui::HandleSDLEvent(pEvent, true))
                continue;
#endif

            // append to the main thread's queue for later processing
            m_pendingEvents.Add(*pEvent);
        }
    }
}

void BaseGame::MainThreadProcessInputEvents(float deltaTime)
{
    MICROPROFILE_SCOPEI("BaseGame", "MainThreadProcessInputEvents", MAKE_COLOR_R8G8B8_UNORM(100, 255, 255));

    for (uint32 i = 0; i < m_pendingEvents.GetSize(); i++)
    {
        const SDL_Event *pEvent = &m_pendingEvents[i];
        //Log_DevPrintf("Process pending event %u type %04X", i, pEvent->type);

        // pass to gamestate
        if (m_pGameState->OnWindowEvent(pEvent))
            continue;

        // pass to overlay console first, it intercepts in front
        if (m_pOverlayConsole->OnInputEvent(pEvent))
            continue;

        // check if it can be handled by input manager
        if (g_pInputManager->HandleSDLEvent(pEvent))
            continue;

        // dunno what else to do..
    }

    m_pendingEvents.Clear();
}

void BaseGame::MainThreadFrame(float deltaTime)
{
    MICROPROFILE_SCOPEI("BaseGame", "MainThreadFrame", MAKE_COLOR_R8G8B8_UNORM(100, 255, 255));

    // wait for the render thread to fill the event buffer
    if (Renderer::HasRenderThread())
        m_renderThreadEventsReadyEvent.Wait();

    // begin frame
    {
        MICROPROFILE_SCOPEI("BaseGame", "BeginFrame", MAKE_COLOR_R8G8B8_UNORM(100, 255, 255));
        OnMainThreadBeginFrame(deltaTime);
    }

    // process input events
    {
        MICROPROFILE_SCOPEI("BaseGame", "ProcesInputEvents", MAKE_COLOR_R8G8B8_UNORM(100, 255, 255));
        MainThreadProcessInputEvents(deltaTime);
    }

    // pre-tick hooks
    {
        MICROPROFILE_SCOPEI("BaseGame", "BeginFrame", MAKE_COLOR_R8G8B8_UNORM(100, 255, 255));
        OnMainThreadBeginFrame(deltaTime);
    }

    // run async tick
    {
        MICROPROFILE_SCOPEI("BaseGame", "UpdateAsync", MAKE_COLOR_R8G8B8_UNORM(100, 255, 255));
        OnMainThreadAsyncTick(deltaTime);
    }

    // wait for async commands to finish, use the main thread to help them out
    {
        MICROPROFILE_SCOPEI("BaseGame", "CompleteAsyncCommands", MAKE_COLOR_R8G8B8_UNORM(100, 255, 255));
        g_pEngine->GetAsyncCommandQueue()->ExecuteQueuedCommands();
    }

    // run any callbacks
    {
        MICROPROFILE_SCOPEI("BaseGame", "ExecuteQueuedCommands", MAKE_COLOR_R8G8B8_UNORM(100, 255, 255));
        g_pEngine->GetMainThreadCommandQueue()->ExecuteQueuedCommands();
    }

    // run normal tick
    {
        MICROPROFILE_SCOPEI("BaseGame", "Update", MAKE_COLOR_R8G8B8_UNORM(100, 255, 255));
        OnMainThreadTick(deltaTime);
    }

    // squish any dead script threads
    g_pScriptManager->CheckPausedThreadTimeout(deltaTime);

    // end simulation
    {
        MICROPROFILE_SCOPEI("BaseGame", "EndFrame", MAKE_COLOR_R8G8B8_UNORM(100, 255, 255));
        OnMainThreadEndFrame(deltaTime);
    }

    // main thread done!
    m_fpsCounter.EndGameThreadFrame();
}

bool BaseGame::RendererStart()
{
    // apply pending renderer cvars
    g_pConsole->ApplyPendingRenderCVars();

    // fill parameters
    RendererInitializationParameters initParameters;
    initParameters.EnableThreadedRendering = CVars::r_use_render_thread.GetBool();
    initParameters.BackBufferFormat = PIXEL_FORMAT_R8G8B8A8_UNORM;
    initParameters.DepthStencilBufferFormat = PIXEL_FORMAT_D24_UNORM_S8_UINT;
    initParameters.HideImplicitSwapChain = false;
    initParameters.GPUFrameLatency = CVars::r_gpu_latency.GetUInt();

    // fill platform
    if (!NameTable_TranslateType(NameTables::RendererPlatform, CVars::r_platform.GetString(), &initParameters.Platform, true))
    {
        initParameters.Platform = Renderer::GetDefaultPlatform();
        Log_ErrorPrintf("Invalid renderer platform: '%s', defaulting to %s", CVars::r_platform.GetString().GetCharArray(), NameTable_GetNameString(NameTables::RendererPlatform, initParameters.Platform));
    }

    // determine w/h to use, windowed fullscreen uses desktop size, ie (0, 0)
    if (CVars::r_fullscreen.GetBool() && CVars::r_fullscreen_exclusive.GetBool())
    {
        initParameters.ImplicitSwapChainFullScreen = RENDERER_FULLSCREEN_STATE_FULLSCREEN;
        initParameters.ImplicitSwapChainWidth = CVars::r_fullscreen_width.GetUInt();
        initParameters.ImplicitSwapChainHeight = CVars::r_fullscreen_height.GetUInt();
    }
    else if (CVars::r_fullscreen.GetBool())
    {
        initParameters.ImplicitSwapChainFullScreen = RENDERER_FULLSCREEN_STATE_WINDOWED_FULLSCREEN;
        initParameters.ImplicitSwapChainWidth = 0;
        initParameters.ImplicitSwapChainHeight = 0;
    }
    else
    {
        initParameters.ImplicitSwapChainFullScreen = RENDERER_FULLSCREEN_STATE_WINDOWED;
        initParameters.ImplicitSwapChainWidth = CVars::r_windowed_width.GetUInt();
        initParameters.ImplicitSwapChainHeight = CVars::r_windowed_height.GetUInt();
    }

    // todo: vsync
    initParameters.ImplicitSwapChainVSyncType = RENDERER_VSYNC_TYPE_NONE;

    // HTML5 does not support threads.
    // It also has no concept of windows, so force fullscreen.
#ifdef Y_PLATFORM_HTML5
    int canvasWidth, canvasHeight, canvasFullscreen;
    emscripten_get_canvas_size(&canvasWidth, &canvasHeight, &canvasFullscreen);
    initParameters.EnableThreadedRendering = false;
    initParameters.ImplicitSwapChainWidth = (uint32)canvasWidth;
    initParameters.ImplicitSwapChainHeight = (uint32)canvasHeight;
    initParameters.ImplicitSwapChainFullScreen = RENDERER_FULLSCREEN_STATE_WINDOWED;
#endif

    // create renderer
    if (!Renderer::Create(&initParameters))
    {
        // try some sensible defaults
        Log_ErrorPrintf("Renderer creation failed with specified parameters. Trying fallback.");
        initParameters.Platform = Renderer::GetDefaultPlatform();
        initParameters.ImplicitSwapChainFullScreen = RENDERER_FULLSCREEN_STATE_WINDOWED;
        initParameters.ImplicitSwapChainWidth = 640;
        initParameters.ImplicitSwapChainHeight = 480;
        initParameters.ImplicitSwapChainVSyncType = RENDERER_VSYNC_TYPE_NONE;

        // create again
        if (!Renderer::Create(&initParameters))
        {
            Panic("Failed to create renderer with fallback parameters.");
            return false;
        }
    }

    // initialize imgui
    bool renderThreadResult = false;
    QUEUE_BLOCKING_RENDERER_LAMBA_COMMAND([this, &renderThreadResult]()
    {
        // store variables
        m_pGPUContext = g_pRenderer->GetGPUContext();
        m_pOutputWindow = g_pRenderer->GetImplicitOutputWindow();

        // get actual viewport dimensions
        uint32 bufferWidth = m_pOutputWindow->GetOutputBuffer()->GetWidth();
        uint32 bufferHeight = m_pOutputWindow->GetOutputBuffer()->GetHeight();

        // initialize remaining resources
        m_guiContext.SetGPUContext(m_pGPUContext);
        m_guiContext.SetViewportDimensions(bufferWidth, bufferHeight);
        m_fpsCounter.SetGPUContext(m_pGPUContext);
        m_fpsCounter.CreateGPUResources();

        // imgui
#ifdef WITH_IMGUI
        if (!ImGui::InitializeBridge())
        {
            Log_ErrorPrintf("Failed to initialize ImGui bridge");
            return;
        }
#endif

        // add render profiler
        if (CVars::r_render_profiler.GetBool())
        {
            m_pRenderProfiler = new RenderProfiler(m_pGPUContext);
            m_pRenderProfiler->CreateRendererResources();
        }

        // create world renderer
        WorldRenderer::Options renderOptions;
        renderOptions.InitFromCVars();
        renderOptions.SetRenderResolution(bufferWidth, bufferHeight);
        m_pWorldRenderer = WorldRenderer::Create(m_pGPUContext, &renderOptions);
        if (m_pWorldRenderer == nullptr)
        {
            Log_ErrorPrintf("Failed to create world renderer instance.");
            delete m_pRenderProfiler;
            m_pRenderProfiler = nullptr;
#ifdef WITH_IMGUI
            ImGui::FreeResources();
#endif
            return;
        }

        // init world renderer
        m_pWorldRenderer->SetGUIContext(&m_guiContext);

        // done
        renderThreadResult = true;        
    });

    if (!renderThreadResult)
    {
        m_pOutputWindow = nullptr;
        m_pGPUContext = nullptr;
        g_pRenderer->Shutdown();
        return false;
    }

    // enable resource creation from this thread
    if (g_pRenderer->GetCapabilities().SupportsMultithreadedResourceCreation)
        g_pRenderer->EnableResourceCreationForCurrentThread();

    // done
    return true;
}

void BaseGame::RenderThreadRestartRenderer()
{
    // free existing world renderer
    delete m_pWorldRenderer;
    m_pWorldRenderer = nullptr;

    // get state
    bool modeChanged = (CVars::r_fullscreen.IsChangePending() | CVars::r_fullscreen_exclusive.IsChangePending() | 
                        CVars::r_fullscreen_width.IsChangePending() | CVars::r_fullscreen_height.IsChangePending() |
                        CVars::r_windowed_width.IsChangePending() | CVars::r_windowed_height.IsChangePending());

    // apply pending cvars
    g_pConsole->ApplyPendingRenderCVars();

    // was a mode change pending?
    if (modeChanged)
    {
        RENDERER_FULLSCREEN_STATE fullscreenState;
        uint32 newWidth, newHeight;
        if (CVars::r_fullscreen.GetBool() && CVars::r_fullscreen_exclusive.GetBool())
        {
            fullscreenState = RENDERER_FULLSCREEN_STATE_FULLSCREEN;
            newWidth = CVars::r_fullscreen_width.GetUInt();
            newHeight = CVars::r_fullscreen_height.GetUInt();
        }
        else if (CVars::r_fullscreen.GetBool())
        {
            fullscreenState = RENDERER_FULLSCREEN_STATE_WINDOWED_FULLSCREEN;
            newWidth = 0;
            newHeight = 0;
        }
        else
        {
            fullscreenState = RENDERER_FULLSCREEN_STATE_WINDOWED;
            newWidth = CVars::r_windowed_width.GetUInt();
            newHeight = CVars::r_windowed_height.GetUInt();
        }

        // switch modes
        if (!g_pRenderer->ChangeResolution(fullscreenState, newWidth, newHeight, 60))
            Log_ErrorPrintf("Failed to change resolutions.");
    }

    // process any pending events due to window changes
    RenderThreadCollectEvents(0.0f);

    // get actual viewport dimensions
    uint32 bufferWidth = m_pOutputWindow->GetOutputBuffer()->GetWidth();
    uint32 bufferHeight = m_pOutputWindow->GetOutputBuffer()->GetHeight();

    // set render options
    WorldRenderer::Options renderOptions;
    renderOptions.InitFromCVars();
    renderOptions.SetRenderResolution(bufferWidth, bufferHeight);

    // allocate new renderer
    m_pWorldRenderer = WorldRenderer::Create(m_pGPUContext, &renderOptions);
    if (m_pWorldRenderer == nullptr)
    {
        Panic("Failed to create world renderer instance.");
        return;
    }

    // update gui context dimensions
    m_guiContext.SetViewportDimensions(bufferWidth, bufferHeight);
    m_pWorldRenderer->SetGUIContext(&m_guiContext);

    // update imgui
#ifdef WITH_IMGUI
    ImGui::SetViewportDimensions(bufferWidth, bufferHeight);
#endif

    // update render profiler
    if (CVars::r_render_profiler.GetBool())
    {
        if (m_pRenderProfiler == nullptr)
        {
            m_pRenderProfiler = new RenderProfiler(m_pGPUContext);
            m_pRenderProfiler->BeginFrame();
        }

        if (CVars::r_render_profiler_gpu_time.GetBool() != m_pRenderProfiler->GetGPUStatsEnabled())
            m_pRenderProfiler->SetGPUStatsEnabled(CVars::r_render_profiler_gpu_time.GetBool());
    }
    else
    {
        if (m_pRenderProfiler != nullptr)
        {
            m_pRenderProfiler->EndFrame();
            delete m_pRenderProfiler;
            m_pRenderProfiler = nullptr;
        }
    }

    // set a default viewport on the main context, most likely the game will override this but it's good for loading etc
    m_pGPUContext->SetFullViewport();
}

void BaseGame::RenderThreadFrame(float deltaTime)
{
    MICROPROFILE_SCOPEI("BaseGame", "RenderThreadFrame", MAKE_COLOR_R8G8B8_UNORM(100, 255, 255));
    MICROPROFILE_SCOPEGPUI("RenderThreadFrame", MAKE_COLOR_R8G8B8_UNORM(100, 255, 255));

    // start of frame
    g_pRenderer->BeginFrame();
    if (m_pRenderProfiler != nullptr)
        m_pRenderProfiler->BeginFrame();

    // begin frame
    RENDER_PROFILER_BEGIN_SECTION(m_pRenderProfiler, "OnRenderThreadBeginFrame", false);
    OnRenderThreadBeginFrame(deltaTime);
    RENDER_PROFILER_END_SECTION(m_pRenderProfiler);

    // collect events
    RENDER_PROFILER_BEGIN_SECTION(m_pRenderProfiler, "RenderThreadCollectEvents", false);
    RenderThreadCollectEvents(deltaTime);
    RENDER_PROFILER_END_SECTION(m_pRenderProfiler);

    // restart the renderer if a request is queued
    if (m_restartRendererFlag)
    {
        m_restartRendererFlag = false;
        RenderThreadRestartRenderer();
    }

    // signal to the main thread that events are ready
    if (Renderer::HasRenderThread())
        m_renderThreadEventsReadyEvent.Signal();

    // clear the backbuffer/depth buffer, this is mainly as a help to tilers
    m_pGPUContext->SetRenderTargets(0, nullptr, nullptr);
    m_pGPUContext->SetFullViewport();
    m_pGPUContext->DiscardTargets(true, true, true);
    m_pGPUContext->ClearTargets(true, true, true);

#ifdef WITH_IMGUI
    // imgui stuff
    if (m_imGuiEnabled)
        ImGui::NewFrame(deltaTime);
#endif

    // call event
    RENDER_PROFILER_BEGIN_SECTION(m_pRenderProfiler, "OnRenderThreadDraw", true);
    OnRenderThreadDraw(deltaTime);
    RENDER_PROFILER_END_SECTION(m_pRenderProfiler);

    // draw overlays
    RENDER_PROFILER_BEGIN_SECTION(m_pRenderProfiler, "RenderThreadDrawOverlays", true);
    RenderThreadDrawOverlays(deltaTime);
    RENDER_PROFILER_END_SECTION(m_pRenderProfiler);

    // end frame
    RENDER_PROFILER_BEGIN_SECTION(m_pRenderProfiler, "OnRenderThreadEndFrame", false);
    OnRenderThreadEndFrame(deltaTime);
    RENDER_PROFILER_END_SECTION(m_pRenderProfiler);

    // clear state of context, and swap buffers
    RENDER_PROFILER_BEGIN_SECTION(m_pRenderProfiler, "Clear and swap buffers", false);
    {
        MICROPROFILE_SCOPEI("BaseGame", "SwapBuffers", MAKE_COLOR_R8G8B8_UNORM(100, 255, 255));

        m_pGPUContext->ClearState(true, true, true, true);
        m_pGPUContext->PresentOutputBuffer(GPU_PRESENT_BEHAVIOUR_IMMEDIATE); /* @TODO */
    }
    RENDER_PROFILER_END_SECTION(m_pRenderProfiler);

    // end of frame
    if (m_pRenderProfiler != nullptr)
        m_pRenderProfiler->EndFrame();

    // end of render thread's work
    m_fpsCounter.EndRenderThreadFrame();

    // signal to the main thread that we have completed the frame
    if (Renderer::HasRenderThread())
        m_renderThreadFrameCompleteEvent.Signal();
}

void BaseGame::RenderThreadDrawOverlays(float deltaTime)
{
    const int32 PANEL_MARGIN = 4;
    const int32 viewportWidth = (int32)m_guiContext.GetViewportWidth();
    const int32 viewportHeight = (int32)m_guiContext.GetViewportHeight();
    MINIGUI_RECT rect;
    UNREFERENCED_PARAMETER(viewportWidth);
    UNREFERENCED_PARAMETER(viewportHeight);

    // fire off any outstanding draw requests using the old state
    m_guiContext.Flush();

    // engine overlays are always drawn over everything else, taking up the whole screen.
    m_pGPUContext->SetRenderTargets(0, nullptr, nullptr);
    m_pGPUContext->SetFullViewport();

    // batch stuff
    m_guiContext.PushManualFlush();

    // draw profiler
    if (m_pRenderProfiler != nullptr)
        m_pRenderProfiler->DrawPreviousFrameSummary(&m_guiContext, nullptr);

    // draw fps counter
    // draw panel outline for fps counter
    rect.Set(viewportWidth - 380 - PANEL_MARGIN, viewportWidth, 0, 96 + PANEL_MARGIN);
    m_guiContext.SetAlphaBlendingEnabled(true);
    m_guiContext.SetAlphaBlendingMode(MiniGUIContext::ALPHABLENDING_MODE_STRAIGHT);
    m_guiContext.PushRect(&rect);
    rect.Set(0, rect.right - rect.left, 0, rect.bottom - rect.top);
    m_guiContext.DrawFilledRect(&rect, MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 100));
    m_fpsCounter.DrawDetails(g_pRenderer->GetFixedResources()->GetDebugFont(), &m_guiContext, PANEL_MARGIN, 0);

    // extended stuff
    {
        WorldRenderer::RenderStats rs;
        m_pWorldRenderer->GetRenderStats(&rs);

        m_guiContext.DrawFormattedTextAt(PANEL_MARGIN, 48, g_pRenderer->GetFixedResources()->GetDebugFont(), 16, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), "framenumber: %u", g_pRenderer->GetFrameNumber());
        m_guiContext.DrawFormattedTextAt(PANEL_MARGIN, 64, g_pRenderer->GetFixedResources()->GetDebugFont(), 16, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), "objects drawn: %u (%u culled)", rs.ObjectCount, rs.ObjectsCulledByOcclusion);
        m_guiContext.DrawFormattedTextAt(PANEL_MARGIN, 80, g_pRenderer->GetFixedResources()->GetDebugFont(), 16, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), "dlights: %u (%u shadow maps)", rs.LightCount, rs.ShadowMapCount);
        m_guiContext.DrawFormattedTextAt(PANEL_MARGIN, 96, g_pRenderer->GetFixedResources()->GetDebugFont(), 16, MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255), "buffers: %u (vram: %s)", rs.IntermediateBufferCount, StringConverter::SizeToHumanReadableString(rs.IntermediateBufferMemoryUsage).GetCharArray());
    }

    // done
    m_guiContext.PopRect();

    // update&draw overlay console
    m_pOverlayConsole->Update(deltaTime);
    m_pOverlayConsole->Draw(&m_guiContext);

    // unbatch and reset state
    m_guiContext.Flush();
    m_guiContext.PopManualFlush();
    m_guiContext.ClearState();

#ifdef WITH_PROFILER
    // profiler
    Profiling::DrawDisplay();

    // in case an event killed us
    if (m_profilerInterceptMouseEvents && !Profiling::IsProfilerDisplayEnabled())
    {
        PopForcedAbsoluteMouseMovement();
        m_profilerInterceptMouseEvents = false;
    }

#endif

#ifdef WITH_IMGUI
    if (m_imGuiEnabled)
    {
        // draw imgui
        RenderThreadDrawImGuiOverlays();
        ImGui::Render();
    }
#endif
}

void BaseGame::RendererShutdown()
{
    delete m_pWorldRenderer;
    m_pWorldRenderer = nullptr;

    delete m_pRenderProfiler;
    m_pRenderProfiler = nullptr;

#ifdef WITH_IMGUI
    ImGui::FreeResources();
#endif

    m_fpsCounter.ReleaseGPUResources();

    g_pResourceManager->ReleaseDeviceResources();

    m_pOutputWindow = nullptr;
    m_pGPUContext = nullptr;
    g_pRenderer->DisableResourceCreationForCurrentThread();
    g_pRenderer->Shutdown();
}

void BaseGame::MainThreadGameLoop(bool isModal)
{
    // loop
    while (!m_quitFlag)
        MainThreadGameLoopIteration();
    
    // if there's a game state stack, we need to end all modals
    while (!m_gameStateStack.IsEmpty())
    {
        EndModalGameState();
        MainThreadGameLoopIteration();
    }

    // if there was a next game state, nuke it
    if (m_pNextGameState != nullptr)
    {
        m_pNextGameState->OnBeforeDelete();
        delete m_pNextGameState;
        m_pNextGameState = nullptr;
    }

    // delete the current game state
    m_pGameState->OnSwitchedOut();
    m_pGameState->OnBeforeDelete();
    delete m_pGameState;
    m_pGameState = nullptr;
}

void BaseGame::MainThreadGameLoopIteration()
{
    // handle gamestate changes
    if (m_pNextGameState != nullptr)
    {
        // switching to modal?
        if (m_nextGameStateIsModal)
        {
            // can't start modal with no game state. push the current to the modal stack
            DebugAssert(m_pGameState != nullptr);
            m_pGameState->OnSwitchedOut();
            m_gameStateStack.Add(m_pGameState);

            // relative mouse movement should be disabled
            DebugAssert(!m_relativeMouseMovement);

            // switch to the new modal state
            Log_DevPrintf("BaseGame::MainThreadGameLoopIteration: Switching to modal game state %p", m_pNextGameState);
            m_pGameState = m_pNextGameState;
            m_pNextGameState = nullptr;
            m_nextGameStateIsModal = false;

            // run callbacks
            m_pGameState->OnSwitchedIn();
        }
        else
        {
            // currently have a gamestate?
            if (m_pGameState != nullptr)
            {
                // nuke the current game state
                m_pGameState->OnSwitchedOut();
                m_pGameState->OnBeforeDelete();
                delete m_pGameState;

                // relative mouse movement should be disabled
                DebugAssert(!m_relativeMouseMovement);
            }

            // set new game state
            Log_DevPrintf("BaseGame::MainThreadGameLoopIteration: Switching to game state %p", m_pNextGameState);
            m_pGameState = m_pNextGameState;
            m_pNextGameState = nullptr;

            // run callbacks
            m_pGameState->OnSwitchedIn();
        }
    }

    // sanity check here
    if (m_pGameState == nullptr)
        Panic("No game state. Unable to continue.");

    // update stats
    m_fpsCounter.BeginFrame();

    // calculate time since last frame
    float deltaTime = (float)m_frameTimer.GetTimeSeconds();
    m_frameTimer.Reset();

    // pre-frame tasks
    OnMainThreadPreFrame(deltaTime);

    // using threaded rendering?
    if (Renderer::HasRenderThread())
    {
        // kick off render thread first 
        QUEUE_RENDERER_LAMBDA_COMMAND([this, deltaTime]() {
            RenderThreadFrame(deltaTime);
        });

        // run the main thread
        MainThreadFrame(deltaTime);

        // wait for the render thread to finish, this is an event so as not to block the render thread as well
        m_renderThreadFrameCompleteEvent.Wait();
    }
    else
    {
        // not using render thread, we run in a different order (process logic, then render, to reduce wait-for-vsync stalls)
        MainThreadFrame(deltaTime);
        RenderThreadFrame(deltaTime);
        Renderer::GetCommandQueue()->ExecuteQueuedCommands();
    }

    // end of this modal state?
    if (m_gameStateEndModal)
    {
        // we should have a game state stack
        DebugAssert(!m_gameStateStack.IsEmpty());
        m_gameStateEndModal = false;

        // switch out the modal state and nuke it
        Log_DevPrintf("BaseGame::MainThreadGameLoopIteration: Modal game state %p ending", m_pGameState);
        m_pGameState->OnSwitchedOut();
        m_pGameState->OnBeforeDelete();
        delete m_pGameState;

        // relative mouse movement should be disabled
        DebugAssert(!m_relativeMouseMovement);

        // restore the last game state
        Log_DevPrintf("BaseGame::MainThreadGameLoopIteration: Restoring game state %p", m_gameStateStack.LastElement());
        m_pGameState = m_gameStateStack.PopBack();
        m_pGameState->OnSwitchedIn();
    }

    // end frame
#ifdef WITH_PROFILER
    Profiling::EndFrame();
#endif
}

#ifdef Y_PLATFORM_HTML5

void BaseGame::HTML5FrameTrampoline(void *pParam)
{
    BaseGame *pBaseGame = reinterpret_cast<BaseGame *>(pParam);
    pBaseGame->MainThreadGameLoopIteration();
}

#endif

int32 BaseGame::MainEntryPoint()
{
    // guard in case something calls this function
    static bool __mainEntered = false;
    DebugAssert(__mainEntered == false);
    __mainEntered = true;

    // initialization timer
    Timer initTimer;
    int32 exitCode = 0;

    // initialize SDL first
    if (SDL_Init(0) < 0)
    {
        CriticalError("SDL initialization failed: %s", SDL_GetError());
        exitCode = -1;
        goto RETURN_LABEL;
    }

    // print version info
    {
        Y_CPUID_RESULT CPUIDResult;
        Y_ReadCPUID(&CPUIDResult);

        Log_DevPrint("Build Configuration: " Y_BUILD_CONFIG_STR);
        Log_DevPrint("Build Platform: " Y_PLATFORM_STR);
        Log_DevPrint("Build Architecture: " Y_CPU_STR Y_CPU_FEATURES_STR);
        Log_DevPrintf("Running on CPU: %s", CPUIDResult.SummaryString);
    }

    // initialize vfs
    Log_InfoPrint("Initializing virtual file system...");
    if (!g_pVirtualFileSystem->Initialize())
    {
        CriticalError("Virtual file system initialization failed. Cannot continue.");
        exitCode = -2;
        goto SHUTDOWN_SDL_LABEL;
    }

    // add types
    Log_InfoPrint("Registering types...");
    g_pEngine->RegisterEngineTypes();
    OnRegisterTypes();

    // fix this...
    if (!g_pEngine->Startup())
    {
        exitCode = -3;
        goto SHUTDOWN_VFS_LABEL;
    }

    // start renderer
    Log_InfoPrint("Starting renderer...");
    if (!RendererStart())
    {
        CriticalError("Failed to start renderer.");
        exitCode = -4;
        goto SHUTDOWN_RESOURCES_LABEL;
    }

    // start input system
    Log_InfoPrint("Starting input subsystem...");
    if (!g_pInputManager->Startup())
    {
        CriticalError("Failed to start input subsystem.");
        exitCode = -5;
        goto SHUTDOWN_RENDERER_LABEL;
    }

    // start script subsystem
    Log_InfoPrint("Starting script subsystem...");
    if (!g_pScriptManager->Startup())
    {
        CriticalError("Failed to start script subsystem.");
        exitCode = -6;
        goto SHUTDOWN_INPUT_LABEL;
    }

#ifdef WITH_PROFILER
    // initialize profiler
    Log_InfoPrintf("Initializing profiler...");
    Profiling::Initialize();
#endif

    // everything started
    Log_InfoPrintf("All engine subsystems initialized in %.2f msec", initTimer.GetTimeMilliseconds());

    // initialize game
    initTimer.Reset();
    Log_InfoPrint("Initializing game...");
    if (!OnStart())
    {
        CriticalError("Failed to initialize game.");
        exitCode = -999;
        goto SHUTDOWN_SCRIPT_LABEL;
    }

#ifndef Y_PLATFORM_HTML5
    // run game loop
    MainThreadGameLoop(false);
#else
    // kick html5 frame off
    emscripten_set_main_loop_arg(HTML5FrameTrampoline, this, 0, 1);
#endif

    // shut down game
    Log_InfoPrint("Shutting down game...");
    OnExit();

    // begin shutting down
    Log_InfoPrint("Shutting down all subsystems...");
    initTimer.Reset();

SHUTDOWN_SCRIPT_LABEL:
    // shutdown script subsystem
    Log_InfoPrint("Shutting down script subsystem...");
    g_pScriptManager->Shutdown();

SHUTDOWN_INPUT_LABEL:
    // shutdown input subsystem
    Log_InfoPrint("Shutting down input subsystem...");
    g_pInputManager->Shutdown();

SHUTDOWN_RENDERER_LABEL:
    // shutdown renderer
    Log_InfoPrint("Shutting down renderer...");
    RendererShutdown();

SHUTDOWN_RESOURCES_LABEL:
    // release resources
    Log_InfoPrint("Unloading resources...");
    g_pResourceManager->ReleaseResources();
    g_pEngine->Shutdown();

SHUTDOWN_VFS_LABEL:
    // shutdown vfs
    g_pVirtualFileSystem->Shutdown();

SHUTDOWN_SDL_LABEL:
    SDL_Quit();

RETURN_LABEL:
    if (exitCode == 0)
        Log_InfoPrintf("All engine subsystems shutdown in %.2f msec", initTimer.GetTimeMilliseconds());

    return 0;
}

#ifdef WITH_IMGUI

void BaseGame::ActivateImGui()
{
    QUEUE_RENDERER_LAMBDA_COMMAND([this]()
    {
        if ((m_imGuiEnabled++) == 0)
        {
            // calls to imgui will fail without this. deliberately not calling our wrapper.
            PushForcedAbsoluteMouseMovement();
            ImGui::NewFrame();
        }
    });
}

void BaseGame::DeactivateImGui()
{
    QUEUE_RENDERER_LAMBDA_COMMAND([this]()
    {
        DebugAssert(m_imGuiEnabled > 0);
        if (--m_imGuiEnabled == 0)
            PopForcedAbsoluteMouseMovement();
    });
}

void BaseGame::OpenDebugRenderMenu()
{
    QUEUE_RENDERER_LAMBDA_COMMAND([this]()
    {
        if (!m_showDebugRenderMenu)
        {
            m_showDebugRenderMenu = true;
            ActivateImGui();
        }
    });
}

void BaseGame::RenderThreadDrawImGuiOverlays()
{
    // debug menu
    if (m_showDebugRenderMenu)
    {
        ImGui::SetNextWindowSize(ImVec2(460, 450), ImGuiSetCond_Once);
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiSetCond_Once);
        
        bool closed = false;
        if (ImGui::Begin("Render Debug", &closed, ImGuiWindowFlags_NoCollapse))
        {
            int intValue;
            bool boolValue;
            ImGui::PushItemWidth(-140.0f);

            if (ImGui::CollapsingHeader("Display", nullptr, true, true))
            {
                ImGui::PushItemWidth(160.0f);
                ImGui::Text("Mode: ");
                ImGui::SameLine();

                intValue = (int)CVars::r_fullscreen.GetPendingBool();
                if (intValue)
                    intValue += (int)CVars::r_fullscreen_exclusive.GetPendingBool();

                if (ImGui::Combo("##mode", &intValue, "Windowed\0Windowed Fullscreen\0Exclusive Fullscreen\0\0"))
                {
                    g_pConsole->SetCVar(&CVars::r_fullscreen, intValue != 0);
                    g_pConsole->SetCVar(&CVars::r_fullscreen_exclusive, intValue > 1);
                }

                ImGui::PopItemWidth();
                
                if (intValue != 1)
                {
                    ImGui::PushItemWidth(80.0f);
                    ImGui::Text("Resolution: ");
                    ImGui::SameLine();

                    if (intValue == 0)
                    {
                        intValue = CVars::r_windowed_width.GetPendingInt();
                        if (ImGui::InputInt("##windowed_width", &intValue))
                            g_pConsole->SetCVar(&CVars::r_windowed_width, intValue);

                        ImGui::SameLine();

                        intValue = CVars::r_windowed_height.GetPendingInt();
                        if (ImGui::InputInt("##windowed_height", &intValue))
                            g_pConsole->SetCVar(&CVars::r_windowed_height, intValue);
                    }
                    else if (intValue == 2)
                    {
                        intValue = CVars::r_fullscreen_width.GetPendingInt();
                        if (ImGui::InputInt("##fullscreen_width", &intValue))
                            g_pConsole->SetCVar(&CVars::r_fullscreen_width, intValue);

                        ImGui::SameLine();

                        intValue = CVars::r_fullscreen_height.GetPendingInt();
                        if (ImGui::InputInt("##fullscreen_height", &intValue))
                            g_pConsole->SetCVar(&CVars::r_fullscreen_height, intValue);
                    }

                    ImGui::PopItemWidth();
                }

                if (ImGui::Button("Update"))
                    QueueRendererRestart();
            }

            if (ImGui::CollapsingHeader("Render Settings", nullptr, true, true))
            {
                // shadows
                boolValue = CVars::r_deferred_shading.GetBool();
                if (ImGui::Checkbox("Deferred Shading", &boolValue))
                {
                    g_pConsole->SetCVar(&CVars::r_deferred_shading, boolValue);
                    QueueRendererRestart();
                }

                // shadows
                intValue = CVars::r_shadows.GetInt();
                if (ImGui::Text("Dynamic shadows: "), ImGui::SameLine(), ImGui::Combo("##dynamic_shadows", &intValue, "No shadows\0All shadows\0Directional shadows only\0\0"))
                {
                    g_pConsole->SetCVar(&CVars::r_shadows, intValue);
                    QueueRendererRestart();
                }

                // shadows
                intValue = CVars::r_shadow_filtering.GetBool();
                if (ImGui::Text("Shadow filtering: "), ImGui::SameLine(), ImGui::Combo("##shadow_filtering", &intValue, "No filtering\0""3x3 filter\0""5x5 filter\0\0"))
                {
                    g_pConsole->SetCVar(&CVars::r_shadow_filtering, intValue);
                    QueueRendererRestart();
                }

                // shadows
                boolValue = CVars::r_shadow_use_hardware_pcf.GetBool();
                if (ImGui::Checkbox("Use hardware pcf", &boolValue))
                {
                    g_pConsole->SetCVar(&CVars::r_shadow_use_hardware_pcf, boolValue);
                    QueueRendererRestart();
                }

                // ssao
                boolValue = CVars::r_ssao.GetBool();
                if (ImGui::Checkbox("SSAO", &boolValue))
                {
                    g_pConsole->SetCVar(&CVars::r_ssao, boolValue);
                    QueueRendererRestart();
                }

                // occlusion culling
                boolValue = CVars::r_occlusion_culling.GetBool();
                if (ImGui::Checkbox("Occlusion Culling", &boolValue))
                {
                    g_pConsole->SetCVar(&CVars::r_occlusion_culling, boolValue);
                    QueueRendererRestart();
                }

                // wait for results
                boolValue = CVars::r_occlusion_culling_wait_for_results.GetBool();
                if (ImGui::Checkbox("Wait for occlusion results", &boolValue))
                {
                    g_pConsole->SetCVar(&CVars::r_occlusion_culling_wait_for_results, boolValue);
                    QueueRendererRestart();
                }

                // predication
                boolValue = CVars::r_occlusion_prediction.GetBool();
                if (ImGui::Checkbox("Occlusion Predication", &boolValue))
                {
                    g_pConsole->SetCVar(&CVars::r_occlusion_prediction, boolValue);
                    QueueRendererRestart();
                }
            }

            if (ImGui::CollapsingHeader("Debugging", nullptr, true, true))
            {
                // wireframe
                boolValue = CVars::r_wireframe.GetBool();
                if (ImGui::Checkbox("Render Wireframe", &boolValue))
                {
                    g_pConsole->SetCVar(&CVars::r_wireframe, boolValue);
                    QueueRendererRestart();
                }

                // fullbright
                boolValue = CVars::r_fullbright.GetBool();
                if (ImGui::Checkbox("Render Fullbright", &boolValue))
                {
                    g_pConsole->SetCVar(&CVars::r_fullbright, boolValue);
                    QueueRendererRestart();
                }

                // normals
                boolValue = CVars::r_debug_normals.GetBool();
                if (ImGui::Checkbox("Render Normals", &boolValue))
                {
                    g_pConsole->SetCVar(&CVars::r_debug_normals, boolValue);
                    QueueRendererRestart();
                }

                // show buffers
                boolValue = CVars::r_show_buffers.GetBool();
                if (ImGui::Checkbox("Show GBuffers", &boolValue))
                {
                    g_pConsole->SetCVar(&CVars::r_show_buffers, boolValue);
                    QueueRendererRestart();
                }

                // show cascades
                boolValue = CVars::r_show_cascades.GetBool();
                if (ImGui::Checkbox("Show Cascades", &boolValue))
                {
                    g_pConsole->SetCVar(&CVars::r_show_cascades, boolValue);
                    QueueRendererRestart();
                }
            }

            if (ImGui::CollapsingHeader("Profiler", nullptr, true, true))
            {
                boolValue = CVars::r_render_profiler.GetBool();
                if (ImGui::Checkbox("Enabled", &boolValue))
                {
                    g_pConsole->SetCVar(&CVars::r_render_profiler, boolValue);
                    QueueRendererRestart();
                }

                boolValue = CVars::r_render_profiler_gpu_time.GetBool();
                if (ImGui::Checkbox("Profile GPU time", &boolValue))
                {
                    g_pConsole->SetCVar(&CVars::r_render_profiler_gpu_time, boolValue);
                    QueueRendererRestart();
                }
            }

            ImGui::PopItemWidth();

            if (ImGui::CollapsingHeader("Frame time history", nullptr, true, true))
            {
                ImGui::PushItemWidth(-120.0f);

                float frameTimes[60];
                m_fpsCounter.GetFrameTimeHistory(frameTimes, countof(frameTimes));
                ImGui::PlotLines("", frameTimes, countof(frameTimes), 0, nullptr, FLT_MAX, FLT_MAX, ImVec2(0.0f, 44.0f));
                float minTime = Y_FLT_MAX, maxTime = Y_FLT_MIN, avgTime = 0.0f;
                for (uint32 i = 0; i < countof(frameTimes); i++)
                {
                    minTime = Min(minTime, frameTimes[i]);
                    maxTime = Max(maxTime, frameTimes[i]);
                    avgTime += frameTimes[i];
                }
                avgTime /= (float)countof(frameTimes);
                ImGui::SameLine();
                ImGui::Text("Min: %.4fms\nMax: %.4fms\nAvg: %.4fms", minTime * 1000.0f, maxTime * 1000.0f, avgTime * 1000.0f);

                ImGui::PopItemWidth();
            }

            if (closed)
            {
                // render debug window closed, only way it could've been activated was from the command, so hide a level of imgui
                m_showDebugRenderMenu = false;
                DeactivateImGui();
            }
        }
        ImGui::End();
    }
}

#endif          // WITH_IMGUI

#ifdef WITH_PROFILER

void BaseGame::SetProfilerDisplayMode(uint32 mode)
{
    Profiling::SetProfilerDisplay(mode);

    if (Profiling::IsProfilerDisplayEnabled())
    {
        // requires intercepting input events
        if (!m_profilerInterceptMouseEvents)
        {
            PushForcedAbsoluteMouseMovement();
            m_profilerInterceptMouseEvents = true;
        }
    }
    else
    {
        // remove interception
        if (m_profilerInterceptMouseEvents)
        {
            PopForcedAbsoluteMouseMovement();
            m_profilerInterceptMouseEvents = false;
        }
    }
}

void BaseGame::ToggleProfilerDisplay()
{
    Profiling::ToggleProfilerDisplay();

    if (Profiling::IsProfilerDisplayEnabled())
    {
        // requires intercepting input events
        if (!m_profilerInterceptMouseEvents)
        {
            PushForcedAbsoluteMouseMovement();
            m_profilerInterceptMouseEvents = true;
        }
    }
    else
    {
        // remove interception
        if (m_profilerInterceptMouseEvents)
        {
            PopForcedAbsoluteMouseMovement();
            m_profilerInterceptMouseEvents = false;
        }
    }
}

void BaseGame::HideProfilerDisplay()
{
    Profiling::SetProfilerDisplay(MP_DRAW_OFF);
    if (m_profilerInterceptMouseEvents)
    {
        PopForcedAbsoluteMouseMovement();
        m_profilerInterceptMouseEvents = false;
    }
}

#endif          // WITH_PROFILER
