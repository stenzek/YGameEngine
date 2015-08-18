#pragma once
#include "DemoGame/DemoGame.h"
#include "DemoGame/DemoCamera.h"
#include "BaseGame/GameState.h"

class BaseDemoGameState : public GameState
{
public:
    BaseDemoGameState(DemoGame *pDemoGame);
    virtual ~BaseDemoGameState();
    
    // initialization
    virtual bool Initialize();

    // shutdown
    virtual void Shutdown();

    // public events
    virtual bool CreateGPUResources();
    virtual void ReleaseGPUResources();

protected:
    virtual void DrawOverlays(float deltaTime);
    virtual void DrawUI(float deltaTime);
    virtual void OnUIToggled(bool enabled);

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

protected:
    DemoGame *m_pDemoGame;

    // hide the ui and enable camera movement
    void ToggleUI();

    // ui active (owned by main thread)
    bool m_uiActive;
};
