#pragma once
#include "Renderer/Common.h"
#include "Renderer/RenderProxy.h"

class Texture2D;
class Material;

class SpriteRenderProxy : public RenderProxy
{
public:
    SpriteRenderProxy(uint32 entityId, const Texture2D *pTexture = nullptr, const float3 &position = float3::Zero);
    ~SpriteRenderProxy();

    // update functions, can be called from game thread safely after adding to render world.
    void SetTexture(const Texture2D *pTexture);
    void SetPosition(const float3 &position);
    void SetSizeByTextureScale(float textureScale);
    void SetSizeByDimensions(float width, float height);
    void SetTintColor(bool enabled, uint32 color = 0);
    void SetVisibility(bool visible);
    void SetShadowFlags(uint32 shadowFlags);

    virtual void QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const override;
    virtual void DrawQueueEntry(const Camera *pCamera, const RENDER_QUEUE_RENDERABLE_ENTRY *pQueueEntry, GPUCommandList *pCommandList) const override;
    virtual bool CreateDeviceResources() const override;
    virtual void ReleaseDeviceResources() const override;

private:
    // real methods
    void RealSetTexture(const Texture2D *pTexture);
    void RealSetPosition(const float3 &position);
    void RealSetSizeByTextureScale(float textureScale);
    void RealSetSizeByDimensions(float width, float height);
    void RealSetTintColor(bool enabled, uint32 color = 0);
    void RealSetVisibility(bool visible);
    void RealSetShadowFlags(uint32 shadowFlags);

    // update bounds, call when setting a new transform.
    void UpdateBounds();
    //void UpdateVertexBuffer();

    // material that we are rendering with
    Material *m_pMaterial;

    // read/write from render thread. no access from game thread.
    bool m_visibility;
    const Texture2D *m_pTexture;
    float3 m_position;
    float m_width;
    float m_height;
    uint32 m_shadowFlags;
    bool m_tintEnabled;
    uint32 m_tintColor;
    
    // gpu resources, also owned by render thread.
    mutable bool m_bGPUResourcesCreated;
    //mutable GPUVertexArray *m_pVertexArray;
};

