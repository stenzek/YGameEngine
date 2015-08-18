#include "Renderer/PrecompiledHeader.h"
#include "Renderer/RenderQueue.h"
#include "Engine/Material.h"
//Log_SetChannel(Renderer);

static uint32 GetTransparencyKey(const RENDER_QUEUE_RENDERABLE_ENTRY *pEntry)
{
    switch (pEntry->pMaterial->GetShader()->GetBlendMode())
    {
    case MATERIAL_BLENDING_MODE_MASKED:
        return 1;

    case MATERIAL_BLENDING_MODE_STRAIGHT:
    case MATERIAL_BLENDING_MODE_PREMULTIPLIED:
    case MATERIAL_BLENDING_MODE_SOFTMASKED:
        return 2;

        //case MATERIAL_BLENDING_MODE_NONE:
        //case MATERIAL_BLENDING_MODE_ADDITIVE:
        //default:
    }

    if ((pEntry->RenderPassMask & RENDER_PASS_TINT) && (pEntry->TintColor >> 24) != 0xFF)
        return 2;
    else
        return 0;
}

static uint64 MakeSortKey(const RENDER_QUEUE_RENDERABLE_ENTRY *pEntry, uint32 transparencyKey)
{
    uint32 transparencyPart, layerPart, materialPart, depthPart;

    static const uint32 transparencyPartFilter  = 0x00000003;   // 2 bits
    //static const uint64 transparencyPartMask    = 0xC000000000000000ULL;   // 2 bits
    static const uint32 layerPartFilter         = 0x00000007;   // 3 bits
    //static const uint64 layerPartMask           = 0x3800000000000000ULL;   // 3 bits
    static const uint32 materialPartFilter      = 0x07FFFFFF;    // 27 bits
    //static const uint64 materialPartMask      = 0x07FFFFFF00000000ULL;    // 27 bits
    static const uint32 depthPartFilter         = 0xFFFFFFFF;   // 32 bits
    //static const uint64 depthPartMask           = 0x00000000FFFFFFFFULL;   // 32 bits

    transparencyPart = transparencyKey & 0xFF;    
    layerPart = pEntry->Layer & layerPartFilter;

    // generate material id, fixme later
    materialPart = HashTrait<String>::GetHash(pEntry->pMaterial->GetName());

    // split the number
    float realPart, fractionalPart;
    Y_splitf(&realPart, &fractionalPart, Y_fabs(pEntry->ViewDistance));

    // use the first 24 bits for the real part
    depthPart = (Min((uint32)0xFFFFFF, (uint32)Y_truncf(realPart))) << 8;

    // use the last 8 bits for the fractional part
    depthPart |= (uint32)Y_truncf(fractionalPart * 255.0f);

    // sort front-to-back for opaque, back-to-front for transparent
    // also flip the depth and material bits around so depth gets first priority
    if (transparencyPart == 2)
    {
        depthPart = 0xFFFFFFFF - depthPart;
        
        return (uint64(transparencyPart & transparencyPartFilter) << 62) |
               (uint64(layerPart & layerPartFilter) << 60) |
               (uint64(depthPart & materialPartFilter) << 27) |
               (uint64(materialPart & depthPartFilter));
    }
    else
    {
        return (uint64(transparencyPart & transparencyPartFilter) << 62) |
               (uint64(layerPart & layerPartFilter) << 60) |
               (uint64(materialPart & materialPartFilter) << 32) |
               (uint64(depthPart & depthPartFilter));
    }
}

RenderQueue::RenderQueue()
    : m_acceptingLights(false),
      m_acceptingRenderPassMask(0),
      m_acceptingOccluders(false),
      m_acceptingDebugObjects(false),
      m_queueSize(0),
      m_numObjectsInvalidatedByOcclusion(0),
      m_directionalLightArray(1),
      m_pointLightArray(16),
      m_spotLightArray(8),
      m_opaqueRenderables(2048),
      m_translucentRenderables(1024),
      m_occluders(512),
      m_debugDrawObjects(128)
{

}

RenderQueue::~RenderQueue()
{

}

void RenderQueue::AddLight(const RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY *pLightEntry)
{
    if (!m_acceptingLights)
        return;

    DebugAssert(pLightEntry->ShadowMapIndex == -1);
    m_directionalLightArray.Add(*pLightEntry);
    m_queueSize++;
}

void RenderQueue::AddLight(const RENDER_QUEUE_POINT_LIGHT_ENTRY *pLightEntry)
{
    if (!m_acceptingLights)
        return;

    DebugAssert(pLightEntry->ShadowMapIndex == -1);
    m_pointLightArray.Add(*pLightEntry);
    m_queueSize++;
}

void RenderQueue::AddLight(const RENDER_QUEUE_SPOT_LIGHT_ENTRY *pLightEntry)
{
    if (!m_acceptingLights)
        return;

    DebugAssert(pLightEntry->ShadowMapIndex == -1);
    m_spotLightArray.Add(*pLightEntry);
    m_queueSize++;
}

void RenderQueue::AddLight(const RENDER_QUEUE_VOLUMETRIC_LIGHT_ENTRY *pLightEntry)
{
    if (!m_acceptingLights)
        return;

    m_volumetricLightArray.Add(*pLightEntry);
    m_queueSize++;
}

void RenderQueue::AddRenderable(const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry)
{
    uint32 newMask = pQueueEntry->RenderPassMask & m_acceptingRenderPassMask;
    if (newMask == 0)
        return;

    // throw out tinted objects with an opacity of zero
    if ((newMask & RENDER_PASS_TINT) && (pQueueEntry->TintColor >> 24) == 0)
        return;

    // calc transparency key
    uint32 transparencyKey = GetTransparencyKey(pQueueEntry);

    RENDER_QUEUE_RENDERABLE_ENTRY *pAddedEntry;
    if (pQueueEntry->pMaterial->GetShader()->GetRenderMode() == MATERIAL_RENDER_MODE_POST_PROCESS)
    {
        m_postProcessRenderables.Add(*pQueueEntry);
        pAddedEntry = m_postProcessRenderables.GetBasePointer() + (m_postProcessRenderables.GetSize() - 1);
    }
    else
    {
        if (transparencyKey != 2)
        {
            m_opaqueRenderables.Add(*pQueueEntry);
            pAddedEntry = m_opaqueRenderables.GetBasePointer() + (m_opaqueRenderables.GetSize() - 1);
        }
        else
        {
            m_translucentRenderables.Add(*pQueueEntry);
            pAddedEntry = m_translucentRenderables.GetBasePointer() + (m_translucentRenderables.GetSize() - 1);
        }
    }

    pAddedEntry->SortKey = MakeSortKey(pAddedEntry, transparencyKey);
    pAddedEntry->RenderPassMask = newMask;
    m_queueSize++;
}

void RenderQueue::AddOccluder(const RENDER_QUEUE_OCCLUDER_ENTRY *pOccluderEntry)
{
    if (!m_acceptingOccluders)
        return;

    m_occluders.Add(*pOccluderEntry);
    m_queueSize++;
}

void RenderQueue::AddOccluder(const RenderProxy *pRenderProxy, const AABox &boundingBox)
{
    if (!m_acceptingOccluders)
        return;

    RENDER_QUEUE_OCCLUDER_ENTRY occluderEntry;
    occluderEntry.pRenderProxy = pRenderProxy;
    occluderEntry.MatchUserData = false;
    occluderEntry.BoundingBox = boundingBox;
    m_occluders.Add(occluderEntry);
}

void RenderQueue::AddOccluder(const RenderProxy *pRenderProxy, const AABox &boundingBox, const uint32 userData[], const void *const userDataPointer[])
{
    if (!m_acceptingOccluders)
        return;

    RENDER_QUEUE_OCCLUDER_ENTRY occluderEntry;
    occluderEntry.pRenderProxy = pRenderProxy;
    Y_memcpy(occluderEntry.UserData, userData, sizeof(occluderEntry.UserData));
    Y_memcpy(occluderEntry.UserDataPointer, userDataPointer, sizeof(occluderEntry.UserDataPointer));
    occluderEntry.MatchUserData = true;
    occluderEntry.BoundingBox = boundingBox;
    m_occluders.Add(occluderEntry);
}

void RenderQueue::AddDebugInfoObject(const RenderProxy *pRenderProxy)
{
    if (!m_acceptingDebugObjects)
        return;

    m_debugDrawObjects.Add(pRenderProxy);
    m_queueSize++;
}

void RenderQueue::InvalidateOpaqueRenderProxy(const RenderProxy *pRenderProxy)
{
    if (m_opaqueRenderables.GetSize() == 0)
        return;

    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_opaqueRenderables.GetBasePointer();
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_opaqueRenderables.GetBasePointer() + m_opaqueRenderables.GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        if (pQueueEntry->pRenderProxy == pRenderProxy)
        {
            pQueueEntry->RenderPassMask = 0;
            m_numObjectsInvalidatedByOcclusion++;
        }
    }

    pQueueEntry = m_translucentRenderables.GetBasePointer();
    pQueueEntryEnd = m_translucentRenderables.GetBasePointer() + m_translucentRenderables.GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        if (pQueueEntry->pRenderProxy == pRenderProxy)
        {
            pQueueEntry->RenderPassMask = 0;
            m_numObjectsInvalidatedByOcclusion++;
        }
    }
}

void RenderQueue::InvalidateOpaqueRenderProxy(const RenderProxy *pRenderProxy, const uint32 userData[], const void *const userDataPointer[])
{
    if (m_opaqueRenderables.GetSize() == 0)
        return;

    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_opaqueRenderables.GetBasePointer();
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_opaqueRenderables.GetBasePointer() + m_opaqueRenderables.GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        if (pQueueEntry->pRenderProxy == pRenderProxy && 
            Y_memcmp(pQueueEntry->UserData, userData, sizeof(pQueueEntry->UserData)) == 0 && 
            Y_memcmp(pQueueEntry->UserDataPointer, userDataPointer, sizeof(pQueueEntry->UserDataPointer)) == 0)
        {
            pQueueEntry->RenderPassMask = 0;
            m_numObjectsInvalidatedByOcclusion++;
        }
    }

    pQueueEntry = m_translucentRenderables.GetBasePointer();
    pQueueEntryEnd = m_translucentRenderables.GetBasePointer() + m_translucentRenderables.GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        if (pQueueEntry->pRenderProxy == pRenderProxy &&
            Y_memcmp(pQueueEntry->UserData, userData, sizeof(pQueueEntry->UserData)) == 0 &&
            Y_memcmp(pQueueEntry->UserDataPointer, userDataPointer, sizeof(pQueueEntry->UserDataPointer)) == 0)
        {
            pQueueEntry->RenderPassMask = 0;
            m_numObjectsInvalidatedByOcclusion++;
        }
    }
}

void RenderQueue::MarkRenderProxyWithPredicate(const RenderProxy *pRenderProxy, GPUQuery *pPredicate)
{
    if (m_opaqueRenderables.GetSize() == 0)
        return;

    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_opaqueRenderables.GetBasePointer();
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_opaqueRenderables.GetBasePointer() + m_opaqueRenderables.GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        if (pQueueEntry->pRenderProxy == pRenderProxy)
        {
            DebugAssert(pQueueEntry->pPredicate == nullptr);
            pQueueEntry->pPredicate = pPredicate;
            m_numObjectsInvalidatedByOcclusion++;
        }
    }

    pQueueEntry = m_translucentRenderables.GetBasePointer();
    pQueueEntryEnd = m_translucentRenderables.GetBasePointer() + m_translucentRenderables.GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        if (pQueueEntry->pRenderProxy == pRenderProxy)
        {
            DebugAssert(pQueueEntry->pPredicate == nullptr);
            pQueueEntry->pPredicate = pPredicate;
            m_numObjectsInvalidatedByOcclusion++;
        }
    }
}

void RenderQueue::MarkRenderProxyWithPredicate(const RenderProxy *pRenderProxy, const uint32 userData[], const void *const userDataPointer[], GPUQuery *pPredicate)
{
    if (m_opaqueRenderables.GetSize() == 0)
        return;

    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry = m_opaqueRenderables.GetBasePointer();
    RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntryEnd = m_opaqueRenderables.GetBasePointer() + m_opaqueRenderables.GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        if (pQueueEntry->pRenderProxy == pRenderProxy &&
            Y_memcmp(pQueueEntry->UserData, userData, sizeof(pQueueEntry->UserData)) == 0 &&
            Y_memcmp(pQueueEntry->UserDataPointer, userDataPointer, sizeof(pQueueEntry->UserDataPointer)) == 0)
        {
            DebugAssert(pQueueEntry->pPredicate == nullptr);
            pQueueEntry->pPredicate = pPredicate;
            m_numObjectsInvalidatedByOcclusion++;
        }
    }

    pQueueEntry = m_translucentRenderables.GetBasePointer();
    pQueueEntryEnd = m_translucentRenderables.GetBasePointer() + m_translucentRenderables.GetSize();
    for (; pQueueEntry != pQueueEntryEnd; pQueueEntry++)
    {
        if (pQueueEntry->pRenderProxy == pRenderProxy &&
            Y_memcmp(pQueueEntry->UserData, userData, sizeof(pQueueEntry->UserData)) == 0 &&
            Y_memcmp(pQueueEntry->UserDataPointer, userDataPointer, sizeof(pQueueEntry->UserDataPointer)) == 0)
        {
            DebugAssert(pQueueEntry->pPredicate == nullptr);
            pQueueEntry->pPredicate = pPredicate;
            m_numObjectsInvalidatedByOcclusion++;
        }
    }
}
/*
static int CompFunc(const RENDER_QUEUE_RENDERABLE_ENTRY *pLHS, const RENDER_QUEUE_RENDERABLE_ENTRY *pRHS)
{
    // bleh, unsigned ints make life harder
    if (pLHS->SortKey > pRHS->SortKey)
        return 1;
    else if (pLHS->SortKey < pRHS->SortKey)
        return -1;
    else
        return 0;
}*/

static bool CompFuncSTL(const RENDER_QUEUE_RENDERABLE_ENTRY &lhs, const RENDER_QUEUE_RENDERABLE_ENTRY &rhs)
{
    return (lhs.SortKey < rhs.SortKey);
}

void RenderQueue::Sort()
{
    if (m_opaqueRenderables.GetSize() > 0)
    {
        //Y_qsortT<RENDER_QUEUE_RENDERABLE_ENTRY>(m_opaqueRenderables.GetBasePointer(), m_opaqueRenderables.GetSize(), CompFunc);
        std::sort(m_opaqueRenderables.GetBasePointer(), m_opaqueRenderables.GetBasePointer() + m_opaqueRenderables.GetSize(), CompFuncSTL);
    }

    if (m_translucentRenderables.GetSize() > 0)
    {
        //Y_qsortT<RENDER_QUEUE_RENDERABLE_ENTRY>(m_translucentRenderables.GetBasePointer(), m_translucentRenderables.GetSize(), CompFunc);
        std::sort(m_translucentRenderables.GetBasePointer(), m_translucentRenderables.GetBasePointer() + m_translucentRenderables.GetSize(), CompFuncSTL);
    }
}

void RenderQueue::Clear()
{
    m_directionalLightArray.Clear();
    m_pointLightArray.Clear();
    m_spotLightArray.Clear();
    m_volumetricLightArray.Clear();
    m_opaqueRenderables.Clear();
    m_translucentRenderables.Clear();
    m_postProcessRenderables.Clear();
    m_occluders.Clear();
    m_debugDrawObjects.Clear();
    m_queueSize = 0;
    m_numObjectsInvalidatedByOcclusion = 0;
}
