#pragma once
#include "Editor/Common.h"

class XMLReader;

class RenderProxy;
class RenderWorld;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Texture2D;
class StaticMesh;
class BlockMesh;
class DirectionalLightRenderProxy;
class PointLightRenderProxy;
class AmbientLightRenderProxy;
class SpriteRenderProxy;
class StaticMeshRenderProxy;
class BlockMeshRenderProxy;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class EditorVisualComponentInstance;

class EditorVisualComponentDefinition
{
public:
    EditorVisualComponentDefinition();
    virtual ~EditorVisualComponentDefinition();

    virtual EditorVisualComponentDefinition *Clone() const = 0;
    virtual bool ParseXML(XMLReader &xmlReader) = 0;

    virtual EditorVisualComponentInstance *CreateVisual(uint32 pickingID) const = 0;
};

class EditorVisualComponentInstance
{
public:
    EditorVisualComponentInstance(uint32 pickingID);
    virtual ~EditorVisualComponentInstance();

    const uint32 GetPickingID() const { return m_pickingID; }
    const AABox &GetVisualBounds() const { return m_visualBounds; }

    virtual void OnAddedToRenderWorld(RenderWorld *pRenderWorld) = 0;
    virtual void OnRemovedFromRenderWorld(RenderWorld *pRenderWorld) = 0;
    virtual void OnPropertyChange(const char *propertyName, const char *propertyValue) = 0;
    virtual void OnSelectionStateChange(bool selected) = 0;

protected:
    uint32 m_pickingID;
    AABox m_visualBounds;
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class EditorVisualComponentDefinition_DirectionalLight : public EditorVisualComponentDefinition
{
public:
    EditorVisualComponentDefinition_DirectionalLight();
    virtual ~EditorVisualComponentDefinition_DirectionalLight();

    const bool GetEnabled() const { return m_bEnabled; }
    const String &GetEnabledFromPropertyName() const { return m_strEnabledFromPropertyName; }

    const Quaternion &GetRotation() const { return m_Rotation; }
    const String &GetRotationFromPropertyName() const { return m_strRotationFromPropertyName; }
    const Quaternion &GetRotationOffset() const { return m_RotationOffset; }

    const float3 &GetColor() const { return m_Color; }
    const String &GetColorFromPropertyName() const { return m_strColorFromPropertyName; }

    const float GetBrightness() const { return m_Brightness; }
    const String &GetBrightnessFromPropertyName() const { return m_strBrightnessFromPropertyName; }

    const uint32 GetShadowFlags() const { return m_iShadowFlags; }
    const String &GetShadowFlagsFromPropertyName() const { return m_strShadowFlagsFromPropertyName; }

    const float GetAmbientFactor() const { return m_fAmbientFactor; }
    const String &GetAmbientFactorFromPropertyName() const { return m_strAmbientFactorFromPropertyName; }

    const bool GetOnlyShowWhenSelected() const { return m_bOnlyShowWhenSelected; }

    virtual EditorVisualComponentDefinition *Clone() const;
    virtual bool ParseXML(XMLReader &xmlReader);

    virtual EditorVisualComponentInstance *CreateVisual(uint32 pickingID) const;

private:
    bool m_bEnabled;
    String m_strEnabledFromPropertyName;
    Quaternion m_Rotation;
    String m_strRotationFromPropertyName;
    Quaternion m_RotationOffset;
    float3 m_Color;
    String m_strColorFromPropertyName;
    float m_Brightness;
    String m_strBrightnessFromPropertyName;
    uint32 m_iShadowFlags;
    String m_strShadowFlagsFromPropertyName;
    float m_fAmbientFactor;
    String m_strAmbientFactorFromPropertyName;

    // tint colour
    bool m_bOnlyShowWhenSelected;
};

class EditorVisualComponentInstance_DirectionalLight : public EditorVisualComponentInstance
{
public:
    EditorVisualComponentInstance_DirectionalLight(uint32 pickingID, const EditorVisualComponentDefinition_DirectionalLight *pDefinition);
    virtual ~EditorVisualComponentInstance_DirectionalLight();

    void OnAddedToRenderWorld(RenderWorld *pRenderWorld) override;
    void OnRemovedFromRenderWorld(RenderWorld *pRenderWorld) override;
    void OnPropertyChange(const char *propertyName, const char *propertyValue) override;
    void OnSelectionStateChange(bool selected) override;

private:
    // recalculate light colours
    void UpdateLightColor();
    float3 CalculateDirection();

    // definition
    const EditorVisualComponentDefinition_DirectionalLight *m_pDefinition;

    // cached property values
    bool m_Enabled;
    Quaternion m_Rotation;
    float3 m_Color;
    float m_Brightness;
    uint32 m_ShadowFlags;
    float m_AmbientFactor;
    
    // render proxy
    DirectionalLightRenderProxy *m_pDirectionalLightRenderProxy;
    AmbientLightRenderProxy *m_pAmbientLightRenderProxy;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class EditorVisualComponentDefinition_PointLight : public EditorVisualComponentDefinition
{
public:
    EditorVisualComponentDefinition_PointLight();
    virtual ~EditorVisualComponentDefinition_PointLight();

    const bool GetEnabled() const { return m_bEnabled; }
    const String &GetEnabledFromPropertyName() const { return m_strEnabledFromPropertyName; }

    const float3 &GetPosition() const { return m_Position; }
    const String &GetPositionFromPropertyName() const { return m_strPositionFromPropertyName; }
    const float3 &GetPositionOffset() const { return m_PositionOffset; }

    const float GetRange() const { return m_Range; }
    const String &GetRangeFromPropertyName() const { return m_strRangeFromPropertyName; }

    const float3 &GetColor() const { return m_Color; }
    const String &GetColorFromPropertyName() const { return m_strColorFromPropertyName; }

    const float GetBrightness() const { return m_Brightness; }
    const String &GetBrightnessFromPropertyName() const { return m_strBrightnessFromPropertyName; }

    const float GetFalloffExponent() const { return m_FalloffExponent; }
    const String &GetFalloffExponentFromPropertyName() const { return m_strFalloffExponentFromPropertyName; }

    const uint32 GetShadowFlags() const { return m_iShadowFlags; }
    const String &GetShadowFlagsFromPropertyName() const { return m_strShadowFlagsFromPropertyName; }

    const bool GetOnlyShowWhenSelected() const { return m_bOnlyShowWhenSelected; }

    virtual EditorVisualComponentDefinition *Clone() const;
    virtual bool ParseXML(XMLReader &xmlReader);

    virtual EditorVisualComponentInstance *CreateVisual(uint32 pickingID) const;

private:
    bool m_bEnabled;
    String m_strEnabledFromPropertyName;
    float3 m_Position;
    String m_strPositionFromPropertyName;
    float3 m_PositionOffset;
    float m_Range;
    String m_strRangeFromPropertyName;
    float3 m_Color;
    String m_strColorFromPropertyName;
    float m_Brightness;
    String m_strBrightnessFromPropertyName;
    float m_FalloffExponent;
    String m_strFalloffExponentFromPropertyName;
    uint32 m_iShadowFlags;
    String m_strShadowFlagsFromPropertyName;

    // tint colour
    bool m_bOnlyShowWhenSelected;
};

class EditorVisualComponentInstance_PointLight : public EditorVisualComponentInstance
{
public:
    EditorVisualComponentInstance_PointLight(uint32 pickingID, const EditorVisualComponentDefinition_PointLight *pDefinition);
    virtual ~EditorVisualComponentInstance_PointLight();

    void OnAddedToRenderWorld(RenderWorld *pRenderWorld) override;
    void OnRemovedFromRenderWorld(RenderWorld *pRenderWorld) override;
    void OnPropertyChange(const char *propertyName, const char *propertyValue) override;
    void OnSelectionStateChange(bool selected) override;

private:
    // recalculates the transform for an instanced visual
    void UpdateBounds();

    // definition
    const EditorVisualComponentDefinition_PointLight *m_pDefinition;

    // cached property values
    bool m_Enabled;
    float3 m_Position;
    float m_Range;
    float3 m_Color;
    float m_Brightness;
    float m_FalloffExponent;
    uint32 m_ShadowFlags;
    float3 m_EntityPosition;
    
    // render proxy
    PointLightRenderProxy *m_pRenderProxy;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class EditorVisualComponentDefinition_Sprite : public EditorVisualComponentDefinition
{
public:
    EditorVisualComponentDefinition_Sprite();
    virtual ~EditorVisualComponentDefinition_Sprite();

    const float3 &GetPosition() const { return m_Position; }
    const String &GetPositionFromPropertyName() const { return m_strPositionFromPropertyName; }
    const float3 &GetPositionOffset() const { return m_PositionOffset; }

    const float3 &GetScale() const { return m_Scale; }
    const String &GetScaleFromPropertyName() const { return m_strScaleFromPropertyName; }
    const float3 &GetScaleOffset() const { return m_ScaleOffset; }

    const uint32 GetShadowFlags() const { return m_iShadowFlags; }
    const String &GetShadowFlagsFromPropertyName() const { return m_strShadowFlagsFromPropertyName; }

    const bool GetVisible() const { return m_bVisible; }
    const String &GetVisibleFromPropertyName() const { return m_strVisibleFromPropertyName; }

    const String &GetTextureName() const { return m_strTextureName; }
    const String &GetTextureFromPropertyName() const { return m_strTextureFromPropertyName; }

    const MATERIAL_BLENDING_MODE GetBlendingMode() const { return m_eBlendingMode; }
    const String &GetBlendingModeFromPropertyName() const { return m_strBlendingModeFromPropertyName; }

    const float &GetWidth() const { return m_fWidth; }
    const String &GetWidthFromPropertyName() const { return m_strWidthFromPropertyName; }

    const float &GetHeight() const { return m_fHeight; }
    const String &GetHeightFromPropertyName() const { return m_strHeightFromPropertyName; }

    const uint32 GetTintColor() const { return m_TintColor; }
    const uint32 GetSelectedTintColor() const { return m_SelectedTintColor; }

    const bool GetOnlyShowWhenSelected() const { return m_bOnlyShowWhenSelected; }

    virtual EditorVisualComponentDefinition *Clone() const;
    virtual bool ParseXML(XMLReader &xmlReader);

    virtual EditorVisualComponentInstance *CreateVisual(uint32 pickingID) const;

private:
    float3 m_Position;
    String m_strPositionFromPropertyName;
    float3 m_PositionOffset;

    float3 m_Scale;
    String m_strScaleFromPropertyName;
    float3 m_ScaleOffset;

    uint32 m_iShadowFlags;
    String m_strShadowFlagsFromPropertyName;

    bool m_bVisible;
    String m_strVisibleFromPropertyName;

    // texture
    String m_strTextureName;
    String m_strTextureFromPropertyName;

    // blending
    MATERIAL_BLENDING_MODE m_eBlendingMode;
    String m_strBlendingModeFromPropertyName;

    // width
    float m_fWidth;
    String m_strWidthFromPropertyName;

    // height
    float m_fHeight;
    String m_strHeightFromPropertyName;

    // tint colour
    uint32 m_TintColor;
    uint32 m_SelectedTintColor;
    bool m_bOnlyShowWhenSelected;
};

class EditorVisualComponentInstance_Sprite : public EditorVisualComponentInstance
{
public:
    EditorVisualComponentInstance_Sprite(uint32 pickingID, const EditorVisualComponentDefinition_Sprite *pDefinition);
    virtual ~EditorVisualComponentInstance_Sprite();

    void OnAddedToRenderWorld(RenderWorld *pRenderWorld) override;
    void OnRemovedFromRenderWorld(RenderWorld *pRenderWorld) override;
    void OnPropertyChange(const char *propertyName, const char *propertyValue) override;
    void OnSelectionStateChange(bool selected) override;

private:
    // recalculates the transform for an instanced visual
    void UpdateBounds();

    // definition
    const EditorVisualComponentDefinition_Sprite *m_pDefinition;

    // cached property values
    const Texture2D *m_pTexture;
    MATERIAL_BLENDING_MODE m_eBlendingMode;
    float3 m_Position;
    float3 m_Scale;
    bool m_Visible;
    uint32 m_ShadowFlags;
    float m_fWidth, m_fHeight;

    // render proxy
    SpriteRenderProxy *m_pRenderProxy;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class EditorVisualComponentDefinition_StaticMesh : public EditorVisualComponentDefinition
{
public:
    EditorVisualComponentDefinition_StaticMesh();
    virtual ~EditorVisualComponentDefinition_StaticMesh();

    const float3 &GetPosition() const { return m_Position; }
    const String &GetPositionFromPropertyName() const { return m_strPositionFromPropertyName; }
    const float3 &GetPositionOffset() const { return m_PositionOffset; }

    const Quaternion &GetRotation() const { return m_Rotation; }
    const String &GetRotationFromPropertyName() const { return m_strRotationFromPropertyName; }
    const Quaternion &GetRotationOffset() const { return m_RotationOffset; }

    const float3 &GetScale() const { return m_Scale; }
    const String &GetScaleFromPropertyName() const { return m_strScaleFromPropertyName; }
    const float3 &GetScaleOffset() const { return m_ScaleOffset; }

    const uint32 GetShadowFlags() const { return m_iShadowFlags; }
    const String &GetShadowFlagsFromPropertyName() const { return m_strShadowFlagsFromPropertyName; }

    const bool GetVisible() const { return m_bVisible; }
    const String &GetVisibleFromPropertyName() const { return m_strVisibleFromPropertyName; }

    const String &GetMeshName() const { return m_strMeshName; }
    const String &GetMeshFromPropertyName() const { return m_strMeshFromPropertyName; }

    const uint32 GetTintColor() const { return m_TintColor; }
    const uint32 GetSelectedTintColor() const { return m_SelectedTintColor; }

    const bool GetOnlyShowWhenSelected() const { return m_bOnlyShowWhenSelected; }
    
    virtual EditorVisualComponentDefinition *Clone() const;
    virtual bool ParseXML(XMLReader &xmlReader);

    virtual EditorVisualComponentInstance *CreateVisual(uint32 pickingID) const;

private:
    float3 m_Position;
    String m_strPositionFromPropertyName;
    float3 m_PositionOffset;

    Quaternion m_Rotation;
    String m_strRotationFromPropertyName;
    Quaternion m_RotationOffset;

    float3 m_Scale;
    String m_strScaleFromPropertyName;
    float3 m_ScaleOffset;

    uint32 m_iShadowFlags;
    String m_strShadowFlagsFromPropertyName;

    bool m_bVisible;
    String m_strVisibleFromPropertyName;

    // mesh
    String m_strMeshName;
    String m_strMeshFromPropertyName;

    // tint colour
    uint32 m_TintColor;
    uint32 m_SelectedTintColor;
    bool m_bOnlyShowWhenSelected;
};

class EditorVisualComponentInstance_StaticMesh : public EditorVisualComponentInstance
{
public:
    EditorVisualComponentInstance_StaticMesh(uint32 pickingID, const EditorVisualComponentDefinition_StaticMesh *pDefinition);
    virtual ~EditorVisualComponentInstance_StaticMesh();

    void OnAddedToRenderWorld(RenderWorld *pRenderWorld) override;
    void OnRemovedFromRenderWorld(RenderWorld *pRenderWorld) override;
    void OnPropertyChange(const char *propertyName, const char *propertyValue) override;
    void OnSelectionStateChange(bool selected) override;

private:
    // recalculates the transform for an instanced visual
    void UpdateTransform();
    void UpdateBounds();
    
    // definition
    const EditorVisualComponentDefinition_StaticMesh *m_pDefinition;

    // cached property values
    const StaticMesh *m_pStaticMesh;
    float3 m_Position;
    Quaternion m_Rotation;
    float3 m_Scale;
    bool m_Visible;
    uint32 m_ShadowFlags;

    // cached transform
    Transform m_CachedTransform;

    // render proxy instance
    StaticMeshRenderProxy *m_pRenderProxy;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class EditorVisualComponentDefinition_BlockMesh : public EditorVisualComponentDefinition
{
public:
    EditorVisualComponentDefinition_BlockMesh();
    virtual ~EditorVisualComponentDefinition_BlockMesh();

    const float3 &GetPosition() const { return m_Position; }
    const String &GetPositionFromPropertyName() const { return m_strPositionFromPropertyName; }
    const float3 &GetPositionOffset() const { return m_PositionOffset; }

    const Quaternion &GetRotation() const { return m_Rotation; }
    const String &GetRotationFromPropertyName() const { return m_strRotationFromPropertyName; }
    const Quaternion &GetRotationOffset() const { return m_RotationOffset; }

    const float3 &GetScale() const { return m_Scale; }
    const String &GetScaleFromPropertyName() const { return m_strScaleFromPropertyName; }
    const float3 &GetScaleOffset() const { return m_ScaleOffset; }

    const uint32 GetShadowFlags() const { return m_iShadowFlags; }
    const String &GetShadowFlagsFromPropertyName() const { return m_strShadowFlagsFromPropertyName; }

    const bool GetVisible() const { return m_bVisible; }
    const String &GetVisibleFromPropertyName() const { return m_strVisibleFromPropertyName; }

    const String &GetMeshName() const { return m_strMeshName; }
    const String &GetMeshFromPropertyName() const { return m_strMeshFromPropertyName; }

    const uint32 GetTintColor() const { return m_TintColor; }
    const uint32 GetSelectedTintColor() const { return m_SelectedTintColor; }

    const bool GetOnlyShowWhenSelected() const { return m_bOnlyShowWhenSelected; }
    
    virtual EditorVisualComponentDefinition *Clone() const;
    virtual bool ParseXML(XMLReader &xmlReader);

    virtual EditorVisualComponentInstance *CreateVisual(uint32 pickingID) const;

private:
    float3 m_Position;
    String m_strPositionFromPropertyName;
    float3 m_PositionOffset;

    Quaternion m_Rotation;
    String m_strRotationFromPropertyName;
    Quaternion m_RotationOffset;

    float3 m_Scale;
    String m_strScaleFromPropertyName;
    float3 m_ScaleOffset;

    uint32 m_iShadowFlags;
    String m_strShadowFlagsFromPropertyName;

    bool m_bVisible;
    String m_strVisibleFromPropertyName;

    // mesh
    String m_strMeshName;
    String m_strMeshFromPropertyName;

    // tint colour
    uint32 m_TintColor;
    uint32 m_SelectedTintColor;
    bool m_bOnlyShowWhenSelected;
};

class EditorVisualComponentInstance_BlockMesh : public EditorVisualComponentInstance
{
public:
    EditorVisualComponentInstance_BlockMesh(uint32 pickingID, const EditorVisualComponentDefinition_BlockMesh *pDefinition);
    virtual ~EditorVisualComponentInstance_BlockMesh();

    void OnAddedToRenderWorld(RenderWorld *pRenderWorld) override;
    void OnRemovedFromRenderWorld(RenderWorld *pRenderWorld) override;
    void OnPropertyChange(const char *propertyName, const char *propertyValue) override;
    void OnSelectionStateChange(bool selected) override;

private:
    // recalculates the transform for an instanced visual
    void UpdateTransform();
    void UpdateBounds();
    
    // definition
    const EditorVisualComponentDefinition_BlockMesh *m_pDefinition;

    // cached property values
    const BlockMesh *m_pBlockMesh;
    float3 m_Position;
    Quaternion m_Rotation;
    float3 m_Scale;
    bool m_Visible;
    uint32 m_ShadowFlags;

    // cached transform
    Transform m_CachedTransform;

    // render proxy instance
    BlockMeshRenderProxy *m_pRenderProxy;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

