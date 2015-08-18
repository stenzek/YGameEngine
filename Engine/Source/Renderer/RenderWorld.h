#pragma once
#include "Renderer/Common.h"
#include "Renderer/RenderProxy.h"

//#define RENDER_WORLD_USE_LINKED_LIST 1

class RenderWorld : public ReferenceCounted
{
public:
    RenderWorld();
    ~RenderWorld();

    // Can be called from game thread.
    void AddRenderable(RenderProxy *pRenderProxy);
    void RemoveRenderable(RenderProxy *pRenderProxy);

    // Can be called from render thread.
    void MoveRenderable(RenderProxy *pRenderProxy);

#if RENDER_WORLD_USE_LINKED_LIST
    // enumerators
    template<typename T>
    void EnumerateRenderables(T &Callback)
    {
        NodeList::Iterator itr = m_nodes.Begin();
        for (; !itr.AtEnd(); itr.Forward())
            Callback(itr->pRenderProxy);
    }
    template<typename T>
    void EnumerateRenderables(T &Callback) const
    {
        NodeList::ConstIterator itr = m_nodes.Begin();
        for (; !itr.AtEnd(); itr.Forward())
            Callback(itr->pRenderProxy);
    }
    template<typename T>
    void EnumerateRenderablesInAABox(const AABox &aaBox, T Callback)
    {
        NodeList::Iterator itr = m_nodes.Begin();
        for (; !itr.AtEnd(); itr.Forward())
        {
            if (aaBox.AABoxIntersection(itr->BoundingBox))
                Callback(itr->pRenderProxy);
        }
    }
    template<typename T>
    void EnumerateRenderablesForEntity(uint32 entityId, T Callback)
    {
        NodeList::Iterator itr = m_nodes.Begin();
        for (; !itr.AtEnd(); itr.Forward())
        {
            Node &node = *itr;
            if (node.pRenderProxy->GetEntityId() == entityId)
                Callback(node.pRenderProxy);
        }
    }
    template<typename T>
    void EnumerateRenderablesInAABox(const AABox &aaBox, T Callback) const
    {
        NodeList::ConstIterator itr = m_nodes.Begin();
        for (; !itr.AtEnd(); itr.Forward())
        {
            if (aaBox.AABoxIntersection(itr->BoundingBox))
                Callback(itr->pRenderProxy);
        }
    }
    template<typename T>
    void EnumerateRenderablesInFrustum(const Frustum &rFrustum, T Callback)
    {
        NodeList::Iterator itr = m_nodes.Begin();
        for (; !itr.AtEnd(); itr.Forward())
        {
            //if (rFrustum.SphereIntersection(itr->BoundingSphere) && rFrustum.AABoxIntersection(itr->BoundingBox))
            if (rFrustum.AABoxIntersection(itr->BoundingBox))
                Callback(itr->pRenderProxy);
        }
    }
    template<typename T>
    void EnumerateRenderablesInFrustum(const Frustum &rFrustum, T Callback) const
    {
        NodeList::ConstIterator itr = m_nodes.Begin();
        for (; !itr.AtEnd(); itr.Forward())
        {
            //if (rFrustum.SphereIntersection(itr->BoundingSphere) && rFrustum.AABoxIntersection(itr->BoundingBox))
            if (rFrustum.AABoxIntersection(itr->BoundingBox))
                Callback(itr->pRenderProxy);
        }
    }
    template<typename T>
    void EnumerateRenderablesForEntity(uint32 entityId, T Callback) const
    {
        NodeList::ConstIterator itr = m_nodes.Begin();
        for (; !itr.AtEnd(); itr.Forward())
        {
            const Node &node = *itr;
            if (node.pRenderProxy->GetEntityId() == entityId)
                Callback(node.pRenderProxy);
        }
    }

    bool RayCast(const Ray &ray, float3 &contactNormal, float3 &contactPoint, bool exitAtFirstIntersection) const
    {
        float closestDistance = Y_FLT_INFINITE;
        float3 closestNormal, closestPoint;

        NodeList::ConstIterator itr = m_nodes.Begin();
        for (; !itr.AtEnd(); itr.Forward())
        {
            const Node &node = *itr;
            float3 nodeContactNormal, nodeContactPoint;
            if (node.pRenderProxy->RayCast(ray, nodeContactNormal, nodeContactPoint, exitAtFirstIntersection))
            {
                if (exitAtFirstIntersection)
                {
                    contactNormal = nodeContactNormal;
                    contactPoint = nodeContactPoint;
                    return true;
                }

                float nodeDistance = (nodeContactPoint - ray.GetOrigin()).SquaredLength();
                if (nodeDistance < closestDistance)
                {
                    closestDistance = nodeDistance;
                    closestNormal = nodeContactNormal;
                    closestPoint = nodeContactPoint;
                }
            }
        }

        if (closestDistance == Y_FLT_INFINITE)
            return false;

        contactNormal = closestNormal;
        contactPoint = closestPoint;
        return true;
    }

    void GetIntersectingTrianglesInAABox(const AABox &aaBox, RenderProxy::IntersectingTriangleArray &intersectingTriangles) const
    {
        NodeList::ConstIterator itr = m_nodes.Begin();
        for (; !itr.AtEnd(); itr.Forward())
        {
            const Node &node = *itr;
            if (node.pRenderProxy->GetBoundingBox().AABoxIntersection(aaBox))
                node.pRenderProxy->GetIntersectingTriangles(aaBox, intersectingTriangles);
        }
    }

#else
    // enumerators
    template<typename T>
    void EnumerateRenderables(T &Callback)
    {
        for (uint32 i = 0; i < m_nodes.GetSize(); i++)
            Callback(m_nodes[i].pRenderProxy);
    }
    template<typename T>
    void EnumerateRenderables(T &Callback) const
    {
        for (uint32 i = 0; i < m_nodes.GetSize(); i++)
            Callback(m_nodes[i].pRenderProxy);
    }
    template<typename T>
    void EnumerateRenderablesInAABox(const AABox &aaBox, T Callback)
    {
        for (uint32 i = 0; i < m_nodes.GetSize(); i++)
        {
            Node &node = m_nodes[i];
            if (aaBox.AABoxIntersection(node.BoundingBox))
                Callback(node.pRenderProxy);
        }
    }
    template<typename T>
    void EnumerateRenderablesForEntity(uint32 entityId, T Callback)
    {
        for (uint32 i = 0; i < m_nodes.GetSize(); i++)
        {
            Node &node = m_nodes[i];
            if (node.pRenderProxy->GetEntityId() == entityId)
                Callback(node.pRenderProxy);
        }
    }
    template<typename T>
    void EnumerateRenderablesInAABox(const AABox &aaBox, T Callback) const
    {
        for (uint32 i = 0; i < m_nodes.GetSize(); i++)
        {
            const Node &node = m_nodes[i];
            if (aaBox.AABoxIntersection(node.BoundingBox))
                Callback(node.pRenderProxy);
        }
    }
    template<typename T>
    void EnumerateRenderablesInFrustum(const Frustum &rFrustum, T Callback)
    {
        for (uint32 i = 0; i < m_nodes.GetSize(); i++)
        {
            Node &node = m_nodes[i];
            if (rFrustum.SphereIntersection(node.BoundingSphere) && rFrustum.AABoxIntersection(node.BoundingBox))
            //if (rFrustum.AABoxIntersection(node.BoundingBox))
                Callback(node.pRenderProxy);
        }
    }
    template<typename T>
    void EnumerateRenderablesInFrustum(const Frustum &rFrustum, T Callback) const
    {
        for (uint32 i = 0; i < m_nodes.GetSize(); i++)
        {
            const Node &node = m_nodes[i];
            if (rFrustum.SphereIntersection(node.BoundingSphere) && rFrustum.AABoxIntersection(node.BoundingBox))
            //if (rFrustum.AABoxIntersection(node.BoundingBox))
                Callback(node.pRenderProxy);
        }
    }
    template<typename T>
    void EnumerateRenderablesForEntity(uint32 entityId, T Callback) const
    {
        for (uint32 i = 0; i < m_nodes.GetSize(); i++)
        {
            const Node &node = m_nodes[i];
            if (node.pRenderProxy->GetEntityId() == entityId)
                Callback(node.pRenderProxy);
        }
    }

    bool RayCast(const Ray &ray, float3 &contactNormal, float3 &contactPoint, bool exitAtFirstIntersection) const
    {
        float closestDistance = Y_FLT_INFINITE;
        float3 closestNormal, closestPoint;

        for (uint32 i = 0; i < m_nodes.GetSize(); i++)
        {
            const Node &node = m_nodes[i];
            float3 nodeContactNormal, nodeContactPoint;
            if (node.pRenderProxy->RayCast(ray, nodeContactNormal, nodeContactPoint, exitAtFirstIntersection))
            {
                if (exitAtFirstIntersection)
                {
                    contactNormal = nodeContactNormal;
                    contactPoint = nodeContactPoint;
                    return true;
                }

                float nodeDistance = (nodeContactPoint - ray.GetOrigin()).SquaredLength();
                if (nodeDistance < closestDistance)
                {
                    closestDistance = nodeDistance;
                    closestNormal = nodeContactNormal;
                    closestPoint = nodeContactPoint;
                }
            }
        }

        if (closestDistance == Y_FLT_INFINITE)
            return false;

        contactNormal = closestNormal;
        contactPoint = closestPoint;
        return true;
    }

    void GetIntersectingTrianglesInAABox(const AABox &aaBox, RenderProxy::IntersectingTriangleArray &intersectingTriangles) const
    {
        for (uint32 i = 0; i < m_nodes.GetSize(); i++)
        {
            const Node &node = m_nodes[i];
            if (node.pRenderProxy->GetBoundingBox().AABoxIntersection(aaBox))
                node.pRenderProxy->GetIntersectingTriangles(aaBox, intersectingTriangles);
        }
    }
#endif
    
private:
    struct Node
    {
        // TODO: Look at storing entity here, using it for grouping?
        AABox BoundingBox;
        Sphere BoundingSphere;
        RenderProxy *pRenderProxy;
    };

#if RENDER_WORLD_USE_LINKED_LIST
    typedef List<Node> NodeList;
#else
    typedef MemArray<Node> NodeList;
#endif
    typedef PODArray<RenderProxy *> RenderProxyQueue;

    // Owned by render thread at async run time.
    // Owned by game thread at synchronization time.
    NodeList m_nodes;
};

