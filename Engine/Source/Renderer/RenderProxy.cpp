#include "Renderer/PrecompiledHeader.h"
#include "Renderer/RenderProxy.h"
#include "Renderer/RenderWorld.h"

RenderProxy::RenderProxy(uint32 entityId) 
    : m_iEntityId(entityId), 
      m_boundingBox(AABox::Zero), 
      m_boundingSphere(Sphere::Zero),
      m_pRenderWorld(NULL)
{

}

RenderProxy::~RenderProxy()
{
    DebugAssert(m_pRenderWorld == NULL);
}

void RenderProxy::SetBounds(const AABox &boundingBox, const Sphere &boundingSphere)
{
    if (m_boundingBox != boundingBox || m_boundingSphere != boundingSphere)
    {
        m_boundingBox = boundingBox;
        m_boundingSphere = boundingSphere;
        if (m_pRenderWorld != NULL)
            m_pRenderWorld->MoveRenderable(this);
    }
}

