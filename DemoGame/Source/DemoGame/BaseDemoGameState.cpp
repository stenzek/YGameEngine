#include "DemoGame/PrecompiledHeader.h"
#include "DemoGame/BaseDemoGameState.h"
#include "Engine/Engine.h"
#include "Engine/EngineCVars.h"
#include "Engine/InputManager.h"
#include "Engine/DynamicWorld.h"
#include "Renderer/ImGuiBridge.h"
Log_SetChannel(BaseDemoGameState);

BaseDemoGameState::BaseDemoGameState(DemoGame *pDemoGame)
    : m_pDemoGame(pDemoGame)
    , m_uiActive(true)
{

}

BaseDemoGameState::~BaseDemoGameState()
{

}

bool BaseDemoGameState::Initialize()
{
    bool result = false;
    QUEUE_BLOCKING_RENDERER_LAMBA_COMMAND([this, &result]() {
        result = CreateGPUResources();
    });

    if (!result)
        return false;

    // get window width/height
    //uint32 windowWidth = m_pDemoGame->GetOutputWindow()->GetWidth();
    //uint32 windowHeight = m_pDemoGame->GetOutputWindow()->GetHeight();
    return true;
}

void BaseDemoGameState::Shutdown()
{
    QUEUE_BLOCKING_RENDERER_LAMBA_COMMAND([this]() {
        ReleaseGPUResources();
    });
}

bool BaseDemoGameState::CreateGPUResources()
{
    return true;
}

void BaseDemoGameState::ReleaseGPUResources()
{

}

void BaseDemoGameState::DrawOverlays(float deltaTime)
{
    if (m_uiActive)
        DrawUI(deltaTime);
}

void BaseDemoGameState::DrawUI(float deltaTime)
{
    uint32 windowWidth = m_pDemoGame->GetOutputWindow()->GetWidth();
    //uint32 windowHeight = m_pDemoGame->GetOutputWindow()->GetHeight();

    ImGui::SetNextWindowPos(ImVec2(float(windowWidth - 125), 110));
    ImGui::SetNextWindowSize(ImVec2(115, 110));
    if (ImGui::Begin("Demo Controls", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        if (ImGui::Button("Toggle UI", ImVec2(100, 20)))
        {
            // call on main thread
            QUEUE_MAIN_THREAD_LAMBDA_COMMAND([this]() {
                ToggleUI();
            });
        }

        if (ImGui::Button("Render Debug", ImVec2(100, 20)))
        {
            // call on main thread
            QUEUE_MAIN_THREAD_LAMBDA_COMMAND([this]() {
                m_pDemoGame->OpenDebugRenderMenu();
            });
        }

        if (ImGui::Button("Exit Demo", ImVec2(100, 20)))
        {
            QUEUE_MAIN_THREAD_LAMBDA_COMMAND([this]() {
                m_pDemoGame->EndModalGameState();
            });
        }

    }
    ImGui::End();

    // light options?
}

void BaseDemoGameState::OnSwitchedIn()
{
    if (m_uiActive)
        m_pDemoGame->ActivateImGui();
}

void BaseDemoGameState::OnSwitchedOut()
{
    if (m_uiActive)
        m_pDemoGame->DeactivateImGui();
}

void BaseDemoGameState::OnBeforeDelete()
{

}

void BaseDemoGameState::OnWindowResized(uint32 width, uint32 height)
{

}

void BaseDemoGameState::OnWindowFocusGained()
{

}

void BaseDemoGameState::OnWindowFocusLost()
{

}

void BaseDemoGameState::ToggleUI()
{
    //DebugAssert(!Renderer::IsOnRenderThread()); @TODO IsOnMainThread
    if (m_uiActive)
    {
        m_pDemoGame->DeactivateImGui();
        m_uiActive = false;
        OnUIToggled(false);
    }
    else
    {
        OnUIToggled(true);
        m_pDemoGame->ActivateImGui();
        m_uiActive = true;
    }
}

void BaseDemoGameState::OnUIToggled(bool enabled)
{

}

bool BaseDemoGameState::OnWindowEvent(const union SDL_Event *event)
{
    if (!m_pDemoGame->IsImGuiActivated())
    {
        if (event->type == SDL_KEYUP && event->key.keysym.sym == SDLK_ESCAPE)
        {
            ToggleUI();
            return true;
        }
    }

    return false;
}

void BaseDemoGameState::OnMainThreadPreFrame(float deltaTime)
{
    
}

void BaseDemoGameState::OnMainThreadBeginFrame(float deltaTime)
{

}

void BaseDemoGameState::OnMainThreadAsyncTick(float deltaTime)
{

}

void BaseDemoGameState::OnMainThreadTick(float deltaTime)
{

}

void BaseDemoGameState::OnMainThreadEndFrame(float deltaTime)
{

}

void BaseDemoGameState::OnRenderThreadPreFrame(float deltaTime)
{
    
}

void BaseDemoGameState::OnRenderThreadBeginFrame(float deltaTime)
{

}

void BaseDemoGameState::OnRenderThreadDraw(float deltaTime)
{
    // most will override this

    // draw overlays
    m_pDemoGame->GetGUIContext()->PushManualFlush();
    DrawOverlays(deltaTime);
    m_pDemoGame->GetGUIContext()->PopManualFlush();
}

void BaseDemoGameState::OnRenderThreadEndFrame(float deltaTime)
{

}
