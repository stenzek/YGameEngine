#include "DemoGame/PrecompiledHeader.h"
#include "DemoGame/DrawCallStressDemo.h"
#include "DemoGame/DemoUtilities.h"
#include "BaseGame/LoadingScreenProgressCallbacks.h"
#include "Engine/World.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/ImGuiBridge.h"
Log_SetChannel(SkeletalAnimationDemo);

DrawCallStressDemo::DrawCallStressDemo(DemoGame *pDemoGame)
    : BaseDemoWorldGameState(pDemoGame)
    , m_pSunLightEntity(nullptr)
    , m_sunLightRotation(float3::Zero)
    , m_pMeshToRender(nullptr)
    , m_newObjectCount(1000)
{
    
}

DrawCallStressDemo::~DrawCallStressDemo()
{
    SAFE_RELEASE(m_pMeshToRender);
    SAFE_RELEASE(m_pSunLightEntity);
}

bool DrawCallStressDemo::Initialize()
{
    if (!BaseDemoWorldGameState::Initialize())
        return false;

    if (!CreateDynamicWorld())
        return false;

    // load mesh to render
    m_pMeshToRender = g_pResourceManager->GetStaticMesh("models/engine/unit_sphere");
    if (m_pMeshToRender == nullptr)
        return false;

    // set model, this creates the renderable too
    LoadingScreenProgressCallbacks progressCallbacks;
    CreateObjects(m_newObjectCount, false, &progressCallbacks);

    // init view parameters
    m_viewParameters.MaximumShadowViewDistance = 500.0f;
    return true;
}

bool DrawCallStressDemo::OnWorldCreated(World *pWorld)
{
    if (!BaseDemoWorldGameState::OnWorldCreated(pWorld))
        return false;

    // sun entity
    //m_pSunLightEntity = DemoUtilities::CreateSunLight(m_pWorld, float3(1.0f, 1.0f, 1.0f), 1.2f, 0.4f);
    m_pSunLightEntity = DemoUtilities::CreateSunLight(m_pWorld, float3(1.0f, 0.992f, 0.8f), 1.2f, 0.4f);

    // ground plane
    AutoReleasePtr<const Material> pGroundPlaneMaterial = g_pResourceManager->GetDefaultMaterial();
    AutoReleasePtr<Entity> pGroundPlaneEntity = DemoUtilities::CreatePlaneShape(m_pWorld, float3::UnitZ, float3(0.0f, 0.0f, 0.0f), 100.0f, pGroundPlaneMaterial, 10.0f);

    // done
    return true;
}

void DrawCallStressDemo::Shutdown()
{
    while (!m_meshProxies.IsEmpty())
    {
        StaticMeshRenderProxy *pRenderProxy = m_meshProxies.LastElement();
        m_pWorld->GetRenderWorld()->RemoveRenderable(pRenderProxy);
        pRenderProxy->Release();
        m_meshProxies.PopBack();
    }

    BaseDemoWorldGameState::Shutdown();
}

void DrawCallStressDemo::OnWorldDeleted(World *pWorld)
{
    BaseDemoWorldGameState::OnWorldDeleted(pWorld);
}

void DrawCallStressDemo::OnMainThreadTick(float deltaTime)
{
    BaseDemoWorldGameState::OnMainThreadTick(deltaTime);

    if (m_sunLightRotation.SquaredLength() > 0.0f)
        m_pSunLightEntity->SetRotation((m_pSunLightEntity->GetRotation() * Quaternion::FromEulerAngles(m_sunLightRotation.x * deltaTime, m_sunLightRotation.y * deltaTime, 0.0f)).Normalize());
}

void DrawCallStressDemo::DrawOverlays(float deltaTime)
{
    BaseDemoWorldGameState::DrawOverlays(deltaTime);
}

void DrawCallStressDemo::DrawUI(float deltaTime)
{
    BaseDemoWorldGameState::DrawUI(deltaTime);

    ImGui::SetNextWindowPos(ImVec2(10, float(m_viewParameters.Viewport.Height - 250)), ImGuiSetCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320, 240), ImGuiSetCond_FirstUseEver);

    // annoyingly, everything is drawn on the UI thread yet commands have to be invoked on the main thread..
    if (ImGui::Begin("Draw Call Stress Demo"))
    {
        int intValue;
        //float floatValue;
        //bool boolValue;

        ImGui::PushItemWidth(-140.0f);

        // @TODO
        intValue = 0;
        if (ImGui::Combo("Mesh", &intValue, "models/engine/unit_sphere\0\0"))
        {
            QUEUE_MAIN_THREAD_LAMBDA_COMMAND([this, intValue]() {
                //SetModelIndex(intValue);
            });
        }

        // count
        ImGui::SliderInt("Object count", reinterpret_cast<int *>(&m_newObjectCount), 0, 20000);

        // update
        if (ImGui::Button("Update"))
        {
            QUEUE_MAIN_THREAD_LAMBDA_COMMAND([this]() {
                LoadingScreenProgressCallbacks callbacks;
                CreateObjects(m_newObjectCount, false, &callbacks);
            });
        }
        
        // @TODO model rotation/scale/etc

        ImGui::PopItemWidth();
    }
    ImGui::End();
}

bool DrawCallStressDemo::OnWindowEvent(const union SDL_Event *event)
{
    if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP)
    {
        bool isKeyDownEvent = (event->type == SDL_KEYDOWN);
        switch (event->key.keysym.sym)
        {
        case SDLK_LEFT:
            m_sunLightRotation.x += (isKeyDownEvent) ? -1.0f : 1.0f;
            return true;

        case SDLK_RIGHT:
            m_sunLightRotation.x += (isKeyDownEvent) ? 1.0f : -1.0f;
            return true;

        case SDLK_UP:
            m_sunLightRotation.y += (isKeyDownEvent) ? 1.0f : -1.0f;
            return true;

        case SDLK_DOWN:
            m_sunLightRotation.y += (isKeyDownEvent) ? -1.0f : 1.0f;
            return true;
        }
    }

    return BaseDemoWorldGameState::OnWindowEvent(event);
}

void DrawCallStressDemo::CreateObjects(uint32 count, bool useInstancing, ProgressCallbacks *pProgressCallbacks)
{
    const float SPACING = 1.5f;
    const float ZPOS = 1.0f;

    uint32 origCount = m_meshProxies.GetSize();
    if (origCount > 0)
    {
        pProgressCallbacks->SetStatusText("Removing existing objects...");
        pProgressCallbacks->SetProgressRange(m_meshProxies.GetSize());
        while (!m_meshProxies.IsEmpty())
        {
            StaticMeshRenderProxy *pRenderProxy = m_meshProxies.LastElement();
            m_pWorld->GetRenderWorld()->RemoveRenderable(pRenderProxy);
            pRenderProxy->Release();
            m_meshProxies.PopBack();

            pProgressCallbacks->SetProgressValue(origCount - m_meshProxies.GetSize());
        }
    }

    pProgressCallbacks->SetFormattedStatusText("Creating %u new objects...", count);
    pProgressCallbacks->SetProgressRange(count);
    pProgressCallbacks->SetProgressValue(0);

    uint32 rowsAndColumns = Math::Truncate(Math::Sqrt((float)count));
    for (uint32 row = 0; row < rowsAndColumns; row++)
    {
        float posX = (-(float(rowsAndColumns) / 2.0f) + (float)row) * SPACING;
        for (uint32 col = 0; col < rowsAndColumns; col++)
        {
            float posY = (-(float(rowsAndColumns) / 2.0f) + (float)col) * SPACING;
            Transform xform(Vector3f(posX, posY, ZPOS), Quaternion::Identity, Vector3f::One);

            StaticMeshRenderProxy *pRenderProxy = new StaticMeshRenderProxy(0, m_pMeshToRender, xform, ENTITY_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS | ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS);
            m_pWorld->GetRenderWorld()->AddRenderable(pRenderProxy);
            m_meshProxies.Add(pRenderProxy);

            pProgressCallbacks->IncrementProgressValue();
        }
    }
}

bool LaunchpadGameState::InvokeDrawCallStressDemo()
{
    return RunDemo(new DrawCallStressDemo(m_pDemoGame));
}
