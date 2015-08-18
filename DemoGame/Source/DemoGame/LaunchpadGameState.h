#pragma once
#include "BaseGame/BaseGame.h"

class DemoGame;
class BaseDemoGameState;

class LaunchpadGameState : public GameState
{
public:
    LaunchpadGameState(DemoGame *pDemoGame);
    virtual ~LaunchpadGameState();

protected:
    // implemented events
    virtual void OnSwitchedIn() override;
    virtual void OnSwitchedOut() override;
    virtual void OnBeforeDelete() override;
    virtual void OnWindowResized(uint32 width, uint32 height) override;
    virtual void OnWindowFocusGained() override;
    virtual void OnWindowFocusLost() override;
    virtual bool OnWindowEvent(const union SDL_Event *event) override;
    virtual void OnMainThreadPreFrame(float deltaTime) override;
    virtual void OnMainThreadBeginFrame(float deltaTime) override;
    virtual void OnMainThreadAsyncTick(float deltaTime) override;
    virtual void OnMainThreadTick(float deltaTime) override;
    virtual void OnMainThreadEndFrame(float deltaTime) override;
    virtual void OnRenderThreadBeginFrame(float deltaTime) override;
    virtual void OnRenderThreadDraw(float deltaTime) override;
    virtual void OnRenderThreadEndFrame(float deltaTime) override;

private:
    DemoGame *m_pDemoGame;

    // demo runner
    bool RunDemo(BaseDemoGameState *pGameState);

    // demo runner 
    bool InvokeSkeletalAnimationDemo();
    bool InvokeImGuiDemo();
    bool InvokeDrawCallStressDemo();
};
