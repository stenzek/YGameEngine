#include "BaseGame/PrecompiledHeader.h"
#include "BaseGame/GameEntity.h"

DEFINE_ENTITY_TYPEINFO(GameEntity, 0);
BEGIN_ENTITY_PROPERTIES(GameEntity)
    PROPERTY_TABLE_MEMBER_BOOL("Visible", 0, offsetof(GameEntity, m_visible), __PS_OnVisibilityChanged, nullptr)
    PROPERTY_TABLE_MEMBER_FLOAT("Opacity", 0, offsetof(GameEntity, m_opacity), __PS_OnOpacityChanged, nullptr)
    PROPERTY_TABLE_MEMBER_COLOR("TintColor", 0, offsetof(GameEntity, m_tintColor), __PS_OnTintColorChanged, nullptr)
END_ENTITY_PROPERTIES()
BEGIN_ENTITY_SCRIPT_FUNCTIONS(GameEntity)
END_ENTITY_SCRIPT_FUNCTIONS()

GameEntity::GameEntity(const EntityTypeInfo *pTypeInfo /*= &s_typeInfo*/)
    : BaseClass(pTypeInfo),
      m_visible(true),
      m_opacity(1.0f),
      m_tintColor(0xFFFFFFFF),
      m_fadeDuration(0.0f),
      m_fadeStartValue(0.0f),
      m_fadeTimeRemaining(0.0f)
{

}

GameEntity::~GameEntity()
{

}

void GameEntity::SetVisible(bool visible)
{
    if (visible == m_visible)
        return;

    m_visible = visible;
    OnVisibilityChanged();
}

void GameEntity::OnVisibilityChanged()
{

}

void GameEntity::SetOpacity(float opacity)
{
    DebugAssert(opacity >= 0.0f);
    if (opacity == m_opacity)
        return;

    m_opacity = opacity;
    SetTintColor((m_tintColor & 0x00FFFFFF) | ((uint32)Math::Clamp(m_opacity * 255.0f, 0.0f, 255.0f) << 24));
    OnOpacityChanged();
}

void GameEntity::OnOpacityChanged()
{

}

void GameEntity::SetTintColor(uint32 tintColor)
{
    m_tintColor = tintColor;
    OnTintColorChanged();
}

void GameEntity::OnTintColorChanged()
{

}

void GameEntity::FadeIn(float duration /* = 1.0f */)
{
    if (m_fadeDuration == 0.0f)
        RegisterForUpdates();

    m_fadeDuration = duration;
    m_fadeTimeRemaining = duration;
    m_fadeStartValue = m_opacity;
}

void GameEntity::FadeOut(float duration /* = 1.0f */)
{
    if (m_fadeDuration == 0.0f)
        RegisterForUpdates();

    m_fadeDuration = duration;
    m_fadeTimeRemaining = duration;
    if (m_opacity == 0.0f)
        m_fadeStartValue = -Y_FLT_EPSILON;
    else
        m_fadeStartValue = -m_opacity;
}

void GameEntity::Update(float timeSinceLastUpdate)
{
    BaseClass::Update(timeSinceLastUpdate);

    if (m_fadeTimeRemaining > 0.0f)
    {
        m_fadeTimeRemaining -= timeSinceLastUpdate;
        if (m_fadeTimeRemaining <= 0.0f)
            m_fadeTimeRemaining = 0.0f;

        // some trickery to hide the direction in the starting value
        float start = Math::Abs(m_fadeStartValue);
        float end = (m_fadeStartValue < 0.0f) ? 0.0f : 1.0f;
        float opacity = Math::Lerp(end, start, m_fadeTimeRemaining / m_fadeDuration);
        SetOpacity(opacity);

        // end of it?
        if (m_fadeTimeRemaining == 0.0f)
        {
            m_fadeDuration = 0.0f;
            m_fadeStartValue = 0.0f;
            UnregisterForUpdates();
        }
    }
}
