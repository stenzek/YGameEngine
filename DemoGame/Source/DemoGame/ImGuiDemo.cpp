#include "DemoGame/PrecompiledHeader.h"
#include "DemoGame/ImGuiDemo.h"
#include "Renderer/ImGuiBridge.h"
Log_SetChannel(SkeletalAnimationDemo);

static const char *MODEL_LIST = "Ninja\0Mario\0\0";

ImGuiDemo::ImGuiDemo(DemoGame *pDemoGame)
    : BaseDemoGameState(pDemoGame)
{
    
}

ImGuiDemo::~ImGuiDemo()
{

}

void ImGuiDemo::OnSwitchedIn()
{
    BaseDemoGameState::OnSwitchedIn();
    if (!m_uiActive)
        ToggleUI();
}

bool ImGuiDemo::OnWindowEvent(const union SDL_Event *event)
{
    // block events from closing UI
    return false;
}

void ImGuiDemo::OnRenderThreadBeginFrame(float deltaTime)
{
    BaseDemoGameState::OnRenderThreadBeginFrame(deltaTime);

    // Clear targets. Have to do since no worldrenderer.
    GPUContext *pGPUContext = m_pDemoGame->GetGPUContext();
    pGPUContext->SetRenderTargets(0, nullptr, nullptr);
    pGPUContext->SetFullViewport();
    pGPUContext->ClearTargets(true, true, true);
}

void ImGuiDemo::DrawUI(float deltaTime)
{
    BaseDemoGameState::DrawUI(deltaTime);

    ImGui::ShowTestWindow();
}

bool LaunchpadGameState::InvokeImGuiDemo()
{
    return RunDemo(new ImGuiDemo(m_pDemoGame));
}
