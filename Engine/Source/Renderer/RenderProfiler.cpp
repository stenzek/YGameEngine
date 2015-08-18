#include "Renderer/PrecompiledHeader.h"
#include "Renderer/RenderProfiler.h"
#include "Renderer/Renderer.h"
#include "Renderer/MiniGUIContext.h"
Log_SetChannel(RenderProfiler);

RenderProfiler::RenderProfiler(GPUContext *pGPUContext)
    : m_pGPUContext(pGPUContext),
      m_gpuTimingEnabled(false),
      m_rendererResourcesCreated(false),
      m_pRootSection(NULL),
      m_pCurrentSection(NULL),
      m_pPreviousFrameRootSection(NULL),
      m_cameraOverrideIndex(-1)
{

}

RenderProfiler::~RenderProfiler()
{
    ReleaseRendererResources();

    while (m_sectionCache.GetSize() > 0)
    {
        delete m_sectionCache[m_sectionCache.GetSize() - 1];
        m_sectionCache.PopBack();
    }
}

RenderProfiler::Section::Section()
    : m_elapsedTime(0.0f),
      m_hasGPUStats(false),
      m_gpuTime(0.0f),
      m_gpuWaitTime(0.0f),
      m_drawCallCount(0),
      m_pGPUTimeQuery(NULL),
      m_pParent(NULL)
{
    
}

RenderProfiler::Section::~Section()
{
    DebugAssert(m_pGPUTimeQuery == NULL);
}

void RenderProfiler::Section::Clear()
{
    m_sectionName.Clear();
    m_elapsedTime = 0.0f;
    m_hasGPUStats = false;
    m_gpuTime = 0.0f;
    m_gpuWaitTime = 0.0f;
    m_drawCallCount = 0;
    m_pGPUTimeQuery = NULL;
    m_pParent = NULL;
    m_children.Clear();
}

void RenderProfiler::Section::Begin(Section *pParentSection, const char *sectionName, bool enableGPUStats, GPUContext *pGPUContext, GPUQuery *pGPUTimeQuery)
{
    m_sectionName = sectionName;
    m_hasGPUStats = enableGPUStats;
    m_startingDrawCallCount = pGPUContext->GetDrawCallCounter();
    m_timer.Reset();
    m_pParent = pParentSection;

    // start time query
    if ((m_pGPUTimeQuery = pGPUTimeQuery) != NULL)
    {
        if (!pGPUContext->BeginQuery(m_pGPUTimeQuery))
            m_pGPUTimeQuery = NULL;
    }
}

void RenderProfiler::Section::End(GPUContext *pGPUContext)
{
    // gpu stats
    if (m_hasGPUStats)
    {
        // store draw calls
        m_drawCallCount = pGPUContext->GetDrawCallCounter() - m_startingDrawCallCount;

        // gpu time
        if (m_pGPUTimeQuery != NULL)
        {
            Timer blockingTime;
            double gpuTime;
            if (pGPUContext->EndQuery(m_pGPUTimeQuery) && pGPUContext->GetQueryData(m_pGPUTimeQuery, &gpuTime, sizeof(gpuTime), 0) == GPU_QUERY_GETDATA_RESULT_OK)
                m_gpuTime = (float)(gpuTime * 1000.0);
            else
                m_gpuTime = 0.0f;

            // blocking time
            m_gpuWaitTime = (float)blockingTime.GetTimeMilliseconds();
        }
        else
        {
            m_gpuTime = 0.0f;
            m_gpuWaitTime = 0.0f;
        }
    }
    else
    {
        m_drawCallCount = 0;
        m_gpuTime = 0.0f;
        m_gpuWaitTime = 0.0f;
    }

    // overall time
    m_elapsedTime = (float)m_timer.GetTimeMilliseconds();
}

void RenderProfiler::BeginSection(const char *sectionName, bool enableGPUStats)
{
    DebugAssert(m_pCurrentSection != NULL);

    // create section
    Section *pSection = GetNewSection();
    pSection->Begin(m_pCurrentSection, sectionName, enableGPUStats, m_pGPUContext, (enableGPUStats) ? GetNewGPUTimeQuery() : NULL);

    // add to current node
    m_pCurrentSection->m_children.Add(pSection);

    // switch out
    m_pCurrentSection = pSection;
}

void RenderProfiler::EndSection()
{
    DebugAssert(m_pCurrentSection != NULL);

    // end section
    m_pCurrentSection->End(m_pGPUContext);

    // release query back to context
    if (m_pCurrentSection->m_pGPUTimeQuery != NULL)
    {
        m_freeGPUTimeQueries.Add(m_pCurrentSection->m_pGPUTimeQuery);
        m_pCurrentSection->m_pGPUTimeQuery = NULL;
    }

    // swap pointer to parent
    m_pCurrentSection = m_pCurrentSection->m_pParent;
}

void RenderProfiler::BeginFrame()
{
    // should not have a current frame
    DebugAssert(m_pRootSection == NULL && m_pCurrentSection == NULL);

    // if this fails, ehh, we just won't time it
    if (!m_rendererResourcesCreated)
        CreateRendererResources();

    // create the root section
    Section *pSection = GetNewSection();
    pSection->Begin(NULL, "<<<Frame>>>", true, m_pGPUContext, GetNewGPUTimeQuery());
    m_pRootSection = pSection;
    m_pCurrentSection = pSection;
}

void RenderProfiler::EndFrame()
{
    Assert(m_pCurrentSection != NULL && m_pCurrentSection == m_pRootSection);
    m_pCurrentSection->End(m_pGPUContext);

    // release query back to context
    if (m_pCurrentSection->m_pGPUTimeQuery != NULL)
    {
        m_freeGPUTimeQueries.Add(m_pCurrentSection->m_pGPUTimeQuery);
        m_pCurrentSection->m_pGPUTimeQuery = NULL;
    }

    // if there is a previous frame, clear it out
    if (m_pPreviousFrameRootSection != NULL)
    {
        CleanupSectionAndChildren(m_pPreviousFrameRootSection);
        m_pPreviousFrameRootSection = NULL;
    }

    // set this frame to be the new previous frame
    m_pPreviousFrameRootSection = m_pRootSection;
    m_pCurrentSection = NULL;
    m_pRootSection = NULL;

    // cleanup the camera list
    m_cameraArray.Clear();
}

bool RenderProfiler::CreateRendererResources()
{
    if (m_rendererResourcesCreated)
        return true;

    //if (m_pGPUTimeQuery == NULL && (m_pGPUTimeQuery = g_pRenderer->CreateQuery(GPU_QUERY_TYPE_TIME_ELAPSED)) == NULL)
        //Log_WarningPrintf("RenderProfiler::CreateRendererResources: Could not create GPU time query.");

    m_rendererResourcesCreated = true;
    return true;
}

void RenderProfiler::ReleaseRendererResources()
{
    //SAFE_RELEASE(m_pGPUTimeQuery);

    while (m_gpuTimeQueryCache.GetSize() > 0)
    {
        m_gpuTimeQueryCache[m_gpuTimeQueryCache.GetSize() - 1]->Release();
        m_gpuTimeQueryCache.PopBack();
    }
    m_freeGPUTimeQueries.Clear();

    // todo: clean from sections?

    m_rendererResourcesCreated = false;
}

void RenderProfiler::DrawThisFrameSummary(MiniGUIContext *pGUIContext, const DrawSummaryOptions *pDrawOptions)
{
    if (m_pRootSection != NULL)
        DrawFrameSummary(m_pRootSection, pGUIContext, pDrawOptions);
}

void RenderProfiler::DrawPreviousFrameSummary(MiniGUIContext *pGUIContext, const DrawSummaryOptions *pDrawOptions)
{
    if (m_pPreviousFrameRootSection != NULL)
        DrawFrameSummary(m_pPreviousFrameRootSection, pGUIContext, pDrawOptions);
}

void RenderProfiler::DrawFrameSummary(const Section *pRootSection, MiniGUIContext *pGUIContext, const DrawSummaryOptions *pDrawOptions)
{
    DrawSummaryOptions defaultDrawOptions;
    if (pDrawOptions == NULL)
    {
        defaultDrawOptions.RedThresholdMs = 16.0f;
        defaultDrawOptions.YellowThresholdMs = 8.0f;
        defaultDrawOptions.pFont = g_pRenderer->GetFixedResources()->GetDebugFont();
        defaultDrawOptions.FontSize = 16;
        //defaultDrawOptions.DrawAreaStartX = 20;
        //defaultDrawOptions.DrawAreaStartY = 20;
        //defaultDrawOptions.DrawAreaWidth = pGUIContext->GetTopRect().right - 40;
        //defaultDrawOptions.DrawAreaHeight = pGUIContext->GetTopRect().bottom - 40;
        defaultDrawOptions.DrawAreaStartX = pGUIContext->GetTopRect().right - 600;
        defaultDrawOptions.DrawAreaStartY = 20;
        defaultDrawOptions.DrawAreaWidth = 600;
        //defaultDrawOptions.DrawAreaHeight = 690;
        defaultDrawOptions.DrawAreaHeight = 1000;
        defaultDrawOptions.DrawAreaBackgroundColor = MAKE_COLOR_R8G8B8A8_UNORM(30, 30, 30, 200);
        defaultDrawOptions.DrawAreaBorderColor = MAKE_COLOR_R8G8B8A8_UNORM(200, 200, 200, 255);
        pDrawOptions = &defaultDrawOptions;
    }

    MINIGUI_RECT rect, bgRect;
    //Log_DevPrintf("total time: %.4f ms", pRootSection->GetElapsedTime());

    // set rect to the draw area
    rect.Set(pDrawOptions->DrawAreaStartX, pDrawOptions->DrawAreaStartX + pDrawOptions->DrawAreaWidth, pDrawOptions->DrawAreaStartY, pDrawOptions->DrawAreaStartY + pDrawOptions->DrawAreaHeight);

    // manual flushing
    pGUIContext->PushManualFlush();
    pGUIContext->SetAlphaBlendingEnabled(true);
    pGUIContext->SetAlphaBlendingMode(MiniGUIContext::ALPHABLENDING_MODE_STRAIGHT);

    // draw the background
    pGUIContext->DrawFilledRect(&rect, pDrawOptions->DrawAreaBackgroundColor);

    // and the outline
    pGUIContext->DrawRect(&rect, pDrawOptions->DrawAreaBorderColor);

    // push the full rectangle
    //pGUIContext->PushRectAndClipRect(&rect);
    pGUIContext->PushRect(&rect);

    // background and line
    bgRect.Set(2, (rect.right - rect.left) - 2, 2 + pDrawOptions->FontSize + 2 + 2 + 2, 2 + pDrawOptions->FontSize + 2 + 2 + 2 + pDrawOptions->FontSize + 2);
    pGUIContext->DrawFilledRect(&bgRect, MAKE_COLOR_R8G8B8A8_UNORM(180, 180, 180, 255));
    //pGUIContext->DrawLine(int2(2, 2 + pDrawOptions->FontSize + 2 + 2 + 2 + pDrawOptions->FontSize + 2), int2((rect.right - rect.left) - 2, 2 + pDrawOptions->FontSize + 2 + 2 + 2 + pDrawOptions->FontSize + 2), MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 255));

    // draw headings
    pGUIContext->DrawText(pDrawOptions->pFont, pDrawOptions->FontSize, 2, 2, "Render Profiler", MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 255, 255));
    pGUIContext->DrawText(pDrawOptions->pFont, pDrawOptions->FontSize, 2, 2 + pDrawOptions->FontSize + 2 + 2 + 2, "Section", MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 255));
    pGUIContext->DrawText(pDrawOptions->pFont, pDrawOptions->FontSize, -240, 2 + pDrawOptions->FontSize + 2 + 2 + 2, "Total", MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 255));
    pGUIContext->DrawText(pDrawOptions->pFont, pDrawOptions->FontSize, -180, 2 + pDrawOptions->FontSize + 2 + 2 + 2, "CPU", MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 255));
    pGUIContext->DrawText(pDrawOptions->pFont, pDrawOptions->FontSize, -120, 2 + pDrawOptions->FontSize + 2 + 2 + 2, "GPU", MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 255));
    pGUIContext->DrawText(pDrawOptions->pFont, pDrawOptions->FontSize, -60, 2 + pDrawOptions->FontSize + 2 + 2 + 2, "Draws", MAKE_COLOR_R8G8B8A8_UNORM(0, 0, 0, 255));

    // add content margins
    rect.Set(2, (rect.right - rect.left) - 2, 2 + pDrawOptions->FontSize + 2 + 2 + 2 + pDrawOptions->FontSize + 2, pDrawOptions->DrawAreaHeight - (2 + pDrawOptions->FontSize + 2 + 2 + 2 + pDrawOptions->FontSize + 2) - 2);
    //pGUIContext->PushRectAndClipRect(&rect);
    pGUIContext->PushRect(&rect);

    // draw all nodes
    DrawFrameSummaryRecursive(pRootSection, pGUIContext, pDrawOptions);

    // pop rect
    //pGUIContext->PopRectAndClipRect();
    //pGUIContext->PopRectAndClipRect();
    pGUIContext->PopRect();
    pGUIContext->PopRect();

    pGUIContext->PopManualFlush();
}

uint32 RenderProfiler::DrawFrameSummaryRecursive(const Section *pCurrentSection, MiniGUIContext *pGUIContext, const DrawSummaryOptions *pDrawOptions)
{
    const uint32 spacing = 2;
    const uint32 indent = 10;
    
    SmallString message;
    uint32 currentPos = 0;

    uint32 textColor;
    if (pCurrentSection->GetElapsedTime() >= pDrawOptions->RedThresholdMs)
        textColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 20, 20, 255);
    else if (pCurrentSection->GetElapsedTime() >= pDrawOptions->YellowThresholdMs)
        textColor = MAKE_COLOR_R8G8B8A8_UNORM(255, 255, 20, 255);
    else
        textColor = MAKE_COLOR_R8G8B8A8_UNORM(200, 200, 200, 255);

    // section name
    pGUIContext->DrawText(pDrawOptions->pFont, pDrawOptions->FontSize, 0, currentPos, pCurrentSection->GetSectionName(), textColor);
    
    // total time
    message.Format("%.2fms", pCurrentSection->GetElapsedTime());
    pGUIContext->DrawText(pDrawOptions->pFont, pDrawOptions->FontSize, -240, currentPos, message, textColor);

    // cpu time
    message.Format("%.2fms", pCurrentSection->GetCPUTime());
    pGUIContext->DrawText(pDrawOptions->pFont, pDrawOptions->FontSize, -180, currentPos, message, textColor);

    if (pCurrentSection->HasGPUStats())
    {
        if (m_gpuTimingEnabled)
        {
            message.Format("%.2fms", pCurrentSection->GetGPUTime());
            pGUIContext->DrawText(pDrawOptions->pFont, pDrawOptions->FontSize, -120, currentPos, message, textColor);
        }
        else
        {
            pGUIContext->DrawText(pDrawOptions->pFont, pDrawOptions->FontSize, -120, currentPos, "-", textColor);
        }
        message.Format("%u", pCurrentSection->GetDrawCallCount());
        pGUIContext->DrawText(pDrawOptions->pFont, pDrawOptions->FontSize, -60, currentPos, message, textColor);

    }
    else
    {
        pGUIContext->DrawText(pDrawOptions->pFont, pDrawOptions->FontSize, -120, currentPos, "-", textColor);
        pGUIContext->DrawText(pDrawOptions->pFont, pDrawOptions->FontSize, -60, currentPos, "-", textColor);
    }

    currentPos += pDrawOptions->FontSize + spacing;

    // draw children
    for (uint32 i = 0; i < pCurrentSection->GetChildCount(); i++)
    {
        // create rect for next level down
        MINIGUI_RECT rect;
        rect.Set(indent, pGUIContext->GetTopRect().right - pGUIContext->GetTopRect().left, currentPos, pGUIContext->GetTopRect().bottom - pGUIContext->GetTopRect().top);
        //pGUIContext->PushRectAndClipRect(&rect);
        pGUIContext->PushRect(&rect);

        // draw it
        currentPos += DrawFrameSummaryRecursive(pCurrentSection->GetChild(i), pGUIContext, pDrawOptions);

        // pop rect
        //pGUIContext->PopRectAndClipRect();
        pGUIContext->PopRect();
    }

    return currentPos;
}

RenderProfiler::Section *RenderProfiler::GetNewSection()
{
    RenderProfiler::Section *pSection;
    
    if (m_freeSections.GetSize() > 0)
    {
        pSection = m_freeSections[0];
        m_freeSections.PopFront();
    }
    else
    {
        pSection = new RenderProfiler::Section();
        m_sectionCache.Add(pSection);
    }

    return pSection;
}

GPUQuery *RenderProfiler::GetNewGPUTimeQuery()
{
    if (!m_gpuTimingEnabled)
        return NULL;

    GPUQuery *pQuery;

    if (m_freeGPUTimeQueries.GetSize() > 0)
    {
        pQuery = m_freeGPUTimeQueries[0];
        m_freeGPUTimeQueries.PopFront();
    }
    else
    {
        pQuery = g_pRenderer->CreateQuery(GPU_QUERY_TYPE_TIMESTAMP);
        m_gpuTimeQueryCache.Add(pQuery);
    }

    return pQuery;
}

void RenderProfiler::CleanupSectionAndChildren(Section *pSection)
{
    DebugAssert(pSection->m_pGPUTimeQuery == NULL);

    // clean children
    for (uint32 i = 0; i < pSection->m_children.GetSize(); i++)
        CleanupSectionAndChildren(pSection->m_children[i]);

    // clean section
    pSection->Clear();

    // add to free list
    m_freeSections.Add(pSection);    
}

void RenderProfiler::AddCamera(const Camera *pCamera, const char *cameraName)
{
    m_cameraArray.Add(KeyValuePair<String, Camera>(cameraName, *pCamera));
}

