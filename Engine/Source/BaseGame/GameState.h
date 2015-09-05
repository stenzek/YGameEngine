#pragma once
#include "BaseGame/Common.h"

class GameState
{
public:
    virtual ~GameState() {}

    //=================================================================================================================================================================================================
    // Events
    //=================================================================================================================================================================================================
    virtual void OnSwitchedIn() = 0;
    virtual void OnSwitchedOut() = 0;
    virtual void OnBeforeDelete() = 0;
    virtual void OnWindowResized(uint32 width, uint32 height) = 0;
    virtual void OnWindowFocusGained() = 0;
    virtual void OnWindowFocusLost() = 0;
    virtual bool OnWindowEvent(const union SDL_Event *event) = 0;
    virtual void OnMainThreadPreFrame(float deltaTime) = 0;
    virtual void OnMainThreadBeginFrame(float deltaTime) = 0;
    virtual void OnMainThreadAsyncTick(float deltaTime) = 0;
    virtual void OnMainThreadTick(float deltaTime) = 0;
    virtual void OnMainThreadEndFrame(float deltaTime) = 0;
    virtual void OnRenderThreadPreFrame(float deltaTime) = 0;
    virtual void OnRenderThreadBeginFrame(float deltaTime) = 0;
    virtual void OnRenderThreadDraw(float deltaTime) = 0;
    virtual void OnRenderThreadEndFrame(float deltaTime) = 0;
};
