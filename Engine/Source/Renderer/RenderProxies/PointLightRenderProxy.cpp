#include "Renderer/PrecompiledHeader.h"
#include "Renderer/RenderProxies/PointLightRenderProxy.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/Renderer.h"

PointLightRenderProxy::PointLightRenderProxy(uint32 entityId,
                                             const float3 &position /* = float3::Zero */,
                                             const float range /* = 64.0f */,
                                             const float3 &color /* = float3::One */,
                                             uint32 shadowFlags /* = 0 */,
                                             const float falloffExponent /* = 2.0f */)
    : RenderProxy(entityId),
      m_enabled(true),
      m_position(position),
      m_range(range),
      m_color(color),
      m_shadowFlags(shadowFlags),
      m_falloffExponent(falloffExponent)
{
    UpdateBounds();
}

PointLightRenderProxy::~PointLightRenderProxy()
{

}

void PointLightRenderProxy::QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    if (!m_enabled || !pRenderQueue->IsAcceptingLights())
        return;

    RENDER_QUEUE_POINT_LIGHT_ENTRY rcLight;
    rcLight.Position = m_position;
    rcLight.Range = m_range;
    rcLight.InverseRange = 1.0f / m_range;
    rcLight.LightColor = PixelFormatHelpers::ConvertSRGBToLinear(m_color);
    rcLight.FalloffExponent = m_falloffExponent;
    rcLight.ShadowFlags = m_shadowFlags;
    rcLight.ShadowMapIndex = -1;
    rcLight.Static = false;
    pRenderQueue->AddLight(&rcLight);
}

void PointLightRenderProxy::UpdateBounds()
{
    SetBounds(CalculatePointLightBoundingBox(m_position, m_range), CalculatePointLightBoundingSphere(m_position, m_range));
}

void PointLightRenderProxy::SetEnabled(bool newEnabled)
{
    if (!IsInWorld())
    {
        m_enabled = newEnabled;
    }
    else
    {
        ReferenceCountedHolder<PointLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, newEnabled]() {
            pThis->m_enabled = newEnabled;
        });
    }
}

void PointLightRenderProxy::SetPosition(const float3 &newPosition)
{
    if (!IsInWorld())
    {
        m_position = newPosition;
        UpdateBounds();
    }
    else
    {
        ReferenceCountedHolder<PointLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, newPosition]() 
        {
            pThis->m_position = newPosition;
            pThis->UpdateBounds();
        });
    }
}

void PointLightRenderProxy::SetRange(const float newRange)
{
    if (!IsInWorld())
    {
        m_range = newRange;
        UpdateBounds();
    }
    else
    {
        ReferenceCountedHolder<PointLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, newRange]()
        {
            pThis->m_range = newRange;
            pThis->UpdateBounds();
        });
    }
}

void PointLightRenderProxy::SetShadowFlags(uint32 newShadowFlags)
{
    if (!IsInWorld())
    {
        m_shadowFlags = newShadowFlags;
    }
    else
    {
        ReferenceCountedHolder<PointLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, newShadowFlags]() {
            pThis->m_shadowFlags = newShadowFlags;
        });
    }
}

void PointLightRenderProxy::SetColor(const float3 &newColor)
{
    if (!IsInWorld())
    {
        m_color = newColor;
    }
    else
    {
        ReferenceCountedHolder<PointLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, newColor]() {
            pThis->m_color = newColor;
        });
    }
}

void PointLightRenderProxy::SetFalloffExponent(const float newFalloffExponent)
{
    if (!IsInWorld())
    {
        m_falloffExponent = newFalloffExponent;
    }
    else
    {
        ReferenceCountedHolder<PointLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, newFalloffExponent]() {
            pThis->m_falloffExponent = newFalloffExponent;
        });
    }
}

AABox PointLightRenderProxy::CalculatePointLightBoundingBox(const float3 &position, float range)
{
    SIMDVector3f lightPosition(position);
    SIMDVector3f lightRange(range, range, range);
    SIMDVector3f minBounds(lightPosition - lightRange);
    SIMDVector3f maxBounds(lightPosition + lightRange);

    return AABox(minBounds, maxBounds);
}

Sphere PointLightRenderProxy::CalculatePointLightBoundingSphere(const float3 &position, float range)
{
    return Sphere(position, range);
}
