#include "DemoGame/PrecompiledHeader.h"
#include "DemoGame/BaseDemoWorldGameState.h"
#include "Engine/Engine.h"
#include "Engine/EngineCVars.h"
#include "Engine/InputManager.h"
#include "Engine/DynamicWorld.h"
#include "Renderer/ImGuiBridge.h"
Log_SetChannel(BaseDemoWorldGameState);

BaseDemoWorldGameState::BaseDemoWorldGameState(DemoGame *pDemoGame)
    : BaseDemoGameState(pDemoGame)
    , m_pWorld(nullptr)
    , m_dynamicWorldFlag(false)
{

}

BaseDemoWorldGameState::~BaseDemoWorldGameState()
{

}

bool BaseDemoWorldGameState::Initialize()
{
    if (!BaseDemoGameState::Initialize())
        return false;

    // get window width/height
    uint32 windowWidth = m_pDemoGame->GetOutputWindow()->GetWidth();
    uint32 windowHeight = m_pDemoGame->GetOutputWindow()->GetHeight();

    // initialize camera
    m_camera.SetProjectionType(CAMERA_PROJECTION_TYPE_PERSPECTIVE);
    m_camera.SetNearFarPlaneDistances(0.1f, 1000.0f);
    m_camera.SetObjectCullDistanceFromNearFar();
    m_camera.SetClippingEnabled(false);
    m_camera.SetPerspectiveAspect((float)windowWidth, (float)windowHeight);

    // initialize view parameters
    m_viewParameters.Viewport.Set(0, 0, windowWidth, windowHeight, 0.0f, 1.0f);
    m_viewParameters.MaximumShadowViewDistance = m_camera.GetObjectCullDistance();
    m_viewParameters.SetCamera(&m_camera);
    return true;
}

void BaseDemoWorldGameState::Shutdown()
{
    if (m_dynamicWorldFlag)
    {
        World *pWorld = m_pWorld;
        OnWorldDeleted(pWorld);
        delete pWorld;
        m_pWorld = nullptr;
    }

    BaseDemoGameState::Shutdown();
}

bool BaseDemoWorldGameState::CreateGPUResources()
{
    if (!BaseDemoGameState::CreateGPUResources())
        return false;

    return true;
}

void BaseDemoWorldGameState::ReleaseGPUResources()
{
    BaseDemoGameState::ReleaseGPUResources();
}

bool BaseDemoWorldGameState::OnWorldCreated(World *pWorld)
{
    m_camera.SetWorld(pWorld);
    m_pWorld->AddObserver(&m_camera, m_camera.GetPosition());
    return true;
}

void BaseDemoWorldGameState::OnWorldDeleted(World *pWorld)
{
    m_camera.SetWorld(nullptr);
}

void BaseDemoWorldGameState::DrawOverlays(float deltaTime)
{
    BaseDemoGameState::DrawOverlays(deltaTime);

    MiniGUIContext *guiContext = m_pDemoGame->GetGUIContext();

    // draw camera position/rotation
    if (!m_uiActive)
    {
        guiContext->DrawFormattedTextAt(10, 26, g_pRenderer->GetFixedResources()->GetDebugFont(), 16, MAKE_COLOR_R8G8B8_UNORM(255, 255, 255), "Camera Position: %s", StringConverter::Float3ToString(m_viewParameters.ViewCamera.GetPosition()).GetCharArray());
        guiContext->DrawFormattedTextAt(10, 42, g_pRenderer->GetFixedResources()->GetDebugFont(), 16, MAKE_COLOR_R8G8B8_UNORM(255, 255, 255), "Camera Direction: %s", StringConverter::Float3ToString(m_viewParameters.ViewCamera.GetRotation() * float3::UnitY).GetCharArray());
        guiContext->DrawFormattedTextAt(10, 10, g_pRenderer->GetFixedResources()->GetDebugFont(), 16, MAKE_COLOR_R8G8B8_UNORM(255, 255, 255), "UI hidden, use WSAD to move camera, ESC to re-open UI");
    }
}

void BaseDemoWorldGameState::DrawUI(float deltaTime)
{
    BaseDemoGameState::DrawUI(deltaTime);

    // light options?
}

void BaseDemoWorldGameState::OnSwitchedIn()
{
    BaseDemoGameState::OnSwitchedIn();

    if (!m_uiActive)
        m_pDemoGame->SetRelativeMouseMovement(true);
}

void BaseDemoWorldGameState::OnSwitchedOut()
{
    BaseDemoGameState::OnSwitchedOut();

    if (!m_uiActive)
        m_pDemoGame->SetRelativeMouseMovement(false);
}

void BaseDemoWorldGameState::OnUIToggled(bool enabled)
{
    BaseDemoGameState::OnUIToggled(enabled);

    if (enabled)
        m_pDemoGame->SetRelativeMouseMovement(false);
    else
        m_pDemoGame->SetRelativeMouseMovement(true);
}

void BaseDemoWorldGameState::OnBeforeDelete()
{
    BaseDemoGameState::OnBeforeDelete();
}

void BaseDemoWorldGameState::OnWindowResized(uint32 width, uint32 height)
{
    BaseDemoGameState::OnWindowResized(width, height);

    m_viewParameters.Viewport.Set(0, 0, width, height, 0.0f, 1.0f);
    m_camera.SetPerspectiveAspect((float)width, (float)height);
}

void BaseDemoWorldGameState::OnWindowFocusGained()
{
    BaseDemoGameState::OnWindowFocusGained();
}

void BaseDemoWorldGameState::OnWindowFocusLost()
{
    BaseDemoGameState::OnWindowFocusLost();
}

bool BaseDemoWorldGameState::OnWindowEvent(const union SDL_Event *event)
{
    if (BaseDemoGameState::OnWindowEvent(event))
        return true;

    // only pass to camera if gui isn't activated
    if (!m_pDemoGame->IsImGuiActivated())
        return m_camera.HandleSDLEvent(event);

    return false;
}

void BaseDemoWorldGameState::OnMainThreadPreFrame(float deltaTime)
{
    BaseDemoGameState::OnMainThreadPreFrame(deltaTime);

    // update camera
    m_camera.Update(deltaTime);

    // update view parameters
    m_viewParameters.WorldTime = m_pWorld->GetGameTime();
    m_viewParameters.ViewCamera = m_camera;
}

void BaseDemoWorldGameState::OnMainThreadBeginFrame(float deltaTime)
{
    BaseDemoGameState::OnMainThreadBeginFrame(deltaTime);

    // update observer before the frame, that way the frame can ensure the correct stuff is loaded before it executes
    m_pWorld->UpdateObserver(&m_camera, m_camera.GetPosition());
    m_pWorld->BeginFrame(deltaTime);
}

void BaseDemoWorldGameState::OnMainThreadAsyncTick(float deltaTime)
{
    BaseDemoGameState::OnMainThreadAsyncTick(deltaTime);

    m_pWorld->UpdateAsync(deltaTime);
}

void BaseDemoWorldGameState::OnMainThreadTick(float deltaTime)
{
    BaseDemoGameState::OnMainThreadTick(deltaTime);

    m_pWorld->Update(deltaTime);
}

void BaseDemoWorldGameState::OnMainThreadEndFrame(float deltaTime)
{
    BaseDemoGameState::OnMainThreadEndFrame(deltaTime);

    m_pWorld->EndFrame();
}

void BaseDemoWorldGameState::OnRenderThreadPreFrame(float deltaTime)
{
    BaseDemoGameState::OnRenderThreadPreFrame(deltaTime);

    // copy camera from main thread to render thread before the main thread changes it
    m_viewParameters.SetCamera(&m_camera);
}

void BaseDemoWorldGameState::OnRenderThreadBeginFrame(float deltaTime)
{
    BaseDemoGameState::OnRenderThreadPreFrame(deltaTime);
}

void BaseDemoWorldGameState::OnRenderThreadDraw(float deltaTime)
{
    // draw world
    m_pDemoGame->GetWorldRenderer()->DrawWorld(m_pWorld->GetRenderWorld(), &m_viewParameters, nullptr, nullptr, m_pDemoGame->GetRenderProfiler());

    // setup overlay draws
    m_pDemoGame->GetGPUContext()->SetRenderTargets(0, nullptr, nullptr);
    m_pDemoGame->GetGPUContext()->SetFullViewport();
    m_pDemoGame->GetGPUContext()->GetConstants()->SetFromCamera(m_viewParameters.ViewCamera, false);
    m_pDemoGame->GetGPUContext()->GetConstants()->CommitChanges();

    // draw overlays
    m_pDemoGame->GetGUIContext()->PushManualFlush();
    DrawOverlays(deltaTime);
    m_pDemoGame->GetGUIContext()->PopManualFlush();
}

void BaseDemoWorldGameState::OnRenderThreadEndFrame(float deltaTime)
{
    BaseDemoGameState::OnRenderThreadEndFrame(deltaTime);
}

bool BaseDemoWorldGameState::CreateDynamicWorld()
{
    DynamicWorld *pWorld = new DynamicWorld();
    m_pWorld = pWorld;
    m_dynamicWorldFlag = true;

    if (!OnWorldCreated(pWorld))
    {
        m_pWorld = nullptr;
        delete pWorld;
        return false;
    }

    return true;
}

