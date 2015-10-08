#pragma once
#include "Renderer/Common.h"
#include "Renderer/RenderQueue.h"

class Camera;
class RenderWorld;
class GPUCommandList;
class MiniGUIContext;
class ShaderProgram;

class RenderProxy : public ReferenceCounted
{
    friend class RenderWorld;

public:
    RenderProxy(uint32 entityId);
    virtual ~RenderProxy();

    uint32 GetEntityId() const { return m_iEntityId; }
    bool IsInWorld() const { return (m_pRenderWorld != nullptr); }
    const AABox &GetBoundingBox() const { return m_boundingBox; }
    const Sphere &GetBoundingSphere() const { return m_boundingSphere; }
    RenderWorld *GetRenderWorld() const { return m_pRenderWorld; }

    void SetEntityID(uint32 entityId) { m_iEntityId = entityId; }
    void SetBounds(const AABox &boundingBox, const Sphere &boundingSphere);

    virtual void OnAddToRenderWorld(RenderWorld *pRenderWorld) { }
    virtual void OnRemoveFromRenderWorld(RenderWorld *pRenderWorld) { }

    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const { }
    virtual void SetupForDraw(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList, ShaderProgram *pShaderProgram) const { }
    virtual void DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList) const { }

    virtual void DrawDebugInfo(const Camera *pCamera, GPUCommandList *pCommandList, MiniGUIContext *pGUIContext) const { }

    virtual bool CreateDeviceResources() const { return true; }
    virtual void ReleaseDeviceResources() const { }

    // todo: (for decals)
    // RayCast
    // GetIntersectingTriangles

    virtual bool RayCast(const Ray &ray, float3 &contactNormal, float3 &contactPoint, bool exitAtFirstIntersection) const { return false; }

    struct IntersectingTriangle
    {
        float3 Vertices[3];
        float3 Normal;

        IntersectingTriangle() {}
        IntersectingTriangle(const float3 &v0, const float3 &v1, const float3 &v2, const float3 &normal)
        {
            Vertices[0] = v0;
            Vertices[1] = v1;
            Vertices[2] = v2;
            Normal = normal;
        }
    };
    typedef MemArray<IntersectingTriangle> IntersectingTriangleArray;
    virtual uint32 GetIntersectingTriangles(const AABox &searchBox, IntersectingTriangleArray &intersectingTriangles) const { return 0; }

private:
    uint32 m_iEntityId;
    AABox m_boundingBox;
    Sphere m_boundingSphere;
    RenderWorld *m_pRenderWorld;
};

