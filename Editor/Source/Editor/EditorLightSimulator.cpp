#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorLightSimulator.h"
#include "Engine/ResourceManager.h"
#include "Engine/Texture.h"
#include "Renderer/RenderProxies/SpriteRenderProxy.h"
#include "Renderer/RenderProxies/DirectionalLightRenderProxy.h"
#include "Renderer/RenderProxies/PointLightRenderProxy.h"
#include "Renderer/RenderWorld.h"

EditorLightSimulator::EditorLightSimulator(uint32 entityID, RenderWorld *pRenderWorld)
    : m_lightType(LightType_Directional),
      m_pRenderWorld(pRenderWorld),
      m_showIndicator(true),
      m_pIndicatorRenderProxy(new SpriteRenderProxy(entityID)),
      m_color(float3::One),
      m_brightness(1.0f),
      m_castShadows(false),
      m_directionalLightDirection(float3::NegativeUnitZ),
      m_directionalLightAmbientFactor(0.2f),
      m_pDirectionalLightRenderProxy(new DirectionalLightRenderProxy(entityID)),
      m_pointLightLocation(float3::Zero),
      m_pointLightRange(16.0f),
      m_pointLightFalloff(1.0f),
      m_pPointLightRenderProxy(new PointLightRenderProxy(entityID))
{
    // set all settings
    UpdateIndicator();
    UpdateProxies();

    // add to render world
    m_pRenderWorld->AddRenderable(m_pIndicatorRenderProxy);
    m_pRenderWorld->AddRenderable(m_pDirectionalLightRenderProxy);
    m_pRenderWorld->AddRenderable(m_pPointLightRenderProxy);
}

EditorLightSimulator::~EditorLightSimulator()
{
    // remove from world
    m_pRenderWorld->RemoveRenderable(m_pPointLightRenderProxy);
    m_pRenderWorld->RemoveRenderable(m_pDirectionalLightRenderProxy);
    m_pRenderWorld->RemoveRenderable(m_pIndicatorRenderProxy);

    // release hold on objects
    SAFE_RELEASE(m_pPointLightRenderProxy);
    SAFE_RELEASE(m_pDirectionalLightRenderProxy);
    SAFE_RELEASE(m_pIndicatorRenderProxy);
}

void EditorLightSimulator::UpdateProxies()
{
    uint32 shadowFlags = (m_castShadows) ? (LIGHT_SHADOW_FLAG_CAST_DYNAMIC_SHADOWS) : (0);

    if (m_lightType == LightType_Directional)
    {
        m_pDirectionalLightRenderProxy->SetEnabled(true);
        m_pPointLightRenderProxy->SetEnabled(false);

        m_pDirectionalLightRenderProxy->SetLightColor(m_color * m_brightness * (1.0f - m_directionalLightAmbientFactor));
        m_pDirectionalLightRenderProxy->SetAmbientColor(m_color * m_brightness * m_directionalLightAmbientFactor);
        m_pDirectionalLightRenderProxy->SetShadowFlags(shadowFlags);
        m_pDirectionalLightRenderProxy->SetDirection(m_directionalLightDirection);
    }
    else if (m_lightType == LightType_Point)
    {
        m_pDirectionalLightRenderProxy->SetEnabled(true);
        m_pPointLightRenderProxy->SetEnabled(false);

        m_pPointLightRenderProxy->SetColor(m_color * m_brightness);
        m_pPointLightRenderProxy->SetShadowFlags(shadowFlags);
        m_pPointLightRenderProxy->SetPosition(m_pointLightLocation);
        m_pPointLightRenderProxy->SetRange(m_pointLightRange);
        m_pPointLightRenderProxy->SetFalloffExponent(m_pointLightFalloff);
    }
    else if (m_lightType == LightType_Spot)
    {
        m_pDirectionalLightRenderProxy->SetEnabled(false);
        m_pPointLightRenderProxy->SetEnabled(false);
    }
}

void EditorLightSimulator::ManipulateFromMouseMovement(const int2 &mousePositionDelta)
{
    if (m_lightType == LightType_Directional)
    {
        // manipulating the x and z axises
        float rotX = (float)mousePositionDelta.x;
        float rotY = (float)mousePositionDelta.y;

        // rotate the current rotation
        Quaternion rotMod((m_directionalLightRotation * Quaternion::FromEulerAngles(rotX, rotY, 0.0f)).Normalize());
        SetDirectionalLightRotation(rotMod);
    }
    else if (m_lightType == LightType_Point)
    {
        // move the light position, use 0.5 as a factor
        const float POS_FACTOR = 0.5;
        float modX = (float)mousePositionDelta.x * POS_FACTOR;
        float modY = (float)mousePositionDelta.y * POS_FACTOR;
        SetPointLightLocation(m_pointLightLocation + float3(modX, modY, 0.0f));
    }
    else if (m_lightType == LightType_Spot)
    {
    }
}

void EditorLightSimulator::UpdateIndicator()
{
    if (!m_showIndicator)
    {
        m_pIndicatorRenderProxy->SetVisibility(false);
        return;
    }

    if (m_lightType == LightType_Directional)
    {
        AutoReleasePtr<const Texture2D> pSpriteTexture = g_pResourceManager->GetTexture2D("textures/editor/light_directional");
        if (pSpriteTexture == nullptr)
            pSpriteTexture = g_pResourceManager->GetDefaultTexture2D();

        // calculate sprite position
        float3 spritePosition(m_directionalLightRotation * (float3::NegativeUnitZ * 5.0f));
        m_pIndicatorRenderProxy->SetPosition(spritePosition);
        m_pIndicatorRenderProxy->SetTexture(pSpriteTexture);
        m_pIndicatorRenderProxy->SetSizeByDimensions(1.0f, 1.0f);
        m_pIndicatorRenderProxy->SetVisibility(true);
    }
    else if (m_lightType == LightType_Point)
    {
        AutoReleasePtr<const Texture2D> pSpriteTexture = g_pResourceManager->GetTexture2D("textures/editor/light_point");
        if (pSpriteTexture == nullptr)
            pSpriteTexture = g_pResourceManager->GetDefaultTexture2D();

        // calculate sprite position
        m_pIndicatorRenderProxy->SetPosition(m_pointLightLocation);
        m_pIndicatorRenderProxy->SetTexture(pSpriteTexture);
        m_pIndicatorRenderProxy->SetSizeByDimensions(1.0f, 1.0f);
        m_pIndicatorRenderProxy->SetVisibility(true);
    }
    else if (m_lightType == LightType_Spot)
    {
        AutoReleasePtr<const Texture2D> pSpriteTexture = g_pResourceManager->GetTexture2D("textures/editor/light_point");
        if (pSpriteTexture == nullptr)
            pSpriteTexture = g_pResourceManager->GetDefaultTexture2D();

        // calculate sprite position
        m_pIndicatorRenderProxy->SetPosition(float3::Zero);
        m_pIndicatorRenderProxy->SetTexture(pSpriteTexture);
        m_pIndicatorRenderProxy->SetSizeByDimensions(1.0f, 1.0f);
        m_pIndicatorRenderProxy->SetVisibility(true);
    }
}

