#include "Renderer/PrecompiledHeader.h"
#include "Renderer/RenderProxies/VolumetricLightRenderProxy.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/Renderer.h"

VolumetricLightRenderProxy::VolumetricLightRenderProxy(uint32 entityId,
                                           bool enabled,
                                           const float3 &color,
                                           const float3 &position,
                                           float falloffRate)
    : RenderProxy(entityId),
      m_enabled(enabled),
      m_color(color),
      m_position(position),
      m_falloffRate(falloffRate),
      m_renderRange(1.0f),
      m_primitive(VOLUMETRIC_LIGHT_PRIMITIVE_SPHERE),
      m_boxPrimitiveExtents(float3::Zero),
      m_spherePrimitiveRadius(1.0f)
{
    UpdateBounds();
}

VolumetricLightRenderProxy::~VolumetricLightRenderProxy()
{

}

void VolumetricLightRenderProxy::UpdateBounds()
{
    AABox boundingBox;
    Sphere boundingSphere;
    float renderRange;

    switch (m_primitive)
    {
    case VOLUMETRIC_LIGHT_PRIMITIVE_BOX:
        boundingBox.SetBounds(m_position - m_boxPrimitiveExtents, m_position + m_boxPrimitiveExtents);
        boundingSphere = Sphere::FromAABox(boundingBox);
        renderRange = m_boxPrimitiveExtents.Length();
        break;

    case VOLUMETRIC_LIGHT_PRIMITIVE_SPHERE:
        boundingSphere.SetCenterAndRadius(m_position, m_spherePrimitiveRadius);
        boundingBox = AABox::FromSphere(boundingSphere);
        renderRange = m_spherePrimitiveRadius;
        break;

    default:
        UnreachableCode();
        return;
    }

    m_renderRange = renderRange;
    SetBounds(boundingBox, boundingSphere);
}

void VolumetricLightRenderProxy::QueueForRender(const Camera *pCamera, RenderQueue *pRenderQueue) const
{
    // check enabled
    if (!m_enabled || !pRenderQueue->IsAcceptingLights())
        return;

    RENDER_QUEUE_VOLUMETRIC_LIGHT_ENTRY rcLight;
    rcLight.Position = m_position;
    rcLight.Range = m_renderRange;
    rcLight.LightColor = PixelFormatHelpers::ConvertSRGBToLinear(m_color);
    rcLight.Primitive = m_primitive;
    rcLight.FalloffRate = m_falloffRate;
    rcLight.BoxExtents = m_boxPrimitiveExtents;
    rcLight.SphereRadius = m_spherePrimitiveRadius;
    rcLight.Static = false;

    pRenderQueue->AddLight(&rcLight);
}

void VolumetricLightRenderProxy::SetEnabled(bool newEnabled)
{
    if (!IsInWorld())
    {
        m_enabled = newEnabled;
    }
    else
    {
        ReferenceCountedHolder<VolumetricLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, newEnabled]() {
            pThis->m_enabled = newEnabled;
        });
    }
}

void VolumetricLightRenderProxy::SetColor(const float3 &newColor)
{
    if (!IsInWorld())
    {
        m_color = newColor;
    }
    else
    {
        ReferenceCountedHolder<VolumetricLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, newColor]() {
            pThis->m_color = newColor;
        });
    }
}

void VolumetricLightRenderProxy::SetPosition(const float3 &position)
{
    if (!IsInWorld())
    {
        m_position = position;
        UpdateBounds();
    }
    else
    {
        ReferenceCountedHolder<VolumetricLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, position]() {
            pThis->m_position = position;
            pThis->UpdateBounds();
        });
    }
}

void VolumetricLightRenderProxy::SetFalloffRate(float falloffRate)
{
    if (!IsInWorld())
    {
        m_falloffRate = falloffRate;
    }
    else
    {
        ReferenceCountedHolder<VolumetricLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, falloffRate]() {
            pThis->m_falloffRate = falloffRate;
        });
    }
}

void VolumetricLightRenderProxy::SetPrimitive(VOLUMETRIC_LIGHT_PRIMITIVE primitive)
{
    if (!IsInWorld())
    {
        DebugAssert(primitive < VOLUMETRIC_LIGHT_PRIMITIVE_COUNT);
        m_primitive = primitive;
        UpdateBounds();
    }
    else
    {
        ReferenceCountedHolder<VolumetricLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, primitive]()
        {
            DebugAssert(primitive < VOLUMETRIC_LIGHT_PRIMITIVE_COUNT);
            pThis->m_primitive = primitive;
            pThis->UpdateBounds();
        });
    }
}

void VolumetricLightRenderProxy::SetBoxPrimitiveExtents(float3 boxExtents)
{
    if (!IsInWorld())
    {
        DebugAssert(m_primitive == VOLUMETRIC_LIGHT_PRIMITIVE_SPHERE);
        m_boxPrimitiveExtents = boxExtents;
        UpdateBounds();
    }
    else
    {
        ReferenceCountedHolder<VolumetricLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, boxExtents]() 
        {
            DebugAssert(pThis->m_primitive == VOLUMETRIC_LIGHT_PRIMITIVE_SPHERE);
            pThis->m_boxPrimitiveExtents = boxExtents;
            pThis->UpdateBounds();
        });
    }

    DebugAssert(!IsInWorld() || Renderer::IsOnRenderThread());

}

void VolumetricLightRenderProxy::SetSpherePrimitiveRadius(float sphereRadius)
{
    if (!IsInWorld())
    {
        DebugAssert(m_primitive == VOLUMETRIC_LIGHT_PRIMITIVE_SPHERE);
        m_spherePrimitiveRadius = sphereRadius;
        UpdateBounds();
    }
    else
    {
        ReferenceCountedHolder<VolumetricLightRenderProxy> pThis(this);
        QUEUE_RENDERER_LAMBDA_COMMAND([pThis, sphereRadius]() 
        {
            DebugAssert(pThis->m_primitive == VOLUMETRIC_LIGHT_PRIMITIVE_SPHERE);
            pThis->m_spherePrimitiveRadius = sphereRadius;
            pThis->UpdateBounds();
        });
    }
}
