#include "Editor/PrecompiledHeader.h"
#include "Editor/EditorVisualComponent.h"
#include "MapCompiler/MapSource.h"
#include "Engine/Entity.h"
#include "Engine/ResourceManager.h"
#include "Renderer/RenderProxies/SpriteRenderProxy.h"
#include "Renderer/RenderProxies/StaticMeshRenderProxy.h"
#include "Renderer/RenderProxies/BlockMeshRenderProxy.h"
#include "Renderer/RenderProxies/PointLightRenderProxy.h"
#include "Renderer/RenderProxies/DirectionalLightRenderProxy.h"
#include "Renderer/RenderWorld.h"
#include "YBaseLib/XMLReader.h"

static Quaternion ParseRotationValue(const char *rotationValue)
{
    uint32 rvLen = Y_strlen(rotationValue);
    char *rvCopy = (char *)alloca(rvLen + 1);
    Y_memcpy(rvCopy, rotationValue, rvLen + 1);

    if (!Y_strnicmp(rvCopy, "euler(", 6))
    {
        char *tokens[3];
        if (Y_strsplit3(rvCopy, ',', tokens, countof(tokens)) == 3)
        {
            float pitch = StringConverter::StringToFloat(tokens[0]);
            float yaw = StringConverter::StringToFloat(tokens[1]);
            float roll = StringConverter::StringToFloat(tokens[2]);
            return Quaternion::FromEulerAngles(pitch, yaw, roll);
        }
        else
        {
            return Quaternion::Identity;
        }
    }

    return StringConverter::StringToQuaternion(rvCopy);
}

EditorVisualComponentDefinition::EditorVisualComponentDefinition()
{

}

EditorVisualComponentDefinition::~EditorVisualComponentDefinition()
{

}

EditorVisualComponentInstance::EditorVisualComponentInstance(uint32 pickingID)
    : m_pickingID(pickingID),
      m_visualBounds(AABox::Zero)
{

}

EditorVisualComponentInstance::~EditorVisualComponentInstance()
{

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EditorVisualComponentDefinition_DirectionalLight::EditorVisualComponentDefinition_DirectionalLight()
{
    m_bEnabled = true;
    m_Rotation = Quaternion::Identity;
    m_RotationOffset = Quaternion::Identity;
    m_Color = float3::One;
    m_Brightness = 1.0f;
    m_iShadowFlags = 0;
    m_fAmbientFactor = 0.2f;
    m_bOnlyShowWhenSelected = false;
}

EditorVisualComponentDefinition_DirectionalLight::~EditorVisualComponentDefinition_DirectionalLight()
{

}

EditorVisualComponentDefinition *EditorVisualComponentDefinition_DirectionalLight::Clone() const
{
    EditorVisualComponentDefinition_DirectionalLight *pClone = new EditorVisualComponentDefinition_DirectionalLight();

    pClone->m_bEnabled = m_bEnabled;
    pClone->m_strEnabledFromPropertyName = m_strEnabledFromPropertyName;
    pClone->m_Rotation = m_Rotation;
    pClone->m_strRotationFromPropertyName = m_strRotationFromPropertyName;
    pClone->m_RotationOffset = m_RotationOffset;
    pClone->m_Color = m_Color;
    pClone->m_strColorFromPropertyName = m_strColorFromPropertyName;
    pClone->m_Brightness = m_Brightness;
    pClone->m_strBrightnessFromPropertyName = m_strBrightnessFromPropertyName;
    pClone->m_iShadowFlags = m_iShadowFlags;
    pClone->m_strShadowFlagsFromPropertyName = m_strShadowFlagsFromPropertyName;
    pClone->m_fAmbientFactor = m_fAmbientFactor;
    pClone->m_strAmbientFactorFromPropertyName = m_strAmbientFactorFromPropertyName;
    pClone->m_bOnlyShowWhenSelected = m_bOnlyShowWhenSelected;

    return pClone;
}

bool EditorVisualComponentDefinition_DirectionalLight::ParseXML(XMLReader &xmlReader)
{
    if (!xmlReader.IsEmptyElement())
    {
        for (; ;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 visualSelection = xmlReader.Select("enabled|rotation|color|brightness|shadow-flags|ambient-factor|only-show-when-selected");
                if (visualSelection < 0)
                    return false;

                switch (visualSelection)
                {
                    // enabled
                case 0:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_bEnabled = StringConverter::StringToBool(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strEnabledFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // rotation
                case 1:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *offsetStr = xmlReader.FetchAttribute("offset");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_Rotation = ParseRotationValue(valueStr);

                        if (offsetStr != NULL)
                            m_RotationOffset = ParseRotationValue(offsetStr);

                        if (fromPropertyStr != NULL)
                            m_strRotationFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // color
                case 2:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_Color = StringConverter::StringToFloat3(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strColorFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // brightness
                case 3:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_Brightness = StringConverter::StringToFloat(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strBrightnessFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // shadow-flags
                case 4:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_iShadowFlags = StringConverter::StringToUInt32(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strShadowFlagsFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // ambient-factor
                case 5:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_fAmbientFactor = StringConverter::StringToFloat(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strAmbientFactorFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // only-show-when-selected
                case 6:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        if (valueStr != NULL)
                            m_bOnlyShowWhenSelected = StringConverter::StringToBool(valueStr);
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "visual") == 0);
                break;
            }
            else
            {
                UnreachableCode();
                break;
            }
        }
    }

    return true;
}

EditorVisualComponentInstance *EditorVisualComponentDefinition_DirectionalLight::CreateVisual(uint32 pickingID) const
{
    return new EditorVisualComponentInstance_DirectionalLight(pickingID, this);
}

EditorVisualComponentInstance_DirectionalLight::EditorVisualComponentInstance_DirectionalLight(uint32 pickingID, const EditorVisualComponentDefinition_DirectionalLight *pDefinition)
    : EditorVisualComponentInstance(pickingID),
      m_pDefinition(pDefinition)
{
    // read other defaults from definition
    m_Enabled = m_pDefinition->GetEnabled();
    m_Rotation = m_pDefinition->GetRotation();
    m_Color = m_pDefinition->GetColor();
    m_Brightness = m_pDefinition->GetBrightness();
    m_ShadowFlags = m_pDefinition->GetShadowFlags();
    m_AmbientFactor = m_pDefinition->GetAmbientFactor();

    // create render proxy
    m_pDirectionalLightRenderProxy = new DirectionalLightRenderProxy(pickingID, m_Enabled, float3::One, float3::Zero, m_ShadowFlags, CalculateDirection());
    UpdateLightColor();

    // update bounds
    m_visualBounds = AABox::MaxSize;
}

EditorVisualComponentInstance_DirectionalLight::~EditorVisualComponentInstance_DirectionalLight()
{
    m_pDirectionalLightRenderProxy->Release();
}

float3 EditorVisualComponentInstance_DirectionalLight::CalculateDirection()
{
    return (m_Rotation * m_pDefinition->GetRotationOffset()) * float3::NegativeUnitZ;
}

void EditorVisualComponentInstance_DirectionalLight::OnAddedToRenderWorld(RenderWorld *pRenderWorld)
{
    pRenderWorld->AddRenderable(m_pDirectionalLightRenderProxy);
}

void EditorVisualComponentInstance_DirectionalLight::OnRemovedFromRenderWorld(RenderWorld *pRenderWorld)
{
    pRenderWorld->RemoveRenderable(m_pDirectionalLightRenderProxy);
}

void EditorVisualComponentInstance_DirectionalLight::OnPropertyChange(const char *propertyName, const char *propertyValue)
{
    // enabled
    if (m_pDefinition->GetEnabledFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Enabled = StringConverter::StringToBool(propertyValue);
        m_pDirectionalLightRenderProxy->SetEnabled(m_Enabled);
    }

    // rotation
    if (m_pDefinition->GetRotationFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Rotation = StringConverter::StringToQuaternion(propertyValue);
        m_pDirectionalLightRenderProxy->SetDirection(CalculateDirection());
    }

    // color
    if (m_pDefinition->GetColorFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Color = StringConverter::StringToFloat3(propertyValue);
        UpdateLightColor();
    }

    // brightness
    if (m_pDefinition->GetBrightnessFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Brightness = StringConverter::StringToFloat(propertyValue);
        UpdateLightColor();
    }

    // dynamic shadows
    if (m_pDefinition->GetShadowFlagsFromPropertyName().CompareInsensitive(propertyName))
    {
        m_ShadowFlags = StringConverter::StringToUInt32(propertyValue);
        m_pDirectionalLightRenderProxy->SetShadowFlags(m_ShadowFlags);
    }

    // ambient coefficient
    if (m_pDefinition->GetAmbientFactorFromPropertyName().CompareInsensitive(propertyName))
    {
        m_AmbientFactor = StringConverter::StringToFloat(propertyValue);
        UpdateLightColor();
    }
}

void EditorVisualComponentInstance_DirectionalLight::OnSelectionStateChange(bool selected)
{
    if (m_pDefinition->GetOnlyShowWhenSelected())
    {
        m_pDirectionalLightRenderProxy->SetEnabled(m_Enabled);
        m_pDirectionalLightRenderProxy->SetEnabled((m_AmbientFactor != 0.0f) ? m_Enabled : false);
    }
}

void EditorVisualComponentInstance_DirectionalLight::UpdateLightColor()
{
    SIMDVector3f lightColor(m_Color);
    SIMDVector3f directionalLightColor(lightColor * (m_Brightness * (1.0f - m_AmbientFactor)));
    SIMDVector3f ambientLightColor(lightColor * (m_Brightness * m_AmbientFactor));

    m_pDirectionalLightRenderProxy->SetLightColor(directionalLightColor);
    m_pDirectionalLightRenderProxy->SetAmbientColor(ambientLightColor);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EditorVisualComponentDefinition_PointLight::EditorVisualComponentDefinition_PointLight()
{
    m_bEnabled = true;
    m_Position = float3::Zero;
    m_PositionOffset = float3::Zero;
    m_Range = 128.0f;
    m_Color = float3::One;
    m_Brightness = 1.0f;
    m_FalloffExponent = 2.0f;
    m_iShadowFlags = 0;
    m_bOnlyShowWhenSelected = false;
}

EditorVisualComponentDefinition_PointLight::~EditorVisualComponentDefinition_PointLight()
{

}

EditorVisualComponentDefinition *EditorVisualComponentDefinition_PointLight::Clone() const
{
    EditorVisualComponentDefinition_PointLight *pClone = new EditorVisualComponentDefinition_PointLight();

    pClone->m_bEnabled = m_bEnabled;
    pClone->m_strEnabledFromPropertyName = m_strEnabledFromPropertyName;
    pClone->m_Position = m_Position;
    pClone->m_strPositionFromPropertyName = m_strPositionFromPropertyName;
    pClone->m_PositionOffset = m_PositionOffset;
    pClone->m_Range = m_Range;
    pClone->m_strRangeFromPropertyName = m_strRangeFromPropertyName;
    pClone->m_Color = m_Color;
    pClone->m_strColorFromPropertyName = m_strColorFromPropertyName;
    pClone->m_Brightness = m_Brightness;
    pClone->m_strBrightnessFromPropertyName = m_strBrightnessFromPropertyName;
    pClone->m_FalloffExponent = m_FalloffExponent;
    pClone->m_strFalloffExponentFromPropertyName = m_strFalloffExponentFromPropertyName;
    pClone->m_iShadowFlags = m_iShadowFlags;
    pClone->m_strShadowFlagsFromPropertyName = m_strShadowFlagsFromPropertyName;
    pClone->m_bOnlyShowWhenSelected = m_bOnlyShowWhenSelected;

    return pClone;
}

bool EditorVisualComponentDefinition_PointLight::ParseXML(XMLReader &xmlReader)
{
    if (!xmlReader.IsEmptyElement())
    {
        for (; ;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 visualSelection = xmlReader.Select("enabled|position|range|color|brightness|shadowflags|falloff-exponent|only-show-when-selected");
                if (visualSelection < 0)
                    return false;

                switch (visualSelection)
                {
                    // enabled
                case 0:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_bEnabled = StringConverter::StringToBool(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strEnabledFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // position
                case 1:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *offsetStr = xmlReader.FetchAttribute("offset");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_Position = StringConverter::StringToFloat3(valueStr);

                        if (offsetStr != NULL)
                            m_PositionOffset = StringConverter::StringToFloat3(offsetStr);

                        if (fromPropertyStr != NULL)
                            m_strPositionFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // range
                case 2:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");
                        
                        if (valueStr != NULL)
                            m_Range = StringConverter::StringToFloat(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strRangeFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // color
                case 3:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_Color = StringConverter::StringToFloat3(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strColorFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // brightness
                case 4:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_Brightness = StringConverter::StringToFloat(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strBrightnessFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // shadowflags
                case 5:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_iShadowFlags = StringConverter::StringToUInt32(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strShadowFlagsFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // falloff-exponent
                case 6:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_FalloffExponent = StringConverter::StringToFloat(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strFalloffExponentFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // only-show-when-selected
                case 7:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        if (valueStr != NULL)
                            m_bOnlyShowWhenSelected = StringConverter::StringToBool(valueStr);
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "visual") == 0);
                break;
            }
            else
            {
                UnreachableCode();
                break;
            }
        }
    }

    return true;
}

EditorVisualComponentInstance *EditorVisualComponentDefinition_PointLight::CreateVisual(uint32 pickingID) const
{
    return new EditorVisualComponentInstance_PointLight(pickingID, this);
}

EditorVisualComponentInstance_PointLight::EditorVisualComponentInstance_PointLight(uint32 pickingID, const EditorVisualComponentDefinition_PointLight *pDefinition)
    : EditorVisualComponentInstance(pickingID),
      m_pDefinition(pDefinition)
{
    // read other defaults from definition
    m_Enabled = m_pDefinition->GetEnabled();
    m_Position = m_pDefinition->GetPosition();
    m_Range = m_pDefinition->GetRange();
    m_Color = m_pDefinition->GetColor();
    m_Brightness = m_pDefinition->GetBrightness();
    m_ShadowFlags = m_pDefinition->GetShadowFlags();
    m_FalloffExponent = m_pDefinition->GetFalloffExponent();
    m_EntityPosition.SetZero();

    // create render proxy
    m_pRenderProxy = new PointLightRenderProxy(pickingID, m_Position + m_pDefinition->GetPositionOffset(), m_Range, m_Color, 0, m_FalloffExponent);

    // update bounds
    UpdateBounds();
}

EditorVisualComponentInstance_PointLight::~EditorVisualComponentInstance_PointLight()
{
    m_pRenderProxy->Release();
}

void EditorVisualComponentInstance_PointLight::OnAddedToRenderWorld(RenderWorld *pRenderWorld)
{
    pRenderWorld->AddRenderable(m_pRenderProxy);
}

void EditorVisualComponentInstance_PointLight::OnRemovedFromRenderWorld(RenderWorld *pRenderWorld)
{
    pRenderWorld->RemoveRenderable(m_pRenderProxy);
}

void EditorVisualComponentInstance_PointLight::OnPropertyChange(const char *propertyName, const char *propertyValue)
{
    // enabled
    if (m_pDefinition->GetEnabledFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Enabled = StringConverter::StringToBool(propertyValue);
        m_pRenderProxy->SetEnabled(m_Enabled);
    }

    // position
    if (m_pDefinition->GetPositionFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Position = StringConverter::StringToFloat3(propertyValue);
        m_pRenderProxy->SetPosition(m_Position + m_pDefinition->GetPositionOffset());
        UpdateBounds();
    }

    // range
    if (m_pDefinition->GetRangeFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Range = StringConverter::StringToFloat(propertyValue);
        m_pRenderProxy->SetRange(m_Range);
        UpdateBounds();
    }

    // color
    if (m_pDefinition->GetColorFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Color = StringConverter::StringToFloat3(propertyValue);
        m_pRenderProxy->SetColor(m_Color * m_Brightness);
    }

    // brightness
    if (m_pDefinition->GetBrightnessFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Brightness = StringConverter::StringToFloat(propertyValue);
        m_pRenderProxy->SetColor(m_Color * m_Brightness);
    }

    // dynamic shadows
    if (m_pDefinition->GetShadowFlagsFromPropertyName().CompareInsensitive(propertyName))
    {
        m_ShadowFlags = StringConverter::StringToUInt32(propertyValue);
        m_pRenderProxy->SetShadowFlags(m_ShadowFlags);
    }

    // falloff exponent
    if (m_pDefinition->GetFalloffExponentFromPropertyName().CompareInsensitive(propertyName))
    {
        m_FalloffExponent = StringConverter::StringToFloat(propertyValue);
        m_pRenderProxy->SetFalloffExponent(m_FalloffExponent);
    }
}

void EditorVisualComponentInstance_PointLight::OnSelectionStateChange(bool selected)
{
    if (m_pDefinition->GetOnlyShowWhenSelected())
        m_pRenderProxy->SetEnabled(selected ? m_Enabled : false);
}

void EditorVisualComponentInstance_PointLight::UpdateBounds()
{
    SIMDVector3f actualPosition = m_Position + m_pDefinition->GetPositionOffset();

    SIMDVector3f minBounds = actualPosition - m_Range;
    SIMDVector3f maxBounds = actualPosition + m_Range;

    m_visualBounds.SetBounds(minBounds, maxBounds);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EditorVisualComponentDefinition_Sprite::EditorVisualComponentDefinition_Sprite()
{
    // set defaults
    m_Position = float3::Zero;
    m_PositionOffset = float3::Zero;
    m_Scale = float3::One;
    m_ScaleOffset = float3::One;
    m_bVisible = true;
    m_iShadowFlags = 0;
    m_strTextureName = g_pEngine->GetDefaultTexture2DName();
    m_eBlendingMode = MATERIAL_BLENDING_MODE_STRAIGHT;
    m_fWidth = 128.0f;
    m_fHeight = 128.0f;
    m_TintColor = 0xFFFFFFFF;
    m_SelectedTintColor = 0xFFFFFFFF;
    m_bOnlyShowWhenSelected = false;
}

EditorVisualComponentDefinition_Sprite::~EditorVisualComponentDefinition_Sprite()
{

}

EditorVisualComponentDefinition *EditorVisualComponentDefinition_Sprite::Clone() const
{
    EditorVisualComponentDefinition_Sprite *pClone = new EditorVisualComponentDefinition_Sprite();

    pClone->m_Position = m_Position;
    pClone->m_strPositionFromPropertyName = m_strPositionFromPropertyName;
    pClone->m_PositionOffset = m_PositionOffset;
    pClone->m_Scale = m_Scale;
    pClone->m_strScaleFromPropertyName = m_strScaleFromPropertyName;
    pClone->m_ScaleOffset = m_ScaleOffset;
    pClone->m_bVisible = m_bVisible;
    pClone->m_strVisibleFromPropertyName = m_strVisibleFromPropertyName;
    pClone->m_iShadowFlags = m_iShadowFlags;
    pClone->m_strShadowFlagsFromPropertyName = m_strShadowFlagsFromPropertyName;
    pClone->m_strTextureName = m_strTextureName;
    pClone->m_strTextureFromPropertyName = m_strTextureFromPropertyName;
    pClone->m_eBlendingMode = m_eBlendingMode;
    pClone->m_strBlendingModeFromPropertyName = m_strBlendingModeFromPropertyName;
    pClone->m_fWidth = m_fWidth;
    pClone->m_strWidthFromPropertyName = m_strWidthFromPropertyName;
    pClone->m_fHeight = m_fHeight;
    pClone->m_strHeightFromPropertyName = m_strHeightFromPropertyName;
    pClone->m_TintColor = m_TintColor;
    pClone->m_SelectedTintColor = m_SelectedTintColor;
    pClone->m_bOnlyShowWhenSelected = m_bOnlyShowWhenSelected;

    return pClone;
}

bool EditorVisualComponentDefinition_Sprite::ParseXML(XMLReader &xmlReader)
{
    if (!xmlReader.IsEmptyElement())
    {
        for (; ;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 visualSelection = xmlReader.Select("position|scale|visible|shadowflags|texture|blending|width|height|tint-color|selected-tint-color|only-show-when-selected");
                if (visualSelection < 0)
                    return false;

                switch (visualSelection)
                {
                    // position
                case 0:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *offsetStr = xmlReader.FetchAttribute("offset");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_Position = StringConverter::StringToFloat3(valueStr);

                        if (offsetStr != NULL)
                            m_PositionOffset = StringConverter::StringToFloat3(offsetStr);

                        if (fromPropertyStr != NULL)
                            m_strPositionFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // scale
                case 1:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *offsetStr = xmlReader.FetchAttribute("offset");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_Scale = StringConverter::StringToFloat3(valueStr);

                        if (offsetStr != NULL)
                            m_ScaleOffset = StringConverter::StringToFloat3(offsetStr);

                        if (fromPropertyStr != NULL)
                            m_strScaleFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // visible
                case 2:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_bVisible = StringConverter::StringToBool(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strVisibleFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // shadowflags
                case 3:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_iShadowFlags = StringConverter::StringToUInt32(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strShadowFlagsFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // texture
                case 4:
                    {
                        const char *nameStr = xmlReader.FetchAttribute("name");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");
                        
                        if (nameStr != NULL)
                            m_strTextureName = nameStr;

                        if (fromPropertyStr != NULL)
                            m_strTextureFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // blending
                case 5:
                    {
                        const char *modeStr = xmlReader.FetchAttribute("mode");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (modeStr != NULL)
                        {
                            if (!NameTable_TranslateType(NameTables::MaterialBlendingMode, modeStr, &m_eBlendingMode, true))
                            {
                                xmlReader.PrintError("invalid blending mode: '%s'", modeStr);
                                return false;
                            }
                        }

                        if (fromPropertyStr != NULL)
                            m_strBlendingModeFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // width
                case 6:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_fWidth = StringConverter::StringToFloat(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strTextureFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // height
                case 7:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_fHeight = StringConverter::StringToFloat(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strTextureFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // tint-color
                case 8:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        if  (valueStr != NULL)
                            m_TintColor = StringConverter::StringToColor(valueStr) | 0xff000000;
                    }
                    break;

                    // selected-tint-color
                case 9:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        if  (valueStr != NULL)
                            m_SelectedTintColor = StringConverter::StringToColor(valueStr) | 0xff000000;
                    }
                    break;

                    // only-show-when-selected
                case 10:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        if (valueStr != NULL)
                            m_bOnlyShowWhenSelected = StringConverter::StringToBool(valueStr);
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "visual") == 0);
                break;
            }
            else
            {
                UnreachableCode();
                break;
            }
        }
    }

    return true;
}

EditorVisualComponentInstance *EditorVisualComponentDefinition_Sprite::CreateVisual(uint32 pickingID) const
{
    return new EditorVisualComponentInstance_Sprite(pickingID, this);
}

EditorVisualComponentInstance_Sprite::EditorVisualComponentInstance_Sprite(uint32 pickingID, const EditorVisualComponentDefinition_Sprite *pDefinition)
    : EditorVisualComponentInstance(pickingID),
      m_pDefinition(pDefinition)
{
    // set default texture
    if ((m_pTexture = g_pResourceManager->GetTexture2D(pDefinition->GetTextureName())) == NULL)
        m_pTexture = g_pResourceManager->GetDefaultTexture2D();

    // read other defaults from definition
    m_eBlendingMode = pDefinition->GetBlendingMode();
    m_Position = pDefinition->GetPosition();
    m_Scale = pDefinition->GetScale();
    m_Visible = pDefinition->GetVisible();
    m_ShadowFlags = pDefinition->GetShadowFlags();
    m_fWidth = pDefinition->GetWidth();
    m_fHeight = pDefinition->GetHeight();

    // create render proxy
    m_pRenderProxy = new SpriteRenderProxy(pickingID, m_pTexture, m_Position + pDefinition->GetPositionOffset());
    m_pRenderProxy->SetSizeByDimensions(m_fWidth, m_fHeight);

    // update bounds
    UpdateBounds();
}

EditorVisualComponentInstance_Sprite::~EditorVisualComponentInstance_Sprite()
{
    m_pRenderProxy->Release();
    m_pTexture->Release();
}

void EditorVisualComponentInstance_Sprite::OnAddedToRenderWorld(RenderWorld *pRenderWorld)
{
    pRenderWorld->AddRenderable(m_pRenderProxy);
}

void EditorVisualComponentInstance_Sprite::OnRemovedFromRenderWorld(RenderWorld *pRenderWorld)
{
    pRenderWorld->RemoveRenderable(m_pRenderProxy);
}

void EditorVisualComponentInstance_Sprite::OnPropertyChange(const char *propertyName, const char *propertyValue)
{
    // position
    if (m_pDefinition->GetPositionFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Position = StringConverter::StringToFloat3(propertyValue);
        m_pRenderProxy->SetPosition(m_Position + m_pDefinition->GetPositionOffset());
        UpdateBounds();
    }

    // scale
    if (m_pDefinition->GetScaleFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Scale = StringConverter::StringToFloat3(propertyValue);
        //UpdateTransform();
    }

    // visible
    if (m_pDefinition->GetVisibleFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Visible = StringConverter::StringToBool(propertyValue);
        m_pRenderProxy->SetVisibility(m_Visible);
    }

    // shadowflags
    if (m_pDefinition->GetVisibleFromPropertyName().CompareInsensitive(propertyName))
    {
        m_ShadowFlags = StringConverter::StringToUInt32(propertyValue);
        //m_pRenderProxy->SetShadowFlags(m_ShadowFlags);
    }

    // texture name
    if (m_pDefinition->GetTextureFromPropertyName().CompareInsensitive(propertyName))
    {
        const Texture2D *pTexture;
        if ((pTexture = g_pResourceManager->GetTexture2D(propertyValue)) == NULL)
        {
            if ((pTexture = g_pResourceManager->GetTexture2D(m_pDefinition->GetTextureName())) == NULL)
                pTexture = g_pResourceManager->GetDefaultTexture2D();
        }

        Swap(m_pTexture, pTexture);
        pTexture->Release();
    }

    // blending mode
    if (m_pDefinition->GetBlendingModeFromPropertyName().CompareInsensitive(propertyValue))
    {
        m_eBlendingMode = (MATERIAL_BLENDING_MODE)StringConverter::StringToUInt32(propertyValue);
        if (m_eBlendingMode >= MATERIAL_BLENDING_MODE_COUNT)
            m_eBlendingMode = m_pDefinition->GetBlendingMode();
    }
    
    // width
    if (m_pDefinition->GetWidthFromPropertyName().CompareInsensitive(propertyValue))
    {
        m_fWidth = StringConverter::StringToFloat(propertyValue);
        UpdateBounds();
    }

    // height
    if (m_pDefinition->GetHeightFromPropertyName().CompareInsensitive(propertyValue))
    {
        m_fHeight = StringConverter::StringToFloat(propertyValue);
        UpdateBounds();
    }
}

void EditorVisualComponentInstance_Sprite::OnSelectionStateChange(bool selected)
{
    uint32 tintColor = m_pDefinition->GetTintColor();
    uint32 selectedTintColor = m_pDefinition->GetSelectedTintColor();
    if (tintColor != selectedTintColor)
    {
        uint32 currentColor = selected ? selectedTintColor : tintColor;
        if (currentColor != 0xFFFFFFFF)
            m_pRenderProxy->SetTintColor(true, selectedTintColor);
        else
            m_pRenderProxy->SetTintColor(false);
    }

    if (m_pDefinition->GetOnlyShowWhenSelected())
        m_pRenderProxy->SetVisibility(selected ? m_Visible : false);
}

void EditorVisualComponentInstance_Sprite::UpdateBounds()
{
    float halfWidth = m_fWidth * 0.5f;
    float halfHeight = m_fHeight * 0.5f;
    float3 actualPosition = m_Position + m_pDefinition->GetPositionOffset();

    float3 minBounds = float3(actualPosition.x - halfWidth, actualPosition.y - halfWidth, actualPosition.z - halfHeight);
    float3 maxBounds = float3(actualPosition.x + halfWidth, actualPosition.y + halfWidth, actualPosition.z + halfHeight);

    m_visualBounds.SetBounds(minBounds, maxBounds);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EditorVisualComponentDefinition_StaticMesh::EditorVisualComponentDefinition_StaticMesh()
{
    m_Position = float3::Zero;
    m_PositionOffset = float3::Zero;
    m_Rotation = Quaternion::Identity;
    m_RotationOffset = Quaternion::Identity;
    m_Scale = float3::One;
    m_ScaleOffset = float3::One;
    m_bVisible = true;
    m_iShadowFlags = 0;
    m_strMeshName = g_pEngine->GetDefaultStaticMeshName();
    m_TintColor = 0xFFFFFFFF;
    m_SelectedTintColor = 0xFFFFFFFF;
    m_bOnlyShowWhenSelected = false;
}

EditorVisualComponentDefinition_StaticMesh::~EditorVisualComponentDefinition_StaticMesh()
{

}

EditorVisualComponentDefinition *EditorVisualComponentDefinition_StaticMesh::Clone() const
{
    EditorVisualComponentDefinition_StaticMesh *pClone = new EditorVisualComponentDefinition_StaticMesh();

    pClone->m_Position = m_Position;
    pClone->m_strPositionFromPropertyName = m_strPositionFromPropertyName;
    pClone->m_PositionOffset = m_PositionOffset;
    pClone->m_Rotation = m_Rotation;
    pClone->m_strRotationFromPropertyName = m_strRotationFromPropertyName;
    pClone->m_RotationOffset = m_RotationOffset;
    pClone->m_Scale = m_Scale;
    pClone->m_strScaleFromPropertyName = m_strScaleFromPropertyName;
    pClone->m_ScaleOffset = m_ScaleOffset;
    pClone->m_bVisible = m_bVisible;
    pClone->m_strVisibleFromPropertyName = m_strVisibleFromPropertyName;
    pClone->m_iShadowFlags = m_iShadowFlags;
    pClone->m_strShadowFlagsFromPropertyName = m_strShadowFlagsFromPropertyName;
    pClone->m_strMeshName = m_strMeshName;
    pClone->m_strMeshFromPropertyName = m_strMeshFromPropertyName;
    pClone->m_TintColor = m_TintColor;
    pClone->m_SelectedTintColor = m_SelectedTintColor;
    pClone->m_bOnlyShowWhenSelected = m_bOnlyShowWhenSelected;

    return pClone;
}

bool EditorVisualComponentDefinition_StaticMesh::ParseXML(XMLReader &xmlReader)
{
    if (!xmlReader.IsEmptyElement())
    {
        for (; ;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 visualSelection = xmlReader.Select("position|rotation|scale|visible|shadowflags|mesh|tint-color|selected-tint-color|only-show-when-selected");
                if (visualSelection < 0)
                    return false;

                switch (visualSelection)
                {
                    // position
                case 0:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *offsetStr = xmlReader.FetchAttribute("offset");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_Position = StringConverter::StringToFloat3(valueStr);

                        if (offsetStr != NULL)
                            m_PositionOffset = StringConverter::StringToFloat3(offsetStr);

                        if (fromPropertyStr != NULL)
                            m_strPositionFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // rotation
                case 1:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *offsetStr = xmlReader.FetchAttribute("offset");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_Rotation = ParseRotationValue(valueStr);

                        if (offsetStr != NULL)
                            m_RotationOffset = ParseRotationValue(offsetStr);

                        if (fromPropertyStr != NULL)
                            m_strRotationFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // scale
                case 2:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *offsetStr = xmlReader.FetchAttribute("offset");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_Scale = StringConverter::StringToFloat3(valueStr);

                        if (offsetStr != NULL)
                            m_ScaleOffset = StringConverter::StringToFloat3(offsetStr);

                        if (fromPropertyStr != NULL)
                            m_strScaleFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // visible
                case 3:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_bVisible = StringConverter::StringToBool(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strVisibleFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // shadowflags
                case 4:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_iShadowFlags = StringConverter::StringToUInt32(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strShadowFlagsFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // mesh
                case 5:
                    {
                        const char *nameStr = xmlReader.FetchAttribute("name");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (nameStr != NULL)
                            m_strMeshName = nameStr;

                        if (fromPropertyStr != NULL)
                            m_strMeshFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // tint-color
                case 6:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        if  (valueStr != NULL)
                            m_TintColor = StringConverter::StringToColor(valueStr) | 0xff000000;
                    }
                    break;

                    // selected-tint-color
                case 7:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        if  (valueStr != NULL)
                            m_SelectedTintColor = StringConverter::StringToColor(valueStr) | 0xff000000;
                    }
                    break;

                    // only-show-when-selected
                case 8:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        if (valueStr != NULL)
                            m_bOnlyShowWhenSelected = StringConverter::StringToBool(valueStr);
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "visual") == 0);
                break;
            }
            else
            {
                UnreachableCode();
                break;
            }
        }
    }

    return true;
}

EditorVisualComponentInstance *EditorVisualComponentDefinition_StaticMesh::CreateVisual(uint32 pickingID) const
{
    return new EditorVisualComponentInstance_StaticMesh(pickingID, this);
}

EditorVisualComponentInstance_StaticMesh::EditorVisualComponentInstance_StaticMesh(uint32 pickingID, const EditorVisualComponentDefinition_StaticMesh *pDefinition)
    : EditorVisualComponentInstance(pickingID),
      m_pDefinition(pDefinition)
{
    // get mesh pointer
    if ((m_pStaticMesh = g_pResourceManager->GetStaticMesh(pDefinition->GetMeshName())) == NULL)
        m_pStaticMesh = g_pResourceManager->GetDefaultStaticMesh();

    // read other defaults from definition
    m_Position = pDefinition->GetPosition();
    m_Rotation = pDefinition->GetRotation();
    m_Scale = pDefinition->GetScale();
    m_Visible = pDefinition->GetVisible();
    m_ShadowFlags = pDefinition->GetShadowFlags();

    // create render proxy
    m_pRenderProxy = new StaticMeshRenderProxy(pickingID, m_pStaticMesh, Transform::Identity, m_ShadowFlags);

    // update bounds
    UpdateTransform();
}

EditorVisualComponentInstance_StaticMesh::~EditorVisualComponentInstance_StaticMesh()
{
    m_pRenderProxy->Release();
    m_pStaticMesh->Release();
}

void EditorVisualComponentInstance_StaticMesh::OnAddedToRenderWorld(RenderWorld *pRenderWorld)
{
    pRenderWorld->AddRenderable(m_pRenderProxy);
}

void EditorVisualComponentInstance_StaticMesh::OnRemovedFromRenderWorld(RenderWorld *pRenderWorld)
{
    pRenderWorld->RemoveRenderable(m_pRenderProxy);
}

void EditorVisualComponentInstance_StaticMesh::OnPropertyChange(const char *propertyName, const char *propertyValue)
{
    // position
    if (m_pDefinition->GetPositionFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Position = StringConverter::StringToFloat3(propertyValue);
        UpdateTransform();
    }

    // rotation
    if (m_pDefinition->GetRotationFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Rotation = StringConverter::StringToQuaternion(propertyValue);
        UpdateTransform();
    }

    // scale
    if (m_pDefinition->GetScaleFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Scale = StringConverter::StringToFloat3(propertyValue);
        UpdateTransform();
    }

    // visible
    if (m_pDefinition->GetVisibleFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Visible = StringConverter::StringToBool(propertyValue);
        m_pRenderProxy->SetVisibility(m_Visible);
    }

    // shadowflags
    if (m_pDefinition->GetShadowFlagsFromPropertyName().CompareInsensitive(propertyName))
    {
        m_ShadowFlags = StringConverter::StringToUInt32(propertyValue);
        m_pRenderProxy->SetShadowFlags(m_ShadowFlags);
    }

    // mesh name
    if (m_pDefinition->GetMeshFromPropertyName().CompareInsensitive(propertyName))
    {
        const StaticMesh *pStaticMesh;
        if ((pStaticMesh = g_pResourceManager->GetStaticMesh(propertyValue)) == NULL)
        {
            if ((pStaticMesh = g_pResourceManager->GetStaticMesh(m_pDefinition->GetMeshName())) == NULL)
                pStaticMesh = g_pResourceManager->GetDefaultStaticMesh();
        }

        if (m_pStaticMesh != pStaticMesh)
        {
            // set mesh
            m_pStaticMesh->Release();
            m_pStaticMesh = pStaticMesh;

            // update render proxy
            m_pRenderProxy->SetStaticMesh(pStaticMesh);

            // fix up bounds
            UpdateBounds();
        }
        else
        {
            // no change needed
            pStaticMesh->Release();
        }
    }
}

void EditorVisualComponentInstance_StaticMesh::OnSelectionStateChange(bool selected)
{
    uint32 tintColor = m_pDefinition->GetTintColor();
    uint32 selectedTintColor = m_pDefinition->GetSelectedTintColor();
    if (tintColor != selectedTintColor)
    {
        uint32 currentColor = selected ? selectedTintColor : tintColor;
        if (currentColor != 0xFFFFFFFF)
            m_pRenderProxy->SetTintColor(true, selectedTintColor);
        else
            m_pRenderProxy->SetTintColor(false);
    }

    if (m_pDefinition->GetOnlyShowWhenSelected())
        m_pRenderProxy->SetVisibility(selected ? m_Visible : false);
}

void EditorVisualComponentInstance_StaticMesh::UpdateTransform()
{
    Transform offsetTransform(m_pDefinition->GetPositionOffset(), m_pDefinition->GetRotationOffset(), m_pDefinition->GetScaleOffset());
    Transform localTransform(m_Position, m_Rotation, m_Scale);
    Transform fullTransform(Transform::ConcatenateTransforms(offsetTransform, localTransform));
    
    // effective order: LS * LR * LT * ER * ET
    m_CachedTransform = fullTransform;
    m_pRenderProxy->SetTransform(m_CachedTransform);

    // fix up bounds too
    UpdateBounds();
}

void EditorVisualComponentInstance_StaticMesh::UpdateBounds()
{
    m_visualBounds = m_CachedTransform.TransformBoundingBox(m_pStaticMesh->GetBoundingBox());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EditorVisualComponentDefinition_BlockMesh::EditorVisualComponentDefinition_BlockMesh()
{
    m_Position = float3::Zero;
    m_PositionOffset = float3::Zero;
    m_Rotation = Quaternion::Identity;
    m_RotationOffset = Quaternion::Identity;
    m_Scale = float3::One;
    m_ScaleOffset = float3::One;
    m_bVisible = true;
    m_iShadowFlags = 0;
    m_strMeshName = g_pEngine->GetDefaultBlockMeshName();
    m_TintColor = 0xFFFFFFFF;
    m_SelectedTintColor = 0xFFFFFFFF;
    m_bOnlyShowWhenSelected = false;
}

EditorVisualComponentDefinition_BlockMesh::~EditorVisualComponentDefinition_BlockMesh()
{

}

EditorVisualComponentDefinition *EditorVisualComponentDefinition_BlockMesh::Clone() const
{
    EditorVisualComponentDefinition_BlockMesh *pClone = new EditorVisualComponentDefinition_BlockMesh();

    pClone->m_Position = m_Position;
    pClone->m_strPositionFromPropertyName = m_strPositionFromPropertyName;
    pClone->m_PositionOffset = m_PositionOffset;
    pClone->m_Rotation = m_Rotation;
    pClone->m_strRotationFromPropertyName = m_strRotationFromPropertyName;
    pClone->m_RotationOffset = m_RotationOffset;
    pClone->m_Scale = m_Scale;
    pClone->m_strScaleFromPropertyName = m_strScaleFromPropertyName;
    pClone->m_ScaleOffset = m_ScaleOffset;
    pClone->m_bVisible = m_bVisible;
    pClone->m_strVisibleFromPropertyName = m_strVisibleFromPropertyName;
    pClone->m_iShadowFlags = m_iShadowFlags;
    pClone->m_strShadowFlagsFromPropertyName = m_strShadowFlagsFromPropertyName;
    pClone->m_strMeshName = m_strMeshName;
    pClone->m_strMeshFromPropertyName = m_strMeshFromPropertyName;
    pClone->m_TintColor = m_TintColor;
    pClone->m_SelectedTintColor = m_SelectedTintColor;
    pClone->m_bOnlyShowWhenSelected = m_bOnlyShowWhenSelected;

    return pClone;
}

bool EditorVisualComponentDefinition_BlockMesh::ParseXML(XMLReader &xmlReader)
{
    if (!xmlReader.IsEmptyElement())
    {
        for (; ;)
        {
            if (!xmlReader.NextToken())
                return false;

            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
            {
                int32 visualSelection = xmlReader.Select("position|rotation|scale|visible|shadowflags|mesh|tint-color|selected-tint-color|only-show-when-selected");
                if (visualSelection < 0)
                    return false;

                switch (visualSelection)
                {
                    // position
                case 0:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *offsetStr = xmlReader.FetchAttribute("offset");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_Position = StringConverter::StringToFloat3(valueStr);

                        if (offsetStr != NULL)
                            m_PositionOffset = StringConverter::StringToFloat3(offsetStr);

                        if (fromPropertyStr != NULL)
                            m_strPositionFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // rotation
                case 1:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *offsetStr = xmlReader.FetchAttribute("offset");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_Rotation = ParseRotationValue(valueStr);

                        if (offsetStr != NULL)
                            m_RotationOffset = ParseRotationValue(offsetStr);

                        if (fromPropertyStr != NULL)
                            m_strRotationFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // scale
                case 2:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *offsetStr = xmlReader.FetchAttribute("offset");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_Scale = StringConverter::StringToFloat3(valueStr);

                        if (offsetStr != NULL)
                            m_ScaleOffset = StringConverter::StringToFloat3(offsetStr);

                        if (fromPropertyStr != NULL)
                            m_strScaleFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // visible
                case 3:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_bVisible = StringConverter::StringToBool(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strVisibleFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // shadowflags
                case 4:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (valueStr != NULL)
                            m_iShadowFlags = StringConverter::StringToUInt32(valueStr);

                        if (fromPropertyStr != NULL)
                            m_strShadowFlagsFromPropertyName = fromPropertyStr;
                    }
                    break;

                    // mesh
                case 5:
                    {
                        const char *nameStr = xmlReader.FetchAttribute("name");
                        const char *fromPropertyStr = xmlReader.FetchAttribute("from-property");

                        if (nameStr != NULL)
                            m_strMeshName = nameStr;

                        if (fromPropertyStr != NULL)
                            m_strMeshFromPropertyName = fromPropertyStr;
                    }
                    break;
                   
                    // tint-color
                case 6:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        if  (valueStr != NULL)
                            m_TintColor = StringConverter::StringToColor(valueStr) | 0xff000000;
                    }
                    break;

                    // selected-tint-color
                case 7:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        if  (valueStr != NULL)
                            m_SelectedTintColor = StringConverter::StringToColor(valueStr) | 0xff000000;
                    }
                    break;

                    // only-show-when-selected
                case 8:
                    {
                        const char *valueStr = xmlReader.FetchAttribute("value");
                        if (valueStr != NULL)
                            m_bOnlyShowWhenSelected = StringConverter::StringToBool(valueStr);
                    }
                    break;

                default:
                    UnreachableCode();
                    break;
                }
            }
            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
            {
                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "visual") == 0);
                break;
            }
            else
            {
                UnreachableCode();
                break;
            }
        }
    }

    return true;
}

EditorVisualComponentInstance *EditorVisualComponentDefinition_BlockMesh::CreateVisual(uint32 pickingID) const
{
    return new EditorVisualComponentInstance_BlockMesh(pickingID, this);
}

EditorVisualComponentInstance_BlockMesh::EditorVisualComponentInstance_BlockMesh(uint32 pickingID, const EditorVisualComponentDefinition_BlockMesh *pDefinition)
    : EditorVisualComponentInstance(pickingID),
      m_pDefinition(pDefinition)
{
    // get mesh pointer
    if ((m_pBlockMesh = g_pResourceManager->GetBlockMesh(pDefinition->GetMeshName())) == NULL)
        m_pBlockMesh = g_pResourceManager->GetDefaultBlockMesh();

    // read other defaults from definition
    m_Position = pDefinition->GetPosition();
    m_Rotation = pDefinition->GetRotation();
    m_Scale = pDefinition->GetScale();
    m_Visible = pDefinition->GetVisible();
    m_ShadowFlags = pDefinition->GetShadowFlags();

    // create render proxy
    m_pRenderProxy = new BlockMeshRenderProxy(pickingID, m_pBlockMesh, Transform::Identity, m_ShadowFlags);

    // update bounds
    UpdateTransform();
}

EditorVisualComponentInstance_BlockMesh::~EditorVisualComponentInstance_BlockMesh()
{
    m_pRenderProxy->Release();
    m_pBlockMesh->Release();
}

void EditorVisualComponentInstance_BlockMesh::OnAddedToRenderWorld(RenderWorld *pRenderWorld)
{
    pRenderWorld->AddRenderable(m_pRenderProxy);
}

void EditorVisualComponentInstance_BlockMesh::OnRemovedFromRenderWorld(RenderWorld *pRenderWorld)
{
    pRenderWorld->RemoveRenderable(m_pRenderProxy);
}

void EditorVisualComponentInstance_BlockMesh::OnPropertyChange(const char *propertyName, const char *propertyValue)
{
    // position
    if (m_pDefinition->GetPositionFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Position = StringConverter::StringToFloat3(propertyValue);
        UpdateTransform();
    }

    // rotation
    if (m_pDefinition->GetRotationFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Rotation = StringConverter::StringToQuaternion(propertyValue);
        UpdateTransform();
    }

    // scale
    if (m_pDefinition->GetScaleFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Scale = StringConverter::StringToFloat3(propertyValue);
        UpdateTransform();
    }

    // visible
    if (m_pDefinition->GetVisibleFromPropertyName().CompareInsensitive(propertyName))
    {
        m_Visible = StringConverter::StringToBool(propertyValue);
        m_pRenderProxy->SetVisibility(m_Visible);
    }

    // shadowflags
    if (m_pDefinition->GetShadowFlagsFromPropertyName().CompareInsensitive(propertyName))
    {
        m_ShadowFlags = StringConverter::StringToUInt32(propertyValue);
        m_pRenderProxy->SetShadowFlags(m_ShadowFlags);
    }

    // mesh name
    if (m_pDefinition->GetMeshFromPropertyName().CompareInsensitive(propertyName))
    {
        const BlockMesh *pStaticBlockMesh;
        if ((pStaticBlockMesh = g_pResourceManager->GetBlockMesh(propertyValue)) == NULL)
        {
            if ((pStaticBlockMesh = g_pResourceManager->GetBlockMesh(m_pDefinition->GetMeshName())) == NULL)
                pStaticBlockMesh = g_pResourceManager->GetDefaultBlockMesh();
        }

        if (m_pBlockMesh != pStaticBlockMesh)
        {
            // set mesh
            m_pBlockMesh->Release();
            m_pBlockMesh = pStaticBlockMesh;

            // update render proxy
            m_pRenderProxy->SetBlockMesh(pStaticBlockMesh);

            // fix up bounds
            UpdateBounds();
        }
        else
        {
            // no change needed
            pStaticBlockMesh->Release();
        }
    }
}

void EditorVisualComponentInstance_BlockMesh::OnSelectionStateChange(bool selected)
{
    uint32 tintColor = m_pDefinition->GetTintColor();
    uint32 selectedTintColor = m_pDefinition->GetSelectedTintColor();
    if (tintColor != selectedTintColor)
    {
        uint32 currentColor = selected ? selectedTintColor : tintColor;
        if (currentColor != 0xFFFFFFFF)
            m_pRenderProxy->SetTintColor(true, selectedTintColor);
        else
            m_pRenderProxy->SetTintColor(false);
    }

    if (m_pDefinition->GetOnlyShowWhenSelected())
        m_pRenderProxy->SetVisibility(selected ? m_Visible : false);
}

void EditorVisualComponentInstance_BlockMesh::UpdateTransform()
{
    Transform offsetTransform(m_pDefinition->GetPositionOffset(), m_pDefinition->GetRotationOffset(), m_pDefinition->GetScaleOffset());
    Transform localTransform(m_Position, m_Rotation, m_Scale);
    Transform fullTransform(Transform::ConcatenateTransforms(offsetTransform, localTransform));

    // effective order: LS * LR * LT * ER * ET
    m_CachedTransform = fullTransform;
    m_pRenderProxy->SetTransform(m_CachedTransform);

    // fix up bounds too
    UpdateBounds();
}

void EditorVisualComponentInstance_BlockMesh::UpdateBounds()
{
    m_visualBounds = m_CachedTransform.TransformBoundingBox(m_pBlockMesh->GetBoundingBox());
}