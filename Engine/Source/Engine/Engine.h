#pragma once
#include "Engine/Common.h"
#include "Engine/CommandQueue.h"
#include "Core/RandomNumberGenerator.h"
#include "YBaseLib/TaskQueue.h"

class Font;

class Engine
{
public:
    Engine();
    ~Engine();

    // default name accessors
    const String &GetDefaultTextureName(TEXTURE_TYPE TextureType) const;
    const String &GetDefaultTexture2DName() const { return m_strDefaultTexture2DName; }
    const String &GetDefaultTexture2DArrayName() const { return m_strDefaultTexture2DArrayName; }
    const String &GetDefaultTextureCubeName() const { return m_strDefaultTextureCubeName; }
    const String &GetDefaultMaterialShaderName() const { return m_strDefaultMaterialShaderName; }
    const String &GetDefaultMaterialName() const { return m_strDefaultMaterialName; }
    const String &GetDefaultFontName() const { return m_strDefaultFontName; }
    const String &GetDefaultStaticMeshName() const { return m_strDefaultStaticMeshName; }
    const String &GetDefaultBlockMeshName() const { return m_strDefaultBlockMeshName; }
    const String &GetDefaultSkeletalMeshName() const { return m_strDefaultSkeletalMeshName; }
    
    // fixed name accessors
    const String &GetSpriteMaterialShaderName() const { return m_strSpriteMaterialShaderName; }
    const String &GetRendererDebugFontName() const { return m_strRendererDebugFontName; }

    // default resource accessors. these DO increment the reference count upon retrieval.
    const Font *GetDefaultFont() const;

    // fixed resource accessors. these DO NOT increment the reference count upon retrieval, as they are assumed to be persistent.

    // Registers classes associated with the engine library
    virtual void RegisterEngineTypes();

    // Unregister all classes.
    virtual void UnregisterTypes();

    // initializes the remaining layers and calls application initialization.
    virtual bool Startup();

    // shuts down all systems
    virtual void Shutdown();

    // command queue access
    TaskQueue *GetMainThreadCommandQueue() { return &m_mainThreadCommandQueue; }
    TaskQueue *GetAsyncCommandQueue() { return &m_asyncCommandQueue; }
    TaskQueue *GetBackgroundCommandQueue() { return &m_backgroundCommandQueue; }

    // game thread random number generator
    // can only be accessed from game thread!
    RandomNumberGenerator *GetRandomNumberGenerator() { return &m_randomNumberGenerator; }

private:
    // registers internal types
    void RegisterEngineResourceTypes();
    void RegisterEngineComponentTypes();
    void RegisterEngineEntityTypes();
    void RegisterExternalTypes();

    // default resource names
    String m_strDefaultTexture2DName;
    String m_strDefaultTexture2DArrayName;
    String m_strDefaultTextureCubeName;
    String m_strDefaultMaterialShaderName;
    String m_strDefaultMaterialName;
    String m_strDefaultFontName;
    String m_strDefaultStaticMeshName;
    String m_strDefaultBlockMeshName;
    String m_strDefaultSkeletalMeshName;

    // fixed resource names
    String m_strSpriteMaterialShaderName;
    String m_strRendererDebugFontName;

    // default resources
    mutable const Font *m_pDefaultFont;

    // fixed resources

    // worker thread pool
    ThreadPool *m_pWorkerThreadPool;

    // main thread command queue
    TaskQueue m_mainThreadCommandQueue;

    // async command queue
    TaskQueue m_asyncCommandQueue;

    // background command queue
    TaskQueue m_backgroundCommandQueue;

    // random number generator
    RandomNumberGenerator m_randomNumberGenerator;
};

extern Engine *g_pEngine;

// command queue helper macros
#define QUEUE_MAIN_THREAD_COMMAND(obj) g_pEngine->GetMainThreadCommandQueue()->QueueTask(&obj, sizeof(obj))
#define QUEUE_MAIN_THREAD_LAMBDA_COMMAND g_pEngine->GetMainThreadCommandQueue()->QueueLambdaTask
#define QUEUE_ASYNC_COMMAND(obj) g_pEngine->GetAsyncCommandQueue()->QueueTask(&obj, sizeof(obj))
#define QUEUE_ASYNC_LAMBDA_COMMAND g_pEngine->GetAsyncCommandQueue()->QueueLambdaTask
#define QUEUE_BACKGROUND_COMMAND(obj) g_pEngine->GetBackgroundCommandQueue()->QueueTask(&obj, sizeof(obj))
#define QUEUE_BACKGROUND_LAMBDA_COMMAND g_pEngine->GetBackgroundCommandQueue()->QueueLambdaTask
