#include "Editor/PrecompiledHeader.h"
#include "Editor/Editor.h"
#include "Editor/EditorCVars.h"
#include "Editor/EditorVisual.h"
#include "Editor/MapEditor/EditorMapWindow.h"
#include "Engine/ResourceManager.h"
#include "Engine/Font.h"
#include "Engine/Entity.h"
#include "Engine/EntityTypeInfo.h"
#include "Engine/SDLHeaders.h"
#include "Renderer/Renderer.h"
#include "ResourceCompiler/ObjectTemplate.h"
#include "ResourceCompiler/ObjectTemplateManager.h"
#include "YBaseLib/CPUID.h"
Log_SetChannel(Editor);

Editor *g_pEditor = NULL;

extern void RunEditorTestBedsPreStartup();
extern void RunEditorTestBeds();

static uint32 CalculateFrameTimerInterval()
{
    float maxFps = CVars::e_max_fps.GetFloat();
    int32 timerKickInterval = Math::Truncate(Math::Floor(1000.0f / maxFps));
    return (uint32)timerKickInterval;
}

Editor::Editor(int &argc, char **argv)
    : QApplication(argc, argv),
      m_running(true),
      m_pFrameExecuteTimer(NULL),
      m_blockFrameExecution(0),
      m_frameExecutionQueued(false),
      m_editorFPSAccumulator(0),
      m_editorFPSTimeAccumulator(0.0f),
      m_editorFPS(0.0f),
      m_editorTheme(EDITOR_THEME_NONE),
      m_pViewportOverlayFont(NULL)
{
    m_pCommandLineArguments = (const char **)argv;
    m_nCommandLineArguments = argc;

    m_pFrameExecuteTimer = new QTimer(this);

    g_pEditor = this;
}

Editor::~Editor()
{
    DebugAssert(m_blockFrameExecution == 0);
    DebugAssert(m_mapWindows.GetSize() == 0);
    
    g_pEditor = NULL;
}

static void SetCVars()
{
    // set vars
    //g_pConsole->SetCVarByName("e_threadpool_worker_count", "1");
    g_pConsole->SetCVarByName("e_camera_max_speed", "6");
    g_pConsole->SetCVarByName("e_camera_acceleration", "12");

    //g_pConsole->SetCVarByName("r_use_shader_caching", "1");
    g_pConsole->SetCVarByName("r_use_shader_caching", "0");
    g_pConsole->SetCVarByName("r_allow_shader_cache_writes", "1");

#if Y_BUILD_CONFIG_DEBUG
    g_pConsole->SetCVarByName("r_use_debug_shaders", "1");
#endif
}

int Editor::Execute()
{
    // sanity recursion check
    static bool executed = false;
    Assert(!executed);
    executed = true;

    // initialize sdl1
    Log_InfoPrint("Initializing SDL...");
    SDL_SetMainReady();
    if (SDL_Init(0) != 0)
    {
        Log_ErrorPrintf("Editor::Execute: SDL_Init failed: %s", SDL_GetError());
        return -1;
    }

    // clean up SDL last
    atexit(SDL_Quit);

    // startup
    {
        Timer startupTimer;
    
        // <<<testbeds>>>
        RunEditorTestBedsPreStartup();

        // initialize
        if (!Initialize())
            return -2;

        // startup
        if (!Startup())
            return -3;

        // <<<testbeds>>>
        RunEditorTestBeds();

        // log startup time
        Log_DevPrintf("<<<Editor started in %.4f msec>>>", startupTimer.GetTimeMilliseconds());
    }

    // pass control over to qt
    int qtReturnCode = QApplication::exec();

    // execute shutdown routines
    Shutdown();

    // return code
    return qtReturnCode;
}

void Editor::Exit()
{
    static bool exited = false;
    if (exited)
        return;
    
    exited = true;
    
    // invoke this method next event loop
    //QMetaObject::invokeMethod(this, SLOT(quit()), Qt::QueuedConnection);
    quit();
}

bool Editor::Initialize()
{
    // set cvars
    SetCVars();

    // pass command line to console
    g_pConsole->ParseCommandLine(m_nCommandLineArguments, m_pCommandLineArguments);

    // print version info
    {
        Y_CPUID_RESULT CPUIDResult;
        Y_ReadCPUID(&CPUIDResult);

        Log_InfoPrint("=========== Editor Version 1.0 Initializing ===========");
        Log_DevPrint("Build Configuration: " Y_BUILD_CONFIG_STR);
        Log_DevPrint("Host Platform: " Y_PLATFORM_STR);
        Log_DevPrint("Host Architecture: " Y_CPU_STR Y_CPU_FEATURES_STR);
        Log_DevPrintf("Host CPU: %s", CPUIDResult.SummaryString);
    }

    Log_InfoPrint("Initializing virtual file system...");
    if (!g_pVirtualFileSystem->Initialize())
    {
        Log_InfoPrint("Virtual file system initialization failed. Cannot continue.");
        return false;
    }

    // add types
    g_pEngine->RegisterEngineTypes();
    RegisterEditorTypes();

    // load property templates
    Log_InfoPrint("Loading object templates...");
    if (!LoadObjectTemplates())
        return false;

    // load entity definitions
    Log_InfoPrint("Loading visual definitions...");
    if (!LoadVisualDefinitions())
        return false;

    // done
    return true;
}

bool Editor::Startup()
{
    // begin startup
    Log_InfoPrint("[Editor] Starting up...");

    // startup engine
    if (!g_pEngine->Startup())
        return false;

    // load icons
    if (!m_iconLibrary.PreloadIcons())
        return false;

    // get viewport overlay font
    m_pViewportOverlayFont = g_pResourceManager->GetFont(g_pEngine->GetRendererDebugFontName());
    if (m_pViewportOverlayFont == NULL)
    {
        Log_ErrorPrintf("Could not load viewport overlay font.");
        return false;
    }

    // create renderer
    RendererInitializationParameters createParameters;
    createParameters.EnableThreadedRendering = false;
    createParameters.HideImplicitSwapChain = true;
    if (!Renderer::Create(&createParameters))
        return false;

    // create the initial map window
    Log_InfoPrint("[Editor] Creating initial map window...");
    EditorMapWindow *pMapWindow = new EditorMapWindow();
    pMapWindow->NewMap();
    pMapWindow->show();

    // start the frame timer
    connect(m_pFrameExecuteTimer, SIGNAL(timeout()), this, SLOT(OnFrameExecuteTimerTriggered()));
    m_pFrameExecuteTimer->start(CalculateFrameTimerInterval());

    // connect the 'last window closed' event to the quit slot
    connect(this, SIGNAL(lastWindowClosed()), this, SLOT(quit()));

    // done
    return true;
}

void Editor::Shutdown()
{
    // block frame execution
    BlockFrameExecution();

    // stop frame execution timer
    m_pFrameExecuteTimer->stop();

    // close all map windows
    CloseAllMapWindows();

    // kill remaining main windows
    while (m_mainWindows.GetSize() > 0)
    {
        m_mainWindows[0]->close();
        delete m_mainWindows[0];
    }

    // kill resources
    m_pViewportOverlayFont->Release();

    // kill definitions
    for (VisualDefinitionHashTable::Iterator itr = m_visualDefinitions.Begin(); !itr.AtEnd(); itr.Forward())
        delete itr->Value;
    m_visualDefinitions.Clear();

    // shutdown subsystems
    g_pRenderer->Shutdown();
    g_pEngine->Shutdown();

    // shutdown VFS layer
    g_pVirtualFileSystem->Shutdown();

    // unregister types
    g_pEngine->UnregisterTypes();

    // unblock again
    UnblockFrameExecution();
}

void Editor::SetEditorTheme(EDITOR_THEME theme)
{
    if (theme == m_editorTheme)
        return;

    m_editorTheme = theme;

    if (theme == EDITOR_THEME_NONE)
    {
        setStyleSheet(QStringLiteral(""));
    }
    else if (theme == EDITOR_THEME_DARK)
    {
        QFile file(":qdarkstyle/qdarkstylesheet.qss");
        if (file.open(QFile::ReadOnly))
            setStyleSheet(QLatin1String(file.readAll()));
    }
    else if (theme == EDITOR_THEME_DARK_OTHER)
    {
        QFile file(":/editor/style_dark/style.css");
        if (file.open(QFile::ReadOnly))
            setStyleSheet(QLatin1String(file.readAll()));
    }
}

EditorMapWindow *Editor::CreateMap()
{
    BlockFrameExecution();

    EditorMapWindow *pMapWindow = new EditorMapWindow();
    pMapWindow->show();

    UnblockFrameExecution();
    return pMapWindow;
}

EditorMapWindow *Editor::OpenMap(const char *mapFileName)
{
    BlockFrameExecution();

    EditorMapWindow *pMapWindow = new EditorMapWindow();
    if (!pMapWindow->OpenMap(mapFileName))
    {
        UnblockFrameExecution();
        delete pMapWindow;
        return NULL;
    }

    pMapWindow->show();

    UnblockFrameExecution();
    return pMapWindow;
}

void Editor::AddMapWindow(EditorMapWindow *pMapWindow)
{
    m_mapWindows.Add(pMapWindow);
}

void Editor::RemoveMapWindow(EditorMapWindow *pMapWindow)
{
    for (uint32 i = 0; i < m_mapWindows.GetSize(); i++)
    {
        if (m_mapWindows[i] == pMapWindow)
        {
            m_mapWindows.OrderedRemove(i);
            return;
        }
    }

    Panic("Attempting to remove untracked map window");
}

void Editor::CloseAllMapWindows()
{
    // block frame execution
    BlockFrameExecution();

    // close map windows, may trigger a modal dialog box
    while (m_mapWindows.GetSize() > 0)
    {
        m_mapWindows[0]->close();
        delete m_mapWindows[0];
    }

    // unblock frame execution
    UnblockFrameExecution();
}

void Editor::AddMainWindow(QMainWindow *pMainWindow)
{
    m_mainWindows.Add(pMainWindow);
}

void Editor::RemoveMainWindow(QMainWindow *pMainWindow)
{
    for (uint32 i = 0; i < m_mainWindows.GetSize(); i++)
    {
        if (m_mainWindows[i] == pMainWindow)
        {
            m_mainWindows.OrderedRemove(i);
            return;
        }
    }

    Panic("Attempting to remove untracked main window");
}

const EditorVisualDefinition *Editor::GetVisualDefinitionByName(const char *typeName) const
{
    const VisualDefinitionHashTable::Member *pMember = m_visualDefinitions.Find(typeName);
    return (pMember != nullptr) ? pMember->Value : nullptr;
}

const EditorVisualDefinition *Editor::GetVisualDefinitionForObjectTemplate(const ObjectTemplate *pTemplate) const
{
    const VisualDefinitionHashTable::Member *pMember = m_visualDefinitions.Find(pTemplate->GetTypeName());
    return (pMember != nullptr) ? pMember->Value : nullptr;
}

bool Editor::LoadObjectTemplates()
{
    if (!ObjectTemplateManager::GetInstance().LoadAllTemplates())
    {
        Log_ErrorPrintf("Editor::LoadObjectTemplates: ObjectTemplateManager::LoadAllTemplates failed.");
        return false;
    }

    // search for some common templates that should always be present
#define CHECK_OBJECT_TEMPLATE(name) MULTI_STATEMENT_MACRO_BEGIN \
                                        if (ObjectTemplateManager::GetInstance().GetObjectTemplate(name) == nullptr) { \
                                            Log_ErrorPrintf("Editor::LoadObjectTemplates: Missing template for required type '%s'", name); \
                                            return false; \
                                        } \
                                    MULTI_STATEMENT_MACRO_END

    CHECK_OBJECT_TEMPLATE("Brush");
    CHECK_OBJECT_TEMPLATE("Entity");
    CHECK_OBJECT_TEMPLATE("Map");

#undef CHECK_OBJECT_TEMPLATE

    return true;
}

bool Editor::LoadVisualDefinitions()
{
    static const char *VISUAL_BASE_PATH = "resources/engine/editor_visuals";

    // load definitions for all known object templates first
    ObjectTemplateManager::GetInstance().EnumerateObjectTemplates([this](const ObjectTemplate *pTemplate)
    {
        PathString fileName;
        fileName.Format("%s/%s.xml", VISUAL_BASE_PATH, pTemplate->GetTypeName().GetCharArray());

        // skip nonexistant visuals
        if (!g_pVirtualFileSystem->FileExists(fileName))
            return;
        
        // create it
        EditorVisualDefinition *pDefinition = new EditorVisualDefinition();
        if (!pDefinition->CreateFromTemplate(pTemplate))
        {
            Log_WarningPrintf("Editor::LoadVisualDefinitions: Failed to load visual definition for object type '%s'", pTemplate->GetTypeName().GetCharArray());
            delete pDefinition;
            return;
        }

        // add to hash map
        DebugAssert(m_visualDefinitions.Find(pDefinition->GetName()) == nullptr);
        m_visualDefinitions.Insert(pDefinition->GetName(), pDefinition);
    });

    // search for remaining templates
    FileSystem::FindResultsArray findResults;
    g_pVirtualFileSystem->FindFiles("resources/engine/editor_visuals", "*.xml", FILESYSTEM_FIND_FILES | FILESYSTEM_FIND_RELATIVE_PATHS, &findResults);

    // go through find results
    for (uint32 i = 0; i < findResults.GetSize(); i++)
    {
        FILESYSTEM_FIND_DATA &findData = findResults[i];

        // strip the xml extension
        SmallString typeName;
        typeName.AppendString(findData.FileName);
        if (!typeName.EndsWith(".xml", false))
            continue;
        typeName.Erase(-4);

        // already exists? and skip anything that's an object
        if (m_visualDefinitions.Find(typeName) != nullptr || ObjectTemplateManager::GetInstance().GetObjectTemplate(typeName) != nullptr)
            continue;

        // create it
        EditorVisualDefinition *pDefinition = new EditorVisualDefinition();
        if (!pDefinition->CreateFromName(typeName))
        {
            Log_WarningPrintf("Editor::LoadVisualDefinitions: Failed to load visual definition for object name '%s'", typeName.GetCharArray());
            delete pDefinition;
            continue;
        }

        // add to hash map
        DebugAssert(m_visualDefinitions.Find(pDefinition->GetName()) == nullptr);
        m_visualDefinitions.Insert(pDefinition->GetName(), pDefinition);
    }

    Log_InfoPrintf("Editor::LoadVisualDefinitions: Loaded %u visuals.", m_visualDefinitions.GetMemberCount());
    return true;
}

void Editor::QueueFrameExecution()
{
    if (!m_frameExecutionQueued)
    {
        //QTimer::singleShot(0, g_pEditor, &Editor::OnFrameExecuteTimerTriggered);
        m_frameExecutionQueued = true;
    }
}

void Editor::OnFrameExecuteTimerTriggered()
{
    if (m_blockFrameExecution > 0)
        return;

    // prevent any recursive calls (i.e. modal dialogs)
    m_blockFrameExecution++;

    // allow another frame to be immediately queued
    m_frameExecutionQueued = false;

    // reset the frame timer
    float timeSinceLastFrame = (float)m_lastFrameTime.GetTimeSeconds();
    m_lastFrameTime.Reset();

    // run any window's frame stuff
    OnFrameExecution(timeSinceLastFrame);

    // run any render commands queued from other threads
    g_pRenderer->GetCommandQueue()->ExecuteQueuedTasks();

    // estimate the current fps
    //m_editorFPS = (float)(1000.0 / m_lastFrameTime.GetTimeMilliseconds());
    m_editorFPSTimeAccumulator += (float)timeSinceLastFrame;
    m_editorFPSAccumulator++;
    if (m_editorFPSTimeAccumulator > 0.1f)
    {
        m_editorFPS = (float)m_editorFPSAccumulator / m_editorFPSTimeAccumulator;
        m_editorFPSTimeAccumulator = 0.0f;
        m_editorFPSAccumulator = 0;
    }

    // other end of recursive call block
    m_blockFrameExecution--;
}

void Editor::OnPendingCloseTriggered()
{

}

void Editor::ProcessBackgroundEvents()
{
    BlockFrameExecution();

    QGuiApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    UnblockFrameExecution();
}
