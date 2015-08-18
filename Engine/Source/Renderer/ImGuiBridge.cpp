#include "Renderer/PrecompiledHeader.h"
#include "Renderer/ImGuiBridge.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/Shaders/OverlayShader.h"
#include "Engine/SDLHeaders.h"
Log_SetChannel(ImGuiBridge);

static GPUBuffer *s_pVertexBuffer = nullptr;
static GPUBuffer *s_pIndexBuffer = nullptr;
static uint32 s_vertexBufferSize = 0;
static uint32 s_indexBufferSize = 0;

static void RenderDrawListsCallback(ImDrawData *pDrawData)
{
    GPUContext *pGPUContext = GPUContext::GetContextForCurrentThread();

#if 1
    // check buffer size
    if ((uint32)pDrawData->TotalVtxCount > s_vertexBufferSize)
    {
        uint32 newVertexCount = Max((s_vertexBufferSize != 0) ? (s_vertexBufferSize * 2) : 1024, (uint32)pDrawData->TotalVtxCount);
        Log_PerfPrintf("Reallocating ImGui vertex buffer, new count = %u (%s)", newVertexCount, StringConverter::SizeToHumanReadableString(newVertexCount * sizeof(ImDrawVert)).GetCharArray());

        GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_VERTEX_BUFFER | GPU_BUFFER_FLAG_MAPPABLE, newVertexCount * sizeof(ImDrawVert));
        GPUBuffer *pNewVertexBuffer = g_pRenderer->CreateBuffer(&bufferDesc, nullptr);
        if (pNewVertexBuffer == nullptr)
        {
            Log_ErrorPrint("Failed to allocate ImGui vertex buffer.");
            return;
        }

        if (s_pVertexBuffer != nullptr)
            s_pVertexBuffer->Release();

        s_pVertexBuffer = pNewVertexBuffer;
        s_vertexBufferSize = newVertexCount;
    }
    if ((uint32)pDrawData->TotalIdxCount > s_indexBufferSize)
    {
        uint32 newIndexCount = Max((s_indexBufferSize != 0) ? (s_indexBufferSize * 2) : 1024, (uint32)pDrawData->TotalIdxCount);
        Log_PerfPrintf("Reallocating ImGui index buffer, new count = %u (%s)", newIndexCount, StringConverter::SizeToHumanReadableString(newIndexCount * sizeof(ImDrawIdx)).GetCharArray());

        GPU_BUFFER_DESC bufferDesc(GPU_BUFFER_FLAG_BIND_INDEX_BUFFER | GPU_BUFFER_FLAG_MAPPABLE, newIndexCount * sizeof(ImDrawIdx));
        GPUBuffer *pNewIndexBuffer = g_pRenderer->CreateBuffer(&bufferDesc, nullptr);
        if (pNewIndexBuffer == nullptr)
        {
            Log_ErrorPrint("Failed to allocate ImGui index buffer.");
            return;
        }

        if (s_pIndexBuffer != nullptr)
            s_pIndexBuffer->Release();

        s_pIndexBuffer = pNewIndexBuffer;
        s_indexBufferSize = newIndexCount;
    }

    // write to buffers
    if (g_pRenderer->GetFeatureLevel() >= RENDERER_FEATURE_LEVEL_ES3)
    {
        ImDrawVert *pMappedVertexBuffer;
        if (!pGPUContext->MapBuffer(s_pVertexBuffer, GPU_MAP_TYPE_WRITE_DISCARD, reinterpret_cast<void **>(&pMappedVertexBuffer)))
        {
            Log_ErrorPrint("Failed to map ImGui vertex buffer");
            return;
        }

        ImDrawIdx *pMappedIndexBuffer;
        if (!pGPUContext->MapBuffer(s_pIndexBuffer, GPU_MAP_TYPE_WRITE_DISCARD, reinterpret_cast<void **>(&pMappedIndexBuffer)))
        {
            pGPUContext->Unmapbuffer(s_pVertexBuffer, pMappedVertexBuffer);
            Log_ErrorPrint("Failed to map ImGui index buffer");
            return;
        }

        // copy vertices in
        ImDrawVert *pCurrentVertex = pMappedVertexBuffer;
        ImDrawIdx *pCurrentIndex = pMappedIndexBuffer;
        for (int i = 0; i < pDrawData->CmdListsCount; i++)
        {
            Y_memcpy(pCurrentVertex, pDrawData->CmdLists[i]->VtxBuffer.Data, pDrawData->CmdLists[i]->VtxBuffer.size() * sizeof(ImDrawVert));
            Y_memcpy(pCurrentIndex, pDrawData->CmdLists[i]->IdxBuffer.Data, pDrawData->CmdLists[i]->IdxBuffer.size() * sizeof(ImDrawIdx));
            pCurrentVertex += pDrawData->CmdLists[i]->VtxBuffer.size();
            pCurrentIndex += pDrawData->CmdLists[i]->IdxBuffer.size();
        }

        // unmap again
        pGPUContext->Unmapbuffer(s_pIndexBuffer, pMappedIndexBuffer);
        pGPUContext->Unmapbuffer(s_pVertexBuffer, pMappedVertexBuffer);
    }
    else
    {
        // annoyingly, ES2 doesn't have the ability to map buffers
        uint32 vertexBufferOffset = 0;
        uint32 indexBufferOffset = 0;
        for (int i = 0; i < pDrawData->CmdListsCount; i++)
        {
            pGPUContext->WriteBuffer(s_pVertexBuffer, pDrawData->CmdLists[i]->VtxBuffer.Data, 0, pDrawData->CmdLists[i]->VtxBuffer.size() * sizeof(ImDrawVert));
            pGPUContext->WriteBuffer(s_pIndexBuffer, pDrawData->CmdLists[i]->IdxBuffer.Data, 0, pDrawData->CmdLists[i]->IdxBuffer.size() * sizeof(ImDrawIdx));
            vertexBufferOffset += pDrawData->CmdLists[i]->VtxBuffer.size() * sizeof(ImDrawVert);
            indexBufferOffset += pDrawData->CmdLists[i]->IdxBuffer.size() * sizeof(ImDrawIdx);
        }
    }

    // set up device
    pGPUContext->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_NONE, false, false, true));
    pGPUContext->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false), 0);
    pGPUContext->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAlphaBlending());

    // load shader
    ShaderProgram *pShaderProgram = g_pRenderer->GetFixedResources()->GetOverlayShaderTexturedScreen();
    pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());
    pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);

    // set buffers
    pGPUContext->SetVertexBuffer(0, s_pVertexBuffer, 0, sizeof(ImDrawVert));
    pGPUContext->SetIndexBuffer(s_pIndexBuffer, GPU_INDEX_FORMAT_UINT16, 0);

    // draw commands
    unsigned int baseVertex = 0;
    unsigned int baseIndex = 0;
    bool adjustBufferPointers = !(g_pRenderer->GetCapabilities().SupportsDrawBaseVertex);
    for (int i = 0; i < pDrawData->CmdListsCount; i++)
    {
        ImDrawList *pCmdList = pDrawData->CmdLists[i];
        unsigned int firstIndex = 0;

        for (int j = 0; j < pCmdList->CmdBuffer.size(); j++)
        {
            const ImDrawCmd *pCmd = &pCmdList->CmdBuffer[j];
            if (pCmd->UserCallback != nullptr)
            {
                pCmd->UserCallback(pCmdList, pCmd);
                continue;
            }

            // set up clip rect
            RENDERER_SCISSOR_RECT scissorRect((uint32)pCmd->ClipRect.x, (uint32)pCmd->ClipRect.y, (uint32)pCmd->ClipRect.z, (uint32)pCmd->ClipRect.w);
            pGPUContext->SetScissorRect(&scissorRect);

            // bind texture
            OverlayShader::SetTexture(pGPUContext, pShaderProgram, reinterpret_cast<GPUTexture2D *>(pCmd->TextureId));

            // adjust buffer pointer
            if (adjustBufferPointers && baseVertex > 0)
            {
                pGPUContext->SetVertexBuffer(0, s_pVertexBuffer, sizeof(ImDrawVert) * baseVertex, sizeof(ImDrawVert));
                pGPUContext->SetIndexBuffer(s_pIndexBuffer, GPU_INDEX_FORMAT_UINT16, sizeof(ImDrawIdx) * baseIndex);
                pGPUContext->DrawIndexed(firstIndex, pCmd->ElemCount, 0);
            }
            else
            {
                pGPUContext->DrawIndexed(baseIndex + firstIndex, pCmd->ElemCount, baseVertex);
            }

            // update pointers
            baseIndex += pCmd->ElemCount;
        }

        baseVertex += pCmdList->VtxBuffer.size();
    }

    // clear bindings
    pGPUContext->ClearState(false, true, false, false);

#else

    // set up device
    pGPUContext->SetRasterizerState(g_pRenderer->GetFixedResources()->GetRasterizerState(RENDERER_FILL_SOLID, RENDERER_CULL_NONE, false, false, true));
    pGPUContext->SetDepthStencilState(g_pRenderer->GetFixedResources()->GetDepthStencilState(false, false), 0);
    pGPUContext->SetBlendState(g_pRenderer->GetFixedResources()->GetBlendStateAlphaBlending());

    // load shader
    ShaderProgram *pShaderProgram = g_pRenderer->GetFixedResources()->GetOverlayShaderTexturedScreen();
    pGPUContext->SetInputLayout(g_pRenderer->GetFixedResources()->GetOverlayInputLayoutScreen());
    pGPUContext->SetShaderProgram(pShaderProgram->GetGPUProgram());
    pGPUContext->SetDrawTopology(DRAW_TOPOLOGY_TRIANGLE_LIST);
    pDrawData->DeIndexAllBuffers();

    // draw commands
    for (int i = 0; i < pDrawData->CmdListsCount; i++)
    {
        ImDrawList *pCmdList = pDrawData->CmdLists[i];
        unsigned int startVertex = 0;

        for (int j = 0; j < pCmdList->CmdBuffer.size(); j++)
        {
            const ImDrawCmd *pCmd = &pCmdList->CmdBuffer[j];
            if (pCmd->UserCallback != nullptr)
            {
                pCmd->UserCallback(pCmdList, pCmd);
                continue;
            }

            // set up clip rect
            RENDERER_SCISSOR_RECT scissorRect((uint32)pCmd->ClipRect.x, (uint32)pCmd->ClipRect.y, (uint32)pCmd->ClipRect.z, (uint32)pCmd->ClipRect.w);
            pGPUContext->SetScissorRect(&scissorRect);

            // bind texture
            OverlayShader::SetTexture(pGPUContext, pShaderProgram, reinterpret_cast<GPUTexture2D *>(pCmd->TextureId));

            // this matches our overlay vertex format so we can just chuck it straight through.. actually would be an optimization to avoid it though
            pGPUContext->DrawUserPointer(&pCmdList->VtxBuffer[startVertex], sizeof(ImDrawVert), pCmd->ElemCount);
            startVertex += pCmd->ElemCount;
        }
    }
#endif
}

bool ImGui::InitializeBridge()
{
    GPUContext *pGPUContext = GPUContext::GetContextForCurrentThread();
    RendererOutputBuffer *pOutputBuffer = pGPUContext->GetOutputBuffer();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = (float)pOutputBuffer->GetWidth();
    io.DisplaySize.y = (float)pOutputBuffer->GetHeight();
    io.DeltaTime = 0.0f;
    io.RenderDrawListsFn = RenderDrawListsCallback;
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    // initialize rendering font
    {
        // get font pixels
        const void *pFontPixels;
        int fontWidth, fontHeight;
        uint32 pitch;
        io.Fonts->GetTexDataAsRGBA32((unsigned char **)&pFontPixels, &fontWidth, &fontHeight);
        pitch = PixelFormat_CalculateRowPitch(PIXEL_FORMAT_R8G8B8A8_UNORM, fontWidth);

        // Create the font as a GPU texture
        GPU_TEXTURE2D_DESC textureDesc(fontWidth, fontHeight, PIXEL_FORMAT_R8G8B8A8_UNORM, GPU_TEXTURE_FLAG_SHADER_BINDABLE, 1);
        GPU_SAMPLER_STATE_DESC samplerStateDesc(TEXTURE_FILTER_MIN_MAG_LINEAR_MIP_POINT, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, TEXTURE_ADDRESS_MODE_CLAMP, float4::Zero, 0.0f, 0, 0, 1, GPU_COMPARISON_FUNC_NEVER);
        GPUTexture2D *pFontTexture = g_pRenderer->CreateTexture2D(&textureDesc, &samplerStateDesc, &pFontPixels, &pitch);
        if (pFontTexture == nullptr)
        {
            Log_ErrorPrintf("ImGui::InitializeBridge: Failed to create font texture.");
            return false;
        }

        io.Fonts->TexID = pFontTexture;
    }

    // initialize keyboard map
    {
        io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
        io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
        io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
        io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
        io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
        io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
        io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
        io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
        io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
        io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
        io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
        io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
        io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
        io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
        io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
        io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;
    }

    // done
    return true;
}

void ImGui::SetViewportDimensions(uint32 width, uint32 height)
{
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = (float)width;
    io.DisplaySize.y = (float)height;
}

void ImGui::NewFrame(float deltaTime)
{
    ImGuiIO& io = ImGui::GetIO();

    // update delta time
    io.DeltaTime = deltaTime;

    // update keyboard modifiers
    SDL_Keymod currentModifiers = SDL_GetModState();
    io.KeyAlt = (currentModifiers & (KMOD_LALT | KMOD_RALT)) != 0;
    io.KeyCtrl = (currentModifiers & (KMOD_LCTRL | KMOD_RCTRL)) != 0;
    io.KeyShift = (currentModifiers & (KMOD_LSHIFT | KMOD_RSHIFT)) != 0;

    // fall through
    ImGui::NewFrame();
}

bool ImGui::HandleSDLEvent(const SDL_Event *pEvent, bool forceCapture /* = false */)
{
    ImGuiIO& io = ImGui::GetIO();

    // keyboard events
    if (io.WantCaptureKeyboard || forceCapture)
    {
        switch (pEvent->type)
        {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            {
                if (pEvent->key.keysym.scancode < countof(io.KeysDown))
                    io.KeysDown[pEvent->key.keysym.scancode] = (pEvent->type == SDL_KEYDOWN);

                return true;
            }

        case SDL_TEXTINPUT:
            {
                // todo: utf-8 to utf-16
                size_t length = Y_strlen(pEvent->text.text);
                for (uint32 i = 0; i < length; i++)
                    io.AddInputCharacter((ImWchar)pEvent->text.text[i]);

                return true;
            }
        }
    }

    // mouse events
    if (io.WantCaptureMouse || forceCapture)
    {
        switch (pEvent->type)
        {
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            {
                static const uint32 buttonMapping[5] = { 0, 0, 2, 1, 3 };
                if (pEvent->button.button < countof(buttonMapping))
                    io.MouseDown[buttonMapping[pEvent->button.button]] = (pEvent->type == SDL_MOUSEBUTTONDOWN);

                return true;
            }

        case SDL_MOUSEMOTION:
            io.MousePos.x = (float)pEvent->motion.x;
            io.MousePos.y = (float)pEvent->motion.y;
            return true;

        case SDL_MOUSEWHEEL:
            io.MouseWheel = (float)pEvent->wheel.y;
            return true;
        }

    }

    return false;
}

void ImGui::FreeResources()
{
    // todo: release font
}
