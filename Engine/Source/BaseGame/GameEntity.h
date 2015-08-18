#pragma once
#include "BaseGame/Common.h"
#include "Engine/Entity.h"

class GameEntity : public Entity
{
    DECLARE_ENTITY_TYPEINFO(GameEntity, Entity);
    DECLARE_ENTITY_NO_FACTORY(Entity);

public:
    GameEntity(const EntityTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~GameEntity();

    const bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible);

    const float GetOpacity() const { return m_opacity; }
    void SetOpacity(float opacity);

    const bool IsTinted() const { return (m_tintColor != 0xFFFFFFFF); }
    const uint32 GetTintColor() const { return m_tintColor; }
    void SetTintColor(uint32 tintColor);

    void FadeIn(float duration = 1.0f);
    void FadeOut(float duration = 1.0f);

    virtual void Update(float timeSinceLastUpdate);

protected:
    // callbacks
    virtual void OnVisibilityChanged();
    virtual void OnOpacityChanged();
    virtual void OnTintColorChanged();

    // rendering variables
    bool m_visible;
    float m_opacity;
    uint32 m_tintColor;

private:
    // fade in/out
    float m_fadeDuration;
    float m_fadeStartValue;
    float m_fadeTimeRemaining;

private:
    // hidden trampolines for property system
    static void __PS_OnVisibilityChanged(GameEntity *pThis, const void *pUserData) { pThis->OnVisibilityChanged(); }
    static void __PS_OnOpacityChanged(GameEntity *pThis, const void *pUserData) { pThis->OnOpacityChanged(); }
    static void __PS_OnTintColorChanged(GameEntity *pThis, const void *pUserData) { pThis->OnTintColorChanged(); }
};

