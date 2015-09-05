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
    virtual void OnSwitchedIn() override final;
    virtual void OnSwitchedOut() override final;
    virtual void OnBeforeDelete() override final;
    virtual void OnWindowResized(uint32 width, uint32 height) override final;
    virtual void OnWindowFocusGained() override final;
    virtual void OnWindowFocusLost() override final;
    virtual bool OnWindowEvent(const union SDL_Event *event) override final;
    virtual void OnMainThreadPreFrame(float deltaTime) override final;
    virtual void OnMainThreadBeginFrame(float deltaTime) override final;
    virtual void OnMainThreadAsyncTick(float deltaTime) override final;
    virtual void OnMainThreadTick(float deltaTime) override final;
    virtual void OnMainThreadEndFrame(float deltaTime) override final;
    virtual void OnRenderThreadPreFrame(float deltaTime) override final;
    virtual void OnRenderThreadBeginFrame(float deltaTime) override final;
    virtual void OnRenderThreadDraw(float deltaTime) override final;
    virtual void OnRenderThreadEndFrame(float deltaTime) override final;

private:
    DemoGame *m_pDemoGame;

    // demo runner
    bool RunDemo(BaseDemoGameState *pGameState);

    // demo runner 
    bool InvokeSkeletalAnimationDemo();
    bool InvokeImGuiDemo();
    bool InvokeDrawCallStressDemo();
};
