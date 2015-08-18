#include "Renderer/PrecompiledHeader.h"
#include "Renderer/RenderProxies/DirectionalLightRenderProxy.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/Renderer.h"

DirectionalLightRenderProxy::DirectionalLightRenderProxy(uint32 entityId,
                                                         bool enabled /* = true */,
                                                         const float3 &lightColor /* = float3::One */,
                                                         const float3 &ambientColor /* = float3::Zero */,
                                                         uint32 shadowFlags /* = 0 */,
                                                         const float3 &direction /* = float3::NegativeUnitZ */)
    : RenderProxy(entityId),
      m_enabled(enabled),
      m_lightColor(lightColor),
      m_ambientColor(ambientColor),
      m_shadowFlags(shadowFlags),
      m_direction(direction)
{
    // set bounds. this is +/- float max, as a directional light will light *everything* in the level.
    SetBounds(AABox::MaxSize, Sphere::MaxSize);
}

DirectionalLightRenderProxy::~DirectionalLightRenderProxy()
{

}

// !!!!!!!!!!
// TODO: Move draw into DrawQueueEntry()!
// !!!!!!!!!!
void DirectionalLightRenderProxy::QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    // check enabled
    if (!m_enabled || !pRenderQueue->IsAcceptingLights())
        return;

    RENDER_QUEUE_DIRECTIONAL_LIGHT_ENTRY rcLight;
    rcLight.LightColor = PixelFormatHelpers::ConvertSRGBToLinear(m_lightColor);
    rcLight.AmbientColor = PixelFormatHelpers::ConvertSRGBToLinear(m_ambientColor);
    rcLight.Direction = m_direction;
    rcLight.ShadowFlags = m_shadowFlags;
    rcLight.ShadowMapIndex = -1;
    rcLight.Static = false;
    pRenderQueue->AddLight(&rcLight);
}

void DirectionalLightRenderProxy::SetEnabled(bool newEnabled)
{
    if (!IsInWorld())
    {
        m_enabled = newEnabled;
    }
    else
    {
        ReferenceCountedHolder<DirectionalLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, newEnabled]() {
            pThis->m_enabled = newEnabled;
        });
    }
}

void DirectionalLightRenderProxy::SetLightColor(const float3 &newColor)
{
    if (!IsInWorld())
    {
        m_lightColor = newColor;
    }
    else
    {
        ReferenceCountedHolder<DirectionalLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, newColor]() {
            pThis->m_lightColor = newColor;
        });
    }
}

void DirectionalLightRenderProxy::SetAmbientColor(const float3 &newColor)
{
    if (!IsInWorld())
    {
        m_ambientColor = newColor;
    }
    else
    {
        ReferenceCountedHolder<DirectionalLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, newColor]() {
            pThis->m_ambientColor = newColor;
        });
    }
}

void DirectionalLightRenderProxy::SetShadowFlags(uint32 newShadowFlags)
{
    if (!IsInWorld())
    {
        m_shadowFlags = newShadowFlags;
    }
    else
    {
        ReferenceCountedHolder<DirectionalLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, newShadowFlags]() {
            pThis->m_shadowFlags = newShadowFlags;
        });
    }
}

void DirectionalLightRenderProxy::SetDirection(const float3 &newDirection)
{
    if (!IsInWorld())
    {
        m_direction = newDirection;
    }
    else
    {
        ReferenceCountedHolder<DirectionalLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, newDirection]() {
            pThis->m_direction = newDirection;
        });
    }
}
