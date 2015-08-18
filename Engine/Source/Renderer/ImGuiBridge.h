#pragma once
#include "Renderer/Common.h"
#include "imgui.h"

union SDL_Event;

// NOTE: All ImGui functions must be called on the render thread.

namespace ImGui
{
    // Ready engine and imgui for drawing
    bool InitializeBridge();

    // Alter the viewport dimensions
    void SetViewportDimensions(uint32 width, uint32 height);

    // On new frame, pass delta-time
    void NewFrame(float deltaTime);

    // Process a SDL event
    bool HandleSDLEvent(const SDL_Event *pEvent, bool forceCapture = false);

    // Release everything
    void FreeResources();
}
