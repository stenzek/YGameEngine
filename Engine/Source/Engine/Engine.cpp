#include "Engine/PrecompiledHeader.h"
#include "Engine/Engine.h"
#include "Engine/ResourceManager.h"
#include "Engine/Font.h"
#include "Engine/EngineCVars.h"
#include "Renderer/Renderer.h"
#include "YBaseLib/CPUID.h"
Log_SetChannel(Engine);

// instance of engine is created in file
static Engine s_Engine;
Engine *g_pEngine = &s_Engine;

Engine::Engine()
{
    // set to defaults, replace with config at some point
    m_strDefaultTexture2DName = "textures/engine/default";
    m_strDefaultTexture2DArrayName = "textures/engine/default_array";
    m_strDefaultTextureCubeName = "textures/engine/default_cube";
    m_strDefaultMaterialShaderName = "materials/engine/default";
    m_strDefaultMaterialName = "materials/engine/default";
    m_strDefaultFontName = "resources/engine/fonts/small_font";
    m_strDefaultStaticMeshName = "models/engine/unit_cube";
    m_strDefaultBlockMeshName = "models/engine/single_block";
    m_strDefaultSkeletalMeshName = "models/engine/default_skeletal_mesh";

    m_strSpriteMaterialShaderName = "shaders/engine/simple_sprite";
    m_strRendererDebugFontName = "resources/engine/fonts/fixedsyse_16";

    // default resources
    m_pDefaultFont = nullptr;

    // fixed resources
    m_pWorkerThreadPool = nullptr;
}

Engine::~Engine()
{
    DebugAssert(m_pWorkerThreadPool == nullptr);
}

const String &Engine::GetDefaultTextureName(TEXTURE_TYPE TextureType) const
{
    switch (TextureType)
    {
    case TEXTURE_TYPE_2D:
        return GetDefaultTexture2DName();

    case TEXTURE_TYPE_CUBE:
        return GetDefaultTextureCubeName();
    }

    UnreachableCode();
    return EmptyString;
}

bool Engine::Startup()
{
    // create threadpool
    int32 workerThreadCount = CVars::e_worker_threads.GetInt();
    if (workerThreadCount < 0)
    {
        // create NTHREADS - 2 (main thread, render thread)
        Y_CPUID_RESULT cpuidResult;
        Y_ReadCPUID(&cpuidResult);
        workerThreadCount = (cpuidResult.ThreadCount < 3) ? 1 : (int32)cpuidResult.ThreadCount - 2;
    }
    
    // HTML5 has no threads.
#ifdef Y_PLATFORM_HTML5
    workerThreadCount = 0;
#endif

    if (workerThreadCount > 0)
    {
        Log_InfoPrintf("Creating %u worker threads...", workerThreadCount);

        // create threadpool
        m_pWorkerThreadPool = new ThreadPool(workerThreadCount);

        // create async command queue
        if (!m_asyncCommandQueue.Initialize(m_pWorkerThreadPool, CommandQueue::DEFAULT_COMMAND_QUEUE_SIZE))
        {
            Log_ErrorPrintf("Engine::Startup: Failed to initialize async command queue.");
            return false;
        }

        // create background command queue
        if (!m_backgroundCommandQueue.Initialize(m_pWorkerThreadPool, CommandQueue::DEFAULT_COMMAND_QUEUE_SIZE, true))
        {
            Log_ErrorPrintf("Engine::Startup: Failed to initialize background command queue.");
            return false;
        }

        // create main thread command queue
        if (!m_mainThreadCommandQueue.Initialize(CommandQueue::DEFAULT_COMMAND_QUEUE_SIZE, 0))
        {
            Log_ErrorPrintf("Engine::Startup: Failed to initialize main thread command queue.");
            return false;
        }
    }
    else
    {
        Log_InfoPrintf("Engine::Startup: Not using worker threads.");

        // create async command queue
        if (!m_asyncCommandQueue.Initialize(CommandQueue::DEFAULT_COMMAND_QUEUE_SIZE, 0))
        {
            Log_ErrorPrintf("Engine::Startup: Failed to initialize async command queue.");
            return false;
        }

        // create background command queue
        if (!m_backgroundCommandQueue.Initialize((uint32)0, 0))     // not using queue since it's not called back -- fix this
        {
            Log_ErrorPrintf("Engine::Startup: Failed to initialize background command queue.");
            return false;
        }

        // create main thread command queue
        if (!m_mainThreadCommandQueue.Initialize(CommandQueue::DEFAULT_COMMAND_QUEUE_SIZE, 0))
        {
            Log_ErrorPrintf("Engine::Startup: Failed to initialize main thread command queue.");
            return false;
        }
    }

    // done
    return true;
}

void Engine::Shutdown()
{
    // Shutdown async workers, run any remaining callbacks
    for (;;)
    {
        bool result;
        result = m_asyncCommandQueue.ExecuteQueuedTasks();
        result |= m_backgroundCommandQueue.ExecuteQueuedTasks();
        result |= m_mainThreadCommandQueue.ExecuteQueuedTasks();
        if (result)
            continue;
        else
            break;
    }

    // Exit worker threads
    delete m_pWorkerThreadPool;
    m_pWorkerThreadPool = nullptr;

    // Release fixed resources
    

    // Release default resources
    SAFE_RELEASE(m_pDefaultFont);

    // Tell resource manager to resource all managed resources.
    g_pResourceManager->ReleaseResources();
}

const Font *Engine::GetDefaultFont() const
{
    if (m_pDefaultFont == NULL && (m_pDefaultFont = g_pResourceManager->GetFont(m_strDefaultFontName)) == NULL)
        Panic("GetDefaultFont() called, and the default font failed to load.");

    m_pDefaultFont->AddRef();
    return m_pDefaultFont;
}
