#include "Engine/InputManager.h"
#include "Engine/ScriptManager.h"
#include "Engine/FPSCounter.h"
#include "Engine/EngineCVars.h"
#include "Engine/Engine.h"
#include "Engine/ResourceManager.h"
#include "Engine/SDLHeaders.h"
#include "Renderer/Renderer.h"
#include "Renderer/ImGuiBridge.h"
#include "YBaseLib/CPUID.h"
Log_SetChannel(TestRenderer);

static FPSCounter g_fpsCounter;
static MiniGUIContext g_guiContext;
static RendererOutputWindow *g_pOutputWindow = nullptr;
static GPUContext *g_pMainGPUContext = nullptr;
static bool g_quitFlag = false;

static bool RendererStart()
{
    // apply pending renderer cvars
    g_pConsole->ApplyPendingRenderCVars();

    // fill parameters
    RendererInitializationParameters initParameters;
    initParameters.EnableThreadedRendering = CVars::r_use_render_thread.GetBool();
    initParameters.BackBufferFormat = PIXEL_FORMAT_R8G8B8A8_UNORM;
    initParameters.DepthStencilBufferFormat = PIXEL_FORMAT_D24_UNORM_S8_UINT;
    initParameters.HideImplicitSwapChain = false;

    // fill platform
    if (!NameTable_TranslateType(NameTables::RendererPlatform, CVars::r_platform.GetString(), &initParameters.Platform, true))
    {
        initParameters.Platform = Renderer::GetDefaultPlatform();
        Log_ErrorPrintf("Invalid renderer platform: '%s', defaulting to %s", CVars::r_platform.GetString().GetCharArray(), NameTable_GetNameString(NameTables::RendererPlatform, initParameters.Platform));
    }

    // determine w/h to use, windowed fullscreen uses desktop size, ie (0, 0)
    if (CVars::r_fullscreen.GetBool())
        initParameters.ImplicitSwapChainFullScreen = (CVars::r_fullscreen_exclusive.GetBool()) ? RENDERER_FULLSCREEN_STATE_FULLSCREEN : RENDERER_FULLSCREEN_STATE_WINDOWED_FULLSCREEN;
    else
        initParameters.ImplicitSwapChainFullScreen = RENDERER_FULLSCREEN_STATE_WINDOWED;
    
    if (initParameters.ImplicitSwapChainFullScreen == RENDERER_FULLSCREEN_STATE_FULLSCREEN)
    {
        initParameters.ImplicitSwapChainWidth = CVars::r_fullscreen_width.GetUInt();
        initParameters.ImplicitSwapChainHeight = CVars::r_fullscreen_height.GetUInt();
    }
    else if (initParameters.ImplicitSwapChainFullScreen == RENDERER_FULLSCREEN_STATE_WINDOWED)
    {
        initParameters.ImplicitSwapChainWidth = CVars::r_windowed_width.GetUInt();
        initParameters.ImplicitSwapChainHeight = CVars::r_windowed_height.GetUInt();
    }

    // todo: vsync
    initParameters.ImplicitSwapChainVSyncType = RENDERER_VSYNC_TYPE_NONE;

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

    // store variables
    g_pMainGPUContext = g_pRenderer->GetGPUContext();
    g_pOutputWindow = g_pRenderer->GetImplicitOutputWindow();

    // get actual viewport dimensions
    uint32 bufferWidth = g_pOutputWindow->GetOutputBuffer()->GetWidth();
    uint32 bufferHeight = g_pOutputWindow->GetOutputBuffer()->GetHeight();

    // initialize remaining resources
    g_fpsCounter.SetGPUContext(g_pMainGPUContext);
    g_fpsCounter.CreateGPUResources();
    g_guiContext.SetViewportDimensions(bufferWidth, bufferHeight);
    g_guiContext.SetGPUContext(g_pMainGPUContext);

    // init imgui
    ImGui::InitializeBridge();

    // done
    return true;
}

static bool Boot(void(*RunCallback)())
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
        Log_ErrorPrintf("SDL initialization failed: %s", SDL_GetError());
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
        Log_InfoPrint("Virtual file system initialization failed. Cannot continue.");
        exitCode = -2;
        goto SHUTDOWN_SDL_LABEL;
    }

    // add types
    Log_InfoPrint("Registering types...");
    g_pEngine->RegisterEngineTypes();

    // start renderer
    Log_InfoPrint("Starting renderer...");
    if (!RendererStart())
    {
        Log_ErrorPrint("Failed to start renderer.");
        exitCode = -3;
        goto SHUTDOWN_VFS_LABEL;
    }

    // start input system
    Log_InfoPrint("Starting input subsystem...");
    if (!g_pInputManager->Startup())
    {
        Log_ErrorPrint("Failed to start input subsystem.");
        exitCode = -4;
        goto SHUTDOWN_RENDERER_LABEL;
    }

    // start script subsystem
    Log_InfoPrint("Starting script subsystem...");
    if (!g_pScriptManager->Startup())
    {
        Log_ErrorPrint("Failed to start script subsystem.");
        exitCode = -5;
        goto SHUTDOWN_INPUT_LABEL;
    }

    // fix this...
    if (!g_pEngine->Startup())
    {
        exitCode = -6;
        goto SHUTDOWN_SCRIPT_LABEL;
    }

    // everything started
    Log_InfoPrintf("All engine subsystems initialized in %.2f msec", initTimer.GetTimeMilliseconds());

    // run
    RunCallback();

//SHUTDOWN_RESOURCES_LABEL:
    // release resources
    Log_InfoPrint("Unloading resources...");
    //g_pResourceManager->ReleaseResources();
    g_pEngine->Shutdown();

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
    g_pRenderer->Shutdown();

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

static void CheckEvents()
{
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
                    if (SDL_GetWindowFromID(pEvent->window.windowID) != g_pOutputWindow->GetSDLWindow())
                        continue;

                    // handle the event
                    switch (pEvent->window.event)
                    {
                    case SDL_WINDOWEVENT_RESIZED:
                        //OnWindowResized(pEvent->window.data1, pEvent->window.data2);
                        //m_restartRendererFlag = true;
                        break;

                    case SDL_WINDOWEVENT_CLOSE:
                        g_quitFlag = true;
                        break;

                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        //OnWindowFocusGained();
                        break;

                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        //OnWindowFocusLost();
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
            }

            // append to the main thread's queue for later processing
            //m_pendingEvents.Add(*pEvent);
            ImGui::HandleSDLEvent(pEvent, true);
        }
    }
}

static Timer s_frameTimer;

static void Frame()
{
    float deltaTime = (float)s_frameTimer.GetTimeSeconds();
    s_frameTimer.Reset();

    // update stats
    g_fpsCounter.BeginFrame();
    CheckEvents();

    // imgui
    ImGui::NewFrame(deltaTime);

    // do stuff
    {
        g_pMainGPUContext->SetRenderTargets(0, nullptr, nullptr);
        g_pMainGPUContext->SetFullViewport();
        g_pMainGPUContext->ClearTargets(true, true, true, Vector4f(0.5f, 0.2f, 0.8f, 0.0f));

        MINIGUI_RECT rect(40, 140, 40, 140);
        g_guiContext.DrawFilledRect(&rect, MAKE_COLOR_R8G8B8_UNORM(255, 200, 150));
        rect.Set(200, 240, 40, 140);
        g_guiContext.DrawFilledRect(&rect, MAKE_COLOR_R8G8B8_UNORM(150, 255, 200));
        g_guiContext.DrawTextAt(40, 160, g_pRenderer->GetFixedResources()->GetDebugFont(), 16, MAKE_COLOR_R8G8B8_UNORM(255, 255, 255), "Hello world");

        g_guiContext.Flush();

        // draw imgui
        {
            ImGui::ShowTestWindow(nullptr);
        }

        // render imgui
        ImGui::Render();

        g_fpsCounter.DrawDetails(g_pRenderer->GetFixedResources()->GetDebugFont(), &g_guiContext);
        g_pMainGPUContext->PresentOutputBuffer(GPU_PRESENT_BEHAVIOUR_IMMEDIATE);
    }

    // end
    g_fpsCounter.EndGameThreadFrame();
}

static void Run()
{
#ifdef Y_PLATFORM_HTML5
    emscripten_set_main_loop(Frame, 0, true);
#else
    while (!g_quitFlag)
    {
        Frame();
    }
#endif
}


int main(int argc, char *argv[])
{
    // change gamename
    g_pConsole->SetCVarByName("vfs_gamedir", "TestGame", true);
    g_pConsole->ApplyPendingAppCVars();

    // set log flags
    Log::GetInstance().SetConsoleOutputParams(true);
    Log::GetInstance().SetDebugOutputParams(true);

    // parse command line
    g_pConsole->ParseCommandLine(argc, (const char **)argv);
    g_pConsole->ApplyPendingAppCVars();

    // force multithreaded rendering off
    g_pConsole->SetCVarByName("r_use_render_thread", "false");
    g_pConsole->ApplyPendingAppCVars();

    return (Boot(Run)) ? 0 : -1;
}
