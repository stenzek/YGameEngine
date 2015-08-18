#include "Renderer/PrecompiledHeader.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/Renderer.h"

RenderWorld::RenderWorld()
{

}

RenderWorld::~RenderWorld()
{
#if RENDER_WORLD_USE_LINKED_LIST
    for (NodeList::Iterator itr = m_nodes.Begin(); !itr.AtEnd(); )
    {
        RenderProxy *pRenderProxy = itr->pRenderProxy;
        pRenderProxy->OnRemoveFromRenderWorld(this);
        pRenderProxy->m_pRenderWorld = NULL;
        pRenderProxy->Release();

        itr = m_nodes.EraseIterator(itr);
    }
#else
    while (m_nodes.GetSize() > 0)
    {
        Node &node = m_nodes[m_nodes.GetSize() - 1];
        node.pRenderProxy->OnRemoveFromRenderWorld(this);
        node.pRenderProxy->m_pRenderWorld = nullptr;
        node.pRenderProxy->Release();
        m_nodes.FastRemove(m_nodes.GetSize() - 1);
    }
    m_nodes.Obliterate();
#endif

    // should be empty
    Assert(m_nodes.GetSize() == 0);
}

void RenderWorld::AddRenderable(RenderProxy *pRenderProxy)
{
    // bind it to the render world, that way any modifications that happen
    // to the proxy will be queued
    DebugAssert(pRenderProxy->m_pRenderWorld == nullptr);
    pRenderProxy->m_pRenderWorld = this;
    pRenderProxy->AddRef();

    // queue the add command
    ReferenceCountedHolder<RenderWorld> pThis(this);
    QUEUE_RENDERER_LAMBDA_COMMAND([pRenderProxy, pThis]()
    {
        Node node;
        node.BoundingBox = pRenderProxy->GetBoundingBox();
        node.BoundingSphere = pRenderProxy->GetBoundingSphere();
        node.pRenderProxy = pRenderProxy;

#if RENDER_WORLD_USE_LINKED_LIST
        pThis->m_nodes.PushBack(node);
#else
        pThis->m_nodes.Add(node);
#endif
    });
}

void RenderWorld::RemoveRenderable(RenderProxy *pRenderProxy)
{
    DebugAssert(pRenderProxy->m_pRenderWorld == this);

    ReferenceCountedHolder<RenderWorld> pThis(this);
    QUEUE_RENDERER_LAMBDA_COMMAND([pRenderProxy, pThis]()
    {
#if RENDER_WORLD_USE_LINKED_LIST

        NodeList::Iterator itr;
        for (itr = m_nodes.Begin(); !itr.AtEnd(); itr.Forward())
        {
            if (itr->pRenderProxy == pRenderProxy)
                break;
        }

        Assert(!itr.AtEnd());
        pRenderProxy->OnRemoveFromRenderWorld(pThis);
        pThis->m_nodes.Erase(itr);

        pRenderProxy->m_pRenderWorld = NULL;
        pRenderProxy->Release();

#else

        for (uint32 i = 0; i < pThis->m_nodes.GetSize(); i++)
        {
            Node &node = pThis->m_nodes[i];
            if (node.pRenderProxy == pRenderProxy)
            {
                pThis->m_nodes.FastRemove(i);
                pRenderProxy->m_pRenderWorld = NULL;
                pRenderProxy->Release();
                return;
            }
        }

        Panic("Attempt to remove renderable not in render world");

#endif
    });
}

void RenderWorld::MoveRenderable(RenderProxy *pRenderProxy)
{
    DebugAssert(Renderer::IsOnRenderThread());

#if RENDER_WORLD_USE_LINKED_LIST

    NodeList::Iterator itr = m_nodes.Begin();
    for (; !itr.AtEnd(); itr.Forward())
    {
        if (itr->pRenderProxy == pRenderProxy)
        {
            itr->BoundingBox = pRenderProxy->GetBoundingBox();
            itr->BoundingSphere = pRenderProxy->GetBoundingSphere();
            return;
        }
    }

#else

    for (uint32 i = 0; i < m_nodes.GetSize(); i++)
    {
        Node &node = m_nodes[i];
        if (node.pRenderProxy == pRenderProxy)
        {
            node.BoundingBox = pRenderProxy->GetBoundingBox();
            node.BoundingSphere = pRenderProxy->GetBoundingSphere();
            return;
        }
    }

#endif

    Panic("Attempting to update renderable not in world.");
}
