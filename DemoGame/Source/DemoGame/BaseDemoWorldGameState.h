#pragma once
#include "DemoGame/DemoGame.h"
#include "DemoGame/DemoCamera.h"
#include "DemoGame/BaseDemoGameState.h"

class BaseDemoWorldGameState : public BaseDemoGameState
{
public:
    BaseDemoWorldGameState(DemoGame *pDemoGame);
    virtual ~BaseDemoWorldGameState();
    
    // initialization
    virtual bool Initialize();

    // shutdown
    virtual void Shutdown();

    // public events
    virtual bool CreateGPUResources();
    virtual void ReleaseGPUResources();

protected:
    virtual bool OnWorldCreated(World *pWorld);
    virtual void OnWorldDeleted(World *pWorld);

    virtual void DrawOverlays(float deltaTime) override;
    virtual void DrawUI(float deltaTime) override;
    virtual void OnUIToggled(bool enabled) override;

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
    // helper function for creating dynamic world
    bool CreateDynamicWorld();

    // world pointer, set by derived classes
    World *m_pWorld;

    // camera
    DemoCamera m_camera;

    // ---- render thread ----
    WorldRenderer::ViewParameters m_viewParameters;

private:
    bool m_dynamicWorldFlag;
};
