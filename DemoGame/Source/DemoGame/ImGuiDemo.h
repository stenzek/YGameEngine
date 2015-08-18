#pragma once
#include "DemoGame/BaseDemoGameState.h"

class ImGuiDemo : public BaseDemoGameState
{
public:
    ImGuiDemo(DemoGame *pDemoGame);
    virtual ~ImGuiDemo();

    virtual void OnSwitchedIn() override;

    virtual bool OnWindowEvent(const union SDL_Event *event) override;

protected:
    virtual void DrawUI(float deltaTime) override;

private:
    DemoGame *m_pDemoGame;
};

